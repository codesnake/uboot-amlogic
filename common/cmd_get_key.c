/*
 * (C) Copyright 2013 Amlogic, Inc
 *
 * This file is used to get keys from flash(nand or emmc)/efuse
 * More detail to check the command "get_key" usage
 * 
 * cheng.wang@amlogic.com,
 * 2013-07-12 @ Shenzhen
 *
 */
#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <config.h>
#include <asm/arch/io.h>
#include <amlogic/efuse.h>

#define SET_DEFAULT_SERIALNO_IF_NOT_GET     1

#if defined(CONFIG_SECURITYKEY)
extern ssize_t uboot_get_keylist(char *listkey);
extern ssize_t uboot_key_read(char *keyname, char *keydata);
extern int uboot_key_initial(char *device);
#else
static ssize_t uboot_get_keylist(char *listkey)
{
    return -1;
}
static ssize_t uboot_key_read(char *keyname, char *keydata)
{
    return -1;
}
static int uboot_key_initial(char *device)
{
    return -1;
}
#endif  /* CONFIG_SECURITYKEY */

#define EFUSE_READ  1
#define KEY_BYTES     512
#define AML_KEY_NAMELEN  16
#define AML_DEVICE_NAMELEN  10

enum {
    FAILED = -1,
    SUCCESS = 0,
    HAVE_NOT_WRITEN,
}command_return_type;

static int inited_secukey_flag = 0;
static int ensure_init_secukey(char *dev)
{
    int ret = -1;

    if(inited_secukey_flag) {
        printf("device already inited!!\n");
        return 1;
    }

    printf("device init start\n");

    if(!strncmp(dev, "flash", strlen("flash"))) {
        ret = uboot_key_initial("auto");
        if(ret >= 0) {
            printf("%s init key ok!!\n", dev);
            inited_secukey_flag = 1;
            ret = 0;
        }
        else {
            printf("%s init error!!\n", dev);
            ret  = -1;
        }
    }
    else {
        printf("no \"%s\" device to mach!!\n", dev);
    }
    
    return ret;
}

static char hex_to_asc(char para)
{
    if(para >= 0 && para <= 9)
        para = para + '0';
    else if(para >= 0xa && para <= 0xf)
        para = para + 'a' - 0xa;
		
    return para;
}

static char asc_to_hex(char para)
{
    if(para >= '0' && para <= '9')
        para = para - '0';
    else if(para >= 'a' && para <= 'f')
        para = para - 'a' + 0xa;
    else if(para >= 'A' && para <= 'F')
        para = para - 'A' + 0xa;
		
    return para;
}

/**
  *     read_secukey
  *     command in *argv[], and read datas saved in buf
  *     return: 0->success, 1->have not writen, -1->failed
  **/
static int read_secukey(int argc, const char *argv[], char *buf)
{
    int i = 0, j = 0, ret = 0, error = -1;
    char *secukey_cmd = NULL, *secukey_name = NULL;
    char namebuf[AML_KEY_NAMELEN], databuf[4096], listkey[1024];

    /* at least two arguments please */
    if(argc < 2)
        goto err;

    memset(namebuf, 0, sizeof(namebuf));
    memset(databuf, 0, sizeof(databuf));
    memset(listkey, 0, sizeof(listkey));

    secukey_cmd = (char *)argv[1];
    if(inited_secukey_flag) {
        if(argc > 2 && argc < 5) {
            if(!strncmp(secukey_cmd, "read", strlen("read"))) {
                if(argc > 3)
                    goto err;
                ret = uboot_get_keylist(listkey);
                printf("all key names list are(ret=%d):\n%s", ret, listkey);
                secukey_name = (char *)argv[2];
                strncpy(namebuf, secukey_name, strlen(secukey_name));
                error = uboot_key_read(namebuf, databuf);
                if(error >= 0) {    //read success
                    memset(buf, 0, KEY_BYTES);
                    for(i=0; i<KEY_BYTES*2; i++)
                        if(databuf[i]) printf("%c:", databuf[i]);
                    printf("\n");
                    for(i=0,j=0; i<KEY_BYTES*2; i++,j++) {
                        buf[j]= (((asc_to_hex(databuf[i]))<<4) | (asc_to_hex(databuf[++i])));
                    }
                    printf("%s is: ", namebuf);
                    for(i=0; i<KEY_BYTES; i++)
                        if(buf[i])  printf("%02x:", buf[i]);
                    printf("\nread ok!!\n");
                    return 0;
                }
                else {                      // read error or have not been writen
                    if(!strncmp(namebuf, "mac_bt", 6) || !strncmp(namebuf, "mac_wifi", 8))
                        ;
                    else if(!strncmp(namebuf, "mac", 3)) {
                        memset(namebuf, 0, sizeof(namebuf));
                        sprintf(namebuf, "%s", "mac\n");
                    }

                    if(strstr(listkey, namebuf)) {
                        printf("find key name: %s, but read error!!\n", namebuf);
                        goto err;       // read error 
                    }
                    else {
                        printf("not find key name: %s, and it doesn't be writen before!!\n", namebuf);
                        return 1;       // has been not writen
                    }
                }
            }
            else {
                printf("Not allow to write!!\n");
                goto err;
            }
        }
    }
    else {
        printf("flash device uninitialized!!");
        goto err;
    }

err:
    return -1;
}

static int read_efuse(int argc, const char *argv[], char *buf)
{
    int i, action = -1;
    efuseinfo_item_t info;
    char *title;
    char *s;
    char *end;
        
    if(!strncmp(argv[1], "read", 4))
        action = EFUSE_READ;
    else {
        printf("%s arg error\n", argv[1]);
        return -1;
    }
				
    if(action == EFUSE_READ) {
        title = argv[2];
        if(efuse_getinfo(title, &info) < 0)
            return -1;

        memset(buf, 0, EFUSE_BYTES);
        efuse_read_usr(buf, info.data_len, (loff_t *)&info.offset);	
        printf("%s is: ", title);
        for(i=0; i<(info.data_len); i++)
            printf("%02x:", buf[i]);
        printf("\n");
    }
    else {
        printf("arg error\n");
        return -1;
    }

    return 0;
}


/* --do_get_mac
  * get mac key from flash(nand or emmc)/efuse
  * key is mac
  * device is flash(nand or emmc)/efuse
  * result:if key exists in device,then set " androidboot.mac=xx:xx:xx:xx:xx:xx" to bootargs
  * use command getprop ro.boot.mac to display in android
  */
static int do_get_mac(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "mac"};
    char *eArgv[3] = {"efuse", "read", "mac"};
    char *key_str = " androidboot.mac=";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            sprintf(key_data, "%s", buff);
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        ret = read_efuse(Argc, eArgv, buff);
        if(!ret) {
            for(i=0; i<6; i++) {
                if(buff[i] != 0x00) {
                    simbol = 1;
                    break;
                }
            }
            if(simbol) {
                sprintf(key_data, "%02x:%02x:%02x:%02x:%02x:%02x", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
                printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                strcpy(bootargs_str, getenv("bootargs"));
                if(strstr(bootargs_str, key_str) == NULL) {
                    strcat(bootargs_str, key_str);
                    strcat(bootargs_str, key_data);
                    setenv("bootargs", bootargs_str);
                    printf("bootargs=%s\n", bootargs_str);
                }
                else {
                    printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
                }
                ret = SUCCESS;
            }
            else {
                printf("%s has been not writen in %s\n", key_name, device);
                ret = HAVE_NOT_WRITEN;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            sprintf(key_data, "%s", buff);
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            ret = read_efuse(Argc, eArgv, buff);
            if(!ret) {
                for(i=0; i<6; i++) {
                    if(buff[i] != 0x00) {
                        simbol = 1;
                        break;
                    }
                }
                if(simbol) {
                    sprintf(key_data, "%02x:%02x:%02x:%02x:%02x:%02x", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
                    printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                    strcpy(bootargs_str, getenv("bootargs"));
                    if(strstr(bootargs_str, key_str) == NULL) {
                        strcat(bootargs_str, key_str);
                        strcat(bootargs_str, key_data);
                        setenv("bootargs", bootargs_str);
                        printf("bootargs=%s\n", bootargs_str);
                    }
                    else {
                        printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
                    }
                    ret = SUCCESS;
                }
                else {
                    printf("%s has been not writen in %s\n", key_name, device);
                    ret = HAVE_NOT_WRITEN;
                }
            }
            else {
                printf("read %s failed from %s\n", key_name, device);
                ret = FAILED;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

/* --do_get_mac_bt
  * get mac_bt key from flash(nand or emmc)/efuse
  * key is mac_bt
  * device is flash(nand or emmc)/efuse
  * result:if key exists in device,then set " androidboot.mac_bt=xx:xx:xx:xx:xx:xx" to bootargs
  * use command getprop ro.boot.mac_bt to display in android
  */
static int do_get_mac_bt(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
     int i, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "mac_bt"};
    char *eArgv[3] = {"efuse", "read", "mac_bt"};
    char *key_str = " androidboot.mac_bt=";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            sprintf(key_data, "%s", buff);
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        ret = read_efuse(Argc, eArgv, buff);
        if(!ret) {
            for(i=0; i<6; i++) {
                if(buff[i] != 0x00) {
                    simbol = 1;
                    break;
                }
            }
            if(simbol) {
                sprintf(key_data, "%02x:%02x:%02x:%02x:%02x:%02x", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
                printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                strcpy(bootargs_str, getenv("bootargs"));
                if(strstr(bootargs_str, key_str) == NULL) {
                    strcat(bootargs_str, key_str);
                    strcat(bootargs_str, key_data);
                    setenv("bootargs", bootargs_str);
                    printf("bootargs=%s\n", bootargs_str);
                }
                else {
                    printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
                }
                ret = SUCCESS;
            }
            else {
                printf("%s has been not writen in %s\n", key_name, device);
                ret = HAVE_NOT_WRITEN;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            sprintf(key_data, "%s", buff);
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            ret = read_efuse(Argc, eArgv, buff);
            if(!ret) {
                for(i=0; i<6; i++) {
                    if(buff[i] != 0x00) {
                        simbol = 1;
                        break;
                    }
                }
                if(simbol) {
                    sprintf(key_data, "%02x:%02x:%02x:%02x:%02x:%02x", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
                    printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                    strcpy(bootargs_str, getenv("bootargs"));
                    if(strstr(bootargs_str, key_str) == NULL) {
                        strcat(bootargs_str, key_str);
                        strcat(bootargs_str, key_data);
                        setenv("bootargs", bootargs_str);
                        printf("bootargs=%s\n", bootargs_str);
                    }
                    else {
                        printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
                    }
                    ret = SUCCESS;
                }
                else {
                    printf("%s has been not writen in %s\n", key_name, device);
                    ret = HAVE_NOT_WRITEN;
                }
            }
            else {
                printf("read %s failed from %s\n", key_name, device);
                ret = FAILED;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

/* --do_get_mac_wifi
  * get mac_wifi key from flash(nand or emmc)/efuse
  * key is mac_wifi
  * device is flash(nand or emmc)/efuse
  * result:if key exists in device,then set " androidboot.mac_wifi=xx:xx:xx:xx:xx:xx" to bootargs
  * use command getprop ro.boot.mac_wifi to display in android
  */
static int do_get_mac_wifi(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "mac_wifi"};
    char *eArgv[3] = {"efuse", "read", "mac_wifi"};
    char *key_str = " androidboot.mac_wifi=";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            sprintf(key_data, "%s", buff);
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        ret = read_efuse(Argc, eArgv, buff);
        if(!ret) {
            for(i=0; i<6; i++) {
                if(buff[i] != 0x00) {
                    simbol = 1;
                    break;
                }
            }
            if(simbol) {
                sprintf(key_data, "%02x:%02x:%02x:%02x:%02x:%02x", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
                printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                strcpy(bootargs_str, getenv("bootargs"));
                if(strstr(bootargs_str, key_str) == NULL) {
                    strcat(bootargs_str, key_str);
                    strcat(bootargs_str, key_data);
                    setenv("bootargs", bootargs_str);
                    printf("bootargs=%s\n", bootargs_str);
                }
                else {
                    printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
                }
                ret = SUCCESS;
            }
            else {
                printf("%s has been not writen in %s\n", key_name, device);
                ret = HAVE_NOT_WRITEN;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            sprintf(key_data, "%s", buff);
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            ret = read_efuse(Argc, eArgv, buff);
            if(!ret) {
                for(i=0; i<6; i++) {
                    if(buff[i] != 0x00) {
                        simbol = 1;
                        break;
                    }
                }
                if(simbol) {
                    sprintf(key_data, "%02x:%02x:%02x:%02x:%02x:%02x", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
                    printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                    strcpy(bootargs_str, getenv("bootargs"));
                    if(strstr(bootargs_str, key_str) == NULL) {
                        strcat(bootargs_str, key_str);
                        strcat(bootargs_str, key_data);
                        setenv("bootargs", bootargs_str);
                        printf("bootargs=%s\n", bootargs_str);
                    }
                    else {
                        printf("androidboot.%s is exist in bootargs,%s\n", sArgv[2], strstr(bootargs_str, key_str));
                    }
                    ret = SUCCESS;
                }
                else {
                    printf("%s has been not writen in %s\n", key_name, device);
                    ret = HAVE_NOT_WRITEN;
                }
            }
            else {
                printf("read %s failed from %s\n", key_name, device);
                ret = FAILED;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

/* --do_get_usid
  * get usid key from flash(nand or emmc)/efuse
  * key is usid
  * device is flash(nand or emmc)/efuse
  * result:if key exists in device,then set " androidboot.serialno=xxxxxx" to bootargs
  * use command getprop ro.boot.serialno or ro.serialno to display in android
  * if not get any usid,so set default value to ro.boot.serialno
  */
static int do_get_usid(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i = 0, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "usid"};
    char *eArgv[3] = {"efuse", "read", "usid"};
    char *key_str = " androidboot.serialno=";
    char *default_usid = "1234567890";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
#if defined(SET_DEFAULT_SERIALNO_IF_NOT_GET)
            printf("now set default usid value(%s)\n", default_usid);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, default_usid);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
#else
            ret = HAVE_NOT_WRITEN;
#endif
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        ret = read_efuse(Argc, eArgv, buff);
        if(!ret) {
            for(i=0; i<strlen(buff); i++) {
                if(buff[i] != 0x00) {
                    simbol = 1;
                    break;
                }
            }
            if(simbol) {
                memcpy(key_data, buff, strlen(buff));
                printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                strcpy(bootargs_str, getenv("bootargs"));
                if(strstr(bootargs_str, key_str) == NULL) {
                    strcat(bootargs_str, key_str);
                    strcat(bootargs_str, key_data);
                    setenv("bootargs", bootargs_str);
                    printf("bootargs=%s\n", bootargs_str);
                }
                else {
                    printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
                }
                ret = SUCCESS;
            }
            else {
                printf("%s has been not writen in %s\n", key_name, device);
#if defined(SET_DEFAULT_SERIALNO_IF_NOT_GET)
                printf("now set default usid value(%s)\n", default_usid);
                strcpy(bootargs_str, getenv("bootargs"));
                if(strstr(bootargs_str, key_str) == NULL) {
                    strcat(bootargs_str, key_str);
                    strcat(bootargs_str, default_usid);
                    setenv("bootargs", bootargs_str);
                    printf("bootargs=%s\n", bootargs_str);
                }
                else {
                    printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
                }
                ret = SUCCESS;
#else
                ret = HAVE_NOT_WRITEN;
#endif
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            ret = read_efuse(Argc, eArgv, buff);
            if(!ret) {
                for(i=0; i<strlen(buff); i++) {
                    if(buff[i] != 0x00) {
                        simbol = 1;
                        break;
                    }
                }
                if(simbol) {
                    memcpy(key_data, buff, strlen(buff));
                    printf("read %s success,%s=%s\n", key_name, key_name, key_data);
                    strcpy(bootargs_str, getenv("bootargs"));
                    if(strstr(bootargs_str, key_str) == NULL) {
                        strcat(bootargs_str, key_str);
                        strcat(bootargs_str, key_data);
                        setenv("bootargs", bootargs_str);
                        printf("bootargs=%s\n", bootargs_str);
                    }
                    else {
                        printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
                    }
                    ret = SUCCESS;
                }
                else {
                    printf("%s has been not writen in %s\n", key_name, device);
#if defined(SET_DEFAULT_SERIALNO_IF_NOT_GET)
                    printf("now set default usid value(%s)\n", default_usid);
                    strcpy(bootargs_str, getenv("bootargs"));
                    if(strstr(bootargs_str, key_str) == NULL) {
                        strcat(bootargs_str, key_str);
                        strcat(bootargs_str, default_usid);
                        setenv("bootargs", bootargs_str);
                        printf("bootargs=%s\n", bootargs_str);
                    }
                    else {
                        printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
                    }
                    ret = SUCCESS;
#else
                    ret = HAVE_NOT_WRITEN;
#endif
                }
            }
            else {
                printf("read %s failed from %s\n", key_name, device);
                ret = FAILED;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

/* --do_get_serialno
  * get serialno key from nand/emmc
  * key is serialno
  * device is nand/emmc
  * result:if key exists in device,then set " androidboot.serialno2=xxxxxx" to bootargs
  * use command getprop ro.boot.serialno2 or getprop ro.serialno2 to display in android
  */
static int do_get_serialno(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i = 0, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "serialno"};
    char *eArgv[3] = {"efuse", "read", "serialno"};
    char *key_str = " androidboot.serialno2=";
    char *default_serialno = "0123456789";

    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
#if defined(SET_DEFAULT_SERIALNO_IF_NOT_GET)
            printf("now set default serialno value(%s)\n", default_serialno);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, default_serialno);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno2 is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
#else
            ret = HAVE_NOT_WRITEN;
#endif
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        printf("not support uuid key in efuse layout at present!!\n");
        ret = FAILED;
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
#if defined(SET_DEFAULT_SERIALNO_IF_NOT_GET)
            printf("now set default serialno value(%s)\n", default_serialno);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, default_serialno);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.serialno2 is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
#else
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            printf("not support boardid key in efuse layout at present!!\n");
            ret = FAILED;
#endif
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

#define HDCP_KEY_SIZE                   308
#define HDCP_IP_KEY_SIZE              288
#define TX_HDCP_DKEY_OFFSET       0x400
static unsigned char hdcp_keys_get[HDCP_KEY_SIZE] = {0x0};
static unsigned char hdcp_keys_reformat[HDCP_IP_KEY_SIZE] = {0x0};
extern void hdmi_hdcp_wr_reg(unsigned long addr, unsigned long data);
/* 
  * verify ksv, 20 ones and 20 zeroes
  */
static int hdcp_ksv_valid(unsigned char * dat)
{
    int i, j, one_num = 0;
    for(i = 0; i < 5; i++){
        for(j=0;j<8;j++) {
            if((dat[i]>>j)&0x1) {
                one_num++;
            }
        }
    }
    return (one_num == 20);
}

/*
  * copy the fetched data into HDMI IP
  */
static int init_hdcp_ram(unsigned char * dat, unsigned int pre_clear)
{
    int i, j;
    char value = 0x0;
    unsigned int ram_addr = 0x0;

    memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));
    //adjust the HDCP key's KSV & DPK position
    memcpy(&hdcp_keys_reformat[0], &hdcp_keys_get[8], HDCP_IP_KEY_SIZE-8);   //DPK
    memcpy(&hdcp_keys_reformat[280], &hdcp_keys_get[0], 5);   //KSV
    j = 0;
    for (i = 0; i < HDCP_IP_KEY_SIZE - 3; i++) {                                //ignore 3 zeroes in reserved KSV
        value = hdcp_keys_reformat[i];
        ram_addr = TX_HDCP_DKEY_OFFSET+j;
        hdmi_hdcp_wr_reg(ram_addr, value);
        j = ((i % 7) == 6) ? j + 2: j + 1;
    }

    if(hdcp_ksv_valid(dat)) {
        printf("hdcp init done\n");
    }
    else {
        if(pre_clear == 0)
            printf("AKSV invalid, hdcp init failed\n");
        else
            printf("pre-clear hdmi ram\n");
    }

    return 1;
}

/* --do_get_hdcp
  * get hdcp key from flash(nand or emmc)/efuse
  * key is hdcp
  * device is nand/emmc
  * result:if key exists in device,then set hdcp key to HDMI IP
  */
static int do_get_hdcp(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i = 0, simbol = 0, ret = -1, Argc = 3;
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], buff[KEY_BYTES];
    char *sArgv[3] = {"secukey", "read", "hdcp"};
    char *eArgv[3] = {"efuse", "read", "hdcp"};

    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));

    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }

    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("device:%s,start to read %s...\n", device, key_name);

    init_hdcp_ram(hdcp_keys_get, 1);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "nand", 4) ||!strncmp(device, "emmc", 4) ||!strncmp(device, "flash", 5) ) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(hdcp_keys_get, buff, HDCP_IP_KEY_SIZE);
            printf("read %s success from %s,%s is:\n", key_name, device, key_name);
            for(i=0; i<HDCP_IP_KEY_SIZE; i++)
                printf("%02x:", hdcp_keys_get[i]);
            printf("\n");
            init_hdcp_ram(hdcp_keys_get, 0);
            memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));  //clear the buffer to prevent reveal keys
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        ret = read_efuse(Argc, eArgv, buff);
        if(!ret) {
            for(i=0; i<HDCP_IP_KEY_SIZE; i++) {
                if(buff[i] != 0x00) {
                    simbol = 1;
                    break;
                }
            }
            if(simbol) {
                memcpy(hdcp_keys_get, buff, HDCP_IP_KEY_SIZE);
                printf("read %s success from %s,%s is:\n", key_name, device, key_name);
                for(i=0; i<HDCP_IP_KEY_SIZE; i++)
                    printf("%02x:", hdcp_keys_get[i]);
                printf("\n");
                init_hdcp_ram(hdcp_keys_get, 0);
                memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));  //clear the buffer to prevent reveal keys
                ret = SUCCESS;
            }
            else {
                printf("%s has been not writen in %s\n", key_name, device);
                ret = HAVE_NOT_WRITEN;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(hdcp_keys_get, buff, HDCP_IP_KEY_SIZE);
            printf("read %s success from %s,%s is:\n", key_name, device, key_name);
            for(i=0; i<HDCP_IP_KEY_SIZE; i++)
                printf("%02x:", hdcp_keys_get[i]);
            printf("\n");
            init_hdcp_ram(hdcp_keys_get, 0);
            memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));  //clear the buffer to prevent reveal keys
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            ret = read_efuse(Argc, eArgv, buff);
            if(!ret) {
                for(i=0; i<HDCP_IP_KEY_SIZE; i++) {
                    if(buff[i] != 0x00) {
                        simbol = 1;
                        break;
                    }
                }
                if(simbol) {
                    memcpy(hdcp_keys_get, buff, HDCP_IP_KEY_SIZE);
                    printf("read %s success from %s,%s is:\n", key_name, device, key_name);
                    for(i=0; i<HDCP_IP_KEY_SIZE; i++)
                        printf("%02x:", hdcp_keys_get[i]);
                    printf("\n");
                    init_hdcp_ram(hdcp_keys_get, 0);
                    memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));  //clear the buffer to prevent reveal keys
                    ret = SUCCESS;
                }
                else {
                    printf("%s has been not writen in %s\n", key_name, device);
                    ret = HAVE_NOT_WRITEN;
                }
            }
            else {
                printf("read %s failed from %s\n", key_name, device);
                ret = FAILED;
            }
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }

    return ret;
}


/* --do_get_boardid
  * get boardid key from nand/emmc
  * key is boardid
  * device is nand/emmc
  * result:if key exists in device,then set " androidboot.boardid=xxxxxx" to bootargs
  * use command getprop ro.boot.boardid to display in android
  */
static int do_get_boardid(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i = 0, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "boardid"};
    char *eArgv[3] = {"efuse", "read", "boardid"};
    char *key_str = " androidboot.boardid=";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.boardid is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        printf("not support boardid key in efuse layout at present!!\n");
        ret = FAILED;
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.boardid is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            printf("not support boardid key in efuse layout at present!!\n");
            ret = FAILED;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

/* --do_get_boardinfo
  * get boardinfo key from nand/emmc
  * key is boardinfo
  * device is nand/emmc
  * result:if key exists in device,then set " androidboot.boardinfo=xxxxxx" to bootargs
  * use command getprop ro.boot.boardinfo to display in android
  */
static int do_get_boardinfo(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i = 0, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "boardinfo"};
    char *eArgv[3] = {"efuse", "read", "boardinfo"};
    char *key_str = " androidboot.boardinfo=";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.boardinfo is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        printf("not support boardinfo key in efuse layout at present!!\n");
        ret = FAILED;
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.boardinfo is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            printf("not support boardinfo key in efuse layout at present!!\n");
            ret = FAILED;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}

/* --do_get_uuid
  * get uuid key from nand/emmc
  * key is uuid
  * device is nand/emmc
  * result:if key exists in device,then set " androidboot.uuid=xxxxxx" to bootargs
  * use command getprop ro.boot.uuid to display in android
  */
static int do_get_uuid(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int i = 0, simbol = 0, ret = -1, Argc = 3;
    char buff[KEY_BYTES], key_data[KEY_BYTES];
    char device[AML_DEVICE_NAMELEN], key_name[AML_KEY_NAMELEN], bootargs_str[1024];
    char *sArgv[3] = {"secukey", "read", "uuid"};
    char *eArgv[3] = {"efuse", "read", "uuid"};
    char *key_str = " androidboot.uuid=";
    
    memset(buff, 0, sizeof(buff));
    memset(device, 0, sizeof(device));
    memset(key_name, 0, sizeof(key_name));
    memset(key_data, 0, sizeof(key_data));
    memset(bootargs_str, 0, sizeof(bootargs_str));
    
    if(argc != 2 || strncmp(argv[0], sArgv[2], strlen(sArgv[2]))) {
        printf("arg cmd not mach\n");
        return FAILED;
    }
    
    strncpy(key_name, argv[0], strlen(argv[0])); 
    strncpy(device, argv[1], strlen(argv[1]));

    printf("type:%s,start to read %s...\n", device, key_name);
    
    /* read from flash(nand/emmc) */
    if(!strncmp(device, "flash", strlen("flash"))) {
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.uuid is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) {
            printf("%s has been not writen in %s\n", key_name, device);
            ret = HAVE_NOT_WRITEN;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        }
    }

    /* read from efuse */
    else if(!strncmp(device, "efuse", 5)) {
        printf("not support uuid key in efuse layout at present!!\n");
        ret = FAILED;
    }
    
    /* read from flash(nand/emmc) first. if not read any datas,then read from efuse */
    else if(!strncmp(device, "auto", 4)) {
        memset(device, 0, sizeof(device));
        strncpy(device, "flash", strlen("flash"));
        printf("read %s from %s first\n", key_name, device);
        ret = ensure_init_secukey(device);
        if (ret == 0) {
            printf("init %s success\n", device);
        }
        else if(ret == 1) {
            printf("%s already inited\n", device);
        }
        else {
            printf("init %s failed\n", device);
            return FAILED;
        }
        
        ret = read_secukey(Argc, sArgv, buff);
        if(!ret) {                                      
            memcpy(key_data, buff, strlen(buff));
            printf("read %s success,%s=%s\n", key_name, key_name, key_data);
            strcpy(bootargs_str, getenv("bootargs"));
            if(strstr(bootargs_str, key_str) == NULL) {
                strcat(bootargs_str, key_str);
                strcat(bootargs_str, key_data);
                setenv("bootargs", bootargs_str);
                printf("bootargs=%s\n", bootargs_str);
            }
            else {
                printf("androidboot.uuid is exist in bootargs,%s\n", strstr(bootargs_str, key_str));
            }
            ret = SUCCESS;
        }
        else if(ret == 1) { //it doesn't read any datas,then read from efuse
            printf("%s has been not writen in %s\n", key_name, device);
            strncpy(device, "efuse", strlen("efuse"));
            printf("start to read %s from %s...\n", key_name, device);
            printf("not support boardid key in efuse layout at present!!\n");
            ret = FAILED;
        }
        else {
            printf("read %s failed from %s\n", key_name, device);
            ret = FAILED;
        } 
    }

    /* device not mach */
    else {
        printf("No \"%s\" device!!\n", device);
        cmd_usage(cmdtp);
    }
    
    return ret;
}


static cmd_tbl_t cmd_getkey_sub[] = {
    U_BOOT_CMD_MKENT(mac, 2, 0, do_get_mac, "", ""),
    U_BOOT_CMD_MKENT(mac_bt, 2, 0, do_get_mac_bt, "", ""),
    U_BOOT_CMD_MKENT(mac_wifi, 2, 0, do_get_mac_wifi, "", ""),
    U_BOOT_CMD_MKENT(usid, 2, 0, do_get_usid, "", ""),
    U_BOOT_CMD_MKENT(serialno, 2, 0, do_get_serialno, "", ""),
    U_BOOT_CMD_MKENT(hdcp, 2, 0, do_get_hdcp, "", ""),
    U_BOOT_CMD_MKENT(boardid, 2, 0, do_get_boardid, "", ""),
    U_BOOT_CMD_MKENT(boardinfo, 2, 0, do_get_boardinfo, "", ""),
    U_BOOT_CMD_MKENT(uuid, 2, 0, do_get_uuid, "", ""),
};

static int do_get_key(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    cmd_tbl_t *c;

    if(argc != 3)
        return cmd_usage(cmdtp);

    argc--;
    argv++;

    c = find_cmd_tbl(argv[0], &cmd_getkey_sub[0], ARRAY_SIZE(cmd_getkey_sub));

    if(c)
        return  c->cmd(cmdtp, flag, argc, argv);
    else
        return cmd_usage(cmdtp);
}

U_BOOT_CMD(
    get_key, 3, 0, do_get_key,
    "get_key sub-system",
    "[key] [device] - get key from device\n"
    "--[key]: mac,mac_bt,mac_wifi,usid,hdcp,boardid,boardinfo,uuid,serialno\n"
    "--[device]: flash,efuse\n"
    "This command will get key from flash(nand or emmc) or efuse,and then set it to bootargs\n"
    "Notice:\n"
    "--only support to read mac,mac_bt,mac_wifi,usid,hdcp from efuse,and support all keys to read from flash(nand or emmc)\n"
    "--if [key] is \"hdcp\", so set hdcp key datas to HDMI IP directly after read successful from device,not set it to bootargs\n"
    "--if [device] is \"flash\", so read key from flash(nand or emmc),select device automatically\n"
    "--if [device] is \"auto\", so read key from flash(nand or emmc) first,select device automatically.if not read any datas,then read from efuse\n"
    "--cmd eg:get_key usid flash, get_key usid efuse, get_key usid auto\n"
);


/*
    ---------------------------------------------------------------------------------
  */
/* --do_cancel_mac
  * cancel mac key from bootargs
  * result: get rid of " androidboot.mac=xx:xx:xx:xx:xx:xx" from bootargs if it exists in bootargs
  */
static int do_cancel_mac(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int str_len = 0;
    char bootargs_str[1024] = {0}, backup_1[1024] = {0}, backup_2[1024] = {0};
    char buff[1024] = {0};
    char *str = buff;
    char *key_str = " androidboot.mac";

    if(strcmp(argv[0], "mac")) {
        printf("Arg cmd not mach\n");
        return FAILED;
    }

    strcpy(bootargs_str, getenv("bootargs"));
    str = strstr(bootargs_str, key_str);
    if(str != NULL) {
        str_len = strlen(str);
        strncpy(backup_1, bootargs_str, strlen(bootargs_str) - str_len);
        do {
            str ++;
            if(*str == ' ' || *str == '\0') {
                strncpy(backup_2, str, strlen(str));
                break;
            }
        }while(*str != '\0');

        memset(bootargs_str, 0, sizeof(bootargs_str));
        strcat(backup_1, backup_2);
        strncpy(bootargs_str, backup_1, strlen(backup_1));
        setenv("bootargs", bootargs_str);
        printf("bootargs=%s\n", bootargs_str);
    }
    else {
        printf("No find \"%s\" in bootargs\n", key_str);
    }

    return 0;
}

/* --do_cancel_mac_bt
  * cancel mac_bt key from bootargs
  * result: get rid of " androidboot.mac_bt=xx:xx:xx:xx:xx:xx" from bootargs if it exists in bootargs
  */
static int do_cancel_mac_bt(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int str_len = 0;
    char bootargs_str[1024] = {0}, backup_1[1024] = {0}, backup_2[1024] = {0};
    char buff[1024] = {0};
    char *str = buff;
    char *key_str = " androidboot.mac_bt";

    if(strcmp(argv[0], "mac_bt")) {
        printf("Arg cmd not mach\n");
        return FAILED;
    }

    strcpy(bootargs_str, getenv("bootargs"));
    str = strstr(bootargs_str, key_str);
    if(str != NULL) {
        str_len = strlen(str);
        strncpy(backup_1, bootargs_str, strlen(bootargs_str) - str_len);
        do {
            str ++;
            if(*str == ' ' || *str == '\0') {
                strncpy(backup_2, str, strlen(str));
                break;
            }
        }while(*str != '\0');

        memset(bootargs_str, 0, sizeof(bootargs_str));
        strcat(backup_1, backup_2);
        strncpy(bootargs_str, backup_1, strlen(backup_1));
        setenv("bootargs", bootargs_str);
        printf("bootargs=%s\n", bootargs_str);
    }
    else {
        printf("No find \"%s\" in bootargs\n", key_str);
    }

    return 0;
}

/* --do_cancel_mac_wifi
  * cancel mac_wifi key from bootargs
  * result: get rid of " androidboot.mac_wifi=xx:xx:xx:xx:xx:xx" from bootargs if it exists in bootargs
  */
static int do_cancel_mac_wifi(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int str_len = 0;
    char bootargs_str[1024] = {0}, backup_1[1024] = {0}, backup_2[1024] = {0};
    char buff[1024] = {0};
    char *str = buff;
    char *key_str = " androidboot.mac_wifi";

    if(strcmp(argv[0], "mac_wifi")) {
        printf("Arg cmd not mach\n");
        return FAILED;
    }

    strcpy(bootargs_str, getenv("bootargs"));
    str = strstr(bootargs_str, key_str);
    if(str != NULL) {
        str_len = strlen(str);
        strncpy(backup_1, bootargs_str, strlen(bootargs_str) - str_len);
        do {
            str ++;
            if(*str == ' ' || *str == '\0') {
                strncpy(backup_2, str, strlen(str));
                break;
            }
        }while(*str != '\0');

        memset(bootargs_str, 0, sizeof(bootargs_str));
        strcat(backup_1, backup_2);
        strncpy(bootargs_str, backup_1, strlen(backup_1));
        setenv("bootargs", bootargs_str);
        printf("bootargs=%s\n", bootargs_str);
    }
    else {
        printf("No find \"%s\" in bootargs\n", key_str);
    }

    return 0;
}

static cmd_tbl_t cmd_cancelkey_sub[] = {
    U_BOOT_CMD_MKENT(mac, 1, 0, do_cancel_mac, "", ""),
    U_BOOT_CMD_MKENT(mac_bt, 1, 0, do_cancel_mac_bt, "", ""),
    U_BOOT_CMD_MKENT(mac_wifi, 1, 0, do_cancel_mac_wifi, "", ""),
//    U_BOOT_CMD_MKENT(usid, 2, 0, do_cancel_usid, "", ""),
//    U_BOOT_CMD_MKENT(boardid, 2, 0, do_cancel_boardid, "", ""),
};

static int do_cancel_key(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    cmd_tbl_t *c;

    if(argc != 2)
        return cmd_usage(cmdtp);

    argc--;
    argv++;

    c = find_cmd_tbl(argv[0], &cmd_cancelkey_sub[0], ARRAY_SIZE(cmd_cancelkey_sub));

    if(c)
        return  c->cmd(cmdtp, flag, argc, argv);
    else
        return cmd_usage(cmdtp);
}

U_BOOT_CMD(
    cancel_key, 2, 0, do_cancel_key,
    "key sub-system",
    "[key] - cancel key from bootargs\n"
    "--key: mac,mac_bt,mac_wifi,usid,hdcp,boardid\n"
    "This command will cancel key from bootargs if it's exist\n"
    "example:\n"
    "cancel before: bootargs=init=/init console=ttyS0...chiprev=D androidboot.mac=0f:0f:a3:45:a0:14\n"
    "cancel after:   bootargs=init=/init console=ttyS0...chiprev=D\n"
);

