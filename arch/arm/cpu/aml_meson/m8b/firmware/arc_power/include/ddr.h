#include "pctl.h"
#include "dmc.h"
#include "register.h"

#define POWER_DOWN_DDRPHY
//#define DDR3_533Mhz
////#define DDR2_400Mhz
//#ifdef DDR2_400Mhz
//#define init_pctl init_pctl_ddr2
//#else
//#define init_pctl init_pctl_ddr3
//#endif
#define NOP_CMD  0
#define PREA_CMD  1
#define REF_CMD  2
#define MRS_CMD  3
#define ZQ_SHORT_CMD 4
#define ZQ_LONG_CMD  5
#define SFT_RESET_CMD 6


//ddr training result
#define 	rslr0           0     
#define 	rdgr0           1    
#define 	t_1us_pck       2     
#define 	t_100ns_pck     3 
#define 	t_init_us       4  
#define 	t_refi_100ns    5  
#define 	t_mrd           6  
#define 	t_rfc           7  
#define 	t_rp            8  
#define 	t_al            9  
#define 	t_cl            10  
#define 	t_ras           11  
#define 	t_rc            12  
#define 	t_rcd           13  
#define 	t_rrd           14 
#define 	t_rtp           15 
#define 	t_wr            16 
#define 	t_wtr           17  
#define 	t_exsr          18  
#define 	t_xp            19  
#define 	t_dqs           20 
#define 	t_trtw          21  
#define 	t_mod           22 
#define   t_cwl           23
#define   mmc_phy_ctrl    24
#define   mmc_ddr_ctrl    25
#define   ddr_pll_cntl    26
#define   ddr_pll_cntl2   27
#define   ddr_pll_cntl3   28
#define   ddr_pll_cntl4   29
#define   mmc_clk_cntl    30
#define   t_rsth_us       31
#define   t_rstl_us       32
#define   t_faw           33
#define   mcfg            34
#define   t_cke           35
#define   t_cksre         36
#define   t_cksrx         37
#define   t_zqcl          38
#define   pub_dtpr0       39
#define   pub_dtpr1       40
#define   pub_dtpr2       41
#define   pub_ptr0        42
#define   pub_ptr1        43
#define   pub_ptr2        44
#define   msr0            45
#define   msr1            46
#define   msr2            47
#define   msr3            48
#define   odtcfg          49
#define   zqcr            50
#define   dllcr9          51
#define   iocr            52
#define   dllcr0          53
#define   dllcr1          54
#define   dllcr2          55
#define   dllcr3          56
#define   dqscr           57
#define   dqsntr          58
#define   tr0             59
#define   tr1             60
#define   tr2             61
#define   tr3             62

#define  dx0gsr0  63 
#define  dx0gsr1  64 
#define  dx0dqstr 65 
	
#define  dx1gsr0  66 
#define  dx1gsr1  67 
#define  dx1dqstr 68 

#define  dx2gsr0  69 
#define  dx2gsr1  70 
#define  dx2dqstr 71 

#define  dx3gsr0  72 
#define  dx3gsr1  73 
#define  dx3dqstr 74 

#define  dx4gsr0  75 
#define  dx4gsr1  76 
#define  dx4dqstr 77 

#define  dx5gsr0  78 
#define  dx5gsr1  79 
#define  dx5dqstr 80 

#define  dx6gsr0  81 
#define  dx6gsr1  82 
#define  dx6dqstr 83 

#define  dx7gsr0  84 
#define  dx7gsr1  85 
#define  dx7dqstr 86 

#define  dx8gsr0  87  
#define  dx8gsr1  88 
#define  dx8dqstr 89 

#define dfiodrcfg_adr 90
#define zq0cr1 91
#define  dtar  92

#define zq0cr0 93
#define cmdzq  94


#define DDR_SETTING_COUNT 95

#define 	v_rslr0          ddr_settings[ rslr0 ]    
#define 	v_rdgr0          ddr_settings[ rdgr0 ]   
#define 	v_t_1us_pck      ddr_settings[ t_1us_pck ]    
#define 	v_t_100ns_pck    ddr_settings[ t_100ns_pck ]
#define 	v_t_init_us      ddr_settings[ t_init_us ] 
#define 	v_t_refi_100ns   ddr_settings[ t_refi_100ns ] 
#define 	v_t_mrd          ddr_settings[ t_mrd ] 
#define 	v_t_rfc          ddr_settings[ t_rfc ] 
#define 	v_t_rp           ddr_settings[ t_rp ] 
#define 	v_t_al           ddr_settings[ t_al ] 
#define 	v_t_cl           ddr_settings[ t_cl ] 
#define 	v_t_ras          ddr_settings[ t_ras ] 
#define 	v_t_rc           ddr_settings[ t_rc ] 
#define 	v_t_rcd          ddr_settings[ t_rcd ] 
#define 	v_t_rrd          ddr_settings[ t_rrd ]
#define 	v_t_rtp          ddr_settings[ t_rtp ]
#define 	v_t_wr           ddr_settings[ t_wr ]
#define 	v_t_wtr          ddr_settings[ t_wtr ] 
#define 	v_t_exsr         ddr_settings[ t_exsr ] 
#define 	v_t_xp           ddr_settings[ t_xp ] 
#define 	v_t_dqs          ddr_settings[ t_dqs ]
#define 	v_t_trtw         ddr_settings[ t_trtw ] 
#define 	v_t_mod          ddr_settings[ t_mod ]
#define 	v_t_cwl          ddr_settings[ t_cwl ]
#define   v_mmc_phy_ctrl   ddr_settings[ mmc_phy_ctrl ]
#define   v_mmc_ddr_ctrl   ddr_settings[ mmc_ddr_ctrl ]
#define   v_ddr_pll_cntl   ddr_settings[ ddr_pll_cntl ]
#define   v_ddr_pll_cntl2  ddr_settings[ ddr_pll_cntl2 ]
#define   v_ddr_pll_cntl3  ddr_settings[ ddr_pll_cntl3 ]
#define   v_ddr_pll_cntl4  ddr_settings[ ddr_pll_cntl4 ]
#define   v_mmc_clk_cntl   ddr_settings[ mmc_clk_cntl ]
#define   v_t_rsth_us   ddr_settings[ t_rsth_us ]
#define   v_t_rstl_us   ddr_settings[ t_rstl_us ]
#define   v_t_faw       ddr_settings[ t_faw ]
#define   v_mcfg        ddr_settings[ mcfg ]
#define   v_t_cke       ddr_settings[ t_cke ]
#define   v_t_cksre     ddr_settings[ t_cksre ]
#define   v_t_cksrx     ddr_settings[ t_cksrx ]
#define   v_t_zqcl      ddr_settings[ t_zqcl ]
#define   v_pub_dtpr0   ddr_settings[ pub_dtpr0 ]
#define   v_pub_dtpr1   ddr_settings[ pub_dtpr1 ]
#define   v_pub_dtpr2   ddr_settings[ pub_dtpr2 ]
#define   v_pub_ptr0    ddr_settings[ pub_ptr0 ]
#define   v_pub_ptr1    ddr_settings[ pub_ptr1 ]
#define   v_pub_ptr2    ddr_settings[ pub_ptr2 ]
#define   v_msr0        ddr_settings[ msr0 ]
#define   v_msr1        ddr_settings[ msr1 ]
#define   v_msr2        ddr_settings[ msr2 ]
#define   v_msr3        ddr_settings[ msr3 ]
#define   v_odtcfg      ddr_settings[ odtcfg ]
#define   v_zqcr        ddr_settings[ zqcr ]

#define   v_dllcr9      ddr_settings[ dllcr9 ]
#define   v_iocr        ddr_settings[ iocr ]
#define   v_dllcr0      ddr_settings[ dllcr0 ]
#define   v_dllcr1      ddr_settings[ dllcr1 ]
#define   v_dllcr2      ddr_settings[ dllcr2 ]
#define   v_dllcr3      ddr_settings[ dllcr3 ]
#define   v_dqscr       ddr_settings[ dqscr ]
#define   v_dqsntr      ddr_settings[ dqsntr ]
#define   v_tr0         ddr_settings[ tr0 ]
#define   v_tr1         ddr_settings[ tr1 ]
#define   v_tr2         ddr_settings[ tr2 ]
#define   v_tr3         ddr_settings[ tr3 ]

#define v_dx0gsr0  ddr_settings[ dx0gsr0  ] 
#define v_dx0gsr1  ddr_settings[ dx0gsr1  ] 
#define v_dx0dqstr ddr_settings[ dx0dqstr ] 
	                                       
#define v_dx1gsr0  ddr_settings[ dx1gsr0  ] 
#define v_dx1gsr1  ddr_settings[ dx1gsr1  ] 
#define v_dx1dqstr ddr_settings[ dx1dqstr ] 
                                         
#define v_dx2gsr0  ddr_settings[ dx2gsr0  ] 
#define v_dx2gsr1  ddr_settings[ dx2gsr1  ] 
#define v_dx2dqstr ddr_settings[ dx2dqstr ] 
                                         
#define v_dx3gsr0  ddr_settings[ dx3gsr0  ] 
#define v_dx3gsr1  ddr_settings[ dx3gsr1  ] 
#define v_dx3dqstr ddr_settings[ dx3dqstr ] 
                                         
#define v_dx4gsr0  ddr_settings[ dx4gsr0  ] 
#define v_dx4gsr1  ddr_settings[ dx4gsr1  ] 
#define v_dx4dqstr ddr_settings[ dx4dqstr ] 
                                         
#define v_dx5gsr0  ddr_settings[ dx5gsr0  ] 
#define v_dx5gsr1  ddr_settings[ dx5gsr1  ] 
#define v_dx5dqstr ddr_settings[ dx5dqstr ] 
                                         
#define v_dx6gsr0  ddr_settings[ dx6gsr0  ] 
#define v_dx6gsr1  ddr_settings[ dx6gsr1  ] 
#define v_dx6dqstr ddr_settings[ dx6dqstr ] 
                                         
#define v_dx7gsr0  ddr_settings[ dx7gsr0  ] 
#define v_dx7gsr1  ddr_settings[ dx7gsr1  ] 
#define v_dx7dqstr ddr_settings[ dx7dqstr ] 
                                         
#define v_dx8gsr0  ddr_settings[ dx8gsr0  ]  
#define v_dx8gsr1  ddr_settings[ dx8gsr1  ] 
#define v_dx8dqstr ddr_settings[ dx8dqstr ] 

#define v_dfiodrcfg_adr ddr_settings[ dfiodrcfg_adr ] 
#define v_zq0cr1   ddr_settings[zq0cr1]


#define v_zq0cr0   ddr_settings[zq0cr0]
#define v_cmdzq   ddr_settings[cmdzq]

#define v_dtar     ddr_settings[dtar]

extern unsigned ddr_settings[];
extern int init_pctl_ddr2(void);
extern int  init_pctl_ddr3(void);
extern int  ddr_phy_data_training(void);
extern void init_dmc(void);

extern void load_nop(void);
extern void load_prea(void);
extern void load_mrs(int mrs_num, int mrs_value);
extern void load_ref(void);
extern void load_zqcl(int zqcl_value);
extern void set_ddr_clock_333(void);
extern void set_ddr_clock_533(void);

