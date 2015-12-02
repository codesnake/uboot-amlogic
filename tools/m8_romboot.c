
#include <config.h>
#include <string.h>
#include "romfuns.h"
#include <time.h>

#define READ_SIZE       32*1024     // Size for data reading
#define CHECK_SIZE      8*1024      // Size for data checking

static unsigned short buf[READ_SIZE/sizeof(short)];
static void m8_caculate(void)
{
	int i;
	unsigned short sum=0;
	//unsigned * magic;
	// Calculate sum
	for(i=0;i<0x1b0/2;i++)
	{
		sum^=buf[i];
	}

	for(i=256;i<CHECK_SIZE/2;i++)
	{
		sum^=buf[i];
	}
	buf[0x1b8/2]=sum;
}
int m8_write(FILE * fp_spl,FILE * fp_in ,FILE * fp_out)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);

//note: 1. Following code is to improve the performance when load TPL (+ seucre OS)
//             get the precise TPL size for SPL
//         2. Refer SECURE_OS_SRAM_BASE from file arch\arm\include\asm\arch-m8\trustzone.h
//             can get more detail info
//         3. Here should care the READ_SIZE  is the SPL size which can be 32KB or 64KB
#define AML_UBOOT_SINFO_OFFSET (READ_SIZE-512-32)
#if defined(AML_UBOOT_SINFO_OFFSET)
	fseek(fp_in,0,SEEK_END);
	unsigned int nINLen = ftell(fp_in);
	nINLen = (nINLen + 0xF ) & (~0xF);
	fseek(fp_in,0,SEEK_SET);
	unsigned int * pAUINF = (int *)(buf+(AML_UBOOT_SINFO_OFFSET>>1));
	*pAUINF++ = READ_SIZE; //32KB or 64KB
	*pAUINF   = nINLen+READ_SIZE; 
	#undef AML_UBOOT_SINFO_OFFSET //for env clean up
#endif //AML_UBOOT_SINFO_OFFSET

	m8_caculate();
#ifdef CONFIG_AML_SECU_BOOT_V2

	#define AML_M8_SECURE_BOOT_ID			(0x4C42384D) //M8BL
	#define AML_M8_SECURE_BOOT_ID_32KBSPL	(0x424B3233) //32KB
	#define AML_M8_SECURE_BOOT_ID_64KBSPL	(0x424B3436) //64KB
	#define AML_M8_SPL_SIZE_32KB  (32<<10)
	#define AML_M8_SPL_SIZE_64KB  (64<<10)
	#define AML_RSA_STAGE_1_0 (0x30315352)	 //RS1_0
	#define AML_RSA_STAGE_2_0 (0x30325352)	 //RS2_0
	#define AMLOGIC_CHKBLK_ID (0x434C4D41)   //414D4C43@AMLC
	#define AMLOGIC_CHKBLK_VER (1)
#ifdef CONFIG_M8_64KB_SPL
	#define AML_M8_SPL_READ_SIZE (AML_M8_SPL_SIZE_64KB)
	#define AML_M8_SPL_SIZE_ID   (AML_M8_SECURE_BOOT_ID_64KBSPL)
#else
	#define AML_M8_SPL_READ_SIZE (AML_M8_SPL_SIZE_32KB)
	#define AML_M8_SPL_SIZE_ID   (AML_M8_SECURE_BOOT_ID_32KBSPL)
#endif

#ifdef CONFIG_AML_SECU_BOOT_V2_2RSA	
	#define AML_M8_AML_RSA_STAGE (AML_RSA_STAGE_2_0)
#else
	#define AML_M8_AML_RSA_STAGE (AML_RSA_STAGE_1_0)
#endif

	//M8BL 32KB(64KB) RS1(2)0 Size(TPL)
	unsigned int nTPLLength = 0;
	if((num << 1) > (31<<10)) //SPL size check, 1KB reserved, too large?
	{
		printf("aml log : ERROR! firmware.bin is too large for secure boot!\n");
		return -1;
	}
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX//
	//Note: buf need fine tune when support 64KB SPL
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX//
	unsigned int *pID = (unsigned int *)((unsigned char *)buf + AML_M8_SPL_READ_SIZE - 4);
	*pID-- = (unsigned int)( (num << 1) + 0x100);
	*pID-- = AML_M8_AML_RSA_STAGE ;
	*pID-- = AML_M8_SPL_SIZE_ID;
	*pID   = AML_M8_SECURE_BOOT_ID;

	typedef struct {
		unsigned int	nSizeH; 		  ///4@0
		struct st_secure{
			unsigned int	nORGFileLen;  ///4@4
			unsigned int	nSkippedLen;  ///4@8
			unsigned int	nHASHLength;  ///4@12
			unsigned int	nAESLength;   ///4@16
			unsigned char	szHashKey[32];//32@20
			unsigned char	szTmCreate[24];	//24@52
			unsigned char	szReserved[60];	//60@76
		}secure; //136@136
		unsigned char	szAES_Key_IMG[60];//60@136
		unsigned char	szTmCreate[48];   //48@196
		unsigned int	nSizeT; 		  ///4@244
		unsigned int	nVer;			  ///4@248
		unsigned int	unAMLID;		  ///4@252
	}st_aml_chk_blk; //256

	st_aml_chk_blk *pchk_blk = (st_aml_chk_blk*)(((unsigned char *)buf)+AML_M8_SPL_READ_SIZE-512);
	pchk_blk->nSizeH = sizeof(st_aml_chk_blk);
	pchk_blk->nSizeT = sizeof(st_aml_chk_blk);
	pchk_blk->unAMLID = AMLOGIC_CHKBLK_ID;
	pchk_blk->nVer = AMLOGIC_CHKBLK_VER;
	pchk_blk->secure.nSkippedLen = AML_M8_SPL_READ_SIZE;

	struct tm ti, *tmx;
	time(&ti);
	tmx= (struct tm *)localtime(&ti);
	//YYYY/MM/DD HH:MM:SS
	sprintf(pchk_blk->szTmCreate,"%04d\/%02d\/%02d %02d:%02d:%02d",
		tmx->tm_year+1900,tmx->tm_mon+1,tmx->tm_mday,tmx->tm_hour,tmx->tm_min,tmx->tm_sec);

	memcpy(pchk_blk->secure.szTmCreate,pchk_blk->szTmCreate,24);
	
#endif //	

	fwrite(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_out);
	while(!feof(fp_spl))
	{
		count=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
		fwrite(buf,sizeof(buf[0]),count,fp_out);
	}
	while(!feof(fp_in))
	{
		count=fread(buf,sizeof(char),sizeof(buf),fp_in);
       
        fwrite(buf,sizeof(char),count,fp_out);
		
		#ifdef CONFIG_AML_SECU_BOOT_V2
		nTPLLength += count;
		//if(sizeof(buf) != count)
		if(count & 0xF)
		{
			fwrite(buf,sizeof(char),(16-(count&15)),fp_out);
			nTPLLength += (16-(count&15));
		}
		#endif
	}

#ifdef CONFIG_AML_SECU_BOOT_V2
	fseek(fp_out,AML_M8_SPL_READ_SIZE - 4,SEEK_SET);
	fwrite(&nTPLLength,sizeof(nTPLLength),1,fp_out);
	
	nTPLLength += AML_M8_SPL_READ_SIZE;

	fseek(fp_out,AML_M8_SPL_READ_SIZE-512+4,SEEK_SET);
	fwrite(&nTPLLength,sizeof(nTPLLength),1,fp_out);
	
	fseek(fp_out,0,SEEK_END);
#endif

	return 0;
}

int m8_write_crypto(FILE * fp_spl,FILE * fp_in ,FILE * fp_out)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
	m8_caculate();
#ifdef CONFIG_AML_SECU_BOOT_V2

	#define AML_M8_SECURE_BOOT_ID			(0x4C42384D) //M8BL
	#define AML_M8_SECURE_BOOT_ID_32KBSPL	(0x424B3233) //32KB
	#define AML_M8_SECURE_BOOT_ID_64KBSPL	(0x424B3436) //64KB
	#define AML_M8_SPL_SIZE_32KB  (32<<10)
	#define AML_M8_SPL_SIZE_64KB  (64<<10)
	#define AML_RSA_STAGE_3_0 (0x30335352)	 //RS1_0
	#define AML_RSA_STAGE_4_0 (0x30345352)	 //RS2_0
	#define AMLOGIC_CHKBLK_ID (0x434C4D41)   //414D4C43@AMLC
	#define AMLOGIC_CHKBLK_VER (1)
#ifdef CONFIG_M8_64KB_SPL
	#define AML_M8_SPL_READ_SIZE (AML_M8_SPL_SIZE_64KB)
	#define AML_M8_SPL_SIZE_ID   (AML_M8_SECURE_BOOT_ID_64KBSPL)
#else
	#define AML_M8_SPL_READ_SIZE (AML_M8_SPL_SIZE_32KB)
	#define AML_M8_SPL_SIZE_ID   (AML_M8_SECURE_BOOT_ID_32KBSPL)
#endif

#ifdef CONFIG_AML_SECU_BOOT_V2_2RSA	
	#define AML_M8_AML_RSA_STAGE (AML_RSA_STAGE_4_0)
#else
	#define AML_M8_AML_RSA_STAGE (AML_RSA_STAGE_3_0)
#endif

	//M8BL 32KB(64KB) RS1(2)0 Size(TPL)
	unsigned int nTPLLength = 0;
	if((num << 1) > (31<<10)) //SPL size check, 1KB reserved, too large?
	{
		printf("aml log : ERROR! firmware.bin is too large for secure boot!\n");
		return -1;
	}
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX//
	//Note: buf need fine tune when support 64KB SPL
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX//
	unsigned int *pID = (unsigned int *)((unsigned char *)buf + AML_M8_SPL_READ_SIZE - 4);
	*pID-- = (unsigned int)( (num << 1) + 0x100);
	*pID-- = AML_M8_AML_RSA_STAGE ;
	*pID-- = AML_M8_SPL_SIZE_ID;
	*pID   = AML_M8_SECURE_BOOT_ID;

	typedef struct {
		unsigned int	nSizeH; 		  ///4@0
		struct st_secure{
			unsigned int	nORGFileLen;  ///4@4
			unsigned int	nSkippedLen;  ///4@8
			unsigned int	nHASHLength;  ///4@12
			unsigned int	nAESLength;   ///4@16
			unsigned char	szHashKey[32];//32@20
			unsigned char	szTmCreate[24];	//24@52
			unsigned char	szReserved[60];	//60@76
		}secure; //136@136
		unsigned char	szAES_Key_IMG[60];//60@136
		unsigned char	szTmCreate[48];   //48@196
		unsigned int	nSizeT; 		  ///4@244
		unsigned int	nVer;			  ///4@248
		unsigned int	unAMLID;		  ///4@252
	}st_aml_chk_blk; //256

	st_aml_chk_blk *pchk_blk = (st_aml_chk_blk*)(((unsigned char *)buf)+AML_M8_SPL_READ_SIZE-512);
	pchk_blk->nSizeH = sizeof(st_aml_chk_blk);
	pchk_blk->nSizeT = sizeof(st_aml_chk_blk);
	pchk_blk->unAMLID = AMLOGIC_CHKBLK_ID;
	pchk_blk->nVer = AMLOGIC_CHKBLK_VER;
	pchk_blk->secure.nSkippedLen = AML_M8_SPL_READ_SIZE;

	struct tm ti, *tmx;
	time(&ti);
	tmx= (struct tm *)localtime(&ti);
	//YYYY/MM/DD HH:MM:SS
	sprintf(pchk_blk->szTmCreate,"%04d\/%02d\/%02d %02d:%02d:%02d",
		tmx->tm_year+1900,tmx->tm_mon+1,tmx->tm_mday,tmx->tm_hour,tmx->tm_min,tmx->tm_sec);

	memcpy(pchk_blk->secure.szTmCreate,pchk_blk->szTmCreate,24);
	
#endif //	

	fwrite(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_out);
	while(!feof(fp_spl))
	{
		count=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
		fwrite(buf,sizeof(buf[0]),count,fp_out);
	}
	while(!feof(fp_in))
	{
		count=fread(buf,sizeof(char),sizeof(buf),fp_in);
       
        fwrite(buf,sizeof(char),count,fp_out);
		
		#ifdef CONFIG_AML_SECU_BOOT_V2
		nTPLLength += count;
		//if(sizeof(buf) != count)
		if(count & 0xF)
		{
			fwrite(buf,sizeof(char),(16-(count&15)),fp_out);
			nTPLLength += (16-(count&15));
		}
		#endif
	}

#ifdef CONFIG_AML_SECU_BOOT_V2
	fseek(fp_out,AML_M8_SPL_READ_SIZE - 4,SEEK_SET);
	fwrite(&nTPLLength,sizeof(nTPLLength),1,fp_out);
	
	nTPLLength += AML_M8_SPL_READ_SIZE;

	fseek(fp_out,AML_M8_SPL_READ_SIZE-512+4,SEEK_SET);
	fwrite(&nTPLLength,sizeof(nTPLLength),1,fp_out);
	
	fseek(fp_out,0,SEEK_END);
#endif

	return 0;
}

int m8_write_ex(FILE * fp_spl,FILE * fp_in ,FILE * fp_out,unsigned addr)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num = fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);   // add num assignment to avoid compile warning
	m8_caculate();
#ifdef CONFIG_AML_SECU_BOOT_V2
	while(1);//deadlock here, not support
#endif //CONFIG_AML_SECU_BOOT_V2
	fwrite(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_out);
	while(!feof(fp_spl))
	{
		count=fread(buf,sizeof(char),sizeof(buf),fp_spl);
		fwrite(buf,sizeof(char),(count+3)&(~3),fp_out);
	}
	while(!feof(fp_in))
	{
		count=fread(buf,sizeof(char),sizeof(buf),fp_in);
       
        fwrite(buf,sizeof(char),count,fp_out);
	}
	return 0;
    
}
