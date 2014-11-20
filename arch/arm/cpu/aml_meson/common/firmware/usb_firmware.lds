#include <config.h>
#include <asm/arch/romboot.h>
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = CONFIG_USB_SPL_ADDR;
	
	. = ALIGN(4);
	.start      :
	{
	  *usb_fw_start*.o	(.text)
	
	}
#ifndef CONFIG_M6_SECU_BOOT
	. =  CONFIG_USB_SPL_ADDR+0x1b0;
	.magic :{
	  *magic.o(.text)
	}
	
	. = CONFIG_USB_SPL_ADDR+0x200;
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
          *(.ARM.extab* .gnu.linkonce.armextab.*)		
	}
	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
	
	.ARM.exidx : {*(.ARM.exidx* .gnu.linkonce.armexidx.*)}      	
	.got : {*(.got* .gnu.linkonce.got.plt.*)}
	
	. = ALIGN(4);
	 __data_start = .;
     .data : { *(.data*)}
     __data_end = .;
	
	_end = .;
	. = ALIGN(4);

	_bssstart = . ;
	.bss : { *(.bss) }
	. = ALIGN(4);
	_bssend = . ;
}
ASSERT(_end <= CONFIG_USB_SPL_ADDR + READ_SIZE, "ROM BOOT bootstrap too big");
#ifndef CONFIG_MESON_TRUSTZONE
ASSERT(_bssend <= CONFIG_USB_SPL_ADDR + READ_SIZE, "ROM BOOT bss  too big");
#endif
