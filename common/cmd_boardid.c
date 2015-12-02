/*
 * (C) Copyright 2012
 * Amlogic. Inc. zongdong.jiao@amlogic.com  modify  haolong.zhang
 *
 * This file is used to prefetch/varify/compare boardid keys
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <asm/byteorder.h>

#include <amlogic/efuse.h>

#define BOARDID_SIZE  1024               //max BOARDID size

static char boardid_prefetch[BOARDID_SIZE] = { 0x00 };

char twoASCByteToByteforboardid(char c1, char c2)
{   
    char cha;
    if(c1>='0' && c1<='9')
        c1 = c1-'0';
    else if(c1>='a' && c1<='f')
        c1 = c1-'a'+0xa;
    else if(c1>='A' && c1<='F')
        c1 = c1-'A'+0xa;
    if(c2>='0' && c2<='9')
        c2 = c2-'0';
    else if(c2>='a' && c2<='f')
        c2 = c2-'a'+0xa;
    else if(c2>='A' && c2<='F')
        c2 = c2-'A'+0xa;    

    cha = c1 &0x0f;
    cha = ((cha<<4)|(c2&0x0f));
    return cha;             
} 

// Prefetch the baordid data from nand, efuse, etc
int do_boardid_prefetch(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int i,j,count=1,ret = 0,prefetch_flag = 0;
	char tmpbuf[1024],str[1024];
#ifdef CONFIG_AML_NAND_KEY
	run_command("secukey nand",0);
#endif

#ifdef CONFIG_AML_EMMC_KEY
	run_command("secukey emmc",0);
#endif

#ifdef CONFIG_STORE_COMPATIBLE && defined(CONFIG_SECURITYKEY)
    if (is_nand_exist()) {
        run_command("secukey nand",0);
    } else if (is_emmc_exist()) {
        run_command("secukey emmc",0);
    }
#endif

	ret=uboot_key_read("boardid",boardid_prefetch);
    if(ret >= 0) {
#ifdef CONFIG_AML_NAND_KEY
		printf("boardid keys read from nand success!\n");
#endif

#ifdef CONFIG_AML_EMMC_KEY
		printf("boardid keys read from emmc success!\n");
#endif

#ifdef CONFIG_STORE_COMPATIBLE && defined(CONFIG_SECURITYKEY)
    if (is_nand_exist()) {
		printf("boardid keys read from nand success!\n");
    } else if (is_emmc_exist()) {
		printf("boardid keys read from emmc success!\n");
    }
#endif

		char outputdata[2];
		memset(outputdata,0,2);
		for(i=0;i<BOARDID_SIZE;i++){
			outputdata[0]=boardid_prefetch[i];
			if(outputdata[0] == 00){
				count++;
				//printf("the data is 0!\n");
			}
		}	
		if(count >= BOARDID_SIZE){					
			prefetch_flag = 0;
			printf("prefetch boardid verify keys from %s failed\n", argv[1]);
#ifdef CONFIG_AML_NAND_KEY
			printf("nand key read boardid fail,boardid valid\n");
#endif

#ifdef CONFIG_AML_EMMC_KEY
			printf("emmc key read boardid fail,boardid valid\n");
#endif

#ifdef CONFIG_STORE_COMPATIBLE && defined(CONFIG_SECURITYKEY)
            if (is_nand_exist()) {
                printf("nand key read boardid fail,boardid valid\n");
            } else if (is_emmc_exist()) {
                printf("emmc key read boardid fail,boardid valid\n");
            }
#endif
		}else{
			for(i=0,j=0;i<ret;j++){
				tmpbuf[j] = twoASCByteToByteforboardid(boardid_prefetch[i],boardid_prefetch[i+1]);
				i += 2;
			}
#ifdef CONFIG_AML_NAND_KEY
			printf("nand boardid read OK,write to bootargs now...\n");
#endif

#ifdef CONFIG_AML_EMMC_KEY
			printf("emmc boardid read OK,write to bootargs now...\n");
#endif
			strcpy(str, getenv("bootargs"));
			if(strstr(str,"androidboot.boardid") == NULL){
				strcat(str, " androidboot.boardid=");
				strcat(str, tmpbuf);
				setenv("bootargs", str);
			}
				prefetch_flag = 1;
			} 
		}else {
			efuseinfo_item_t info;
			int efuse_ret;
			char *argv[3]={"efuse","read","boardid"};
			printf("boardid keys not in nand,read from efuse\n");
			if((!strncmp(argv[1],"read",sizeof("read"))) &&  (!strncmp(argv[2],"boardid",sizeof("boardid")))){
				efuse_getinfo("boardid", &info);
				efuse_ret = efuse_read_usr(boardid_prefetch, info.data_len, (loff_t*)&info.offset);
			}
			printf("efuse_ret=%d\n",efuse_ret);
			if(efuse_ret >= 0){
				boardid_prefetch[info.data_len] = 0x00;
				printf("efuse read boardid ok,write to bootargs now...\n");
				strcpy(str, getenv("bootargs"));
				if(strstr(str,"androidboot.boardid") == NULL){
					strcat(str, " androidboot.boardid=");
					strcat(str, boardid_prefetch);
					setenv("bootargs", str);
				}
				prefetch_flag = 1;
			}
			else{
				printf("boardid read boardid fail,boardid valid\n");
				prefetch_flag = 0;
			}    
		}
		if(prefetch_flag == 0)
			printf("fetch boardid fail\n");
	return 1;
}

U_BOOT_CMD(
	boardid_prefetch, 1, 0, do_boardid_prefetch,
	"boardid sub-system",
    ""
);