//#define _GATESIM_
//---------------------------------------------------
//       clocks_set_sys_defaults
//---------------------------------------------------

#include <asm/arch/reg_addr.h>
#include <asm/arch/clk_util.h>

#if 0
void    clocks_set_sys_defaults( )
{
    // unsigned long   rd_data;

    // Defines in $top/common/stimulus_util.v or stimulus.v can be used to force
    // bits on the PLL_SCLK_SIM0 register to adjust the clocks from verilog
    unsigned long   mpeg_pll_override   = (Rd(ISA_PLL_CLK_SIM0) & (1 << 31)) ? 1 : 0;
    unsigned long   mpeg_hiu_pll_reg    = Rd(ISA_PLL_CLK_SIM0) & 0xFFFF;
    unsigned long   mpeg_hiu_pll_xd     = (Rd(ISA_PLL_CLK_SIM0) >> 16) & 0x7F;

    
    // -----------------------------------------
    // Switch clk81 to the oscillator input (boot.s might have already
    // programmed clk81 to a PLL)
    Wr( HHI_MPEG_CLK_CNTL, Rd(HHI_MPEG_CLK_CNTL) & ~(1 << 8) );  
    // Switch to oscillator, the SYS CPU might have already been programmed
    //        .cntl_scale_out_sync_sel    ( hi_sys_cpu_clk_cntl[7]        ),  // 0 = oscin, 1 = scale out
    //        .cntl_scale_pll_sel         ( hi_sys_cpu_clk_cntl[1:0]      ),  // 0 = oscin, 1 = clk1, 2 = clk2, 3 = clk3
    Wr( HHI_SYS_CPU_CLK_CNTL, Rd(HHI_SYS_CPU_CLK_CNTL) & ~((1 << 7) | (0x3 << 0)) );  

    // if NOT overriding the default
    if( !mpeg_pll_override ) {
        // -------------------------
        // set SYS PLL
        // -------------------------

        // +------------------------------------------------------------+   +----------------------------------------+
        // |                      <<< PLL >>>                           |   |         <<< Clock Reset Test >>>       |
        // +---------+-----------+----------------------+---------------+   +------+-----------+---------------------+  +------------
        // |         | PLL Ports |      PLL        PLL  | PLL  PLL      |   | CRT  |    Final  |               Ideal |  |
        // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     |   |  XD  |    Clock  |    Error      Clock |  |  HIU REG
        // +---------+-----------+----------------------+---------------+   +------+-----------+---------------------+  +------------
        // | 25.0000 |  1    18  |  25.0000   450.0000  |   0  450.0000 |   |   1  |  450.0000 |   0.000% ( 450.000) |  | 0x00000212
        // | 24.0000 |  4    75  |   6.0000   450.0000  |   0  450.0000 |   |   1  |  450.0000 |   0.000% ( 450.000) |  | 0x0000084b
        Wr( HHI_SYS_PLL_CNTL, 0x0000084b);   

        // -------------------------
        // set "MISC" PLL 
        // -------------------------
        //  +------------------------------------------------------------+   +----------------------------------------+
        //  |                      <<< PLL >>>                           |   |         <<< Clock Reset Test >>>       |
        //  +---------+-----------+----------------------+---------------+   +------+-----------+---------------------+  +------------
        //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      |   | CRT  |    Final  |               Ideal |  |
        //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     |   |  XD  |    Clock  |    Error      Clock |  |  HIU REG
        //  +---------+-----------+----------------------+---------------+   +------+-----------+---------------------+  +------------
        //  | 25.0000 |  8   169  |   3.1250   528.1250  |   0  528.1250 |   |   3  |  176.0417 |   0.024% ( 176.000) |  | 0x000010a9
        //  | 24.0000 |  1    22  |  24.0000   528.0000  |   0  528.0000 |   |   3  |  176.0000 |   0.000% ( 176.000) |  | 0x00000216
        Wr( HHI_OTHER_PLL_CNTL, 0x00000216);   

        // -------------------------
        // set CLK81 to "MISC" / 3
        // -------------------------
        // wire    [2:0]   cntl_hi_mpeg_clk_sel        = hi_mpeg_clk_cntl[14:12];
        // wire            cntl_hi_mpeg_oscin_sel_n    = hi_mpeg_clk_cntl[8];  // Set this high to mux from oscillator to PLL divider
        // wire            cntl_hi_mpeg_div_en         = hi_mpeg_clk_cntl[7];  // Set this high to mux from oscillator to PLL
        // wire    [6:0]   cntl_hi_mpeg_pll_xd         = hi_mpeg_clk_cntl[6:0];
        Wr( HHI_MPEG_CLK_CNTL, ((Rd(HHI_MPEG_CLK_CNTL) & ~((0x7 << 12) | (1 << 7) | (0x7F << 0))) | ((2 << 12) |   // select other PLL
                                                                                                   (1 << 7)  |   // enable the divider
                                                                                                   (2 << 0))) );  // divide by 3
    } else {
        // TODO:  Do we ever override? ever?
        // Bits[16]     XD value
        // Bits[15:0]   PLL M,N data 
        Wr( HHI_SYS_PLL_CNTL, (0 << 15) | mpeg_hiu_pll_reg );   
        Wr( HHI_MPEG_CLK_CNTL, (Rd(HHI_MPEG_CLK_CNTL) & ~(0xFF << 0)) | (1 << 7) | (mpeg_hiu_pll_xd << 0) );
    }


    // -----------------------------------------
    // Connect clk81 to the PLL divider output
    // -----------------------------------------
    Wr( HHI_MPEG_CLK_CNTL, Rd(HHI_MPEG_CLK_CNTL) | (1 << 8) );  

    // -----------------------------------------
    // Connect SYS CPU to the PLL divider output
    // -----------------------------------------
    //        .cntl_scale_pll_sel         ( hi_sys_cpu_clk_cntl[1:0]      ),  // 0 = oscin, 1 = clk1, 2 = clk2, 3 = clk3
    //        .cntl_scale_out_sync_sel    ( hi_sys_cpu_clk_cntl[7]        ),  // 0 = oscin, 1 = scale out
    Wr( HHI_SYS_CPU_CLK_CNTL, (Rd(HHI_SYS_CPU_CLK_CNTL) & ~(0x3 << 0)) | (0x1 << 0) );  
    Wr( HHI_SYS_CPU_CLK_CNTL, Rd(HHI_SYS_CPU_CLK_CNTL) | (1 << 7) );  

    // -----------------------------------------
    // HDMI (90Mhz)
    // -----------------------------------------
    //         .clk_div            ( hi_hdmi_clk_cntl[6:0] ),
    //         .clk_en             ( hi_hdmi_clk_cntl[8]   ),
    //         .clk_sel            ( hi_hdmi_clk_cntl[11:9]),
    Wr( HHI_HDMI_CLK_CNTL,  ((2 << 9)  |   // select "misc" PLL
                             (1 << 8)  |   // Enable gated clock
                             (5 << 0)) );  // Divide the "other" PLL output by 6

    // -----------------------------------------
    // ETHERNET 
    // -----------------------------------------
    //        .clk_div            ( hi_eth_clk_cntl[6:0]  ),
    //        .clk_en             ( hi_eth_clk_cntl[8]    ),
    //        .clk_sel            ( hi_eth_clk_cntl[11:9] ),
    Wr( HHI_ETH_CLK_CNTL,   ((3 << 9)  |   // select DDR pll
                             (1 << 8)  |   // Enable gated clock
                             (7 << 0)) );  // Divide the "400Mhz" PLL output by 8

    // -----------------------------------------
    // Enable the Audio PLL
    // -----------------------------------------
    //  ---------+-----------+----------------------+----------+-----------+-------+------------------------------+---------
    //           | PLL Ports |      PLL        PLL  | PLL Div  |      PLL  |  CRT  |    Final              Ideal  | HIU Reg 
    //     Fin   |  N    M   |     Fref        VCO  |  XD  OD  |      FOUT |  DIV  |    Clock    Error     Clock  |
    //  ---------+-----------+----------------------+----------+-----------+-------+------------------------------+---------
    // | 25.0000 |  5    81  |   5.0000   405.0000  |   1   1  |  202.5000 |    5  |   40.500   0.000% (  40.500) | 0x00004a51
    // | 24.0000 |  1    27  |  24.0000   648.0000  |   1  324.0000 |   |   8  |   40.5000 |   0.000% (  40.500) |  | 0x0000421b

    Wr( HHI_AUD_PLL_CNTL, 0x0000421b );     // divide by 5 by default
    Wr( HHI_AUD_CLK_CNTL, (Rd(HHI_AUD_CLK_CNTL) & ~(0xFF << 0)) | (7 << 0) );

#if 0  // video clock will be set seperately, no default setting anymore
    // -----------------------------------------
    // Set clock muxing inside crt_video()
    // -----------------------------------------
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   2  540.0000 | |     1   |   1.0 | |   5  |  108.0000 |   0.000% |  | 0x000114d8 | 0x00000803 | 0x00000004 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |     5   |   1.0 | |   1  |  108.0000 |   0.000% |  | 0x0001042d | 0x00010843 | 0x00000000 |
    Wr( HHI_VID_PLL_CNTL,     0x0001042d );  
    Wr( HHI_VID_DIVIDER_CNTL, 0x00010843 );
    Wr( HHI_VID_CLK_DIV,      0x00000000 );
    rd_data = Rd( HHI_VID_CLK_CNTL );
    rd_data = rd_data | (1 << 19);                      // [19] enable the divider
    rd_data = (rd_data & ~(0x3 << 11)) | (0x1 << 11);   // [12:11] = 2'h1;                 // select 54Mhz
    rd_data = (rd_data & ~(0x3 << 8))  | (0x1 << 8);    // [9:8] = 2'h1;                 // select 54Mhz
    rd_data = (rd_data & ~(0x3 << 6))  | (0x1 << 6);    // [7:6] = 2'h1;                 // select 54Mhz
    rd_data = (rd_data & ~(0x3 << 4))  | (0x2 << 4);    // [5:4] = 2'h2; // select 27Mhz
    Wr( HHI_VID_CLK_CNTL, rd_data );  

    // Disable PLL Sleep
    Wr( HHI_VID_PLL_CNTL, (Rd(HHI_VID_PLL_CNTL) & ~(1 << 15)) );   

    // -----------------------------------------
    // MUX Away from SCLK_XIN
    // -----------------------------------------
    // mux the Video PLL away from SCLK_XIN to the Media PLL
    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) | (1 << 10) );  
#endif

    // In the case of the Audio domain, simply remove the clock gating
    // At this point the PLL clock should be stable
    Wr( HHI_AUD_CLK_CNTL, Rd(HHI_AUD_CLK_CNTL) | ((1 << 23) | (1 << 8)) );      // gate audac_clkpi and amclk

    // ---------------------------------------------------
    // Enable MPEG Spread Spectrum
    // ---------------------------------------------------
    // data16[15:12] = 4'h8;       // CMOD_CLK_SEL: 8 = 31.5khz, 2 = 140.6khz
    // data16[9:6]   = 4'hF;       // PLL_CMOD_CTR: 1 = 0.625%, 15 = 5%
    Wr( HHI_SYS_PLL_CNTL2, (Rd(HHI_SYS_PLL_CNTL2) & ~((0xf << 12) | (0xf << 6))) | (8 << 12) | (0xf << 6) );
    Wr( HHI_SYS_PLL_CNTL3, Rd(HHI_SYS_PLL_CNTL3) | (1 << 8) );    //  SSEN

}

// --------------------------------------------------
//              clocks_set_hdtv
// --------------------------------------------------
void clocks_set_hdtv()
{
    clocks_set_sys_defaults();    // set MPEG, audio and default video
    clk_util_clocks_set_video_other( (unsigned long)148.5, 1, 2);   // vclk3 = 148.5, vclk2 = 148.5, vclk3 = 74.25
}
// --------------------------------------------------
//              clk_util_clocks_set_video_other
// --------------------------------------------------
// Many of Xiaoming's tests (test800) have 
// vclk, clk54 and clk27 set to different (but known)
// frequencies.  The task below accepts the highest
// frequency (which is assigned to vclk (vclk3)) 
// and a ratio of vclk3 to vclk2 (clk54) 
// and a ratio of vclk3 to vclk1 (clk27) 
// 
// Example  task(108,2,4) sets  vclk3 to 108Mhz, 
//                              vclk2 = 108/2 = 54
//                              vclk1 = 108/4 = 27
void clk_util_clocks_set_video_other(unsigned long vclk3_freq, unsigned long vclk2_ratio, unsigned long vclk1_ratio)
{
    unsigned long   rdata;

    // -----------------------------------------
    // Set Video Clocks to HDTV mode
    // -----------------------------------------

    // Mux the Video PLL to SCLK_XIN
    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );   // Switch to the SCLK

    rdata = Rd(HHI_VID_CLK_CNTL);
    // Now that we've muxed to the oscillator
    // Mux the clocks to the appropriate source
    rdata = (rdata & ~(0x3 << 8)) | (0 << 8);   // data16[9:8] = 2'h0;                 // select vclk3 = desired clock frequency
    // data16[7:6] = (vclk2_ratio == 2) ? 2'h1 : (vclk2_ratio == 4) ? 2'h2 : 2'h0;  // select vclk2 mux
    rdata = (rdata & ~(0x3 << 6)) | ((vclk2_ratio == 2) ? (1 << 6) : (vclk2_ratio == 4) ? (2 << 6) : (0 << 6));

    // data16[5:4] = (vclk1_ratio == 2) ? 2'h1 : (vclk1_ratio == 4) ? 2'h2 : 2'h0;  // select vclk1 mux
    rdata = (rdata & ~(0x3 << 4)) | ((vclk1_ratio == 2) ? (1 << 4) : (vclk1_ratio == 4) ? (2 << 4) : (0 << 4));
    Wr( HHI_VID_CLK_CNTL, rdata);

    // Set the Video Clock PLL based on the vclk3 parameter
    switch( vclk3_freq ) {
        //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
        //  |                                                            | |  Video Divider  | |                             |
        //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
        //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
        //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
        //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
        //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
        //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
        //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
        //  | 24.0000 |  2    99  |  12.0000  1188.0000  |   4  297.0000 | |     1   |   1.0 | |   2  |  148.5000 |   0.000% |  | 0x00020863 | 0x00010803 | 0x00000001 |
        case 148: 
        case 149: clk_util_set_video_clock( 0x00020863, 0x00010803, 2 );    
                  break;
        // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
        // |                                                            | |  Video Divider  | |                             |
        // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
        // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
        // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
        // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
        // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
        // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
        // | 25.0000 |  5   249  |   5.0000  1245.0000  |   2  622.5000 | |     5   |   1.0 | |   1  |  124.5000 |   0.000% |  | 0x000114f9 | 0x00010843 | 0x00000000 |
        // | 24.0000 |  2    83  |  12.0000   996.0000  |   4  249.0000 | |     1   |   1.0 | |   2  |  124.5000 |   0.000% |  | 0x00020853 | 0x00010803 | 0x00000001 |
        case 124: 
        case 125: clk_util_set_video_clock( 0x00020853, 0x00010803, 2 );
                  break;
        // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
        // |                                                            | |  Video Divider  | |                             |
        // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
        // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
        // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
        // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
        // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
        // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
        // | 25.0000 |  5   216  |   5.0000  1080.0000  |   2  540.0000 | |     1   |   1.0 | |   5  |  108.0000 |   0.000% |  | 0x000114d8 | 0x00010803 | 0x00000004 |
        // | 24.0000 |  1    36  |  24.0000   864.0000  |   4  216.0000 | |     1   |   1.0 | |   2  |  108.0000 |   0.000% |  | 0x00020424 | 0x00010803 | 0x00000001 |
        case 108: clk_util_set_video_clock( 0x00020424, 0x00010803, 2 );
                  break;

        // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
        // |                                                            | |  Video Divider  | |                             |
        // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
        // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
        // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
        // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
        // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
        // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
        // | 25.0000 |  5   387  |   5.0000  1935.0000  |   2  967.5000 | |     5   |   1.0 | |   1  |  193.5000 |   0.000% |  | 0x00011583 | 0x00010843 | 0x00000000 |
        case 193:
        case 194: clk_util_set_video_clock( 0x00011583, 0x00010843, 1);         
                break;
    }

}


// -----------------------------------------------------------------
// clk_util_set_audio_clock()
//
// This routine sets the clock "cts_amclk" inside clk_rst_tst.
// Clocks derived from "cts_amclk" are handled separately.
void clk_util_set_audio_clock( unsigned long hiu_reg, unsigned long xd )
{
    // gate the clock off
    Wr( HHI_AUD_CLK_CNTL, Rd(HHI_AUD_CLK_CNTL) & ~(1 << 8));

    // Put the PLL to sleep
    Wr( HHI_AUD_PLL_CNTL, Rd(HHI_AUD_PLL_CNTL) | (1 << 15));

    // Bring out of reset but keep bypassed to allow to stablize
    //Wr( HHI_AUD_PLL_CNTL, (1 << 15) | (0 << 14) | (hiu_reg & 0x3FFF) );
    Wr( HHI_AUD_PLL_CNTL, (1 << 15) | (hiu_reg & 0x7FFF) );
    // Set the XD value
    Wr( HHI_AUD_CLK_CNTL, (Rd(HHI_AUD_CLK_CNTL) & ~(0xff << 0)) | xd);
    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    // Bring the PLL out of sleep
    Wr( HHI_AUD_PLL_CNTL, Rd(HHI_AUD_PLL_CNTL) & ~(1 << 15));

    // gate the clock on
    Wr( HHI_AUD_CLK_CNTL, Rd(HHI_AUD_CLK_CNTL) | (1 << 8));
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    
}


// -----------------------------------------------------------------
// clk_util_set_video_clock()
// 
// This function sets the "master clock" in the video clock
// module $clk_rst_tst/rtl/crt_video
//
// wire            cntl_sclk_n     = hi_video_clk_cntl[10];
// wire    [1:0]   cntl_vclk3_mux  = hi_video_clk_cntl[9:8];
// wire    [1:0]   cntl_vclk2_mux  = hi_video_clk_cntl[7:6];
// wire    [1:0]   cntl_vclk1_mux  = hi_video_clk_cntl[5:4];
// wire    [3:0]   cntl_xd         = hi_video_clk_cntl[3:0];
void clk_util_set_video_clock( unsigned long pll_reg, unsigned long vid_div_reg, unsigned long xd )
{
    unsigned int tmp;
    // switch video clock to sclk_n and enable the PLL
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );   // Switch to the SCLK
    tmp = Rd(HHI_VID_CLK_CNTL) & (1 << 20);
    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    // Disable the master clock
    Wr( HHI_VID_CLK_DIV,  Rd(HHI_VID_CLK_DIV) & ~(1 << 16) );       // disable

    // Turn off the clock before switching the PLL
    
    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    
    // Bring out of reset but keep bypassed to allow to stablize
    //Wr( HHI_VID_PLL_CNTL, (0 << 15) | (0 << 14) | (pll_reg & 0x3FFF) );    
    Wr( HHI_VID_PLL_CNTL,       pll_reg );    
    Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    // Setup external divider in $clk_rst_tst/rtl/crt_video.v
    // wire    [3:0]   cntl_div_by_1       = hi_vid_clk_cntl[3:0];     // for LVDS we can divide by 1 (pll direct)
    // wire            cntl_vclk_gate      = hi_vid_clk_cntl[0];       // needed to mux divide by 1
    // 
    // wire    [7:0]   cntl_xd             = hi_vid_clk_div[7:0];

    //if( xd == 1 ) {
    //    // Set divide by 1 in crt_video
    //    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) | (0xF << 0) );
    //} else {
    //    Wr( HHI_VID_CLK_CNTL, (Rd(HHI_VID_CLK_CNTL) & ~(0xF << 0)) );               // disable divide by 1 control
    //}
    Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) | (1 << 10) );   // Switch to the PLL
    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) | tmp);     //change back clk_div1 
    // Eanble the master clock
    Wr( HHI_VID_CLK_DIV,  Rd(HHI_VID_CLK_DIV) | (1 << 16) );       // enable
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    
}

// --------------------------------------------------
//              clk_util_clocks_set_lcd240x160
// --------------------------------------------------
// Sets the phase clocks to 10Mhz

void clk_util_clocks_set_lcd240x160( unsigned long pll_divide) 
{
    unsigned long   rd_data;

    // First set clock defaults for all other clocks
    clocks_set_sys_defaults();

    // Mux the Video PLL to SCLK_XIN
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );  
    //
    // Set clock muxing inside venc_clk()
    // 
    rd_data = Rd( HHI_VID_CLK_CNTL );

    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (0 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (2 << 5)  |     //tcon high_thresh
                    (2 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);

    
    rd_data = (rd_data & ~(0x3 << 11)) | (0x3 << 11);   // [12:11] vclk3 = 10Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 8))  | (0x3 << 8);    // [9:8] vclk2 = 10Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 6))  | (0x3 << 6);    // [7:6] vclk1 = 60Mhz
    rd_data = (rd_data & ~(0x3 << 4))  | (0x0 << 4);    // [5:4] = 2'h2; // select 27Mhz
    Wr( HHI_VID_CLK_CNTL, rd_data );  

    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   1 1080.0000 | |     3   |   1.0 | |   1  |  360.0000 |   0.000% |  | 0x000014d8 | 0x00010823 | 0x00000000 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   1 1080.0000 | |     3   |   1.0 | |   1  |  360.0000 |   0.000% |  | 0x0000042d | 0x00010823 | 0x00000000 |
    clk_util_set_video_clock( 0x0000042d, 0x00010823, pll_divide );
    //set_video_pll( 16'h0328, 5 );
    //set_video_pll( 16'h0328, 2 );  // 360/3 = 120M
    // set_video_pll( 16'h0328, pll_divide_m1 );  // 360/(pll_divide_m1+1)

    Wr_reg_bits (HHI_VID_CLK_DIV,
                        pll_divide - 1, 
                        8, 8);
    
    Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
    Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset

}

// --------------------------------------------------
//         clk_util_clocks_set_lcd480x234
// --------------------------------------------------
// Sets the phase clocks to 10Mhz

void clk_util_clocks_set_lcd480x234()
{
    unsigned long   rd_data;

    // First set clock defaults for all other clocks
    clocks_set_sys_defaults();

    // Mux the Video PLL to SCLK_XIN
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );  

    rd_data = Rd( HHI_VID_CLK_CNTL );

    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (0 << 10) |     //clk_inv
                    (1 << 9)  |     //neg_edge_sel
                    (14 << 5)  |     //tcon high_thresh
                    (14 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);
    
    rd_data = (rd_data & ~(0x3 << 11)) | (0x3 << 11);   // data16[12:11] = 2'h3;               // select dclk = 10Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 8))  | (0x3 << 8);    // data16[9:8] = 2'h3;                 // select vclk3 = 10Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 6))  | (0x3 << 6);    // data16[7:6] = 2'h3;                 // select vclk2 = 10Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 4))  | (0x0 << 4);    // data16[5:4] = 2'h0;                 // select vclk1 = 60Mhz
    Wr( HHI_VID_CLK_CNTL, rd_data );  

    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  1    48  |  25.0000  1200.0000  |   4  300.0000 | |     1   |   1.0 | |   5  |   60.0000 |   0.000% |  | 0x00020430 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    35  |  24.0000   840.0000  |   2  420.0000 | |     1   |   1.0 | |   7  |   60.0000 |   0.000% |  | 0x00010423 | 0x00010803 | 0x00000006 |
    clk_util_set_video_clock( 0x00010423, 0x00010803, 7 );

    Wr_reg_bits (HHI_VID_CLK_DIV,
                        0,
                        8, 8);
    
    Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
    Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset

}

// --------------------------------------------------
//         clk_util_clocks_set_lcd1024x768
// --------------------------------------------------
// Sets the phase clocks to 78.64Mhz

void clk_util_clocks_set_lcd1024x768()
{
    unsigned long   rd_data;

    // First set clock defaults for all other clocks
    clocks_set_sys_defaults();

    // Mux the Video PLL to SCLK_XIN
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );  
    //
    // Set clock muxing inside venc_clk()
    // 
    rd_data = Rd( HHI_VID_CLK_CNTL );

    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (1 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (0 << 5)  |     //tcon high_thresh
                    (0 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);

    
    rd_data = (rd_data & ~(0x3 << 11)) | (0x0 << 11);   // [12:11] = 2'h0;               // select dclk = 78.6Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 8))  | (0x0 << 8);    // [9:8] = 2'h0;                 // select vclk3 = 78.6Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 6))  | (0x0 << 6);    // [7:6] = 2'h0;                 // select vclk2 = 78.6Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 4))  | (0x0 << 4);    // [5:4] = 2'h0;                 // select vclk1 = 78.6Mhz
    Wr( HHI_VID_CLK_CNTL, rd_data );  

    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  3   151  |   8.3333  1258.3333  |   4  314.5833 | |     1   |   1.0 | |   4  |   78.6458 |   0.007% |  | 0x00020c97 | 0x00010803 | 0x00000003 |
    // | 24.0000 |  1    59  |  24.0000  1416.0000  |   2  708.0000 | |     3   |   1.0 | |   3  |   78.6667 |   0.034% |  | 0x0001043b | 0x00010823 | 0x00000002 |
    clk_util_set_video_clock( 0x0001043b, 0x00010823, 3 );

    Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
    Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
    Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
}

void video_clocks_set_lcd1024x768()
{
    unsigned long   rd_data;

    // Mux the Video PLL to SCLK_XIN
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );  
    //
    // Set clock muxing inside venc_clk()
    // 

    // Mux the Video PLL to sys_oscin
    Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 10, 1); 

    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (1 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (0 << 5)  |     //tcon high_thresh
                    (0 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);

    rd_data = Rd( HHI_VID_CLK_CNTL );

    
    rd_data = (rd_data & ~(0x3 << 11)) | (0x0 << 11);   // [12:11] = 2'h0;               // select dclk = 78.6Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 8))  | (0x0 << 8);    // [9:8] = 2'h0;                 // select vclk3 = 78.6Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 6))  | (0x0 << 6);    // [7:6] = 2'h0;                 // select vclk2 = 78.6Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 4))  | (0x0 << 4);    // [5:4] = 2'h0;                 // select vclk1 = 78.6Mhz
    Wr( HHI_VID_CLK_CNTL, rd_data );  

    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  3   151  |   8.3333  1258.3333  |   4  314.5833 | |     1   |   1.0 | |   4  |   78.6458 |   0.007% |  | 0x00020c97 | 0x00010803 | 0x00000003 |
    clk_util_set_video_clock( 0x00020c97, 0x00010803, 4 );

    Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
    Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
    Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
}

// --------------------------------------------------
//              clocks_set_lcd1280x1024
// --------------------------------------------------
// Sets the phase clocks to 128.25 Mhz

void clocks_set_lcd1280x1024()
{
    unsigned long   rd_data;

    // First set clock defaults for all other clocks
    clocks_set_sys_defaults();

    // Mux the Video PLL to SCLK_XIN
    //Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 10) );  

    //
    // Set clock muxing inside venc_clk()
    // 
    rd_data = Rd( HHI_VID_CLK_CNTL );

    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (1 << 11) |     //clk_div1_sel
                    (0 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (0 << 5)  |     //tcon high_thresh
                    (0 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);

    rd_data = (rd_data & ~(0x3 << 11)) | (0x0 << 11);   // [12:11] = 2'h0;               // select dclk = 128.0Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 8))  | (0x0 << 8);    // [9:8] = 2'h0;                 // select vclk3 = 128.0Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 6))  | (0x0 << 6);    // [7:6] = 2'h0;                 // select vclk2 = 128.0Mhz (ph1)
    rd_data = (rd_data & ~(0x3 << 4))  | (0x0 << 4);    // [5:4] = 2'h0;                 // select vclk1 = 128.0Mhz
    Wr( HHI_VID_CLK_CNTL, rd_data );  

    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  3   154  |   8.3333  1283.3333  |   2  641.6667 | |     5   |   1.0 | |   1  |  128.3333 |   0.065% |  | 0x00010c9a | 0x00010843 | 0x00000000 |
    // | 24.0000 |  4   171  |   6.0000  1026.0000  |   4  256.5000 | |     1   |   1.0 | |   2  |  128.2500 |   0.000% |  | 0x000210ab | 0x00010803 | 0x00000001 |
    clk_util_set_video_clock( 0x000210ab, 0x00010803, 2 );

    Wr_reg_bits (HHI_VID_CLK_DIV,
                        0,          //divide 1 for clk_div1
                        8, 8);
    
    Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
    Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
}

// -----------------------------------------
// clk_util_lvds_set_clk_div()
// -----------------------------------------
// This task is used to setup the LVDS dividers.
// 
//        .video_pll_in           ( vid_pll_clk_pre           ),      // From the video PLL
//        .scan_clk               ( cts_scan_clk              ),
//        .scan_reset_n           ( chip_reset_n              ),
//        .test_mode              ( test_mode                 ),
//        .vid_div_reset_n_pre    ( hi_vid_divider_cntl[0]    ),
//        .vid_div_reset_n_post   ( hi_vid_divider_cntl[1]    ),
//        .vid_div_soft_reset_pre ( hi_vid_divider_cntl[3]    ),
//        .vid_div_soft_reset_post( hi_vid_divider_cntl[7]    ),
//        .vid_div_pre_sel        ( hi_vid_divider_cntl[6:4]  ),      // associate pre-divider with PLL controls  
//        .vid_div_m_test_ck      ( prod_test_tm[11]          ), 
//        .vid_div_m_test         ( hi_vid_divider_cntl[2]    ),
//        .vid_div_post_sel       ( hi_vid_divider_cntl[9:8]  ),
//        .vid_div_post_tcnt      ( hi_vid_divider_cntl[14:12]),
//        .vid_div_lvds_div2      ( hi_vid_divider_cntl[10]   ),
//        .vid_div_lvds_clk_en    ( hi_vid_divider_cntl[11]   ),
//        .vid_div_video_clk_out  ( vid_pll_clk               ),      // To clk_rst_tst 
//        .mlvds_div_half_dly     ( mlvds_clk_ctl[24]         ),
//        .mlvds_div_pattern      ( mlvds_clk_ctl[23:0]       ),
void    clk_util_lvds_set_clk_div(  unsigned long   pre_divider,    // 1..6
                                    unsigned long   post_divider,   // 1..7  Use 99 for divide by 3.5
                                    unsigned long   phy_div2_en  )  // 1 = LVDS PHY divide by 2 enable, 0 = PHY divide by 1 
{
    // Set Pre divider
    Wr( HHI_VID_DIVIDER_CNTL, ((Rd(HHI_VID_DIVIDER_CNTL) & ~(0x7 << 4)) | ((pre_divider-1) << 4)) );

    // Set Post divider
    if( post_divider == 1 ) {
        Wr( HHI_VID_DIVIDER_CNTL, ((Rd(HHI_VID_DIVIDER_CNTL) & ~(0x3 << 8)) | (0 << 8)) );
    } else if( post_divider == 99) {
        Wr( HHI_VID_DIVIDER_CNTL, ((Rd(HHI_VID_DIVIDER_CNTL) & ~(0x3 << 8)) | (2 << 8)) );
    } else {
        // set the divider count
        Wr( HHI_VID_DIVIDER_CNTL, ((Rd(HHI_VID_DIVIDER_CNTL) & ~(0x7 << 12)) | ((post_divider-1) << 12)) );
        Wr( HHI_VID_DIVIDER_CNTL, ((Rd(HHI_VID_DIVIDER_CNTL) & ~(0x3 << 8)) | (1 << 8)) );
    }
    
    // Set the PHY clock divider select
    Wr( HHI_VID_DIVIDER_CNTL, ((Rd(HHI_VID_DIVIDER_CNTL) & ~(1 << 10)) | (1 << 11) | (phy_div2_en << 10)) );
}
#endif

// -----------------------------------------
// clk_util_clk_msr
// -----------------------------------------
// from twister_core.v
//        .clk_to_msr_in          ( { 18'h0,                      // [63:46]
//                                    cts_pwm_A_clk,              // [45]
//                                    cts_pwm_B_clk,              // [44]
//                                    cts_pwm_C_clk,              // [43]
//                                    cts_pwm_D_clk,              // [42]
//                                    cts_eth_rx_tx,              // [41]
//                                    cts_pcm_mclk,               // [40]
//                                    cts_pcm_sclk,               // [39]
//                                    cts_vdin_meas_clk,          // [38]
//                                    cts_vdac_clk[1],            // [37]
//                                    cts_hdmi_tx_pixel_clk,      // [36]
//                                    cts_mali_clk,               // [35] 
//                                    cts_sdhc_clk1,              // [34]
//                                    cts_sdhc_clk0,              // [33]
//                                    cts_audac_clkpi,            // [32] 
//                                    cts_a9_clk,                 // [31]
//                                    cts_ddr_clk,                // [30]
//                                    cts_vdac_clk[0],            // [29]
//                                    cts_sar_adc_clk,            // [28]
//                                    cts_enci_clk,               // [27]
//                                    sc_clk_int,                 // [26]
//                                    usb_clk_12mhz,              // [25]
//                                    lvds_fifo_clk,              // [24]
//                                    HDMI_CH3_TMDSCLK,           // [23]
//                                    mod_eth_clk50_i,            // [22]
//                                    mod_audin_amclk_i,          // [21]
//                                    cts_btclk27,                // [20]
//                                    cts_hdmi_sys_clk,           // [19]
//                                    cts_led_pll_clk,            // [18]
//                                    cts_vghl_pll_clk,           // [17]
//                                    cts_FEC_CLK_2,              // [16]
//                                    cts_FEC_CLK_1,              // [15]
//                                    cts_FEC_CLK_0,              // [14]
//                                    cts_amclk,                  // [13]
//                                    vid2_pll_clk,               // [12]
//                                    cts_eth_rmii,               // [11]
//                                    cts_enct_clk,               // [10]
//                                    cts_encl_clk,               // [9]
//                                    cts_encp_clk,               // [8]
//                                    clk81,                      // [7]
//                                    vid_pll_clk,                // [6]
//                                    aud_pll_clk,                // [5]
//                                    misc_pll_clk,               // [4]
//                                    ddr_pll_clk,                // [3]
//                                    sys_pll_clk,                // [2]
//                                    am_ring_osc_clk_out[1],     // [1]
//                                    am_ring_osc_clk_out[0]} ),  // [0]
//
// For Example
//
// unsigend long    clk81_clk   = clk_util_clk_msr( 2,      // mux select 2
//                                                  50 );   // measure for 50uS
//
// returns a value in "clk81_clk" in Hz 
//
// The "uS_gate_time" can be anything between 1uS and 65535 uS, but the limitation is
// the circuit will only count 65536 clocks.  Therefore the uS_gate_time is limited by
//
//   uS_gate_time <= 65535/(expect clock frequency in MHz) 
//
// For example, if the expected frequency is 400Mhz, then the uS_gate_time should 
// be less than 163.
//
// Your measurement resolution is:
//
//    100% / (uS_gate_time * measure_val )
//
//
// #define MSR_CLK_DUTY                               0x21d6
// #define MSR_CLK_REG0                               0x21d7
// #define MSR_CLK_REG1                               0x21d8
// #define MSR_CLK_REG2                               0x21d9
//

/*
STATIC_PREFIX unsigned long    clk_util_clk_msr_1(unsigned long   clk_mux)
{
    unsigned long   measured_val;
    unsigned long   uS_gate_time=64;
    writel(0,P_MSR_CLK_REG0);
    // Set the measurement gate to 64uS
    clrsetbits_le32(P_MSR_CLK_REG0,0xffff,(uS_gate_time-1));
    // Disable continuous measurement
    // disable interrupts
    clrbits_le32(P_MSR_CLK_REG0,(1 << 18) | (1 << 17));
//    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~((1 << 18) | (1 << 17))) );
    clrsetbits_le32(P_MSR_CLK_REG0,
        (0xf << 20)|(1<<19)|(1<<16),
        (clk_mux<<20) |(1<<19)|(1<<16));
    { unsigned long dummy_rd = readl(P_MSR_CLK_REG0); }
    // Wait for the measurement to be done
    while( (readl(P_MSR_CLK_REG0) & (1 << 31)) ) {} 
    // disable measuring
    clrbits_le32(P_MSR_CLK_REG0, 1<<16 );

    measured_val = readl(P_MSR_CLK_REG2)&0x000FFFFF;
    // Return value in Hz*measured_val
    // Return Mhz
    return (measured_val>>6);
    // Return value in Hz*measured_val
}
*/

unsigned long    clk_util_clk_msr(unsigned long   clk_mux, unsigned long   uS_gate_time)
{
    unsigned long   measured_val;
    writel(0,P_MSR_CLK_REG0);
    // Set the measurement gate to 64uS
    clrsetbits_le32(P_MSR_CLK_REG0,0xffff,(uS_gate_time-1));
    // Disable continuous measurement
    // disable interrupts
    clrbits_le32(P_MSR_CLK_REG0,(1 << 18) | (1 << 17));
//    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~((1 << 18) | (1 << 17))) );
    clrsetbits_le32(P_MSR_CLK_REG0,
        (0xf << 20)|(1<<19)|(1<<16),
        (clk_mux<<20) |(1<<19)|(1<<16));
    // Wait for the measurement to be done
    while( (readl(P_MSR_CLK_REG0) & (1 << 31)) ) {} 
    // disable measuring
    clrbits_le32(P_MSR_CLK_REG0, 1<<16 );

    measured_val = readl(P_MSR_CLK_REG2)&0x000FFFFF;
    // Return value in Hz*measured_val
    // Return Mhz
    return (measured_val>>6);
    // Return value in Hz*measured_val
}
/*
unsigned long    clk_util_clk_msr(   unsigned long   clk_mux, unsigned long   uS_gate_time )
{
    // Set the measurement gate to 100uS
    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~(0xFFFF << 0)) | ((uS_gate_time-1) << 0) );
    // Disable continuous measurement
    // disable interrupts
    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~((1 << 18) | (1 << 17))) );
    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~(0x1f << 20)) | ((clk_mux << 20) |  // Select MUX 
                                                          (1 << 19) |     // enable the clock
                                                          (1 << 16)) );    // enable measuring
    // Delay
    Rd(MSR_CLK_REG0); 
    // Wait for the measurement to be done
    while( (Rd(MSR_CLK_REG0) & (1 << 31)) ) {} 
    // disable measuring
    Wr(MSR_CLK_REG0, (Rd(MSR_CLK_REG0) & ~(1 << 16)) | (0 << 16) );

    unsigned long   measured_val = (Rd(MSR_CLK_REG2) & 0x000FFFFF); // only 20 bits

    if( measured_val == 0xFFFFF ) {     // 20 bits max
        return(0);
    } else {
        // Return value in Hz
        return(measured_val*(1000000/uS_gate_time));
    }
}
*/

#if 0
void vclk_set_lcd( int pll_sel, int pll_div_sel, int vclk_sel, 
                   unsigned long pll_reg, unsigned long vid_div_reg, unsigned int xd)
{

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    else Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 20) );     //enable clk_div1 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    



    // set tcon_clko setting
    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (1 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (0 << 5)  |     //tcon high_thresh
                    (0 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);


    Wr_reg_bits (HHI_VID_CLK_DIV, 
                 0,      // select clk_div1 
                 20, 4); // [23:20] enct_clk_sel 

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
}

void vclk_set_lcd_lvds( int lcd_lvds, int pll_sel, int pll_div_sel, int vclk_sel, 
                   unsigned long pll_reg, unsigned long vid_div_reg, unsigned int xd)
{

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    else Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 20) );     //enable clk_div1 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    



    // set tcon_clko setting
    Wr_reg_bits (HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (1 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (0 << 5)  |     //tcon high_thresh
                    (0 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);


  if(lcd_lvds){
    Wr_reg_bits (HHI_VIID_CLK_DIV, 
                 8,      // select v2_clk_div1 
                 12, 4); // [23:20] encl_clk_sel 
  }
  else {
    Wr_reg_bits (HHI_VID_CLK_DIV, 
                 0,      // select clk_div1 
                 20, 4); // [23:20] enct_clk_sel 
  }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
}
#endif

#if 0
void vclk_set_lcd720x480( int pll_sel, int pll_div_sel, int vclk_sel)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   2  540.0000 | |     1   |   1.0 | |   5  |  108.0000 |   0.000% |  | 0x000114d8 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |     5   |   1.0 | |   1  |  108.0000 |   0.000% |  | 0x0001042d | 0x00010843 | 0x00000000 |
// +------------------------------------------------------------+
// |                                                            |
// |                      <<< PLL >>>                           |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// |         |           |                      |               | |         | |        PLL |
// |         | PLL Ports |      PLL        PLL  | PLL  PLL      | |         | |       CBUS |
// |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | |  Error  | |     0x1047 |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |  0.000% | | 0x0001022d |

    unsigned long pll_reg;
    unsigned long vid_div_reg = 0x00010803;
    unsigned int xd = 5; 

    pll_reg = pll_sel ? 0x0001022d : 0x000114d8;


    vclk_set_lcd(pll_sel, pll_div_sel, vclk_sel, pll_reg, vid_div_reg, xd);

}

void vclk_set_lcd480x234( int pll_sel, int pll_div_sel, int vclk_sel)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  1    48  |  25.0000  1200.0000  |   4  300.0000 | |     1   |   1.0 | |   5  |   60.0000 |   0.000% |  | 0x00020430 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    35  |  24.0000   840.0000  |   2  420.0000 | |     1   |   1.0 | |   7  |   60.0000 |   0.000% |  | 0x00010423 | 0x00010803 | 0x00000006 |

// +------------------------------------------------------------+
// |                                                            |
// |                      <<< PLL >>>                           |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// |         |           |                      |               | |         | |        PLL |
// |         | PLL Ports |      PLL        PLL  | PLL  PLL      | |         | |       CBUS |
// |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | |  Error  | |     0x1047 |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// | 24.0000 |  1    35  |  24.0000   840.0000  |   2  420.0000 | |  0.000% | | 0x00010223 |

    unsigned long pll_reg;
    unsigned long vid_div_reg = 0x00010803;
    unsigned int xd = 7; 

    pll_reg = pll_sel ? 0x00010223 : 0x00010423;

    vclk_set_lcd(pll_sel, pll_div_sel, vclk_sel, pll_reg, vid_div_reg, xd);

}

void vclk_set_lcd1024x768( int pll_sel, int pll_div_sel, int vclk_sel)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  3   151  |   8.3333  1258.3333  |   4  314.5833 | |     1   |   1.0 | |   4  |   78.6458 |   0.007% |  | 0x00020c97 | 0x00010803 | 0x00000003 |
//  +------------------------------------------------------------+
//  |                                                            |
//  |                      <<< PLL >>>                           |
//  +---------+-----------+----------------------+---------------+ +---------+ +------------+
//  |         |           |                      |               | |         | |        PLL |
//  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | |         | |       CBUS |
//  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | |  Error  | |     0x1047 |
//  +---------+-----------+----------------------+---------------+ +---------+ +------------+
//  | 25.0000 |  3   151  |   8.3333  1258.3333  |   4  314.5833 | |  0.000% | | 0x00020697 |

    unsigned long pll_reg = 0x00020c97;
    unsigned long vid_div_reg = 0x00010803;
    unsigned int xd = 4; 

    pll_reg = pll_sel ? 0x00020697 : 0x00020c97;

    vclk_set_lcd(pll_sel, pll_div_sel, vclk_sel, pll_reg, vid_div_reg, xd);

}

void vclk_set_lcd1280x1024( int pll_sel, int pll_div_sel, int vclk_sel)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  3   154  |   8.3333  1283.3333  |   2  641.6667 | |     5   |   1.0 | |   1  |  128.3333 |   0.065% |  | 0x00010c9a | 0x00010843 | 0x00000000 |
    // | 24.0000 |  4   171  |   6.0000  1026.0000  |   4  256.5000 | |     1   |   1.0 | |   2  |  128.2500 |   0.000% |  | 0x000210ab | 0x00010803 | 0x00000001 |

// +------------------------------------------------------------+
// |                                                            |
// |                      <<< PLL >>>                           |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// |         |           |                      |               | |         | |        PLL |
// |         | PLL Ports |      PLL        PLL  | PLL  PLL      | |         | |       CBUS |
// |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | |  Error  | |     0x1047 |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// | 24.0000 |  4   171  |   6.0000  1026.0000  |   4  256.5000 | |  0.000% | | 0x000208ab |

    unsigned long pll_reg;
    unsigned long vid_div_reg = 0x00010803;
    unsigned int xd = 2; 

    pll_reg = pll_sel ? 0x000208ab : 0x000210ab;

    vclk_set_lcd(pll_sel, pll_div_sel, vclk_sel, pll_reg, vid_div_reg, xd);

    
}

void vclk_set_lcd480x234_2( int pll_sel, int pll_div_sel, int vclk_sel, int pll_divide)
{

    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  1    48  |  25.0000  1200.0000  |   4  300.0000 | |     1   |   1.0 | |   5  |   60.0000 |   0.000% |  | 0x00020430 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    35  |  24.0000   840.0000  |   2  420.0000 | |     1   |   1.0 | |   7  |   60.0000 |   0.000% |  | 0x00010423 | 0x00010803 | 0x00000006 |
    clk_util_set_video_clock( 0x00010423, 0x00010803, 7 );

    unsigned long pll_reg = 0x00010423;
    unsigned long vid_div_reg = 0x00010803;
    unsigned int xd = 7; 

    vclk_set_lcd(pll_sel, pll_div_sel, vclk_sel, pll_reg, vid_div_reg, xd);

    Wr_reg_bits (HHI_VID_CLK_DIV, 
                 3,      // select clk_div6 
                 20, 4); // [23:20] enct_clk_sel 

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<3),  // Enable cntl_div6_en
                   0, 4    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<3),  // Enable cntl_div6_en
                   0, 4    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }

}


void vclk_set_lcd240x160( int pll_sel, int pll_div_sel, int vclk_sel, int pll_divide)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   1 1080.0000 | |     3   |   1.0 | |   1  |  360.0000 |   0.000% |  | 0x000014d8 | 0x00010823 | 0x00000000 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   1 1080.0000 | |     3   |   1.0 | |   1  |  360.0000 |   0.000% |  | 0x0000042d | 0x00010823 | 0x00000000 |
    unsigned long pll_reg;
    unsigned long vid_div_reg = 0x00010823;
    unsigned int xd = pll_divide; 
    
    pll_reg = pll_sel ? 0x0000022d : 0x0000042d;

    vclk_set_lcd(pll_sel, pll_div_sel, vclk_sel, pll_reg, vid_div_reg, xd);

    Wr_reg_bits (HHI_VID_CLK_DIV, 
                 3,      // select clk_div6 
                 20, 4); // [23:20] enct_clk_sel 

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<3),  // Enable cntl_div6_en
                   0, 4    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<3),  // Enable cntl_div6_en
                   0, 4    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }

}



void vclk_set_enci_720x480( int pll_sel, int pll_div_sel, int vclk_sel, int upsample)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   2  540.0000 | |     1   |   1.0 | |   5  |  108.0000 |   0.000% |  | 0x000114d8 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |     5   |   1.0 | |   1  |  108.0000 |   0.000% |  | 0x0001042d | 0x00010843 | 0x00000000 |
    unsigned long pll_reg;
    unsigned long vid_div_reg = 0x00010843;
    unsigned int xd = 1; 

    pll_reg = pll_sel ? 0x0001022d : 0x0001042d;

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) {
      Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    }
    else {
      Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    }

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [27:24] encp_clk_sel 
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [27:24] encp_clk_sel 
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    Wr_reg_bits (HHI_VID_CLK_DIV, 
                   2|(vclk_sel<<3),      // select clk_div4 
                   28, 4);               // [31:28] enci_clk_sel 

    if(upsample) {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0x111|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div2 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }
    else {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0x111|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div2 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (6<<0),  // Enable cntl_div4_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (6<<0),  // Enable cntl_div4_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
}

void vclk_set_enci_720x576( int pll_sel, int pll_div_sel, int vclk_sel, int upsample)
{
  vclk_set_enci_720x480( pll_sel, pll_div_sel, vclk_sel, upsample);
}



void vclk_set_encp_720x480( int pll_sel, int pll_div_sel, int vclk_sel, int upsample)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   2  540.0000 | |     1   |   1.0 | |   5  |  108.0000 |   0.000% |  | 0x000114d8 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |     5   |   1.0 | |   1  |  108.0000 |   0.000% |  | 0x0001042d | 0x00010843 | 0x00000000 |
// +------------------------------------------------------------+
// |                                                            |
// |                      <<< PLL >>>                           |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// |         |           |                      |               | |         | |        PLL |
// |         | PLL Ports |      PLL        PLL  | PLL  PLL      | |         | |       CBUS |
// |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | |  Error  | |     0x1047 |
// +---------+-----------+----------------------+---------------+ +---------+ +------------+
// | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |  0.000% | | 0x0001022d |
          
    unsigned long pll_reg;
    unsigned long vid_div_reg = 0x00010843;
    unsigned int xd = 1; 

    pll_reg = pll_sel ? 0x0001022d : 0x0001042d;

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) {
      Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    }
    else {
      Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    }

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    Wr_reg_bits (HHI_VID_CLK_DIV, 
                   1|(vclk_sel<<3),      // select clk_div2 
                   24, 4); // [23:20] encp_clk_sel 

    if(upsample) {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div1 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }
    else {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0x111|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div2 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
}


void vclk_set_encp_720x576( int pll_sel, int pll_div_sel, int vclk_sel, int fast_clock)
{
    // +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    // |                                                            | |  Video Divider  | |                             |
    // |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    // +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    // |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    // |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    // |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    // +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    // | 25.0000 |  5   216  |   5.0000  1080.0000  |   2  540.0000 | |     1   |   1.0 | |   5  |  108.0000 |   0.000% |  | 0x000114d8 | 0x00010803 | 0x00000004 |
    // | 24.0000 |  1    45  |  24.0000  1080.0000  |   2  540.0000 | |     5   |   1.0 | |   1  |  108.0000 |   0.000% |  | 0x0001042d | 0x00010843 | 0x00000000 |
    unsigned long pll_reg = 0x0001042d;
    unsigned long vid_div_reg = 0x00010843;
    unsigned int xd = 1; 

    int upsample = 0;


    if(fast_clock){
        //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
        //  |                                                            | |  Video Divider  | |                             |
        //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
        //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
        //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
        //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
        //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
        //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
        //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
        //  | 24.0000 |  2    99  |  12.0000  1188.0000  |   4  297.0000 | |     1   |   1.0 | |   2  |  148.5000 |   0.000% |  | 0x00020863 | 0x00010803 | 0x00000001 |
        pll_reg = 0x00020863;
        vid_div_reg = 0x00010803;
        xd = 2;
    }

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) {
      Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    }
    else {
      Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    }

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0|(vclk_sel<<3),      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 

    if(upsample) {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div1 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }
    else {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div1 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
}
//------------------------
// pll_sel :
// 0=VID_PLL, 1=VIID_PLL
//
// pll_div_sel : 
// 0=VID_DIV, 1=VIID_DIV
//
// vclk_sel :
// 0=VID_PLL_CLK, 1=VIID_PLL_CLK
// 
// enc_sel : 
// 0=ENCL, 1=ENCI, 2=ENCP, 3=ENCT, -1=ALL
void vclk_set_enc_720x480( int pll_sel, int pll_div_sel, int vclk_sel, int enc_sel, int upsample)
{
    unsigned long pll_reg;
    unsigned long vid_div_reg;
    unsigned int xd; 

    if (pll_sel) { // Setting for VIID_PLL
    //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    //  |                                                            | |  Video Divider  | |                             |
    //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
    //  | 24.0000 |  2    99  |  12.0000  1188.0000  |   4  297.0000 | |     1   |   1.0 | |   2  |  148.5000 |   0.000% |  | 0x00020863 | 0x00010803 | 0x00000001 |
        pll_reg = 0x00020863;
        vid_div_reg = 0x00010803;
        xd = 2; 
    } else { // Setting for VID_PLL, consideration for supporting HDMI clock
    //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    //  |                                                            | |  Video Divider  | |                             |
    //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    //  | 24.0000 |  1    36  |  24.0000   864.0000  |   4  216.0000 | |     1   |   1.0 | |   8  |   27.0000 |   0.000% |  | 0x002a0424 | 0x00010803 |       0x07 |

     
        pll_reg = 0x0002a0424;
        vid_div_reg = 0x00010803;
        xd = 8; 
    }

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) {
      Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    }
    else {
      Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    }

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_I)
              Wr_reg_bits (HHI_VID_CLK_DIV, 
                           8,      // select clk_div1 
                           28, 4); // [23:20] encp_clk_sel 
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_P)
              Wr_reg_bits (HHI_VID_CLK_DIV, 
                           8,      // select clk_div1 
                           24, 4); // [23:20] encp_clk_sel 
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_T)
              Wr_reg_bits (HHI_VID_CLK_DIV, 
                           8,      // select clk_div1 
                           20, 4); // [23:20] encp_clk_sel 
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_L)
              Wr_reg_bits (HHI_VIID_CLK_DIV, 
                           8,      // select clk_div1 
                           12, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


//    Wr_reg_bits (HHI_VID_CLK_DIV, 
//                   1|(vclk_sel<<3),      // select clk_div2 
//                   24, 4); // [23:20] encp_clk_sel 

    if(upsample) {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div1 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }
    else {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0x111|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div2 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
} /* vclk_set_enc_720x480 */


//------------------------
// pll_sel :
// 0=VID_PLL, 1=VIID_PLL
//
// pll_div_sel : 
// 0=VID_DIV, 1=VIID_DIV
//
// vclk_sel :
// 0=VID_PLL_CLK, 1=VIID_PLL_CLK
// 
// enc_sel : 
// 0=ENCL, 1=ENCI, 2=ENCP, 3=ENCT, -1=ALL
void vclk_set_enc_1920x1080( int pll_sel, int pll_div_sel, int vclk_sel, int enc_sel, int upsample)
{
    unsigned long pll_reg;
    unsigned long vid_div_reg;
    unsigned int xd; 

    if (pll_sel) { // Setting for VIID_PLL
    //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    //  |                                                            | |  Video Divider  | |                             |
    //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
    //  | 24.0000 |  2    99  |  12.0000  1188.0000  |   4  297.0000 | |     1   |   1.0 | |   2  |  148.5000 |   0.000% |  | 0x00020863 | 0x00010803 | 0x00000001 |
        pll_reg = 0x00020863;
        vid_div_reg = 0x00010803;
        xd = 2; 
    } else { // Setting for VID_PLL, consideration for supporting HDMI clock
    //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    //  |                                                            | |  Video Divider  | |                             |
    //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
    //  | 24.0000 |  1    62  |  24.0000  1488.0000  |   2  744.0000 | |     5   |   1.0 | |   1  |  148.8000 |   0.202% |  | 0x0001043e | 0x00010843 | 0x00000000 |
     
        pll_reg = 0x0001043e;
        vid_div_reg = 0x00010843;
        xd = 1; 
    }

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) {
      Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    }
    else {
      Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    }

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_I)
              Wr_reg_bits (HHI_VID_CLK_DIV, 
                           8,      // select clk_div1 
                           28, 4); // [23:20] encp_clk_sel 
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_P)
              Wr_reg_bits (HHI_VID_CLK_DIV, 
                           8,      // select clk_div1 
                           24, 4); // [23:20] encp_clk_sel 
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_T)
              Wr_reg_bits (HHI_VID_CLK_DIV, 
                           8,      // select clk_div1 
                           20, 4); // [23:20] encp_clk_sel 
      if(enc_sel == ENC_SEL_ALL || enc_sel == ENC_SEL_L)
              Wr_reg_bits (HHI_VIID_CLK_DIV, 
                           8,      // select clk_div1 
                           12, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


//    Wr_reg_bits (HHI_VID_CLK_DIV, 
//                   1|(vclk_sel<<3),      // select clk_div2 
//                   24, 4); // [23:20] encp_clk_sel 

    if(upsample) {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div1 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }
    else {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0x111|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div2 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
} /* vclk_set_enc_1920x1080 */


//-------------------------------------
// setting for progressive only
void vclk_set_encp_1920x1080( int pll_sel, int pll_div_sel, int vclk_sel, int upsample)
{
    unsigned long pll_reg;
    unsigned long vid_div_reg;
    unsigned int xd; 

    if (pll_sel) { // Setting for VIID_PLL
    //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    //  |                                                            | |  Video Divider  | |                             |
    //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
    //  | 24.0000 |  2    99  |  12.0000  1188.0000  |   4  297.0000 | |     1   |   1.0 | |   2  |  148.5000 |   0.000% |  | 0x00020863 | 0x00010803 | 0x00000001 |
        pll_reg = 0x00020863;
        vid_div_reg = 0x00010803;
        xd = 2; 
    } else { // Setting for VID_PLL, consideration for supporting HDMI clock
    //  +------------------------------------------------------------+ +-----------------+ +-----------------------------+
    //  |                                                            | |  Video Divider  | |                             |
    //  |                      <<< PLL >>>                           | |  u_lvds_phy_top | |   <<< Clock Reset Test >>>  |
    //  +---------+-----------+----------------------+---------------+ +-----------------+ +------+-----------+----------+  +------------+------------+------------+
    //  |         |           |                      |               | | Pre       Post  | |      |           |          |  |        PLL |      Video |        crt |
    //  |         | PLL Ports |      PLL        PLL  | PLL  PLL      | | Divider   Div   | | CRT  |    Final  |          |  |       CBUS |    Divider |    Divider |
    //  |   Fin   |  N    M   |     Fref        VCO  |  OD  FOUT     | | (1 ~ 6)         | |  XD  |    Clock  |    Error |  |     0x105c |     0x1066 |     0x1059 |
    //  +---------+-----------+----------------------+---------------+ +---------+-------+ +------+-----------+----------+  +------------+------------+------------+
    //  | 25.0000 |  5   297  |   5.0000  1485.0000  |   2  742.5000 | |     5   |   1.0 | |   1  |  148.5000 |   0.000% |  | 0x00011529 | 0x00010843 | 0x00000000 |
    //  | 24.0000 |  1    62  |  24.0000  1488.0000  |   2  744.0000 | |     5   |   1.0 | |   1  |  148.8000 |   0.202% |  | 0x0001043e | 0x00010843 | 0x00000000 |
        pll_reg = 0x0001043e;
        vid_div_reg = 0x00010843;
        xd = 1; 
    }

    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
    
   
    if(vclk_sel) {
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(pll_sel) Wr( HHI_VIID_PLL_CNTL,       pll_reg );    
    else Wr( HHI_VID_PLL_CNTL,       pll_reg );

    if(pll_div_sel ) {
      Wr( HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
    }
    else {
      Wr( HHI_VID_DIVIDER_CNTL,   vid_div_reg);
    }

    if(vclk_sel) Wr( HHI_VIID_CLK_DIV, (Rd(HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else Wr( HHI_VID_CLK_DIV, (Rd(HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 5 ) {}    

    if(vclk_sel) {
      if(pll_div_sel) Wr_reg_bits (HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VIID_CLK_CNTL, Rd(HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_DIV,
                        1,              //divide 2 for clk_div1
                        8, 8);
    
      if(pll_div_sel) Wr_reg_bits (HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
      else Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
      Wr( HHI_VID_CLK_CNTL, Rd(HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
    }
    // delay 2uS

    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   8,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_DIV, 
                   0,      // select clk_div1 
                   24, 4); // [23:20] encp_clk_sel 
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
    // delay 2uS
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < 2 ) {}    


//    Wr_reg_bits (HHI_VID_CLK_DIV, 
//                   1|(vclk_sel<<3),      // select clk_div2 
//                   24, 4); // [23:20] encp_clk_sel 

    if(upsample) {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div1 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }
    else {
        Wr_reg_bits (HHI_VIID_CLK_DIV, 
                     0x111|(vclk_sel<<3)|(vclk_sel<<7)|(vclk_sel<<11),      // select clk_div2 
                     20, 12); // [31:20] dac0_clk_sel, dac1_clk_sel, dac2_clk_sel
    }

    if(vclk_sel) {
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      Wr_reg_bits (HHI_VID_CLK_CNTL, 
                   (3<<0),  // Enable cntl_div1_en and cntl_div2_en
                   0, 2    // cntl_div1_en
                   );
      Wr_reg_bits (HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      Wr_reg_bits (HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    
} /* vclk_set_encp_1920x1080 */

#endif
