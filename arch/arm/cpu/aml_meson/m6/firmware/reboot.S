/*
 * aml8726m M3
 * @author wenwu.zhang
 *
 */
#include <config.h>
#include <asm/arch/romboot.h>

#define CONFIG_RST_ENV_BASE 0x00EF00
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
pwr_restart:
    dmb
    isb
    /* for debug */
    //wfi
     
		/********************************************/
		/* restore reboot enviroment                */    
    ldr r0, =CONFIG_RST_ENV_BASE
    ldr sp,[r0,#0]
       
		/* common registers */
    ldr r2,[r0,#12]
    ldr r3,[r0,#16]
    ldr r4,[r0,#20]
    ldr r5,[r0,#24]
    ldr r6,[r0,#28]
    ldr r7,[r0,#32]
    ldr r8,[r0,#36]
    ldr r9,[r0,#40]
    ldr r10,[r0,#44]
    ldr r11,[r0,#48]
    ldr r12,[r0,#52]
    
		/* lr and vector settings */    
    ldr lr,[r0,#56]
    ldr r1,[r0,#60]
    mcr p15,0,r1,c12,c0,0 /*restore vector base addr*/
    ldr r1,[r0,#64]
    mcr p15,0,r1,c12,c0,1 /*restore monitor vector base addr*/
    
 		ldr r1,[r0,#8]
	  MCR p15,0,r1,c2,c0,0      		// setup TTB0
	  
  	/*restore system control. bit[12]:icache, bit[2]:dcache*/
    ldr r1,[r0,#4]
    mcr p15,0,r1,c1,c0,0
    

 		/* return to powerdown location*/
    mov pc,lr
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
  // setup stack pointer (relocated 0x49002000) 			
	mov r1 , pc
	ldr r2 , =0xFFFF0000
	and r1, r1, r2
	add r1, r1, #0x2000
	mov sp, r1
	bl      disp_exception
	wfi
	
disp_exception:
	push	{r3, r4, r5, lr}
	mov r5, r0
	mov r0, pc
l_cur:
	add r0, #(exp_str-l_cur-4)
	bl  serial_puts
	mov r4,#32
loop0:	
	sub r4,#4
  mov	r0, r5, LSR R4
  and r0, #0xF
  cmp r0, #9
  bls le_9 
  add r0,r0,#7
le_9:
	add r0,r0,#0x30
	bl serial_putc
	cmp r4,#0
	bne loop0
  ldr r2, = 0x4f4240
  ldr r3, = 0xc1109900
  str	r2, [r3, #0] // enable watchdog			
	pop {r3, r4, r5, pc}
	
exp_str:
	.asciz "Enter Exception: 0x"
  