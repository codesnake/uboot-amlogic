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
#include <asm/arch/reg_addr.h>


//P_PREG_JTAG_GPIO_ADDR
SPL_STATIC_FUNC void disable_sdio(unsigned por_config)
{
#if CONFIG_SDIO_B1    
    if(POR_GET_SDIO_CFG(por_config)==0)
    {
        clrbits_le32(P_PERIPHS_PIN_MUX_7,(0x3f<<24));
        return;
    }
#endif
    //set test_n as gpio
    clrbits_le32(P_PREG_JTAG_GPIO_ADDR,
                          (1<<16));

    //clear pinmux        
    switch(POR_GET_SDIO_CFG(por_config))
    {
#if CONFIG_SDIO_A        
        case POR_SDIO_A_ENABLE://SDIOA,GPIOX_31~GPIOX_36
            clrbits_le32(P_PERIPHS_PIN_MUX_0,0x3f<<23);
            clrbits_le32(P_PREG_EGPIO_EN_N,(0x3f<<13));
            clrbits_le32(P_PREG_EGPIO_O,(0x3f<<13));
            break;
#endif      
#if CONFIG_SDIO_B      
        case POR_SDIO_B_ENABLE://SDIOB,GPIOX_37~GPIOX_42
            
            clrbits_le32(P_PERIPHS_PIN_MUX_1,0x3f);
            clrbits_le32(P_PREG_EGPIO_EN_N,(0x3f<<4));
            clrbits_le32(P_PREG_EGPIO_O,(0x3f<<4));
            break;
#endif
#if CONFIG_SDIO_C            
        case POR_SDIO_C_ENABLE:
            clrbits_le32(P_PERIPHS_PIN_MUX_2,(0xf<<16)|(1<<12)|(1<<8));//(0x3f<<21)
            clrbits_le32(P_PREG_EGPIO_EN_N,(0x3f<<21));
            clrbits_le32(P_PREG_EGPIO_O,(0x3f<<21));
            break;
#endif
                    
        default://SDIOC, GPIOX_17~GPIOX_22, iNAND or eMMC, don't need to send CARD_EN
            
            // disable SPI and SDIO_B
            
            break;
    }
    //pull up test_n
    setbits_le32(P_PREG_JTAG_GPIO_ADDR,
                           (1<<20));
    
}
SPL_STATIC_FUNC unsigned enable_sdio(unsigned por_config)
{
#if CONFIG_SDIO_B1    
    if(POR_GET_SDIO_CFG(por_config)==0)
    {
        clrbits_le32(P_PERIPHS_PIN_MUX_1,( (1<<29) | (1<<27) | (1<<25) | (1<<23)|0x3f));
        clrbits_le32(P_PERIPHS_PIN_MUX_6,0x7fff);
        setbits_le32(P_PERIPHS_PIN_MUX_7,(0x3f<<24));
        return 1;
    }
#endif    
	clrbits_le32(P_PREG_JTAG_GPIO_ADDR,(1<<20)   // test_n_gpio_o
                        |(1<<16));
    switch(POR_GET_SDIO_CFG(por_config))
    {
#if CONFIG_SDIO_A        
        case POR_SDIO_A_ENABLE://SDIOA,GPIOX_31~GPIOX_36
            setbits_le32(P_PERIPHS_PIN_MUX_0,0x3f<<23);
            return 0;
#endif            
#if CONFIG_SDIO_B
        case POR_SDIO_B_ENABLE://SDIOB,GPIOX_37~GPIOX_42
            //diable SDIO_B1
            clrbits_le32(P_PERIPHS_PIN_MUX_7,(0x3f<<24));
            setbits_le32(P_PERIPHS_PIN_MUX_1,0x3f);
            return 1;
#endif            
            
#if CONFIG_SDIO_C
        case POR_SDIO_C_ENABLE:
            setbits_le32(P_PERIPHS_PIN_MUX_2,(0xf<<16)|(1<<12)|(1<<8));
            return 2;
#endif            
    }
    return -1;
}
SPL_STATIC_FUNC int sdio_get_port(unsigned por_config)
{
    
    switch(POR_GET_SDIO_CFG(por_config))
    {
#if CONFIG_SDIO_A
        case POR_SDIO_A_ENABLE://SDIOA,GPIOX_31~GPIOX_36
            return 0;
#endif            
#if CONFIG_SDIO_B
        case POR_SDIO_B_ENABLE:
            return 1;
#endif
#if CONFIG_SDIO_B1
        case 0:
            return 1;
#endif

#if CONFIG_SDIO_C            
        case POR_SDIO_C_ENABLE:
            return 2;
#endif            
    }

    return -1;
}



