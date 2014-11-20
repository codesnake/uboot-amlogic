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
 * File:     scu.c
 * ----------------------------------------------------------------
 */
 
#include "appf_types.h"
#include "appf_internals.h"
#include "appf_helpers.h"

typedef struct
{
    /* 0x00 */  volatile unsigned int control;
    /* 0x04 */  const unsigned int configuration;
    /* 0x08 */  union
                {
                    volatile unsigned int w;
                    volatile unsigned char b[4];
                } power_status;
    /* 0x0c */  volatile unsigned int invalidate_all;
                char padding1[48];
    /* 0x40 */  volatile unsigned int filtering_start;
    /* 0x44 */  volatile unsigned int filtering_end;
                char padding2[8];
    /* 0x50 */  volatile unsigned int access_control;
    /* 0x54 */  volatile unsigned int ns_access_control;
} a9_scu_registers;

/* 
 * TODO: we need to use the power status register, not save it!
 */

void save_a9_scu(appf_u32 *pointer, unsigned scu_address)
{
    a9_scu_registers *scu = (a9_scu_registers *)scu_address;
    
    pointer[0] = scu->control;
    pointer[1] = scu->power_status.w;
    pointer[2] = scu->filtering_start;
    pointer[3] = scu->filtering_end;
    pointer[4] = scu->access_control;
    pointer[5] = scu->ns_access_control;
}

void restore_a9_scu(appf_u32 *pointer, unsigned scu_address)
{
    a9_scu_registers *scu = (a9_scu_registers *)scu_address;
    
    scu->invalidate_all = 0xffff;
    scu->filtering_start = pointer[2];
    scu->filtering_end = pointer[3];
    scu->access_control = pointer[4];
    scu->ns_access_control = pointer[5];
    scu->power_status.w = pointer[1];
    scu->control = pointer[0];
}

void set_status_a9_scu(unsigned cpu_index, unsigned status, unsigned scu_address)
{
    a9_scu_registers *scu = (a9_scu_registers *)scu_address;
    unsigned power_status;

    switch(status)
    {
        case STATUS_STANDBY:
        case STATUS_DORMANT:
            power_status = 2;
            break;
        case STATUS_SHUTDOWN:
            power_status = 3;
            break;
        default:
            power_status = 0;
    }
        
    scu->power_status.b[cpu_index] = power_status;
    dsb();
}

int num_cpus_from_a9_scu(unsigned scu_address)
{
    a9_scu_registers *scu = (a9_scu_registers *)scu_address;

    return ((scu->configuration) & 0x3) + 1;
}
