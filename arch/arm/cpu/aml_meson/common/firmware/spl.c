#define  CONFIG_AMLROM_SPL 1
#include <timer.c>
#include <timming.c>
#include <uartpin.c>
#include <serial.c>
#include <pinmux.c>
#include <sdpinmux.c>
#include <memtest.c>
#include <pll.c>
#ifdef CONFIG_POWER_SPL
#include <hardi2c_lite.c>
#include <power.c>
#endif
#include <ddr.c>
#include <mtddevices.c>
#include <sdio.c>
//#include <debug_rom.c>

extern void ipl_memcpy(void*, const void *, __kernel_size_t);
#define memcpy ipl_memcpy

#include <loaduboot.c>
#ifdef CONFIG_ACS
#include <storage.c>
#include <acs.c>
#endif

#include <asm/arch/reboot.h>

#ifdef CONFIG_POWER_SPL
#include <amlogic/aml_pmu_common.h>
#endif

#ifdef CONFIG_MESON_TRUSTZONE
#include <secureboot.c>
unsigned int ovFlag;
#endif

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
#if (MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
	//enable watchdog for 5s
	//if bootup failed, switch to next boot device
	AML_WATCH_DOG_SET(5000); //5s
	writel(readl(0xc8100000), SKIP_BOOT_REG_BACK_ADDR); //[By Sam.Wu]backup the skip_boot flag to sram for v2_burning
#endif
	//setbits_le32(0xda004000,(1<<0));	//TEST_N enable: This bit should be set to 1 as soon as possible during the Boot process to prevent board changes from placing the chip into a production test mode

	writel((readl(0xDA000004)|0x08000000), 0xDA000004);	//set efuse PD=1

//write ENCI_MACV_N0 (CBUS 0x1b30) to 0, disable Macrovision
#if defined(CONFIG_M6) || defined(CONFIG_M6TV)||defined(CONFIG_M6TVD)
	writel(0, CBUS_REG_ADDR(ENCI_MACV_N0));
#endif

//Default to open ARM JTAG for M6 only
#if  defined(CONFIG_M6) || defined(CONFIG_M6TV)|| defined(CONFIG_M6TVD)
	#define AML_M6_JTAG_ENABLE
	#define AML_M6_JTAG_SET_ARM

	//for M6 only. And it will cause M3 fail to boot up.
	//TEST_N enable: This bit should be set to 1 as soon as possible during the 
	//Boot process to prevent board changes from placing the chip into a production test mode
	setbits_le32(0xda004000,(1<<0));

	// set bit [12..14] to 1 in AO_RTI_STATUS_REG0
	// This disables boot device fall back feature in MX Rev-D
	// This still enables bootloader to detect which boot device
	// is selected during boot time. 
	switch(readl(0xc8100000))
	{
	case 0x6b730001:
	case 0x6b730002: writel(readl(0xc8100000) |(0x70<<8),0xc8100000);break;
	}

#endif


#if (MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
	//A9 JTAG enable
	writel(0x102,0xda004004);
	//TDO enable
	writel(readl(0xc8100014)|0x4000,0xc8100014);
	
	//detect sdio debug board
	unsigned pinmux_2 = readl(P_PERIPHS_PIN_MUX_2);
	
	// clear sdio pinmux
	setbits_le32(P_PREG_PAD_GPIO0_O,0x3f<<22);
	setbits_le32(P_PREG_PAD_GPIO0_EN_N,0x3f<<22);
	clrbits_le32(P_PERIPHS_PIN_MUX_2,7<<12);  //clear sd d1~d3 pinmux
	
	if(!(readl(P_PREG_PAD_GPIO0_I)&(1<<26))){  //sd_d3 low, debug board in
		serial_puts("\nsdio debug board detected ");
		clrbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);   //clear AO uart pinmux
		setbits_le32(P_PERIPHS_PIN_MUX_8,3<<9);
		
		if((readl(P_PREG_PAD_GPIO0_I)&(1<<22)))
			writel(0x220,P_AO_SECURE_REG1);  //enable sdio jtag
	}
	else{
		serial_puts("\nno sdio debug board detected ");
		writel(pinmux_2,P_PERIPHS_PIN_MUX_2);
	}
#endif 


#ifdef AML_M6_JTAG_ENABLE
	#ifdef AML_M6_JTAG_SET_ARM
		//A9 JTAG enable
		writel(0x80000510,0xda004004);
		//TDO enable
		writel(readl(0xc8100014)|0x4000,0xc8100014);
	#elif AML_M6_JTAG_SET_ARC
		//ARC JTAG enable
		writel(0x80051001,0xda004004);
		//ARC bug fix disable
		writel((readl(0xc8100040)|1<<24),0xc8100040);
	#endif	//AML_M6_JTAG_SET_ARM

	//Watchdog disable
	//writel(0,0xc1109900);
	//asm volatile ("wfi");

#endif //AML_M6_JTAG_ENABLE

	//Note: Following msg is used to calculate romcode boot time
	//         Please DO NOT remove it!
    serial_puts("\nTE : ");
    unsigned int nTEBegin = TIMERE_GET();
    serial_put_dec(nTEBegin);
    serial_puts("\nBT : ");
	//Note: Following code is used to show current uboot build time
	//         For some fail cases which in SPL stage we can not target
	//         the uboot version quickly. It will cost about 5ms.
	//         Please DO NOT remove it! 
	serial_puts(__TIME__);
	serial_puts(" ");
	serial_puts(__DATE__);
	serial_puts("\n");	

#ifdef CONFIG_POWER_SPL
    power_init(POWER_INIT_MODE_NORMAL);
#endif

#if !defined(CONFIG_VLSI_EMULATOR)
    // initial pll
    pll_init(&__plls);
	serial_init(__plls.uart);
#else
	serial_init(readl(P_UART_CONTROL(UART_PORT_CONS))|UART_CNTL_MASK_TX_EN|UART_CNTL_MASK_RX_EN);
	serial_puts("\n\nAmlogic log: UART OK for emulator!\n");
#endif //#if !defined(CONFIG_VLSI_EMULATOR)


	//TEMP add 
	unsigned int nPLL = readl(P_HHI_SYS_PLL_CNTL);
	unsigned int nA9CLK = ((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
	serial_puts("\nCPU clock is ");
	serial_put_dec(nA9CLK);
	serial_puts("MHz\n\n");

    nTEBegin = TIMERE_GET();

    // initial ddr
    ddr_init_test();

    serial_puts("\nDDR init use : ");
    serial_put_dec(get_utimer(nTEBegin));
    serial_puts(" us\n");

#if defined(CONFIG_M8B) && defined(CONFIG_AML_SECU_BOOT_V2) && \
    defined(CONFIG_AML_SPL_L1_CACHE_ON)
    asm volatile ("ldr	sp, =(0x12000000)");
    //serial_puts("aml log : set SP to 0x12000000\n");
#endif

	//asm volatile ("wfi");
	    
    // load uboot
#ifdef CONFIG_ENABLE_WATCHDOG
	if(load_uboot(__TEXT_BASE,__TEXT_SIZE)){
   		serial_puts("\nload uboot error,now reset the chip");
		AML_WATCH_DOG_START();
	}
#else
    load_uboot(__TEXT_BASE,__TEXT_SIZE);
#endif


    serial_puts("\nTE : ");
	serial_put_dec(TIMERE_GET());
	serial_puts("\n");

	//asm volatile ("wfi");
	// load secureOS
#ifdef CONFIG_MESON_TRUSTZONE
	if(load_secureos()){
		serial_puts("\nload secureOS fail,now reset the chip");
		ovFlag = 1;		
		AML_WATCH_DOG_START();
	}
	else{		
		ovFlag = 0;
		serial_puts("\nOV System Started\n");
		serial_wait_tx_empty();    
	}
#endif	

    serial_puts("\nSystem Started\n");

#ifdef TEST_UBOOT_BOOT_SPEND_TIME
	unsigned int spl_boot_end = TIMERE_GET();
	serial_puts("\ntime: spl boot time(us):");
	serial_put_dec(spl_boot_end);
	//serial_put_dword((spl_boot_end));
#endif

#if (MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
	//if bootup failed, switch to next boot device
	AML_WATCH_DOG_DISABLE(); //disable watchdog
	//temp added
	writel(0,0xc8100000);
#endif

#if defined(CONFIG_M8B) && defined(CONFIG_AML_SECU_BOOT_V2) && \
    defined(CONFIG_AML_SPL_L1_CACHE_ON)

    unsigned int fpAddr = CONFIG_SYS_TEXT_BASE;

#ifdef CONFIG_IMPROVE_UCL_DEC
    fpAddr = CONFIG_SYS_TEXT_BASE+0x800000;
#endif

#ifdef CONFIG_MESON_TRUSTZONE
    fpAddr = SECURE_OS_DECOMPRESS_ADDR;
#endif

    typedef  void (*t_func_v1)(void);
    t_func_v1 fp_program = (t_func_v1)fpAddr;
    //here need check ?
    fp_program();
#endif


#ifdef CONFIG_MESON_TRUSTZONE		
    return ovFlag;
#else
	return 0;
#endif    
}

