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

	/**
	 * Enable PLLs pins
	 */
	//*P_AM_ANALOG_TOP_REG1 |= 0x1; // Enable DDR_PLL enable pin
	//*P_HHI_MPLL_CNTL5	  |= 0x1; // Enable Both MPLL and SYS_PLL enable pin
//	0xc11081bc
	writel(readl(0xc11081bc)|1,0xc11081bc);
	//#define AM_ANALOG_TOP_REG1                         0x206f

	//switch a9 clock to  oscillator in the first.	This is sync mux.
#if 0	
    Wr( HHI_A9_CLK_CNTL, 0);
#else
	Wr( HHI_A9_CLK_CNTL, Rd(HHI_A9_CLK_CNTL) & (~(1<<7)));
	__udelay(10);
	Wr( HHI_A9_CLK_CNTL, 0);
	__udelay(10);
	Wr(HHI_MPEG_CLK_CNTL, Rd(HHI_MPEG_CLK_CNTL) & (~(1<<8)) );	
#endif

	serial_init(52|UART_CNTL_MASK_TX_EN|UART_CNTL_MASK_RX_EN);	//clk81 switch to 24M, init serial to print info

	__udelay(100);

	int pll_times=0;

	do{
		//BANDGAP reset for SYS_PLL,VIID_PLL,MPLL lock fail
		//Note: once SYS PLL is up, there is no need to
		//          use AM_ANALOG_TOP_REG1 for VIID, MPLL
		//          lock fail
		Wr_reg_bits(HHI_MPLL_CNTL5,0,0,1);
		__udelay(10);
		Wr_reg_bits(HHI_MPLL_CNTL5,1,0,1);
		__udelay(1000); //1ms for bandgap bootup

		M6_PLL_RESET(HHI_SYS_PLL_CNTL);
		Wr(HHI_SYS_PLL_CNTL2,M6_SYS_PLL_CNTL_2);
		Wr(HHI_SYS_PLL_CNTL3,M6_SYS_PLL_CNTL_3);
		Wr(HHI_SYS_PLL_CNTL4,M6_SYS_PLL_CNTL_4);
		Wr(HHI_SYS_PLL_CNTL, plls->sys_pll_cntl);
		M6_PLL_LOCK_CHECK(pll_times,1);
	}while(M6_PLL_IS_NOT_LOCK(HHI_SYS_PLL_CNTL));

	//A9 clock setting
	Wr(HHI_A9_CLK_CNTL,(plls->sys_clk_cntl & (~(1<<7))));
	__udelay(1);
	//enable A9 clock
	Wr(HHI_A9_CLK_CNTL,(plls->sys_clk_cntl | (1<<7)));

	pll_times=0;

	//VIID PLL
	do{		
		M6_PLL_RESET(HHI_VIID_PLL_CNTL);
		Wr(HHI_VIID_PLL_CNTL2, M6_VIID_PLL_CNTL_2 );
		Wr(HHI_VIID_PLL_CNTL3, M6_VIID_PLL_CNTL_3 );
		Wr(HHI_VIID_PLL_CNTL4, M6_VIID_PLL_CNTL_4 );
		//Wr(HHI_VIID_PLL_CNTL,  0x20242 ); //change PLL from 1.584GHz to 1.512GHz
		Wr(HHI_VIID_PLL_CNTL,  0x2023F ); //change PLL from 1.584GHz to 1.512GHz
		M6_PLL_LOCK_CHECK(pll_times,2);
	}while(M6_PLL_IS_NOT_LOCK(HHI_VIID_PLL_CNTL));
	
	pll_times=0;

	//FIXED PLL/Multi-phase PLL, fixed to 2GHz
	do{
		M6_PLL_RESET(HHI_MPLL_CNTL);
		Wr(HHI_MPLL_CNTL2, M6_MPLL_CNTL_2 );
		Wr(HHI_MPLL_CNTL3, M6_MPLL_CNTL_3 );
		Wr(HHI_MPLL_CNTL4, M6_MPLL_CNTL_4 );
		Wr(HHI_MPLL_CNTL5, M6_MPLL_CNTL_5 );
		Wr(HHI_MPLL_CNTL6, M6_MPLL_CNTL_6 );
		Wr(HHI_MPLL_CNTL7, M6_MPLL_CNTL_7 );
		Wr(HHI_MPLL_CNTL8, M6_MPLL_CNTL_8 );
		Wr(HHI_MPLL_CNTL9, M6_MPLL_CNTL_9 );
		Wr(HHI_MPLL_CNTL10,M6_MPLL_CNTL_10);
		Wr(HHI_MPLL_CNTL, 0x67d );		
		M6_PLL_LOCK_CHECK(pll_times,3);
	}while(M6_PLL_IS_NOT_LOCK(HHI_MPLL_CNTL));
	
	//clk81=fclk_div5 /2=400/2=200M
	Wr(HHI_MPEG_CLK_CNTL, plls->mpeg_clk_cntl );
	serial_init(plls->uart);	//clk81 switch to MPLL, init serial to print info
	
	pll_times=0;

	//VID PLL
	do{
		//BANDGAP reset for VID_PLL,DDR_PLL lock fail
		//Note: once VID PLL is up, there is no need to
		//          use AM_ANALOG_TOP_REG1 for DDR PLL
		//          lock fail
		Wr_reg_bits(AM_ANALOG_TOP_REG1,0,0,1);
		__udelay(10);
		Wr_reg_bits(AM_ANALOG_TOP_REG1,1,0,1);
		__udelay(1000); //1ms for bandgap bootup

		M6_PLL_RESET(HHI_VID_PLL_CNTL);
		Wr(HHI_VID_PLL_CNTL2, M6_VID_PLL_CNTL_2 );
		Wr(HHI_VID_PLL_CNTL3, M6_VID_PLL_CNTL_3 );
		Wr(HHI_VID_PLL_CNTL4, M6_VID_PLL_CNTL_4 );
		//Wr(HHI_VID_PLL_CNTL,  0xb0442 ); //change PLL from 1.584GHz to 1.512GHz
		Wr(HHI_VID_PLL_CNTL,  0xb043F ); //change PLL from 1.584GHz to 1.512GHz		
		M6_PLL_LOCK_CHECK(pll_times,4);
	}while(M6_PLL_IS_NOT_LOCK(HHI_VID_PLL_CNTL));

 	__udelay(100);

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
