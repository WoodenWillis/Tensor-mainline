// SPDX-License-Identifier: GPL-2.0-only
/*
 * PCIe root complex driver for Google Tensor G4 (zumapro), HSI2 channel.
 *
 * Copyright 2026 Will
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/iopoll.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "pcie-designware.h"

#define PCIE_DEVICE_TYPE		0x0080
#define   DEVICE_TYPE_RC		0x4
#define PCIE_APP_LTSSM_ENABLE		0x0054
#define   LTSSM_ENABLE			0x1
#define PCIE_L1_EXIT_OFF		0x1078
#define PCIE_SOFT_PWR_RST		0x03a4
#define PCIE_SLV_PEND_SEL_NAK		0x03d8
#define PCIE_APP_XFER_PENDING		0x0074
#define PCIE_LINKDOWN_RST_CTRL_SEL	0x03a0
#define   LINKDOWN_RST_MANUAL		BIT(1)
#define PCIE_QCH_SEL			0x03a8
#define   CLOCK_GATING_MASK		GENMASK(11, 0)
#define PCIE_APP_REQ_EXIT_L1_MODE	0x03bc
#define   APP_REQ_EXIT_L1_MODE		BIT(0)
#define   L1_REQ_NAK_CONTROL_MASTER	BIT(4)
#define PCIE_MSTR_PEND_SEL_NAK		0x0474
#define   NACK_ENABLE			0x1
#define PCIE_PMA_RST_CTRL0		0x1400
#define PCIE_PMA_RST_CTRL1		0x1404
#define PCIE_PMA_RST_CTRL2		0x1408
#define PCIE_ELBI_RDLH_LINKUP		0x02c8
#define   LTSSM_STATE_MASK		0x3f
#define   S_RCVRY_LOCK			0x0d
#define   S_L0				0x11
#define   S_L1_IDLE			0x14

#define PCIE_SOC_CTRL			0x4004
#define   SOC_CTRL_PHY_CLK_SEL_EXTPLL	(0x3 << 4)
#define   SOC_CTRL_PHY_CLK_TCXO		BIT(2)

#define PCIE_UDBG_EXTPLL		0xc700
#define   EXTPLL_INIT			BIT(0)
#define   EXTPLL_RESETB			BIT(1)
#define PCIE_UDBG_EXTPLL_LOCK		0xc734
#define   EXTPLL_LOCK_BIT		2

#define PCIE_PHY_CLK_GATE		0x032c
#define PCIE_PHY_PLL_LOCK		0x0a80
#define PCIE_PHY_CDR_LOCK		0x15c0
#define   CDR_LOCK_BIT			4
#define PCIE_PHY_OC_DONE		0x140c
#define   OC_DONE_BIT			7

#define PCIE_PHY_CONTROL_MASK		0x1
#define PCIE_PHY_BYPASS			0x1

#define PCIE_PCS_PM_POWER_STATE		0x188

#define LINK_WAIT_US			10
#define LINK_WAIT_CNT			10000

#define WLAN_POWER_ON_US		10000

struct zumapro_pcie {
	struct dw_pcie		pci;
	void __iomem		*soc;
	void __iomem		*udbg;
	void __iomem		*phy;
	void __iomem		*pcs;
	struct clk_bulk_data	*clks;
	int			num_clks;
	struct regmap		*pmu;
	u32			pmu_offset;
	struct gpio_desc	*perst;
	struct gpio_desc	*wlan;
	u32			perst_delay_us;
};

#define to_zumapro_pcie(x)	container_of((x), struct zumapro_pcie, pci)

static bool zumapro_pcie_poll_bit(void __iomem *base, u32 reg, int bit)
{
	u32 val;

	return !readl_poll_timeout(base + reg, val, val & BIT(bit),
				   LINK_WAIT_US, LINK_WAIT_US * LINK_WAIT_CNT);
}

static void zumapro_pcie_phy_config(struct zumapro_pcie *zp)
{
	void __iomem *elbi = zp->pci.elbi_base;
	void __iomem *soc = zp->soc;
	void __iomem *udbg = zp->udbg;
	void __iomem *phy = zp->phy;
	void __iomem *pcs = zp->pcs;
	struct device *dev = zp->pci.dev;
	bool extpll, pll, cdr, oc;
	u32 val;

	writel(0x3, phy + PCIE_PHY_CLK_GATE);

	val = readl(udbg + PCIE_UDBG_EXTPLL) | EXTPLL_INIT;
	writel(val, udbg + PCIE_UDBG_EXTPLL);
	val = readl(udbg + PCIE_UDBG_EXTPLL) & ~EXTPLL_RESETB;
	writel(val, udbg + PCIE_UDBG_EXTPLL);
	extpll = zumapro_pcie_poll_bit(udbg, PCIE_UDBG_EXTPLL_LOCK, EXTPLL_LOCK_BIT);

	val = readl(soc + PCIE_SOC_CTRL) & ~SOC_CTRL_PHY_CLK_TCXO;
	writel(val, soc + PCIE_SOC_CTRL);
	val = readl(soc + PCIE_SOC_CTRL) | SOC_CTRL_PHY_CLK_SEL_EXTPLL;
	writel(val, soc + PCIE_SOC_CTRL);

	writel(DEVICE_TYPE_RC, elbi + PCIE_DEVICE_TYPE);

	writel(0xf, elbi + PCIE_SOFT_PWR_RST);
	writel(0xd, elbi + PCIE_SOFT_PWR_RST);
	udelay(10);
	writel(0xf, elbi + PCIE_SOFT_PWR_RST);
	udelay(10);

	writel(0x1, elbi + PCIE_PMA_RST_CTRL1);
	writel(0x1, elbi + PCIE_PMA_RST_CTRL2);
	writel(0x1, elbi + PCIE_PMA_RST_CTRL0);
	writel(0x0, elbi + PCIE_PMA_RST_CTRL1);
	writel(0x0, elbi + PCIE_PMA_RST_CTRL2);
	writel(0x0, elbi + PCIE_PMA_RST_CTRL0);

	writel(0x1, elbi + PCIE_SLV_PEND_SEL_NAK);

	writel(0x88, phy + 0x0000);
	writel(0x66, phy + 0x001c);
	writel(0x00, phy + 0x01f4);
	writel(0x59, phy + 0x0514);
	writel(0x11, phy + 0x051c);
	writel(0x0e, phy + 0x062c);
	writel(0x22, phy + 0x0644);
	writel(0x03, phy + 0x0688);
	writel(0x28, phy + 0x06d4);
	writel(0x64, phy + 0x0788);
	writel(0x64, phy + 0x078c);
	writel(0x50, phy + 0x0790);
	writel(0x50, phy + 0x0794);
	writel(0x05, phy + 0x0944);
	writel(0x05, phy + 0x0948);
	writel(0x05, phy + 0x094c);
	writel(0x05, phy + 0x0950);

	writel(0x02, phy + 0x0590);
	writel(0xb0, phy + 0x07f8);
	writel(0x08, phy + 0x0730);

	writel(0xc0, phy + 0x0344);

	writel(0x04, phy + 0x0040);
	writel(0x03, phy + 0x0204);
	writel(0x1f, phy + 0x02d4);
	writel(0x10, phy + 0x0358);

	writel(0x01, phy + 0x0018);

	writel(0x5b, phy + 0x0514);
	writel(0x0c, phy + 0x0608);
	writel(0x0f, phy + 0x060c);
	writel(0x0f, phy + 0x0610);
	writel(0x0f, phy + 0x0614);
	writel(0x0f, phy + 0x0618);
	writel(0x80, phy + 0x0510);
	writel(0x03, phy + 0x0688);
	writel(0x23, phy + 0x0644);
	writel(0x11, phy + 0x0624);

	writel(0x0f, phy + 0x0630);
	writel(0x53, phy + 0x06d0);

	writel(0x04, phy + 0x1140);
	writel(0x04, phy + 0x1144);
	writel(0x04, phy + 0x1148);
	writel(0x02, phy + 0x114c);
	writel(0x00, phy + 0x1150);
	writel(0x00, phy + 0x1154);
	writel(0x00, phy + 0x1158);
	writel(0x00, phy + 0x115c);
	writel(0x1c, phy + 0x12cc);
	writel(0x6c, phy + 0x12dc);
	writel(0x29, phy + 0x130c);
	writel(0x2f, phy + 0x13b4);
	writel(0x05, phy + 0x1a64);
	writel(0x05, phy + 0x1a68);
	writel(0x05, phy + 0x1a84);
	writel(0x05, phy + 0x1a88);
	writel(0x00, phy + 0x1a98);
	writel(0x00, phy + 0x1a9c);
	writel(0x07, phy + 0x1aa8);
	writel(0x00, phy + 0x1ab8);
	writel(0x00, phy + 0x1abc);
	writel(0x90, phy + 0x1af8);
	writel(0x03, phy + 0x1b34);
	writel(0x03, phy + 0x1bb0);
	writel(0x03, phy + 0x1bb4);
	writel(0x06, phy + 0x1bc0);
	writel(0x06, phy + 0x1bc4);
	writel(0x01, phy + 0x1be8);
	writel(0x04, phy + 0x1bf8);
	writel(0x00, phy + 0x1c98);
	writel(0x81, phy + 0x1ca4);

	writel(0x02, phy + 0x1b20);
	writel(0x23, phy + 0x1340);
	writel(0x3c, phy + 0x002c);

	val = readl(phy + 0x1350) | 0x1;
	writel(val, phy + 0x1350);

	writel(0x100b0604, pcs + 0x190);
	writel(0x000700d5, pcs + 0x154);
	writel(0x16400000, pcs + 0x100);
	writel(0x08600000, pcs + 0x104);
	writel(0x18500000, pcs + 0x114);
	writel(0x60700000, pcs + 0x124);
	writel(0x00000007, pcs + 0x174);
	writel(0x00000100, pcs + 0x178);
	writel(0x00000700, pcs + 0x17c);
	writel(0x01600202, pcs + 0x110);

	writel(0x1, elbi + PCIE_PMA_RST_CTRL1);
	udelay(10);
	writel(0x1, elbi + PCIE_PMA_RST_CTRL2);
	writel(0x1, elbi + PCIE_PMA_RST_CTRL0);

	pll = zumapro_pcie_poll_bit(phy, PCIE_PHY_PLL_LOCK, 0);
	cdr = zumapro_pcie_poll_bit(phy, PCIE_PHY_CDR_LOCK, CDR_LOCK_BIT);
	oc = zumapro_pcie_poll_bit(phy, PCIE_PHY_OC_DONE, OC_DONE_BIT);

	dev_info(dev, "phy locks: extpll=%d pll=%d cdr=%d oc=%d\n",
		 extpll, pll, cdr, oc);

	writel(0x0, phy + PCIE_PHY_CLK_GATE);
	writel(0x1, elbi + PCIE_L1_EXIT_OFF);
}

static int zumapro_pcie_host_init(struct dw_pcie_rp *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct zumapro_pcie *zp = to_zumapro_pcie(pci);
	int ret;

	ret = clk_bulk_prepare_enable(zp->num_clks, zp->clks);
	if (ret)
		return ret;

	regmap_update_bits(zp->pmu, zp->pmu_offset, PCIE_PHY_CONTROL_MASK,
			   PCIE_PHY_BYPASS);

	gpiod_set_value_cansleep(zp->wlan, 1);
	usleep_range(WLAN_POWER_ON_US, WLAN_POWER_ON_US + 1000);

	zumapro_pcie_phy_config(zp);

	gpiod_set_value_cansleep(zp->perst, 1);
	usleep_range(zp->perst_delay_us, zp->perst_delay_us + 2000);

	return 0;
}

static const struct dw_pcie_host_ops zumapro_pcie_host_ops = {
	.init = zumapro_pcie_host_init,
};

static int zumapro_pcie_start_link(struct dw_pcie *pci)
{
	struct zumapro_pcie *zp = to_zumapro_pcie(pci);
	void __iomem *elbi = pci->elbi_base;
	u32 val;
	int i;

	val = readl(elbi + PCIE_APP_REQ_EXIT_L1_MODE);
	val |= APP_REQ_EXIT_L1_MODE | L1_REQ_NAK_CONTROL_MASTER;
	writel(val, elbi + PCIE_APP_REQ_EXIT_L1_MODE);

	writel(LINKDOWN_RST_MANUAL, elbi + PCIE_LINKDOWN_RST_CTRL_SEL);
	writel(0x1, elbi + PCIE_APP_XFER_PENDING);

	val = readl(elbi + PCIE_QCH_SEL) & ~CLOCK_GATING_MASK;
	writel(val, elbi + PCIE_QCH_SEL);

	writel(NACK_ENABLE, elbi + PCIE_MSTR_PEND_SEL_NAK);

	writel(LTSSM_ENABLE, elbi + PCIE_APP_LTSSM_ENABLE);

	for (i = 0; i < 30; i++) {
		usleep_range(1000, 1100);
		val = readl(elbi + PCIE_ELBI_RDLH_LINKUP) & LTSSM_STATE_MASK;
		dev_info(pci->dev, "ltssm[%d] = 0x%02x\n", i, val);
	}

	dev_info(pci->dev, "post: ltssm=0x%02x dbg1=0x%08x pcs_pm=0x%08x\n",
		 readl(elbi + PCIE_ELBI_RDLH_LINKUP) & LTSSM_STATE_MASK,
		 dw_pcie_readl_dbi(pci, PCIE_PORT_DEBUG1),
		 readl(zp->pcs + PCIE_PCS_PM_POWER_STATE));

	return 0;
}

static void zumapro_pcie_stop_link(struct dw_pcie *pci)
{
	struct zumapro_pcie *zp = to_zumapro_pcie(pci);

	writel(0x0, zp->pci.elbi_base + PCIE_APP_LTSSM_ENABLE);
}

static bool zumapro_pcie_link_up(struct dw_pcie *pci)
{
	struct zumapro_pcie *zp = to_zumapro_pcie(pci);
	u32 val;

	val = readl(zp->pci.elbi_base + PCIE_ELBI_RDLH_LINKUP) & LTSSM_STATE_MASK;

	return val >= S_RCVRY_LOCK && val <= S_L1_IDLE;
}

static const struct dw_pcie_ops zumapro_dw_pcie_ops = {
	.link_up	= zumapro_pcie_link_up,
	.start_link	= zumapro_pcie_start_link,
	.stop_link	= zumapro_pcie_stop_link,
};

static void __iomem *zumapro_pcie_map(struct platform_device *pdev,
				      const char *name)
{
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);

	return devm_ioremap_resource(&pdev->dev, res);
}

static int zumapro_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct zumapro_pcie *zp;

	zp = devm_kzalloc(dev, sizeof(*zp), GFP_KERNEL);
	if (!zp)
		return -ENOMEM;

	zp->pci.dev = dev;
	zp->pci.ops = &zumapro_dw_pcie_ops;
	zp->pci.pp.ops = &zumapro_pcie_host_ops;

	zp->soc = zumapro_pcie_map(pdev, "soc");
	if (IS_ERR(zp->soc))
		return PTR_ERR(zp->soc);
	zp->udbg = zumapro_pcie_map(pdev, "udbg");
	if (IS_ERR(zp->udbg))
		return PTR_ERR(zp->udbg);
	zp->phy = zumapro_pcie_map(pdev, "phy");
	if (IS_ERR(zp->phy))
		return PTR_ERR(zp->phy);
	zp->pcs = zumapro_pcie_map(pdev, "pcs");
	if (IS_ERR(zp->pcs))
		return PTR_ERR(zp->pcs);

	zp->num_clks = devm_clk_bulk_get_all(dev, &zp->clks);
	if (zp->num_clks < 0)
		return zp->num_clks;

	zp->pmu = syscon_regmap_lookup_by_phandle(np, "samsung,pmu-syscon");
	if (IS_ERR(zp->pmu))
		return PTR_ERR(zp->pmu);
	if (of_property_read_u32(np, "samsung,pmu-offset", &zp->pmu_offset))
		return -EINVAL;

	zp->perst = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(zp->perst))
		return PTR_ERR(zp->perst);
	zp->wlan = devm_gpiod_get_optional(dev, "wlan-reg-on", GPIOD_OUT_LOW);
	if (IS_ERR(zp->wlan))
		return PTR_ERR(zp->wlan);

	if (of_property_read_u32(np, "samsung,perst-delay-us",
				 &zp->perst_delay_us))
		zp->perst_delay_us = 15000;

	platform_set_drvdata(pdev, zp);

	return dw_pcie_host_init(&zp->pci.pp);
}

static const struct of_device_id zumapro_pcie_of_match[] = {
	{ .compatible = "google,zumapro-pcie" },
	{ }
};
MODULE_DEVICE_TABLE(of, zumapro_pcie_of_match);

static struct platform_driver zumapro_pcie_driver = {
	.probe	= zumapro_pcie_probe,
	.driver	= {
		.name		= "pcie-zumapro",
		.of_match_table	= zumapro_pcie_of_match,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(zumapro_pcie_driver);

MODULE_DESCRIPTION("Google Tensor G4 (zumapro) PCIe RC driver");
MODULE_LICENSE("GPL");
