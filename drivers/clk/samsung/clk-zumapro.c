// SPDX-License-Identifier: GPL-2.0-only
/*
 * Google Tensor G4 (zumapro) clock controller
 *
 * Copyright 2026 Will
 *
 * Common Clock Framework support for CMU_TOP and CMU_MISC.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <dt-bindings/clock/google,zumapro.h>

#include "clk.h"
#include "clk-exynos-arm64.h"

/* ---- CMU_TOP ------------------------------------------------------------- */

/* Register Offset definitions for CMU_TOP (0x26040000) */
#define PLL_LOCKTIME_PLL_SHARED0		0x0004
#define PLL_LOCKTIME_PLL_SHARED1		0x0008
#define PLL_LOCKTIME_PLL_SHARED2		0x000c
#define PLL_LOCKTIME_PLL_SHARED3		0x0010
#define PLL_LOCKTIME_PLL_SPARE			0x0014
#define PLL_CON3_PLL_SHARED0			0x014c
#define PLL_CON3_PLL_SHARED1			0x018c
#define PLL_CON3_PLL_SHARED2			0x01cc
#define PLL_CON3_PLL_SHARED3			0x020c
#define PLL_CON3_PLL_SPARE			0x024c
#define CLK_CON_MUX_MUX_CLKCMU_MISC_NOC		0x10d0
#define CLK_CON_DIV_CLKCMU_MISC_NOC		0x18c4
#define CLK_CON_GAT_GATE_CLKCMU_MISC_NOC	0x20f4

static const unsigned long top_clk_regs[] __initconst = {
	PLL_LOCKTIME_PLL_SHARED0,
	PLL_LOCKTIME_PLL_SHARED1,
	PLL_LOCKTIME_PLL_SHARED2,
	PLL_LOCKTIME_PLL_SHARED3,
	PLL_LOCKTIME_PLL_SPARE,
	PLL_CON3_PLL_SHARED0,
	PLL_CON3_PLL_SHARED1,
	PLL_CON3_PLL_SHARED2,
	PLL_CON3_PLL_SHARED3,
	PLL_CON3_PLL_SPARE,
	CLK_CON_MUX_MUX_CLKCMU_MISC_NOC,
	CLK_CON_DIV_CLKCMU_MISC_NOC,
	CLK_CON_GAT_GATE_CLKCMU_MISC_NOC,
};

static const struct samsung_pll_clock top_pll_clks[] __initconst = {
	PLL(pll_0517x, CLK_FOUT_SHARED0_PLL, "fout_shared0_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED0, PLL_CON3_PLL_SHARED0, NULL),
	PLL(pll_0517x, CLK_FOUT_SHARED1_PLL, "fout_shared1_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED1, PLL_CON3_PLL_SHARED1, NULL),
	PLL(pll_0518x, CLK_FOUT_SHARED2_PLL, "fout_shared2_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED2, PLL_CON3_PLL_SHARED2, NULL),
	PLL(pll_0518x, CLK_FOUT_SHARED3_PLL, "fout_shared3_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED3, PLL_CON3_PLL_SHARED3, NULL),
	PLL(pll_0518x, CLK_FOUT_SPARE_PLL, "fout_spare_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SPARE, PLL_CON3_PLL_SPARE, NULL),
};

PNAME(mout_cmu_misc_noc_p) = { "dout_shared0_div4", "dout_shared2_div2",
			      "dout_shared3_div2", "fout_spare_pll" };

static const struct samsung_fixed_factor_clock top_fixed_factor_clks[] __initconst = {
	FFACTOR(CLK_DOUT_SHARED0_DIV4, "dout_shared0_div4", "fout_shared0_pll", 1, 4, 0),
	FFACTOR(CLK_DOUT_SHARED2_DIV2, "dout_shared2_div2", "fout_shared2_pll", 1, 2, 0),
	FFACTOR(CLK_DOUT_SHARED3_DIV2, "dout_shared3_div2", "fout_shared3_pll", 1, 2, 0),
};

static const struct samsung_mux_clock top_mux_clks[] __initconst = {
	MUX(CLK_MOUT_CMU_MISC_NOC, "mout_cmu_misc_noc", mout_cmu_misc_noc_p,
	    CLK_CON_MUX_MUX_CLKCMU_MISC_NOC, 0, 2),
};

static const struct samsung_div_clock top_div_clks[] __initconst = {
	DIV(CLK_DOUT_CMU_MISC_NOC, "dout_cmu_misc_noc", "mout_cmu_misc_noc",
	    CLK_CON_DIV_CLKCMU_MISC_NOC, 0, 4),
};

static const struct samsung_gate_clock top_gate_clks[] __initconst = {
	GATE(CLK_GOUT_CMU_MISC_NOC, "gout_cmu_misc_noc", "dout_cmu_misc_noc",
	     CLK_CON_GAT_GATE_CLKCMU_MISC_NOC, 21, CLK_IGNORE_UNUSED, 0),
};

static const struct samsung_cmu_info top_cmu_info __initconst = {
	.pll_clks		= top_pll_clks,
	.nr_pll_clks		= ARRAY_SIZE(top_pll_clks),
	.mux_clks		= top_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(top_mux_clks),
	.div_clks		= top_div_clks,
	.nr_div_clks		= ARRAY_SIZE(top_div_clks),
	.gate_clks		= top_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(top_gate_clks),
	.fixed_factor_clks	= top_fixed_factor_clks,
	.nr_fixed_factor_clks	= ARRAY_SIZE(top_fixed_factor_clks),
	.nr_clk_ids		= CLKS_NR_TOP,
	.clk_regs		= top_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(top_clk_regs),
};

static void __init zumapro_cmu_top_init(struct device_node *np)
{
	exynos_arm64_register_cmu(NULL, np, &top_cmu_info);
}

/* CMU_TOP is a dependency of all other CMUs, register it early. */
CLK_OF_DECLARE(zumapro_cmu_top, "google,zumapro-cmu-top", zumapro_cmu_top_init);

/* ---- CMU_MISC ------------------------------------------------------------ */

/* Register Offset definitions for CMU_MISC (0x10010000) */
#define PLL_CON0_MUX_CLKCMU_MISC_NOC_USER	0x0600
#define CLK_CON_DIV_DIV_CLK_MISC_NOCP		0x1808
#define CLK_CON_GAT_GOUT_BLK_MISC_UID_MCT_IPCLKPORT_PCLK		0x20c8
#define CLK_CON_GAT_GOUT_BLK_MISC_UID_WDT_CLUSTER0_IPCLKPORT_PCLK	0x2164
#define CLK_CON_GAT_GOUT_BLK_MISC_UID_WDT_CLUSTER1_IPCLKPORT_PCLK	0x2168

static const unsigned long misc_clk_regs[] __initconst = {
	PLL_CON0_MUX_CLKCMU_MISC_NOC_USER,
	CLK_CON_DIV_DIV_CLK_MISC_NOCP,
	CLK_CON_GAT_GOUT_BLK_MISC_UID_MCT_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_MISC_UID_WDT_CLUSTER0_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_MISC_UID_WDT_CLUSTER1_IPCLKPORT_PCLK,
};

PNAME(mout_misc_noc_user_p) = { "oscclk", "dout_cmu_misc_noc" };

static const struct samsung_mux_clock misc_mux_clks[] __initconst = {
	MUX(CLK_MOUT_MISC_NOC_USER, "mout_misc_noc_user", mout_misc_noc_user_p,
	    PLL_CON0_MUX_CLKCMU_MISC_NOC_USER, 4, 1),
};

static const struct samsung_div_clock misc_div_clks[] __initconst = {
	DIV(CLK_DOUT_MISC_NOCP, "dout_misc_nocp", "mout_misc_noc_user",
	    CLK_CON_DIV_DIV_CLK_MISC_NOCP, 0, 3),
};

static const struct samsung_gate_clock misc_gate_clks[] __initconst = {
	GATE(CLK_GOUT_MISC_MCT_PCLK, "gout_misc_mct_pclk", "dout_misc_nocp",
	     CLK_CON_GAT_GOUT_BLK_MISC_UID_MCT_IPCLKPORT_PCLK, 21, 0, 0),
	GATE(CLK_GOUT_MISC_WDT_CL0_PCLK, "gout_misc_wdt_cl0_pclk", "dout_misc_nocp",
	     CLK_CON_GAT_GOUT_BLK_MISC_UID_WDT_CLUSTER0_IPCLKPORT_PCLK, 21, 0, 0),
	GATE(CLK_GOUT_MISC_WDT_CL1_PCLK, "gout_misc_wdt_cl1_pclk", "dout_misc_nocp",
	     CLK_CON_GAT_GOUT_BLK_MISC_UID_WDT_CLUSTER1_IPCLKPORT_PCLK, 21, 0, 0),
};

static const struct samsung_cmu_info misc_cmu_info __initconst = {
	.mux_clks		= misc_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(misc_mux_clks),
	.div_clks		= misc_div_clks,
	.nr_div_clks		= ARRAY_SIZE(misc_div_clks),
	.gate_clks		= misc_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(misc_gate_clks),
	.nr_clk_ids		= CLKS_NR_MISC,
	.clk_regs		= misc_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(misc_clk_regs),
	.clk_name		= "bus",
};

static void __init zumapro_cmu_misc_init(struct device_node *np)
{
	exynos_arm64_register_cmu(NULL, np, &misc_cmu_info);
}

/* Register CMU_MISC early, as it is needed for the MCT timer and watchdog. */
CLK_OF_DECLARE(zumapro_cmu_misc, "google,zumapro-cmu-misc", zumapro_cmu_misc_init);
