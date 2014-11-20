#include <config.h>
#include <asm/arch/cpu.h>

#if CONFIG_ENABLE_DEBUGROM
SPL_STATIC_FUNC void debug_rom(char * file,unsigned line)
{
}
#else
#define debug_rom(a,b) 
#endif 