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

	AREA APPF, CODE, ALIGN=6

	IMPORT appf_reset_entry_point
	EXPORT platform_cpu_stacks
	EXPORT platform_reset_handler
        EXPORT appf_platform_get_stack_pointer
    
	PRESERVE8      

platform_reset_handler
        ; TODO: Restore Endianness bit(s) in CPSR
	; TODO: Check for a warm start - either here or in boot monitor
                
	b	appf_reset_entry_point


	AREA APPF_ENTRY_POINT_CODE, CODE

appf_platform_get_stack_pointer	
  adr	r1, platform_cpu_stacks
	mrc	p15, 0, r0, c0, c0, 5	; Read MPIDR
  and	r0, r0, #0xff		; extract CPU number
  ldr	r0, [r1, r0, lsl #2]
	bx	lr        

platform_cpu_stacks
	SPACE	16

    END
