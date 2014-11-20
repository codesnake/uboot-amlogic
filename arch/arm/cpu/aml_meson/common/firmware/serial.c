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
#include <asm/arch/uart.h>
#include <asm/arch/io.h>

int serial_set_pin_port(unsigned port_base);

SPL_STATIC_FUNC void serial_init(unsigned set)
{
    /* baud rate */
	writel(set
	    |UART_CNTL_MASK_RST_TX
	    |UART_CNTL_MASK_RST_RX
	    |UART_CNTL_MASK_CLR_ERR
	    |UART_CNTL_MASK_TX_EN
	    |UART_CNTL_MASK_RX_EN
	,P_UART_CONTROL(UART_PORT_CONS));
    serial_set_pin_port(UART_PORT_CONS);
    clrbits_le32(P_UART_CONTROL(UART_PORT_CONS),
	    UART_CNTL_MASK_RST_TX | UART_CNTL_MASK_RST_RX | UART_CNTL_MASK_CLR_ERR);

}
//SPL_STATIC_FUNC
void serial_putc(const char c)
{
    if (c == '\n')
    {
        while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL));
        writel('\r', P_UART_WFIFO(UART_PORT_CONS));
    }
    /* Wait till dataTx register is not full */
    while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL));
    writel(c, P_UART_WFIFO(UART_PORT_CONS));
    /* Wait till dataTx register is empty */
}

//SPL_STATIC_FUNC
void serial_wait_tx_empty(void)
{
    while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_EMPTY)==0);

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

//SPL_STATIC_FUNC
void serial_puts(const char *s)
{
    while (*s) {
        serial_putc(*s++);
    }

	serial_wait_tx_empty();
}
SPL_STATIC_FUNC
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

	serial_wait_tx_empty();

}

SPL_STATIC_FUNC void serial_put_dec(unsigned int data)
{
	char szTxt[10];
	szTxt[0] = 0x30;
	int i = 0;
	while(data)
	{
		szTxt[i++] = (data % 10) + 0x30;
		data = data / 10;
	}

	for(--i;i >=0;--i)	
		serial_putc(szTxt[i]);

	serial_wait_tx_empty();
}


#define serial_put_char(data) serial_puts("0x");serial_put_hex((unsigned)data,8);serial_putc('\n')
#define serial_put_dword(data) serial_puts("0x");serial_put_hex((unsigned)data,32);serial_putc('\n')
void do_exception(unsigned reason,unsigned lr)
{
    serial_puts("Enter Exception:");
    serial_put_dword(reason);
    serial_puts("\tlink addr:");
        serial_put_dword(lr);
#ifdef CONFIG_ENABLE_WATCHDOG
	AML_WATCH_DOG_START();//enable watch dog
#endif
}
