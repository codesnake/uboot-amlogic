void load_nop(void)
{
	writel((1 << 31) |
		(DDR_RANK << 20) |   //rank select
		NOP_CMD
		, P_UPCTL_MCMD_ADDR);
	__udelay(1000);
	while ( readl(P_UPCTL_MCMD_ADDR) & 0x80000000 ) {}
}
void load_prea(void)
{
	writel((1 << 31) |
		(DDR_RANK << 20) |   //rank select
		PREA_CMD
		, P_UPCTL_MCMD_ADDR);
	while ( readl(P_UPCTL_MCMD_ADDR) & 0x80000000 ) {}
}

void load_mrs(int mrs_num, int mrs_value)
{
	writel((1 << 31) | 
		(DDR_RANK << 20) |   //rank select
		(mrs_num << 17) |
		(mrs_value << 4) |
		MRS_CMD
		, P_UPCTL_MCMD_ADDR);
	__udelay(1000);
	while ( readl(P_UPCTL_MCMD_ADDR) & 0x80000000 ) {};
}

void load_ref(void )
{
	writel((1 << 31) | 
		(DDR_RANK << 20) |   //rank select
		REF_CMD
		, P_UPCTL_MCMD_ADDR);
	while ( readl(P_UPCTL_MCMD_ADDR) & 0x80000000 ) {}
}

void load_zqcl(int zqcl_value )
{
	writel((1 << 31) | 
		(DDR_RANK << 20) |   //rank select
		(zqcl_value << 4 ) |
		ZQ_LONG_CMD
		, P_UPCTL_MCMD_ADDR);
	while ( readl(P_UPCTL_MCMD_ADDR) & 0x80000000 ) {};
  
}

unsigned get_mrs0(struct ddr_set * timing_reg)
{    
    //only calculate CL (6:4) and WR (11:9)
    //all others load from timming.c          
    unsigned short nMask = ~((7<<4)|(7<<9));
    unsigned short nMR0 = timing_reg->mrs[0] & nMask;    
    
    //CL
    nMR0 |=  ((timing_reg->cl-4)&0x7)<<4;
    
    //WR
    if(timing_reg->t_wr <= 8)
        nMR0|=((timing_reg->t_wr-4)&0x7)<<9;
    else
    	nMR0|=((timing_reg->t_wr>>1)&7)<<9;
    	
    return nMR0;      
}

unsigned get_mrs1(struct ddr_set * timing_reg)
{   
    //only calculate AL (4:3)
    //all others load from timming.c          
    unsigned short nMask = ~((3<<3));
    unsigned short nMR1 = timing_reg->mrs[1] & nMask;
    
    
    //AL
    if(timing_reg->t_al)
    {
        nMR1|=((timing_reg->cl-timing_reg->t_al)&3)<<3;
    }
        	
    return nMR1;
}
unsigned get_mrs2(struct ddr_set * timing_reg)
{   
    //only calculate CWL (5:3)
    //all others load from timming.c          
    unsigned short nMask = ~((7<<3));  
    unsigned short nMR2 = timing_reg->mrs[2] & nMask;
    
    
    //CWL
    nMR2 |= (((timing_reg->t_cwl-5)&7)<<3);// | (1 << 6 );
            	
    return nMR2;
}

#ifdef CONFIG_CMD_DDR_TEST
static unsigned short zqcr = 0;
#endif
int init_pctl_ddr3(struct ddr_set * timing_reg)
{	
	int nTempVal;
	int ret = 0;

	//UPCTL memory timing registers
	writel(timing_reg->t_1us_pck, P_UPCTL_TOGCNT1U_ADDR);	 //1us = nn cycles.

	writel(timing_reg->t_100ns_pck, P_UPCTL_TOGCNT100N_ADDR);//100ns = nn cycles.

	writel(timing_reg->t_init_us, P_UPCTL_TINIT_ADDR);  //200us.

	writel(timing_reg->t_rsth_us, P_UPCTL_TRSTH_ADDR);  // 0 for ddr2;  2 for simulation; 500 for ddr3.
	writel(timing_reg->t_rstl_us, P_UPCTL_TRSTL_ADDR);
	
	nTempVal = timing_reg->t_faw / timing_reg->t_rrd;
	if(nTempVal<4) nTempVal = 4;
	if(nTempVal>6) nTempVal = 6;
	//nTempVal -= (nTemp >= 4 ? 4: nTemp);
	//MMC_Wr(UPCTL_MCFG_ADDR,(nTempVal <<18)|(timing_reg->mcfg & (~(3<<18))));
	writel(((nTempVal-4) <<18)|  // 0:tFAW=4*tRRD 1:tFAW=5*tRRD 2:tFAW=6*tRRD
		(timing_reg->mcfg & (~(3<<18)))
		, P_UPCTL_MCFG_ADDR);
	  
	#ifndef CONFIG_TURN_OFF_ODT
    writel(0x8,0xc8000244);
	#endif
	  
	//configure DDR PHY PUBL registers.
	//  2:0   011: DDR3 mode.	 100:	LPDDR2 mode.
	//  3:    8 bank. 
	writel(0x3 | (1 << 3), P_PUB_DCR_ADDR);
	writel(0x01842e04, P_PUB_PGCR_ADDR); //PUB_PGCR_ADDR: c8001008

	// program PUB MRx registers.	
	writel(get_mrs0(timing_reg), P_PUB_MR0_ADDR);
	writel(get_mrs1(timing_reg), P_PUB_MR1_ADDR);
	writel(get_mrs2(timing_reg), P_PUB_MR2_ADDR);
	writel(0x0, P_PUB_MR3_ADDR);	

	//program DDR SDRAM timing parameter.
	writel((0x0 |		//tMRD.
		(timing_reg->t_rtp <<2) |		//tRTP
		( timing_reg->t_wtr << 5) |		//tWTR
		(timing_reg->t_rp << 8) |		//tRP
		(timing_reg->t_rcd << 12) |		//tRCD
		(timing_reg->t_ras <<16) |		//tRAS
		( timing_reg->t_rrd <<21 ) |		//tRRD
		(timing_reg->t_rc <<25) |		//tRC
		( 0 <<31) )
		, P_PUB_DTPR0_ADDR);	//tCCD

	writel(((timing_reg->t_faw << 3) |   //tFAW
		//(timing_reg->t_mod << 9) |   //tMOD
		(0 << 11) |   //tRTODT
		( timing_reg->t_rfc << 16) |   //tRFC
		( 0 << 24) |   //tDQSCK
		( 0 << 27) )
		, P_PUB_DTPR1_ADDR); //tDQSCKmax

	writel(( 512 |		 //tXS
		( timing_reg->t_xp << 10) |		 //tXP
		( timing_reg->t_cke << 15) |		 //tCKE
		( 512 << 19) )
		, P_PUB_DTPR2_ADDR);	 //tDLLK

	// initialization PHY.
	writel(( 27 |	  //tDLL_SRST 
		(2750 << 6) |	  //tDLLLOCK 
		(8 <<18))
		, P_PUB_PTR0_ADDR);	   //tITMSRST 

	#ifndef CONFIG_TURN_OFF_ODT
	writel(0x17, P_PUB_PIR_ADDR);//INIT,DLLSRST,DLLLOCK,ITMSRST
	#endif
	__udelay(10);	

	//wait PHY DLL LOCK
	while(!(readl(P_PUB_PGSR_ADDR) & 1)) {}

	// configure DDR3_rst pin.
	writel(readl(P_PUB_ACIOCR_ADDR) & 0xdfffffff, P_PUB_ACIOCR_ADDR);
	writel(readl(P_PUB_DSGCR_ADDR) & 0xffffffef, P_PUB_DSGCR_ADDR); 

    if(timing_reg->ddr_ctrl & (1<<7)){
        writel(readl(P_PUB_DX2GCR_ADDR) & 0xfffffffe, P_PUB_DX2GCR_ADDR);
        writel(readl(P_PUB_DX3GCR_ADDR) & 0xfffffffe, P_PUB_DX3GCR_ADDR);
    }
	writel(0x191,P_PUB_DXCCR_ADDR);//DQS
#ifdef CONFIG_CMD_DDR_TEST
    if(zqcr)
	#ifdef CONFIG_TURN_OFF_ODT
		writel(zqcr | (1<<28), P_PUB_ZQ0CR0_ADDR);
	#else
	    writel(zqcr, P_PUB_ZQ0CR1_ADDR);
	#endif
	else
#endif
#ifdef CONFIG_TURN_OFF_ODT
    writel(0x108 | (1<<28), P_PUB_ZQ0CR0_ADDR);
#else
    writel(timing_reg->zq0cr1, P_PUB_ZQ0CR1_ADDR);	//get from __ddr_setting
#endif

	//for simulation to reduce the init time.
	//MMC_Wr(PUB_PTR1_ADDR,  (20000 | 		  // Tdinit0   DDR3 : 500us.  LPDDR2 : 200us.
	//					  (192 << 19)));	  //tdinit1    DDR3 : tRFC + 10ns. LPDDR2 : 100ns.
	//MMC_Wr(PUB_PTR2_ADDR,  (10000 | 		  //tdinit2    DDR3 : 200us for power up. LPDDR2 : 11us.  
	//					(40 << 17)));		  //tdinit3    LPDDR2 : 1us. 

	#ifndef CONFIG_TURN_OFF_ODT
	writel(0x9, P_PUB_PIR_ADDR); ////INIT,ZCAL ??
	#endif
    __udelay(10);
	//wait DDR3_ZQ_DONE: 
	#ifndef CONFIG_TURN_OFF_ODT
	while(!(readl(P_PUB_PGSR_ADDR) & (1<< 2))) {}
	#endif
	// wait DDR3_PHY_INIT_WAIT : 
	#ifndef CONFIG_TURN_OFF_ODT
	while (!(readl(P_PUB_PGSR_ADDR) & 1 )) {}
	#endif
	// Monitor DFI initialization status.
	#ifndef CONFIG_TURN_OFF_ODT
	while(!(readl(P_UPCTL_DFISTSTAT0_ADDR) & 1)) {} 
	#endif
	writel(1, P_UPCTL_POWCTL_ADDR);
	while(!(readl(P_UPCTL_POWSTAT_ADDR) & 1) ) {}




	// initial upctl ddr timing.
	writel(timing_reg->t_refi_100ns, P_UPCTL_TREFI_ADDR);  // 7800ns to one refresh command.
	// wr_reg UPCTL_TREFI_ADDR, 78

	writel(timing_reg->t_mrd, P_UPCTL_TMRD_ADDR);
	//wr_reg UPCTL_TMRD_ADDR, 4

	writel(timing_reg->t_rfc, P_UPCTL_TRFC_ADDR);	//64: 400Mhz.  86: 533Mhz. 
	// wr_reg UPCTL_TRFC_ADDR, 86

	writel(timing_reg->t_rp, P_UPCTL_TRP_ADDR);	// 8 : 533Mhz.
	//wr_reg UPCTL_TRP_ADDR, 8

	writel(timing_reg->t_al, P_UPCTL_TAL_ADDR);
	//wr_reg UPCTL_TAL_ADDR, 0

	writel(timing_reg->t_cwl, P_UPCTL_TCWL_ADDR);
	//wr_reg UPCTL_TCWL_ADDR, 6

	writel(timing_reg->cl, P_UPCTL_TCL_ADDR);	 //6: 400Mhz. 8 : 533Mhz.
	// wr_reg UPCTL_TCL_ADDR, 8

	writel(timing_reg->t_ras, P_UPCTL_TRAS_ADDR); //16: 400Mhz. 20: 533Mhz.
	//  wr_reg UPCTL_TRAS_ADDR, 20 

	writel(timing_reg->t_rc, P_UPCTL_TRC_ADDR);  //24 400Mhz. 28 : 533Mhz.
	//wr_reg UPCTL_TRC_ADDR, 28

	writel(timing_reg->t_rcd, P_UPCTL_TRCD_ADDR);	//6: 400Mhz. 8: 533Mhz.
	// wr_reg UPCTL_TRCD_ADDR, 8

	writel(timing_reg->t_rrd, P_UPCTL_TRRD_ADDR); //5: 400Mhz. 6: 533Mhz.
	//wr_reg UPCTL_TRRD_ADDR, 6

	writel(timing_reg->t_rtp, P_UPCTL_TRTP_ADDR);
	// wr_reg UPCTL_TRTP_ADDR, 4

	writel(timing_reg->t_wr, P_UPCTL_TWR_ADDR);
	// wr_reg UPCTL_TWR_ADDR, 8

	writel(timing_reg->t_wtr, P_UPCTL_TWTR_ADDR);
	//wr_reg UPCTL_TWTR_ADDR, 4


	writel(timing_reg->t_exsr, P_UPCTL_TEXSR_ADDR);
	//wr_reg UPCTL_TEXSR_ADDR, 200

	writel(timing_reg->t_xp, P_UPCTL_TXP_ADDR);
	//wr_reg UPCTL_TXP_ADDR, 4

	writel(timing_reg->t_dqs, P_UPCTL_TDQS_ADDR);
	// wr_reg UPCTL_TDQS_ADDR, 2

	writel(timing_reg->t_rtw, P_UPCTL_TRTW_ADDR);
	//wr_reg UPCTL_TRTW_ADDR, 2

	writel(timing_reg->t_cksre, P_UPCTL_TCKSRE_ADDR);
	//wr_reg UPCTL_TCKSRE_ADDR, 5 

	writel(timing_reg->t_cksrx, P_UPCTL_TCKSRX_ADDR);
	//wr_reg UPCTL_TCKSRX_ADDR, 5 

	//writel(timing_reg->t_mod, P_UPCTL_TMOD_ADDR);
	writel(12, P_UPCTL_TMOD_ADDR);

	//wr_reg UPCTL_TMOD_ADDR, 8

	writel(timing_reg->t_cke, P_UPCTL_TCKE_ADDR);
	//wr_reg UPCTL_TCKE_ADDR, 4 

	writel(64, P_UPCTL_TZQCS_ADDR);
	//wr_reg UPCTL_TZQCS_ADDR , 64 

	writel(timing_reg->t_zqcl, P_UPCTL_TZQCL_ADDR);
	//wr_reg UPCTL_TZQCL_ADDR , 512 
	
	writel(10, P_UPCTL_TXPDLL_ADDR);
	// wr_reg UPCTL_TXPDLL_ADDR, 10  

	writel(1000, P_UPCTL_TZQCSI_ADDR);
	// wr_reg UPCTL_TZQCSI_ADDR, 1000  

	writel(0xf00, P_UPCTL_SCFG_ADDR);
	// wr_reg UPCTL_SCFG_ADDR, 0xf00 

	writel(1, P_UPCTL_SCTL_ADDR);
	while (!(readl(P_UPCTL_STAT_ADDR) & 1))  {}

	//config the DFI interface.
	writel((0xf0 << 1), P_UPCTL_PPCFG_ADDR);
	writel(2, P_UPCTL_DFITCTRLDELAY_ADDR);
	writel(0x1, P_UPCTL_DFITPHYWRDATA_ADDR);
	writel(timing_reg->t_cwl -1, P_UPCTL_DFITPHYWRLAT_ADDR);    //CWL -1
	writel(timing_reg->cl - 2, P_UPCTL_DFITRDDATAEN_ADDR);    //CL -2
	writel(15, P_UPCTL_DFITPHYRDLAT_ADDR);
	writel(2, P_UPCTL_DFITDRAMCLKDIS_ADDR);
	writel(2, P_UPCTL_DFITDRAMCLKEN_ADDR);
	writel(0x4, P_UPCTL_DFISTCFG0_ADDR);
	writel(0x4000, P_UPCTL_DFITCTRLUPDMIN_ADDR);
	writel(( 1 | (7 << 4) | (1 << 8) | (10 << 12) | (12 <<16) | (1 <<24) | ( 7 << 28))
		, P_UPCTL_DFILPCFG0_ADDR);

	writel(1, P_UPCTL_CMDTSTATEN_ADDR);
	while (!(readl(P_UPCTL_CMDTSTAT_ADDR) & 1 )) {}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//following code is for DDR training, Please DO NOT try to modify!!!
#if !defined(CONFIG_M6_DDR_DTAR_ADDR)
	#define CONFIG_M6_DDR_DTAR_ADDR (0x9fffff00)
#else
	#if (CONFIG_M6_DDR_DTAR_ADDR < PHYS_MEMORY_START) || \
		(CONFIG_M6_DDR_DTAR_ADDR >= (PHYS_MEMORY_START+PHYS_MEMORY_SIZE))
		#error "ERROR! CONFIG_M6_DDR_DTAR_ADDR is out of DDR space range!"
	#elif (CONFIG_M6_DDR_DTAR_ADDR & 0x3F)
		#error "ERROR! CONFIG_M6_DDR_DTAR_ADDR is not 64 alignment!"
	#elif (((CONFIG_M6_DDR_DTAR_ADDR >> 10) & 0x7) != 7) && \
		(((CONFIG_M6_DDR_DTAR_ADDR >> 10) & 0x7) != 0)
		#error "ERROR! CONFIG_M6_DDR_DTAR_ADDR bit 12,11,10 must be identical!"
	#elif (CONFIG_M6_DDR_DTAR_ADDR != (0x9fffff00))
		#warning "CONFIG_M6_DDR_DTAR_ADDR is not M6 default value!"
	#endif
#endif

	#define M6_DDR_ROW_BITS (ddr3_row_size ? (ddr3_row_size+12) : 16)
	#define M6_DDR_COL_BITS (ddr3_col_size+8)
	 
	//16bit,32bit
#if defined(M6_DDR3_DATABUS_16MODE)
	#define M6_DDR_BIT_WIDTH (16)
#else
	#define M6_DDR_BIT_WIDTH (32)
#endif
	#define M6_DDR_ADDR_LSB_WIDTH    (M6_DDR_BIT_WIDTH/16) 
	#define M6_DDR_ADDR_LOW_COL_LEN  (M6_DDR_ADDR_LSB_WIDTH==1 ? 9 : 8)
	#define M6_DDR_ADDR_LOW_COL_MASK ((1<<M6_DDR_ADDR_LOW_COL_LEN)-1)
	//16bit : The ARM/system linear address = {bank_addr[2], row_addr[ROW_BITS-1:0], col_addr[COL_BITS-1:9], bank_addr[1:0], col_addr[8:0], 1¡¯b0};
	//32bit : The ARM/system linear address = {bank_addr[2], row_addr[ROW_BITS-1:0], col_addr[COL_BITS-1:8], bank_addr[1:0], col_addr[7:0], 2¡¯b00}; 
	#define M6_DTAR_DTBANK  (((CONFIG_M6_DDR_DTAR_ADDR >> 10) & 0x3) | \
	(((CONFIG_M6_DDR_DTAR_ADDR >> (M6_DDR_ROW_BITS+M6_DDR_COL_BITS+2+M6_DDR_ADDR_LSB_WIDTH)) & 1) <<2))
	#define M6_DTAR_DTROW   ((CONFIG_M6_DDR_DTAR_ADDR >> (M6_DDR_COL_BITS+2+M6_DDR_ADDR_LSB_WIDTH)) & ((1<<M6_DDR_ROW_BITS) - 1)) 
	#define M6_DTAR_DTCOL   (((CONFIG_M6_DDR_DTAR_ADDR >> M6_DDR_ADDR_LSB_WIDTH ) & M6_DDR_ADDR_LOW_COL_MASK)| \
	((CONFIG_M6_DDR_DTAR_ADDR >> 12) & ((1<<M6_DDR_ADDR_LSB_WIDTH) - 1)) << M6_DDR_ADDR_LOW_COL_LEN)
	 
	//writel((0x0 | (0x0 <<12) | (7 << 28)), P_PUB_DTAR_ADDR); 
#ifdef CONFIG_ACS
	writel(__ddr_setting.pub_dtar, P_PUB_DTAR_ADDR);
#else
	writel((M6_DTAR_DTCOL | (M6_DTAR_DTROW <<12) | (M6_DTAR_DTBANK << 28)), P_PUB_DTAR_ADDR); 
#endif
	//writel(0x0 | (0x0 <<12) | (7 << 28), P_PUB_DTAR_ADDR); //0xa0001800@1GB,32bit, not calulate but find, why?
	//writel(0x0 | (0x0 <<12) | (7 << 28), P_PUB_DTAR_ADDR); //0x90001800@512MB,32bit,not calulate but find, why?
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// DDR PHY initialization 
	//MMC_Wr( PUB_PIR_ADDR, 0x1e1);
	#ifdef CONFIG_TURN_OFF_ODT
	writel(0x1e1, P_PUB_PIR_ADDR);
	#else
	writel(0x1e9, P_PUB_PIR_ADDR);
	#endif
	//DDR3_SDRAM_INIT_WAIT : 
	while( !(readl(P_PUB_PGSR_ADDR) & 1)) {}

    ret |= readl(P_PUB_PGSR_ADDR) & (3<<5);
    
    if(ret){
        serial_puts("\nPGSR: ");
        serial_put_hex(readl(P_PUB_PGSR_ADDR), 8);

        writel(0x1e9 | (1<<28), P_PUB_PIR_ADDR);

	    while( !(readl(P_PUB_PGSR_ADDR) & 1)) {}

	    ret &= ~(3<<5);
	    ret |= readl(P_PUB_PGSR_ADDR) & (3<<5);
	    serial_puts("\nPGSR: ");
        serial_put_hex(readl(P_PUB_PGSR_ADDR), 8);
        
        serial_puts("\n");
        serial_put_hex(readl(P_PUB_DX0GSR0_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(readl(P_PUB_DX1GSR0_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(readl(P_PUB_DX2GSR0_ADDR), 32);
        serial_puts("\n");
        serial_put_hex(readl(P_PUB_DX3GSR0_ADDR), 32);
    }

	writel(2, P_UPCTL_SCTL_ADDR); // init: 0, cfg: 1, go: 2, sleep: 3, wakeup: 4

	while ((readl(P_UPCTL_STAT_ADDR) & 0x7 ) != 3 ) {}

    return ret;
	//return 0;

}
