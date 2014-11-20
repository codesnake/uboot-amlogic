/*
 * Command for accessing SPI flash.
 *
 * Copyright (C) 2008 Atmel Corporation
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <asm/io.h>
#include <amlogic/efuse.h>
extern unsigned int get_cpu_temp(int tsc,int flag);
extern int efuse_init(void);
extern ssize_t efuse_read(char *buf, size_t count, loff_t *ppos );

int do_read_efuse(int ppos,int *flag,int *temp,int *TS_C, int print)
{
    char buf[2];
    int ret;
    int cpu = 0;
    char *cpu_str[] = {"M8", "M8M2", "M8Baby"};
    *flag=0;
    buf[0]=0;buf[1]=0;
    //read efuse tsc,flag
    efuse_init();
    ret=efuse_read(buf,2,(loff_t *)&ppos);
    if (print) {
        printf("buf[0]=%x,buf[1]=%x\n",buf[0],buf[1]);
    }
    if (IS_MESON_M8_CPU) {
        *temp=buf[1];
        *temp=(*temp<<8)|buf[0];
        *TS_C=*temp&0xf;
        *flag=(*temp&0x8000)>>15;
        *temp=(*temp&0x7fff)>>4;
        cpu = 0;
    } else if (IS_MESON_M8M2_CPU) {
        *temp=buf[1];
        *temp=(*temp<<8)|buf[0];
        *TS_C=*temp&0x1f;
        *flag=(*temp&0x8000)>>15;
        *temp=(*temp&0x7fff)>>5;
        cpu = 1;
    } else if (IS_MESON_M8BABY_CPU) {
        *temp=buf[1];
        *temp=(*temp<<8)|buf[0];
        *TS_C=*temp&0x1f;
        *flag=(*temp&0x8000)>>15;
        *temp=(*temp&0x7fff)>>5;
        cpu = 2;
    }
    if (print) {
        printf("cpu:%s, adc=%d,TS_C=%d,flag=%d\n", cpu_str[cpu],*temp,*TS_C,*flag);
    }
    return ret;
}

static int do_read_temp(cmd_tbl_t *cmdtp, int flag1, int argc, char * const argv[])
{
    int temp = 0;
    int i,TS_C = 0;
    int flag,ret;
    int print = 0;
    static int saradc_open = 0;
    char buf[100] = {};

    if (!saradc_open) {
        run_command("saradc open", 0);
        saradc_open = 1;
    }
   
    i = 0;
    while (i < argc) {
        if (!strcmp(argv[i], "-p")) {
            print = 1;    
        }
        i++; 
    } 

    /*
     * add emmc key check here if efuse data is not correct
     */
    flag = 0;
    setenv("tempa", " ");
    ret = do_read_efuse(502, &flag, &temp, &TS_C, print);
    if (ret > 0) {
        int adc = get_cpu_temp(TS_C, flag);
        int tempa = 0;
        if (print) {
            printf("adc=%d\n",adc);
        }
        if (flag) {
            if (IS_MESON_M8_CPU) {
                tempa=(18*(adc-temp)*10000)/1024/10/85 + 27;
            } else if (IS_MESON_M8M2_CPU) {
                tempa=(10*(adc-temp))/32 + 27;
            } else if (IS_MESON_M8BABY_CPU){
                tempa=(10*(adc-temp))/32 + 27;
            } else {
                printf("CPU type unknow\n");
                return -1;
            }
            printf("%d\n",tempa);
            sprintf(buf, "%d", tempa);
            setenv("tempa", buf);
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "temp:%d, adc:%d, tsc:%d, dout:%d", tempa, adc, TS_C, temp);
            setenv("err_info_tempa", buf);
        } else {
            printf("This chip is not trimmed\n");
            sprintf(buf, "%s", "This chip is not trimmed");
            setenv("err_info_tempa", buf);
            return -1;
        }
    } else {
        printf("read efuse failed\n");
        sprintf(buf, "%s", "read efuse failed");
        setenv("err_info_tempa", buf);
    }
    return ret;
}

U_BOOT_CMD(
        cpu_temp,	5,	1,	do_read_temp,
        "cpu temp-system",
        "probe [bus:]cs [hz] [mode]	- init flash device on given SPI bus"
        );

