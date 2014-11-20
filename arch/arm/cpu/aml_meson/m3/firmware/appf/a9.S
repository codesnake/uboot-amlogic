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

	AREA APPF, CODE

	EXPORT save_a9_debug
	EXPORT restore_a9_debug

	EXPORT save_a9_global_timer
	EXPORT restore_a9_global_timer

	EXPORT save_a9_other
	EXPORT restore_a9_other

CA9_SCU_ICD   EQU 0x1000       ; GIC Distributor offset from SCU_BASE
CA9_SCU_ICC   EQU 0x100        ; GIC CPU Interface offset from SCU_BASE
CA9_SCU_TIM64 EQU 0x200        ; (64-bit) timer block offset from SCU_BASE

			; SCU_BASE offsets:
SCU_Ctl		EQU 0x0
SCU_Config	EQU 0x4
SCU_PwrStatus	EQU 0x8
SCU_InvAll	EQU 0xC
SCU_FiltStart	EQU 0x40
SCU_FiltEnd	EQU 0x44
SCU_SAC		EQU 0x50
SCU_SSAC	EQU 0x54

; Global timer offsets
TIM64_CntLo	EQU 0x0
TIM64_CntHi	EQU 0x4
TIM64_Ctl	EQU 0x8
TIM64_Status	EQU 0xC
TIM64_CmpLo	EQU 0x10
TIM64_CmpHi	EQU 0x14
TIM64_AutoInc	EQU 0x18


save_a9_global_timer	FUNCTION
	PUSH	{r4, r5}
	MOVW	r12, #CA9_SCU_TIM64
	ADD	r1,r1,r12
	LDR	r2, [r1, #TIM64_Ctl]		; 64-bit timer control
	BIC	r3, r2, #0xF
	STR	r3, [r1, #TIM64_Ctl]		; disable the features
	; the registers are now frozen for the context save          
	LDR	r3, [r1, #TIM64_AutoInc]	; Autoincrement register
	LDR	r4, [r1, #TIM64_CmpLo]		; comparator - lo word
	LDR	r5, [r1, #TIM64_CmpHi]		; comparator - hi word
	STM	r0!, {r2-r5}
	LDR	r2, [r1, #TIM64_CntLo]		; counter - lo word          
	LDR	r3, [r1, #TIM64_CntHi]		; counter - hi word
	STM	r0!, {r2-r3}
	POP	{r4, r5}
	BX	lr
	ENDFUNC

restore_a9_global_timer	FUNCTION
	PUSH	{r4, r5}
	MOVW	r12, #CA9_SCU_TIM64
	ADD	r1,r1,r12
	LDM	r0!, {r2-r5}       
	STR	r3, [r1, #TIM64_AutoInc]	; Autoincrement register
	STR	r4, [r1, #TIM64_CmpLo]		; comparator - lo word
	STR	r5, [r1, #TIM64_CmpHi]		; comparator - hi word
	LDM	r0!, {r3-r4}
	STR	r3, [r1, #TIM64_CntLo]		; counter - lo word          
	STR	r4, [r1, #TIM64_CntHi]		; counter - hi word
	STR	r2, [r1, #TIM64_Ctl]		; restore the control last 
	
	POP	{r4, r5}
	BX	LR
	ENDFUNC	


save_a9_debug	FUNCTION
	; TODO
	bx	lr	
	ENDFUNC

restore_a9_debug	FUNCTION
	; TODO
	bx	lr	
	ENDFUNC

save_a9_other	FUNCTION
	MRC	p15,0,r1,c15,c0,0	; Read Power Control Register
	STR	r1, [r0], #4

	MRC	p15,0,r3,c0,c0,0	; Read Main ID Register
	UBFX	r3, r3, #20, #4		; Extract major version number
	CMP	r3, #2
	BLT	%f1			; PLE only possible in r2p0 onwards
	MRC	p15,0,r3,c11,c0,0	; Read PLE IDR
	CMP	r3, #0
	BEQ	%f1			; No PLE present
	
	MRC	p15,0,r1,c11,c1,0	; Read PLE UAR
	MRC	p15,0,r2,c11,c1,1	; Read PLE PCR
	STM	r0!, {r1, r2}
	
1	bx	lr	
	ENDFUNC

restore_a9_other	FUNCTION
	LDR	r1, [r0], #4
	ANDS	r1, r1, #0x01		; We only restore the Dynamic Clock gating bit
	MCR	p15,0,r1,c15,c0,0	; Write Power Control Register
	
	MRC	p15,0,r3,c0,c0,0	; Read Main ID Register
	UBFX	r3, r3, #20, #4		; Extract major version number
	CMP	r3, #2
	BLT	%f1			; PLE only possible in r2p0 onwards
	MRC	p15,0,r3,c11,c0,0	; Read PLE IDR
	CMP	r3, #0
	BEQ	%f1			; No PLE present
	
	LDM	r0!, {r1, r2}
	MCR	p15,0,r1,c11,c1,0	; Write PLE UAR
	MCR	p15,0,r2,c11,c1,1	; Write PLE PCR
	
1	bx	lr	
	ENDFUNC

	
	END



