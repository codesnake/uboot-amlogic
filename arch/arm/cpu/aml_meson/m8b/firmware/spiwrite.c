#include <asm/arch/timing.h>
#include <memtest.h>
#include <config.h>
#include <asm/arch/io.h>
#ifndef STATIC_PREFIX 
#define STATIC_PREFIX 
#endif
extern struct pll_clk_settings __plls;
SPL_STATIC_FUNC void spi_pinmux_init(void)
{

}

STATIC_PREFIX int SPI_page_program(unsigned * addr_source, unsigned spi_addr, int byte_length)
{
   //unsigned temp;
   unsigned temp_addr;
   int temp_bl,i;
   
   temp_addr = spi_addr;
   temp_bl = byte_length;
   if(byte_length&0x1f)
        return -1;
   ///remove SPI nor from AHB bus)
   clrbits_le32(P_SPI_FLASH_CTRL,1<<SPI_ENABLE_AHB);
   for(temp_addr=0;temp_addr<byte_length;temp_addr+=32)
   {
        writel(((temp_addr+spi_addr) & 0xffffff)|( 32 << SPI_FLASH_BYTES_LEN ),
            P_SPI_FLASH_ADDR);
        for(i=0;i<8;i++)
        {
            writel(*addr_source++,P_SPI_FLASH_C0+(i<<2));
        }
        writel(1 << SPI_FLASH_WREN,P_SPI_FLASH_CMD);
        while(readl(P_SPI_FLASH_CMD)!=0);
        writel(1 << SPI_FLASH_PP,P_SPI_FLASH_CMD);
        while(readl(P_SPI_FLASH_CMD)!=0);
        do{
            writel(1 << SPI_FLASH_RDSR,P_SPI_FLASH_CMD);
            while(readl(P_SPI_FLASH_CMD)!=0);
        }while((readl(P_SPI_FLASH_STATUS)&1)==1);
   }
   
  ///enable SPI nor from AHB bus
  setbits_le32(P_SPI_FLASH_CTRL,1<<SPI_ENABLE_AHB); 
  return 0;
}
STATIC_PREFIX int SPI_sector_erase(unsigned addr )
{
   //unsigned temp;
   clrbits_le32(P_SPI_FLASH_CTRL,1<<SPI_ENABLE_AHB);
   writel(addr & 0xffffff,P_SPI_FLASH_ADDR);
   
   //Write Enable 
   writel(1 << SPI_FLASH_WREN,P_SPI_FLASH_CMD);
   while(readl(P_SPI_FLASH_CMD)!=0);
        
   // sector erase  64Kbytes erase is block erase.
   writel(1 << SPI_FLASH_SE,P_SPI_FLASH_CMD);
   while(readl(P_SPI_FLASH_CMD)!=0);
   
   // check erase is finished.
  do{
        writel(1 << SPI_FLASH_RDSR,P_SPI_FLASH_CMD);
        while(readl(P_SPI_FLASH_CMD)!=0);
  }while((readl(P_SPI_FLASH_STATUS)&1)==1);
  
  ///enable SPI nor from AHB bus
  setbits_le32(P_SPI_FLASH_CTRL,1<<SPI_ENABLE_AHB); 
  
  return 0;
}
STATIC_PREFIX void spi_init(void)
{
    spi_pinmux_init();
    writel(__plls.spi_setting,P_SPI_FLASH_CTRL);
    
}
#if 0
STATIC_PREFIX void spi_erase(void)
{
    spi_pinmux_init();
    writel(__plls.spi_setting,P_SPI_FLASH_CTRL);
    serial_puts("\nErase..");
    SPI_sector_erase(0);
}
#endif
static void spi_disable_write_protect(void)
{
    unsigned char statusValue;
    int ret, var;

    statusValue = 0;

    /*write enable*/ 
    var = 1 << SPI_FLASH_WREN;
    writel(var, P_SPI_FLASH_CMD);
    while(readl(P_SPI_FLASH_CMD)!=0);
    ret=1;
    while ( (ret&1)==1 ) {   	
        var=1<<SPI_FLASH_RDSR;
        writel(var, P_SPI_FLASH_CMD);
        while(readl(P_SPI_FLASH_CMD)!=0);
        ret = readl(P_SPI_FLASH_STATUS)&0xff;
        }

    /*write status register*/
    writel(statusValue,P_SPI_FLASH_STATUS);
    var = 1<<SPI_FLASH_WRSR;
    writel(var, P_SPI_FLASH_CMD);
    while(readl(P_SPI_FLASH_CMD)!=0);
    ret=1;
    while ( (ret&1)==1 ) {   	
        var=1<<SPI_FLASH_RDSR;
        writel(var, P_SPI_FLASH_CMD);
        while(readl(P_SPI_FLASH_CMD)!=0);
        ret = readl(P_SPI_FLASH_STATUS)&0xff;
        }
}
STATIC_PREFIX void spi_program(unsigned dest,unsigned src,unsigned size)
{
	unsigned addr=0;
	spi_disable_write_protect();
	serial_puts("\nProgram..");
	for(addr=0;addr<size;addr+=4096){
        SPI_sector_erase(dest+addr);
        SPI_page_program((unsigned *)(src+addr),(dest+addr),4096);
    }
    serial_puts("\nEnd..\n");

}
