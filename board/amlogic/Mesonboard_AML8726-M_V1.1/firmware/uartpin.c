#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/uart.h>
SPL_STATIC_FUNC int serial_set_pin_port(unsigned port_base)
{
    if(port_base==UART_PORT_0)
    {
        setbits_le32(P_PERIPHS_PIN_MUX_3,((1 << 23) | (1 << 24)));
        return 0;
    }
    if(port_base==UART_PORT_1)
    {
        clrbits_le32(P_PERIPHS_PIN_MUX_6, ((1 << 11) | (1 << 12)));
        setbits_le32(P_PERIPHS_PIN_MUX_5, ((1 << 23) | (1 << 24)));
        return 0;
    }
    return -1;
}