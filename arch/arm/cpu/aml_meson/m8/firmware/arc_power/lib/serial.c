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
//#include <cpu.h>
#include <uart.h>
#include <arc_pwr.h>

unsigned delay_tick(unsigned count);
void delay_ms(int usec);

#ifndef CONFIG_AML_ROMBOOT_SPL
#define UART_PORT_CONS UART_PORT_AO
static int serial_set_pin_port(unsigned port_base)
{
    setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}
#endif


static void serial_clr_err(void)
{
    if(readl(P_UART_STATUS(UART_PORT_CONS))&(UART_STAT_MASK_PRTY_ERR|UART_STAT_MASK_FRAM_ERR))
	    setbits_le32(P_UART_CONTROL(UART_PORT_CONS),UART_CNTL_MASK_CLR_ERR);
}

void __udelay(int n)
{	
	int i;
	for(i=0;i<n;i++)
	{
	    asm("mov r0,r0");
	}
}

void uart_reset()
{
	if(arc_param->serial_disable)
		return;

	if(readl(P_UART_REG5(UART_PORT_CONS))==0){
	unsigned uart_cfg = readl(P_UART_CONTROL(UART_PORT_CONS));
	unsigned uart_misc = readl(P_AO_UART_MISC);
	setbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	clrbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	__udelay(100);
	writel(uart_cfg,P_UART_CONTROL(UART_PORT_CONS));
	writel(uart_misc,P_AO_UART_MISC);
	}else{
	unsigned uart_cfg = readl(P_UART_CONTROL(UART_PORT_CONS));
	unsigned uart_misc = readl(P_AO_UART_MISC);
	unsigned new_baudrate=readl(P_UART_REG5(UART_PORT_CONS));
	setbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	clrbits_le32(P_AO_RTI_GEN_CNTL_REG0,1<<17);
	__udelay(100);
	writel(uart_cfg,P_UART_CONTROL(UART_PORT_CONS));
	writel(uart_misc,P_AO_UART_MISC);
	writel(new_baudrate,P_UART_REG5(UART_PORT_CONS));
	__udelay(100);
	}
}
static void serial_init(unsigned set,unsigned tag)
{
	if(arc_param->serial_disable)
		return;

    /* baud rate */
    /*unsigned baud_para=0;*/
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
extern struct ARC_PARAM *arc_param;

static void serial_putc(const char c)
{
	if(arc_param->serial_disable)
		return;

    if (c == '\n'){
        while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL));
        writel('\r', P_UART_WFIFO(UART_PORT_CONS));
    }
    /* Wait till dataTx register is not full */
    while ((readl(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL));
    writel(c, P_UART_WFIFO(UART_PORT_CONS));
    /* Wait till dataTx register is empty */
}

void f_serial_puts(const char *s)
{
	if(arc_param->serial_disable)
		return;
    while (*s != '\0') {
        serial_putc(*s++);
    }
}

void wait_uart_empty()
{
	if(arc_param->serial_disable)
		return;

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

void serial_put_hex(unsigned int data,unsigned bitlen)
{
	int i;
	if(arc_param->serial_disable)
		return;
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

