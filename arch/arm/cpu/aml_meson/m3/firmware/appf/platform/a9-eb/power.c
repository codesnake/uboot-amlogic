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
 * File:     aem/power.c
 * ----------------------------------------------------------------
 */
 
#include "appf_types.h"
#include "appf_platform_api.h"
#include "appf_internals.h"
#include "appf_helpers.h"
#include <arc_code.dat>
/**
 * This function powers up a CPU
 *
 * It is entered with cluster->lock held.
 */
 
int appf_platform_power_up_cpu(struct appf_cpu *cpu,
                               struct appf_cluster *cluster)
{
    if (cpu->power_state == 1)
    {
        cpu->power_state = 0;
        dsb();
//        send_interrupt_gic(cluster->ic_address);  /* TODO: will this work between clusters? */
        /* TODO: cluster->power_state = rstate; */
    }
    else
    {
        /* Wake another CPU from deeper sleep */
        
#ifdef USE_REALVIEW_EB_RESETS
        int cpu_index = cpu - cluster->cpu_table;
        cpu->power_state = 0;
        dsb();
        /* Unlock system registers */
        *(volatile unsigned *)0x10000020 = 0xa05f;
        /* Cause the model to take the CPU out of reset */
        *(volatile unsigned *)0x10000074 |= 1 << (6 + cpu_index);
#endif
    }
    /* TODO: Add code to handle a cpu in another cluster */
    
    return APPF_OK;
}

/**
 * This function tells the PCU to power down the executing CPU, and possibly the whole cluster.
 *
 * It is entered with cluster->lock held.
 */

void pwr_delay(int n)
{
		int i;
		while(n > 0){
			for(i = 0; i < 10000; i++){
				asm volatile("mov r0,r0");
			}
			n--;
		}
}
#define AUD_ARC_CTL           0x2659
#define P_AO_RTI_STATUS_REG0         (0xC8100000 | (0x00 << 10) | (0x00 << 2))
#define P_AO_RTI_STATUS_REG1         (0xC8100000 | (0x00 << 10) | (0x01 << 2))
#define P_AO_RTI_STATUS_REG2         (0xC8100000 | (0x00 << 10) | (0x02 << 2))
#define P_AO_REMAP_REG0              (0xC8100000 | (0x00 << 10) | (0x07 << 2))
#define P_AO_REMAP_REG1              (0xC8100000 | (0x00 << 10) | (0x08 << 2))


#define RESET0_REGISTER                            0x1101
#define RESET1_REGISTER                            0x1102
#define RESET2_REGISTER                            0x1103
#define AHB_ARBDEC_REG                             0x2643

#define IO_CBUS_BASE            0xC1100000

#define CBUS_REG_OFFSET(reg) ((reg) << 2)
#define CBUS_REG_ADDR(reg)	 (IO_CBUS_BASE + CBUS_REG_OFFSET(reg))

#define P_RESET2_REGISTER   0xC110440c //CBUS_REG_ADDR(RESET2_REGISTER)
#define RESET_ARC625        (1<<13)
#define P_AUD_ARC_CTL       0xC1109964 //CBUS_REG_ADDR(AUD_ARC_CTL)
#define P_AHB_ARBDEC_REG    0xC110990c //CBUS_REG_ADDR(AHB_ARBDEC_REG)

void l2x0_clean_all(void); 
extern void platform_reset_handler(void);
void run_arc_program()
{
	 char cmd;
	  int i;
	  unsigned vaddr1,vaddr2,v;
	  unsigned* pbuffer;
		vaddr1 = 0x49000000;  
		vaddr2 = 0x4900c000;
		
    /** copy ARM code*/
    //change arm mapping
   // appf_memcpy((char*)vaddr2,(char*)vaddr1,16*1024);
    
 		//remap arm memory
   // writel((vaddr2>>14)&0xf,P_AO_REMAP_REG0);
  
    /** copy ARC code*/
    //copy code to 49000000 and remap to zero
   // memcpy((char*)vaddr2,(char*)addr,16*1024);
   	//load arc code into memory
   	writel(0,P_AO_REMAP_REG0);
   	pwr_delay(10);
   	pbuffer = (unsigned*)vaddr2;
		for(i = 0; i < sizeof(arc_code)/4; i++){
				*pbuffer = arc_code[i];
				pbuffer++;
		}
    writel((0x1<<4) | ((vaddr2>>14)&0xf),P_AO_REMAP_REG1);
     *(volatile unsigned *)(vaddr2 +0x20) = (unsigned)(&platform_reset_handler);
    l2x0_clean_all();
    
    //switch to ARC jtag
    writel(0x51001,0xc8100030);
        
    //reset arc
    writel(RESET_ARC625,P_RESET2_REGISTER);
    pwr_delay(1);
    
    cmd = 0;
    writel((unsigned)cmd,P_AO_RTI_STATUS_REG0);
   	//enable arc
   	writel(1,P_AUD_ARC_CTL);
   	pwr_delay(2);
   	writel(0,P_AUD_ARC_CTL);
		//for(i = 0; i < 2; i++)
	  pwr_delay(2);	
    cmd = 't';
	writel((unsigned)cmd,P_AO_RTI_STATUS_REG0); 
}
int appf_platform_enter_cstate(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster)
{
    /*
     * This is where the code to talk to the PCU goes. We could check the value
     * of cpu->power_state, but we don't, since we can only do run or shutdown.
     */
//    if (cluster->scu_address)
//    {
//        set_status_a9_scu(cpu_index, STATUS_SHUTDOWN, cluster->scu_address);
//   }

		//invoke arc program to reset arm
		run_arc_program();
    return APPF_OK;
}


/**
 * This function tells the PCU this CPU has finished powering up.
 *
 * It is entered with cluster->lock held.
 */
int appf_platform_leave_cstate(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster)
{
    /*
     * Housekeeping...
     */
    ++cluster->active_cpus;
    cluster->power_state = 0;
    cpu->power_state = 0;
    /*
     * We need to ensure that a CPU coming out of reset with caches off can see
     *  -  cluster->active_cpus
     *  -  cluster->power_state
     * so that it can tell whether it is the first CPU up. Note that mva==pa here.
     */
    dsb();
    clean_mva_dcache_v7_l1((void *)&(cluster->active_cpus));
    clean_mva_dcache_v7_l1((void *)&(cluster->power_state));
    if (cluster->l2_address && is_enabled_pl310(cluster->l2_address))
    {
        dsb();
        clean_range_pl310((void *)&(cluster->active_cpus), 4, cluster->l2_address);
        clean_range_pl310((void *)&(cluster->power_state), 4, cluster->l2_address);
    }

    /*
     * We could check the value in the SCU power status register as it is written at 
     * power up by the PCU. But we don't, since we know we're always powering up.
     */
    if (cluster->scu_address)
    {
//        set_status_a9_scu(cpu_index, STATUS_RUN, cluster->scu_address);
    }
    
    return APPF_OK;
}

/**
 * This function puts the executing CPU, and possibly the whole cluster, into STANDBY
 *
 * It is entered with cluster->lock held.
 */
int appf_platform_enter_cstate1(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster)
{
//    if (cluster->power_state == 1 && cluster->l2_address)
//    {
//        set_status_pl310(STATUS_STANDBY, cluster->l2_address);
//    }

    return APPF_OK;
}
    
/**
 * This function removes the executing CPU, and possibly the whole cluster, from STANDBY
 *
 * It is entered with cluster->lock held.
 */
int appf_platform_leave_cstate1(unsigned cpu_index, struct appf_cpu *cpu, struct appf_cluster *cluster)
{
//    if (cluster->l2_address)
//    {
//        set_status_pl310(STATUS_RUN, cluster->l2_address);
//    }

    return APPF_OK;
}

