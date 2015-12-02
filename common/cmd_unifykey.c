/*
 * \file        cmd_key.c
 * \brief       get key based on drivers/keymanage
 *
 * \version     1.0.0
 * \date        2014/2/20
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2014 Amlogic Inc.. All Rights Reserved.
 *
 */
#include <common.h>
#include <amlogic/keyunify.h>
#include <malloc.h>

#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif//#ifdef CONFIG_OF_LIBFDT

#define debugP(fmt...) //printf("L%d:", __LINE__),printf(fmt)
#define errorP(fmt...) printf("Err key(L%d):", __LINE__),printf(fmt)
#define wrnP(fmt...)   printf("wrn:"fmt)

extern int v2_key_read(const char* keyName, u8* keyVal, const unsigned keyValLen, char* errInfo, unsigned* fmtLen);

static int key_drv_init(void)
{
    static int _keyDrvInited = 0;
    int rcode = 0;

    if(_keyDrvInited)return 0;

    rcode = key_unify_init(0, 0);//Not need to init secure storage for read emmc/efuse key, unifykey cmd used to read mmc/efuse key
    return rcode;
}

static int do_key_get(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rcode = 0;
    unsigned reallen = 0;
    char* keyName = argv[1];
    char* keyValBuf = NULL;
    const int keyValMaxLen = 1024;

    rcode = key_drv_init();
    if(rcode){
        errorP("fail in key_drv_init\n");
        return __LINE__;
    }

    keyValBuf = (char*)malloc(keyValMaxLen);
    if(!keyValBuf){
        errorP("malloc failed\n");
        return __LINE__;
    }
    rcode = v2_key_read(keyName, (u8*)keyValBuf, keyValMaxLen/2, keyValBuf + keyValMaxLen/2, &reallen);
    if(rcode < 0 || !reallen){
        debugP("failed to read key [%s], rc=%d, reallen=%d\n", keyName, rcode, reallen);
        free(keyValBuf);
        return __LINE__;
    }

    keyValBuf[reallen] = 0;
    setenv(keyName, keyValBuf);

    free(keyValBuf);
    return 0;
}

static cmd_tbl_t cmd_key_sub[] = {
	U_BOOT_CMD_MKENT(get,    4, 0, do_key_get, "", ""),
};

static int do_key(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	/* Strip off leading 'bmp' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_key_sub[0], ARRAY_SIZE(cmd_key_sub));

	if (c) {
		return	c->cmd(cmdtp, flag, argc, argv);
	} else {
		cmd_usage(cmdtp);
		return 1;
	}
}

U_BOOT_CMD(
   unifykey,         //command name
   5,               //maxargs
   0,               //repeatable
   do_key,   //command function
   "unifykey read/write based on the driver keymanage",           //description
   "    argv: <key> <get> <keyName> \n"   //usage
   "    which device the key read from is decided by dtd\n"   //usage
   "unifykey get  --- Read the key and save it as env if it is string\n"
   "    - e.g. \n"
   "        to read usid from emmc/nand and save value with env in name usid: <unifykey get usid> \n"   //usage
);

