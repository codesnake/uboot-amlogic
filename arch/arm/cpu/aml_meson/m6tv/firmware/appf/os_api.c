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
 * File:     os_api.c
 * ----------------------------------------------------------------
 */
 
#include "appf_types.h"
#include "appf_platform_api.h"
#include "appf_internals.h"
#include "appf_helpers.h"
void dbg_wait(void);
static int late_init(void);
static int power_down_cpu(unsigned cstate, unsigned rstate, unsigned flags);
static int power_up_cpus(unsigned cluster_index, unsigned cpus);
/**
 * This function handles the OS firmware calls. It is called from entry.S,
 * just after the OS translation tables and stack have been switched for the
 * firmware translation tables and stack. Returning from this function
 * returns to the OS via some code in entry.S.
 */
int appf_runtime_call(unsigned function, unsigned arg1, unsigned arg2, unsigned arg3)
{

    switch (function)
    {
    case APPF_INITIALIZE:
        return late_init();
    case APPF_POWER_DOWN_CPU:
        return power_down_cpu(arg1, arg2, arg3);
    case APPF_POWER_UP_CPUS:
        return power_up_cpus(arg1, arg2);
    }
    return APPF_BAD_FUNCTION;
}

/**
 * This function is called after a warm reset, from entry.S.
 */

extern void pwr_delay(int n);

int appf_warm_reset(void)
{
    int ret;
		writel(0,0xC810001C);
		writel(0,0xC8100020);
		writel(0,0xC110990C);
    
    struct appf_cpu *cpu;
    struct appf_cluster *cluster;
    int cpu_index, cluster_index;

	//**********************//
//	f_serial_puts("************A******\n");
	//writel(readl(0xDA00434c)&(~(0x1<<29)),0xDA00434c);// Disable GPO filter for 32k
	writel(0x3600000,0xDA00434c);// Disable GPO filter for 32k
	//**********************//

	writel(secure_reg_1,0xDA004004);//restore secure_reg_1
    
    cpu_index = appf_platform_get_cpu_index();
    cluster_index = appf_platform_get_cluster_index();

    cluster = main_table.cluster_table + cluster_index;
    cpu = cluster->cpu_table + cpu_index;

    get_spinlock(cpu_index, cluster->context->lock);

    appf_platform_restore_context(cluster, cpu);
    ret = appf_platform_leave_cstate(cpu_index, cpu, cluster);
    
    release_spinlock(cpu_index, cluster->context->lock);

		pwr_delay(10);
		
    return ret;
}


/** 
 * This function is called while running flat-mapped, on the boot loader's stack.
 */
int appf_boottime_init(void) __attribute__ ((section ("APPF_BOOT_ENTRY_POINT")));
int appf_boottime_init(void)
{
    /* Set up stack pointers per CPU, per cluster */
    /* Discover devices, set up tables */
		update_offset();
    appf_platform_boottime_init();

    *((unsigned*)reloc_addr((unsigned)&appf_runtime_call_flat_mapped)) =  (unsigned)appf_runtime_call;
    *((unsigned*)reloc_addr((unsigned)&appf_device_memory_flat_mapped)) = reloc_addr((unsigned)appf_device_memory);
		appf_setup_translation_tables();
    return 0;
}


/*
 * The next function is called using OS stack, with just the entry point mapped
 * so we put it (and its globals) in the APPF_ENTRY_POINT_* sections
 */
int __attribute__ ((section ("APPF_ENTRY_POINT_DATA")))  use_smc = FALSE;
int __attribute__ ((section ("APPF_ENTRY_POINT_DATA"))) is_smp = FALSE;

/**
 * This function is called when the OS makes a firmware call with the 
 * function code APPF_INITIALIZE
 *
 * It is called using the OS translation tables and stack, with only the 
 * entry point is mapped in, so it cannot call any functions that are 
 * not linked in the entry point.
 */

int appf_runtime_init(void) __attribute__ ((section ("APPF_ENTRY_POINT_CODE")));
int appf_runtime_init(void)
{
    int ret,i;
    unsigned va, pa;
    struct appf_main_table* pmaintable = (struct appf_main_table*)reloc_addr((unsigned)&main_table);
    struct appf_cluster* pcluster;
    
    update_offset();
      
    /* TODO: read U bit */
    is_smp = FALSE;

    pa = reloc_addr((unsigned)&main_table);
    va = (unsigned)&main_table;
    if (pa == va)
    {
    	 __V(flat_mapped) = TRUE;
    }
 /*   
    if (__V(flat_mapped) == 0 && use_smc == 0)
    {
    		ret = 0;
        ret = appf_setup_translation_tables();
        if (ret < 0)
        {
            return ret;
        }
    }*/

    return appf_platform_runtime_init();
}


/**
 * This function is called when the OS makes a firmware call with the 
 * function code APPF_INTIALIZE.
 *
 * It is called using the APPF translation tables that were set up in
 * appf_runtime_init, above.
 */
static int late_init(void)
{
    struct appf_cluster *cluster;
    int cluster_index;
    unsigned maintable_pa;
		dbg_prints("late_init ...\n");
    cluster_index = appf_platform_get_cluster_index();
    
    maintable_pa = reloc_addr((unsigned)&main_table);
    cluster = ((struct appf_main_table*)maintable_pa)->cluster_table + cluster_index;
     /*
     * Clean the translation tables out of the L1 dcache
     * (see comments in disable_clean_inv_dcache_v7_l1)
     */
    dsb();
    
    clean_dcache_v7_l1();

    return appf_platform_late_init(cluster);
}



#define         MODE_DELAYED_WAKE       0
#define         MODE_IRQ_DELAYED_WAKE   1
#define         MODE_IRQ_ONLY_WAKE      2

static void auto_clk_gating_setup(
	unsigned int sleep_dly_tb, unsigned int mode, unsigned int clear_fiq, unsigned int clear_irq,
	unsigned int   start_delay,unsigned int   clock_gate_dly,unsigned int   sleep_time,unsigned int   enable_delay)
{
	writel((sleep_dly_tb << 24)    |   // sleep timebase
		(sleep_time << 16)      |   // sleep time
		(clear_irq << 5)        |   // clear IRQ
		(clear_fiq << 4)        |   // clear FIQ
		(mode << 2),								// mode
		0xc1100000 + 0x1078*4);//P_HHI_A9_AUTO_CLK0                
	writel(	(0 << 20)               |   // start delay timebase
		(enable_delay << 12)    |   // enable delay
		(clock_gate_dly << 8)   |   // clock gate delay
		(start_delay << 0),  				// start delay
		0xc1100000 + 0x1079*4);    //P_HHI_A9_AUTO_CLK1
	
	writel(readl(0xc1100000 + 0x1078*4) | 1<<0,0xc1100000 + 0x1078*4);
	//SET_CBUS_REG_MASK(HHI_A9_AUTO_CLK0, 1 << 0);
}



/**
 * This function is called when the OS makes a firmware call with the 
 * function code APPF_POWER_DOWN_CPU
 */
static int power_down_cpu(unsigned cstate, unsigned rstate, unsigned flags)
{
    struct appf_cpu *cpu;
    struct appf_cluster *cluster;
    int cpu_index, cluster_index;
    int i, rc, cluster_can_enter_cstate1;
    struct appf_main_table* pmaintable = (struct appf_main_table*)reloc_addr((unsigned)&main_table);
#ifdef USE_REALVIEW_EB_RESETS
    int system_reset = FALSE, last_cpu = FALSE;
#endif
    cpu_index = appf_platform_get_cpu_index();
    cluster_index = appf_platform_get_cluster_index();
	 
    cluster = pmaintable->cluster_table;
    cluster += cluster_index;
    
    dbg_print("cluster:",(int)cluster);
    
    cpu = cluster->cpu_table;
    cpu += cpu_index;   
   
    dbg_print("cpu:",cpu_index);
    dbg_print("cluster_index:",cluster_index);

    /* Validate arguments */
    if (cstate > 3)
    {
        return APPF_BAD_CSTATE;
    }
    if (rstate > 3)
    {
        return APPF_BAD_RSTATE;
    }
    /* If we're just entering standby mode, we don't mark the CPU as inactive */
    if (cstate == 1)
    {
        get_spinlock(cpu_index, cluster->context->lock);
        cpu->power_state = 1;
        
        /* See if we can make the cluster standby too */
        if (rstate == 1)
        {
            cluster_can_enter_cstate1 = TRUE;
            for(i=0; i<cluster->num_cpus; ++i)
            {
                if (cluster->cpu_table[i].power_state != 1)
                {
                    cluster_can_enter_cstate1 = FALSE;
                    break;
                }
            }
            if (cluster_can_enter_cstate1)
            {
                cluster->power_state = 1;
            }
        }
                
        rc = appf_platform_enter_cstate1(cpu_index, cpu, cluster);

        if (rc == 0)
        {
            release_spinlock(cpu_index, cluster->context->lock);
            dsb();
            wfi();
            get_spinlock(cpu_index, cluster->context->lock);
            rc = appf_platform_leave_cstate1(cpu_index, cpu, cluster);
        }
        
        cpu->power_state = 0;
        cluster->power_state = 0;
        release_spinlock(cpu_index, cluster->context->lock);
        return rc;
    }

    /* Ok, we're not just entering standby, so we are going to lose the context on this CPU */
		dbg_prints("step1\n");
	  get_spinlock(cpu_index, cluster->context->lock);
    --cluster->active_cpus;
		dbg_prints("step2\n");
		
    cpu->power_state = cstate;
    if (cluster->active_cpus == 0)
    {
        cluster->power_state = rstate;
#ifdef USE_REALVIEW_EB_RESETS
        /* last CPU down must not issue WFI, or we get stuck! */
        last_cpu = TRUE;
        if (rstate > 1)
        {
            system_reset = TRUE;
        }
#endif
    }
  
    /* add flags as required by hardware (e.g. APPF_SAVE_L2 if L2 is on) */
    flags |= cpu->context->flags;
    appf_platform_save_context(cluster, cpu, flags);
		
		dbg_prints("step3\n");

    /* Call the platform-specific shutdown code */
    rc = appf_platform_enter_cstate(cpu_index, cpu, cluster);
   
     /* Did the power down succeed? */
    if (rc == APPF_OK)
    {

        release_spinlock(cpu_index, cluster->context->lock);

        while (1) 
        {
#if 0
#if defined(NO_PCU) || defined(USE_REALVIEW_EB_RESETS)
            extern void platform_reset_handler(unsigned, unsigned, unsigned, unsigned);
            void (*reset)(unsigned, unsigned, unsigned, unsigned) = platform_reset_handler;

#ifdef USE_REALVIEW_EB_RESETS
            /* Unlock system registers */
            *(volatile unsigned *)0x10000020 = 0xa05f;
            if (system_reset)
            {
                /* Tell the Realview EB to do a system reset */
                *(volatile unsigned *)0x10000040 = 6;
                /* goto reset vector! */
            }
            else
            {
                if (!last_cpu)
                {
                    /* Tell the Realview EB to put this CPU into reset */
                    *(volatile unsigned *)0x10000074 &= ~(1 << (6 + cpu_index));
                    /* goto reset vector! (when another CPU takes us out of reset) */
                }
            }
#endif
            /*
             * If we get here, either we are the last CPU, or the EB resets 
             * aren't present (e.g. Emulator). So, fake a reset: Turn off MMU, 
             * corrupt registers, wait for a while, jump to warm reset entry point
             */
            write_sctlr(read_sctlr() & ~0x10001807); /* clear TRE, I Z C M */
            dsb();
            for (i=0; i<10000; ++i)
            {
                __nop();
            }
            reset(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
#endif
#endif

 /*           auto_clk_gating_setup(	2,						    // select 100uS timebase
                                MODE_IRQ_ONLY_WAKE, 	    // Set interrupt wakeup only
                                0,						    // don't clear the FIQ global mask
                                0,						    // don't clear the IRQ global mask
                                1,						    // 1us start delay
                                1,						    // 1uS gate delay
                                1,						    // Set the delay wakeup time (1mS)
                                1); 					    // 1uS enable delay
*/
            asm volatile("cpsid	if");
            dsb();    
            wfi(); /* This signals the power controller to cut the power */
            /* Next stop, reset vector! */
        }
    }
    else
    {
        /* Power down failed for some reason, return to the OS */
        appf_platform_restore_context(cluster, cpu);
        cpu->power_state = 0;
        cluster->power_state = 0;
        ++cluster->active_cpus;
        release_spinlock(cpu_index, cluster->context->lock);
    }
    return rc;
}

/**
 * This function is called when the OS makes a firmware call with the 
 * function code APPF_POWER_UP_CPUS
 *
 * It brings powered down CPUs/clusters back to running state.
 */
static int power_up_cpus(unsigned cluster_index, unsigned cpus)
{
    struct appf_cpu *cpu;
    struct appf_cluster *cluster;
    unsigned cpu_index, this_cpu_index;
    int ret = APPF_OK;

    if (cluster_index >= main_table.num_clusters)
    {
        return APPF_BAD_CLUSTER;
    }
    
    cluster = main_table.cluster_table + cluster_index;

    if (cpus >= (1<<cluster->num_cpus))
    {
        return APPF_BAD_CPU;
    }
    
    /* TODO: add cluster-waking code for multi-cluster systems */
    /* TODO: locks will have to be expanded once extra-cluster CPUs can contend for them */

    this_cpu_index = appf_platform_get_cpu_index();
    get_spinlock(this_cpu_index, cluster->context->lock);
    
    for (cpu_index=0; cpu_index < cluster->num_cpus; ++cpu_index)
    {
        cpu = cluster->cpu_table + cpu_index;
        if (cpu->power_state == 0)
        {
            continue;
        }
        ret = appf_platform_power_up_cpu(cpu, cluster);
        if (ret)
        {
            break;
        }
    }

    release_spinlock(this_cpu_index, cluster->context->lock);
    return ret;
}


