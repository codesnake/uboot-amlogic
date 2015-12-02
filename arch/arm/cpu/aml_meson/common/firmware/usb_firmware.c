#define  CONFIG_AMLROM_SPL 1
#include <timer.c>
#include <timming.c>
#include <uartpin.c>
#include <serial.c>
#include <pinmux.c>
#include <sdpinmux.c>
#include <memtest.c>
#include <pll.c>
#include <hardi2c_lite.c>
#include <power.c>
#include <ddr.c>
#include <mtddevices.c>
#include <sdio.c>
#include <debug_rom.c>

#include <loaduboot.c>
#ifdef CONFIG_ACS
#include <storage.c>
#include <acs.c>
#endif

#include <asm/arch/reboot.h>

#ifdef CONFIG_POWER_SPL
#include <amlogic/aml_pmu_common.h>
#endif

#include <asm/cpu_id.h>

#ifdef CONFIG_MESON_TRUSTZONE
#include <secureboot.c>
unsigned int ovFlag;
#endif

static int _usb_ucl_decompress(unsigned char* compressData, unsigned char* decompressedAddr, unsigned* decompressedLen)
{
    int ret = __LINE__; 

    serial_puts("\n\nucl Decompress START ====>\n");
    serial_puts("compressData "), serial_put_hex((unsigned)compressData, 32), serial_puts(",");
    serial_puts("decompressedAddr "), serial_put_hex((unsigned)decompressedAddr, 32), serial_puts(".\n");

#if defined(CONFIG_AML_MESON_8)
        AML_WATCH_DOG_SET(8000); //8s for ucl decompress, maybe it's enough!? Dog will silently reset system if timeout...
#endif// #if defined(CONFIG_AML_MESON_8)

#if CONFIG_UCL
#ifndef CONFIG_IMPROVE_UCL_DEC
    extern int uclDecompress(char* destAddr, unsigned* o_len, char* srcAddr);
    ret = uclDecompress((char*)decompressedAddr, decompressedLen, (char*)compressData);

    serial_puts("uclDecompress "), serial_puts(ret ? "FAILED!!\n": "OK.\n");
    serial_puts("\n<====ucl Decompress END. \n\n");
#endif// #ifndef CONFIG_IMPROVE_UCL_DEC
#endif//#if CONFIG_UCL

    if(ret){
        serial_puts("decompress FAILED ret="), serial_put_dec(ret), serial_puts("\n");
    }
    else{
        serial_puts("decompressedLen "), serial_put_hex(*decompressedLen, 32), serial_puts(".\n");
    }

    return ret;
}

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
	//setbits_le32(0xda004000,(1<<0));	//TEST_N enable: This bit should be set to 1 as soon as possible during the Boot process to prevent board changes from placing the chip into a production test mode

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

#if 0
#if defined(CONFIG_AML_SMP)
	load_smp_code();
#endif

#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
	aml_cache_enable();
	//serial_puts("\nSPL log : ICACHE & DCACHE ON\n");
#endif	//CONFIG_AML_SPL_L1_CACHE_ON
#endif//#if 0
	   
    serial_puts("\nTE : ");
	serial_put_dec(TIMERE_GET());
	serial_puts("\n");

#if defined(CONFIG_AML_MESON_8)
        AML_WATCH_DOG_SET(5000); //5s for secue boot check, maybe it's enough!? Dog will silently reset system if timeout...
#endif// #if defined(CONFIG_AML_MESON_8)
    if(aml_sec_boot_check((unsigned char*)__TEXT_BASE))
    {
        serial_puts("\nSecure_boot_check FAILED,reset chip to let PC know!\n");
        AML_WATCH_DOG_START();
    }
    serial_puts("\nUSB boot signature checked OK.\n");

#ifdef CONFIG_MESON_TRUSTZONE
    {
        unsigned* ubootBinAddr  = (unsigned*)CONFIG_USB_SPL_ADDR;
        unsigned  decompressedLen = 0;
        unsigned  secureosOffset  = 0;
        unsigned char* compressedAddr   = 0;
        unsigned char* decompressedAddr = (unsigned char*)SECURE_OS_DECOMPRESS_ADDR;
        int ret = 1;

        secureosOffset = ubootBinAddr[(READ_SIZE - SECURE_OS_OFFSET_POSITION_IN_SRAM)>>2];
        compressedAddr   = (unsigned char*)ubootBinAddr + secureosOffset;
        serial_puts("secureos offset "), serial_put_hex(secureosOffset, 32), serial_puts(",");
        ret = _usb_ucl_decompress(compressedAddr, decompressedAddr, &decompressedLen);
        if(ret){
            serial_puts("\nload secureos error,now reset chip to let PC know\n");
            AML_WATCH_DOG_START();
        }
    }
#endif//#ifdef CONFIG_MESON_TRUSTZONE
 
    // load uboot
    {
        int ret = 1;
        unsigned char* tplTargeTextBase = (unsigned char*)__TEXT_BASE;
        unsigned char* tplTempAddr = tplTargeTextBase + 0x800000;
        unsigned  decompressedLen = 0;

		memcpy(tplTempAddr, tplTargeTextBase, __TEXT_SIZE); //here need fine tune!!

        ret = _usb_ucl_decompress(tplTempAddr, tplTargeTextBase, &decompressedLen);
        if(ret){
            serial_puts("\nload uboot error,reset chip to let PC know");
            AML_WATCH_DOG_START();
        }
        ret = check_sum((unsigned*)tplTargeTextBase, 0, 0);
        if(ret){
            serial_puts("check magic error, So reset the chip!!\n");
            AML_WATCH_DOG_START();
        }
    }

    serial_puts("\nSystem Started\n");

#ifdef CONFIG_MESON_TRUSTZONE		
    return ovFlag;
#else
	return 0;
#endif    
}

