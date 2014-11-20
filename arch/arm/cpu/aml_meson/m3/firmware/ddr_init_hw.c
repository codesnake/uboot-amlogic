static int ddr_phy_data_traning(void)
{
   // int  dqs0_delay;
   // int  dqs1_delay;
   // int  dqs2_delay;
    //int  dqs3_delay;
    //int  msdll0_phase;
    //int  msdll1_phase;
    //int  msdll2_phase;
    //int  msdll3_phase;
    //int  find_windows;
    int  data_temp;
    clrbits_le32(P_PCTL_DTUAWDT_ADDR,3<<9);
        APB_Wr(PCTL_DTUWD0_ADDR, 0xdd22ee11);
        APB_Wr(PCTL_DTUWD1_ADDR, 0x7788bb44);
        APB_Wr(PCTL_DTUWD2_ADDR, 0xdd22ee11);
        APB_Wr(PCTL_DTUWD3_ADDR, 0x7788bb44);
        APB_Wr(PCTL_DTUWACTL_ADDR, 0x300 |    // col addr
                                   (0x7<<10) |  //bank addr
                                   (0x1fff <<13) |  // row addr
                                   (0 <<30 ));    // rank addr
        APB_Wr(PCTL_DTURACTL_ADDR, 0x300 |    // col addr
                                   (0x7<<10) |  //bank addr
                                   (0x1fff <<13) |  // row addr
                                   (0 <<30 ));    // rank addr

     if (0) {

        APB_Wr(PCTL_RSLR0_ADDR, (0 << 0) |       // system additinal latency.
                                (0 << 3) |    
                                (0 << 6) |
                                (0 << 9) );

        APB_Wr(PCTL_RDGR0_ADDR, (0 << 0) |        //DQS GATing phase.
                                (0 << 2) |
                                (0 << 4) |
                                (0 << 6) );

       APB_Wr(PCTL_DLLCR0_ADDR, (APB_Rd(PCTL_DLLCR0_ADDR) & 0xfffc3fff) | 
                                (3 << 14 ));
    
       APB_Wr(PCTL_DLLCR1_ADDR, (APB_Rd(PCTL_DLLCR1_ADDR) & 0xfffc3fff) | 
                                (3 << 14 ));
    
       APB_Wr(PCTL_DLLCR2_ADDR, (APB_Rd(PCTL_DLLCR2_ADDR) & 0xfffc3fff) | 
                                (3 << 14 ));
            
       APB_Wr(PCTL_DLLCR3_ADDR, (APB_Rd(PCTL_DLLCR3_ADDR) & 0xfffc3fff) | 
                                (3 << 14 ));

       APB_Wr(PCTL_DQSTR_ADDR,  (0 << 0 ) |
                                (0 << 3 ) |
                                (0 << 6 ) |
                                (0 << 9 ));
     
       APB_Wr(PCTL_DQSNTR_ADDR, (0 << 0 ) |
                                (0 << 3 ) |
                                (0 << 6 ) |
                                (0 << 9 ));

       APB_Wr(PCTL_DQTR0_ADDR,  (0 << 0 ) |
                                (0 << 4 ) |
                                (0 << 8 ) |
                                (0 << 12 ) |
                                (0 << 16) |
                                (0 << 20) |
                                (0 << 24) |
                                (0 << 28) );

       APB_Wr(PCTL_DQTR1_ADDR,  (0 << 0 ) |
                                (0 << 4 ) |
                                (0 << 8 ) |
                                (0 << 12 ) |
                                (0 << 16) |
                                (0 << 20) |
                                (0 << 24) |
                                (0 << 28) );

       APB_Wr(PCTL_DQTR2_ADDR,  (0 << 0 ) |
                                (0 << 4 ) |
                                (0 << 8 ) |
                                (0 << 12 ) |
                                (0 << 16) |
                                (0 << 20) |
                                (0 << 24) |
                                (0 << 28) );

       APB_Wr(PCTL_DQTR3_ADDR,  (0 << 0 ) |
                                (0 << 4 ) |
                                (0 << 8 ) |
                                (0 << 12 ) |
                                (0 << 16) |
                                (0 << 20) |
                                (0 << 24) |
                                (0 << 28) );
       }
 
       // hardware build in  data training.
       APB_Wr(PCTL_PHYCR_ADDR, APB_Rd(PCTL_PHYCR_ADDR) | (1<<31));
       APB_Wr(PCTL_SCTL_ADDR, 1); // init: 0, cfg: 1, go: 2, sleep: 3, wakeup: 4         rddata = 1;        // init: 0, cfg: 1, cfg_req: 2, access: 3, access_req: 4, low_power: 5, low_power_enter_req: 6, low_power_exit_req: 7
       while ((APB_Rd(PCTL_STAT_ADDR) & 0x7 ) != 1 ) {}
       APB_Wr(PCTL_SCTL_ADDR, 2); // init: 0, cfg: 1, go: 2, sleep: 3, wakeup: 4
       data_temp = APB_Rd(PCTL_PHYCR_ADDR);
       while (data_temp & 0x80000000 ) {
          data_temp = APB_Rd(PCTL_PHYCR_ADDR);
       }  // waiting the data trainning finish.  
       data_temp = APB_Rd(PCTL_PHYSR_ADDR);
       //printf("PCTL_PHYSR_ADDR=%08x\n",data_temp& 0x00340000);
       serial_puts("PHY trainning Result=");
       serial_put_dword(data_temp);
       if ( data_temp & 0x00340000 ) {       // failed.
           return (1); 
       } else {
           return (0);                      //passed.
       }
}
int ddr_init_hw(struct ddr_set * timing_reg)
{
    if(timing_reg->init_pctl(timing_reg))
        return 1;
    if(ddr_phy_data_traning())
        return 1;
    init_dmc(timing_reg);
    return 0;
}