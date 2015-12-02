    .global _start
;; -- start of program section --------------------------
    .text
;; -- auto-vector space, jump to start of code ---------- 

	.global enable_irq
	.global disable_irq
/*-------------------------------------------------------------- 
 * Save caller saved registers (scratch registers) ( r0 - r12 )
 * Registers are pushed / popped in the order defined in struct ptregs
 * in asm/ptrace.h
 *-------------------------------------------------------------*/
.macro  SAVE_CALLER_SAVED
    st.a    r0, [sp, -4]
;    st.a    r1, [sp, -4]
    st.a    r2, [sp, -4]
  /*  st.a    r3, [sp, -4]
    st.a    r4, [sp, -4]
    st.a    r5, [sp, -4]
    st.a    r6, [sp, -4]
    st.a    r7, [sp, -4]
    st.a    r8, [sp, -4]
    st.a    r9, [sp, -4]
    st.a    r10, [sp, -4]
    st.a    r11, [sp, -4]
    st.a    r12, [sp, -4]*/
    st.a    blink, [sp, -4]
/*    lr      r0, [lp_start]
    st.a    r0, [sp, -4]
    lr      r0, [lp_end]
    st.a    r0, [sp, -4]
    mov     r0, lp_count
    st.a    r0, [sp, -4]*/
.endm

/*-------------------------------------------------------------- 
 * Restore caller saved registers (scratch registers)
 *-------------------------------------------------------------*/
.macro RESTORE_CALLER_SAVED
/*    ld.ab   r0, [sp, 4]
    mov     lp_count,r0
    ld.ab   r0, [sp, 4]
    sr      r0, [lp_end]
    ld.ab   r0, [sp, 4]
    sr      r0, [lp_start]*/
    ld.ab   blink, [sp, 4]
/*    ld.ab   r12, [sp, 4]
    ld.ab   r11, [sp, 4]
    ld.ab   r10, [sp, 4]
    ld.ab   r9, [sp, 4]
    ld.ab   r8, [sp, 4]
    ld.ab   r7, [sp, 4]
    ld.ab   r6, [sp, 4]
    ld.ab   r5, [sp, 4]
    ld.ab   r4, [sp, 4]
    ld.ab   r3, [sp, 4]*/
    ld.ab   r2, [sp, 4]
;    ld.ab   r1, [sp, 4]
    ld.ab   r0, [sp, 4]
.endm

_start:

	b test_start
	    ; reserved for arm reboot code. (0x280 bytes)
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    
	test_start:
		; Initialize the register file.  
	; Initialize general purpose registers
		mov r0, 0x00000000
		mov r1, 0x00000000
		mov r2, 0x00000000
		mov r3, 0x00000000
		mov r4, 0x00000000
		mov r5, 0x00000000
		mov r6, 0x00000000
		mov r7, 0x00000000
		mov r8, 0x00000000
		mov r9, 0x00000000
		mov r10, 0x00000000
		mov r11, 0x00000000
		mov r12, 0x00000000
		mov r13, 0x00000000
		mov r14, 0x00000000
		mov r15, 0x00000000
		mov r16, 0x00000000
		mov r17, 0x00000000
		mov r18, 0x00000000
		mov r19, 0x00000000
		mov r20, 0x00000000
		mov r21, 0x00000000
		mov r22, 0x00000000
		mov r23, 0x00000000
		mov r24, 0x00000000
		mov r25, 0x00000000
		mov r26, 0x00000000
		mov r27, 0x00000000
		mov r28, 0x00000000
	

	
		mov  sp, 0x8000 ;=_STACK_END
		NOP
		NOP

		;clear bss segment
		mov r0,0
		mov r1, __bss_start
clear_loop:
		st  r0,[r1]
		add r1,r1,4
		cmp r1,_end
		bne clear_loop


	mov r11, 0xc8100080
	st 0, [r11]//clean intr mask
	mov r11, 0xc810008c
	st 0x0ffff, [r11]//clean intr status

	mov r11, 0x206 //IRQ_PRIORITY
	mov r12, 0x40b //IRQ_SELECT
;	mov r10, 0x40d //IRQ_TRIGGER set level or pulse interrupt
	sr 19, [r12]
	sr 0, [r11]  //p0 level
	mov r10, proc_fiq
	st r10, [0x4c]

	sr 18, [r12]
	sr 1, [r11]  //p1 level
	mov r10,proc_irq
	st r10, [0x48]
	kflag 0x2 //set itr priority
	seti

	mov r11, 0xa
	lr r11,[r11]
	mov r12,0xc8100004
	st r11, [r12]

	.ifdef No_C_code
	.else
		bl		main
	.endif
		nop
		nop
		nop
		sleep
		nop
		nop
	
	test_end:
	mov r12, 0x8888
	mov r11, 0xc8100004
	st  r12, [r11]
		j test_end
	
	arc_fail:
	mov r12, 0x7777
	mov r11, 0xc8100004
	st  r12, [r11]
		j arc_fail



proc_irq:
	
SAVE_CALLER_SAVED
	NOP
	mov r0, 0xc1109844//clean intr status
	mov r2,0xffffffff
	st r2,[r0]
;	sync
;	MOV      r10, r0
	NOP
	bl Process_Cec_Clk_Irq          ; irq5
	NOP
;	MOV      r0, r10
	NOP

/*
	;BL		 read_int_ack
	ld r2, [0xc810008c]
	cmp r2,0
	jne irq1

	;mov r2,0xffffffff
	st 0x2,[0xc1109844]

	bl Process_Cec_Clk_Irq		; irq5
	j irq2
irq1:
;	mov r0, 0xc810008c//clean intr status
;	mov r2,0xffffffff
	st  0x8,[0xc810008c]
;	mov r0, 0xc8100004
	mov r2, 0x1234abcd
	st  r2,[0xc8100004]

irq2:
*/
RESTORE_CALLER_SAVED
;	j proc_irq
	rtie

proc_fiq:
SAVE_CALLER_SAVED
	mov r2,0xffffffff
	st  r2,[0xc810008c]//clean intr status
	mov r2, 0x1234abcd
	st  r2,[0xc8100004]
RESTORE_CALLER_SAVED
	rtie

enable_irq:
	seti


disable_irq:
	clri

