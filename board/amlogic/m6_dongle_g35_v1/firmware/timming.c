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

#if defined(M6_DDR3_1GB)
	#define DDR3_4Gbx16
#elif defined(M6_DDR3_512M)
	#define DDR3_2Gbx16
#else
	#error "Please define DDR3 memory capacity first in file aml_tv_m2c.h!\n"
#endif


#ifdef DDR3_2Gbx16
	//row_size 00 : A0~A15.  01 : A0~A12, 10 : A0~A13, 11 : A0~A14. 
	#define ddr3_row_size 2 
	//col size 00 : A0~A7,   01 : A0~A8, 10: A0 ~A9.  11, A0~A9, A11. 
	#define ddr3_col_size 2
#elif defined DDR3_4Gbx16
	//row_size 00 : A0~A15.  01 : A0~A12, 10 : A0~A13, 11 : A0~A14. 
	#define ddr3_row_size 3
	//col size 00 : A0~A7,   01 : A0~A8, 10: A0 ~A9.  11, A0~A9, A11. 
	#define ddr3_col_size 2
#endif

static struct ddr_set __ddr_setting={
                    .ddr_test		=  0x7,	//full define in ddr.c DDR_TEST_BASEIC = 0x7, DDR_TEST_ALL = 0xf
                    .phy_memory_start = PHYS_MEMORY_START,
                    .phy_memory_size  = PHYS_MEMORY_SIZE,
                #ifdef M6_DDR3_1GB
                    .pub_dtar		= 0x37fff3c0,
                #endif
                #ifdef M6_DDR3_512M
                    .pub_dtar		= 0x73fff3c0,	//512M: 0x73fff3c0, 1GB: 0x37fff3c0
                #endif
                #ifdef DDR3_9_9_9
                    .cl             =   9,
                    .t_faw          =  20,//30:page size 2KB; 20:page size 1KB
                    .t_ras          =  24,
                    .t_rc           =  33,
                    .t_rcd          =   9,
                    .t_rfc          = 107,//4Gb:174~200; 2Gb:107; 1Gb:74
                    .t_rp           =   9,
                    .t_rrd          =   4,//6 or 5:page size 2KB; 4:page size 1KB
                    .t_rtp          =   5,
                    .t_wr           =  10,
                    .t_wtr          =   5,
                    .t_cwl          =   7,
                    .t_mod          =  10,
                #endif
                #ifdef DDR3_7_7_7
                    .cl             =   7,
                    .t_faw          =  20,//27:page size 2KB; 20:page size 1KB
                    .t_ras          =  20,
                    .t_rc           =  27,
                    .t_rcd          =   7,
                    .t_rfc          =  86,//4Gb:139~160; 2Gb:86; 1Gb:59
                    .t_rp           =   7,
                    .t_rrd          =   4,//6 or 5:page size 2KB; 4:page size 1KB
                    .t_rtp          =   4,
                    .t_wr           =   8,
                    .t_wtr          =   4,
                    .t_cwl          =   6,
                    .t_mod          =   8,
                #endif
                    .t_mrd          =   4,
                    .t_1us_pck      = (M6_DDR_CLK),
                    .t_100ns_pck    = (M6_DDR_CLK)/10,
                    .t_init_us      = 512,
                    .t_rsth_us      = 500,
                    .t_rstl_us      = 100,
                    .t_refi_100ns   =  39,//78 for temperature over 85 degrees
                    .t_xp           =   3,//4,
                    .t_xsrd         =   0,
                    .t_xsnr         =   0,
                    .t_exsr         = 512,
                    .t_al           =   0,
                    .t_clr          =   8,
                    .t_dqs          =   2,
                    .t_zqcl         = 512,
                    .t_rtw          =   2,
                    .t_cksrx        =   5,//7,
                    .t_cksre        =   5,//7,
                    .t_cke          =   3,//4,
                    .mrs={  [0]=(1 << 12) |   //[B12] 1 fast exit from power down (tXARD), 0 slow (txARDS).
                    			(5 <<  9) |//(4 <<  9) |   //@@[B11,B10,B9]WR recovery. It will be calcualted by get_mrs0()@ddr_init_pctl.c
                    						  //001 = 5
                    						  //010 = 6
                    						  //011 = 7
                    						  //100 = 8
                    						  //101 = 10
                    						  //110 = 12                    						  
                    			(1 <<  8) |   //[B8]DLL reset.
                    			(0 <<  7) |   //[B7]0= Normal 1=Test.
                    			(5 <<  4) |   //CL cas latency high 3 bits (B6,B5, B4, B2=0).
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
                    			                    						      
                            [1]=(0 << 9)|(1 << 6)|(1 << 2)|	//RTT (B9,B6,B2) 000 ODT disable;001:RZQ/4= 60;010: RZQ/2;011:RZQ/6;100:RZQ/12;101:RZQ/8
                                (0 << 5)|(1 << 1) |			//DIC(B5,B1) 00: Reserved for RZQ/6; 01:RZQ/7= 34;10,11 Reserved
                                (0 <<3 ),					//@@[B4,B3]AL: It will be calcualted by get_mrs1()@ddr_init_pctl.c
                                							//00: AL disabled; 01:CL-1;10:CL-2;11:reserved
                                
                                                                	
                            [2]=(2 << 9)|     //[B10,B9]Rtt_WR. 00:off. 01:Rzq/4. 02:Rzq/2
                                //(3 << 6)|     //[B6]Auto self refresh. [B7]Self refresh temperature range. 0:Normal. 1:Extended
                                (1 << 3),//(2<<3),	//@@CWL:(B5,B4,B3)
	                            		//000 = 5 (tCK = 2.5ns) 
    	                        		//001 = 6 (2.5ns > tCK = 1.875ns)
        	                    		//010 = 7 (1.875ns > tCK = 1.5ns)
            	                		//011 = 8 (1.5ns > tCK = 1.25ns)
                            [3]=0
                        },
                    .mcfg = {  1 |				   //[B0] burst length: 0 for 4; 1 for 8
                    		  (0 << 2) |		   //[B2] bl8int_en.   enable bl8 interrupt function.Only valid for DDR2
                    		  					   // and is ignored for mDDR/LPDDR2 and DDR3
                              (1 << 5) |      	   //[B5] 1: ddr3 protocal; 0 : ddr2 protocal
                              //(1 << 3) |    	            //[B3]2T mode, default is disable
                              //(tFAW/tRRD <<18) | //@@[B19,B18]tFAW will be set according to the calculation with t_rrd and t_faw
                                              	   // in file /firmware/ddr_init_pctl.c
                                              	   // 0:tFAW=4*tRRD 1:tFAW=5*tRRD 2:tFAW=6*tRRD
                              (1 << 17) |     	   //[B17]0: slow exit; 1: fast exit. power down exit
                                                   // always enable auto power down.  Disable this only
                                                   // if you care about bandwidth/efficiency.
						      (0xf << 8)      	   // [B15-B8]15 cycles empty will entry power down mode.
                           },
                    .zqcr  = (( 1 << 24) | 0x11dd),   //0x11dd->22 ohm;0x1155->0 ohm
                    .zq0cr1 = 0x1d,//0x18,   //PUB ZQ0CR1
         .ddr_pll_cntl = 0x10200 | (M6_DDR_CLK/12), //528MHz
         .ddr_clk = (M6_DDR_CLK),
         .ddr_ctrl= (0 << 24 ) |      //pctl_brst 4,
                    (0xff << 16) |    //reorder en for the 8 channel.
                    (0 << 15 ) |      // pctl16 mode = 0.  pctl =   32bits data pins
                    (0 << 14 ) |      // page policy = 0.
                    (1 << 13 ) |      // command reorder enabled.
                    (0 << 12 ) |      // bank map = 0, bank sweep between 4 banks.
                    (1 << 11 ) |      // Block size.  0 = 32x32 bytes.  1 = 64x32 bytes.
                    (0 << 10 ) |      // ddr burst 0 = 8 burst. 1 = 4 burst.
                    (3 << 8 )  |      // ddr type.  2 = DDR2 SDRAM.  3 = DDR3 SDRAM.
                    (0 << 7 )  |      // ddr 16 bits mode.  0 = 32bits mode.
                    (1 << 6 )  |      // 1 = 8 banks.  0 = 4 banks.
                    (0 << 4 )  |      // rank size.   0= 1 rank.   1 = 2 rank.
                    (ddr3_row_size << 2) |
                    (ddr3_col_size),
         .init_pctl = init_pctl_ddr3        
};

STATIC_PREFIX_DATA struct pll_clk_settings __plls __attribute__((section(".setting")))
={
	/*
	* SYS_PLL setting:
	* 24MHz: [30]:PD=0, [29]:RESET=0, [17:16]OD=1, [13:9]N=1, [8:0]M=50, PLL_FOUT= (24*(50/1))/2 = 600MHz
	* 25MHz: [30]:PD=0, [29]:RESET=0, [17:16]OD=1, [13:9]N=1, [8:0]M=48, PLL_FOUT= (25*(48/1))/2 = 600MHz
	*/
	//current test: >=1320MHz  can not work stable@VDD_CPU=1.2V
	//PLL=1296MHz: PD=0,RESET=0,OD=0,N=1,M=54
	//PLL=1200MHz: PD=0,RESET=0,OD=0,N=1,M=50
	//PLL=1000MHz: PD=0,RESET=0,OD=0,N=3,M=125
	//PLL=900MHz:   PD=0,RESET=0,OD=0,N=2,M=75
	//PLL=800MHz:   PD=0,RESET=0,OD=0,N=3,M=100
	//PLL=700MHz:   PD=0,RESET=0,OD=0,N=6,M=175
	//PLL=600MHz:   PD=0,RESET=0,OD=1,N=1,M=50
	//0x1098[0xc1104260]
	.sys_pll_cntl=	(1  << 16) | //OD
					(1  << 9 ) | //N
					(50 << 0 ),	 //M
	//A9 clock setting
	//0x1067[0xc110419c]
    .sys_clk_cntl=	(1 << 7) | // 0:oscin 1:scale out
                  	(1 << 5) | // A9_AT CLKEN
                  	(1 << 4) | // A9_APB CLKEN
                  	(0 << 2) | // 0:div1, 1:div2, 2:div3, 3:divn
                  	(1 << 0),  // 0:oscin, 1:sys_pll, 2:ddr_pll, 3:no clock 
    //A9 clock              	
    .sys_clk=600,//MHz
    .a9_clk=600, //MHz
    
    .other_pll_cntl=0x00000219,//0x19*24/1=600M

	//MPEG clock(clock81) setting
	//[14:12]MPEG_CK_SEL 0:socin 1:ddr_pll 2:mp0_clko 3:mp1_clko 4:mp2_clko 5:fclk_div2 6:fclk_div3 7:fclk_div5
	//[8]0:clk81=XTL 1:clk81=pll
	//[7]enable gating
	//0x105d [0xc1104174]
    .mpeg_clk_cntl= (7 << 12) |    //[B14,B13,B12] select fclk_div5=400MHz
    				(1 << 8 ) |    //[B8] select pll
    				(1 << 7 ) |    //[B7] cntl_hi_mpeg_div_en, enable gating
                    (1 << 0 ) |    //[B6-B0] div 2 (n+1)  fclk_div5=2G/5=400MHz, clk81=400MHz/(1+1)=200MHz
					(1 << 15),     //[B15] Connect clk81 to the PLL divider output

	.clk81=200000000,	

    .demod_pll400m_cntl=(1<<9)  | //n 1200=xtal*m/n 
            (50<<0),    		//m 50*24
            
    .spi_setting=0xea949,
    .nfc_cfg=(((0)&0xf)<<10) | (0<<9) | (0<<5) | 5,
    .sdio_cmd_clk_divide=5,
    .sdio_time_short=(250*180000)/(2*(12)),
    .uart=
        (200000000/(CONFIG_BAUDRATE*4) -1)
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
