#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/uart.h>
SPL_STATIC_FUNC int serial_set_pin_port(unsigned port_base)

{
    //UART in "Always On Module"
    //GPIOAO_0==tx,GPIOAO_1==rx
    setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}