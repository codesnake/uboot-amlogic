
void init_dmc(struct ddr_set * timing_set)
{
	writel(0xffffffff, P_DMC_SOFT_RST);
	writel(0xffffffff, P_DMC_SOFT_RST1);
	writel(0x20109a27, P_DMC_REFR_CTRL2);
	writel(0x80389d, P_DMC_REFR_CTRL1);

#ifdef CONFIG_DDR_MODE_AUTO_DETECT
	//enable pctl clock
	writel(readl(P_DDR0_CLK_CTRL) | 0x1, P_DDR0_CLK_CTRL);
	__udelay(10);
	//re-init DMC reg
	if(cfg_ddr_mode > CFG_DDR_32BIT)
		timing_set->t_mmc_ddr_ctrl = (timing_set->t_mmc_ddr_ctrl & (~(0x8080))) | (1<<15) | (1<<7);
	else
		timing_set->t_mmc_ddr_ctrl = (timing_set->t_mmc_ddr_ctrl & (~(0x8080))) | (0<<15) | (0<<7);
	writel(timing_set->t_mmc_ddr_ctrl, P_DMC_DDR_CTRL);
	//re-init DT address
	int mem_mode = (cfg_ddr_mode > CFG_DDR_32BIT) ? (1) : (0);
	timing_set->t_pub0_dtar	= ((0x0 + (M8BABY_DDR_DTAR_DTCOL_GET(CONFIG_M8B_RANK0_DTAR_ADDR,CONFIG_DDR_COL_BITS,CONFIG_M8B_DDR_BANK_SET,mem_mode)))|\
		((M8BABY_DDR_DTAR_DTROW_GET(CONFIG_M8B_RANK0_DTAR_ADDR,CONFIG_DDR_ROW_BITS,CONFIG_DDR_COL_BITS,CONFIG_M8B_DDR_BANK_SET,mem_mode)) << 12)|\
		((M8BABY_DDR_DTAR_BANK_GET(CONFIG_M8B_RANK0_DTAR_ADDR,CONFIG_DDR_ROW_BITS,CONFIG_DDR_COL_BITS,CONFIG_M8B_DDR_BANK_SET,mem_mode)) << 28));
	writel((0x0  + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR0);
	writel((0x08 + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR1);
	writel((0x10 + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR2);
	writel((0x18 + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR3);
	__udelay(10);
	//disable pctl clock
	writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);
	__udelay(1);
#else
	writel(timing_set->t_mmc_ddr_ctrl, P_DMC_DDR_CTRL);
#endif

	writel(0xffff0000, DMC_SEC_RANGE0_CTRL);
	writel(0xffffffff, DMC_SEC_RANGE1_CTRL);
	writel(0xffffffff, DMC_SEC_RANGE2_CTRL);
	writel(0xffffffff, DMC_SEC_RANGE3_CTRL);
	writel(0xffffffff, DMC_SEC_AXI_PORT_CTRL);
	writel(0xffffffff, DMC_SEC_AM_PORT_CTRL);
	writel(0xffffffff, DMC_DEV_RANGE_CTRL);
	writel(0xffffffff, DMC_DEV_RANGE_CTRL1);
	writel(0x80000000, DMC_SEC_CTRL);
	//writel(0x12a, P_DDR0_CLK_CTRL);
	writel(0xffff, P_DMC_REQ_CTRL);

	//change PL310 address filtering to allow DRAM reads to go to M1
	writel(0xc0000000, 0xc4200c04);
	writel(0x00000001, 0xc4200c00);

	//put some code here to try to stop bus traffic
	asm("NOP");
	asm("DMB");
	asm("ISB");
}

int ddr_init_hw(struct ddr_set * timing_set)
{
	int ret = 0;
	ret = timing_set->init_pctl(timing_set);

	if(ret)
	{
#ifdef CONFIG_DDR_MODE_AUTO_DETECT
		//serial_puts("PUB init fail!\n");
		return -1;
#else
		serial_puts("\nPUB init fail! Reset...\n");
		__udelay(10000);
		AML_WATCH_DOG_START();
#endif
	}

	//asm volatile("wfi");
	//while(init_dmc(timing_set);){}
	init_dmc(timing_set);

	return 0;
}

#if defined(CONFIG_AML_EXT_PGM)
void ddr_info_dump(struct ddr_set * timing_set){}
#else
void ddr_info_dump(struct ddr_set * timing_set)
{
#ifdef DDR_SCRAMBE_ENABLE
	unsigned int dmc_sec_ctrl_value;
	unsigned int ddr_key;

	ddr_key = readl(P_RAND64_ADDR0);
	dmc_sec_ctrl_value = 0x80000000 | (1<<0);
	writel(dmc_sec_ctrl_value, DMC_SEC_CTRL);
	while( readl(DMC_SEC_CTRL) & 0x80000000 ) {}
	writel(ddr_key &0x0000ffff, DMC_SEC_KEY0);
	writel((ddr_key >>16)&0x0000ffff, DMC_SEC_KEY1);

#ifdef CONFIG_DUMP_DDR_INFO
	dmc_sec_ctrl_value = readl(DMC_SEC_CTRL);
	if(dmc_sec_ctrl_value & (1<<0)){
		serial_puts("ddr scramb EN\n");
	}
#endif
#endif

#ifdef CONFIG_DUMP_DDR_INFO
	int nPLL = readl(AM_DDR_PLL_CNTL);
	int nDDRCLK = 2*((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
	serial_puts("DDR clock: ");
#ifdef CONFIG_DDR_BYPASS_PHY_PLL
	nDDRCLK=nDDRCLK/4;
#endif
	serial_put_dec(nDDRCLK);
	serial_puts("MHz with ");
#ifdef CONFIG_DDR_LOW_POWER
	serial_puts("Low Power & ");
#endif
	if((timing_set->t_pctl_mcfg) & (1<<3)) //DDR0, DDR1 same setting?
		serial_puts("2T mode\n");
	else
		serial_puts("1T mode\n");
#endif

#ifdef CONFIG_DDR_BYPASS_PHY_PLL
	serial_puts("DDR pll bypass: Enabled");
#else
	serial_puts("DDR pll bypass: Disabled");
#endif
}
#endif 
