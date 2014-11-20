OUTPUT_FORMAT("elf32-littlearc", "elf32-littlearc", "elf32-littlearc")
OUTPUT_ARCH(arc)
ENTRY(_start)
SECTIONS
{
	. = 0;
	
	. = ALIGN(4);
	.start      :
	{
	  *start_arc*.o	(.text)
	
	}
	. = ALIGN(4);
	.text			:
	{
		*  (.text)
	}
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
	.data : { *(.data) }
	 .sdata          :
  {
    __SDATA_BEGIN__ = .;
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  }

	_end = .;
	. = ALIGN(4);
	_bssstart = . ;
	.bss : { *(.bss) }
	. = ALIGN(4);
	_bssend = . ;
}