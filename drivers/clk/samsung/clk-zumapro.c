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

static const struct samsung_fixed_factor_clock top_fixed_factor_clks[] __initconst = {
	FFACTOR(CLK_DOUT_SHARED0_DIV4, "dout_shared0_div4", "fout_shared0_pll", 1, 4, 0),
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
