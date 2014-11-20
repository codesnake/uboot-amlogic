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

	AREA  APPF_ENTRY_POINT_CODE, CODE, ALIGN=10
	PRESERVE8

APPF_FUNCTION_INITIALIZE	EQU 0

	EXPORT appf_entry_point
	EXPORT appf_reset_entry_point
        EXPORT appf_return_to_os
	EXPORT appf_runtime_call_flat_mapped
        EXPORT appf_ttbr0
	EXPORT appf_ttbcr
	EXPORT flat_mapped

        IMPORT appf_runtime_init
        IMPORT appf_platform_get_stack_pointer
        IMPORT appf_runtime_call
	IMPORT appf_warm_reset
	IMPORT invalidate_icache_v7_pou
	IMPORT invalidate_dcache_v7
	IMPORT read_sctlr
	IMPORT write_sctlr

SCTLR_I EQU (1<<12)
SCTLR_Z EQU (1<<11)

	
appf_entry_point
	; This is the entry point from the OS

	; TODO: Decide how were calling the firmware - BL or SMC
	;  - for now we dont have SMC
	; Save some stuff on the OS stack (preserve 8 byte alignment)
	push	{r4-r12, lr}		
				;---------------------------------------
				;save A9 registers all mode
				mrs r4, spsr
				push {r4}
				
        cps #1 ;switch to FIRQ mode
        mov r4,sp
        mov r5,lr
        mrs r6,spsr
        cps #3 ;switch to SVC mode
        push {r4,r5,r6}
        
       cps #1 ;switch to FIRQ mode
        mov r4,r8
        mov r5,r9
        mov r6,r10
        mov r7,r11
        cps #3 ;switch to SVC mode
        push {r4,r5,r6,r7}
        cps #1 ;switch to FIRQ mode
        mov r4,r12
        cps #3 ;switch to SVC mode
        push {r4}
        
				
        cps #2 ;switch to IRQ mode
        mov r4,sp
        mov r5,lr
        mrs r6,spsr
        cps #3 ;switch to SVC mode
        push {r4,r5,r6}

        cps #6 ;switch to monitor user mode
        mov r4,sp
        mov r5,lr
        mrs r6,spsr
        cps #3 ;switch to SVC mode
        push {r4,r5,r6}
        
        cps #6 ;switch to FIRQ mode
        mov r4,r8
        mov r5,r9
        mov r6,r10
        mov r7,r11
        cps #3 ;switch to SVC mode
        push {r4,r5,r6,r7}
        cps #6 ;switch to FIRQ mode
        mov r4,r12
        cps #3 ;switch to SVC mode
        push {r4}
        
        
        cps #7 ;switch to Abort mode
        mov r4,sp
        mov r5,lr
        mrs r6,spsr
        cps #3 ;switch to SVC mode
        push {r4,r5,r6}        ; Preserve arguments
        
        cps #0Xb ;switch to undef mode
        mov r4,sp
        mov r5,lr
        mrs r6,spsr
        cps #3 ;switch to SVC mode
        push {r4,r5,r6}        ; Preserve arguments
        
        cps #0Xf ;switch to sys mode
        mov r4,sp
        mov r5,lr
        mrs r6,spsr
        cps #3 ;switch to SVC mode
        push {r4,r5,r6}        ; Preserve arguments

        mov	r5, r0				;save arguments
        mov	r6, r1
        mov	r7, r2
        mov	r8, r3     
		;		bl dbg_wait
next_run				
			
				; cache flush
				cpsid	if

				; r0, r1, r2, r3 args from pwrtest_entry(). r1==1, l2 cache disable
	  			bl v7_flush_cache_all_ref ; disable l1 i/d cache in it

				; switch mmu table
				;1. disable dcache and i cache; this op disable all level cache @v7

;				mrc p15,0,r12,c1,c0,0; read control
;				bic	r12, r12, #0x1000			;@ ...i............
;				bic	r12, r12, #0x0006			;@ .............ca.
;				mcr p15,0,r12,c1,c0,0; read control
;	 			mov r12,#0
;				mcr p15, 0, r12, c7, c10, 4
 				dsb
 				isb
	      ; Is this runtime initialization?
	cmp	r0, #APPF_FUNCTION_INITIALIZE
	bne	%f1

	; Runtime intialization (build translation tables, etc)
  bl	appf_runtime_init
	adr	r1, appf_return_to_os_address
	adr	r2, appf_return_to_os
	str	r2, [r1]
				
  
	; Get new stack pointer
1			bl appf_platform_get_stack_pointer 
      mov	r11, r0			; save SP value for later

        ; Restore arguments
        mov	r0, r5
        mov	r1, r6
        mov	r2, r7
        mov	r3, r8
 	; This function uses the following registers:
	; r12 - OS stack pointer
	; r11 - APPF stack pointer
        ; r10 - APPF TTBR0
        ;  r9 - APPF TTBCR
        ;  r5 - APPF DACR
        ;  r8 - OS TTBR0
        ;  r7 - OS TTBCR
        ;  r6 - OS DACR
        ;  r4 - OS CPSR
	; Disable interrupts
	mrs	r4, CPSR
  cpsid	if
           

	; Check whether we are flat-mapped
	adr	r12, flat_mapped
	ldr	r12, [r12]
	cmp	r12, #0
	bne	%f0	; Skip the translation table switch code
	
	; Get APPF values for TTBR0, TTBCR, DACR
        adr	r10, appf_ttbr0;
        adr	r9, appf_ttbcr;
        ldr	r10, [r10]
        ldr	r9, [r9]
        bfc r10,#29,#3
       	orr r10,r10,#0x9<<28 ;table 0x9FF00000;
   			adr r12 ,next_label
   			bfc r12,#29,#3
   			orr r12,r12,#0x9<<28 ;switch to 0x9FFxxxxx
				;bl dbg_wait	
			  ; Get current TTBR0, TTBCR, DACR
        mrc	p15, 0, r8, c2, c0, 0	; read TTBR0
        mrc	p15, 0, r7, c2, c0, 2	; read TTBCR
        mrc	p15, 0, r6, c3, c0, 0	; read DACR
        ; Switch translation tables to APPF        
        movw r5, #0x5555
        movt r5, #0x5555
        mcr	p15, 0, r10, c2, c0, 0	; write TTBR0
        mcr	p15, 0,  r9, c2, c0, 2	; write TTBCR
        mcr	p15, 0,  r5, c3, c0, 0	; write DACR
        mrc p15, 0,  r10, c2, c0, 1 ; read TTBR1
              
        mov r9,#0
				MCR p15,0,r9,c7,c5,6 			// Invalidate BTAC
				MCR p15,0,r9,c7,c5,0			// Invalidate ICache
;				MCR p15,0,r9,c7,c6,1			// Invalidate DCache
				DSB
				MCR p15,0,r9,c8,c7,0			// Invalidate TLBs
				dsb
				isb
				mov pc,r12
			
next_label
	; Switch stacks, save OS stack pointer and TTBR0, TTBCR
	
0 
	mov	r12, sp
	bfc r11,#29,#3
	orr r11,r11,#9<<28  ;modified to 9FF
	mov	sp, r11
	push	{r4-r8,r12}		; see comment in appf_reset_entry_point below
	
;	mov r0, 0
;	mov r1, 0
;	mov r2, 0
;	mov r3, 0
	
	adr	r7, appf_runtime_call_flat_mapped
	ldr	r7, [r7]
	blx	r7
      
appf_return_to_os

	; retrieve OS stack pointer, TTBR0, TTBCR, DACR
	pop	{r4-r8,r12}
     
	; Switch stacks
	mov	sp, r12
	mov	r12, #0


	; Check whether we are flat-mapped
	adr	r12, flat_mapped
	ldr	r12, [r12]
	cmp	r12, #0
	bne	%f0	; Skip the translation table switch code
	
;	bl dbg_wait
	;switch to 0xDFFxxxxx kernel virtual address space.
	adr r5 ,label_df
	bfc r5,#28,#4
	movw r9,#0
	movt r9,#0xDF00
	orr r5,r5,r9
	mov pc,r5
	
label_df
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0		; I+BTB cache invalidate

	dsb
	isb
	
	; Switch translation tables back to OS
	mcr	p15, 0, r10, c2, c0, 0	; write TTBR1
	mcr	p15, 0, r8, c2, c0, 0	; write TTBR0
	mcr	p15, 0, r7, c2, c0, 2	; write TTBCR
    mcr	p15, 0, r6, c3, c0, 0	; write DACR

	bl	invalidate_dcache_v7

    mov	r12, #0
    MCR p15,0,r12,c7,c5,6 		// Invalidate BTAC
	MCR p15,0,r12,c7,c5,0			// Invalidate ICache
//	MCR p15,0,r9,c7,c6,1			// Invalidate DCache
	ISB
	DSB	
	MCR p15,0,r12,c8,c7,0		// Invalidate TLBs
    DSB

	
	;enable mmu for reset arm
	mrc p15,0,r12,c1,c0,0; read control
 	ORR r12, r12, #1 << 12      // enable ICache
 	ORR r12, r12, #1 << 2      	// enable ICache
 	ORR r12, r12, #1         		// enable MMU
 	mov r12, r12
	mcr p15,0,r12,c1,c0,0	
 	mov r12, r12
;	mov r12,#0
;	MCR p15,0,r12,c8,c7,0			// Invalidate TLBs
	dsb
	isb

	ldr r0, =0xf3100004
	mov r1,#0x72   ;'r'
	ldr r0, [r0]
	cmp r0, r1
	bne %f1
	bl l2x0_inv_all_ref
	bl l2x0_enable_ref

1
	mov r4,r4
	; Restore interrupts
0	msr	CPSR_c, r4
	; Return to OS

	;--------------------------------
	;restore A9 registers
	pop {r4,r5,r6}
	cps #0Xf ;switch to sys mode
  mov sp,r4
  mov lr,r5
  msr spsr,r6
  cps #3 ;switch to SVC mode	
	
	pop {r4,r5,r6}
	cps #0Xb ;switch to undef mode
  mov sp,r4
  mov lr,r5
  msr spsr,r6
  cps #3 ;switch to SVC mode	
	
	pop {r4,r5,r6}
	cps #7 ;switch to Abort mode
  mov sp,r4
  mov lr,r5
  msr spsr,r6
  cps #3 ;switch to SVC mode	
	      
	pop {r4}
  cps #6 ;switch to monitor user mode
  mov r12,r4
  cps #3 ;switch to SVC mode
  
 	pop {r4,r5,r6,r7}
  cps #6 ;switch to monitor user mode
  mov r8,r4
  mov r9,r5
  mov r0,r6
  mov r11,r7
  cps #3 ;switch to SVC mode

 	pop {r4,r5,r6}
	cps #6 ;switch to monitor user mode
  mov sp,r4
  mov lr,r5
  msr spsr,r6
  cps #3 ;switch to SVC mode	
   
 	pop {r4,r5,r6}
	cps #2 ;switch to IRQ mode
  mov sp,r4
  mov lr,r5
  msr spsr,r6
  cps #3 ;switch to SVC mode	
  
 	pop {r4}
  cps #1 ;switch to FIRQ mode
  mov r12,r4
  cps #3 ;switch to SVC mode
  
 	pop {r4,r5,r6,r7}
  cps #1 ;switch to FIRQ mode
  mov r8,r4
  mov r9,r5
  mov r0,r6
  mov r11,r7
  cps #3 ;switch to SVC mode

 	pop {r4,r5,r6}
	cps #1 ;switch to FIRQ mode
  mov sp,r4
  mov lr,r5
  msr spsr,r6
  cps #3 ;switch to SVC mode	

	pop {r4}
	msr spsr,r4  
	
	pop	{r4-r12, pc}

_cpu_stat
	.word	0x01ff80
	.word 0x01ff84
	.word 0
	.word 0
not_cpu0
    mov r2,#1
    mov r2,r2, asl r0
    orr r2,r2,#1
not_cpu0_loop
		movw r3, 0x990c
		movt r3, 0xc110
	  ldr r1,[r3]
		movw r3,#0xFF80
	  cmp r1,#0
	  beq next1
		movt r3,#0x1
		b next
next1	
		movt r3,#0xd901			
next
		;ldr r3,_cpu_stat		
    ldr r1,[r3]
    and r1,r2,r1
    cmp r1,r2
    wfene               @wait for event 
    bne   not_cpu0_loop
    ldr     pc, [r3, r0, asl #2]
    b   not_cpu0_loop

appf_reset_entry_point
	; This is the entry point from the platform warm start code

    mov r0,#0x4    ;P_AO_RTI_STATUS_REG1
    movt r0,#0xc810
    mov r1,#0x72   ;'r'
    str r1,[r0]
;wait_test
;    ldr r1,[r0]
;    cmp r1,#0x66 ;'f'
;    bne wait_test

	;--------------------------
	;CPU1 will go to standby 
	 MRC p15, 0, r0, c0, c0, 5   @ Read MPIDR
   AND r0, #0x3                @ Get CPU ID
   ldr r3,_cpu_stat
   mov     r2,#0               @ cpu_stat[r0]=0
   str     r2, [r3, r0,LSL #2]
   CMP r0, #0                  @ Is this CPU0
   BNE not_cpu0

	;--------------------------

	; Get stack pointer
	bl	appf_platform_get_stack_pointer
	sub	r0, r0, #24		; The 6 registers PUSHed above are still valid!
	mov	sp, r0
	
	;------------------------------
	;using mmu to avaliabe 0x9FF00000 segment
	; Switch translation tables back to OS
;	bl dbg_wait
  adr	r10, appf_ttbr0;
  adr	r9, appf_ttbcr;
  ldr	r10, [r10]
  ldr	r9, [r9]
  bfc r10,#29,#3
	orr r10,r10,#0x9<<28
	
  movw r5, #0x5555
  movt r5, #0x5555

	mcr	p15, 0, r10, c2, c0, 0	; write TTBR0
	mcr	p15, 0, r9, c2, c0, 2	; write TTBCR
  mcr	p15, 0, r5, c3, c0, 0	; write DACR
		
	
; 	mov r9,#0
;  MCR p15,0,r9,c7,c5,6 		// Invalidate BTAC
;	MCR p15,0,r9,c7,c5,0			// Invalidate ICache
;	DSB
;	MCR p15,0,r9,c8,c7,0			// Invalidate TLBs
;	DSB

;	BL new_d_inval
;	DSB

	;enable mmu for reset arm
;	mrc p15,0,r12,c1,c0,0; read control
; 	ORR r12, r12, #1         		// enable MMU
; 	mov r12, r12
;	mcr p15,0,r12,c1,c0,0
;	mov r12, r12
;	dsb
;	isb
	
    ; Turn I cache and branch prediction on
;	bl	read_sctlr
;	orr	r0, r0, #(SCTLR_I :OR: SCTLR_Z)
;	bl	write_sctlr

	; Clear all data cache levels visible to CPU
;	bl	invalidate_dcache_v7

	;bl l2x0_inv_all
	;bl  l2x0_enable
	
	; Caches have been invalidated, and any other init completed.
        ; Stack pointer is valid, and I cache on
	; So we are ready to go to the C code
	bl	appf_warm_reset
	b  appf_return_to_os
	

flat_mapped
	DCD	0

appf_runtime_call_flat_mapped
	DCD	0
        
appf_return_to_os_address
	DCD	0

appf_ttbr0
	DCD	0
        
appf_ttbcr
	DCD	0

	END
