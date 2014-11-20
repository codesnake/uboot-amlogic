/*
 * Copyright (C) 2008-2010 ARM Limited                           
 *
 * This software is provided 'as-is', without any express or implied
 * warranties including the implied warranties of satisfactory quality, 
 * fitness for purpose or non infringement.  In no event will  ARM be 
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use, copy and modify this software for 
 * any purpose, and to redistribute the software, subject to the following 
 * restrictions: 
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.                                       
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * ----------------------------------------------------------------
 * File:     appf_platform_api.h
 * ----------------------------------------------------------------
 */
 
extern struct appf_main_table main_table;

/* Functions in platform.c */
extern int appf_platform_boottime_init(void);
extern int appf_platform_runtime_init(void);
extern int appf_platform_late_init(struct appf_cluster *cluster);
extern int appf_platform_get_cpu_index(void);
extern int appf_platform_get_cluster_index(void);

/* Functions in power.c */
extern int appf_platform_power_up_cpu(struct appf_cpu *cpu, struct appf_cluster *cluster);
extern int appf_platform_enter_cstate(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster);
extern int appf_platform_leave_cstate(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster);
extern int appf_platform_enter_cstate1(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster);
extern int appf_platform_leave_cstate1(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster);

/* Functions in context.c */
extern int appf_platform_restore_context(struct appf_cluster *cluster, struct appf_cpu *cpu);
extern int appf_platform_save_context(struct appf_cluster *cluster, struct appf_cpu *cpu, unsigned flags);

/* Functions in reset.S */

/**
 * This function returns a suitable stack pointer for this CPU.
 *
 * The address returned must be the initial stack pointer - that
 * is, the address of the first byte AFTER the allocated region.
 *
 * The memory must be 8-byte aligned.
 */
extern unsigned appf_platform_get_stack_pointer(void);


/*
 * A 4KB page that will be mapped as DEVICE memory
 * Locks must be allocated here.
 */
extern unsigned appf_device_memory[1024];

/*
 * Addresses of translation tables, etc
 */
extern unsigned appf_translation_table1[];
extern unsigned appf_translation_table2a[];
extern unsigned appf_translation_table2b[];
extern unsigned appf_ttbr0, appf_ttbcr;
