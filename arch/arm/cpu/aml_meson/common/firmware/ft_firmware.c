#define  CONFIG_AMLROM_SPL 1
#include <timer.c>
#include <timming.c>
#include <uartpin.c>
#include <serial.c>
#include <pinmux.c>
#include <sdpinmux.c>
#include <memtest.c>
#include <pll.c>
#include <ddr.c>
#include <mtddevices.c>
#include <sdio.c>

extern void ipl_memcpy(void*, const void *, __kernel_size_t);
#define memcpy ipl_memcpy
#define get_timer get_utimer

#include <loaduboot.c>

#ifdef CONFIG_ACS
#include <storage.c>
#include <acs.c>
#endif

#include <asm/arch/reboot.h>

#define CONFIG_FT_INIT_SP_ADDR 0x12000000

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
#if defined(CONFIG_M8) || defined(CONFIG_M8B)
	//enable watchdog for 5s
	//if bootup failed, switch to next boot device
	AML_WATCH_DOG_SET(5000); //5s
	writel(readl(0xc8100000), SKIP_BOOT_REG_BACK_ADDR); //[By Sam.Wu]backup the skip_boot flag to sram for v2_burning
#endif
	//setbits_le32(0xda004000,(1<<0));	//TEST_N enable: This bit should be set to 1 as soon as possible during the Boot process to prevent board changes from placing the chip into a production test mode

	writel((readl(0xDA000004)|0x08000000), 0xDA000004);	//set efuse PD=1

#if defined(CONFIG_M8) || defined(CONFIG_M8B)
	//A9 JTAG enable
	writel(0x102,0xda004004);
	//TDO enable
	writel(readl(0xc8100014)|0x4000,0xc8100014);	
#endif 

#if !defined(CONFIG_AML_EXT_PGM_SILENT)
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
#endif

    // initial pll
    pll_init(&__plls);
	serial_init(__plls.uart);


#if !defined(CONFIG_AML_EXT_PGM_SILENT)
	unsigned int nPLL = readl(P_HHI_SYS_PLL_CNTL);
	unsigned int nA9CLK = ((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
	serial_puts("\nCPU clock is ");
	serial_put_dec(nA9CLK);
	serial_puts("MHz\n\n");

    nTEBegin = TIMERE_GET();
#endif 

    // initial ddr
	ddr_init_test();

	//asm volatile ("ldr	sp, =(0x12000000)");

#if !defined(CONFIG_AML_EXT_PGM_SILENT)
    serial_puts("\nDDR init use : ");
    serial_put_dec(get_utimer(nTEBegin));
    serial_puts(" us\n");
#endif

	//asm volatile ("wfi");

	AML_WATCH_DOG_DISABLE(); //disable watchdog
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

    serial_puts("\nSystem Started\n");

#if defined(CONFIG_M8) || defined(CONFIG_M8B)
	//if bootup failed, switch to next boot device
	AML_WATCH_DOG_DISABLE(); //disable watchdog
	//temp added
	writel(0,0xc8100000);
#endif

	typedef  void (*t_func_v1)(void);
	t_func_v1 fp_program = (t_func_v1)CONFIG_AML_EXT_PGM_ENTRY;

	fp_program();
	
	return 0;
}

