#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/timing.h>
#include <asm/arch/uart.h>
#ifndef CONFIG_DDR_16BITMODE
#define CONFIG_DDR_16BITMODE 0
#define SRAM_SIZE(a)    (a<<0)
#else
#undef CONFIG_DDR_16BITMODE
#define CONFIG_DDR_16BITMODE (1<<7)
#define SRAM_SIZE(a)    (a<<1)
#endif
//DDR_W972GG6JB
SPL_STATIC_VAR struct ddr_set __ddr_setting=
{
                    .cl             =   6,
                    .t_faw          =  18,
                    .t_mrd          =   2,
                    .t_1us_pck      = 400,
                    .t_100ns_pck    =  40,
                    .t_init_us      = 511,
                    .t_ras          =  18,//16->18 for windond 
                    .t_rc           =  23,
                    .t_rcd          =   5,
                    .t_refi_100ns   =  78,
                    .t_rfc          =  80,
                    .t_rp           =   5,
                    .t_rrd          =   4,
                    .t_rtp          =   3,
                    .t_wr           =   6,
                    .t_wtr          =   3,
                    .t_xp           =   2,
                    .t_xsrd         =   0,   // init to 0 so that if only one of them is defined, this is chosen
                    .t_xsnr         =   0,
                    .t_exsr         = 200,
                    .t_al           =   0,   // Additive Latency
                    .t_clr          =   6,   // cas_latency for DDR2 (nclk cycles)
                    .t_dqs          =   2,   // distance between data phases to different ranks
                    .zqcr			=   0x1166,
                    .iocr           =   0xf00|(3<<30)|(3<<26),
                    .mcfg           =   0 |     // burst length = 4
                                        (0 << 2) |     // bl8int_en.   enable bl8 interrupt function.
                                        (1 <<18),
                    .ddr_ctrl       =   0xff225a | CONFIG_DDR_16BITMODE,
				    .ddr_pll_cntl   =   0x110664,//400
				    .ram_size       =   SRAM_SIZE(256*1024*1024),
};

SPL_STATIC_VAR struct pll_clk_settings __plls 
={
    .sys_pll_cntl=0x232,//1200M
	.other_pll_cntl=0x00000219,//0x2d*24/2=540M
	.mpeg_clk_cntl=	(1 << 12) |                     // select other PLL
			        ((3- 1) << 0 ) |    // div1
			        (1 << 7 ) |                     // cntl_hi_mpeg_div_en, enable gating
			        (1 << 8 ) |(1<<15),                    // Connect clk81 to the PLL divider output
	.demod_pll400m_cntl=(1<<9)	| //n 1200=xtal*m/n 
			(50<<0),	//m 50*24
	.clk81=200000000,//750/4=180M
	.a9_clk=1200000000/2,
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
