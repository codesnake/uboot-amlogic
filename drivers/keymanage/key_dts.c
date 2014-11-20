#include <common.h>
#include <linux/string.h>
#include <errno.h>
#include <malloc.h>
#include <fdt.h>
#include <libfdt.h>
#include "key_manage.h"


#ifndef CONFIG_DTB_LOAD_ADDR
#define CONFIG_DTB_LOAD_ADDR	(PHYS_MEMORY_START + 0x03000000)
#endif


#define UNIFYKEY_DATAFORMAT_HEXDATA	"hexdata"
#define UNIFYKEY_DATAFORMAT_HEXASCII	"hexascii"
#define UNIFYKEY_DATAFORMAT_ALLASCII	"allascii"

#define UNIFYKEY_DEVICE_EFUSEKEY	"efusekey"
#define UNIFYKEY_DEVICE_NANDKEY		"nandkey"
#define UNIFYKEY_DEVICE_SECURESKEY	"secureskey"

#define UNIFYKEY_PERMIT_READ		"read"
#define UNIFYKEY_PERMIT_WRITE		"write"
#define UNIFYKEY_PERMIT_DEL			"del"

static struct key_info_t unify_key_info={.key_num =0, .key_flag = 0};
static struct key_item_t *unifykey_item=NULL;

int unifykey_item_verify_check(struct key_item_t *key_item)
{
	if(!key_item){
		printf("%s:%d unify key item is invalid\n",__func__,__LINE__);
		return -1;
	}
	if(!key_item->name || (key_item->dev == KEY_M_UNKNOW_DEV) ||(key_item->df == KEY_M_MAX_DF)){
		printf("%s:%d unify key item is invalid\n",__func__,__LINE__);
		return -1;
	}
	return 0;
}

struct key_item_t *unifykey_find_item_by_name(char *name)
{
	struct key_item_t *pre_item;
	pre_item = unifykey_item;
	while(pre_item){
		if(!strncmp(pre_item->name,name,strlen(pre_item->name))){
			return pre_item;
		}
		pre_item = pre_item->next;
	}
	return NULL;
}

const char* key_unify_query_key_format(char *keyname)
{
	char *keyformat=NULL;
	struct key_item_t *key_manage;
	key_manage = unifykey_find_item_by_name(keyname);
	if(key_manage == NULL){
		printf("%s:%d,%s key name is not exist\n",__func__,__LINE__,keyname);
		return NULL;
	}
	if(unifykey_item_verify_check(key_manage)){
		printf("%s:%d,%s key name is invalid\n",__func__,__LINE__,keyname);
		return NULL;
	}
	switch(key_manage->df){
		case KEY_M_HEXDATA:
			keyformat= UNIFYKEY_DATAFORMAT_HEXDATA;
		break;
		case KEY_M_HEXASCII:
			keyformat = UNIFYKEY_DATAFORMAT_HEXASCII;
		break;
		case KEY_M_ALLASCII:
			keyformat = UNIFYKEY_DATAFORMAT_ALLASCII;
		break;
		case KEY_M_MAX_DF:
		default:
		break;
	}
	return keyformat;
}


const char* key_unify_query_key_device(char *keyname)
{
	char *keydevice=NULL;
	struct key_item_t *key_manage;
	key_manage = unifykey_find_item_by_name(keyname);
	if(key_manage == NULL){
		printf("%s:%d,%s key name is not exist\n",__func__,__LINE__,keyname);
		return NULL;
	}
	if(unifykey_item_verify_check(key_manage)){
		printf("%s:%d,%s key name is invalid\n",__func__,__LINE__,keyname);
		return NULL;
	}
	switch(key_manage->dev){
		case KEY_M_EFUSE_NORMAL:
			keydevice= UNIFYKEY_DEVICE_EFUSEKEY;
            break;
		case KEY_M_SECURE_STORAGE:
			keydevice = UNIFYKEY_DEVICE_SECURESKEY;
            break;
		case KEY_M_GENERAL_NANDKEY:
			keydevice = UNIFYKEY_DEVICE_NANDKEY;
            break;
		default:
			keydevice = "unkown device";
            break;
	}
	return keydevice;
}

char unifykey_get_efuse_version(void)
{
	char ver=0;
	if(unify_key_info.efuse_version != -1){
		ver = (char)unify_key_info.efuse_version;
	}
	return ver;
}

static int unifykey_create_list(struct key_item_t *item)
{
	struct key_item_t *pre_item;
	if(unifykey_item == NULL){
		unifykey_item = item;
	}
	else{
		pre_item = unifykey_item;
		while(pre_item->next != NULL){
			pre_item = pre_item->next;
		}
		pre_item->next = item;
	}
	return 0;
}
extern int fdt_stringlist_contains(const char *strlist, int listlen, const char *str);
static int unifykey_item_dt_parse(unsigned int dt_addr,int nodeoffset,int id,char *item_path)
{
	int ret=-1;
#ifdef CONFIG_OF_LIBFDT
	struct key_item_t *temp_item=NULL;
	char *propdata;
	struct fdt_property *prop;
	int count;

	temp_item = malloc(sizeof(struct key_item_t));
	if(!temp_item){
		printf("malloc mem fail,%s:%d\n",__func__,__LINE__);
		return -ENOMEM;
	}
	memset(temp_item,0,sizeof(struct key_item_t));
	propdata = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "key-name",NULL);
	if(!propdata){
		printf("%s get key-name fail,%s:%d\n",item_path,__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}
	memset(temp_item->name,0,KEY_UNIFY_NAME_LEN);
	count = strlen(propdata);
	if(count >= KEY_UNIFY_NAME_LEN){
		count = KEY_UNIFY_NAME_LEN - 1;
	}
	strncpy(temp_item->name,propdata,count);
	propdata = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "key-device",NULL);
	if(!propdata){
		printf("%s get key-device fail,%s:%d\n",item_path,__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}
	if(propdata){
		if(strcmp(propdata,UNIFYKEY_DEVICE_EFUSEKEY) == 0){
			temp_item->dev = KEY_M_EFUSE_NORMAL;
		}
		else if(strcmp(propdata,UNIFYKEY_DEVICE_NANDKEY) == 0){
			temp_item->dev = KEY_M_GENERAL_NANDKEY;
		}
		else if(strcmp(propdata,UNIFYKEY_DEVICE_SECURESKEY) == 0){
			temp_item->dev = KEY_M_SECURE_STORAGE;
		}
		else{
			temp_item->dev = KEY_M_UNKNOW_DEV;
		}
	}
	propdata = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "key-dataformat",NULL);
	if(!propdata){
		printf("%s get key-dataformat fail,%s:%d\n",item_path,__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}
	if(propdata){
		if(strcmp(propdata,UNIFYKEY_DATAFORMAT_HEXDATA) == 0){
			temp_item->df = KEY_M_HEXDATA;
		}
		else if(strcmp(propdata,UNIFYKEY_DATAFORMAT_HEXASCII) == 0){
			temp_item->df = KEY_M_HEXASCII;
		}
		else if(strcmp(propdata,UNIFYKEY_DATAFORMAT_ALLASCII) == 0){
			temp_item->df = KEY_M_ALLASCII;
		}
		else{
			temp_item->df = KEY_M_MAX_DF;
		}
	}
	//propdata = fdt_getprop(dt_addr, nodeoffset, "key-permit",NULL);
	//if(!propdata){
	//	printf("%s get key-permit fail,%s:%d\n",item_path,__func__,__LINE__);
	//	break;
	//}
	prop = (struct fdt_property*)fdt_get_property((const void *)dt_addr,nodeoffset,"key-permit",NULL);
	if(!prop){
		printf("%s get key-permit fail,%s:%d\n",item_path,__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}
	if(prop){
		temp_item->permit = 0;
		if(fdt_stringlist_contains(prop->data, prop->len, UNIFYKEY_PERMIT_READ)){
			temp_item->permit |= KEY_M_PERMIT_READ;
		}
		if(fdt_stringlist_contains(prop->data, prop->len, UNIFYKEY_PERMIT_WRITE)){
			temp_item->permit |= KEY_M_PERMIT_WRITE;
		}
		if(fdt_stringlist_contains(prop->data, prop->len, UNIFYKEY_PERMIT_DEL)){
			temp_item->permit |= KEY_M_PERMIT_DEL;
		}
	}
	temp_item->id = id;
	unifykey_create_list(temp_item);
	return 0;
exit:
	if(temp_item){
		free(temp_item);
	}
#endif
	return ret;
}

static int unifykey_item_create(unsigned int dt_addr,int num)
{
#ifdef CONFIG_OF_LIBFDT
	int ret;
	int i,nodeoffset,count=0;
	char item_path[100];
	char cha[10];
	
	memset(item_path,0,sizeof(item_path));
	memset(cha,0,sizeof(cha));
	for(i=0;i<num;i++){
		strcpy(item_path,"/unifykey/key_");
		sprintf(cha,"%d",i);
		strcat(item_path,cha);
		
		nodeoffset = fdt_path_offset((const void *)dt_addr, item_path);
		if(nodeoffset < 0) {
			printf(" dts: not find  node %s.\n",fdt_strerror(nodeoffset));
			break;
		}
		ret = unifykey_item_dt_parse(dt_addr,nodeoffset,count,item_path);
		if(!ret){
			count++;
		}
		memset(item_path,0,sizeof(item_path));
		memset(cha,0,sizeof(cha));
	}
//	printf("unifykey-num fact is %x\n",count);
#endif
	return 0;
}

int unifykey_dt_parse(void)
{
#ifdef CONFIG_OF_LIBFDT
	int nodeoffset;
	unsigned int  dt_addr;
	char *punifykey_num;

	if (getenv("dtbaddr") == NULL) {
		dt_addr = CONFIG_DTB_LOAD_ADDR;
	}
	else {
		dt_addr = simple_strtoul (getenv ("dtbaddr"), NULL, 16);
	}
	
	if(fdt_check_header((void*)dt_addr)!= 0){
        printf(" error: image data is not a fdt\n");
        return -1;
    }
	nodeoffset = fdt_path_offset((const void *)dt_addr, "/unifykey");
	if(nodeoffset < 0) {
		printf(" dts: not find  node %s.\n",fdt_strerror(nodeoffset));
		return -1;
	}
	unify_key_info.efuse_version = -1;
	punifykey_num = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "efuse-version",NULL);
	if(punifykey_num){
		unify_key_info.efuse_version = be32_to_cpup((unsigned int*)punifykey_num);
		printf("efuse-version config is %x\n",unify_key_info.efuse_version);
	}
	
	unify_key_info.key_num = 0;
	punifykey_num = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "unifykey-num",NULL);
	if(punifykey_num){
//		printf("unifykey-num config is %x\n",be32_to_cpup((unsigned int*)punifykey_num));
		unify_key_info.key_num = be32_to_cpup((unsigned int*)punifykey_num);
	}
	else{
		printf("unifykey-num is not configured\n");
	}
	
	if(!unify_key_info.key_flag && unify_key_info.key_num > 0){
		unifykey_item_create(dt_addr,unify_key_info.key_num);
		unify_key_info.key_flag = 1;
	}
#endif
	return 0;
}



