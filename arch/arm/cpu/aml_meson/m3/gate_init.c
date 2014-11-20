#include <asm/arch/power_gate.h>
#include <asm/arch/io.h>
#include <common.h>

static int is_sd_format(unsigned char *mode)
{
    int i;
    unsigned char *sd_mode[] = {(unsigned char *)("480I"), (unsigned char *)("480i"), (unsigned char *)("576I"), (unsigned char *)("576i"), (unsigned char *)("480cvbs"), (unsigned char *)("480CVBS"), (unsigned char *)("576cvbs"), (unsigned char *)("576CVBS"), NULL};

    for(i = 0; sd_mode[i]; i++) {
        if(strncmp((const char *)mode, (const char *)sd_mode[i], strlen((const char *)sd_mode[i])) == 0)
            return 1;
    }
    return 0;
}

void gate_init(void)
{
#if 1
	unsigned char * disp_mode = (unsigned char *)getenv((char *)("outputmode"));
	int i_flag = is_sd_format(disp_mode);

	/* close spi */
	CLK_GATE_OFF(SPICC);
	CLK_GATE_OFF(SPI1);
	CLK_GATE_OFF(SPI2);
    
	CLK_GATE_OFF(AUD_BUF);                          // CBUS[0x1050], gate off Audio buffer
	/* can't open HDMI */
	CLK_GATE_OFF(RANDOM_NUM_GEN);                   // CBUS[0x1050], gate off RANDOM_NUM_GEN 
	CLK_GATE_OFF(ASYNC_FIFO);                       // CBUS[0x1050], gate off ASYNC FIFO
	
	/* close card */
	#if 1
	CLK_GATE_OFF(SDHC);
	//CLK_GATE_OFF(SDIO);
	CLK_GATE_OFF(SMART_CARD_MPEG_DOMAIN);
	#endif
	
	/* close stream */
	CLK_GATE_OFF(STREAM);
	
	/* close USB */
	#if 1
	// can't connect to PC
	//CLK_GATE_OFF(USB_GENERAL);
	CLK_GATE_OFF(USB0);
	CLK_GATE_OFF(USB1);
	CLK_GATE_OFF(MISC_USB1_TO_DDR);
	CLK_GATE_OFF(MISC_USB0_TO_DDR);
	#endif
	
	/* close demux */
	CLK_GATE_OFF(DEMUX);

	/* clsoe BLK_MOV */
	CLK_GATE_OFF(BLK_MOV);                          // CBUS[0x1051], gate off Block move core logic

	/* close ethernet */
	CLK_GATE_OFF(ETHERNET);
	
	/* close ge2d */
	CLK_GATE_OFF(GE2D);
	
	/* close rom */
	CLK_GATE_OFF(ROM_CLK); //disable this bit will make other cpu can not be booted.
	
	/* close efuse */
	CLK_GATE_OFF(EFUSE);
	
	/* can't open HDMI */
	CLK_GATE_OFF(VCLK1_HDMI);
	CLK_GATE_OFF(HDMI_INTR_SYNC);                   // CBUS[0x1052], gate off HDMI interrupt synchronization
	CLK_GATE_OFF(HDMI_PCLK);                        // CBUS[0x1052], gate off HDMI PCLK


	/* close UARTS */
	CLK_GATE_OFF(UART1);
	CLK_GATE_OFF(UART2);
	
	/* close audio in */
	#if 1
	CLK_GATE_OFF(AUD_IN);
	#endif
	
	/* close AIU */
	#if 1
	CLK_GATE_OFF(AIU_AI_TOP_GLUE);
	CLK_GATE_OFF(AIU_IEC958);
	CLK_GATE_OFF(AIU_I2S_OUT);
	CLK_GATE_OFF(AIU_AMCLK_MEASURE);
	CLK_GATE_OFF(AIU_AIFIFO2);
	CLK_GATE_OFF(AIU_AUD_MIXER);
	CLK_GATE_OFF(AIU_MIXER_REG);
	CLK_GATE_OFF(AIU_ADC);
	//CLK_GATE_OFF(AIU_TOP_LEVEL);
	//CLK_GATE_OFF(AIU_PCLK);
	CLK_GATE_OFF(AIU_AOCLK);
	CLK_GATE_OFF(AIU_ICE958_AMCLK);
	#endif
	
	/* close media cpu */
	CLK_GATE_OFF(MEDIA_CPU);
    
	if(i_flag == 0) {
		CLK_GATE_OFF(VCLK2_ENCI);                       // CBUS[0x1054], gate off VCLK2_ENCI
	}

#ifndef CONFIG_ENABLE_CVBS
	CLK_GATE_OFF(DAC_CLK);                          // CBUS[0x1054], gate off DAC_CLK 
#endif
#ifndef CONFIG_VIDEO_AMLLCD
	CLK_GATE_OFF(VCLK2_VENCL);                          // CBUS[0x1054], gate off VCLK2_VENCL 
	//CLK_GATE_OFF(CTS_ENCL);
#endif

	/* close MDEC */
	CLK_GATE_OFF(VLD_CLK);
	CLK_GATE_OFF(IQIDCT_CLK);
	CLK_GATE_OFF(MC_CLK);
	CLK_GATE_OFF(MDEC_CLK_ASSIST);
	CLK_GATE_OFF(MDEC_CLK_PSC);
	CLK_GATE_OFF(MDEC_CLK_DBLK);
	CLK_GATE_OFF(MDEC_CLK_PIC_DC);
	
	/*close viu */
	CLK_GATE_OFF(VENC_I_TOP);
	CLK_GATE_OFF(VENC_P_TOP);
	CLK_GATE_OFF(VENC_T_TOP);
	CLK_GATE_OFF(VI_CORE);
	CLK_GATE_OFF(VENC_DAC);
	
	/* close venc */
	CLK_GATE_OFF(ENC480P);                          // CBUS[0x1054], gate off ENC480P
	CLK_GATE_OFF(VCLK2_ENCT);                       // CBUS[0x1054], gate off VCLK2_ENCT
	CLK_GATE_OFF(VCLK2_OTHER1);                     // CBUS[0x1054], gate off VCLK2_OTHER1
	CLK_GATE_OFF(VENCI_INT);
	CLK_GATE_OFF(VIU2);
	CLK_GATE_OFF(VENCP_INT);
	CLK_GATE_OFF(VENCT_INT);
	CLK_GATE_OFF(VENCL_INT);
	CLK_GATE_OFF(VENC_L_TOP);
	CLK_GATE_OFF(VCLK2_VENCI);
	CLK_GATE_OFF(VCLK2_VENCI1);
	CLK_GATE_OFF(VCLK2_VENCP);
	CLK_GATE_OFF(VCLK2_VENCP1);
	CLK_GATE_OFF(VCLK2_VENCT);
	CLK_GATE_OFF(VCLK2_VENCT1);
	CLK_GATE_OFF(VCLK2_OTHER);
	CLK_GATE_OFF(VCLK2_ENCP);
	CLK_GATE_OFF(VCLK2_ENCL);
	
	/* close spi */
	CLK_GATE_OFF(SPICC);
	CLK_GATE_OFF(SPI2);
	CLK_GATE_OFF(SPI1);
	
	/* close bt656 */
	CLK_GATE_OFF(BT656_IN);

	
	/* close LED_PWM */
	CLK_GATE_OFF(LED_PWM);
	
	CLK_GATE_OFF(HIU_PARSER_TOP);
	
	/* close vdin */
	CLK_GATE_OFF(VIDEO_IN);
	CLK_GATE_OFF(MISC_DVIN);
	
	/* close RDMA */
	CLK_GATE_OFF(MISC_RDMA);
	
	/* close uarts */
	CLK_GATE_OFF(UART1);
	CLK_GATE_OFF(UART2);
#endif

}
