#ifdef DDR_DMC
#else

#define DDR_DMC
#define MMC_DDR_CTRL        0x6000
#define MMC_ARB_CTRL        0x6008 
#define MMC_ARB_CTRL1       0x600c 

#define MMC_QOS0_CTRL  0x6010
   //bit 31     qos enable.
   //bit 26     1 : danamic change the bandwidth percentage. 0 : fixed bandwidth.  all 64. 
   //bit 25       grant mode. 1 grant clock cycles. 0 grant data cycles.
   //bit 24       leakybucket counter goes to 0. When no req or no other request. 
   //bit 21:16    bankwidth requirement. unit 1/64. 
   //bit 15:0.    after stop the re_enable threadhold.

#define MMC_QOS0_MAX   0x6014
#define MMC_QOS0_MIN   0x6018
#define MMC_QOS0_LIMIT 0x601c
#define MMC_QOS0_STOP  0x6020
#define MMC_QOS1_CTRL  0x6024
#define MMC_QOS1_MAX   0x6028
#define MMC_QOS1_MIN   0x602c
#define MMC_QOS1_STOP  0x6030
#define MMC_QOS1_LIMIT 0x6034
#define MMC_QOS2_CTRL  0x6038
#define MMC_QOS2_MAX   0x603c
#define MMC_QOS2_MIN   0x6040
#define MMC_QOS2_STOP  0x6044
#define MMC_QOS2_LIMIT 0x6048
#define MMC_QOS3_CTRL  0x604c
#define MMC_QOS3_MAX   0x6050
#define MMC_QOS3_MIN   0x6054
#define MMC_QOS3_STOP  0x6058
#define MMC_QOS3_LIMIT 0x605c
#define MMC_QOS4_CTRL  0x6060
#define MMC_QOS4_MAX   0x6064
#define MMC_QOS4_MIN   0x6068
#define MMC_QOS4_STOP  0x606c
#define MMC_QOS4_LIMIT 0x6070
#define MMC_QOS5_CTRL  0x6074
#define MMC_QOS5_MAX   0x6078
#define MMC_QOS5_MIN   0x607c
#define MMC_QOS5_STOP  0x6080
#define MMC_QOS5_LIMIT 0x6084
#define MMC_QOS6_CTRL  0x6088
#define MMC_QOS6_MAX   0x608c
#define MMC_QOS6_MIN   0x6090
#define MMC_QOS6_STOP  0x6094
#define MMC_QOS6_LIMIT 0x6098
#define MMC_QOS7_CTRL  0x609c
#define MMC_QOS7_MAX   0x60a0
#define MMC_QOS7_MIN   0x60a4
#define MMC_QOS7_STOP  0x60a8
#define MMC_QOS7_LIMIT 0x60ac

#define MMC_QOSMON_CTRL     0x60b0
#define MMC_QOSMON_TIM      0x60b4
#define MMC_QOSMON_MST      0x60b8
#define MMC_MON_CLKCNT      0x60bc
#define MMC_ALL_REQCNT      0x60c0
#define MMC_ALL_GANTCNT     0x60c4
#define MMC_ONE_REQCNT      0x60c8
#define MMC_ONE_CYCLE_CNT   0x60cc
#define MMC_ONE_DATA_CNT    0x60d0



#define DC_CAV_CTRL               0x6300

#define DC_CAV_LVL3_GRANT         0x6304
#define DC_CAV_LVL3_GH            0x6308
  // this is a 32 bit grant regsiter.
  // each bit grant a thread ID for LVL3 use.

#define DC_CAV_LVL3_FLIP          0x630c
#define DC_CAV_LVL3_FH            0x6310
  // this is a 32 bit FLIP regsiter.
  // each bit to define  a thread ID for LVL3 use.

#define DC_CAV_LVL3_CTRL0         0x6314
#define DC_CAV_LVL3_CTRL1         0x6318
#define DC_CAV_LVL3_CTRL2         0x631c
#define DC_CAV_LVL3_CTRL3         0x6320
#define DC_CAV_LUT_DATAL          0x6324
#define DC_CAV_LUT_DATAH          0x6328
#define DC_CAV_LUT_ADDR           0x632c
#define DC_CAV_LVL3_MODE          0x6330
#define MMC_PROT_ADDR             0x6334 
#define MMC_PROT_SELH             0x6338 
#define MMC_PROT_SELL             0x633c 
#define MMC_PROT_CTL_STS          0x6340 
#define MMC_INT_STS               0x6344 

#define MMC_PHY_CTRL              0x6380

#define MMC_REQ0_CTRL             0x6388
   // bit 31,            request in enable.
   // 30:24:             cmd fifo counter when request generate to dmc arbitor if there's no lbrst.
   // 23:16:             waiting time when request generate to dmc arbitor if there's o lbrst.
   // 15:8:              how many write rsp can hold in the whole dmc pipe lines.
   // 7:0:               how many read data can hold in the whole dmc pipe lines.

#define MMC_REQ1_CTRL             0x638c
#define MMC_REQ2_CTRL             0x6390
#define MMC_REQ3_CTRL             0x6394
#define MMC_REQ4_CTRL             0x6398
#define MMC_REQ5_CTRL             0x639c
#define MMC_REQ6_CTRL             0x63a0
#define MMC_REQ7_CTRL             0x63a4
                                           

#define MMC_REQ_CTRL        0x6400 
#define MMC_SOFT_RST        0x6404
  // bit 23.    reset no hold for chan7.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 22.    reset no hold for chan6.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 21.    reset no hold for chan5.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 20.    reset no hold for chan4.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 19.    reset no hold for chan3.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 18.    reset no hold for chan2.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 17.    reset no hold for chan1.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 16.    reset no hold for chan0.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 12.    write 1 to reset the whole MMC module(not include the APB3 interface).  read =0 means the reset finished. 
  // bit 10.    write 1 to reset DDR PHY only.    read 0 means the reset finished.
  // bit 9.     write 1 to reset the whole PCTL module.   read 0 means the reset finished.
  // bit 8.     write 1 to reset the whole DMC module.    read 0 means the reset finished.
  // bit 7.     write 1 to reset dmc request channel 7.   read 0 means the reset finished.
  // bit 6.     write 1 to reset dmc request channel 6.   read 0 means the reset finished.
  // bit 5.     write 1 to reset dmc request channel 5.   read 0 means the reset finished.
  // bit 4.     write 1 to reset dmc request channel 4.   read 0 means the reset finished.
  // bit 3.     write 1 to reset dmc request channel 3.   read 0 means the reset finished.
  // bit 2.     write 1 to reset dmc request channel 2.   read 0 means the reset finished.
  // bit 1.     write 1 to reset dmc request channel 1.   read 0 means the reset finished.
  // bit 0.     write 1 to reset dmc request channel 0.   read 0 means the reset finished.

#define MMC_RST_STS        0x6408
  // bit 23.    reset no hold for chan7.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 22.    reset no hold for chan6.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 21.    reset no hold for chan5.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 20.    reset no hold for chan4.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 19.    reset no hold for chan3.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 18.    reset no hold for chan2.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 17.    reset no hold for chan1.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 16.    reset no hold for chan0.   when do mmc/dmc reset, don't wait chan7 reset finished. maybe there's no clock. active high. 
  // bit 12.    write 1 to reset the whole MMC module(not include the APB3 interface).  read =0 means the reset finished. 
  // bit 10.    write 1 to reset DDR PHY only.    read 0 means the reset finished.
  // bit 9.     write 1 to reset the whole PCTL module.   read 0 means the reset finished.
  // bit 8.     write 1 to reset the whole DMC module.    read 0 means the reset finished.
  // bit 7.     write 1 to reset dmc request channel 7.   read 0 means the reset finished.
  // bit 6.     write 1 to reset dmc request channel 6.   read 0 means the reset finished.
  // bit 5.     write 1 to reset dmc request channel 5.   read 0 means the reset finished.
  // bit 4.     write 1 to reset dmc request channel 4.   read 0 means the reset finished.
  // bit 3.     write 1 to reset dmc request channel 3.   read 0 means the reset finished.
  // bit 2.     write 1 to reset dmc request channel 2.   read 0 means the reset finished.
  // bit 1.     write 1 to reset dmc request channel 1.   read 0 means the reset finished.
  // bit 0.     write 1 to reset dmc request channel 0.   read 0 means the reset finished.

#define MMC_APB3_CTRL             0x640c

#define MMC_CHAN_STS        0x6410
  //bit15    chan7 request bit.    1 means there's request for chan7.
  //bit14    chan6 request bit.    1 means there's request for chan6.
  //bit13    chan5 request bit.    1 means there's request for chan5.
  //bit12    chan4 request bit.    1 means there's request for chan4.
  //bit11    chan3 request bit.    1 means there's request for chan3.
  //bit10    chan2 request bit.    1 means there's request for chan2.
  //bit9     chan1 request bit.    1 means there's request for chan1.
  //bit8     chan0 request bit.    1 means there's request for chan0.
  //bit 7    chan7 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 6    chan6 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 5    chan5 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 4    chan4 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 3    chan3 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 2    chan2 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 1    chan1 status. 1 : idle.  0 busy.     it show idle only after disable this channel.
  //bit 0    chan0 status. 1 : idle.  0 busy.     it show idle only after disable this channel.


#define MMC_CLKG_CNTL0      0x6414
  // bit 18. 1: enable dmc cbus auto clock gating 
  // bit 17. 1: enable rdfifo auto clock gating 
  // bit 16. 1: enable pending read auto clock gating 
  // bit 15. 1: enable wdfifo auto clock gating 
  // bit 14. 1: enable pending wrte auto clock gating 
  // bit 13. 1: enalbe mgr auto clock gating 
  // bit 12. 1: enalbe cmdenc auto clock gating 
  // bit 11. 1: enable canvas cbus auto clock gating 
  // bit 10. 1: enable canvas auto clock gating 
  // bit 9.  1: enable pipefifo auto clock gating 
  // bit 8.  1: enable qos auto clock gating   
  // bit 7.  1: enable chan7 auto clock gating 
  // bit 6.  1: enable chan6 auto clock gating 
  // bit 5.  1: enable chan5 auto clock gating 
  // bit 4.  1: enable chan4 auto clock gating 
  // bit 3.  1: enable chan3 auto clock gating 
  // bit 2.  1: enable chan2 auto clock gating 
  // bit 1.  1: enable chan1 auto clock gating 
  // bit 0.  1: enable chan0 auto clock gating 

#define MMC_CLKG_CNTL1      0x6418
  // bit 22. 1: disable ddr_phy reading clock.
  // bit 21. 1: disable the PCTL APB2 clock.
  // bit 20. 1: disable the PCTL clock.
  // bit 19. 1: disable the whole dmc clock
  // bit 18. 1: disable the dmc cbus clock
  // bit 17. 1: disable rdfifo  clock  
  // bit 16. 1: disable pending read  clock  
  // bit 15. 1: disable wdfifo  clock  
  // bit 14. 1: disable pending wrte  clock  
  // bit 13. 1: disable mgr  clock  
  // bit 12. 1: disable cmdenc  clock  
  // bit 11. 1: disable canvas cbus  clock  
  // bit 10. 1: disable canvas  clock  
  // bit 9.  1: disable pipefifo  clock  
  // bit 8.  1: disable qos  clock    
  // bit 7.  1: disable chan7  clock  
  // bit 6.  1: disable chan6  clock  
  // bit 5.  1: disable chan5  clock  
  // bit 4.  1: disable chan4  clock  
  // bit 3.  1: disable chan3  clock  
  // bit 2.  1: disable chan2  clock  
  // bit 1.  1: disable chan1  clock  
  // bit 0.  1: disable chan0  clock  

#define MMC_CLK_CNTL      0x641c
   //bit 31     1 disabel all clock.
   //bit 30.    1 enable  auto clock gating. 0 : enable all clock if bit 31 = 0;
   //bit 29.    DDR PLL lock signal.   DDR PLL locked = 1.  
   //bit  7.    dll_clk_sel. 1 : the DLL input is directly from DDR PLL.  0: the DLL input is from slow clock or from DDR PLL clock divider. 
   //bit  6.    pll_clk_en.  1 : enable the DDR PLL clock to DDR DLL path. 0 : disable the DDR PLL clock to DDR PLL path.
   //bit  5.    divider/slow clock selection.   1 = slow clock.  0 = DDR PLL clock divider.  
   //bit  4.    slow clock enable.     1 = slow clock en.  0 : disalbe slow clock.
   //bit  3.    PLL clk divider enable. 1 = enable. 0 disable.
   //bit  2.    divider clock output enalbe.
   //bit 1:0    divider:    00 : /2.   01: /4. 10 : /8. 11: /16. 
   
#define MMC_DDR_PHY_GPR0   0x6420
#define MMC_DDR_PHY_GPR1   0x6424
#define MMC_LP_CTRL1       0x6428
#define MMC_LP_CTRL2       0x642c
#define MMC_LP_CTRL3       0x6430
#define MMC_PCTL_STAT      0x6434 
#define MMC_CMDZQ_CTRL     0x6438 

#define P_MMC_DDR_CTRL        APB_REG_ADDR(MMC_DDR_CTRL)
#define P_MMC_REQ_CTRL        APB_REG_ADDR(MMC_REQ_CTRL)
#define P_MMC_ARB_CTRL        APB_REG_ADDR(MMC_ARB_CTRL)
#define P_MMC_ARB_CTRL1       APB_REG_ADDR(MMC_ARB_CTRL1)

#define P_MMC_SOFT_RST       APB_REG_ADDR(MMC_SOFT_RST)
#define P_MMC_CHAN_STS       APB_REG_ADDR(MMC_CHAN_STS)
#define P_MMC_INT_STS        APB_REG_ADDR(MMC_INT_STS)
#define P_MMC_PHY_CTRL       APB_REG_ADDR(MMC_PHY_CTRL)
#define P_MMC_RST_STS        APB_REG_ADDR(MMC_RST_STS)
#define P_MMC_CLK_CNTL       APB_REG_ADDR(MMC_CLK_CNTL)

#define DMC_SEC_RANGE0_ST   		0xda002000
#define DMC_SEC_RANGE0_END  		0xda002004
#define DMC_SEC_RANGE1_ST   		0xda002008
#define DMC_SEC_RANGE1_END  		0xda00200c
#define DMC_SEC_RANGE2_ST   		0xda002010
#define DMC_SEC_RANGE2_END  		0xda002014
#define DMC_SEC_RANGE3_ST   		0xda002018
#define DMC_SEC_RANGE3_END  		0xda00201c
#define DMC_SEC_PORT0_RANGE0		0xda002020
#define DMC_SEC_PORT1_RANGE0		0xda002024
#define DMC_SEC_PORT2_RANGE0		0xda002028
#define DMC_SEC_PORT3_RANGE0		0xda00202c
#define DMC_SEC_PORT4_RANGE0		0xda002030
#define DMC_SEC_PORT5_RANGE0		0xda002034
#define DMC_SEC_PORT6_RANGE0		0xda002038
#define DMC_SEC_PORT7_RANGE0		0xda00203c
#define DMC_SEC_PORT0_RANGE1		0xda002040
#define DMC_SEC_PORT1_RANGE1		0xda002044
#define DMC_SEC_PORT2_RANGE1		0xda002048
#define DMC_SEC_PORT3_RANGE1		0xda00204c
#define DMC_SEC_PORT4_RANGE1		0xda002050
#define DMC_SEC_PORT5_RANGE1		0xda002054
#define DMC_SEC_PORT6_RANGE1		0xda002058
#define DMC_SEC_PORT7_RANGE1		0xda00205c
#define DMC_SEC_PORT0_RANGE2		0xda002060
#define DMC_SEC_PORT1_RANGE2		0xda002064
#define DMC_SEC_PORT2_RANGE2		0xda002068
#define DMC_SEC_PORT3_RANGE2		0xda00206c
#define DMC_SEC_PORT4_RANGE2		0xda002070
#define DMC_SEC_PORT5_RANGE2		0xda002074
#define DMC_SEC_PORT6_RANGE2		0xda002078
#define DMC_SEC_PORT7_RANGE2		0xda00207c
#define DMC_SEC_PORT0_RANGE3		0xda002080
#define DMC_SEC_PORT1_RANGE3		0xda002084
#define DMC_SEC_PORT2_RANGE3		0xda002088
#define DMC_SEC_PORT3_RANGE3		0xda00208c
#define DMC_SEC_PORT4_RANGE3		0xda002090
#define DMC_SEC_PORT5_RANGE3		0xda002094
#define DMC_SEC_PORT6_RANGE3		0xda002098
#define DMC_SEC_PORT7_RANGE3		0xda00209c
#define DMC_SEC_BAD_ACCESS  		0xda0020a0
#define DMC_SEC_CTRL     		0xda0020a4

#endif
