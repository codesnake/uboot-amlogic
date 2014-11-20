#include <config.h>
#include <asm/arch/romboot.h>
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = CONFIG_DDR_INIT_ADDR;
	
	. = ALIGN(4);
	.start      :
	{
	  *ddr_init_start.o	(.text)
	
	}
#ifndef CONFIG_M6_SECU_BOOT
	. =  CONFIG_DDR_INIT_ADDR+0x1b0;
	.magic :{
	  *magic.o(.text)
	}
	
	. = CONFIG_DDR_INIT_ADDR+0x200;
	.acs_entry :{
	  *acs_entry.o(.text)
	}
#endif
	. = ALIGN(4);
	.setting		:
	{
		* (.setting)
	}
		
	
	. = ALIGN(4);
	.text			:
	{
		*  (.text)
	}
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
	.data : { *(.data) }
	
	_end = .;
	. = ALIGN(4);
	_bssstart = . ;
	.bss : { *(.bss) }
	. = ALIGN(4);
	_bssend = . ;
}
ASSERT(_end <= CONFIG_DDR_INIT_ADDR + READ_SIZE, "ROM BOOT bootstrap too big");
ASSERT(_bssend <= CONFIG_DDR_INIT_ADDR + READ_SIZE, "ROM BOOT bss  too big");
