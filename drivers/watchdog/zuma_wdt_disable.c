// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 Will
 */

#include <linux/bits.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>

#define S3C2410_WTCON			0x00
#define S3C2410_WTCON_RSTEN		BIT(0)
#define S3C2410_WTCON_INTEN		BIT(2)
#define S3C2410_WTCON_ENABLE		BIT(5)

static int zuma_wdt_disable_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 wtcon;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	wtcon = readl(reg + S3C2410_WTCON);
	wtcon &= ~(S3C2410_WTCON_ENABLE | S3C2410_WTCON_RSTEN | S3C2410_WTCON_INTEN);
	writel(wtcon, reg + S3C2410_WTCON);

	dev_info(&pdev->dev, "watchdog disabled, WTCON=0x%08x\n",
		 readl(reg + S3C2410_WTCON));

	return 0;
}

static const struct of_device_id zuma_wdt_disable_match[] = {
	{ .compatible = "google,zuma-cl0-wdt" },
	{ .compatible = "google,zuma-cl1-wdt" },
	{}
};
MODULE_DEVICE_TABLE(of, zuma_wdt_disable_match);

static struct platform_driver zuma_wdt_disable_driver = {
	.probe = zuma_wdt_disable_probe,
	.driver = {
		.name = "zuma-wdt-disable",
		.of_match_table = zuma_wdt_disable_match,
	},
};
module_platform_driver(zuma_wdt_disable_driver);

MODULE_DESCRIPTION("Google Zuma/Zumapro watchdog disable shim");
MODULE_LICENSE("GPL");
