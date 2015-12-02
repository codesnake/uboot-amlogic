/*
 * \file        aml_v2_burning.c
 * \brief       common interfaces for version 2 burning
 *
 * \version     1.0.0
 * \date        09/15/2013
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic. All Rights Reserved.
 *
 */
#include "v2_burning_i.h"
#include <mmc.h>

extern int optimus_burn_package_in_sdmmc(const char* sdc_cfg_file);

//check ${sdcburncfg} exist in external mmc and internal flash not burned yet!
int aml_check_is_ready_for_sdc_produce(void)
{
    char* sdc_cfg_file = NULL;
    const char* cmd = NULL;
    int ret = 0;

    //Check 'upgrade_step' when raw flash in factory
    if(!is_the_flash_first_burned())//'upgrade_step == 0' means flash not burned yet!
    {
        //'MESON_SDC_BURNER_REBOOT == reboot_mode'  means sdcard upgrade mode
        if(MESON_SDC_BURNER_REBOOT != reboot_mode){
            DWN_MSG("reboot_mode=0x%x\n", reboot_mode);
            return 0;//not ready
        }
    }

    cmd = "mmcinfo 0";
    ret = run_command(cmd, 0);
    if(ret){
        DWN_MSG("mmcinfo failed!\n");
        return 0;//not ready
    }

    sdc_cfg_file = getenv("sdcburncfg");
    if(!sdc_cfg_file) {
        sdc_cfg_file = "aml_sdc_burn.ini";
        setenv("sdcburncfg", sdc_cfg_file);
    }

    cmd = "fatexist mmc 0 ${sdcburncfg}";
    ret = run_command(cmd, 0);
    if(ret){
        DWN_DBG("%s not exist in external mmc\n", sdc_cfg_file);
        return 0;
    }
    
    return 1;//is ready for sdcard producing
}

static unsigned _get_romcode_boot_id(void)
{
    unsigned* pBootId = &C_ROM_BOOT_DEBUG->boot_id;
    unsigned boot_id = *pBootId;
#ifdef CONFIG_MESON_TRUSTZONE
    boot_id = meson_trustzone_sram_read_reg32(pBootId);
#endif// #ifdef CONFIG_MESON_TRUSTZONE

    return boot_id;
}

//is the uboot loaded from usb otg
int is_tpl_loaded_from_usb(void)
{
#if 0
    return (MESON_USB_BURNER_REBOOT == readl(CONFIG_TPL_BOOT_ID_ADDR));
#else
    DWN_DBG("_get_romcode_boot_id()=%d\n", _get_romcode_boot_id());
    return (2 <= _get_romcode_boot_id());
#endif//
}

//is the uboot loaded from sdcard mmc 0
//note only sdmmc supported by romcode when external device boot
int is_tpl_loaded_from_ext_sdmmc(void)
{
    /*return (MESON_SDC_BURNER_REBOOT == readl(CONFIG_TPL_BOOT_ID_ADDR));*/
    //see loaduboot.c, only boot from sdcard when "boot_id == 1"
    return (1 == _get_romcode_boot_id());
}

//Check if uboot loaded from external sdmmc or usb otg
int check_uboot_loaded_for_burn(int flag)
{
    int usb_boot = is_tpl_loaded_from_usb();
    int sdc_boot = is_tpl_loaded_from_ext_sdmmc();

    return usb_boot || sdc_boot;
}

//producing mode means boot from raw flash, i.e, uboot is loaded from usb
int aml_v2_usb_producing(int flag, bd_t* bis)
{
    flag = flag; bis = bis;//avoid compile warning

    optimus_work_mode_set(OPTIMUS_WORK_MODE_USB_PRODUCE);

    close_usb_phy_clock(0);//disconect before re-connect to enhance pc compatibility
    return v2_usbburning(20000);
}

int aml_v2_sdc_producing(int flag, bd_t* bis)
{
    optimus_work_mode_set(OPTIMUS_WORK_MODE_SDC_PRODUCE);

    return optimus_burn_package_in_sdmmc(getenv("sdcburncfg"));
}

