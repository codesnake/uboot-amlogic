#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/uart.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/pctl.h>
#include <asm/arch/dmc.h>
#include <asm/arch/ddr.h>
#include <asm/arch/memtest.h>
#include <asm/arch/pctl.h>

extern void delay_ms(int ms);

#define dbg_out(s,v) serial_puts(s);serial_put_hex(v,32);serial_putc('\n');
#define dbg_puts serial_puts

void ts_delay(int n)
{	
    int i;
		for(i = 0; i < n; i++){
			delay_tick(100);
		}
}

void disable_mmc_req(void)
{
	APB_Wr(MMC_REQ_CTRL,0X0);
  	while(APB_Rd(MMC_CHAN_STS) == 0){
		ts_delay(1);	
	}
}

void reset_mmc(void)
{
	unsigned ustate;
	writel(1<<3, P_RESET1_REGISTER);
	delay_ms(10);
 	ustate = readl(P_MMC_CHAN_RST);
	writel(0x17ff,P_MMC_CHAN_RST);
	while((ustate = readl(P_MMC_CHAN_RST)) != 0){
		writel(ustate,P_MMC_CHAN_RST);
	}
}

void enable_mmc_req(void)
{
	// Next, we enable all requests
	APB_Wr(MMC_REQ_CTRL, 0xff);
	while(APB_Rd(MMC_CHAN_STS) == 0){
		ts_delay(1);
	}
}
void mmc_sleep(void)
{
	int stat;
	do
	{
		stat = APB_Rd(PCTL_STAT_ADDR);
		if(stat == PCTL_STAT_INIT) {
			APB_Wr(PCTL_SCTL_ADDR, SCTL_CMD_CONFIG);
		}
		else if(stat == PCTL_STAT_CONFIG) {
			APB_Wr(PCTL_SCTL_ADDR, SCTL_CMD_GO);
		}
		else if(stat == PCTL_STAT_ACCESS) {
				APB_Wr(PCTL_SCTL_ADDR, SCTL_CMD_SLEEP);
		}
	}while(stat != PCTL_STAT_LOW_POWER);
}

void mmc_wakeup(void)
{
	int stat;
	do
	{
		stat = APB_Rd(PCTL_STAT_ADDR);
		if(stat == PCTL_STAT_LOW_POWER) {
			APB_Wr(PCTL_SCTL_ADDR, SCTL_CMD_WAKEUP);
			while(stat != PCTL_STAT_LOW_POWER);
		}
		if(stat == PCTL_STAT_INIT) {
			APB_Wr(PCTL_SCTL_ADDR, SCTL_CMD_CONFIG);
			while(stat != PCTL_STAT_CONFIG);
		}
		if(stat == PCTL_STAT_CONFIG) {
			APB_Wr(PCTL_SCTL_ADDR, SCTL_CMD_GO);
			while(stat != PCTL_STAT_ACCESS);
		}
	}while(stat != PCTL_STAT_LOW_POWER);
}

#define DDR_RSLR_LEN 6
#define DDR_RDGR_LEN 4
#define PHYS_MEMORY_START 0x80000000
#define DDR3_2Gbx16
#define DDR_RANK  1   // 2'b11 means 2 rank.
#define NOP_CMD  0
#define PREA_CMD  1
#define REF_CMD  2
#define MRS_CMD  3
#define ZQ_SHORT_CMD 4
#define ZQ_LONG_CMD  5
#define SFT_RESET_CMD 6

void load_nop(void)
{
	APB_Wr(PCTL_MCMD_ADDR, (1 << 31) |
		(DDR_RANK << 20) |   //rank select
		NOP_CMD );
	while ( APB_Rd(PCTL_MCMD_ADDR) & 0x80000000 ) {}
}
void load_prea(void)
{
	APB_Wr(PCTL_MCMD_ADDR, (1 << 31) |
		(DDR_RANK << 20) |   //rank select
		PREA_CMD );
	while ( APB_Rd(PCTL_MCMD_ADDR) & 0x80000000 ) {}
}

void load_mrs(int mrs_num, int mrs_value)
{
	APB_Wr(PCTL_MCMD_ADDR, (1 << 31) | 
		(DDR_RANK << 20) |   //rank select
		(mrs_num << 17) |
		(mrs_value << 4) |
		MRS_CMD );
	while ( APB_Rd(PCTL_MCMD_ADDR) & 0x80000000 ) {};
}

void load_ref(void )
{
	APB_Wr(PCTL_MCMD_ADDR, (1 << 31) | 
		(DDR_RANK << 20) |   //rank select
		REF_CMD );
	while ( APB_Rd(PCTL_MCMD_ADDR) & 0x80000000 ) {}
}

void load_zqcl(int zqcl_value )
{
	APB_Wr(PCTL_MCMD_ADDR, (1 << 31) | 
		(DDR_RANK << 20) |   //rank select
		(zqcl_value << 4 ) |
		ZQ_LONG_CMD );
	while ( APB_Rd(PCTL_MCMD_ADDR) & 0x80000000 ) {}
}

unsigned get_mrs0()
{
    unsigned mmc_ctrl=v_mmc_ddr_ctrl;
    unsigned ret=1<<12;
    //bl 2==4 0==8
    ret|=mmc_ctrl&(1<<10)?2:0;
    //cl
    ret|=((v_t_cl-4)&0x7)<<4;
    //wr: write recovery 
    ret|=((v_t_wr-4)&0x7)<<9;
    return ret&0x1fff;
}

unsigned get_mrs1()
{
    unsigned ret=(1<<6)|(1<<2);//rtt_nominal;      //(A9, A6, A2) 000 : disabled. 001 : RZQ/4   (A6:A2)
    //cl
    if(v_t_al)
    {
        ret|=((v_t_cl-v_t_al)&3)<<3;
    }
    return ret&0x1fff;
}
unsigned get_mrs2()
{
    unsigned ret=((v_t_cwl-5)&7)<<3;
    return ret&0x1fff;
}

void init_dmc (void)
{
    APB_Wr(MMC_DDR_CTRL, v_mmc_ddr_ctrl);
    APB_Wr(MMC_REQ_CTRL, 0xff ); 
	ts_delay(8);
}
void disp_pctl(void)
{
	dbg_out("HHI_DDR_PLL_CNTL:",readl(P_HHI_DDR_PLL_CNTL));
	dbg_out("PCTL_TOGCNT1U_ADDR:",APB_Rd(PCTL_TOGCNT1U_ADDR));
	dbg_out("PCTL_TOGCNT100N_ADDR:",APB_Rd(PCTL_TOGCNT100N_ADDR));
	dbg_out("PCTL_TINIT_ADDR:",APB_Rd(PCTL_TINIT_ADDR));
	dbg_out("MMC_PHY_CTRL:",readl(P_MMC_PHY_CTRL));
	dbg_out("MMC_DDR_CTRL:",readl(P_MMC_DDR_CTRL));

	dbg_puts("***\n");
	dbg_out("PCTL_IOCR_ADDR:",APB_Rd(PCTL_IOCR_ADDR));
	dbg_out("PCTL_TRSTH_ADDR:",APB_Rd(PCTL_TRSTH_ADDR));
	dbg_out("PCTL_TSRTL_ADDR:",APB_Rd(PCTL_TSRTL_ADDR));
	dbg_puts("***\n");

	dbg_out("PCTL_POWSTAT_ADDR:",APB_Rd(PCTL_POWSTAT_ADDR));
	dbg_out("PCTL_POWCTL_ADDR:",APB_Rd(PCTL_POWCTL_ADDR));
	dbg_out("PCTL_ODTCFG_ADDR:",APB_Rd(PCTL_ODTCFG_ADDR));
	dbg_out("PCTL_ZQCR_ADDR:",APB_Rd(PCTL_ZQCR_ADDR));

	dbg_puts("***\n");
	dbg_out("PCTL_TREFI_ADDR:",APB_Rd(PCTL_TREFI_ADDR));
	dbg_out("PCTL_TMRD_ADDR:",APB_Rd(PCTL_TMRD_ADDR));
	dbg_out("PCTL_TRFC_ADDR:",APB_Rd(PCTL_TRFC_ADDR));
	dbg_out("PCTL_TRP_ADDR:",APB_Rd(PCTL_TRP_ADDR));
	dbg_out("PCTL_TAL_ADDR:",APB_Rd(PCTL_TAL_ADDR));
	dbg_out("PCTL_TCWL_ADDR:",APB_Rd(PCTL_TCWL_ADDR));
	dbg_out("PCTL_TCL_ADDR:",APB_Rd(PCTL_TCL_ADDR));
	dbg_out("PCTL_TRAS_ADDR:",APB_Rd(PCTL_TRAS_ADDR));
	dbg_out("PCTL_TRC_ADDR:",APB_Rd(PCTL_TRC_ADDR));
	dbg_out("PCTL_TRCD_ADDR:",APB_Rd(PCTL_TRCD_ADDR));
	dbg_out("PCTL_TRRD_ADDR:",APB_Rd(PCTL_TRRD_ADDR));
	dbg_out("PCTL_TRTP_ADDR:",APB_Rd(PCTL_TRTP_ADDR));
	dbg_out("PCTL_TWR_ADDR:",APB_Rd(PCTL_TWR_ADDR));
	dbg_out("PCTL_TWTR_ADDR:",APB_Rd(PCTL_TWTR_ADDR));
	dbg_out("PCTL_TEXSR_ADDR:",APB_Rd(PCTL_TEXSR_ADDR));
	dbg_out("PCTL_TXP_ADDR:",APB_Rd(PCTL_TXP_ADDR));
	dbg_out("PCTL_TDQS_ADDR:",APB_Rd(PCTL_TDQS_ADDR));
	dbg_out("PCTL_TMOD_ADDR:",APB_Rd(PCTL_TMOD_ADDR));
	dbg_out("PCTL_TZQCL_ADDR:",APB_Rd(PCTL_TZQCL_ADDR));
	dbg_out("PCTL_TZQCSI_ADDR:",APB_Rd(PCTL_TZQCSI_ADDR));
	dbg_out("PCTL_TCKSRX_ADDR:",APB_Rd(PCTL_TCKSRX_ADDR));
	dbg_out("PCTL_TCKSRE_ADDR:",APB_Rd(PCTL_TCKSRE_ADDR));
	dbg_out("PCTL_TCKE_ADDR:",APB_Rd(PCTL_TCKE_ADDR));
	dbg_puts("***\n");
	dbg_out("PCTL_MCFG_ADDR:",APB_Rd(PCTL_MCFG_ADDR));
	dbg_out("PCTL_PHYCR_ADDR:",APB_Rd(PCTL_PHYCR_ADDR));

	dbg_out("rdgr0=",APB_Rd(PCTL_RDGR0_ADDR));
	dbg_out("rslr0=",APB_Rd(PCTL_RSLR0_ADDR));
	
	dbg_out("sys pll cntl=",readl(P_HHI_SYS_PLL_CNTL));
	dbg_out("other pll cntl=",readl(P_HHI_OTHER_PLL_CNTL));
	dbg_out("sys cpu clk cntl=",readl(P_HHI_SYS_CPU_CLK_CNTL));
	dbg_out("mpeg clk cntl=",readl(P_HHI_MPEG_CLK_CNTL));

}

unsigned ddr_settings[DDR_SETTING_COUNT];

void save_ddr_settings()
{
	v_ddr_pll_cntl = readl(P_HHI_DDR_PLL_CNTL);
	v_mmc_ddr_ctrl = readl(P_MMC_DDR_CTRL);
	v_mmc_phy_ctrl = readl(P_MMC_PHY_CTRL);
	
	v_t_1us_pck   = APB_Rd(PCTL_TOGCNT1U_ADDR);
	v_t_100ns_pck = APB_Rd(PCTL_TOGCNT100N_ADDR);
	v_t_init_us   = APB_Rd(PCTL_TINIT_ADDR);

	v_iocr   = APB_Rd(PCTL_IOCR_ADDR);
	v_t_rsth = APB_Rd(PCTL_TRSTH_ADDR);
	v_t_srtl = APB_Rd(PCTL_TSRTL_ADDR);

	v_powstat = APB_Rd(PCTL_POWSTAT_ADDR);
	v_powctl  = APB_Rd(PCTL_POWCTL_ADDR);
	v_odtcfg  = APB_Rd(PCTL_ODTCFG_ADDR);
	v_zqcr    = APB_Rd(PCTL_ZQCR_ADDR);

	v_t_refi = APB_Rd(PCTL_TREFI_ADDR);
	v_t_mrd  = APB_Rd(PCTL_TMRD_ADDR);
	v_t_rfc  = APB_Rd(PCTL_TRFC_ADDR);
	v_t_rp   = APB_Rd(PCTL_TRP_ADDR);
	v_t_al   = APB_Rd(PCTL_TAL_ADDR);
	v_t_cwl  = APB_Rd(PCTL_TCWL_ADDR);
	v_t_cl   = APB_Rd(PCTL_TCL_ADDR);
	v_t_ras  = APB_Rd(PCTL_TRAS_ADDR);
	v_t_rc   = APB_Rd(PCTL_TRC_ADDR);
	v_t_rcd  = APB_Rd(PCTL_TRCD_ADDR);
	v_t_rrd  = APB_Rd(PCTL_TRRD_ADDR);
	v_t_rtp  = APB_Rd(PCTL_TRTP_ADDR);
	v_t_wr   = APB_Rd(PCTL_TWR_ADDR);
	v_t_wtr  = APB_Rd(PCTL_TWTR_ADDR);
	v_t_exsr = APB_Rd(PCTL_TEXSR_ADDR);
	v_t_xp   = APB_Rd(PCTL_TXP_ADDR);
	v_t_dqs  = APB_Rd(PCTL_TDQS_ADDR);
	v_t_mod  = APB_Rd(PCTL_TMOD_ADDR);
	v_t_zqcl = APB_Rd(PCTL_TZQCL_ADDR);
	v_t_zqcsi = APB_Rd(PCTL_TZQCSI_ADDR);
	v_t_cksrx = APB_Rd(PCTL_TCKSRX_ADDR);
	v_t_cksre = APB_Rd(PCTL_TCKSRE_ADDR);
	v_t_cke   = APB_Rd(PCTL_TCKE_ADDR);

	v_mcfg  = APB_Rd(PCTL_MCFG_ADDR);
	v_phycr = APB_Rd(PCTL_PHYCR_ADDR);

	v_rdgr0 = APB_Rd(PCTL_RDGR0_ADDR);
	v_rslr0 = APB_Rd(PCTL_RSLR0_ADDR);
	
	v_ddr_pll_cntl2 = readl(P_HHI_DDR_PLL_CNTL2);
	v_ddr_pll_cntl3 = readl(P_HHI_DDR_PLL_CNTL3);
	
	v_dllcr9 = APB_Rd(PCTL_DLLCR9_ADDR);

  v_dllcr0 = APB_Rd(PCTL_DLLCR0_ADDR);
  v_dllcr1 = APB_Rd(PCTL_DLLCR1_ADDR);
  v_dllcr2 = APB_Rd(PCTL_DLLCR2_ADDR);
  v_dllcr3 = APB_Rd(PCTL_DLLCR3_ADDR);
  v_dqscr = APB_Rd(PCTL_DQSTR_ADDR);
  v_dqsntr = APB_Rd(PCTL_DQSNTR_ADDR);
  v_tr0 = APB_Rd(PCTL_DQTR0_ADDR);
  v_tr1 = APB_Rd(PCTL_DQTR1_ADDR);
  v_tr2 = APB_Rd(PCTL_DQTR2_ADDR);
  v_tr3 = APB_Rd(PCTL_DQTR3_ADDR);
}

void init_ddr_pll(void)
{
 	writel(v_ddr_pll_cntl | 0x8000, P_HHI_DDR_PLL_CNTL);
 	writel(v_ddr_pll_cntl2, P_HHI_DDR_PLL_CNTL2);
 	writel(v_ddr_pll_cntl3, P_HHI_DDR_PLL_CNTL3);    
	writel(v_ddr_pll_cntl & (~0x8000), P_HHI_DDR_PLL_CNTL);
 	writel(1<<0, P_RESET5_REGISTER);
	delay_ms(50);
  
 	writel(readl(P_HHI_DDR_PLL_CNTL)&(~(1<<15)),P_HHI_DDR_PLL_CNTL);
 	delay_ms(50);
}

void init_pctl(void)
{
	//int i;
	int mrs0_value;
	int mrs1_value;
	//int mrs2_value;
	int mrs3_value = 0;
    
 	APB_Wr(MMC_DDR_CTRL, v_mmc_ddr_ctrl);
	APB_Wr(PCTL_DLLCR9_ADDR, v_dllcr9);
	APB_Wr(PCTL_TOGCNT1U_ADDR,   v_t_1us_pck); //timing_reg.t_1us_pck);
	APB_Wr(PCTL_TOGCNT100N_ADDR, v_t_100ns_pck);  //timing_reg.t_100ns_pck);
	APB_Wr(PCTL_TINIT_ADDR,      v_t_init_us);//200); //timing_reg.t_init_us);

	APB_Wr(PCTL_TRSTH_ADDR, v_t_rsth);       // 500us  to hold reset high before assert CKE. change it to 50 for fast simulation time.
	APB_Wr(PCTL_TSRTL_ADDR, v_t_srtl);        //  100 clock cycles for reset low 

	APB_Wr(MMC_PHY_CTRL,   v_mmc_phy_ctrl );  
	APB_Wr(PCTL_IOCR_ADDR, v_iocr);
	APB_Wr(PCTL_PHYCR_ADDR, v_phycr);

	while (!(APB_Rd(PCTL_POWSTAT_ADDR) & 2)) {} // wait for dll lock
	APB_Wr(PCTL_POWCTL_ADDR, 1);            // start memory power up sequence
	while (!(APB_Rd(PCTL_POWSTAT_ADDR) & 1)) {} // wait for memory power up

	APB_Wr(PCTL_MCFG_ADDR,v_mcfg);

	//configure DDR3 SDRAM parameter.
	APB_Wr(PCTL_TREFI_ADDR, v_t_refi); //timing_reg.t_refi_100ns);
	APB_Wr(PCTL_TMRD_ADDR,  v_t_mrd);  //timing_reg.t_mrd);
	APB_Wr(PCTL_TRFC_ADDR,  v_t_rfc); //86 timing_reg.t_rfc);
	APB_Wr(PCTL_TRP_ADDR,   v_t_rp);  //8timing_reg.t_rp);
	APB_Wr(PCTL_TAL_ADDR,   v_t_al);  //timing_reg.t_al);
	APB_Wr(PCTL_TCWL_ADDR,  v_t_cwl);  //timing_reg.t_cwl);
	APB_Wr(PCTL_TCL_ADDR,   v_t_cl);  //timing_reg.cl);
	APB_Wr(PCTL_TRAS_ADDR,  v_t_ras); //20timing_reg.t_ras);
	APB_Wr(PCTL_TRC_ADDR,   v_t_rc); //28timing_reg.t_rc);
	APB_Wr(PCTL_TRCD_ADDR,  v_t_rcd);  //timing_reg.t_rcd);
	APB_Wr(PCTL_TRRD_ADDR,  v_t_rrd);  //6timing_reg.t_rrd);
	APB_Wr(PCTL_TRTP_ADDR,  v_t_rtp);  //4timing_reg.t_rtp);
	APB_Wr(PCTL_TWR_ADDR,   v_t_wr);  //8timing_reg.t_wr);
	APB_Wr(PCTL_TWTR_ADDR,  v_t_wtr);  //timing_reg.t_wtr);
	APB_Wr(PCTL_TEXSR_ADDR, v_t_exsr);//timing_reg.t_exsr);
	APB_Wr(PCTL_TXP_ADDR,   v_t_xp);  //timing_reg.t_xp);
	APB_Wr(PCTL_TDQS_ADDR,  v_t_dqs);  //timing_reg.t_dqs);
	APB_Wr(PCTL_TMOD_ADDR,  v_t_mod);  //timing_reg.t_mod);
	APB_Wr(PCTL_TZQCL_ADDR, v_t_zqcl);//timing_reg.t_zqcl);
	APB_Wr(PCTL_TZQCSI_ADDR, v_t_zqcsi);//timing_reg.t_zqcsi);
	APB_Wr(PCTL_TCKSRX_ADDR, v_t_cksrx); //timing_reg.t_cksrx);
	APB_Wr(PCTL_TCKSRE_ADDR, v_t_cksre); //timing_reg.t_cksre);
	APB_Wr(PCTL_TCKE_ADDR,   v_t_cke); //timing_reg.t_cke);
	APB_Wr(PCTL_ODTCFG_ADDR, v_odtcfg);         //configure ODT
	APB_Wr(PCTL_ZQCR_ADDR, v_zqcr );

	load_nop();
	load_mrs(2, get_mrs2());
	load_mrs(3, mrs3_value);
	mrs1_value = get_mrs1() & 0xfffffffe; //dll enable 
	load_mrs(1, mrs1_value);
	mrs0_value = get_mrs0() | (1 << 8);    // dll reset.
	load_mrs(0, mrs0_value);
	load_zqcl(0);     // send ZQ calibration long command.

  APB_Wr(PCTL_RDGR0_ADDR ,v_rdgr0);
  APB_Wr(PCTL_RSLR0_ADDR ,v_rslr0);
  APB_Wr(PCTL_DLLCR0_ADDR, v_dllcr0);
  APB_Wr(PCTL_DLLCR1_ADDR, v_dllcr1);
  APB_Wr(PCTL_DLLCR2_ADDR, v_dllcr2);
  APB_Wr(PCTL_DLLCR3_ADDR, v_dllcr3);
  APB_Wr(PCTL_DQSTR_ADDR, v_dqscr);
  APB_Wr(PCTL_DQSNTR_ADDR, v_dqsntr);
  APB_Wr(PCTL_DQTR0_ADDR, v_tr0);
  APB_Wr(PCTL_DQTR1_ADDR, v_tr1);
  APB_Wr(PCTL_DQTR2_ADDR, v_tr2);
  APB_Wr(PCTL_DQTR3_ADDR, v_tr3);
}

void enable_retention(void)
{
   writel(readl(P_AO_RTI_PIN_MUX_REG)|(1<<20),P_AO_RTI_PIN_MUX_REG);
}

void disable_retention(void)
{
  writel(readl(P_AO_RTI_PIN_MUX_REG)&(~(1<<20)),P_AO_RTI_PIN_MUX_REG);
}

