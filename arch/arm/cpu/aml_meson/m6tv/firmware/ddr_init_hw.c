#define reg(a) (*(volatile unsigned*)(a))

static inline int dtu_enable(void)
{
    unsigned timeout = 10000;
    reg(P_UPCTL_DTUECTL_ADDR) = 0x1;  // start wr/rd
    while((reg(P_UPCTL_DTUECTL_ADDR)&1) && timeout)
        --timeout;
    return timeout;
}

static inline int start_ddr_config(void)
{
    unsigned timeout = -1;
    reg(P_UPCTL_SCTL_ADDR) = 0x1;
    while((reg(P_UPCTL_STAT_ADDR) != 0x1) && timeout)
        --timeout;

    return timeout;
}

static inline int end_ddr_config(void)
{
    unsigned timeout = 10000;
    reg(P_UPCTL_SCTL_ADDR) = 0x2;
    while((readl(P_UPCTL_STAT_ADDR) != 0x3) && timeout)
        --timeout;

    return timeout;
}

static void display_more_training_result(void)
{
    serial_puts("\nDX0GSR0: 0x");
	serial_put_hex(readl(P_PUB_DX0GSR0_ADDR),32);
	serial_puts("\nDX0GSR2: 0x");
	serial_put_hex(readl(P_PUB_DX0GSR2_ADDR),32);
	serial_puts("\n");
	serial_puts("\nDX1GSR0: 0x");
	serial_put_hex(readl(P_PUB_DX1GSR0_ADDR),32);
	serial_puts("\nDX1GSR2: 0x");
	serial_put_hex(readl(P_PUB_DX1GSR2_ADDR),32);
	serial_puts("\n");
	serial_puts("\nDX2GSR0: 0x");
	serial_put_hex(readl(P_PUB_DX2GSR0_ADDR),32);
	serial_puts("\nDX2GSR2: 0x");
	serial_put_hex(readl(P_PUB_DX2GSR2_ADDR),32);
	serial_puts("\n");
	serial_puts("\nDX3GSR0: 0x");
	serial_put_hex(readl(P_PUB_DX3GSR0_ADDR),32);
	serial_puts("\nDX3GSR2: 0x");
	serial_put_hex(readl(P_PUB_DX3GSR2_ADDR),32);
	serial_puts("\n");
}

static void dtu_test_for_debug_training_result(void)
{
    int i;
	//debug 11.20
	/*
	reg(P_UPCTL_DTUWD0_ADDR) = 0xdd22ee11;
	reg(P_UPCTL_DTUWD1_ADDR) = 0x7788bb44;
	reg(P_UPCTL_DTUWD2_ADDR) = 0xdd22ee11;
	reg(P_UPCTL_DTUWD3_ADDR) = 0x7788bb44;
	*/
    reg(P_UPCTL_DTUWD0_ADDR) = 0x55AA55AA;
    reg(P_UPCTL_DTUWD1_ADDR) = 0xAA55AA55;
    reg(P_UPCTL_DTUWD2_ADDR) = 0xdd22ee11;
    reg(P_UPCTL_DTUWD3_ADDR) = 0x7788bb44;
	//debug 11.20
	
    reg(P_UPCTL_DTUWACTL_ADDR) = 0;
    reg(P_UPCTL_DTURACTL_ADDR) = 0;
    for(i = 0; i < 4; i++)
    {
    	if(i%2)
    	{
	    	reg(P_UPCTL_DTUWD0_ADDR) = 0xaa55aa55;
	    	reg(P_UPCTL_DTUWD1_ADDR) = 0x33445566;
		    reg(P_UPCTL_DTUWD2_ADDR) = 0x77889900;
	    	reg(P_UPCTL_DTUWD3_ADDR) = 0x11223344;
    	}
		else
		{
			reg(P_UPCTL_DTUWD0_ADDR) = 0x55AA55AA;
		    reg(P_UPCTL_DTUWD1_ADDR) = 0xAA55AA55;
		    reg(P_UPCTL_DTUWD2_ADDR) = 0xdd22ee11;
		    reg(P_UPCTL_DTUWD3_ADDR) = 0x7788bb44;
		}
		
        serial_puts("\n\n");
        serial_put_hex(i, 8);
        serial_puts(" byte lane:\n");
        start_ddr_config();
        reg(P_UPCTL_DTUCFG_ADDR) = (i << 10) | 1;
        end_ddr_config();

        dtu_enable();
        serial_puts("\n");
        serial_put_hex(reg(P_UPCTL_DTURD0_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(reg(P_UPCTL_DTURD1_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(reg(P_UPCTL_DTURD2_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(reg(P_UPCTL_DTURD3_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(reg(P_UPCTL_DTUPDES_ADDR), 32);

    }
}

void init_dmc(struct ddr_set * ddr_setting)
{	
	writel(ddr_setting->ddr_ctrl, P_MMC_DDR_CTRL);
	writel(0xffff, P_DMC_SEC_PORT0_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT1_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT2_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT3_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT4_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT5_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT6_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT7_RANGE0);
    writel(0x80000000, P_DMC_SEC_CTRL);
	while( readl(P_DMC_SEC_CTRL) & 0x80000000 ) {}


#ifdef CONFIG_DDR_LOW_POWER	
	writel( (0x4c )|
	(0xf5 << 8) |
	(0xf5 <<16) |
	( 0 << 30)  |
	( 0 << 31), P_MMC_LP_CTRL4 );   //tREFI        
	writel( (0xf  <<  0) | //t_PWD_WAIT  wait for Power down pipeline finish, 
	(0xf  <<  8) | //t_SELF_WAIT.  wait for self refresh pipeline finish.               
	(0x2f << 16) | // APB command hold time.                
	(0x28 << 24)   //t100ns                
	,P_MMC_LP_CTRL3 );	
	writel( (0x20 << 0 )|  //tRFC, disable ddr command after auto refresh command issued.                 
	(0x100 << 8 ) |  //idle cycle numbers for MMC enter self refresh mode                
	(1 << 28 )    |  //enable MMC auto power down mode.               
	(1 << 29 )    |  //enable hardwire wakeup enable. use c_active_in for ddr wakeup.                
	(1 << 30 )       //enable MMC auto self refresh mode.                 
	,P_MMC_LP_CTRL1 );
#endif

	writel((readl(P_MMC_CHAN3_CTRL) & (~0x3ff)) | (0x3f), P_MMC_CHAN3_CTRL);

	writel(0x1ff, P_MMC_REQ_CTRL);

	//asm volatile ("wfi");
	
	//re read write DDR SDRAM several times to make sure the AXI2DDR bugs dispear.
	//refer from arch\arm\cpu\aml_meson\m6\firmware\kreboot.s	
	int nCnt,nMax,nVal;
	for(nCnt=0,nMax= 9;nCnt<nMax;++nCnt)
	{
		//asm volatile ("LDR  r0, =0x55555555");
		//asm volatile ("LDR  r1, =0x9fffff00");
		//asm volatile ("STR  r0, [r1]");
		writel(0x55555555, 0x9fffff00);
	}		

	//asm volatile ("wfi");
	
	for(nCnt=0,nMax= 12;nCnt<nMax;++nCnt)
	{
		//asm volatile ("LDR  r1, =0x9fffff00");
		//asm volatile ("LDR  r0, [r1]");		
		nVal = readl(0x9fffff00);
	}	
	asm volatile ("dmb"); //debug 11.20
	asm volatile ("isb");	 //debug 11.20
	//
	//asm volatile ("wfi");	

}
int ddr_init_hw(struct ddr_set * timing_reg)
{
    int ret = 0;
    
    ret = timing_reg->init_pctl(timing_reg);
	
	if(ret)
    {
        display_more_training_result();
        dtu_test_for_debug_training_result();
        __udelay(10);        
		serial_puts("\nPUB init fail! Reset...\n");
		__udelay(10000);
		AML_WATCH_DOG_START();
        return ret;
    }    

	//asm volatile("wfi");

    init_dmc(timing_reg);


#ifdef CONFIG_M6TV_DUMP_DDR_INFO
		int nPLL = readl(P_HHI_DDR_PLL_CNTL);
		int nDDRCLK = 2*((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
		serial_puts("\nDDR clock is ");
		serial_put_dec(nDDRCLK);
		serial_puts("MHz with ");
	#ifdef CONFIG_DDR_LOW_POWER
		serial_puts("Low Power & ");
	#endif
		if(readl(P_UPCTL_MCFG_ADDR)& (1<<3))
			serial_puts("2T mode\n");
		else
			serial_puts("1T mode\n");
#endif

	
    return 0;
}
