#include <config.h>
//#include <asm/arch/io.h>
#include <asm/arch/uart.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/pctl.h>
#include <asm/arch/dmc.h>
#include <asm/arch/ddr.h>
#include <asm/arch/memtest.h>
#include <asm/arch/pctl.h>
//#include <asm/arch/c_always_on_pointer.h>
#include "boot_code.dat"

//#define CONFIG_ARC_SARDAC_ENABLE
#ifdef CONFIG_ARC_SARDAC_ENABLE
#include "sardac_arc.c"
#endif


#define CONFIG_IR_REMOTE_WAKEUP 1//for M6 MBox

#ifdef CONFIG_IR_REMOTE_WAKEUP
#include "irremote2arc.c"
#endif

//----------------------------------------------------
unsigned UART_CONFIG_24M= (200000000/(115200*4)  );
unsigned UART_CONFIG= (32*1000/(300*4));
//#define EN_DEBUG
//----------------------------------------------------
//functions declare

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

//#define f_serial_puts(a)
//#define serial_put_hex(a,b)
//#define wait_uart_empty()
//#define udelay(a)

extern void udelay(int i);
extern void wait_uart_empty();
extern int check_all_regulators(void);
extern void power_down_ddr_phy(void);
extern inline void power_off_at_24M();
extern inline void power_off_at_32K_1();
extern inline void power_off_at_32K_2();
extern void power_off_ddr15(void);
extern unsigned char get_charging_state();
extern void power_on_ddr15(void);
extern inline void power_on_at_32k_2();
extern inline void power_on_at_32k_1();
extern inline void power_on_at_24M();
extern void uart_reset();
extern void init_ddr_pll(void);
extern void store_vid_pll(void);
extern void shut_down();
extern void __udelay(int n);
extern void init_I2C();
extern void hx_save_ddr_settings();
extern void hx_enter_power_down();
extern void hx_leave_power_down();

#define dbg_out(s,v) f_serial_puts(s);serial_put_hex(v,32);f_serial_puts('\n');wait_uart_empty();

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
	return 0;
}

void delay_ms(int ms)
{
		while(ms > 0){
		delay_tick(32);
		ms--;
	}
}

void udelay(int i)
{
    int delays = 0;
    for(delays=0;delays<i;delays++)
    {
        asm("mov r0,r0");
    }
}

#define __udelay(a)	\
	udelay(24*a)

static void save_pll(void);
static void store_pll(void);

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
/*	 	f_serial_puts("[ ");
		serial_put_hex(*arm_base,8);
	 	f_serial_puts(" , ");
		serial_put_hex(*pcode,8);
	 	f_serial_puts(" ]  ");
*/	 	
		
		if(i != 32 && i != 33 && i != 34 && i != 35) //skip firmware's reboot entry address.
				*arm_base = *pcode;
		pcode++;
		arm_base++;
	}
}

#if 0
static void enable_iso_ee()
{
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<4)),P_AO_RTI_PWR_CNTL_REG0);
}
static void disable_iso_ee()
{
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<4),P_AO_RTI_PWR_CNTL_REG0);
}
#endif

static void cpu_off()
{
	writel(readl(P_HHI_SYS_CPU_CLK_CNTL)|(1<<19),P_HHI_SYS_CPU_CLK_CNTL);
}

#if 0
static void switch_to_rtc()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<8),P_AO_RTI_PWR_CNTL_REG0);
   udelay(100);
}
static void switch_to_81()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<8)),P_AO_RTI_PWR_CNTL_REG0);
   udelay(100);
}
static void enable_iso_ao()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(0xF<<0)),P_AO_RTI_PWR_CNTL_REG0);
}
static void disable_iso_ao()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(0xD<<0),P_AO_RTI_PWR_CNTL_REG0);
}
static void ee_off()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(0x1<<9)),P_AO_RTI_PWR_CNTL_REG0);
}
static void ee_on()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(0x1<<9),P_AO_RTI_PWR_CNTL_REG0);
}
#endif

static void switch_out_32k()
{
/*
	switch_to_81();
	// ee go back to clk81
	writel(readl(P_HHI_MPEG_CLK_CNTL)&(~(0x1<<9)),P_HHI_MPEG_CLK_CNTL);
	udelay(10000);
*/
}
static void switch_in_32k()
{
	// ee use 32k, So interrup status can be accessed.
/*	writel(readl(P_HHI_MPEG_CLK_CNTL)|(1<<9),P_HHI_MPEG_CLK_CNTL);
	switch_to_rtc();
	udelay(1000);
	*/
}
extern void power_off_at_24M();
extern void power_on_at_24M();
extern void power_off_at_32k();
extern void power_on_at_32k();

#if 1
void restart_arm()
{
	// Set up conditions on the ports of the A9 BEFORE releasing the
	// isolation clamps (bit[4] of AO_RTI_PWR_CNTL_REG0)
	// Start the A9 using the Crystal as the master clock
	writel(readl(P_HHI_SYS_CPU_CLK_CNTL) & ~(1<<7),P_HHI_SYS_CPU_CLK_CNTL);
	// Set the soft reset of the A9
	setbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
	// Set the soft resets of the APB, AXI and A9 Debug module
	// bits[18] AT soft reset
	// bits[17] APB soft reset
	// bits[16] AXI soft reset
	setbits_le32( P_A9_CFG2,7<<16);
	// 1. write flag
	writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
	// 2. remap AHB SRAM
	writel(1,P_AHB_ARBDEC_REG);
	// 3. turn off romboot clock (not sure this is necessary but saves power)
	writel(readl(P_HHI_GCLK_MPEG1)&0x7fffffff,P_HHI_GCLK_MPEG1);
	// Release ISO for A9 domain. This enables the controls from the EE domain
	// into the A9 domain.
	setbits_le32(P_AO_RTI_PWR_CNTL_REG0,1<<4);
	// Delay a little to let things stabilize
	udelay(100);
	// Reset the Clock dividers associated with the A9. 1 = reset
	// [15] general divider soft reset
	// [14] Soft reset for the AXI, APB, AT, and peripheral clock dividers
	setbits_le32( P_HHI_SYS_CPU_CLK_CNTL, (0x3 << 14) );
	udelay(10);
	clrbits_le32( P_HHI_SYS_CPU_CLK_CNTL, (0x3 << 14) );
	udelay(100);
	// Asynchronously reset the A9, APB, AT and AXI modules
	writel(0xF, P_RESET4_REGISTER);
	// Reset the Mali
	writel(1<<14,P_RESET2_REGISTER);
	// Release all soft resets except the A9
	clrbits_le32(P_A9_CFG2,7<<16);
	udelay(100);
	// Release the A9 to start the booting process
	clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
}

#endif
#define v_outs(s,v) {f_serial_puts(s);serial_put_hex(v,32);f_serial_puts("\n"); wait_uart_empty();}


#define pwr_ddr_off 
#define POWER_OFF_24M
//#define POWER_OFF_32K

void enter_power_down()
{
	unsigned int uboot_cmd_flag=readl(P_AO_RTI_STATUS_REG2);//u-boot suspend cmd flag
	unsigned power_key;
	//	disp_pctl();
	//	test_ddr(0);
	// First, we disable all memory accesses.
#ifdef pwr_ddr_off
	f_serial_puts("step 1: DDR enter self-refresh\n");	
 	wait_uart_empty();
	//DDR save setting
	hx_save_ddr_settings();	
	//DDR suspend
	hx_enter_power_down();
#endif

 	f_serial_puts("step 2: CPU off\n");
 	wait_uart_empty();
	cpu_off();

	f_serial_puts("step 3: store pll\n");
 	wait_uart_empty();
	//store_restore_plls(1);
	
	save_pll();
#ifdef POWER_OFF_24M
	f_serial_puts("step 4: power off domain\n");
	wait_uart_empty();
	power_off_at_24M();
#endif
	
	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<4)),P_AO_RTI_PWR_CNTL_REG0);

	switch_in_32k();
	
#ifdef POWER_OFF_32K
	power_off_at_32k();
#endif
	// gate off UART
	writel(readl(P_AO_RTI_GEN_CTNL_REG0)&(~(0x8)),P_AO_RTI_GEN_CTNL_REG0);
#ifdef CONFIG_IR_REMOTE_WAKEUP
//backup the remote config (on arm)
    backup_remote_register();
	
//	power_off_via_gpio();    
    //set the ir_remote to 32k mode at ARC
    //init_custom_trigger();
    writel(readl(P_AO_RTI_GEN_CTNL_REG0) | 0x00010000,P_AO_RTI_GEN_CTNL_REG0);
    udelay(2000);
    writel(readl(P_AO_RTI_GEN_CTNL_REG0) & 0xfffeffff,P_AO_RTI_GEN_CTNL_REG0);
    //resume ir regs
    resume_remote_register();
    	do{
		udelay(2000);
		power_key=readl(P_AO_IR_DEC_FRAME);
		  power_key = (power_key>>16)&0xff;

		  if(power_key==0x10 || power_key==0x0c || power_key==0x46){
				  writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
				  //printf("remote power key\n");
				  //the reference remote power key code
	        	  break;
		  	}
        }while((readl(0xc8100028)&0x100));
/*
	while(1){
		udelay(2000);
		power_key=readl(P_AO_IR_DEC_FRAME);
		  power_key = (power_key>>16)&0xff;

		  if(power_key==0x10 || power_key==0x0c){
				  writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
				  f_serial_puts("remote power key\n");
				  //the reference remote power key code
	        	  break;
		  	}
		}
*/
#endif
#if 0
//	udelay(200000);//Drain power
	unsigned char charging_state;
	unsigned char vcin_state;
	int i;

	if(uboot_cmd_flag == 0x87654321)//u-boot suspend cmd flag
	{
//		power_off_ddr15();

#ifdef CONFIG_ARC_SARDAC_ENABLE
		do{
			udelay(2000);
			vcin_state=get_charging_state();
			if(!vcin_state)
				break;
		}while((!(readl(0xc1109860)&0x100)) && !(get_adc_sample_in_arc(4)<0x1000));
#else
		do{
			udelay(2000);
			vcin_state=get_charging_state();
			if(!vcin_state)
				break;
		}while(!(readl(0xc1109860)&0x100));
#endif//CONFIG_ARC_SARDAC_ENABLE
//		power_on_ddr15();
	}
	else
	{
		charging_state=get_charging_state();//get state before enter polling
		do{
			udelay(200);
			if(get_charging_state() ^ charging_state)//when the state is changed, wakeup
				break;
		}while(!(readl(0xc1109860)&0x100));
	}

//	while(!(readl(0xc1109860)&0x100)){break;}
//#else
	for(i=0;i<200;i++)
   {
        udelay(1000);
        //udelay(1000);
   }
#endif


// gate on REMOTE, UART
	writel(readl(P_AO_RTI_GEN_CTNL_REG0)|0x8,P_AO_RTI_GEN_CTNL_REG0);

#ifdef POWER_OFF_32K
	power_on_at_32k();
#endif

	switch_out_32k();

#ifdef POWER_OFF_24M
	power_on_at_24M();
#endif
	
 	uart_reset();


	f_serial_puts("step 7: restore pll\n");
	wait_uart_empty();
	//store_restore_plls(0);
	store_pll();

	/*Need reset pwm*/
	writel((readl(P_PWM_MISC_REG_AB)& ~(0x7f << 16)) | ((1 << 23) | (0 << 16) | (1 << 1)),P_PWM_MISC_REG_AB);
	writel(0x00000020,P_PWM_PWM_B);
	writel(readl(P_PERIPHS_PIN_MUX_2) | (1 << 1),P_PERIPHS_PIN_MUX_2);
	
#ifdef pwr_ddr_off    
	f_serial_puts("step 8: resume ddr\n");
	wait_uart_empty();
	hx_leave_power_down();
#endif   //pwr_ddr_off

	if(uboot_cmd_flag == 0x87654321)//u-boot suspend cmd flag
	{
		{
//			writel(0,P_AO_RTI_STATUS_REG2);
			writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<4),P_AO_RTI_PWR_CNTL_REG0);
			clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
			writel(10,0xc1109904);
			writel(1<<22|3<<24,0xc1109900);

		    do{udelay(20000);f_serial_puts("wait reset...\n");wait_uart_empty();}while(1);
		}
	}

  // Moved the enable mmc req and SEC to ARM code.
  //enable_mmc_req();
	
	f_serial_puts("restart arm\n");
	wait_uart_empty();

	/*serial_put_hex(readl(P_VGHL_PWM_REG0),32);
	f_serial_puts("  VGHL_PWM before\n");
	wait_uart_empty();
	writel(0x631000, P_VGHL_PWM_REG0);    //enable VGHL_PWM
	udelay(1000);
*/
	restart_arm();
  
}


#define _UART_DEBUG_COMMUNICATION_
int main(void)
{
	unsigned cmd;
	char c;
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
			copy_reboot_code();
			enter_power_down();
			//test_arc_core();
			break;
		}
		else if(c == 'q')
		{
				f_serial_puts(" - quit command loop\n");
				writel(0,P_AO_RTI_STATUS_REG0);
			  break;
		}
		else
		{
				f_serial_puts(" - cmd no support (ARC)\n");
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
	        udelay(60000);
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
		} else if ( cmd == 1 )
		{
			serial_put_hex(cmd,32);
			f_serial_puts(" ARM has started running\n");
			wait_uart_empty();
		} else if ( cmd == 2 )
		{
			serial_put_hex(cmd,32);
			f_serial_puts(" Reenable SEC\n");
			wait_uart_empty();
	}
	    else if(c=='r')
	    {
	        writel(0,0xc8100030);
	        //f_serial_puts("arm boot succ\n");
	        //wait_uart_empty();
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
	    do
	    {
	       asm(".long 0x003f236f"); //add sync instruction.
	       //asm("SLEEP");
	       asm("flag 1");
	       asm("nop");
	       asm("nop");
	       asm("nop");
	    }while(1);
	}
	return 0;
}

unsigned pll_settings[4];
unsigned mpll_settings[10];
unsigned vidpll_settings[4];
unsigned clk_settings[2]={0,0};

static void save_pll(void)
{
	int i;
	
	//save sys pll
	pll_settings[0]=readl(P_HHI_SYS_PLL_CNTL);
	pll_settings[1]=readl(P_HHI_SYS_PLL_CNTL2);
	pll_settings[2]=readl(P_HHI_SYS_PLL_CNTL3);
	pll_settings[3]=readl(P_HHI_SYS_PLL_CNTL4);

	for(i=0;i<10;i++)//store mpll
	{
		mpll_settings[i]=readl(P_HHI_MPLL_CNTL + 4*i);
	}

	vidpll_settings[0]=readl(P_HHI_VID_PLL_CNTL);
	vidpll_settings[1]=readl(P_HHI_VID_PLL_CNTL2);
	vidpll_settings[2]=readl(P_HHI_VID_PLL_CNTL3);
	vidpll_settings[3]=readl(P_HHI_VID_PLL_CNTL4);
	
	clk_settings[0]=readl(P_HHI_A9_CLK_CNTL);
	clk_settings[1]=readl(P_HHI_MPEG_CLK_CNTL);
}

static void store_pll(void)
{
#define M6TV_PLL_RESET(pll) \
		Wr(pll,/*Rd(pll) |*/ (1<<29));

#define M6TV_PLL_WAIT_FOR_LOCK(pll) \
	do{\
		__udelay(1000);\
	}while((Rd(pll)&0x80000000)==0);

	//Enable PLLs pins
	//*P_AM_ANALOG_TOP_REG1 |= 0x1; // Enable DDR_PLL enable pin	
	//#define AM_ANALOG_TOP_REG1    0x206F  ->  0xC11081BC 	
//	Wr(AM_ANALOG_TOP_REG1, Rd(AM_ANALOG_TOP_REG1)|1);

	//*P_HHI_MPLL_CNTL5   |= 0x1; // Enable Both MPLL and SYS_PLL enable pin
	//move to following SYS PLL init

	Wr(HHI_MPLL_CNTL, mpll_settings[0] );
	serial_put_hex(mpll_settings[0],32);
	f_serial_puts("\n");
//		Wr(HHI_MPLL_CNTL, 0x4000067d );

	//switch a9 clock to  oscillator in the first.  This is sync mux.
  //  Wr( HHI_A9_CLK_CNTL, 0);
	//__udelay(100);

 
	do{
		//BANDGAP reset for SYS_PLL,AUD_PLL,MPLL lock fail
		//Note: once SYS PLL is up, there is no need to
		//          use AM_ANALOG_TOP_REG1 for AUD, MPLL
		//          lock fail
		Wr_reg_bits(HHI_MPLL_CNTL5,0,0,1);
		__udelay(10);
		Wr_reg_bits(HHI_MPLL_CNTL5,1,0,1);
		__udelay(1000); //1ms for bandgap bootup

		M6TV_PLL_RESET(HHI_SYS_PLL_CNTL);
		Wr(HHI_SYS_PLL_CNTL2,pll_settings[1]);
		Wr(HHI_SYS_PLL_CNTL3,pll_settings[2]);
		Wr(HHI_SYS_PLL_CNTL4,pll_settings[3]);
		Wr(HHI_SYS_PLL_CNTL, pll_settings[0] & (~(1<<30)|1<<29));
		Wr(HHI_SYS_PLL_CNTL, pll_settings[0] & ~(3<<29));
		//M6TV_PLL_WAIT_FOR_LOCK(HHI_SYS_PLL_CNTL);

		__udelay(100); //wait 100us for PLL lock
		//f_serial_puts("sys pll store done....\n");
	//wait_uart_empty();

	}while((Rd(HHI_SYS_PLL_CNTL)&0x80000000)==0);
	f_serial_puts("sys pll store done.\n");
	wait_uart_empty();
	Wr(HHI_A9_CLK_CNTL, clk_settings[0] );

	
	//A9 clock setting
//	Wr(HHI_A9_CLK_CNTL,(clk_settings[0] & (~(1<<7))));
//	__udelay(1);
	//enable A9 clock
//	Wr(HHI_A9_CLK_CNTL,(clk_settings[0] | (1<<7)));

	/*
	//AUDIO PLL
	M6TV_PLL_RESET(HHI_AUDCLK_PLL_CNTL);
	Wr(HHI_AUDCLK_PLL_CNTL2, M6TV_AUD_PLL_CNTL_2 );
	Wr(HHI_AUDCLK_PLL_CNTL3, M6TV_AUD_PLL_CNTL_3 );
	Wr(HHI_AUDCLK_PLL_CNTL4, M6TV_AUD_PLL_CNTL_4 );
	Wr(HHI_AUDCLK_PLL_CNTL5, M6TV_AUD_PLL_CNTL_5 );
	Wr(HHI_AUDCLK_PLL_CNTL6, M6TV_AUD_PLL_CNTL_6 );
	Wr(HHI_AUDCLK_PLL_CNTL,  0x20242 );
	M6TV_PLL_WAIT_FOR_LOCK(HHI_AUDCLK_PLL_CNTL);
	*/
	
	//FIXED PLL/Multi-phase PLL, fixed to 2GHz
	M6TV_PLL_RESET(HHI_MPLL_CNTL);
	Wr(HHI_MPLL_CNTL2, mpll_settings[1] );
	Wr(HHI_MPLL_CNTL3, mpll_settings[2] );
	Wr(HHI_MPLL_CNTL4, mpll_settings[3] );
	Wr(HHI_MPLL_CNTL5, mpll_settings[4] );
	Wr(HHI_MPLL_CNTL6, mpll_settings[5] );
	Wr(HHI_MPLL_CNTL7, mpll_settings[6] );
	Wr(HHI_MPLL_CNTL8, mpll_settings[7] );
	Wr(HHI_MPLL_CNTL9, mpll_settings[8] );
	Wr(HHI_MPLL_CNTL10,mpll_settings[9]);
	Wr(HHI_MPLL_CNTL, mpll_settings[0] );
	M6TV_PLL_WAIT_FOR_LOCK(HHI_MPLL_CNTL);

	//clk81=fclk_div5 /2=400/2=200M
	Wr(HHI_MPEG_CLK_CNTL, clk_settings[1] );

	f_serial_puts("mpll store done.\n");
	wait_uart_empty();

	Wr_reg_bits(AM_ANALOG_TOP_REG1,0,0,1);
	__udelay(10);
	Wr_reg_bits(AM_ANALOG_TOP_REG1,1,0,1);
	__udelay(1000); //1ms for bandgap bootup

		/*	
	//asm volatile ("wfi");
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

		M6TV_PLL_RESET(HHI_VID_PLL_CNTL);
		//Wr(HHI_VID_PLL_CNTL,  0x600b0442 ); //change VID PLL from 1.584GHz to 1.512GHz
		Wr(HHI_VID_PLL_CNTL,  vidpll_settings[0] );//change VID PLL from 1.584GHz to 1.512GHz
		Wr(HHI_VID_PLL_CNTL2, vidpll_settings[1] );
		Wr(HHI_VID_PLL_CNTL3, vidpll_settings[2] );
		Wr(HHI_VID_PLL_CNTL4, vidpll_settings[3] );
		//Wr(HHI_VID_PLL_CNTL,  0x400b0442 ); //change VID PLL from 1.584GHz to 1.512GHz
		Wr(HHI_VID_PLL_CNTL,  vidpll_settings[0] ); //change VID PLL from 1.584GHz to 1.512GHz
		//M6TV_PLL_WAIT_FOR_LOCK(HHI_VID_PLL_CNTL);

		__udelay(500); //wait 100us for PLL lock

	}while((Rd(HHI_VID_PLL_CNTL)&0x80000000)==0);
	f_serial_puts("vid pll store done.\n");
	wait_uart_empty();
*/
 	__udelay(100);

}

#define CONFIG_SYS_PLL_SAVE
void store_restore_plls(int flag)
{
    int i;
    if(flag)
    {
#ifdef CONFIG_SYS_PLL_SAVE 
		pll_settings[0]=readl(P_HHI_SYS_PLL_CNTL);
		pll_settings[1]=readl(P_HHI_SYS_PLL_CNTL2);
		pll_settings[2]=readl(P_HHI_SYS_PLL_CNTL3);
		pll_settings[3]=readl(P_HHI_SYS_PLL_CNTL4);

		for(i=0;i<10;i++)//store mpll
		{
			mpll_settings[i]=readl(P_HHI_MPLL_CNTL + 4*i);
		}
/*
		viidpll_settings[0]=readl(P_HHI_VIID_PLL_CNTL);
		viidpll_settings[1]=readl(P_HHI_VIID_PLL_CNTL2);
		viidpll_settings[2]=readl(P_HHI_VIID_PLL_CNTL3);
		viidpll_settings[3]=readl(P_HHI_VIID_PLL_CNTL4);
*/
		/*Audio PLL instead*/


		vidpll_settings[0]=readl(P_HHI_VID_PLL_CNTL);
		vidpll_settings[1]=readl(P_HHI_VID_PLL_CNTL2);
		vidpll_settings[2]=readl(P_HHI_VID_PLL_CNTL3);
		vidpll_settings[3]=readl(P_HHI_VID_PLL_CNTL4);

#endif //CONFIG_SYS_PLL_SAVE

		//save_ddr_settings();
		return;
    }    
    
#ifdef CONFIG_SYS_PLL_SAVE 

	//temp define
#define P_HHI_MPLL_CNTL5         CBUS_REG_ADDR(HHI_MPLL_CNTL5)

	do{
		//BANDGAP reset for SYS_PLL,VIID_PLL,MPLL lock fail
		//Note: once SYS PLL is up, there is no need to 
		//          use AM_ANALOG_TOP_REG1 for VIID, MPLL
		//          lock fail
		writel(readl(P_HHI_MPLL_CNTL5)&(~1),P_HHI_MPLL_CNTL5); 
		udelay(3);
		writel(readl(P_HHI_MPLL_CNTL5)|1,P_HHI_MPLL_CNTL5); 
		udelay(30); //1ms in 32k for bandgap bootup
		
		writel(1<<29,P_HHI_SYS_PLL_CNTL);		
		writel(pll_settings[1],P_HHI_SYS_PLL_CNTL2);
		writel(pll_settings[2],P_HHI_SYS_PLL_CNTL3);
		writel(pll_settings[3],P_HHI_SYS_PLL_CNTL4);
		writel((pll_settings[0] & ~(1<<30))|1<<29,P_HHI_SYS_PLL_CNTL);
		writel(pll_settings[0] & ~(3<<29),P_HHI_SYS_PLL_CNTL);
		
		//M6_PLL_WAIT_FOR_LOCK(HHI_SYS_PLL_CNTL);

		udelay(10); //wait 100us for PLL lock
	}while((readl(P_HHI_SYS_PLL_CNTL)&0x80000000)==0);

	do{
		//no need to do bandgap reset
		writel(1<<29,P_HHI_MPLL_CNTL);
		for(i=1;i<10;i++)
			writel(mpll_settings[i],P_HHI_MPLL_CNTL+4*i);

		writel((mpll_settings[0] & ~(1<<30))|1<<29,P_HHI_MPLL_CNTL);
		writel(mpll_settings[0] & ~(3<<29),P_HHI_MPLL_CNTL);
		udelay(10); //wait 200us for PLL lock		
	}while((readl(P_HHI_MPLL_CNTL)&0x80000000)==0);
/*
	do{
		//no need to do bandgap reset
		if(!(viidpll_settings[0] & 0x3fff))//M,N domain == 0, not restore vid pll
			break;
		writel(1<<29,P_HHI_VIID_PLL_CNTL);		
		writel(viidpll_settings[1],P_HHI_VIID_PLL_CNTL2);
		writel(viidpll_settings[2],P_HHI_VIID_PLL_CNTL3);
		writel(viidpll_settings[3],P_HHI_VIID_PLL_CNTL4);

		writel((viidpll_settings[0] & ~(1<<30))|1<<29,P_HHI_VIID_PLL_CNTL);
		writel(viidpll_settings[0] & ~(3<<29),P_HHI_VIID_PLL_CNTL);
		udelay(10); //wait 200us for PLL lock		
	}while((readl(P_HHI_VIID_PLL_CNTL)&0x80000000)==0);
*/
	/*Audio PLL instead*/

	udelay(3);
#endif //CONFIG_SYS_PLL_SAVE
}

void store_vid_pll()
{
	if(!(vidpll_settings[0] & 0x7fff))//M,N domain == 0, not restore vid pll
		return;
	do{
		//no need to do bandgap reset
		writel(1<<29,P_HHI_VID_PLL_CNTL);
		//writel((vidpll_settings[0] & ~(1<<30))|1<<29,P_HHI_VID_PLL_CNTL);
		writel(vidpll_settings[1],P_HHI_VID_PLL_CNTL2);
		writel(vidpll_settings[2],P_HHI_VID_PLL_CNTL3);
		writel(vidpll_settings[3],P_HHI_VID_PLL_CNTL4);

		writel((vidpll_settings[0] & ~(1<<30))|1<<29,P_HHI_VID_PLL_CNTL);
		writel(vidpll_settings[0] & ~(3<<29),P_HHI_VID_PLL_CNTL);
		udelay(24000); //wait 200us for PLL lock
	}while((readl(P_HHI_VID_PLL_CNTL)&0x80000000)==0);
}


