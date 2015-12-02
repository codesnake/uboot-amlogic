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

static int init_pctl_ddr3(struct ddr_set * ddr_setting);

/////////////////////////////////////////////////////////////////////////////
//DDR timming fine tune code 2012.11.23
//
#if (CFG_M6TV_DDR_CLK >= 400) && (CFG_M6TV_DDR_CLK < 750)
	#define CFG_M6TV_PLL_OD 2
	#define CFG_M6TV_PLL_N  1
	#define CFG_M6TV_PLL_M  (((CFG_M6TV_DDR_CLK/6)*6)/12)
#elif (CFG_M6TV_DDR_CLK >= 750) && (CFG_M6TV_DDR_CLK <= 900)
	#define CFG_M6TV_PLL_OD 1
	#define CFG_M6TV_PLL_N  1
	#define CFG_M6TV_PLL_M  (((CFG_M6TV_DDR_CLK/12)*12)/24)
#else
	#error "Over PLL range! Please check CFG_M6TV_DDR_CLK with file m6tv_ref_v1.h! \n"
#endif

#if (CFG_M6TV_DDR_CLK >= 400 ) && (CFG_M6TV_DDR_CLK <533)
	#define DDR3_7_7_7
#elif  (CFG_M6TV_DDR_CLK >= 533 ) && (CFG_M6TV_DDR_CLK <680)
	#define DDR3_9_9_9 
#elif  (CFG_M6TV_DDR_CLK >= 680 ) && (CFG_M6TV_DDR_CLK <900)
	#define DDR3_11_11_11
#endif

#ifdef DDR3_11_11_11
	#define CFG_M6TV_DDR_CL  11
	#define CFG_M6TV_DDR_FAW 32
	#define CFG_M6TV_DDR_RAS 28
	#define CFG_M6TV_DDR_RC  38
	#define CFG_M6TV_DDR_RCD 11
	#define CFG_M6TV_DDR_RFC 240
	#define CFG_M6TV_DDR_RP  11
	#define CFG_M6TV_DDR_RRD 6
	#define CFG_M6TV_DDR_WR  12
	#define CFG_M6TV_DDR_CWL 8
	#define CFG_M6TV_DDR_MOD 12
	#define CFG_M6TV_DDR_MRD 4
	#define CFG_M6TV_DDR_AL  0
#endif

#ifdef DDR3_9_9_9
	#define CFG_M6TV_DDR_CL  10
	#define CFG_M6TV_DDR_FAW 30
	#define CFG_M6TV_DDR_RAS 24
	#define CFG_M6TV_DDR_RC  33
	#define CFG_M6TV_DDR_RCD 9
	#define CFG_M6TV_DDR_RFC 200
	#define CFG_M6TV_DDR_RP  9
	#define CFG_M6TV_DDR_RRD 5
	#define CFG_M6TV_DDR_WR  10
	#define CFG_M6TV_DDR_CWL 7
	#define CFG_M6TV_DDR_MOD 12
	#define CFG_M6TV_DDR_MRD 7
	#define CFG_M6TV_DDR_AL  0

#endif

#ifdef DDR3_7_7_7
	#define CFG_M6TV_DDR_CL  7
	#define CFG_M6TV_DDR_FAW 27
	#define CFG_M6TV_DDR_RAS 20
	#define CFG_M6TV_DDR_RC  27
	#define CFG_M6TV_DDR_RCD 7
	#define CFG_M6TV_DDR_RFC 160
	#define CFG_M6TV_DDR_RP  7
	#define CFG_M6TV_DDR_RRD 6
	#define CFG_M6TV_DDR_WR  8
	#define CFG_M6TV_DDR_CWL 6
	#define CFG_M6TV_DDR_MOD 12
	#define CFG_M6TV_DDR_MRD 4
	#define CFG_M6TV_DDR_AL  0
#endif

#if defined(M6TV_DDR3_1GB)
	#define DDR3_4Gbx16
#elif defined(M6TV_DDR3_512M)
	#define DDR3_2Gbx16
#else
	#error "Please define DDR3 memory capacity first in file m6tv_ref_v1.h!\n"
#endif
/////////////////////////////////////////////////////////////////////////////


//row_size 00 : A0~A15.  01 : A0~A12, 10 : A0~A13, 11 : A0~A14. 
//col size 00 : A0~A7,	 01 : A0~A8, 10: A0 ~A9.  11, A0~A9, A11. 
#ifdef DDR3_2Gbx16
	#define ddr3_row_size 2 
	#define ddr3_col_size 2
#elif defined DDR3_4Gbx16
	#define ddr3_row_size 3
	#define ddr3_col_size 2
#endif

static struct ddr_set __ddr_setting={

                    .cl             =  CFG_M6TV_DDR_CL,
                    .t_faw          =  CFG_M6TV_DDR_FAW,
                    .t_ras          =  CFG_M6TV_DDR_RAS,
                    .t_rc           =  CFG_M6TV_DDR_RC,
                    .t_rcd          =  CFG_M6TV_DDR_RCD,
                    .t_rfc          =  CFG_M6TV_DDR_RFC,
                    .t_rp           =  CFG_M6TV_DDR_RP,
                    .t_rrd          =  CFG_M6TV_DDR_RRD,
                    .t_wr           =  CFG_M6TV_DDR_WR,
                    .t_cwl          =  CFG_M6TV_DDR_CWL,
                    .t_mod          =  CFG_M6TV_DDR_MOD,
                    .t_mrd          =  CFG_M6TV_DDR_MRD,
                    .t_1us_pck      =  CFG_M6TV_DDR_CLK,
                    .t_100ns_pck    =  CFG_M6TV_DDR_CLK/10,
                    .t_init_us      = 512,
                    .t_rsth_us      = 500,
                    .t_rstl_us      = 100,
                    .t_refi_100ns   =  39,//78 for temperature over 85 degrees
                    .t_xsrd         =   0,
                    .t_xsnr         =   0,
                    .t_exsr         = 512,
                    .t_al           =  CFG_M6TV_DDR_AL,//0
                    .t_clr          =   8,
                    .t_dqs          =   2,
                    .t_zqcl         = 512,
                    .t_rtw          =   4,
                    .t_rtp          =   6,//8,//4,//8,//FIXED
                    .t_wtr          =   6,//8,//4,//8,//FIXED
                    .t_xp           =   5,//26,//3,//26,//FIXED
                    .t_xpdll        =   20,//26,//20,//26,
                    .t_cksrx        =   5,//8,//5,//8,//FIXED
                    .t_cksre        =   5,//8,//5,//8,//FIXED
                    .t_cke          =   4,//6,//4,//6,//FIXED
                    
                    .mrs={  [0]=(1 << 12) |   //[B12] 1 fast exit from power down (tXARD), 0 slow (txARDS).
                    			(6<<9)| //(4 <<  9) |   //@@[B11,B10,B9]WR recovery. It will be calcualted by get_mrs0()@ddr_init_pctl.c
                    						  //001 = 5
                    						  //010 = 6
                    						  //011 = 7
                    						  //100 = 8
                    						  //101 = 10
                    						  //110 = 12                    						  
                    			(1 <<  8) |   //[B8]DLL reset.
                    			(0 <<  7) |   //[B7]0= Normal 1=Test.
                    			(6 <<  4) |   //(5 <<  4) |   //CL cas latency high 3 bits (B6,B5, B4, B2=0).
                    						  //@@[B6,B5,B4,B2]CL will be calcualted by get_mrs0()@ddr_init_pctl.c
                    						  //0010 = 5
                    						  //0100 = 6
                    						  //0110 = 7
                    						  //1000 = 8
                    						  //1010 = 9
                    						  //1100 = 10
                    						  //1110 = 11
                    			(0 << 3 ) |   //[B3]burst type,  0:sequential; 1:Interleave.
                    			(0 << 2 ) |   //[B2]cas latency bit 0.
								(0 << 0 ),    //[B1,B0]burst length	:  00: fixed BL8; 01: 4 or 8 on the fly; 10:fixed BL4; 11: reserved
                    			                    						      
                            [1]=(0 << 9)|(1 << 6)|(0 << 2)|	//RTT (B9,B6,B2) 000 ODT disable;001:RZQ/4= 60;010: RZQ/2;011:RZQ/6;100:RZQ/12;101:RZQ/8
                                (0 << 5)|(0 << 1) |			//DIC(B5,B1) 00: Reserved for RZQ/6; 01:RZQ/7= 34;10,11 Reserved
#ifdef ENABLE_WRITE_LEVELING
                                (1 << 7)|     // Write leveling enable
#endif
                                (0 << 3 ),					//@@[B4,B3]AL: It will be calcualted by get_mrs1()@ddr_init_pctl.c
                                							//00: AL disabled; 01:CL-1;10:CL-2;11:reserved
                                
#ifdef ENABLE_WRITE_LEVELING
                            [2]=0,
#else
                            [2]=(1 << 10)|(0 <<9 ),//(A10:A9) 00:Dynamic ODT off , 01:Rzq/4, 10:Rzq/2								
#endif
                            [3]=0
                        },
                    .mcfg =   1 |				   //[B0] burst length: 0 for 4; 1 for 8
                    		  (0 << 2) |		   //[B2] bl8int_en.   enable bl8 interrupt function.Only valid for DDR2
                    		  					   // and is ignored for mDDR/LPDDR2 and DDR3
                              (1 << 5) |      	   //[B5] 1: ddr3 protocal; 0 : ddr2 protocal
                              (1 << 3) |    	            //[B3]2T mode, default is disable
                              //(tFAW/tRRD <<18) | //@@[B19,B18]tFAW will be set according to the calculation with t_rrd and t_faw
                                              	   // in file /firmware/ddr_init_pctl.c
                                              	   // 0:tFAW=4*tRRD 1:tFAW=5*tRRD 2:tFAW=6*tRRD
                              (1 << 17) |     	   //[B17]0: slow exit; 1: fast exit. power down exit
						      (0xf << 8)      	   // [B15-B8]15 cycles empty will entry power down mode.
                           ,
                    .zq0cr0  = 0x109ad,
                    .zq0cr1  = 0x19, // 0x1d for GT ddr
                    .cmdzq   = 0x109ce,  //need enable FORCE_CMDZQ_ENABLE
                    .t_dxccr_dqsres  = 0x1, //ODT: pull down, 688ohms
                    					    //PUB_DXCCR[8:5]: DQS resister. DQSRES[3]: 0 - pull down, 1-pull up. 
                    				        //DQSRES[2:0]:000-open, use extern ODT,
                    				        //                      001-688ohms,010-611ohms,011-550ohms,
                    				        //                      100-500ohms,101-458ohms,110-393ohms,
                    				        //                      111-344ohms
                    .t_dxccr_dqsnres = 0x2, //ODT: pull down,611ohms
                    					    //PUB_DXCCR[12:9]: DQS# resister                    
                    .t_acbdlr_ck0bd = 34,   //PUB_ACBDLR[5:0]: ck0 bit delay
                    .t_acbdlr_acbd  = 0,    //PUB_ACBDLR[23:18]: address/command bit delay
         .ddr_pll_cntl= (CFG_M6TV_PLL_OD << 16)|(CFG_M6TV_PLL_N<<9)|(CFG_M6TV_PLL_M<<0),
         .ddr_clk= CFG_M6TV_DDR_CLK/2,
	     //#define P_MMC_DDR_CTRL 	   0xc8006000 
         .ddr_ctrl= (0x7f << 16) |   // bit 25:16  ddr command filter bank and read write over timer limit
                    (1 << 7 ) |      // bit 7         ddr command filter bank policy. 1 = keep open. 0 : close bank if no request.
                    (1 << 6 ) |      // bit 6         ddr address map bank mode  1 =  address switch between 4 banks. 0 = address switch between 2 banks.
                    (2 << 4 ) |      // bit 5:4      ddr rank size.  0, 1, one rank.  2 : 2 ranks. 
                    (ddr3_row_size << 2 ) |      // bit 3:2      ddr row size.  2'b01 : A0~A12.   2'b10 : A0~A13.  2'b11 : A0~A14.  2'b00 : A0~A15.
                    (ddr3_col_size << 0 ),       // bit 1:0      ddr col size.  2'b01 : A0~A8,    2'b10 : A0~A9.
         .init_pctl=init_pctl_ddr3        
};

//0x1098[0xc1104260]
#if   (700 == CONFIG_SYS_CPU_CLK)
	#define	M6TV_SYS_PLL_N  (6)
	#define	M6TV_SYS_PLL_M  (175)
	#define M6TV_SYS_PLL_OD (0)
#elif (800 == CONFIG_SYS_CPU_CLK)
	#define	M6TV_SYS_PLL_N  (3)
	#define	M6TV_SYS_PLL_M  (100)	
	#define M6TV_SYS_PLL_OD (0)
#elif (900 == CONFIG_SYS_CPU_CLK)
	#define	M6TV_SYS_PLL_N  (2)
	#define	M6TV_SYS_PLL_M  (75)
	#define M6TV_SYS_PLL_OD (0)
#elif (1000 == CONFIG_SYS_CPU_CLK)
	#define	M6TV_SYS_PLL_N  (3)
	#define	M6TV_SYS_PLL_M  (125)
	#define M6TV_SYS_PLL_OD (0)
#elif (1200 == CONFIG_SYS_CPU_CLK)
	#define	M6TV_SYS_PLL_N  (1)
	#define	M6TV_SYS_PLL_M  (50)
	#define M6TV_SYS_PLL_OD (0)
#elif (1296 == CONFIG_SYS_CPU_CLK)
	#define	M6TV_SYS_PLL_N  (1)
	#define	M6TV_SYS_PLL_M  (54)
	#define M6TV_SYS_PLL_OD (0)
#else
	#error "CONFIG_SYS_CPU_CLK is not set! Please set M6TV CPU clock first!\n"
#endif

#ifndef CONFIG_CLK81
	#define M6TV_CLK81		(200000000)
	#define M6TV_CLK81_PLL_SEL	(7)
	#define M6TV_CLK81_PLL_DIV	(1)
#else
#if CONFIG_CLK81>200
	#define M6TV_CLK81		(222000000)
	#define M6TV_CLK81_PLL_SEL	(6)
	#define M6TV_CLK81_PLL_DIV	(2)
#elif CONFIG_CLK81<200
	#define M6TV_CLK81		(167000000)
	#define M6TV_CLK81_PLL_SEL	(6)
	#define M6TV_CLK81_PLL_DIV	(3)
#else
	#define M6TV_CLK81		(200000000)
	#define M6TV_CLK81_PLL_SEL	(7)
	#define M6TV_CLK81_PLL_DIV	(1)
#endif
#endif
STATIC_PREFIX_DATA struct pll_clk_settings __plls __attribute__((section(".setting")))
={
	//current test: >=1320MHz  can not work stable@VDD_CPU=1.2V	
	//0x1098[0xc1104260]
	.sys_pll_cntl=	(M6TV_SYS_PLL_OD << 16) | //OD
					(M6TV_SYS_PLL_N  << 9 ) | //N
					(M6TV_SYS_PLL_M  << 0 ),  //M
	//A9 clock setting
	//0x1067[0xc110419c]
    .sys_clk_cntl=	(1 << 7) | // 0:oscin 1:scale out
                  	(1 << 5) | // A9_AT CLKEN
                  	(1 << 4) | // A9_APB CLKEN
                  	(0 << 2) | // 0:div1, 1:div2, 2:div3, 3:divn
                  	(1 << 0),  // 0:oscin, 1:sys_pll, 2:ddr_pll, 3:no clock 
    //A9 clock              	
    .sys_clk=800,//MHz
    .a9_clk=800, //MHz
    
    .other_pll_cntl=0x00000219,//0x19*24/1=600M

	//MPEG clock(clock81) setting
	//[14:12]MPEG_CK_SEL 0:socin 1:ddr_pll 2:mp0_clko 3:mp1_clko 4:mp2_clko 5:fclk_div2 6:fclk_div3 7:fclk_div5
	//[8]0:clk81=XTL 1:clk81=pll
	//[7]enable gating
	//0x105d [0xc1104174]
    .mpeg_clk_cntl= (M6TV_CLK81_PLL_SEL << 12) |    //[B14,B13,B12] 7 select fclk_div5=400MHz; 6 select fclk_div3=667MHz
    				(1 << 8 ) |    //[B8] select pll
    				(1 << 7 ) |    //[B7] cntl_hi_mpeg_div_en, enable gating
                    (M6TV_CLK81_PLL_DIV << 0 ) |    //[B6-B0] div 2 (n+1)  fclk_div5=2G/5=400MHz, clk81=400MHz/(1+1)=200MHz
					(1 << 15),     //[B15] Connect clk81 to the PLL divider output

	.clk81=M6TV_CLK81,	

    .demod_pll400m_cntl=(1<<9)  | //n 1200=xtal*m/n 
            (50<<0),    		//m 50*24
            
    .spi_setting=0xea949,
    .nfc_cfg=(((0)&0xf)<<10) | (0<<9) | (0<<5) | 5,
    .sdio_cmd_clk_divide=5,
    .sdio_time_short=(250*180000)/(2*(12)),
    .uart=
        (M6TV_CLK81/(CONFIG_BAUDRATE*4) -1)
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
#define DEBUGROM_CMD_BUF_SIZE ((0x1b0-0xc0-sizeof(__ddr_setting)-sizeof(__plls))&~0x1f)

STATIC_PREFIX_DATA char init_script[DEBUGROM_CMD_BUF_SIZE] __attribute__((section(".setting")))
="r c1107d54";
