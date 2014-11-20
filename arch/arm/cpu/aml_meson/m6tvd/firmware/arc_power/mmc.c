#include <asm/arch/uart.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/pctl.h>
#include <asm/arch/dmc.h>
#include <asm/arch/ddr.h>
#include <asm/arch/memtest.h>
#include <asm/arch/pctl.h>
#include <m6tv_mmc.h>
#include <config.h>

//following code not used currently
#if 1
	//M6 all pll controler use bit 29 as reset bit
#define M6TV_PLL_RESET(pll) \
	Wr(pll,Rd(pll) | (1<<29));

//wait for pll lock
//must wait first (100us+) then polling lock bit to check
#define M6TV_PLL_WAIT_FOR_LOCK(pll) \
	do{\
		__udelayx(1000);\
	}while((Rd(pll)&0x80000000)==0);
	
#endif

//MMC LOW Power enable switch
#define MMC_LOWPWR_ENABLE (0x5<<28)

static void __udelayx(int n)
{	
	int i;
	for(i=0;i<n;i++)
	{
	    asm("mov r0,r0");
	}
}

static void hx_disable_mmc_req(void)
{	
	f_serial_puts("hx_disable_mmc_req\n");
	
	writel(0, P_MMC_REQ_CTRL);
  	
  	//while(APB_Rd(MMC_CHAN_STS) == 0) //To check??
  	{
		__udelayx(100);
	}
}

#if 0
static void hx_enable_mmc_req(void)
{
	f_serial_puts("hx_enable_mmc_req\n");
	
	writel(0x1ff, P_MMC_REQ_CTRL);
	__udelayx(100);
}
#endif

//temp solution for DDR resume fail with Android
//To Android system reset MMC will cause 
//ARM fail to access DDR for most times
//But with busybox it does work and no fail found
#define M6TV_MMC_RESET_TEMP
static void hx_reset_mmc(void)
{
	f_serial_puts("hx_reset_mmc\n");
#ifdef	M6TV_MMC_RESET_TEMP
	f_serial_puts("NOT DO MMC reset! --> Please check!\n");
	return;
#endif
	//reset all sub module
	writel(0, P_MMC_SOFT_RST);
	writel(0, P_MMC_SOFT_RST1); //To check??
	while((readl(P_MMC_RST_STS)&0xffff) != 0x0);
	//while((readl(P_MMC_RST_STS1)&0xffff) != 0x0); //To check??

	__udelayx(100);// wait clock stable //To check??

	//deseart all reset.
	writel((~0), P_MMC_SOFT_RST);
	writel((~0), P_MMC_SOFT_RST1);//To check??
	while((readl(P_MMC_RST_STS)) != (~0));
	//while((readl(P_MMC_RST_STS1)) != (~0)); //To check??
	__udelayx(100);	// wait DLL lock.

}

#define  UPCTL_STAT_MASK		(7)
#ifndef  UPCTL_STAT_INIT
#define  UPCTL_STAT_INIT        (0)
#endif
#ifndef  UPCTL_STAT_CONFIG
#define  UPCTL_STAT_CONFIG      (1)
#endif
#ifndef  UPCTL_STAT_ACCESS
#define  UPCTL_STAT_ACCESS      (3)
#endif
#ifndef  UPCTL_STAT_LOW_POWER
#define  UPCTL_STAT_LOW_POWER   (5)
#endif

#define  UPCTL_CMD_INIT         (0) 
#define  UPCTL_CMD_CONFIG       (1) 
#define  UPCTL_CMD_GO           (2) 
#define  UPCTL_CMD_SLEEP        (3) 
#define  UPCTL_CMD_WAKEUP       (4) 


static void hx_mmc_sleep(void)
{
	f_serial_puts("hx_mmc_sleep\n");
	int stat;
	do
	{		
		stat = readl(P_UPCTL_STAT_ADDR) & UPCTL_STAT_MASK;				
		if(stat == UPCTL_STAT_INIT) {
			writel(UPCTL_CMD_CONFIG,P_UPCTL_SCTL_ADDR);
		}
		else if(stat == UPCTL_STAT_CONFIG) {
			writel(UPCTL_CMD_GO,P_UPCTL_SCTL_ADDR);
		}
		else if(stat == UPCTL_STAT_ACCESS) {
			writel(UPCTL_CMD_SLEEP,P_UPCTL_SCTL_ADDR);
		}
		
		stat = readl(P_UPCTL_STAT_ADDR) & UPCTL_STAT_MASK;
		
	}while(stat != UPCTL_STAT_LOW_POWER);
	
}

#if 0
static void hx_mmc_wakeup(void)
{
	f_serial_puts("hx_mmc_wakeup\n");
	int stat;
	
	do
	{
		stat = readl(P_UPCTL_STAT_ADDR) & UPCTL_STAT_MASK;		
		if(stat == UPCTL_STAT_LOW_POWER) {
			writel(UPCTL_CMD_WAKEUP,P_UPCTL_SCTL_ADDR);
		}
		else if(stat == UPCTL_STAT_INIT) {
			writel(UPCTL_CMD_CONFIG,P_UPCTL_SCTL_ADDR);
		}
		else if(stat == UPCTL_STAT_CONFIG) {
			writel(UPCTL_CMD_GO,P_UPCTL_SCTL_ADDR);
		}
		stat = readl(P_UPCTL_STAT_ADDR) & UPCTL_STAT_MASK;
		
	} while(stat != UPCTL_STAT_ACCESS);
}
#endif

//ddr training result
typedef enum {
	_UPCTL_TOGCNT1U  =0,																				//0
	_UPCTL_TOGCNT100N,	_UPCTL_TINIT	,	_UPCTL_TRSTH	,	_UPCTL_TRSTL	,	_UPCTL_MCFG		,	//5
	_UPCTL_MCFG1	,	_UPCTL_DFIODTCFG,																//7	
	_PUB_DCR		,																					//8
	_PUB_MR0		,	_PUB_MR1		, 	_PUB_MR2		,	_PUB_MR3		,						//12
	_PUB_DTPR0		,	_PUB_DTPR1		,	_PUB_DTPR2		, 											//15	
	_PUB_PTR0		,	_PUB_PTR1		,	_PUB_PTR2		,	_PUB_PTR3		,	_PUB_PTR4		,	//20	
	_PUB_ACBDLR		,	_PUB_ACIOCR		,	_PUB_DSGCR      ,											//23	
	_UPCTL_POWCTL   ,	_UPCTL_POWSTAT  , 																//25	
	_PUB_DXCCR		,	_PUB_ZQ0CR1		,	_PUB_ZQ0CR0		,											//28
	_UPCTL_TREFI	,	_UPCTL_TMRD		, 	_UPCTL_TRFC		,	_UPCTL_TRP		,	_UPCTL_TAL		,	//33	
	_UPCTL_TCWL		,	_UPCTL_TCL		, 	_UPCTL_TRAS		,	_UPCTL_TRC		,	_UPCTL_TRCD		,	//38
	_UPCTL_TRRD		,	_UPCTL_TRTP		, 	_UPCTL_TWR		,	_UPCTL_TWTR		,	_UPCTL_TEXSR	,	//43
	_UPCTL_TXP		,	_UPCTL_TDQS		, 	_UPCTL_TRTW		,	_UPCTL_TCKSRE	,	_UPCTL_TCKSRX	,	//48
	_UPCTL_TMOD		,	_UPCTL_TCKE		, 	_UPCTL_TZQCS	,	_UPCTL_TZQCL	,	_UPCTL_TXPDLL	,	//53
	_UPCTL_TZQCSI	,	_UPCTL_SCFG		, 	_UPCTL_PPCFG	, 											//56
	_UPCTL_DFISTCFG0		, _UPCTL_DFISTCFG1		, _UPCTL_DFITCTRLDELAY	, _UPCTL_DFITPHYWRDATA	, 	//60
	_UPCTL_DFITPHYWRLAT		, _UPCTL_DFITRDDATAEN	, _UPCTL_DFITPHYRDLAT	, _UPCTL_DFITDRAMCLKDIS	, 	//64
	_UPCTL_DFITDRAMCLKEN	, _UPCTL_DFITCTRLUPDMIN	, _UPCTL_DFITPHYUPDTYPE0, _UPCTL_DFITPHYUPDTYPE1, 	//68
	_UPCTL_DFILPCFG0		, _UPCTL_DFITCTRLUPDI	, 													//70
	_MMC_LP_CTRL1		,	_MMC_LP_CTRL2	,	_MMC_LP_CTRL3	,	_MMC_LP_CTRL4	,					//74
	_UPCTL_CMDTSTATEN	,																				//71
	_PUB_PGCR0	,	_PUB_PGCR1	,	_PUB_PGCR2	,														//74
	_PUB_DTAR0	, 	_PUB_DTAR1	,	_PUB_DTAR2	,	_PUB_DTAR3	,										//78
	_PUB_DTCR	, 																						//79
	_MMC_DDR_CTRL	,																					//80
//	_HHI_SYS_PLL_CNTL    , _HHI_SYS_PLL_CNTL2	, _HHI_SYS_PLL_CNTL3	, _HHI_SYS_PLL_CNTL4	,	//84	
	_HHI_DDR_PLL_CNTL    , _HHI_DDR_PLL_CNTL2 	, _HHI_DDR_PLL_CNTL3	, _HHI_DDR_PLL_CNTL4   	,	//88
//	_HHI_VID_PLL_CNTL    , _HHI_VID_PLL_CNTL2   , _HHI_VID_PLL_CNTL3    , _HHI_VID_PLL_CNTL4    ,	//92	
//	_HHI_MPLL_CNTL       , _HHI_MPLL_CNTL2      , _HHI_MPLL_CNTL3       , _HHI_MPLL_CNTL4       ,	//96 
//	_HHI_MPLL_CNTL5      , _HHI_MPLL_CNTL6      , _HHI_MPLL_CNTL7       , _HHI_MPLL_CNTL8       ,	//100
//	_HHI_MPLL_CNTL9      , _HHI_MPLL_CNTL10     , 													//102	
//	_HHI_AUDCLK_PLL_CNTL , _HHI_AUDCLK_PLL_CNTL2, _HHI_AUDCLK_PLL_CNTL3 , _HHI_AUDCLK_PLL_CNTL4 ,	//106
//	_HHI_AUDCLK_PLL_CNTL5,_HHI_AUDCLK_PLL_CNTL6,  													//108
		
} back_reg_index;

#ifndef DDR_SETTING_COUNT
#define DDR_SETTING_COUNT 128
#endif
static unsigned int g_ddr_settings[DDR_SETTING_COUNT];
void hx_save_ddr_settings()
{	
	f_serial_puts("hx_save_ddr_settings\n");
	
	//check & turn off
	g_ddr_settings[_MMC_LP_CTRL1] = readl(P_MMC_LP_CTRL1);
	if(g_ddr_settings[_MMC_LP_CTRL1] & MMC_LOWPWR_ENABLE)
	{
		//MMC LP switch off
		writel( g_ddr_settings[_MMC_LP_CTRL1] & (~MMC_LOWPWR_ENABLE),P_MMC_LP_CTRL1 );		
	}	

	
#ifndef DDR_SUSPEND_SAVE
	#define DDR_SUSPEND_SAVE(a)  {g_ddr_settings[a] = readl(P##a##_ADDR);}
#endif

	DDR_SUSPEND_SAVE(_UPCTL_TOGCNT1U);	       
	DDR_SUSPEND_SAVE(_UPCTL_TOGCNT100N);
	DDR_SUSPEND_SAVE(_UPCTL_TINIT);
	DDR_SUSPEND_SAVE(_UPCTL_TRSTH);
	DDR_SUSPEND_SAVE(_UPCTL_TRSTL);
		
	DDR_SUSPEND_SAVE(_UPCTL_MCFG);
	DDR_SUSPEND_SAVE(_UPCTL_MCFG1);
	DDR_SUSPEND_SAVE(_UPCTL_DFIODTCFG);
	DDR_SUSPEND_SAVE(_PUB_DCR);
	
	DDR_SUSPEND_SAVE(_PUB_MR0);
	DDR_SUSPEND_SAVE(_PUB_MR1);
	DDR_SUSPEND_SAVE(_PUB_MR2);
	DDR_SUSPEND_SAVE(_PUB_MR3);
	
	DDR_SUSPEND_SAVE(_PUB_DTPR0);
	DDR_SUSPEND_SAVE(_PUB_DTPR1);
	DDR_SUSPEND_SAVE(_PUB_DTPR2);
	
	DDR_SUSPEND_SAVE(_PUB_PTR0);
	DDR_SUSPEND_SAVE(_PUB_PTR1);
	DDR_SUSPEND_SAVE(_PUB_PTR2);
	DDR_SUSPEND_SAVE(_PUB_PTR3);
	DDR_SUSPEND_SAVE(_PUB_PTR4);

	DDR_SUSPEND_SAVE(_PUB_ACBDLR);
	DDR_SUSPEND_SAVE(_PUB_ACIOCR);
	
	DDR_SUSPEND_SAVE(_PUB_DSGCR);
	DDR_SUSPEND_SAVE(_UPCTL_POWCTL);
	DDR_SUSPEND_SAVE(_UPCTL_POWSTAT);
	
	DDR_SUSPEND_SAVE(_PUB_DXCCR);
	DDR_SUSPEND_SAVE(_PUB_ZQ0CR1);
	DDR_SUSPEND_SAVE(_PUB_ZQ0CR0);
	
	DDR_SUSPEND_SAVE(_UPCTL_TREFI);
	DDR_SUSPEND_SAVE(_UPCTL_TMRD);
	DDR_SUSPEND_SAVE(_UPCTL_TRFC);
	DDR_SUSPEND_SAVE(_UPCTL_TRP);
	DDR_SUSPEND_SAVE(_UPCTL_TAL);
	
	DDR_SUSPEND_SAVE(_UPCTL_TCWL);
	DDR_SUSPEND_SAVE(_UPCTL_TCL);
	DDR_SUSPEND_SAVE(_UPCTL_TRAS);
	DDR_SUSPEND_SAVE(_UPCTL_TRC);
	DDR_SUSPEND_SAVE(_UPCTL_TRCD);
	DDR_SUSPEND_SAVE(_UPCTL_TRRD);
	DDR_SUSPEND_SAVE(_UPCTL_TRTP);
	DDR_SUSPEND_SAVE(_UPCTL_TWR);
	DDR_SUSPEND_SAVE(_UPCTL_TWTR);
	
	DDR_SUSPEND_SAVE(_UPCTL_TEXSR);
	DDR_SUSPEND_SAVE(_UPCTL_TXP);
	DDR_SUSPEND_SAVE(_UPCTL_TDQS);
	DDR_SUSPEND_SAVE(_UPCTL_TRTW);
	DDR_SUSPEND_SAVE(_UPCTL_TCKSRE);
	DDR_SUSPEND_SAVE(_UPCTL_TCKSRX);
	DDR_SUSPEND_SAVE(_UPCTL_TMOD);
	DDR_SUSPEND_SAVE(_UPCTL_TCKE);	
	DDR_SUSPEND_SAVE(_UPCTL_TZQCS);
	
	DDR_SUSPEND_SAVE(_UPCTL_TZQCL);
	DDR_SUSPEND_SAVE(_UPCTL_TXPDLL);
	DDR_SUSPEND_SAVE(_UPCTL_TZQCSI);
	DDR_SUSPEND_SAVE(_UPCTL_SCFG);
		
	DDR_SUSPEND_SAVE(_UPCTL_PPCFG);
	DDR_SUSPEND_SAVE(_UPCTL_DFISTCFG0);
	DDR_SUSPEND_SAVE(_UPCTL_DFISTCFG1);
	DDR_SUSPEND_SAVE(_UPCTL_DFITCTRLDELAY);
	DDR_SUSPEND_SAVE(_UPCTL_DFITPHYWRDATA);
	DDR_SUSPEND_SAVE(_UPCTL_DFITPHYWRLAT);		
	DDR_SUSPEND_SAVE(_UPCTL_DFITRDDATAEN);
	DDR_SUSPEND_SAVE(_UPCTL_DFITPHYRDLAT);
	DDR_SUSPEND_SAVE(_UPCTL_DFITDRAMCLKDIS);
	DDR_SUSPEND_SAVE(_UPCTL_DFITDRAMCLKEN);
	DDR_SUSPEND_SAVE(_UPCTL_DFITCTRLUPDMIN);
	DDR_SUSPEND_SAVE(_UPCTL_DFITPHYUPDTYPE0);
	DDR_SUSPEND_SAVE(_UPCTL_DFITPHYUPDTYPE1);
		
	DDR_SUSPEND_SAVE(_UPCTL_DFILPCFG0);
	DDR_SUSPEND_SAVE(_UPCTL_DFITCTRLUPDI);
	DDR_SUSPEND_SAVE(_UPCTL_CMDTSTATEN);
	
	DDR_SUSPEND_SAVE(_PUB_PGCR0);
	DDR_SUSPEND_SAVE(_PUB_PGCR1);
	DDR_SUSPEND_SAVE(_PUB_PGCR2);
	DDR_SUSPEND_SAVE(_PUB_DTAR0);
	DDR_SUSPEND_SAVE(_PUB_DTAR1);
	
	DDR_SUSPEND_SAVE(_PUB_DTAR2);
	DDR_SUSPEND_SAVE(_PUB_DTAR3);
	DDR_SUSPEND_SAVE(_PUB_DTCR);
		
#undef DDR_SUSPEND_SAVE

#ifndef DDR_SUSPEND_SAVE
	#define DDR_SUSPEND_SAVE(a)  {g_ddr_settings[a] = readl(P##a);}
#endif
/*
	//SYS PLL
	DDR_SUSPEND_SAVE(_HHI_SYS_PLL_CNTL);
	DDR_SUSPEND_SAVE(_HHI_SYS_PLL_CNTL2);
	DDR_SUSPEND_SAVE(_HHI_SYS_PLL_CNTL3);
	DDR_SUSPEND_SAVE(_HHI_SYS_PLL_CNTL4);
*/
	//DDR PLL
	DDR_SUSPEND_SAVE(_HHI_DDR_PLL_CNTL);
	DDR_SUSPEND_SAVE(_HHI_DDR_PLL_CNTL2);
	DDR_SUSPEND_SAVE(_HHI_DDR_PLL_CNTL3);
	DDR_SUSPEND_SAVE(_HHI_DDR_PLL_CNTL4);
	
	DDR_SUSPEND_SAVE(_MMC_DDR_CTRL);	
	
	if(g_ddr_settings[_MMC_LP_CTRL1])
	{		
		DDR_SUSPEND_SAVE(_MMC_LP_CTRL3);
		DDR_SUSPEND_SAVE(_MMC_LP_CTRL4);
	}
	 /*            
	//VID PLL
	DDR_SUSPEND_SAVE(_HHI_VID_PLL_CNTL);
	DDR_SUSPEND_SAVE(_HHI_VID_PLL_CNTL2);
	DDR_SUSPEND_SAVE(_HHI_VID_PLL_CNTL3);
	DDR_SUSPEND_SAVE(_HHI_VID_PLL_CNTL4);	
                     
	//MPLL 
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL2);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL3);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL4);	
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL5);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL6);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL7);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL8);	
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL9);
	DDR_SUSPEND_SAVE(_HHI_MPLL_CNTL10);
	
	//AUD PLL
	DDR_SUSPEND_SAVE(_HHI_AUDCLK_PLL_CNTL);
	DDR_SUSPEND_SAVE(_HHI_AUDCLK_PLL_CNTL2);
	DDR_SUSPEND_SAVE(_HHI_AUDCLK_PLL_CNTL3);
	DDR_SUSPEND_SAVE(_HHI_AUDCLK_PLL_CNTL4);
	DDR_SUSPEND_SAVE(_HHI_AUDCLK_PLL_CNTL5);
	DDR_SUSPEND_SAVE(_HHI_AUDCLK_PLL_CNTL6);
*/
#undef DDR_SUSPEND_SAVE
	
	/*
	int i = 0;
	int nMax = 100;
	for(i = 0 ;i<nMax;++i)
	{
		serial_puts("\n0x");
		serial_put_hex(g_ddr_settings[i],32);
	}
	serial_puts("\n");	
	*/	
}

#if 0
static void hx_dump_ddr_settings()
{
	f_serial_puts("hx_dump_ddr_settings\n");
#ifndef DDR_SUSPEND_DUMP
	#define DDR_SUSPEND_DUMP(a)  do{ f_serial_puts("\n[ 0x"); \
		serial_put_hex(P##a##_ADDR,32); \
		f_serial_puts(" ] : 0x"); \
		serial_put_hex(readl(P##a##_ADDR),32);}while(0);
#endif	

	DDR_SUSPEND_DUMP(_UPCTL_TOGCNT1U);	       
	DDR_SUSPEND_DUMP(_UPCTL_TOGCNT100N);
	DDR_SUSPEND_DUMP(_UPCTL_TINIT);
	DDR_SUSPEND_DUMP(_UPCTL_TRSTH);
	DDR_SUSPEND_DUMP(_UPCTL_TRSTL);
		
	DDR_SUSPEND_DUMP(_UPCTL_MCFG);
	DDR_SUSPEND_DUMP(_UPCTL_MCFG1);
	DDR_SUSPEND_DUMP(_UPCTL_DFIODTCFG);
	DDR_SUSPEND_DUMP(_PUB_DCR);
	
	DDR_SUSPEND_DUMP(_PUB_MR0);
	DDR_SUSPEND_DUMP(_PUB_MR1);
	DDR_SUSPEND_DUMP(_PUB_MR2);
	DDR_SUSPEND_DUMP(_PUB_MR3);
	
	DDR_SUSPEND_DUMP(_PUB_DTPR0);
	DDR_SUSPEND_DUMP(_PUB_DTPR1);
	DDR_SUSPEND_DUMP(_PUB_DTPR2);
	
	DDR_SUSPEND_DUMP(_PUB_PTR0);
	DDR_SUSPEND_DUMP(_PUB_PTR1);
	DDR_SUSPEND_DUMP(_PUB_PTR2);
	DDR_SUSPEND_DUMP(_PUB_PTR3);
	DDR_SUSPEND_DUMP(_PUB_PTR4);

	DDR_SUSPEND_DUMP(_PUB_ACBDLR);
	DDR_SUSPEND_DUMP(_PUB_ACIOCR);
	
	DDR_SUSPEND_DUMP(_PUB_DSGCR);
	DDR_SUSPEND_DUMP(_UPCTL_POWCTL);
	DDR_SUSPEND_DUMP(_UPCTL_POWSTAT);
	
	DDR_SUSPEND_DUMP(_PUB_DXCCR);
	DDR_SUSPEND_DUMP(_PUB_ZQ0CR1);
	DDR_SUSPEND_DUMP(_PUB_ZQ0CR0);
	
	DDR_SUSPEND_DUMP(_UPCTL_TREFI);
	DDR_SUSPEND_DUMP(_UPCTL_TMRD);
	DDR_SUSPEND_DUMP(_UPCTL_TRFC);
	DDR_SUSPEND_DUMP(_UPCTL_TRP);
	DDR_SUSPEND_DUMP(_UPCTL_TAL);
	
	DDR_SUSPEND_DUMP(_UPCTL_TCWL);
	DDR_SUSPEND_DUMP(_UPCTL_TCL);
	DDR_SUSPEND_DUMP(_UPCTL_TRAS);
	DDR_SUSPEND_DUMP(_UPCTL_TRC);
	DDR_SUSPEND_DUMP(_UPCTL_TRCD);
	DDR_SUSPEND_DUMP(_UPCTL_TRRD);
	DDR_SUSPEND_DUMP(_UPCTL_TRTP);
	DDR_SUSPEND_DUMP(_UPCTL_TWR);
	DDR_SUSPEND_DUMP(_UPCTL_TWTR);
	
	DDR_SUSPEND_DUMP(_UPCTL_TEXSR);
	DDR_SUSPEND_DUMP(_UPCTL_TXP);
	DDR_SUSPEND_DUMP(_UPCTL_TDQS);
	DDR_SUSPEND_DUMP(_UPCTL_TRTW);
	DDR_SUSPEND_DUMP(_UPCTL_TCKSRE);
	DDR_SUSPEND_DUMP(_UPCTL_TCKSRX);
	DDR_SUSPEND_DUMP(_UPCTL_TMOD);
	DDR_SUSPEND_DUMP(_UPCTL_TCKE);	
	DDR_SUSPEND_DUMP(_UPCTL_TZQCS);
	
	DDR_SUSPEND_DUMP(_UPCTL_TZQCL);
	DDR_SUSPEND_DUMP(_UPCTL_TXPDLL);
	DDR_SUSPEND_DUMP(_UPCTL_TZQCSI);
	DDR_SUSPEND_DUMP(_UPCTL_SCFG);
		
	DDR_SUSPEND_DUMP(_UPCTL_PPCFG);
	DDR_SUSPEND_DUMP(_UPCTL_DFISTCFG0);
	DDR_SUSPEND_DUMP(_UPCTL_DFISTCFG1);
	DDR_SUSPEND_DUMP(_UPCTL_DFITCTRLDELAY);
	DDR_SUSPEND_DUMP(_UPCTL_DFITPHYWRDATA);
	DDR_SUSPEND_DUMP(_UPCTL_DFITPHYWRLAT);		
	DDR_SUSPEND_DUMP(_UPCTL_DFITRDDATAEN);
	DDR_SUSPEND_DUMP(_UPCTL_DFITPHYRDLAT);
	DDR_SUSPEND_DUMP(_UPCTL_DFITDRAMCLKDIS);
	DDR_SUSPEND_DUMP(_UPCTL_DFITDRAMCLKEN);
	DDR_SUSPEND_DUMP(_UPCTL_DFITCTRLUPDMIN);
	DDR_SUSPEND_DUMP(_UPCTL_DFITPHYUPDTYPE0);
	DDR_SUSPEND_DUMP(_UPCTL_DFITPHYUPDTYPE1);
	
	DDR_SUSPEND_DUMP(_UPCTL_DFILPCFG0);
	DDR_SUSPEND_DUMP(_UPCTL_DFITCTRLUPDI);
	DDR_SUSPEND_DUMP(_UPCTL_CMDTSTATEN);
	
	DDR_SUSPEND_DUMP(_PUB_PGCR0);
	DDR_SUSPEND_DUMP(_PUB_PGCR1);
	DDR_SUSPEND_DUMP(_PUB_PGCR2);
	DDR_SUSPEND_DUMP(_PUB_DTAR0);
	DDR_SUSPEND_DUMP(_PUB_DTAR1);
	
	DDR_SUSPEND_DUMP(_PUB_DTAR2);
	DDR_SUSPEND_DUMP(_PUB_DTAR3);
	DDR_SUSPEND_DUMP(_PUB_DTCR);  
	
	DDR_SUSPEND_DUMP(_PUB_DTEDR0);  
	DDR_SUSPEND_DUMP(_PUB_DTEDR1);  
	
	f_serial_puts("\n\n");
#undef 	DDR_SUSPEND_DUMP	

}

static void hx_dump_pub_pctl_all()
{
	f_serial_puts("hx_dump_pub_pctl_all\n");
#ifndef DDR_SUSPEND_DUMP_ALL
	#define DDR_SUSPEND_DUMP_ALL(id,a)  do{ f_serial_puts("\n[ 0x"); \
		serial_put_hex(id|(a<<2),32); \
		f_serial_puts(" ] : 0x"); \
		serial_put_hex(readl(id|(a<<2)),32);}while(0);
#endif	

	int i,nMax,id;
	
	f_serial_puts("\n============================================");
	f_serial_puts("\nPUB dump : \n");
	for(i = 0,nMax=254,id = 0xc8001000;i<nMax;++i)
	{
		DDR_SUSPEND_DUMP_ALL(id,i);
	}
	
	f_serial_puts("\n============================================");
	f_serial_puts("\nPCTL dump : \n");
	
	for(i = 0,nMax=254,id = 0xc8000000;i<nMax;++i)
	{
		DDR_SUSPEND_DUMP_ALL(id,i);
	}
	
	f_serial_puts("\n============================================\n");

#undef DDR_SUSPEND_DUMP_ALL 
}

static void hx_dump_mmc_dmc_all()
{
	f_serial_puts("hx_dump_mmc_dmc_all\n");
#ifndef DDR_SUSPEND_DUMP_ALL
	#define DDR_SUSPEND_DUMP_ALL(id,a)  do{ f_serial_puts("\n[ 0x"); \
		serial_put_hex(id|(a<<2),32); \
		f_serial_puts(" ] : 0x"); \
		serial_put_hex(readl(id|(a<<2)),32);}while(0);
#endif	

	int i,nMax,id;
	
	f_serial_puts("\n============================================");
	f_serial_puts("\nMMC dump : \n");
	for(i = 0,nMax=278,id = 0xc8006000;i<nMax;++i)
	{
		DDR_SUSPEND_DUMP_ALL(id,i);
	}
	
	f_serial_puts("\n============================================");
	f_serial_puts("\nDMC SEC dump : \n");
	
	for(i = 0,nMax=71,id = 0xda002000;i<nMax;++i)
	{
		DDR_SUSPEND_DUMP_ALL(id,i);
	}
	
	f_serial_puts("\n============================================\n");

#undef DDR_SUSPEND_DUMP_ALL 
}
#endif

static void hx_store_restore_plls(int flag)
{
	f_serial_puts("hx_store_restore_plls\n");
	//store or load PLL setting
	//note: bandgap !!!
			
}
static void hx_enable_retention(void)
{
	f_serial_puts("hx_enable_retention\n");
	 //RENT_N/RENT_EN_N switch from 01 to 10 (2'b10 = ret_enable)
	writel((readl(P_AO_RTI_PWR_CNTL_REG0)&(~(3<<16)))|(2<<16),P_AO_RTI_PWR_CNTL_REG0);
	__udelayx(200000);
}

static void hx_disable_retention(void)
{
	f_serial_puts("hx_disable_retention\n");
	//RENT_N/RENT_EN_N switch from 10 to 01
	writel((readl(P_AO_RTI_PWR_CNTL_REG0)&(~(3<<16)))|(1<<16),P_AO_RTI_PWR_CNTL_REG0);
	__udelayx(200);
}

void hx_enter_power_down()
{
	f_serial_puts("hx_enter_power_down\n");

	asm(".long 0x003f236f"); //add sync instruction.

	hx_disable_mmc_req();

	//serial_put_hex(readl(P_MMC_LP_CTRL1),32);
	//serial_puts("  LP_CTRL1\n");
	//wait_uart_empty();

	//serial_put_hex(readl(P_UPCTL_MCFG_ADDR),32);
	//serial_puts("  MCFG\n");
	//wait_uart_empty();

	hx_store_restore_plls(1);

	//APB_Wr(UPCTL_SCTL_ADDR, 1);
	//writel(SCTL_CMD_CONFIG,P_UPCTL_SCTL_ADDR);
	//APB_Wr(UPCTL_MCFG_ADDR, 0x60021 );
	//writel(0xa0029,P_UPCTL_MCFG_ADDR); //???
	//APB_Wr(UPCTL_SCTL_ADDR, 2);
	//writel(SCTL_CMD_GO,P_UPCTL_SCTL_ADDR);

	//serial_put_hex(APB_Rd(UPCTL_MCFG_ADDR),32);
	//serial_put_hex(readl(P_UPCTL_MCFG_ADDR),32);
	//serial_puts("  MCFG\n");
	//wait_uart_empty();

	// MMC sleep 
	f_serial_puts("Start DDR off\n");
	//wait_uart_empty();
	
	//Next, we sleep
	hx_mmc_sleep();

	//Clear PGCR0 CK
	writel(readl(P_PUB_PGCR0_ADDR)&(~(0x3F<<26)),P_PUB_PGCR0_ADDR);
		
	// enable retention
	//only necessory if you want to shut down the EE 1.1V and/or DDR I/O 1.5V power supply.
	//but we need to check if we enable this feature, we can save more power on DDR I/O 1.5V domain or not.
	hx_enable_retention();

	// save ddr power
	// before shut down DDR PLL, keep the DDR PHY DLL in reset mode.
	// that will save the DLL analog power.
	f_serial_puts("mmc soft rst\n");
	
#ifdef	M6TV_MMC_RESET_TEMP
	f_serial_puts("Only PUB/PCTL soft reset! --> Please check!\n");
	#define MMC_RESET_SELECT (0x1f)
	unsigned nTempVal = readl(P_MMC_SOFT_RST) & (~(MMC_RESET_SELECT<<25));			
	writel(nTempVal, P_MMC_SOFT_RST);
	#undef MMC_RESET_SELECT
#else
	writel(0, P_MMC_SOFT_RST);
    writel(0, P_MMC_SOFT_RST1); //To check??
#endif 

	// shut down DDR PLL. 
	writel(readl(P_HHI_DDR_PLL_CNTL)|(1<<30),P_HHI_DDR_PLL_CNTL);

	f_serial_puts("Done DDR off\n");
	//wait_uart_empty();
}

static int hx_init_pctl_ddr3()
{	
	int nTempVal = 0;	

	//asm volatile ("wfi");	
	
#ifndef DDR_SUSPEND_LOAD
	#define DDR_SUSPEND_LOAD(a)  {writel(g_ddr_settings[a],P##a##_ADDR);}
#endif

pub_retry:

	//try to reset mmc ...
	// bit 30.  lower power control module reset bit.
	
	// bit 29.  PHY IP reset bit.
	
	// bit 28.  uPCTL APB bus interface reset bit.
	// bit 27.  uPCTL IP reset bit.
	// bit 26.  PUBL  APB bus interface reset bit.
	// bit 25.  PUBL IP reset bit
  	
#define MMC_RESET_SELECT (0x1f)

	nTempVal = readl(P_MMC_SOFT_RST) & (~(MMC_RESET_SELECT<<25));			
	writel(nTempVal, P_MMC_SOFT_RST);	
	__udelayx(10000);
	nTempVal |= (MMC_RESET_SELECT<<25);
	writel(nTempVal, P_MMC_SOFT_RST);
	__udelayx(10000);
	
	//UPCTL memory timing registers
	DDR_SUSPEND_LOAD(_UPCTL_TOGCNT1U);
	DDR_SUSPEND_LOAD(_UPCTL_TOGCNT100N);
	DDR_SUSPEND_LOAD(_UPCTL_TINIT);
	DDR_SUSPEND_LOAD(_UPCTL_TRSTH);
	DDR_SUSPEND_LOAD(_UPCTL_TRSTL);
	
	DDR_SUSPEND_LOAD(_UPCTL_MCFG);		
	DDR_SUSPEND_LOAD(_UPCTL_MCFG1);	    
	  
	DDR_SUSPEND_LOAD(_UPCTL_DFIODTCFG);
    	  
    DDR_SUSPEND_LOAD(_PUB_DCR);	  	

	DDR_SUSPEND_LOAD(_PUB_MR0);	  	
	DDR_SUSPEND_LOAD(_PUB_MR1);	  	
	DDR_SUSPEND_LOAD(_PUB_MR2);	  	
	DDR_SUSPEND_LOAD(_PUB_MR3);	  	
	
	DDR_SUSPEND_LOAD(_PUB_DTPR0);	
	DDR_SUSPEND_LOAD(_PUB_DTPR1);	
	DDR_SUSPEND_LOAD(_PUB_DTPR2);	
	
	DDR_SUSPEND_LOAD(_PUB_PTR0);
	DDR_SUSPEND_LOAD(_PUB_PTR1);
	DDR_SUSPEND_LOAD(_PUB_PTR2);
	DDR_SUSPEND_LOAD(_PUB_PTR3);
	DDR_SUSPEND_LOAD(_PUB_PTR4);

	DDR_SUSPEND_LOAD(_PUB_ACBDLR);
	
	DDR_SUSPEND_LOAD(_PUB_ACIOCR);
	writel(readl(P_PUB_ACIOCR_ADDR) & 0xdfffffff, P_PUB_ACIOCR_ADDR);
	
	
	DDR_SUSPEND_LOAD(_PUB_DSGCR);

	writel(1, P_UPCTL_POWCTL_ADDR);
	
	while(!(readl(P_UPCTL_POWSTAT_ADDR) & 1) ) {}

	DDR_SUSPEND_LOAD(_PUB_DXCCR);   
	
	if(g_ddr_settings[_PUB_ZQ0CR0])
        writel(g_ddr_settings[_PUB_ZQ0CR0] | (1<<28), P_PUB_ZQ0CR0_ADDR);
    else
        DDR_SUSPEND_LOAD(_PUB_ZQ0CR1);
    		
	// initial upctl ddr timing.
	DDR_SUSPEND_LOAD(_UPCTL_TREFI);
	DDR_SUSPEND_LOAD(_UPCTL_TMRD);
	DDR_SUSPEND_LOAD(_UPCTL_TRFC);
	DDR_SUSPEND_LOAD(_UPCTL_TRP);
	DDR_SUSPEND_LOAD(_UPCTL_TAL);
	DDR_SUSPEND_LOAD(_UPCTL_TCWL);
	DDR_SUSPEND_LOAD(_UPCTL_TCL);
	
	
	DDR_SUSPEND_LOAD(_UPCTL_TRAS);
	DDR_SUSPEND_LOAD(_UPCTL_TRC);
	DDR_SUSPEND_LOAD(_UPCTL_TRCD);
	DDR_SUSPEND_LOAD(_UPCTL_TRRD);
	DDR_SUSPEND_LOAD(_UPCTL_TRTP);		
	DDR_SUSPEND_LOAD(_UPCTL_TWR);
	
	DDR_SUSPEND_LOAD(_UPCTL_TWTR);
	DDR_SUSPEND_LOAD(_UPCTL_TEXSR);
	DDR_SUSPEND_LOAD(_UPCTL_TXP);
	DDR_SUSPEND_LOAD(_UPCTL_TDQS);
			
	DDR_SUSPEND_LOAD(_UPCTL_TRTW);
	DDR_SUSPEND_LOAD(_UPCTL_TCKSRE);
	DDR_SUSPEND_LOAD(_UPCTL_TCKSRX);
	DDR_SUSPEND_LOAD(_UPCTL_TMOD);
	
	DDR_SUSPEND_LOAD(_UPCTL_TCKE);
	
	DDR_SUSPEND_LOAD(_UPCTL_TZQCS);
	DDR_SUSPEND_LOAD(_UPCTL_TZQCL);
	DDR_SUSPEND_LOAD(_UPCTL_TXPDLL);
	DDR_SUSPEND_LOAD(_UPCTL_TZQCSI);
	DDR_SUSPEND_LOAD(_UPCTL_SCFG);

	writel(UPCTL_CMD_CONFIG, P_UPCTL_SCTL_ADDR);
	
	while (!(readl(P_UPCTL_STAT_ADDR) & UPCTL_STAT_CONFIG)){
		writel(UPCTL_CMD_CONFIG, P_UPCTL_SCTL_ADDR);
	}

	DDR_SUSPEND_LOAD(_UPCTL_PPCFG);
	DDR_SUSPEND_LOAD(_UPCTL_DFISTCFG0);
	DDR_SUSPEND_LOAD(_UPCTL_DFISTCFG1);
	DDR_SUSPEND_LOAD(_UPCTL_DFITCTRLDELAY);
	DDR_SUSPEND_LOAD(_UPCTL_DFITPHYWRDATA);
	DDR_SUSPEND_LOAD(_UPCTL_DFITPHYWRLAT);
	DDR_SUSPEND_LOAD(_UPCTL_DFITRDDATAEN);
	
	DDR_SUSPEND_LOAD(_UPCTL_DFITPHYRDLAT);
	DDR_SUSPEND_LOAD(_UPCTL_DFITDRAMCLKDIS);
	DDR_SUSPEND_LOAD(_UPCTL_DFITDRAMCLKEN);
	DDR_SUSPEND_LOAD(_UPCTL_DFITCTRLUPDMIN);
	
	DDR_SUSPEND_LOAD(_UPCTL_DFITPHYUPDTYPE0);
	DDR_SUSPEND_LOAD(_UPCTL_DFITPHYUPDTYPE1);
	
	DDR_SUSPEND_LOAD(_UPCTL_DFILPCFG0);
	DDR_SUSPEND_LOAD(_UPCTL_DFITCTRLUPDI);
	
	writel(1,P_UPCTL_CMDTSTATEN_ADDR);
	while (!(readl(P_UPCTL_CMDTSTAT_ADDR) & 1 )) {}


	DDR_SUSPEND_LOAD(_PUB_PGCR0);
	DDR_SUSPEND_LOAD(_PUB_PGCR1);
	DDR_SUSPEND_LOAD(_PUB_PGCR2);

	//DTAR setting, load but not calculate again.
	DDR_SUSPEND_LOAD(_PUB_DTAR0);
	DDR_SUSPEND_LOAD(_PUB_DTAR1);
	DDR_SUSPEND_LOAD(_PUB_DTAR2);
	DDR_SUSPEND_LOAD(_PUB_DTAR3);
	//set training address to 0x9fffff00
	//MMC_Wr( PUB_DTAR_ADDR, (0xFc0 | (0xFFFF <<12) | (7 << 28))); //let training address is 0x9fffff00;
	//writel((0x000 | (0xFFF0 <<12) | (0x7 << 28)),P_PUB_DTAR0_ADDR);
	//writel((0x008 | (0xFFF0 <<12) | (0x7 << 28)),P_PUB_DTAR1_ADDR);
	//writel((0x010 | (0xFFF0 <<12) | (0x7 << 28)),P_PUB_DTAR2_ADDR);
	//writel((0x018 | (0xFFF0 <<12) | (0x7 << 28)),P_PUB_DTAR3_ADDR);
	
	DDR_SUSPEND_LOAD(_PUB_DTCR);
	
	
#define PUB_PIR_INIT  		(1<<0)
#define PUB_PIR_ZCAL  		(1<<1)
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


#define PUB_PGSR0_ZCERR     (1<<20)
#define PUB_PGSR0_WLERR     (1<<21)
#define PUB_PGSR0_QSGERR    (1<<22)
#define PUB_PGSR0_WLAERR    (1<<23)
#define PUB_PGSR0_RDERR     (1<<24)
#define PUB_PGSR0_WDERR     (1<<25)
#define PUB_PGSR0_REERR     (1<<26)
#define PUB_PGSR0_WEERR     (1<<27)
#define PUB_PGSR0_DTERR     (PUB_PGSR0_RDERR|PUB_PGSR0_WDERR|PUB_PGSR0_REERR|PUB_PGSR0_WEERR)


	//asm ("wfi");

	//===============================================
	//PLL,DCAL,PHY RST,ZCAL
	if(g_ddr_settings[_PUB_ZQ0CR0])
	    nTempVal =	PUB_PIR_INIT | PUB_PIR_PLLINIT | PUB_PIR_DCAL;
	else
        nTempVal =	PUB_PIR_INIT | PUB_PIR_ZCAL | PUB_PIR_PLLINIT | PUB_PIR_DCAL | \
					PUB_PIR_PHYRST;
	writel(nTempVal, P_PUB_PIR_ADDR);
	//while( !(readl(P_PUB_PGSR0_ADDR) & 1 ) ) {}
	while(readl(P_PUB_PGSR0_ADDR) != 0x8000000f)
	{
		__udelayx(10);
		
		if(readl(P_PUB_PGSR0_ADDR) & PUB_PGSR0_ZCERR)
			goto pub_retry;
	}

	//===============================================
	//DRAM INIT
	nTempVal =	PUB_PIR_INIT | PUB_PIR_DRAMRST | PUB_PIR_DRAMINIT ;
	writel(nTempVal, P_PUB_PIR_ADDR);
	//while( !(readl(P_PUB_PGSR0_ADDR) & 1 ) ) {}
	while(readl(P_PUB_PGSR0_ADDR) != 0x8000001f)
	{
		__udelayx(10);
		
		//if(readl(P_PUB_PGSR0_ADDR) & (1<<20))
		//	goto pub_retry;
		if(readl(P_PUB_PGSR0_ADDR) & 1)
			break;
		
	}

	//===============================================	
	//WL init
	nTempVal =	PUB_PIR_INIT | PUB_PIR_WL ;
	writel(nTempVal, P_PUB_PIR_ADDR);
	//while( !(readl(P_PUB_PGSR0_ADDR) & 1 ) ) {}
	while(readl(P_PUB_PGSR0_ADDR) != 0x8000003f)
	{
		__udelayx(10);
		
		if(readl(P_PUB_PGSR0_ADDR) & PUB_PGSR0_WLERR)
			goto pub_retry;
	}

	//===============================================	
	//DQS Gate training
	nTempVal =	PUB_PIR_INIT | PUB_PIR_QSGATE ;
	writel(nTempVal, P_PUB_PIR_ADDR);
	//while( !(readl(P_PUB_PGSR0_ADDR) & 1 ) ) {}
	while(readl(P_PUB_PGSR0_ADDR) != 0x8000007f)
	{
		__udelayx(10);
		
		if(readl(P_PUB_PGSR0_ADDR) & PUB_PGSR0_QSGERR)
			goto pub_retry;
	}
	
	//===============================================	
	//Write leveling ADJ
	nTempVal =	PUB_PIR_INIT | PUB_PIR_WLADJ ;
	writel(nTempVal, P_PUB_PIR_ADDR);
	//while( !(readl(P_PUB_PGSR0_ADDR) & 1 ) ) {}
	while(readl(P_PUB_PGSR0_ADDR) != 0x800000ff)
	{
		__udelayx(10);
		
		if(readl(P_PUB_PGSR0_ADDR) & PUB_PGSR0_WLAERR)
			goto pub_retry;
	}

	//===============================================	
	//Data bit deskew & data eye training
	nTempVal =	PUB_PIR_INIT | PUB_PIR_RDDSKW | PUB_PIR_WRDSKW|
	PUB_PIR_RDEYE | PUB_PIR_WREYE;
	writel(nTempVal, P_PUB_PIR_ADDR);
	//while( !(readl(P_PUB_PGSR0_ADDR) & 1 ) ) {}
	while(readl(P_PUB_PGSR0_ADDR) != 0x80000fff)
	{
		__udelayx(10);
		
		if(readl(P_PUB_PGSR0_ADDR) & PUB_PGSR0_DTERR)
			goto pub_retry;
	}
	//===============================================

	f_serial_puts("\nPGSR0: 0x");
	serial_put_hex(readl(P_PUB_PGSR0_ADDR),32);

	f_serial_puts("\nPGSR1: 0x");
	serial_put_hex(readl(P_PUB_PGSR1_ADDR),32);
	f_serial_puts("\n");

	nTempVal = readl(P_PUB_PGSR0_ADDR);
	
	writel(UPCTL_CMD_GO, P_UPCTL_SCTL_ADDR);

	while ((readl(P_UPCTL_STAT_ADDR) & UPCTL_STAT_MASK ) != UPCTL_STAT_ACCESS ) {}

    if (( (nTempVal >> 20) & 0xfff ) != 0x800 )
    {
	    return nTempVal;
    }
    else
    {
        return 0;
    }
}

static void hx_init_dmc()
{	

#ifndef DDR_MMC_LOAD
	#define DDR_MMC_LOAD(a)  {writel(g_ddr_settings[a],P##a);}
#endif
	
	DDR_MMC_LOAD(_MMC_DDR_CTRL);
/*
	writel(0xffff, P_DMC_SEC_PORT0_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT1_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT2_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT3_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT4_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT5_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT6_RANGE0);
    writel(0xffff, P_DMC_SEC_PORT7_RANGE0);
    writel(0x80000000, P_DMC_SEC_CTRL);
	while( readl(P_DMC_SEC_CTRL) & 0x800000000 ) {}

	hx_enable_mmc_req();
*/
	//hx_dump_pub_pctl_all();
	//hx_dump_mmc_dmc_all();
	

	
	//re read write DDR SDRAM several times to 
	//make sure the AXI2DDR bugs dispear.
	//refer to arch\arm\cpu\aml_meson\m6\firmware\kreboot.s	
//	int nCnt,nMax,nVal;
//	for(nCnt=0,nMax= 9;nCnt<nMax;++nCnt)
//		writel(0x55555555, 0x9fffff00);
	
	//asm volatile ("wfi");	
//	for(nCnt=0,nMax= 12;nCnt<nMax;++nCnt)
//		nVal = readl(0x9fffff00);
		


/*
	if(g_ddr_settings[_MMC_LP_CTRL1] & MMC_LOWPWR_ENABLE)
	{
		DDR_MMC_LOAD(_MMC_LP_CTRL4);
		DDR_MMC_LOAD(_MMC_LP_CTRL3);
		DDR_MMC_LOAD(_MMC_LP_CTRL1);
	}	
		*/
#undef DDR_MMC_LOAD
	
	
}

static void hx_init_pctl_resume(void)
{
	//hx_reset_mmc();
	hx_init_pctl_ddr3();
	
	hx_init_dmc();
	
	//hx_dump_pub_pctl_all();	
	//hx_dump_mmc_dmc_all();
}

static void hx_init_ddr_pll()
{
	//asm volatile ("wfi");
	//band gap reset???
	//....
	writel(readl(P_HHI_VID_PLL_CNTL) |(1<<30), P_HHI_VID_PLL_CNTL);
#ifndef DDR_PLL_LOAD
	#define DDR_PLL_LOAD(a)  {writel(g_ddr_settings[a],P##a);}
#endif
	
	M6TV_PLL_RESET(HHI_DDR_PLL_CNTL);
	DDR_PLL_LOAD(_HHI_DDR_PLL_CNTL2);
	DDR_PLL_LOAD(_HHI_DDR_PLL_CNTL3);
	DDR_PLL_LOAD(_HHI_DDR_PLL_CNTL4);
	DDR_PLL_LOAD(_HHI_DDR_PLL_CNTL);	
	M6TV_PLL_WAIT_FOR_LOCK(HHI_DDR_PLL_CNTL);

#undef DDR_PLL_LOAD

    writel(0x00000080, P_MMC_CLK_CNTL);
    writel(0x000000c0, P_MMC_CLK_CNTL);

    //enable the clock.
    writel(0x000001c0, P_MMC_CLK_CNTL);

	__udelayx(100);
}

extern void wait_uart_empty();
void hx_leave_power_down()
{	
	f_serial_puts("step 8\n");	
	//wait_uart_empty();	
	
	hx_init_ddr_pll();

   	//Next, we reset all channels 
	hx_reset_mmc();
	f_serial_puts("step 9\n");
	//wait_uart_empty();

	//from M6
	//disable retention
	//disable retention before init_pctl
	//because init_pctl you need to data training stuff.
	hx_disable_retention();

	// initialize mmc and put it to sleep
	hx_init_pctl_resume();
	f_serial_puts("step 10\n");
	wait_uart_empty();
	
	f_serial_puts("leave_power_down\n");
	__udelayx(100000);	
	  
}

int ddr_suspend_test()
{
	int abort = 0;

	//DDR save setting
	hx_save_ddr_settings();	
	//DDR suspend
	hx_enter_power_down();	
#if 0
	int bootdelay = 1;
	int nLen = 0;
	f_serial_puts("Hit any key to stop power down: (second counting) ");
	nLen = hx_serial_put_dec(bootdelay);
	
	//asm volatile ("wfi");	
		
	//while ((bootdelay > 0) && (!abort)) {
	while ((!abort)) {
		int i;

		++bootdelay;
		/* delay 100 * 10ms */
		for (i=0; !abort && i<100; ++i) {
			if (serial_tstc()) {	/* we got a key press	*/
				abort  = 1;	    /* don't auto boot	*/
				bootdelay = 0;	/* no more delay	*/
                serial_getc();
				break;
			}
			__udelay(10000);
		}

		if(abort)
			break;
		
		while(nLen--)
		{
			f_serial_puts("\b\b  \b");
		}
		nLen = hx_serial_put_dec(bootdelay);
	}


	f_serial_puts("\n");
	__udelay(100000);
#endif
	//DDR resume
	hx_leave_power_down();
	
	return abort;
}

