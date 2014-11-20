#include <config.h>
/*#include <asm/arch/romboot.h>*/
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(pwr_reboot)
RBT_SDRAM_BASE = 0;
SECTIONS
{
	. = RBT_SDRAM_BASE;
	. = ALIGN(4);
	.reboot     :
	{
	  *reboot.o	(.text)
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

