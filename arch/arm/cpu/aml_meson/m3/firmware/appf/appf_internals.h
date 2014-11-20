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
 * File:     appf_internals.h
 * ----------------------------------------------------------------
 */

 
#define TRUE  1
#define FALSE 0

#define A9_SMP_BIT (1<<6)

/*
 * Global variables 
 */
extern int flat_mapped;
extern int use_smc;
extern int is_smp;
extern struct appf_main_table main_table;
extern appf_u32 appf_translation_table1[];
extern appf_u32 appf_translation_table2[];
extern unsigned appf_runtime_call_flat_mapped;
extern unsigned appf_device_memory_flat_mapped;

/*
 * Entry points
 */
extern appf_entry_point_t appf_entry_point;
extern int appf_warm_reset(void);

/*
 * Context save/restore
 */
extern int save_cpu_context(struct appf_cluster *cluster, struct appf_cpu *cpu, unsigned flags);


int appf_setup_translation_tables(void);

/* 
 * Power statuses
 */
#define STATUS_RUN      0
#define STATUS_STANDBY  1
#define STATUS_DORMANT  2
#define STATUS_SHUTDOWN 3
 
/*
 * Flags to record which items of CPU context have been saved
 */
#define SAVED_VFP              (1<<0)
#define SAVED_PMU              (1<<1)
#define SAVED_DEBUG            (1<<2)
#define SAVED_TIMERS           (1<<3)

/*
 * Flags to record which items of cluster context have been saved
 */
#define SAVED_L2               (1<<16)
#define SAVED_GLOBAL_TIMER     (1<<17)

/* TODO: check these sizes */
#define STACK_AND_CONTEXT_SPACE 16384
#define STACK_SIZE 1024

/*
 * Maximum size of each item of context, in bytes
 * We round these up to 32 bytes to preserve cache-line alignment
 */

#define PMU_DATA_SIZE               128
#define TIMER_DATA_SIZE             128
#define VFP_DATA_SIZE               288
#define GIC_INTERFACE_DATA_SIZE      64
#define GIC_DIST_PRIVATE_DATA_SIZE   96
#define BANKED_REGISTERS_SIZE       128
#define CP15_DATA_SIZE               64
#define DEBUG_DATA_SIZE            1024
#define MMU_DATA_SIZE                64
#define OTHER_DATA_SIZE              32

#define GIC_DIST_SHARED_DATA_SIZE  2592
#define SCU_DATA_SIZE                32
#define L2_DATA_SIZE                256
#define GLOBAL_TIMER_DATA_SIZE      128


/*
 * Lamport's Bakery algorithm for spinlock handling
 *
 * Note that the algorithm requires the bakery struct
 * to be in Strongly-Ordered memory.
 */

#define MAX_CPUS 4

/** 
 * Bakery structure - declare/allocate one of these for each lock.
 * A pointer to this struct is passed to the lock/unlock functions.
 */
typedef struct 
{
    volatile char entering[MAX_CPUS];
    volatile unsigned number[MAX_CPUS];
} bakery_t;

extern void init_bakery_spinlock(bakery_t *bakery);
extern void get_bakery_spinlock(unsigned cpuid, bakery_t *bakery);
extern void release_bakery_spinlock(unsigned cpuid, bakery_t *bakery);


/*
 * Structures we hide from the OS API
 */

struct appf_cpu_context
{
    /*
     * Maintain the correlation between this structure and 
     * the offsets in cpu_helpers.S 
     */
    appf_u32 endianness;
    appf_u32 actlr;
    appf_u32 sctlr;
    appf_u32 cpacr;
    appf_u32 flags;
    appf_u32 saved_items;
    appf_u32 *pmu_data;
    appf_u32 *timer_data;
    appf_u32 *vfp_data;
    appf_u32 *gic_interface_data;
    appf_u32 *gic_dist_private_data;
    appf_u32 *banked_registers;
    appf_u32 *cp15_data;
    appf_u32 *debug_data;
    appf_u32 *mmu_data;
    appf_u32 *other_data;
};

struct appf_cluster_context
{
    bakery_t *lock;
    appf_u32 saved_items;
    appf_u32 *global_timer_data;
    appf_u32 *gic_dist_shared_data;
    appf_u32 *l2_data;
    appf_u32 *scu_data;
};

