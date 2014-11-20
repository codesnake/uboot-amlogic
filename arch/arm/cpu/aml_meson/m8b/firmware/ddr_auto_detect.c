
#if defined(CONFIG_AML_EXT_PGM)

void print_ddr_size(unsigned int size){}
void print_ddr_mode(void){}

#else
void print_ddr_size(unsigned int size)
{
	serial_puts("DDR size: ");
	unsigned int mem_size = size >> 20; //MB
	(mem_size) >= 1024 ? serial_put_dec(mem_size >> 10):serial_put_dec(mem_size);
	(mem_size) >= 1024 ? serial_puts("GB"):serial_puts("MB");
#ifdef CONFIG_DDR_SIZE_AUTO_DETECT
	serial_puts(" (auto)\n");
#else
	serial_puts("\n");
#endif
}

void print_ddr_mode(void){
	serial_puts("DDR mode: ");
	switch(cfg_ddr_mode){
		case CFG_DDR_NOT_SET:
			serial_puts("Not Set"); break;
		case CFG_DDR_32BIT:
			serial_puts("32 bit mode"); break;
		case CFG_DDR_16BIT_LANE02:
			serial_puts("16 bit mode lane0+2"); break;
		case CFG_DDR_16BIT_LANE01:
			serial_puts("16 bit mode lane0+1"); break;
	}
#ifdef CONFIG_DDR_MODE_AUTO_DETECT
	serial_puts(" (auto)\n");
#else
	serial_puts("\n");
#endif
}
#endif

/*Following code is for DDR MODE AUTO DETECT*/
#ifdef CONFIG_DDR_MODE_AUTO_DETECT
static inline unsigned ddr_init(struct ddr_set * timing_reg);

int ddr_mode_auto_detect(struct ddr_set * timing_reg){
	int ret = 0;
#ifdef CONFIG_DDR_MODE_AUTO_DETECT_SKIP_32BIT
	int try_times = 2;
#else
	int try_times = 1;
#endif
	for(; try_times <= 3; try_times++){
		cfg_ddr_mode = try_times;
		ret = ddr_init(timing_reg);
		if(!ret){
			print_ddr_mode();
			return 0;
		}
	}
	serial_puts("\nAll ddr mode test failed, reset...\n");
	__udelay(10000); 
	AML_WATCH_DOG_START();
}
#endif

/*Following code is for DDR SIZE AUTO DETECT*/
#define DDR_SIZE_AUTO_DETECT_DEBUG 0
#ifdef CONFIG_DDR_SIZE_AUTO_DETECT
#define DDR_SIZE_AUTO_DETECT_PATTERN	0xAABBBBAA
#define MEMORY_ROW_MASK_BIT_OFFSET_32BIT_MAX 28	//1<<28, mask 1GB
#define MEMORY_ROW_MASK_BIT_OFFSET_32BIT_MIN 26	//1<<26, mask 256MB
#define MEMORY_ROW_MASK_BIT_OFFSET_16BIT_MAX 27	//1<<27, mask 512MB
#define MEMORY_ROW_MASK_BIT_OFFSET_16BIT_MIN 25	//1<<25, mask 128MB
#define MEMORY_ROW_BITS(mem_size, mode_16bit) ((mem_size >> 29) + (mode_16bit ? 14:13))
#define MEMORY_ROW_BITS_DMC_REG(mem_size, mode_16bit) \
	(MEMORY_ROW_BITS(mem_size, mode_16bit) > 15)?(0):(MEMORY_ROW_BITS(mem_size, mode_16bit) - 12)
#define MEMORY_ROW_BITS_DMC_REG_SIMP(row_bits) ((row_bits > 15)?(0):(row_bits- 12))
void ddr_size_auto_detect(struct ddr_set * timing_reg){
/*memory size auto detect function only support:
32bit mode: 1GB, 512MB, 256MB
16bit mode: 512MB, 256MB, 128MB*/
	//enable pctl clk
	writel(readl(P_DDR0_CLK_CTRL) | 0x1, P_DDR0_CLK_CTRL);
	__udelay(1000);
#if DDR_SIZE_AUTO_DETECT_DEBUG //debug info
	serial_puts("timing_reg->t_pub0_dtar: ");
	serial_put_hex(timing_reg->t_pub0_dtar, 32);
	serial_puts("\n");
	serial_puts("P_DDR0_PUB_DTAR0: ");
	serial_put_hex(readl(P_DDR0_PUB_DTAR0), 32);
	serial_puts("\n");
	serial_puts("timing_reg->t_mmc_ddr_ctrl: ");
	serial_put_hex(timing_reg->t_mmc_ddr_ctrl, 32);
	serial_puts("\n");
	serial_puts("P_DMC_DDR_CTRL: ");
	serial_put_hex(readl(P_DMC_DDR_CTRL), 32);
	serial_puts("\n");
	serial_puts("DT_ADDR: ");
	serial_put_hex(CONFIG_M8B_RANK0_DTAR_ADDR, 32);
	serial_puts("\n");
	serial_puts("M8BABY_GET_DT_ADDR: ");
	serial_put_hex(M8BABY_GET_DT_ADDR(readl(P_DDR0_PUB_DTAR0), readl(P_DMC_DDR_CTRL)), 32);
	serial_puts("\n");
#endif
	unsigned int mem_sizes[] = {
		0x10000000,	//256MB
		0x20000000,	//512MB
		0x40000000,	//1GB
		//0x80000000	//2GB
	};

	//set max row_size, then use "ROW ADDRESS MASK" to detect memory size
	unsigned int dmc_reg_max_row = ((timing_reg->t_mmc_ddr_ctrl & (~(3<<2))) | (0<<2) | (0<<10));
	writel(dmc_reg_max_row, P_DMC_DDR_CTRL);

	//row_size of 16bit and 32bit mode are different
	unsigned int mem_mode = 0;
	unsigned int row_mask_bit_offset = MEMORY_ROW_MASK_BIT_OFFSET_32BIT_MIN;
	if(cfg_ddr_mode > CFG_DDR_32BIT){
		mem_mode = 1;
		row_mask_bit_offset = MEMORY_ROW_MASK_BIT_OFFSET_16BIT_MIN;
	}
	unsigned int cur_mask_addr = 0;
	unsigned int cur_mem_size = 0;
	int loop = 0;
	for(loop = 0; loop < (sizeof(mem_sizes) / sizeof(unsigned int)); loop++){
		cur_mem_size = (mem_sizes[loop] >> 20) >> (mem_mode);	//Byte->MByte
		cur_mask_addr = (1 << (row_mask_bit_offset + loop));
		__udelay(1);
		writel(0x12345678, 0);
		writel(0x87654321, cur_mask_addr);
#if DDR_SIZE_AUTO_DETECT_DEBUG	//debug info
		serial_puts("write address: ");
		serial_put_hex(cur_mask_addr, 32);
		serial_puts(" with 0x87654321\nread back: ");
		serial_put_hex(readl(cur_mask_addr), 32);
		serial_puts("\n");
		serial_puts("readl 0: ");
		serial_put_hex(readl(0), 32);
		serial_puts("\n");
#endif
		asm volatile("DSB"); /*sync ddr data*/
		if(readl(0) == 0x87654321){
			break;
		}
	}
	print_ddr_size(cur_mem_size << 20);

	/*Get corresponding row_bits setting and write back to DMC reg*/
	unsigned int cur_row_bits = MEMORY_ROW_BITS((cur_mem_size<<20), mem_mode);
	unsigned int cur_row_bit_dmc = MEMORY_ROW_BITS_DMC_REG_SIMP(cur_row_bits);
	timing_reg->t_mmc_ddr_ctrl = ((timing_reg->t_mmc_ddr_ctrl & (~(3<<2))) | (cur_row_bit_dmc<<2) | ((cur_row_bit_dmc<<10)));
	writel(timing_reg->t_mmc_ddr_ctrl, P_DMC_DDR_CTRL);
	__udelay(10);

	timing_reg->t_pub0_dtar	= ((0x0 + (M8BABY_DDR_DTAR_DTCOL_GET(CONFIG_M8B_RANK0_DTAR_ADDR,CONFIG_DDR_COL_BITS,CONFIG_M8B_DDR_BANK_SET,mem_mode)))|\
		((M8BABY_DDR_DTAR_DTROW_GET(CONFIG_M8B_RANK0_DTAR_ADDR,cur_row_bits,CONFIG_DDR_COL_BITS,CONFIG_M8B_DDR_BANK_SET,mem_mode)) << 12)|\
		((M8BABY_DDR_DTAR_BANK_GET(CONFIG_M8B_RANK0_DTAR_ADDR,cur_row_bits,CONFIG_DDR_COL_BITS,CONFIG_M8B_DDR_BANK_SET,mem_mode)) << 28));
	writel((0x0  + timing_reg->t_pub0_dtar), P_DDR0_PUB_DTAR0);
	writel((0x08 + timing_reg->t_pub0_dtar), P_DDR0_PUB_DTAR1);
	writel((0x10 + timing_reg->t_pub0_dtar), P_DDR0_PUB_DTAR2);
	writel((0x18 + timing_reg->t_pub0_dtar), P_DDR0_PUB_DTAR3);
	__udelay(10);
#if DDR_SIZE_AUTO_DETECT_DEBUG	//debug info
	serial_puts("cur_row_bits: ");
	serial_put_hex(cur_row_bits, 32);
	serial_puts("\n");
	serial_puts("cur_row_bit_dmc: ");
	serial_put_hex(cur_row_bit_dmc, 32);
	serial_puts("\n");
	serial_puts("P_DMC_DDR_CTRL: ");
	serial_put_hex(readl(P_DMC_DDR_CTRL), 32);
	serial_puts("\n");
	serial_puts("timing_reg->t_mmc_ddr_ctrl: ");
	serial_put_hex(timing_reg->t_mmc_ddr_ctrl, 32);
	serial_puts("\n");
	serial_puts("timing_reg->t_pub0_dtar: ");
	serial_put_hex(timing_reg->t_pub0_dtar, 32);
	serial_puts("\n");
	serial_puts("M8BABY_GET_DT_ADDR: ");
	serial_put_hex(M8BABY_GET_DT_ADDR((readl(P_DDR0_PUB_DTAR0)), (readl(P_DMC_DDR_CTRL))), 32);
	serial_puts("\n");
#endif

	timing_reg->phy_memory_size = (cur_mem_size << 20);

	//disable pctl clk
	writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);
}
#endif
