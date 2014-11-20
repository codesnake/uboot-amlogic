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
    dmb
    isb
    mov sp,#0x4000
/*//for JTEG enable
       ldr r0,=0xda004000
       ldr r1,=0x80000510
       str r1,[r0,#4]
       mov r1,#1
       str r1,[r0]
*/
       //send debug status to ARC
       // ARM is running.
       LDR  r0, =0xc8100004 /*AO_RTI_STATUS_REG1*/
       mov  r1, #1
       str   r1, [r0]

       //restore MMC Security setting.
 		ldr r0,=0xda002020
 		ldr r1,=0xFFFF
 		str r1,[r0]      /*DMC_SEC_PORT0_RANGE0*/
 		str r1,[r0,#4]   /*DMC_SEC_PORT1_RANGE0*/
 		str r1,[r0,#8]   /*DMC_SEC_PORT2_RANGE0*/
 		str r1,[r0,#12]  /*DMC_SEC_PORT3_RANGE0*/
 		str r1,[r0,#16]  /*DMC_SEC_PORT4_RANGE0*/
 		str r1,[r0,#20]  /*DMC_SEC_PORT5_RANGE0*/
 		str r1,[r0,#24]  /*DMC_SEC_PORT6_RANGE0*/
 		str r1,[r0,#28]  /*DMC_SEC_PORT7_RANGE0*/
 		ldr r1,=0x80000000
 		str r1,[r0,#0x84] /*DMC_SEC_CTRL*/

       //enable MMC lower power mode.
       //LDR r1, =0x27
       //LDR r0, =0xc8006434    /*MMC_LP_CTRL4*/
       //STR r1, [r0]
       //LDR r1, =0x34f00f03
       //LDR r0, =0xc8006430    /*MMC_LP_CTRL3*/
       //STR r1, [r0]
       //LDR r1, =0x08160203
       //LDR r0, =0xc800642c    /*MMC_LP_CTRL2*/
       //STR r1, [r0]
       //LDR r1, =0x78000030
       //LDR r0, =0xc8006428    /*MMC_LP_CTRL1*/
       //STR r1, [r0]

        //enable MMC request.
 		mov r1,#0xFF
 		ldr r0,=0xc8006400
 		str r1,[r0]

      //re read write DDR SDRAM several times to make sure the AXI2DDR bugs dispear.
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];
      LDR  r0, =0x55555555;
      LDR  r1, =0x9fffff00;
      STR  r0, [r1];

      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      LDR  r1, =0x9fffff00
      LDR  r0, [r1]
      dmb
      isb
#if 0
      LDR r0, =0xc1104260 /*clrbits_le32(P_HHI_SYS_PLL_CNTL, (1 << 30));*/
      ldr r1, [r0]
      LDR r2, =0xbfffffff
      and r1, r1, r2
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
#endif
       //send debug status to ARC
       LDR  r0, =0xc8100004
       mov  r1, #2
       str   r1, [r0]

/* for debug */
    adr r1,reset_addr
    ldr r0,[r1]
 	  mov pc,r0
    mov r1, #0
dbg_loop:
		cmp r1,#0
		beq dbg_loop 
    wfi
    
pwr_undefined_instruction: 
	mov r0,#1
	b pwr_fail_sleep
pwr_software_interrupt:	
	mov r0,#2
	b pwr_fail_sleep
pwr_prefetch_abort:	
	mov r0,#3
	b pwr_fail_sleep
pwr_data_abort:
	mov r0,#4
	b pwr_fail_sleep		
pwr_not_used:		
	mov r0,#5
	b pwr_fail_sleep
pwr_irq:	
	mov r0,#6
	b pwr_fail_sleep		
pwr_fiq:
	mov r0,#7
	b pwr_fail_sleep	
pwr_fail_sleep:
	wfi

