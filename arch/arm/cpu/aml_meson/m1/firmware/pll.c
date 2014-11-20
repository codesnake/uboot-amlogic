/*
 *  This file is for M1 Only . If you want to implement your pll initial function 
 *  please copy this file into the arch/$(ARCH)/$(CPU)/$SOC/firmware directory .
 */
#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/timing.h>
#include <asm/arch/romboot.h>
#include <asm/arch/uart.h>
SPL_STATIC_FUNC void serial_init(unsigned set);
SPL_STATIC_FUNC void pll_init(struct pll_clk_settings * plls) 
{
    
	writel(plls->sys_pll_cntl,P_HHI_SYS_PLL_CNTL);
	writel(// A9 clk set to system clock/2
			(0 << 10) | // 0 - sys_pll_clk, 1 - audio_pll_clk
			(1 << 0 ) | // 1 - sys/audio pll clk, 0 - XTAL
			(1 << 4 ) | // APB_CLK_ENABLE
			(1 << 5 ) | // AT_CLK_ENABLE
			(0 << 2 ) | // div1
			(1 << 7 )   // Connect A9 to the PLL divider output
			,P_HHI_A9_CLK_CNTL);
     
    
	romboot_info->a9_clk = plls->a9_clk;
	/**
	 * clk81 settings From kernel
	 */
    writel(plls->other_pll_cntl,P_HHI_OTHER_PLL_CNTL);
	writel(plls->mpeg_clk_cntl,P_HHI_MPEG_CLK_CNTL);

#ifdef CONFIG_CMD_NET
	/*Demod PLL,1.2G for SATA/Ethernet*/
	writel(plls->demod_pll400m_cntl,P_HHI_DEMOD_PLL_CNTL);
#endif /*CONFIG_CMD_NET*/
	
	romboot_info->clk81 = plls->clk81;
	serial_init(UART_CONTROL_SET(CONFIG_BAUDRATE,plls->clk81));
}
SPL_STATIC_FUNC void ddr_pll_init(struct ddr_set * ddr_setting) 
{
    writel(ddr_setting->ddr_pll_cntl,P_HHI_DDR_PLL_CNTL);    
}
