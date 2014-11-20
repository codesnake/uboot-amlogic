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
 * File:     translation.c
 * ----------------------------------------------------------------
 */
 
#include "appf_types.h"
#include "appf_internals.h"
#include "appf_platform_api.h"
#include "appf_helpers.h"

#define SECTION    (1<<1)
#define PAGE_TABLE (1<<0)
#define SMALL_PAGE (1<<1)
#define CACHE      (1<<3)
#define BUFFER     (1<<2)

/* Section bits */
#define S_NG       (1<<17)
#define S_S        (1<<16)
#define S_APX      (1<<15)
#define S_TEX0     (1<<12)
#define S_TEX1     (1<<13)
#define S_TEX2     (1<<14)
#define S_AP0      (1<<10)
#define S_AP1      (1<<11)
#define S_PARITY   (1<<9)
#define S_XN       (1<<4)
/* TODO: Add secure stuff */
#define S_NS       (1<<19)

/* Small Page bits */
#define SP_NG      (1<<11)
#define SP_S       (1<<10)
#define SP_AP2     (1<<9)
#define SP_TEX2    (1<<8)
#define SP_TEX1    (1<<7)
#define SP_TEX0    (1<<6)
#define SP_AP1     (1<<5)
#define SP_AP0     (1<<4)
#define SP_XN      (1<<0)

#define PAGE_SIZE     4096
#define PAGE_SHIFT    12
#define PAGE_MASK     0xfffff000
#define SECTION_MASK  0xfff00000
#define SECTION_SHIFT 20
#define TRE_MASK      (1<<28)


unsigned int appf_device_memory_flat_mapped = 0;

/*
 * find_mapping: Searches the PRRR and NMRR and returns memory attribute bits:
 *   bit 2: TEX0
 *   bit 1: C
 *   bit 0: B
 */

#define M_TEX0 (1<<2)
#define M_C    (1<<1)
#define M_B    (1<<0)
#define REMAPPING_DEVICE 1
#define REMAPPING_NORMAL 2
#define CACHE_WBWA 0x1

#define l2x0_way_mask ((1<<8)-1) //8 ways

extern void dbg_wait(void);

#define L2X0_BASE 0xC4200000
#define L2X0_CACHE_SYNC                (L2X0_BASE+0x730)
#define L2X0_CLEAN_INV_WAY             (L2X0_BASE+0x7FC)
#define L2X0_CTRL                      (L2X0_BASE+0x100)
#define L2X0_AUX_CTRL                  (L2X0_BASE+0x104)
#define L2X0_INV_WAY                   (L2X0_BASE+0x77C)
#define L2X0_CLEAN_WAY                 (L2X0_BASE+0x7BC)
#define L2X0_CLEAN_INV_LINE_PA         (L2X0_BASE+0x7F0)
#define L2X0_INV_LINE_PA               (L2X0_BASE+0x770)
#define L2X0_CLEAN_LINE_PA             (L2X0_BASE+0x780)


#define L2X0_BASE_rel 0xFE000000//0xF4200000

#define L2X0_CACHE_SYNC_rel            (L2X0_BASE_rel+0x730)
#define L2X0_CLEAN_WAY_rel             (L2X0_BASE_rel+0x7BC)
#define L2X0_CTRL_ref                  (L2X0_BASE_rel+0x100)
#define L2X0_INV_WAY_ref               (L2X0_BASE_rel+0x77c)
#define L2X0_CLEAN_INV_WAY_ref         (L2X0_BASE_rel+0x7FC)

#define CACHE_LINE_SIZE 32
/*void _clean_dcache_addr(unsigned long addr);
void dcache_clean_range(unsigned start,unsigned size)
{
    unsigned st,end,i;
    st=start&(~(CACHE_LINE_SIZE-1));
    end=start+size;
    for(i=st;i<end;i+=CACHE_LINE_SIZE)
    {
      //  dcache_clean_line(i);
        _clean_dcache_addr(i);
    }
}

void _clean_invd_dcache(void);
*/

//function declaration, in order to remove warnings.
void l2x0_disable();
void l2x0_disable_ref();

static void cache_wait(unsigned reg, unsigned long mask)
{
	/* wait for the operation to complete */
	while (readl(reg) & mask)
		;
}
void l2x0_clean_line(unsigned long addr)
{
	cache_wait(L2X0_CLEAN_LINE_PA, 1);
	writel(addr,  L2X0_CLEAN_LINE_PA);
}
void l2x0_flush_line(unsigned long addr)
{
	cache_wait( L2X0_CLEAN_INV_LINE_PA, 1);
	writel(addr,L2X0_CLEAN_INV_LINE_PA);
}
void cache_sync(void)
{
	writel(0, L2X0_CACHE_SYNC);
	cache_wait(L2X0_CACHE_SYNC, 1);
}
void cache_sync_ref(void)
{
	writel(0, L2X0_CACHE_SYNC_rel);
	cache_wait(L2X0_CACHE_SYNC_rel, 1);
}
void cache_sync_rel(void)
{
	writel(0, L2X0_CACHE_SYNC_rel);
	cache_wait(L2X0_CACHE_SYNC_rel, 1);
}
int l2x0_status()
{
    return readl( L2X0_CTRL) & 1;
}
void pwr_wait(int n)
{
		int i;
		while(n > 0){
			for(i = 0; i < 10000; i++){
				asm volatile("mov r0,r0");
			}
			n--;
		}
}
void l2x0_clean_all_rel ()
{
   
	/* invalidate all ways */
	writel(l2x0_way_mask, L2X0_CLEAN_WAY_rel);
	asm("dsb");
	asm("isb");
	pwr_wait(100);
	cache_wait(L2X0_CLEAN_WAY_rel, l2x0_way_mask);
	cache_sync_rel();
}

void l2x0_clean_all ()
{
   
	/* invalidate all ways */
	writel(l2x0_way_mask, L2X0_CLEAN_WAY);
	asm("dsb");
	asm("isb");
	pwr_wait(100);
	cache_wait(L2X0_CLEAN_WAY, l2x0_way_mask);
	cache_sync();
}

void l2x0_clean_inv_all ()
{
   if(l2x0_status())
   	{
		/* invalidate all ways */
		writel(l2x0_way_mask, L2X0_CLEAN_INV_WAY);
		cache_wait(L2X0_CLEAN_INV_WAY, l2x0_way_mask);
		cache_sync();
	}
}

void l2x0_disable_flush()
{
	if(l2x0_status())
	{
		l2x0_disable();
		l2x0_clean_all();
		l2x0_clean_inv_all();		
	}
}

void l2_cache_disable_ref(void)
{
	l2x0_disable_ref();
}

void l2x0_inv_line(unsigned long addr)
{
	cache_wait(L2X0_INV_LINE_PA, 1);
	writel(addr,  L2X0_INV_LINE_PA);
}

void l2x0_inv_all(void)
{
  /* invalidate all ways */
	writel(l2x0_way_mask, L2X0_INV_WAY);
	asm("dsb");
	asm("isb");
	pwr_wait(100);

	cache_wait(L2X0_INV_WAY, l2x0_way_mask);
	cache_sync();
}

void l2x0_inv_all_ref(void)
{
  /* invalidate all ways */
	writel(l2x0_way_mask, L2X0_INV_WAY_ref);
	asm("dsb");
	asm("isb");
	pwr_wait(100);

	cache_wait(L2X0_INV_WAY_ref, l2x0_way_mask);
	cache_sync_ref();
}
#define min(a,b) (a<b?a:b)
void l2x0_invalid_range(unsigned long start, unsigned long end)
{
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		l2x0_flush_line(start);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		l2x0_flush_line(end);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			
		}
	}
	cache_wait( L2X0_INV_LINE_PA, 1);
	cache_sync();
	   
}
void l2x0_flush_range(unsigned long start, unsigned long end)
{
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
	}
	cache_wait(L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
}

void l2x0_clean_range(unsigned long start, unsigned long end)
{
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + end - start;

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}
	}
	cache_wait(L2X0_CLEAN_LINE_PA, 1);
	cache_sync();
	
}
void l2x0_enable()
{
	unsigned aux;
	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl( L2X0_CTRL) & 1)) {

		/* l2x0 controller is disabled */

		aux = readl(L2X0_AUX_CTRL);
		aux &= 0xff800fff;
		aux |= 0x00020000;
		writel(aux,L2X0_AUX_CTRL);

		l2x0_inv_all();

		/* enable L2X0 */
		writel(1,  L2X0_CTRL);
	}

}
void l2x0_disable()
{
    writel(0,  L2X0_CTRL);
}

void l2x0_enable_ref()
{
	asm ("dsb");
	asm ("isb");
	writel(1,  L2X0_CTRL_ref);
	asm ("dsb");
	asm ("isb");
}
void l2x0_disable_ref()
{
    writel(0,  L2X0_CTRL_ref);
}


void l2x0_flush_all_ref(void)
{
	writel(l2x0_way_mask/*8 ways*/, L2X0_CLEAN_INV_WAY_ref);
	cache_wait(L2X0_CLEAN_INV_WAY_ref, l2x0_way_mask);
	cache_sync_ref();
}

void l2x0_flush_all(void)
{
	writel(l2x0_way_mask/*8 ways*/, L2X0_CLEAN_INV_WAY);
	cache_wait(L2X0_CLEAN_INV_WAY, l2x0_way_mask);
	cache_sync();
}

#if 1
int appf_setup_translation_tables(void)
{
	  unsigned firmware_start_pa, firmware_start_va,attr;
	  unsigned tab1_pa,pa;
	  int i; 
	  firmware_start_va = ((unsigned)&main_table) & PAGE_MASK;
    firmware_start_pa = reloc_addr(firmware_start_va);
    tab1_pa = reloc_addr((unsigned)appf_translation_table1);  

//    tex_remap = read_sctlr() & TRE_MASK;/* TRE: 0:remap disable, 1: remap enable*/
    
    for (i = 0; i < 4096; ++i)
    {
    	if(i == 0x04f)
			attr = 0xc0e;
    	else if(i <=0x3FF)
    		attr = 0x10c02;
    	else if( i < 0x40F)
    		attr = 0xc0e;
    	else if( i == 0x490)
    		attr = 0xc0e;
    	else if(i == 0x498)
    		attr = 0xc0e;
    	else if(i >= 0x800 && i <= 0x9FF)
    		attr = 0xc0e;
    	else if(i >= 0xa00 && i <= 0xbff)
    		attr = 0x10c02;
    	else if(i == 0xc11)
    		attr = 0x10412;
    	else if(i == 0xc12)
    		attr = 0x10412;
    	else if(i == 0xc42)
    		attr = 0x412;
    	else if(i==0xc43)
    		attr = 0x2412;
    	else if(i>=0xc80 && i <= 0xCFF)
    		attr = 0x412;
    	else if(i>=0xe00 && i <= 0xfff)
    		attr = 0xc0e;
    	else 
    		attr = 0;
    	if(i >= 0xC00 && i <= 0xC06)
    	{
      	((unsigned*)tab1_pa) [i] = 0xc0e|((0x800 + (i - 0xc00))<<20);
    	}		
    	else if(i >= 0xD00 & i < 0xDFF)
    	{
      	((unsigned*)tab1_pa) [i] = 0xc0e|((0x900 + (i - 0xD00))<<20);
    	}
    	else if(i == 0xc4F)
    	{
      	((unsigned*)tab1_pa) [i] = 0x402|(0x04F<<20);//0xc0e
    	}
        else if(i == 0xc4D)
        {
        ((unsigned*)tab1_pa) [i] = 0x402|(0x04F<<20);//0xc0e
        }
    	else if(i == 0xCFF)
    	{
      	((unsigned*)tab1_pa) [i] = 0xc0e|(0xFF<<20);
    	}
   		else if(i == 0xF11)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC11<<20);
    	else if(i == 0xF13)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC13<<20);
     	else if(i == 0xF22)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC42<<20);
    	else if(i == 0xF23)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC43<<20);
     	else if(i == 0xF30)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC80<<20);
    	else if(i == 0xF31)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC81<<20);
    	else if(i == 0xF32)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xC90<<20);
    	else if(i == 0xF40)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xCC0<<20);
    	else if(i == 0xF80)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xd00<<20);
    	else if(i == 0xF90)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xd90<<20);
    	else if(i == 0xFA0)
    	   		((unsigned*)tab1_pa) [i] = 0x412|(0xda0<<20);
    	else
      	((unsigned*)tab1_pa) [i] = attr|(i<<20);
    }

//   ((unsigned*)tab1_pa)[firmware_start_va >> SECTION_SHIFT]  = reloc_addr((unsigned)appf_translation_table2a) | PAGE_TABLE;
//   ((unsigned*)tab1_pa)[firmware_start_pa >> SECTION_SHIFT] = reloc_addr((unsigned)appf_translation_table2b) | PAGE_TABLE;

   pa = reloc_addr((unsigned)&appf_ttbr0);
    *((volatile unsigned*)pa) = reloc_addr((unsigned)appf_translation_table1);
     
    __V(appf_ttbcr) = 0;

    clean_dcache_v7_l1();
	
    return APPF_OK;
}
#endif

