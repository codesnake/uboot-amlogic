#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define SECURE_DEBUG 1
//#define SECURE_EFUSE_DEBUG 1
#define CONFIG_AUTH_RSA_IN_SRAM 1


//#define CONFIG_BCH_PACK0 1

#define RSA_KEYSIZE 1024

// SECURE used memory
#define SECURE_MEM_BASE  0x9FE00000
#define SECURE_MEM_SIZE   0x00100000
#define SECURE_MEM_END  (RSA_MEM_BASE+RSA_MEM_SIZE)

#define COMMON_DATA_PHY_BASE 0x9FE00800
#define COMMON_DATA_BASE  0xDFE00800

#ifdef UBOOT_DEBUG
#define SECURE_STACK_END	0x9FEFFFF0   //256B  kernel virtual
#else
#define SECURE_STACK_END	0xDFEFFFF0   //256B  kernel virtual
#endif

// efuse function call from which: uboot/kernel
#define FROM_UBOOT 0
#define FROM_KERNEL 1

//JAVA_BUFF_DUMP is for dump buffer to compare bewteen big & little
//#define JAVA_BUFF_DUMP
#define RSA_1024_ENC_BLK_SIZE_MAX (127)
#define RSA_1024_BLK_SIZE_MAX     (128)
#define JAVA_BYTE_ARR_SIZE_MAX    (117)
#define JAVA_BYTE_ARR_PAD_VAL     (0xFF)
#define JAVA_BYTE_ARR_PAD_ID_1    (1)
#define JAVA_BYTE_ARR_PAD_ID_0    (0)
#define PC_BYTE_ARR_PAD_VAL       (0)
#define RSA_HASH_SIZE (32)


// RSA key location
#define SECU_RSA_N_OFFSET		(0x7e60)
#define SECU_RSA_E_OFFSET		(0x7e60+128)

#define AUTH_RSA_N_OFFSET		(0x7e60+0x100)
#define AUTH_RSA_E_OFFSET		(0x7e60+0x100+128)	

/**********************************************************************************/
/* EFUSE layout */
#define CONFIG_EFUSE_SN_OFFSET 172
#define CONFIG_EFUSE_SN_BCH_STEP 12
#define CONFIG_EFUSE_SN_DATA_SIZE	24

#define CONFIG_EFUSE_IN_OFFSET   196
#define CONFIG_EFUSE_IN_BCH_STEP 12
#define CONFIG_EFUSE_IN_DATA_SIZE 24

#define CONFIG_EFUSE_CI_OFFSET 168
#define CONFIG_EFUSE_CI_BCH_STEP 4
#define CONFIG_EFUSE_CI_DATA_SIZE 4

//#define CONFIG_EFUSE_AUTH_HASH_OFFSET 200
//#define CONFIG_EFUSE_AUTH_HASH_BCH_STEP 16
//#define CONFIG_EFUSE_AUTH_HASH_DATA_SIZE 32

#define CONFIG_EFUSE_MAC_OFFSET 436
#define CONFIG_EFUSE_MAC_BCH_STEP 6
#define CONFIG_EFUSE_MAC_DATA_SIZE 6

#define CONFIG_EFUSE_MAC_BT_OFFSET 442
#define CONFIG_EFUSE_MAC_BT_BCH_STEP 6
#define CONFIG_EFUSE_MAC_BT_DATA_SIZE 6


#define CONFIG_EFUSE_VERSION_OFFSET 3
#define CONFIG_EFUSE_VERSION_BCH_STEP 3
#define CONFIG_EFUSE_VERSION_DATA_SIZE 1

#define CONFIG_EFUSE_RSA_OFFSET 8
#define CONFIG_EFUSE_RSA_BCH_STEP 0
#define CONFIG_EFUSE_RSA_DATA_SIZE 128


#define CONFIG_EFUSE_BCH_DISABLE   1
/**********************************************************************************/
/* kernel function call: registers access use VA, 
	uboot debug call: registers access use PA */
/**********************************************************************************/
#define writel(v,addr) (*((volatile unsigned*)addr) = v)
#define readl(addr) (*((volatile unsigned*)addr))
#define setbits_le32(reg,mask)  writel(readl(reg)|(mask),reg)
#define clrbits_le32(reg,mask)  writel(readl(reg)&(~(mask)),reg)
#define clrsetbits_le32(reg,clr,mask)  writel((readl(reg)&(~(clr)))|(mask),reg)


/**********************************************************************************/
#define SECURE_DRIVER_VERSION (0x1)

#endif
