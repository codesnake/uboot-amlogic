/*
 * aml6236-m nand spl
 * @author jerry.yu
 *
 */
/* These define will be replace with */
#include <config.h>
#include <asm/arch/romboot.h>
.globl _start
_start: 
	b	reset
	ldr	pc,_undefined_instruction
	ldr	pc,_software_interrupt
	ldr	pc,_prefetch_abort
	ldr	pc,_data_abort
	ldr	pc,_not_used
	ldr	pc,_irq
	ldr	pc,_fiq

#ifdef CONFIG_M6_SECU_BOOT	
	.global __ihw_setting
__ihw_setting:
  .space 0x1b0 - (. - _start)
  .org 0x1b0
.global __magic_word
__magic_word:
  .word MAGIC_WORD1
  .word MAGIC_WORD2
  .word 0
  .word 0
  .word 0xfe0b0101
  .word 0x3ec1e07f
  .word 0xa9e00000
  .word 0x00000075
  .space 0x1fe - (. - _start)
  .short 0xaa55
#ifdef CONFIG_ACS
.globl _acs_start
_acs_start:
  .word __acs_set
#endif
#endif

reset:
#if 0
	ldr     r0,=0xda004004
    ldr     r1,=0x80000510
    str     r1,[r0]
    
    ldr     r0,=0xc8100014
    mov     r1,#0x4000
    str     r1,[r0]

    
    ldr     r0,=0xc1109900
    mov     r1,#0
    str     r1,[r0]
    wfi
#endif
    
    ldr     r0,=0xc1109900
    mov     r1,#0
    str     r1,[r0]
    
#ifdef CONFIG_ROMBOOT_DEBUG
    ldr     r0,=0xc1109900
    mov     r1,#0
    str     r1,[r0]
	wfi
#endif
    dmb
    isb
  	ldr     sp, __ROM_STACK_END  // setup stack pointer 
	/*
		Whether I am running in SRAM 
    */
    adr r0, _start       		// r0 <- current position of code
    ldr r1, _AHB_SRAM_BASE  // test if we run from flash or RAM
    cmp r0, r1           		// dont reloc during debug
    beq sdram_running				// skip relocate 
    ldr r2,_READ_SIZE
    bl  ipl_memcpy
    add pc,r1,#( sdram_running - _start )
sdram_running:
	ldr r3,__BSS_START
	ldr r4,__BSS_END
	mov r5,#0
clr_bss:
	cmp r3,r4
	beq clr_bss_end
	str r5,[r3]
	add r3,r3,#4
	b 	clr_bss
clr_bss_end:

    /*
        Set vector base
    */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	mcr	p15, 0, r0, c1, c0, 0
	adr r0, _start
    mcr p15,0,r0,c12,c0,0
    ldr r0,__TEXT_BASE
    ldr r1,__TEXT_SIZE
    bl	main 
#ifdef CONFIG_MESON_TRUSTZONE
/* #ifdef CONFIG_MESON_SECUREBOOT_WITHOUT_DECRYPT*/
    cmp r0, #0x0
	ldreq pc, __SECURE_OS_BASE
/* #else
	ldr pc, __SECURE_OS_BASE
#endif	
*/
#endif       
#ifndef CONFIG_IMPROVE_UCL_DEC    
    ldr pc,__TEXT_BASE
#else
	ldr pc,__TEXT_BASE_UCL_DEC
#endif    
_AHB_SRAM_BASE:
    .word AHB_SRAM_BASE
_READ_SIZE:
	.word READ_SIZE
__TEXT_BASE:
    .word TEXT_BASE
#ifdef CONFIG_IMPROVE_UCL_DEC        
__TEXT_BASE_UCL_DEC:
    .word UCL_TEXT_BASE
#endif    
__TEXT_SIZE:
#ifdef CONFIG_BOARD_SIZE_LIMIT
    .word CONFIG_BOARD_SIZE_LIMIT
#else
#ifdef CONFIG_MESON_TRUSTZONE
	.word CONFIG_SECURE_UBOOT_SIZE - READ_SIZE
#elif defined CONFIG_AML_SECURE
   .word 0x70000 - READ_SIZE
#else
   .word 0x60000 - READ_SIZE
#endif    
#endif  

#ifdef CONFIG_MESON_TRUSTZONE
__SECURE_OS_BASE:
	.word SECURE_OS_DECOMPRESS_ADDR
#endif	

__ROM_STACK_END:
	.word ROM_STACK_END
__BSS_START:
	.word _bssstart
__BSS_END:
	.word _bssend
    
    .global	romboot_info
    .type   romboot_info, %object
	.size   romboot_info, 4
romboot_info:
		.word   ROM_BOOT_INFO
.global	magic_info
  .type   magic_info, %object
	.size   magic_info, 4
magic_info:
	.word  __magic_word
    .global	__load_table
    .type   __load_table, %object
	.size   __load_table, 24
__load_table:
  .space 24
	
    .global	ipl_memcpy
	.type	ipl_memcpy, %function
ipl_memcpy:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	str	r4, [sp, #-4]!
	movs	r2, r2, lsr #2
	beq	.L4
	mov	ip, #0
	mov	r3, ip
.L3:
	mov	ip, ip, asl #2
	ldr	r4, [r1, ip]
	str	r4, [r0, ip]
	add	r3, r3, #1
	mov	ip, r3
	cmp	r3, r2
	bcc	.L3
.L4:
	ldmfd	sp!, {r4}
	bx	lr
  .global	memset
	.type	memset, %function
memset:
	@ args: address, value, count
  str r1,[r0]
  add r0,r0,#4
  sub r2,r2,#4
  cmp r2,#0
  BNE memset
	bx  lr
	
undefined_instruction: 
	mov r0,#1
	b fail_sleep
software_interrupt:	
	mov r0,#2
	b fail_sleep
prefetch_abort:	
	mov r0,#3
	b fail_sleep
data_abort:
	mov r0,#4
	b fail_sleep		
	
not_used:		
	mov r0,#5
	b fail_sleep
irq:	
	mov r0,#6
	b fail_sleep		
fiq:
	mov r0,#7
	b fail_sleep	
fail_sleep:
	ldr     sp, =_STACK_END  // setup stack pointer 	hisun 20120208		
    mov     r1,lr
	bl do_exception
	wfi

_undefined_instruction: .word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq
_pad:			.word 0x12345678 /* now 16*4=64 */
.global _end_rom_spl
_end_rom_spl:


#ifdef CONFIG_M6_TEST_CPU_SWITCH
.global get_cpu_id
    .type   get_cpu_id, %function
get_cpu_id:
     MRC     p15, 0, r0, c0, c0, 5   @ Read CPU ID register
     AND     r0, r0, #0x03           @ Mask off, leaving the CPU ID field
     BX      lr
     .size   get_cpu_id, .-get_cpu_id
#endif
