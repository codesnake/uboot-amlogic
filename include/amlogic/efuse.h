#ifndef __EFUSE_H
#define __EFUSE_H

#include <config.h>
#include <common.h>

typedef struct efuseinfo_item{
	char title[40];	
	unsigned offset;    // write offset
	unsigned enc_len;
	unsigned data_len;
	int we;    // write enable 	 
	int bch_en; //BCH enable
	int bch_reverse;
} efuseinfo_item_t;


typedef struct efuseinfo{
	struct efuseinfo_item *efuseinfo_version;
	int size;
	int version;
}efuseinfo_t;

typedef int (*pfn) (char *title, efuseinfo_item_t *info); 
typedef int(*pfn_byPos)(unsigned pos, efuseinfo_item_t *info);


char *efuse_dump(void);
unsigned efuse_readcustomerid(void);
int efuse_set_versioninfo(efuseinfo_item_t *info);

int efuse_getinfo(char *title, efuseinfo_item_t *info);
int efuse_read_usr(char *buf, size_t count, loff_t *ppos);
int efuse_write_usr(char* buf, size_t count, loff_t* ppos);

// for m6 and after efuse length
#define EFUSE_BYTES				512   //(EFUSE_BITS/8)
#define EFUSE_DWORDS		128   //(EFUSE_BITS/32)

#ifdef CONFIG_MESON_TRUSTZONE
// efuse HAL_API arg
struct efuse_hal_api_arg{
	unsigned int cmd;		// R/W
	unsigned int offset;
	unsigned int size;
	unsigned int buffer_phy;
	unsigned int retcnt_phy;	
};
#define EFUSE_HAL_API_READ	0
#define EFUSE_HAL_API_WRITE 1
#define EFUSE_HAL_API_WRITE_PATTERN 2
#endif

#endif

