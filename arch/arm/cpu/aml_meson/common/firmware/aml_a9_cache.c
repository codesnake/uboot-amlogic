
#ifndef  __AML_A9_CACHE__
#define  __AML_A9_CACHE__

#include <asm/system.h>
#include <asm/types.h>

#define CP15DSB     asm volatile("dsb": : : "memory");
#define CP15ISB     asm volatile("isb": : : "memory");

#define ARMV7_DCACHE_INVAL_ALL		1
#define ARMV7_DCACHE_CLEAN_INVAL_ALL	2
#define ARMV7_DCACHE_INVAL_RANGE	3
#define ARMV7_DCACHE_CLEAN_INVAL_RANGE	4

/* CCSIDR */
#define CCSIDR_LINE_SIZE_OFFSET		0
#define CCSIDR_LINE_SIZE_MASK		0x7
#define CCSIDR_ASSOCIATIVITY_OFFSET	3
#define CCSIDR_ASSOCIATIVITY_MASK	(0x3FF << 3)
#define CCSIDR_NUM_SETS_OFFSET		13
#define CCSIDR_NUM_SETS_MASK		(0x7FFF << 13)

/*
 * Values for InD field in CSSELR
 * Selects the type of cache
 */
#define ARMV7_CSSELR_IND_DATA_UNIFIED	0
#define ARMV7_CSSELR_IND_INSTRUCTION	1

/* Values for Ctype fields in CLIDR */
#define ARMV7_CLIDR_CTYPE_NO_CACHE		0
#define ARMV7_CLIDR_CTYPE_INSTRUCTION_ONLY	1
#define ARMV7_CLIDR_CTYPE_DATA_ONLY		2
#define ARMV7_CLIDR_CTYPE_INSTRUCTION_DATA	3
#define ARMV7_CLIDR_CTYPE_UNIFIED		4

static inline s32 log_2_n_round_up(u32 n)
{
	s32 log2n = -1;
	u32 temp = n;

	while (temp) {
		log2n++;
		temp >>= 1;
	}

	if (n & (n - 1))
		return log2n + 1; /* not power of 2 - round up */
	else
		return log2n; /* power of 2 */
}

static void set_csselr(u32 level, u32 type)
{	u32 csselr = level << 1 | type;

	/* Write to Cache Size Selection Register(CSSELR) */
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r" (csselr));
}

static u32 get_ccsidr(void)
{
	u32 ccsidr;

	/* Read current CP15 Cache Size ID Register */
	asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));
	return ccsidr;
}

static u32 get_clidr(void)
{
	u32 clidr;

	/* Read current CP15 Cache Level ID Register */
	asm volatile ("mrc p15,1,%0,c0,c0,1" : "=r" (clidr));
	return clidr;
}

static void v7_inval_dcache_level_setway(u32 level, u32 num_sets,
					 u32 num_ways, u32 way_shift,
					 u32 log2_line_len)
{
	int way, set, setway;

	/*
	 * For optimal assembly code:
	 *	a. count down
	 *	b. have bigger loop inside
	 */
	for (way = num_ways - 1; way >= 0 ; way--) {
		for (set = num_sets - 1; set >= 0; set--) {
			setway = (level << 1) | (set << log2_line_len) |
				 (way << way_shift);
			/* Invalidate data/unified cache line by set/way */
			asm volatile ("	mcr p15, 0, %0, c7, c6, 2"
					: : "r" (setway));
		}
	}
	/* DSB to make sure the operation is complete */
	CP15DSB;
}

static void v7_clean_inval_dcache_level_setway(u32 level, u32 num_sets,
					       u32 num_ways, u32 way_shift,
					       u32 log2_line_len)
{
	int way, set, setway;

	/*
	 * For optimal assembly code:
	 *	a. count down
	 *	b. have bigger loop inside
	 */
	for (way = num_ways - 1; way >= 0 ; way--) {
		for (set = num_sets - 1; set >= 0; set--) {
			setway = (level << 1) | (set << log2_line_len) |
				 (way << way_shift);
			/*
			 * Clean & Invalidate data/unified
			 * cache line by set/way
			 */
			asm volatile ("	mcr p15, 0, %0, c7, c14, 2"
					: : "r" (setway));
		}
	}
	/* DSB to make sure the operation is complete */
	CP15DSB;
}

static void v7_maint_dcache_level_setway(u32 level, u32 operation)
{
	u32 ccsidr;
	u32 num_sets, num_ways, log2_line_len, log2_num_ways;
	u32 way_shift;

	set_csselr(level, ARMV7_CSSELR_IND_DATA_UNIFIED);

	ccsidr = get_ccsidr();

	log2_line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >>
				CCSIDR_LINE_SIZE_OFFSET) + 2;
	/* Converting from words to bytes */
	log2_line_len += 2;

	num_ways  = ((ccsidr & CCSIDR_ASSOCIATIVITY_MASK) >>
			CCSIDR_ASSOCIATIVITY_OFFSET) + 1;
	num_sets  = ((ccsidr & CCSIDR_NUM_SETS_MASK) >>
			CCSIDR_NUM_SETS_OFFSET) + 1;
	/*
	 * According to ARMv7 ARM number of sets and number of ways need
	 * not be a power of 2
	 */
	log2_num_ways = log_2_n_round_up(num_ways);

	way_shift = (32 - log2_num_ways);
	if (operation == ARMV7_DCACHE_INVAL_ALL) {
		v7_inval_dcache_level_setway(level, num_sets, num_ways,
				      way_shift, log2_line_len);
	} else if (operation == ARMV7_DCACHE_CLEAN_INVAL_ALL) {
		v7_clean_inval_dcache_level_setway(level, num_sets, num_ways,
						   way_shift, log2_line_len);
	}
}

static void v7_maint_dcache_all(u32 operation)
{
	u32 level, cache_type, level_start_bit = 0;

	u32 clidr = get_clidr();

	for (level = 0; level < 7; level++) {
		cache_type = (clidr >> level_start_bit) & 0x7;
		if ((cache_type == ARMV7_CLIDR_CTYPE_DATA_ONLY) ||
		    (cache_type == ARMV7_CLIDR_CTYPE_INSTRUCTION_DATA) ||
		    (cache_type == ARMV7_CLIDR_CTYPE_UNIFIED))
			v7_maint_dcache_level_setway(level, operation);
		level_start_bit += 3;
	}
}

static void v7_dcache_clean_inval_range(u32 start,
					u32 stop, u32 line_len)
{
	u32 mva;

	/* Align start to cache line boundary */
	start &= ~(line_len - 1);
	for (mva = start; mva < stop; mva = mva + line_len) {
		/* DCCIMVAC - Clean & Invalidate data cache by MVA to PoC */
		asm volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r" (mva));
	}
}

static void v7_dcache_inval_range(u32 start, u32 stop, u32 line_len)
{
	u32 mva;

	/*
	 * If start address is not aligned to cache-line do not
	 * invalidate the first cache-line
	 */
	if (start & (line_len - 1)) {
		
		/* move to next cache line */
		start = (start + line_len - 1) & ~(line_len - 1);
	}

	/*
	 * If stop address is not aligned to cache-line do not
	 * invalidate the last cache-line
	 */
	if (stop & (line_len - 1)) {
		
		/* align to the beginning of this cache line */
		stop &= ~(line_len - 1);
	}

	for (mva = start; mva < stop; mva = mva + line_len) {
		/* DCIMVAC - Invalidate data cache by MVA to PoC */
		asm volatile ("mcr p15, 0, %0, c7, c6, 1" : : "r" (mva));
	}
}

static void v7_dcache_maint_range(u32 start, u32 stop, u32 range_op)
{
	u32 line_len, ccsidr;

	ccsidr = get_ccsidr();
	line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >>
			CCSIDR_LINE_SIZE_OFFSET) + 2;
	/* Converting from words to bytes */
	line_len += 2;
	/* converting from log2(linelen) to linelen */
	line_len = 1 << line_len;

	switch (range_op) {
	case ARMV7_DCACHE_CLEAN_INVAL_RANGE:
		v7_dcache_clean_inval_range(start, stop, line_len);
		break;
	case ARMV7_DCACHE_INVAL_RANGE:
		v7_dcache_inval_range(start, stop, line_len);
		break;
	}

	/* DSB to make sure the operation is complete */
	CP15DSB;
}

/* Invalidate TLB */
static void v7_inval_tlb(void)
{
	/* Invalidate entire unified TLB */
	asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
	/* Invalidate entire data TLB */
	asm volatile ("mcr p15, 0, %0, c8, c6, 0" : : "r" (0));
	/* Invalidate entire instruction TLB */
	asm volatile ("mcr p15, 0, %0, c8, c5, 0" : : "r" (0));
	/* Full system DSB - make sure that the invalidation is complete */
	CP15DSB;
	/* Full system ISB - make sure the instruction stream sees it */
	CP15ISB;
}

void invalidate_dcache_all(void)
{
	v7_maint_dcache_all(ARMV7_DCACHE_INVAL_ALL);

}

/*
 * Performs a clean & invalidation of the entire data cache
 * at all levels
 */
void flush_dcache_all(void)
{
	v7_maint_dcache_all(ARMV7_DCACHE_CLEAN_INVAL_ALL);

}

/*
 * Invalidates range in all levels of D-cache/unified cache used:
 * Affects the range [start, stop - 1]
 */
void invalidate_dcache_range(unsigned long start, unsigned long stop)
{
	//serial_puts("\invalidate range\n");
	v7_dcache_maint_range(start, stop, ARMV7_DCACHE_INVAL_RANGE);

}

/*
 * Flush range(clean & invalidate) from all levels of D-cache/unified
 * cache used:
 * Affects the range [start, stop - 1]
 */
void flush_dcache_range(unsigned long start, unsigned long stop)
{
	v7_dcache_maint_range(start, stop, ARMV7_DCACHE_CLEAN_INVAL_RANGE);

}

void arm_init_before_mmu(void)
{
	invalidate_dcache_all();
	v7_inval_tlb();
}


/*
 * Flush range from all levels of d-cache/unified-cache used:
 * Affects the range [start, start + size - 1]
 */
void  flush_cache(unsigned long start, unsigned long size)
{
	flush_dcache_range(start, start + size);
}

static inline void cp_delay (void)
{
	volatile int i;
	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < 100; i++)
		nop();
	asm volatile("" : : : "memory");
}

/* to activate the MMU we need to set up virtual memory: use 1M areas */
static inline void mmu_setup(void)
{	
	arm_init_before_mmu();
	/*
	 * Invalidate L1 I/D
	 */
	asm volatile("mov	r0, #0");//			@ set up for MCR
	asm volatile("mcr	p15, 0, r0, c8, c7, 0");//	@ invalidate TLBs
	asm volatile("mcr	p15, 0, r0, c7, c5, 0");//	@ invalidate icache

	/*
	 * disable MMU stuff and caches
	 */
	asm volatile("mrc	p15, 0, r0, c1, c0, 0");//
	asm volatile("bic	r0, r0, #0x00002000");//	@ clear bits 13(--V--)	

	asm volatile("bic	r0, r0, #0x00000007");//	@ clear bits 2:0 (-CAM)
	asm volatile("orr	r0, r0, #0x00000002");//	@ set bit 1 (--A-) Align
	asm volatile("orr	r0, r0, #0x00000800");//	@ set bit 12 (Z---) BTB
	asm volatile("mcr	p15, 0, r0, c1, c0, 0");//

	/////////////////////////////////////////////////////////////	
	unsigned int *pVMMUTable = (unsigned int *)(0xd9000000 + 32 * 1024);
	int i = 0;
	uint nVal = 0;
	for(i = 0 ; i < 0x1000;++i)
	{
#if defined (CONFIG_AML_MESON_8)

		if(i< CONFIG_MMU_DDR_SIZE || 0xd90 == i)
			nVal = (i<<20)|(SEC_PROT_RW_RW | SEC_WB);		
		else if(i>= 0xC00 && i< 0xC13)
			nVal = (i<<20)|(SEC_PROT_RW_NA | SEC_XN | SEC_SO_MEM);
		else if((i>= 0xC42 && i< 0xC44) || (0xda0 == i))
			nVal = (i<<20)|(SEC_PROT_RW_NA |  SEC_XN | SEC_DEVICE);
		else if(i>= 0xC80 && i< 0xd02)
			nVal = (i<<20)|(SEC_PROT_RW_NA | SEC_XN | SECTION);
		else
			nVal = 0;

#else
		if(i< 0x10)
			nVal = (i<<20)|(SEC_PROT_RW_RW | SEC_SO_MEM);
		if((i>= 0x10 && i< 0x800) ||
			(i>= 0xd91 && i< 0xda0) ||
			(i>= 0xC13 && i< 0xC42) ||
			(i>= 0xC44 && i< 0xC80) ||
			(i>= 0xC00 && i< 0xC11) ||
			(i>= 0xd01 && i< 0xd90) ||
			(i>= 0xda1 && i< 0xe00))
			nVal = 0;
		
		if((i>= 0x800 && i< 0xa00) || (i>= 0xd90 && i< 0xd91))
			nVal = (i<<20)|(SEC_PROT_RW_RW | SEC_WB);
#ifdef M6_DDR3_1GB
		if(i>= 0xA00 && i< 0xC00)
			nVal = (i<<20)|(SEC_PROT_RW_RW | SEC_WB);
#else
		if(i>= 0xA00 && i< 0xC00)
			nVal = (i<<20)|(SEC_PROT_RW_RW | SEC_SO_MEM);
#endif
		if(i>= 0xC11 && i< 0xC13)
			nVal = (i<<20)|(SEC_PROT_RW_NA | SEC_XN | SEC_SO_MEM);

		if((i>= 0xC42 && i< 0xC44) || (0xda0 == i))
			nVal = (i<<20)|(SEC_PROT_RW_NA |  SEC_XN | SEC_DEVICE);

		if(i>= 0xC80 && i< 0xd01)
			nVal = (i<<20)|(SEC_PROT_RW_NA | SEC_XN | SECTION);
				 
		if(i>= 0xe00 && i<= 0xfff)
			nVal = (i<<20)|(SEC_PROT_RW_RW | SEC_SO_MEM);
#endif //CONFIG_M8

		*(pVMMUTable+i) = nVal;//(i << 20 | (SEC_PROT_RW_RW | SEC_WB));
		
	}
	u32 *page_table = (u32 *)(pVMMUTable);
	//int i;
	u32 reg;

	/* Copy the page table address to cp15 */
	asm volatile("mcr p15, 0, %0, c2, c0, 0"
		     : : "r" (page_table) : "memory");
	/* Set the access control to client */
	asm volatile("mcr p15, 0, %0, c3, c0, 0"
		     : : "r" (0x55555555));
		     	
 	asm volatile("mcr p15, 0, %0, c7, c5, 6"   : : "r" (0));   // invalidate BTAC    				   	
 	asm volatile("mcr p15, 0, %0, c7, c5, 0"   : : "r" (0));   // invalidate ICache
    asm volatile("dsb");
    asm volatile("mcr p15, 0, %0, c8, c7, 0"	  : : "r" (0));    // invalidate TLBs
	asm volatile("dsb");
 	asm volatile("isb");
	
	/* and enable the mmu */
	reg = get_cr();	/* get control reg. */
	cp_delay();
	set_cr(reg | CR_M);
}

/* cache_bit must be either CR_I or CR_C */
static void cache_enable(uint32_t cache_bit)
{
	uint32_t reg;

	/* The data cache is not active unless the mmu is enabled too */
	if (cache_bit == CR_C)
		mmu_setup();
	reg = get_cr();	/* get control reg. */
	cp_delay();
	set_cr(reg | cache_bit);
}


/* cache_bit must be either CR_I or CR_C */
static void cache_disable(uint32_t cache_bit)
{
	uint32_t reg;

	if (cache_bit == CR_C) {
		/* if cache isn;t enabled no need to disable */
		reg = get_cr();
		if ((reg & CR_C) != CR_C)
			return;
		/* if disabling data cache, disable mmu too */
		cache_bit |= CR_M;
		//flush_cache(0, ~0);
		flush_dcache_all();
	}
	reg = get_cr();
	cp_delay();
	set_cr(reg & ~cache_bit);
}

static void aml_cache_disable(void)
{
	flush_dcache_all();
	cache_disable(CR_I);
	cache_disable(CR_C);
	unsigned int *pVMMUTable = (unsigned int *)(0xd9000000 + 32 * 1024);
	int i = 0;
	for(i = 0;i< 0x1000;++i)
		*(pVMMUTable+i) = 0;	
}

static void aml_cache_enable(void)
{	
	cache_enable(CR_I);
	cache_enable(CR_C);
}


#endif // __AML_A9_CACHE__
