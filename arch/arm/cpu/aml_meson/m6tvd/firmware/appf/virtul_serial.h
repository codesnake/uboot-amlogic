#define IO_AOBUS_BASE       0xf3100000
#define AOBUS_REG_OFFSET(reg)   ((reg) )
#define AOBUS_REG_ADDR(reg)        (IO_AOBUS_BASE + AOBUS_REG_OFFSET(reg))
#define P_AO_UART_WFIFO                AOBUS_REG_ADDR((0x01 << 10) | (0x30 << 2))      ///../ucode/c_always_on_pointer.h
#define P_AO_UART_RFIFO                AOBUS_REG_ADDR((0x01 << 10) | (0x31 << 2))      ///../ucode/c_always_on_pointer.h
#define P_AO_UART_CONTROL              AOBUS_REG_ADDR((0x01 << 10) | (0x32 << 2))      ///../ucode/c_always_on_pointer.h
#define P_AO_UART_STATUS               AOBUS_REG_ADDR((0x01 << 10) | (0x33 << 2))      ///../ucode/c_always_on_pointer.h
#define P_AO_UART_MISC                 AOBUS_REG_ADDR((0x01 << 10) | (0x34 << 2))      ///../ucode/c_always_on_pointer.h
#define P_AO_UART_REG5                 AOBUS_REG_ADDR((0x01 << 10) | (0x35 << 2))      ///../ucode/c_always_on_pointer.h
#define UART_STAT_MASK_TFIFO_FULL               (1<<21)
#define UART_STAT_MASK_TFIFO_EMPTY                             (1<<22)


static void delay_tick(int count)
{
       int i;
       while(count > 0){
               for(i = 0; i < 100; i++);
               count--;
       };
//       return count;
}
static void serial_putc(const char c)
{
    if (c == '\n'){
        while ((readl(P_AO_UART_STATUS) & UART_STAT_MASK_TFIFO_FULL));
        writel('\r', P_AO_UART_WFIFO);
    }
    /* Wait till dataTx register is not full */
    while ((readl(P_AO_UART_STATUS) & UART_STAT_MASK_TFIFO_FULL));
    writel(c, P_AO_UART_WFIFO);
    /* Wait till dataTx register is empty */
}
void v_serial_puts(const char *s)
{
    while (*s != '\0') {
        serial_putc(*s++);
    }
    delay_tick(32*1000);
}
void v_wait_uart_empty()
{
    unsigned int count=0;
    do{
        if((readl(P_AO_UART_STATUS) & UART_STAT_MASK_TFIFO_EMPTY) == 0)
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
//#define v_prints(s) {v_serial_puts(s);v_wait_uart_empty();}
#define v_prints(s)

