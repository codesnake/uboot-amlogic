#include "config.h"
#include <io.h>
#include <uart.h>
#include <reg_addr.h>
#ifndef CONFIG_MESON_TRUSTZONE
#include "boot_code.dat"
#endif
#include <arc_pwr.h>

#include <pwr_op.c>
#include <linux/types.h>

static struct arc_pwr_op arc_pwr_op;
static struct arc_pwr_op *p_arc_pwr_op;


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

#define dbg_out(s,v) f_serial_puts(s);serial_put_hex(v,32);f_serial_puts("\n");wait_uart_empty();

extern void udelay(int i);
extern void wait_uart_empty();

extern void uart_reset();
extern void init_ddr_pll(void);
extern void __udelay(int n);

#if 0
static void timer_init()
{
	//100uS stick timer a mode : periodic, timer a enable, timer e enable
    setbits_le32(P_AO_TIMER_REG,0x1f);
}
#endif

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
	return count;
}

void delay_ms(int ms)
{
		while(ms > 0){
		delay_tick(32);
		ms--;
	}
}
void udelay__(int i)
{
    int delays = 0;
	if(readl(P_HHI_MPEG_CLK_CNTL)&(1<<9))//@32K
	{
	    for(delays=0;delays<i;delays++)
	    {
	        asm("mov r0,r0");
	    }
	}else{
	    for(delays=0;delays<i*24;delays++)
	    {
	        asm("mov r0,r0");
	    }
	}
}

#define delay_1s() delay_tick(TICK_OF_ONE_SECOND);

//volatile unsigned * arm_base=(volatile unsigned *)0x8000;
void copy_reboot_code()
{
	int i;
	int code_size;
#ifdef CONFIG_MESON_TRUSTZONE
	volatile unsigned char* pcode = *(int *)(0x0004);//appf_arc_code_memory[1]
	volatile unsigned char * arm_base = (volatile unsigned char *)0x0000;

	code_size = *(int *)(0x0008);//appf_arc_code_memory[2]
#else
	volatile unsigned char* pcode = (volatile unsigned char*)arm_reboot;
	volatile unsigned char * arm_base = (volatile unsigned char *)0x0000;

	code_size = sizeof(arm_reboot);
#endif
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

unsigned int PWR_A9_MEM_PD[2];
#define P_AO_RTI_PWR_A9_MEM_PD0 0xc81000F4
#define P_AO_RTI_PWR_A9_MEM_PD1 0xc81000F8

static void cpu_off()
{

//	writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<4),P_AO_RTI_PWR_CNTL_REG0);

	writel(readl(P_AO_RTI_PWR_A9_CNTL1) & (~(1 << 1)),P_AO_RTI_PWR_A9_CNTL1);

	writel(readl(P_HHI_SYS_CPU_CLK_CNTL1) | (0x1<<30),P_HHI_SYS_CPU_CLK_CNTL1);

	writel(readl(P_HHI_SYS_CPU_CLK_CNTL) | (0x7<<16) | (0x7<<28),P_HHI_SYS_CPU_CLK_CNTL);
	
	udelay__(100);
	//A9_TOPCLAMP
	writel(readl(P_AO_RTI_PWR_A9_CNTL0) | (1<<13),P_AO_RTI_PWR_A9_CNTL0);
	udelay__(100);
	// l2 sram sleep
	writel(readl(P_AO_RTI_PWR_A9_CNTL1) | (0x1 << 0),P_AO_RTI_PWR_A9_CNTL1);

	PWR_A9_MEM_PD[1] = readl(P_AO_RTI_PWR_A9_MEM_PD1);
	writel(readl(P_AO_RTI_PWR_A9_MEM_PD1) | 0xf, P_AO_RTI_PWR_A9_MEM_PD1);
	PWR_A9_MEM_PD[0] = readl(P_AO_RTI_PWR_A9_MEM_PD0);
	writel(readl(P_AO_RTI_PWR_A9_MEM_PD0) | (0x3ffff << 0), P_AO_RTI_PWR_A9_MEM_PD0);

	writel(readl(P_AO_RTI_PWR_A9_CNTL1) | (0x3 << 2),P_AO_RTI_PWR_A9_CNTL1);
}

void restart_arm()
{
	writel(readl(P_AO_RTI_PWR_A9_CNTL1) & (~(0x3 << 2)),P_AO_RTI_PWR_A9_CNTL1);

	writel(PWR_A9_MEM_PD[0], P_AO_RTI_PWR_A9_MEM_PD0);
	writel(PWR_A9_MEM_PD[1], P_AO_RTI_PWR_A9_MEM_PD1);
// l2 sram sleep
	writel(readl(P_AO_RTI_PWR_A9_CNTL1) & (~(0x1 << 0)),P_AO_RTI_PWR_A9_CNTL1);

	writel(readl(P_RESET4_REGISTER) | (1<<0) | (1<<3), P_RESET4_REGISTER);
    writel(readl(P_RESET2_REGISTER) | (1<<12),  P_RESET2_REGISTER);
    writel(readl(P_RESET3_REGISTER) | (1<<7), P_RESET3_REGISTER);
	
// enable rom boot clock
	writel(readl(P_HHI_GCLK_MPEG1) | (1<<31), P_HHI_GCLK_MPEG1);	
// Remap AHB-Lite so AHB_SRAM is at address 0x0000
	writel(0x0001,P_AHB_ARBDEC_REG);

//signals from AO domain to the Everything Else domain are isolated
//	writel(readl(P_AO_RTI_PWR_CNTL_REG0)& (~(1<<4)),P_AO_RTI_PWR_CNTL_REG0);
//	A9_TOPCLAMP
	writel(readl(P_AO_RTI_PWR_A9_CNTL0) & (~(1<<13)),P_AO_RTI_PWR_A9_CNTL0);

// release reset assert on u_axi_matrix (pl301)
// reset u_axi_matrix
	writel(readl(P_AO_RTI_PWR_A9_CNTL1) | (1<<1), P_AO_RTI_PWR_A9_CNTL1);

// deassert reset
	writel(readl(P_HHI_SYS_CPU_CLK_CNTL) & (~((0x7<<16) | (0x7<<28))), P_HHI_SYS_CPU_CLK_CNTL);
	writel(readl(P_HHI_SYS_CPU_CLK_CNTL1) & (~(0x1<<30)),P_HHI_SYS_CPU_CLK_CNTL1);
}

static void switch_to_rtc()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<8),P_AO_RTI_PWR_CNTL_REG0);
}
static void switch_to_81()
{
	 writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<8)),P_AO_RTI_PWR_CNTL_REG0);
}



inline void switch_24M_to_32K(void)
{
	// ee use 32k, So interrup status can be accessed.
	writel(readl(P_HHI_MPEG_CLK_CNTL)|(1<<9),P_HHI_MPEG_CLK_CNTL);	
	switch_to_rtc();
	udelay__(100);
}

inline void switch_32K_to_24M(void)
{
	switch_to_81();
	// ee go back to clk81
	writel(readl(P_HHI_MPEG_CLK_CNTL)&(~(0x1<<9)),P_HHI_MPEG_CLK_CNTL);
	udelay__(100);
}

#define v_outs(s,v) {f_serial_puts(s);serial_put_hex(v,32);f_serial_puts("\n"); wait_uart_empty();}

#define pwr_ddr_off 
void enter_power_down()
{
	//int i;
	unsigned int uboot_cmd_flag=readl(P_AO_RTI_STATUS_REG2);//u-boot suspend cmd flag
	unsigned int vcin_state = 0;

    //int voltage   = 0;
    //int axp_ocv = 0;
	int wdt_flag;
	// First, we disable all memory accesses.

	f_serial_puts("step 1\n");

	asm(".long 0x003f236f"); //add sync instruction.

	store_restore_plls(0);

	f_serial_puts("ddr self-refresh\n");
	wait_uart_empty();

	ddr_self_refresh();

 	f_serial_puts("CPU off...\n");
 	wait_uart_empty();
	cpu_off();
	f_serial_puts("CPU off done\n");
	wait_uart_empty();
 	if(p_arc_pwr_op->power_off_at_24M)
		p_arc_pwr_op->power_off_at_24M();


//	while(readl(0xc8100000) != 0x13151719)
//	{}

	switch_24M_to_32K();

	if(p_arc_pwr_op->power_off_at_32K_1)
		p_arc_pwr_op->power_off_at_32K_1();

	if(p_arc_pwr_op->power_off_at_32K_2)
		p_arc_pwr_op->power_off_at_32K_2();

	// gate off:  bit0: REMOTE;   bit3: UART
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)&(~(0x8)),P_AO_RTI_GEN_CNTL_REG0);

	if(uboot_cmd_flag == 0x87654321)//u-boot suspend cmd flag
	{
		if(p_arc_pwr_op->power_off_ddr15)
			p_arc_pwr_op->power_off_ddr15();
	}
	wdt_flag=readl(P_WATCHDOG_TC)&(1<<19);
	if(wdt_flag)
		writel(readl(P_WATCHDOG_TC)&(~(1<<19)),P_WATCHDOG_TC);
#if 1
	vcin_state = p_arc_pwr_op->detect_key(uboot_cmd_flag);
#else
	for(i=0;i<10;i++)
	{
		udelay__(1000);
		//udelay(1000);
	}
#endif

	if(uboot_cmd_flag == 0x87654321)//u-boot suspend cmd flag
	{
		if(p_arc_pwr_op->power_on_ddr15)
			p_arc_pwr_op->power_on_ddr15();
	}
	if(wdt_flag)
		writel((6*7812|((1<<16)-1))|(1<<19),P_WATCHDOG_TC);

// gate on:  bit0: REMOTE;   bit3: UART
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)|0x8,P_AO_RTI_GEN_CNTL_REG0);

	if(p_arc_pwr_op->power_on_at_32K_2)
		p_arc_pwr_op->power_on_at_32K_2();

	if(p_arc_pwr_op->power_on_at_32K_1)
		p_arc_pwr_op->power_on_at_32K_1();


	switch_32K_to_24M();


	// power on even more domains
	if(p_arc_pwr_op->power_on_at_24M)
		p_arc_pwr_op->power_on_at_24M();

 	uart_reset();
	f_serial_puts("step 8: ddr resume\n");
	wait_uart_empty();
	ddr_resume();

	f_serial_puts("restore pll\n");
	wait_uart_empty();
	store_restore_plls(1);//Before switch back to clk81, we need set PLL

    if (uboot_cmd_flag == 0x87654321 && (vcin_state == FLAG_WAKEUP_PWROFF)) {
        /*
         * power off system before ARM is restarted
         */
        f_serial_puts("no extern power shutdown\n");
	    wait_uart_empty();
        p_arc_pwr_op->shut_down();
        do{
            udelay__(2000 * 100);
            f_serial_puts("wait shutdown...\n");
            wait_uart_empty();
        }while(1);
    }

	writel(vcin_state,P_AO_RTI_STATUS_REG2);
	f_serial_puts("restart arm\n");
	wait_uart_empty();
	restart_arm();

    if (uboot_cmd_flag == 0x87654321) {
        writel(0,P_AO_RTI_STATUS_REG2);
        writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<4),P_AO_RTI_PWR_CNTL_REG0);
        clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
        //writel(10,0xc1109904);
        writel(1<<19|1<<24|10,0xc1109900);
        
        do{udelay__(200);f_serial_puts("wait reset...\n");wait_uart_empty();}while(1);
    }
}


//#define ART_CORE_TEST

struct ARC_PARAM *arc_param=(struct ARC_PARAM *)ARC_PARAM_ADDR;//

#define _UART_DEBUG_COMMUNICATION_

int main(void)
{
	unsigned cmd;
	char c;
	p_arc_pwr_op = &arc_pwr_op;
//	timer_init();
	arc_pwr_register((struct arc_pwr_op *)p_arc_pwr_op);//init arc_pwr_op
	writel(0,P_AO_RTI_STATUS_REG1);
	f_serial_puts("sleep .......\n");
	arc_param->serial_disable=0;

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
			init_I2C();
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

	
//	asm("SLEEP");


	while(1){
//	    udelay__(600);
	    cmd = readl(P_AO_RTI_STATUS_REG1);
	    c = (char)cmd;
//		f_serial_puts("REG2=");


//		serial_put_hex(readl(P_AO_RTI_STATUS_REG2),32);

	    if(c == 0)
	    {
	        udelay__(6000);
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
	        f_serial_puts("arm boot succ\n");
	        wait_uart_empty();
				    
		asm(".long 0x003f236f"); //add sync instruction.
		asm("SLEEP");
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

	}
	return 0;
}

unsigned pll_settings[5];
unsigned mpll_settings[10];


#define CONFIG_SYS_PLL_SAVE
void store_restore_plls(int flag)
{
    int i;

    if(!flag)
    {
		pll_settings[0]=readl(P_HHI_SYS_PLL_CNTL);
		pll_settings[1]=readl(P_HHI_SYS_PLL_CNTL2);
		pll_settings[2]=readl(P_HHI_SYS_PLL_CNTL3);
		pll_settings[3]=readl(P_HHI_SYS_PLL_CNTL4);
		pll_settings[4]=readl(P_HHI_SYS_PLL_CNTL5);

		for(i=0;i<10;i++)//store mpll
			mpll_settings[i]=readl(P_HHI_MPLL_CNTL + 4*i);

		//disable sys pll & mpll
		writel(readl(P_HHI_SYS_PLL_CNTL) & (~(1<<30)),P_HHI_SYS_PLL_CNTL);
		writel(readl(P_HHI_MPLL_CNTL) & (~(1<<30)),P_HHI_MPLL_CNTL);

		return;
    }    
    
#define M8_PLL_ENTER_RESET(pll) \
	writel((1<<29), pll);

#define M8_PLL_RELEASE_RESET(pll) \
	writel(readl(pll)&(~(1<<29)),pll);

//M8 PLL enable: bit 30 ; 1-> enable;0-> disable
#define M8_PLL_SETUP(set, pll) \
	writel((set) |(1<<29) |(1<<30), pll);\
	udelay__(1000); //wait 1ms for PLL lock

//wait for pll lock
//must wait first (100us+) then polling lock bit to check
#define M8_PLL_WAIT_FOR_LOCK(pll) \
	do{\
		udelay__(1000);\
			f_serial_puts(" Lock mpll\n");	\
			wait_uart_empty();	\
	}while((readl(pll)&0x80000000)==0);


	writel(readl(P_AM_ANALOG_TOP_REG1)|1, P_AM_ANALOG_TOP_REG1);

	//bandgap enable
	//SYS,MPLL
	writel(readl(P_HHI_MPLL_CNTL6)|(1<<26), P_HHI_MPLL_CNTL6);
	//VID,HDMI
	writel(readl(P_HHI_VID_PLL_CNTL5)|(1<<16), P_HHI_VID_PLL_CNTL5);
	//DDR
//	writel(readl(0xc8000410)|(1<<12),0xc8000410);

	//???
//	writel(readl(P_HHI_MPEG_CLK_CNTL)&(~(1<<8)), P_HHI_MPEG_CLK_CNTL);
//	udelay__(10);

	//PLL setup: bandgap enable -> 1ms delay -> PLL reset + PLL set ->
	//                 PLL enable -> 1ms delay -> PLL release from reset

	//SYS PLL init
	do{
		//BANDGAP reset for SYS_PLL,MPLL lock fail		
		writel_reg_bits(P_HHI_MPLL_CNTL6,0,26,1);
		udelay__(10);
		writel_reg_bits(P_HHI_MPLL_CNTL6,1,26,1);
		udelay__(1000); //1ms for bandgap bootup

		M8_PLL_ENTER_RESET(P_HHI_SYS_PLL_CNTL);
		writel(pll_settings[1],P_HHI_SYS_PLL_CNTL2);
		writel(pll_settings[2],P_HHI_SYS_PLL_CNTL3);
		writel(pll_settings[3],P_HHI_SYS_PLL_CNTL4);
		writel(pll_settings[4],P_HHI_SYS_PLL_CNTL5);

		M8_PLL_SETUP(pll_settings[0], P_HHI_SYS_PLL_CNTL);
		M8_PLL_RELEASE_RESET(P_HHI_SYS_PLL_CNTL);

		if(arc_param->serial_disable)
			udelay__(1000);
		else
		{
			f_serial_puts(" Lock sys pll\n");
			wait_uart_empty();
		}

	}while((readl(P_HHI_SYS_PLL_CNTL)&(1<<31))==0);


	//MPLL init
	//FIXED PLL/Multi-phase PLL, fixed to 2.55GHz
	M8_PLL_ENTER_RESET(P_HHI_MPLL_CNTL);	//set reset bit to 1

	for(i=1;i<10;i++)
		writel(mpll_settings[i],P_HHI_MPLL_CNTL+4*i);

	M8_PLL_SETUP(mpll_settings[0], P_HHI_MPLL_CNTL);	//2.55G, FIXED
	M8_PLL_RELEASE_RESET(P_HHI_MPLL_CNTL);	//set reset bit to 0

//	M8_PLL_WAIT_FOR_LOCK(P_HHI_MPLL_CNTL); //need bandgap reset?

	do{
		if(arc_param->serial_disable)
			udelay__(1000);
		else
		{
			udelay__(500);
			f_serial_puts(" Lock mpll~\n");
			wait_uart_empty();
		}
		//M8_PLL_LOCK_CHECK(n_pll_try_times,5);
		if((readl(P_HHI_MPLL_CNTL)&(1<<31))!=0)
			break;
		writel(readl(P_HHI_MPLL_CNTL) | (1<<29), P_HHI_MPLL_CNTL);
		udelay__(500);
		M8_PLL_RELEASE_RESET(P_HHI_MPLL_CNTL);
		udelay__(500);
	}while((readl(P_HHI_MPLL_CNTL)&(1<<31))==0);

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

