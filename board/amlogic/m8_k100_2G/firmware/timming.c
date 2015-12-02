#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/timing.h>
#include <asm/arch/uart.h>
#include <ddr_types.h>


#ifndef FIRMWARE_IN_ONE_FILE
#define STATIC_PREFIX_DATA
#else
#define STATIC_PREFIX_DATA static
#endif

static int init_pctl_ddr3(struct ddr_set * timing_set);

#if (CONFIG_DDR_CLK >= 384) && (CONFIG_DDR_CLK < 750)
	#define CFG_PLL_OD 2
	#define CFG_PLL_N  1
	#define CFG_PLL_M  (((CONFIG_DDR_CLK/6)*6)/12)
#elif (CONFIG_DDR_CLK >= 750) && (CONFIG_DDR_CLK <= 912)
	#define CFG_PLL_OD 1
	#define CFG_PLL_N  1
	#define CFG_PLL_M  (((CONFIG_DDR_CLK/12)*12)/24)
#else
	#error "Over PLL range! Please check CONFIG_DDR_CLK with file m8_skt_v1.h! \n"
#endif

#if (CONFIG_DDR_CLK >= 384 ) && (CONFIG_DDR_CLK <533)
	#define DDR_7_7_7
#elif  (CONFIG_DDR_CLK >= 533 ) && (CONFIG_DDR_CLK <667)
	#define DDR_9_9_9 
#elif  (CONFIG_DDR_CLK >= 667 ) && (CONFIG_DDR_CLK <=912)
	#define DDR_11_11_11
#endif

/////////////////////////////////////////////////////////////////////////////////
//Following setting for board XXXXXXX with DDR K4B4G1646B(SANSUNG)
#ifdef DDR_7_7_7
	//DTPR0
	#define CFG_DDR_RTP (6)
	#define CFG_DDR_WTR (6)
	#define CFG_DDR_RP  (7)
	#define CFG_DDR_RCD (7)
	#define CFG_DDR_RAS (20)
	#define CFG_DDR_RRD (6)
	#define CFG_DDR_RC  (27)

	//DTPR1
	#define CFG_DDR_MRD (4)
	#define CFG_DDR_MOD (12)
	#define CFG_DDR_FAW (27)
	#define CFG_DDR_RFC (139)
	#define CFG_DDR_WLMRD (40)
	#define CFG_DDR_WLO (6)

	//DTPR2
	#define CFG_DDR_XS   (512)
	#define CFG_DDR_XP   (5)
	#define CFG_DDR_CKE  (4)
	#define CFG_DDR_DLLK (512)
	#define CFG_DDR_RTODT (0)
	#define CFG_DDR_RTW   (4)

	#define CFG_DDR_REFI  (78)
	#define CFG_DDR_REFI_MDDR3  (4)

	#define CFG_DDR_CL    (7)
	#define CFG_DDR_WR    (12)
	#define CFG_DDR_CWL   (8)
	#define CFG_DDR_AL    (0)
	#define CFG_DDR_EXSR  (512)
	#define CFG_DDR_DQS   (4)
	#define CFG_DDR_CKSRE (8)
	#define CFG_DDR_CKSRX (8)
	#define CFG_DDR_ZQCS  (64)
	#define CFG_DDR_ZQCL  (512)
	#define CFG_DDR_XPDLL (20)
	#define CFG_DDR_ZQCSI (1000)
#endif

#ifdef DDR_9_9_9
	//DTPR0
	#define CFG_DDR_RTP (6)
	#define CFG_DDR_WTR (6)
	#define CFG_DDR_RP  (9)
	#define CFG_DDR_RCD (9)
	#define CFG_DDR_RAS (24)
	#define CFG_DDR_RRD (5)
	#define CFG_DDR_RC  (33)

	//DTPR1
	#define CFG_DDR_MRD (4)
	#define CFG_DDR_MOD (12)
	#define CFG_DDR_FAW (30)
	#define CFG_DDR_RFC (174)
	#define CFG_DDR_WLMRD (40)
	#define CFG_DDR_WLO (6)

	//DTPR2
	#define CFG_DDR_XS   (512)
	#define CFG_DDR_XP   (5)
	#define CFG_DDR_CKE  (4)
	#define CFG_DDR_DLLK (512)
	#define CFG_DDR_RTODT (0)
	#define CFG_DDR_RTW   (4)

	#define CFG_DDR_REFI  (78)
	#define CFG_DDR_REFI_MDDR3  (4)
		
	#define CFG_DDR_CL    (9)
	#define CFG_DDR_WR    (12)
	#define CFG_DDR_CWL   (8)
	#define CFG_DDR_AL    (0)
	#define CFG_DDR_EXSR  (512)
	#define CFG_DDR_DQS   (4)
	#define CFG_DDR_CKSRE (8)
	#define CFG_DDR_CKSRX (8)
	#define CFG_DDR_ZQCS  (64)
	#define CFG_DDR_ZQCL  (136)
	#define CFG_DDR_XPDLL (20)
	#define CFG_DDR_ZQCSI (1000)

#endif

#ifdef DDR_11_11_11
	//DTPR0
	#define CFG_DDR_RTP (6)
	#define CFG_DDR_WTR (6)
	#define CFG_DDR_RP  (11)
	#define CFG_DDR_RCD (11)
	#define CFG_DDR_RAS (28)
	#define CFG_DDR_RRD (6)
	#define CFG_DDR_RC  (39)

	//DTPR1
	#define CFG_DDR_MRD (4)
	#define CFG_DDR_MOD (12)
	#define CFG_DDR_FAW (32)
	#define CFG_DDR_RFC  208 //(128)
	#define CFG_DDR_WLMRD (40)
	#define CFG_DDR_WLO  7 // (6)  //jiaxing add

	//DTPR2
	#define CFG_DDR_XS   (512)
	#define CFG_DDR_XP   (5)
	#define CFG_DDR_CKE  (4)
	#define CFG_DDR_DLLK (512)
	#define CFG_DDR_RTODT (0)
	#define CFG_DDR_RTW   (4)

	#define CFG_DDR_REFI  (78)
	#define CFG_DDR_REFI_MDDR3  (4)
		
	#define CFG_DDR_CL    (11)
	#define CFG_DDR_WR    (12)
	#define CFG_DDR_CWL   (8)
	#define CFG_DDR_AL    (0)
	#define CFG_DDR_EXSR  (512)
	#define CFG_DDR_DQS   (4)
	#define CFG_DDR_CKSRE (8)
	#define CFG_DDR_CKSRX (8)
	#define CFG_DDR_ZQCS  (64)
	#define CFG_DDR_ZQCL  (512)  //jiaxing add
	#define CFG_DDR_XPDLL (20)
	#define CFG_DDR_ZQCSI (1000)

#endif


/////////////////////////////////////////////////////////////////////////////


static struct ddr_set __ddr_setting={
	.ddr_test		= 0x7,	//full define in ddr.c DDR_TEST_BASEIC = 0x7, DDR_TEST_ALL = 0xf
	.phy_memory_start	= PHYS_MEMORY_START,
	.phy_memory_size	= PHYS_MEMORY_SIZE,
	.t_pub0_dtar	= ((0x0 + CONFIG_M8_DDR0_DTAR_DTCOL)|(CONFIG_M8_DDR0_DTAR_DTROW <<12)|(CONFIG_M8_DDR0_DTAR_DTBANK << 28)),
	.t_pub1_dtar	= ((0x0 + CONFIG_M8_DDR1_DTAR_DTCOL)|(CONFIG_M8_DDR1_DTAR_DTROW <<12)|(CONFIG_M8_DDR1_DTAR_DTBANK << 28)),

	.t_ddr_apd_ctrl = ( 32 << 0 ) | //[B7..B0]: DMC active latency.  latency to enter LOW POWER state after the the DMC is not actived.
					  ( 6  << 8 ) | //[B15..B8]: Low Power enter latency.when the logic check the PCTL send LP_REQ and ACKed by PUB. after this regsiter bits cycles, we'll gated the PCTL clock and PUB clock if the pub and pctl auto clock gating enabled.
					  ( 0  << 16), 	//[B17,B16]: DDR mode to genreated the refresh ack signal and prcharge signal to DMC for dmc refresh control.
					  			    // 2'b00: DDR3  2'b01.  LPDDR2/3. 2'b10, 2'b11: no refresh ack and precharge signals generated. 
					  			    
	.t_ddr_clk_ctrl = ( 1  << 0 ) | //[B0]: DDR0 PCTL PCLK enable.	before access PCTL regsiter, we need enable PCTL PCLK 24 cycles early.
					    		    //after access PCTL regsiter, we can disable PCTL PCTLK 24 cycles later.
					  ( 1  << 1 ) | //[B1]: DDR0 PUB PCLK auto clock gating enable.
					  ( 0  << 2 ) | //[B2]: Disable DDR0 PUB PCLK clock.
					  ( 1  << 3 ) | //[B3]: DDR0 PCTL n_clk auto clock gating enable.
					  ( 0  << 4 ) | //[B4]: DDR0 PCTL n_clk disable.
					  ( 1  << 5 ) | //[B5]: DDR0 PUB n_clk auto clock gating enable.  (may not be work).
					  ( 0  << 6 ) | //[B6]: DDR0 PUB n_clk disable.
					  ( 0  << 7 ) | //[B7]: NOT used. 
					  ( 1  << 8 ) | //[B8]: DDR0 PHY ctl_clk enable.  
					  ( 0  << 9 ),  //[B9]: DDR0 PHY RF mode.  we can invert the clock to DDR PHY by setting this bit to 1.

	.t_pctl_1us_pck		=  CONFIG_DDR_CLK/2,
	.t_pctl_100ns_pck	=  CONFIG_DDR_CLK/20,

#if defined (CONFIG_VLSI_EMULATOR)
	.t_pctl_init_us		=  2,
	.t_pctl_rsth_us		=  2,
	.t_pctl_rstl_us		=  0,
#else
	.t_pctl_init_us		=  512,
	.t_pctl_rsth_us		=  500,
	.t_pctl_rstl_us		=  100,
#endif //#if defined (CONFIG_VLSI_EMULATOR)

	.t_pctl_mcfg =   1 |	//[B0] mem-bl: 0 for 4; 1 for 8
			  (0 << 2) |	//[B2] bl8int_en.   enable bl8 interrupt function.Only valid for DDR2
			  				// and is ignored for mDDR/LPDDR2 and DDR3
	          (1 << 5) |    //[B5] ddr3en: 1: ddr3 protocal; 0 : ddr2 protocal
	  //        (1 << 3) |    //[B3]two_t_en: DDR 2T mode, default is disable
	          (((((CFG_DDR_FAW+CFG_DDR_RRD-1)/CFG_DDR_RRD)-4)&0x3)<<18) | //[B19,B18] tfaw_cfg
	                          	   //tFAW= (4 + MCFG.tfaw_cfg)*tRRD - MCFG1.tfaw_cfg_offset
	                          	   // 0:tFAW=4*tRRD - mcfg1.tfaw_cfg_offset 
	                          	   // 1:tFAW=5*tRRD - mcfg1.tfaw_cfg_offset 
	                          	   // 2:tFAW=6*tRRD - mcfg1.tfaw_cfg_offset
	          (1 << 17) |    //[B17]pd_exit_mode: 0: slow exit; 1: fast exit. power down exit						
	          (0 << 16) |    //[B16]pd_type: 0: precharge power down, 1 active power down
		      (0x2f << 8)    //[B15-B8]pd_idle: power-down idle in n_clk cycles. 15 cycles empty will entry power down mode.
		      ,
	.t_pctl_mcfg1 = (1<<31)| //[B31]hw_exit_idle_en
			  (((CFG_DDR_FAW%CFG_DDR_RRD)?(CFG_DDR_RRD-(CFG_DDR_FAW%CFG_DDR_RRD)):0)&0x7)<<8 //[B10,B9,B8] tfaw_cfg_offset:
			  //tFAW= (4 + MCFG.tfaw_cfg)*tRRD - tfaw_cfg_offset
			  ,

	.t_pub_zq0pr = 0x19,
	.t_pub_dxccr = 4|(0xf7<<5)|(3<<19),

	.t_pub_acbdlr0 = 0,
	
	.t_pub_dcr = 0x8b,

	.t_pub_mr={
		[0]=(1 << 12) |   //[B12] 1 fast exit from power down , 0 slow 
			((((CFG_DDR_WR <=8) ? (CFG_DDR_WR - 4) : (CFG_DDR_WR>>1)) & 7) <<  9) | //[B11,B10,B9]WR recovery. It will be calcualted by get_mrs0()@ddr_init_pctl.c 
			(0 <<  8) |   //[B8]DLL reset
			(0 <<  7) |   //[B7]0= Normal 1=Test.
			(((CFG_DDR_CL - 4) & 0x7) <<  4) |   //[B6,B5,B4]CL cas latency high 3 bits (B6,B5,B4).
			(((CFG_DDR_CL - 4) & 0x8) >> 1 ) |   //[B2]CL bit 0
			(0 << 3 ) |   //[B3]burst type,  0:sequential; 1:Interleave.
			(0 << 0 ),    //[B1,B0]burst length	:  00: fixed BL8; 01: 4 or 8 on the fly; 10:fixed BL4; 11: reserved
				                    						      
       	[1]=(0 << 9)|(1<< 6)|(0 << 2)|	//RTT (B9,B6,B2) 000 ODT disable;001:RZQ/4= 60;010: RZQ/2;011:RZQ/6;100:RZQ/12;101:RZQ/8
           	(0 << 5)|(0 << 1) |			//DIC(B5,B1) 00: Reserved for RZQ/6; 01:RZQ/7= 34;10,11 Reserved
		#ifdef CONFIG_ENABLE_WRITE_LEVELING
	            (1 << 7)|     // Write leveling enable
		#endif
	            ((CFG_DDR_AL ? ((CFG_DDR_CL - CFG_DDR_AL)&3): 0) << 3 ),//[B4,B3] AL:
	            			  //00: AL disabled; 01:CL-1;10:CL-2;11:reserved

		//#ifdef CONFIG_ENABLE_WRITE_LEVELING //jiaxing delete
	   	//[2]=0,
		//#else
	   	[2]=(0 << 10)|(0 <<9)|//[B10,B9]TRRWR: 00:Dynamic ODT off , 01:Rzq/4, 10:Rzq/2								
	   		(((CFG_DDR_CWL-5)&0x7)<<3), //[B5,B4,B3] CWL: 
					//000 = 5 (tCK ? 2.5ns)
	        		//001 = 6 (2.5ns > tCK * 1.875ns)
	        		//010 = 7 (1.875ns > tCK * 1.5ns)
    				//011 = 8 (1.5ns > tCK * 1.25ns)
    				//100 = 9 (1.25ns > tCK * 1.07ns)
    				//101 = 10 (1.07ns > tCK * 0.935ns)
    				//110 = 11 (0.935ns > tCK * 0.833ns)
    				//111 = 12 (0.833ns > tCK * 0.75ns)
		//#endif
	   	[3]=0,
	},

	.t_pub_dtpr={
		[0] =  ((CFG_DDR_RTP << 0  )|       //tRTP       //4 TCK,7500ps
				(CFG_DDR_WTR << 4  )|       //tWTR       //4 TCK,7500ps
				(CFG_DDR_RP  << 8  )|       //tRP        //12500ps
				(CFG_DDR_RCD << 12 )|       //tRCD       //12500ps
				(CFG_DDR_RAS << 16 )|       //tRAS       //35000ps
				(CFG_DDR_RRD << 22 )|       //tRRD       //7500ps
				(CFG_DDR_RC  << 26)),
		
		[1] =  ((CFG_DDR_MRD - 4 << 0 ) |		//tMRD      //4 TCK     For DDR3 and LPDDR2, the value used for tMRD is 4 plus the value programmed in these bits, i.e. tMRDvalue for DDR3 (ranges from 4 to 7)
				((CFG_DDR_MOD - 12) << 2 ) | //tMOD      //0: 12 TCK
				(CFG_DDR_FAW  << 5 )  |      //tFAW      //40000ps
				(CFG_DDR_RFC  << 11)  |      //tRFC      //160000~70312500
				(CFG_DDR_WLMRD << 20) |      //tWLMRD    //40 TCK
				(CFG_DDR_WLO   << 26)),      //tWLO      //7500ps

		[2] =  ((CFG_DDR_XS  << 0 ) |    		//tXS        //MAX(tXS,tXSDLL),tXS=170000ps,tXSDLL=512 TCK
				(CFG_DDR_XP  << 10) |    //tXP        //tXP=6000 tXPDLL=24000
				(CFG_DDR_CKE << 15) |    //tCKE       //tCKE=5000ps,tCKE_TCK=3,DDR3,set to tCKESR
				(CFG_DDR_DLLK  << 19)|   //tDLLK      //512
				(CFG_DDR_RTODT << 29)|    //tRTODT
				(1   << 30)|    //tRTW //  CFG_DDR_RTW //jiaxing add
				(0 << 31)),

		[3] = 0,
	},

	.t_pub_ptr={
		[0] =	( 6 |	         //PHY RESET APB clock cycles.
				(320 << 6) |	 //tPLLGS    APB clock cycles. 4us
				(80 << 21)),     //tPLLPD    APB clock cycles. 1us.
				
		[1] =   ( 120 |	         //tPLLRST   APB clock cycles. 9us 
				(1000 << 16)),   //TPLLLOCK  APB clock cycles. 100us. ,
				
		[2] = 0,
		[3] = (20000   |       //tDINIT0  CKE low time with power and lock stable. 500us.
				(186 << 20)),  //tDINIT1. CKE high time to first command(tRFC + 10ns). //jiaxing add
		[4] = (10000 |         //tDINIT2. RESET low time,  200us. 
				(800 << 18)),   //tDINIT3. ZQ initialization command to first command. 1us., //jiaxing add
					
	},

	.t_pctl_trefi  =  CFG_DDR_REFI,
	.t_pctl_trefi_mem_ddr3 = CFG_DDR_REFI_MDDR3,
	.t_pctl_tmrd   = CFG_DDR_MRD,
	.t_pctl_trfc   = CFG_DDR_RFC,
	.t_pctl_trp    = CFG_DDR_RP,
	.t_pctl_tal    = CFG_DDR_AL,
	.t_pctl_tcwl   = CFG_DDR_CWL,
	.t_pctl_tcl    = CFG_DDR_CL,
	.t_pctl_tras   = CFG_DDR_RAS,
	.t_pctl_trc    = CFG_DDR_RC,
	.t_pctl_trcd   = CFG_DDR_RCD,
	.t_pctl_trrd   = CFG_DDR_RRD,
	.t_pctl_trtp   = CFG_DDR_RTP,
	.t_pctl_twr    = CFG_DDR_WR,
	.t_pctl_twtr   = CFG_DDR_WTR,
	.t_pctl_texsr  = CFG_DDR_EXSR,
	.t_pctl_txp    = CFG_DDR_XP,
	.t_pctl_tdqs   = CFG_DDR_DQS,
	.t_pctl_trtw   = CFG_DDR_RTW,
	.t_pctl_tcksre = CFG_DDR_CKSRE,
	.t_pctl_tcksrx = CFG_DDR_CKSRX,
	.t_pctl_tmod   = CFG_DDR_MOD,
	.t_pctl_tcke   = CFG_DDR_CKE,
	.t_pctl_tzqcs  = CFG_DDR_ZQCS,
	.t_pctl_tzqcl  = CFG_DDR_ZQCL,
	.t_pctl_txpdll = CFG_DDR_XPDLL,
	.t_pctl_tzqcsi = CFG_DDR_ZQCSI,
	.t_pctl_scfg   = 0xf01,

	.t_ddr_pll_cntl= (CFG_PLL_OD << 16)|(CFG_PLL_N<<9)|(CFG_PLL_M<<0),
	.t_ddr_clk= CONFIG_DDR_CLK/2, //DDR PLL output is half of target DDR clock
	
	.t_mmc_ddr_ctrl= (CONFIG_DDR_CHANNEL_SET << 24) | //[B25,B24]:   ddr chanel selection. 
								//2'b00 : address bit 12.	
								//2'b01 : all address goes to ddr1. ddr0 is not used.
								//2'b10 : address bit 30.
								//2'b11 : all address goes to ddr0. ddr1 is not used.									
					(0xff<<16) | //[B16]: bank page policy.	
					(2<<14) |    //[B15,B14]: undefined
					(CONFIG_DDR_BANK_SET<<13) |	  //[B13]: ddr1 address map bank mode  1 =  address switch between 4 banks. 0 = address switch between 2 banks.	 
					(0<<12) |	 //[B12]: ddr1 rank size.  0, 1, one rank.  2 : 2 ranks. 
					(CONFIG_DDR1_ROW_SIZE<<10) |  //[B11,B10]: ddr1 row size.  2'b01 : A0~A12.	2'b10 : A0~A13.  2'b11 : A0~A14.  2'b00 : A0~A15. 
					(CONFIG_DDR1_COL_SIZE<<8)	| //[B9,B8]: ddr1 col size.	2'b01 : A0~A8,	  2'b10 : A0~A9.  
					(0<<6)  |    //[B7,B6]: undefined
					(CONFIG_DDR_BANK_SET<<5)  |    //[B5]: ddr0 address map bank mode	1 =  address switch between 4 banks. 0 = address switch between 2 banks.	
					(0<<4)  |	 //[B4]: ddr0 rank size.  0, 1, one rank.  2 : 2 ranks.	 
					(CONFIG_DDR0_ROW_SIZE<<2)  |   //[B3,B2]: ddr0 row size.	2'b01 : A0~A12.   2'b10 : A0~A13.  2'b11 : A0~A14.	2'b00 : A0~A15. 
					(CONFIG_DDR0_COL_SIZE<<0), 	   //[B1,B0]: ddr0 col size.	2'b01 : A0~A8,  2'b10 : A0~A9.  

	//All following t_mmc_ddr_timmingx unit is half of DDR timming setting
	//e.g t_mmc_ddr_timming0.tCWL = (CFG_DDR_CWL+1)/2-1;
	.t_mmc_ddr_timming0 = (((CFG_DDR_CWL+1)/2-1) << 28 )| // DDR timing register is used for ddr command filter to generate better performace
						  			  // In M6TV we only support HDR mode, that means this timing counnter is half of the real DDR clock cycles.
						              // bit 31:28.  tCWL
					      (((CFG_DDR_RP+1)/2-1)  << 24 )| // bit 27:24.  tRP
						  (((CFG_DDR_RTP+1)/2-1) << 20 )| // bit 23:20.  tRTP
						  (((CFG_DDR_RTW+1)/2-1) << 16 )| // bit 19:16.  tRTW
						  ( 4  << 12 )| // bit 15:12.  tCCD //where is the tCCD, not found in PUB/PCTL spec, 4 from ucode
						  (((CFG_DDR_RCD+1)/2-1) <<  8 )| // bit 11:8.   tRCD
						  (((CFG_DDR_CL+1)/2-1)  <<  4 )| // bit 7:4.    tCL
						  ( 3 <<  0 ) , 					  // bit 3:0.    Burst length.   
						
	.t_mmc_ddr_timming1 = (((CFG_DDR_CL+CFG_DDR_RCD+CFG_DDR_RP+1)/2-1)  << 24 )|  //bit 31:24.  tMISS. the waiting clock cycles for a miss page access ( the same bank different page is actived. = tCL + tRCD + tRP).
						  ( 6 << 20 )| 						   //bit 23:20.  cmd hold number for read command. 
						  ( 0 << 16 )| 						   //bit 19:16.  cmd hold number for write command. 
						  (((CFG_DDR_RFC+1)/2-1) <<  8 )|  //bit 15:8.   tRFC.  refresh to other command time.
						  (((CFG_DDR_WTR+1)/2-1) <<  4 )|  //bit 7:4.    tWTR.
						  (((CFG_DDR_WR+1)/2-1)  <<  0 ) , //bit 3:0.    tWR
						  
	.t_mmc_ddr_timming2 = (((CFG_DDR_ZQCL+1)/2-1) << 24 )|  //bit 31:24  tZQCL
						  ( 16 << 16 )| 				        //bit 23:16  tPVTI
						  (((CFG_DDR_REFI+1)/2-1) <<  8 )|  //bit 15:8    tREFI
						  (((CONFIG_DDR_CLK/10+1)/2 -1) <<  0 ) ,//bit 7:0  t100ns,
						  
	.t_mmc_arefr_ctrl = ( 0 << 9 ) | //bit 9      hold dmc command after refresh cmd sent to DDR3 SDRAM.
						( 0 << 8 ) | //bit 8      hold dmc command when dmc need to send refresh command to PCTL.
						( 1 << 7 ) | //bit 7      dmc to control auto_refresh enable
						( 0 << 4 ) | //bit 6:4    refresh number per refresh cycle..
						( 1 << 3 ) | //bit 3      pvt enable
						( 1 << 2 ) | //bit 2      zqc enable
						( 1 << 1 ) | //bit 1      ddr1 auto refresh dmc control select.
						( 1 << 0 ),  //bit 0      ddr0 auto refresh dmc control select.,
	.init_pctl=init_pctl_ddr3
};

//M8 SYS PLL setting
//CLKIN=24MHz,VCO=750MHz~1.5GHz
//0x1098[0xc1104260]
#if   (600 == CONFIG_SYS_CPU_CLK)
	#define	SYS_PLL_N  (1)
	#define	SYS_PLL_M  (50)
	#define SYS_PLL_OD (1)
#elif (792 == CONFIG_SYS_CPU_CLK)
	#define	SYS_PLL_N  (1)
	#define	SYS_PLL_M  (66)
	#define SYS_PLL_OD (1)
#elif (996 == CONFIG_SYS_CPU_CLK)
	#define	SYS_PLL_N  (1)
	#define	SYS_PLL_M  (83)
	#define SYS_PLL_OD (1)
#elif (1200 == CONFIG_SYS_CPU_CLK)
	#define	SYS_PLL_N  (1)
	#define	SYS_PLL_M  (50)
	#define SYS_PLL_OD (0)
#else
	#error "CONFIG_SYS_CPU_CLK is not set! Please set M8 CPU clock first!\n"
#endif

STATIC_PREFIX_DATA struct pll_clk_settings __plls
={
	//current test: >=1320MHz  can not work stable@VDD_CPU=1.2V	
	//0x1098[0xc1104260]
	.sys_pll_cntl=	(SYS_PLL_OD << 16) | //OD
					(SYS_PLL_N  << 9 ) | //N
					(SYS_PLL_M  << 0 ) | //M
					(2<<20) | // 4 bits SYS_DPLL_SS_CLK 
					(1<<24) | // 4 bits SYS_DPLL_SS_AMP 
					(0<<28) | // 1 bit SYS_DPLL_SSEN 
					(1<<29) | // 1 bit SYS_DPLL_RESET 
					(1<<30),  // 1 bit SYS_DPLL_EN 
	//A9 clock setting
	//0x1067[0xc110419c]
    .sys_clk_cntl=	(1 << 7) | // 0:oscin 1:scale out
                  	(1 << 5) | // A9_AT CLKEN
                  	(1 << 4) | // A9_APB CLKEN
                  	(0 << 2) | // 0:div1, 1:div2, 2:div3, 3:divn
                  	(1 << 0),  // 0:oscin, 1:sys_pll, 2:ddr_pll, 3:no clock 
    //A9 clock              	
    .sys_clk = CONFIG_SYS_CPU_CLK,//MHz
    .a9_clk  = CONFIG_SYS_CPU_CLK,//MHz

	.mpll_cntl = 0x600009A9,	//2.5G, fixed

	//MPEG clock(clock81) setting
	//[B14:B12]MPEG_CK_SEL 0:XTAL/32KHz 1:ddr_pll 2:mpll_clk_out0
	//                                        3:mpll_clk_out1 4:mpll_clk_out2 5:fclk_div4
	//                                        6:fclk_div3 7:fclk_div5
	//[B8]0:clk81=XTAL/32KHz 1:clk81=pll
	//[B7]enable gating
	//[B9]XTAL/32KHz, 0 : 24MHz, 1:32KHz
	//0x105d [0xc1104174]
    .mpeg_clk_cntl= (5 << 12) |    //[B14,B13,B12] select fclk_div4: 2.55GHz/4=637.5MHz
    				(1 << 8 ) |    //[B8] select pll
    				(1 << 7 ) |    //[B7] cntl_hi_mpeg_div_en, enable gating
                    (3 << 0 ) |    //[B6-B0] div  (n+1)  fclk_div5=2.55G/4=637.5MHz, clk81=637.5MHz/(3+1)=159.375MHz
					(1 << 15),     //[B15] Connect clk81 to the PLL divider output

	.vid_pll_cntl = 0x6001043D,
	.vid2_pll_cntl = 0x61000032,
	.gp_pll_cntl = 0x40120232,
	.gp2_pll_cntl = 0, //reserve for g9tv
	.clk81=159375000, //2.55GHz/4/4=159.375MHz

#if defined (CONFIG_VLSI_EMULATOR)
    .spi_setting=0xeb949, //it need fine tune?
#else	
    .spi_setting=0xea949, //it need fine tune?
#endif    
    .nfc_cfg=(((0)&0xf)<<10) | (0<<9) | (0<<5) | 5,
    .sdio_cmd_clk_divide=5,
    .sdio_time_short=(250*180000)/(2*(12)),
    .uart=
        (159375000/(CONFIG_BAUDRATE*4) -1)
        | UART_STP_BIT 
        | UART_PRTY_BIT
        | UART_CHAR_LEN 
        //please do not remove these setting , jerry.yu
        | UART_CNTL_MASK_TX_EN
        | UART_CNTL_MASK_RX_EN
        | UART_CNTL_MASK_RST_TX
        | UART_CNTL_MASK_RST_RX
        | UART_CNTL_MASK_CLR_ERR,
};
//#define DEBUGROM_CMD_BUF_SIZE ((0x1b0-0xc0-sizeof(__ddr_setting)-sizeof(__plls))&~0x1f)
#define DEBUGROM_CMD_BUF_SIZE (0x1f)	//debugrom buffer size

STATIC_PREFIX_DATA char init_script[DEBUGROM_CMD_BUF_SIZE] __attribute__((section(".setting"))) ="r c1107d54";
