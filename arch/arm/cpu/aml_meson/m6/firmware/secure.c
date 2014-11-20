#include <config.h>
#include <asm/arch/cpu.h>


//0:memset, 1:memcmp, 2:memcpy, 3:fp_exptmod, 4:fp_count_bits, 5:efuse_read
//6:aes256_dec
unsigned int m6_g_action[2][16]={
	{0xd9046f94,0xd9046bcc,0xd9046c78,0xd9044b64,0xd9045f2c,0xd9040368,
	 0xd90444b8,},//m6 Rev-D
	{0xd90465c8,0xd90461e8,0xd9046290,0xd90442f4,0xd9045590,0xd9040290,
	 0xd9043c8c,}, //m6 Rev-B
};
enum {
	M6_CHIP_REV_ERROR=-1,
	M6_CHIP_REV_D=0,
	M6_CHIP_REV_B,
};

typedef struct {   int dp[136];int used, sign;} aml_intx;

typedef  void (*t_func_v1)( int a);
typedef  void (*t_func_v2)( int a, int b);
typedef  void (*t_func_v3)( int a, int b, int c);
typedef  void (*t_func_v4)( int a, int b, int c, int d);

typedef  int  (*t_func_r1)( int a);
typedef  int  (*t_func_r2)( int a, int b);
typedef  int  (*t_func_r3)( int a, int b, int c);

typedef  int (*func_rsa_exp)(aml_intx * G, aml_intx * X, aml_intx * P, aml_intx * Y);		

#define FP_SIZE    	136
#define FP_ZPOS     0
/* clamp digits */
#define fp_clamp(a)   { while ((a)->used && (a)->dp[(a)->used-1] == 0) --((a)->used); (a)->sign = (a)->used ? (a)->sign : FP_ZPOS; }


static int check_m6_chip_version(void)
{
	int version;
	//code reuse with bootrom
	unsigned int *pID1 =(unsigned int *)0xd9040004;
	unsigned int *pID2 =(unsigned int *)0xd904002c;
	if(0xe2000003 == *pID1 && 0x00000bbb == *pID2)
	{
		//Rev-B
		//MX_ROM_EXPT_MODE = 0xd90442f4;
		//MX_ROM_AES_DEC   = 0xd9043c8c;
		//MX_ROM_EFUSE_READ = 0;
		version = M6_CHIP_REV_B;
	}
	else if(0x00000d67 == *pID1 && 0x0e3a01000 == *pID2)
	{
		//Rev-D
		//MX_ROM_EXPT_MODE = 0xd9044b64;
		//MX_ROM_AES_DEC   = 0xd90444b8;
		//MX_ROM_EFUSE_READ = 0xd90403d8;
		version = M6_CHIP_REV_D;
	}
	else{
		version = M6_CHIP_REV_ERROR;
	}
	return version;
}

/*m6_rsa_dec_pub_apage
 * index: m6 chip index 
 * src : buff size fixed 128byte
 * out : buff size fixed 124byte
 * */
static int m6_rsa_dec_pub_apage(int index,aml_intx *n,aml_intx *e,unsigned char *src,unsigned char *out)
{
	aml_intx c,m;
	t_func_v3 fp_00 = (t_func_v3)m6_g_action[index][0];
	t_func_v3 fp_02 = (t_func_v3)m6_g_action[index][2];
	func_rsa_exp fp_03 = (func_rsa_exp)m6_g_action[index][3];
	fp_00((int)&c,0,sizeof(aml_intx));
	fp_00((int)&m,0,sizeof(aml_intx));

	fp_02((int)&c.dp[0],(int)src,128);
	c.used=32;
	fp_clamp(&c);
	fp_03(&c,e,n,&m);
	fp_02((int)out,(int)&m.dp[0],124);
	return 0;
}
static int m6_rsa_dec_pub(int index,aml_intx *n,aml_intx *e,unsigned char *ct,int ctlen,unsigned char *pt,int *ptlen)
{
	int inlen,outlen;
	outlen = 0;
	for(inlen=0;inlen<ctlen;inlen+=128){
		m6_rsa_dec_pub_apage(index,n,e,ct+inlen,pt+outlen);
		outlen+=124;
	}
	*ptlen = outlen;

	return 0;
}

#define AMLOGIC_CHKBLK_ID_2  (0x444C4D41) //414D4C43 AMLD
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
	}struct_aml_chk_blk;//128+4+60+16+12


#if defined(CONFIG_M6_SECU_BOOT_2RSA)
int get_2RSA(int index,aml_intx *n,aml_intx *e, unsigned char *key,int keylen)
{
	int ret=-1;
	unsigned char szRSAPUK1[256];
	int nOutLen=256;
	unsigned char szRSAPUK1Hash[32];
	unsigned char efuseHash[32];

	t_func_v3 fp_00 = (t_func_v3)m6_g_action[index][0];
	t_func_r3 fp_01 = (t_func_r3)m6_g_action[index][1];
	t_func_v3 fp_02 = (t_func_v3)m6_g_action[index][2];
	t_func_v3 fp_05 = (t_func_v3)m6_g_action[index][5];

	fp_00((int)&szRSAPUK1Hash[0],0,sizeof(szRSAPUK1Hash));
	fp_00((int)&efuseHash[0],0,sizeof(efuseHash));
	fp_00((int)&szRSAPUK1[0],0,sizeof(szRSAPUK1));
	
	keylen = 256;
	m6_rsa_dec_pub(index,n,e,key,keylen,szRSAPUK1,&nOutLen);
	//fp_00((int)n,0,sizeof(*n));
	//fp_00((int)e,0,sizeof(*e));
	fp_05((int)&efuseHash[0],136,sizeof(efuseHash));
	ret = -1;
	sha2_sum( szRSAPUK1Hash,szRSAPUK1,128+4);
	if(!fp_01((int)&szRSAPUK1Hash[0],(int)&efuseHash[0],32)){
		fp_02((int)(&n->dp[0]),(int)&szRSAPUK1[0],128);
		n->used = 32;
		fp_02((int)(&e->dp[0]),(int)(&szRSAPUK1[0]+128),4);
		e->used = 1;
		ret = 0;
		
		fp_02((int)key,(int)&szRSAPUK1[0],keylen);
		int i;
	}
	return ret;
}
#endif

typedef struct{
	unsigned char ks[16];
} aml_aes_key_t_x;

typedef struct{
	aml_aes_key_t_x key[14+1];
} aml_aes256_ctx_t_x;

int m6_aes_decrypt(int index,unsigned char *ct,int ctlen,unsigned char *key,int keylen)
{
	int len;
	aml_aes256_ctx_t_x ctx;
	t_func_v2 fp_06 = (t_func_v2)m6_g_action[index][6];
	t_func_v3 fp_00 = (t_func_v3)m6_g_action[index][0];
	t_func_v3 fp_02 = (t_func_v3)m6_g_action[index][2];
	if(keylen>(14*sizeof(aml_aes_key_t_x))){
		keylen = 14*sizeof(aml_aes_key_t_x);
	}

	fp_00((int)&ctx,0,sizeof(ctx));
	fp_02((int)&ctx.key[0],(int)key,keylen);
	for(len=0;len<ctlen;len+=16){
		fp_06((int)(ct+len),(int)&ctx);
	}
	return 0;
}



