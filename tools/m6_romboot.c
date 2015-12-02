
#include <config.h>
#include <string.h>
#include "romfuns.h"
#define READ_SIZE       32*1024     // Size for data reading
#define CHECK_SIZE      8*1024      // Size for data checking

static unsigned short buf[READ_SIZE/sizeof(short)];
static void m6_caculate(void)
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
int m6_write(FILE * fp_spl,FILE * fp_in ,FILE * fp_out)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);

//note: 1. Following code is to improve the performance when load TPL (+ seucre OS)
//             get the precise TPL size for SPL
//         2. Refer SECURE_OS_SRAM_BASE from file arch\arm\include\asm\arch-m6\trustzone.h
//             can get more detail info
//         3. Here should care the READ_SIZE  is the SPL size which can be 32KB or 64KB
#define AML_UBOOT_SINFO_OFFSET (READ_SIZE-32)
#if defined(AML_UBOOT_SINFO_OFFSET)
	fseek(fp_in,0,SEEK_END);
	int nINLen = ftell(fp_in);
	nINLen = (nINLen + 0xF ) & (~0xF);
	fseek(fp_in,0,SEEK_SET);
	unsigned int * pAUINF = (int *)(buf+(AML_UBOOT_SINFO_OFFSET>>1));
	*pAUINF++ = READ_SIZE;  //32KB or 64KB
	*pAUINF   = nINLen+READ_SIZE;
	#undef AML_UBOOT_SINFO_OFFSET //for env clean up
#endif //AML_UBOOT_SINFO_OFFSET

	m6_caculate();
#ifdef CONFIG_M6_SECU_BOOT
	if((num << 1) > (30<<10)) //SPL size check, 2KB reserved, too large?
		return -1;
	unsigned int *pID = (unsigned int *)((unsigned char *)buf + READ_SIZE - 4);
	*pID = (unsigned int)( (num << 1) + 0x100);
	--pID;
	#define AML_TWO_RSA_0 (0)
	#define AML_TWO_RSA_1 (0x30315352)   //RS10
	#define AML_TWO_RSA_2 (0x30325352)   //RS20
	#define AML_M6_SECURE_BOOT_ID   (0x4C42364D) //M6BL
	#ifdef CONFIG_M6_SECU_BOOT_2RSA	
	*pID = AML_TWO_RSA_2;
	#else
	*pID = AML_TWO_RSA_1;
	#endif
	pID--;
	*pID = AML_M6_SECURE_BOOT_ID;
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
	}
	return 0;
}
int m6_write_ex(FILE * fp_spl,FILE * fp_in ,FILE * fp_out,unsigned addr)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num = fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);   // add num assignment to avoid compile warning
	m6_caculate();
#ifdef CONFIG_M6_SECU_BOOT
	*((int *)((unsigned char *)buf + READ_SIZE - 4))= num*sizeof(buf[0]);
#endif //CONFIG_HISUN
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
