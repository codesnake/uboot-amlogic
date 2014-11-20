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
 * File:     appf_helpers.h
 * ----------------------------------------------------------------
 */
 
extern unsigned va_to_pa(unsigned virtual_address);

extern unsigned read_mpidr(void);
extern unsigned read_sctlr(void);
extern unsigned write_sctlr(unsigned value);
extern unsigned read_actlr(void);
extern unsigned write_actlr(unsigned value);
extern unsigned read_prrr(void);
extern unsigned read_nmrr(void);
extern unsigned read_mvbar(void);

extern void dsb(void);
extern void dmb(void);
extern void wfi(void);
extern void wfe(void);
extern void sev(void);

extern unsigned * copy_words(volatile unsigned *destination, volatile unsigned *source, unsigned num_words);
extern void appf_memcpy(void *dst, const void *src, unsigned length);
extern void appf_memset(void *dst, unsigned value, unsigned length);

extern void initialize_spinlock(bakery_t *bakery);
extern void get_spinlock(unsigned cpuid, bakery_t *bakery);
extern void release_spinlock(unsigned cpuid, bakery_t *bakery);

/*
 * V7 functions
 */
extern void save_control_registers(struct appf_cpu_context *context);
extern void save_mmu(appf_u32 *pointer);
extern void save_performance_monitors(appf_u32 *pointer);
extern void save_banked_registers(appf_u32 *pointer);
extern void save_cp15(appf_u32 *pointer);
extern void save_vfp(appf_u32 *pointer);
extern void restore_control_registers(struct appf_cpu_context *context);
extern void restore_mmu(appf_u32 *pointer);
extern void restore_performance_monitors(appf_u32 *pointer);
extern void restore_banked_registers(appf_u32 *pointer);
extern void restore_cp15(appf_u32 *pointer);
extern void restore_vfp(appf_u32 *pointer);

extern void disable_clean_inv_dcache_v7_l1(void);
extern void disable_clean_inv_cache_pl310(unsigned pl310_address, unsigned stack_start, unsigned stack_size, int disable);
extern void clean_dcache_v7_l1(void);
extern void clean_mva_dcache_v7_l1(void *mva);
extern void invalidate_icache_v7_pou(void);
extern void invalidate_dcache_v7(void);

extern void appf_smc_handler(void);
extern void enter_secure_monitor_mode(void);
extern void enter_nonsecure_svc_mode(void);
extern void set_security_state(int ns);

/*
 * PL310 functions
 */
extern void clean_inv_range_pl310(void *start, unsigned size, unsigned pl310_address);  /* addresses are inclusive */
extern void clean_range_pl310(void *start, unsigned size, unsigned pl310_address);  /* addresses are inclusive */
extern void inv_range_pl310(void *start, unsigned size, unsigned pl310_address);
extern void clean_inv_pl310(unsigned pl310_address);
extern void clean_pl310(unsigned pl310_address);
extern void save_pl310(appf_u32 *pointer, unsigned pl310_address);
extern void restore_pl310(appf_u32 *pointer, unsigned pl310_address, int dormant);
extern void set_status_pl310(unsigned status, unsigned pl310_address);
extern int is_enabled_pl310(unsigned pl310_address);

/*
 * GIC functions
 */
extern void save_gic_interface(appf_u32 *pointer, unsigned gic_interface_address);
extern int save_gic_distributor_private(appf_u32 *pointer, unsigned gic_distributor_address);
extern int save_gic_distributor_shared(appf_u32 *pointer, unsigned gic_distributor_address);
extern void restore_gic_interface(appf_u32 *pointer, unsigned gic_interface_address);
extern void restore_gic_distributor_private(appf_u32 *pointer, unsigned gic_distributor_address);
extern void restore_gic_distributor_shared(appf_u32 *pointer, unsigned gic_distributor_address);

/*
 * A9 functions
 */
extern void save_a9_timers(appf_u32 *pointer, unsigned scu_address);
extern void save_a9_global_timer(appf_u32 *pointer, unsigned scu_address);
extern void save_a9_debug(appf_u32 *pointer);
extern void save_a9_other(appf_u32 *pointer);

extern void restore_a9_timers(appf_u32 *pointer, unsigned scu_address);
extern void restore_a9_global_timer(appf_u32 *pointer, unsigned scu_address);
extern void restore_a9_debug(appf_u32 *pointer);
extern void restore_a9_other(appf_u32 *pointer);

/*
 * A9 SCU functions
 */
extern void save_a9_scu(appf_u32 *pointer, unsigned scu_address);
extern void restore_a9_scu(appf_u32 *pointer, unsigned scu_address);
extern void set_status_a9_scu(unsigned cpu_index, unsigned status, unsigned scu_address);
extern int num_cpus_from_a9_scu(unsigned scu_address);

extern void update_offset(void);
extern unsigned  reloc_addr(unsigned);

#define __V(addr) (*((unsigned*)reloc_addr(&addr)))

/*
 * GCC Compatibility
 */
#ifndef __ARMCC_VERSION
#define __nop() __asm__ __volatile__( "nop\n" )
#endif
