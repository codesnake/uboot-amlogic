#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>
//conflict with spiwrite.c, function redefine
#if 0
#if CONFIG_CMD_SF
SPL_STATIC_FUNC void spi_init()
{
    spi_pinmux_init();
    writel(__plls.spi_setting,P_SPI_FLASH_CTRL);
}
#endif
#endif

#if CONFIG_CMD_NAND
#include <asm/arch/nand.h>
SPL_STATIC_FUNC void nf_pinmux_init(void)
{
    NAND_IO_ENABLE(0);
}
SPL_STATIC_FUNC  void nf_reset(unsigned ce)
{
	NFC_SEND_CMD(1<<31);

	NFC_SEND_CMD_IDLE(ce,0);     // FIFO cleanup
	NFC_SEND_CMD_CLE(ce,0xff);   //Send reset command
	NFC_SEND_CMD_IDLE(ce,10);    //Wait tWB
	NFC_SEND_CMD_RB(ce,0);      //Wait Busy Signal
	NFC_SEND_CMD_IDLE(ce,0);     // FIFO cleanup
	NFC_SEND_CMD_IDLE(ce,0);     // FIFO cleanup
	while(NFC_CMDFIFO_SIZE()>0)   // wait until all command is finished
	{
		if(NFC_CHECEK_RB_TIMEOUT())  // RB time out
			break;
	}

	while(!((NFC_INFO_GET()>>26)&1));
}
/**
   page_mode : 0, small page ; 1 large page
*/
SPL_STATIC_FUNC  int nf_send_read_cmd(unsigned page_off,unsigned page_mode)
{
	while (NFC_CMDFIFO_SIZE() > 15);

	NFC_SEND_CMD_CLE(CE0,0);                //Send NAND read start command

	NFC_SEND_CMD_ALE(CE0,0);                // Send Address
	if(page_mode)
		NFC_SEND_CMD_ALE(CE0,0);
	NFC_SEND_CMD_ALE(CE0,page_off&0xff);
	NFC_SEND_CMD_ALE(CE0,(page_off&0xff00)>>8);
	NFC_SEND_CMD_ALE(CE0,(page_off&0xff0000)>>16);

	if(page_mode)
		NFC_SEND_CMD_CLE(CE0,0x30);

	/** waiting for tWB **/
	NFC_SEND_CMD_IDLE(CE0,5); // tWB
	NFC_SEND_CMD_RB(CE0,14);
	if(((NFC_INFO_GET()>>26)&1))
	{
		serial_puts("BUG RB CMD\n");
		while(!((NFC_INFO_GET()>>26)&1));
	}

	NFC_SEND_CMD_IDLE(CE0,0);
	while (NFC_CMDFIFO_SIZE() > 0);      // all cmd is finished
	return 0;
}
SPL_STATIC_FUNC int nf_read_dma(unsigned * dest, volatile unsigned * info,unsigned size,unsigned ecc_mode)
{
   unsigned cnt,i;

   if(ecc_mode!=NAND_ECC_BCH16)
	   return -1;
   if(size&0x1ff)
      return -1; // the size must be aligned with 512
  cnt=size>>9;
  for(i=0;i<cnt;i++)
  {
      info[i]=0;
  }
  NFC_SET_DADDR(dest);
  NFC_SET_IADDR(&info[0]);
//  NFC_SEND_CMD_N2M(size,ecc_mode);
  NFC_SEND_CMD_IDLE(CE0,5);
  NFC_SEND_CMD_IDLE(CE0,0);
  while (NFC_CMDFIFO_SIZE() > 0);      // all cmd is finished


  for(i=0;i<cnt;i++)
  {
 	  while((NAND_INFO_DONE(info[i]))==0);

	  if(NAND_ECC_ENABLE(info[i])&&(NAND_ECC_FAIL(info[i])))
         return -1;

	  if(NAND_INFO_DATA_2INFO(info[i])!=0x55aa)
	  {
	    serial_put_hex(NAND_INFO_DATA_2INFO(info[i]),16);
         return -2;
      }
  }

  return 0;
}
SPL_STATIC_FUNC void nf_cntl_init(const T_ROM_BOOT_RETURN_INFO* bt_info)
{
    nf_pinmux_init();
    //Set NAND to DDR mode , time mode 0 , no adjust
	WRITE_CBUS_REG(NAND_CFG, (0<<10) | (0<<9) | (0<<5) | 19);


}

/*
   Large Page NAND flash Read flow
*/
#define CONFIG_NAND_INFO_DMA_ADDR CONFIG_SYS_SDRAM_BASE
SPL_STATIC_FUNC int  nf_lp_read(volatile unsigned  dest, volatile unsigned size)
{
	volatile  	unsigned page_base,cnt,cur;
	volatile   	int ret=0;
	//unsigned char lp=0;
	nf_cntl_init(romboot_info);
	cnt=0;
	cur=READ_SIZE/(3*512);
	page_base= 0;
//	romboot_info->nand_addr<<8;

	for(;cnt<size&&cur<256;cnt+=3*512,cur++)
	{

		do{

			nf_send_read_cmd(page_base+cur,1);//Large Page mode

			ret=nf_read_dma((unsigned*)(dest+cnt),
			    ( volatile unsigned*)CONFIG_NAND_INFO_DMA_ADDR,3*512, NAND_ECC_BCH16);

			if(ret==0)
				break;
			else
			{

				if(ret==-1)
				    serial_puts(" ecc err\n");

				if(ret==-2)
				    serial_puts("not 55aa\n");

				page_base+=256;
				if(page_base==1024)
				{
					serial_puts("lp 1024\n");
					goto bad;
					//	while(1);//dead , booting fail
				}
			}
		} while(ret!=0);
	}
	return 0;
bad:
	return 1;
}
SPL_STATIC_FUNC int nf_sp_read(unsigned dest,unsigned size)
{
   unsigned page_base,cnt,cur;
   int ret;
   nf_cntl_init(romboot_info);
   page_base= 0;

   for(cnt=0,cur=READ_SIZE/512;cnt<size&&(page_base+cur)<1024;cnt+=512,cur++)
   {
	   do{
		   nf_send_read_cmd(page_base+cur,1);//small Page mode
		   ret=nf_read_dma((unsigned*)(dest+cnt),(unsigned*)CONFIG_NAND_INFO_DMA_ADDR,
				   512, NAND_ECC_BCH8);

		   if(ret!=-1)
			   break;
		   else{
			   page_base+=CONFIG_NAND_SP_BLOCK_SIZE;
			   if((page_base+cur)>=  4096*1024/*CONFIG_NAND_PAGES*/)
				   return -1;
		   }
	   } while(ret==-1);
   }
   return 0;

}


#endif