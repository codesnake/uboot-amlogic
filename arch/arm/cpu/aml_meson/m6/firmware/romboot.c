#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>

#ifdef CONFIG_MESON_TRUSTZONE
#include <asm/arch/trustzone.h>
#include <secureloader.c>
#ifdef CONFIG_SPI_NOR_SECURE_STORAGE
#include <spisecustorage.c>
#endif
#endif

#if CONFIG_UCL
#ifndef CONFIG_IMPROVE_UCL_DEC
extern int uclDecompress(char* op, unsigned* o_len, char* ip);
#endif
#endif

#ifndef FIRMWARE_IN_ONE_FILE
#define STATIC_PREFIX
#else
#define STATIC_PREFIX static inline
#endif
#ifndef CONFIG_AML_UBOOT_MAGIC
#define CONFIG_AML_UBOOT_MAGIC 0x12345678
#endif
#ifndef memcpy
#define memcpy ipl_memcpy:
#endif
#ifndef CONFIG_DISABLE_INTERNAL_U_BOOT_CHECK
short check_sum(unsigned * addr,unsigned short check_sum,unsigned size);
#else
STATIC_PREFIX short check_sum(unsigned * addr,unsigned short check_sum,unsigned size)
{    
    serial_put_dword(addr[15]);
    if(addr[15]!=CONFIG_AML_UBOOT_MAGIC)
        return -1;
#if 0
	int i;   
	unsigned short * p=(unsigned short *)addr;
    for(i=0;i<size>>1;i++)
        check_sum^=p[i];
#endif
    return 0;
}
#endif

#if defined(CONFIG_M6_SECU_BOOT)

#include "../../../../../../drivers/efuse/efuse_regs.h"
static void __x_udelay(unsigned long usec)
{
#define MAX_DELAY (0x100)
	do {

		int i = 0;
		for(i = 0;i < MAX_DELAY;++i);
		usec -= 1;
	} while(usec);
}

#if 0
static void __x_efuse_read_dword( unsigned long addr, unsigned long *data )
{
    unsigned long auto_rd_is_enabled = 0;
    
    if( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_ENABLE_BIT ) )
    {
        auto_rd_is_enabled = 1;
    }
    else
    {
        /* temporarily enable Read mode */
        WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
            CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
    }

    /* write the address */    
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
        CNTL1_BYTE_ADDR_BIT,  CNTL1_BYTE_ADDR_SIZE );
    	
    /* set starting byte address */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
        CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );	
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
        CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
   /* start the read process */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
        CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );      
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
        CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
      
    /* dummy read */
    READ_EFUSE_REG( P_EFUSE_CNTL1 );
    while ( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_BUSY_BIT ) )
    {
        __x_udelay(1);
    }
    /* read the 32-bits value */
    ( *data ) = READ_EFUSE_REG( P_EFUSE_CNTL2 );    

    /* if auto read wasn't enabled and we enabled it, then disable it upon exit */
    if ( auto_rd_is_enabled == 0 )
    {
        WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
            CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
    }

}

static void aml_mx_efuse_load(const unsigned char *pEFUSE)
{	
	//efuse init
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL4, CNTL4_ENCRYPT_ENABLE_OFF,
	CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE );
	// clear power down bit
	WRITE_EFUSE_REG_BITS(P_EFUSE_CNTL1, CNTL1_PD_ENABLE_OFF, 
		CNTL1_PD_ENABLE_BIT, CNTL1_PD_ENABLE_SIZE);
	
	// Enabel auto-read mode
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
	CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	int i = 0;
	for(i=8; i<8+128+32; i+=4)
		__x_efuse_read_dword(i,	(unsigned long*)(pEFUSE+i));		
	
	 // Disable auto-read mode	  
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
		 CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );

}
#endif

#if 1
#include "secure.c"
static void aml_m6_sec_boot_check_2RSA_key(const unsigned char *pSRC)
{
	int index = check_m6_chip_version();
	if(index < 0){
		serial_puts("\nErr! Wrong MX chip!\n");
		writel((1<<22) | (3<<24)|500, P_WATCHDOG_TC);
		while(1);
	}
	t_func_v3 fp_05 = (t_func_v3)m6_g_action[index][5];
	unsigned int info=0;
	fp_05((int)&info,0,4);
	if(!(info &(1<<7))){
		return ;
	}

	writel(readl(0xda004004) & ~0x80000510,0xda004004);//clear JTAG for long time hash
	#define AML_RSA_STAGE_1_1 (0x31315352)   //RS11
	#define AML_RSA_STAGE_2_2 (0x32325352)   //RS22

	unsigned int *pID = (unsigned int *)(AHB_SRAM_BASE+(32<<10)-8);
	switch(*pID)
	{
	case AML_RSA_STAGE_1_1:
	case AML_RSA_STAGE_2_2:break;
	default:
		{
			//serial_puts("\nError! SPL is corrupt!\n");
			serial_puts("\nErr! SPL is corrupt!\n");
			writel((1<<22) | (3<<24)|500, P_WATCHDOG_TC);
			while(1);
		}break;
	}

	unsigned int *pRSAAuthID = (unsigned int *)(AHB_SRAM_BASE+32*1024-16);
	#define AML_M6_AMLA_ID	  (0x414C4D41)	 //AMLA
	int nBlkOffset = 32*1024 - (sizeof(struct_aml_chk_blk)+256*2+32);
	if( AML_M6_AMLA_ID == *pRSAAuthID)
		nBlkOffset -= 128;
	
	//struct_aml_chk_blk *pBlk = (struct_aml_chk_blk *)(AHB_SRAM_BASE+ *pSPLSize - sizeof(struct_aml_chk_blk) - 2*256);
	struct_aml_chk_blk *pBlk = (struct_aml_chk_blk *)(AHB_SRAM_BASE+ nBlkOffset);

	if((AMLOGIC_CHKBLK_ID == pBlk->unAMLID) && (AMLOGIC_CHKBLK_VER >= pBlk->nVer) &&
		sizeof(*pBlk) == pBlk->nSize2)
	{
		aml_intx n,e;
		unsigned char szEFUSE[128];
		fp_05((int)&szEFUSE[0],8,128);

		memset_aml(&n,0,sizeof(n));
		memset_aml(&e,0,sizeof(e));

		memcpy_aml(n.dp,szEFUSE,128);
		n.used = 32;

		e.dp[0] = 0x010001;
		e.used = 1;				
#if defined(CONFIG_M6_SECU_BOOT_2RSA)
		//get_2RSA(int index,aml_intx *n,aml_intx *e, unsigned char *key,int len);
		unsigned char *pRSAPUK1 = (unsigned char *)pBlk + sizeof(struct_aml_chk_blk)+256;
		//unsigned char szRSAPUK1[256];
		if(get_2RSA(index,&n,&e,pRSAPUK1,256)){
			serial_puts("\nError! RSA 1st key is corrupt!\n");
			writel((1<<22) | (3<<24)|500, P_WATCHDOG_TC);
			while(1);
		}
#endif //#if defined(CONFIG_M6_SECU_BOOT_2RSA)
		unsigned char *pAESkey = (unsigned char *)pBlk + sizeof(struct_aml_chk_blk);

		unsigned char szAESkey[256];
		int szAESkey_len=0;
		memset_aml(szAESkey,0,sizeof(szAESkey));
		
		m6_rsa_dec_pub(index,&n,&e,pAESkey,256,szAESkey,&szAESkey_len);
		
		unsigned char szCHK_temp[128];
		int szCHK_len=0;
		m6_rsa_dec_pub(index,&n,&e,pBlk->szCHK,128,szCHK_temp,&szCHK_len);
		
		memcpy_aml(pBlk->szCHK,szCHK_temp,124);
		
		//int m6_aes_decrypt(unsigned char *ct,int ctlen,unsigned char *key,int keylen);
		m6_aes_decrypt(index,pSRC,pBlk->secure.nAESLength,szAESkey,sizeof(szAESkey));
		unsigned char szHashKey[32];
		memset_aml(szHashKey,0,sizeof(szHashKey));

		szHashKey[0] = 0;
		sha2_sum_2( szHashKey,(unsigned char *)(AHB_SRAM_BASE+pBlk->secure.hashInfo.nOffset1),pBlk->secure.hashInfo.nSize1);

		szHashKey[0] = 1;		
		sha2_sum_2( szHashKey,(unsigned char *)(AHB_SRAM_BASE+pBlk->secure.hashInfo.nOffset2), pBlk->secure.hashInfo.nSize2) ;

		szHashKey[0] = 2;	
		sha2_sum_2( szHashKey,(unsigned char *)pSRC,pBlk->secure.nHashDataLen);

		if(memcmp_aml(szHashKey,pBlk->secure.szHashKey,sizeof(szHashKey)))
		{
			//serial_puts("\nError! UBoot is corrupt!\n");
			serial_puts("\nErr! UBoot is corrupt!\n");
			writel((1<<22) | (3<<24)|500, P_WATCHDOG_TC);
			while(1);			
		}

	}
	else
	{	
		//serial_puts("\nError! UBoot is corrupt!\n");
		serial_puts("\nErr! UBoot is corrupt!\n");
		writel((1<<22) | (3<<24)|500, P_WATCHDOG_TC);
		while(1);	
	}

}
#else
static void aml_m6_sec_boot_check_2RSA_key(const unsigned char *pSRC)
{	
#define AMLOGIC_CHKBLK_ID  (0x434C4D41) //414D4C43 AMLC
#define AMLOGIC_CHKBLK_VER (3)
#define AMLOGIC_CHKBLK_HASH_KEY_LEN (32)
#define AMLOGIC_CHKBLK_INFO_MAX (60)
#define AMLOGIC_CHKBLK_INFO_LEN (56)
#define AMLOGIC_CHKBLK_AESKEY_LEN (48)
#define AMLOGIC_CHKBLK_TIME_MAX (16)
#define AMLOGIC_CHKBLK_CHK_MAX  (128)
#define AMLOGIC_CHKBLK_PAD_MAX  (4)
	
	typedef struct 
	{	
		unsigned int	nSize1; 		//sizeof(this)
		union{
			struct{ 			
				unsigned int	nTotalFileLen; //4
				unsigned int	nHashDataLen;  //4
				unsigned int	nAESLength;    //4	
				unsigned char	szHashKey[AMLOGIC_CHKBLK_HASH_KEY_LEN];//32
				unsigned int	nInfoType; //4: 0-> info; 1-> AES key; 2,3,4...
				union{
					unsigned char	szInfo[AMLOGIC_CHKBLK_INFO_MAX];	//60
					unsigned char	szAESKey[AMLOGIC_CHKBLK_AESKEY_LEN];//48
				};
				union{
				unsigned char	szTime[AMLOGIC_CHKBLK_TIME_MAX];	//16
				struct{
					unsigned int nOffset1;
					unsigned int nSize1;
					unsigned int nOffset2;
					unsigned int nSize2;
					}hashInfo;
				};
				unsigned char	szPad1[AMLOGIC_CHKBLK_PAD_MAX]; 	//4 
				}secure; //124+4	
			unsigned char	szCHK[AMLOGIC_CHKBLK_CHK_MAX]; //128
			};		//
		unsigned char	szPad2[AMLOGIC_CHKBLK_PAD_MAX]; 	
		unsigned char	szInfo[AMLOGIC_CHKBLK_INFO_MAX];	//
		unsigned char	szTime[AMLOGIC_CHKBLK_TIME_MAX];	//
		unsigned int	nSize2; 		//sizeof(this)
		unsigned int	nVer;			//AMLOGIC_CHKBLK_VER	
		unsigned int	unAMLID;		//AMLC 
	}struct_aml_chk_blk;


	writel(readl(0xda004004) & ~0x80000510,0xda004004);//clear JTAG for long time hash

	#define AML_RSA_STAGE_1_1 (0x31315352)   //RS11
	#define AML_RSA_STAGE_2_2 (0x32325352)   //RS22

	typedef struct {   int dp[136];int used, sign;} fp_intx;

	unsigned int *pID = (unsigned int *)(AHB_SRAM_BASE+(32<<10)-8);
	switch(*pID)
	{
	case AML_RSA_STAGE_1_1:
	case AML_RSA_STAGE_2_2:break;
	default:
		{
			//serial_puts("\nError! SPL is corrupt!\n");
			serial_puts("\nErr! SPL is corrupt!\n");
			AML_WATCH_DOG_START();
		}break;
	}

	unsigned int *pRSAAuthID = (unsigned int *)(AHB_SRAM_BASE+32*1024-16);
	#define AML_M6_AMLA_ID	  (0x414C4D41)	 //AMLA
	int nBlkOffset = 32*1024 - (sizeof(struct_aml_chk_blk)+256*2+32);
	if( AML_M6_AMLA_ID == *pRSAAuthID)
		nBlkOffset -= 128;
	
	//struct_aml_chk_blk *pBlk = (struct_aml_chk_blk *)(AHB_SRAM_BASE+ *pSPLSize - sizeof(struct_aml_chk_blk) - 2*256);
	struct_aml_chk_blk *pBlk = (struct_aml_chk_blk *)(AHB_SRAM_BASE+ nBlkOffset);

	if((AMLOGIC_CHKBLK_ID == pBlk->unAMLID) && (AMLOGIC_CHKBLK_VER >= pBlk->nVer) &&
		sizeof(*pBlk) == pBlk->nSize2)
	{	
		int i = 0;
		
		//RSA1204 decrypt check block
		//code reuse with bootrom
		unsigned int *pID1 =(unsigned int *)0xd9040004;
		unsigned int *pID2 =(unsigned int *)0xd904002c;
		unsigned int MX_ROM_EXPT_MODE, MX_ROM_AES_DEC;//,MX_ROM_EFUSE_READ;
		if(0xe2000003 == *pID1 && 0x00000bbb == *pID2)
		{
			//Rev-B
			MX_ROM_EXPT_MODE = 0xd90442f4;
			MX_ROM_AES_DEC   = 0xd9043c8c;
			//MX_ROM_EFUSE_READ = 0;
		}
		else if(0x00000d67 == *pID1 && 0x0e3a01000 == *pID2)
		{
			//Rev-D
			MX_ROM_EXPT_MODE = 0xd9044b64;
			MX_ROM_AES_DEC   = 0xd90444b8;
			//MX_ROM_EFUSE_READ = 0xd90403d8;
		}
		else
		{
			//serial_puts("\nError! Wrong MX chip!\n");
			serial_puts("\nErr! Wrong MX chip!\n");
			AML_WATCH_DOG_START();		
		}
		typedef  int (*func_rsa_exp)(fp_intx * G, fp_intx * X, fp_intx * P, fp_intx * Y);		
		func_rsa_exp fp_exptmodx = (func_rsa_exp)MX_ROM_EXPT_MODE;
		
			
		fp_intx c,m,n,e,d;
		
		memset_aml(&c,0,sizeof(c));
		memset_aml(&m,0,sizeof(c));
		memset_aml(&n,0,sizeof(c));
		memset_aml(&e,0,sizeof(c));
		memset_aml(&d,0,sizeof(c));

		unsigned char szEFUSE[512];
		memset_aml(szEFUSE,0,sizeof(szEFUSE));

		//load BOOT RSA key from EFUSE
		aml_mx_efuse_load(szEFUSE);

		memcpy_aml(n.dp,szEFUSE+8,128);
		n.used = 32;

		e.dp[0] = 0x010001;
		e.used = 1;				

#if defined(CONFIG_M6_SECU_BOOT_2RSA)
		unsigned char *pRSAPUK1 = (unsigned char *)pBlk + sizeof(struct_aml_chk_blk)+256;

		unsigned char szRSAPUK1[256];
		memset_aml(szRSAPUK1,0,sizeof(szRSAPUK1));

		memset_aml(&c,0,sizeof(c));
		memcpy_aml(c.dp,pRSAPUK1,128);
		c.used=32;
		
		fp_exptmodx(&c,&e,&n,&m);
		memcpy_aml(szRSAPUK1,m.dp,124);

		memset_aml(&c,0,sizeof(c));
		memcpy_aml(c.dp,pRSAPUK1+128,128);
		c.used=32;
		
		fp_exptmodx(&c,&e,&n,&m);
		memcpy_aml(szRSAPUK1+124,m.dp,124);

		memset_aml(&n,0,sizeof(c));
		memset_aml(&e,0,sizeof(c));
		
		memcpy_aml(n.dp,szRSAPUK1,128);
		n.used = 32;

		memcpy_aml(e.dp,szRSAPUK1+128,4);
		e.used = 1;	

		unsigned char szRSAPUK1Hash[32];
		memset_aml(szRSAPUK1Hash,0,sizeof(szRSAPUK1Hash));

		sha2_sum( szRSAPUK1Hash,szRSAPUK1,128+4);

		if(memcmp_aml(szRSAPUK1Hash,szEFUSE+128+8,32))
		{
			serial_puts("\nError! RSA 1st key is corrupt!\n");
			AML_WATCH_DOG_START();
		}
#endif //#if defined(CONFIG_M6_SECU_BOOT_2RSA)

		unsigned char *pAESkey = (unsigned char *)pBlk + sizeof(struct_aml_chk_blk);

		typedef struct{
			unsigned char ks[16];
		} aes_roundkey_t;
		
		typedef struct{
			aes_roundkey_t key[14+1];
		} aes256_ctx_t;
		
		typedef struct{
			aes_roundkey_t key[1]; /* just to avoid the warning */
		} aes_genctx_t;

		unsigned char szAESkey[224];
		memset_aml(szAESkey,0,sizeof(szAESkey));

		memset_aml(&c,0,sizeof(c));
		memcpy_aml(c.dp,pAESkey,128);
		c.used=32;
		
		fp_exptmodx(&c,&e,&n,&m);
		memcpy_aml(szAESkey,m.dp,124);
		
		memset_aml(&c,0,sizeof(c));
		memcpy_aml(c.dp,pAESkey+128,128);
		c.used=32;
		
		fp_exptmodx(&c,&e,&n,&m);
		memcpy_aml(szAESkey+124,m.dp,124);

		pAESkey = szAESkey;

		aes256_ctx_t ctx;
		memset_aml(&ctx,0,sizeof(ctx));	
		memcpy_aml(ctx.key,pAESkey,14*sizeof(aes_roundkey_t));

		typedef  void (*aes_dec_hx)(void* buffer, aes256_ctx_t* ctx);

		aes_dec_hx aes_dec_xxx = (aes_dec_hx) MX_ROM_AES_DEC;


		memset_aml(&c,0,sizeof(c));
		memcpy_aml(c.dp,pBlk->szCHK,128);
		c.used=32;
		fp_exptmodx(&c,&e,&n,&m);
		memcpy_aml(pBlk->szCHK,m.dp,124);

		//asm volatile ("wfi");			
		for(i = 0;i<pBlk->secure.nAESLength;i+=16)
			aes_dec_xxx((void *)(pSRC+i),&ctx);

		unsigned char szHashKey[32];
		memset_aml(szHashKey,0,sizeof(szHashKey));

		szHashKey[0] = 0;
		sha2_sum_2( szHashKey,(unsigned char *)(AHB_SRAM_BASE+pBlk->secure.hashInfo.nOffset1),pBlk->secure.hashInfo.nSize1);

		szHashKey[0] = 1;		
		sha2_sum_2( szHashKey,(unsigned char *)(AHB_SRAM_BASE+pBlk->secure.hashInfo.nOffset2), pBlk->secure.hashInfo.nSize2) ;

		szHashKey[0] = 2;	
		sha2_sum_2( szHashKey,(unsigned char *)pSRC,pBlk->secure.nHashDataLen);

		if(memcmp_aml(szHashKey,pBlk->secure.szHashKey,sizeof(szHashKey)))
		{
			//serial_puts("\nError! UBoot is corrupt!\n");
			serial_puts("\nErr! UBoot is corrupt!\n");
			AML_WATCH_DOG_START();		
		}

#if defined(CONFIG_M6_SECU_BOOT_2RSA)
		memcpy_aml(pRSAPUK1,szRSAPUK1,sizeof(szRSAPUK1));
#endif //#if defined(CONFIG_M6_SECU_BOOT_2RSA)

	}
	else
	{	
		//serial_puts("\nError! UBoot is corrupt!\n");
		serial_puts("\nErr! UBoot is corrupt!\n");
		AML_WATCH_DOG_START();
	}
}
#endif

static void aml_m6_sec_boot_check(const unsigned char *pSRC)
{

#if defined(CONFIG_M6_SECU_BOOT_2RSA)

#define AML_UBOOT_SIZE_MAX 			(600*1024)	
#define AML_MX_UCL_SIZE_ADDR 		(AHB_SRAM_BASE+0x20)
//note: the length of uboot ucl image which has beed hashed is placed to AML_MX_UCL_SIZE_ADDR
#define AML_MX_UCL_HASH_ADDR_1 		(AHB_SRAM_BASE+0x24)
#define AML_MX_UCL_HASH_SIZE_1 		(16)
#define AML_MX_UCL_HASH_ADDR_2 		(AHB_SRAM_BASE+0x1A0)
#define AML_MX_UCL_HASH_SIZE_2 		(16)
//note:  the hash key of uboot ucl image is splitted to two parts (16+16bytes) and locate in  AML_MX_UCL_HASH_ADDR_1/2

	int index = check_m6_chip_version();
	if(index < 0){
		serial_puts("\nErr! Wrong MX chip!\n");
		AML_WATCH_DOG_START();
	}
	t_func_v3 fp_05 = (t_func_v3)m6_g_action[index][5];
	unsigned int info=0;
	fp_05((int)&info,0,4);
	if(!(info &(1<<7))){
		return ;
	}

	writel(readl(0xda004004) & ~0x80000510,0xda004004);//clear JTAG for long time hash
	unsigned char szHashUCL[SHA256_HASH_BYTES];
	unsigned int nSizeUCL = *((unsigned int *)(AML_MX_UCL_SIZE_ADDR)); //get ucl length to be hashed
	memcpy(szHashUCL,(const void *)AML_MX_UCL_HASH_ADDR_1,AML_MX_UCL_HASH_SIZE_1);   //get 1st hash key
	memcpy(szHashUCL+AML_MX_UCL_HASH_SIZE_1,(const void *)AML_MX_UCL_HASH_ADDR_2,AML_MX_UCL_HASH_SIZE_2);//get 2nd hash key
	//to clear
	memcpy((void *)AML_MX_UCL_SIZE_ADDR,(const void *)(AML_MX_UCL_HASH_ADDR_1+16),AML_MX_UCL_HASH_SIZE_1+4);
	memcpy((void *)AML_MX_UCL_HASH_ADDR_2,(const void *)(AML_MX_UCL_HASH_ADDR_1+16),AML_MX_UCL_HASH_SIZE_1);

	if((!nSizeUCL) || (nSizeUCL > AML_UBOOT_SIZE_MAX) || //illegal uboot image size
		sha_256(szHashUCL,pSRC,nSizeUCL))
	{
		//serial_puts("\nError! UBoot is corrupt!\n");
		serial_puts("\nErr! UBoot is corrupt!\n");
		AML_WATCH_DOG_START();
	}	

#endif

	aml_m6_sec_boot_check_2RSA_key((const unsigned char *)pSRC);

}

#endif //CONFIG_M6_SECU_BOOT

SPL_STATIC_FUNC void fw_print_info(unsigned por_cfg,unsigned stage)
{
    serial_puts("Boot from");
    if(stage==0){
        //serial_puts(" internal device ");
        serial_puts(" int dev ");
        if(POR_GET_1ST_CFG(por_cfg) != 0)	{
	        switch(POR_GET_1ST_CFG(por_cfg))
	        {
	        		case POR_1ST_NAND:
	        			serial_puts("1stNAND\n");
	        		break;
	         		case POR_1ST_NAND_RB:
	        			serial_puts("1stNAND RB\n");
	        		break;
	        		case POR_1ST_SPI:
	        			serial_puts("1stSPI\n");
	        		break;
	        		case POR_1ST_SPI_RESERVED:
	        			serial_puts("1stSPI RESERVED\n");
	        		break;
	        		case POR_1ST_SDIO_C:
	        			serial_puts("1stSDIO C\n");
	        		break;
	        		case POR_1ST_SDIO_B:
	        			serial_puts("1stSDIO B\n");
	        		break;
	        		case POR_1ST_SDIO_A:
	        			serial_puts("1STSDIO A\n");
	        		break;       				
	        }
      }
      else{
      	switch(POR_GET_2ND_CFG(por_cfg))
      	{
      		case POR_2ND_SDIO_C:
      			serial_puts("2nd SDIO C\n");
      			break;
      		case POR_2ND_SDIO_B:
      			serial_puts("2nd SDIO B\n");
      			break;
      		case POR_2ND_SDIO_A:
      			serial_puts("2nd SDIO A\n");
      			break;
      		default:
      			serial_puts("Never checked\n");
       			break;
      	}
      }
      return;
    }
    //serial_puts(" external device ");
    serial_puts(" ext dev ");
    return ;
}


#ifdef CONFIG_BOARD_8726M_ARM
#define P_PREG_JTAG_GPIO_ADDR  (volatile unsigned long *)0xc110802c
#endif


STATIC_PREFIX int fw_load_intl(unsigned por_cfg,unsigned target,unsigned size)
{
    int rc=0;
    unsigned temp_addr;

#ifdef CONFIG_MESON_TRUSTZONE
	unsigned secure_addr;
	unsigned secure_size;
	unsigned *sram;
#endif    
#if CONFIG_UCL
    temp_addr=target-0x800000;
#else
    temp_addr=target;
#endif

    unsigned * mem;    
    switch(POR_GET_1ST_CFG(por_cfg))
    {
        case POR_1ST_NAND:
        case POR_1ST_NAND_RB:        	
            rc=nf_read(temp_addr,size);            
            break;
        case POR_1ST_SPI :
        case POR_1ST_SPI_RESERVED :
            mem=(unsigned *)(NOR_START_ADDR+READ_SIZE);
            spi_init();
#if CONFIG_UCL==0
            if((rc=check_sum(target,0,0))!=0)
            {
                return rc;
            }
#endif
            serial_puts("BootFrom SPI\n");
            memcpy((unsigned*)temp_addr,mem,size);
#ifdef CONFIG_SPI_NOR_SECURE_STORAGE
            serial_puts("BootFrom SPI get storage\n");
            spi_secure_storage_get(NOR_START_ADDR,0,0);
#endif
            break;
        case POR_1ST_SDIO_C:
        	serial_puts("BootFrom SDIO C\n");
        	rc=sdio_read(temp_addr,size,POR_2ND_SDIO_C<<3);
        	break;
        case POR_1ST_SDIO_B:
        	rc=sdio_read(temp_addr,size,POR_2ND_SDIO_B<<3);break;
        case POR_1ST_SDIO_A:
           rc=sdio_read(temp_addr,size,POR_2ND_SDIO_A<<3);break;
           break;
        default:
           return 1;
    }
    
#if defined(CONFIG_M6_SECU_BOOT)
	aml_m6_sec_boot_check((const unsigned char *)temp_addr);
#endif //CONFIG_M6_SECU_BOOT

#ifdef CONFIG_MESON_TRUSTZONE
	sram = (unsigned*)(AHB_SRAM_BASE + READ_SIZE-SECURE_OS_OFFSET_POSITION_IN_SRAM);
	secure_addr = (*sram) + temp_addr - READ_SIZE;
	sram = (unsigned*)(AHB_SRAM_BASE + READ_SIZE-SECURE_OS_SIZE_POSITION_IN_SRAM);
	secure_size = (*sram);
	secure_load(secure_addr, secure_size);	
#endif	  
	
#if CONFIG_UCL    
#ifndef CONFIG_IMPROVE_UCL_DEC
	unsigned len;    
    if(rc==0){
        serial_puts("ucl decompress\n");
        rc=uclDecompress((char*)target,&len,(char*)temp_addr);
        serial_puts(rc?"decompress false\n":"decompress true\n");
    }
#endif    
#endif    

#ifndef CONFIG_IMPROVE_UCL_DEC
    if(rc==0)    	
        rc=check_sum((unsigned*)target,magic_info->crc[1],size);
#else
    if(rc==0)    	
        rc=check_sum((unsigned*)temp_addr,magic_info->crc[1],size);
#endif              	

    return rc;
}
STATIC_PREFIX int fw_init_extl(unsigned por_cfg)
{
	 int rc=sdio_init(por_cfg);
   return rc;
}
STATIC_PREFIX int fw_load_extl(unsigned por_cfg,unsigned target,unsigned size)
{
    unsigned temp_addr;

#ifdef CONFIG_MESON_TRUSTZONE
	unsigned secure_addr;
	unsigned secure_size;
	unsigned *sram;
#endif
    
#if CONFIG_UCL
    temp_addr=target-0x800000;
#else
    temp_addr=target;
#endif	
    int rc=sdio_read(temp_addr,size,por_cfg);

#if defined(CONFIG_M6_SECU_BOOT)
	aml_m6_sec_boot_check((const unsigned char *)temp_addr);
#endif //CONFIG_M6_SECU_BOOT

#ifdef CONFIG_MESON_TRUSTZONE
	sram = (unsigned*)(AHB_SRAM_BASE + READ_SIZE-SECURE_OS_OFFSET_POSITION_IN_SRAM);
	secure_addr = (*sram) + temp_addr - READ_SIZE;
	sram = (unsigned*)(AHB_SRAM_BASE + READ_SIZE-SECURE_OS_SIZE_POSITION_IN_SRAM);
	secure_size = (*sram);
	secure_load(secure_addr, secure_size);	
#endif	

#if CONFIG_UCL
#ifndef CONFIG_IMPROVE_UCL_DEC
	unsigned len;
    if(!rc){
	    serial_puts("ucl decompress\n");
	    rc=uclDecompress((char*)target,&len,(char*)temp_addr);
        serial_puts("decompress finished\n");
    }
#endif    
#endif

#ifndef CONFIG_IMPROVE_UCL_DEC
    if(!rc)
        rc=check_sum((unsigned*)target,magic_info->crc[1],size);
#else
    if(!rc)
        rc=check_sum((unsigned*)temp_addr,magic_info->crc[1],size);
#endif        
    return rc;
}
struct load_tbl_s{
    unsigned dest;
    unsigned src;
    unsigned size;
};
extern struct load_tbl_s __load_table[2];
STATIC_PREFIX void load_ext(unsigned por_cfg,unsigned bootid,unsigned target)
{
    int i;
    unsigned temp_addr;
#if CONFIG_UCL
    temp_addr=target-0x800000;
#else
    temp_addr=target;
#endif  
     
    if(bootid==0&&(POR_GET_1ST_CFG(por_cfg)==POR_1ST_SPI||POR_GET_1ST_CFG(por_cfg)==POR_1ST_SPI_RESERVED))
    {
        // spi boot
        temp_addr=(unsigned)(NOR_START_ADDR+READ_SIZE);        
    }
        
    for(i=0;i<sizeof(__load_table)/sizeof(__load_table[0]);i++)
    {
        if(__load_table[i].size==0)
            continue;
#if CONFIG_UCL
#ifndef CONFIG_IMPROVE_UCL_DEC
    	unsigned len;
    	int rc;
        if( __load_table[i].size&(~0x3fffff))
        {
            rc=uclDecompress((char*)(__load_table[i].dest),&len,(char*)(temp_addr+__load_table[i].src));
            if(rc)
            {
                serial_put_dword(i);
                serial_puts("decompress Fail\n");
            }
        }else
#endif        
#endif  
        memcpy((void*)(__load_table[i].dest),(const void*)(__load_table[i].src+temp_addr),__load_table[i].size&0x3fffff);      
    }
}

