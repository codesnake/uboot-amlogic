/*
 * aml8726m M3
 * @author wenwu.zhang
 *
 */
#include <config.h>
#include <asm/arch/romboot.h>

.globl pwr_reboot
pwr_reboot: 
	b	pwr_restart
	b pwr_undefined_instruction
	b pwr_software_interrupt
	b pwr_prefetch_abort
	b pwr_data_abort
	b pwr_not_used
	b pwr_irq
	b pwr_fiq
reset_addr:
	.word 0
pwr_restart:

/*//for JTEG enable
       ldr r0,=0xda004000
       ldr r1,=0x80000510
       str r1,[r0,#4]
       mov r1,#1
       str r1,[r0]
*/


	MRC p15, 0, r0, c0, c0, 5   @ Read MPIDR
   AND r0, #0x3                @ Get CPU ID
   ldr r3,_cpu_stat
   mov     r2,#0               @ cpu_stat[r0]=0
   str     r2, [r3, r0,LSL #2]
   CMP r0, #0                  @ Is this CPU0
   BNE not_cpu0

//send debug status to ARC
// ARM is running.
	LDR  r0, =0xc8100004 /*AO_RTI_STATUS_REG1*/
	mov  r1, #1
	str   r1, [r0]
	dsb

	dmb
	isb
	mov sp,#0x7800 /*2k before arc sp: 0x8000*/
/*
	cps #0x16 //switch to monitor user mode
	mrc p15, 0, r0, c1, c1, 0  @ Read Secure Configuration Register
	bic 	r0, r0, #1
	mcr p15, 0, r0, c1, c1, 0  @ Write Secure Configuration Register
*/
	LDR r0, =0xc1104260 /*setbits_le32(P_HHI_SYS_PLL_CNTL, (1 << 30));*/
	ldr r1, [r0]
	LDR r2, =0x40000000
	orr r1, r1, r2
	str r1, [r0]

	LDR r0, =0xc110419c /*setbits_le32(P_HHI_SYS_CPU_CLK_CNTL , (1 << 7));*/
	ldr r1, [r0]
	mov r2, #0x80
	orr r1, r1, r2
	str r1, [r0]

	mov r0,#0
	ldr r1, =0x1000
delay_loop:
	nop
	nop
	nop
	nop
	add r0, r0, #1
	cmp r0, r1
	bne delay_loop

#if 1
	LDR  r9, =0xc8100004 /*AO_RTI_STATUS_REG1*/
	mov	r10, #11
	str	 r10, [r9]
	dsb
	
	//restore MMC Security setting.
	ldr r0,=0xda002000 //DMC_SEC_RANGE0_CTRL
	ldr r1,=0xffff0000
	str r1,[r0]
	
	ldr r1,=0xffffffff
	str r1,[r0,#4]	/*DMC_SEC_RANGE1_CTRL*/

	str r1,[r0,#8]	/*DMC_SEC_RANGE2_CTRL*/
	str r1,[r0,#12]  /*DMC_SEC_RANGE3_CTRL*/
	str r1,[r0,#16]  /*DMC_SEC_AXI_PORT_CTRL*/
	str r1,[r0,#20]  /*DMC_SEC_AM_PORT_CTRL*/

	ldr r0, =0xda002024
	str r1,[r0]
	str r1,[r0,#4]

	ldr r0, =0xda002018
	ldr r1,=0x80000000
	str r1,[r0]

	ldr r0, =0xc8006000//#P_DMC_REQ_CTRL
	ldr r1,=0xffff
	str r1,[r0]

/*
wait_dmc_sec_ctrl: //while( readl(DMC_SEC_CTRL) & (1<<31)) {}
	ldr r2, [r0]
	and r2, r2, r1
	cmp r2, #0
	bne wait_dmc_sec_ctrl
*/
#endif
       LDR  r0, =0xc8100004 /*AO_RTI_STATUS_REG1*/
       mov  r1, #12
       str   r1, [r0]
       dsb


	   NOP
	   DMB
	   ISB
	   
	   LDR r1, =0xc4200c04
	   LDR r0, =0xbff00000
	   STR r0, [r1]
	   
	   LDR r1, =0xc4200c00
	   LDR r0, =0x01000001
	   STR r0, [r1]
	   
	   NOP
	   DMB
	   ISB

#if 1
		//enable all channel
//		ldr r1,=0xFFF
//		ldr r0,=0xc8006204
//		str r1,[r0]


	//send debug status to ARC
	LDR  r0, =0xc8100004
	mov  r1, #2
	str   r1, [r0]
	dsb

/*Retore ddr data which is rewrite by ddr trainning. See power.c for more info*/
	ldr r0, =0x10000 //The sram address which save the data
	ldr r1, [r0]

//	ldr r5, =0xffffffff
//	cmp r1, r5
//	beq store_ch2

	mov r4, #32 //trainning len
store_loop1:
	ldr r3, [r0, #4]!
	str r3, [r1], #4
	sub r4, r4, #1
	cmp r4, #0
	bne store_loop1

//send debug status to ARC
	LDR  r0, =0xc8100004
	mov  r1, #3
	str   r1, [r0]
	dsb

#endif
/* for debug */
//    adr r1,reset_addr
//    ldr r0,[r1]

	adr r1,reset_addr
	ldr r0,[r1]

//dbg_loop__:
//	b dbg_loop__


	mov pc,r0
	mov r1, #0
dbg_loop:
	cmp r1,#0
	beq dbg_loop 
    wfi

	
not_cpu0:
	mov r2,#1
	mov r2,r2, asl r0
	orr r2,r2,#1
not_cpu0_loop:
	ldr r3,_cpu_stat
	ldr r1,[r3]
	and r1,r2,r1
	cmp r1,r2
	wfene				@wait for event 
	bne   not_cpu0_loop
	ldr 	pc, [r3, r0, asl #2]
_cpu_stat:
	.word	0x01ff80

pwr_undefined_instruction: 
	mov r0,#40
	b pwr_fail_sleep
pwr_software_interrupt:	
	mov r0,#41
	b pwr_fail_sleep
pwr_prefetch_abort:	
	mov r0,#42
	b pwr_fail_sleep
pwr_data_abort:
	mov r0,#43
	b pwr_fail_sleep		
pwr_not_used:		
	mov r0,#44
	b pwr_fail_sleep
pwr_irq:	
	mov r0,#45
	b pwr_fail_sleep		
pwr_fiq:
	mov r0,#46
	b pwr_fail_sleep	
pwr_fail_sleep:
	LDR  r1, =0xc8100004 /*AO_RTI_STATUS_REG1*/
	;mov  r1, #9
	str   r0, [r1]
	dsb
	wfi

