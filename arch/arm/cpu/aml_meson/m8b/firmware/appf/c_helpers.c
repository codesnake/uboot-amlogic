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
 * File:     c_helpers.c
 * ----------------------------------------------------------------
 */
 
 
/**
 * Lamport's Bakery algorithm for spinlock handling
 *
 * Note that the algorithm requires the stack and the bakery struct
 * to be in Strongly-Ordered memory.
 */

#include "appf_types.h"
#include "appf_internals.h"
#include "appf_helpers.h"

/**
 * Initialize a bakery - only required if the bakery_t is
 * on the stack or heap, as static data is zeroed anyway.
 */
void initialize_spinlock(bakery_t *bakery)
{
    appf_memset(bakery, 0, sizeof(bakery_t));
}

/**
 * Claim a bakery lock. Function does not return until
 * lock has been obtained.
 */
void get_spinlock(unsigned cpuid, bakery_t *bakery)
{
    unsigned i, max=0, my_full_number, his_full_number;
	
    /* Get a ticket */
    bakery->entering[cpuid] = TRUE;
    for (i=0; i<MAX_CPUS; ++i)
    { 		
        if (bakery->number[i] > max)
        {
            max = bakery->number[i];
        }
    }

    ++max;
    bakery->number[cpuid] = max;
    bakery->entering[cpuid] = FALSE;
    /* Wait for our turn */
    my_full_number = (max << 8) + cpuid;
    for (i=0; i<MAX_CPUS; ++i)
    {
        while(bakery->entering[i]);  /* Wait */
        do
        {
            his_full_number = bakery->number[i];
            if (his_full_number)
            {
                his_full_number = (his_full_number << 8) + i;
            }
        }
        while(his_full_number && (his_full_number < my_full_number));
    }
    dmb();
}

/**
 * Release a bakery lock.
 */
void release_spinlock(unsigned cpuid, bakery_t *bakery)
{
    dmb();
    bakery->number[cpuid] = 0;
}
