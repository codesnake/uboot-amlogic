
#include <string.h>
#include "romfuns.h"
#define READ_SIZE       32*1024     // Size for data reading
#define CHECK_SIZE      8*1024      // Size for data checking

static unsigned short buf[READ_SIZE/sizeof(short)];
#if 0
unsigned short checksum(unsigned base,short sum)
{
    unsigned short * buf=(unsigned short * )base;
    int i;
    unsigned short tmp=0;
    for(i=0;i<0x1b0/2;i++)
    {
        tmp^=buf[i];
    }

    for(i=256;i<(CHECK_SIZE/2);i++)
    {
        tmp^=buf[i];
    }
    tmp^=buf[0x1b8/2];
    return tmp;

}
#endif
static void m3_caculate(void)
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
int m3_write(FILE * fp_spl,FILE * fp_in ,FILE * fp_out)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
	m3_caculate();
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
int m3_write_ex(FILE * fp_spl,FILE * fp_in ,FILE * fp_out,unsigned addr)
{
    int count;
    int num;
    memset(buf,0,sizeof(buf));
	num = fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);   // add num assignment to avoid compile warning
	m3_caculate();
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
