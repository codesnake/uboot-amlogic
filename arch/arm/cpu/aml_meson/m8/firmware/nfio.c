#include <asm/arch/reg_addr.h>
#include <asm/arch/timing.h>

#include <memtest.h>
#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/nand.h>

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
#include <aml_a9_cache.c>
#endif //defined(CONFIG_AML_SPL_L1_CACHE_ON)


//#include <asm/arch/firm/config.h>
/**
read one page only
*/
/*
    ext
    bit 0~21 , same with NAND to MEM define of A3
    bit 22 , 1:large  0:small
    bit 23 , 1:without RB pin  0:with RB pin
*/
#undef NAND_INFO_BUF


#define MAX_CE_NUM_SUPPORT 4

#ifdef CONFIG_NAND_AML_M8
#define NAND_INFO_BUF 		0x10000
#define NAND_OOB_BUF  		0x22000
#define NAND_TEMP_BUF 	0x2000000
#define NAND_DATA_BUF   0x3000000
#else
#define NAND_INFO_BUF 		0x80000000
#define NAND_OOB_BUF  		0x80002000
#define NAND_TEMP_BUF 	0x82000000
#define NAND_DATA_BUF   0x83000000
#endif

#ifdef CONFIG_SECURE_NAND
#define NAND_SECURE_BUF   (SECUREOS_KEY_BASE_ADDR+0xe0000)
#endif

#define 	SECURE_HEAD_MAGIC 			0x6365736e  //stand for "nsec"  in phynand.h
#define CONFIG_SECURE_SIZE         		(0x10000*2) //128k
#define NAND_SECURE_BLK      2
#define NAND_SEC_MAX_BLK_NUM  30

//static struct nand_page0_info_t *page0_info = (NAND_TEMP_BUF+384)-sizeof(nand_page0_info_t);
static struct nand_retry_t nand_retry;

//** ECC mode 7, dma 528 bytes(data+parity), short mode , with scramble
//** Default large page mode and without RB PIN, 4 ecc units
#define DEFAULT_ECC_MODE  ((1<<23) |(1<<22) | (2<<20) |(1<<19) |(1<<17)|(7<<14)|(1<<13)|(48<<6)|1)
//#define	NAND_RETRY_ROMCODE

extern void memset(void *,int,int);

unsigned char pagelist_hynix256[128] = {
	0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B,
	0x0E, 0x0F, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B,
	0x1E, 0x1F, 0x22, 0x23, 0x26, 0x27, 0x2A, 0x2B,
	0x2E, 0x2F, 0x32, 0x33, 0x36, 0x37, 0x3A, 0x3B,

	0x3E, 0x3F, 0x42, 0x43, 0x46, 0x47, 0x4A, 0x4B,
	0x4E, 0x4F, 0x52, 0x53, 0x56, 0x57, 0x5A, 0x5B,
	0x5E, 0x5F, 0x62, 0x63, 0x66, 0x67, 0x6A, 0x6B,
	0x6E, 0x6F, 0x72, 0x73, 0x76, 0x77, 0x7A, 0x7B,

	0x7E, 0x7F, 0x82, 0x83, 0x86, 0x87, 0x8A, 0x8B,
	0x8E, 0x8F, 0x92, 0x93, 0x96, 0x97, 0x9A, 0x9B,
	0x9E, 0x9F, 0xA2, 0xA3, 0xA6, 0xA7, 0xAA, 0xAB,
	0xAE, 0xAF, 0xB2, 0xB3, 0xB6, 0xB7, 0xBA, 0xBB,

	0xBE, 0xBF, 0xC2, 0xC3, 0xC6, 0xC7, 0xCA, 0xCB,
	0xCE, 0xCF, 0xD2, 0xD3, 0xD6, 0xD7, 0xDA, 0xDB,
	0xDE, 0xDF, 0xE2, 0xE3, 0xE6, 0xE7, 0xEA, 0xEB,
	0xEE, 0xEF, 0xF2, 0xF3, 0xF6, 0xF7, 0xFA, 0xFB,
};
unsigned char pagelist_1ynm_hynix256[128] = {
	0x00, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d,
	0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d,
	0x1f, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d,
	0x2f, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d,

	0x3f, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d,	
	0x4f, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d,
	0x5f, 0x61, 0x63, 0x65, 0x67, 0x69, 0x6b, 0x6d,
	0x6f, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7d,
	
	0x7f, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d,
	0x8f, 0x91, 0x93, 0x95, 0x97, 0x99, 0x9b, 0x9d,
	0x9f, 0xa1, 0xA3, 0xA5, 0xA7, 0xA9, 0xAb, 0xAd,
	0xAf, 0xb1, 0xB3, 0xB5, 0xB7, 0xB9, 0xBb, 0xBd,

	0xBf, 0xc1, 0xC3, 0xC5, 0xC7, 0xC9, 0xCb, 0xCd,
	0xCf, 0xd1, 0xD3, 0xD5, 0xD7, 0xD9, 0xDb, 0xDd,
	0xDf, 0xe1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEb, 0xEd,
	0xEf, 0xf1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFb, 0xFd,
};
STATIC_PREFIX short nfio_reset(int no_rb)
{
    unsigned long tmp=0;

	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

	writel((CE0 | CLE | 0xff), P_NAND_CMD);  //reset
	writel((CE0 | IDLE | 2), P_NAND_CMD);	 // dummy
	
	// set Timer 13 ms.
	if(no_rb) { // without RB/
		writel((CE0 | CLE | 0x70), P_NAND_CMD);  //read status
		writel((CE0 | IDLE | 2), P_NAND_CMD);	 // tWB
		writel((IO6 | RB | 16), P_NAND_CMD);	 //wait IO6
		writel((CE0 | IDLE | 2), P_NAND_CMD);	 // dummy
		writel(CE0 | CLE  | 0, P_NAND_CMD); 	 //chage to page read mode
		//		  (*P_NAND_CMD) = CE0 | CLE | 0;
		writel((CE0 | IDLE | 2), P_NAND_CMD);	 // dummy
	}else{ // with RB
			writel((CE0 | RB | 16), P_NAND_CMD);	 //wait R/B
	}
	
	while ((readl(P_NAND_CMD)>>22&0x1f) > 0){
		if (tmp>>27&1) { // timer out
		    //OUT_PUT(tmp);
		    return ERROR_NAND_TIMEOUT;
		}
   	 }
    
    // reset OK
    return 0;
}

STATIC_PREFIX short nfio_read_id(void)
{
	writel((CE0 | IDLE | 0), P_NAND_CMD);	 
    	writel((CE0 | CLE | 0x90), P_NAND_CMD);  
	writel((CE0 | IDLE | 3), P_NAND_CMD);	 	
	writel((CE0 | ALE | 0), P_NAND_CMD);	 	
	writel((CE0 | IDLE | 3), P_NAND_CMD);	 	
	writel((CE0 | DRD | 4 ), P_NAND_CMD);	 	

	
	writel((CE0 | IDLE | 0), P_NAND_CMD);	 		
	writel((CE0 | IDLE | 0), P_NAND_CMD);	 	


	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

	nand_retry.id = readb(P_NAND_BUF)&0xff;
	return 0;
}
#ifdef NAND_RETRY_ROMCODE

#define EFUSE_NAND_EXTRA_CMD        4 // size: 16
#define EFUSE_LICENSE_OFFSET	    0 // size:  4
#define LICENSE_NAND_USE_RB_PIN 	   (1<<22) 
#define LICENSE_NAND_ENABLE_RETRY	   (1<<21) 
#define LICENSE_NAND_EXTRA_CMD		   (1<<20) 

// toshiba retry ------------------------------------------
// 0    : pre condition + retry 0.
// 1~6  : valid retry, otherwise no action. 
// 7, 0xff : exit, back to normal.
// loop from 0 to 7, total 8.
void read_retry_toshiba(int retry, int *cmd)
{
    unsigned char val[] = {
        0x04, 0x04, 0x7C, 0x7E,  
        0x00, 0x7C, 0x78, 0x78,  
        0x7C, 0x76, 0x74, 0x72,  
        0x08, 0x08, 0x00, 0x00,  
        0x0B, 0x7E, 0x76, 0x74,  
        0x10, 0x76, 0x72, 0x70,  
        0x02, 0x00, 0x7E, 0x7C,  
        0x00, 0x00, 0x00, 0x00};
    unsigned i, k, retry_val_idx;

    i = 0;
    if (retry == 7) retry = 0xff;
    retry_val_idx = retry == 0xff ? 7 : retry;
    
    if (retry_val_idx < 8) {
        // pre condition, send before first.
        if (retry == 0) { 
            cmd[i++] = (CE0 | CLE | 0x5c);
            cmd[i++] = (CE0 | CLE | 0xc5);
            cmd[i++] = (CE0 | IDLE);
        }

        for (k=4; k<8; k++) {
            cmd[i++] = (CE0 | CLE  | 0x55);
            cmd[i++] = (CE0 | IDLE | 2);        //Tcalsv            
            cmd[i++] = (CE0 | ALE  | k);   
            cmd[i++] = (CE0 | IDLE | 2);        //Tcalsv         
            cmd[i++] = (CE0 | DWR  | val[retry_val_idx*4 + k-4]);
            cmd[i++] = (CE0 | IDLE);    
        }

        cmd[i++] = (CE0 | CLE  | 0x55);
        cmd[i++] = (CE0 | IDLE | 2);        //Tcalsv        
        cmd[i++] = (CE0 | ALE  | 0xd);
        cmd[i++] = (CE0 | IDLE | 2);        //Tcalsv          
        cmd[i++] = (CE0 | DWR  | 0); 
        cmd[i++] = (CE0 | IDLE);    

        if (retry == 6){ 
            cmd[i++] = (CE0 | CLE | 0xb3);
            cmd[i++] = (CE0 | IDLE);                                     
        }

        if (retry != 0xff) {
            cmd[i++] = (CE0 | CLE | 0x26);
            cmd[i++] = (CE0 | CLE | 0x5d);
            cmd[i++] = (CE0 | IDLE);    
        }
    }   

    // exit and check rb
    if (retry == 0xff) {   
        cmd[i++] = (CE0 | CLE  | 0xff);
        cmd[i++] = (CE0 | IDLE | 2);        //Twb            
    
        if (nand_retry.no_rb) { 
            cmd[i++] = (CE0 | CLE  | 0x70); 
            cmd[i++] = (CE0 | IDLE | 2);    
            cmd[i++] = (IO6 | RB   | 16); 
        }
        else{
            cmd[i++] = (CE0 | RB | 16);    
        }
    }   
    
    cmd[i] = 0;
}

// sandisk retry ------------------------------------------
// 0    : activation + retry 0.
// 1~20 : valid retry, otherwise no action. 
// 21, 0xff : exit
// loop from 0 to 21, total 22.
void read_retry_sandisk(int retry, int *cmd)
{
    int i, k;
    unsigned char val[] = {
        0x00, 0x00, 0x00, 
        0xf0, 0xf0, 0xf0, 
        0xef, 0xe0, 0xe0, 
        0xdf, 0xd0, 0xd0, 
        0x1e, 0xe0, 0x10, 
        0x2e, 0xd0, 0x20, 
        0x3d, 0xf0, 0x30, 
        0xcd, 0xe0, 0xd0, 
        0x0d, 0xd0, 0x10, 
        0x01, 0x10, 0x20, 
        0x12, 0x20, 0x20, 
        0xb2, 0x10, 0xd0, 
        0xa3, 0x20, 0xd0, 
        0x9f, 0x00, 0xd0, 
        0xbe, 0xf0, 0xc0, 
        0xad, 0xc0, 0xc0, 
        0x9f, 0xf0, 0xc0, 
        0x01, 0x00, 0x00,   
        0x02, 0x00, 0x00,   
        0x0d, 0xb0, 0x00,   
        0x0c, 0xa0, 0x00};

    i = 0;
    if (retry == 21) retry = 0xff;

    if (retry == 0) { // activation and init, entry 0.
        cmd[i++] = (CE0 | CLE | 0x3b);
        cmd[i++] = (CE0 | CLE | 0xb9);
        for (k=4; k<13; k++) {
            cmd[i++] = (CE0 | CLE | 0x54);
            cmd[i++] = (CE0 | ALE | k);
            cmd[i++] = (CE0 | DWR | 0);
        }
        cmd[i++] = (CE0 | CLE | 0xb6);
    }
    else if (retry < 21) { // entry: 1~20
        cmd[i++] = (CE0 | CLE | 0x3b);
        cmd[i++] = (CE0 | CLE | 0xb9);
        for (k=0; k<3; k++) {
            cmd[i++] = (CE0 | CLE | 0x54);
            cmd[i++] = (CE0 | ALE | (k==0?4:k==1?5:7));
            cmd[i++] = (CE0 | DWR | val[retry*3+k]);
        }
        cmd[i++] = (CE0 | CLE | 0xb6);
    }
    else if (retry == 255) {
        cmd[i++] = (CE0 | CLE | 0x3b);
        cmd[i++] = (CE0 | CLE | 0xb9);
        for (k=0; k<3; k++) {
            cmd[i++] = (CE0 | CLE | 0x54);
            cmd[i++] = (CE0 | ALE | (k==0?4:k==1?5:7));
            cmd[i++] = (CE0 | DWR | 0);
        }
        cmd[i++] = (CE0 | CLE | 0xd6);
    }
    cmd[i] = 0;
}

// micron retry ------------------------------------------
// 0    : disable 
// 1~7  : valid retry, otherwise no action. 
// 0xff : same as 0.
// loop from 0 to 7, total 8.
void read_retry_micron(int retry, int *cmd)
{
    int i;
	
    i = 0;
    if (retry == 0xff) retry = 0;
    
    if (retry < 8) {
        cmd[i++] = (CE0 | CLE  | 0xef);
        cmd[i++] = (CE0 | ALE  | 0x89);
        cmd[i++] = (CE0 | IDLE | 3);        //Tadl
        cmd[i++] = (CE0 | DWR  | retry);
        cmd[i++] = (CE0 | DWR  | 0);
        cmd[i++] = (CE0 | DWR  | 0);
        cmd[i++] = (CE0 | DWR  | 0);
        cmd[i++] = (CE0 | IDLE | 2);        //Twb

        //check rb for Tfeat  
        if (nand_retry.no_rb) { 
            cmd[i++] = (CE0 | CLE  | 0x70); 
            cmd[i++] = (CE0 | IDLE | 2);	
            cmd[i++] = (IO6 | RB   | 16); 
        }
        else{
            cmd[i++] = (CE0 | RB | 16);             
        }
    }

    cmd[i] = 0;
}

void read_retry_hynix(int retry, int *cmd)
{
    // use SLC mode.
}

// samsung retry ------------------------------------------
// 0    : disable
// 1~14 : valid retry, otherwise no action. 
// 0xff : same as 0.
// loop from 0 to 14, total 15.
void read_retry_samsung(int retry, int *cmd)
{
    int i, k;
    unsigned char adr[] = {
        0xa7, 0xa4, 0xa5, 0xa6};
    unsigned char val[] = {
        0x00, 0x00, 0x00, 0x00,
        0x05, 0x0A, 0x00, 0x00,
        0x28, 0x00, 0xEC, 0xD8,
        0xED, 0xF5, 0xED, 0xE6,
        0x0A, 0x0F, 0x05, 0x00,
        0x0F, 0x0A, 0xFB, 0xEC,
        0xE8, 0xEF, 0xE8, 0xDC,
        0xF1, 0xFB, 0xFE, 0xF0,
        0x0A, 0x00, 0xFB, 0xEC,
        0xD0, 0xE2, 0xD0, 0xC2,
        0x14, 0x0F, 0xFB, 0xEC,
        0xE8, 0xFB, 0xE8, 0xDC,
        0x1E, 0x14, 0xFB, 0xEC,
        0xFB, 0xFF, 0xFB, 0xF8,
        0x07, 0x0C, 0x02, 0x00};

    i = 0;
    if (retry == 0xff) retry = 0;
    if (retry < 15) {
        for (k=0; k<4; k++) {
            cmd[i++] = (CE0 | CLE  | 0xa1);
            cmd[i++] = (CE0 | ALE  | 0);
            cmd[i++] = (CE0 | ALE  | adr[k]);
            cmd[i++] = (CE0 | IDLE | 2);        //Tadl
            cmd[i++] = (CE0 | DWR  | val[retry*4+k]);
            cmd[i++] = (CE0 | IDLE | 8);        //over 300ns before next cmd
        }
    }
    cmd[i] = 0;
}

// user retry ------------------------------------------
// 0    : disable
// n    : valid retry, otherwise no action. 
// 0xff : same as 0.
// loop from 0 to n, total n+1.
// each cmd is {type, val}, each retry is ended with {0, 0},
// can be used to enable slc mode or read retry.
void read_retry_user(int retry, int *cmd)
{
    static int pcmd = 0;
    nand_cmd_t *p = (nand_cmd_t *)(NAND_TEMP_BUF + sizeof(nand_page0_cfg_t));
    int i;

    if (retry == 0xff) retry = 0;
    if (retry == 0) pcmd = 0;
    
    i = 0;
    while (p[pcmd].type != 0) {
        cmd[i++] = CE0 | (p[pcmd].type<<14) | (p[pcmd].val);
        pcmd++;
    }
    cmd[i] = 0;
    pcmd++;
}

void read_retry_efuse(int retry, int *cmd)
{
    nand_cmd_t p[8];
    int i, pcmd;

    efuse_read( (unsigned *)p, EFUSE_NAND_EXTRA_CMD, 16 );

    i = 0;
    pcmd = 0;
    while (pcmd < 8 && p[pcmd].type != 0) {
        cmd[i++] = CE0 | (p[pcmd].type<<14) | (p[pcmd].val);
        pcmd++;
    }
    cmd[i] = 0;
}

void nfio_read_retry(int mode) 
{
    static int retry = 0;
    int i, cmd[128];

    if (nand_retry.max == 0)
        return;
	
    // pre and post.
    if (mode == 0) 
        retry = 0;
    else if (mode == 2) 
        retry = 0xff;

    switch (nand_retry.id) {
    case NAND_MFR_MICRON: 
        read_retry_micron(retry, cmd);
        break;

    case NAND_MFR_TOSHIBA:
        read_retry_toshiba(retry, cmd);
        break;

    case NAND_MFR_HYNIX:
        read_retry_hynix(retry, cmd);
        break;

    case NAND_MFR_SAMSUNG:
        read_retry_samsung(retry, cmd);
        break;

    case NAND_MFR_SANDISK:
        read_retry_sandisk(retry, cmd);
        break;

    case NAND_MFR_USER:
        read_retry_user(retry, cmd);
        break;

    case NAND_MFR_EFUSE:
        read_retry_efuse(retry, cmd);
        break;
    }
	
    retry = retry+1 < nand_retry.max ? retry+1 : 0;

    i = 0;
    while ((readl(P_NAND_CMD)>>22&0x1f)<0x1f && cmd[i] != 0) {
	writel(cmd[i++], P_NAND_CMD);
    }
}
#else
void nfio_read_retry(int mode) 
{

}
#endif

STATIC_PREFIX short nfio_page_read_hwctrl(unsigned src,unsigned mem, unsigned char *oob_buf, unsigned ext)
{
	//int i;
	int k, ecc_mode, short_mode, short_size, pages, page_size;
	unsigned long info_adr = NAND_INFO_BUF;
	volatile unsigned long long * info_buf=(volatile unsigned long long *)NAND_INFO_BUF;
	int ret = 0;

	// nand parameters
	ecc_mode = ext>>14&7;
	short_mode = ext>>13&1;
	short_size = ext>>6&0x7f;
	pages = ext&0x3f;
	page_size = short_mode ? short_size :
	ecc_mode<2 ? 64 : 128; // unit: 8 bytes;

	memset((void*)info_buf, 0, pages*PER_INFO_BYTE);

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
	flush_dcache_range((unsigned long)info_buf,(unsigned long)(info_buf+pages*PER_INFO_BYTE));
	invalidate_dcache_range(mem, mem+page_size * 8 * pages);
#endif  

	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

		//serial_puts("nand read addr=0x");        
		//serial_put_hex(src,32);            
		//serial_puts("\n");
		
	// set data/info address
	writel(info_adr, P_NAND_IADR);
	writel(mem, P_NAND_DADR);

    	//add A2H command for SLC read
    	if((ext>>24)&1){
		writel(CE0 | CLE  | 0xa2, P_NAND_CMD);
		writel((CE0 | IDLE | 2), P_NAND_CMD);
    	}

	// send cmds read 00 ... 30
	writel(CE0 | IDLE, P_NAND_CMD);
	writel(CE0 | CLE  | 0, P_NAND_CMD);
	writel(CE0 | ALE  | 0, P_NAND_CMD);
	if((ext>>22)&1){
	  	writel(CE0 | ALE  | 0,P_NAND_CMD);
	   //     (*P_NAND_CMD) = CE0 | ALE  | 0;
	}
	writel(CE0 | ALE  | ((src)&0xff), P_NAND_CMD);
	writel(CE0 | ALE  | ((src>>8)&0xff), P_NAND_CMD);
	writel(CE0 | ALE  | 0, P_NAND_CMD);
	if((ext>>22)&1){
		writel(CE0 | CLE  | 0x30, P_NAND_CMD);
	}

	writel((CE0 | IDLE | 40), P_NAND_CMD);    // tWB, 8us

	// set Timer 13 ms.
	if(ext>>23&1) { // without RB
		writel((CE0 | CLE | 0x70), P_NAND_CMD);  //read status
		writel((CE0 | IDLE | 2), P_NAND_CMD);    // tWB
		writel((IO6 | RB | 16), P_NAND_CMD);     //wait IO6
		writel((CE0 | IDLE | 2), P_NAND_CMD);    // dummy
		writel(CE0 | CLE  | 0, P_NAND_CMD);      //chage to page read mode
		//        (*P_NAND_CMD) = CE0 | CLE | 0;
		writel((CE0 | IDLE | 2), P_NAND_CMD);    // dummy
	}else{ // with RB
	    	writel((CE0 | RB | 16), P_NAND_CMD);     //wait R/B
	}

	// scramble, first page is disabled by default.
	// code page is enabled/disabled by ext[19].
	// 0xa3 + (src&0xff) is used as seed,
	writel(SEED | (0xc2 + (src&0x7fff)), P_NAND_CMD);
	writel((ext&0x3fffff), P_NAND_CMD);

	//    (*P_NAND_CMD) = (ext&0x3fffff);
	writel((CE0 | IDLE ), P_NAND_CMD);           //send dummy cmd idle
	writel((CE0 | IDLE ), P_NAND_CMD);           //send dummy cmd idle

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
	while((readl(P_NAND_CMD)>>22&0x1f) > 0);
	do{
	invalidate_dcache_range((unsigned long)info_buf,(unsigned long)(info_buf+pages*PER_INFO_BYTE));
	}while(info_buf[pages-1]==0);
#else
	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
#endif //defined(CONFIG_AML_SPL_L1_CACHE_ON)
	while(info_buf[pages-1]==0);

	for (k=0; k<pages; k++){

        while(info_buf[k]==0);

		if((info_buf[k]>>16&0x3f) < 0xa){
			ret = ERROR_NAND_BLANK_PAGE;
		}

	    	if ((info_buf[k]>>24&0x3f) == 0x3f) {// uncorrectable
			if((ext>>24)&1){ //if with a2h cmd, just need reset

				return nfio_reset(ext>>23&1);
			}

			if(ret != ERROR_NAND_BLANK_PAGE){ //if randomization, check 0xff page, zero_cnt less than 10
			    serial_puts("ERROR_NAND_ECC at:");
		        serial_put_hex(k,8);            
		        serial_puts(" info_buf:");		        
#if 0//defined(CONFIG_AML_SPL_L1_CACHE_ON)		
                serial_puts("\n");        		        
            	for(i=0;i<pages;i++){
            	   serial_put_hex(i,32);  
            	   serial_puts("\t"); 
            	   serial_put_hex(info_buf[i],64);          
                   serial_puts("\n");
            	}	
#else
        	    serial_put_hex(info_buf[k],32);          
                serial_puts("\n");	            		                    
#endif		    		
				ret = ERROR_NAND_ECC;
			}
			break;
	    	}

		if(ret)
			break;

#if 1

		oob_buf[0] = (info_buf[k]&0xff);
		oob_buf[1] = ((info_buf[k]>>8)&0xff);
#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
		invalidate_dcache_range((unsigned long)oob_buf,(unsigned long)(oob_buf+2));
#endif
		oob_buf += 2;
#else
	    	if ((info_buf[k]&0xc000ffff) != 0xc000aa55) //magic word error
	       {
	       	serial_puts("info_buf ERROR_NAND_MAGIC_WORD\n");
			return ERROR_NAND_MAGIC_WORD;
	    	}
#endif
	}

#if 0//defined(CONFIG_AML_SPL_L1_CACHE_ON)
	    invalidate_dcache_range(mem,mem+page_size * 8 * pages);
#endif

	return ret;
}

STATIC_PREFIX short nfio_page_read(unsigned src, unsigned mem, unsigned char *oob_buf, unsigned ext)
{
	unsigned read_page/*, new_nand_type, pages_in_block*/;
	int retry_cnt, ret = 0;
	struct nand_page0_info_t *page0_info; 

#if 0//defined(CONFIG_AML_SPL_L1_CACHE_ON)
	    invalidate_dcache_range((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t),(NAND_TEMP_BUF+384));
#endif    
    
    page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));
    
	read_page = src;
	if(page0_info->new_nand_type) {		//for new nand
		if(page0_info->new_nand_type < 10){   //for hynix slc mode
		if(page0_info->new_nand_type == 6)
		
		read_page = pagelist_1ynm_hynix256[src%page0_info->pages_in_block] \
				+ (src/page0_info->pages_in_block)*page0_info->pages_in_block;
		else
			read_page = pagelist_hynix256[src%page0_info->pages_in_block] \
				+ (src/page0_info->pages_in_block)*page0_info->pages_in_block;
		}
		else if(page0_info->new_nand_type == 40) {  //for sandisk slc mode
			
			read_page = (src%page0_info->pages_in_block)*2 + (src/page0_info->pages_in_block)*page0_info->pages_in_block;
		}
	}

	retry_cnt = 0;
page_retry:
	ret = nfio_page_read_hwctrl(read_page, mem, oob_buf, ext);
	if (ret == ERROR_NAND_ECC){
		serial_puts("nand read page addr=0x");
		serial_put_hex(read_page,32);
		serial_puts(" ecc err\n");

            // read retry
            if (ret == ERROR_NAND_ECC) {
                if (retry_cnt < nand_retry.max) {
                    retry_cnt++;
                    nfio_read_retry(1);
		    goto page_retry;
                }
            }
	}

	return ret;
}

/***************************************
*init nand pinmux;
*setting nand controller configuration;
*read page0 and get nand info.
***************************************/
STATIC_PREFIX int nf_init(unsigned ext, unsigned *data_size)
{
	int ecc_mode, short_mode, short_size, pages, page_size;
	//unsigned por_cfg = romboot_info->por_cfg;
	unsigned ret = 0;//, fsm;
	unsigned partition_num  = 0;//, rand_mode = 0;
	//unsigned a2h_cmd_flag = 0;
	unsigned nand_dma_mode = DEFAULT_ECC_MODE;
#ifdef NAND_RETRY_ROMCODE
	 unsigned license = 0;
	 unsigned toggle_mode = 0; 
#endif
	 unsigned no_rb = 1;   //default no rb
	 
	 struct nand_page0_cfg_t * cfg = (struct nand_page0_cfg_t *)NAND_TEMP_BUF;
	 
#ifdef NAND_RETRY_ROMCODE
	efuse_read(&license,EFUSE_LICENSE_OFFSET,sizeof(license));

	toggle_mode = (license&0x7) == POR_1ST_NAND_TOG ? 2:0;
	no_rb = license & LICENSE_NAND_USE_RB_PIN ? 0 : 1;
#endif
	    // pull up enable
	setbits_le32(P_PAD_PULL_UP_EN_REG2, 0x84ff);

    // pull direction, dqs pull down
	setbits_le32(P_PAD_PULL_UP_REG2, 0x0400);
	
	//default CE0 is enable
	setbits_le32(P_PERIPHS_PIN_MUX_2, ((0x3ff<<18) | (1<<17)));

#if 0
		// NAND uses crystal, 24 or 25 MHz 0xc110425c
	writel((4<<9) | (1<<8) | 0, P_HHI_NAND_CLK_CNTL);
		
	//set nand hw controller config here
	// Crystal 24 or 25Mhz, clock cycle 40 ns.
	// Nand cycle = 200ns, timing at 3rd clock.
	// Change to dma mode
	writel((7<<0)  	// bus cycle = (4+1)*40 = 200 ns.
		|(7<<5)  	// bus time = 3
	    	|(0<<10) 	// async mode
	    	|(0<<12) 	// disable cmd_start
	    	|(0<<13) 	// disable cmd_auto
	    	|(0<<14) 	// apb_mode set to DMA mode
	   	|(1<<31)	// disable NAND bus gated clock.
	    	, P_NAND_CFG);
#else
	// NAND pll setting to 160MHz
	writel((((0<<9) | (1<<8) | 3)), P_HHI_NAND_CLK_CNTL);
	
	//set nand hw controller config here
	// Crystal 24 or 25Mhz, clock cycle 40 ns.
	// Nand cycle = 200ns, timing at 3rd clock.
	// Change to dma mode
	writel((4<<0)  	// bus cycle = 31.25 ns.
		|(5<<5)  	// bus time = 4
	    	|(0<<10) 	// async mode
	    	|(0<<12) 	// disable cmd_start
	    	|(0<<13) 	// disable cmd_auto
	    	|(0<<14) 	// apb_mode set to DMA mode
	   	|(1<<31)	// disable NAND bus gated clock.
	    	, P_NAND_CFG);	
#endif	
	// reset
	ret = nfio_reset( no_rb );
	if (ret) {
		serial_puts("nand reset failed here\n");
		return ret;
	}

    // read ID    
	nfio_read_id();
	
	nand_retry.no_rb = no_rb;
	
#ifdef NAND_RETRY_ROMCODE
    if ( license & LICENSE_NAND_EXTRA_CMD ) 
        nand_retry.id = NAND_MFR_EFUSE;
        
    // set efuse bit to enable read retry
    if ( license & LICENSE_NAND_ENABLE_RETRY ) {

        switch (nand_retry.id) {
        case NAND_MFR_MICRON :
            nand_retry.max = 8;
            break; 
        case NAND_MFR_TOSHIBA :
             nand_retry.max = 8;
            break; 
        case NAND_MFR_SAMSUNG :
             nand_retry.max = 15;
            break; 
        case NAND_MFR_SANDISK :
             nand_retry.max = 22;
            break;
        case NAND_MFR_HYNIX :
        case NAND_MFR_FUJITSU :	
        case NAND_MFR_NATIONAL :	
        case NAND_MFR_RENESAS :	
        case NAND_MFR_STMICRO :	
        case NAND_MFR_AMD :	
        case NAND_MFR_INTEL :	
             nand_retry.max = 0;
            break; 
        case NAND_MFR_EFUSE :
             nand_retry.max = 1;
            break;
        default :
            serial_puts( "nand init unknown mfr id" );
            //OUT_PUT(nand_retry.id);
             nand_retry.max = 0;
            break;
        }
    }
    else {
        nand_retry.max = 0;
    }
#endif

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
        memset((void*)NAND_TEMP_BUF, 0, 384);
	    flush_dcache_range(NAND_TEMP_BUF, NAND_TEMP_BUF+384);
#endif

	// 4 copies of same data.
	for (partition_num=0; partition_num<4; partition_num++) {

		ret = nfio_page_read((partition_num<<8), NAND_TEMP_BUF, (unsigned char *)NAND_OOB_BUF, nand_dma_mode);

		if (ret == ERROR_NAND_ECC) {
			
			if (nand_retry.id == NAND_MFR_SANDISK) {
				nand_dma_mode |= (1<<24);
				nand_retry.max = 0;
				ret = nfio_page_read((partition_num<<8), NAND_TEMP_BUF, (unsigned char *)NAND_OOB_BUF, nand_dma_mode);
			}
#if 0		//no need since romcode already finish this work
			else if (toggle_mode == 0 && (nand_retry->id == NAND_MFR_TOSHIBA || 
										  nand_retry->id == NAND_MFR_SAMSUNG)) {
				toggle_mode = 2;
				(*P_NAND_CFG) |= (toggle_mode<<10);
				ret = nfio_page_read((partition_num<<8), NAND_TEMP_BUF, (unsigned char *)NAND_OOB_BUF, nand_dma_mode);
			}
#endif			
		}

		if (ret == 0)
			break;
	}

	if(ret == ERROR_NAND_ECC){
		serial_puts("check bootid or init failed here\n");
		return -ERROR_NAND_ECC;
	}

	romboot_info->ext = cfg->ext;
	    // override 
	nand_retry.id = cfg->id;
	nand_retry.max = cfg->max;
	//nand_retry->no_rb = nand_dma_mode>>23&1;

	// nand parameters
	// when ecc_mode==1, page_size is 512 bytes.
	ecc_mode = cfg->ext>>14&7;
	short_mode = cfg->ext>>13&1;
	short_size = cfg->ext>>6&0x7f;
	pages = cfg->ext&0x3f;
	page_size = short_mode ? short_size : (ecc_mode<2 ? 64 : 128); 	// unit: 8 bytes;

	*data_size = page_size * 8 * pages;

	return 0;

}

STATIC_PREFIX void nf_set_pux(unsigned ce)
{
//	if(ce == CE0){		
//		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 25));		
//	}else if (ce == CE1){
//		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 24));	
//	}else if(ce == CE2){
//		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 23));	
//	}else if(ce == CE3){
//		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 22));
//	}
	SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 25));
	SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 24));	
	SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 23));
	SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 22));	
	writel((ce | IDLE | 40), P_NAND_CMD); 
}

STATIC_PREFIX unsigned int  select_chip(int num)
{
	unsigned int ce=CE0;
	int  i ;
	i=num;

	switch (i){
		 case 0:
			ce = CE0;
			break;
		case 1:
			ce = CE1;
			break;
		case 2:
			ce = CE2;
			break;
		case 3:
			ce = CE3;
			break;
		default :
			serial_puts("nand chip_num =0x");
		        	serial_put_hex(i,32);
		        	serial_puts("nand select chip err here\n");		
			break;
	}
	
	nf_set_pux(ce);

	return ce;
}

STATIC_PREFIX  int index_of_valid_ceNum(int chipnum)
{
	int i,ce_num;

	unsigned int ce_mask;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));

	ce_mask = page0_info->ce_mask;
	ce_num = chipnum;

	if(ce_num > 4)
		return	0xff;

	for(i=0;i<MAX_CE_NUM_SUPPORT;i++){
		if(ce_mask&(0x01<<ce_num))
			break;
		else
			ce_num++;
	}
	return ce_num;
}


STATIC_PREFIX void send_plane0_cmd(unsigned page, unsigned ext,unsigned ce)
{
	int  /*chip_num,*/ plane_mode;
	unsigned int /*nand_read_info,*/ ran_mode;
	unsigned char micron_nand_flag=0;
	unsigned page_in_blk, blk_num, /*pages_in_block,*/ plane0, plane1;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));

	//page0_info->nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	plane_mode = (page0_info->nand_read_info >> 2) & 0x1;
	ran_mode = (page0_info->nand_read_info >> 3) & 0x1;
	micron_nand_flag = (page0_info->nand_read_info >> 4) & 0x1;
	//pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));

	blk_num = page /page0_info->pages_in_block;
	page_in_blk = page - blk_num * page0_info->pages_in_block;
	blk_num <<= 1;
	plane0 = blk_num *page0_info->pages_in_block + page_in_blk;
	plane1 = (blk_num + 1) *page0_info->pages_in_block + page_in_blk;

#if 0
	serial_puts("nand send_plane0_cmd page=0x");
	serial_put_hex(page,32);
	serial_puts("\n");
	
	serial_puts("nand send_plane0_cmd page_in_blk=0x");
	serial_put_hex(page_in_blk,32);
	serial_puts("\n");
	
	serial_puts("nand send_plane0_cmd blk_num=0x");
	serial_put_hex(blk_num,32);
	serial_puts("\n");
	
	serial_puts("nand send_plane0_cmd plane0=0x");
	serial_put_hex(plane0,32);
	serial_puts("\n");
	
	serial_puts("nand send_plane0_cmd plane1=0x");
	serial_put_hex(plane1,32);
	serial_puts("\n");
#endif

	if(micron_nand_flag){
		// plane 0
		writel(ce | IDLE, P_NAND_CMD);
		writel(ce | CLE  | 0, P_NAND_CMD);
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
		  	writel(ce | ALE  | 0,P_NAND_CMD);
		}
		
		writel(ce | ALE  | ((plane0)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane0>>8)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);   
		writel(ce | ALE  | ((plane0>>16)&0xff), P_NAND_CMD);
		
		//plane 1
		writel(ce | CLE  | 0, P_NAND_CMD);
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
		  	writel(ce | ALE  | 0,P_NAND_CMD);
		}
		
		writel(ce | ALE  | ((plane1)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>8)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>16)&0xff), P_NAND_CMD);

		if((ext>>22)&1){
			writel(ce | CLE  | 0x30, P_NAND_CMD);
		}

		writel((ce | IDLE | 40), P_NAND_CMD);    // tWB, 8us
		//read plane 0 
	
	}else{
		// plane 0
		writel(ce | IDLE, P_NAND_CMD);
		writel(ce | CLE  | 0x60, P_NAND_CMD);
	
		writel(ce | ALE  | ((plane0)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane0>>8)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);
		writel(ce | ALE  | ((plane0>>16)&0xff), P_NAND_CMD);
		
		writel((ce | IDLE ), P_NAND_CMD);           //send dummy cmd idle
		
		//plane 1
		writel(ce | CLE  | 0x60, P_NAND_CMD);
		
		writel(ce | ALE  | ((plane1)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>8)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>16)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);

		if((ext>>22)&1){
			writel(ce | CLE  | 0x30, P_NAND_CMD);
		}

		writel((ce | IDLE | 40), P_NAND_CMD);    // tWB, 8us

		writel(ce | CLE  | 0, P_NAND_CMD);
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
		  	writel(ce | ALE  | 0,P_NAND_CMD);
		}

		writel(ce | ALE  | ((plane0)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane0>>8)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane0>>16)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);

		writel(ce | CLE  | 0x05, P_NAND_CMD);
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
		  	writel(ce | ALE  | 0,P_NAND_CMD);
		}
		
		writel(ce | CLE  | 0xE0, P_NAND_CMD);
		
		writel((ce | IDLE | 40), P_NAND_CMD);    // tWB, 8us
		//read plane 0 data
	
	}
	// set Timer 13 ms.
	if(ext>>23&1) { // without RB
		//serial_puts("with out rb \n");
		writel((ce | CLE | 0x70), P_NAND_CMD);  //read status
		writel((ce | IDLE | 2), P_NAND_CMD);    // tWB
		writel((IO6  | RB | 16), P_NAND_CMD);     //wait IO6
		writel((ce | IDLE | 2), P_NAND_CMD);    // dummy
		writel(ce | CLE  | 0, P_NAND_CMD);      //chage to page read mode
		//        (*P_NAND_CMD) = CE0 | CLE | 0;
		writel((ce | IDLE | 2), P_NAND_CMD);    // dummy
	}else{ // with RB
	    	writel((ce | RB | 16), P_NAND_CMD);     //wait R/B
	}

	//writel(SEED | (0xc2 + (src&0xff)), P_NAND_CMD);
	writel((ext&0x3fffff), P_NAND_CMD);

	writel((ce | IDLE ), P_NAND_CMD);           //send dummy cmd idle
	writel((ce | IDLE ), P_NAND_CMD);           //send dummy cmd idle

}

STATIC_PREFIX void send_plane1_cmd(unsigned page, unsigned ext,unsigned ce)
{
	int  /*chip_num,*/ plane_mode;
	//unsigned int nand_read_info;
	unsigned char micron_nand_flag=0;
	unsigned page_in_blk, blk_num, /*pages_in_block,*/ plane0, plane1;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));

	//nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	plane_mode = (page0_info->nand_read_info >> 2) & 0x1;
	micron_nand_flag = (page0_info->nand_read_info >> 4) & 0x1;
	//pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));

	blk_num = page /page0_info->pages_in_block;
	page_in_blk = page - blk_num * page0_info->pages_in_block;
	blk_num <<= 1;
	plane0 = blk_num *page0_info->pages_in_block + page_in_blk;
	plane1 = (blk_num + 1) *page0_info->pages_in_block + page_in_blk;

	if(micron_nand_flag){
		
		writel(ce | IDLE, P_NAND_CMD);
		writel(ce | CLE  | 0x06, P_NAND_CMD);
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
			writel(ce | ALE  | 0,P_NAND_CMD);
		}
		
		writel(ce | ALE  | ((plane1)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>8)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>16)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);
		//read plane 1
	
	}else{

		writel(ce | IDLE, P_NAND_CMD);
		writel(ce | CLE  | 0, P_NAND_CMD);
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
			writel(ce | ALE  | 0,P_NAND_CMD);
		}
		
		writel(ce | ALE  | ((plane1)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>8)&0xff), P_NAND_CMD);
		writel(ce | ALE  | ((plane1>>16)&0xff), P_NAND_CMD);
		//writel(ce | ALE  | 0, P_NAND_CMD);
		
		writel(ce | CLE  | 0x05, P_NAND_CMD);
		
		writel(ce | ALE  | 0, P_NAND_CMD);
		if((ext>>22)&1){
			writel(ce | ALE  | 0,P_NAND_CMD);
		}
		//read plane 1

	}

	writel(ce | CLE  | 0xE0, P_NAND_CMD);

	writel((ce | IDLE | 40), P_NAND_CMD);	 // tWB, 8us

	//writel(SEED | (0xc2 + (src&0xff)), P_NAND_CMD);
	writel((ext&0x3fffff), P_NAND_CMD);

	writel((ce | IDLE ), P_NAND_CMD);			//send dummy cmd idle
	writel((ce | IDLE ), P_NAND_CMD);			//send dummy cmd idle

}

STATIC_PREFIX void send_read_cmd(unsigned src, unsigned ext,unsigned ce)
{
	int  /*chip_num,*/ plane_mode;
	//unsigned int nand_read_info;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));

	//nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	plane_mode = (page0_info->nand_read_info >> 2) & 0x1;

	//add A2H command for SLC read
    	if((ext>>24)&1){
		writel(ce | CLE  | 0xa2, P_NAND_CMD);
		writel((ce | IDLE | 2), P_NAND_CMD);
    	}

	// send cmds read 00 ... 30
	writel(ce | IDLE, P_NAND_CMD);
	writel(ce | CLE  | 0, P_NAND_CMD);
	writel(ce | ALE  | 0, P_NAND_CMD);
	if((ext>>22)&1){
	  	writel(ce | ALE  | 0,P_NAND_CMD);
	   //     (*P_NAND_CMD) = CE0 | ALE  | 0;
	}
	writel(ce | ALE  | ((src)&0xff), P_NAND_CMD);
	writel(ce | ALE  | ((src>>8)&0xff), P_NAND_CMD);
	writel(ce | ALE  | ((src>>16)&0xff), P_NAND_CMD);
	//writel(ce | ALE  | 0, P_NAND_CMD);
	//writel(ce | ALE  | ((src>>16)&0xff), P_NAND_CMD);
	
	if((ext>>22)&1){
		writel(ce | CLE  | 0x30, P_NAND_CMD);
	}

	writel((ce | IDLE | 40), P_NAND_CMD);    // tWB, 8us
	// set Timer 13 ms.
	if(ext>>23&1) { // without RB
		//serial_puts("with out rb \n");
		writel((ce | CLE | 0x70), P_NAND_CMD);  //read status
		writel((ce | IDLE | 2), P_NAND_CMD);    // tWB
		writel((IO6  | RB | 16), P_NAND_CMD);     //wait IO6
		writel((ce | IDLE | 2), P_NAND_CMD);    // dummy
		writel(ce | CLE  | 0, P_NAND_CMD);      //chage to page read mode
		//        (*P_NAND_CMD) = CE0 | CLE | 0;
		writel((ce | IDLE | 2), P_NAND_CMD);    // dummy
	}else{ // with RB
	    	writel((ce | RB | 16), P_NAND_CMD);     //wait R/B
	}

	// scramble, first page is disabled by default.
	// code page is enabled/disabled by ext[19].
	// 0xa3 + (src&0xff) is used as seed,
	writel(SEED|(0xc2 + (src&0x7fff)), P_NAND_CMD);
	writel((ext&0x3fffff), P_NAND_CMD);

	//    (*P_NAND_CMD) = (ext&0x3fffff);
	writel((ce | IDLE ), P_NAND_CMD);           //send dummy cmd idle
	writel((ce | IDLE ), P_NAND_CMD);           //send dummy cmd idle

}
STATIC_PREFIX void send_reset_cmd(unsigned ext)
{
	unsigned int i, chip_num, ce_mask;
	unsigned int ce;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));
	
	//nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	//chip_num  = page0_info->nand_read_info & 0x3;
		
	chip_num  = MAX_CE_NUM_SUPPORT;
	ce_mask = page0_info->ce_mask;
	
	for(i=0;i<chip_num;i++){ //send reset cmd 0xff

		if(ce_mask&(0x01<<i)){
		ce = select_chip(i);
		writel((ce | CLE | 0xff), P_NAND_CMD);	//reset
		writel((ce | IDLE | 2), P_NAND_CMD);	// dummy
		// set Timer 13 ms.
		if(ext>>23&1) { // without RB/
			serial_puts("with out rb \n");
			writel((ce | CLE | 0x70), P_NAND_CMD);	//read status
			writel((ce | IDLE | 2), P_NAND_CMD);	// tWB
			writel((IO6 | RB | 16), P_NAND_CMD);	 //wait IO6
			writel((ce | IDLE | 2), P_NAND_CMD);	// dummy
			writel(ce | CLE  | 0, P_NAND_CMD);		//chage to page read mode
			//		  (*P_NAND_CMD) = CE0 | CLE | 0;
			writel((ce | IDLE | 2), P_NAND_CMD);	// dummy
		}else{ // with RB
			writel((ce | RB | 16), P_NAND_CMD); 	//wait R/B
		}

		while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
				serial_puts("reset_cmd \n");
		}
	}

}

STATIC_PREFIX unsigned nf_read_check(volatile unsigned long long *info_buf, unsigned char * oob_buf,unsigned ext, unsigned ce )
{	
	int pages, k, ret=0, ecc_mode,extra_len=0,newoobtype=0;

	pages = ext&0x3f;
	ecc_mode = ext>>14&7;
	
	if((ecc_mode >= 2)&&(pages >=8)){
		//serial_puts("new oob here\n");
		extra_len = 2;
		newoobtype =1;
	}
	else{
		newoobtype =0;
		extra_len = 0;
	}
	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
		do{ 
			flush_dcache_range((unsigned long)info_buf,(unsigned long)info_buf+(pages +extra_len )*PER_INFO_BYTE);
			//invalidate_dcache_range((unsigned long)info_buf,(unsigned long)info_buf+(pages +extra_len )*PER_INFO_BYTE); 
		}

#endif //defined(CONFIG_AML_SPL_L1_CACHE_ON)
		while(info_buf[pages + extra_len -1]==0);
	
	//while(info_buf[pages-1]==0);

	for (k=extra_len; k<pages + extra_len; k++){

	    	while(info_buf[k]==0);

		if((info_buf[k]>>16&0x3f) < 0xa){
			ret = ERROR_NAND_BLANK_PAGE;
		}

	    	if ((info_buf[k]>>24&0x3f) == 0x3f) {// uncorrectable

			if((ext>>24)&1){ //if with a2h cmd, just need reset
				writel((ce | CLE | 0xff), P_NAND_CMD);  //reset
				writel((ce | IDLE | 2), P_NAND_CMD);    // dummy

				// set Timer 13 ms.
				if(ext>>23&1) { // without RB/
					writel((ce | CLE | 0x70), P_NAND_CMD);  //read status
					writel((ce | IDLE | 2), P_NAND_CMD);    // tWB
					writel((IO6 | RB | 16), P_NAND_CMD);     //wait IO6
					writel((ce | IDLE | 2), P_NAND_CMD);    // dummy
					writel(ce | CLE  | 0, P_NAND_CMD);      //chage to page read mode
					//        (*P_NAND_CMD) = CE0 | CLE | 0;
					writel((ce | IDLE | 2), P_NAND_CMD);    // dummy
				}else{ // with RB
				    	writel((ce | RB | 16), P_NAND_CMD);     //wait R/B
				}
				while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

			}

			if(ret != ERROR_NAND_BLANK_PAGE){ //if randomization, check 0xff page, zero_cnt less than 10
				ret = ERROR_NAND_ECC;
			}
			break;
	    	}

		if(ret)
			break;
		if(newoobtype != 1){

			oob_buf[0] = (info_buf[k]&0xff);
			oob_buf[1] = ((info_buf[k]>>8)&0xff);

			oob_buf += 2;
		}
	    	
	}
	if(newoobtype == 1){
		for(k=0; k<8; k++){
			oob_buf[k] =((info_buf[0]>> 8*k)&0xff);
			oob_buf[k + 8] =((info_buf[1]>> 8*k)&0xff);			
		}
	}
	
	return ret;
}

STATIC_PREFIX short nf_normal_read_page_hwctrl(unsigned page,unsigned  mem, unsigned char *oob_buf, unsigned ext, unsigned data_size, int chipnr)
{
	int i, k, chip_num, plane_mode=0, ecc_mode, newoobtype = 0,extra_len=0;
	unsigned long info_adr = NAND_INFO_BUF;
	volatile unsigned long long * info_buf=(volatile unsigned long long *)NAND_INFO_BUF;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));
	//unsigned int nand_read_info;
	int ce, pages, ret=0;
	
	//nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	unsigned int ce_mask;
	ce_mask = page0_info->ce_mask;
	//chip_num = page0_info->nand_read_info & 0x3;
	chip_num  = MAX_CE_NUM_SUPPORT;
	//serial_puts("nf_normal_read_page_hwctrl : chip_num\n");
	//serial_put_hex(chip_num,32);
	//serial_puts("\n");

	//serial_puts("nf_normal_read_page_hwctrl : ce_mask\n");
	//serial_put_hex(ce_mask,32);
	//serial_puts("\n");
	//plane_mode = (nand_read_info >> 2) & 0x1;
	pages = ext&0x3f;
	ecc_mode = ext>>14&7;
	
	if((ecc_mode >= 2)&&(pages >=8)){
		//serial_puts("new oob here\n");
		newoobtype =1;
		extra_len = 2;
	}
	else{
		extra_len = 0;
		newoobtype =0;
	}
	writel(info_adr, P_NAND_IADR);
	for(k=0;k<pages +extra_len;k++){
		info_buf[k]=0;
	}
	
#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
	flush_dcache_range((unsigned long)info_buf,(unsigned long)info_buf+(pages +extra_len )*PER_INFO_BYTE);
	invalidate_dcache_range(mem, mem+ 128* 8 * pages);
#endif  	

	for(i=0;i<chip_num;i++){
		if(ce_mask&(0x01<<chipnr))
			break;
		else
			chipnr++;
	}
		
		while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
		writel(mem, P_NAND_DADR);
				
		ce = select_chip(chipnr);
		if(plane_mode == 1){  // 1 represent two_plane_mode

			send_plane0_cmd(page, ext, ce);
			ret = nf_read_check(info_buf, oob_buf, ext,ce);
			if (ret == ERROR_NAND_ECC){
				serial_puts("nand read addr=0x");
				serial_put_hex(page,32);
				serial_puts("nfio_read read ecc err: plane0 \n");
			}		
			mem += data_size;
			oob_buf += pages *2;	
			writel(mem, P_NAND_DADR);
			send_plane1_cmd(page, ext, ce);
			ret = nf_read_check(info_buf, oob_buf, ext,ce);
			if (ret == ERROR_NAND_ECC){
				serial_puts("nand read addr=0x");
				serial_put_hex(page,32);
				serial_puts("nfio_read read  ecc err: plane1\n");
			}
			
		}else{

			send_read_cmd(page, ext, ce);		
			ret = nf_read_check(info_buf, oob_buf, ext,ce);			
			if (ret == ERROR_NAND_ECC){
				serial_puts("nand read addr=0x");
				serial_put_hex(page,32);
				serial_puts("nfio_read read err here\n");
			}
		}
		mem += data_size;
		oob_buf += pages *2;


	return ret;
}

STATIC_PREFIX int nf_normal_read_page(unsigned page, unsigned  mem, unsigned char *oob_buf, unsigned ext, unsigned data_size, unsigned chipnr)
{
	unsigned read_page/*, new_nand_type, pages_in_block*/;
	//unsigned chip_num, plane_mode, ram_mode;
	int /*i,retry_cnt,*/ ret = 0;
	//struct nand_page0_info_t *page0_info = (NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t);

	//new_nand_type  = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int) + sizeof(int));
	//page0_info->pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));
	
	read_page = page;
	
	ret = nf_normal_read_page_hwctrl(read_page, mem, oob_buf, ext, data_size, chipnr);
	if (ret == ERROR_NAND_ECC){
		serial_puts("nand read addr=0x");
		serial_put_hex(read_page,32);
		serial_puts("nfio_read read err here\n");
	}
	
	return ret;
}
#ifdef CONFIG_SECURE_NAND
STATIC_PREFIX void nf_read_secure(unsigned target);
#endif
//for uboot
STATIC_PREFIX short nf_read(unsigned target, unsigned size)
{
	unsigned ext = romboot_info->ext;
	unsigned int read_size, data_size;
	int /*retry_cnt, read_page, tmp_page,*/ page_base;
	unsigned char *oob_buf = (unsigned char *)NAND_OOB_BUF;
	unsigned mem, count, pages;
	int /*pages_in_block,*/ total_page = 1024;
    unsigned int next_copy_flag =0;
	int i, k, ret = 0;
#ifdef 	CONFIG_SECURE_NAND
	unsigned char *psecure;
#endif	

	// check the boot area
	// If enter this function
	// we the boot_id should be 0
	// and init should be Okay
	if(romboot_info->boot_id ||romboot_info->init[0]){
		serial_puts("check bootid or init failed here\n");
        	return -ERROR_MAGIC_WORD_ERROR;
	}

	data_size = 0;
	pages = (ext&0x3f);

	//init hw config and get nand info from page0
	ret = nf_init(ext, &data_size);
	if(ret){
		serial_puts("nfio init failed here\n");
		return ret;
	}

    page_base = 1+ (READ_SIZE/data_size);

    mem = target;
	mem -= READ_SIZE%data_size;
	count = size + READ_SIZE%data_size;

    //pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));

    ret = 0;
    for(i=0,read_size=0; i<total_page && read_size<count; i++, read_size+=data_size){

	        ret = nfio_page_read(page_base+i, mem + read_size, oob_buf, ext);

            for(k=0; k<pages; k++){
				if(((*oob_buf&0xff) != 0x55) && ((((*(oob_buf+1)&0xff) != 0xaa)))){
					ret = ERROR_NAND_MAGIC_WORD;
				}
				oob_buf +=2;
			}
			if((ret == 0) && (next_copy_flag == 1)){
                page_base -= M3_BOOT_PAGES_PER_COPY;
                next_copy_flag = 0;
			}
            if((ret == ERROR_NAND_ECC) || (ret == ERROR_NAND_MAGIC_WORD)){
                page_base += M3_BOOT_PAGES_PER_COPY;
                read_size -= data_size;
                i--;
	        	next_copy_flag = 1;
	        	if(page_base > total_page)
                        break;
            }

#if 0//defined(CONFIG_AML_SPL_L1_CACHE_ON)
        invalidate_dcache_range(read_size+target,read_size+target+data_size);
#endif  
            
    }
#ifdef CONFIG_SECURE_NAND
	//load os_key	
	psecure = (unsigned char*)NAND_SECURE_BUF;
	//extern void * memset(void * s,char c,size_t count);
	memset((void*)psecure, 0, 0x100);
	nf_read_secure(target);
#endif
	return ret;
}

//target & size for Secure OS
//Secure key buf: NAND_TEMP_BUF
#ifdef CONFIG_SECURE_NAND
STATIC_PREFIX void nf_read_secure(unsigned target)
{
	unsigned ext = romboot_info->ext;
	unsigned pages_in_block, data_size /*, read_size, page_base*/;
	unsigned ok_flag, read_page,tmp_page, key_addr, sec_pages, pages, reg_v32, ecc_mode,newoobtype=0;
	unsigned  data_buf = NAND_DATA_BUF;
	unsigned char *oob_buf = (unsigned char *)NAND_OOB_BUF;
	int i, j, times, times_tamp=0, ret = 0;
   	unsigned int magic_name,plane,chip_num /*,nand_read_info,ce*/;
	unsigned int secure_blk,chipnr, tmp_blk, start_blk = 0;
	struct nand_page0_info_t *page0_info = (struct nand_page0_info_t *)((NAND_TEMP_BUF+384)-sizeof(struct nand_page0_info_t));	
	
	plane = 1;
	data_size = 0;
	pages = (ext&0x3f);
	serial_put_hex(pages,32);
	serial_puts("\n");
	ecc_mode = (ext>>14)&7;
	
	if((ecc_mode >= 2)&&(pages >=8)){
		//serial_puts("new oob here\n");
		newoobtype =1;
	}
	//init hw config and get nand info from page0
	ret = nf_init(ext, &data_size);
	if(ret){
		serial_puts("nfio init failed here\n");
		//return ret;
		return;
	}

	#define AMLNF_WRITE_REG(reg, val)			*(volatile unsigned *)(reg) = (val)
	#define AMLNF_READ_REG(reg) 				*(volatile unsigned *)(reg)	
	
	if(newoobtype == 1) {
		reg_v32=AMLNF_READ_REG(P_NAND_CFG);
		reg_v32 |= 3<<26;
		AMLNF_WRITE_REG(P_NAND_CFG, reg_v32);
	}	

	//pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));
	pages_in_block = page0_info->pages_in_block;
	chip_num  = page0_info->nand_read_info & 0x3;

	
#if 0	
	serial_puts("nfio data_size = 0x \n");
	serial_put_hex(data_size,32);
	serial_puts("\n");

	serial_puts("nfio chip_num = 0x \n");
	serial_put_hex(chip_num,32);
	serial_puts("\n");

	serial_puts("nfio pages_in_block = 0x \n");
	serial_put_hex(pages_in_block,32);
	serial_puts("\n");
#endif

	   setbits_le32(P_PERIPHS_PIN_MUX_2, ((0x3ff<<18) | (1<<17)));
	send_reset_cmd(ext);
	
	ok_flag = 0;
	key_addr = 0;
	//read_page = 1024 *data_size; //search from offset page
	tmp_blk = start_blk = 4;
	
	serial_puts("nf_read_secure  step 1\n");
	for(; start_blk<NAND_SEC_MAX_BLK_NUM; start_blk++){

		tmp_page =(((start_blk - start_blk % chip_num) /chip_num) + tmp_blk -tmp_blk/chip_num) * pages_in_block;
		chipnr = start_blk % chip_num;
		i=0;
		ret = nf_normal_read_page(tmp_page, (data_buf+ i*(data_size)), oob_buf, ext, data_size,chipnr);
		if(ret == ERROR_NAND_BLANK_PAGE) {
			continue;
		}

		if(ret == 0){  //read sucess, check oob magic

			times  = *(oob_buf+4);
			magic_name = ((*(oob_buf)&0xff) | ((*(oob_buf + 1)&0xff) << 8) | ((*(oob_buf + 2)&0xff) << 16) | ((*(oob_buf + 3)&0xff) << 24));
			times = ( (*(oob_buf+4)&0xff) | ((*(oob_buf+5)&0xff) << 8)); /*| ((*(oob_buf+6)&0xff) << 16) | ((*(oob_buf+7)&0xff) << 24));*/

			if (magic_name == SECURE_HEAD_MAGIC){
#if  0
				serial_puts("nf_read_secure secure key here@@\n");
				serial_puts("nfio magic_name = 0x \n");
				serial_put_hex(magic_name,32);
				serial_puts("\n");

				serial_puts("nfio times = 0x \n");
				serial_put_hex(times,32);
				serial_puts("\n");
#endif
				if(times >= times_tamp){
				times_tamp = times;
				ok_flag = 1;
				key_addr = start_blk;   			
				}
				/*if((i > NAND_SECURE_BLK)&&(ok_flag == 1)) {
						 break;
				}*/
			}else {
				ret = ERROR_NAND_MAGIC_WORD;
			}
		}
	}

	secure_blk = key_addr;
	serial_puts("nf_read_secure: secure_blk= 0x \n");
	serial_put_hex(secure_blk,32);
	serial_puts("\n");
	
	key_addr = 0;
	sec_pages = (CONFIG_SECURE_SIZE/data_size)+1;

	serial_puts("nf_read_secure:  sec_pages = 0x");
	serial_put_hex(sec_pages,32);
	serial_puts("\n");
	
	serial_puts("nf_read_secure:  step 2\n");
	//find page num here;
	if(ok_flag == 1){
		
		ok_flag = 0;
		times_tamp = 0;
		times = 0;
		magic_name = 0;
		
		for(i=0; i<pages_in_block; i+=sec_pages){


			tmp_page =(i+(((secure_blk - secure_blk % chip_num) /chip_num) + tmp_blk -tmp_blk/chip_num) * pages_in_block);
			chipnr = secure_blk %chip_num; 
			ret = nf_normal_read_page(tmp_page, (data_buf+ i*data_size), oob_buf, ext, data_size,chipnr);
			if(ret == ERROR_NAND_BLANK_PAGE){
				break;
			}

			times = ( (*(oob_buf+4)&0xff) | ((*(oob_buf+5)&0xff) << 8));// | ((*(oob_buf+6)&0xff) << 16) | ((*(oob_buf+7)&0xff) << 24));
			magic_name = ((*(oob_buf)&0xff) | ((*(oob_buf + 1)&0xff) << 8) | ((*(oob_buf + 2)&0xff) << 16) | ((*(oob_buf + 3)&0xff) << 24));
			if(ret == 0){  //read sucess, check oob magic
				if(magic_name == SECURE_HEAD_MAGIC){
#if  0
					serial_puts("nf_read_secure secure key here####\n");
					serial_puts("nfio magic_name = 0x \n");
					serial_put_hex(magic_name,32);
					serial_puts("\n");
	
					serial_puts("nfio times = 0x \n");
					serial_put_hex(times,32);
					serial_puts("\n");
#endif

					if(times >= times_tamp){
						times_tamp = times;
						ok_flag = 1;
						key_addr = tmp_page;
					}
				}else{
					ret = ERROR_NAND_MAGIC_WORD;
				}
			}
		}
	}
	else{
		serial_puts("foun no secure key here\n");
		ret = ERROR_NAND_MAGIC_WORD;
		goto error_exit;
	}
	
	ret = 0;
	read_page = key_addr;
	serial_puts("nf_read_secure:  step 3 chipnr = 0x");
	serial_put_hex(chipnr,32);
	serial_puts("\n");

      	serial_puts("nf_read_secure:  step 3 result block  = 0x");
	serial_put_hex(secure_blk,32);
	serial_puts("\n");
	
	serial_puts("nf_read_secure:  step 3 read_page = 0x");
	serial_put_hex(read_page,32);
	serial_puts("\n");
		
	//read secure key here;
	if(ok_flag == 1){
		
		for(j=0; j<sec_pages;j++){
			tmp_page =read_page + j;
			chipnr =secure_blk % chip_num;		
			ret = nf_normal_read_page(tmp_page, (NAND_SECURE_BUF+ j*data_size), oob_buf, ext, data_size,chipnr);
			if(ret == ERROR_NAND_ECC){
				serial_puts("secure key read failed here\n");
				break;
			}
		}
		if(ret != ERROR_NAND_ECC)
            	serial_puts("secure key read sucess here\n");
	}
	else{
		serial_puts("found no secure key here\n");
		goto error_exit;
	}

error_exit:
	if(newoobtype == 1) {	
		reg_v32=AMLNF_READ_REG(P_NAND_CFG);
		reg_v32 &= ~(3<<26);
		AMLNF_WRITE_REG(P_NAND_CFG, reg_v32);
	}
	return;
}
#endif
