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
#include <asm/arch/reg_addr.h>

unsigned long    clk_util_clk_msr(unsigned long   clk_mux);

static void wait_pll(unsigned clk,unsigned dest)
{
    unsigned cur;
    do{
        cur=clk_util_clk_msr(clk);
        
        serial_puts("wait pll-0x");
        serial_put_hex(clk,8);
        serial_puts(" target is ");
        serial_put_hex(dest,16);
        serial_puts(" now it is ");
        serial_put_dword(cur);
        __udelay(100);
    }while(cur<dest-1 || cur >dest+1);
}

SPL_STATIC_FUNC void pll_init(struct pll_clk_settings * plls) 
{
    clrbits_le32(P_HHI_A9_CLK_CNTL,1<<7);
    clrbits_le32(P_HHI_MPEG_CLK_CNTL,1<<8);
     //* sys pll
    writel(plls->sys_pll_cntl|0x8000,P_HHI_SYS_PLL_CNTL);//800Mhz(0x664),600Mhz,400Mhz , 200Mhz
    writel(0x65e31ff  ,P_HHI_SYS_PLL_CNTL2);
    writel(0x1649a941 , P_HHI_SYS_PLL_CNTL3);
    writel(plls->sys_pll_cntl&(~0x8000),P_HHI_SYS_PLL_CNTL);
	
    /** misc pll **/
    writel(plls->other_pll_cntl|0x8000,P_HHI_OTHER_PLL_CNTL);//800Mhz(0x664)
    writel(0x65e31ff  ,P_HHI_OTHER_PLL_CNTL2);
    writel(0x1649a941 , P_HHI_OTHER_PLL_CNTL3);
    writel(plls->other_pll_cntl&(~0x8000),P_HHI_OTHER_PLL_CNTL);
    
    wait_pll(2,plls->sys_clk);	
    wait_pll(4,plls->other_clk);
    serial_puts("\n\n\n");
    __udelay(3000);
		serial_wait_tx_empty();
		writel(plls->sys_clk_cntl
				,P_HHI_A9_CLK_CNTL);
		serial_wait_tx_empty();
		writel(plls->mpeg_clk_cntl,
		    P_HHI_MPEG_CLK_CNTL);
		serial_init(__plls.uart);
		__udelay(1000);
	serial_put_dword(clk_util_clk_msr(7));
#if 0
    Wr(HHI_DDR_PLL_CNTL, timing_reg->ddr_pll_cntl |0x8000);//667,533,400((0x664)),333
    Wr(HHI_DDR_PLL_CNTL2,0x65e31ff);
    Wr(HHI_DDR_PLL_CNTL3,0x1649a941);
    Wr(HHI_DDR_PLL_CNTL, timing_reg->ddr_pll_cntl);
#endif	
	
#if 0			
    /** Setting sys cpu pll**/
	writel(plls->sys_pll_cntl,P_HHI_SYS_PLL_CNTL);
	writel( (1<<0)    |  //select sys pll for sys cpu
	        (1<<2)    |  // divided 2
	        (1<<4)    |  //APB_en
	        (1<<5)    |  //AT en
	        (1<<7)      // send to sys cpu
	          
			,P_HHI_SYS_CPU_CLK_CNTL);
	/**
	 * clk81 settings From kernel
	 */
  writel(plls->other_pll_cntl,P_HHI_OTHER_PLL_CNTL);
	writel(plls->mpeg_clk_cntl,P_HHI_MPEG_CLK_CNTL);
	
#endif	

    int i;
    for(i=0;i<46;i++)
    {
        serial_put_hex(i,8);
        serial_puts("=");
        serial_put_dword(clk_util_clk_msr(i));
    }
}
SPL_STATIC_FUNC void ddr_pll_init(struct ddr_set * ddr_setting) 
{
    writel(ddr_setting->ddr_pll_cntl,P_HHI_DDR_PLL_CNTL);    
}

unsigned long    clk_util_clk_msr(unsigned long   clk_mux)
{
    unsigned long   measured_val;
    unsigned long   uS_gate_time=64;
    unsigned long dummy_rd;
    writel(0,P_MSR_CLK_REG0);
    // Set the measurement gate to 64uS
    clrsetbits_le32(P_MSR_CLK_REG0,0xffff,(uS_gate_time-1));
    // Disable continuous measurement
    // disable interrupts
    clrbits_le32(P_MSR_CLK_REG0,(1 << 18) | (1 << 17));
//    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~((1 << 18) | (1 << 17))) );
    clrsetbits_le32(P_MSR_CLK_REG0,
        (0xf << 20)|(1<<19)|(1<<16),
        (clk_mux<<20) |(1<<19)|(1<<16));
    { dummy_rd = readl(P_MSR_CLK_REG0); }
    // Wait for the measurement to be done
    while( (readl(P_MSR_CLK_REG0) & (1 << 31)) ) {} 
    // disable measuring
    clrbits_le32(P_MSR_CLK_REG0, 1<<16 );

    measured_val = readl(P_MSR_CLK_REG2)&0x000FFFFF;
    // Return value in Hz*measured_val
    // Return Mhz
    return (measured_val>>6);
    // Return value in Hz*measured_val
}

STATIC_PREFIX void pll_clk_list(void)
{


    unsigned long   clk_freq;
    unsigned char clk_list[]={3,10,11};
    char *clk_list_name[]={"arm","ddr","other"};
    unsigned long  i;
	for(i=0;i<3;i++)
	{
		clk_freq = clk_util_clk_msr(clk_list[i]     // unsigned long   clk_mux,             // Measure A9 clock
								);   // unsigned long   uS_gate_time )       // 50us gate time
		serial_puts(clk_list_name[i]);
		serial_puts("_clk=");
		serial_put_dword(clk_freq);
		serial_puts("\n");
	}
}
#define CONFIG_AML_CLK_LIST_ENABLE 1
