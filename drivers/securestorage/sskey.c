#include <common.h>
#include <linux/types.h>
#include <asm/arch/register.h>
#include <asm/arch/io.h>
#include <amlogic/secure_storage.h>
#include <linux/rand_ext.h>

#define SECUREOS_INTERFACE

#ifdef SECUREOS_INTERFACE
#include <asm/arch/trustzone.h>
#endif

#ifdef SECUREOS_INTERFACE
#if defined(CONFIG_M8) || defined(CONFIG_M8B)
#define AESKEY_SIZE   0x30
#else
#define AESKEY_SIZE   0x20
#endif
#define SECUREOS_KEY_DEFAULT_ADDR	(SECUREOS_KEY_BASE_ADDR+0xe0000)
#define SECUREOS_KEY_DEFAULT_SIZE	(128*1024)
//#define SECUREOS_KEY_DEFAULT_ADDR	COMM_NS_CONTENT_ADDR
//#define SECUREOS_KEY_DEFAULT_SIZE	COMM_NS_CONTENT_SIZE

struct sstorekey_device_op_s{
	int (*read)(char *buf, unsigned int len);
	int (*write)(char *buf, unsigned  int len);
};
static struct sstorekey_device_op_s sstorekey_device_op;
static int sstorekey_device_status=0;//0: prohibit, 1:permit

#if defined(CONFIG_M8) || defined(CONFIG_M8B)
static int auto_find_device(void)
{
	int R_BOOT_DEVICE_FLAG_m8 = READ_CBUS_REG(ASSIST_POR_CONFIG);
	int R_BOOT_DEVICE_FLAG = ((((R_BOOT_DEVICE_FLAG_m8>>9)&1)<<2) | ((R_BOOT_DEVICE_FLAG_m8>>6)&3));
	int POR_NAND_BOOT = (((R_BOOT_DEVICE_FLAG & 7) == 7) || ((R_BOOT_DEVICE_FLAG & 7) == 6));
	int POR_SPI_BOOT = (((R_BOOT_DEVICE_FLAG & 7) == 5) || ((R_BOOT_DEVICE_FLAG & 7) == 4));
	int POR_EMMC_BOOT = (((R_BOOT_DEVICE_FLAG & 7) == 3) || ((R_BOOT_DEVICE_FLAG & 7) == 2) 
						|| ((R_BOOT_DEVICE_FLAG & 7) == 1));
	int POR_CARD_BOOT = ((R_BOOT_DEVICE_FLAG & 7) == 0);
	int dev=0;
	if(POR_NAND_BOOT)	dev=SECURE_STORAGE_NAND_TYPE;
	if(POR_SPI_BOOT)	dev=SECURE_STORAGE_SPI_TYPE;
	if(POR_EMMC_BOOT)	dev=SECURE_STORAGE_EMMC_TYPE;
	return dev;
}
#else
static int auto_find_device(void)
{
//#define R_BOOT_DEVICE_FLAG READ_CBUS_REG(ASSIST_POR_CONFIG)
//#define POR_NAND_BOOT() (((R_BOOT_DEVICE_FLAG & 7) == 7) || ((R_BOOT_DEVICE_FLAG & 7) == 6))
//#define POR_SPI_BOOT() (((R_BOOT_DEVICE_FLAG & 7) == 5) || ((R_BOOT_DEVICE_FLAG & 7) == 4))
//#define POR_EMMC_BOOT() ((R_BOOT_DEVICE_FLAG & 7) == 3)
//#define POR_CARD_BOOT() ((R_BOOT_DEVICE_FLAG & 7) == 0)
	int R_BOOT_DEVICE_FLAG = READ_CBUS_REG(ASSIST_POR_CONFIG);
	int POR_NAND_BOOT = (((R_BOOT_DEVICE_FLAG & 7) == 7) || ((R_BOOT_DEVICE_FLAG & 7) == 6));
	int POR_SPI_BOOT = (((R_BOOT_DEVICE_FLAG & 7) == 5) || ((R_BOOT_DEVICE_FLAG & 7) == 4));
	int POR_EMMC_BOOT = ((R_BOOT_DEVICE_FLAG & 7) == 3);
	int POR_CARD_BOOT = ((R_BOOT_DEVICE_FLAG & 7) == 0);
	int dev=0;
	if(POR_NAND_BOOT)	dev=SECURE_STORAGE_NAND_TYPE;
	if(POR_SPI_BOOT)	dev=SECURE_STORAGE_SPI_TYPE;
	if(POR_EMMC_BOOT)	dev=SECURE_STORAGE_EMMC_TYPE;
	return dev;
}
#endif

#define SMC_ENOMEM          7
#define SMC_EOPNOTSUPP      6
#define SMC_EINVAL_ADDR     5
#define SMC_EINVAL_ARG      4
#define SMC_ERROR           3
#define SMC_INTERRUPTED     2
#define SMC_PENDING         1
#define SMC_SUCCESS         0


#endif



/*
 * function name: securestore_key_init
 *  seed : make random
 * len  : > 0
 * return : 0: ok, other: fail
 * */
int securestore_key_init( char *seed,int len)
{
#ifdef SECUREOS_INTERFACE
	int device;
	int err;
	int i;
	//unsigned int size,addr;
	char aeskey_data[AESKEY_SIZE];
	struct storage_hal_api_arg cmd_arg;
	unsigned int retval;
	unsigned int keyseed = (seed[0]<<24)|(seed[1]<<16)|(seed[2]<<8)|seed[3];
	memset(aeskey_data,0,AESKEY_SIZE);
	err = random_generate(keyseed,(unsigned char *)aeskey_data,AESKEY_SIZE);
	if(err < 0){
		printf("generate random err :%d,%s:%d\n",err,__func__,__LINE__);
		return err;
	}
#ifdef CONFIG_MESON_STORAGE_DEBUG
	printf("random:\n");
	for(i=0;i<AESKEY_SIZE;i++){
		printf("%2x ",aeskey_data[i]);
		if(i%16 == 15){
			printf("\n");
		}
	}
	printf("\n");
#endif
	device = auto_find_device();
	if((device != SECURE_STORAGE_NAND_TYPE) && (device != SECURE_STORAGE_SPI_TYPE)
		&& (device != SECURE_STORAGE_EMMC_TYPE)){
		printf("secure storage device not found\n");
		return -1;
	}
	if(device == SECURE_STORAGE_NAND_TYPE){
		sstorekey_device_op.read = secure_storage_nand_read;
		sstorekey_device_op.write = secure_storage_nand_write;
		printf("secure storage found nand\n");
	}
	else if(device == SECURE_STORAGE_SPI_TYPE){
		sstorekey_device_op.read = secure_storage_spi_read;
		sstorekey_device_op.write = secure_storage_spi_write;
		printf("secure storage found nor\n");
	}
	else if(device == SECURE_STORAGE_EMMC_TYPE){
		sstorekey_device_op.read = secure_storage_emmc_read;
		sstorekey_device_op.write = secure_storage_emmc_write;
		printf("secure storage found emmc\n");
	}
#if 0
	addr = SECUREOS_KEY_DEFAULT_ADDR;
	size = SECUREOS_KEY_DEFAULT_SIZE;
	err = sstorekey_device_op.read((char*)addr,size);
	if(err){
		printf("%s:%d,read key fail from %s\n",__func__,__LINE__,(device == SECURE_STORAGE_NAND_TYPE)?"nand device":
				(device == SECURE_STORAGE_SPI_TYPE)?"spi device":(device == SECURE_STORAGE_EMMC_TYPE)?"emmc device":"unknow device");
		return err;
	}
#endif
	//secure os init function
	cmd_arg.cmd = STORAGE_HAL_API_INIT;
	cmd_arg.namelen = 0;
	cmd_arg.name_phy_addr = 0;
	cmd_arg.datalen = AESKEY_SIZE;
	cmd_arg.data_phy_addr = (unsigned int)&aeskey_data[0];
	cmd_arg.retval_phy_addr = (unsigned int)&retval;
	err = meson_trustzone_storage(&cmd_arg);
	if(err){
		printf("%s:%d,meson_trustzone_storage init fail\n",__func__,__LINE__);
	}
	return err;
#else
	return __securestore_key_init( seed, len);
#endif
}

/*
*    securestore_key_query - query whether key was burned.
*    @keyname : key name will be queried.
*    @query_result: query return value, 0: key was NOT burned; 1: key was burned; others: reserved.
*    
*    return: 0: successful; others: failed. 
*/
int securestore_key_query(char *keyname, unsigned int *query_return)
{
#ifdef SECUREOS_INTERFACE
	int err;
	struct storage_hal_api_arg cmd_arg;
	//unsigned int retval;
	cmd_arg.cmd = STORAGE_HAL_API_QUERY;
	cmd_arg.namelen = strlen(keyname);
	cmd_arg.name_phy_addr = (unsigned int)keyname;
	cmd_arg.datalen = 0;
	cmd_arg.data_phy_addr = 0;//(unsigned int)&aeskey_data[0];
	cmd_arg.retval_phy_addr = (unsigned int)query_return;
	err = meson_trustzone_storage(&cmd_arg);
	if(err){
		printf("%s:%d,meson_trustzone_storage query fail\n",__func__,__LINE__);
	}
	return err;
#else
	return  __securestore_key_query(keyname, query_return);
#endif
}

/*
 *function name: securestore_key_read
 *function:  securestore_key_read is disabled to read
 * 
 * */
int securestore_key_read(char *keyname,char *keybuf,unsigned int keylen,unsigned int *reallen)
{
#ifdef SECUREOS_INTERFACE
	int err = -1;
#ifdef CONFIG_MESON_STORAGE_DEBUG
	struct storage_hal_api_arg cmd_arg;
	//unsigned int retval;
	unsigned int size,addr;
	addr = SECUREOS_KEY_DEFAULT_ADDR;
	size = SECUREOS_KEY_DEFAULT_SIZE;
	err = sstorekey_device_op.read((char*)addr,size);
	if(err){
		//printf("%s:%d,read key fail from %s\n",__func__,__LINE__,(device == SECURE_STORAGE_NAND_TYPE)?"nand device":
		//		(device == SECURE_STORAGE_SPI_TYPE)?"spi device":(device == SECURE_STORAGE_EMMC_TYPE)?"emmc device":"unknow device");
		printf("%s:%d,read key fail\n",__func__,__LINE__);
		return err;
	}
	cmd_arg.cmd = STORAGE_HAL_API_READ;
	cmd_arg.namelen = strlen(keyname);
	cmd_arg.name_phy_addr = (unsigned int)keyname;
	cmd_arg.datalen = keylen;
	cmd_arg.data_phy_addr = (unsigned int)keybuf;
	cmd_arg.retval_phy_addr = (unsigned int)reallen;
	err = meson_trustzone_storage(&cmd_arg);
	if(err){
		printf("%s:%d,meson_trustzone_storage read fail\n",__func__,__LINE__);
	}
#endif
	return err;
#else
	return  __securestore_key_read(keyname,keybuf,keylen,reallen);
#endif
}
/* funtion name: securestore_key_write
 * keyname : key name is ascii string
 * keybuf : key buf
 * keylen : key buf len
 * keytype: 0: no care key type, 1: aes key, 2:rsa key 
 *          if aes/rsa key, uboot tool need decrypt 
 *
 * return  0: ok, 0x1fe: no space, other fail
 * */
int securestore_key_write(char *keyname, char *keybuf,unsigned int keylen,int keytype)
{
#ifdef SECUREOS_INTERFACE
	unsigned int len,addr;
	int err;
	
	//write key to mem
	struct storage_hal_api_arg cmd_arg;
	unsigned int retval;
	cmd_arg.cmd = STORAGE_HAL_API_WRITE;
	cmd_arg.namelen = strlen(keyname);
	cmd_arg.name_phy_addr = (unsigned int)keyname;
	cmd_arg.datalen = keylen;
	cmd_arg.data_phy_addr = (unsigned int)keybuf;
	cmd_arg.retval_phy_addr = (unsigned int)&retval;
	err = meson_trustzone_storage(&cmd_arg);
	if(err){
		if(err == SMC_ENOMEM ){
			err = 0x1fe;
			printf("%s:%d,secure storage no space to save key\n",__func__,__LINE__);
		}
		else{
			printf("%s:%d,write a key secure storage fail\n",__func__,__LINE__);
		}
		return err;
	}

	addr = SECUREOS_KEY_DEFAULT_ADDR;
	len = SECUREOS_KEY_DEFAULT_SIZE;
	err = sstorekey_device_op.write((char*)addr,len);
	if(err){
		printf("%s:%d,write key to spi fail\n",__func__,__LINE__);
		return err;
	}
	return err;
#else
	return __securestore_key_write(keyname, keybuf, keylen, keytype);
#endif
}
/*function name: securestore_key_uninit
 *functiion : 
 * */
int securestore_key_uninit(void)
{
#ifdef SECUREOS_INTERFACE
#else
	return __securestore_key_uninit();
#endif
}

