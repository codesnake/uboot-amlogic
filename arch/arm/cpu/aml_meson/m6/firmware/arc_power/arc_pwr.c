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

//#define CONFIG_ARC_SARDAC_ENABLE
#ifdef CONFIG_ARC_SARDAC_ENABLE
#include "sardac_arc.c"
#endif

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

#if (defined(CONFIG_AW_AXP20) || defined(CONFIG_ACT8942QJ233_PMU) || defined(CONFIG_AML_PMU))
#define PLATFORM_HAS_PMU
#endif

extern void udelay(int i);
extern void wait_uart_empty();
extern void power_down_ddr_phy(void);
#ifdef PLATFORM_HAS_PMU
extern int check_all_regulators(void);
extern inline void power_off_at_24M();
extern inline void power_off_at_32K_1();
extern inline void power_off_at_32K_2();
extern void power_off_ddr15(void);
extern unsigned char get_charging_state();
extern void power_on_ddr15(void);
extern inline void power_on_at_32k_2();
extern inline void power_on_at_32k_1();
extern inline void power_on_at_24M();
extern void shut_down();
extern void init_I2C();
#endif
extern void uart_reset();
extern void init_ddr_pll(void);
extern void store_vid_pll(void);
extern void __udelay(int n);

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
	return count;
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

void disp_code()
{
#if 0
	int i;
	int code_size;
	volatile unsigned char * arm_base = (volatile unsigned char *)0x0000;
	unsigned addr;
	code_size = sizeof(arm_reboot);
	addr = 0;
	//copy new code for ARM restart
	for(i = 0; i < code_size; i++)
	{
	 	f_serial_puts(",");
		serial_put_hex(*arm_base,8);
		if(i == 32)
			addr |= *arm_base;
		else if(i == 33)
			addr |= (*arm_base)<<8;
		else if(i == 34)
			addr |= (*arm_base)<<16;
		else if(i == 35)
			addr |= (*arm_base)<<24;
			
		arm_base++;
	}
	
	f_serial_puts("\n addr:");
	serial_put_hex(addr,32);
	
	f_serial_puts(" value:");
	wait_uart_empty();
	serial_put_hex(readl(addr),32);
	wait_uart_empty();
#endif
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
#if 0
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
void restart_arm()
{
	//------------------------------------------------------------------------
	// restart arm
		//0. make sure a9 reset
	setbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
		
	//1. write flag
	writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
	
	//2. remap AHB SRAM
//	writel(3,P_AO_REMAP_REG0);
	writel(1,P_AHB_ARBDEC_REG);
 
	//3. turn off romboot clock
	writel(readl(P_HHI_GCLK_MPEG1)&0x7fffffff,P_HHI_GCLK_MPEG1);
 
	//4. Release ISO for A9 domain.
	setbits_le32(P_AO_RTI_PWR_CNTL_REG0,1<<4);

	//use sys pll for speed up
	//clrbits_le32(P_HHI_SYS_PLL_CNTL, (1 << 30));
	//setbits_le32(P_HHI_SYS_CPU_CLK_CNTL , (1 << 7));

	//reset A9
	writel(0xF,P_RESET4_REGISTER);// -- reset arm.ww
	writel(1<<14,P_RESET2_REGISTER);// -- reset arm.mali
	delay_ms(1);
	clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19); // release A9 reset
  
 //	f_serial_puts("arm restarted ...done\n");
//	wait_uart_empty();
}
#define v_outs(s,v) {f_serial_puts(s);serial_put_hex(v,32);f_serial_puts("\n"); wait_uart_empty();}

void test_ddr(int i)
{
#if 0
	f_serial_puts("test_ddr...\n");

	volatile unsigned addr = (volatile unsigned*)0x80000000;
	unsigned v;
	if(i == 0){
		for(i = 0; i < 100; i++){
//		v_outs("addr:",pAddr);
//		v_outs("value:",*pAddr);
			writel(i,addr);
			addr+=4;
		}
	}
	else if(i == 1){
			for(i = 0; i < 100; i++){
			//	writel(i,addr);
				v = readl(addr);
			//	if(v != i)
				{
			//		serial_put_hex(addr,32);
					f_serial_puts(" , ");
					serial_put_hex(v,32);
				}
				addr+=4;
			}
	}
	f_serial_puts("\n");
	wait_uart_empty();
#endif			
}

#define pwr_ddr_off 
void enter_power_down()
{
	int i;
	unsigned int uboot_cmd_flag=readl(P_AO_RTI_STATUS_REG2);//u-boot suspend cmd flag
	unsigned char vcin_state = 0;
	unsigned char charging_state;
    int delay_cnt = 0;
    int voltage   = 0;
    int axp_ocv = 0;


	//	disp_pctl();
	//	test_ddr(0);
	// First, we disable all memory accesses.

	f_serial_puts("step 1\n");


#ifdef pwr_ddr_off
	asm(".long 0x003f236f"); //add sync instruction.

	disable_mmc_req();

	serial_put_hex(APB_Rd(MMC_LP_CTRL1),32);
	f_serial_puts("  LP_CTRL1\n");
	wait_uart_empty();

	serial_put_hex(APB_Rd(UPCTL_MCFG_ADDR),32);
	f_serial_puts("  MCFG\n");
	wait_uart_empty();

	store_restore_plls(1);

	APB_Wr(UPCTL_SCTL_ADDR, 1);
	APB_Wr(UPCTL_MCFG_ADDR, 0x60021 );
	APB_Wr(UPCTL_SCTL_ADDR, 2);

	serial_put_hex(APB_Rd(UPCTL_MCFG_ADDR),32);
	f_serial_puts("  MCFG\n");
	wait_uart_empty();

#endif

#ifdef CHECK_ALL_REGULATORS
	// Check regulator
	f_serial_puts("Chk regulators\n");
 	wait_uart_empty();
	check_all_regulators();
#endif

#ifdef pwr_ddr_off
	// MMC sleep 
 	f_serial_puts("Start DDR off\n");
	wait_uart_empty();
	// Next, we sleep
	mmc_sleep();

#if 1
	//Clear PGCR CK
	APB_Wr(PUB_PGCR_ADDR,APB_Rd(PUB_PGCR_ADDR)&(~(3<<12)));
	APB_Wr(PUB_PGCR_ADDR,APB_Rd(PUB_PGCR_ADDR)&(~(7<<9)));
	//APB_Wr(PUB_PGCR_ADDR,APB_Rd(PUB_PGCR_ADDR)&(~(3<<9)));
#endif
	mmc_sleep();
	// enable retention
	//only necessory if you want to shut down the EE 1.1V and/or DDR I/O 1.5V power supply.
	//but we need to check if we enable this feature, we can save more power on DDR I/O 1.5V domain or not.
	enable_retention();

    // save ddr power
    // before shut down DDR PLL, keep the DDR PHY DLL in reset mode.
    // that will save the DLL analog power.
#ifndef POWER_DOWN_DDRPHY
 	f_serial_puts("mmc soft rst\n");
	wait_uart_empty();
	APB_Wr(MMC_SOFT_RST, 0x0);	 // keep all MMC submodules in reset mode
#else
 	f_serial_puts("pwr dn ddr\n");
	wait_uart_empty();
	power_down_ddr_phy();
#endif

  // shut down DDR PLL. 
	writel(readl(P_HHI_DDR_PLL_CNTL)|(1<<30),P_HHI_DDR_PLL_CNTL);

	f_serial_puts("Done DDR off\n");
	wait_uart_empty();

#endif

 	f_serial_puts("CPU off\n");
 	wait_uart_empty();
	cpu_off();
  
   	f_serial_puts("Set up pwr key\n");
 	wait_uart_empty();
#ifdef CONFIG_AW_AXP20       
    axp_ocv = axp_get_ocv();
    f_serial_puts("axp_ocv=");
    wait_uart_empty();
    serial_put_hex(axp_ocv,32);
    wait_uart_empty();
    f_serial_puts("\n");
    wait_uart_empty();
#endif
	//enable power_key int	
	writel(0x100,0xc1109860);//clear int
 	writel(readl(0xc1109868)|1<<8,0xc1109868);
	writel(readl(0xc8100080)|0x1,0xc8100080);

	f_serial_puts("Pwr off domains\n");
 	wait_uart_empty();

#ifdef PLATFORM_HAS_PMU
	power_off_at_24M();
#endif  /* PLATFORM_HAS_PMU */

	writel(readl(P_AO_RTI_PWR_CNTL_REG0)&(~(1<<4)),P_AO_RTI_PWR_CNTL_REG0);

#ifdef CONFIG_ARC_SARDAC_ENABLE
	if(uboot_cmd_flag != 0x87654321)//u-boot suspend cmd flag
#endif
	{
	// ee use 32k, So interrup status can be accessed.
		writel(readl(P_HHI_MPEG_CLK_CNTL)|(1<<9),P_HHI_MPEG_CLK_CNTL);
		switch_to_rtc();
		udelay(1000);

    #ifdef PLATFORM_HAS_PMU
		power_off_at_32K_1();
    #endif
	}

#ifdef PLATFORM_HAS_PMU
	power_off_at_32K_2();
#endif

	// gate off REMOTE, UART
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)&(~(0x9)),P_AO_RTI_GEN_CNTL_REG0);

#if 1
//	udelay(200000);//Drain power

	if(uboot_cmd_flag == 0x87654321)//u-boot suspend cmd flag
	{
    #ifdef PLATFORM_HAS_PMU
		power_off_ddr15();
    #endif

#ifdef CONFIG_ARC_SARDAC_ENABLE
		do{
			udelay(2000);
        #ifdef PLATFORM_HAS_PMU
			vcin_state=get_charging_state();
			if(!vcin_state)
				break;
        #endif /* PLATFORM_HAS_PMU */
		}while((!(readl(0xc1109860)&0x100)) && !(get_adc_sample_in_arc(4)<0x1000));
#else
		do{
			udelay(2000);
    #ifdef PLATFORM_HAS_PMU
			vcin_state=get_charging_state();
			if (!vcin_state) {
			#ifdef CONFIG_AML_PMU
				shut_down();
				while(1);
			#else
				break;
			#endif
			}
        #ifdef CONFIG_AML_PMU
            delay_cnt++;
            if (delay_cnt >= 32768 * 60) {                  // at least delay 1 minutes
                if (vcin_state) {                           // work around for cannot stop charge
                    if (get_charge_end_det()) {
                        break;
                    }
                }
                voltage = aml_pmu_get_voltage();
                if (voltage < 3410) {
                    break;
                }
                delay_cnt = 0;
            }
        #endif /* CONFIG_AML_PMU */
        /* prevent  AXP202 from discharge too low.*/
        #ifdef CONFIG_AW_AXP20
            delay_cnt++;
            if (delay_cnt >= 3000) {                  //do not delay too long.
                axp_ocv = axp_get_ocv();
                if (axp_ocv < 3480) {
                    break;
                }
                delay_cnt = 0;
            }
        #endif /* CONFIG_AW_AXP20 */
    #endif      /* PLATFORM_HAS_PMU */
		}while(!(readl(0xc1109860)&0x100));
#endif//CONFIG_ARC_SARDAC_ENABLE
    #ifdef PLATFORM_HAS_PMU
		power_on_ddr15();
    #endif /* PLATFORM_HAS_PMU */
	}
	else
	{
    #ifdef PLATFORM_HAS_PMU
		charging_state=get_charging_state();//get state before enter polling
    #endif
		do{
			udelay(200);
    #ifdef PLATFORM_HAS_PMU
			if(get_charging_state() ^ charging_state)//when the state is changed, wakeup
				break;
        #ifdef CONFIG_AML_PMU
            delay_cnt++;
            if (delay_cnt >= 32768 * 60) {                  // at least delay 1 minutes
                if (charging_state) {                       // work around for cannot stop charge
                    if (get_charge_end_det()) {
                        break;
                    }
                }
                voltage = aml_pmu_get_voltage();
                if (voltage < 3410) {                       // check over-dishcarge
                    break;
                }
                delay_cnt = 0;
            }
        #endif /* CONFIG_AML_PMU */
        /* prevent  AXP202 from discharge too low.*/
        #ifdef CONFIG_AW_AXP20
            delay_cnt++;
            if (delay_cnt >= 3000) {                  //do not delay too long.
                axp_ocv = axp_get_ocv();
                if (axp_ocv < 3480) {
                    break;
                }
                delay_cnt = 0;
            }
        #endif /* CONFIG_AW_AXP20 */
    #endif /* PLATFORM_HAS_PMU */
		}while(!(readl(0xc1109860)&0x100));
	}

//	while(!(readl(0xc1109860)&0x100)){break;}
#else
	for(i=0;i<200;i++)
   {
        udelay(1000);
        //udelay(1000);
   }
#endif

	//disable power_key int
	writel(readl(0xc1109868)&(~(1<<8)),0xc1109868);
	writel(readl(0xc8100080)&(~0x1),0xc8100080);
	writel(0x100,0xc1109860);//clear int

// gate on REMOTE, UART
	writel(readl(P_AO_RTI_GEN_CNTL_REG0)|0x9,P_AO_RTI_GEN_CNTL_REG0);

#ifdef PLATFORM_HAS_PMU
	power_on_at_32k_2();
#endif

#ifdef CONFIG_ARC_SARDAC_ENABLE
	if(uboot_cmd_flag != 0x87654321)//u-boot suspend cmd flag
#endif
	{
    #ifdef PLATFORM_HAS_PMU
		power_on_at_32k_1();
    #endif

	//  In 32k mode, we had better not print any log.
    #ifndef CONFIG_AML_PMU
		store_restore_plls(0);//Before switch back to clk81, we need set PLL
    #endif

	//	dump_pmu_reg();
	
		switch_to_81();
	  // ee go back to clk81
		writel(readl(P_HHI_MPEG_CLK_CNTL)&(~(0x1<<9)),P_HHI_MPEG_CLK_CNTL);
		udelay(10000);
	}

	// power on even more domains
	f_serial_puts("Pwr up avdd33/3gvcc\n");
	wait_uart_empty();
	
#ifdef PLATFORM_HAS_PMU
	power_on_at_24M();
#endif
#ifdef CONFIG_AML_PMU
	store_restore_plls(0);//Before switch back to clk81, we need set PLL
#endif

 	uart_reset();

//	f_serial_puts("step 7\n");
//	wait_uart_empty();
//	store_restore_plls(0);
	
#ifdef pwr_ddr_off    
	f_serial_puts("step 8\n");
	wait_uart_empty();  
	init_ddr_pll();


#ifdef CONFIG_AW_AXP20       
    axp_ocv = axp_get_ocv();
    f_serial_puts(" axp_ocv=");
    wait_uart_empty();
    serial_put_hex(axp_ocv,32);
    wait_uart_empty();
    f_serial_puts("\n");
    wait_uart_empty();
#endif
	//store_vid_pll();

   // Next, we reset all channels 
	reset_mmc();
	f_serial_puts("step 9\n");
	wait_uart_empty();

	// disable retention
	// disable retention before init_pctl is because init_pctl you need to data training stuff.
	disable_retention();

	// initialize mmc and put it to sleep
	init_pctl();
	f_serial_puts("step 10\n");
	wait_uart_empty();

	//print some useful information to help debug.
	serial_put_hex(APB_Rd(MMC_LP_CTRL1),32);
	f_serial_puts("  MMC_LP_CTRL1\n");
	wait_uart_empty();

	serial_put_hex(APB_Rd(UPCTL_MCFG_ADDR),32);
	f_serial_puts("  MCFG\n");
	wait_uart_empty();

	if(uboot_cmd_flag == 0x87654321)//u-boot suspend cmd flag
	{
//		writel(readl(P_HHI_SYS_PLL_CNTL)&(~1<<30),P_HHI_SYS_PLL_CNTL);//power on sys pll
		if(!vcin_state)//plug out ACIN
		{
        #ifdef PLATFORM_HAS_PMU
			shut_down();
        #endif      /* PLATFORM_HAS_PMU */
			do{
				udelay(20000);
				f_serial_puts("wait shutdown...\n");
				wait_uart_empty();
			}while(1);
		}
		else
		{
			writel(0,P_AO_RTI_STATUS_REG2);
			writel(readl(P_AO_RTI_PWR_CNTL_REG0)|(1<<4),P_AO_RTI_PWR_CNTL_REG0);
			clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL,1<<19);
			writel(10,0xc1109904);
			writel(1<<22|3<<24,0xc1109900);

		    do{udelay(20000);f_serial_puts("wait reset...\n");wait_uart_empty();}while(1);
		}
	}

#endif   //pwr_ddr_off
  // Moved the enable mmc req and SEC to ARM code.
  //enable_mmc_req();
	
//	disp_pctl();
	
//	test_ddr(1);
//	test_ddr(0);
//	test_ddr(1);
	
//	disp_code();	

	f_serial_puts("restart arm\n");
	wait_uart_empty();

	serial_put_hex(readl(P_VGHL_PWM_REG0),32);
	f_serial_puts("  VGHL_PWM before\n");
	wait_uart_empty();
	writel(0x631000, P_VGHL_PWM_REG0);    //enable VGHL_PWM
	__udelay(1000);

	restart_arm();
  
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
#ifdef PLATFORM_HAS_PMU 
			init_I2C();
#endif
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
unsigned viidpll_settings[4];
unsigned vidpll_settings[4];

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

		viidpll_settings[0]=readl(P_HHI_VIID_PLL_CNTL);
		viidpll_settings[1]=readl(P_HHI_VIID_PLL_CNTL2);
		viidpll_settings[2]=readl(P_HHI_VIID_PLL_CNTL3);
		viidpll_settings[3]=readl(P_HHI_VIID_PLL_CNTL4);

		vidpll_settings[0]=readl(P_HHI_VID_PLL_CNTL);
		vidpll_settings[1]=readl(P_HHI_VID_PLL_CNTL2);
		vidpll_settings[2]=readl(P_HHI_VID_PLL_CNTL3);
		vidpll_settings[3]=readl(P_HHI_VID_PLL_CNTL4);

#endif //CONFIG_SYS_PLL_SAVE

		save_ddr_settings();
		return;
    }    
    
#ifdef CONFIG_SYS_PLL_SAVE 

	//temp define
#define P_HHI_MPLL_CNTL5         CBUS_REG_ADDR(HHI_MPLL_CNTL5)

	/*
	//to find bandgap is disabled!
	if(!(readl(P_HHI_MPLL_CNTL5) & 1))
	{
		wait_uart_empty();
		f_serial_puts("\nERROR! Stop here! SYS PLL bandgap disabled!\n");
	    wait_uart_empty();
		while(1);
	}
	*/
	
#ifdef CONFIG_AML_PMU
    printf_arc("store_restore_plls, in\n");
#endif
	do{
		//BANDGAP reset for SYS_PLL,VIID_PLL,MPLL lock fail
		//Note: once SYS PLL is up, there is no need to 
		//          use AM_ANALOG_TOP_REG1 for VIID, MPLL
		//          lock fail
		writel(readl(P_HHI_MPLL_CNTL5)&(~1),P_HHI_MPLL_CNTL5); 
    #ifdef CONFIG_AML_PMU
		udelay(3 * 750);
    #else
		udelay(3);
    #endif
		writel(readl(P_HHI_MPLL_CNTL5)|1,P_HHI_MPLL_CNTL5); 
    #ifdef CONFIG_AML_PMU
		udelay(30 * 750);
    #else
		udelay(30); //1ms in 32k for bandgap bootup
    #endif
		
		writel(1<<29,P_HHI_SYS_PLL_CNTL);		
		writel(pll_settings[1],P_HHI_SYS_PLL_CNTL2);
		writel(pll_settings[2],P_HHI_SYS_PLL_CNTL3);
		writel(pll_settings[3],P_HHI_SYS_PLL_CNTL4);
		writel((pll_settings[0] & ~(1<<30))|1<<29,P_HHI_SYS_PLL_CNTL);
		writel(pll_settings[0] & ~(3<<29),P_HHI_SYS_PLL_CNTL);
		
		//M6_PLL_WAIT_FOR_LOCK(HHI_SYS_PLL_CNTL);

    #ifdef CONFIG_AML_PMU
		udelay(10 * 750);
    #else
		udelay(10); //wait 100us for PLL lock
    #endif
	}while((readl(P_HHI_SYS_PLL_CNTL)&0x80000000)==0);

#ifdef CONFIG_AML_PMU
    printf_arc("store_restore_plls, SYS_PLL out\n");
#endif
	do{
		//no need to do bandgap reset
		writel(1<<29,P_HHI_MPLL_CNTL);
		for(i=1;i<10;i++)
			writel(mpll_settings[i],P_HHI_MPLL_CNTL+4*i);

    #ifdef CONFIG_AML_PMU //add
		writel((mpll_settings[0] |1<<30),P_HHI_MPLL_CNTL);
		udelay(24 * 100);
    #endif
		writel((mpll_settings[0] & ~(1<<30))|1<<29,P_HHI_MPLL_CNTL);
		writel(mpll_settings[0] & ~(3<<29),P_HHI_MPLL_CNTL);
    #ifdef CONFIG_AML_PMU
		udelay(10 * 750);
    #else
		udelay(10); //wait 200us for PLL lock		
    #endif
	}while((readl(P_HHI_MPLL_CNTL)&0x80000000)==0);
#ifdef CONFIG_AML_PMU
    printf_arc("store_restore_plls, HHI_PLL out\n");
#endif

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
	#ifdef CONFIG_AML_PMU
	    udelay(10 * 750);
	#else
		udelay(10); //wait 200us for PLL lock		
	#endif
	}while((readl(P_HHI_VIID_PLL_CNTL)&0x80000000)==0);

#ifdef CONFIG_AML_PMU
    udelay(3 * 750);
#else
	udelay(3);
#endif

#endif //CONFIG_SYS_PLL_SAVE
}

void store_vid_pll(void)
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

