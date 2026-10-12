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
#define CLK_CON_MUX_MUX_CLKCMU_PERIC0_IP	0x10ec
#define CLK_CON_MUX_MUX_CLKCMU_PERIC0_NOC	0x10f0
#define CLK_CON_DIV_CLKCMU_MISC_NOC		0x18c4
#define CLK_CON_DIV_CLKCMU_PERIC0_IP		0x18e4
#define CLK_CON_DIV_CLKCMU_PERIC0_NOC		0x18e8
#define CLK_CON_GAT_GATE_CLKCMU_MISC_NOC	0x20f4
#define CLK_CON_GAT_GATE_CLKCMU_PERIC0_IP	0x2110
#define CLK_CON_GAT_GATE_CLKCMU_PERIC0_NOC	0x2114
#define CLK_CON_MUX_MUX_CLKCMU_HSI2_NOC		0x10b0
#define CLK_CON_MUX_MUX_CLKCMU_HSI2_UFS_EMBD	0x10b8
#define CLK_CON_DIV_CLKCMU_HSI2_NOC		0x18a8
#define CLK_CON_DIV_CLKCMU_HSI2_UFS_EMBD	0x18b0
#define CLK_CON_GAT_GATE_CLKCMU_HSI2_NOC	0x20d8
#define CLK_CON_GAT_GATE_CLKCMU_HSI2_UFS_EMBD	0x20e0
#define CLK_CON_MUX_MUX_CLKCMU_G3D_GLB		0x106c
#define CLK_CON_MUX_MUX_CLKCMU_G3D_NOCD		0x1070
#define CLK_CON_MUX_MUX_CLKCMU_G3D_SWITCH	0x1074
#define CLK_CON_MUX_MUX_CLKCMU_G3D_TRACE	0x1078
#define CLK_CON_DIV_CLKCMU_G3D_GLB		0x1864
#define CLK_CON_DIV_CLKCMU_G3D_NOCD		0x1868
#define CLK_CON_DIV_CLKCMU_G3D_SWITCH		0x186c
#define CLK_CON_DIV_CLKCMU_G3D_TRACE		0x1870
#define CLK_CON_GAT_GATE_CLKCMU_G3D_GLB		0x2094
#define CLK_CON_GAT_GATE_CLKCMU_G3D_NOCD	0x2098
#define CLK_CON_GAT_GATE_CLKCMU_G3D_SWITCH	0x209c
#define CLK_CON_GAT_GATE_CLKCMU_G3D_TRACE	0x20a0

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
	CLK_CON_MUX_MUX_CLKCMU_PERIC0_IP,
	CLK_CON_MUX_MUX_CLKCMU_PERIC0_NOC,
	CLK_CON_DIV_CLKCMU_MISC_NOC,
	CLK_CON_DIV_CLKCMU_PERIC0_IP,
	CLK_CON_DIV_CLKCMU_PERIC0_NOC,
	CLK_CON_GAT_GATE_CLKCMU_MISC_NOC,
	CLK_CON_GAT_GATE_CLKCMU_PERIC0_IP,
	CLK_CON_GAT_GATE_CLKCMU_PERIC0_NOC,
	CLK_CON_MUX_MUX_CLKCMU_HSI2_NOC,
	CLK_CON_MUX_MUX_CLKCMU_HSI2_UFS_EMBD,
	CLK_CON_DIV_CLKCMU_HSI2_NOC,
	CLK_CON_DIV_CLKCMU_HSI2_UFS_EMBD,
	CLK_CON_GAT_GATE_CLKCMU_HSI2_NOC,
	CLK_CON_GAT_GATE_CLKCMU_HSI2_UFS_EMBD,
	CLK_CON_MUX_MUX_CLKCMU_G3D_GLB,
	CLK_CON_MUX_MUX_CLKCMU_G3D_NOCD,
	CLK_CON_MUX_MUX_CLKCMU_G3D_SWITCH,
	CLK_CON_MUX_MUX_CLKCMU_G3D_TRACE,
	CLK_CON_DIV_CLKCMU_G3D_GLB,
	CLK_CON_DIV_CLKCMU_G3D_NOCD,
	CLK_CON_DIV_CLKCMU_G3D_SWITCH,
	CLK_CON_DIV_CLKCMU_G3D_TRACE,
	CLK_CON_GAT_GATE_CLKCMU_G3D_GLB,
	CLK_CON_GAT_GATE_CLKCMU_G3D_NOCD,
	CLK_CON_GAT_GATE_CLKCMU_G3D_SWITCH,
	CLK_CON_GAT_GATE_CLKCMU_G3D_TRACE,
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

PNAME(mout_cmu_peric0_p) = { "dout_shared0_div4", "dout_shared2_div2",
			     "dout_shared3_div2", "fout_spare_pll" };

PNAME(mout_cmu_hsi2_noc_p) = { "dout_shared0_div4", "dout_shared1_div4",
			      "dout_shared2_div2", "dout_shared3_div2",
			      "fout_spare_pll", "oscclk", "oscclk", "oscclk" };

PNAME(mout_cmu_hsi2_ufs_embd_p) = { "oscclk", "dout_shared0_div4",
				   "dout_shared2_div2", "fout_spare_pll" };

PNAME(mout_cmu_g3d_glb_p) = { "dout_shared0_div2", "dout_shared1_div2",
			     "fout_shared2_pll", "fout_shared3_pll",
			     "dout_shared0_div3", "dout_shared1_div3",
			     "dout_shared0_div4", "fout_spare_pll" };

PNAME(mout_cmu_g3d_switch_p) = { "fout_shared2_pll", "dout_shared0_div3",
				"fout_shared3_pll", "dout_shared1_div3",
				"dout_shared0_div4", "dout_shared1_div4",
				"fout_spare_pll", "fout_spare_pll" };

PNAME(mout_cmu_g3d_trace_p) = { "dout_shared0_div4", "dout_shared1_div4",
			       "dout_shared2_div2", "dout_shared3_div2" };

static const struct samsung_fixed_factor_clock top_fixed_factor_clks[] __initconst = {
	FFACTOR(CLK_DOUT_SHARED0_DIV2, "dout_shared0_div2", "fout_shared0_pll", 1, 2, 0),
	FFACTOR(CLK_DOUT_SHARED0_DIV3, "dout_shared0_div3", "fout_shared0_pll", 1, 3, 0),
	FFACTOR(CLK_DOUT_SHARED0_DIV4, "dout_shared0_div4", "fout_shared0_pll", 1, 4, 0),
	FFACTOR(CLK_DOUT_SHARED1_DIV2, "dout_shared1_div2", "fout_shared1_pll", 1, 2, 0),
	FFACTOR(CLK_DOUT_SHARED1_DIV3, "dout_shared1_div3", "fout_shared1_pll", 1, 3, 0),
	FFACTOR(CLK_DOUT_SHARED1_DIV4, "dout_shared1_div4", "fout_shared1_pll", 1, 4, 0),
	FFACTOR(CLK_DOUT_SHARED2_DIV2, "dout_shared2_div2", "fout_shared2_pll", 1, 2, 0),
	FFACTOR(CLK_DOUT_SHARED3_DIV2, "dout_shared3_div2", "fout_shared3_pll", 1, 2, 0),
};

static const struct samsung_mux_clock top_mux_clks[] __initconst = {
	MUX(CLK_MOUT_CMU_MISC_NOC, "mout_cmu_misc_noc", mout_cmu_misc_noc_p,
	    CLK_CON_MUX_MUX_CLKCMU_MISC_NOC, 0, 2),
	MUX(CLK_MOUT_CMU_PERIC0_NOC, "mout_cmu_peric0_noc", mout_cmu_peric0_p,
	    CLK_CON_MUX_MUX_CLKCMU_PERIC0_NOC, 0, 2),
	MUX(CLK_MOUT_CMU_PERIC0_IP, "mout_cmu_peric0_ip", mout_cmu_peric0_p,
	    CLK_CON_MUX_MUX_CLKCMU_PERIC0_IP, 0, 2),
	MUX(CLK_MOUT_CMU_HSI2_NOC, "mout_cmu_hsi2_noc", mout_cmu_hsi2_noc_p,
	    CLK_CON_MUX_MUX_CLKCMU_HSI2_NOC, 0, 3),
	MUX(CLK_MOUT_CMU_HSI2_UFS_EMBD, "mout_cmu_hsi2_ufs_embd",
	    mout_cmu_hsi2_ufs_embd_p, CLK_CON_MUX_MUX_CLKCMU_HSI2_UFS_EMBD, 0, 2),
	MUX(CLK_MOUT_CMU_G3D_GLB, "mout_cmu_g3d_glb", mout_cmu_g3d_glb_p,
	    CLK_CON_MUX_MUX_CLKCMU_G3D_GLB, 0, 3),
	MUX(CLK_MOUT_CMU_G3D_NOCD, "mout_cmu_g3d_nocd", mout_cmu_g3d_glb_p,
	    CLK_CON_MUX_MUX_CLKCMU_G3D_NOCD, 0, 3),
	MUX(CLK_MOUT_CMU_G3D_SWITCH, "mout_cmu_g3d_switch", mout_cmu_g3d_switch_p,
	    CLK_CON_MUX_MUX_CLKCMU_G3D_SWITCH, 0, 3),
	MUX(CLK_MOUT_CMU_G3D_TRACE, "mout_cmu_g3d_trace", mout_cmu_g3d_trace_p,
	    CLK_CON_MUX_MUX_CLKCMU_G3D_TRACE, 0, 2),
};

static const struct samsung_div_clock top_div_clks[] __initconst = {
	DIV(CLK_DOUT_CMU_MISC_NOC, "dout_cmu_misc_noc", "mout_cmu_misc_noc",
	    CLK_CON_DIV_CLKCMU_MISC_NOC, 0, 4),
	DIV(CLK_DOUT_CMU_PERIC0_NOC, "dout_cmu_peric0_noc", "gout_cmu_peric0_noc",
	    CLK_CON_DIV_CLKCMU_PERIC0_NOC, 0, 4),
	DIV(CLK_DOUT_CMU_PERIC0_IP, "dout_cmu_peric0_ip", "gout_cmu_peric0_ip",
	    CLK_CON_DIV_CLKCMU_PERIC0_IP, 0, 4),
	DIV(CLK_DOUT_CMU_HSI2_NOC, "dout_cmu_hsi2_noc", "gout_cmu_hsi2_noc",
	    CLK_CON_DIV_CLKCMU_HSI2_NOC, 0, 4),
	DIV(CLK_DOUT_CMU_HSI2_UFS_EMBD, "dout_cmu_hsi2_ufs_embd",
	    "gout_cmu_hsi2_ufs_embd", CLK_CON_DIV_CLKCMU_HSI2_UFS_EMBD, 0, 4),
	DIV(CLK_DOUT_CMU_G3D_GLB, "dout_cmu_g3d_glb", "gout_cmu_g3d_glb",
	    CLK_CON_DIV_CLKCMU_G3D_GLB, 0, 4),
	DIV(CLK_DOUT_CMU_G3D_NOCD, "dout_cmu_g3d_nocd", "gout_cmu_g3d_nocd",
	    CLK_CON_DIV_CLKCMU_G3D_NOCD, 0, 4),
	DIV(CLK_DOUT_CMU_G3D_SWITCH, "dout_cmu_g3d_switch", "gout_cmu_g3d_switch",
	    CLK_CON_DIV_CLKCMU_G3D_SWITCH, 0, 3),
	DIV(CLK_DOUT_CMU_G3D_TRACE, "dout_cmu_g3d_trace", "gout_cmu_g3d_trace",
	    CLK_CON_DIV_CLKCMU_G3D_TRACE, 0, 4),
};

static const struct samsung_gate_clock top_gate_clks[] __initconst = {
	GATE(CLK_GOUT_CMU_MISC_NOC, "gout_cmu_misc_noc", "dout_cmu_misc_noc",
	     CLK_CON_GAT_GATE_CLKCMU_MISC_NOC, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_PERIC0_NOC, "gout_cmu_peric0_noc", "mout_cmu_peric0_noc",
	     CLK_CON_GAT_GATE_CLKCMU_PERIC0_NOC, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_PERIC0_IP, "gout_cmu_peric0_ip", "mout_cmu_peric0_ip",
	     CLK_CON_GAT_GATE_CLKCMU_PERIC0_IP, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_HSI2_NOC, "gout_cmu_hsi2_noc", "mout_cmu_hsi2_noc",
	     CLK_CON_GAT_GATE_CLKCMU_HSI2_NOC, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_HSI2_UFS_EMBD, "gout_cmu_hsi2_ufs_embd",
	     "mout_cmu_hsi2_ufs_embd", CLK_CON_GAT_GATE_CLKCMU_HSI2_UFS_EMBD, 21,
	     CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_G3D_GLB, "gout_cmu_g3d_glb", "mout_cmu_g3d_glb",
	     CLK_CON_GAT_GATE_CLKCMU_G3D_GLB, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_G3D_NOCD, "gout_cmu_g3d_nocd", "mout_cmu_g3d_nocd",
	     CLK_CON_GAT_GATE_CLKCMU_G3D_NOCD, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_G3D_SWITCH, "gout_cmu_g3d_switch", "mout_cmu_g3d_switch",
	     CLK_CON_GAT_GATE_CLKCMU_G3D_SWITCH, 21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_CMU_G3D_TRACE, "gout_cmu_g3d_trace", "mout_cmu_g3d_trace",
	     CLK_CON_GAT_GATE_CLKCMU_G3D_TRACE, 21, CLK_IGNORE_UNUSED, 0),
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

/* ---- CMU_PERIC0 ---------------------------------------------------------- */

/* Register Offset definitions for CMU_PERIC0 (0x10800000) */
#define PLL_CON0_MUX_CLKCMU_PERIC0_NOC_USER			0x0600
#define PLL_CON0_MUX_CLKCMU_PERIC0_USI0_UART_USER		0x0620
#define CLK_CON_DIV_DIV_CLK_PERIC0_USI0_UART			0x1808
#define CLK_CON_GAT_CLK_BLK_PERIC0_UID_USI0_UART_IPCLKPORT_IPCLK	0x2060
#define CLK_CON_GAT_CLK_BLK_PERIC0_UID_USI0_UART_IPCLKPORT_PCLK	0x2064

static const unsigned long peric0_clk_regs[] __initconst = {
	PLL_CON0_MUX_CLKCMU_PERIC0_NOC_USER,
	PLL_CON0_MUX_CLKCMU_PERIC0_USI0_UART_USER,
	CLK_CON_DIV_DIV_CLK_PERIC0_USI0_UART,
	CLK_CON_GAT_CLK_BLK_PERIC0_UID_USI0_UART_IPCLKPORT_IPCLK,
	CLK_CON_GAT_CLK_BLK_PERIC0_UID_USI0_UART_IPCLKPORT_PCLK,
};

PNAME(mout_peric0_noc_user_p) = { "oscclk", "dout_cmu_peric0_noc" };
PNAME(mout_peric0_usi0_uart_user_p) = { "oscclk", "dout_cmu_peric0_ip" };

static const struct samsung_mux_clock peric0_mux_clks[] __initconst = {
	MUX(CLK_MOUT_PERIC0_NOC_USER, "mout_peric0_noc_user",
	    mout_peric0_noc_user_p, PLL_CON0_MUX_CLKCMU_PERIC0_NOC_USER, 4, 1),
	MUX(CLK_MOUT_PERIC0_USI0_UART_USER, "mout_peric0_usi0_uart_user",
	    mout_peric0_usi0_uart_user_p,
	    PLL_CON0_MUX_CLKCMU_PERIC0_USI0_UART_USER, 4, 1),
};

static const struct samsung_div_clock peric0_div_clks[] __initconst = {
	DIV(CLK_DOUT_PERIC0_USI0_UART, "dout_peric0_usi0_uart",
	    "mout_peric0_usi0_uart_user", CLK_CON_DIV_DIV_CLK_PERIC0_USI0_UART,
	    0, 4),
};

static const struct samsung_gate_clock peric0_gate_clks[] __initconst = {
	GATE(CLK_GOUT_PERIC0_USI0_UART_IPCLK, "gout_peric0_usi0_uart_ipclk",
	     "dout_peric0_usi0_uart",
	     CLK_CON_GAT_CLK_BLK_PERIC0_UID_USI0_UART_IPCLKPORT_IPCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_PERIC0_USI0_UART_PCLK, "gout_peric0_usi0_uart_pclk",
	     "mout_peric0_noc_user",
	     CLK_CON_GAT_CLK_BLK_PERIC0_UID_USI0_UART_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
};

static const struct samsung_cmu_info peric0_cmu_info __initconst = {
	.mux_clks		= peric0_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(peric0_mux_clks),
	.div_clks		= peric0_div_clks,
	.nr_div_clks		= ARRAY_SIZE(peric0_div_clks),
	.gate_clks		= peric0_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(peric0_gate_clks),
	.nr_clk_ids		= CLKS_NR_PERIC0,
	.clk_regs		= peric0_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(peric0_clk_regs),
	.clk_name		= "noc",
};

static void __init zumapro_cmu_peric0_init(struct device_node *np)
{
	exynos_arm64_register_cmu(NULL, np, &peric0_cmu_info);
}

CLK_OF_DECLARE(zumapro_cmu_peric0, "google,zumapro-cmu-peric0",
	       zumapro_cmu_peric0_init);

/* ---- CMU_HSI2 ------------------------------------------------------------ */

/* Register Offset definitions for CMU_HSI2 (0x13000000) */
#define PLL_CON0_MUX_CLKCMU_HSI2_NOC_USER				0x0610
#define PLL_CON0_MUX_CLKCMU_HSI2_UFS_EMBD_USER				0x0630
#define CLK_CON_GAT_GOUT_BLK_HSI2_UID_QE_UFS_EMBD_HSI2_IPCLKPORT_ACLK	0x20cc
#define CLK_CON_GAT_GOUT_BLK_HSI2_UID_QE_UFS_EMBD_HSI2_IPCLKPORT_PCLK	0x20d0
#define CLK_CON_GAT_GOUT_BLK_HSI2_UID_SYSREG_HSI2_IPCLKPORT_PCLK		0x20e8
#define CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_ACLK		0x210c
#define CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_CLK_UNIPRO	0x2110
#define CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_FMP_CLK	0x2114

static const unsigned long hsi2_clk_regs[] __initconst = {
	PLL_CON0_MUX_CLKCMU_HSI2_NOC_USER,
	PLL_CON0_MUX_CLKCMU_HSI2_UFS_EMBD_USER,
	CLK_CON_GAT_GOUT_BLK_HSI2_UID_QE_UFS_EMBD_HSI2_IPCLKPORT_ACLK,
	CLK_CON_GAT_GOUT_BLK_HSI2_UID_QE_UFS_EMBD_HSI2_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_HSI2_UID_SYSREG_HSI2_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_ACLK,
	CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_CLK_UNIPRO,
	CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_FMP_CLK,
};

PNAME(mout_hsi2_noc_user_p) = { "oscclk", "dout_cmu_hsi2_noc" };
PNAME(mout_hsi2_ufs_embd_user_p) = { "oscclk", "dout_cmu_hsi2_ufs_embd" };

static const struct samsung_mux_clock hsi2_mux_clks[] __initconst = {
	MUX(CLK_MOUT_HSI2_NOC_USER, "mout_hsi2_noc_user", mout_hsi2_noc_user_p,
	    PLL_CON0_MUX_CLKCMU_HSI2_NOC_USER, 4, 1),
	MUX(CLK_MOUT_HSI2_UFS_EMBD_USER, "mout_hsi2_ufs_embd_user",
	    mout_hsi2_ufs_embd_user_p, PLL_CON0_MUX_CLKCMU_HSI2_UFS_EMBD_USER,
	    4, 1),
};

static const struct samsung_gate_clock hsi2_gate_clks[] __initconst = {
	GATE(CLK_GOUT_HSI2_UFS_EMBD_I_ACLK, "gout_hsi2_ufs_embd_i_aclk",
	     "mout_hsi2_noc_user",
	     CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_ACLK, 21, 0, 0),
	GATE(CLK_GOUT_HSI2_UFS_EMBD_I_CLK_UNIPRO, "gout_hsi2_ufs_embd_i_clk_unipro",
	     "mout_hsi2_ufs_embd_user",
	     CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_CLK_UNIPRO, 21, 0, 0),
	GATE(CLK_GOUT_HSI2_UFS_EMBD_I_FMP_CLK, "gout_hsi2_ufs_embd_i_fmp_clk",
	     "mout_hsi2_noc_user",
	     CLK_CON_GAT_GOUT_BLK_HSI2_UID_UFS_EMBD_IPCLKPORT_I_FMP_CLK, 21, 0, 0),
	GATE(CLK_GOUT_HSI2_QE_UFS_EMBD_HSI2_ACLK, "gout_hsi2_qe_ufs_embd_hsi2_aclk",
	     "mout_hsi2_noc_user",
	     CLK_CON_GAT_GOUT_BLK_HSI2_UID_QE_UFS_EMBD_HSI2_IPCLKPORT_ACLK, 21, 0, 0),
	GATE(CLK_GOUT_HSI2_QE_UFS_EMBD_HSI2_PCLK, "gout_hsi2_qe_ufs_embd_hsi2_pclk",
	     "mout_hsi2_noc_user",
	     CLK_CON_GAT_GOUT_BLK_HSI2_UID_QE_UFS_EMBD_HSI2_IPCLKPORT_PCLK, 21, 0, 0),
	GATE(CLK_GOUT_HSI2_SYSREG_HSI2_PCLK, "gout_hsi2_sysreg_hsi2_pclk",
	     "mout_hsi2_noc_user",
	     CLK_CON_GAT_GOUT_BLK_HSI2_UID_SYSREG_HSI2_IPCLKPORT_PCLK, 21, 0, 0),
};

static const struct samsung_cmu_info hsi2_cmu_info __initconst = {
	.mux_clks		= hsi2_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(hsi2_mux_clks),
	.gate_clks		= hsi2_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(hsi2_gate_clks),
	.nr_clk_ids		= CLKS_NR_HSI2,
	.clk_regs		= hsi2_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(hsi2_clk_regs),
	.clk_name		= "noc",
};

static void __init zumapro_cmu_hsi2_init(struct device_node *np)
{
	exynos_arm64_register_cmu(NULL, np, &hsi2_cmu_info);
}

CLK_OF_DECLARE(zumapro_cmu_hsi2, "google,zumapro-cmu-hsi2",
	       zumapro_cmu_hsi2_init);

#define PLL_CON0_MUX_CLKCMU_G3D_GLB_USER	0x0600
#define PLL_CON0_MUX_CLKCMU_G3D_NOCD_USER	0x0610
#define PLL_CON0_MUX_CLKCMU_G3D_SWITCH_USER	0x0620
#define PLL_CON0_MUX_CLK_G3DCORE_COREGROUP_USER	0x0630
#define PLL_CON0_MUX_CLK_G3DCORE_STACKS_USER	0x0640
#define PLL_CON0_MUX_CLK_G3DCORE_TOP_USER	0x0650
#define PLL_CON0_MUX_CLK_G3DCORE_TRACE_USER	0x0660
#define CLK_CON_MUX_MUX_CLK_G3D_L2_GLB	0x1000
#define CLK_CON_MUX_MUX_CLK_G3D_STACKS	0x1004
#define CLK_CON_MUX_MUX_CLK_G3D_TOP	0x1008
#define CLK_CON_DIV_CLK_G3D_ADD_CH_CLK	0x1800
#define CLK_CON_DIV_DIV_CLK_G3D_NOCP	0x1808
#define CLK_CON_DIV_DIV_CLK_G3D_NOCP_LH	0x180c
#define CLK_CON_DIV_DIV_CLK_G3D_TOP	0x1814
#define CLK_CON_GAT_CLK_BLK_G3D_UID_ADD_G3D_IPCLKPORT_CH_CLK	0x2000
#define CLK_CON_GAT_CLK_BLK_G3D_UID_ADM_DAP_G_GPU_IPCLKPORT_DAPCLKM	0x2004
#define CLK_CON_GAT_CLK_BLK_G3D_UID_AD_APB_SYSMMU_G3D_IPCLKPORT_PCLKM	0x2008
#define CLK_CON_GAT_CLK_BLK_G3D_UID_BLK_G3D_FRC_OTP_DESERIAL_IPCLKPORT_I_CLK	0x200c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_DAPAHBAP_GPU_IPCLKPORT_DAPCLK	0x2010
#define CLK_CON_GAT_CLK_BLK_G3D_UID_DAPAHBAP_GPU_IPCLKPORT_HCLK	0x2014
#define CLK_CON_GAT_CLK_BLK_G3D_UID_G3D_CMU_G3D_IPCLKPORT_PCLK	0x2018
#define CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK	0x201c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_COREGROUP	0x2020
#define CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_STACKS	0x2024
#define CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_TRACE	0x2028
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D0_G3D_IPCLKPORT_I_CLK	0x202c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D1_G3D_IPCLKPORT_I_CLK	0x2030
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D2_G3D_IPCLKPORT_I_CLK	0x2034
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D3_G3D_IPCLKPORT_I_CLK	0x2038
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ATB_SI_LT_G3D_CPUCL0_IPCLKPORT_I_CLK	0x203c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_AXI_MI_IP_G3D_IPCLKPORT_I_CLK	0x2040
#define CLK_CON_GAT_CLK_BLK_G3D_UID_LH_AXI_SI_P_G3D_CU_IPCLKPORT_I_CLK	0x2044
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D0_IPCLKPORT_ACLK	0x2048
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D0_IPCLKPORT_PCLK	0x204c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D1_IPCLKPORT_ACLK	0x2050
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D1_IPCLKPORT_PCLK	0x2054
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D0_IPCLKPORT_ACLK	0x2058
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D0_IPCLKPORT_PCLK	0x205c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D1_IPCLKPORT_ACLK	0x2060
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D1_IPCLKPORT_PCLK	0x2064
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D2_IPCLKPORT_ACLK	0x2068
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D2_IPCLKPORT_PCLK	0x206c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D3_IPCLKPORT_ACLK	0x2070
#define CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D3_IPCLKPORT_PCLK	0x2074
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3DCORE_TOP_IPCLKPORT_CLK	0x2078
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_G3DCORE_NOCD_IPCLKPORT_CLK	0x207c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_NOCP_LH_IPCLKPORT_CLK	0x2080
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_OSCCLK_IPCLKPORT_CLK	0x2084
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3DCORE_TOP_IPCLKPORT_CLK	0x2088
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3DCORE_TRACE_IPCLKPORT_CLK	0x208c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_G3DCORE_NOCD_IPCLKPORT_CLK	0x2090
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_NOCP_IPCLKPORT_CLK	0x2094
#define CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_NOCP_LH_IPCLKPORT_CLK	0x2098
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SLH_AXI_MI_P_G3D_IPCLKPORT_I_CLK	0x209c
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SLH_AXI_SI_D_G3DMMU_IPCLKPORT_I_CLK	0x20a0
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D0_IPCLKPORT_ACLK	0x20a4
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D0_IPCLKPORT_PCLK	0x20a8
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D1_IPCLKPORT_ACLK	0x20ac
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D1_IPCLKPORT_PCLK	0x20b0
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D2_IPCLKPORT_ACLK	0x20b4
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D2_IPCLKPORT_PCLK	0x20b8
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D3_IPCLKPORT_ACLK	0x20bc
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D3_IPCLKPORT_PCLK	0x20c0
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_G3D_IPCLKPORT_CLK	0x20c4
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU0_G3D_IPCLKPORT_CLK	0x20c8
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU1_G3D_IPCLKPORT_CLK	0x20cc
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU2_G3D_IPCLKPORT_CLK	0x20d0
#define CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU3_G3D_IPCLKPORT_CLK	0x20d4
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_ADD_APBIF_G3D_IPCLKPORT_PCLK	0x20e4
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_D_TZPC_G3D_IPCLKPORT_PCLK	0x20e8
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_GPC_G3D_IPCLKPORT_PCLK	0x20ec
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_GRAY2BIN_G3D_IPCLKPORT_CLK	0x20f0
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_LH_AXI_MI_P_G3D_CU_IPCLKPORT_I_CLK	0x20f4
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_LH_AXI_SI_IP_G3D_IPCLKPORT_I_CLK	0x20f8
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_RSTNSYNC_CLK_G3D_DD_IPCLKPORT_CLK	0x20fc
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_RSTNSYNC_CLK_G3D_NOCP_IPCLKPORT_CLK	0x2100
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK	0x2104
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_UASC_G3D_IPCLKPORT_ACLK	0x2108
#define CLK_CON_GAT_GOUT_BLK_G3D_UID_UASC_G3D_IPCLKPORT_PCLK	0x210c

static const unsigned long g3d_clk_regs[] __initconst = {
	PLL_CON0_MUX_CLKCMU_G3D_GLB_USER,
	PLL_CON0_MUX_CLKCMU_G3D_NOCD_USER,
	PLL_CON0_MUX_CLKCMU_G3D_SWITCH_USER,
	PLL_CON0_MUX_CLK_G3DCORE_COREGROUP_USER,
	PLL_CON0_MUX_CLK_G3DCORE_STACKS_USER,
	PLL_CON0_MUX_CLK_G3DCORE_TOP_USER,
	PLL_CON0_MUX_CLK_G3DCORE_TRACE_USER,
	CLK_CON_MUX_MUX_CLK_G3D_L2_GLB,
	CLK_CON_MUX_MUX_CLK_G3D_STACKS,
	CLK_CON_MUX_MUX_CLK_G3D_TOP,
	CLK_CON_DIV_CLK_G3D_ADD_CH_CLK,
	CLK_CON_DIV_DIV_CLK_G3D_NOCP,
	CLK_CON_DIV_DIV_CLK_G3D_NOCP_LH,
	CLK_CON_DIV_DIV_CLK_G3D_TOP,
	CLK_CON_GAT_CLK_BLK_G3D_UID_ADD_G3D_IPCLKPORT_CH_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_ADM_DAP_G_GPU_IPCLKPORT_DAPCLKM,
	CLK_CON_GAT_CLK_BLK_G3D_UID_AD_APB_SYSMMU_G3D_IPCLKPORT_PCLKM,
	CLK_CON_GAT_CLK_BLK_G3D_UID_BLK_G3D_FRC_OTP_DESERIAL_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_DAPAHBAP_GPU_IPCLKPORT_DAPCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_DAPAHBAP_GPU_IPCLKPORT_HCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_G3D_CMU_G3D_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_COREGROUP,
	CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_STACKS,
	CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_TRACE,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D0_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D1_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D2_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D3_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ATB_SI_LT_G3D_CPUCL0_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_AXI_MI_IP_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_LH_AXI_SI_P_G3D_CU_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D0_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D0_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D1_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D1_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D0_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D0_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D1_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D1_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D2_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D2_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D3_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D3_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3DCORE_TOP_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_G3DCORE_NOCD_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_NOCP_LH_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_OSCCLK_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3DCORE_TOP_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3DCORE_TRACE_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_G3DCORE_NOCD_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_NOCP_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_NOCP_LH_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SLH_AXI_MI_P_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SLH_AXI_SI_D_G3DMMU_IPCLKPORT_I_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D0_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D0_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D1_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D1_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D2_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D2_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D3_IPCLKPORT_ACLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D3_IPCLKPORT_PCLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_G3D_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU0_G3D_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU1_G3D_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU2_G3D_IPCLKPORT_CLK,
	CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU3_G3D_IPCLKPORT_CLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_ADD_APBIF_G3D_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_D_TZPC_G3D_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_GPC_G3D_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_GRAY2BIN_G3D_IPCLKPORT_CLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_LH_AXI_MI_P_G3D_CU_IPCLKPORT_I_CLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_LH_AXI_SI_IP_G3D_IPCLKPORT_I_CLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_RSTNSYNC_CLK_G3D_DD_IPCLKPORT_CLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_RSTNSYNC_CLK_G3D_NOCP_IPCLKPORT_CLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_UASC_G3D_IPCLKPORT_ACLK,
	CLK_CON_GAT_GOUT_BLK_G3D_UID_UASC_G3D_IPCLKPORT_PCLK,
};

PNAME(mout_g3d_glb_user_p) = { "oscclk", "dout_cmu_g3d_glb" };
PNAME(mout_g3d_nocd_user_p) = { "oscclk", "dout_cmu_g3d_nocd" };
PNAME(mout_g3d_switch_user_p) = { "oscclk", "dout_cmu_g3d_switch" };
PNAME(mout_g3dcore_coregroup_user_p) = { "oscclk", "dout_g3d_l2_glb" };
PNAME(mout_g3dcore_stacks_user_p) = { "oscclk", "dout_g3d_stacks" };
PNAME(mout_g3dcore_top_user_p) = { "oscclk", "dout_g3d_top" };
PNAME(mout_g3dcore_trace_user_p) = { "oscclk", "dout_cmu_g3d_trace" };
PNAME(mout_g3d_l2_glb_p) = { "g3dl2", "mout_g3d_glb_user" };
PNAME(mout_g3d_stacks_p) = { "g3d", "mout_g3d_switch_user" };
PNAME(mout_g3d_top_p) = { "mout_g3d_stacks", "mout_g3d_nocd_user", "mout_g3d_l2_glb", "oscclk" };

static const struct samsung_mux_clock g3d_mux_clks[] __initconst = {
	MUX(CLK_MOUT_G3D_GLB_USER, "mout_g3d_glb_user", mout_g3d_glb_user_p,
	    PLL_CON0_MUX_CLKCMU_G3D_GLB_USER, 4, 1),
	MUX(CLK_MOUT_G3D_NOCD_USER, "mout_g3d_nocd_user", mout_g3d_nocd_user_p,
	    PLL_CON0_MUX_CLKCMU_G3D_NOCD_USER, 4, 1),
	MUX(CLK_MOUT_G3D_SWITCH_USER, "mout_g3d_switch_user", mout_g3d_switch_user_p,
	    PLL_CON0_MUX_CLKCMU_G3D_SWITCH_USER, 4, 1),
	MUX(CLK_MOUT_G3DCORE_COREGROUP_USER, "mout_g3dcore_coregroup_user", mout_g3dcore_coregroup_user_p,
	    PLL_CON0_MUX_CLK_G3DCORE_COREGROUP_USER, 4, 1),
	MUX(CLK_MOUT_G3DCORE_STACKS_USER, "mout_g3dcore_stacks_user", mout_g3dcore_stacks_user_p,
	    PLL_CON0_MUX_CLK_G3DCORE_STACKS_USER, 4, 1),
	MUX(CLK_MOUT_G3DCORE_TOP_USER, "mout_g3dcore_top_user", mout_g3dcore_top_user_p,
	    PLL_CON0_MUX_CLK_G3DCORE_TOP_USER, 4, 1),
	MUX(CLK_MOUT_G3DCORE_TRACE_USER, "mout_g3dcore_trace_user", mout_g3dcore_trace_user_p,
	    PLL_CON0_MUX_CLK_G3DCORE_TRACE_USER, 4, 1),
	MUX(CLK_MOUT_G3D_L2_GLB, "mout_g3d_l2_glb", mout_g3d_l2_glb_p,
	    CLK_CON_MUX_MUX_CLK_G3D_L2_GLB, 0, 1),
	MUX(CLK_MOUT_G3D_STACKS, "mout_g3d_stacks", mout_g3d_stacks_p,
	    CLK_CON_MUX_MUX_CLK_G3D_STACKS, 0, 1),
	MUX(CLK_MOUT_G3D_TOP, "mout_g3d_top", mout_g3d_top_p,
	    CLK_CON_MUX_MUX_CLK_G3D_TOP, 0, 2),
};

static const struct samsung_div_clock g3d_div_clks[] __initconst = {
	DIV(CLK_DOUT_G3D_ADD_CH_CLK, "dout_g3d_add_ch_clk", "oscclk",
	    CLK_CON_DIV_CLK_G3D_ADD_CH_CLK, 0, 4),
	DIV(CLK_DOUT_G3D_NOCP, "dout_g3d_nocp", "dout_g3d_l2_glb",
	    CLK_CON_DIV_DIV_CLK_G3D_NOCP, 0, 3),
	DIV(CLK_DOUT_G3D_NOCP_LH, "dout_g3d_nocp_lh", "dout_g3d_nocp",
	    CLK_CON_DIV_DIV_CLK_G3D_NOCP_LH, 0, 3),
	DIV(CLK_DOUT_G3D_TOP, "dout_g3d_top", "mout_g3d_top",
	    CLK_CON_DIV_DIV_CLK_G3D_TOP, 0, 3),
};

static const struct samsung_fixed_factor_clock g3d_fixed_factor_clks[] __initconst = {
	FFACTOR(CLK_DOUT_G3D_L2_GLB, "dout_g3d_l2_glb", "mout_g3d_l2_glb", 1, 1, 0),
	FFACTOR(CLK_DOUT_G3D_STACKS, "dout_g3d_stacks", "mout_g3d_stacks", 1, 1, 0),
};

static const struct samsung_gate_clock g3d_gate_clks[] __initconst = {
	GATE(CLK_GOUT_G3D_ADD_G3D_CH_CLK, "gout_g3d_add_g3d_ch_clk",
	     "dout_g3d_add_ch_clk", CLK_CON_GAT_CLK_BLK_G3D_UID_ADD_G3D_IPCLKPORT_CH_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_ADM_DAP_G_GPU_DAPCLKM, "gout_g3d_adm_dap_g_gpu_dapclkm",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_ADM_DAP_G_GPU_IPCLKPORT_DAPCLKM,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_AD_APB_SYSMMU_G3D_PCLKM, "gout_g3d_ad_apb_sysmmu_g3d_pclkm",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_AD_APB_SYSMMU_G3D_IPCLKPORT_PCLKM,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_BLK_G3D_FRC_OTP_DESERIAL_I_CLK, "gout_g3d_blk_g3d_frc_otp_deserial_i_clk",
	     "oscclk", CLK_CON_GAT_CLK_BLK_G3D_UID_BLK_G3D_FRC_OTP_DESERIAL_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_DAPAHBAP_GPU_DAPCLK, "gout_g3d_dapahbap_gpu_dapclk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_DAPAHBAP_GPU_IPCLKPORT_DAPCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_DAPAHBAP_GPU_HCLK, "gout_g3d_dapahbap_gpu_hclk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_DAPAHBAP_GPU_IPCLKPORT_HCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_G3D_CMU_G3D_PCLK, "gout_g3d_g3d_cmu_g3d_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_G3D_CMU_G3D_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_GPU_CLK, "gout_g3d_gpu_clk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK,
	     21, 0, 0),
	GATE(CLK_GOUT_G3D_GPU_CLK_COREGROUP, "gout_g3d_gpu_clk_coregroup",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_COREGROUP,
	     21, 0, 0),
	GATE(CLK_GOUT_G3D_GPU_CLK_STACKS, "gout_g3d_gpu_clk_stacks",
	     "mout_g3dcore_stacks_user", CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_STACKS,
	     21, 0, 0),
	GATE(CLK_GOUT_G3D_GPU_CLK_TRACE, "gout_g3d_gpu_clk_trace",
	     "mout_g3dcore_trace_user", CLK_CON_GAT_CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK_TRACE,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_ACEL_SI_D0_G3D_I_CLK, "gout_g3d_lh_acel_si_d0_g3d_i_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D0_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_ACEL_SI_D1_G3D_I_CLK, "gout_g3d_lh_acel_si_d1_g3d_i_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D1_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_ACEL_SI_D2_G3D_I_CLK, "gout_g3d_lh_acel_si_d2_g3d_i_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D2_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_ACEL_SI_D3_G3D_I_CLK, "gout_g3d_lh_acel_si_d3_g3d_i_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ACEL_SI_D3_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_ATB_SI_LT_G3D_CPUCL0_I_CLK, "gout_g3d_lh_atb_si_lt_g3d_cpucl0_i_clk",
	     "mout_g3dcore_trace_user", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_ATB_SI_LT_G3D_CPUCL0_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_AXI_MI_IP_G3D_I_CLK, "gout_g3d_lh_axi_mi_ip_g3d_i_clk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_AXI_MI_IP_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_AXI_SI_P_G3D_CU_I_CLK, "gout_g3d_lh_axi_si_p_g3d_cu_i_clk",
	     "dout_g3d_nocp_lh", CLK_CON_GAT_CLK_BLK_G3D_UID_LH_AXI_SI_P_G3D_CU_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPCFW_G3D0_ACLK, "gout_g3d_ppcfw_g3d0_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D0_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPCFW_G3D0_PCLK, "gout_g3d_ppcfw_g3d0_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D0_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPCFW_G3D1_ACLK, "gout_g3d_ppcfw_g3d1_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D1_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPCFW_G3D1_PCLK, "gout_g3d_ppcfw_g3d1_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_PPCFW_G3D1_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D0_ACLK, "gout_g3d_ppmu_g3d_d0_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D0_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D0_PCLK, "gout_g3d_ppmu_g3d_d0_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D0_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D1_ACLK, "gout_g3d_ppmu_g3d_d1_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D1_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D1_PCLK, "gout_g3d_ppmu_g3d_d1_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D1_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D2_ACLK, "gout_g3d_ppmu_g3d_d2_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D2_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D2_PCLK, "gout_g3d_ppmu_g3d_d2_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D2_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D3_ACLK, "gout_g3d_ppmu_g3d_d3_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D3_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_PPMU_G3D_D3_PCLK, "gout_g3d_ppmu_g3d_d3_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_PPMU_G3D_D3_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_CLK_G3DCORE_TOP_CLK, "gout_g3d_rstnsync_clk_g3dcore_top_clk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3DCORE_TOP_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_CLK_G3D_G3DCORE_NOCD_CLK, "gout_g3d_rstnsync_clk_g3d_g3dcore_nocd_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_G3DCORE_NOCD_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_CLK_G3D_NOCP_LH_CLK, "gout_g3d_rstnsync_clk_g3d_nocp_lh_clk",
	     "dout_g3d_nocp_lh", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_NOCP_LH_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_CLK_G3D_OSCCLK_CLK, "gout_g3d_rstnsync_clk_g3d_oscclk_clk",
	     "oscclk", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_CLK_G3D_OSCCLK_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_SR_CLK_G3DCORE_TOP_CLK, "gout_g3d_rstnsync_sr_clk_g3dcore_top_clk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3DCORE_TOP_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_SR_CLK_G3DCORE_TRACE_CLK, "gout_g3d_rstnsync_sr_clk_g3dcore_trace_clk",
	     "mout_g3dcore_trace_user", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3DCORE_TRACE_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_SR_CLK_G3D_G3DCORE_NOCD_CLK, "gout_g3d_rstnsync_sr_clk_g3d_g3dcore_nocd_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_G3DCORE_NOCD_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_SR_CLK_G3D_NOCP_CLK, "gout_g3d_rstnsync_sr_clk_g3d_nocp_clk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_NOCP_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_SR_CLK_G3D_NOCP_LH_CLK, "gout_g3d_rstnsync_sr_clk_g3d_nocp_lh_clk",
	     "dout_g3d_nocp_lh", CLK_CON_GAT_CLK_BLK_G3D_UID_RSTNSYNC_SR_CLK_G3D_NOCP_LH_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SLH_AXI_MI_P_G3D_I_CLK, "gout_g3d_slh_axi_mi_p_g3d_i_clk",
	     "dout_g3d_nocp_lh", CLK_CON_GAT_CLK_BLK_G3D_UID_SLH_AXI_MI_P_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SLH_AXI_SI_D_G3DMMU_I_CLK, "gout_g3d_slh_axi_si_d_g3dmmu_i_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SLH_AXI_SI_D_G3DMMU_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D0_ACLK, "gout_g3d_ssmt_g3d0_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D0_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D0_PCLK, "gout_g3d_ssmt_g3d0_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D0_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D1_ACLK, "gout_g3d_ssmt_g3d1_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D1_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D1_PCLK, "gout_g3d_ssmt_g3d1_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D1_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D2_ACLK, "gout_g3d_ssmt_g3d2_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D2_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D2_PCLK, "gout_g3d_ssmt_g3d2_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D2_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D3_ACLK, "gout_g3d_ssmt_g3d3_aclk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D3_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SSMT_G3D3_PCLK, "gout_g3d_ssmt_g3d3_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_CLK_BLK_G3D_UID_SSMT_G3D3_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SYSMMU_S0_G3D_CLK, "gout_g3d_sysmmu_s0_g3d_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_G3D_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SYSMMU_S0_PMMU0_G3D_CLK, "gout_g3d_sysmmu_s0_pmmu0_g3d_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU0_G3D_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SYSMMU_S0_PMMU1_G3D_CLK, "gout_g3d_sysmmu_s0_pmmu1_g3d_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU1_G3D_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SYSMMU_S0_PMMU2_G3D_CLK, "gout_g3d_sysmmu_s0_pmmu2_g3d_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU2_G3D_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SYSMMU_S0_PMMU3_G3D_CLK, "gout_g3d_sysmmu_s0_pmmu3_g3d_clk",
	     "mout_g3dcore_coregroup_user", CLK_CON_GAT_CLK_BLK_G3D_UID_SYSMMU_S0_PMMU3_G3D_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_ADD_APBIF_G3D_PCLK, "gout_g3d_add_apbif_g3d_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_ADD_APBIF_G3D_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_D_TZPC_G3D_PCLK, "gout_g3d_d_tzpc_g3d_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_D_TZPC_G3D_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_GPC_G3D_PCLK, "gout_g3d_gpc_g3d_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_GPC_G3D_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_GRAY2BIN_G3D_CLK, "gout_g3d_gray2bin_g3d_clk",
	     "mout_g3dcore_top_user", CLK_CON_GAT_GOUT_BLK_G3D_UID_GRAY2BIN_G3D_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_AXI_MI_P_G3D_CU_I_CLK, "gout_g3d_lh_axi_mi_p_g3d_cu_i_clk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_LH_AXI_MI_P_G3D_CU_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_LH_AXI_SI_IP_G3D_I_CLK, "gout_g3d_lh_axi_si_ip_g3d_i_clk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_LH_AXI_SI_IP_G3D_IPCLKPORT_I_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_CLK_G3D_DD_CLK, "gout_g3d_rstnsync_clk_g3d_dd_clk",
	     "mout_g3d_stacks", CLK_CON_GAT_GOUT_BLK_G3D_UID_RSTNSYNC_CLK_G3D_DD_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_RSTNSYNC_CLK_G3D_NOCP_CLK, "gout_g3d_rstnsync_clk_g3d_nocp_clk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_RSTNSYNC_CLK_G3D_NOCP_IPCLKPORT_CLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_SYSREG_G3D_PCLK, "gout_g3d_sysreg_g3d_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_UASC_G3D_ACLK, "gout_g3d_uasc_g3d_aclk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_UASC_G3D_IPCLKPORT_ACLK,
	     21, CLK_IGNORE_UNUSED, 0),
	GATE(CLK_GOUT_G3D_UASC_G3D_PCLK, "gout_g3d_uasc_g3d_pclk",
	     "dout_g3d_nocp", CLK_CON_GAT_GOUT_BLK_G3D_UID_UASC_G3D_IPCLKPORT_PCLK,
	     21, CLK_IGNORE_UNUSED, 0),
};

static const struct samsung_cmu_info g3d_cmu_info __initconst = {
	.mux_clks		= g3d_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(g3d_mux_clks),
	.div_clks		= g3d_div_clks,
	.nr_div_clks		= ARRAY_SIZE(g3d_div_clks),
	.gate_clks		= g3d_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(g3d_gate_clks),
	.fixed_factor_clks	= g3d_fixed_factor_clks,
	.nr_fixed_factor_clks	= ARRAY_SIZE(g3d_fixed_factor_clks),
	.nr_clk_ids		= CLKS_NR_G3D,
	.clk_regs		= g3d_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(g3d_clk_regs),
};

static void __init zumapro_cmu_g3d_init(struct device_node *np)
{
	exynos_arm64_register_cmu(NULL, np, &g3d_cmu_info);
}

CLK_OF_DECLARE(zumapro_cmu_g3d, "google,zumapro-cmu-g3d",
	       zumapro_cmu_g3d_init);
