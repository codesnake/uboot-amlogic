/*
 * \file        optimus_sdc_burn_i.h
 * \brief       internal struct types and interfaces for sdc burn
 *
 * \version     1.0.0
 * \date        2013-7-12
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic. All Rights Reserved.
 *
 */
#ifndef __OPTIMUS_SDC_BURN_I_H__
#define __OPTIMUS_SDC_BURN_I_H__

#include "../v2_burning_i.h"
#include <fat.h>
#include <part.h>

typedef struct _burnEx{
    char        pkgPath[128];
    char        mediaPath[128];
    struct {
        unsigned pkgPath    : 1;
        unsigned mediaPath  : 1;
        unsigned reserv     : 32 - 2;
    }bitsMap;
}BurnEx_t;

typedef struct _customPara{
    int         eraseBootloader;
    int         eraseFlash;
    int         rebootAfterBurn;
    struct{
        unsigned eraseBootloader    : 1;
        unsigned eraseFlash         : 1;
        unsigned rebootAfterBurn    : 1;
        unsigned resev              : 32 - 3;
    }bitsMap;
}CustomPara_t;

#define MAX_BURN_PARTS      (32)
#define PART_NAME_LEN_MAX   (32)

typedef struct _burnParts{
    int         burn_num;
    char        burnParts[MAX_BURN_PARTS][PART_NAME_LEN_MAX];
    unsigned    bitsMap4BurnParts;
}BurnParts_t;

typedef struct _ConfigPara{
    BurnParts_t     burnParts;
    CustomPara_t    custom;
    BurnEx_t        burnEx;
    struct {
        unsigned    burnParts : 1;
        unsigned    custom    : 1;
        unsigned    burnEx    : 1;
        unsigned    reserv    : 32 - 3;
    }setsBitMap;
}ConfigPara_t;

//ini parser
int parse_ini_cfg_file(const char* filePath);

int check_cfg_burn_parts(const ConfigPara_t* burnPara);
int print_burn_parts_para(const BurnParts_t* pBurnParts);

int sdc_burn_verify(const char* verifyFile);

//burn a partition with a image file
int optimus_burn_partition_image(const char* partName, const char* imgItemPath, const char* fileFmt, const char* verifyFile, const unsigned itemSizeNotAligned);

int sdc_burn_buf_manager_init(const char* partName, s64 imgItemSz, const char* fileFmt, 
                            const unsigned itemSizeNotAligned /* if item offset 3 and bytepercluste 4k, then it's 4k -3 */);

int get_burn_parts_from_img(HIMAGE hImg, ConfigPara_t* pcfg);

u64 get_data_parts_size(HIMAGE hImg);

#endif//#ifndef __OPTIMUS_SDC_BURN_I_H__

