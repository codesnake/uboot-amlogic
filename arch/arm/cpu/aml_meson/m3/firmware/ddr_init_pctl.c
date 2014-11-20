void load_nop(void)
{
  APB_Wr(PCTL_MCMD_ADDR, (1 << 31) |
                         (DDR_RANK << 20) |   //rank select
                          NOP_CMD );
  __udelay(1000);
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
  __udelay(1000);
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

unsigned get_mrs0(struct ddr_set * timing_reg)
{
    unsigned mmc_ctrl=timing_reg->ddr_ctrl;
    unsigned ret=(1<<12)|(0<<3);
    //bl 2==4 0==8
    ret|=mmc_ctrl&(1<<10)?2:0;
    //cl
    ret|=((timing_reg->cl-4)&0x7)<<4;
    //wr: write recovery 
    if(timing_reg->t_wr <= 8)
        ret|=((timing_reg->t_wr-4)&0x7)<<9;
    else
    	  ret|=((timing_reg->t_wr>>1)&7)<<9;
    return ret&0x1fff;
}

unsigned get_mrs1(struct ddr_set * timing_reg)
{
    //unsigned ret=(1<<6)|(1<<2);//rtt_nominal;      //(A9, A6, A2) 000 : disabled. 001 : RZQ/4   (A6:A2)
    unsigned ret = 0;
    ret|=timing_reg->mrs[1]&((1<<9)|(1<<6)|(1<<2)| //rtt_nominal
                             (1<<5)|(1<<1) |       //(A5 A1),Output driver impedance control 00:RZQ/6,01:RZQ/7,10£ºRZQ/5 11:Reserved
                             (0<<12)|              //Qoff output buffer
                             (0<<7) );             //Write level  

    //cl
    if(timing_reg->t_al)
    {
        ret|=((timing_reg->cl-timing_reg->t_al)&3)<<3;
    }
    return ret&0x1fff;
}
unsigned get_mrs2(struct ddr_set * timing_reg)
{
    unsigned ret=((timing_reg->t_cwl-5)&7)<<3;
    return ret&0x1fff;
}


int init_pctl_ddr3(struct ddr_set * timing_reg)
{
   int i;
   int mrs0_value;
   int mrs1_value;   
   int mrs3_value;
   static unsigned time_tag = 1;
    Wr(RESET1_REGISTER,1<<3);
    if(time_tag){
    	serial_puts(__DATE__);
    	serial_puts(__TIME__);
    	time_tag=0;
    }
		APB_Wr(MMC_DDR_CTRL,timing_reg->ddr_ctrl);
#if 0    
		int mrs2_value;
     mrs0_value =           (1 << 12 ) |   // 1 fast exit from power down (tXARD), 0 slow (txARDS).
                            (4 <<9 )   |   //wr recovery   4 means write recovery = 8 cycles..
                            (0  << 8 ) |   //DLL reset.
                            (0  << 7 ) |   //0= Normal 1=Test.
                            (4  << 4 ) |   //cas latency high 3 bits (A6,A5, A4, A2) = 8 
                            (0  << 3 ) |   //burst type,  0:sequential 1 Interleave.
                            (0  << 2 ) |   //cas latency bit 0.
                                   0 ;     //burst length  :  2'b00 fixed BL8 

      mrs1_value =       (0 << 12 )  | // qoff 0=op buf enabled : 1=op buf disabled  (A12)
                         (0 << 11 )  | // rdqs enable 0=disabled : 1=enabled                (A11)
                         (0 << 7  )  |  // write leveling enable 0 = disable.               (A7) 
                  (timing_reg->t_al << 3) | //additive_latency; // 0 - 4                                 (A4-A3)

                         ( 0 << 9)   |
                         ( 0 << 6)   |
                         ( 1 << 2)   | //rtt_nominal;      //(A9, A6, A2) 000 : disabled. 001 : RZQ/4   (A6:A2)
                         ( 0 << 5)   |
                         ( 0 << 1 )  | //ocd_impedence;    (A5, A1) 00 : RZQ/6. 01: RZQ/7. 10, 11: TBD  (A5,A1)
                         ( 0 << 0 ) ;  // dll_enable;       // 0=enable : 1=disable                     (A0)

   // load_mrs(2);
   // PASR = 0 : full Array self-Refresh.   A2 ~ A0.
   // CWL cas write latency = 5     A5~A3.
   // ASR  0: Manual Self-refrsh.  1: Auto Self_refresh. A6
   // SRT.   self-Frfresh temperature range. 0 : normal operationg temperature range. A7
   // Rtt_wr A10~A9:   2'b00:   Synamic ODT off. 2'b01: RZQ/4.  2'b10: RZQ/2.  2'b11 : reserved.
   //                 CWL : 0 for 400Mhz(CWL=5).  1 for 1.876 clock period.(CWL=6) 2 :  1.5ns <= tck <1.875ns(CWL=7).
       mrs2_value =  1 << 3;

#endif
       mrs3_value = 0;
	   
      //configure basic DDR PHY parameter.
      APB_Wr(PCTL_TOGCNT1U_ADDR, timing_reg->t_1us_pck);
      APB_Wr(PCTL_TOGCNT100N_ADDR, timing_reg->t_100ns_pck);
      APB_Wr(PCTL_TINIT_ADDR, timing_reg->t_init_us);
             
      APB_Wr(PCTL_TRSTH_ADDR, 500);       // 500us  to hold reset high before assert CKE. change it to 50 for fast simulation time.
      APB_Wr(PCTL_TRSTL_ADDR, 100);        //  100 clock cycles for reset low 

      // to disalbe cmd lande receiver current. 
      // we don't need receive data from command lane.
      APB_Wr(MMC_PHY_CTRL,   1 );  
#define ENABLE_POWER_SAVING
#ifdef ENABLE_POWER_SAVING    
      APB_Wr(PCTL_IOCR_ADDR, 0xfe000f0d);
#else
      APB_Wr(PCTL_IOCR_ADDR, 0xcc000f0d);
#endif

#ifdef ENABLE_POWER_SAVING
      APB_Wr(PCTL_PHYCR_ADDR, 0x070); // bit 0 0 = active windowing mode.
                                     // bit 3 0 = drift compensation disabled.
                                     // bit 8 0 = automatic data training enable.
#else
      APB_Wr(PCTL_PHYCR_ADDR, APB_Rd(PCTL_PHYCR_ADDR) & 0xfffffeff );   //disable auto data training when PCTL exit low power state.
#endif                                     

      while (!(APB_Rd(PCTL_POWSTAT_ADDR) & 2)) {} // wait for dll lock
      APB_Wr(PCTL_POWCTL_ADDR, 1);            // start memory power up sequence
      while (!(APB_Rd(PCTL_POWSTAT_ADDR) & 1)) {} // wait for memory power up


     
       //configure the PCTL for DDR3 SDRAM burst length = 8
       
      for(i=4;(i)*timing_reg->t_rrd<timing_reg->t_faw&&i<6;i++);
      APB_Wr(PCTL_MCFG_ADDR,     1 |            // burst length 0 = 4; 1 = 8
                                (0 << 2) |     // bl8int_en.   enable bl8 interrupt function.
                                (1 << 5) |     // 1: ddr3 protocal; 0 : ddr2 protocal
                                ((i-4) <<18) |      // tFAW.
                               (1 << 17) |      // power down exit which fast exit.
                                (0xf << 8)      // 0xf cycle empty will entry power down mode.
                                        );

      //configure DDR3 SDRAM parameter.
      APB_Wr(PCTL_TREFI_ADDR, timing_reg->t_refi_100ns);
      APB_Wr(PCTL_TMRD_ADDR,  timing_reg->t_mrd);
      APB_Wr(PCTL_TRFC_ADDR,  timing_reg->t_rfc);
      APB_Wr(PCTL_TRP_ADDR,   timing_reg->t_rp);
      APB_Wr(PCTL_TAL_ADDR,   timing_reg->t_al);
      APB_Wr(PCTL_TCWL_ADDR,  timing_reg->t_cwl);
      APB_Wr(PCTL_TCL_ADDR,   timing_reg->cl);
      APB_Wr(PCTL_TRAS_ADDR,  timing_reg->t_ras);
      APB_Wr(PCTL_TRC_ADDR,   timing_reg->t_rc);
      APB_Wr(PCTL_TRCD_ADDR,  timing_reg->t_rcd);
      APB_Wr(PCTL_TRRD_ADDR,  timing_reg->t_rrd);
      APB_Wr(PCTL_TRTP_ADDR,  timing_reg->t_rtp);
      APB_Wr(PCTL_TWR_ADDR,   timing_reg->t_wr);
      APB_Wr(PCTL_TWTR_ADDR,  timing_reg->t_wtr);
      APB_Wr(PCTL_TEXSR_ADDR, timing_reg->t_exsr);
      APB_Wr(PCTL_TXP_ADDR,   timing_reg->t_xp);
      APB_Wr(PCTL_TDQS_ADDR,  timing_reg->t_dqs);
      APB_Wr(PCTL_TMOD_ADDR,  timing_reg->t_mod);
      APB_Wr(PCTL_TZQCL_ADDR, timing_reg->t_zqcl);
      APB_Wr(PCTL_TZQCSI_ADDR, 4095);
      APB_Wr(PCTL_TCKSRX_ADDR, timing_reg->t_cksrx);
      APB_Wr(PCTL_TCKSRE_ADDR, timing_reg->t_cksre);
      APB_Wr(PCTL_TCKE_ADDR,   timing_reg->t_cke);
      APB_Wr(PCTL_ODTCFG_ADDR, 8);         //configure ODT
#if 0
    //ZQ calibration 
    APB_Wr(PCTL_ZQCR_ADDR, ( 1 << 31) | (0x7b <<16) );
    //wait for ZQ calibration.
    while ( APB_Rd(PCTL_ZQCR_ADDR ) & ( 1<<31) ) {}
        if ( APB_Rd(PCTL_ZQSR_ADDR) & (1 << 30) ) {
            serial_puts("ZQ Fail\n");
        return -1;
    }
#else
    APB_Wr(PCTL_ZQCR_ADDR, ( 1 << 24) | 0x11dd);
#endif

      // initialize DDR3 SDRAM
        load_nop();
        load_mrs(2, get_mrs2(timing_reg));
        load_mrs(3, mrs3_value);
        mrs1_value = get_mrs1(timing_reg) & 0xfffffffe; //dll enable 
        load_mrs(1, mrs1_value);
        mrs0_value = get_mrs0(timing_reg) | (1 << 8);    // dll reset.
        load_mrs(0, mrs0_value);
        load_zqcl(0);     // send ZQ calibration long command.
        return 0;
}
