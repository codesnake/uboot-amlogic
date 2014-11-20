#include "config.h"
OUTPUT_FORMAT("elf32-littlearc", "elf32-littlearc", "elf32-littlearc")
OUTPUT_ARCH(arc)
ENTRY(_start)
SECTIONS
{
        . = RAM_START;

        . = ALIGN(4);
        .text   :
        {
                *start_arc.o  (.text)
                 *(.text)
        }

        . = ALIGN(4);
        .rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
        .data : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data*))) }
     	 .sdata          :
			  {
  	 		 	__SDATA_BEGIN__ = .;
   				 *(.sdata .sdata.* .gnu.linkonce.s.*)
  			}

        . = ALIGN(4);
        __bss_start = .;
        .bss : { *(.bss) }
        . = ALIGN(4);
        _end = .;
}
ASSERT(_end <= _STACK_END-1024, " too big");

