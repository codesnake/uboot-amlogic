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
//#include <ao_reg.h>
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

#define P_AO_RTC_ADDR0				0xc8100740
#define P_AO_RTC_ADDR1				0xc8100744
#define P_AO_RTC_ADDR2				0xc8100748
#define P_AO_RTC_ADDR3				0xc810074c
#define P_AO_RTC_ADDR4				0xc8100750


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

#define SECBUS2_REG_ADDR(reg)       (0xda004000+((reg)<<2))

#define AO_SECURE_REG0 0x00 	///../ucode/secure_apb.h:19
#define P_AO_SECURE_REG0 		SECBUS2_REG_ADDR(AO_SECURE_REG0)

#define IO_AOBUS_BASE       0xc8100000  ///1M

#define AOBUS_REG_OFFSET(reg)   ((reg) )
#define AOBUS_REG_ADDR(reg)	    (IO_AOBUS_BASE + AOBUS_REG_OFFSET(reg))

#define AO_CPU_CNTL ((0x00 << 10) | (0x0E << 2)) 	///../ucode/c_always_on_pointer.h:33
#define P_AO_CPU_CNTL 		AOBUS_REG_ADDR(AO_CPU_CNTL)

#define P_DDR0_CLK_CTRL		0xc8000800
#define P_DDR0_PUB_DTAR0	0xc80010b4
#define P_DMC_DDR_CTRL		0xc8006040


#define CONFIG_M8_DDR1_ONLY  (1)

#define M8BABY_GET_DT_ADDR(dtar, dmc) \
	((((dtar >> 28) & 0x7) & ((((dmc >> 5) & 0x3)&1)?3:1)) << (((((dmc >> 5) & 0x3)&2) ? 6 : ((dmc & 0x3) + 8))+(2-((dmc >> 7) & 0x1)))) | \
	((((((dtar >> 28) & 0x7) >> ((((dmc >> 5) & 0x3) & 1)+1))) & ((((dmc >> 5) & 0x3)&1) ? 1 : 3)) << ((((dmc >> 2) & 0x3) ? (((dmc >> 2) & 0x3)+12) : (16))+((dmc & 0x3) + 8)+(3-((dmc >> 7) & 0x1))+(((dmc >> 5) & 0x3)&1))) | \
	((((dtar) & 0xfff) & ((1<< (((((dmc >> 5) & 0x3)) & 2) ? 6 : (((dmc & 0x3) + 8))))-1)) << (2-((dmc >> 7) & 0x1))) | \
	(((((dmc >> 5) & 0x3)) & 2) ? (((((dtar) & 0xfff) >> 6) & ((1<<((((dmc & 0x3) + 8))-6))-1)) << (((((dmc >> 5) & 0x3)) & 1)+(9-((dmc >> 7) & 0x1)))):(0)) | \
	(((dtar >> 12) & 0xffff) << (((dmc & 0x3) + 8)+((((dmc >> 5) & 0x3)&1)+(3-((dmc >> 7) & 0x1)))))


void run_arc_program()
{
	char cmd;
	int i;
	unsigned vaddr1,vaddr2,v;
	unsigned address0;
	unsigned* pbuffer;
	unsigned* pbuffer1;
	vaddr1 = 0xd9010000; //store ddr trainning original data
	vaddr2 = 0xd9000000;

	dbg_prints("run_arc_program ...==\n");
	
   	writel(0,P_AO_REMAP_REG0);
   	pwr_delay(10);
   	pbuffer = (unsigned*)vaddr2;
	for(i = 0; i < sizeof(arc_code)/4; i++){
			*pbuffer = arc_code[i];
			pbuffer++;
	}
    //writel((0x1<<4) | ((vaddr2>>14)&0xf),P_AO_REMAP_REG1);
    v = ((vaddr2 & 0xFFFFF)>>12);
    writel(v<<8 | readl(P_AO_SECURE_REG0),P_AO_SECURE_REG0); //TEST_N : 1->output mode; 0->input mode
    *(volatile unsigned *)(vaddr2 +0x20) = (unsigned)(&platform_reset_handler);
	dbg_print("reset_addr11=",*(volatile unsigned *)(vaddr2 +0x20));

	/*********Restore ddr training target address and its data***********
	**********First of vaddr1 two words is restore target address********
	**********From 3rd of vaddr1 address, store two target trainning 
	**********original data. ********************************************
	********************************************************************/

	writel(readl(P_DDR0_CLK_CTRL)|(1), P_DDR0_CLK_CTRL);
	address0 = M8BABY_GET_DT_ADDR(readl(P_DDR0_PUB_DTAR0), readl(P_DMC_DDR_CTRL));
	dbg_print("training addr=0x",address0);
	//disable clock
	writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);  

	pbuffer = (unsigned*)vaddr1;
	*pbuffer = address0;
	pbuffer ++;

	pbuffer1 = (unsigned*)address0;

	for(i = 0; i < 32; i++){
		*pbuffer = *pbuffer1;
		pbuffer++;
		pbuffer1++;
	}

	clean_dcache_v7_l1();

	//**********************//

//	writel(readl(P_AO_RTC_ADDR3)|(0x1<<29),P_AO_RTC_ADDR3);// Enable GPO filter if running at 32khz
	writel(readl(P_AO_RTC_ADDR0)|(0xf<<12),P_AO_RTC_ADDR0);//set int edge of GPO
	writel(readl(P_AO_RTC_ADDR1)|(0xf<<12),P_AO_RTC_ADDR1);//clear int
	//**********************//
    //switch to ARC jtag
    //set pinmux
/*    writel(readl(0xc8100014)|(1<<14),0xc8100014);
 		//ARC JTAG enable
		writel(0x80051001,0xda004004);		
		//ARC bug fix disable
		writel((readl(0xc8100040)|1<<24),0xc8100040);
   	//Watchdog disable
		writel(0,0xc1109900);
*/
  
    
    cmd = 0;
    writel((unsigned)cmd,P_AO_RTI_STATUS_REG0);

//	writel(0x200,P_AO_CPU_CNTL);//halt first
	writel(readl(P_RESET2_REGISTER)|(1<<13),P_RESET2_REGISTER);//reset AO_CPU
    

  //enable arc
    writel(0x0c900101,P_AO_CPU_CNTL);//remap is right?
    
//    pwr_delay(1);
   	//enable arc
//   	writel(1,P_AUD_ARC_CTL);
//   	pwr_delay(2);
//   	writel(0,P_AUD_ARC_CTL);
		//for(i = 0; i < 2; i++)
//	  pwr_delay(2);	
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
//    clean_mva_dcache_v7_l1((void *)&(cluster->active_cpus));
//    clean_mva_dcache_v7_l1((void *)&(cluster->power_state));
 //   if (cluster->l2_address && is_enabled_pl310(cluster->l2_address))
//    {
//        dsb();
//        clean_range_pl310((void *)&(cluster->active_cpus), 4, cluster->l2_address);
//        clean_range_pl310((void *)&(cluster->power_state), 4, cluster->l2_address);
//    }

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

