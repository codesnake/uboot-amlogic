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
/*
#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/uart.h>
*/
#include "appf_types.h"

//////////////////////////////
#define IO_CBUS_BASE			0xc1100000
#define IO_AXI_BUS_BASE			0xc1300000
#define IO_AHB_BUS_BASE			0xc9000000
#define IO_APB_BUS_BASE			0xc8000000
#define IO_USB_A_BASE			0xc9040000
#define IO_USB_B_BASE			0xc90C0000

#define MESON_PERIPHS1_VIRT_BASE	0xc1108400
#define MESON_PERIPHS1_PHYS_BASE	0xc1108400

// ----------------------------
// UART
// ----------------------------
#define P_AO_UART_WFIFO              (0xc8100000 | (0x01 << 10) | (0x30 << 2))
#define P_AO_UART_RFIFO              (0xc8100000 | (0x01 << 10) | (0x31 << 2))
#define P_AO_UART_CONTROL            (0xc8100000 | (0x01 << 10) | (0x32 << 2))
#define P_AO_UART_STATUS             (0xc8100000 | (0x01 << 10) | (0x33 << 2))
#define P_AO_UART_MISC               (0xc8100000 | (0x01 << 10) | (0x34 << 2))


#define CBUS_REG_OFFSET(reg) ((reg) << 2)
#define CBUS_REG_ADDR(reg)	 (IO_CBUS_BASE + CBUS_REG_OFFSET(reg))

#define AXI_REG_OFFSET(reg)  ((reg) << 2)
#define AXI_REG_ADDR(reg)	 (IO_AXI_BUS_BASE + AXI_REG_OFFSET(reg))

#define AHB_REG_OFFSET(reg)  ((reg) << 2)
#define AHB_REG_ADDR(reg)	 (IO_AHB_BUS_BASE + AHB_REG_OFFSET(reg))

#define APB_REG_OFFSET(reg)  (reg)
#define APB_REG_ADDR(reg)	 (IO_APB_BUS_BASE + APB_REG_OFFSET(reg))
#define APB_REG_ADDR_VALID(reg) (((unsigned long)(reg) & 3) == 0)


#define UART_PORT_0     CBUS_REG_ADDR(UART0_WFIFO)
#define UART_PORT_1     CBUS_REG_ADDR(UART1_WFIFO)
#define UART_PORT_2     CBUS_REG_ADDR(UART2_WFIFO)
#define UART_PORT_AO    P_AO_UART_WFIFO

#define UART_WFIFO      (0<<2)
#define UART_RFIFO      (1<<2)
#define UART_CONTROL    (2<<2)
#define UART_STATUS     (3<<2)
#define UART_MISC       (4<<2)

#define P_UART(uart_base,reg)    	(uart_base+reg)
#define P_UART_WFIFO(uart_base)   	P_UART(uart_base,UART_WFIFO)
#define P_UART_RFIFO(uart_base)   	P_UART(uart_base,UART_RFIFO)

#define P_UART_CONTROL(uart_base)    P_UART(uart_base,UART_CONTROL)
    #define UART_CNTL_MASK_BAUD_RATE                (0xfff)
    #define UART_CNTL_MASK_TX_EN                    (1<<12)
    #define UART_CNTL_MASK_RX_EN                    (1<<13)
    #define UART_CNTL_MASK_2WIRE                    (1<<15)
    #define UART_CNTL_MASK_STP_BITS                 (3<<16)
    #define UART_CNTL_MASK_STP_1BIT                 (0<<16)
    #define UART_CNTL_MASK_STP_2BIT                 (1<<16)
    #define UART_CNTL_MASK_PRTY_EVEN                (0<<18)
    #define UART_CNTL_MASK_PRTY_ODD                 (1<<18)
    #define UART_CNTL_MASK_PRTY_TYPE                (1<<18)
    #define UART_CNTL_MASK_PRTY_EN                  (1<<19)
    #define UART_CNTL_MASK_CHAR_LEN                 (3<<20)
    #define UART_CNTL_MASK_CHAR_8BIT                (0<<20)
    #define UART_CNTL_MASK_CHAR_7BIT                (1<<20)
    #define UART_CNTL_MASK_CHAR_6BIT                (2<<20)
    #define UART_CNTL_MASK_CHAR_5BIT                (3<<20)
    #define UART_CNTL_MASK_RST_TX                   (1<<22)
    #define UART_CNTL_MASK_RST_RX                   (1<<23)
    #define UART_CNTL_MASK_CLR_ERR                  (1<<24)
    #define UART_CNTL_MASK_INV_RX                   (1<<25)
    #define UART_CNTL_MASK_INV_TX                   (1<<26)
    #define UART_CNTL_MASK_RINT_EN                  (1<<27)
    #define UART_CNTL_MASK_TINT_EN                  (1<<28)
    #define UART_CNTL_MASK_INV_CTS                  (1<<29)
    #define UART_CNTL_MASK_MASK_ERR                 (1<<30)
    #define UART_CNTL_MASK_INV_RTS                  (1<<31)

#define P_UART_STATUS(uart_base)  P_UART(uart_base,UART_STATUS )
    #define UART_STAT_MASK_RFIFO_CNT                (0x7f<<0)
    #define UART_STAT_MASK_TFIFO_CNT                (0x7f<<8)
    #define UART_STAT_MASK_PRTY_ERR                 (1<<16)
    #define UART_STAT_MASK_FRAM_ERR                 (1<<17)
    #define UART_STAT_MASK_WFULL_ERR                (1<<18)
    #define UART_STAT_MASK_RFIFO_FULL               (1<<19)
    #define UART_STAT_MASK_RFIFO_EMPTY              (1<<20)
    #define UART_STAT_MASK_TFIFO_FULL               (1<<21)
    #define UART_STAT_MASK_TFIFO_EMPTY              (1<<22)

#define P_UART_MISC(uart_base)    P_UART(uart_base,UART_MISC   )

#define SPL_STATIC_FUNC

#define WATCHDOG_TC                                0x2640
#define WATCHDOG_RESET                             0x2641

#define P_WATCHDOG_TC					CBUS_REG_ADDR(WATCHDOG_TC)
#define P_WATCHDOG_RESET      CBUS_REG_ADDR(WATCHDOG_RESET)

#define setbits_le32(reg,mask)  writel(readl(reg)|(mask),reg)
#define clrbits_le32(reg,mask)  writel(readl(reg)&(~(mask)),reg)
#define clrsetbits_le32(reg,clr,mask)  writel((readl(reg)&(~(clr)))|(mask),reg)


/////////////////////////////

unsigned delay_tick(int count)
{
	int i;
	while(count > 0){
		for(i = 0; i < 100; i++);
		count--;
	};
	return count;
}
void delay_ms(int usec);
#define UART_PORT_CONS UART_PORT_AO
/*
#ifndef CONFIG_AML_ROMBOOT_SPL
SPL_STATIC_FUNC int serial_set_pin_port(unsigned port_base)
{
    setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}
#endif
*/

SPL_STATIC_FUNC void serial_clr_err(void)
{
    if(readl(P_UART_STATUS(UART_PORT_CONS))&(UART_STAT_MASK_PRTY_ERR|UART_STAT_MASK_FRAM_ERR))
	    setbits_le32(P_UART_CONTROL(UART_PORT_CONS),UART_CNTL_MASK_CLR_ERR);
}
#if 0
void __udelay(int n);
SPL_STATIC_FUNC void uart_reset()
{
	unsigned uart_cfg = readl(P_UART_CONTROL(UART_PORT_CONS));
	unsigned uart_misc = readl(P_AO_UART_MISC);
	setbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	clrbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	__udelay(100);
	writel(uart_cfg,P_UART_CONTROL(UART_PORT_CONS));
	writel(uart_misc,P_AO_UART_MISC);
}
SPL_STATIC_FUNC void serial_init(unsigned set,unsigned tag)
{
    /* baud rate */
    unsigned baud_para=0;
    if(tag){//reset uart.
	    setbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	    clrbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	    delay_ms(10);
			//delay_tick(32*1000);
			//delay_tick(32*1000);
    }

	writel((set&0xfff)
	      | UART_STP_BIT
        | UART_PRTY_BIT
        | UART_CHAR_LEN
        //please do not remove these setting , jerry.yu
        | UART_CNTL_MASK_TX_EN
        | UART_CNTL_MASK_RX_EN
        | UART_CNTL_MASK_RST_TX
        | UART_CNTL_MASK_RST_RX
        | UART_CNTL_MASK_CLR_ERR
	,P_UART_CONTROL(UART_PORT_CONS));
	clrsetbits_le32(P_AO_UART_MISC ,0xf<<20,(set&0xf000)<<8);
  serial_set_pin_port(UART_PORT_CONS);
  clrbits_le32(P_UART_CONTROL(UART_PORT_CONS),
	    UART_CNTL_MASK_RST_TX | UART_CNTL_MASK_RST_RX | UART_CNTL_MASK_CLR_ERR);

}
#endif

SPL_STATIC_FUNC void serial_putc(const char c)
{
    if (c == '\n'){
        while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL));
        writel('\r', P_UART_WFIFO(UART_PORT_CONS));
    }
    /* Wait till dataTx register is not full */
    while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL));
    writel(c, P_UART_WFIFO(UART_PORT_CONS));
    /* Wait till dataTx register is empty */
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise 0.
 */

SPL_STATIC_FUNC
int serial_tstc(void)
{
	return (readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_RFIFO_CNT);
}

/*
 * Read a single byte from the serial port. 
 */
SPL_STATIC_FUNC
int serial_getc(void)
{
    unsigned char ch;   
    /* Wait till character is placed in fifo */
  	while((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_RFIFO_CNT)==0) ;
    
    /* Also check for overflow errors */
    if (readl(P_UART_STATUS(UART_PORT_CONS)) & (UART_STAT_MASK_PRTY_ERR | UART_STAT_MASK_FRAM_ERR))
	{
	    setbits_le32(P_UART_CONTROL(UART_PORT_CONS),UART_CNTL_MASK_CLR_ERR);
	    clrbits_le32(P_UART_CONTROL(UART_PORT_CONS),UART_CNTL_MASK_CLR_ERR);
	}

    ch = readl(P_UART_RFIFO(UART_PORT_CONS)) & 0x00ff;
    return ((int)ch);
}



SPL_STATIC_FUNC
void serial_puts(const char *s)
{
    while (*s != '\0') {
        serial_putc(*s++);
    }
    delay_tick(32*1000);
}

void f_serial_puts(const char *s)
{
    while (*s != '\0') {
        serial_putc(*s++);
    }
}

void wait_uart_empty()
{
//	while((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_EMPTY) == 0)
//		delay_tick(4);
    unsigned int count=0;
    do{
        if((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_EMPTY) == 0)
        {
            delay_tick(4);
        }
        else
        {
            break;
        }
        count++;
    }while(count<20000);
}

SPL_STATIC_FUNC
void serial_puts_no_delay(const char *s)
{
    while (*s != '\0') {
        serial_putc(*s++);
    }
}



void k_delay(void)
{
   delay_tick(32*1000);
}

void serial_put_hex(unsigned int data,unsigned bitlen)
{
	int i;
  for (i=bitlen-4;i>=0;i-=4){
        if((data>>i)==0)
        {
            serial_putc(0x30);
            continue;
        }
        unsigned char s = (data>>i)&0xf;
        if (s<10)
            serial_putc(0x30+s);
        else
            serial_putc(0x61+s-10);
    }
}

void do_exception(unsigned reason)
{
    serial_puts("Enter Exception:");
    serial_put_hex(reason,32);
    writel((1<<22)|1000000,P_WATCHDOG_TC);//enable Watchdog
}



void disp_uart_ctrl(void)
{
	serial_puts("uart_ctrl:");
	serial_put_hex(readl(P_UART_CONTROL(UART_PORT_CONS)),32);
}

unsigned get_uart_ctrl(void)
{
	return (readl(P_UART_CONTROL(UART_PORT_CONS)) & 0xFFF);
}
