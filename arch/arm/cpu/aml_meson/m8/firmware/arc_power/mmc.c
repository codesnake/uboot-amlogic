/*************************************************************************************
 * Amlogic DDR suspend driver
 *
 * Copyright (C) 2013 Amlogic, Inc.
 *
 * Author: haixiang.bao@amlogic.com
 *
 * version: 
 *             1. 2013.09.20 v0.1 create for suspend/resume flow integration
 *
 * History: 
 *             1. 2013.09.20 create for M8 DDR suspend/resume feature 
 *                 1.1 test with AML_8726-MB_SOK_V1.0 20130812, 2GB DDR, 4pcs, SAMSUNG K4B4G1646B-HCK0
 *                 1.2 place hx_ddr_suspend_test() to SPL stage, after load uboot success
 *                 1.3 test DDR 1.5V current with VICTOR 86E(range 10A)
 *                      1.3.1 DDR access: 101mA
 *                      1.3.2 DDR suspend: 24mA
 *                      1.3.3 uboot console: 43mA
 *                 1.4 To integrate this feature to ARC code
 *                      1.4.1 just use two APIs hx_ddr_power_down_enter() and hx_ddr_power_down_leave()
 *                 1.5 this version has no error recovery like watch dog etc
 *
 **************************************************************************************/

#ifndef __M8_DDR_SUSPEND__
#define __M8_DDR_SUSPEND__
#include <io.h>
#include "config.h"
#define readl(a) (*(volatile unsigned int *)(a))
#define writel(v,a) (*(volatile unsigned int *)(a) = (v))
//M8 DDR0/1 channel select
#define CONFIG_M8_DDRX2_S12  (0)
#define CONFIG_M8_DDR1_ONLY  (1)
#define CONFIG_M8_DDRX2_S30  (2)
#define CONFIG_M8_DDR0_ONLY  (3)

#define M8_CHIP_ID_M8M2 0x11111112

unsigned int m8_chip_id;
#ifdef hx_serial_puts
  #undef hx_serial_puts
  #undef hx_serial_put_hex
#endif 

#if 0
    #define hx_serial_puts    f_serial_puts
	#define hx_serial_put_hex serial_put_hex
#else
    #define hx_serial_puts 
	#define hx_serial_put_hex 
#endif
#if 0
static void * hx_memcpy(void *dest, const void *src, size_t count)
{
	unsigned long *dl = (unsigned long *)dest, *sl = (unsigned long *)src;
	char *d8, *s8;

	/* while all data is aligned (common case), copy a word at a time */
	if ( (((ulong)dest | (ulong)src) & (sizeof(*dl) - 1)) == 0) {
		while (count >= sizeof(*dl)) {
			*dl++ = *sl++;
			count -= sizeof(*dl);
		}
	}
	/* copy the reset one byte at a time */
	d8 = (char *)dl;
	s8 = (char *)sl;
	while (count--)
		*d8++ = *s8++;

	return dest;
}

static int hx_memcmp(const unsigned char * cs,const unsigned char * ct, int count)
{
	const unsigned char *su1, *su2;
	int res = 0;
	int nRet = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--,++nRet)
		if ((res = *su1 - *su2) != 0)
			break;
	return (res ? nRet:(-1));
}
#endif
static void __udelayx(int n)
{	
	//#define ISA_TIMERE    0x2655
	//0xc1109954
	unsigned long base=readl(0xc1109954);
    while(( readl(0xc1109954) - base)<n);
}
#if 0
static int hx_serial_put_dec(unsigned int data)
{
	char szTxt[20];
	szTxt[0] = 0x30;
	int i = 0;
	int nRet = 0;
	while(data)
	{
		szTxt[i++] = (data % 10) + 0x30;
		data = data / 10;
	}
	nRet = i;
	
	for(--i;i >=0;--i)	
		serial_putc(szTxt[i]);

	return nRet;
}
#endif
//Note:
//Please make sure following will cover all PUB/PCTL setting in file
//uboot\arch\arm\cpu\aml_meson\m8\firmware\ddr_init_pctl.c 
typedef enum {

	  M8_DDR_CHANNEL_SET=0,   _MMC_DDR_CTRL, _DMC_DDR_CTRL, _SOFT_RESET , _APD_CTRL, _CLK_CTRL, // 5
	
	 _PCTL_TOGCNT1U , _PCTL_TOGCNT100N ,  _PCTL_TINIT ,  _PCTL_TRSTH ,  _PCTL_TRSTL, //10

	 _PCTL_MCFG, _PCTL_MCFG1, _PUB_DCR, _PUB_MR0, _PUB_MR1, _PUB_MR2, _PUB_MR3,  // 17 
	 
	 _PUB_DTPR0, _PUB_DTPR1, _PUB_DTPR2, _PUB_DTPR3,  //21

	 _PUB_PGCR2,_PUB_PGCR3, _PUB_DTCR, _PUB_DXCCR,  //25
	 
	 _PUB_DX0GCR3,_PUB_DX1GCR3,_PUB_DX2GCR3,_PUB_DX3GCR3, //29

	 _PUB_PTR0, _PUB_PTR1, _PUB_PTR2, _PUB_PTR3, _PUB_PTR4, //34

	 _PUB_ACIOCR0, _PUB_DSGCR, _PUB_ZQ0PR,_PUB_ZQCR,_PUB_ACBDLR0, //39

	 _PCTL_TREFI ,  _PCTL_TREFI_MEM_DDR3 ,  _PCTL_TMRD ,  _PCTL_TRFC ,  _PCTL_TRP , //44
	 
	 _PCTL_TAL   ,  _PCTL_TCWL           ,  _PCTL_TCL  ,  _PCTL_TRAS ,  _PCTL_TRC , //49
	 
	 _PCTL_TRCD  ,  _PCTL_TRRD           ,  _PCTL_TRTP ,  _PCTL_TWR  ,  _PCTL_TWTR, //54
	
	 _PCTL_TEXSR ,  _PCTL_TXP   ,  _PCTL_TDQS ,  _PCTL_TRTW  ,  _PCTL_TCKSRE , //59
	 
	 _PCTL_TCKSRX,  _PCTL_TMOD  ,  _PCTL_TCKE ,  _PCTL_TZQCS ,  _PCTL_TZQCL  , //64
	 
	 _PCTL_TXPDLL,  _PCTL_TZQCSI,  _PCTL_SCFG ,  //67

	 _PCTL_PPCFG ,  _PCTL_DFISTCFG0 ,  _PCTL_DFISTCFG1 ,  _PCTL_DFITCTRLDELAY , //71

	 _PCTL_DFITPHYWRDATA ,  _PCTL_DFITPHYWRLAT ,  _PCTL_DFITRDDATAEN,

	 _PCTL_DFITPHYRDLAT  ,  _PCTL_DFITDRAMCLKDIS,  _PCTL_DFITDRAMCLKEN, //77
  
	 _PCTL_DFITCTRLUPDMIN,  _PCTL_DFILPCFG0     , _PCTL_DFITPHYUPDTYPE1, //80

	 _PCTL_DFIODTCFG     ,  _PCTL_DFIODTCFG1,

	 _PUB_DTAR0, _PUB_DTAR1, _PUB_DTAR2, _PUB_DTAR3, //86

	_DDR_PLL_CNTL        , _DDR_PLL_CNTL1       , _DDR_PLL_CNTL2 	    , _DDR_PLL_CNTL3	    , _DDR_PLL_CNTL4,	//91
	
	_PUB_PLLCR,_PUB_PGCR0,_PUB_IOVCR0,_PUB_IOVCR1, _PUB_DX1GCR0, _PUB_DX2GCR0, _PUB_DX3GCR0, _PUB_DX4GCR0, _PUB_DX5GCR0,
	
} back_reg_index;

//when above entend make sure g_ddr_settings is updae also    
static unsigned int g_ddr_settings[120];

static void hx_ddr_setting_save()
{	
	hx_serial_puts("Aml log : hx_ddr_setting_save\n");
	
	int nOffset = 0;
	
	//default load PCTL0 and switch to PCTL1 if only DDR1 is used
	if(CONFIG_M8_DDR1_ONLY == g_ddr_settings[M8_DDR_CHANNEL_SET])
		nOffset = 0x2000;

#ifdef DDR_SUSPEND_SAVE
    #undef DDR_SUSPEND_SAVE
#endif 

#define DDR_SUSPEND_SAVE(a)  {g_ddr_settings[a] = readl(P_DDR0##a + nOffset);}

	DDR_SUSPEND_SAVE( _SOFT_RESET);	       
	DDR_SUSPEND_SAVE( _APD_CTRL);
	DDR_SUSPEND_SAVE( _CLK_CTRL);
	DDR_SUSPEND_SAVE(_PUB_IOVCR0);
	DDR_SUSPEND_SAVE(_PUB_IOVCR1);
	
	DDR_SUSPEND_SAVE( _PUB_PLLCR);

	//PUB
	DDR_SUSPEND_SAVE( _PCTL_TOGCNT1U);
	DDR_SUSPEND_SAVE( _PCTL_TOGCNT100N);
	DDR_SUSPEND_SAVE( _PCTL_TINIT);
	DDR_SUSPEND_SAVE( _PCTL_TRSTH);
	DDR_SUSPEND_SAVE( _PCTL_TRSTL);
		
	DDR_SUSPEND_SAVE( _PCTL_MCFG);
	DDR_SUSPEND_SAVE( _PCTL_MCFG1);
	DDR_SUSPEND_SAVE( _PUB_DCR);

	DDR_SUSPEND_SAVE( _PUB_MR0);
	DDR_SUSPEND_SAVE( _PUB_MR1);
	DDR_SUSPEND_SAVE( _PUB_MR2);
	DDR_SUSPEND_SAVE( _PUB_MR3);
	
	DDR_SUSPEND_SAVE( _PUB_DTPR0);
	DDR_SUSPEND_SAVE( _PUB_DTPR1);
	DDR_SUSPEND_SAVE( _PUB_DTPR2);

	DDR_SUSPEND_SAVE( _PUB_PGCR3);	
	DDR_SUSPEND_SAVE( _PUB_PGCR2);
	DDR_SUSPEND_SAVE( _PUB_PGCR0);
	
	DDR_SUSPEND_SAVE( _PUB_DTCR);	
	DDR_SUSPEND_SAVE( _PUB_DXCCR);

	DDR_SUSPEND_SAVE( _PUB_DX0GCR3);
	DDR_SUSPEND_SAVE( _PUB_DX1GCR3);
	DDR_SUSPEND_SAVE( _PUB_DX2GCR3);
	DDR_SUSPEND_SAVE( _PUB_DX3GCR3);

	DDR_SUSPEND_SAVE( _PUB_ZQ0PR);
	DDR_SUSPEND_SAVE( _PUB_ZQCR);

	DDR_SUSPEND_SAVE( _PUB_ACBDLR0);
			
	DDR_SUSPEND_SAVE( _PUB_PTR0);
	DDR_SUSPEND_SAVE( _PUB_PTR1);
	DDR_SUSPEND_SAVE( _PUB_PTR2);
	DDR_SUSPEND_SAVE( _PUB_PTR3);
	DDR_SUSPEND_SAVE( _PUB_PTR4);


	DDR_SUSPEND_SAVE( _PUB_ACIOCR0);
	DDR_SUSPEND_SAVE( _PUB_DSGCR);

	//UPCTL
	DDR_SUSPEND_SAVE( _PCTL_TREFI);
	DDR_SUSPEND_SAVE( _PCTL_TREFI_MEM_DDR3);
	DDR_SUSPEND_SAVE( _PCTL_TMRD);
	DDR_SUSPEND_SAVE( _PCTL_TRFC);
	DDR_SUSPEND_SAVE( _PCTL_TRP);	
	DDR_SUSPEND_SAVE( _PCTL_TAL);
	DDR_SUSPEND_SAVE( _PCTL_TCWL);
	
	DDR_SUSPEND_SAVE( _PCTL_TCL);
	DDR_SUSPEND_SAVE( _PCTL_TRAS);
	DDR_SUSPEND_SAVE( _PCTL_TRC);
	DDR_SUSPEND_SAVE( _PCTL_TRCD);
	DDR_SUSPEND_SAVE( _PCTL_TRRD);
	DDR_SUSPEND_SAVE( _PCTL_TRTP);
	DDR_SUSPEND_SAVE( _PCTL_TWR);

	DDR_SUSPEND_SAVE( _PCTL_TWTR);
	DDR_SUSPEND_SAVE( _PCTL_TEXSR);
	DDR_SUSPEND_SAVE( _PCTL_TXP);

	DDR_SUSPEND_SAVE( _PCTL_TDQS);
	DDR_SUSPEND_SAVE( _PCTL_TRTW);
	DDR_SUSPEND_SAVE( _PCTL_TCKSRE);
	DDR_SUSPEND_SAVE( _PCTL_TCKSRX);
	DDR_SUSPEND_SAVE( _PCTL_TMOD);
	DDR_SUSPEND_SAVE( _PCTL_TCKE);
	DDR_SUSPEND_SAVE( _PCTL_TZQCS);
	DDR_SUSPEND_SAVE( _PCTL_TZQCL);
	DDR_SUSPEND_SAVE( _PCTL_TXPDLL);
	DDR_SUSPEND_SAVE( _PCTL_TZQCSI);
	DDR_SUSPEND_SAVE( _PCTL_SCFG);	

	
	DDR_SUSPEND_SAVE( _PCTL_PPCFG);	
	DDR_SUSPEND_SAVE( _PUB_DX1GCR0);
	DDR_SUSPEND_SAVE( _PUB_DX2GCR0);
	DDR_SUSPEND_SAVE( _PUB_DX3GCR0);
	DDR_SUSPEND_SAVE( _PUB_DX4GCR0);
	DDR_SUSPEND_SAVE( _PUB_DX5GCR0);
	DDR_SUSPEND_SAVE( _PCTL_DFISTCFG0);	
	DDR_SUSPEND_SAVE( _PCTL_DFISTCFG1);	
	DDR_SUSPEND_SAVE( _PCTL_DFITCTRLDELAY);	
	DDR_SUSPEND_SAVE( _PCTL_DFITPHYWRDATA);	
	DDR_SUSPEND_SAVE( _PCTL_DFITPHYWRLAT);	
	DDR_SUSPEND_SAVE( _PCTL_DFITRDDATAEN);	
	
	DDR_SUSPEND_SAVE( _PCTL_DFITPHYRDLAT);	
	DDR_SUSPEND_SAVE( _PCTL_DFITDRAMCLKDIS);	
	DDR_SUSPEND_SAVE( _PCTL_DFITDRAMCLKEN);	
	DDR_SUSPEND_SAVE( _PCTL_DFITCTRLUPDMIN);	
	DDR_SUSPEND_SAVE( _PCTL_DFILPCFG0);

	DDR_SUSPEND_SAVE( _PCTL_DFITPHYUPDTYPE1);
	DDR_SUSPEND_SAVE( _PCTL_DFIODTCFG);
	DDR_SUSPEND_SAVE( _PCTL_DFIODTCFG1);
	
	DDR_SUSPEND_SAVE( _PUB_DTAR0);
	DDR_SUSPEND_SAVE( _PUB_DTAR1);
	DDR_SUSPEND_SAVE( _PUB_DTAR2);
	DDR_SUSPEND_SAVE( _PUB_DTAR3);

#undef DDR_SUSPEND_SAVE

	//DDR PLL setting save
	g_ddr_settings[_DDR_PLL_CNTL]  = readl(AM_DDR_PLL_CNTL);
	g_ddr_settings[_DDR_PLL_CNTL1] = readl(AM_DDR_PLL_CNTL1);
	g_ddr_settings[_DDR_PLL_CNTL2] = readl(AM_DDR_PLL_CNTL2);
	g_ddr_settings[_DDR_PLL_CNTL3] = readl(AM_DDR_PLL_CNTL3);
	g_ddr_settings[_DDR_PLL_CNTL4] = readl(AM_DDR_PLL_CNTL4);
	             
}

static void hx_mmc_req_set(int nFlagEnable)
{	
	int nSet = 0xfff;
	if(M8_CHIP_ID_M8M2 == m8_chip_id){
		nSet = 0xffff;
	}
	if(nFlagEnable)
	{
		hx_serial_puts("Aml log : hx_mmc_request_enable\n");
	}
	else
	{
		nSet = 0;
		hx_serial_puts("Aml log : hx_mmc_request_disable\n");	
	}
	if(M8_CHIP_ID_M8M2 == m8_chip_id)
		writel(nSet, P_DMC_REQ_CTRL);
	else
		writel(nSet, P_MMC_REQ_CTRL);
}


#define  UPCTL_STAT_MASK		(7)
#define  UPCTL_STAT_INIT        (0)
#define  UPCTL_STAT_CONFIG      (1)
#define  UPCTL_STAT_ACCESS      (3)
#define  UPCTL_STAT_LOW_POWER   (5)

#define  UPCTL_CMD_INIT         (0) 
#define  UPCTL_CMD_CONFIG       (1) 
#define  UPCTL_CMD_GO           (2) 
#define  UPCTL_CMD_SLEEP        (3) 
#define  UPCTL_CMD_WAKEUP       (4) 


static void hx_pctl_sleep(void)
{
	hx_serial_puts("Aml log : hx_pctl_sleep\n");
	
	int stat;
	
	if(CONFIG_M8_DDR1_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET]) //channel 0 used
	{
		writel(readl(P_DDR0_CLK_CTRL) | 1, P_DDR0_CLK_CTRL); 
		
		hx_serial_puts("Aml log : hx_pctl_sleep_ch-0\n");
		do
		{		
			stat = readl(P_DDR0_PCTL_STAT) & UPCTL_STAT_MASK;				
			if(stat == UPCTL_STAT_INIT) {
				writel(UPCTL_CMD_CONFIG,P_DDR0_PCTL_SCTL);
			}
			else if(stat == UPCTL_STAT_CONFIG) {
				writel(UPCTL_CMD_GO,P_DDR0_PCTL_SCTL);
			}
			else if(stat == UPCTL_STAT_ACCESS) {
				writel(UPCTL_CMD_SLEEP,P_DDR0_PCTL_SCTL);
			}
			
			stat = readl(P_DDR0_PCTL_STAT) & UPCTL_STAT_MASK;
			
		}while(stat != UPCTL_STAT_LOW_POWER);

	}
	if(CONFIG_M8_DDR0_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET]) //channel 1 used
	{
		writel(readl(P_DDR1_CLK_CTRL) | 1, P_DDR1_CLK_CTRL);
		
		hx_serial_puts("Aml log : hx_pctl_sleep_ch-1\n");
		do
		{		
			stat = readl(P_DDR1_PCTL_STAT) & UPCTL_STAT_MASK;				
			if(stat == UPCTL_STAT_INIT) {
				writel(UPCTL_CMD_CONFIG,P_DDR1_PCTL_SCTL);
			}
			else if(stat == UPCTL_STAT_CONFIG) {
				writel(UPCTL_CMD_GO,P_DDR1_PCTL_SCTL);
			}
			else if(stat == UPCTL_STAT_ACCESS) {
				writel(UPCTL_CMD_SLEEP,P_DDR1_PCTL_SCTL);
			}
			
			stat = readl(P_DDR1_PCTL_STAT) & UPCTL_STAT_MASK;
			
		}while(stat != UPCTL_STAT_LOW_POWER);
	}
}

static void hx_ddr_retention_set(int nFlagEnable)
{
	if(nFlagEnable)
		hx_serial_puts("Aml log : hx_enable_retention\n");
	else
		hx_serial_puts("Aml log : hx_disable_retention\n");
		
	//RENT_N/RENT_EN_N switch from 01 to 10 (2'b10 = ret_enable)
	/*P_AO_RTI_PWR_CNTL_REG1[b3,2,1,0]
	3	R/W	1	DDR1PHYIO_REG_EN
	2	R/W	1	DDR1PHYIO_REG_EN_N 
	1	R/W	1	DDR0PHYIO_REG_EN
	0	R/W	1	DDR0PHYIO_REG_EN_N
	*/

	int nSetting = readl(P_AO_RTI_PWR_CNTL_REG1);
	
	if(CONFIG_M8_DDR1_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET]) //channel 0 used
	{
		nSetting &= (~3);
		nSetting |= (nFlagEnable ? 2 : 1);
	}

	if(CONFIG_M8_DDR0_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET]) //channel 1 used
	{
		nSetting &= (~(3<<2));
		nSetting |= ((nFlagEnable ? 2 : 1)<<2);
	}
	
	hx_serial_puts("Aml log : ");
	hx_serial_puts(nFlagEnable ? "enable rentation with 0x" : "disable rentation with 0x");
	hx_serial_put_hex(nSetting,32);
	hx_serial_puts("\n");

	writel(nSetting,P_AO_RTI_PWR_CNTL_REG1);
	
	__udelayx(200); //need fine tune?
}

void hx_ddr_power_down_enter()
{	
	hx_serial_puts("Aml log : hx_power_down_enter\n");

	//verify pass, all keep same value before and after
	//hx_dump_mmc_dmc_all();
	
	//asm(".long 0x003f236f"); //add sync instruction.

	//disable MMC request
	hx_mmc_req_set(0);

	//For M8 does shut down PCTL clock for power saving
	//before access PCTL registers we must enable clock for PCTL

	if(M8_CHIP_ID_M8M2 == m8_chip_id){
		g_ddr_settings[_DMC_DDR_CTRL] = readl(P_DMC_DDR_CTRL);
		g_ddr_settings[M8_DDR_CHANNEL_SET] = ((((g_ddr_settings[_DMC_DDR_CTRL] >> 26)&0x3)==0x3)?(((g_ddr_settings[_DMC_DDR_CTRL]>>24)&0x1)?(CONFIG_M8_DDR1_ONLY):(CONFIG_M8_DDR0_ONLY)):(CONFIG_M8_DDRX2_S12));
	}
	else{
		g_ddr_settings[_MMC_DDR_CTRL] = readl(P_MMC_DDR_CTRL);
		g_ddr_settings[M8_DDR_CHANNEL_SET] = (g_ddr_settings[_MMC_DDR_CTRL] >> 24 ) & 3;
	}

	if(CONFIG_M8_DDR1_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET]) //channel 0 used
	{
		writel(readl(P_DDR0_CLK_CTRL) | 1, P_DDR0_CLK_CTRL);
		writel(readl(P_DDR0_CLK_CTRL) | 1, P_DDR1_CLK_CTRL);
	}
	
	if(CONFIG_M8_DDR0_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET]) //channel 1 used
	{
		writel(readl(P_DDR1_CLK_CTRL) | 1, P_DDR1_CLK_CTRL); 
		writel(readl(P_DDR1_CLK_CTRL) | 1, P_DDR0_CLK_CTRL);
	}	

	//hx_dump_pub_pctl_all();	

	//hx_dump_mmc_dmc_all();
	
	//DDR setting save
	hx_ddr_setting_save(); 
		
	//DDR pctl sleep
	hx_pctl_sleep();

	//Clear PGCR3 CKEN to 00
	//or you can try 11?
	//writel(readl(P_DDR0_PUB_PGCR3)&(~(0xFF<<16)),P_DDR0_PUB_PGCR3);
	//writel(readl(P_DDR1_PUB_PGCR3)&(~(0xFF<<16)),P_DDR1_PUB_PGCR3);
	writel(g_ddr_settings[_PUB_PGCR3]&(~(0xFF<<16)),P_DDR0_PUB_PGCR3);
	writel(g_ddr_settings[_PUB_PGCR3]&(~(0xFF<<16)),P_DDR1_PUB_PGCR3);

	writel(readl(P_DDR0_PUB_ACIOCR3)|0x2,P_DDR0_PUB_ACIOCR3);
	writel(readl(P_DDR1_PUB_ACIOCR3)|0x2,P_DDR1_PUB_ACIOCR3);

	//asm volatile ("wfi");
	
	// enable retention
	hx_ddr_retention_set(1);

	//shut down PUB-PLL
	//if(CONFIG_M8_DDR1_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET])
	//	writel((readl(P_DDR0_PUB_PLLCR))|0x60000000, P_DDR0_PUB_PLLCR);				
	//if(CONFIG_M8_DDR0_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET])
	//	writel((readl(P_DDR1_PUB_PLLCR))|0x60000000, P_DDR1_PUB_PLLCR);

	writel(g_ddr_settings[_PUB_PLLCR]|0x60000000, P_DDR0_PUB_PLLCR);
	writel(g_ddr_settings[_PUB_PLLCR]|0x60000000, P_DDR1_PUB_PLLCR);


	writel((1<<2)|(1<<6)|(1<<4), P_DDR0_CLK_CTRL);
	writel((1<<2)|(1<<6)|(1<<4), P_DDR1_CLK_CTRL);

	// shut down DDR PLL
	writel(readl(AM_DDR_PLL_CNTL)|(1<<30),AM_DDR_PLL_CNTL);
	//need this ? 
	writel(0,AM_DDR_PLL_CNTL1);
	writel(0,AM_DDR_PLL_CNTL2);
	writel(0,AM_DDR_PLL_CNTL3);
	writel(0,AM_DDR_PLL_CNTL4);	
	
	hx_serial_puts("Aml log : DDR PLL is power down\n");
	
}

static int hx_init_pctl_ddr3()
{	
	int nRet = -1;
	int nTempVal = 0;
	int nRetryCnt = 0; //mmc reset try counter; max to try 8 times 

#define MMC_RESET_CNT_MAX       (8)

	//asm volatile ("wfi");
	
mmc_init_all:

	hx_serial_puts("Aml log : mmc_init_all\n");
	nRetryCnt++;

	if(nRetryCnt > MMC_RESET_CNT_MAX )
		return nRet;

//PUB PIR setting
#define PUB_PIR_INIT  		(1<<0)
#define PUB_PIR_ZCAL  		(1<<1)
#define PUB_PIR_CA  		(1<<2)
#define PUB_PIR_PLLINIT 	(1<<4)
#define PUB_PIR_DCAL    	(1<<5)
#define PUB_PIR_PHYRST  	(1<<6)
#define PUB_PIR_DRAMRST 	(1<<7)
#define PUB_PIR_DRAMINIT 	(1<<8)
#define PUB_PIR_WL       	(1<<9)
#define PUB_PIR_QSGATE   	(1<<10)
#define PUB_PIR_WLADJ    	(1<<11)
#define PUB_PIR_RDDSKW   	(1<<12)
#define PUB_PIR_WRDSKW   	(1<<13)
#define PUB_PIR_RDEYE    	(1<<14)
#define PUB_PIR_WREYE    	(1<<15)
#define PUB_PIR_ICPC     	(1<<16)
#define PUB_PIR_PLLBYP   	(1<<17)
#define PUB_PIR_CTLDINIT 	(1<<18)
#define PUB_PIR_RDIMMINIT	(1<<19)
#define PUB_PIR_CLRSR    	(1<<27)
#define PUB_PIR_LOCKBYP  	(1<<28)
#define PUB_PIR_DCALBYP 	(1<<29)
#define PUB_PIR_ZCALBYP 	(1<<30)
#define PUB_PIR_INITBYP  	(1<<31)

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//HX DDR init code
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//PUB PGSR0
#define PUB_PGSR0_IDONE     (1<<0)
#define PUB_PGSR0_PLDONE    (1<<1)
#define PUB_PGSR0_DCDONE    (1<<2)
#define PUB_PGSR0_ZCDONE    (1<<3)
#define PUB_PGSR0_DIDONE    (1<<4)
#define PUB_PGSR0_WLDONE    (1<<5)
#define PUB_PGSR0_QSGDONE   (1<<6)
#define PUB_PGSR0_WLADONE   (1<<7)
#define PUB_PGSR0_RDDONE    (1<<8)
#define PUB_PGSR0_WDDONE    (1<<9)
#define PUB_PGSR0_REDONE    (1<<10)
#define PUB_PGSR0_WEDONE    (1<<11)
#define PUB_PGSR0_CADONE    (1<<12)

#define PUB_PGSR0_ZCERR     (1<<20)
#define PUB_PGSR0_WLERR     (1<<21)
#define PUB_PGSR0_QSGERR    (1<<22)
#define PUB_PGSR0_WLAERR    (1<<23)
#define PUB_PGSR0_RDERR     (1<<24)
#define PUB_PGSR0_WDERR     (1<<25)
#define PUB_PGSR0_REERR     (1<<26)
#define PUB_PGSR0_WEERR     (1<<27)
#define PUB_PGSR0_CAERR     (1<<28)
#define PUB_PGSR0_CAWERR    (1<<29)
#define PUB_PGSR0_VTDONE    (1<<30)
#define PUB_PGSR0_DTERR     (PUB_PGSR0_RDERR|PUB_PGSR0_WDERR|PUB_PGSR0_REERR|PUB_PGSR0_WEERR)


#ifdef DDR0_SUSPEND_LOAD
   #undef DDR0_SUSPEND_LOAD
#endif
#ifdef DDR1_SUSPEND_LOAD
   #undef DDR1_SUSPEND_LOAD
#endif

	#define DDR0_SUSPEND_LOAD(a)  {writel(g_ddr_settings[a],P_DDR0##a);}
	#define DDR1_SUSPEND_LOAD(a)  {writel(g_ddr_settings[a],P_DDR1##a);}
	
	int nM8_DDR_CHN_SET = g_ddr_settings[M8_DDR_CHANNEL_SET];
	
	//M8 DDR0/1 channel select
	//#define CONFIG_M8_DDRX2_S12  (0)
	//#define CONFIG_M8_DDR1_ONLY  (1)
	//#define CONFIG_M8_DDRX2_S30  (2)
	//#define CONFIG_M8_DDR0_ONLY  (3)
	//#define CONFIG_M8_DDR_CHANNEL_SET (CONFIG_M8_DDRX2_S12)


/////////////////////////////////////////////////////
pub_init_ddr0:
pub_init_ddr1:

	//uboot timming.c setting
	//UPCTL and PUB clock and reset.
    //if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
    {
    	hx_serial_puts("Aml log : DDR0 - init start...\n");
		writel(0, P_DDR0_SOFT_RESET); //DDR 0
		DDR0_SUSPEND_LOAD(_SOFT_RESET);
		DDR0_SUSPEND_LOAD(_APD_CTRL);
		DDR0_SUSPEND_LOAD(_CLK_CTRL);				
		DDR0_SUSPEND_LOAD(_PUB_IOVCR0);
		DDR0_SUSPEND_LOAD(_PUB_IOVCR1);
		//writel(g_ddr_settings[_CLK_CTRL]|1,P_DDR0_CLK_CTRL);
		hx_serial_puts("Aml log : DDR0 - APD,CLK set done\n");
    }

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
    {
    	hx_serial_puts("Aml log : DDR1 - init start...\n");
		writel(0, P_DDR1_SOFT_RESET); //DDR 0
		DDR1_SUSPEND_LOAD(_SOFT_RESET);
		DDR1_SUSPEND_LOAD(_APD_CTRL);
		DDR1_SUSPEND_LOAD(_CLK_CTRL);				
		DDR1_SUSPEND_LOAD(_PUB_IOVCR0);
		DDR1_SUSPEND_LOAD(_PUB_IOVCR1);
		//writel(g_ddr_settings[_CLK_CTRL]|1,P_DDR1_CLK_CTRL);
		hx_serial_puts("Aml log : DDR1 - APD,CLK set done\n");
    }	

	/*DDR0*/
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
	{
		//PCTL memory timing registers	
		DDR0_SUSPEND_LOAD(_PCTL_TOGCNT1U);
		DDR0_SUSPEND_LOAD(_PCTL_TOGCNT100N);
		DDR0_SUSPEND_LOAD(_PCTL_TINIT);
		DDR0_SUSPEND_LOAD(_PCTL_TRSTH);
		DDR0_SUSPEND_LOAD(_PCTL_TRSTL);
		
		DDR0_SUSPEND_LOAD(_PCTL_MCFG);
		DDR0_SUSPEND_LOAD(_PCTL_MCFG1);
		DDR0_SUSPEND_LOAD(_PUB_DCR);

		// program PUB MRx registers.			
		DDR0_SUSPEND_LOAD(_PUB_MR0);
		DDR0_SUSPEND_LOAD(_PUB_MR1);
		DDR0_SUSPEND_LOAD(_PUB_MR2);
		DDR0_SUSPEND_LOAD(_PUB_MR3);

		//program DDR SDRAM timing parameter.
		DDR0_SUSPEND_LOAD(_PUB_DTPR0);
		DDR0_SUSPEND_LOAD(_PUB_DTPR1);
		DDR0_SUSPEND_LOAD(_PUB_DTPR2);

		//DDR0_SUSPEND_LOAD(_PUB_PGCR3);
		writel(g_ddr_settings[_PUB_PGCR3] & (~(0x7f<<9)), 
			P_DDR0_PUB_PGCR3);
		
		DDR0_SUSPEND_LOAD(_PUB_PGCR2);
		DDR0_SUSPEND_LOAD(_PUB_DTCR);

		DDR0_SUSPEND_LOAD(_PUB_DX0GCR3);
		DDR0_SUSPEND_LOAD(_PUB_DX1GCR3);
		DDR0_SUSPEND_LOAD(_PUB_DX2GCR3);
		DDR0_SUSPEND_LOAD(_PUB_DX3GCR3);

		DDR0_SUSPEND_LOAD(_PUB_DSGCR);
		
		writel(g_ddr_settings[_PUB_ZQCR] & 0xff01fff8, 
			P_DDR0_PUB_ZQCR);
		DDR0_SUSPEND_LOAD(_PUB_ZQ0PR);
		writel((1<<1)|readl(P_DDR0_PUB_ZQCR), P_DDR0_PUB_ZQCR); 

		DDR0_SUSPEND_LOAD(_PUB_DXCCR);

		DDR0_SUSPEND_LOAD(_PUB_ACBDLR0);
		
		DDR0_SUSPEND_LOAD(_PUB_PTR0);
		DDR0_SUSPEND_LOAD(_PUB_PTR1);		
		
		DDR0_SUSPEND_LOAD(_PUB_ACIOCR0);
		//DDR0_SUSPEND_LOAD(_PUB_DSGCR);		
		
		DDR0_SUSPEND_LOAD(_PUB_PTR3);
		DDR0_SUSPEND_LOAD(_PUB_PTR4);

		
		
	}

	
	/*DDR1*/
	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
	{
		//PCTL memory timing registers	
		DDR1_SUSPEND_LOAD(_PCTL_TOGCNT1U);
		DDR1_SUSPEND_LOAD(_PCTL_TOGCNT100N);
		DDR1_SUSPEND_LOAD(_PCTL_TINIT);
		DDR1_SUSPEND_LOAD(_PCTL_TRSTH);
		DDR1_SUSPEND_LOAD(_PCTL_TRSTL);
		
		DDR1_SUSPEND_LOAD(_PCTL_MCFG);
		DDR1_SUSPEND_LOAD(_PCTL_MCFG1);
		DDR1_SUSPEND_LOAD(_PUB_DCR);

		// program PUB MRx registers.			
		DDR1_SUSPEND_LOAD(_PUB_MR0);
		DDR1_SUSPEND_LOAD(_PUB_MR1);
		DDR1_SUSPEND_LOAD(_PUB_MR2);
		DDR1_SUSPEND_LOAD(_PUB_MR3);

		//program DDR SDRAM timing parameter.
		DDR1_SUSPEND_LOAD(_PUB_DTPR0);
		DDR1_SUSPEND_LOAD(_PUB_DTPR1);
		DDR1_SUSPEND_LOAD(_PUB_DTPR2);

		//DDR1_SUSPEND_LOAD(_PUB_PGCR3);
		writel(g_ddr_settings[_PUB_PGCR3] & (~(0x7f<<9)), 
			P_DDR1_PUB_PGCR3);
		
		DDR1_SUSPEND_LOAD(_PUB_PGCR2);
		DDR1_SUSPEND_LOAD(_PUB_DTCR);

		DDR1_SUSPEND_LOAD(_PUB_DX0GCR3);
		DDR1_SUSPEND_LOAD(_PUB_DX1GCR3);
		DDR1_SUSPEND_LOAD(_PUB_DX2GCR3);
		DDR1_SUSPEND_LOAD(_PUB_DX3GCR3);

		DDR1_SUSPEND_LOAD(_PUB_DSGCR);

		writel(g_ddr_settings[_PUB_ZQCR] & 0xff01fff8,
			P_DDR1_PUB_ZQCR);
		DDR1_SUSPEND_LOAD(_PUB_ZQ0PR);
		writel((1<<1)|readl(P_DDR1_PUB_ZQCR), 
			P_DDR1_PUB_ZQCR); 
		
		DDR1_SUSPEND_LOAD(_PUB_DXCCR);
		DDR1_SUSPEND_LOAD(_PUB_ACBDLR0);

		DDR1_SUSPEND_LOAD(_PUB_PTR0);
		DDR1_SUSPEND_LOAD(_PUB_PTR1);		
		
		DDR1_SUSPEND_LOAD(_PUB_ACIOCR0);
		//DDR1_SUSPEND_LOAD(_PUB_DSGCR);		
		
		DDR1_SUSPEND_LOAD(_PUB_PTR3);
		DDR1_SUSPEND_LOAD(_PUB_PTR4);

		
	}

	// Monitor DFI initialization status.
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
	{
		while(!(readl(P_DDR0_PCTL_DFISTSTAT0) & 1)) {} //DDR 0
		hx_serial_puts("Aml log : DDR0 - DFI status check done\n");
	}

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
	{
		while(!(readl(P_DDR1_PCTL_DFISTSTAT0) & 1)) {} //DDR1
		hx_serial_puts("Aml log : DDR1 - DFI status check done\n");
	}

	//power up PCTL
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
		writel(1, P_DDR0_PCTL_POWCTL);				//DDR 0	

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
		writel(1, P_DDR1_PCTL_POWCTL);				//DDR 1

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while(!(readl(P_DDR0_PCTL_POWSTAT) & 1) ) {}
		hx_serial_puts("Aml log : DDR0 - PCTL power up\n");
	}

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while(!(readl(P_DDR1_PCTL_POWSTAT) & 1) ) {}
		hx_serial_puts("Aml log : DDR1 - PCTL power up\n");
	}

	//Initial UPCTL0 DDR timing.
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
	{	
		writel(g_ddr_settings[_PCTL_TREFI] & (~(1<<31)), P_DDR0_PCTL_TREFI);	
		DDR0_SUSPEND_LOAD(_PCTL_TREFI_MEM_DDR3);
		writel(g_ddr_settings[_PCTL_TREFI] | (1<<31), P_DDR0_PCTL_TREFI);	
		
		DDR0_SUSPEND_LOAD(_PCTL_TMRD);
		DDR0_SUSPEND_LOAD(_PCTL_TRFC);
		DDR0_SUSPEND_LOAD(_PCTL_TRP);
		DDR0_SUSPEND_LOAD(_PCTL_TAL);
		DDR0_SUSPEND_LOAD(_PCTL_TCWL);
		
		DDR0_SUSPEND_LOAD(_PCTL_TCL);
		DDR0_SUSPEND_LOAD(_PCTL_TRAS);
		DDR0_SUSPEND_LOAD(_PCTL_TRC);
		DDR0_SUSPEND_LOAD(_PCTL_TRCD);
		DDR0_SUSPEND_LOAD(_PCTL_TRRD);
		DDR0_SUSPEND_LOAD(_PCTL_TRTP);
		DDR0_SUSPEND_LOAD(_PCTL_TWR);


		DDR0_SUSPEND_LOAD(_PCTL_TWTR);
		DDR0_SUSPEND_LOAD(_PCTL_TEXSR);
		DDR0_SUSPEND_LOAD(_PCTL_TXP);
		DDR0_SUSPEND_LOAD(_PCTL_TDQS);
		DDR0_SUSPEND_LOAD(_PCTL_TRTW);
		DDR0_SUSPEND_LOAD(_PCTL_TCKSRE);

		DDR0_SUSPEND_LOAD(_PCTL_TCKSRX);
		DDR0_SUSPEND_LOAD(_PCTL_TMOD);
		DDR0_SUSPEND_LOAD(_PCTL_TCKE);
		DDR0_SUSPEND_LOAD(_PCTL_TZQCS);
		DDR0_SUSPEND_LOAD(_PCTL_TZQCL);
		DDR0_SUSPEND_LOAD(_PCTL_TXPDLL);

		DDR0_SUSPEND_LOAD(_PCTL_TZQCSI);
		DDR0_SUSPEND_LOAD(_PCTL_SCFG);
		//Initial UPCTL0 DDR timing.		 
	}


	//Initial UPCTL1 DDR timing.
	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
	{	
		writel(g_ddr_settings[_PCTL_TREFI] & (~(1<<31)), P_DDR1_PCTL_TREFI);	
		DDR1_SUSPEND_LOAD(_PCTL_TREFI_MEM_DDR3);
		writel(g_ddr_settings[_PCTL_TREFI] | (1<<31), P_DDR1_PCTL_TREFI);

		DDR1_SUSPEND_LOAD(_PCTL_TMRD);
		DDR1_SUSPEND_LOAD(_PCTL_TRFC);
		DDR1_SUSPEND_LOAD(_PCTL_TRP);
		DDR1_SUSPEND_LOAD(_PCTL_TAL);
		DDR1_SUSPEND_LOAD(_PCTL_TCWL);
		
		DDR1_SUSPEND_LOAD(_PCTL_TCL);
		DDR1_SUSPEND_LOAD(_PCTL_TRAS);
		DDR1_SUSPEND_LOAD(_PCTL_TRC);
		DDR1_SUSPEND_LOAD(_PCTL_TRCD);
		DDR1_SUSPEND_LOAD(_PCTL_TRRD);
		DDR1_SUSPEND_LOAD(_PCTL_TRTP);
		DDR1_SUSPEND_LOAD(_PCTL_TWR);


		DDR1_SUSPEND_LOAD(_PCTL_TWTR);
		DDR1_SUSPEND_LOAD(_PCTL_TEXSR);
		DDR1_SUSPEND_LOAD(_PCTL_TXP);
		DDR1_SUSPEND_LOAD(_PCTL_TDQS);
		DDR1_SUSPEND_LOAD(_PCTL_TRTW);
		DDR1_SUSPEND_LOAD(_PCTL_TCKSRE);

		DDR1_SUSPEND_LOAD(_PCTL_TCKSRX);
		DDR1_SUSPEND_LOAD(_PCTL_TMOD);
		DDR1_SUSPEND_LOAD(_PCTL_TCKE);
		DDR1_SUSPEND_LOAD(_PCTL_TZQCS);
		DDR1_SUSPEND_LOAD(_PCTL_TZQCL);
		DDR1_SUSPEND_LOAD(_PCTL_TXPDLL);

		DDR1_SUSPEND_LOAD(_PCTL_TZQCSI);
		DDR1_SUSPEND_LOAD(_PCTL_SCFG);
		//Initial UPCTL0 DDR timing.		 
	}


	//Set PCTL to config mode
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
		writel(UPCTL_CMD_CONFIG, P_DDR0_PCTL_SCTL); //DDR 0

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
		writel(UPCTL_CMD_CONFIG, P_DDR1_PCTL_SCTL); //DDR 1

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		while(!(readl(P_DDR0_PCTL_STAT) & 1)) {}

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		while(!(readl(P_DDR1_PCTL_STAT) & 1)) {}

	
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		//DDR 0 DFI
		DDR0_SUSPEND_LOAD(_PCTL_PPCFG);
		DDR0_SUSPEND_LOAD(_PUB_DX1GCR0);
		DDR0_SUSPEND_LOAD(_PUB_DX2GCR0);
		DDR0_SUSPEND_LOAD(_PUB_DX3GCR0);
		DDR0_SUSPEND_LOAD(_PUB_DX4GCR0);
		DDR0_SUSPEND_LOAD(_PUB_DX5GCR0);
		DDR0_SUSPEND_LOAD(_PCTL_DFISTCFG0);
		DDR0_SUSPEND_LOAD(_PCTL_DFISTCFG1);
		DDR0_SUSPEND_LOAD(_PCTL_DFITCTRLDELAY);
		DDR0_SUSPEND_LOAD(_PCTL_DFITPHYWRDATA);

		DDR0_SUSPEND_LOAD(_PCTL_DFITPHYWRLAT);
		DDR0_SUSPEND_LOAD(_PCTL_DFITRDDATAEN);
		DDR0_SUSPEND_LOAD(_PCTL_DFITPHYRDLAT);
		DDR0_SUSPEND_LOAD(_PCTL_DFITDRAMCLKDIS);
			

		DDR0_SUSPEND_LOAD(_PCTL_DFITDRAMCLKEN);
		DDR0_SUSPEND_LOAD(_PCTL_DFITCTRLUPDMIN);
		DDR0_SUSPEND_LOAD(_PCTL_DFILPCFG0);
		DDR0_SUSPEND_LOAD(_PCTL_DFITPHYUPDTYPE1);

		DDR0_SUSPEND_LOAD(_PCTL_DFIODTCFG);
		DDR0_SUSPEND_LOAD(_PCTL_DFIODTCFG1);
		
		DDR0_SUSPEND_LOAD(_PUB_DTAR0);
		DDR0_SUSPEND_LOAD(_PUB_DTAR1);
		DDR0_SUSPEND_LOAD(_PUB_DTAR2);
		DDR0_SUSPEND_LOAD(_PUB_DTAR3);
			
	}


	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		//DDR 1 DFI
		DDR1_SUSPEND_LOAD(_PCTL_PPCFG);
		DDR1_SUSPEND_LOAD(_PUB_DX1GCR0);
		DDR1_SUSPEND_LOAD(_PUB_DX2GCR0);
		DDR1_SUSPEND_LOAD(_PUB_DX3GCR0);
		DDR1_SUSPEND_LOAD(_PUB_DX4GCR0);
		DDR1_SUSPEND_LOAD(_PUB_DX5GCR0);
		DDR1_SUSPEND_LOAD(_PCTL_DFISTCFG0);
		DDR1_SUSPEND_LOAD(_PCTL_DFISTCFG1);
		DDR1_SUSPEND_LOAD(_PCTL_DFITCTRLDELAY);
		DDR1_SUSPEND_LOAD(_PCTL_DFITPHYWRDATA);

		DDR1_SUSPEND_LOAD(_PCTL_DFITPHYWRLAT);
		DDR1_SUSPEND_LOAD(_PCTL_DFITRDDATAEN);
		DDR1_SUSPEND_LOAD(_PCTL_DFITPHYRDLAT);
		DDR1_SUSPEND_LOAD(_PCTL_DFITDRAMCLKDIS);		

		DDR1_SUSPEND_LOAD(_PCTL_DFITDRAMCLKEN);
		DDR1_SUSPEND_LOAD(_PCTL_DFITCTRLUPDMIN);
		DDR1_SUSPEND_LOAD(_PCTL_DFILPCFG0);
		DDR1_SUSPEND_LOAD(_PCTL_DFITPHYUPDTYPE1);

		DDR1_SUSPEND_LOAD(_PCTL_DFIODTCFG);
		DDR1_SUSPEND_LOAD(_PCTL_DFIODTCFG1);
		
		DDR1_SUSPEND_LOAD(_PUB_DTAR0);
		DDR1_SUSPEND_LOAD(_PUB_DTAR1);
		DDR1_SUSPEND_LOAD(_PUB_DTAR2);
		DDR1_SUSPEND_LOAD(_PUB_DTAR3);
			
	}


	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
		writel(1,P_DDR0_PCTL_CMDTSTATEN);			//DDR 0

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
		writel(1,P_DDR1_PCTL_CMDTSTATEN);			//DDR 0

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		while(!(readl(P_DDR0_PCTL_CMDTSTAT) & 0x1)) {}

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		while(!(readl(P_DDR1_PCTL_CMDTSTAT) & 0x1)) {}


	hx_serial_puts("Aml log : HX DDR PUB training begin:\n");

	//HX PUB INIT
	
	//===============================================
	//PLL,DCAL,PHY RST,ZCAL
	nTempVal =	PUB_PIR_ZCAL | PUB_PIR_PLLINIT | PUB_PIR_DCAL | PUB_PIR_PHYRST;
	
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
		writel(nTempVal, P_DDR0_PUB_PIR); //DDR 0	

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
		writel(nTempVal, P_DDR1_PUB_PIR); //DDR 1	

		
	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR); //DDR 0
	
	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR); //DDR 1
	
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000000f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000000f))
		{				
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_ZCERR)
				goto pub_init_ddr0;
		}
		hx_serial_puts("Aml log : DDR0 - PLL,DCAL,RST,ZCAL done\n");
	}
		
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)		//DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000000f) && 
			(readl(P_DDR1_PUB_PGSR0) != 0xC000000f))
		{
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_ZCERR)
				goto pub_init_ddr1;
		}
		hx_serial_puts("Aml log : DDR1 - PLL,DCAL,RST,ZCAL done\n");
	}

	//===============================================
	//DRAM INIT
	nTempVal =	PUB_PIR_DRAMRST | PUB_PIR_DRAMINIT ;

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR); //DDR 0

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR); //DDR 1

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR); //DDR 0

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000001f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000001f))
		{
			if(readl(P_DDR0_PUB_PGSR0) & 1)
				break;	
		}
		hx_serial_puts("Aml log : DDR0 - DRAM INIT done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000001f) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC000001f))
		{
			if(readl(P_DDR1_PUB_PGSR0) & 1)
				break;	
		}
		hx_serial_puts("Aml log : DDR1 - DRAM INIT done\n");
	}

	//===============================================	
	//WL init
	nTempVal =	PUB_PIR_WL ;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0	
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000003f) && 
			(readl(P_DDR0_PUB_PGSR0) != 0xC000003f))
		{
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_WLERR)
				goto pub_init_ddr0;
		}
		hx_serial_puts("Aml log : DDR0 - WL done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000003f) && 
			(readl(P_DDR1_PUB_PGSR0) != 0xC000003f))
		{
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_WLERR)
				goto pub_init_ddr1;
		}
		hx_serial_puts("Aml log : DDR1 - WL done\n");
	}
	
	//===============================================	
	//DQS Gate training
	nTempVal =	PUB_PIR_QSGATE ;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR); //DDR 1


	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000007f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000007f))
		{
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_QSGERR)
				goto pub_init_ddr0;
		}
		hx_serial_puts("Aml log : DDR0 - DQS done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000007f) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC000007f))
		{
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_QSGERR)
				goto pub_init_ddr0;
		}
		hx_serial_puts("Aml log : DDR1 - DQS done\n");
	}

	//===============================================	
	//Write leveling ADJ
	nTempVal =	PUB_PIR_WLADJ ;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR); //DDR 0
	
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x800000ff) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC00000ff))
		{
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_WLAERR)
			{	
				hx_serial_puts("Aml log : DDR0 - WLADJ fail...\n");
				goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - WLADJ done\n");
	}


	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//HX DDR init code
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x800000ff) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC00000ff))
		{
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_WLAERR)
			{
				hx_serial_puts("Aml log : DDR1 - WLADJ fail...\n");
				goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - WLADJ done\n");
	}

	//===============================================	
	//Data bit deskew & data eye training
	nTempVal =	PUB_PIR_RDDSKW | PUB_PIR_WRDSKW | PUB_PIR_RDEYE | PUB_PIR_WREYE;
	
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR); //DDR 0

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR); //DDR 1

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x80000fff)&&
			(readl(P_DDR0_PUB_PGSR0) != 0xC0000fff))
		{
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_DTERR)
			{		
			    hx_serial_puts("Aml log : DDR0 - Bit deskew error with 0x");
			    hx_serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
			    hx_serial_puts("\n");
			    goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - BIT deskew & data eye done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x80000fff) && 
			(readl(P_DDR1_PUB_PGSR0) != 0xC0000fff))
		{
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_DTERR)
			{
			    hx_serial_puts("Aml log : DDR1 - Bit deskew error with 0x");
			    hx_serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
			    hx_serial_puts("\n");

			    goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - BIT deskew & data eye done\n");
	}

	//===============================================

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		DDR0_SUSPEND_LOAD(_PUB_PGCR3);
		DDR0_SUSPEND_LOAD(_PUB_PGCR0);
	}
	
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
		DDR1_SUSPEND_LOAD(_PUB_PGCR3);
		DDR1_SUSPEND_LOAD(_PUB_PGCR0);
	}

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		DDR0_SUSPEND_LOAD(_PUB_DXCCR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
		DDR1_SUSPEND_LOAD(_PUB_DXCCR);
	

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0
		writel(UPCTL_CMD_GO, P_DDR0_PCTL_SCTL); //DDR 0 

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1	
		writel(UPCTL_CMD_GO, P_DDR1_PCTL_SCTL); //DDR 1

	//if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0	
	{
		while ((readl(P_DDR0_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_ACCESS ) {}
		hx_serial_puts("Aml log : DDR0 - PCTL enter run state\n");
	}

	//if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1
	{
		while ((readl(P_DDR1_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_ACCESS ) {}
		hx_serial_puts("Aml log : DDR1 - PCTL enter run state\n");
	}


	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)	//DDR 0	
	{
		nTempVal = readl(P_DDR0_PUB_PGSR0);
	    if (( (nTempVal >> 20) & 0xfff ) != 0xC00 )
	    {
		hx_serial_puts("\nAml log : DDR0 PUB init fail with PGSR0 : 0x");
		hx_serial_put_hex(nTempVal,32);
		hx_serial_puts("\nTry again with MMC reset!\n");

		goto mmc_init_all;
	    }
	    else
	    {	    	
			hx_serial_puts("\nAml log : DDR0 - PUB init pass!");
			
			hx_serial_puts("\n  PGSR0 : 0x");
			hx_serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);

			hx_serial_puts("\n  PGSR1 : 0x");
			hx_serial_put_hex(readl(P_DDR0_PUB_PGSR1),32);
			hx_serial_puts("\n\n");		
	    }
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)	//DDR 1	
	{

		nTempVal = readl(P_DDR1_PUB_PGSR0);
		if (( (nTempVal >> 20) & 0xfff ) != 0xC00 )
		{
		    hx_serial_puts("\nAml log : DDR1 PUB init fail with PGSR0 : 0x");
		    hx_serial_put_hex(nTempVal,32);
		    hx_serial_puts("\nTry again with MMC reset!\n");

		    goto mmc_init_all;
		}
		else
		{
			hx_serial_puts("\nAml log : DDR1 PUB init pass!");
			
			hx_serial_puts("\n  PGSR0 : 0x");
			hx_serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);

			hx_serial_puts("\n  PGSR1 : 0x");
			hx_serial_put_hex(readl(P_DDR1_PUB_PGSR1),32);
			hx_serial_puts("\n\n");			
		}
	}

#if 0
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		DDR0_SUSPEND_LOAD(_PUB_ZQCR);
		writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);  //for power pctl gate clk save EE power	
	}
	
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
		DDR1_SUSPEND_LOAD(_PUB_ZQCR);
		writel(readl(P_DDR1_CLK_CTRL) & (~1), P_DDR1_CLK_CTRL); //for power pctl gate clk save EE power	
	}
#endif

	nRet = 0;
	return nRet;
	
   #undef DDR0_SUSPEND_LOAD
   #undef DDR1_SUSPEND_LOAD
	
}

static void hx_dmc_init()
{
	if(M8_CHIP_ID_M8M2 == m8_chip_id)
		writel(g_ddr_settings[_DMC_DDR_CTRL], P_DMC_DDR_CTRL);
	else
		writel(g_ddr_settings[_MMC_DDR_CTRL], P_MMC_DDR_CTRL);
/*
	writel(0x00000000, DMC_SEC_RANGE0_ST);
	writel(0xffffffff, DMC_SEC_RANGE0_END);
	
	writel(0xffff, DMC_SEC_PORT0_RANGE0);
	writel(0xffff, DMC_SEC_PORT1_RANGE0);
	writel(0xffff, DMC_SEC_PORT2_RANGE0);
	writel(0xffff, DMC_SEC_PORT3_RANGE0);
	writel(0xffff, DMC_SEC_PORT4_RANGE0);
	
	writel(0xffff, DMC_SEC_PORT5_RANGE0);
	writel(0xffff, DMC_SEC_PORT6_RANGE0);
	writel(0xffff, DMC_SEC_PORT7_RANGE0);
	writel(0xffff, DMC_SEC_PORT8_RANGE0);
	writel(0xffff, DMC_SEC_PORT9_RANGE0);
	
	writel(0xffff, DMC_SEC_PORT10_RANGE0);
	writel(0xffff, DMC_SEC_PORT11_RANGE0);
	
	writel(0x80000000, DMC_SEC_CTRL);
	
	while( readl(DMC_SEC_CTRL) & (1<<31)) {}

	//enable all channel
	hx_mmc_req_set(1);
*/
	//need fine tune
	/*
	//for performance enhance
	//MMC will take over DDR refresh control
	writel(timing_set->t_mmc_ddr_timming0, P_MMC_DDR_TIMING0); 
	writel(timing_set->t_mmc_ddr_timming1, P_MMC_DDR_TIMING1);
	writel(timing_set->t_mmc_ddr_timming2, P_MMC_DDR_TIMING2); 
	writel(timing_set->t_mmc_arefr_ctrl, P_MMC_AREFR_CTRL);
	*/
}

static void hx_ddr_resume(void)
{
	hx_init_pctl_ddr3();
	
	hx_dmc_init();

	//hx_dump_pub_pctl_all();	

	//hx_dump_mmc_dmc_all();

	writel((0x7f<<9)|(readl(P_DDR0_PUB_PGCR3)),
		P_DDR0_PUB_PGCR3);
	writel(readl(P_DDR0_PUB_ZQCR) | (0x4),
		P_DDR0_PUB_ZQCR);

	writel((0x7f<<9)|(readl(P_DDR1_PUB_PGCR3)),
		P_DDR1_PUB_PGCR3);
	writel(readl(P_DDR1_PUB_ZQCR) | (0x4),
		P_DDR1_PUB_ZQCR);

	__udelayx(1);

	switch(g_ddr_settings[M8_DDR_CHANNEL_SET])
	{
	case CONFIG_M8_DDR0_ONLY:
		{
			//close DDR-1
			writel(UPCTL_CMD_SLEEP, P_DDR1_PCTL_SCTL);
			while ((readl(P_DDR1_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_LOW_POWER ) {}

			writel(1<<29, P_DDR1_PUB_PLLCR);
			__udelayx(1);
			writel((1<<2)|(1<<6)|(1<<4),
				P_DDR1_CLK_CTRL);

			writel(readl(P_DDR0_CLK_CTRL) & (~1),
				P_DDR0_CLK_CTRL);
		}break;
	case CONFIG_M8_DDR1_ONLY:
		{
			//close DDR-0
			writel(UPCTL_CMD_SLEEP, P_DDR0_PCTL_SCTL);
			while ((readl(P_DDR0_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_LOW_POWER ) {}

			writel(1<<29, P_DDR0_PUB_PLLCR);
			__udelayx(1);
			writel((1<<2)|(1<<6)|(1<<4),
				P_DDR0_CLK_CTRL);

			writel(readl(P_DDR1_CLK_CTRL) & (~1),
				P_DDR1_CLK_CTRL);
		}break;
	default:
		{
			writel(readl(P_DDR0_CLK_CTRL) & (~1),
				P_DDR0_CLK_CTRL);
			writel(readl(P_DDR1_CLK_CTRL) & (~1),
				P_DDR1_CLK_CTRL);
		}break;
	}

	__udelayx(1);	
}

static void hx_ddr_pll_init()
{
	//asm volatile ("wfi");
	
	hx_serial_puts("Aml log : hx_init_ddr_pll...");

	writel(readl(P_AM_ANALOG_TOP_REG1)|1,P_AM_ANALOG_TOP_REG1);

	do{
		//BGP setting
		writel(readl(0xc8000410)& (~(1<<12)),0xc8000410);
		writel(readl(0xc8000410)|(1<<12),0xc8000410);
			 
		writel((1<<29),AM_DDR_PLL_CNTL); 
		writel(g_ddr_settings[_DDR_PLL_CNTL1],AM_DDR_PLL_CNTL1);
		writel(g_ddr_settings[_DDR_PLL_CNTL2],AM_DDR_PLL_CNTL2);
		writel(g_ddr_settings[_DDR_PLL_CNTL3],AM_DDR_PLL_CNTL3);
		writel(g_ddr_settings[_DDR_PLL_CNTL4],AM_DDR_PLL_CNTL4);
		writel(g_ddr_settings[_DDR_PLL_CNTL]|(1<<29),AM_DDR_PLL_CNTL);         
		writel(readl(AM_DDR_PLL_CNTL) & (~(1<<29)),AM_DDR_PLL_CNTL);

		__udelayx(100);
	       
	} while((readl(AM_DDR_PLL_CNTL)&(1<<31))==0);

    writel(0, P_DDR0_SOFT_RESET);
    writel(0, P_DDR1_SOFT_RESET);

	writel(0x80004040,AM_DDR_CLK_CNTL);
	writel(0x90004040,AM_DDR_CLK_CNTL);
	writel(0xb0004040,AM_DDR_CLK_CNTL);
	writel(0x90004040,AM_DDR_CLK_CNTL);

	hx_serial_puts("done\n");	  
}

void hx_ddr_power_down_leave()
{		
	hx_serial_puts("Aml log : hx_power_down_leave\n");

	hx_ddr_pll_init();

	//hx_dump_mmc_dmc_all();

	//shut down PUB-PLL
	//if(CONFIG_M8_DDR1_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET])
	//	writel(g_ddr_settings[_PUB_PLLCR], P_DDR0_PUB_PLLCR);
		
	//if(CONFIG_M8_DDR0_ONLY != g_ddr_settings[M8_DDR_CHANNEL_SET])
	//	writel(g_ddr_settings[_PUB_PLLCR], P_DDR1_PUB_PLLCR);
	
	//disable retention before init_pctl
	//because init_pctl you need to data training stuff.
	hx_ddr_retention_set(0);	

	// initialize mmc and put it to sleep
	hx_ddr_resume();
}
#endif //#define __M8_DDR_SUSPEND__

void ddr_self_refresh(void)
{
	/*get chip id for dynamic identify*/
	m8_chip_id = readl(0xc11081A8);
	//DDR suspend
	f_serial_puts("enter ddr_self_refresh\n");
	hx_ddr_power_down_enter();	
}

/*
  * Resume ddr self-refresh
*/
void ddr_resume(void)
{
	//DDR resume
	f_serial_puts("enter ddr_resume\n");
	hx_ddr_power_down_leave();
}
