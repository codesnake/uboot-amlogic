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
 * File:     context.c
 * ----------------------------------------------------------------
 */
 
#include "appf_types.h"
#include "appf_platform_api.h"
#include "appf_internals.h"
#include "appf_helpers.h"
void dbg_wait(void);
/**
 * This function saves all the context that will be lost 
 * when a CPU and cluster enter a low power state.
 *
 * This function is called with cluster->context->lock held
 */
int appf_platform_save_context(struct appf_cluster *cluster, struct appf_cpu *cpu, unsigned flags)
{
    appf_u32 saved_items = 0;
    appf_u32 cluster_saved_items = 0;
    struct appf_cpu_context *context = cpu->context;
    struct appf_cluster_context *cluster_context = cluster->context;
    int cluster_down;
    
    dbg_prints("save step 1\n");

    /* Save perf. monitors first, so we don't interfere too much with counts */
    if (flags & APPF_SAVE_PMU)
    {
        save_performance_monitors(context->pmu_data);
        saved_items |= SAVED_PMU;
    }
    dbg_prints("save step 2\n");

    if (flags & APPF_SAVE_TIMERS)
    {
        save_a9_timers(context->timer_data, cluster->scu_address);
        saved_items |= SAVED_TIMERS;
    }
    dbg_prints("save step 3\n");

    if (flags & APPF_SAVE_VFP)
    {
        save_vfp(context->vfp_data);
        saved_items |= SAVED_VFP;
    }
    
    dbg_prints("save step 4\n");
		
		if(cluster->ic_address)
    	save_gic_interface(context->gic_interface_data, cluster->ic_address);
    	
	   if(cluster->ic_address)	
    	save_gic_distributor_private(context->gic_dist_private_data, cluster->ic_address);
    /* TODO: check return value and quit if nonzero! */
    dbg_prints("save step 5\n");

    save_banked_registers(context->banked_registers);
    
    save_cp15(context->cp15_data);
    save_a9_other(context->other_data);
    
    if (flags & APPF_SAVE_DEBUG)
    {
        save_a9_debug(context->debug_data);
        saved_items |= SAVED_DEBUG;
    }
    dbg_prints("save step 6\n");
    
    cluster_down = cluster->power_state >= 2;

  //  if (cluster_down)
    {
        if ((flags & APPF_SAVE_TIMERS) && cluster->cpu_version >= 0x0100)
        {	
            save_a9_global_timer(cluster_context->global_timer_data, cluster->scu_address);
            cluster_saved_items |= SAVED_GLOBAL_TIMER;
        }

        save_gic_distributor_shared(cluster_context->gic_dist_shared_data, cluster->ic_address);
        
    }
 
    save_control_registers(context);
    save_mmu(context->mmu_data);
    context->saved_items = saved_items;
    dbg_prints("save step 7\n");
    
  //  if (cluster_down)
    {
    		if(cluster->scu_address)
        	save_a9_scu(cluster_context->scu_data, cluster->scu_address);

        if (flags & APPF_SAVE_L2)
        {
            save_pl310(cluster_context->l2_data, cluster->l2_address);
            cluster_saved_items |= SAVED_L2;
        }
        cluster_context->saved_items = cluster_saved_items;
    }
    dbg_prints("save step 8\n");

    /* 
     * DISABLE DATA CACHES
     *
     * First, disable, then clean+invalidate the L1 cache.
     *
     * Note that if L1 was to be dormant and we were the last CPU, we would only need to clean some key data
     * out of L1 and clean+invalidate the stack.
     */
    asm volatile("mov r0,#0");
    asm volatile("mcr p15, 0, r0, c7, c5, 0");

    		
		disable_clean_inv_dcache_v7_l1();


    /* 
     * Next, disable cache coherency
     */
    if (cluster->scu_address)
    {
        write_actlr(read_actlr() & ~A9_SMP_BIT);
    }
    dbg_prints("save step 9\n");

    /*
     * If the L2 cache is in use, there is still more to do.
     *
     * Note that if the L2 cache is not in use, we don't disable the MMU, as clearing the C bit is good enough.
     */
    if (flags & APPF_SAVE_L2)
    {
        /*
         * Disable the MMU (and the L2 cache if necessary), then clean+invalidate the stack in the L2.
         * This all has to be done one assembler function as we can't use the C stack during these operations.
         */
        disable_clean_inv_cache_pl310(cluster->l2_address, appf_platform_get_stack_pointer() - STACK_SIZE, 
                                      STACK_SIZE, cluster_down);
  
        /*
         * We need to partially or fully clean the L2, because we will enter reset with cacheing disabled
         */
        if (cluster_down)
        {
            /* Clean the whole thing */
            clean_pl310(cluster->l2_address);
        }
        else
        {
            /* 
	     * L2 staying on, so just clean everything this CPU will need before the MMU is reenabled
	     *
             * TODO: some of this data won't change after boottime init, could be cleaned once during late_init
	     */
            clean_range_pl310(cluster,           sizeof(struct appf_cluster),         cluster->l2_address);
            clean_range_pl310(cpu,               sizeof(struct appf_cpu),             cluster->l2_address);
            clean_range_pl310(context,           sizeof(struct appf_cpu_context),     cluster->l2_address);
            clean_range_pl310(context->mmu_data, MMU_DATA_SIZE,                       cluster->l2_address);
            clean_range_pl310(cluster_context,   sizeof(struct appf_cluster_context), cluster->l2_address);
        }
    }
    dbg_prints("save step 10\n");

    return APPF_OK;
}

/** 
 * This function restores all the context that was lost 
 * when a CPU and cluster entered a low power state. It is called shortly after
 * reset, with the MMU and data cache off.
 *
 * This function is called with cluster->context->lock held
 */
int appf_platform_restore_context(struct appf_cluster *cluster, struct appf_cpu *cpu)
{
    struct appf_cpu_context *context;
    struct appf_cluster_context *cluster_context;
    int cluster_init;
    appf_u32 saved_items, cluster_saved_items = 0;
    /*
     * At this point we may not write to any static data, and we may
     * only read the data that we explicitly cleaned from the L2 above.
     */
    cluster_context = cluster->context;
    context = cpu->context;

    /* Should we initialize the cluster: are we the first CPU back on, and has the cluster been off? */
    cluster_init = (cluster->active_cpus == 0 && cluster->power_state >= 2);
    saved_items = context->saved_items;
    
    /* First set up the SCU & L2, if necessary */
  //  if (cluster_init)
    {
        cluster_saved_items = cluster_context->saved_items;
        if(cluster->scu_address)
        	restore_a9_scu(cluster_context->scu_data, cluster->scu_address);
        if (cluster_saved_items & SAVED_L2)
        {
            restore_pl310(cluster_context->l2_data, cluster->l2_address, cluster->power_state == 2);
        }
    }

    /* Next get the MMU back on */
    restore_mmu(context->mmu_data);
    restore_control_registers(context);
        
    /* 
     * MMU and L1 and L2 caches are on, we may now read/write any data.
     * Now we need to restore the rest of this CPU's context 
     */

    /* Get the debug registers restored, so we can debug most of the APPF code sensibly! */    
    if (saved_items & SAVED_DEBUG)
    {
        restore_a9_debug(context->debug_data);
    }

    /* Restore shared items if necessary */
//    if (cluster_init)
    {
        restore_gic_distributor_shared(cluster_context->gic_dist_shared_data, cluster->ic_address);
        if (cluster_saved_items & SAVED_GLOBAL_TIMER)
        {
            restore_a9_global_timer(cluster_context->global_timer_data, cluster->scu_address);
        }
    }

		if(cluster->ic_address)
    	restore_gic_distributor_private(context->gic_dist_private_data, cluster->ic_address);
    if(cluster->ic_address)
    	restore_gic_interface(context->gic_interface_data, cluster->ic_address);
    if(context->other_data)
    	restore_a9_other(context->other_data);
    if(context->cp15_data)
    	restore_cp15(context->cp15_data);
    if(context->banked_registers)
    	restore_banked_registers(context->banked_registers);

    if (saved_items & SAVED_VFP)
    {
        restore_vfp(context->vfp_data);
    }

    if (saved_items & SAVED_TIMERS)
    {
        restore_a9_timers(context->timer_data, cluster->scu_address);
    }

    if (saved_items & SAVED_PMU)
    {
        restore_performance_monitors(context->pmu_data);
    }

    /* Return to OS */
    return APPF_OK;
}
