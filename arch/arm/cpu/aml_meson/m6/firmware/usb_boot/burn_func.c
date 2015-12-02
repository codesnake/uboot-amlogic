#include <linux/ctype.h>
//#include "usb_pcd.h"

#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
#include <aml_a9_cache.c>
#endif  //CONFIG_AML_SPL_L1_CACHE_ON

const unsigned char _ctype[] = {
_C,_C,_C,_C,_C,_C,_C,_C,			/* 0-7 */
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,		/* 8-15 */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 16-23 */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 24-31 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,			/* 32-39 */
_P,_P,_P,_P,_P,_P,_P,_P,			/* 40-47 */
_D,_D,_D,_D,_D,_D,_D,_D,			/* 48-55 */
_D,_D,_P,_P,_P,_P,_P,_P,			/* 56-63 */
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,	/* 64-71 */
_U,_U,_U,_U,_U,_U,_U,_U,			/* 72-79 */
_U,_U,_U,_U,_U,_U,_U,_U,			/* 80-87 */
_U,_U,_U,_P,_P,_P,_P,_P,			/* 88-95 */
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,	/* 96-103 */
_L,_L,_L,_L,_L,_L,_L,_L,			/* 104-111 */
_L,_L,_L,_L,_L,_L,_L,_L,			/* 112-119 */
_L,_L,_L,_P,_P,_P,_P,_C,			/* 120-127 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 128-143 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 144-159 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,   /* 160-175 */
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,       /* 176-191 */
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,       /* 192-207 */
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,       /* 208-223 */
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,       /* 224-239 */
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};      /* 240-255 */

/**
 * strcpy - Copy a %NUL terminated string
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 */
char * strcpy(char * dest,const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}


/**
 * strncmp - Compare two length-limited strings
 * @cs: One string
 * @ct: Another string
 * @count: The maximum number of bytes to compare
 */
int strncmp(const char * cs,const char * ct,size_t count)
{
	register signed char __res = 0;

	while (count) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
		count--;
	}

	return __res;
}



int parse_line (char *line, char *argv[])
{
	int nargs = 0;

#ifdef DEBUG_PARSER
	printf ("parse_line: \"%s\"\n", line);
#endif
	while (nargs < CONFIG_SYS_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		printf ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		printf ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

#ifdef DEBUG_PARSER
	printf ("parse_line: nargs=%d\n", nargs);
#endif
	return (nargs);
}

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

//extern void * memset(void * s,int c,size_t count);
u32 checkcum_32(const unsigned char *buf, u32 len)
{
	u32 fake_len, chksum = 0;
	u32 *ptr = (u32 *)buf;
	int i;
	serial_puts("\nbuf=");
	serial_put_hex((uint)buf, 32);
	serial_puts("len=");
	serial_put_hex(len, 32);
	if(len%4)
	{
		fake_len = len - len%4 + 4;
		memset((void *)(buf+len), 0, (fake_len-len));
	}
	else
	{
		fake_len = len;
	}
	serial_puts("fake_len=");
	serial_put_hex(fake_len, 32);
	for(i=0; i<fake_len; i+=4, ptr++)
	{
		chksum += *ptr;
	}
	return chksum;
}

#if defined(CONFIG_M6_SECU_BOOT)

#include "../../../../../../drivers/efuse/efuse_regs.h"
#include "../../../../../../drivers/secure/sha2.c"
static void __x_udelay(unsigned long usec)
{
#define MAX_DELAY (0x100)
	do {

		int i = 0;
		for(i = 0;i < MAX_DELAY;++i);
		usec -= 1;
	} while(usec);
}

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
			serial_puts("\nErr! UBoot-1 is corrupt!\n");
			AML_WATCH_DOG_START();			
		}

#if defined(CONFIG_M6_SECU_BOOT_2RSA)
		memcpy_aml(pRSAPUK1,szRSAPUK1,sizeof(szRSAPUK1));
#endif //#if defined(CONFIG_M6_SECU_BOOT_2RSA)

	}
	else
	{	
		//serial_puts("\nError! UBoot is corrupt!\n");
		serial_puts("\nErr! UBoot-2 is corrupt!\n");
		AML_WATCH_DOG_START();	
	}
}

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

int usb_run_command (const char *cmd, char *buffer)
{
	u32 addr = 0, length = 0;
	u32 crc_value, crc_verify = 0;
	int argc;
	char *argv[CONFIG_SYS_MAXARGS + 1];	/* NULL terminated	*/
	
	serial_puts("cmd:");
	serial_puts(cmd);
	
	memset(buffer, 0, CMD_BUFF_SIZE);
	if(strncmp(cmd,"crc",(sizeof("crc")-1)) == 0)
	{
		if ((argc = parse_line ((char *)cmd, argv)) == 0) {
			return -1;	/* no command at all */
		}
		addr = simple_strtoul (argv[1], NULL, 16);
		length = simple_strtoul (argv[2], NULL, 10);
		crc_verify = simple_strtoul (argv[3], NULL, 16);
		//crc_value = crc32 (0, (const uchar *) addr, length);
		crc_value = checkcum_32((unsigned char *)addr, length);
		serial_puts("crc_value=");
		serial_put_hex(crc_value, 32);
		if(crc_verify == crc_value)
		{
		//note: here the decrypt will affect buffer content, need debug !!!
#ifdef CONFIG_M6_SECU_BOOT

#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
		aml_cache_enable();		
#endif	//CONFIG_AML_SPL_L1_CACHE_ON

		serial_puts("\naml log : decrypt begin!");
		aml_m6_sec_boot_check(0x8f800000);
		serial_puts("\naml log : decrypt done!\n");
		
#if CONFIG_AML_SPL_L1_CACHE_ON
		aml_cache_disable();
#endif

#endif
			strcpy(buffer, "success");		
		}
		else
		{
			strcpy(buffer, "failed");
		}
	}
	return 0;
}
