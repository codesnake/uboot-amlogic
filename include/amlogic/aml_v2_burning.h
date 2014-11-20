/*
 * \file        aml_v2_burning.h
 * \brief       common interfaces for version burning
 *
 * \version     1.0.0
 * \date        09/15/2013
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic. All Rights Reserved.
 *
 */

//is the uboot loaded from usb otg
int is_tpl_loaded_from_usb(void);

//is the uboot loaded from sdcard mmc 0
//note only sdmmc supported by romcode when external device boot
int is_tpl_loaded_from_ext_sdmmc(void);

//Enter usb burning
int v2_usbburning(unsigned timeout);

//Enter sdcard burning
int aml_v2_sdc_producing(int flag, bd_t* bis);

//Check if uboot loaded from external sdmmc or usb otg
int check_uboot_loaded_for_burn(int flag);

//usb producing mode, if tpl loaded from usb pc tool and auto enter producing mode 
int aml_v2_usb_producing(int flag, bd_t* bis);

//check ${sdcburncfg} exist in external mmc and internal flash not burned yet!
int aml_check_is_ready_for_sdc_produce(void);

