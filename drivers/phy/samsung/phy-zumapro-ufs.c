// SPDX-License-Identifier: GPL-2.0-only

#include <linux/iopoll.h>

#include "phy-samsung-ufs.h"

#define TENSOR_ZUMAPRO_PHY_CTRL		0x3ec0
#define TENSOR_ZUMAPRO_PHY_CTRL_MASK	0x1
#define TENSOR_ZUMAPRO_PHY_CTRL_EN	BIT(0)
#define PHY_ZUMAPRO_LANE_OFFSET		0x200
#define TRSV_REG31D			0x31d
#define LN0_MON_RX_CAL_DONE		BIT(0)

#define PHY_PMA_TRSV_ADDR(reg, lane)	(PHY_APB_ADDR((reg) + \
					((lane) * PHY_ZUMAPRO_LANE_OFFSET)))

#define PHY_TRSV_REG_CFG_ZUMAPRO(o, v, d) \
	PHY_TRSV_REG_CFG_OFFSET(o, v, d, PHY_ZUMAPRO_LANE_OFFSET)

static const struct samsung_ufs_phy_cfg tensor_zumapro_pre_init_cfg[] = {
	PHY_COMN_REG_CFG(0x50, 0x08, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x05, 0x19, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x0B, 0x44, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x0C, 0xc4, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x0D, 0xc3, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x0F, 0x88, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x16, 0x1a, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x19, 0x04, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x54, 0x88, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x67, 0x4c, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x68, 0x4c, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x201, 0x44, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x202, 0x44, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x203, 0x00, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x204, 0x18, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x205, 0xc0, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x207, 0x1c, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x2EC, 0x8c, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x27C, 0xd0, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x288, 0xfa, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x289, 0x60, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x234, 0x30, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x239, 0x05, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x23D, 0x05, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x24D, 0x1a, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x24E, 0x12, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x24F, 0x5e, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x259, 0x2a, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x260, 0x54, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x266, 0x54, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x273, 0x00, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x274, 0x00, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x2AB, 0x00, PWR_MODE_ANY),
	PHY_TRSV_REG_CFG_ZUMAPRO(0x2AC, 0x02, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x50, 0x0c, PWR_MODE_ANY),
	PHY_COMN_REG_CFG(0x50, 0x00, PWR_MODE_ANY),
	END_UFS_PHY_CFG,
};

static const struct samsung_ufs_phy_cfg *tensor_zumapro_ufs_phy_cfgs[CFG_TAG_MAX] = {
	[CFG_PRE_INIT]		= tensor_zumapro_pre_init_cfg,
};

static const char * const tensor_zumapro_ufs_phy_clks[] = {
	"ref_clk",
};

static int zumapro_phy_wait_for_calibration(struct phy *phy, u8 lane)
{
	struct samsung_ufs_phy *ufs_phy = get_samsung_ufs_phy(phy);
	const unsigned int timeout_us = 4000;
	const unsigned int sleep_us = 40;
	u32 val;
	int err;

	err = readl_poll_timeout(ufs_phy->reg_pma +
				 PHY_PMA_TRSV_ADDR(TRSV_REG31D, lane),
				 val, (val & LN0_MON_RX_CAL_DONE),
				 sleep_us, timeout_us);
	if (err)
		dev_warn(ufs_phy->dev, "lane %u: no phy cal done\n", lane);

	return 0;
}

/* TODO(will): port zuma post_h8_enter/pre_h8_exit (needs a 10us wait row) */
const struct samsung_ufs_phy_drvdata tensor_zumapro_ufs_phy = {
	.cfgs = tensor_zumapro_ufs_phy_cfgs,
	.isol = {
		.offset = TENSOR_ZUMAPRO_PHY_CTRL,
		.mask = TENSOR_ZUMAPRO_PHY_CTRL_MASK,
		.en = TENSOR_ZUMAPRO_PHY_CTRL_EN,
	},
	.clk_list = tensor_zumapro_ufs_phy_clks,
	.num_clks = ARRAY_SIZE(tensor_zumapro_ufs_phy_clks),
	.wait_for_cal = zumapro_phy_wait_for_calibration,
};
