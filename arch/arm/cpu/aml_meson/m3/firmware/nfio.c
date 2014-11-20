#include <asm/arch/reg_addr.h>
#include <asm/arch/timing.h>

#include <memtest.h>
#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/nand.h>
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
#define NAND_INFO_BUF 0x80000000
#define NAND_TEMP_BUF 0x82000000
#define DEFAULT_ECC_MODE  ((2<<20)|(1<<17)|(7<<14)|(1<<13)|(48<<6)|1)

#if 1
STATIC_PREFIX short nfio_page_read(unsigned src,unsigned mem,unsigned ext)
{
    int i, k, ecc_mode, short_mode, short_size, pages, page_size;
    unsigned long info_adr = NAND_INFO_BUF;
    volatile unsigned long long * info_buf=(volatile unsigned long long *)NAND_INFO_BUF;


    // nand parameters
    ecc_mode = ext>>14&7;
    short_mode = ext>>13&1;
    short_size = ext>>6&0x7f;
    pages = ext&0x3f;
    page_size = short_mode ? short_size :
	ecc_mode<2 ? 64 : 128; // unit: 8 bytes;
    for(i=0;i<pages;i++)
    {
        info_buf[i]=0;
    }
    while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

    // set data/info address 
    writel(info_adr,P_NAND_IADR);
    writel(mem,     P_NAND_DADR);

    // send cmds read 00 ... 30
    writel(CE0 | IDLE,P_NAND_CMD);
    writel(CE0 | CLE  | 0,P_NAND_CMD);
    writel(CE0 | ALE  | 0,P_NAND_CMD);
    if((ext>>22)&1){
        writel(CE0 | ALE  | 0,P_NAND_CMD);
//        (*P_NAND_CMD) = CE0 | ALE  | 0;
    }
    writel(CE0 | ALE  | ((src)&0xff),P_NAND_CMD);
    writel(CE0 | ALE  | ((src>>8)&0xff),P_NAND_CMD);
    writel(CE0 | ALE  | 0,P_NAND_CMD);
    if((ext>>22)&1){
        writel(CE0 | CLE  | 0x30,P_NAND_CMD);
    }
    writel((CE0 | IDLE | 40),P_NAND_CMD);    // tWB, 8us
    // set Timer 13 ms.
    if(ext>>23&1) { // without RB
        writel((CE0 | CLE | 0x70),P_NAND_CMD);  //read status
        writel((CE0 | IDLE | 2),P_NAND_CMD);    // tWB
        writel((IO6 | RB | 16),P_NAND_CMD);     //wait IO6
        writel((CE0 | IDLE | 2),P_NAND_CMD);    // dummy
        writel(CE0 | CLE  | 0,P_NAND_CMD);      //chage to page read mode
//        (*P_NAND_CMD) = CE0 | CLE | 0;
	    writel((CE0 | IDLE | 2),P_NAND_CMD);    // dummy
    }else{ // with RB
        writel((CE0 | RB | 16),P_NAND_CMD);     //wait R/B
    }

    // scramble, first page is disabled by default.
    // code page is enabled/disabled by ext[19].
    // 0xa3 + (src&0xff) is used as seed,
    writel(SEED | (0xa3 + (src&0xff)),P_NAND_CMD);
    writel((ext&0x3fffff),P_NAND_CMD);
//    (*P_NAND_CMD) = (ext&0x3fffff);
    writel((CE0 | IDLE ),P_NAND_CMD);           //send dummy cmd idle
    writel((CE0 | IDLE ),P_NAND_CMD);           //send dummy cmd idle

#if 0
    // dma by apb, check timer
    for (k=0; k<pages; k++) { // page
	for (i=0; i<page_size+1; i++) { // data + info
	    while (((*P_NAND_DC)>>11&1) == 0) {
		tmp = (*P_NAND_CMD);
		if (tmp>>27&1) return ERROR_NAND_TIMEOUT;
	    };

	    p_mem = (unsigned long *)(*P_NAND_ADR);
	    *(p_mem++)=(*P_NAND_DL);
	    *(p_mem++)=(*P_NAND_DH);
	}
    }

    // check info
    for (k=0; k<pages; k++) {
	    tmp = *(volatile long *)info_adr;

	    if ((tmp>>24&0x3f) == 0x3f) // uncorrectable
	        return ERROR_NAND_ECC;

    	if ((tmp&0xc000ffff)!=0xc000aa55) //magic word error
	        return ERROR_NAND_MAGIC_WORD;

	info_adr += 8;
    }
#else
    while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
    while(info_buf[pages-1]==0);
    for (k=0; k<pages; k++) 
    {
        while(info_buf[k]==0);
        if ((info_buf[k]>>24&0x3f) == 0x3f) // uncorrectable
	        return ERROR_NAND_ECC;
	    if ((info_buf[k]&0xc000ffff)!=0xc000aa55) //magic word error
	        return ERROR_NAND_MAGIC_WORD;
    }
    
#endif    

    return ERROR_NONE;
}
#endif
STATIC_PREFIX short nfio_read(unsigned src,unsigned mem,unsigned count,unsigned ext,unsigned area)
{
    int i, ecc_mode, short_mode, short_size, pages, page_size,pages_in_block;
    int ret, data_size, page_base, read_size;
	int total_page = 1024;
    // nand parameters
    // when ecc_mode==1, page_size is 512 bytes.
    ecc_mode = ext>>14&7;
    short_mode = ext>>13&1;
    short_size = ext>>6&0x7f;
    pages = ext&0x3f;
    page_size = short_mode ? short_size :
	    ecc_mode<2 ? 64 : 128; // unit: 8 bytes;
	
	pages_in_block = *(volatile int *)(NAND_TEMP_BUF+sizeof(int));
	
	serial_puts("pages_in_block=0x");
	serial_put_hex(pages_in_block,32);
	serial_puts("\n");
	
    data_size = page_size * 8 * pages;
    
    page_base = ((area<<8)+1)+(src/data_size);
    mem-=src%data_size;
    count+=src%data_size;

    for(i=0,read_size=0; i<total_page && read_size<count; i++, read_size+=data_size)
    {
       // do{
       		ret = nfio_page_read(i+page_base, mem+read_size, ext);
	        if (ret) 
	        {	        	
	        	page_base += pages_in_block;
	        	i--;
	        	total_page -= pages_in_block;
	        }
	    //}while(ret!=ERROR_NONE);
    }

    return 0;// ret;
}
STATIC_PREFIX short 
 nf_read(unsigned target, unsigned size)
{
    unsigned por_cfg=romboot_info->por_cfg;
    unsigned ext=romboot_info->ext;
    //unsigned area;
    unsigned temp_addr = NAND_TEMP_BUF;
    
    
    switch(POR_GET_1ST_CFG(por_cfg))
    {
        case POR_1ST_NAND:
            setbits_le32(P_PERIPHS_PIN_MUX_2,(0x3ff<<18));
            break;
        case POR_1ST_NAND_RB:
//            *P_PERIPHS_PIN_MUX_6 &= ~(0x3<<24);     // Clear SD_C_CMD&CLK
//            *P_PERIPHS_PIN_MUX_2 &= ~(0x3<<22);     // Clear NAND_CS2&CS3
            clrbits_le32(P_PERIPHS_PIN_MUX_4,3<<24);
            clrsetbits_le32(P_PERIPHS_PIN_MUX_2,(0xf3f<<16)|(3<<22),(0xf3f<<16));
            break;
    }
    // Crystal 24 or 25Mhz, clock cycle 40 ns.
    // Nand cycle = 200ns, timing at 3rd clock.
    // Change to dma mode 
    writel((7<<0)  // bus cycle = (4+1)*40 = 200 ns.
        |(7<<5)  // bus time = 3
        |(0<<10) // async mode
        |(0<<12) // disable cmd_start
        |(0<<13) // disable cmd_auto
        |(0<<14) // apb_mode set to DMA mode
	    |(1<<31)// disable NAND bus gated clock.
	    ,P_NAND_CFG);
    // check the boot area 
    // If enter this function 
    // we the boot_id should be 0
    // and init should be Okay 
    if(romboot_info->boot_id||romboot_info->init[0])
        return -20;
        
    nfio_page_read(0,temp_addr,DEFAULT_ECC_MODE|(ext&(3<<22)));
//    for(area=0;area<4;area++)
//    {
//        if(romboot_info->load[0][area]==0&&romboot_info->dchk[0][area]==0)
//            break;
//    }	
    return nfio_read(READ_SIZE, target, size,ext,0);
}
