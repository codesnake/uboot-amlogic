/***************************************************************************************************************
 * Amlogic M6TVD secure-boot solution
 *
 * Feature: decrypt & check image (uboot/kernel/efuse)
 *
 * Author: Haixiang.bao <haixiang.bao@amlogic.com>
 * 
 * Function:
 *           1. decrypt & check TPL/kernel image for secure boot
 *           2. decrypt & check EFUSE pattern for secure boot
 *
 * Copyright (c) 2013 Amlogic Inc.
 * 
 * Create date: 2013.12.10
 *
 * IMPORTANT : All the feature is chip dependent, please DO NOT update
 *             any of this file before verified.
 * Remark: 
 *           1. 2013.12.10 v0.1 for decrypt only
 *           2. 2014.01.10 v0.2 for support M6TVD
 *
 **************************************************************************************************************/

#if !defined(__AMLOGIC_M6TVD_SECURE_C_BFD0A6CA_8E97_47E1_9DDD_2E4544A831AE__)
#define __AMLOGIC_M6TVD_SECURE_C_BFD0A6CA_8E97_47E1_9DDD_2E4544A831AE__

//help info show config
#define AML_SECURE_PROCESS_MSG_SHOW 1

unsigned int g_action[2][16]={
	{0xd9046e38,0xd904a230,0xd9040498,0xd90441e8,0xd9047224,0xd90442e4,
	 0xd9044328,0xd9044368,0xd9046c10,0xd904cc88,0xd904cbd4,0xd904a188},	
	{0xd904716c,0xd904b364,0xd904038c,0xd934441c,0xd9047258,0xd9044578, 
	 0xd904438c,0xd90444c4,0xd9046d98,0xd904cfe0,0xd904ce2c,0xd904a280},
	 };

typedef  void (*t_func_v1)( int a);
typedef  void (*t_func_v2)( int a, int b);
typedef  void (*t_func_v3)( int a, int b, int c);
typedef  void (*t_func_v4)( int a, int b, int c, int d);

typedef  int  (*t_func_r1)( int a);
typedef  int  (*t_func_r2)( int a, int b);
typedef  int  (*t_func_r3)( int a, int b, int c);

static int aml_m6tvd_sec_boot_check(unsigned char *pSRC,unsigned char *pkey1,int nkey1Len,unsigned char *pkey2,int nkey2Len)
{	

#if defined(AML_SECURE_PROCESS_MSG_SHOW)

#if defined(CONFIG_AMLROM_SPL)
	#define AML_MSG_FAIL ("Aml log : ERROR! TPL secure check fail!\n")
	#define AML_MSG_PASS ("Aml log : TPL secure check pass!\n")	
	#define MSG_SHOW serial_puts	
#else
	#define AML_MSG_FAIL ("Aml log : ERROR! Image secure check fail!\n")
	#define AML_MSG_PASS ("Aml log : Image secure check pass!\n")
	#define MSG_SHOW printf
#endif

#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)
		
#define AMLOGIC_CHKBLK_ID  (0x434C4D41) //414D4C43 AMLC
#define AMLOGIC_CHKBLK_VER (1)

	typedef struct {
		unsigned int	nSizeH; 	   ////4@0
		unsigned int	nLength1;      ////4@4
		unsigned int	nLength2;      ////4@8
		unsigned int	nLength3;      ////4@12
		unsigned int	nLength4;      ////4@16
		unsigned char	szkey1[116];   ////116@20
		unsigned char	szkey2[108];   ////108@136
		unsigned int	nSizeT;        ////4@244
		unsigned int	nVer;          ////4@248
		unsigned int	unAMLID;       ////4@252
	}st_aml_chk_blk; //256

	typedef struct{
		int ver; int len;
		unsigned char szBuf1[12];
		unsigned char szBuf2[188];
	} st_crypto_blk1;

	typedef struct{
		int nr;
		unsigned int buff[80];
	} st_crypto_blk2;

	int i;
	int nRet = -1;
	st_crypto_blk1 cb1_ctx;	
	st_crypto_blk2 cb2_ctx;
	st_aml_chk_blk chk_blk;	
	unsigned char szkey[32+16];	
	unsigned int *ct32 = (unsigned int *)(pSRC);
	unsigned char *pBuf = (unsigned char *)&chk_blk;
	int nStep = 0;	
	
	switch(* (unsigned int *)0xd9040004)
	{
	case 0x7ac: break;	
	default: goto exit;break;
	}
		
	t_func_v3 fp_00 = (t_func_v3)g_action[nStep][0]; //void rsa_init(1,2,3)
	t_func_r3 fp_01 = (t_func_r3)g_action[nStep][1]; //int mpi_read_string(1,2,3)
	t_func_v3 fp_02 = (t_func_v3)g_action[nStep][2]; //void efuse_read(1,2,3)
	t_func_r2 fp_03 = (t_func_r2)g_action[nStep][3]; //int boot_rsa_read_puk(a,b)
	t_func_r3 fp_04 = (t_func_r3)g_action[nStep][4]; //int rsa_public(1,2,3)
	t_func_v2 fp_05 = (t_func_v2)g_action[nStep][5]; //void boot_aes_setkey_dec(1,2)
	t_func_v1 fp_06 = (t_func_v1)g_action[nStep][6]; //void boot_aes_setiv_init(1)
	t_func_v4 fp_07 = (t_func_v4)g_action[nStep][7]; //void boot_aes_crypt_cbc(1,2,3,4)
	t_func_v4 fp_08 = (t_func_v4)g_action[nStep][8]; //void sha2(1,2,3,4)
	t_func_r3 fp_09 = (t_func_r3)g_action[nStep][9]; //int memcpy(1,2,3)
	t_func_r3 fp_10 = (t_func_r3)g_action[nStep][10];//int memcmp(1,2,3)
	t_func_r1 fp_11 = (t_func_r1)g_action[nStep][11];//int mpi_msb(1)

	fp_00(&cb1_ctx,0,0);
	if(pkey1 && pkey2)
	{	
		if(fp_01(cb1_ctx.szBuf1,pkey1,nkey1Len) ||	fp_01(cb1_ctx.szBuf2,pkey2,nkey2Len))
			goto exit;
		cb1_ctx.len = ( fp_11( cb1_ctx.szBuf1 ) + 7 ) >> 3;			
	}
	else
	{
		unsigned int nState  = 0;
		fp_02(&nState,0,4);		
		fp_03(&cb1_ctx,(nState & (1<<23)) ? 1 : 0);
		cb1_ctx.len = (nState & (1<<23)) ? 256 : 128;
	}
	
	fp_09((unsigned char*)&chk_blk,(unsigned char*)pSRC,sizeof(chk_blk));

	for(i = 0;i< sizeof(chk_blk);i+=cb1_ctx.len)
		if(fp_04(&cb1_ctx, pBuf+i, pBuf+i ))
			goto exit;

	if(AMLOGIC_CHKBLK_ID != chk_blk.unAMLID ||
		AMLOGIC_CHKBLK_VER < chk_blk.nVer)
		goto exit;

	if(sizeof(st_aml_chk_blk) != chk_blk.nSizeH ||
		sizeof(st_aml_chk_blk) != chk_blk.nSizeT ||
		chk_blk.nSizeT != chk_blk.nSizeH)
		goto exit;

	if((sizeof(st_aml_chk_blk) != chk_blk.nLength2))
		goto exit;

	if(chk_blk.nLength2)
		fp_09((void*)pSRC,(void*)(pSRC+chk_blk.nLength1),
			chk_blk.nLength2);

	fp_09((void*)szkey,(void*)chk_blk.szkey2,sizeof(szkey));
	fp_05( &cb2_ctx, szkey );
	fp_06(&szkey[32]);
	for (i=0; i<(chk_blk.nLength4)/16; i++)
		fp_07 ( &cb2_ctx, &szkey[32], &ct32[i*4], &ct32[i*4] );
	
	fp_08( pSRC,chk_blk.nLength3, szkey, 0 );	
	if(fp_10(szkey,chk_blk.szkey1,32))
		goto exit;

	nRet = 0;

exit:

#if defined(AML_SECURE_PROCESS_MSG_SHOW)
	MSG_SHOW (nRet ? AML_MSG_FAIL : AML_MSG_PASS);
		
	#undef AML_MSG_FAIL
	#undef AML_MSG_PASS
	#undef MSG_SHOW		
	
#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)	

	return nRet;
	
}


int aml_sec_boot_check(unsigned char *pSRC)
{
	return aml_m6tvd_sec_boot_check(pSRC,0,0,0,0);
}

#if !defined(CONFIG_AMLROM_SPL)
int aml_sec_boot_check_efuse(unsigned char *pSRC)
{
	unsigned char sz1[] = {
	0xC4,0x78,0x4C,0xC7,0x7A,0xE8,0xF2,0xDB,0x2C,0xBD,0x08,0x17,0xF7,0xB5,0xE9,0xAE,
	0xBB,0x5B,0xD0,0xEC,0x65,0x2A,0xF8,0x47,0xAA,0x7C,0xE8,0x71,0x39,0x6E,0xD7,0xFB,
	0xFC,0x8A,0x38,0x94,0x34,0xE3,0x3A,0xDF,0x0E,0x19,0x5A,0x24,0x9B,0xC1,0x88,0x77,
	0x6B,0xFA,0x63,0x27,0x89,0x18,0x27,0x67,0xE1,0xE7,0x0E,0xD2,0xC4,0x88,0x2B,0xE8,
	0xF1,0x0E,0x0C,0x26,0x22,0xB5,0xB0,0x81,0x22,0x63,0xAA,0xAD,0x46,0xC8,0xD6,0x19,
	0x7C,0x0C,0xD0,0x8C,0xEF,0xE7,0x9A,0x8E,0x14,0x6A,0x49,0x27,0xCD,0xD5,0xBF,0x91,
	0xF8,0x5B,0x9E,0x2A,0xF1,0x0C,0xD2,0x07,0x67,0xF5,0x90,0xB2,0x84,0x71,0x68,0x82,
	0x09,0x0F,0x63,0x13,0x1B,0x4F,0xED,0x55,0x49,0x8B,0x95,0x4F,0x28,0xC0,0x8E,0x29,
	};
	
	unsigned char sz2[] = {
	0x01,0x37,0x4B,};

	return aml_m6tvd_sec_boot_check(pSRC,sz1,sizeof(sz1),sz2,sizeof(sz2));
}
#endif //

//here can add more feature like encrypt...

#endif //__AMLOGIC_M6TVD_SECURE_C_BFD0A6CA_8E97_47E1_9DDD_2E4544A831AE__