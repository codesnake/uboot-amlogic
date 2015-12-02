/***************************************************************************************************************
 * Amlogic M8 secure-boot solution
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
 *			 1. 2013.12.10 v0.1 for decrypt only
 *			 2. 2014.04.10 v0.2 check secure boot flag before decrypt 
 *			 3. 2014.06.26 v0.3 support image check
 *			 4. 2014.06.26 v0.4 support 2-RSA
  *			 5. 2014.07.18 v0.5 unify secure boot for M8/M8B/M8M2
 *
 **************************************************************************************************************/

#if !defined(__AMLOGIC_M8_SECURE_C_BFD0A6CA_8E97_47E1_9DDD_2E4544A831AE__)
#define __AMLOGIC_M8_SECURE_C_BFD0A6CA_8E97_47E1_9DDD_2E4544A831AE__

//help info show config
#define AML_SECURE_PROCESS_MSG_SHOW 1

int g_nStep = 0;

unsigned int g_action[5][18]={
	{0x000025e2,0xd904706c,0xd904a59c,0xd904038c,0xd904441c,0xd9047458,0xd9044518,0xd904455c,
	 0xd904459c,0xd9046e44,0xd904d014,0xd904cf60,0xd904a4f4,0xd904d220,0xd9018050},
	{0x000027ed,0xd9046f90,0xd904a388,0xd904038c,0xd9044340,0xd904737c,0xd904443c,0xd9044480,
	 0xd90444c0,0xd9046d68,0xd904cde0,0xd904cd2c,0xd904a2e0,0xd904cfec,0xd9018050},
	{0x0000074E,0xd9046f08,0xd904a300,0xd9040390,0xd90442ac,0xd90472f4,0xd90443a8,0xd90443ec,
	 0xd9044438,0xd9046ce0,0xd904cd58,0xd904cca4,0xd904a258,0xd904cf64,0xd9018040},
	{0x00000b72,0xd9046fb4,0xd904a3ac,0xd9040408,0xd9044358,0xd90473a0,0xd9044454,0xd9044498,
	 0xd90444e4,0xd9046d8c,0xd904ce04,0xd904cd50,0xd904a304,0xd904d010,0xd9018040},
	{0x00000000,0x00000000},
};

typedef  void (*t_func_v1)( int a);
typedef  void (*t_func_v2)( int a, int b);
typedef  void (*t_func_v3)( int a, int b, int c);
typedef  void (*t_func_v4)( int a, int b, int c, int d);

typedef  int  (*t_func_r1)( int a);
typedef  int  (*t_func_r2)( int a, int b);
typedef  int  (*t_func_r3)( int a, int b, int c);

static int aml_m8_sec_boot_check(unsigned char *pSRC,unsigned char *pkey1,int nkey1Len,
		unsigned char *pkey2,int nkey2Len,unsigned int *pState)
{	

#if defined(AML_SECURE_PROCESS_MSG_SHOW)

	char * pInfo1[2][2]={
		{"Aml log : M8-R1024 ",   "Aml log : M8-R2048 "},
		{"Aml log : M8-R1024-2X ","Aml log : M8-R2048-2X "},
	};
	
#if defined(CONFIG_AMLROM_SPL)
	#define AML_MSG_FAIL ("TPL fail!\n")
	#define AML_MSG_PASS ("TPL pass!\n")
	#define MSG_SHOW serial_puts	
#else
	#define AML_MSG_FAIL ("IMG fail!\n")
	#define AML_MSG_PASS ("IMG pass!\n")
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
	
	g_nStep = 0;

	/*
	switch(* (unsigned int *)0xd9040004)
	{
	case 0x25e2: break;
	case 0x27ed: g_nStep = 1 ; break;
	case 0x74E : g_nStep = 2 ; break;
	case 0xb72:  g_nStep = 3 ; break;
	default: goto exit;break;
	}
	*/
	for(g_nStep = 0;g_nStep<sizeof(g_action)/sizeof(g_action[0]);++g_nStep)
	{
		if(!g_action[g_nStep][0])
			goto exit;
		if((*(unsigned int *)0xd9040004) == g_action[g_nStep][0])
			break;
	}

	t_func_v3 fp_01 = (t_func_v3)g_action[g_nStep][1]; //void rsa_init(1,2,3)
	t_func_r3 fp_02 = (t_func_r3)g_action[g_nStep][2]; //int mpi_read_string(1,2,3)
	t_func_v3 fp_03 = (t_func_v3)g_action[g_nStep][3]; //void efuse_read(1,2,3)
	t_func_r2 fp_04 = (t_func_r2)g_action[g_nStep][4]; //int boot_rsa_read_puk(a,b)
	t_func_r3 fp_05 = (t_func_r3)g_action[g_nStep][5]; //int rsa_public(1,2,3)
	t_func_v2 fp_06 = (t_func_v2)g_action[g_nStep][6]; //void boot_aes_setkey_dec(1,2)
	t_func_v1 fp_07 = (t_func_v1)g_action[g_nStep][7]; //void boot_aes_setiv_init(1)
	t_func_v4 fp_08 = (t_func_v4)g_action[g_nStep][8]; //void boot_aes_crypt_cbc(1,2,3,4)
	t_func_v4 fp_09 = (t_func_v4)g_action[g_nStep][9]; //void sha2(1,2,3,4)
	t_func_r3 fp_10 = (t_func_r3)g_action[g_nStep][10]; //int memcpy(1,2,3)
	t_func_r3 fp_11 = (t_func_r3)g_action[g_nStep][11];//int memcmp(1,2,3)
	t_func_r1 fp_12 = (t_func_r1)g_action[g_nStep][12];//int mpi_msb(1)
	t_func_v3 fp_13 = (t_func_v3)g_action[g_nStep][13];//void memset(1,2,3)

	fp_13(g_action[g_nStep][14],0,4);

	fp_01(&cb1_ctx,0,0);

	if(pkey1 && pkey2)
	{
		if(fp_02(cb1_ctx.szBuf1,pkey1,nkey1Len) ||	fp_02(cb1_ctx.szBuf2,pkey2,nkey2Len))
			goto exit;
		cb1_ctx.len = ( fp_12( cb1_ctx.szBuf1 ) + 7 ) >> 3;			
	}
	else
	{
		unsigned int nState  = 0;
		fp_03(&nState,0,4);
		if(pState)
			*pState = nState;
		if(!(nState & (1<<7)))
		{
			nRet = 0;
			goto exit;
		}
		fp_04(&cb1_ctx,(nState & (1<<23)) ? 1 : 0);
		cb1_ctx.len = (nState & (1<<23)) ? 256 : 128;
	}

	fp_10((unsigned char*)&chk_blk,(unsigned char*)pSRC,sizeof(chk_blk));

	for(i = 0;i< sizeof(chk_blk);i+=cb1_ctx.len)
		if(fp_05(&cb1_ctx, pBuf+i, pBuf+i ))
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
		fp_10((void*)pSRC,(void*)(pSRC+chk_blk.nLength1),
			chk_blk.nLength2);

	fp_10((void*)szkey,(void*)chk_blk.szkey2,sizeof(szkey));

	fp_06( &cb2_ctx, szkey );

	fp_07(&szkey[32]);

	for (i=0; i<(chk_blk.nLength4)/16; i++)
		fp_08 ( &cb2_ctx, &szkey[32], &ct32[i*4], &ct32[i*4] );

	fp_09( pSRC,chk_blk.nLength3, szkey, 0 );	

	if(fp_11(szkey,chk_blk.szkey1,32))
		goto exit;

	nRet = 0;

exit:

#if defined(AML_SECURE_PROCESS_MSG_SHOW)

	if((0 != cb1_ctx.len) && !pkey1)
	{
		MSG_SHOW (pInfo1[(!pState )? 0 : ((*pState & (1<<31)) ? 1 : 0) ][(128 == cb1_ctx.len )?0:1]);
		MSG_SHOW (nRet ? AML_MSG_FAIL : AML_MSG_PASS);
	}
	#undef AML_MSG_FAIL
	#undef AML_MSG_PASS
	#undef MSG_SHOW
#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)	

	return nRet;
	
}

int aml_sec_boot_check(unsigned char *pSRC)
{

#if defined(AML_SECURE_PROCESS_MSG_SHOW)
	#define AML_MSG_RSA_1024 ("Aml log : M8-R1024-1X ")
	#define AML_MSG_RSA_2048 ("Aml log : M8-R2048-1X ")
#if defined(CONFIG_AMLROM_SPL)
	#define AML_MSG_FAIL ("TPL fail!\n")
	#define AML_MSG_PASS ("TPL pass!\n")
	#define MSG_SHOW serial_puts
#else
	#define AML_MSG_FAIL ("IMG fail!\n")
	#define AML_MSG_PASS ("IMG pass!\n")
	#define MSG_SHOW printf
#endif
#endif

	int nRet = -1;
	unsigned int nState = 0;
	int nSPLLen = 32<<10;
	unsigned char szCheck[36];
	unsigned char szHash[32];

	nRet = aml_m8_sec_boot_check(pSRC,0,0,0,0,&nState);
	if(nRet || !(nState & (1<<7)))
		goto exit;

	if(!(nState & (1<<31))) //2-rsa ?
		goto exit;

	if(nState & (1<<5)) //64KB SPL?
		nSPLLen = (nSPLLen << 1);

	typedef struct{
		unsigned char sz1[260];
		unsigned char sz2[256]; //rsa key N
		unsigned char sz3[4];   //rsa key E
		unsigned int  nSPLStartOffset; //spl length for hash
		unsigned int  splLenght;//spl length for hash
		unsigned char sz4[32];  //spl hash key
	} aml_spl_blk;

	aml_spl_blk *pblk = (aml_spl_blk *)(0xd9000000 + nSPLLen - 1152 );

	t_func_v3 fp_3 = (t_func_v3)g_action[g_nStep][3];
	t_func_v4 fp_9 = (t_func_v4)g_action[g_nStep][9];
	t_func_r3 fp_11 = (t_func_r3)g_action[g_nStep][11];

	fp_3(szCheck,452,36);

	fp_9(pblk->sz2,260,szHash, 0 );

	nRet = fp_11(szCheck+2,szHash,32);

	if(nRet)
	{
#if defined(AML_SECURE_PROCESS_MSG_SHOW)
		MSG_SHOW ((nState & (1<<30)) ? AML_MSG_RSA_2048  : AML_MSG_RSA_1024);
		MSG_SHOW (AML_MSG_FAIL);
#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)
		goto exit;
	}

	nRet = aml_m8_sec_boot_check(pSRC,pblk->sz2,((nState & (1<<30)) ? 256:128),pblk->sz3,4,0);

	if(nRet)
	{
#if defined(AML_SECURE_PROCESS_MSG_SHOW)
		MSG_SHOW ((nState & (1<<30)) ? AML_MSG_RSA_2048  : AML_MSG_RSA_1024);
		MSG_SHOW (AML_MSG_FAIL);
#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)
		goto exit;
	}

	fp_9(0xd9000000+pblk->nSPLStartOffset,pblk->splLenght,szHash, 0 );

	nRet = fp_11(pblk->sz4,szHash,32);

	if(nRet)
	{
#if defined(AML_SECURE_PROCESS_MSG_SHOW)
		MSG_SHOW ((nState & (1<<30)) ? AML_MSG_RSA_2048  : AML_MSG_RSA_1024);
		MSG_SHOW (AML_MSG_FAIL);
#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)

		goto exit;
	}

#if defined(AML_SECURE_PROCESS_MSG_SHOW)
	MSG_SHOW ((nState & (1<<30)) ? AML_MSG_RSA_2048  : AML_MSG_RSA_1024);
	MSG_SHOW (AML_MSG_PASS);
#endif //#if defined(AML_SECURE_PROCESS_MSG_SHOW)

exit:

	return nRet;
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

	return aml_m8_sec_boot_check(pSRC,sz1,sizeof(sz1),sz2,sizeof(sz2),0);
}
int aml_img_key_check(unsigned char *pSRC,int nLen)
{
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

	int i;
	int nRet = -1;
	st_crypto_blk1 cb1_ctx;
	st_aml_chk_blk chk_blk;
	unsigned char szkey[32+16];
	unsigned char *pBuf = (unsigned char *)&chk_blk;
	unsigned int nState  = 0;

	if(nLen & 0xF)
		goto exit;

	/*
	g_nStep = 0;

	switch(* (unsigned int *)0xd9040004)
	{
	case 0x25e2: break;
	case 0x27ed: g_nStep = 1 ; break;
	case 0x74E : g_nStep = 2 ; break;
	case 0xb72:  g_nStep = 3 ; break;
	default: goto exit;break;
	}
	*/
	for(g_nStep = 0;g_nStep<sizeof(g_action)/sizeof(g_action[0]);++g_nStep)
	{
		if(!g_action[g_nStep][0])
			goto exit;
		if((*(unsigned int *)0xd9040004) == g_action[g_nStep][0])
			break;
	}

	t_func_v3 fp_01 = (t_func_v3)g_action[g_nStep][1]; //void rsa_init(1,2,3)
	t_func_v3 fp_03 = (t_func_v3)g_action[g_nStep][3]; //void efuse_read(1,2,3)
	t_func_r2 fp_04 = (t_func_r2)g_action[g_nStep][4]; //int boot_rsa_read_puk(a,b)
	t_func_r3 fp_05 = (t_func_r3)g_action[g_nStep][5]; //int rsa_public(1,2,3)
	t_func_v4 fp_09 = (t_func_v4)g_action[g_nStep][9]; //void sha2(1,2,3,4)
	t_func_r3 fp_10 = (t_func_r3)g_action[g_nStep][10]; //int memcpy(1,2,3)
	t_func_r3 fp_11 = (t_func_r3)g_action[g_nStep][11];//int memcmp(1,2,3)
	t_func_v3 fp_13 = (t_func_v3)g_action[g_nStep][13];//void memset(1,2,3)

	fp_13(g_action[g_nStep][14],0,4);

	fp_01(&cb1_ctx,0,0);

	fp_03(&nState,0,4);
	if(!(nState & (1<<7)))
		goto exit;

	fp_04(&cb1_ctx,(nState & (1<<23)) ? 1 : 0);

	cb1_ctx.len = (nState & (1<<23)) ? 256 : 128;

	fp_10((unsigned char*)&chk_blk,(unsigned char*)(pSRC+nLen-sizeof(chk_blk)),sizeof(chk_blk));

	for(i = 0;i< sizeof(chk_blk);i+=cb1_ctx.len)
		if(fp_05(&cb1_ctx, pBuf+i, pBuf+i ))
			goto exit;

	if(AMLOGIC_CHKBLK_ID != chk_blk.unAMLID ||
		AMLOGIC_CHKBLK_VER < chk_blk.nVer)
		goto exit;

	if(sizeof(st_aml_chk_blk) != chk_blk.nSizeH ||
		sizeof(st_aml_chk_blk) != chk_blk.nSizeT ||
		chk_blk.nSizeT != chk_blk.nSizeH)
		goto exit;

	fp_09( pSRC,chk_blk.nLength3, szkey, 0 );

	nRet = fp_11(szkey,chk_blk.szkey1,32);

exit:

	return nRet;
}
#endif

int aml_is_secure_set()
{
	int nRet = 0;
	int nState = 0;
	/*
	g_nStep = 0;

	switch(* (unsigned int *)0xd9040004)
	{
	case 0x25e2: break;
	case 0x27ed: g_nStep = 1 ; break;
	case 0x74E : g_nStep = 2 ; break;
	case 0xb72:  g_nStep = 3 ; break;	
	default: goto exit;break;
	}
	*/
	for(g_nStep = 0;g_nStep<sizeof(g_action)/sizeof(g_action[0]);++g_nStep)
	{
		if(!g_action[g_nStep][0])
			goto exit;
		if((*(unsigned int *)0xd9040004) == g_action[g_nStep][0])
			break;
	}

	t_func_v3 fp_03 = (t_func_v3)g_action[g_nStep][3];
	t_func_v3 fp_13 = (t_func_v3)g_action[g_nStep][13];

	fp_13(g_action[g_nStep][13],0,4);
	fp_03(&nState,0,4);
	if((nState & (1<<7)))
	{
		nRet = 1;
		if((nState & (1<<31)))
			nRet +=1;
	}
exit:

	return nRet;
}

//here can add more feature like encrypt...
#endif //__AMLOGIC_M8_SECURE_C_BFD0A6CA_8E97_47E1_9DDD_2E4544A831AE__
