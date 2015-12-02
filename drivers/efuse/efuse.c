#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <amlogic/efuse.h>
#include "efuse_bch_8.h"
#include "efuse_regs.h"


char efuse_buf[EFUSE_BYTES] = {0};

extern int efuseinfo_num;
extern efuseinfo_t efuseinfo[];
extern int efuse_active_version;
extern int efuse_active_customerid;
extern pfn efuse_getinfoex;
extern pfn_byPos efuse_getinfoex_byPos;
extern int printf(const char *fmt, ...);
extern void __efuse_write_byte( unsigned long addr, unsigned long data );
extern void __efuse_read_dword( unsigned long addr, unsigned long *data);
extern void efuse_init(void);

ssize_t efuse_read(char *buf, size_t count, loff_t *ppos )
{
    unsigned long contents[EFUSE_DWORDS];
	unsigned pos = *ppos;
    unsigned long *pdw;
    unsigned residunt = pos%4;
    unsigned int dwsize = (count+residunt+3)>>2;
    
	if (pos >= EFUSE_BYTES)
		return 0;
	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -1;

#ifndef CONFIG_MESON_TRUSTZONE
	memset(contents, 0, sizeof(contents));
 	// Enabel auto-read mode    
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
             CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
 
    pos = (pos/4)*4;
    for (pdw = contents; dwsize-- > 0 && pos < EFUSE_BYTES; pos += 4, ++pdw)
		__efuse_read_dword(pos, pdw);	    

     // Disable auto-read mode    
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
             CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
            
	memcpy(buf, (char*)contents+residunt, count);	
	*ppos += count;
    return count;
        
#else
	struct efuse_hal_api_arg arg;
	unsigned int retcnt;
	int ret;
	arg.cmd=EFUSE_HAL_API_READ;
	arg.offset=pos;
	arg.size=count;
	arg.buffer_phy = (unsigned int)buf;
	arg.retcnt_phy = (unsigned int)&retcnt;
	ret = meson_trustzone_efuse(&arg);
	if(ret == 0){
		*ppos+=retcnt;
		return retcnt;
	}
	else
		return ret;
		
#endif	

}

ssize_t efuse_write(const char *buf, size_t count, loff_t *ppos )
{ 	
	unsigned pos = *ppos;
	const char *pc;

	if (pos >= EFUSE_BYTES)
		return 0;	/* Past EOF */
	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -1;

#ifndef CONFIG_MESON_TRUSTZONE
	//Wr( EFUSE_CNTL1, Rd(EFUSE_CNTL1) |  (1 << 12) );
    
    for (pc = buf; count>0;count--, ++pos, ++pc)
		__efuse_write_byte(pos, *pc);		
	*ppos = pos;	

	   // Disable the Write mode
    //Wr( EFUSE_CNTL1, Rd(EFUSE_CNTL1) & ~(1 << 12) );
	return count;
	
#else
	struct efuse_hal_api_arg arg;
	unsigned int retcnt;
	arg.cmd=EFUSE_HAL_API_WRITE;
	arg.offset = pos;
	arg.size=count;
	arg.buffer_phy=(unsigned int)buf;
	arg.retcnt_phy=&retcnt;
	int ret;
	ret = meson_trustzone_efuse(&arg);
	if(ret==0){
		*ppos=retcnt;
		return retcnt;
	}
	else
		return ret;
			
#endif	
}

static int cpu_is_before_m6(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c0, c0, 5	@ get MPIDR" : "=r" (val) : : "cc");
	
	return ((val & 0x40000000) == 0x40000000);
}

#if 0
static char *soc_chip[]={
	{"efuse soc chip m0"},
	{"efuse soc chip m1"},
	{"efuse soc chip m3"},
	{"efuse soc chip m6"},
	{"efuse soc chip m6tv"},
	{"efuse soc chip m6lite"},
	{"efuse soc chip m8"},
	{"efuse soc chip unknow"},
};
#endif

#if 0
struct efuse_chip_info_t{
	unsigned int Id1;
	unsigned int Id2;
	efuse_socchip_type_e type;
};
static const struct efuse_chip_info_t efuse_chip_info[]={
	{.Id1=0x000027ed, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M8},   //M8 second version
	{.Id1=0x000025e2, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M8},   //M8 first version
	{.Id1=0xe2000003, .Id2=0x00000bbb, .type=EFUSE_SOC_CHIP_M6}, //M6 Rev-B
	{.Id1=0x00000d67, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M6}, //M6 Rev-D
	{.Id1=0x00001435, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M6TV}, //M6TV
	{.Id1=0x000005cb, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M6TVLITE}, //M6TVC,M6TVLITE(M6C)
};
#define EFUSE_CHIP_INFO_NUM		sizeof(efuse_chip_info)/sizeof(efuse_chip_info[0])
#endif

struct efuse_chip_identify_t{
	unsigned int chiphw_mver;
	unsigned int chiphw_subver;
	unsigned int chiphw_thirdver;
	efuse_socchip_type_e type;
};
static const struct efuse_chip_identify_t efuse_chip_hw_info[]={
	{.chiphw_mver=27, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M8BABY},      //M8BABY ok
	{.chiphw_mver=26, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6TVD},      //M6TVD ok
	{.chiphw_mver=25, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M8},      //M8 ok
	{.chiphw_mver=24, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6TVLITE},  //M6TVC,M6TVLITE(M6C)
	{.chiphw_mver=23, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6TV},    //M6TV  ok
	{.chiphw_mver=22, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6},      //M6  ok
	{.chiphw_mver=21, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M3},
};
#define EFUSE_CHIP_HW_INFO_NUM  sizeof(efuse_chip_hw_info)/sizeof(efuse_chip_hw_info[0])


efuse_socchip_type_e efuse_get_socchip_type(void)
{
	efuse_socchip_type_e type;
	unsigned int *pID1 =(unsigned int *)0xd9040004;
	unsigned int *pID2 =(unsigned int *)0xd904002c;
	type = EFUSE_SOC_CHIP_UNKNOW;
	if(cpu_is_before_m6()){
		type = EFUSE_SOC_CHIP_M3;
	}
	else{
		unsigned int regval;
		int i;
		struct efuse_chip_identify_t *pinfo = (struct efuse_chip_identify_t*)&efuse_chip_hw_info[0];
		regval = READ_CBUS_REG(ASSIST_HW_REV);
		//printf("chip ASSIST_HW_REV reg:%d \n",regval);
		for(i=0;i<EFUSE_CHIP_HW_INFO_NUM;i++){
			if(pinfo->chiphw_mver == regval){
				type = pinfo->type;
				break;
			}
			pinfo++;
		}
	}
	//printf("%s \n",soc_chip[type]);
	return type;
}
static int efuse_checkversion(char *buf)
{
	efuse_socchip_type_e soc_type;
	int i;
	int ver = buf[0];
	for(i=0; i<efuseinfo_num; i++){
		if(efuseinfo[i].version == ver){
			soc_type = efuse_get_socchip_type();
			switch(soc_type){
				case EFUSE_SOC_CHIP_M3:
					if(ver != 1){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M6:
					if((ver != 2) && ((ver != 4))){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M6TV:
				case EFUSE_SOC_CHIP_M6TVLITE:
					if(ver != 2){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M6TVD:
					if(ver != M6TVD_EFUSE_VERSION_SERIALNUM_V1){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M8:
				case EFUSE_SOC_CHIP_M8BABY:
					if((ver != M8_EFUSE_VERSION_SERIALNUM_V1)&&
						ver != M8_EFUSE_VERSION_SERIALNUM_V2_2RSA){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_UNKNOW:
				default:
					printf("soc is unknow\n");
					ver = -1;
					break;
			}
			return ver;
		}
	}
	return -1;
}

static int efuse_readversion(void)
{
#ifdef CONFIG_EFUSE	
//	loff_t ppos;
	char ver_buf[4], buf[4];
	efuseinfo_item_t info;
	int ret;
	
	#if defined(CONFIG_VLSI_EMULATOR)
        efuse_active_version = 2;
	#endif //#if defined(CONFIG_VLSI_EMULATOR)

	if(efuse_active_version != -1)
		return efuse_active_version;
		
	efuse_init();
	ret = efuse_set_versioninfo(&info);
	if(ret < 0){
		return ret;
	}
	memset(ver_buf, 0, sizeof(ver_buf));		
	memset(buf, 0, sizeof(buf));
	
	efuse_read(buf, info.enc_len, (loff_t *)(&info.offset));
	if(info.bch_en)
		efuse_bch_dec(buf, info.enc_len, ver_buf, info.bch_reverse);
	else
		memcpy(ver_buf, buf, sizeof(buf));

#ifdef CONFIG_M1	//version=0
	if(ver_buf[0] == 0){
		efuse_active_version = ver_buf[0];
		reurn ver_buf[0];
	}
	else
		reurn -1;
#else
	ret = efuse_checkversion(ver_buf);   //m3,m6,m8
	if((ret > 0) && (ver_buf[0] != 0)){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];  // version right
	}
	return -1; //version err
#endif

#ifdef CONFIG_M6   //version=2
#ifndef CONFIG_MESON_TRUSTZONE
	if(ver_buf[0] == 2){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];
	}
#else
	if(ver_buf[0] == 4){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];
	}
#endif	
	else
		return -1;

#elif defined(CONFIG_M3)   //version=1
	if(ver_buf[0] == 1){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];
	}
	else
		return -1;
		
#elif defined(CONFIG_M1)   //version=0
	if(ver_buf[0] == 0){
		efuse_active_version = ver_buf[0];
		reurn ver_buf[0];
	}
	else
		reurn -1;
#else
	return -1;
#endif	
#else
	return -1;
#endif	
}

static int efuse_getinfo_byPOS(unsigned pos, efuseinfo_item_t *info)
{
#ifdef CONFIG_EFUSE	
	int ver;
	int i;
	efuseinfo_t *vx = NULL;
	efuseinfo_item_t *item = NULL;
	int size;
	int ret = -1;		
	efuse_socchip_type_e soc_type;
	
	unsigned versionPOS;
#if 0
	if(cpu_is_before_m6())
		versionPOS = EFUSE_VERSION_OFFSET; //380;
	else
		versionPOS = V2_EFUSE_VERSION_OFFSET; //3;
#else
	soc_type = efuse_get_socchip_type();
	switch(soc_type){
		case EFUSE_SOC_CHIP_M3:
			versionPOS = EFUSE_VERSION_OFFSET; //380;
			break;
		case EFUSE_SOC_CHIP_M6:
		case EFUSE_SOC_CHIP_M6TV:
		case EFUSE_SOC_CHIP_M6TVLITE:
			versionPOS = V2_EFUSE_VERSION_OFFSET; //3;
			break;
		case EFUSE_SOC_CHIP_M8:
		case EFUSE_SOC_CHIP_M8BABY:
			versionPOS = M8_EFUSE_VERSION_OFFSET;//509
			break;
		case EFUSE_SOC_CHIP_M6TVD:
			versionPOS = M6TVD_EFUSE_VERSION_OFFSET;
			break;
		case EFUSE_SOC_CHIP_UNKNOW:
		default:
			return ret;
	}
#endif
	if(pos == versionPOS){
		ret = efuse_set_versioninfo(info);
		return ret;
	}
	
	ver = efuse_readversion();
		if(ver < 0){
			printf("efuse version is not selected.\n");
			return -1;
		}
		
		for(i=0; i<efuseinfo_num; i++){
			if(efuseinfo[i].version == ver){
				vx = &(efuseinfo[i]);
				break;
			}				
		}
		if(!vx){
			printf("efuse version %d is not supported.\n", ver);
			return -1;
		}	
		
		// BSP setting priority is higher than version table
		if((efuse_getinfoex_byPos != NULL)){
			ret = efuse_getinfoex_byPos(pos, info);			
			if(ret >=0 )
				return ret;
		}
		
		item = vx->efuseinfo_version;
		size = vx->size;		
		ret = -1;		
		for(i=0; i<size; i++, item++){			
			if(pos == item->offset){
				strcpy(info->title, item->title);				
				info->offset = item->offset;				
				info->data_len = item->data_len;			
				info->enc_len = item->enc_len;
				info->bch_en = item->bch_en;
				info->bch_reverse = item->bch_reverse;			
				info->we=item->we;		
				ret = 0;
				break;
			}
		}
		
		//if((ret < 0) && (efuse_getinfoex != NULL))
		//	ret = efuse_getinfoex(id, info);		
		if(ret < 0)
			printf("POS:%d is not found.\n", pos);
			
		return ret;
#else
	return -1;
#endif		
}


int efuse_chk_written(loff_t pos, size_t count)
{
#ifdef CONFIG_EFUSE	
	loff_t local_pos = pos;	
	int i;
	//unsigned char* buf = NULL;
	char buf[EFUSE_BYTES];
	efuseinfo_item_t info;
	unsigned enc_len ;		
	
	if(efuse_getinfo_byPOS(pos, &info) < 0){
		printf("not found the position:%d.\n", (int)pos);
		return -1;
	}
	 if(count>info.data_len){
		printf("data length: %d is out of EFUSE layout!\n", count);
		return -1;
	}
	if(count == 0){
		printf("data length: 0 is error!\n");
		return -1;
	}
	
	efuse_init();
	enc_len = info.enc_len;				
	if (efuse_read(buf, enc_len, &local_pos) == enc_len) {
		for (i = 0; i < enc_len; i++) {
			if (buf[i]) {
				printf("pos %d value is %d", (size_t)(pos + i), buf[i]);
				return 1;
			}
		}
	}	
	return 0;
#else
	return -1;
#endif	
}



int efuse_read_usr(char *buf, size_t count, loff_t *ppos)
{	
#ifdef CONFIG_EFUSE	
	char data[EFUSE_BYTES];
//	int ret;
	unsigned enc_len;			
	char *penc = NULL;
	char *pdata = NULL;		
	int reverse = 0;
	unsigned pos = (unsigned)*ppos;
	efuseinfo_item_t info;	
		
	if(efuse_getinfo_byPOS(pos, &info) < 0){
		printf("not found the position:%d.\n", pos);
		return -1;
	}			
	if(count>info.data_len){
		printf("data length: %d is out of EFUSE layout!\n", count);
		return -1;
	}
	if(count == 0){
		printf("data length: 0 is error!\n");
		return -1;
	}
	
	efuse_init();
	enc_len = info.enc_len;
	reverse=info.bch_reverse;			
	memset(efuse_buf, 0, EFUSE_BYTES);	
	memset(data, 0, EFUSE_BYTES);
		
	penc = efuse_buf;
	pdata = data;			
	if(info.bch_en){						
		efuse_read(efuse_buf, enc_len, ppos);		
		while(enc_len >= 31){
			efuse_bch_dec(penc, 31, pdata, reverse);
			penc += 31;
			pdata += 30;
			enc_len -= 31;
		}
		if((enc_len > 0))
			efuse_bch_dec(penc, enc_len, pdata, reverse);
	}	
	else
		efuse_read(pdata, enc_len, ppos);	
		
	memcpy(buf, data, count);		

	return count;	
#else
	return 0;
#endif	
}

int efuse_write_usr(char* buf, size_t count, loff_t* ppos)
{
#ifdef CONFIG_EFUSE	
	char data[EFUSE_BYTES];
//	int ret;		
	char *pdata = NULL;
	char *penc = NULL;			
	unsigned enc_len,data_len, reverse;
	unsigned pos = (unsigned)*ppos;	
	efuseinfo_item_t info;
	
	if(efuse_getinfo_byPOS(pos, &info) < 0){
		printf("not found the position:%d.\n", pos);
		return -1;
	}
	if(count>info.data_len){
		printf("data length: %d is out of EFUSE layout!\n", count);
		return -1;
	}
	if(count == 0){
		printf("data length: 0 is error!\n");
		return -1;
	}	
	if(strcmp(info.title, "version") == 0){
		if(efuse_checkversion(buf) < 0){
			printf("efuse version NO. error\n");
			return -1;
		}
	}
	
	if(efuse_chk_written(pos, info.data_len)){
		printf("error: efuse has written.\n");
		return -1;
	}
		
	memset(data, 0, EFUSE_BYTES);
	memset(efuse_buf, 0, EFUSE_BYTES);
	
	efuse_init();	
	memcpy(data, buf, count)	;	
	pdata = data;
	penc = efuse_buf;			
	enc_len=info.enc_len;
	data_len=info.data_len;
	reverse = info.bch_reverse;	
	
	if(info.bch_en){				
		while(data_len >= 30){
			efuse_bch_enc(pdata, 30, penc, reverse);
			data_len -= 30;
			pdata += 30;
			penc += 31;		
		}
		if(data_len > 0)
			efuse_bch_enc(pdata, data_len, penc, reverse);
	}	
	else
		memcpy(penc, pdata, enc_len);
		
	efuse_write(efuse_buf, enc_len, ppos);
	
	return enc_len;	
#else
	return -1;
#endif	
}

int efuse_set_versioninfo(efuseinfo_item_t *info)
{
#ifdef CONFIG_EFUSE	
	efuse_socchip_type_e soc_type;
	int ret=-1;
	strcpy(info->title, "version");			
#if 0
	if(cpu_is_before_m6()){
			info->offset = EFUSE_VERSION_OFFSET; //380;		
			info->data_len = EFUSE_VERSION_DATA_LEN; //3;	
			info->enc_len = EFUSE_VERSION_ENC_LEN; //4;
			info->bch_en = EFUSE_VERSION_BCH_EN; //1;		
			info->we = 1;									//add 
			info->bch_reverse = EFUSE_VERSION_BCH_REVERSE;
	}
		else{
			info->offset = V2_EFUSE_VERSION_OFFSET; //3;		
			info->data_len = V2_EFUSE_VERSION_DATA_LEN; //1;		
			info->enc_len = V2_EFUSE_VERSION_ENC_LEN; //1;
			info->bch_en = V2_EFUSE_VERSION_BCH_EN; //0;
			info->we = 1;									//add 
			info->bch_reverse = V2_EFUSE_VERSION_BCH_REVERSE;
		}
#else
	soc_type = efuse_get_socchip_type();
	switch(soc_type){
		case EFUSE_SOC_CHIP_M3:
			info->offset = EFUSE_VERSION_OFFSET; //380;		
			info->data_len = EFUSE_VERSION_DATA_LEN; //3;	
			info->enc_len = EFUSE_VERSION_ENC_LEN; //4;
			info->bch_en = EFUSE_VERSION_BCH_EN; //1;		
			info->we = 1;									//add 
			info->bch_reverse = EFUSE_VERSION_BCH_REVERSE;
			ret = 0;
			break;
		case EFUSE_SOC_CHIP_M6:
		case EFUSE_SOC_CHIP_M6TV:
		case EFUSE_SOC_CHIP_M6TVLITE:
			info->offset = V2_EFUSE_VERSION_OFFSET; //3;		
			info->data_len = V2_EFUSE_VERSION_DATA_LEN; //1;		
			info->enc_len = V2_EFUSE_VERSION_ENC_LEN; //1;
			info->bch_en = V2_EFUSE_VERSION_BCH_EN; //0;
			info->we = 1;									//add 
			info->bch_reverse = V2_EFUSE_VERSION_BCH_REVERSE;
			ret = 0;
			break;
		case EFUSE_SOC_CHIP_M8:
		case EFUSE_SOC_CHIP_M8BABY:
			info->offset = M8_EFUSE_VERSION_OFFSET; //509
			info->data_len = M8_EFUSE_VERSION_DATA_LEN;
			info->enc_len = M8_EFUSE_VERSION_ENC_LEN;
			info->bch_en = M8_EFUSE_VERSION_BCH_EN;
			info->we = 1;
			info->bch_reverse = M8_EFUSE_VERSION_BCH_REVERSE;
			ret = 0;
			break;
		case EFUSE_SOC_CHIP_M6TVD:
			info->offset = M6TVD_EFUSE_VERSION_OFFSET;
			info->data_len = M6TVD_EFUSE_VERSION_DATA_LEN;
			info->enc_len = M6TVD_EFUSE_VERSION_ENC_LEN;
			info->bch_en = M6TVD_EFUSE_VERSION_BCH_EN;
			info->we = 1;
			info->bch_reverse = M6TVD_EFUSE_VERSION_BCH_REVERSE;
			ret = 0;
			break;
		case EFUSE_SOC_CHIP_UNKNOW:
		default:
			printf("efuse: soc is error\n");
			ret = -1;
			break;
	}
	return ret;
#endif
#endif		
}


int efuse_getinfo(char *title, efuseinfo_item_t *info)
{
#ifdef CONFIG_EFUSE	
	int ver;
	int i;
	efuseinfo_t *vx = NULL;
	efuseinfo_item_t *item = NULL;
	int size;
	int ret = -1;		
	
	if(strcmp(title, "version") == 0){
		ret = efuse_set_versioninfo(info);		
		return ret;	
	}
	
		ver = efuse_readversion();
		if(ver < 0){
			printf("efuse version is not selected.\n");
			return -1;
		}		
		for(i=0; i<efuseinfo_num; i++){
			if(efuseinfo[i].version == ver){
				vx = &(efuseinfo[i]);
				break;
			}				
		}
		if(!vx){
			printf("efuse version %d is not supported.\n", ver);
			return -1;
		}	
		
		// BSP setting priority is higher than versiontable
		if(efuse_getinfoex != NULL){
			ret = efuse_getinfoex(title, info);		
			if(ret >= 0)
				return ret;
		}
		
		item = vx->efuseinfo_version;
		size = vx->size;
		ret = -1;		
		for(i=0; i<size; i++, item++){
			if(strcmp(item->title, title) == 0){
				strcpy(info->title, item->title);				
				info->offset = item->offset;
				info->enc_len = item->enc_len;
				info->data_len = item->data_len;
				info->we = item->we;
				info->bch_en = item->bch_en;				
				info->bch_reverse = item->bch_reverse;	
				ret = 0;
				break;
			}
		}
		
		if(ret < 0)
			printf("%s is not found.\n", title);			
		return ret;
#else
	return -1;
#endif		
}

unsigned efuse_readcustomerid(void)
{
#ifdef CONFIG_EFUSE	
	if(efuse_active_customerid != 0)
		return efuse_active_customerid;
	
	loff_t ppos;
	char buf[4];
	efuse_socchip_type_e soc_type;
	memset(buf, 0, sizeof(buf));
	efuse_init();	
	
#if 0
	if(cpu_is_before_m6()){
		ppos = 380;
		efuse_read(efuse_buf, 4, &ppos);
		efuse_bch_dec(efuse_buf, 4, buf, 0);
		if((buf[1] != 0) || (buf[2] != 0))
			efuse_active_customerid = (buf[2]<<8) + buf[1];		
	}
	else{
		ppos = 4;
		efuse_read(buf, 4, &ppos);		
		int i;
		unsigned val = 0;
		for(i=3; i>=0;i--)
			val = ((val<<8) + buf[i]);
		if(val != 0)
			efuse_active_customerid = val;			
	}
#else
	soc_type = efuse_get_socchip_type();
	switch(soc_type){
		case EFUSE_SOC_CHIP_M3:
			ppos = 380;
			efuse_read(efuse_buf, 4, &ppos);
			efuse_bch_dec(efuse_buf, 4, buf, 0);
			if((buf[1] != 0) || (buf[2] != 0))
				efuse_active_customerid = (buf[2]<<8) + buf[1];		
			break;
		case EFUSE_SOC_CHIP_M6:
		case EFUSE_SOC_CHIP_M6TV:
		case EFUSE_SOC_CHIP_M6TVLITE:
			ppos = 4;
			efuse_read(buf, 4, &ppos);
			int i;
			unsigned val = 0;
			for(i=3; i>=0;i--)
				val = ((val<<8) + buf[i]);
			if(val != 0)
				efuse_active_customerid = val;			
			break;
		case EFUSE_SOC_CHIP_M8:
		case EFUSE_SOC_CHIP_M8BABY:
			break;
		case EFUSE_SOC_CHIP_M6TVD:
			break;
		case EFUSE_SOC_CHIP_UNKNOW:
		default:
			printf("efuse: soc is unknow\n");
			break;
	}
#endif
	
	return efuse_active_customerid;
#else
	return 0;
#endif	
}

/*void efuse_getinfo_version(efuseinfo_item_t *info)
{
	strcpy(info->title, "version");
	info->we = 1;
	info->bch_reverse = 0;
		
	if(cpu_is_before_m6()){
		info->offset = 380;
		info->enc_len = 4;
		info->data_len = 3;
		info->bch_en = 1;	
	}
	else{
		info->offset = 3;
		info->enc_len = 1;
		info->data_len = 1;
		info->bch_en = 0;
	}		
}*/

char* efuse_dump(void)
{
#ifndef CONFIG_MESON_TRUSTZONE	
	int i=0;
    //unsigned pos;
    memset(efuse_buf, 0, sizeof(efuse_buf));
	efuse_init();

 	// Enabel auto-read mode
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
             CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );

	for(i=0; i<EFUSE_BYTES; i+=4)
		__efuse_read_dword(i,  (unsigned long*)(&(efuse_buf[i])));	    
		
     // Disable auto-read mode    
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
             CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
     
     return (char*)efuse_buf;
#else
	return NULL;
#endif     
}

#ifdef CONFIG_AML_EFUSE_INIT_PLUS
int efuse_aml_init_plus(void)
{
	int nRet = 0;
	efuse_socchip_type_e soc_type;
	//check efuse size
	if(EFUSE_BYTES != 512)
		return nRet;
	
#if 0
	//check MX or not
	if(cpu_is_before_m6())
		return nRet;
	
	int i=0;
	char szBuffer[EFUSE_BYTES];
	memset(szBuffer,0,sizeof(szBuffer));
	efuse_init();

 	// Enabel auto-read mode
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
             CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );

	for(i=0; i<EFUSE_BYTES; i+=4)
		__efuse_read_dword(i,  (unsigned long*)(&(szBuffer[i])));	    
		
     // Disable auto-read mode    
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
             CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	
	//check
#define AML_MX_EFUSE_CHECK_1FD (0x1FD)
	if(szBuffer[AML_MX_EFUSE_CHECK_1FD])
		return nRet;	
#undef AML_MX_EFUSE_CHECK_1FD

#define AML_MX_EFUSE_CHECK_1 (0x1)
	if(!(szBuffer[AML_MX_EFUSE_CHECK_1] & 0x1))
	{
		efuse_init();		
		szBuffer[AML_MX_EFUSE_CHECK_1] |= 0x01;
		loff_t pos = AML_MX_EFUSE_CHECK_1;
		efuse_write(&szBuffer[AML_MX_EFUSE_CHECK_1], 1, (loff_t*)&pos);	
		//printf("AML efuse init\n");
	}
#undef AML_MX_EFUSE_CHECK_1	
#else
	soc_type = efuse_get_socchip_type();
	if(soc_type == EFUSE_SOC_CHIP_M3){
		return nRet;
	}
	else if((soc_type == EFUSE_SOC_CHIP_M6)||(soc_type == EFUSE_SOC_CHIP_M6TV)
			||(soc_type == EFUSE_SOC_CHIP_M6TVLITE)){
		int i=0;
		char szBuffer[EFUSE_BYTES];
		memset(szBuffer,0,sizeof(szBuffer));
		efuse_init();

		// Enabel auto-read mode
		WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
				 CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );

		for(i=0; i<EFUSE_BYTES; i+=4)
			__efuse_read_dword(i,  (unsigned long*)(&(szBuffer[i])));	    
			
		 // Disable auto-read mode    
		WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
				 CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
		
		//check
	#define AML_MX_EFUSE_CHECK_1FD (0x1FD)
		if(szBuffer[AML_MX_EFUSE_CHECK_1FD])
			return nRet;	
	#undef AML_MX_EFUSE_CHECK_1FD

	#define AML_MX_EFUSE_CHECK_1 (0x1)
		if(!(szBuffer[AML_MX_EFUSE_CHECK_1] & 0x1))
		{
			efuse_init();		
			szBuffer[AML_MX_EFUSE_CHECK_1] |= 0x01;
			loff_t pos = AML_MX_EFUSE_CHECK_1;
			efuse_write(&szBuffer[AML_MX_EFUSE_CHECK_1], 1, (loff_t*)&pos);	
			//printf("AML efuse init\n");
		}
	#undef AML_MX_EFUSE_CHECK_1	
	}
	else if((soc_type == EFUSE_SOC_CHIP_M8)||(soc_type == EFUSE_SOC_CHIP_M8BABY)){
	}
	else if(soc_type == EFUSE_SOC_CHIP_M6TVD ){
	}
#endif
	return nRet;
}
#endif //CONFIG_AML_EFUSE_INIT_PLUS

/* function: efuse_read_intlItem
 * intl_item: item name,name is [temperature,cvbs_trimming,temper_cvbs]
 *            [temperature: 2byte]
 *            [cvbs_trimming: 2byte]
 *            [temper_cvbs: 4byte]
 * buf:  output para
 * size: buf size
 * */
int efuse_read_intlItem(char *intl_item,char *buf,int size)
{
	efuse_socchip_type_e soc_type;
	loff_t pos;
	int len;
	int ret=-1;
	soc_type = efuse_get_socchip_type();
	switch(soc_type){
		case EFUSE_SOC_CHIP_M3:
			//pos = ;
			break;
		case EFUSE_SOC_CHIP_M6:
		case EFUSE_SOC_CHIP_M6TV:
		case EFUSE_SOC_CHIP_M6TVLITE:
			//pos = ; 
			break;
		case EFUSE_SOC_CHIP_M8:
		case EFUSE_SOC_CHIP_M8BABY:
			if(strcmp(intl_item,"temperature") == 0){
				pos = 502;
				len = 2;
				if(size <= 0){
					printf("%s input size:%d is error\n",intl_item,size);
					return -1;
				}
				if(len > size){
					len = size;
				}
				ret = efuse_read( buf, len, &pos );
				return ret;
			}
			if(strcmp(intl_item,"cvbs_trimming") == 0){
				/* cvbs note: 
				 * cvbs has 2 bytes, position is 504 and 505, 504 is low byte,505 is high byte
				 * p504[bit2~0] is cvbs trimming CDAC_GSW<2:0>
				 * p505[bit7] : 1--wrote cvbs, 0-- not wrote cvbs
				 * */
				pos = 504;
				len = 2;
				if(size <= 0){
					printf("%s input size:%d is error\n",intl_item,size);
					return -1;
				}
				if(len > size){
					len = size;
				}
				ret = efuse_read( buf, len, &pos );
				return ret;
			}
			if(strcmp(intl_item,"temper_cvbs") == 0){
				pos = 502;
				len = 4;
				if(size <= 0){
					printf("%s input size:%d is error\n",intl_item,size);
					return -1;
				}
				if(len > size){
					len = size;
				}
				ret = efuse_read( buf, len, &pos );
				return ret;
			}
			break;
		case EFUSE_SOC_CHIP_M6TVD:
			break;
		case EFUSE_SOC_CHIP_UNKNOW:
		default:
			printf("%s:%d chip is unkow\n",__func__,__LINE__);
			//return -1;
			break;
	}
	return ret;
}


