
#include <config.h>
#include <version.h>
#include <asm/arch/romboot.h>
    PRESERVE8

Mode_USR        EQU     0x10
Mode_FIQ        EQU     0x11
Mode_IRQ        EQU     0x12
Mode_SVC        EQU     0x13
Mode_ABT        EQU     0x17
Mode_UND        EQU     0x1B
Mode_SYS        EQU     0x1F ; available on ARM Arch 4 and later

I_Bit           EQU     0x80 ; when I bit is set, IRQ is disabled
F_Bit           EQU     0x40 ; when F bit is set, FIQ is disabled

; --- System memory locations


        AREA   INIT, CODE, READONLY   ; name this block of code

        ENTRY

        EXPORT  _start
        IMPORT ||Image$$BSS$$ZI$$Base||
        IMPORT ||Image$$BSS$$ZI$$Limit||
_start   FUNCTION
				ldr 		r0,=||Image$$BSS$$ZI$$Base||
				ldr 		r1,=||Image$$BSS$$ZI$$Limit||
				mov			r2,#0
clr_bss
				str     r2,[r0]
				add			r0,r0,#4
				cmp		  r0,r1
				bne			clr_bss
				ldr 		sp,__ROM_STACK_END ; setup stack pointer
				;
				;Check whether we are running int SRAM
				;
        adr r0, _start       		; r0 <- current position of code
			  ldr r1, __AHB_SRAM_BASE   ; test if we run from flash or RAM
			  cmp r0, r1           		  ; dont reloc during debug
			  beq sdram_running				  ; skip relocate 
			  ldr r2,__READ_SIZE
			  bl  ipl_memcpy
			  add pc,r1,#( sdram_running - _start )
sdram_running
			  bl	lowlevel_init
#ifndef AML_DEBUG_ROM
;				WFI
			  ldr   r0,__TEXT_BASE
			  ldr   r1,__TEXT_SIZE
			  bl	relocate_init
			  ldr   pc,__TEXT_BASE
#endif  
					ENDFUNC

;__ROM_STACK_END  DCD  ROM_STACK_END
;__AHB_SRAM_BASE  DCD  AHB_SRAM_BASE
;__READ_SIZE			 DCD  READ_SIZE
;__AHB_SRAM_END   DCD  AHB_SRAM_BASE + READ_SIZE
;__TEXT_BASE			 DCD  TEXT_BASE
;__TEXT_SIZE			 DCD  0x60000 - READ_SIZE
;P_ASSIST_POR_CONFIG DCD 0xc1107d54

	AREA ||.magic||, DATA,READONLY, ALIGN=0
	
__magic_word
  DCDU MAGIC_WORD1
  DCDU MAGIC_WORD2
  DCDU 0
  DCDU 0
  DCDU 0xfe0b0101
  DCDU 0x3ec1e07f
  DCDU 0xa9e00000
  DCDU 0x00000075
  SPACE 0x1fe-0x1b0 - (. - __magic_word)
 	DCWU 0xaa55				
	AREA ||.rodata||, DATA,READONLY, ALIGN=0
romboot_info     
	DCDU  C_ROM_BOOT_DEBUG
magic_info
	DCDU  __magic_word
__ROM_STACK_END  
	DCDU  ROM_STACK_END
__AHB_SRAM_BASE  
	DCDU  AHB_SRAM_BASE            	
__READ_SIZE			 
	DCDU  READ_SIZE                
__TEXT_BASE			 
	DCDU  TEXT_BASE                
__TEXT_SIZE			 
	DCDU  0x60000 - READ_SIZE      

				IMPORT ipl_memcpy 		[CODE]
				IMPORT relocate_init	[CODE]
				IMPORT lowlevel_init 	[CODE]
				EXPORT romboot_info   [DATA]
				EXPORT magic_info   	[DATA]
        END                                 ; mark the end of this file

