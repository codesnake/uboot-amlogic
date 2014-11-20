
static inline int start_ddr_config(void)
{
    unsigned timeout = -1;
    APB_Wr(PCTL_SCTL_ADDR, 0x1);
    while((APB_Rd(PCTL_STAT_ADDR) != 0x1) && timeout)
        --timeout;

    return timeout;
}

static inline int end_ddr_config(void)
{
    unsigned timeout = 10000;
    APB_Wr(PCTL_SCTL_ADDR, 0x2);
    while((APB_Rd(PCTL_STAT_ADDR) != 0x3) && timeout)
        --timeout;

    return timeout;
}

static inline int dtu_enable(void)
{
    unsigned timeout = 10000;
    APB_Wr(PCTL_DTUECTL_ADDR, 0x1);  // start wr/rd
    while((APB_Rd(PCTL_DTUECTL_ADDR)&1) && timeout)
        --timeout;
    return timeout;
}

static unsigned char get_best_dtu(unsigned char* p, unsigned char* best)
{
    unsigned char i;

    for(i=0;i<=DDR_MAX_WIN_LEN -DDR_RDGR_LEN +1;i++)
    {
        if(*(p+i) + *(p+i+1) + *(p+i+2) == 48)
            goto next;
    }
    return 1;

next:

    for(i=0;i<=DDR_MAX_WIN_LEN -DDR_RDGR_LEN;i++)
    {
        if(*(p+i) + *(p+i+1) + *(p+i+2) + *(p+i+3) == 64)
        {
            if(!i)
                *best = 2;
            else
            {
                if(*(p+i-1)>*(p+i+4))
                    *best = i + 1;
                else
                    *best = i + 2;
            }

            return 0;
        }
    }

    for(i=0;i<=DDR_MAX_WIN_LEN -DDR_RDGR_LEN +1;i++)
    {
        if(*(p+i) + *(p+i+1) + *(p+i+2) == 48)
        {
            *best = i + 1;
            return 0;
        }
    }

    return 2;
}

static void set_result(unsigned char* res)
{
    unsigned rslr0=((res[0]>>2)&7) | (((res[1]>>2)&7)<<3) | (((res[2]>>2)&7)<<6) | (((res[3]>>2)&7)<<9);
    unsigned rdgr0=(res[0]&3)      | ((res[1]&3)     <<2) | ((res[2]&3)     <<4) | ((res[3]&3)<<6);
    APB_Wr(PCTL_RSLR0_ADDR,rslr0);
    APB_Wr(PCTL_RDGR0_ADDR,rdgr0);
}

void init_dmc(struct ddr_set * ddr_setting)
{
	APB_Wr(MMC_DDR_CTRL, ddr_setting->ddr_ctrl);
	APB_Wr(MMC_REQ_CTRL,0xff);
}

static unsigned char dllcr[7]={3,2,1,0,0xe,0xd,0xc};
//static unsigned char max_lane = 2;
#define TEST_COUNT 16
#define MAX_PATERN 18
static unsigned phd[MAX_PATERN][4]={
                            {0xdd22ee11, 0x7788bb44, 0x337755aa, 0xff00aa55},
                            {0xff000000, 0x000000ff, 0x00ffffff, 0xffffff00},

                            {0x01fe00ff, 0x01fe00ff, 0x01fe00ff, 0x01fe00ff},
                            {0x02fc00ff, 0x02fc00ff, 0x02fc00ff, 0x02fc00ff},
                            {0x04fb00ff, 0x04fb00ff, 0x04fb00ff, 0x04fb00ff},
                            {0x08f700ff, 0x08f700ff, 0x08f700ff, 0x08f700ff},
                            {0x10ef00ff, 0x10ef00ff, 0x10ef00ff, 0x10ef00ff},
                            {0x20df00ff, 0x20df00ff, 0x20df00ff, 0x20df00ff},
                            {0x40bf00ff, 0x40bf00ff, 0x40bf00ff, 0x40bf00ff},
                            {0x807f00ff, 0x807f00ff, 0x807f00ff, 0x807f00ff},

                            {0xfe01ff00, 0xfe01ff00, 0xfe01ff00, 0xfe01ff00},
                            {0xfd02ff00, 0xfd02ff00, 0xfd02ff00, 0xfd02ff00},
                            {0xfb04ff00, 0xfb04ff00, 0xfb04ff00, 0xfb04ff00},
                            {0xf708ff00, 0xf708ff00, 0xf708ff00, 0xf708ff00},
                            {0xef10ff00, 0xef10ff00, 0xef10ff00, 0xef10ff00},
                            {0xdf20ff00, 0xdf20ff00, 0xdf20ff00, 0xdf20ff00},
                            {0xbf40ff00, 0xbf40ff00, 0xbf40ff00, 0xbf40ff00},
                            {0x7f80ff00, 0x7f80ff00, 0x7f80ff00, 0x7f80ff00},
                          };

static int dtu_test(struct ddr_set *ddr_setting, unsigned lane, unsigned char *tra, unsigned char phase, unsigned dq, unsigned char *dqs, unsigned char *dqsn, unsigned pattern_index, unsigned repeat_count)
{
    int i=0;
    int result = -1;
    //int dtu_result;
#ifdef RESET_MMC_FOR_DTU_TEST
    ddr_setting->init_pctl(ddr_setting);
#endif
    for (i=0;i<repeat_count;i++){
        APB_Wr(PCTL_PHYCR_ADDR, 0x60);
        APB_Wr(PCTL_PHYCR_ADDR, 0x70);
        if (!start_ddr_config())
            return 0xff;
            set_result(tra);
        APB_Wr(PCTL_DQSTR_ADDR, (APB_Rd(PCTL_DQSTR_ADDR)&~(0x7<<(lane*3)))|(dqs[lane]<<(lane*3)));
        APB_Wr(PCTL_DQSNTR_ADDR, (APB_Rd(PCTL_DQSNTR_ADDR)&~(0x7<<(lane*3)))|(dqsn[lane]<<(lane*3)));
        APB_Wr(PCTL_DTUCFG_ADDR, (lane << 10) | 0x01);
        APB_Wr(PCTL_DLLCR0_ADDR+(lane<<2), (APB_Rd(PCTL_DLLCR0_ADDR+(lane<<2)) & ~(0xf<<14)) | (phase<<14));
        APB_Wr(PCTL_DQTR0_ADDR+(lane<<2), dq);
        APB_Wr(PCTL_DTUWD0_ADDR, phd[pattern_index][0]);
        APB_Wr(PCTL_DTUWD1_ADDR, phd[pattern_index][1]);
        APB_Wr(PCTL_DTUWD2_ADDR, phd[pattern_index][2]);
        APB_Wr(PCTL_DTUWD3_ADDR, phd[pattern_index][3]);
        APB_Wr(PCTL_DTUWACTL_ADDR, repeat_count*0x20);
        APB_Wr(PCTL_DTURACTL_ADDR, repeat_count*0x20);
        if (!end_ddr_config())
            return 0xff;
        if (dtu_enable()){
        if (APB_Rd(PCTL_DTUPDES_ADDR)>>8){
            //printf("\n%d ****** dtu test error: %x ********\n", i, APB_Rd(PCTL_DTUPDES_ADDR));
                return 0xff;
        }
        else{
            if (result==-1)
              result = 0;
                result |= APB_Rd(PCTL_DTUPDES_ADDR)&0xff;
        }    
        }
        else{
            //printf("\n%d ****** dtu enable error ********\n", i);
            return 0xff;
        }
        APB_Wr(PCTL_DTUECTL_ADDR, 0x0);
    }
    return result&0xff;
}

static unsigned char ddr_window(struct ddr_set *ddr_setting, int lane, unsigned char *chk, unsigned char *tra, unsigned char *dll_phase, unsigned *dq_dly, unsigned char *dqs, unsigned char *dqsn, int pattern_mask, int test_count, int tra_mask)
{
    int i,j,l;
  unsigned char Tra_tmp[4];

  unsigned result;
#ifndef RESET_MMC_FOR_DTU_TEST
    ddr_setting->init_pctl(ddr_setting);
#endif
    serial_puts("\nwindow select:\n");
    serial_puts("lane ");
    serial_put_hex(lane,8);
    serial_puts(", phase ");
    serial_put_hex(dll_phase[lane],8);
    serial_puts(":");
	
	for (i=0;i<4;i++)
    	Tra_tmp[i] = tra[i];

		for (i = 0; i < DDR_RSLR_LEN; i++) {
			for (j = 0; j < DDR_RDGR_LEN; j++) {
        if (chk[lane*DDR_RSLR_LEN*DDR_RDGR_LEN + i * DDR_RDGR_LEN + j]==16){
          if (tra_mask==0)
              Tra_tmp[3] = Tra_tmp[2] = Tra_tmp[1] = Tra_tmp[0] = i*DDR_RDGR_LEN+j;
          else
              Tra_tmp[lane] = i*DDR_RDGR_LEN+j;
        result = 0;
        if (i==DDR_RSLR_LEN-1)
          i = DDR_RSLR_LEN-1;
        for (l=0;l<32;l++){
          if ((1<<l)&pattern_mask){
            //printf("%d %d %d: %d %d %x %x %d\n", k, i, j, Tra_tmp[0], Tra_tmp[1], dllcr[dll_phase[k]], dq_dly[k], l);
              result |= dtu_test(ddr_setting, lane, Tra_tmp, dllcr[dll_phase[lane]], dq_dly[lane], dqs, dqsn, l, test_count);
          }
        }
          if (!result){
            chk[lane*DDR_RSLR_LEN*DDR_RDGR_LEN + i * DDR_RDGR_LEN + j] = 16;
          }
          else{
            chk[lane*DDR_RSLR_LEN*DDR_RDGR_LEN + i * DDR_RDGR_LEN + j] = 0;
          }
        }
			}
		}
    
    for (i = 0; i < DDR_RSLR_LEN; i++) {
			for (j = 0; j < DDR_RDGR_LEN; j++) {
            if (chk[lane*DDR_RSLR_LEN*DDR_RDGR_LEN + i*DDR_RDGR_LEN + j]==16)
                serial_puts("o");
            else
                serial_puts("x");
      }
    }
    if (get_best_dtu(&chk[lane*DDR_RSLR_LEN*DDR_RDGR_LEN], &Tra_tmp[lane])) {
        serial_puts(" -- ");
        serial_puts("not found\n");
        Tra_tmp[lane] = 0xff;
		}else{
		    serial_puts(" -- ");
        serial_put_hex(Tra_tmp[lane],8);
        serial_puts("\n");
	}
    return Tra_tmp[lane];
}

static int dwc_dq_alignment(struct ddr_set *ddr_setting, unsigned lane, unsigned char *tra, unsigned char *dqs, unsigned char *dqsn)
{
    int i,j,k,dqcr,result;
    int first_good = -1; 
    int last_good = -1;
    unsigned dq_result; 
    
#ifndef RESET_MMC_FOR_DTU_TEST
    ddr_setting->init_pctl(ddr_setting);
#endif
    serial_puts("\nLane ");
    serial_put_hex(lane,8);
    serial_puts(" go phase from 0 to 6: ");
    dqcr = 0xffffffff;
    for(i=0;i<7;i++)
    {
        result = 0;
        for (k=0;k<MAX_PATERN;k++)
            result |= dtu_test(ddr_setting, lane, tra, dllcr[i], dqcr, dqs, dqsn, k, TEST_COUNT);
        if (result){
                serial_puts("x");
        }
        else{
            if (first_good<0){
                first_good = i;
                last_good = i;
            }
            else{
                last_good = i;            
            }
            serial_puts("o");
        }
    }
    if (last_good<0){
      serial_puts(" could not find phase.\n");
      return -1;
    }
    serial_puts("\n");
    if (dqcr==0xffffffff){
        dq_result = 0;
        for (j=3;j>=0;j--){
            for (i=0;i<8;i++){
                if (!(dq_result & (1<<i))){
                    dqcr = (dqcr&~(0xf<<(i*4))) | (((j<<2)|j)<<(i*4));
                }       
            }
            result = 0;
            for (k=0;k<MAX_PATERN;k++)
              result |= dtu_test(ddr_setting, lane, tra, dllcr[last_good], dqcr, dqs, dqsn, k, TEST_COUNT);
            serial_put_hex(j,8);
            serial_puts(": ");
            serial_put_hex(dllcr[last_good],8);
            serial_puts(" ");
            serial_put_hex(dqcr,32);
            serial_puts(" ");
            serial_put_hex(result&0xff,8);
            serial_puts(" ");
            for (i=0;i<8;i++){
                if ((!(dq_result & (1<<i)))&&(result & (1<<i))){
                    if (j<3)
                        dqcr = (dqcr&~(0xf<<(i*4))) | ((((j+1)<<2)|(j+1))<<(i*4));
                    //else
                    //    printf("err -==================- \n");
                }       
            }
            dq_result |= result;
            serial_put_hex(dq_result&0xff,8);
            serial_puts(" ");
            serial_put_hex(dqcr,32);
            serial_puts("\n");
        }
    }
    return dqcr;
}

static int ddr_phase_sel(struct ddr_set *ddr_setting, int lane, unsigned char *tra, unsigned *tr, unsigned char *dqs, unsigned char *dqsn)
{
    int i, k;
    int first_good;
    int last_good;
    int result;

#ifndef RESET_MMC_FOR_DTU_TEST
    ddr_setting->init_pctl(ddr_setting);
#endif
    serial_puts("\nphase select:\n");
    {
        serial_puts("lane ");
        serial_put_hex(lane,8);
        serial_puts(": ");
        first_good = last_good = -1;
        for (i = 0; i < 7; i++)
        {
            result = 0;
            for(k = 0; k < MAX_PATERN; k++)
                result |= dtu_test(ddr_setting, lane, tra, dllcr[i], tr[lane], dqs, dqsn, k, TEST_COUNT);
            if (!result){
                if (first_good == -1)
                    first_good = i;
                last_good = i;
                serial_puts("o");
                    }
            else{
              serial_puts("x");
            }
        }

        if (first_good == -1){
		    serial_puts(" could not find phase\n");
            return 3;
        }
        else{
            serial_puts(" -- ");
			serial_put_hex((first_good + last_good) >> 1, 8);
            serial_puts("\n");
            return (first_good + last_good) >> 1;
        }
    }
}

static int dqs_dly_select(struct ddr_set *ddr_setting, unsigned char *phase, unsigned char *tra, unsigned *tr, unsigned char *dqs, unsigned char *dqsn)
{
  int i,j,k;
  unsigned char result;
  int first_good, last_good;
  unsigned char dqs_temp[4];
  unsigned char dqsn_temp[4];

#ifndef RESET_MMC_FOR_DTU_TEST
  ddr_setting->init_pctl(ddr_setting);
#endif
  serial_puts("\ndqs_dly:\n");
  for (i=0;i<4;i++)
  	dqsn_temp[i] = 3;
  for (i=0;i<4;i++){
    serial_puts("lane ");
    serial_put_hex(i,8);
    serial_puts(": ");
    dqs_temp[0] = dqs_temp[1] = 3;
    first_good = last_good = -1;
    for (j=0;j<8;j++){
      dqs_temp[i] = j; 
      result = 0;
      for (k=0;k<MAX_PATERN;k++)
        result |= dtu_test(ddr_setting, i, tra, dllcr[phase[i]], tr[i], dqs_temp, dqsn_temp, k, TEST_COUNT);
      if (!result){
        if (first_good<0)
          first_good = last_good = j;
        else
          last_good = j;
        serial_puts("o");
      }
      else{
        serial_puts("x");
      }
    }
    if (first_good>=0){
      dqs[i] = (first_good+last_good)>>1;
	  serial_puts(" -- ");
      serial_put_hex(dqs[i],8);
      serial_puts("\n");
    }
    else{
      serial_puts(" failed\n"); 
      dqs[i] = 3;
    }
  }

  for (i=0;i<4;i++)
  	dqs_temp[i] = dqs[i];
  serial_puts("\ndqsn_dly:\n");
  for (i=0;i<4;i++){
    serial_puts("lane ");
    serial_put_hex(i,8);
    serial_puts(": ");
    dqsn_temp[0] = dqsn_temp[1] = 3;
    first_good = last_good = -1;
    for (j=0;j<8;j++){
      dqsn_temp[i] = j; 
      result = 0;
      for (k=0;k<MAX_PATERN;k++)
        result |= dtu_test(ddr_setting, i, tra, dllcr[phase[i]], tr[i], dqs_temp, dqsn_temp, k, TEST_COUNT);
      if (!result){
        if (first_good<0)
          first_good = last_good = j;
        else
          last_good = j;
        serial_puts("o");
      }
      else{
        serial_puts("x");
      }
    }
    if (first_good>=0){
      dqsn[i] = (first_good+last_good)>>1;
	  serial_puts(" -- ");
      serial_put_hex(dqsn[i],8);
      serial_puts("\n");
    }
    else{
      serial_puts(" failed\n");
      dqsn[i] = 3; 
    }
  }
  return 0;
}

static unsigned ddr_init_sw(struct ddr_set * ddr_setting)
{
    int i,j;
    unsigned char phase_retry[5]={3,4,2,5,1};
    unsigned char Tra[4]={5,5,5,5};
    unsigned char Phase[4]={3,3,3,3};
	unsigned int Tr[4]={0xffffffff,0xffffffff};
  unsigned char chk[4*DDR_RSLR_LEN*DDR_RDGR_LEN];
	unsigned char Dqs[4]={3,3,3,3};
	unsigned char Dqsn[4]={3,3,3,3};
	
    writel(RESET_DDR,P_RESET1_REGISTER);
	serial_puts("\nTraining start:\n");
    for (i=0;i<4*DDR_RSLR_LEN*DDR_RDGR_LEN;i++)
      chk[i] = 16;
    for (i=0;i<5;i++){
        Phase[0] = phase_retry[i];
        for (j=0;j<DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
          chk[j] = 16;
        Tra[0] = ddr_window(ddr_setting, 0, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 0);
        if (Tra[0]!=0xff)
            break;
    }
    if (Tra[0]==0xff){
        for (i=0;i<5;i++){
            Phase[0] = phase_retry[i];
            for (j=0;j<DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
              chk[j] = 16;
            Tra[0] = ddr_window(ddr_setting, 0, chk, Tra, Phase, Tr, Dqs, Dqsn, 1, TEST_COUNT, 0);
            if (Tra[0]!=0xff)
                break;
        }
        if (Tra[0]==0xff) Tra[0]=5;
    }
    for (i=0;i<5;i++){
        Phase[1] = phase_retry[i];
        for (j=DDR_RSLR_LEN*DDR_RDGR_LEN;j<2*DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
          chk[j] = 16;
        Tra[1] = ddr_window(ddr_setting, 1, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
        if (Tra[1]!=0xff)
            break;
    }
    if (Tra[1]==0xff){
        for (i=0;i<5;i++){
            Phase[1] = phase_retry[i];
            for (j=DDR_RSLR_LEN*DDR_RDGR_LEN;j<2*DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
              chk[j] = 16;
            Tra[1] = ddr_window(ddr_setting, 1, chk, Tra, Phase, Tr, Dqs, Dqsn, 1, TEST_COUNT, 1);
            if (Tra[1]!=0xff)
                break;
        }
        if (Tra[1]==0xff) Tra[1]=5;
    }
	for (i=0;i<5;i++){
        Phase[2] = phase_retry[i];
        for (j=DDR_RSLR_LEN*DDR_RDGR_LEN;j<2*DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
          chk[j] = 16;
        Tra[2] = ddr_window(ddr_setting, 2, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
        if (Tra[2]!=0xff)
            break;
    }
    if (Tra[2]==0xff){
        for (i=0;i<5;i++){
            Phase[2] = phase_retry[i];
            for (j=DDR_RSLR_LEN*DDR_RDGR_LEN;j<2*DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
              chk[j] = 16;
            Tra[2] = ddr_window(ddr_setting, 2, chk, Tra, Phase, Tr, Dqs, Dqsn, 1, TEST_COUNT, 1);
            if (Tra[2]!=0xff)
                break;
        }
        if (Tra[2]==0xff) Tra[2]=5;
    }
	for (i=0;i<5;i++){
        Phase[3] = phase_retry[i];
        for (j=DDR_RSLR_LEN*DDR_RDGR_LEN;j<2*DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
          chk[j] = 16;
        Tra[3] = ddr_window(ddr_setting, 3, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
        if (Tra[3]!=0xff)
            break;
    }
    if (Tra[3]==0xff){
        for (i=0;i<5;i++){
            Phase[3] = phase_retry[i];
            for (j=DDR_RSLR_LEN*DDR_RDGR_LEN;j<2*DDR_RSLR_LEN*DDR_RDGR_LEN;j++)
              chk[j] = 16;
            Tra[3] = ddr_window(ddr_setting, 3, chk, Tra, Phase, Tr, Dqs, Dqsn, 1, TEST_COUNT, 1);
            if (Tra[3]!=0xff)
                break;
        }
        if (Tra[3]==0xff) Tra[3]=5;
    }
    Tr[0] = dwc_dq_alignment(ddr_setting, 0, Tra, Dqs, Dqsn);
    Tr[1] = dwc_dq_alignment(ddr_setting, 1, Tra, Dqs, Dqsn);
    Tr[2] = dwc_dq_alignment(ddr_setting, 2, Tra, Dqs, Dqsn);
    Tr[3] = dwc_dq_alignment(ddr_setting, 3, Tra, Dqs, Dqsn);
    Phase[0] = ddr_phase_sel(ddr_setting, 0, Tra, Tr, Dqs, Dqsn);
    Phase[1] = ddr_phase_sel(ddr_setting, 1, Tra, Tr, Dqs, Dqsn);
    Phase[2] = ddr_phase_sel(ddr_setting, 2, Tra, Tr, Dqs, Dqsn);
    Phase[3] = ddr_phase_sel(ddr_setting, 3, Tra, Tr, Dqs, Dqsn);
    dqs_dly_select(ddr_setting, Phase, Tra, Tr, Dqs, Dqsn);
    Tra[0] = ddr_window(ddr_setting, 0, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
    Tra[1] = ddr_window(ddr_setting, 1, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
    Tra[2] = ddr_window(ddr_setting, 2, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
    Tra[3] = ddr_window(ddr_setting, 3, chk, Tra, Phase, Tr, Dqs, Dqsn, (1<<MAX_PATERN)-1, TEST_COUNT, 1);
  
    ddr_setting->init_pctl(ddr_setting);
    if (!start_ddr_config())
      return 1;
    set_result(Tra);
    APB_Wr(PCTL_DLLCR0_ADDR, (APB_Rd(PCTL_DLLCR0_ADDR) & 0xfffc3fff) | (dllcr[Phase[0]]<<14));
    APB_Wr(PCTL_DLLCR1_ADDR, (APB_Rd(PCTL_DLLCR1_ADDR) & 0xfffc3fff) | (dllcr[Phase[1]]<<14));
    APB_Wr(PCTL_DLLCR2_ADDR, (APB_Rd(PCTL_DLLCR2_ADDR) & 0xfffc3fff) | (dllcr[Phase[2]]<<14));
    APB_Wr(PCTL_DLLCR3_ADDR, (APB_Rd(PCTL_DLLCR3_ADDR) & 0xfffc3fff) | (dllcr[Phase[3]]<<14));
    APB_Wr(PCTL_DQSTR_ADDR, (APB_Rd(PCTL_DQSTR_ADDR)&~(0xfff))|(Dqs[3]<<9)|(Dqs[2]<<6)|(Dqs[1]<<3)|Dqs[0]);
    APB_Wr(PCTL_DQSNTR_ADDR, (APB_Rd(PCTL_DQSNTR_ADDR)&~(0xfff))|(Dqsn[3]<<9)|(Dqsn[2]<<6)|(Dqsn[1]<<3)|Dqsn[0]);
    APB_Wr(PCTL_DQTR0_ADDR, Tr[0]);
    APB_Wr(PCTL_DQTR1_ADDR, Tr[1]);
    APB_Wr(PCTL_DQTR2_ADDR, Tr[2]);
    APB_Wr(PCTL_DQTR3_ADDR, Tr[3]);
    APB_Wr(PCTL_PHYCR_ADDR, 0x78);
    if (!end_ddr_config())
      return 1;
    	
    init_dmc(ddr_setting);

    serial_puts("\nTraining result:\n");
    serial_puts("\tRSLR0=");
	serial_put_hex(APB_Rd(PCTL_RSLR0_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tRDGR0=");
	serial_put_hex(APB_Rd(PCTL_RDGR0_ADDR), 32);
	serial_puts("\n");

    serial_puts("\tDLLCR0="); 
	serial_put_hex(APB_Rd(PCTL_DLLCR0_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tDLLCR1="); 
	serial_put_hex(APB_Rd(PCTL_DLLCR1_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tDLLCR2="); 
	serial_put_hex(APB_Rd(PCTL_DLLCR2_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tDLLCR3="); 
	serial_put_hex(APB_Rd(PCTL_DLLCR3_ADDR), 32);    
	serial_puts("\n");

	serial_puts("\tDQTR0="); 
	serial_put_hex(APB_Rd(PCTL_DQTR0_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tDQTR1=");
    serial_put_hex(APB_Rd(PCTL_DQTR1_ADDR), 32);
	serial_puts("\n");
	serial_puts("\tDQTR2="); 
	serial_put_hex(APB_Rd(PCTL_DQTR2_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tDQTR3=");
    serial_put_hex(APB_Rd(PCTL_DQTR3_ADDR), 32);
	serial_puts("\n");

    serial_puts("\tDQSTR=");
    serial_put_hex(APB_Rd(PCTL_DQSTR_ADDR), 32);
	serial_puts("\n");
    serial_puts("\tDQSNTR=");
    serial_put_hex(APB_Rd(PCTL_DQSNTR_ADDR), 32);
	serial_puts("\n");

  return 0;
}