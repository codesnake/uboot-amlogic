
#include "romfuns.h"
#include <stdio.h>
#include <string.h>
#define READ_SIZE       32*1024     // Size for data reading
#define CHECK_SIZE      8*1024      // Size for data checking

static unsigned short buf[READ_SIZE/sizeof(short)];

#define MAGIC_WORD1         0x4f53454d  /* "MESO" */
#define MAGIC_WORD2         0x33412d4e  /* "N-A3" */

static unsigned short arc_crc_instruction(unsigned short input, unsigned short syn,
		unsigned short poly) {
	unsigned ret=input^syn;
	int i, tag;
	for (i=0; i<16; i++) {
		tag=ret&0x8000;
		ret<<=1;
		if (tag) {
			ret=ret^poly;
		}
	}
	return ret;
}

static unsigned short arc_a1h_crc_16(unsigned short * indata) {
	unsigned long i;
	unsigned short nAccum = 0;
	unsigned short *p;
	p=indata;
	for (i = 0; i < 0x1b0/2; i++)
		nAccum = arc_crc_instruction(p[i], nAccum, 0x8005);
	for (i = 512/2; i < CHECK_SIZE/2; i++)
		nAccum = arc_crc_instruction(p[i], nAccum, 0x8005);
	return nAccum;
}
static void caculate(void)
{
	unsigned short* magic;
	
	buf[0x1b8/2]=arc_a1h_crc_16(buf);
	magic=(unsigned short *)(&(buf[0x1b0/2]));
	magic[0]=(MAGIC_WORD1 & 0xFFFF);
	magic[1]=MAGIC_WORD1>>16;
	magic[2]=(MAGIC_WORD2 & 0xFFFF);
	magic[3]=MAGIC_WORD2>>16;
}
int a3_write(FILE * fp_spl,FILE * fp_in ,FILE * fp_out)
{
    int count;
    memset(buf,0,sizeof(buf));
	count = fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
	caculate();
	fwrite(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_out);
	while(!feof(fp_spl))
	{
		count=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
		fwrite(buf,sizeof(buf[0]),count,fp_out);
	}
	while(!feof(fp_in))
	{
		count=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_in);
		fwrite(buf,sizeof(buf[0]),count,fp_out);
	}
	return 0;
}
