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
//#include <asm/arch/cpu.h>
#include <asm/arch/uart.h>

unsigned delay_tick(unsigned count);
void delay_ms(int usec);

#ifndef CONFIG_AML_ROMBOOT_SPL
#define UART_PORT_CONS UART_PORT_AO

#define SPL_STATIC_FUNC

#define SPL_STATIC_VAR

void __udelay(int n)
{
        int i;
        for(i=0;i<n;i++)
        {
            asm("mov r0,r0");
        }
}

SPL_STATIC_FUNC int serial_set_pin_port(unsigned port_base)
{
    setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}
#endif


SPL_STATIC_FUNC void serial_clr_err(void)
{
    if(readl(P_UART_STATUS(UART_PORT_CONS))&(UART_STAT_MASK_PRTY_ERR|UART_STAT_MASK_FRAM_ERR))
	    setbits_le32(P_UART_CONTROL(UART_PORT_CONS),UART_CNTL_MASK_CLR_ERR);
}
void __udelay(int n);
SPL_STATIC_FUNC void uart_reset()
{
	if(readl(P_UART_REG5(UART_PORT_CONS))==0)
	{
	unsigned uart_cfg = readl(P_UART_CONTROL(UART_PORT_CONS));
	unsigned uart_misc = readl(P_AO_UART_MISC);
	setbits_le32(P_AO_RTI_GEN_CTNL_REG0,1<<17);
	clrbits_le32(P_AO_RTI_GEN_CTNL_REG0,1<<17);
	__udelay(100);
	writel(uart_cfg,P_UART_CONTROL(UART_PORT_CONS));
	writel(uart_misc,P_AO_UART_MISC);
	}else {
	unsigned uart_cfg = readl(P_UART_CONTROL(UART_PORT_CONS));
	unsigned uart_misc = readl(P_AO_UART_MISC);
	unsigned new_baudrate=readl(P_UART_REG5(UART_PORT_CONS));
	setbits_le32(P_AO_RTI_GEN_CTNL_REG0,1<<17);
	clrbits_le32(P_AO_RTI_GEN_CTNL_REG0,1<<17);
	__udelay(100);
	writel(uart_cfg,P_UART_CONTROL(UART_PORT_CONS));
	writel(uart_misc,P_AO_UART_MISC);
	writel(new_baudrate,P_UART_REG5(UART_PORT_CONS));
	//writel(1<<23,P_UART_REG5(UART_PORT_CONS));
	__udelay(100);
	
	}
}
SPL_STATIC_FUNC void serial_init(unsigned set,unsigned tag)
{
    if(tag){//reset uart.
	    setbits_le32(P_AO_RTI_GEN_CTNL_REG0,1<<17);
	    clrbits_le32(P_AO_RTI_GEN_CTNL_REG0,1<<17);
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
#if 0
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
#endif
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
/*
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
*/
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
/*
void do_exception(unsigned reason)
{
    serial_puts("Enter Exception:");
    serial_put_dword(reason);
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
}*/
