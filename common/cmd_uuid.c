/*
 * (C) Copyright 2012
 * Amlogic. Inc. zongdong.jiao@amlogic.com
 *
 * This file is used to prefetch/varify/compare uuid keys
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

#define UUID_SIZE  32

static char uuid_prefetch[UUID_SIZE] = { 0x00 };

static char twoASCByteToByte(char c1, char c2)
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

// Prefetch the uuid data from nand, efuse, etc
static int do_uuid_prefetch(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int i,j,count=1,ret = 0,prefetch_flag = 0;
	char tmpbuf[1024],str[1024];
	//if (argc >= 1)
	//	return cmd_usage(cmdtp);
	run_command("secukey nand",0);
    //ret = uboot_key_get(argv[1], "uuid", uuid_prefetch, 16, 0);
	ret=uboot_key_read("uuid",uuid_prefetch);
    //printf("ret=%d\n",ret);
    if(ret >= 0) {
		printf("uuid keys read from nand success!\n");
		char outputdata[2];
		memset(outputdata,0,2);
		for(i=0;i<UUID_SIZE;i++){
			outputdata[0]=uuid_prefetch[i];
			//printf("%02x\n",outputdata[0]);
			if(outputdata[0] == 00){
				count++;
				//printf("the data is 0!\n");
			}
		}	
		if(count >= UUID_SIZE){					
			prefetch_flag = 0;
			printf("prefetch uuid verify keys from %s failed\n", argv[1]);
			printf("nand key read uuid fail,uuid valid\n");
		}else{
#if 0	/* do not convert,because it writes uuid(0~f) datas directly */
			for(i=0,j=0;i<ret;j++){
				tmpbuf[j] = twoASCByteToByte(uuid_prefetch[i],uuid_prefetch[i+1]);
				i += 2;
			}
#endif        
                printf("nand uuid read uuid=%s\n", uuid_prefetch);
			printf("nand uuid read OK,write to bootargs now...\n");
			strcpy(str, getenv("bootargs"));
			if(strstr(str,"androidboot.serialno") == NULL){
				strcat(str, " androidboot.serialno=");
				strcat(str, uuid_prefetch);
				setenv("bootargs", str);
			}
				prefetch_flag = 1;
			} 
		}
#if 0 /* no uuid key in efuse */
          else {
			efuseinfo_item_t info;
			int efuse_ret;
			char *argv[3]={"efuse","read","uuid"};
			printf("uuid keys not in nand,read from efuse\n");
			if((!strncmp(argv[1],"read",sizeof("read"))) &&  (!strncmp(argv[2],"uuid",sizeof("uuid")))){
				efuse_getinfo("uuid", &info);
				efuse_ret = efuse_read_usr(uuid_prefetch, info.data_len, (loff_t*)&info.offset);
			}
			printf("efuse_ret=%d\n",efuse_ret);
			if(efuse_ret >= 0){
				uuid_prefetch[info.data_len] = 0x00;
				printf("efuse read uuid ok,write to bootargs now...\n");
				strcpy(str, getenv("bootargs"));
				if(strstr(str,"androidboot.serialno") == NULL){
					strcat(str, " androidboot.serialno=");
					strcat(str, uuid_prefetch);
					setenv("bootargs", str);
				}
				prefetch_flag = 1;
			}
			else{
				printf("uuid read uuid fail,uuid valid\n");
				prefetch_flag = 0;
			}    
		}
#endif        
		if(prefetch_flag == 0)
			printf("fetch uuid fail\n");
	return 1;
}

U_BOOT_CMD(
	uuid_prefetch, 1, 0, do_uuid_prefetch,
	"uuid sub-system",
    ""
);