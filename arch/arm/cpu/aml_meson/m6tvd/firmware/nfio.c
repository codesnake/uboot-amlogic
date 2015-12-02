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
#define NAND_INFO_BUF 		0x80000000
#define NAND_OOB_BUF  		0x80002000
#define NAND_TEMP_BUF 	0x82000000
#define NAND_DATA_BUF   0x83000000
#define NAND_SECURE_BUF   0xa00e0000

#define DEFAULT_ECC_MODE  ((2<<20)|(1<<17)|(7<<14)|(1<<13)|(48<<6)|1)

#ifdef MX_REVD
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
#endif

#if 1   //for micron read retry
STATIC_PREFIX short retry_micron_handle(unsigned retry_cnt)
{
	serial_puts("enter retry_cnt=0x");
	serial_put_hex(retry_cnt,32);
	serial_puts("\n");
		
	writel(CE0 | CLE  | 0xef,P_NAND_CMD);
	writel(CE0 | IDLE,P_NAND_CMD);
	writel(CE0 | ALE  | 0x89,P_NAND_CMD);
	writel(CE0 | IDLE,P_NAND_CMD);
	writel(CE0 | DWR  | (retry_cnt + 1),P_NAND_CMD);
	writel(CE0 | IDLE,P_NAND_CMD);
	writel(CE0 | DWR  | 0,P_NAND_CMD);
	writel(CE0 | IDLE,P_NAND_CMD);
	writel(CE0 | DWR  | 0,P_NAND_CMD);
	writel(CE0 | IDLE,P_NAND_CMD);
	writel(CE0 | DWR  | 0,P_NAND_CMD);
	writel(CE0 | IDLE,P_NAND_CMD);
}

STATIC_PREFIX void retry_micron_exit(void)
{
    serial_puts("retry_micron_exit\n");
    
    writel(CE0 | CLE  | 0xef,P_NAND_CMD); 
    writel(CE0 | IDLE,P_NAND_CMD);   
    writel(CE0 | ALE  | 0x89,P_NAND_CMD);
    writel(CE0 | IDLE,P_NAND_CMD);
    writel(CE0 | DWR  | 0,P_NAND_CMD);
    writel(CE0 | IDLE,P_NAND_CMD);
    writel(CE0 | DWR  | 0,P_NAND_CMD);
    writel(CE0 | IDLE,P_NAND_CMD);
    writel(CE0 | DWR  | 0,P_NAND_CMD);
    writel(CE0 | IDLE,P_NAND_CMD);
    writel(CE0 | DWR  | 0,P_NAND_CMD);	 
    writel(CE0 | IDLE,P_NAND_CMD);    
}
#endif

STATIC_PREFIX short nfio_page_read_hwctrl(unsigned src,unsigned mem, unsigned char *oob_buf, unsigned ext)
{
	int i, k, ecc_mode, short_mode, short_size, pages, page_size;
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

	for(i=0;i<pages;i++){
	    	info_buf[i]=0;
	}

	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

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
	writel(SEED | (0xc2 + (src&0xff)), P_NAND_CMD);
	writel((ext&0x3fffff), P_NAND_CMD);

	//    (*P_NAND_CMD) = (ext&0x3fffff);
	writel((CE0 | IDLE ), P_NAND_CMD);           //send dummy cmd idle
	writel((CE0 | IDLE ), P_NAND_CMD);           //send dummy cmd idle

	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

	while(info_buf[pages-1]==0);

	for (k=0; k<pages; k++){

	    	while(info_buf[k]==0);

		if((info_buf[k]>>16&0x3f) < 0xa){
			ret = ERROR_NAND_BLANK_PAGE;
		}

	    	if ((info_buf[k]>>24&0x3f) == 0x3f) {// uncorrectable
			if((ext>>24)&1){ //if with a2h cmd, just need reset
				writel((CE0 | CLE | 0xff), P_NAND_CMD);  //reset
				writel((CE0 | IDLE | 2), P_NAND_CMD);    // dummy

				// set Timer 13 ms.
				if(ext>>23&1) { // without RB/
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

				while ((readl(P_NAND_CMD)>>22&0x1f) > 0);

			}

			if(ret != ERROR_NAND_BLANK_PAGE){ //if randomization, check 0xff page, zero_cnt less than 10
				ret = ERROR_NAND_ECC;
			}
			break;
	    	}

		if(ret)
			break;

#if 1

		oob_buf[0] = (info_buf[k]&0xff);
		oob_buf[1] = ((info_buf[k]>>8)&0xff);

		oob_buf += 2;
#else
	    	if ((info_buf[k]&0xc000ffff) != 0xc000aa55) //magic word error
	       {
	       	serial_puts("info_buf ERROR_NAND_MAGIC_WORD\n");
			return ERROR_NAND_MAGIC_WORD;
	    	}
#endif
	}

	return ret;
}

STATIC_PREFIX short nfio_page_read(unsigned src, unsigned mem, unsigned char *oob_buf, unsigned ext)
{
	unsigned read_page, new_nand_type, pages_in_block;
	int retry_cnt, ret = 0;

	new_nand_type  = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int) + sizeof(int));
	pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));
    
	read_page = src;
#ifdef MX_REVD
	if((new_nand_type) && (new_nand_type < 10)){		//for new nand
		read_page = pagelist_hynix256[src%pages_in_block] \
			+ (src/pages_in_block)*pages_in_block;
	}
#endif    

	retry_cnt = 0;
page_retry:
	ret = nfio_page_read_hwctrl(read_page, mem, oob_buf, ext);
	if (ret == ERROR_NAND_ECC){
		serial_puts("nand read addr=0x");
		serial_put_hex(read_page,32);
		serial_puts("nfio_read read err here\n");

        	if((new_nand_type == MICRON_20NM) && (retry_cnt < 7)) {
        		retry_micron_handle(retry_cnt);
        		 retry_cnt++;
        		 goto page_retry;
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
	unsigned por_cfg = romboot_info->por_cfg;
	unsigned ret = 0, fsm;
	unsigned partition_num  = 0, rand_mode = 0;
	unsigned a2h_cmd_flag = 0;
	unsigned default_dma_mode = 0;

    	//set pinmux here
	switch(POR_GET_1ST_CFG(por_cfg)){
		case POR_1ST_NAND:
			setbits_le32(P_PERIPHS_PIN_MUX_2, (0x3ff<<18));
			break;
		case POR_1ST_NAND_RB:
			//            *P_PERIPHS_PIN_MUX_6 &= ~(0x3<<24);     // Clear SD_C_CMD&CLK
			//            *P_PERIPHS_PIN_MUX_2 &= ~(0x3<<22);     // Clear NAND_CS2&CS3
			clrbits_le32(P_PERIPHS_PIN_MUX_4, 3<<24);
			clrsetbits_le32(P_PERIPHS_PIN_MUX_2, (0xf3f<<16)|(3<<22), (0xf3f<<16));
			break;
	}

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

	// 4 copies of same data.
	for (partition_num=0; partition_num<4; partition_num++) {

		for (fsm=0; fsm<3; fsm++) {
			switch (fsm) {
			case 0: //normal
				rand_mode = 0;
				a2h_cmd_flag = 0;
				break;

			case 1: //  random
				rand_mode = 1;
				a2h_cmd_flag = 0;
				break;

			case 2: // normal, retry
				rand_mode = 1;
				a2h_cmd_flag = 1;
				break;

			default: //normal
				rand_mode = 0;
				a2h_cmd_flag = 0;
				break;
			}

			// read only 1 ecc page, 384 bytes.
			default_dma_mode = DEFAULT_ECC_MODE;
			default_dma_mode |= (ext&(3<<22))  | (partition_num<<26);
			default_dma_mode |= (rand_mode<<19) | (a2h_cmd_flag<<24);

			ret = nfio_page_read((partition_num<<8), NAND_TEMP_BUF, NAND_OOB_BUF, default_dma_mode);

			if (ret == 0)
				break;
		}

		if (ret == 0)
			break;
	}

	if(ret == ERROR_NAND_ECC){
		serial_puts("check bootid or init failed here\n");
		return -ERROR_NAND_ECC;
	}

	// nand parameters
	// when ecc_mode==1, page_size is 512 bytes.
	ecc_mode = ext>>14&7;
	short_mode = ext>>13&1;
	short_size = ext>>6&0x7f;
	pages = ext&0x3f;
	page_size = short_mode ? short_size : (ecc_mode<2 ? 64 : 128); 	// unit: 8 bytes;

	*data_size = page_size * 8 * pages;

	return 0;

}

STATIC_PREFIX void nf_set_pux(unsigned ce)
{
	if(ce == CE0){		
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 25));
		
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 17));
	}else if (ce == CE1){
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 24));	
		
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 16));
	}else if(ce == CE2){
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 23));	
	}else if(ce == CE3){
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1 << 22));
	}
	
	writel((ce | IDLE | 40), P_NAND_CMD); 
}

STATIC_PREFIX unsigned int  select_chip(int num)
{
	unsigned int ce;
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

STATIC_PREFIX void send_plane0_cmd(unsigned page, unsigned ext,unsigned ce)
{
	int  chip_num, plane_mode;
	unsigned int nand_read_info, ran_mode;
	unsigned char micron_nand_flag=0;
	unsigned page_in_blk, blk_num, pages_in_block, plane0, plane1;

	nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	plane_mode = (nand_read_info >> 2) & 0x1;
	ran_mode = (nand_read_info >> 3) & 0x1;
	micron_nand_flag = (nand_read_info >> 4) & 0x1;
	pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));

	blk_num = page /pages_in_block;
	page_in_blk = page - blk_num * pages_in_block;
	blk_num <<= 1;
	plane0 = blk_num *pages_in_block + page_in_blk;
	plane1 = (blk_num + 1) *pages_in_block + page_in_blk;

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
		serial_puts("with out rb \n");
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
	int  chip_num, plane_mode;
	unsigned int nand_read_info;
	unsigned char micron_nand_flag=0;
	unsigned page_in_blk, blk_num, pages_in_block, plane0, plane1;

	nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	plane_mode = (nand_read_info >> 2) & 0x1;
	micron_nand_flag = (nand_read_info >> 4) & 0x1;
	pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));

	blk_num = page /pages_in_block;
	page_in_blk = page - blk_num * pages_in_block;
	blk_num <<= 1;
	plane0 = blk_num *pages_in_block + page_in_blk;
	plane1 = (blk_num + 1) *pages_in_block + page_in_blk;

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
	int  chip_num, plane_mode;
	unsigned int nand_read_info;

	nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	plane_mode = (nand_read_info >> 2) & 0x1;

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
		serial_puts("with out rb \n");
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
	unsigned int i, chip_num,nand_read_info;
	unsigned int ce;
	nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	chip_num  = nand_read_info & 0x3;
	
	for(i=0;i<chip_num;i++){ //send reset cmd 0xff

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
	}

}

STATIC_PREFIX unsigned nf_read_check(volatile unsigned long long *info_buf, unsigned char * oob_buf,unsigned ext, unsigned ce )
{	
	int j, pages, k, ret=0;

	pages = ext&0x3f;
	while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
	
	while(info_buf[pages-1]==0);

	for (k=0; k<pages; k++){

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

		oob_buf[0] = (info_buf[k]&0xff);
		oob_buf[1] = ((info_buf[k]>>8)&0xff);

		oob_buf += 2;
	    	
	}
	
	return ret;
}

STATIC_PREFIX short nf_normal_read_page_hwctrl(unsigned page,unsigned  mem, unsigned char *oob_buf, unsigned ext, unsigned data_size)
{
	int i, k, chip_num, plane_mode,ecc_mode, short_mode, short_size;
	unsigned long info_adr = NAND_INFO_BUF;
	volatile unsigned long long * info_buf=(volatile unsigned long long *)NAND_INFO_BUF;
	unsigned int nand_read_info;
	int ce, pages, ret=0;
	
	nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	chip_num  = nand_read_info & 0x3;
	plane_mode = (nand_read_info >> 2) & 0x1;
	pages = ext&0x3f;
	writel(info_adr, P_NAND_IADR);
	for(k=0;k<pages;k++){
		info_buf[k]=0;
	}

	for(i=0;i<chip_num;i++){
		
		while ((readl(P_NAND_CMD)>>22&0x1f) > 0);
		writel(mem, P_NAND_DADR);
				
		ce = select_chip(i);
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
	}

	return ret;
}

STATIC_PREFIX int nf_normal_read_page(unsigned page, unsigned  mem, unsigned char *oob_buf, unsigned ext, unsigned data_size)
{
	unsigned read_page, new_nand_type, pages_in_block;
	unsigned chip_num, plane_mode, ram_mode;
	int i,retry_cnt, ret = 0;

	new_nand_type  = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int) + sizeof(int));
	pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));
	
	read_page = page;
	
	ret = nf_normal_read_page_hwctrl(read_page, mem, oob_buf, ext, data_size);
	if (ret == ERROR_NAND_ECC){
		serial_puts("nand read addr=0x");
		serial_put_hex(read_page,32);
		serial_puts("nfio_read read err here\n");
	}
	
	return ret;
}

//for uboot
STATIC_PREFIX short nf_read(unsigned target, unsigned size)
{
	unsigned ext = romboot_info->ext;
	unsigned int read_size, data_size;
	int retry_cnt, read_page, page_base, tmp_page;
	unsigned char *oob_buf = NAND_OOB_BUF;
	unsigned mem, count, pages;
	int pages_in_block, total_page = 1024;
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

    pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));

    ret = 0;
    for(i=0,read_size=0; i<total_page && read_size<count; i++, read_size+=data_size){

	        ret = nfio_page_read(page_base+i, mem + read_size, oob_buf, ext);
	        if (ret == ERROR_NAND_ECC){
	        	serial_puts("nand read addr=0x");	
	        	serial_put_hex(i+page_base,32);
	        	serial_puts("nfio_read read err here\n");
	        }	        

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
    }
#ifdef CONFIG_SECURE_NAND
	//load os_key	
	psecure = (unsigned char*)NAND_SECURE_BUF;
	memset(psecure, 0, 0x100);
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
	unsigned pages_in_block, data_size, read_size, page_base;
	unsigned ok_flag, read_page,tmp_page, key_addr, sec_pages, pages;
	unsigned  data_buf = NAND_DATA_BUF;
	unsigned char *oob_buf = NAND_OOB_BUF;
	int i, j, k, times, times1, times_tamp=0, ret = 0;
   	unsigned int magic_name,plane,plane_mode,chip_num,nand_read_info,ce;
	unsigned secure_block = 0;
	
	plane = 1;
	data_size = 0;
	pages = (ext&0x3f);
	//init hw config and get nand info from page0
	ret = nf_init(ext, &data_size);
	if(ret){
		serial_puts("nfio init failed here\n");
		return ret;
	}

	pages_in_block = *(volatile int *)(NAND_TEMP_BUF + sizeof(int));
	nand_read_info = *(volatile unsigned int *)(NAND_TEMP_BUF+sizeof(int)+sizeof(int) + sizeof(int) );
	chip_num  = nand_read_info & 0x3;
	plane_mode = (nand_read_info >> 2) & 0x1;
	secure_block = *(volatile unsigned int *)(NAND_TEMP_BUF+ 4 *sizeof(int));
	if (plane_mode == 1)
		plane = 2;
	
#if 0	
	serial_puts("nfio data_size = 0x \n");
	serial_put_hex(data_size,32);
	serial_puts("\n");

	serial_puts("nfio chip_num = 0x \n");
	serial_put_hex(chip_num,32);
	serial_puts("\n");

	serial_puts("nfio plane = 0x \n");
	serial_put_hex(plane,32);
	serial_puts("\n");

	serial_puts("nfio secure_block = 0x \n");
	serial_put_hex(secure_block,32);
	serial_puts("\n");

	serial_puts("nfio pages_in_block = 0x \n");
	serial_put_hex(pages_in_block,32);
	serial_puts("\n");
#endif

	send_reset_cmd(ext);
	
	ok_flag = 0;
	key_addr = 0;
	read_page = secure_block * pages_in_block; //search from offset page

	serial_puts("nf_read_secure: secure_block = 0x");
	serial_put_hex(secure_block,32);
	serial_puts("\n");
	serial_puts("nf_read_secure: pages_in_block = 0x");
	serial_put_hex(pages_in_block,32);
	serial_puts("\n");
	
	serial_puts("nf_read_secure  step 1\n");
	for(i=0; i<NAND_SEC_MAX_BLK_NUM; i++){

		tmp_page =read_page+ i* pages_in_block;
		
		ret = nf_normal_read_page(tmp_page, (data_buf+ i*(plane*chip_num*data_size)), oob_buf, ext, data_size);
		if(ret == ERROR_NAND_BLANK_PAGE) {
			continue;
		}

		if(ret == 0){  //read sucess, check oob magic

			times  = *(oob_buf+4);
			magic_name = ((*(oob_buf)&0xff) | ((*(oob_buf + 1)&0xff) << 8) | ((*(oob_buf + 2)&0xff) << 16) | ((*(oob_buf + 3)&0xff) << 24));
			times = ( (*(oob_buf+4)&0xff) | ((*(oob_buf+5)&0xff) << 8) | ((*(oob_buf+6)&0xff) << 16) | ((*(oob_buf+7)&0xff) << 24));
			if (magic_name == SECURE_STORE_MAGIC){
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
				key_addr = tmp_page;   			
				}
				if((i > NAND_SECURE_BLK)&&(ok_flag == 1)) {
						 break;
				}
			}else {
				ret = ERROR_NAND_MAGIC_WORD;
			}
		}
	}

	read_page = key_addr;
	key_addr = 0;
	
	serial_puts("nf_read_secure: step1 result block read_page = 0x");
	serial_put_hex(read_page,32);
	serial_puts("\n");

	sec_pages = CONFIG_SECURE_SIZE/(plane*chip_num*data_size);

	serial_puts("nf_read_secure: secure pages = 0x");
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
			tmp_page = read_page+i;

			ret = nf_normal_read_page(tmp_page, (data_buf+ i*(plane*chip_num*data_size)), oob_buf, ext, data_size);
			if(ret == ERROR_NAND_BLANK_PAGE){
				break;
			}

			times = ( (*(oob_buf+4)&0xff) | ((*(oob_buf+5)&0xff) << 8) | ((*(oob_buf+6)&0xff) << 16) | ((*(oob_buf+7)&0xff) << 24));
			magic_name = ((*(oob_buf)&0xff) | ((*(oob_buf + 1)&0xff) << 8) | ((*(oob_buf + 2)&0xff) << 16) | ((*(oob_buf + 3)&0xff) << 24));
			if(ret == 0){  //read sucess, check oob magic
				if(magic_name == SECURE_STORE_MAGIC){

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
	
	serial_puts("nf_read_secure:  step 3 read_page = 0x");
	serial_put_hex(read_page,32);
	serial_puts("\n");
		
	//read secure key here;
	if(ok_flag == 1){
		
		for(j=0; j<sec_pages;j++){
			ret = nf_normal_read_page(read_page+j, (NAND_SECURE_BUF+ j*(plane*chip_num*data_size)), oob_buf, ext, data_size);
			//ret = nfio_page_read(read_page+j, (NAND_SECURE_BUF+(j*data_size)), oob_buf, ext);
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
	return;
}
#endif
