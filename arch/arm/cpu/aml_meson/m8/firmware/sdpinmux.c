/*******************************************************************
 *
 *  Copyright C 2010 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description: Serial driver.
 *
 *  Author: Jerry Yu
 *  Created: 2009-3-12
 *
 *******************************************************************/
#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/romboot.h>

//P_PREG_JTAG_GPIO_ADDR
SPL_STATIC_FUNC void disable_sdio(unsigned por_config)
{
    //disable power
    setbits_le32(P_PREG_PAD_GPIO5_O,(1<<31));
    clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
        
    switch(POR_GET_2ND_CFG(por_config))
    {
        case POR_2ND_SDIO_A://SDIOA,GPIOX_31~GPIOX_36
            clrbits_le32(P_PERIPHS_PIN_MUX_8,0x3f);
            break;
        case POR_2ND_SDIO_B://SDIOB,GPIOX_37~GPIOX_42
            clrbits_le32(P_PERIPHS_PIN_MUX_2,0x3f<<10);
            break;
        case POR_2ND_SDIO_C:
            clrbits_le32(P_PERIPHS_PIN_MUX_6,(0x3f<<24));
            break;
    }
    
}
SPL_STATIC_FUNC unsigned enable_sdio(unsigned por_config)
{
    unsigned SD_boot_type=2;

//    //enable power
    clrbits_le32(P_PREG_PAD_GPIO5_O,(1<<31));
    clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
                           
    switch(POR_GET_2ND_CFG(por_config))
    {
        case POR_2ND_SDIO_A://SDIOA,GPIOX_31~GPIOX_36
            setbits_le32(P_PERIPHS_PIN_MUX_8,0x3f);
            SD_boot_type=0;
            break;
        case POR_2ND_SDIO_B://SDIOB,GPIOX_37~GPIOX_42
            setbits_le32(P_PERIPHS_PIN_MUX_2,0x3f<<10);
            SD_boot_type=1;
            break;
        case POR_2ND_SDIO_C:
            clrbits_le32(P_PERIPHS_PIN_MUX_2,(0x1f<<22));
            setbits_le32(P_PERIPHS_PIN_MUX_6,(0x3f<<24));
            SD_boot_type=2;
            break;
    }
    return SD_boot_type;
}
SPL_STATIC_FUNC int sdio_get_port(unsigned por_config)
{
    
    switch(POR_GET_2ND_CFG(por_config))
    {
        case POR_2ND_SDIO_A://SDIOA,GPIOX_31~GPIOX_36
            return 0;
        case POR_2ND_SDIO_B:
            return 1;
        case POR_2ND_SDIO_C:
            return 2;
    }
    return 1;
}



