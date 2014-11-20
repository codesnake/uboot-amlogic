/*
 * \file        optimus_ini_parser.c
 * \brief       ini parser for sdc burnning
 *
 * \version     1.0.0
 * \date        2013-7-11
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic. All Rights Reserved.
 *
 */
#include "optimus_sdc_burn_i.h"

#define dbg(fmt ...)  //printf(fmt)
#define msg           DWN_MSG
#define err           DWN_ERR

#define  SET_BURN_PARTS     "burn_parts"
#define  SET_CUSTOM_PARA    "common"
#define  SET_BURN_PARA_EX    "burn_ex"

static const char* _iniSets[] = {
    SET_BURN_PARTS      ,
    SET_CUSTOM_PARA     ,
    SET_BURN_PARA_EX    ,
};

#define TOTAL_SET_NUM   ( sizeof(_iniSets)/sizeof(const char*) )

ConfigPara_t g_sdcBurnPara = {
    .setsBitMap.burnParts   = 0,
    .setsBitMap.custom      = 0,
    .setsBitMap.burnEx      = 0,

    .burnParts      = {
        .burn_num           = 0,
        .bitsMap4BurnParts  = 0,
    },

    .custom         = {
        .eraseBootloader    = 1,//default to erase bootloader!
        .eraseFlash         = 0,
        .bitsMap.eraseBootloader    = 0,
        .bitsMap.eraseFlash         = 0,
    },

    .burnEx         = {
        .bitsMap.pkgPath    = 0,
        .bitsMap.mediaPath  = 0,
    },
};

int init_config_para(ConfigPara_t* pCfgPara)
{
    memset(pCfgPara, 0, sizeof(ConfigPara_t));

    pCfgPara->setsBitMap.burnParts   = 0;
    pCfgPara->setsBitMap.custom      = 0;
    pCfgPara->setsBitMap.burnEx      = 0;

    pCfgPara->burnParts.burn_num           = 0;
    pCfgPara->burnParts.bitsMap4BurnParts  = 0;

    pCfgPara->custom.eraseBootloader    = 1;//default to erase bootloader!
    pCfgPara->custom.eraseFlash         = 0;
    pCfgPara->custom.bitsMap.eraseBootloader    = 0;
    pCfgPara->custom.bitsMap.eraseFlash         = 0;

    pCfgPara->burnEx.bitsMap.pkgPath    = 0;
    pCfgPara->burnEx.bitsMap.mediaPath  = 0;

    return 0;
}

int print_burn_parts_para(const BurnParts_t* pBurnParts)
{
    int partIndex = 0;

    printf("[%s]\n", SET_BURN_PARTS);
    printf("burn_num         = %d\n", pBurnParts->burn_num);

    for(; partIndex < pBurnParts->burn_num; ++partIndex)
    {
        printf("burn_part%d       = %s\n", partIndex, pBurnParts->burnParts[partIndex]);
    }
    printf("\n");

    return 0;
}

int print_sdc_burn_para(const ConfigPara_t* pCfgPara)
{
    printf("\n=========sdc_burn_paras=====>>>\n");

    {
        const CustomPara_t* pCustom = &pCfgPara->custom;
        
        printf("[%s]\n", SET_CUSTOM_PARA);
        printf("erase_bootloader = %d\n", pCustom->eraseBootloader);
        printf("erase_flash      = %d\n", pCustom->eraseFlash);
        printf("reboot           = 0x%x\n", pCustom->rebootAfterBurn);
        printf("\n");
    }

    {
        const BurnEx_t*     pBurnEx = &pCfgPara->burnEx;

        printf("[%s]\n", SET_BURN_PARA_EX);
        printf("package          = %s\n", pBurnEx->pkgPath);
        printf("media            = %s\n", pBurnEx->mediaPath);
        printf("\n");
    }

    print_burn_parts_para(&pCfgPara->burnParts);

    printf("<<<<=====sdc_burn_paras======\n\n");

    return 0;
}

int parse_set_burnEx(const char* key, const char* strVal)
{
    BurnEx_t* pBurnEx = &g_sdcBurnPara.burnEx;

    if (!strcmp("package", key))
    {
        if (pBurnEx->bitsMap.pkgPath) {
            err("key package in burn_ex is duplicated!\n");
            return __LINE__;
        }
        if (!strVal) {
            err("value for package in set burn_ex can't be empty!\n");
            return __LINE__;
        }

        strcpy(pBurnEx->pkgPath, strVal);
        pBurnEx->bitsMap.pkgPath = 1;

        return 0;
    }

    if (!strcmp("media", key))
    {
        if (pBurnEx->bitsMap.mediaPath) {
            err("key media in burn_ex is duplicated!\n");
            return __LINE__;
        }
        if (strVal)
        {
            strcpy(pBurnEx->mediaPath, strVal);
            pBurnEx->bitsMap.mediaPath = 1;
        }

        return 0;
    }

    return 0;
}

int parse_set_custom_para(const char* key, const char* strVal)
{
    CustomPara_t* pCustome = &g_sdcBurnPara.custom;

    if(!strcmp(key, "erase_bootloader"))
    {
        if (pCustome->bitsMap.eraseBootloader) {
            goto _key_dup;
        }
        
        if(strVal)
        {
            pCustome->eraseBootloader = simple_strtoul(strVal, NULL, 0);
            pCustome->bitsMap.eraseBootloader = 1;
        }

    }

    if (!strcmp(key, "erase_flash"))
    {
        if (pCustome->bitsMap.eraseFlash) {
            goto _key_dup;
        }

        if(strVal)
        {
            pCustome->eraseFlash = simple_strtoul(strVal, NULL, 0);
            pCustome->bitsMap.eraseFlash = 1;
        }

    }

    if (!strcmp(key, "reboot"))
    {
        if (pCustome->bitsMap.rebootAfterBurn) {
            goto _key_dup;
        }

        if(strVal)
        {
            pCustome->rebootAfterBurn = simple_strtoul(strVal, NULL, 0);
            pCustome->bitsMap.rebootAfterBurn = 1;
        }

    }

    return 0;

_key_dup:
    err("key %s is duplicated!\n", key);
    return -1;
}

int check_custom_para(const CustomPara_t* pCustome)
{
    //TODO: not completed!!
    return 0;
}

int parse_burn_parts(const char* key, const char* strVal)
{
    BurnParts_t* pBurnParts = &g_sdcBurnPara.burnParts;

    if ( !strcmp("burn_num", key) )
    {
        if(!strVal){
            err("burn_num in burn_parts can't be empty!!");
            return __LINE__;
        }

        pBurnParts->burn_num = simple_strtoul(strVal, NULL, 0);
        if(pBurnParts->burn_num < 1){
            err("value for burn_num in burn_parts in invalid\n");
            return __LINE__;
        }

        return 0;
    }

    if(pBurnParts->burn_num < 1){
        err("burn_num is not config or 0 ??\n");
        return __LINE__;
    }

    {
        const char burn_partx[] = "burn_partx";
        const int  validKeyLen = sizeof(burn_partx) - 2;
        const int totalBurnNum = pBurnParts->burn_num;
        int burnIndex = 0;
        char* partName = NULL;

        if (strncmp(burn_partx, key, validKeyLen))
        {
            err("error burn part name [%s]\n", key);
            return __LINE__;
        }

        burnIndex = key[validKeyLen] - '0';
        if (!(burnIndex >= 0 && burnIndex < totalBurnNum))
        {
            err("Error \"%s\", only burn_part[0~%d] is valid as burn_num is %d\n", 
                key, totalBurnNum - 1, totalBurnNum);
            return __LINE__;
        }

        if (pBurnParts->bitsMap4BurnParts & (1U<<burnIndex)) {
            err("key %s is duplicated in burn_parts\n", key);
            return __LINE__;
        }
        pBurnParts->bitsMap4BurnParts |= 1U<<burnIndex;

        partName = (char*)pBurnParts->burnParts[burnIndex];
        if (!strVal) {
            err("value of %s can't empty\n", key);
            return __LINE__;
        }
        
        if(!strcmp("bootloader", strVal)){
            err("bootloader not need to configure at burn_parts\n");
            return __LINE__;
        }

        strcpy(partName, strVal);
    }

    return 0;
}

int check_cfg_burn_parts(const ConfigPara_t* burnPara)
{
    const BurnParts_t* pBurnParts = &burnPara->burnParts;
    const int cfgBurnNum    = pBurnParts->burn_num;
    const unsigned bitsMap  = pBurnParts->bitsMap4BurnParts;
    int mediaPathHasCfg = burnPara->burnEx.bitsMap.mediaPath;
    int i = 0;


    for (i = 0; i < cfgBurnNum; i++) 
    {
        int b = bitsMap & (1U<<i);

        if(!b){
            err("Please cfg burn_part%d\n", i);
            return __LINE__;
        }

        if(mediaPathHasCfg)
        {
            if(!strcmp(pBurnParts->burnParts[i], "media")) {
                DWN_ERR("media can't cfg in both media_path and burn_parts\n");
                return __LINE__;
            }
        }
    }

    return 0;
}

#define MAX_ARGS    4
#define is_space_char(c) ('\t' == c || ' ' == c)
#define is_delimeter(c)  ('[' == c || ']' == c || '=' == c)

#define is_valid_char(c) ( ('0' <= c && '9' >= c) || ('_' == c)\
                        || ('a' <= c && 'z' >= c) || ('A' <= c && 'Z' >= c) ) \
                        || ('.' == c) || ('\\' == c) || ('/' == c) || ('-' == c)

static int line_is_valid(char* line)
{
    char c = 0;

    while(c = *line++, c)
    {
        int ret = is_delimeter(c) || is_valid_char(c) || is_space_char(c);

        if(!ret){
            err("line contain invalid chars!! ascii val(0x%x)\n", c);
            return 0;
        }
    }

    return 1;//line is valid
}

//valid lines type: set or key/value pair
enum _INI_LINE_TYPE{
    INI_LINE_TYPE_ERR       = 0,
    INI_LINE_TYPE_SET          ,
    INI_LINE_TYPE_KE_VALUE     ,
};

//this func is used after line_is_valid
static int line_2_words(char* line, char* argv[], const int maxWords)
{
    int nargs = 0;
    char cur = 0;

    for(cur = *line; is_space_char(cur); cur = *++line){}

    argv[nargs++] = line;
    for(;cur = *line, cur; ++line)
    {
        if(!is_space_char(cur))continue;
        //following do with space character

        *line = 0; 
        for(cur = *++line; is_space_char(cur) && cur; cur = *++line){}//ignore all space between words

        if(!cur)break;//line ended

        argv[nargs++] = line;
        if(maxWords <= nargs){
            err("too many words num %d, max is %d\n", nargs, maxWords);
            return 0;
        }
    }

    return nargs;
}

//1, Read the whole file content to buffer
//2, parse file content to lines
//3, parse each valid line 
static int parse_ini_file(const char* filePath, u8* iniBuf, const unsigned bufSz)
{
    const int MaxLines = 1024;//
    const int MaxWordsALine = 32;

    char* lines[MaxLines];
    char* wordsALine[MaxWordsALine];
    int ret = 0;
    unsigned fileSz = 0;
    unsigned lineNum = 0; 
    int nwords = 0;
    int currentSetIndex = -1;
    int hFile = -1;
    unsigned readLen = 0;
    unsigned i = 0;
    unsigned lineIndex = 0; 

    fileSz = (unsigned)do_fat_get_fileSz(filePath);
    if(!fileSz){
        err("File %s not exist in sdcard??\n", filePath);
        return __LINE__;
    }
    if(fileSz >= bufSz){
        err("file size 0x%x illegal, buf size is 0x%x\n", fileSz, bufSz);
        return __LINE__;
    }
    DWN_MSG("ini sz 0x%xB\n", fileSz);

    hFile = do_fat_fopen(filePath);
    if(hFile < 0){
        err("Fail to open file %s\n", filePath);
        return __LINE__;
    }

    readLen = do_fat_fread(hFile, iniBuf, fileSz);
    if(readLen != fileSz){
        err("failed to load cfg file, want size 0x%x, but 0x%x\n", fileSz, readLen);
        do_fat_fclose(hFile);
        return __LINE__;
    }
    iniBuf[fileSz] = 0;

    do_fat_fclose(hFile);

    dbg("\\r is 0x%x\t, \\n is 0x%x\n", '\r', '\n');
    char* curLine = (char*)iniBuf;
    char* pTemp = curLine;

    //step1:first loop to seprate buffer to lines
    for (i = 0; i < fileSz ; i++, ++pTemp)
    {
        char c = *pTemp;
        int isFileEnd = i + 1 >= fileSz;

        if(MaxLines <= lineNum){
            err("total line number %d too many, at most %d lines!\n", lineNum, MaxLines);
            break;
        }

        if(isFileEnd)
        {
            lines[lineNum++] = curLine;
            break;//End to read file if file ended
        }

        if('\r' != c && '\n' != c) {
            continue;
        }
        *pTemp = 0;///

        dbg("%3d: %s\n", lineNum, curLine);
        if('\r' == c)//for DOS \r\n mode
        {
            if('\n' == pTemp[1])
            {
                lines[lineNum++] = curLine;

                ++pTemp;
                curLine = pTemp + 1;
                ++i;//skip '\n' which follows '\r'
            }
            else
            {
                err("Syntax error at line %d, DOS end \\r\\n, but \\r%x\n", lineNum + 1, pTemp[1]);
                return __LINE__;
            }
        }
        else if('\n' == c)//for UNIX '\n' mode
        {
            lines[lineNum++] = curLine;
            curLine = pTemp + 1;
        }
    }
    
    //step 2: abandon comment or space lines
    for (lineIndex = 0; lineIndex < lineNum ; lineIndex++)
    {
        int isSpaceLine = 1;
        char c = 0;
        curLine = lines[lineIndex];

        while(c = *curLine++, c)
        {
            //escape space and tab
            if (is_space_char(c))
            {
                continue;
            }
            
            isSpaceLine = 0;//no space line
            //test if frist char is comment delimeter
            if(';' == c)
            {
                lines[lineIndex] = NULL;//invalid comment lines
            }
        }

        //if all character is space or tab, also invlalid it 
        if (isSpaceLine)
        {
            lines[lineIndex] = NULL;
        }
    }

    dbg("\nvalid lines:\n");
    for (lineIndex = 0; lineIndex < lineNum ; lineIndex++)
    {
        int lineType = INI_LINE_TYPE_ERR;
        const char* iniKey = NULL;
        const char* iniVal = NULL;
        const char* iniSet = NULL;

        curLine = lines[lineIndex];

        if(!curLine)continue;

        if(!line_is_valid(curLine)) //only comment lines can contain non-ASCII letters
        {
            err("line %d contain invalid chars\n", lineIndex + 1);
            ret = __LINE__;
            break;
        }
        dbg("%3d: %s\n",lineIndex, curLine);

        nwords = line_2_words(curLine, wordsALine, MaxWordsALine);
        if(nwords <= 0){
            ret = __LINE__;
            break;
        }
        if(nwords > 3){
            err("line %d error: ini support at most 3 words, but %d\n", lineIndex + 1, nwords);
            ret = __LINE__;
            break;
        }

        switch (nwords)
        {
        case 3:
            {
                if (!strcmp("=", wordsALine[1]))//k/v pair
                {
                    lineType = INI_LINE_TYPE_KE_VALUE;
                    iniKey = wordsALine[0]; iniVal = wordsALine[2];
                    break;
                }
                else if(!strcmp("[" , wordsALine[0]) && !strcmp("]" , wordsALine[2]))//set line
                {
                    lineType = INI_LINE_TYPE_SET;
                    iniSet = wordsALine[1];
                    break;
                }
                else
                {
                    lineType = INI_LINE_TYPE_ERR;
                    err("Ini syntax error when parse line %d\n", lineIndex + 1);
                    ret = __LINE__; break;
                }
            }
        	break;

        case 2:
            {
                if('[' == wordsALine[0][0])//set like "[set ]" or "[ set]"
                {
                    if(!strcmp("]", wordsALine[1]))
                    {
                        lineType = INI_LINE_TYPE_SET;
                        iniSet = wordsALine[0] + 1;
                        break;
                    }
                    else if (']' == wordsALine[1][strlen(wordsALine[1]) - 1] && !strcmp("[", wordsALine[0]))
                    {
                        lineType = INI_LINE_TYPE_SET;
                        iniSet = wordsALine[1];
                        wordsALine[1][strlen(wordsALine[1]) - 1] = 0;
                        break;
                    }
                }
                else if(!strcmp("=", wordsALine[1]))//k/v pair like "key = " 
                {
                    lineType = INI_LINE_TYPE_KE_VALUE;
                    iniKey = wordsALine[0];
                    break;
                }
                else if('=' == wordsALine[1][0])//k/v pair like "key =v" or "key= v"
                {
                    lineType = INI_LINE_TYPE_KE_VALUE;
                    iniKey = wordsALine[0];
                    iniVal = wordsALine[1] + 1;
                    break;
                }
                else if ('=' == wordsALine[0][strlen(wordsALine[0]) - 1])//k/v pair like "key= v"
                {
                    wordsALine[0][strlen(wordsALine[0]) - 1] = 0;
                    lineType = INI_LINE_TYPE_KE_VALUE;
                    iniKey = wordsALine[0];
                    iniVal = wordsALine[1];
                }
            }
            break;

        case 1:
            {
                char* word = wordsALine[0];
                char firstChar = word[0];
                char lastChar  = word[strlen(word) - 1];

                if('[' == firstChar && ']' == lastChar)
                {
                    lineType = INI_LINE_TYPE_SET;
                    iniSet = word + 1;
                    word[strlen(word) - 1] = 0;
                    break;
                }
                else 
                {
                    char c = 0;

                    iniKey = word;
                    while(c = *word++, c)
                    {
                        if ('=' == c)//TODO: not assert only delimeter in a line yet
                        {
                            lineType = INI_LINE_TYPE_KE_VALUE;
                            *--word = 0;
                            iniVal = ++word;
                            iniVal = *iniVal ? iniVal : NULL;
                            break;
                        }
                    }
                }
            }
            break;

        default:
            break;
        }

        if (INI_LINE_TYPE_SET == lineType)
        {
            const char* setname = NULL;
            dbg("set line, set is %s\n", iniSet);
            
            currentSetIndex = -1;//set index to invalid

            for (i = 0; i < TOTAL_SET_NUM; i++)
            {
                setname = _iniSets[i];
                if (!strcmp(setname, iniSet))break;//set is useful for sdc burn
            }

            //set is useful
            if(i < TOTAL_SET_NUM)
            {
                if(!strcmp(setname, SET_BURN_PARTS))
                {
                    if(g_sdcBurnPara.setsBitMap.burnParts){
                        ret = __LINE__; goto _set_duplicated;
                    }
                    g_sdcBurnPara.setsBitMap.burnParts = 1;
                }

                if(!strcmp(setname, SET_BURN_PARA_EX))
                {
                    if(g_sdcBurnPara.setsBitMap.burnEx){
                        ret = __LINE__; goto _set_duplicated;
                    }
                    g_sdcBurnPara.setsBitMap.burnEx = 1;
                }

                if(!strcmp(setname, SET_CUSTOM_PARA))
                {
                    if(g_sdcBurnPara.setsBitMap.custom){
                        ret = __LINE__; goto _set_duplicated;
                    }
                    g_sdcBurnPara.setsBitMap.custom = 1;
                }

                currentSetIndex = i;//set set index to valid
            }

        }

        if(INI_LINE_TYPE_KE_VALUE == lineType)
        {
            dbg("k/v line, key (%s), val (%s)\n", iniKey, iniVal);

            if (currentSetIndex >= 0 && currentSetIndex < TOTAL_SET_NUM)//set is valid
            {
                const char* setName = _iniSets[currentSetIndex];

                if (!strcmp(setName, SET_BURN_PARTS))
                {
                    ret = parse_burn_parts(iniKey, iniVal);
                    if(ret){
                        ret = __LINE__; goto _line_err;
                    }
                }

                if (!strcmp(setName, SET_BURN_PARA_EX))
                {
                    ret = parse_set_burnEx(iniKey, iniVal);
                    if(ret){
                        ret = __LINE__; goto _line_err;
                    }
                }

                if (!strcmp(setName, SET_CUSTOM_PARA))
                {
                    ret = parse_set_custom_para(iniKey, iniVal);
                    if(ret){
                        ret = __LINE__; goto _line_err;
                    }
                }
            }
        }
    }


	return ret;

_line_err:
    err("Fail to parse line %d\n", lineIndex + 1);
    return ret;

_set_duplicated:
    err("line %d err:set is duplicated!!\n", lineIndex + 1);
    return ret;
}

int parse_ini_cfg_file(const char* filePath)
{
    const int MaxFileSz = OPTIMUS_DOWNLOAD_SLOT_SZ;
    u8* CfgFileLoadAddr = (u8*)OPTIMUS_DOWNLOAD_TRANSFER_BUF_ADDR;
    int rcode = 0;
   
    init_config_para(&g_sdcBurnPara);

    rcode = parse_ini_file(filePath, CfgFileLoadAddr, MaxFileSz);
    if(rcode){
        err("error in parse ini file\n");
        return __LINE__;
    }

    rcode = check_cfg_burn_parts(&g_sdcBurnPara);
    if(rcode){
        err("Fail in check burn parts.\n");
        return __LINE__;
    }

    print_sdc_burn_para(&g_sdcBurnPara);

    return 0;
}

#define MYDBG 0
#if MYDBG
int do_ini_parser(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rcode = 0;
    const char* filePath = "dos_dc_burn.ini";
    
    //mmc info to ensure sdcard inserted and inited, mmcinfo outer as there U-disk later
    rcode = run_command("mmcinfo", 0);
    if(rcode){
        err("Fail in init mmc, Does sdcard not plugged in?\n");
        return __LINE__;
    }

    if(2 <= argc){
        filePath = argv[1];
    }

    rcode = parse_ini_cfg_file(filePath);
    if(rcode){
        err("error in parse ini file\n");
        return __LINE__;
    }

    return 0;
}

U_BOOT_CMD(
   ini_parser,      //command name
   5,               //maxargs
   0,               //repeatable
   do_ini_parser,   //command function
   "Burning a partition from sdmmc ",           //description
   "Usage: sdc_update partiton image_file_path fileFmt(android sparse, other normal) [,verify_file]\n"   //usage
);
#endif//#if MYDBG

