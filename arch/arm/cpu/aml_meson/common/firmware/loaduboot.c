#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>

#if defined(CONFIG_M6_SECU_BOOT)
#include "../../../../../../drivers/secure/sha2.c"
#endif

#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
#include <aml_a9_cache.c>
#endif  //CONFIG_AML_SPL_L1_CACHE_ON

#include <romboot.c>
#ifndef CONFIG_AML_UBOOT_MAGIC
#define CONFIG_AML_UBOOT_MAGIC 0x12345678
#endif

#if defined(CONFIG_AML_SMP)
#include <smp.dat>
SPL_STATIC_FUNC int load_smp_code()
{
	//serial_puts("Start load SMP code!\n");
	unsigned * paddr = (unsigned*)PHYS_MEMORY_START;
	unsigned size = sizeof(smp_code)/sizeof(unsigned);
	int i;
	for(i = 0; i < size; i++){
		*paddr = smp_code[i];
		paddr++;
	}
	//serial_puts("Load SMP code finished!\n");
}
#endif

#ifndef CONFIG_DISABLE_INTERNAL_U_BOOT_CHECK
short check_sum(unsigned * addr,unsigned short check_sum,unsigned size)
{
   serial_put_dword(addr[15]);
   if(addr[15]!=CONFIG_AML_UBOOT_MAGIC)
        return -1;
#if 0
    {
     int i;
     unsigned short * p=(unsigned short *)addr;
     for(i=0;i<size>>1;i++)
        check_sum^=p[i];
    }
#endif
    return 0;
}
#endif

SPL_STATIC_FUNC int load_uboot(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
	unsigned por_cfg=romboot_info->por_cfg;
	unsigned boot_id=romboot_info->boot_id;
	unsigned size;
	int rc=0;

#if !defined(CONFIG_AML_EXT_PGM)
    serial_puts("\nHHH\n");
#endif

#if defined(CONFIG_AML_SMP) && !defined(CONFIG_AML_EXT_PGM)
	load_smp_code();
#endif

#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
	aml_cache_enable();
	//serial_puts("\nSPL log : ICACHE & DCACHE ON\n");
#endif	//CONFIG_AML_SPL_L1_CACHE_ON

#if defined(CONFIG_VLSI_EMULATOR)
	size= 0xC0000; //max is 1MBytes ?
#else
	size=__TEXT_SIZE;

#if defined(AML_UBOOT_SINFO_OFFSET)
	unsigned int *pAUINF = (unsigned int *)(((unsigned int)load_uboot & 0xFFFF8000)+(AML_UBOOT_SINFO_OFFSET));
	size = *pAUINF++; //SPL
	size = *pAUINF++ - size; //TPL - SPL
	size += *pAUINF;  // + Secure OS
	size += 0x200;  //for secure boot, just add 512 without check, for simple
	if(size < (100<<10) || size > (1<<20)) //illegal size, restore to default from rom_spl.s
		size = __TEXT_SIZE;
#endif //AML_UBOOT_SINFO_OFFSET

#endif
	//boot_id = 1;
	if(boot_id>1)
        boot_id=0;
	if(boot_id==0)
    {
       rc=fw_load_intl(por_cfg,__TEXT_BASE,size);
	}else{
	   rc=fw_load_extl(por_cfg,__TEXT_BASE,size);
	}

	//here no need to flush I/D cache?
#if CONFIG_AML_SPL_L1_CACHE_ON
	aml_cache_disable();
	//dcache_flush();
#endif	//CONFIG_AML_SPL_L1_CACHE_ON

#if defined(CONFIG_AML_EXT_PGM)
	return rc;
#endif


#ifndef CONFIG_DISABLE_INTERNAL_U_BOOT_CHECK
	if(!rc&&check_sum((unsigned*)__TEXT_BASE,0,0)==0)
	{
	    fw_print_info(por_cfg,boot_id);
        return rc;
    }
#else
    if(rc==0)
	{
	    fw_print_info(por_cfg,boot_id);
        return rc;
    }
#endif

#ifdef CONFIG_ENABLE_WATCHDOG
	if(rc){
	serial_puts(__FILE__);
		serial_puts(__FUNCTION__);
		serial_put_dword(__LINE__);
		AML_WATCH_DOG_START();
	}
#endif
#if CONFIG_ENABLE_EXT_DEVICE_RETRY
	while(rc)
	{
     debug_rom(__FILE__,__LINE__);

	    rc=fw_init_extl(por_cfg);//INTL device  BOOT FAIL
	    if(rc)
	        continue;
	    rc=fw_load_extl(por_cfg,__TEXT_BASE,size);
	    if(rc)
	        continue;
#ifndef CONFIG_DISABLE_INTERNAL_U_BOOT_CHECK
	    rc=check_sum((unsigned*)__TEXT_BASE,0,0);
#endif
	}
#endif
	fw_print_info(por_cfg,1);

    return rc;
}
