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
 * File:     pl310.c
 * ----------------------------------------------------------------
 */
 
#include "appf_types.h"
#include "appf_internals.h"
#include "appf_helpers.h"

#define C_BIT 0x01

struct lockdown_regs
{
    volatile unsigned int d, i;
};

typedef struct
{
    /* 0x000 */ const unsigned cache_id;
    /* 0x004 */ const unsigned cache_type;
                char padding1[0x0F8];
    /* 0x100 */ volatile unsigned control;
    /* 0x104 */ volatile unsigned aux_control;
    /* 0x108 */ volatile unsigned tag_ram_control;
    /* 0x10C */ volatile unsigned data_ram_control;
                char padding2[0x0F0];
    /* 0x200 */ volatile unsigned ev_counter_ctrl;
    /* 0x204 */ volatile unsigned ev_counter1_cfg;
    /* 0x208 */ volatile unsigned ev_counter0_cfg;
    /* 0x20C */ volatile unsigned ev_counter1;
    /* 0x210 */ volatile unsigned ev_counter0;
    /* 0x214 */ volatile unsigned int_mask;
    /* 0x218 */ const volatile unsigned int_mask_status;
    /* 0x21C */ const volatile unsigned int_raw_status;
    /* 0x220 */ volatile unsigned int_clear;
                char padding3[0x50C];
    /* 0x730 */ volatile unsigned cache_sync;
                char padding4[0x03C];
    /* 0x770 */ volatile unsigned inv_pa;
                char padding5[0x008];
    /* 0x77C */ volatile unsigned inv_way;
                char padding6[0x030];
    /* 0x7B0 */ volatile unsigned clean_pa;
                char padding7[0x004];
    /* 0x7B8 */ volatile unsigned clean_index;
    /* 0x7BC */ volatile unsigned clean_way;
                char padding8[0x030];
    /* 0x7F0 */ volatile unsigned clean_inv_pa;
                char padding9[0x004];
    /* 0x7F8 */ volatile unsigned clean_inv_index;
    /* 0x7FC */ volatile unsigned clean_inv_way;
                char paddinga[0x100];
    /* 0x900 */ volatile struct lockdown_regs lockdown[8];
                char paddingb[0x010];
    /* 0x950 */ volatile unsigned lock_line_en;
    /* 0x954 */ volatile unsigned unlock_way;
                char paddingc[0x2A8];
    /* 0xC00 */ volatile unsigned addr_filtering_start;
    /* 0xC04 */ volatile unsigned addr_filtering_end;
                char paddingd[0x338];
    /* 0xF40 */ volatile unsigned debug_ctrl;
                char paddinge[0x01C];
    /* 0xF60 */ volatile unsigned prefetch_ctrl;
                char paddingf[0x01C];
    /* 0xF80 */ volatile unsigned power_ctrl;
} pl310_registers;


typedef struct
{
    unsigned int aux_control;
    unsigned int tag_ram_control;
    unsigned int data_ram_control;
    unsigned int ev_counter_ctrl;
    unsigned int ev_counter1_cfg;
    unsigned int ev_counter0_cfg;
    unsigned int ev_counter1;
    unsigned int ev_counter0;
    unsigned int int_mask;
    unsigned int lock_line_en;
    struct lockdown_regs lockdown[8];
    unsigned int unlock_way;
    unsigned int addr_filtering_start;
    unsigned int addr_filtering_end;
    unsigned int debug_ctrl;
    unsigned int prefetch_ctrl;
    unsigned int power_ctrl;
} pl310_context;

/* TODO: should be determined from cache? */
static unsigned const cache_line_size = 32;

void clean_inv_range_pl310(void *start, unsigned size, unsigned pl310_address)
{
    unsigned addr;
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    
    /* Align the start address to the start of a cache line */
    addr = (unsigned)start & ~(cache_line_size - 1);

    /* Wait for any background operations to finish */
    while(pl310->clean_inv_pa & C_BIT);
    
    while(addr <= size + (unsigned)start)
    {
        pl310->clean_inv_pa = addr; 
        addr += cache_line_size;
        /* For this to work on L220 we would have to poll the C bit now */
    }
    dmb();
}

void clean_range_pl310(void *start, unsigned size, unsigned pl310_address)
{
    unsigned addr;
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    
    /* Align the start address to the start of a cache line */
    addr = (unsigned)start & ~(cache_line_size - 1);
    
    /* Wait for any background operations to finish */
    while(pl310->clean_pa & C_BIT);
    
    while(addr <= size + (unsigned)start)
    {
        pl310->clean_pa = addr; 
        addr += cache_line_size;
        /* For this to work on L220 we would have to poll the C bit now */
    }
    dmb();
}

void inv_range_pl310(void *start, unsigned size, unsigned pl310_address)
{
    unsigned addr;
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    
    /* Align the start address to the start of a cache line */
    addr = (unsigned)start & ~(cache_line_size - 1);
    
    /* Wait for any background operations to finish */
    while(pl310->inv_pa & C_BIT);
    
    while(addr <= size + (unsigned)start)
    {
        pl310->inv_pa = addr; 
        addr += cache_line_size;
        /* For this to work on L220 we would have to poll the C bit now */
    }
}

void clean_inv_pl310(unsigned pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    int i;
    
    pl310->clean_inv_way = 0xffff;
    while (pl310->clean_inv_way)
    {
        /* Spin */
        for (i=10; i>0; --i)
        {
            __nop();
        }
    }
}

void clean_pl310(unsigned pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    int i;
    
    pl310->clean_way = 0xffff;
    while (pl310->clean_way)
    {
        /* Spin */
        for (i=10; i>0; --i)
        {
            __nop();
        }
    }
}

static void inv_pl310(unsigned pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    int i;

    pl310->inv_way = 0xffff;
    while (pl310->inv_way & 0xffff)
    {
        /* Spin */
        for (i=10; i>0; --i)
        {
            __nop();
        }
    }

	pl310->cache_sync = 0;

	while (pl310->cache_sync & 1)
		;		
}

int is_enabled_pl310(unsigned pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;

    return (pl310->control & 1);
}    

void save_pl310(appf_u32 *pointer, unsigned pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    pl310_context *context = (pl310_context *)pointer;
    int i;

    /* TODO: are all these registers are present in earlier PL310 versions? */
    context->aux_control = pl310->aux_control;
    context->tag_ram_control = pl310->tag_ram_control;
    context->data_ram_control = pl310->data_ram_control;
    context->ev_counter_ctrl = pl310->ev_counter_ctrl;
    context->ev_counter1_cfg = pl310->ev_counter1_cfg;
    context->ev_counter0_cfg = pl310->ev_counter0_cfg;
    context->ev_counter1 = pl310->ev_counter1;
    context->ev_counter0 = pl310->ev_counter0;
    context->int_mask = pl310->int_mask;
    context->lock_line_en = pl310->lock_line_en;
    for (i=0; i<8; ++i)
    {
        context->lockdown[i].d = pl310->lockdown[i].d;
        context->lockdown[i].i = pl310->lockdown[i].i;
    }
    context->addr_filtering_start = pl310->addr_filtering_start;
    context->addr_filtering_end = pl310->addr_filtering_end;
    context->debug_ctrl = pl310->debug_ctrl;
    context->prefetch_ctrl = pl310->prefetch_ctrl;
    context->power_ctrl = pl310->power_ctrl;
}

void restore_pl310(appf_u32 *pointer, unsigned pl310_address, int dormant)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    pl310_context *context = (pl310_context *)pointer;
    int i;
    
    /* We may need to disable the PL310 if the boot code has turned it on */
    if (pl310->control)
    {
        /* Wait for the cache to be idle, then disable */
        pl310->cache_sync = 0;
        dsb();
        pl310->control = 0;
    }
    
    /* TODO: are all these registers present in earlier PL310 versions? */
    pl310->aux_control = context->aux_control;
    pl310->tag_ram_control = context->tag_ram_control;
    pl310->data_ram_control = context->data_ram_control;
    pl310->ev_counter_ctrl = context->ev_counter_ctrl;
    pl310->ev_counter1_cfg = context->ev_counter1_cfg;
    pl310->ev_counter0_cfg = context->ev_counter0_cfg;
    pl310->ev_counter1 = context->ev_counter1;
    pl310->ev_counter0 = context->ev_counter0;
    pl310->int_mask = context->int_mask;
    pl310->lock_line_en = context->lock_line_en;
    for (i=0; i<8; ++i)
    {
        pl310->lockdown[i].d = context->lockdown[i].d;
        pl310->lockdown[i].i = context->lockdown[i].i;
    }
    pl310->addr_filtering_start = context->addr_filtering_start;
    pl310->addr_filtering_end = context->addr_filtering_end;
    pl310->debug_ctrl = context->debug_ctrl;
    pl310->prefetch_ctrl = context->prefetch_ctrl;
    pl310->power_ctrl = context->power_ctrl;
    dsb();
    
    /*
     * If the RAMs were powered off, we need to invalidate the cache
     */
//    if (!dormant)
//    {
//        inv_pl310(pl310_address);
//       l2x0_inv_all(); //invalid be moved to entry.s
//    }
    
//    pl310->control = 1; // Not enable here. Move to entry.s to enable it after OS mmu set.
    dsb();
}


void set_status_pl310(unsigned status, unsigned pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;

    /* Wait for the cache to be idle */
    pl310->cache_sync = 0;
    dsb();

    if (status == STATUS_STANDBY)
    {
        pl310->power_ctrl |= 1;
    }
    else
    {
        pl310->power_ctrl &= ~1;
    }
}
