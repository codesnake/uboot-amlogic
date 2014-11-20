/*
 * \file        optimus_sdc_burn.c
 * \brief       burning itself from Pheripheral tf/sdmmc card
 *
 * \version     1.0.0
 * \date        2013-7-11
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic. All Rights Reserved.
 *
 */
#include "optimus_sdc_burn_i.h"
#include "optimus_led.h"

static int is_bootloader_old(void)
{
    int sdc_boot = is_tpl_loaded_from_ext_sdmmc();

    return !sdc_boot;
}

int get_burn_parts_from_img(HIMAGE hImg, ConfigPara_t* pcfgPara)
{
    BurnParts_t* pburnPartsCfg = &pcfgPara->burnParts;
    int i = 0;
    int ret = 0;
    int burnNum = 0;
    const int totalItemNum = get_total_itemnr(hImg);

    for (i = 0; i < totalItemNum; i++) 
    {
        const char* main_type = NULL;
        const char* sub_type  = NULL;

        ret = get_item_name(hImg, i, &main_type, &sub_type);
        if(ret){
            DWN_ERR("Exception:fail to get item name!\n");
            return __LINE__;
        }

        if(!strcmp("PARTITION", main_type))
        {
            char* partName = pburnPartsCfg->burnParts[burnNum];

            if(!strcmp("bootloader", sub_type))continue;

            strcpy(partName, sub_type);
            pburnPartsCfg->bitsMap4BurnParts |= 1U<<burnNum;
            burnNum += 1;
        }
    }

    if(burnNum)
    {
        pburnPartsCfg->burn_num = burnNum;

        ret = check_cfg_burn_parts(pcfgPara);
        if(ret){
            DWN_ERR("Fail in check burn parts\n");
            return __LINE__;
        }
        print_burn_parts_para(pburnPartsCfg);
    }

    return OPT_DOWN_OK;
}

#define ITEM_NOT_EXIST   0x55 

int optimus_verify_partition(const char* partName, HIMAGE hImg, char* _errInfo)
{
#define MaxSz (64 - 7) //verify file to at most 64B to corresponding to USB burn, strlen("verify ") == 7

    char* argv[4];
    int ret = 0;
    HIMAGEITEM hImgItem = NULL;
    int imgItemSz = 0;
    char CmdVerify[MaxSz + 7] = {0};

    hImgItem = image_item_open(hImg, "VERIFY", partName);
    if(!hImgItem){
        DWN_ERR("Fail to open verify file for part (%s)\n", partName);
        return ITEM_NOT_EXIST;
    }

    imgItemSz = (int)image_item_get_size(hImgItem);
    if(imgItemSz > MaxSz || !imgItemSz){
        DWN_ERR("verify file size %d for part %s invalid, max is %d\n", imgItemSz, partName, MaxSz);
        ret = __LINE__; goto _finish;
    }
    DWN_DBG("item sz %u\n", imgItemSz);

    ret = image_item_read(hImg, hImgItem, CmdVerify, imgItemSz);
    if(ret){
        DWN_ERR("Fail to read verify item for part %s\n", partName);
        goto _finish;
    }
    CmdVerify[imgItemSz] = 0;
    DWN_DBG("verify[%s]\n", CmdVerify);

    argv[0] = "verify";
    ret = parse_line(CmdVerify, argv + 1);
    if(ret != 2){
        DWN_ERR("verify cmd argc must be 2, but %d\n", ret);
        return __LINE__;
    }

    ret = optimus_media_download_verify(3, argv, _errInfo);
    if(ret){
        DWN_ERR("Fail when verify\n");
        return __LINE__;
    }

_finish:
    image_item_close(hImgItem);
    return ret;
}

int optimus_burn_one_partition(const char* partName, HIMAGE hImg, __hdle hUiProgress)
{
    int rcode = 0;
    s64 imgItemSz       = 0;
    s64 leftItemSz      = 0;
    u32 thisReadLen     = 0;
    __hdle hImgItem     = NULL;
    char* downTransBuf  = NULL;//get buffer from optimus_buffer_manager
    const unsigned ItemReadBufSz = OPTIMUS_DOWNLOAD_SLOT_SZ;//read this size from image item each time
    unsigned sequenceNo = 0;
    const char* fileFmt = NULL;
    /*static */char _errInfo[512];
    unsigned itemSizeNotAligned = 0;

    printf("\n");
    DWN_MSG("=====>To burn part [%s]\n", partName);
    optimus_progress_ui_printf("Burning part[%s]\n", partName);
    hImgItem = image_item_open(hImg, "PARTITION", partName);
    if(!hImgItem){
        DWN_ERR("Fail to open item for part (%s)\n", partName);
        return __LINE__;
    }

    imgItemSz = leftItemSz = image_item_get_size(hImgItem);
    if(!imgItemSz){
        DWN_ERR("image size is 0 , image of part (%s) not exist ?\n", partName);
        return __LINE__;
    }

    fileFmt = (IMAGE_ITEM_TYPE_SPARSE == image_item_get_type(hImgItem)) ? "sparse" : "normal";

    itemSizeNotAligned = image_item_get_first_cluster_size(hImgItem);
    leftItemSz        -= itemSizeNotAligned;
    rcode = sdc_burn_buf_manager_init(partName, imgItemSz, fileFmt, itemSizeNotAligned);
    if(rcode){
        DWN_ERR("fail in sdc_burn_buf_manager_init, rcode %d\n", rcode);
        return __LINE__;
    }

    //for each loop: 
    //1, get buffer from buffer_manager, 
    //2, read item data to buffer, 
    //3, report data ready to buffer_manager
    for(; leftItemSz > 0; leftItemSz -= thisReadLen, sequenceNo++)
    {
        thisReadLen = leftItemSz > ItemReadBufSz ? ItemReadBufSz: (u32)leftItemSz;

        rcode = optimus_buf_manager_get_buf_for_bulk_transfer(&downTransBuf, thisReadLen, sequenceNo, _errInfo);
        if(rcode){
            DWN_ERR("fail in get buf, msg[%s]\n", _errInfo);
            goto _finish;
        }

		//If the item head is not alinged to FAT cluster, Read it firstly to speed up mmc read
        if(itemSizeNotAligned && !sequenceNo)
        {
            DWN_DBG("itemSizeNotAligned 0x%x\n", itemSizeNotAligned);
            rcode = image_item_read(hImg, hImgItem, downTransBuf - itemSizeNotAligned, itemSizeNotAligned);
            if(rcode){
                DWN_ERR("fail in read data from item,rcode %d, len 0x%x, sequenceNo %d\n", rcode, itemSizeNotAligned, sequenceNo);
                goto _finish;
            }
        }

        rcode = image_item_read(hImg, hImgItem, downTransBuf, thisReadLen);
        if(rcode){
            DWN_ERR("fail in read data from item,rcode %d\n", rcode);
            goto _finish;
        }

        rcode = optimus_buf_manager_report_transfer_complete(thisReadLen, _errInfo);
        if(rcode){
            DWN_ERR("fail in report data ready, rcode %d\n", rcode);
            goto _finish;
        }
        if(hUiProgress)optimus_progress_ui_update_by_bytes(hUiProgress, thisReadLen);
    }

    DWN_DBG("BURN part %s %s!\n", partName, leftItemSz ? "FAILED" : "SUCCESS");

_finish:
    image_item_close(hImgItem);

    if(rcode){
        DWN_ERR("Fail to burn part(%s) with in format (%s) before verify\n", partName, fileFmt);
        optimus_progress_ui_printf("Failed at burn part[%s] befor VERIFY\n", partName);
        return rcode;
    }

#if 1
    rcode = optimus_verify_partition(partName, hImg, _errInfo);
    if(ITEM_NOT_EXIST == rcode)
    {
        printf("WRN:part(%s) NOT verified\n", partName);
        return 0;
    }
    if(rcode) {
        printf("Fail in verify part(%s)\n", partName);
        optimus_progress_ui_printf("Failed at VERIFY part[%s]\n", partName);
        return __LINE__;
    }
#endif//#fi 0

    return rcode;
}

int optimus_sdc_burn_partitions(ConfigPara_t* pCfgPara, HIMAGE hImg, __hdle hUiProgress)
{
    BurnParts_t* cfgParts = &pCfgPara->burnParts;
    int burnNum       = cfgParts->burn_num;
    int i = 0;
    int rcode = 0;

    //update burn_parts para if burnNum is 0, i.e, not configured
    if(!burnNum)
    {
        rcode = get_burn_parts_from_img(hImg, pCfgPara);
        if(rcode){
            DWN_ERR("Fail to get burn parts from image\n");
            return __LINE__;
        }
        burnNum = cfgParts->burn_num;
        DWN_DBG("Data part num %d\n", burnNum);
    }
    if(!burnNum){
        DWN_ERR("Data part num is 0!!\n");
        return __LINE__;
    }

    for (i = 0; i < burnNum; i++) 
    {
        const char* partName = cfgParts->burnParts[i];

        rcode = optimus_burn_one_partition(partName, hImg, hUiProgress);
        if(rcode){
            DWN_ERR("Fail in burn part %s\n", partName);
            return __LINE__;
        }
    }

    return rcode;
}

//not need to verify as not config ??
int optimus_sdc_burn_media_partition(const char* mediaImgPath, const char* verifyFile)
{
    //TODO:change configure to 'partName = image' and work it using cmd 'sdc_update'
    return optimus_burn_partition_image("media", mediaImgPath, "normal", verifyFile, 0);
}

int optimus_burn_bootlader(HIMAGE hImg)
{
    int rcode = 0;

    rcode = optimus_burn_one_partition("bootloader", hImg, NULL);
    if(rcode){
        DWN_ERR("Fail when burn bootloader\n");
        return __LINE__;
    }
    
    return rcode;
}

//flag, 0 is burn completed, else burn failed
static int optimus_report_burn_complete_sta(int isFailed, int rebootAfterBurn)
{
    if(isFailed)
    {
        DWN_MSG("======sdc burn Failed!!!!!\n");
        DWN_MSG("PLS long-press power key to shut down\n");
        optimus_led_show_burning_failure();
        while(1){
            /*if(ctrlc())*/
        }

        return __LINE__;
    }

    DWN_MSG("======sdc burn SUCCESS.\n");
    optimus_led_show_burning_success();
    optimus_burn_complete(rebootAfterBurn ? rebootAfterBurn : OPTIMUS_BURN_COMPLETE__POWEROFF_AFTER_POWERKEY);//set complete flag and poweroff if burn successful
    return 0;
}

static int sdc_burn_dtb_load(HIMAGE hImg)
{
    s64 itemSz = 0;
    HIMAGEITEM hImgItem = NULL;
    int rc = 0;
    const char* partName = "dtb";
    const u64 partBaseOffset = OPTIMUS_DOWNLOAD_TRANSFER_BUF_ADDR; 
    unsigned char* dtbTransferBuf     = (unsigned char*)(unsigned)partBaseOffset;

    hImgItem = image_item_open(hImg, partName, "meson");
    if(!hImgItem){
        DWN_ERR("Fail to open verify file for part (%s)\n", partName);
        return ITEM_NOT_EXIST;
    }

    itemSz = image_item_get_size(hImgItem);
    if(!itemSz){
        DWN_ERR("Item size 0\n");
        image_item_close(hImgItem); return __LINE__;
    }

    rc = image_item_read(hImg, hImgItem, dtbTransferBuf, (unsigned)itemSz);
    if(rc){
        DWN_ERR("Failed at item read, rc = %d\n", rc);
        image_item_close(hImgItem); return __LINE__;
    }
    image_item_close(hImgItem);

    rc = optimus_parse_img_download_info(partName, itemSz, "normal", "mem", partBaseOffset);
    if(rc){
        DWN_ERR("Failed in init down info\n"); return __LINE__;
    }

    {
        unsigned wrLen = 0;
        char errInfo[512];

        wrLen = optimus_download_img_data(dtbTransferBuf, (unsigned)itemSz, errInfo);
        rc = (wrLen == itemSz) ? 0 : wrLen;
    }

    return rc;
}

int optimus_burn_with_cfg_file(const char* cfgFile)
{
    extern ConfigPara_t g_sdcBurnPara ;

    int ret = 0;
    HIMAGE hImg = NULL;
    ConfigPara_t* pSdcCfgPara = &g_sdcBurnPara;
    const char* pkgPath = pSdcCfgPara->burnEx.pkgPath;
    __hdle hUiProgress = NULL;

    ret = parse_ini_cfg_file(cfgFile);
    if(ret){
        DWN_ERR("Fail to parse file %s\n", cfgFile);
        ret = __LINE__; goto _finish;
    }

    if(pSdcCfgPara->custom.eraseBootloader)
    {
        if(is_bootloader_old())
        {
#if ROM_BOOT_SKIP_BOOT_ENABLED
            optimus_enable_romboot_skip_boot();
#else
            DWN_MSG("To erase OLD bootloader !\n");
            ret = optimus_erase_bootloader(NULL);
            if(ret){
                DWN_ERR("Fail to erase bootloader\n");
                ret = __LINE__; goto _finish;
            }

            //As env also depended on flash init, don't use it if skip_boot supported
#if 0//Check reboot_mode == meson_sdc_burner_reboot instead of env 'upgrade_step'
            ret = setenv("upgrade_step", "0");
            if(ret){
                DWN_ERR("Fail set upgrade_step to def value 0\n");
                ret = __LINE__; goto _finish;
            }
            ret = run_command("saveenv", 0);
            if(ret){
                DWN_ERR("Fail to saveenv before reset\n");
                ret = __LINE__; goto _finish;
            }
#endif//#if 0
#endif// #if ROM_BOOT_SKIP_BOOT_ENABLED

#if defined(CONFIG_VIDEO_AMLLCD)
            //axp to low power off LCD, no-charging
            DWN_MSG("To close LCD\n");
            ret = run_command("video dev disable", 0);
            if(ret){
                printf("Fail to close back light\n");
                /*return __LINE__;*/
            }
#endif// #if defined(CONFIG_VIDEO_AMLLCD)

            DWN_MSG("Reset to load NEW uboot from ext-mmc!\n");
            optimus_reset(OPTIMUS_BURN_COMPLETE__REBOOT_SDC_BURN);
            return __LINE__;//should never reach here!!
        }
    }

    if(OPTIMUS_WORK_MODE_SDC_PRODUCE == optimus_work_mode_get())//led not depend on image res, can init early
    {
        if(optimus_led_open(LED_TYPE_PWM)){
            DWN_ERR("Fail to open led for sdc_produce\n");
            return __LINE__;
        }
        optimus_led_show_in_process_of_burning();
    }

    hImg = image_open(pkgPath);
    if(!hImg){
        DWN_ERR("Fail to open image %s\n", pkgPath);
        ret = __LINE__; goto _finish;
    }

    //update dtb for burning drivers
    ret = sdc_burn_dtb_load(hImg);
    if(ret){
        DWN_ERR("Fail in load dtb for sdc_burn\n");
        ret = __LINE__; goto _finish;
    }

    if(video_res_prepare_for_upgrade(hImg)){
        DWN_ERR("Fail when prepare bm res or init video for upgrade\n");
        ret = __LINE__; goto _finish;
    }
    show_logo_to_report_burning();

    hUiProgress = optimus_progress_ui_request_for_sdc_burn();
    if(!hUiProgress){
        DWN_ERR("request progress handle failed!\n");
        ret = __LINE__; goto _finish;
    }
    optimus_progress_ui_direct_update_progress(hUiProgress, UPGRADE_STEPS_AFTER_IMAGE_OPEN_OK);

    ret = optimus_storage_init(pSdcCfgPara->custom.eraseFlash);
    if(ret){
        DWN_ERR("Fail to init stoarge for sdc burn\n");
        return __LINE__;
    }

    optimus_progress_ui_direct_update_progress(hUiProgress, UPGRADE_STEPS_AFTER_DISK_INIT_OK);
    ret = optimus_progress_ui_set_smart_mode(hUiProgress, get_data_parts_size(hImg), 
                UPGRADE_STEPS_FOR_BURN_DATA_PARTS_IN_PKG(!pSdcCfgPara->burnEx.bitsMap.mediaPath));
    if(ret){
        DWN_ERR("Fail to set smart mode\n");
        ret = __LINE__; goto _finish;
    }
    
    ret = optimus_sdc_burn_partitions(pSdcCfgPara, hImg, hUiProgress);
    if(ret){
        DWN_ERR("Fail when burn partitions\n");
        ret = __LINE__; goto _finish;
    }

    if(pSdcCfgPara->burnEx.bitsMap.mediaPath)//burn media image
    {
        const char* mediaPath = pSdcCfgPara->burnEx.mediaPath;

        ret = optimus_sdc_burn_media_partition(mediaPath, NULL);//no progress bar info if have partition image not in package
        if(ret){
            DWN_ERR("Fail to burn media partition with image %s\n", mediaPath);
            optimus_storage_exit();
            return __LINE__;
        }
    }
    optimus_progress_ui_direct_update_progress(hUiProgress, UPGRADE_STPES_AFTER_BURN_DATA_PARTS_OK);

#if 1
    //burn bootloader
    ret = optimus_burn_bootlader(hImg);
    if(ret){
        DWN_ERR("Fail in burn bootloader\n");
        goto _finish;
    }
    ret = optimus_set_burn_complete_flag();
    if(ret){
        DWN_ERR("Fail in set_burn_complete_flag\n");
        ret = __LINE__; goto _finish;
    }
#endif
    optimus_progress_ui_direct_update_progress(hUiProgress, UPGRADE_STEPS_AFTER_BURN_BOOTLOADER_OK);

_finish:
    image_close(hImg);
    optimus_progress_ui_report_upgrade_stat(hUiProgress, !ret);
    optimus_report_burn_complete_sta(ret, pSdcCfgPara->custom.rebootAfterBurn);
    optimus_progress_ui_release(hUiProgress);
    //optimus_storage_exit();//temporary not exit storage driver when failed as may continue burning after burn
    return ret;
}

int optimus_burn_package_in_sdmmc(const char* sdc_cfg_file)
{
    int rcode = 0;

#if defined(CONFIG_AML_MESON_8)
        AML_WATCH_DOG_DISABLE(); //disable watchdog
#endif//#ifdef CONFIG_AML_MESON_8

    DWN_MSG("mmcinfo\n");
    rcode = run_command("mmcinfo", 0);
    if(rcode){
        DWN_ERR("Fail in init mmc, Does sdcard not plugged in?\n");
        return __LINE__;
    }

#if 0//this asserted by 'run update' and 'aml_check_is_ready_for_sdc_produce'
    rcode = do_fat_get_fileSz(sdc_cfg_file);
    if(!rcode){
        printf("The [%s] not exist in bootable mmc card\n", sdc_cfg_file);
        return __LINE__;
    }
#endif//#if 0

    rcode = optimus_burn_with_cfg_file(sdc_cfg_file);

    return rcode;
}

int do_sdc_burn(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rcode = 0;
    const char* sdc_cfg_file = argv[1];

    if(argc < 2 ){
        cmd_usage(cmdtp);
        return __LINE__;
    }

    optimus_work_mode_set(OPTIMUS_WORK_MODE_SDC_UPDATE);
    show_logo_to_report_burning();//indicate enter flow of burning! when 'run update'
    if(optimus_led_open(LED_TYPE_PWM)){
        DWN_ERR("Fail to open led for burn\n");
        return __LINE__;
    }
    optimus_led_show_in_process_of_burning();

    rcode = optimus_burn_package_in_sdmmc(sdc_cfg_file);

    return rcode;
}

U_BOOT_CMD(
   sdc_burn,      //command name
   5,               //maxargs
   0,               //repeatable
   do_sdc_burn,   //command function
   "Burning with amlogic format package in sdmmc ",           //description
   "argv: [sdc_burn_cfg_file]\n"//usage
   "    -aml_sdc_burn.ini is usually used configure file\n"
);

