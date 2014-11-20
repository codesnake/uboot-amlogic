    .global _start
;; -- start of program section --------------------------
    .text
;; -- auto-vector space, jump to start of code ---------- 

_start:

	.long  0x00000284
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
		j test_end
	
	arc_fail:
		j arc_fail

