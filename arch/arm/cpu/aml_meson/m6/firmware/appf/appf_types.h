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
 * File:     appf_types.h
 * ----------------------------------------------------------------
 */
 
#define APPF_ENTRY_POINT_SIZE (32 * 1024)

#define CPU_A9MPCORE 0xca9

#define APPF_INITIALIZE             0
#define APPF_POWER_DOWN_CPU         1
#define APPF_POWER_UP_CPUS          2

/*
 * Return codes
 */
#define APPF_EARLY_RETURN        1 
#define APPF_OK                  0
#define APPF_BAD_FUNCTION      (-1)
#define APPF_BAD_CSTATE        (-2)
#define APPF_BAD_RSTATE        (-3)
#define APPF_NO_MEMORY_MAPPING (-4)
#define APPF_BAD_CLUSTER       (-5)
#define APPF_BAD_CPU           (-6)
#define APPF_CPU_NOT_SHUTDOWN  (-7)

/*
 * Values for flags parameter
 * TODO: Most of these are internal-only - move to appf_internals.h
 */
#define APPF_SAVE_PMU          (1<<0)
#define APPF_SAVE_TIMERS       (1<<1)
#define APPF_SAVE_VFP          (1<<2)
#define APPF_SAVE_DEBUG        (1<<3)
#define APPF_SAVE_L2           (1<<4)

#define APPF_UBOOT_FLAG        (1<<31) //call from uboot

typedef unsigned int appf_u32;
typedef signed int appf_i32;


typedef appf_i32 (appf_entry_point_t)(appf_u32, appf_u32, appf_u32, appf_u32);

struct appf_main_table
{
    appf_u32 ram_start;
    appf_u32 ram_size;
    appf_u32 num_clusters;
    struct appf_cluster            *cluster_table;
    appf_entry_point_t *entry_point;
};

struct appf_cluster_context;
struct appf_cpu_context;

/*
 * A cluster is a container for CPUs, typically either a single CPU or a 
 * coherent cluster.
 * We assume the CPUs in the cluster can be switched off independently.
 */
struct appf_cluster
{
    appf_u32 cpu_type;                /* A9mpcore, 11mpcore, etc                  */
    appf_u32 cpu_version;             
    appf_i32 num_cpus;
    volatile appf_i32 active_cpus;    /* Initialized to number of cpus present    */
    appf_u32 scu_address;             /*  0 => no SCU                             */
    appf_u32 ic_address;              /*  0 => no Interrupt Controller            */
    appf_u32 l2_address;              /*  0 => no L2CC                            */
    struct appf_cluster_context *context;
    struct appf_cpu *cpu_table;
    volatile appf_i32 power_state;
};

struct appf_cpu
{
    appf_u32 ic_address;              /*  0 => no Interrupt Controller            */
    struct appf_cpu_context *context;
    volatile appf_u32 power_state;
};

#define writel(v,addr) (*((volatile unsigned*)addr) = v)
#define readl(addr) (*((volatile unsigned*)addr))

void f_serial_puts(const char *s);
void wait_uart_empty();
void serial_put_hex(unsigned int data,unsigned bitlen);

#define dbg_print(s,v)
#define dbg_prints(s)

//#define dbg_print(s,v) {f_serial_puts(s);serial_put_hex(v,32);f_serial_puts("\n");}
//#define dbg_prints(s)  {f_serial_puts(s);wait_uart_empty();}