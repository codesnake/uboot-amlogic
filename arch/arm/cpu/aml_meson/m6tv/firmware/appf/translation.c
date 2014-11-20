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
#include "virtul_serial.h"
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

#if 0
int find_remapping(unsigned prrr, unsigned nmrr, unsigned desired_type)
{
    int i;
    unsigned type, outer_cacheable, inner_cacheable;
    
    /*
     * Search through the PRRR for the desired memory type 
     */
    for (i = 0; i <= 7; ++i)
    {
        if (i == 6)
        {
            continue; /* Remapping 6 is IMPLEMENTATION DEFINED, ignore */
        }
        type = (prrr >> (2*i)) & 0x3; //prr type: normal:2, device:1, 
        if (type == desired_type)
        {
            /* for Normal memory: check inner/outer cache attributes of this remapping */
            if (desired_type == REMAPPING_NORMAL) 
            {
            		/* nmrr: two fields: 0:inner :outer*/
                inner_cacheable = (nmrr >> (2*i)) & 0x3;
                outer_cacheable = (nmrr >> (16 + 2*i)) & 0x3;
                if ((inner_cacheable == CACHE_WBWA) && (outer_cacheable == CACHE_WBWA))
                {
                    break;  /* Found a suitable remapping */
                }
            }
            else /* for other memory types, we don't care about cache attributes */
            {
                break;  /* Found a suitable remapping */
            }
        }
    }
    
    if (i<=7)
    {
        return i;
    }
    else
    {
        return APPF_NO_MEMORY_MAPPING;
    }
}

int find_shareable_bit(unsigned prrr, unsigned type, unsigned desired_shareable)
{
    unsigned s0, s1;
    
    if (type == REMAPPING_DEVICE)
    {
        s0 = (prrr >> 16) & 0x1;
        s1 = (prrr >> 17) & 0x1;
    }
    else /* Normal memory */
    {
        s0 = (prrr >> 18) & 0x1;
        s1 = (prrr >> 19) & 0x1;
    }

    /* Deal with special case: both values of S bit encode the same shareability */
    if (s0 == s1)
    {
        if (s0 != desired_shareable)
        {
            return APPF_NO_MEMORY_MAPPING;
        }
        else
        {
            return s0;  /* Doesn't matter which value of S bit is chosen! */
        }
    }
    
    /* Otherwise: one value of S means shareable, the other means non-shareable */
    return s0 ^ desired_shareable;
}
#endif

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

#define L2X0_BASE_rel 0xF2200000
#define L2X0_CACHE_SYNC_rel            (L2X0_BASE_rel+0x730)
#define L2X0_CLEAN_WAY_rel             (L2X0_BASE_rel+0x7BC)

#define CACHE_LINE_SIZE 32

void l2x0_clean_all_rel ();
void l2x0_clean_all ();

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
	dbg_prints("enter cache_sync\n");
	writel(0, L2X0_CACHE_SYNC);
	cache_wait(L2X0_CACHE_SYNC, 1);
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
void l2x0_clean()
{
	unsigned va, pa;
	update_offset();
	pa = reloc_addr((unsigned)&main_table);
    	va = (unsigned)&main_table;
	if( pa==va)
	{
		dbg_prints("uboot enter!\n");
		l2x0_clean_all();
	}
	else
	{
		v_prints("kernel enter!\n");
		l2x0_clean_all_rel();
	}
}
void l2x0_clean_all_rel ()
{
	/* invalidate all ways */
	writel(0xff, L2X0_CLEAN_WAY_rel);
	asm("dsb");
	asm("isb");
	pwr_wait(100);
//	cache_wait(L2X0_CLEAN_WAY, 0xff);
	cache_sync_rel();
}

void l2x0_clean_all ()
{
	if((readl(L2X0_CTRL))&1)//uboot l2 cache disable
	{
	/* invalidate all ways */
	writel(0xff, L2X0_CLEAN_WAY);
	asm("dsb");
	asm("isb");
	pwr_wait(100);
//	cache_wait(L2X0_CLEAN_WAY, 0xff);
	cache_sync();
	}
}

void l2x0_clean_inv_all ()
{
   if(l2x0_status())
   	{
		/* invalidate all ways */
		writel(0xff, L2X0_CLEAN_INV_WAY);
		cache_wait(L2X0_CLEAN_INV_WAY, 0xff);
		cache_sync();
	}
}
void l2x0_inv_line(unsigned long addr)
{
	cache_wait(L2X0_INV_LINE_PA, 1);
	writel(addr,  L2X0_INV_LINE_PA);
}

void l2x0_inv_all(void)
{
  /* invalidate all ways */
	writel(0xff, L2X0_INV_WAY);
	asm("dsb");
	asm("isb");
	pwr_wait(100);

//	cache_wait(L2X0_INV_WAY, 0xff);
	cache_sync();
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

#if 0
typedef struct mmu_tlb{
	int rep;
	int vaddr;
	int paddr;
	int flag;
}s_mmu_tlb;
s_mmu_tlb mmu_tlb[]={
	{0x400,0x0   ,  0x0  , 0x10c02},
	{0x10, 0x400 ,  0x400, 0xc0e},
	{1,    0x490 ,  0x490, 0xc0e},
	{1,    0x498 ,  0x498, 0xc0e},
	{0x200, 0x800 ,  0x800, 0xc0e},
	{0x200, 0xA00 ,  0xA00, 0x10c02},
	{0x6  , 0xc00 , 0x800 ,0xc0e},
	{0x100, 0xD00 , 0x900 ,0xc0e},
	{0x1  , 0xCFF , 0x0FF  ,0xc0e},
	{0x1,  0xF11 ,  0xc11, 0x412},
	{0x1,  0xF80 ,  0xc80, 0x412},
	{0x1,  0xF81 ,  0xc81, 0x481},
	{0x1,  0xF90 ,  0x490, 0x40e}
};

int appf_setup_translation_tables(void)
{
//	  unsigned firmware_start_pa, firmware_start_va,attr;
	  unsigned tab1_pa,pa;
	  int i,count,t,vaddr,paddr;
	  s_mmu_tlb* ptlb;
	
  //	  firmware_start_va = ((unsigned)&main_table) & PAGE_MASK;
 //   firmware_start_pa = reloc_addr(firmware_start_va);
    tab1_pa = reloc_addr((unsigned)appf_translation_table1);  
    
    //clear table
    for (i = 0; i < 4096; ++i) 
    	((unsigned*)tab1_pa) [i] = 0;

    count = sizeof(mmu_tlb)/sizeof(s_mmu_tlb);
    ptlb = mmu_tlb; 
    for( i = 0; i < count; i++){
   		vaddr = ptlb->vaddr;
   		paddr = ptlb->paddr;
    	for(t = 0; t < ptlb->rep; t++){
    		((unsigned*)tab1_pa) [vaddr] = (ptlb->flag)|((paddr)<<20);
    		vaddr++;
    		paddr++;
     	}
			ptlb++;   	
    }
	 pa = reloc_addr((unsigned)&appf_ttbr0);
    *((unsigned*)pa) = reloc_addr((unsigned)appf_translation_table1);
     
    __V(appf_ttbcr) = 0;
       
 //   _clean_invd_dcache();
 //    l2x0_clean_inv_all();
    
    return APPF_OK;
}
#endif

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
    	if(i <=0x3FF)
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
    		attr = 0x02412;
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
    	else if(i == 0xDFF)
    	{
      	((unsigned*)tab1_pa) [i] = 0xc0e|(0x9FF<<20);
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
    return APPF_OK;
}
#endif

#if 0
struct mmu_tbl{
	unsigned virt_addr_s;
	unsigned virt_addr_e;	
	unsigned phy_addr;	
	unsigned flag;
};

struct mmu_tbl mmutable[]={
	{0x000  , 0x00f,  0x000 , 0x10c02},
	{0x010  , 0x7ff,  0x010 , 0x0},
	{0x800  , 0xbff,  0x800 , 0xc0e},
	{0xc00  , 0xc10,  0xc00 , 0x0},
	{0xc11  , 0xc12,  0xc11 , 0x10412},
//	{0xc13  , 0xc41,  0xc13 , 0x0},
	{0xc13  , 0xc3f,  0xc13 , 0x0},
	{0xc40  , 0xc77,  0x840 , 0x010406},
//	{0xc42  , 0xc43,  0xc42 , 0x2412},
	{0xc78  , 0xc7f,  0xc44 , 0x0},
	{0xc80  , 0xd00,  0xc80 , 0x412},
	{0xd01  , 0xd8f,  0xd01 , 0x0},
	{0xd90  , 0xd90,  0xd90 , 0xc0e},
	{0xd91  , 0xd9F,  0xd91 , 0x0},
	{0xda0  , 0xda0,  0xda0 , 0x2412},
	{0xda1  , 0xdfe,  0xda1 , 0x0},
	
	{0xdff  , 0xdff,  0x9ff , 0x1140e},
	
	{0xe00  , 0xf10,  0xe00 , 0x10c02},	
	{0xf11  , 0xf13,  0xc11 , 0x11452},	
	
	{0xf14  , 0xf21,  0xf14 , 0x10c02},	
	
	{0xf22  , 0xf22,  0x9fe , 0xfd841},	
	{0xf23  , 0xf23,  0xc43 , 0x11452},
	
	{0xf24  , 0xf2f,  0xf24 , 0x10c02},	
		
	{0xf30  , 0xf31,  0xc80 , 0x11452},	
	{0xf32  , 0xf39,  0xc90 , 0x11452},	

	{0xf3a  , 0xf3f,  0xf3a , 0x10c02},	
	
	{0xf40  , 0xf7f,  0xcc0 , 0x940e},	
	{0xf80  , 0xf80,  0xd00 , 0x940e},	
	
	{0xf81  , 0xf8f,  0xf81 , 0x10c02},	
	
	{0xf90  , 0xf90,  0x9fe , 0xfc841},	
	{0xf91  , 0xf91,  0x9fe , 0xfcc41},	

	{0xf92  , 0xf9f,  0xf92 , 0x10c02},	
	
	{0xfa0  , 0xfa0,  0x9fe , 0xfa841},	
	{0xfa1  , 0xfa1,  0x9fe , 0xfac41},	
	
	{0xfa2  , 0xfff,  0xfa2 , 0x10c02},	
};

int appf_setup_translation_tables(void)
{
		int count;
	 // unsigned firmware_start_pa, firmware_start_va,attr;
	  unsigned tab1_pa,pa;
	  unsigned virt_addr,phy_addr;
	  unsigned data_addr;
	  unsigned flag;
	  struct mmu_tbl* ptbl;
	  int i; 
    data_addr = reloc_addr((unsigned)appf_translation_table1);  
    
		
    count = sizeof(mmutable)/sizeof(struct mmu_tbl);
    	ptbl = &(mmutable[0]);
    for( i = 0; i < count; i++){
			 phy_addr = ptbl->phy_addr;
			 flag = ptbl->flag;
			 for(virt_addr = ptbl->virt_addr_s; virt_addr <= ptbl->virt_addr_e; virt_addr++){
			 		writel(((phy_addr<<20) | flag),data_addr);
			 		phy_addr++;
			 		data_addr += 4;
			 }   	
    	
    	ptbl++;
    }
    
    pa = reloc_addr((unsigned)&appf_ttbr0);
    *((volatile unsigned*)pa) = reloc_addr((unsigned)appf_translation_table1);
     
    __V(appf_ttbcr) = 0;
//extern  void v7_invalidate_dcache_all(void);
//  	v7_flush_cache_all();    
    return APPF_OK;
}
#endif
