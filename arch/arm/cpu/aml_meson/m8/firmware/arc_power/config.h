#ifndef _CONFIG_H_
#define _CONFIG_H_
/*
	Memory layout for EM4:
	0~0x7fff: em4 run space
	0x10000~0x101ff: trainning store data (It just use 2 words + 32*2 words = 0x108)
	0x10200~0x102ff: save param from kernel/uboot to comunicat with em4
	0x10300~0x103ff: save DMC_SEC_* group registers value for ddr
	             0x10300 save DMC_SEC_CTRL value
	             0x10304 save DMC_SEC_KEY0 value
	             0x10308 save DMC_SEC_KEY1 value
*/

#define RAM_START       0x000000
#define RAM_SIZE        32*1024
#define RAM_END         (RAM_START+RAM_SIZE)
#define _STACK_END      RAM_END
#define ROMBOOT_START  RAM_START
#define ARC_PARAM_ADDR  0x10200

#define CONFIG_BAUDRATE 115200
#define CONFIG_CRYSTAL_MHZ 24

#ifndef __ASSEMBLY__
void serial_init_with_clk(unsigned clk);
void serial_put_dword(unsigned int data);
void serial_put_dec(unsigned int data);
void serial_puts(const char *s);
void serial_putc(const char c);
int serial_tstc(void);
int serial_getc(void);
void serial_init(unsigned set,unsigned tag);
void serial_put_hex(unsigned int data,unsigned bitlen);
void f_serial_puts(const char *s);
void k_delay(void);
unsigned get_uart_ctrl(void);
unsigned delay_tick(unsigned count);
#define OUT_INIT()      
#define OUT_PUT(a)      
#endif
#endif
