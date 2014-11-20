#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/timer.h>

SPL_STATIC_FUNC int timer_init(void)
{
    WRITE_CBUS_REG_BITS(PREG_CTLREG0_ADDR,CONFIG_CRYSTAL_MHZ,4,5);
    //set timer E usec base
  	clrsetbits_le32(P_ISA_TIMER_MUX,0x7<<8,0x1<<8);
	return 0;
}
SPL_STATIC_FUNC unsigned get_utimer(unsigned base)
{
    return TIMERE_SUB(TIMERE_GET(),base);
}

SPL_STATIC_FUNC void __udelay(unsigned long usec)
{
    unsigned long base=TIMERE_GET();
    while(TIMERE_SUB(TIMERE_GET(),base)<usec);
    
}

