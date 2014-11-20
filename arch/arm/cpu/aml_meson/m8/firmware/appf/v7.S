; Copyright (C) 2008-2010 ARM Limited                           
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

	EXPORT save_performance_monitors
	EXPORT restore_performance_monitors

	EXPORT save_banked_registers
	EXPORT restore_banked_registers

	EXPORT save_cp15
	EXPORT restore_cp15

	EXPORT save_control_registers
	EXPORT restore_control_registers

	EXPORT save_mmu
	EXPORT restore_mmu

	EXPORT save_vfp
	EXPORT restore_vfp

	EXPORT disable_clean_inv_dcache_v7_l1
	EXPORT disable_clean_inv_cache_pl310
	EXPORT clean_dcache_v7_l1
	EXPORT clean_mva_dcache_v7_l1
;	EXPORT invalidate_icache_v7_pou
	EXPORT invalidate_dcache_v7
	EXPORT v7_flush_dcache_all
	
	EXPORT v7_flush_cache_all_ref
	EXPORT v7_flush_cache_all
	EXPORT new_d_inval
	
	IMPORT read_sctlr
	IMPORT write_sctlr

	EXPORT enter_secure_monitor_mode
	EXPORT appf_smc_handler
	EXPORT enter_nonsecure_svc_mode
	EXPORT set_security_state

	AREA APPF, CODE

; Aliases for mode encodings - do not change
MODE_USR        EQU     0x10
MODE_FIQ        EQU     0x11
MODE_IRQ        EQU     0x12
MODE_SVC        EQU     0x13
MODE_ABT        EQU     0x17
MODE_UND        EQU     0x1B
MODE_SYS        EQU     0x1F

MODE_MON        EQU     0x16  ; A-profile (Security Extensions) only
SCR_NS		EQU	0x01  ; A-profile (Security Extensions) only

; Maintain correlation between these offsets and struct appf_cpu_context in appf_types.h

ENDIAN_OFFSET	EQU     0x00   ; flag for CPSR.E bit status
ACTLR_OFFSET	EQU     0x04   ; CP15 register content
SCTLR_OFFSET	EQU     0x08   ; CP15 register content
CPACR_OFFSET	EQU     0x0C   ; CP15 register content

CACHE_LINE_SIZE	EQU	32

;#define L2_CACHE_BASE_VIRT	0xFE000000
;#define L2_CACHE_CRT_VIRT 0xFE000100


save_performance_monitors	FUNCTION

	PUSH	{r4, r8, r9, r10}

	; Ignore:
	;        Count Enable Clear Register
	;        Software Increment Register
	;        Interrupt Enable Clear Register

	MRC	p15,0,r8,c9,c12,0	; PMon: Control Register
	BIC	r1,r8,#1
	MCR	p15,0,r1,c9,c12,0	; disable counter updates from here
	ISB				; 0b0 => PMCR<0>
	MRC	p15,0,r9,c9,c12,3	; PMon: Overflow Flag Status Reg
	MRC	p15,0,r10,c9,c12,5	; PMon: Event Counter Selection Reg
	STM	r0!, {r8-r10}
	UBFX	r9,r8,#11,#5		; extract # of event counters, N
	TST	r9, r9
	BEQ	%f1

0	SUBS	r9,r9,#1		; decrement N
	MCR	p15,0,r9,c9,c12,5	; PMon: select CounterN
	ISB
	MRC	p15,0,r3,c9,c13,1	; PMon: save Event Type register
	MRC	p15,0,r4,c9,c13,2	; PMon: save Event Counter register
	STM	r0!, {r3,r4}
	BNE	%b0

1	MRC	p15,0,r1,c9,c13,0	; PMon: Cycle Count Register
	MRC	p15,0,r2,c9,c14,0	; PMon: User Enable Register
	MRC	p15,0,r3,c9,c14,1	; PMon: Interrupt Enable Set Reg
	MRC	p15,0,r4,c9,c12,1	; PMon: Count Enable Set Register
	STM	r0!, {r1-r4}

	POP	{r4, r8, r9, r10}
	bx	lr
	ENDFUNC

restore_performance_monitors	FUNCTION

	PUSH	{r4-r5, r8-r10, lr}
	; NOTE: all counters disabled by PMCR<0> == 0 on reset

	; Restore performance counters
	LDM	r0!,{r8-r10}	; recover first block of PMon context
				; (PMCR, PMOVSR, PMSELR)
	MOV	r1, #0		; generate register of all 0's
	MVN	r2, #0		; generate register of all 1's
	MCR	p15,0,r2,c9,c14,2	; disable all counter related interrupts
	MCR	p15,0,r2,c9,c12,3	; clear all overflow flags
	ISB

	UBFX	r12,r8,#11,#5	; extract # of event counters, N (0-31)
	TST	r12, r12
	BEQ	%f20
	MOV	r3, r12		; for N >0, generate a 2nd copy of N
	MOV	r4, #1
	LSL	r4, r4, r3
	SUB	r4, r4, #1	; set bits<N-1:0> to all 1's   

0	SUBS    r3,r3,#1            ; decrement N
	MCR	p15,0,r3,c9,c12,5   ; select Event CounterN
	ISB
	MRC	p15,0,r5,c9,c13,1   ; read Event Type register
	BFC	r5,#0,#8
	MCR	p15,0,r5,c9,c13,1   ; set Event Type to 0x0
	MCR	p15,0,r2,c9,c13,2   ; set Event Counter to all 1's
	ISB
	BNE	%b0

	MOV	r3, #1
	BIC	r5, r9, #1<<31
	MCR	p15,0,r5,c9,c12,1	; enable Event Counters 
					; (PMOVSR bits set)
	MCR	p15,0,r3,c9,c12,0	; set the PMCR global enable bit
	ISB
	MCR	p15,0,r9,c9,c12,4   ; set event count overflow bits
	ISB
	MCR	p15,0,r4,c9,c12,2   ; disable Event Counters

	; restore the event counters
10	SUBS	r12,r12,#1          ; decrement N
	MCR	p15,0,r12,c9,c12,5  ; select Event CounterN
	ISB
	LDM	r0!,{r3-r4}
	MCR	p15,0,r3,c9,c13,1   ; restore Event Type
	MCR	p15,0,r4,c9,c13,2   ; restore Event Counter
	ISB
	BNE	%b10

20	TST	r9, #0x80000000		; check for cycle count overflow flag
	BEQ	%f40
	MCR	p15,0,r2,c9,c13,0	; set Cycle Counter to all 1's
	ISB
	MOV	r3, #0x80000000
	MCR	p15,0,r3,c9,c12,1	; enable the Cycle Counter
	ISB

30	MRC	p15,0,r4,c9,c12,3	; check cycle count overflow now set
	MOVS	r4,r4			; test bit<31>
	BPL	%b30
	MCR	p15,0,r3,c9,c12,2	; disable the Cycle Counter

40	MCR	p15,0,r1,c9,c12,0	; clear the PMCR global enable bit
	ISB

	; restore the remaining PMon registers
	LDM	r0!,{r1-r4}
	MCR	p15,0,r1,c9,c13,0	; restore Cycle Count Register
	MCR	p15,0,r2,c9,c14,0	; restore User Enable Register
	MCR	p15,0,r3,c9,c14,1	; restore Interrupt Enable Set Reg
	MCR	p15,0,r4,c9,c12,1	; restore Count Enable Set Register
	MCR	p15,0,r10,c9,c12,5	; restore Event Counter Selection
	ISB
	MCR	p15,0,r8,c9,c12,0	; restore the PM Control Register
	ISB

	POP	{r4-r5, r8-r10, pc}
	ENDFUNC
	

save_banked_registers	FUNCTION
	[ {false} ; TODO should be SECURITY_EXTNS_ValidS
		; Monitor Mode in use? A-profile ONLY
		CPS	#MODE_MON	; switch to Monitor mode
		STR	SP,[r0], #4	; save the User SP
		STR	LR,[r0], #4	; save the User LR         
	]

	CPS	#MODE_SYS		; switch to System mode
	STR	SP,[r0], #4		; save the Monitor SP
	STR	LR,[r0], #4		; save the Monitor LR
	CPS	#MODE_ABT		; switch to Abort mode
	STR	SP,[r0], #4		; save the current SP   
	MRS	r3,SPSR 
	STM	r0!,{r3,LR}		; save the current SPSR, LR
	CPS	#MODE_UND		; switch to Undefined mode
	STR	SP,[r0], #4		; save the current SP
	MRS	r3,SPSR 
	STM	r0!,{r3,LR}		; save the current SPSR, LR
	CPS	#MODE_IRQ		; switch to IRQ mode
	STR	SP,[r0], #4		; save the current SP
	MRS	r3,SPSR 
	STM	r0!,{r3,LR}		; save the current SPSR, LR
	CPS	#MODE_FIQ		; switch to FIQ mode
	STR	SP,[r0], #4		; save the current SP
	MRS	r3,SPSR 
	STM	r0!,{r8-r12,LR}		; save the current SPSR,r8-r12,LR
	CPS	#MODE_SVC		; switch back to Supervisor mode

	BX	lr
	ENDFUNC

restore_banked_registers	FUNCTION
	[ {false}	; TODO
			; A-profile ONLY
	CPS	#MODE_MON		; switch to Monitor mode
	LDR	SP,[r0], #4		; restore the Monitor SP
	LDR	LR,[r0], #4		; restore the Monitor LR         
        ]

	CPS	#MODE_SYS		; switch to System mode 
	LDR	SP,[r0],#4		; restore the User SP 
	LDR	LR,[r0],#4		; restore the User LR
	CPS	#MODE_ABT		; switch to Abort mode 
	LDR	SP,[r0],#4		; restore the current SP    
	LDM	r0!,{r3,LR}		; restore the current LR 
	MSR	SPSR_fsxc,r3		; restore the current SPSR
	CPS	#MODE_UND		; switch to Undefined mode 
	LDR	SP,[r0],#4		; restore the current SP    
	LDM	r0!,{r3,LR}		; restore the current LR 
	MSR	SPSR_fsxc,r3		; restore the current SPSR
	CPS	#MODE_IRQ		; switch to IRQ mode 
	LDR	SP,[r0],#4		; restore the current SP    
	LDM	r0!,{r3,LR}		; restore the current LR 
	MSR	SPSR_fsxc,r3		; restore the current SPSR
	CPS	#MODE_FIQ		; switch to FIQ mode 
	LDR	SP,[r0],#4		; restore the current SP    
	LDM	r0!,{r8-r12,LR}		; restore the current r8-r12,LR
	MSR	SPSR_fsxc,r4		; restore the current SPSR
	CPS	#MODE_SVC		; switch back to Supervisor mode

	bx	lr
	ENDFUNC

	
save_cp15	FUNCTION
	; CSSELR – Cache Size Selection Register
	MRC	p15,2,r3,c0,c0,0
	STR	r3,[r0], #4

	; IMPLEMENTATION DEFINED - proprietary features: 
	; (CP15 register 15, TCM support, lockdown support, etc.)

	; NOTE: IMP DEF registers might have save and restore order that relate
	; to other CP15 registers or logical grouping requirements and can 
	; therefore occur at any point in this sequence.
	BX	lr
	ENDFUNC
        
restore_cp15	FUNCTION
	; CSSELR – Cache Size Selection Register
	LDR	r3,[r0], #4
	MCR	p15,2,r3,c0,c0,0

	bx	lr
	ENDFUNC
	

	; Function called with single argument:
	;	 r0 contains address of struct appf_cpu_context

save_control_registers	FUNCTION
	; ACTLR - Auxiliary Control Register
	MRC	p15,0,r1,c1,c0,1
	; SCTLR - System Control Register
	MRC	p15,0,r2,c1,c0,0
	; CPACR - Coprocessor Access Control Register
	MRC	p15,0,r3,c1,c0,2
	STR	r1,[r0, #ACTLR_OFFSET]	; fixed address
	STR	r2,[r0, #SCTLR_OFFSET]	; fixed address
	STR	r3,[r0, #CPACR_OFFSET]	; fixed address
	BX	lr
	ENDFUNC
                

restore_control_registers	FUNCTION
	LDR	r1,[r0, #ACTLR_OFFSET]	; fixed address
	LDR	r2,[r0, #SCTLR_OFFSET]	; fixed address
	LDR	r3,[r0, #CPACR_OFFSET]	; fixed address
	; ACTLR - Auxiliary Control Register
	MCR	p15,0,r1,c1,c0,1
	; SCTLR - System Control Register
	MCR	p15,0,r2,c1,c0,0
	; CPACR - Coprocessor Access Control Register
	MCR	p15,0,r3,c1,c0,2
	BX	lr
	ENDFUNC

save_mmu	FUNCTION

	;;; save_control_registers saves ACTLR, SCTLR, CPACR

	PUSH	{r4, r5, r6, r7}
	; ASSUMPTION: no useful fault address / fault status information

	MRC	p15,0,r4,c12,c0,0	; VBAR
	MRC	p15,0,r5,c2,c0,0	; TTBR0
	MRC	p15,0,r6,c2,c0,1	; TTBR1
	MRC	p15,0,r7,c2,c0,2	; TTBCR
	STM	r0!, {r4-r7}

	MRC	p15,0,r4,c3,c0,0	; DACR
	MRC	p15,0,r5,c7,c4,0	; PAR
	MRC	p15,0,r6,c10,c2,0	; PRRR
	MRC	p15,0,r7,c10,c2,1	; NMRR
	STM	r0!, {r4-r7}

	; TODO: IMPLEMENTATION DEFINED - TCM, lockdown and performance monitor support
	;     CP15 registers 9 and 11

	MRC	p15,0,r4,c13,c0,1	; CONTEXTIDR
	MRC	p15,0,r5,c13,c0,2	; TPIDRURW
	MRC	p15,0,r6,c13,c0,3	; TPIDRURO
	MRC	p15,0,r7,c13,c0,4	; TPIDRPRW
	STM	r0!, {r4-r7}

	POP	{r4, r5, r6, r7}
	bx	lr
	ENDFUNC


restore_mmu	FUNCTION

	PUSH	{r4, r5, r6, r7}
	LDM	r0!, {r4-r7}
	MCR	p15,0,r4,c12,c0,0	; VBAR
	MCR	p15,0,r5,c2,c0,0	; TTBR0
	MCR	p15,0,r6,c2,c0,1	; TTBR1
	MCR	p15,0,r7,c2,c0,2	; TTBCR

	LDM	r0!, {r4-r7}
	MCR	p15,0,r4,c3,c0,0	; DACR
	MCR	p15,0,r5,c7,c4,0	; PAR
	MCR	p15,0,r6,c10,c2,0	; PRRR
	MCR	p15,0,r7,c10,c2,1	; NMRR

	; TODO: IMPLEMENTATION DEFINED - TCM, lockdown and performance monitor support
	;     CP15 registers 9 and 11

	LDM	r0!, {r4-r7}
	MCR	p15,0,r4,c13,c0,1	; CONTEXTIDR
	MCR	p15,0,r5,c13,c0,2	; TPIDRURW
	MCR	p15,0,r6,c13,c0,3	; TPIDRURO
	MCR	p15,0,r7,c13,c0,4	; TPIDRPRW

	POP	{r4, r5, r6, r7}
	bx	lr
	ENDFUNC
	
	
save_vfp	FUNCTION
        ; FPU state save/restore.
        ; FPSID,MVFR0 and MVFR1 don't get serialized/saved (Read Only).
;  push {r0,r1,r2,r3,r12,lr}
;	bl dbg_wait
	MRC	p15,0,r3,c1,c0,2	; CPACR allows CP10 and CP11 access
	ORR	r2,r3,#0xF00000       
	MCR	p15,0,r2,c1,c0,2
	ISB
	MRC	p15,0,r2,c1,c0,2
	AND	r2,r2,#0xF00000
	CMP	r2,#0xF00000
	BEQ	%f0
	MOVS	r2, #0
	; Override to 0 to indicate that no FPU is present
	;	STR     r2,[r11,#DM_VFP]		; TODO: autodetect VFP in C!!
	B	%f1
                
0	;	Save configuration registers and enable.
	VMRS	r12,FPEXC
	STR	r12,[r0],#4		; Save the FPEXC
        ; Enable FPU access to save/restore the other registers.
	LDR	r2,=0x40000000
	VMSR	FPEXC,r2
	VMRS	r2,FPSCR
	STR	r2,[r0],#4		; Save the FPSCR
        ; Store the VFP-D16 registers. 
	VSTM	r0!, {D0-D15}
        ; Check for Advanced SIMD/VFP-D32 support
	VMRS	r2,MVFR0
	AND	r2,r2,#0xF		; extract the A_SIMD bitfield
	CMP	r2, #0x2
	BLT	%f1
        ; Store the Advanced SIMD/VFP-D32 additional registers.
	VSTM	r0!, {D16-D31}
	
	        ; IMPLEMENTATION DEFINED: save any subarchitecture defined state 
        ; NOTE: Don't change the order of the FPEXC and CPACR restores
	VMSR	FPEXC,r12         ; Restore the original En bit of FPU.
	tst	r12, #(1<<31)
	beq	s_next
	VMRS r2, FPINST		@ FPINST (only if FPEXC.EX is set)
	str r2,[r0],#4
	tst	r12, #(1<<28)		@ is there an FPINST2 to read?
	beq	s_next
	vmrs r2, FPINST2		@ FPINST2 if needed (and present)
	str r2,[r0],#4
s_next
1
	MCR	p15,0,r3,c1,c0,2 ; Restore the original CPACR value.
;	pop {r0,r1,r2,r3,r12,pc}
	BX	lr
	ENDFUNC


restore_vfp	FUNCTION
	; FPU state save/restore. Obviously FPSID,MVFR0 and MVFR1 don't get
	; serialized (RO).
	; Modify CPACR to allow CP10 and CP11 access
;  push {r0,r1,r2,r3,r12,lr}
;	bl dbg_wait
	MRC	p15,0,r2,c1,c0,2
	mov r1,r2
	ORR	r2,r2,#0x00F00000       
	MCR	p15,0,r2,c1,c0,2
	; Enable FPU access to save/restore the rest of registers.
	LDR	r2,=0x40000000
	VMSR	FPEXC, r2
	; Recover FPEXC and FPSCR. These will be restored later.        
	LDM	r0!,{r3,r12}
	; Restore the VFP-D16 registers.
	VLDM	r0!, {D0-D15}
	; Check for Advanced SIMD/VFP-D32 support
	VMRS	r2, MVFR0
	AND	r2,r2,#0xF		; extract the A_SIMD bitfield
	CMP	r2, #0x2
	BLT	%f0

	; Store the Advanced SIMD/VFP-D32 additional registers.
	VLDM	r0!, {D16-D31}

	; IMPLEMENTATION DEFINED: restore any subarchitecture defined state 

0	; Restore configuration registers and enable.
	; Restore FPSCR _before_ FPEXC since FPEXC could disable FPU
	; and make setting FPSCR unpredictable.
	
	VMSR	FPSCR,r12    	
	VMSR	FPEXC,r3		; Restore FPEXC after FPSCR

	tst	r3, #(1<<31)
	beq	r_next
	LDM r0!,{r2}
	VMSR FPINST,r2
	tst	r3, #(1<<28)		@ is there an FPINST2 to read?
	beq	r_next
	LDM r0!,{r2}
	VMSR FPINST2,r2
r_next
	
	; CPACR (c1,c0,2) is restored later.
	MCR	p15,0,r1,c1,c0,2
;  pop {r0,r1,r2,r3,r12,pc}
	BX	lr
	ENDFUNC


	; This function disables L2 data caching, then cleans and invalidates 
	; the stack in L2.
	; r0 contains the PL310 address
	; r1 contains the stack start addres
	; r2 contains the stack size
	; r3 contains 1 if the L2 is to be disabled, 0 if it is to be left on.

disable_clean_inv_cache_pl310	FUNCTION
	; Should we disable the L2 cache?
	cmp	r3, #0
	beq	%f2
	
	; Sync and Disable L2 cache
	mov	r3, #0
	str	r3, [r0, #0x730]	; PL310: Cache sync register
	dsb
	ldr	r3, [r0, #0x100]	; PL310: Control register
	bic	r3, #1
	str	r3, [r0, #0x100]

2	; Disable MMU
	dsb
	mrc	p15, 0, r3, c1, c0, 0
        bic	r3, #1			; Clear M bit
	mcr	p15, 0, r3, c1, c0, 0
	dsb
		
	; Clean+invalidate stack in L2
;	bic	r1, #(CACHE_LINE_SIZE - 1)
;5	cmp	r2, #0
;	blt	%f10
;	str	r1, [r0, #0x7f0]	; PL310: Clean and invalidate by PA	
;	add	r1, #CACHE_LINE_SIZE
;	subs	r2, #CACHE_LINE_SIZE
;	b	%b5
10	bx	lr        		; Stack invalidated, we can return to C
	ENDFUNC

	; This function disables L1 data caching, then cleans and invalidates 
	; the whole L1 data cache.

disable_clean_inv_dcache_v7_l1	FUNCTION
	push	{r4, lr}

	; Disable L1 cache
	dsb
	mrc	p15, 0, r3, c1, c0, 0
        bic	r3, #4			; Clear C bit
	mcr	p15, 0, r3, c1, c0, 0
	dsb

	; No more Data cache allocations can happen at L1.
        ; Until we finish cleaning the Inner cache, any accesses to dirty data
        ; (e.g. by translation table walks) may get the wrong (Outer) data, so
        ; we have to be sure everything that might be accessed is clean.
        ; We already know that the translation tables are clean (see late_init).

	mov	r0, #0			; Select L1 Data/Unified cache
     	mcr	p15,2,r0,c0,c0,0
     	mrc	p15,1,r0,c0,c0,0	; Read size
        ubfx	r3, r0, #13, #15	; sets - 1
        add	r3, r3, #1		; sets
        ubfx	r4, r0, #0, #3		; log2(words per line) - 2
        add	r4, r4, #4		; set shift = log2(bytes per line)
        ubfx	r2, r0, #3, #10		; ways - 1
        clz	r12, r2			; way shift
        add	r2, r2, #1		; ways
        
        ; r2,r3 inner, outer loop targets, r1 inner loop counter, r0 zero
5	cmp	r3, #0
	beq	%f20
  	sub	r3, r3, #1      
	mov	r1, r2

10	cmp	r1, #0
        beq	%b5
        sub	r1, r1, #1
	mov	r0, r1, lsl r12		; Fill in Way field
        orr	r0, r0, r3, lsl r4	; Fill in Set field
	mcr	p15,0,r0,c7,c14,2	; DCCISW
        b	%b10
        
20	dsb
	pop	{r4, lr}
	bx	lr
	ENDFUNC	
	

;invalidate_icache_v7_pou FUNCTION
;	mov     r0, #0
;	mcr     p15, 0, r0, c7, c5, 0      ; ICIALLU
;	bx	lr
;	ENDFUNC	

v7_flush_dcache_all FUNCTION
 ; Must iterate over the caches in order to synthesise a complete clean
 ; of data/unified cache
	push    {r4-r11}
	dmb
	mrc     p15, 1, r0, c0, c0, 1       ; read clidr
	ands    r3, r0, #0x7000000          ; extract loc from clidr
	mov     r3, r3, lsr #23             ; left align loc bit field
	beq     finished                    ; if loc is 0, then no need to clean
	mov     r10, #0                     ; start clean at cache level 0 (in r10)
loop1
	add     r2, r10, r10, lsr #1        ; work out 3x current cache level
	mov     r12, r0, lsr r2             ; extract cache type bits from clidr
	and     r12, r12, #7                ; mask of bits for current cache only
	cmp     r12, #2                     ; see what cache we have at this level
	blt     skip                        ; skip if no cache, or just i-cache
	mcr     p15, 2, r10, c0, c0, 0      ; select current cache level in cssr
	isb
;	mov     r12, #0
; mcr     p15, 0, r12, c7, c5, 4      ; prefetchflush to sync new cssr&csidr
	mrc     p15, 1, r12, c0, c0, 0      ; read the new csidr
	and     r2, r12, #7                 ; extract the length of the cache lines
	add     r2, r2, #4                  ; add 4 (line length offset)
	ldr     r6, =0x3ff
	ands    r6, r6, r12, lsr #3         ; find maximum number on the way size
	clz     r5, r6                      ; find bit pos of way size increment
	ldr     r7, =0x7fff
	ands    r7, r7, r12, lsr #13        ; extract max number of the index size
loop2
	mov     r8, r6                      ; create working copy of max way size
loop3
	orr     r11, r10, r8, lsl r5        ; factor way and cache number into r11
	orr     r11, r11, r7, lsl r2        ; factor index number into r11
	;mcr     p15, 0, r11, c7, c6, 2      ; invalidate by set/way
	mcr     p15, 0, r11, c7 , c14, 2    ;clean & invalidate by set/way
	
	subs    r8, r8, #1                  ; decrement the way
	bge     loop3
	subs    r7, r7, #1                  ; decrement the index
	bge     loop2
skip
	add     r10, r10, #2                ; increment cache number
	cmp     r3, r10
	bgt     loop1
finished
	mov     r10, #0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb
;	mcr	p15, 0, r10, c7, c5, 4		@ flush prefetch buffer,
;						@ with armv7 this is 'isb',
;						@ but we compile with armv5
	isb
;	dsb
;	mcr     p15, 0, r10, c7, c10, 4     ; drain write buffer
;	mcr     p15, 0, r10, c8, c7, 0      ; invalidate I + D TLBs
;	mcr     p15, 0, r10, c2, c0, 2      ; TTB control register
	pop     {r4-r11}
	bx      lr
	ENDFUNC

invalidate_dcache_v7 FUNCTION
 ; Must iterate over the caches in order to synthesise a complete clean
 ; of data/unified cache
	push    {r4-r11}
	dmb
	mrc     p15, 1, r0, c0, c0, 1       ; read clidr
	ands    r3, r0, #0x7000000          ; extract loc from clidr
	mov     r3, r3, lsr #23             ; left align loc bit field
	beq     finished0                   ; if loc is 0, then no need to clean
	mov     r10, #0                     ; start clean at cache level 0 (in r10)
loop10
	add     r2, r10, r10, lsr #1        ; work out 3x current cache level
	mov     r12, r0, lsr r2             ; extract cache type bits from clidr
	and     r12, r12, #7                ; mask of bits for current cache only
	cmp     r12, #2                     ; see what cache we have at this level
	blt     skip0                       ; skip if no cache, or just i-cache
	mcr     p15, 2, r10, c0, c0, 0      ; select current cache level in cssr
	isb
;	mov     r12, #0
; mcr     p15, 0, r12, c7, c5, 4      ; prefetchflush to sync new cssr&csidr
	mrc     p15, 1, r12, c0, c0, 0      ; read the new csidr
	and     r2, r12, #7                 ; extract the length of the cache lines
	add     r2, r2, #4                  ; add 4 (line length offset)
	ldr     r6, =0x3ff
	ands    r6, r6, r12, lsr #3         ; find maximum number on the way size
	clz     r5, r6                      ; find bit pos of way size increment
	ldr     r7, =0x7fff
	ands    r7, r7, r12, lsr #13        ; extract max number of the index size
loop20
	mov     r8, r6                      ; create working copy of max way size
loop30
	orr     r11, r10, r8, lsl r5        ; factor way and cache number into r11
	orr     r11, r11, r7, lsl r2        ; factor index number into r11
	mcr     p15, 0, r11, c7, c6, 2      ; invalidate by set/way

	
	subs    r8, r8, #1                  ; decrement the way
	bge     loop30
	subs    r7, r7, #1                  ; decrement the index
	bge     loop20
skip0
	add     r10, r10, #2                ; increment cache number
	cmp     r3, r10
	bgt     loop10
finished0
	mov     r10, #0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb
;	mcr	p15, 0, r10, c7, c5, 4		@ flush prefetch buffer,
;						@ with armv7 this is 'isb',
;						@ but we compile with armv5
	isb
;	dsb
;	mcr     p15, 0, r10, c7, c10, 4     ; drain write buffer
;	mcr     p15, 0, r10, c8, c7, 0      ; invalidate I + D TLBs
;	mcr     p15, 0, r10, c2, c0, 2      ; TTB control register
	pop     {r4-r11}
	bx      lr
	ENDFUNC

	AREA APPF_ENTRY_POINT_CODE, CODE

	
	; This function cleans the whole L1 data cache 
clean_dcache_v7_l1	FUNCTION
	push	{r4, lr}

	mov	r0, #0			; Select L1 Data/Unified cache
     	mcr	p15,2,r0,c0,c0,0
     	mrc	p15,1,r0,c0,c0,0	; Read size (CCSIDR)
        ubfx	r3, r0, #13, #15	; sets - 1
        add	r3, r3, #1		; sets
        ubfx	r4, r0, #0, #3		; log2(words per line) - 2
        add	r4, r4, #4		; set shift = log2(bytes per line)
        ubfx	r2, r0, #3, #10		; ways - 1
        clz	r12, r2			; way shift
        add	r2, r2, #1		; ways
        
        ; r2,r3 inner, outer loop targets, r1 inner loop counter, r0 zero
0	cmp	r3, #0
	beq	%f20
  	sub	r3, r3, #1      
	mov	r1, r2

10	cmp	r1, #0
        beq	%b0
        sub	r1, r1, #1
	mov	r0, r1, lsl r12		; Fill in Way field
        orr	r0, r0, r3, lsl r4	; Fill in Set field
	mcr	p15,0,r0,c7,c10,2	; DCCSW
        b	%b10
        
20	dsb
	pop	{r4, lr}
	bx	lr
	ENDFUNC	

	; This function cleans a single line from the L1 dcache
clean_mva_dcache_v7_l1
	MCR	p15,0,r0,c7,c10,1	; DCCMVAC
	bx	lr


enter_secure_monitor_mode	FUNCTION
	smc	#0	 
appf_smc_handler
	; We are now in Monitor mode, make sure we're Secure
	mrc	p15, 0, r1, c1, c1, 0
	bic	r1, #SCR_NS
	mcr	p15, 0, r1, c1, c1, 0

	; Copy the SVC mode sp and lr values
	cps	#MODE_SVC
	mov	r2, lr
	mov	r3, sp
	cps	#MODE_MON
	; Restore sp and return
	mov	sp, r3
	bx	r2
	ENDFUNC

enter_nonsecure_svc_mode	FUNCTION
	; Copy the Monitor mode sp and lr values
	mov	r2, lr
	mov	r3, sp
	mrc	p15, 0, r1, c1, c1, 0
	orr	r1, #SCR_NS
	mcr	p15, 0, r1, c1, c1, 0
	adr	lr, non_secure
	movs	pc, lr
non_secure                      
	; We are now in non-secure state
	; Restore sp and return
	mov	sp, r3
	bx	r2
	ENDFUNC

set_security_state	FUNCTION
	cmp	r0, #0
	mrc	p15, 0, r1, c1, c1, 0
	biceq	r1, #SCR_NS
	orrne	r1, #SCR_NS
	mcr	p15, 0, r1, c1, c1, 0
	bx	lr
	ENDFUNC
	
v7_flush_cache_all_ref FUNCTION ; refer to kernel Machine_kexec.c (y:\android_jb\common\arch\arm\kernel)	3235	2013-2-5
	stmfd	sp!, {r0-r7, r9-r11, lr}
;	bl clean_dcache_v7_l1
	dmb
	mov r4, r3 ; save arg1
	
	bl	v7_flush_dcache_all	
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		; I+BTB cache invalidate

	mov r3, r4
	ldr r1, =0xffff0000 ; get l2 base addr
	and r3, r3, r1
	add r3, r3, #0x100 ; l2cache control reg addr
	ldr r1, [r3]
	and r5, r1, #0x1
	cmp r5, #0
	beq %f0

	bl l2x0_flush_all_ref

	bl l2x0_disable_ref
0
	mrc p15,0,r0,c1,c0,0; read control
	bic	r0, r0, #0x1000			;@ ...i............
	bic	r0, r0, #0x0006			;@ .............ca.
	mcr p15,0,r0,c1,c0,0; read control

	cmp r5,#0						; l2 cache on or off
	beq %f1

	bl l2x0_inv_all_ref
1
	bl	v7_flush_dcache_all	
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		; I+BTB cache invalidate
	
	ldmfd	sp!, {r0-r7, r9-r11, lr}
	mov	pc, lr
	ENDFUNC

v7_flush_cache_all FUNCTION
	stmfd	sp!, {r0-r7, r9-r11, lr}
	;bl clean_dcache_v7_l1
	bl	v7_flush_dcache_all
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		; I+BTB cache invalidate
	bl l2x0_disable_flush
	ldmfd	sp!, {r0-r7, r9-r11, lr}
	mov	pc, lr
	ENDFUNC
    		
new_d_inval FUNCTION
    MOV     r0,#0
    MCR     p15,2,r0,c0,c0,0  @ cache size selection register, select dcache
    MRC     p15,1,r0,c0,c0,0  @ cache size ID register
    MOV     r0,r0,ASR #13
    LDR     r3,=0xfff
    AND     r0,r0,r3        
    CMP     r0,#0x7f
    MOVEQ   r6,#0x1000
    BEQ     size_done
    CMP     r0,#0xff
    MOVEQ   r6,#0x2000
    MOVNE   r6,#0x4000
        
size_done
    MOV     r2,#0
    MOV     r3,#0x40000000
    MOV     r4,#0x80000000
    MOV     r5,#0xc0000000
        
d_inv_loop
    MCR     p15,0,r2,c7,c6,2  @ invalidate dcache by set / way
    MCR     p15,0,r3,c7,c6,2  @ invalidate dcache by set / way
    MCR     p15,0,r4,c7,c6,2  @ invalidate dcache by set / way
    MCR     p15,0,r5,c7,c6,2  @ invalidate dcache by set / way
    ADD     r2,r2,#0x20
    ADD     r3,r3,#0x20
    ADD     r4,r4,#0x20
    ADD     r5,r5,#0x20
    CMP     r2,r6
    BNE     d_inv_loop
    BX 	r14
    ENDFUNC
	END
	
