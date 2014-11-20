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
 * File:     aem/platform.c
 * ----------------------------------------------------------------
 */
 
/** \file platform.c
 * This file contains code to set up the APPF tables, context save/restore space,
 * reset handling and stack space for this platform.
 *
 * Some code and data in this file must be linked in the entry point sections. 
 * Functions that need to be in the entry point must be given the following attribute:
 *     __attribute__ ((section ("APPF_ENTRY_POINT_CODE")))
 * 
 * APPF tables must be given the following attribute:
 *     __attribute__ ((section ("APPF_TABLE_DATA")))
 *
 */

#include "appf_types.h"
#include "appf_platform_api.h"
#include "appf_internals.h"
#include "appf_helpers.h"

#ifndef MAX_CPUS
#define MAX_CPUS 4
#endif
unsigned  reloc_addr(unsigned);
extern unsigned platform_cpu_stacks[MAX_CPUS];

/*
 * This will be copied to the correct place in memory at boottime_init
 * so that the OS knows where to find it.
 */
//ram_start; ram_size; num_clusters
static struct appf_main_table temp_main_table = {0x0,0x10000000,1};

/*
 * These tables go in the APPF_TABLE_DATA section 
 */
struct appf_cluster aem_cluster[] __attribute__ ((section ("APPF_TABLE_DATA")))
    = {{CPU_A9MPCORE,  //cpu_type
        0x0200,        //cpu_version
        2,             //num_cpus
        2,            //active_cpus
        0xC4300000,//0x1f000000,   //scu_address ,for multi-core
        0xc4300000,//0x1f001000,   //ic_address; interrupt controller
        0xC4200000,//0x1f002000,   //l2_address ; L2cc
        (void *)0,    // appf_cluster_context* context
        (void *)0,    //appf_cpu* cpu_table
        0}           // appf_i32 power_state 
	};
	
//appf_cpu: ic_address; *context; power_state;
struct appf_cpu aem_cpu[MAX_CPUS] __attribute__ ((section ("APPF_TABLE_DATA")))
     = {{0, (void *)0, 0},
        {0, (void *)0, 0},
        {0, (void *)0, 0},
        {0, (void *)0, 0}};        
/*     = {{0x1f000100, (void *)0, 0},
        {0x1f000100, (void *)0, 0},
        {0x1f000100, (void *)0, 0},
        {0x1f000100, (void *)0, 0}};
        */

static long long stack_and_context_memory[STACK_AND_CONTEXT_SPACE / sizeof(long long)];
/**
 * Simple memory allocator function.
 * Returns start address of allocated region (be careful when allocating descending stacks!)
 * Allocates region of size bytes, size will be rounded up to multiple of sizeof(long long)
 */
static unsigned get_memory(unsigned size)
{
    static unsigned watermark = 0;
    unsigned ret, chunks_required;
    
    ret = watermark;
    chunks_required = (size + sizeof(long long) - 1) / sizeof(long long);
    watermark += chunks_required;
    
    if (watermark >= STACK_AND_CONTEXT_SPACE / sizeof(long long))
    {
        while (1);  /* No output possible, so loop */
    }
    
    return (unsigned) &stack_and_context_memory[ret];
}

/**
 * This is where the reset vector jumps to.
 *
 * It needs to restore the endianness bit in the CPSR, and 
 * decide whether this is a warm-or-cold start
 */
extern void platform_reset_handler(void);

/**
 * This function is called at boot time. Memory will be flat-mapped, and the code will use 
 * the bootloader's stack . The code must examine the hardware and set up the APPF tables.
 * The values could alternatively be hard-coded if, for example, the platform always provides
 * certain hardware features. 
 *
 * The values for the master table must be written to the global variable
 * main_table, which is a pointer to the start of the entry point section.
 * All the other tables must be in the APPF_TABLE_DATA section.
 */
int appf_platform_boottime_init(void)
{
    int i;

    /* Ensure that the CPUs enter APPF code at reset */
    /* TODO: Consider high vectors */
		unsigned addr = update_mvbar();
		unsigned* pstacks;
	//	addr = 0x49000000;
	//	addr &= 0xFFFFFFF;
	//	dbg_wait();
//		addr = va_to_pa(addr);
	 // *(unsigned *)(addr) = reloc_addr((unsigned)&platform_reset_handler);
	 // *(unsigned *)(addr +0x20) = (unsigned)reloc_addr(&platform_reset_handler);
	
    /* Also ensure that the SMC instruction reaches APPF code */
 /*   addr = read_mvbar();
  	// if (read_mvbar() )
  	if(addr != 0 && (addr & 0xF) == 0)
    {
        *(unsigned *)(read_mvbar() + 0x20) = reloc_addr((unsigned)&appf_smc_handler);
    }*/
    
    /* Setup tables - Note that pointers are flat-mapped/physical addresses */
     if (aem_cluster[0].scu_address)
    {
        aem_cluster[0].num_cpus = num_cpus_from_a9_scu(aem_cluster[0].scu_address);
    }
    else
    {
        aem_cluster[0].num_cpus = 1;
    }

    aem_cluster[0].active_cpus        = aem_cluster[0].num_cpus;
    aem_cluster[0].cpu_table          = &aem_cpu[0];
    
    aem_cluster[0].context = (void *)get_memory(sizeof(struct appf_cluster_context));
    
    aem_cluster[0].context->gic_dist_shared_data = (void *)get_memory(GIC_DIST_SHARED_DATA_SIZE);
    aem_cluster[0].context->l2_data              = (void *)get_memory(L2_DATA_SIZE);
    aem_cluster[0].context->scu_data             = (void *)get_memory(SCU_DATA_SIZE);
    aem_cluster[0].context->global_timer_data    = (void *)get_memory(GLOBAL_TIMER_DATA_SIZE);
    aem_cluster[0].context->lock                 = (void *)reloc_addr((unsigned)&appf_device_memory);
    
    initialize_spinlock(aem_cluster[0].context->lock);

    for (i=0; i<aem_cluster[0].num_cpus; ++i)
    {
        aem_cpu[i].context = (void *)get_memory(sizeof(struct appf_cpu_context));
        aem_cpu[i].context->pmu_data              = (void *)get_memory(PMU_DATA_SIZE);
        aem_cpu[i].context->timer_data            = (void *)get_memory(TIMER_DATA_SIZE);
        aem_cpu[i].context->vfp_data              = (void *)get_memory(VFP_DATA_SIZE);
        aem_cpu[i].context->gic_interface_data    = (void *)get_memory(GIC_INTERFACE_DATA_SIZE);
        aem_cpu[i].context->gic_dist_private_data = (void *)get_memory(GIC_DIST_PRIVATE_DATA_SIZE);
        aem_cpu[i].context->banked_registers      = (void *)get_memory(BANKED_REGISTERS_SIZE);
        aem_cpu[i].context->cp15_data             = (void *)get_memory(CP15_DATA_SIZE);
        aem_cpu[i].context->debug_data            = (void *)get_memory(DEBUG_DATA_SIZE);
        aem_cpu[i].context->mmu_data              = (void *)get_memory(MMU_DATA_SIZE);
        aem_cpu[i].context->other_data            = (void *)get_memory(OTHER_DATA_SIZE);
        aem_cpu[i].context->flags = APPF_SAVE_PMU    |
                                    APPF_SAVE_TIMERS |
                                    APPF_SAVE_L2 |
                                    APPF_SAVE_VFP |
                                    APPF_SAVE_DEBUG;
                                   // APPF_SAVE_L2;
			  pstacks = (unsigned*)reloc_addr((unsigned)platform_cpu_stacks);
        pstacks[i] = get_memory(STACK_SIZE) + STACK_SIZE;
    }
    temp_main_table.cluster_table           = &aem_cluster[0];
    temp_main_table.num_clusters            = sizeof(aem_cluster) / sizeof(aem_cluster[0]);
    temp_main_table.entry_point             = (appf_entry_point_t *)reloc_addr((unsigned)appf_entry_point);
		/* Copy our temp table to the right place in memory */
    appf_memcpy((void *)reloc_addr((unsigned)&main_table), &temp_main_table, sizeof(struct appf_main_table));
    return APPF_OK;
}

/* 
 * The next three functions may be called using OS stack, with just the entry point mapped
 * so we put them in the APPF_ENTRY_POINT_CODE section
 */
int appf_platform_runtime_init(void)      __attribute__ ((section ("APPF_ENTRY_POINT_CODE")));
int appf_platform_get_cpu_index(void)     __attribute__ ((section ("APPF_ENTRY_POINT_CODE")));
int appf_platform_get_cluster_index(void) __attribute__ ((section ("APPF_ENTRY_POINT_CODE")));

/**
 * This function is called when the OS initializes the firmware at run time.
 * This function uses the OS stack and translation tables, and must be in the entry point section.
 * It is responsible for setting up the reset vector so it enters the platform-specific reset
 * code (in reset.S) and may update the cpu and cluster flags field based on the state
 * of the system (e.g. setting APPF_SAVE_L2 if the L2 cache controller is enabled)
 */
int appf_platform_runtime_init(void)
{
    /* TODO: Add flags to each CPU struct, as required by hardware (e.g. APPF_SAVE_L2 if L2 is on) */
    return APPF_OK;
}

/**
 * This function returns the index in the CPU table for the currently executing CPU.
 * Normally, reading the MPIDR is sufficient.
 *
 * This function must be in the entry point section.
 */
int appf_platform_get_cpu_index(void)
{
    return read_mpidr() & 0xff;
}

/**
 * This function returns the index in the cluster table for the currently executing CPU.
 * Normally, either returning zero or reading the MPIDR is sufficient.
 *
 * This function must be in the entry point section.
 */
int appf_platform_get_cluster_index(void)
{
	return 0;
  //  return (read_mpidr() >> 8) & 0xff;
}


/**
 * This function is called at the end of runtime initialization.
 *
 * It is called using APPF's translation tables and stack, by the same CPU that
 * did the early initialization.
 */
int appf_platform_late_init(struct appf_cluster *cluster)
{
    /*
     * Clean the APPF code and translation tables from L2 cache, if it's enabled
     * - this matters as we will disable the L2 during power down.
     */
    if (cluster->l2_address && is_enabled_pl310(cluster->l2_address))
    {
        clean_pl310(cluster->l2_address);
    }
    return APPF_OK;
}

