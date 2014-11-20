#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>
SPL_STATIC_FUNC void fw_print_info(unsigned por_cfg,unsigned stage)
{
    serial_puts("Boot from");
    if(stage==0){
        serial_puts(" internal device ");
        switch(por_cfg&POR_INTL_CFG_MASK)
        {
    #if CONFIG_CMD_SF        
            case POR_INTL_SPI:
                serial_puts("SPI NOR\n");
                break;
    #endif
    #if CONFIG_CMD_NAND            
            case  POR_INTL_NAND_LP:
               serial_puts("Large page NAND\n");
               break;
            case  POR_INTL_NAND_SP:
               serial_puts("Small page NAND\n");
               break;
    #endif
    #if CONFIG_CMD_MMC && CONFIG_SDIO_B1          
            case  POR_INTL_SDIO_B1:
               serial_puts("Internal MMC\n");
               break;
    #endif           
            default:
               serial_put_dword(por_cfg);
               serial_puts("Config Error");
               
        }
        return ;
    }
    serial_puts(" external device ");
    return ;
}
SPL_STATIC_FUNC int fw_load_intl(unsigned por_cfg,unsigned target,unsigned size)
{
    int rc=0;

    switch(por_cfg&POR_INTL_CFG_MASK)
    {
#if CONFIG_CMD_SF        
        case POR_INTL_SPI:
	    {
            unsigned* mem;
            mem=(unsigned *)(SPI_MEM_BASE+READ_SIZE);
            spi_init();
            ipl_memcpy((unsigned*)target,mem,size);
            }
            break;
#endif
#if CONFIG_CMD_NAND            
        case  POR_INTL_NAND_LP:
           rc=nf_lp_read(target,size);
           break;
        case  POR_INTL_NAND_SP:
           rc=nf_sp_read(target,size);
           break;
#endif
#if CONFIG_CMD_MMC && CONFIG_SDIO_B1               
        case  POR_INTL_SDIO_B1:
           rc=sdio_read(target,size,0);
           break;
#endif           
        default:
           return 1;
    }
    return rc;
}
#if CONFIG_CMD_MMC
SPL_STATIC_FUNC int fw_init_extl(unsigned por_cfg)
{
	int rc=sdio_init(por_cfg);
    return rc;
}
SPL_STATIC_FUNC int fw_load_extl(unsigned por_cfg,unsigned target,unsigned size)
{
    int rc=sdio_read(target,size,por_cfg);
    return rc;
}
#else
SPL_STATIC_FUNC int fw_init_extl(unsigned por_cfg)
{
    return 1;
}
SPL_STATIC_FUNC int fw_load_extl(unsigned por_cfg,unsigned target,unsigned size)
{
    return 1;
}

#endif


