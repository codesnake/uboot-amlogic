; Copyright (C) 2010 ARM Limited                           
;
; This software is provided 'as-is', without any express or implied
; warranties including the implied warranties of satisfactory quality, 
; fitness for purpose or non infringement.  In no event will  ARM be 
; liable for any damages arising from the use of this software.
;
; Permission is granted to anyone to use, copy and modify this software for 
; any purpose, and to redistribute the software, subject to the following 
; restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.                                       
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.

	EXPORT dmb
	EXPORT wfi
	EXPORT wfe
	EXPORT sev
	EXPORT copy_words
	EXPORT appf_memcpy
	EXPORT appf_memset

	EXPORT dsb
	EXPORT read_mpidr 
        EXPORT va_to_pa
        EXPORT pa_to_va
	EXPORT read_sctlr
        EXPORT write_sctlr
	EXPORT read_actlr
        EXPORT write_actlr
	EXPORT read_prrr
	EXPORT read_nmrr
	EXPORT read_mvbar
	EXPORT update_mvbar
	EXPORT dbg_wait
	EXPORT update_offset
	EXPORT reloc_addr
        

	AREA APPF, CODE


dmb	FUNCTION
	dmb
	bx	lr
	ENDFUNC
        	
wfi	FUNCTION
	wfi
	bx	lr
	ENDFUNC

wfe	FUNCTION
	wfe
	bx	lr
	ENDFUNC

sev	FUNCTION
	sev
	bx	lr
	ENDFUNC

	; This function takes three arguments
	; r0: Destination start address (must be word aligned)
	; r1: Source start address (must be word aligned)
	; r2: Number of words to copy
copy_words	FUNCTION
	push {r0,r1,r2,r3,lr}
	cmp	r2, #0
	bxeq	lr
0	ldr	r3, [r1], #4
	str	r3, [r0], #4
	subs	r2, #1
	bne	%b0
	pop {r0,r1,r2,r3,pc}
	ENDFUNC	
	
appf_memcpy	FUNCTION
	cmp	r2, #0
	bxeq	lr
0	ldrb	r3, [r1], #1
	strb	r3, [r0], #1
	subs	r2, #1
	bne	%b0
	bx	lr
	ENDFUNC

appf_memset	FUNCTION
	cmp	r2, #0
	bxeq	lr
0	strb	r1, [r0], #1
	subs	r2, #1
	bne	%b0
	bx	lr
	ENDFUNC

update_mvbar FUNCTION
 	mrc	p15, 0, r0, c12, c0, 0 ; vector base	
;  mcr p15, 0, r0, c12, c0, 1 ; set monitor vector base.
	ENDFUNC

read_mvbar	FUNCTION
	mrc	p15, 0, r0, c12, c0, 1 ; monitor vector base
	bx	lr
	ENDFUNC


	AREA APPF_ENTRY_POINT_CODE, CODE

        ; Functions we need in the runtime entry point, i.e. before we switch pagetables,
        ; are placed in this area.
        
dsb	FUNCTION
	dsb
	bx	lr
        ENDFUNC
        
read_mpidr FUNCTION
	mrc	p15, 0, r0, c0, c0, 5
	bx	lr
	ENDFUNC

va_to_pa FUNCTION	; Note: assumes conversion will be successful!
    mov	r1, r0
	  mcr	p15, 0, r0, c7, c8, 1	; Priv Write Current World VA-PA
	  mrc	p15, 0, r0, c7, c4, 0	; Get PA
        bfc	r0, #0, #12		; We want top bits of translated addr
        bfc	r1, #12, #20		; plus bottom bits of input addr
        orr	r0, r0, r1
        bx	lr
    ENDFUNC

;pa_to_va FUNCTION
;	mov	 r1, r0
;	bfc  r0,#28,#4
;	mov  r1,#0
;	movt r1,#0xF000
;	orr r0,r0,r1
;	bx lr
;	ENDFUNC	


update_offset FUNCTION
	push {r0,r1,lr}
	ldr  r0,=reloc_offset
	adr r1,reloc_offset
	sub r1, r1,r0
	str r1, reloc_offset
	pop  {r0,r1,pc}
reloc_offset
	DCD 0
	ENDFUNC

reloc_addr FUNCTION
	push {r1,lr}
	ldr r1,reloc_offset
	add r0,r0,r1
	pop {r1,pc}
	ENDFUNC
	
;va_to_pa FUNCTION 
;  push {r1,lr}
;	mov r1,pc
;	bfc r1,#0,#28
;	bfc r0,#28,#4
;	orr r0,r0,r1
;	pop {r1,pc}
;	ENDFUNC

read_sctlr FUNCTION        
    mrc     p15, 0, r0, c1, c0, 0
    bx      lr
    ENDFUNC

write_sctlr FUNCTION
    mcr     p15, 0, r0, c1, c0, 0
    bx      lr
    ENDFUNC

read_actlr FUNCTION        
    mrc     p15, 0, r0, c1, c0, 1
    bx      lr
    ENDFUNC

write_actlr FUNCTION
    mcr     p15, 0, r0, c1, c0, 1
    bx      lr
    ENDFUNC

;read primary region remap register
read_prrr FUNCTION        
    mrc     p15, 0, r0, c10, c2, 0
    bx      lr
    ENDFUNC

;read normal memory remap register
read_nmrr FUNCTION        
    mrc     p15, 0, r0, c10, c2, 1
    bx      lr
    ENDFUNC

dbg_wait FUNCTION
     push {r0,r4,lr}
     ;need disable interrupts before wfi
;     mrs	r4, CPSR
;     cpsid	if
     mov r0,#0
dead_lock
		cmp r0,#0
		beq dead_lock
		pop {r0,r4,pc}
	END
