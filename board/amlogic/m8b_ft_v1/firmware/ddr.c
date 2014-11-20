#include <asm/arch/io.h>
#include <asm/arch/uart.h>


//#define PHYS_MEMORY_START 0x80000000
#define RESET_MMC_FOR_DTU_TEST

#ifdef CONFIG_CMD_DDR_TEST
static unsigned zqcr = 0, ddr_pll;
static unsigned short ddr_clk;
#endif

static void wait_pll(unsigned clk,unsigned dest);
int cfg_ddr_mode = CFG_DDR_MODE;

#include "ddr_init_hw.c"
#include "ddr_init_pctl.c"
#include "ddr_auto_detect.c"

void set_ddr_clock(struct ddr_set * timing_reg)
{
	int n_pll_try_times = 0;
 
	#if defined(CONFIG_VLSI_EMULATOR)
	Wr_cbus(AM_ANALOG_TOP_REG1, Rd_cbus(AM_ANALOG_TOP_REG1)|1);
	#endif //
	do {
		//BANDGAP reset for SYS_PLL,MPLL lock fail 
		writel(readl(0xc8000410)& (~(1<<12)),0xc8000410);
		//__udelay(100);
		writel(readl(0xc8000410)|(1<<12),0xc8000410);
		//__udelay(1000);//1ms for bandgap bootup

		#if !defined(CONFIG_VLSI_EMULATOR)
		writel((1<<29),AM_DDR_PLL_CNTL); 
		writel(CFG_DDR_PLL_CNTL_2,AM_DDR_PLL_CNTL1);
		writel(CFG_DDR_PLL_CNTL_3,AM_DDR_PLL_CNTL2);
		writel(CFG_DDR_PLL_CNTL_4,AM_DDR_PLL_CNTL3);
		writel(CFG_DDR_PLL_CNTL_5,AM_DDR_PLL_CNTL4);
		#ifdef CONFIG_CMD_DDR_TEST
		if((readl(P_PREG_STICKY_REG0)>>20) == 0xf13){
			zqcr = readl(P_PREG_STICKY_REG0) & 0xfffff;
			ddr_pll = readl(P_PREG_STICKY_REG1);
			writel(ddr_pll|(1<<29),AM_DDR_PLL_CNTL);
			writel(0,P_PREG_STICKY_REG0);
			writel(0,P_PREG_STICKY_REG1);

			ddr_clk = ((24 * (ddr_pll&0x1ff))/((ddr_pll>>9)&0x1f))>>((ddr_pll>>16)&0x3);
			timing_reg->t_ddr_clk = ddr_clk;
			timing_reg->t_pctl_1us_pck = ddr_clk;
			timing_reg->t_pctl_100ns_pck = ddr_clk/10;
			timing_reg->t_mmc_ddr_timming2 = (timing_reg->t_mmc_ddr_timming2 & (~0xff)) | ((ddr_clk/10 - 1)&0xff);
		}
		else
		#endif
		writel(timing_reg->t_ddr_pll_cntl|(1<<29),AM_DDR_PLL_CNTL);
		writel(readl(AM_DDR_PLL_CNTL) & (~(1<<29)),AM_DDR_PLL_CNTL);
		#else
		//from ucode-261
		writel(1,AM_DDR_PLL_CNTL1);
		writel(3<<29,AM_DDR_PLL_CNTL);
		writel(timing_reg->t_ddr_pll_cntl,AM_DDR_PLL_CNTL);
		#endif

		#if !defined(CONFIG_VLSI_EMULATOR)
		PLL_LOCK_CHECK(n_pll_try_times,3);
		#endif // #if !defined(CONFIG_VLSI_EMULATOR)

	} while((readl(AM_DDR_PLL_CNTL)&(1<<31))==0);
	//serial_puts("DDR Clock initial...\n");
	writel(0, P_DDR0_SOFT_RESET);
	writel(0, P_DDR1_SOFT_RESET);
	MMC_Wr(P_DDR_CLK_CNTL, 0x80004040);   // enable DDR PLL CLOCK.
	MMC_Wr(P_DDR_CLK_CNTL, 0x90004040);   // come out of reset.
	MMC_Wr(P_DDR_CLK_CNTL, 0xb0004040);
	MMC_Wr(P_DDR_CLK_CNTL, 0x90004040);
	//M8 still need keep MMC in reset mode for power saving?
	//relese MMC from reset mode
	writel(0xffffffff, P_DMC_SOFT_RST);
	writel(0xffffffff, P_DMC_SOFT_RST1);
	//delay_us(100);//No delay need.
}

#ifdef DDR_ADDRESS_TEST_FOR_DEBUG
static void ddr_addr_test()
{
	unsigned i, j = 0;

addr_start:
	serial_puts("********************\n");
	writel(0x55aaaa55, PHYS_MEMORY_START);
	for(i=2;(((1<<i)&PHYS_MEMORY_SIZE) == 0) ;i++)
	{
		writel((i-1)<<(j*8), PHYS_MEMORY_START+(1<<i));
	}

	serial_put_hex(PHYS_MEMORY_START, 32);
	serial_puts(" = ");
	serial_put_hex(readl(PHYS_MEMORY_START), 32);
	serial_puts("\n");
	for(i=2;(((1<<i)&PHYS_MEMORY_SIZE) == 0) ;i++)
	{
	   serial_put_hex(PHYS_MEMORY_START+(1<<i), 32);
	   serial_puts(" = ");
	   serial_put_hex(readl(PHYS_MEMORY_START+(1<<i)), 32);
	   serial_puts("\n");
	}

	if(j < 3){
		++j;
		goto addr_start;
	}
}
#endif

#define PL310_ST_ADDR	 (volatile unsigned *)0xc4200c00
#define PL310_END_ADDR	(volatile unsigned *)0xc4200c04
static inline unsigned ddr_init(struct ddr_set * timing_reg)
{
	int result;

	set_ddr_clock(timing_reg);

	result = ddr_init_hw(timing_reg);

	// assign DDR Memory Space
	if(result == 0) {
		//serial_puts("Assign DDR Memory Space\n");
		*PL310_END_ADDR = 0xc0000000;
		*PL310_ST_ADDR  = 0x00000001;
	}

	//need fine tune!
	//@@@@ Setup PL310 Latency
	//LDR r1, =0xc4200108
	//LDR r0, =0x00000222	 @ set 333 or 444,  if 222 does not work at a higher frequency
	//STR r0, [r1]
	//LDR r1, =0xc420010c
	//LDR r0, =0x00000222
	//STR r0, [r1]
	writel(0x00000222,0xc4200108);
	writel(0x00000222,0xc420010c);

	return(result);
}
static inline unsigned lowlevel_mem_test_device(unsigned tag,struct ddr_set * timing_reg)
{
#ifdef CONFIG_ACS
	return tag&&(unsigned)memTestDevice((volatile datum *)(timing_reg->phy_memory_start),timing_reg->phy_memory_size);
#else
	return tag&&(unsigned)memTestDevice((volatile datum *)PHYS_MEMORY_START,PHYS_MEMORY_SIZE);
#endif
}

static inline unsigned lowlevel_mem_test_data(unsigned tag,struct ddr_set * timing_reg)
{
#ifdef CONFIG_ACS
	return tag&&(unsigned)memTestDataBus((volatile datum *) (timing_reg->phy_memory_start));
#else
	return tag&&(unsigned)memTestDataBus((volatile datum *) PHYS_MEMORY_START);
#endif
}

static inline unsigned lowlevel_mem_test_addr(unsigned tag,struct ddr_set * timing_reg)
{
	//asm volatile ("wfi");
#ifdef DDR_ADDRESS_TEST_FOR_DEBUG
	ddr_addr_test();
#endif

#ifdef CONFIG_ACS
	return tag&&(unsigned)memTestAddressBus((volatile datum *)(timing_reg->phy_memory_start), timing_reg->phy_memory_size);
#else
	return tag&&(unsigned)memTestAddressBus((volatile datum *)PHYS_MEMORY_START, PHYS_MEMORY_SIZE);
#endif
}

static unsigned ( * mem_test[])(unsigned tag,struct ddr_set * timing_reg)={
	lowlevel_mem_test_addr,
	lowlevel_mem_test_data,
#ifdef CONFIG_ENABLE_MEM_DEVICE_TEST
	lowlevel_mem_test_device
#endif
};

#define MEM_DEVICE_TEST_ITEMS_BASE (sizeof(mem_test)/sizeof(mem_test[0]))

#ifdef FT_DDR_TEST
#include "gpio.c"
#endif

unsigned ddr_test(int arg)
{
	int i; //,j;
	unsigned por_cfg=1;
	//serial_putc('\n');
	por_cfg=0;
	//to protect with watch dog?
	//AML_WATCH_DOG_SET(1000);
	
#ifdef FT_DDR_TEST
  serial_puts("FT_DDR_TEST\n");

  // clear core_pin_mux of gpioAO_7 and gpioAO_8
  writel(readl(P_AO_RTI_PIN_MUX_REG) & ~((1<<30) | (1<<21) | (1<<0)), P_AO_RTI_PIN_MUX_REG);
  writel(0, P_AO_SECURE_REG1);
  writel(readl(P_AO_GPIO_O_EN_N) & ~(0xf<<5), P_AO_GPIO_O_EN_N);  // Set output enable to TEST_OUT[1:0], TEST_DONE, DEBUG_VALID
  writel(readl(P_PREG_PAD_GPIO3_EN_N) & ~(0x1ff<<19), P_PREG_PAD_GPIO3_EN_N); // output enable to gpioH[8:0]
  writel(readl(P_PREG_PAD_GPIO0_EN_N) & ~(0x7f<<22), P_PREG_PAD_GPIO0_EN_N);  // output enable to card[6:0]

	CLR_TEST_DONE_FLAG;

	output_debug_level(1);

	
	// BOOT_4 is used to test DDR PLL clock output
	// writel(readl(PERIPHS_PIN_MUX_5) | (1<<22), PERIPHS_PIN_MUX_5);
	// if(test_ddr_pll_min || test_ddr_pll_max) {
	// 	writel(readl(HHI_MPEG_CLK_CNTL) | (1<<15), HHI_MPEG_CLK_CNTL);
	// }
#endif
	
	for(i=0;i<MEM_DEVICE_TEST_ITEMS_BASE&&por_cfg==0;i++)
	{
		//writel(0, P_WATCHDOG_RESET);
		por_cfg=mem_test[i](arg&(1<<i),&__ddr_setting);
		if(por_cfg){
			serial_puts("Stage ");
			serial_put_hex(i,4);
			serial_puts(" : ");
			serial_put_hex(por_cfg,(por_cfg < 15) ? 4 : 32);
			serial_puts("\n");
#ifdef FT_DDR_TEST
			// wiggle GPIO to indicate FAIL
			SET_TEST_OUT(i);
			SET_TEST_DONE_FLAG; 
#endif
			return por_cfg;
		}
#ifdef FT_DDR_TEST
		output_debug_level(i+1);
#endif
	}
#if !defined(CONFIG_AML_EXT_PGM)
	serial_puts("DDR check: Pass!\n");
#endif

#ifdef FT_DDR_TEST
	// wiggle GPIO to indicate PASS
	SET_TEST_OUT(0);
	SET_TEST_DONE_FLAG; 
#endif
	//AML_WATCH_DOG_DISABLE();
	return por_cfg;
}

SPL_STATIC_FUNC unsigned ddr_init_test(void)
{
#define DDR_INIT_START  (0)
#define DDR_TEST_ADDR   (1<<0)
#define DDR_TEST_DATA   (1<<1)
#define DDR_TEST_DEVICE (1<<2)

//normal DDR init setting
#define DDR_TEST_BASEIC (DDR_INIT_START|DDR_TEST_ADDR|DDR_TEST_DATA)
//complete DDR init setting with a full memory test
#define DDR_TEST_ALL	(DDR_TEST_BASEIC|DDR_TEST_DEVICE)

#ifdef CONFIG_DDR_MODE_AUTO_DETECT
	ddr_mode_auto_detect(&__ddr_setting);
#else
	if(ddr_init(&__ddr_setting))
	{
		serial_puts("\nDDR init fail! Reset...\n");
		__udelay(10000);
		AML_WATCH_DOG_START();
	}
	print_ddr_mode();
#endif

#ifdef CONFIG_DDR_SIZE_AUTO_DETECT
	ddr_size_auto_detect(&__ddr_setting);
#else
#ifdef CONFIG_ACS
	print_ddr_size(__ddr_setting.phy_memory_size);
#else
	print_ddr_size(PHYS_MEMORY_SIZE);
#endif
#endif

int ddr_test_mode = 0;
#ifdef CONFIG_ACS
	ddr_test_mode = __ddr_setting.ddr_test;
#else
	ddr_test_mode = DDR_TEST_BASEIC;
#endif

	if(ddr_test(ddr_test_mode)){
		serial_puts("\nDDR test failed! Reset...\n");
		__udelay(10000);
		AML_WATCH_DOG_START();
	}

	ddr_info_dump(&__ddr_setting);

#ifdef CONFIG_ACS
	writel(((__ddr_setting.phy_memory_size)>>20), CONFIG_DDR_SIZE_IND_ADDR);
#endif

	return 0;
}

