#ifndef __KEY_MANAGE_H__
#define __KEY_MANAGE_H__

enum key_manager_dev_e{
       KEY_M_UNKNOW_DEV=0,
       KEY_M_EFUSE_NORMAL,
       KEY_M_SECURE_STORAGE,                     /*secure storage key*/
       KEY_M_GENERAL_NANDKEY,  /*include general key(nand key,emmc key)*/
       KEY_M_MAX_DEV,
};

/*key data format*/
enum key_manager_df_e{
       KEY_M_HEXDATA=0,
       KEY_M_HEXASCII,
       KEY_M_ALLASCII,
       KEY_M_MAX_DF,
};
enum key_manager_permit_e{
       KEY_M_PERMIT_READ = (1<<0),
       KEY_M_PERMIT_WRITE = (1<<1),
       KEY_M_PERMIT_DEL    = (1<<2),
       KEY_M_PERMIT_MASK   = 0Xf,
};

#if 0
struct key_manager_t{
       char *name;
       union{
               unsigned int keydevice;
               struct keydevice_t{
                       unsigned int dev:8;
                       unsigned int df:8;
                       unsigned int permit:4;
                       unsigned int flag:8;
                       unsigned int other:4;
               }devcfg;
       }k;
       unsigned int reserve;
};
#else
#define KEY_UNIFY_NAME_LEN	48
struct key_item_t{
	char name[KEY_UNIFY_NAME_LEN];
	int id;
	unsigned int dev; //key save in device //efuse,
	unsigned int df;
	unsigned int permit;
	int flag;
	int reserve;
	struct key_item_t *next;
};

struct key_info_t{
	int key_num;
	int efuse_version;
	int key_flag;
};

#endif

extern int unifykey_dt_parse(void);
extern struct key_item_t *unifykey_find_item_by_name(char *name);
extern int unifykey_item_verify_check(struct key_item_t *key_item);
extern char unifykey_get_efuse_version(void);
/*
#ifdef CONFIG_UNIFY_KEY_MANAGE

struct key_manager_t key_unify_manager[]={
       {       .name ="mac",
               .k.devcfg.dev = KEY_EFUSE_NORMAL,
               .k.devcfg.df  = KEY_HEXDATA,
               .k.devcfg.permit = KEY_PERMIT_READ | KEY_PERMIT_WRITE,
       },
       {       .name ="key1",
               .k.devcfg.dev = KEY_GENERAL_NAND,
               .k.devcfg.df  = KEY_HEXASCII,
               .k.devcfg.permit = KEY_PERMIT_READ | KEY_PERMIT_WRITE,
       },
       {       .name ="key2",
               .k.devcfg.dev = KEY_GENERAL_NAND,
               .k.devcfg.df  = KEY_HEXASCII,
               .k.devcfg.permit = KEY_PERMIT_READ | KEY_PERMIT_WRITE,
       },
};
#endif
*/
#endif

