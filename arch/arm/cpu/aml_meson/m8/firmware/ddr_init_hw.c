
void init_dmc_m8m2(struct ddr_set * timing_set)
{
	//serial_puts("timing_set->t_mmc_ddr_ctrl: ");
	//serial_put_hex(timing_set->t_mmc_ddr_ctrl, 32);
	//serial_puts("\n");
	writel(timing_set->t_mmc_ddr_ctrl, P_DMC_DDR_CTRL);

	writel(0xffff0000, DMC_SEC_RANGE0_CTRL);
	writel(0xffffffff, DMC_SEC_RANGE1_CTRL);
	writel(0xffffffff, DMC_SEC_RANGE2_CTRL);
	writel(0xffffffff, DMC_SEC_RANGE3_CTRL);
	writel(0xffffffff, DMC_SEC_AXI_PORT_CTRL);
	writel(0xffffffff, DMC_SEC_AM_PORT_CTRL);
	writel(0xffffffff, DMC_DEV_RANGE_CTRL);
	writel(0xffffffff, DMC_DEV_RANGE_CTRL1);
	writel(0x80000000, M8M2_DMC_SEC_CTRL);

	//changed for AXI and AMBUS arbiter weight control.
	writel((0x1f | (0xf << 6)), P_DMC_2ARB_CTRL);

	//enable the DMC auto refresh function
	writel(0x20109a27, P_DMC_REFR_CTRL2);
	writel(0x80389f, P_DMC_REFR_CTRL1);

	//enable the dc_reqs.
	writel(0xffff, P_DMC_REQ_CTRL);

	//put some code here to try to stop bus traffic
	asm("NOP");
	asm("DMB");
	asm("ISB");

	//change PL310 address filtering to allow DRAM reads to go to M1
	writel(0xbff00000, 0xc4200c04);
	writel(0x00000001, 0xc4200c00);
}

void init_dmc_m8(struct ddr_set * timing_set)
{
	//~ while((readl(P_MMC_RST_STS)&0x1FFFF000) != 0x0);
	//~ while((readl(P_MMC_RST_STS1)&0xFFE) != 0x0);
	//deseart all reset.
	//~ writel(0x1FFFF000, P_MMC_SOFT_RST);
	//~ writel(0xFFE, P_MMC_SOFT_RST1);
	//delay_us(100); //No delay need.
	//~ while((readl(P_MMC_RST_STS) & 0x1FFFF000) != (0x1FFFF000));
	//~ while((readl(P_MMC_RST_STS1) & 0xFFE) != (0xFFE));
	//delay_us(100); //No delay need.

	writel(timing_set->t_mmc_ddr_ctrl, P_MMC_DDR_CTRL);

	writel(0x00000000, DMC_SEC_RANGE0_ST);
	writel(0xffffffff, DMC_SEC_RANGE0_END);
	writel(0xffff, DMC_SEC_PORT0_RANGE0);
	writel(0xffff, DMC_SEC_PORT1_RANGE0);
	writel(0xffff, DMC_SEC_PORT2_RANGE0);
	writel(0xffff, DMC_SEC_PORT3_RANGE0);
	writel(0xffff, DMC_SEC_PORT4_RANGE0);
	writel(0xffff, DMC_SEC_PORT5_RANGE0);
	writel(0xffff, DMC_SEC_PORT6_RANGE0);
	writel(0xffff, DMC_SEC_PORT7_RANGE0);
	writel(0xffff, DMC_SEC_PORT8_RANGE0);
	writel(0xffff, DMC_SEC_PORT9_RANGE0);
	writel(0xffff, DMC_SEC_PORT10_RANGE0);
	writel(0xffff, DMC_SEC_PORT11_RANGE0);
	writel(0x80000000, M8_DMC_SEC_CTRL);
	while( readl(M8_DMC_SEC_CTRL) & 0x80000000 ) {}

	//for MMC low power mode. to disable PUBL and PCTL clocks
	//writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);
	//writel(readl(P_DDR1_CLK_CTRL) & (~1), P_DDR1_CLK_CTRL);

	//enable all channel
	writel(0xfff, P_MMC_REQ_CTRL);

	//for performance enhance
	//MMC will take over DDR refresh control
	writel(timing_set->t_mmc_ddr_timming0, P_MMC_DDR_TIMING0);
	writel(timing_set->t_mmc_ddr_timming1, P_MMC_DDR_TIMING1);
	writel(timing_set->t_mmc_ddr_timming2, P_MMC_DDR_TIMING2);
	writel(timing_set->t_mmc_arefr_ctrl, P_MMC_AREFR_CTRL);

	//Fix retina mid graphic flicker issue
	writel(0, P_MMC_PARB_CTRL);
}

#ifdef DDR_SCRAMBE_ENABLE
void ddr_scramble(void){
	unsigned int dmc_sec_ctrl_value;
	unsigned int ddr_key;

	ddr_key = readl(P_RAND64_ADDR0);
	if(IS_MESON_M8M2_CPU){ //m8m2
		writel(ddr_key, DMC_SEC_KEY);
		dmc_sec_ctrl_value = 0x80000000 | (1<<0);
		writel(dmc_sec_ctrl_value, M8M2_DMC_SEC_CTRL);
		while( readl(M8M2_DMC_SEC_CTRL) & 0x80000000 ) {}
	}
	else{
		writel(ddr_key &0x0000ffff, DMC_SEC_KEY0);
		writel((ddr_key >>16)&0x0000ffff, DMC_SEC_KEY1);
		dmc_sec_ctrl_value = 0x80000000 | (1<<0);
		writel(dmc_sec_ctrl_value, M8_DMC_SEC_CTRL);
		while( readl(M8_DMC_SEC_CTRL) & 0x80000000 ) {}
	}
}
#endif

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

	/*get chip id*/
	if(IS_MESON_M8M2_CPU) //m8m2
		init_dmc_m8m2(timing_set);
	else
		init_dmc_m8(timing_set);

#ifdef DDR_SCRAMBE_ENABLE
	ddr_scramble();
#endif

	return 0;
}

void ddr_info_dump(struct ddr_set * timing_set)
{
#ifdef CONFIG_DUMP_DDR_INFO
	int nPLL = readl(AM_DDR_PLL_CNTL);
	int nDDRCLK = 2*((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
	//serial_puts("DDR clock: ");
	serial_puts(" @ ");
	serial_put_dec(nDDRCLK);
	serial_puts("MHz(");
#ifdef CONFIG_DDR_LOW_POWER
	serial_puts("LP&");
#endif
	if((timing_set->t_pctl_mcfg) & (1<<3)) //DDR0, DDR1 same setting?
		serial_puts("2T)");
	else
		serial_puts("1T)");
#endif
#if defined(DDR_SCRAMBE_ENABLE) || defined(CONFIG_DUMP_DDR_INFO)
	unsigned int dmc_sec_ctrl_value;
	if(IS_MESON_M8M2_CPU){ //m8m2
		dmc_sec_ctrl_value = readl(M8M2_DMC_SEC_CTRL);
		if(dmc_sec_ctrl_value & (1<<0)){
			serial_puts("+Scramb EN");
		}
	}
	else{
		dmc_sec_ctrl_value = readl(M8_DMC_SEC_CTRL);
		if(dmc_sec_ctrl_value & (1<<0)){
			serial_puts("+Scramb EN");
		}
	}
#endif
	serial_puts("\n");
}
