#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/uart.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/pctl.h>
#include <asm/arch/dmc.h>
#include <asm/arch/ddr.h>
#include <asm/arch/memtest.h>
#include <asm/arch/pctl.h>
#include "boot_code.dat"
//----------------------------------------------------
unsigned UART_CONFIG_24M= (200000000/(115200*4)  );
unsigned UART_CONFIG= (32*1000/(300*4));
//#define EN_DEBUG
//----------------------------------------------------
//functions declare
void store_restore_plls(int flag);
#define clkoff_a9()

#define ISOLATE_RESET_N_TO_EE       clrbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,(1 << 5))
#define ISOLATE_TEST_MODE_FROM_EE   clrbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,(1 << 3))
#define ISOLATE_IRQ_FROM_EE         clrbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,(1 << 2))
#define ISOLATE_RESET_N_FROM_EE     clrbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,(1 << 1))
#define ISOLATE_AHB_BUS_FROM_EE     clrbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,(1 << 0))

#define ENABLE_RESET_N_TO_EE        setbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,  (1 << 5))
#define ENABLE_TEST_MODE_FROM_EE    setbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,  (1 << 3))
#define ENABLE_IRQ_FROM_EE          setbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,  (1 << 2))
#define ENABLE_RESET_N_FROM_EE      setbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,  (1 << 1))
#define ENABLE_AHB_BUS_FROM_EE      setbits_le32(P_AO_RTI_PWR_CNTL_REG0 ,  (1 << 0))

#define TICK_OF_ONE_SECOND 32000

#define dbg_out(s,v) serial_puts(s);serial_put_hex(v,32);serial_putc('\n');

static void timer_init()
{
	//100uS stick timer a mode : periodic, timer a enable, timer e enable
    setbits_le32(P_AO_TIMER_REG,0x1f);
}

unsigned  get_tick(unsigned base)
{
    return readl(P_AO_TIMERE_REG)-base;
}

unsigned t_delay_tick(unsigned count)
{
    unsigned base=get_tick(0);
    if(readl(P_AO_RTI_PWR_CNTL_REG0)&(1<<8)){
        while(get_tick(base)<count);
    }else{
        while(get_tick(base)<count*100);
    }
    return 0;
}

unsigned delay_tick(unsigned count)
{
    unsigned i,j;
    for(i=0;i<count;i++)
    {
        for(j=0;j<1000;j++)
        {
            asm("mov r0,r0");
            asm("mov r0,r0");
        }
    }
}

void delay_ms(int ms)
{
		while(ms > 0){
		delay_tick(32);
		ms--;
	}
}

#define delay_1s() delay_tick(TICK_OF_ONE_SECOND);

//volatile unsigned * arm_base=(volatile unsigned *)0x8000;
void copy_reboot_code()
{
	int i;
	int code_size;
	volatile unsigned char* pcode = (volatile unsigned char*)arm_reboot;
  volatile unsigned char * arm_base = (volatile unsigned char *)0x0000;
	code_size = sizeof(arm_reboot);
	//copy new code for ARM restart
	for(i = 0; i < code_size; i++)
	{
		if(i != 32 && i != 33 && i != 34 && i != 35) //skip firmware's reboot entry address.
				*arm_base = *pcode;
		pcode++;
		arm_base++;
	}
}

void power_off_vddio();
#define POWER_OFF_VDDIO
#define POWER_OFF_HDMI_VCC
#define POWER_OFF_AVDD33
#define POWER_OFF_AVDD25
#define POWER_DOWN_VCC12
#define POWER_DOWN_DDR
//#define POWER_OFF_EE
void enter_power_down()
{
	int i;
	unsigned v1,v2,v;
	unsigned rtc_ctrl;
	unsigned power_key;
	//*******************************************
	//*  power down flow  
	//*******************************************
	f_serial_puts("\n");
	wait_uart_empty();
	// disable all memory accesses.
    disable_mmc_req();
   
    //save registers for clk and ddr
    store_restore_plls(1);
    
    //mmc enter sleep
    mmc_sleep();
//    delay_ms(20);
    
    // save ddr power
    APB_Wr(MMC_PHY_CTRL, APB_Rd(MMC_PHY_CTRL)|(1<<0)|(1<<8)|(1<<13));
    APB_Wr(PCTL_PHYCR_ADDR, APB_Rd(PCTL_PHYCR_ADDR)|(1<<6));
    APB_Wr(PCTL_DLLCR9_ADDR, APB_Rd(PCTL_DLLCR9_ADDR)|(1<<31));
// 	  delay_ms(20);

 	// power down DDR
 	writel(readl(P_HHI_DDR_PLL_CNTL)|(1<<15),P_HHI_DDR_PLL_CNTL);

	// enable retention
	enable_retention();

	writel(0,P_AO_RTI_STATUS_REG1);

 	// reset A9
	setbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
	 
	// enable iso ee for A9
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<4)),P_AO_RTI_PWR_CNTL_REG0);

#ifdef POWER_OFF_HDMI_VCC
	reg7_off();
#endif
#ifdef POWER_OFF_AVDD33
	reg5_off();
#endif

#ifdef POWER_OFF_EE 
	//iso EE from AO
	//comment isolate EE. otherwise cannot detect power key.
	// writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<0)),P_AO_RTI_PWR_CNTL_REG0); 
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<2)),P_AO_RTI_PWR_CNTL_REG0);
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<3)),P_AO_RTI_PWR_CNTL_REG0);

	//?? Gate off clk81 to EE domain
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)&(~(1<<12)),P_AO_RTI_GEN_CNTL_REG0);
	//-------------------------------
	//turn off EE voltage
	//v = readl(0xC8100024);
	//v &= ~(1<<9);
	//v &= ~(1<<25);
	//writel(v,0xC8100024);
#else
	// ee use 32k
	writel(readl(P_HHI_MPEG_CLK_CNTL)|(1<<9),P_HHI_MPEG_CLK_CNTL);
#endif

	
	// change RTC filter for 32k
  rtc_ctrl = readl(0xC810074c);
	writel(0x00800000,0xC810074c);
	// switch to 32k
    writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<8),P_AO_RTI_PWR_CNTL_REG0);
    udelay(100);
#ifdef POWER_OFF_VDDIO 
	vddio_off(); 
#endif		
#ifdef POWER_OFF_AVDD25
	reg6_off();
#endif
	udelay(100);
#if (defined(POWER_DOWN_VCC12) || defined(POWER_DOWN_DDR))
	switch_voltage(1);
#endif
#ifdef POWER_DOWN_DDR
	powerdown_ddr();
#endif
#ifdef POWER_DOWN_VCC12
	powerdown_vcc12();
#endif

	// gate off REMOTE, UART
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)&(~(0xF)),P_AO_RTI_GEN_CNTL_REG0);
	// wait key
    power_key = readl(0Xc8100744);
    #if 1
    while (((power_key&4) != 0)&&((power_key&8) == 0))
   {
     	power_key = readl(0Xc8100744);
   }
   #else
    for(i=0;i<64;i++)
    {
        udelay(1000);
        //udelay(1000);
    }
   #endif
    
	// gate on REMOTE, I2C s/m, UART
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)|0xF, P_AO_RTI_GEN_CNTL_REG0); 
	udelay(10);
#ifdef POWER_DOWN_DDR
	powerup_ddr();
#endif
#ifdef POWER_DOWN_VCC12
	powerup_vcc12();
#endif
#if (defined(POWER_DOWN_VCC12) || defined(POWER_DOWN_DDR))
	switch_voltage(0);
#endif

#ifdef POWER_OFF_AVDD25
	reg6_on();
#endif
#ifdef POWER_OFF_VDDIO 
	vddio_on();
#endif
	udelay(100);
   // switch to clk81 
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(0x1<<8)),P_AO_RTI_PWR_CNTL_REG0);
	udelay(100);
	// restore RTC filter
	writel(rtc_ctrl,0xC810074c);

	// set AO interrupt mask
	writel(0xFFFF,P_AO_IRQ_STAT_CLR);
	
#ifdef POWER_OFF_EE
	//turn on EE voltage
	//v = readl(0xC8100024);
	//v &= ~(1<<9);
	//v |= (1<<25);
	//writel(v,0xC8100024);
	//delay_ms(200);

	// un-iso AO domain from EE bit0=signals, bit1=reset, bit2=irq, bit3=test_mode
 	writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(0xD<<0),P_AO_RTI_PWR_CNTL_REG0);

	//un isolate the reset in the EE
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(0x1<<5),P_AO_RTI_PWR_CNTL_REG0);

	writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(0x1<<5)|(1<<3)|(1<<2)|(1<<1)|(1<<0), \
			   P_AO_RTI_PWR_CNTL_REG0);
#else
    // ee go back to clk81
	writel(readl(P_HHI_MPEG_CLK_CNTL)&(~(0x1<<9)),P_HHI_MPEG_CLK_CNTL);
#endif
	
#ifdef POWER_OFF_AVDD33
	reg5_on();
#endif
#ifdef POWER_OFF_HDMI_VCC
	reg7_on();
#endif    

    store_restore_plls(0);
     
    init_ddr_pll();
    
		uart_reset();

    reset_mmc();

    // initialize mmc and put it to sleep
    init_pctl();

    mmc_sleep();
    
    // disable retention
    disable_retention();

    // Next, we wake up
    mmc_wakeup();

    // Next, we enable all requests
    enable_mmc_req();

//	f_serial_puts("restart arm...\n");
	
	//0. make sure a9 reset
	setbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
		
	//1. write flag
	if (power_key&8)
		writel(0xabcd1234,P_AO_RTI_STATUS_REG2);
	else
		writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
	
	//2. remap AHB SRAM
	writel(3,P_AO_REMAP_REG0);
	writel(2,P_AHB_ARBDEC_REG);
 
	//3. turn off romboot clock
	writel(readl(P_HHI_GCLK_MPEG1)&0x7fffffff,P_HHI_GCLK_MPEG1);
 
	//4. Release ISO for A9 domain.
	setbits_le32(P_AO_RTI_PWR_CNTL_REG0,1<<4);

	//reset A9
	writel(0xF,P_RESET4_REGISTER);// -- reset arm.ww
	writel(1<<14,P_RESET2_REGISTER);// -- reset arm.mali
	delay_ms(1);
	clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19); // release A9 reset
  
//	delay_1s();
//	delay_1s();
//	delay_1s();
}

//#define ART_CORE_TEST
#ifdef ART_CORE_TEST
void test_arc_core()
{
    int i;
    int j,k;
    unsigned int power_key=0;
    
    for(i=0;i<1000;i++)
    {
        asm("mov r0,r0");
        asm("mov r0,r0");
        //udelay(1000);
        //udelay(1000);
        
    }
    
    
	f_serial_puts("\n");
	wait_uart_empty();

    writel(0,P_AO_RTI_STATUS_REG1);    

 	// reset A9 clock
	//setbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);

	// enable iso ee for A9
	//writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<4)),P_AO_RTI_PWR_CNTL_REG0);


	// wait key
    power_key = readl(0Xc8100744);
    
    f_serial_puts("get power_key\n");
    #if 0
    while (((power_key&4) != 0)&&((power_key&8) == 0))
   {
     	power_key = readl(0Xc8100744);
   }
   #else
    for(i=0;i<1000;i++)
    {
        for(j=0;j<1000;j++)
        {
            for(k=0;k<100;k++)
            {
                asm("mov r0,r0");
            }
        }
        //udelay(1000);
        //udelay(1000);
        
    }
   #endif

    f_serial_puts("delay 2s\n");

	//0. make sure a9 reset
//	setbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);

#if 0
	//1. write flag
	if (power_key&8)
		writel(0xabcd1234,P_AO_RTI_STATUS_REG2);
	else
		writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
#endif
	//2. remap AHB SRAM
	writel(3,P_AO_REMAP_REG0);
	writel(2,P_AHB_ARBDEC_REG);
	
	f_serial_puts("remap arm arc\n");

	//3. turn off romboot clock
	writel(readl(P_HHI_GCLK_MPEG1)&0x7fffffff,P_HHI_GCLK_MPEG1);
	
	f_serial_puts("off romboot clock\n");

	//4. Release ISO for A9 domain.
	//setbits_le32(P_AO_RTI_PWR_CNTL_REG0,1<<4);

	//reset A9
	writel(0xF,P_RESET4_REGISTER);// -- reset arm.ww
//	writel(1<<14,P_RESET2_REGISTER);// -- reset arm.mali
	delay_ms(1);
//	clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19); // release A9 reset

//	f_serial_puts("arm reboot\n");
//	wait_uart_empty();
    f_serial_puts("arm reboot\n");

}
#endif
#define _UART_DEBUG_COMMUNICATION_
int main(void)
{
	unsigned cmd;
	char c;
	int i = 0,j;
	timer_init();
#ifdef POWER_OFF_VDDIO	
	f_serial_puts("sleep ... off\n");
#else
	f_serial_puts("sleep .......\n");
#endif
		
	while(1){
		
		cmd = readl(P_AO_RTI_STATUS_REG0);
		if(cmd == 0)
		{
			delay_ms(10);
			continue;
		}
		c = (char)cmd;
		if(c == 't')
		{
#if (defined(POWER_OFF_VDDIO) || defined(POWER_OFF_HDMI_VCC) || defined(POWER_OFF_AVDD33) || defined(POWER_OFF_AVDD25))
			init_I2C();
#endif
			copy_reboot_code();
			enter_power_down();
			//test_arc_core();
			break;
		}
		else if(c == 'q')
		{
				serial_puts(" - quit command loop\n");
				writel(0,P_AO_RTI_STATUS_REG0);
			  break;
		}
		else
		{
				serial_puts(" - cmd no support (ARC)\n");
		}
		//command executed
		writel(0,P_AO_RTI_STATUS_REG0);
	}
	
	while(1){
	    udelay(6000);
	    cmd = readl(P_AO_RTI_STATUS_REG1);
	    c = (char)cmd;
	    if(c == 0)
	    {
	        udelay(6000);
	        cmd = readl(P_AO_RTI_STATUS_REG1);
	        c = (char)cmd;
	        if((c == 0)||(c!='r'))
	        {
	            #ifdef _UART_DEBUG_COMMUNICATION_
	            serial_put_hex(cmd,32);
	            f_serial_puts(" arm boot fail\n\n");
	            wait_uart_empty();
	            #endif
	            #if 0 //power down 
	            cmd = readl(P_AO_GPIO_O_EN_N);
	            cmd &= ~(1<<6);
	            cmd &= ~(1<<22);
	            writel(cmd,P_AO_GPIO_O_EN_N);
	            #endif
	        }
	    }
	    else if(c=='r')
	    {
	        writel(0,0xc8100030);
	        #ifdef _UART_DEBUG_COMMUNICATION_
	        //f_serial_puts("arm boot succ\n");
	        //wait_uart_empty();
	        #endif
	    }
	    else
	    {
	        #ifdef _UART_DEBUG_COMMUNICATION_
	        serial_put_hex(cmd,32);
	        f_serial_puts(" arm unkonw state\n");
	        wait_uart_empty();
	        #endif
	    }
	    //cmd='f';
	    //writel(cmd,P_AO_RTI_STATUS_REG1);
	    
		asm(".long 0x003f236f"); //add sync instruction.
		asm("SLEEP");
	}
	return 0;
}
unsigned clk_settings[2]={0,0};
unsigned pll_settings[2][3]={{0,0,0},{0,0,0}};
void store_restore_plls(int flag)
{
    int i;
    if(flag)
    {
#ifdef POWER_OFF_EE 
        pll_settings[0][0]=readl(P_HHI_SYS_PLL_CNTL);
        pll_settings[0][1]=readl(P_HHI_SYS_PLL_CNTL2);
        pll_settings[0][2]=readl(P_HHI_SYS_PLL_CNTL3);
		
        pll_settings[1][0]=readl(P_HHI_OTHER_PLL_CNTL);
        pll_settings[1][1]=readl(P_HHI_OTHER_PLL_CNTL2);
        pll_settings[1][2]=readl(P_HHI_OTHER_PLL_CNTL3);

        clk_settings[0]=readl(P_HHI_SYS_CPU_CLK_CNTL);
        clk_settings[1]=readl(P_HHI_MPEG_CLK_CNTL);
#endif

        save_ddr_settings();
        return;
    }    
    
#ifdef POWER_OFF_EE 
    /* restore from default settings */ 
    writel(pll_settings[0][0]|0x8000, P_HHI_SYS_PLL_CNTL);
    writel(pll_settings[0][1], P_HHI_SYS_PLL_CNTL2);
    writel(pll_settings[0][2], P_HHI_SYS_PLL_CNTL3);
    writel(pll_settings[0][0]&(~0x8000),P_HHI_SYS_PLL_CNTL);
    writel(1<<2, P_RESET5_REGISTER);

    writel(pll_settings[1][0]|0x8000, P_HHI_OTHER_PLL_CNTL);
    writel(pll_settings[1][1], P_HHI_OTHER_PLL_CNTL2);
    writel(pll_settings[1][2], P_HHI_OTHER_PLL_CNTL3);
    writel(pll_settings[1][0]&(~0x8000),P_HHI_OTHER_PLL_CNTL);
    writel(1<<1, P_RESET5_REGISTER);

    writel(clk_settings[0],P_HHI_SYS_CPU_CLK_CNTL);
    writel(clk_settings[1],P_HHI_MPEG_CLK_CNTL);	    
    delay_ms(50);
#endif
}

void __raw_writel(unsigned val,unsigned reg)
{
	(*((volatile unsigned int*)(reg)))=(val);
	asm(".long 0x003f236f"); //add sync instruction.
}

unsigned __raw_readl(unsigned reg)
{
	asm(".long 0x003f236f"); //add sync instruction.
	return (*((volatile unsigned int*)(reg)));
}
