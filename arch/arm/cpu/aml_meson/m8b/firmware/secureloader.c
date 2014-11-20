#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>

#ifndef memcpy
#define memcpy ipl_memcpy:
#endif


void secure_load(unsigned addr, unsigned size)
{	
	memcpy((unsigned char*)SECURE_OS_COMPRESS_ADDR, (unsigned char*)addr, size);
}