#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/timing.h>
#ifndef SPL_STATIC_FUNC
#define SPL_STATIC_FUNC
#endif

#define DDR_RSLR_LEN 6
#define DDR_RDGR_LEN 4
#define DDR_MAX_WIN_LEN (DDR_RSLR_LEN*DDR_RDGR_LEN)

static unsigned ddr_start_again=1;
#define APB_Wr WRITE_APB_REG
#define APB_Rd READ_APB_REG

#define NOP_CMD   0
#define PREA_CMD   1
#define REF_CMD   2
#define MRS_CMD   3
#define ZQ_SHORT_CMD  4
#define ZQ_LONG_CMD   5
#define SFT_RESET_CMD  6

static inline void load_mcmd(unsigned val)
{
	APB_Wr(PCTL_MCMD_ADDR, val);
	while ( APB_Rd(PCTL_MCMD_ADDR) & 0x80000000 );
}
#define load_nop()  load_mcmd((1<<31)|(1<<20)|NOP_CMD)
#define load_prea()  	load_mcmd((1<<31)|(1<<20)|PREA_CMD)
#define load_ref()  	load_mcmd((1<<31)|(1<<20)|REF_CMD)

#define load_mrs(a,b)	load_mcmd((1 << 31) | \
								  (8 << 24) | \
								  (1 << 20) | \
							(a << 17) | \
						   (b << 4) | \
								   MRS_CMD )


SPL_STATIC_FUNC
void __udelay(unsigned long usec);
static void init_pctl(struct ddr_set * ddr_setting)
{
    int mrs0_value;
    int mrs1_value;
    int mrs2_value;
    int mrs3_value;
    writel(RESET_DDR,P_RESET1_REGISTER);
    __udelay(1000);
    mrs0_value =  (0 << 12 ) |              //0 fast, 1 slow.
                  (ddr_setting->t_wr <<9 ) |  //wr cycles.
                  (0  << 8 )   |            //DLL reset.
                  (0  << 7)    |            //0= Normal 1=Test.
                  (ddr_setting->cl <<4) |     //cas latency.
                  (0 << 3) |                //burst type,  0:sequential 1 Interleave.
                  2 ;                       //burst length  : 010 : 4,  011: 8.

    mrs1_value =  (0 << 12 )  |             // qoff 0=op buf enabled : 1=op buf disabled  (A12)
                  (0 << 11 )  |             // rdqs enable 0=disabled : 1=enabled         (A11)
                  (0 << 10 )  |             // DQSN enable 1=disabled : 0=enabled         (A10)
                  (0 << 7  )  |             // ocd_program;      // 0, 1, 2, 3, or 7      (A9-A7)
                  (ddr_setting->t_al << 3) |  //additive_latency; // 0 - 4                  (A5-A3)
				  ( 0 << 6)   |
                  ( 1 << 2)   |             //rtt_nominal;      // 50, 75, or 150 ohms    (A6:A2)
                  ( 1 << 1 )  | //( 0 << 1 )  |             //ocd_impedence;    // 0=full : 1=reduced strength (A1)
                  ( 0 << 0 ) ;              // dll_enable;       // 0=enable : 1=disable       (A0)
    mrs2_value = 0;
    mrs3_value = 0;

    APB_Wr(PCTL_IOCR_ADDR, ddr_setting->iocr);
    //write memory timing registers
    APB_Wr(PCTL_TOGCNT1U_ADDR, 		ddr_setting->t_1us_pck);
    APB_Wr(PCTL_TINIT_ADDR, 		ddr_setting->t_init_us);
    APB_Wr(PCTL_TOGCNT100N_ADDR, 	ddr_setting->t_100ns_pck);
    APB_Wr(PCTL_TREFI_ADDR, 		ddr_setting->t_refi_100ns);

    APB_Wr(PCTL_TRSTH_ADDR, 0);       // 0 for ddr2
    while (!(APB_Rd(PCTL_POWSTAT_ADDR) & 2)) {} // wait for dll lock

    APB_Wr(PCTL_POWCTL_ADDR, 1);            // start memory power up sequence
    while (!(APB_Rd(PCTL_POWSTAT_ADDR) & 1)) {} // wait for memory power up

    APB_Wr(PCTL_ODTCFG_ADDR, 8);         //configure ODT

    APB_Wr(PCTL_TMRD_ADDR, 	ddr_setting->t_mrd);
    APB_Wr(PCTL_TRFC_ADDR, 	ddr_setting->t_rfc);
    APB_Wr(PCTL_TRP_ADDR, 	ddr_setting->t_rp);
    APB_Wr(PCTL_TAL_ADDR,  	ddr_setting->t_al);
    APB_Wr(PCTL_TCL_ADDR, 	ddr_setting->cl);
    APB_Wr(PCTL_TCWL_ADDR,  ddr_setting->cl-1 + ddr_setting->t_al);
    APB_Wr(PCTL_TRAS_ADDR, 	ddr_setting->t_ras);
    APB_Wr(PCTL_TRC_ADDR, 	ddr_setting->t_rc);
    APB_Wr(PCTL_TRCD_ADDR, 	ddr_setting->t_rcd);
    APB_Wr(PCTL_TRRD_ADDR, 	ddr_setting->t_rrd);
    APB_Wr(PCTL_TRTP_ADDR, 	ddr_setting->t_rtp);
    APB_Wr(PCTL_TWR_ADDR, 	ddr_setting->t_wr);
    APB_Wr(PCTL_TWTR_ADDR, 	ddr_setting->t_wtr);
    APB_Wr(PCTL_TEXSR_ADDR, ddr_setting->t_exsr);
    APB_Wr(PCTL_TXP_ADDR, 	ddr_setting->t_xp);
    APB_Wr(PCTL_TDQS_ADDR, 	ddr_setting->t_dqs);
    //configure the PCTL for DDR2 SDRAM burst length = 4
   APB_Wr(PCTL_MCFG_ADDR, ddr_setting->mcfg);

    APB_Wr(PCTL_PHYCR_ADDR, APB_Rd(PCTL_PHYCR_ADDR)&(~0x100));

   // Don't do it for gates simulation.
	APB_Wr(PCTL_ZQCR_ADDR,   (1 << 24) | ddr_setting->zqcr);

    // initialize SDRAM
    load_nop();
    load_prea();
    load_mrs(2, mrs2_value);
    load_mrs(3, mrs3_value);
    mrs1_value = mrs1_value & 0xfffffffe; //dll enable
    load_mrs(1, mrs1_value);
    mrs0_value = mrs0_value | (1 << 8);    // dll reset.
    load_mrs(0, mrs0_value);
    load_prea();
    load_ref();
    load_ref();
    mrs0_value = mrs0_value & (~(1 << 8));    // dll reset.
    load_mrs(0, mrs0_value);
    mrs1_value = (mrs1_value & (~(7 << 7))) | (7 << 7 );  //OCD default.
    load_mrs(1, mrs1_value);
    mrs1_value = (mrs1_value & (~(7 << 7))) | (0 << 7 );  // ocd_exit;
    load_mrs(1, mrs1_value);
}

static inline void init_dmc (struct ddr_set * ddr_setting)
{
	APB_Wr(MMC_DDR_CTRL, ddr_setting->ddr_ctrl);
}


static int hw_training(void)
{
    APB_Wr(PCTL_DLLCR2_ADDR, (APB_Rd(PCTL_DLLCR2_ADDR) & 0xfffc3fff) |
									 (3 << 14 ));
    APB_Wr(PCTL_DLLCR3_ADDR, (APB_Rd(PCTL_DLLCR3_ADDR) & 0xfffc3fff) |
									 (3 << 14 ));
    APB_Wr(PCTL_DTUWD0_ADDR, 0xdd22ee11);
    APB_Wr(PCTL_DTUWD1_ADDR, 0x7788bb44);
    APB_Wr(PCTL_DTUWD2_ADDR, 0xdd22ee11);
    APB_Wr(PCTL_DTUWD3_ADDR, 0x7788bb44);
    APB_Wr(PCTL_DTUWACTL_ADDR,0);
//    		0x300 |          // col addr
//                               (0x7<<10) |      // bank addr
//                               (0x1fff <<13) |  // row addr
//                               (0 <<30 ));      // rank addr
    APB_Wr(PCTL_DTURACTL_ADDR,0);
//    		0x300 |          // col addr
//                               (0x7<<10) |      // bank addr
//                               (0x1fff <<13) |  // row addr
//                               (0 <<30 ));      // rank addr
        APB_Wr(PCTL_PHYCR_ADDR, APB_Rd(PCTL_PHYCR_ADDR) | (1<<31));
        APB_Wr(PCTL_SCTL_ADDR, 1); // init: 0, cfg: 1, go: 2, sleep: 3, wakeup: 4         rddata = 1;        // init: 0, cfg: 1, cfg_req: 2, access: 3, access_req: 4, low_power: 5, low_power_enter_req: 6, low_power_exit_req: 7

        while ((APB_Rd(PCTL_STAT_ADDR) & 0x7 ) != 1 ) {}

        APB_Wr(PCTL_SCTL_ADDR, 2); // init: 0, cfg: 1, go: 2, sleep: 3, wakeup: 4

        while ((APB_Rd(PCTL_STAT_ADDR) & 0x7 ) != 3 ) {}

        while (APB_Rd(PCTL_PHYCR_ADDR) & (1<<31)) {}
        volatile unsigned aaa;
        aaa=APB_Rd(PCTL_DTURD0_ADDR);
		aaa=APB_Rd(PCTL_DTURD1_ADDR);
		aaa=APB_Rd(PCTL_DTURD2_ADDR);
		aaa=APB_Rd(PCTL_DTURD3_ADDR);
		aaa=APB_Rd(PCTL_PHYSR_ADDR) ;
	serial_puts("\nHw Training result");
	serial_put_dword(APB_Rd(PCTL_PHYSR_ADDR) );
    if ( APB_Rd(PCTL_PHYSR_ADDR) & 0x00340000 ) {
        return (1);  // failed.
    } else {
        return (0);  // passed.
    }
}

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

static inline void dtu_enable(void)
{
    APB_Wr(PCTL_DTUECTL_ADDR, 0x1);  // start wr/rd
}

static inline unsigned char check_dtu(void)
{
    unsigned char r_num = 0;
    volatile char *pr, *pw;
    unsigned char i;
    volatile unsigned * rd;
    volatile unsigned * wd;
    pr = (volatile char *)APB_REG_ADDR(PCTL_DTURD0_ADDR);
    pw = (volatile char *)APB_REG_ADDR(PCTL_DTUWD0_ADDR);
    for(i=0;i<16;i++)
    {
        if(*(pr+i) == *(pw+i))
            ++r_num;
    }
    rd = (volatile unsigned *)APB_REG_ADDR(PCTL_DTURD0_ADDR);
    wd = (volatile unsigned *)APB_REG_ADDR(PCTL_DTUWD0_ADDR);
    if(ddr_start_again)
    {
        serial_puts("rd=");
        for(i=0;i<4;i++)
            serial_put_hex(*rd++,32);
        serial_puts(" wd=");
        for(i=0;i<4;i++)
            serial_put_hex(*wd++,32);
    }
    return r_num;
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
            else if(i == 8)
                *best = 9;
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
    unsigned rslr0=((res[0]>>2)&3) | (((res[1]>>2)&3)<<3) | (((res[2]>>2)&3)<<6) | (((res[3]>>2)&3)<<9);
    unsigned rdgr0=(res[0]&3)      | ((res[1]&3)     <<2) | ((res[2]&3)     <<4) | ((res[3]&3)<<6);
    APB_Wr(PCTL_RSLR0_ADDR,rslr0);
    APB_Wr(PCTL_RDGR0_ADDR,rdgr0);
    serial_puts("\nRSLR0=");
    serial_put_hex(rslr0,32);
    serial_puts(" RDGR0=");
    serial_put_dword(rdgr0);
}

SPL_STATIC_FUNC unsigned ddr_init (struct ddr_set * ddr_setting)
{
	unsigned char Tra[4];
	unsigned char chk[DDR_RSLR_LEN*DDR_RDGR_LEN];

	int i,j,k;

	for (k = 0; k < ((ddr_setting->ddr_ctrl&(1<<7))?0x2:0x4); k++) {

		for (i = 0; i < DDR_RSLR_LEN; i++) {
			for (j = 0; j < DDR_RDGR_LEN; j++) {
				init_pctl(ddr_setting);

				if (!start_ddr_config()) {
					return 1;
				}

				// add for DTU
				APB_Wr(PCTL_DTUWD0_ADDR, 0x55aa55aa);
				APB_Wr(PCTL_DTUWD1_ADDR, 0xaa55aa55);
				APB_Wr(PCTL_DTUWD2_ADDR, 0xcc33cc33);
				APB_Wr(PCTL_DTUWD3_ADDR, 0x33cc33cc);
				APB_Wr(PCTL_DTUWACTL_ADDR, 0);
				APB_Wr(PCTL_DTURACTL_ADDR, 0);

				APB_Wr(PCTL_DTUCFG_ADDR, (k << 10) | 0x01); // select byte lane, & enable DTU

				APB_Wr(PCTL_RSLR0_ADDR, i | (i << 3) | (i << 6) | (i << 9));
				APB_Wr(PCTL_RDGR0_ADDR, j | (j << 2) | (j << 4) | (j << 6));

				dtu_enable();

				if (!end_ddr_config()) {
					return 1;
				}
				if(ddr_start_again){
				    serial_puts("\ndtu windows:");
				    serial_put_hex(i * 4 + j,8);

				    chk[i * DDR_RDGR_LEN + j] = check_dtu();
				    serial_puts("result windows :");
				    serial_put_hex(chk[i * 4 + j],8);
			    }else{
				    chk[i * DDR_RDGR_LEN + j] = check_dtu();
				}
			}
		}

		if (get_best_dtu(chk, &Tra[k])) {
			Tra[k]=0;
			serial_puts("\nlane");
			serial_put_hex(k,8);

			serial_puts(" Fail\n");


		}else{
			serial_puts("\nlane");
			serial_put_hex(k,8);

			serial_puts(" Success");
			serial_put_hex(Tra[k],8);

		}

	}

    init_pctl(ddr_setting);
	if (!start_ddr_config()) {
    	return 1;
	}

	set_result(Tra);

	if (!end_ddr_config()) {

		return 1;
	}

	init_dmc(ddr_setting);
    return 0;

}
static inline void hw_tran(void)
{

	init_pctl(&__ddr_setting);
	hw_training();
	init_dmc(&__ddr_setting);

}
static const unsigned lowlevel_ddr(void)
{
    return ddr_init(&__ddr_setting);
}
static const unsigned lowlevel_mem_test_device(void)
{
    return (unsigned)memTestDevice((volatile datum *)CONFIG_SYS_SDRAM_BASE
		    ,__ddr_setting.ram_size);
}
static const unsigned lowlevel_mem_test_data(void)
{
    return (unsigned)memTestDataBus((volatile datum *) CONFIG_SYS_SDRAM_BASE);
}
static const unsigned lowlevel_mem_test_addr(void)
{
    return (unsigned)memTestAddressBus((volatile datum *)CONFIG_SYS_SDRAM_BASE,
				    __ddr_setting.ram_size);
}

static const unsigned ( * mem_test[])(void)={
    lowlevel_ddr,
    lowlevel_mem_test_addr,
    lowlevel_mem_test_data,
#ifdef CONFIG_ENABLE_MEM_DEVICE_TEST
    lowlevel_mem_test_device
#endif
};
#define MEM_DEVICE_TEST_ITEMS (sizeof(mem_test)/sizeof(mem_test[0]))
#ifdef CONFIG_ENABLE_MEM_DEVICE_TEST
#define MEM_DEVICE_TEST_ITEMS_BASE (MEM_DEVICE_TEST_ITEMS -1)
#else
#define MEM_DEVICE_TEST_ITEMS_BASE (MEM_DEVICE_TEST_ITEMS -0)
#endif
SPL_STATIC_FUNC unsigned ddr_init_test(void)
{
    int i;
    unsigned ret=0;
    serial_putc('\n');
    ddr_pll_init(&__ddr_setting);

#ifdef CONFIG_ENABLE_MEM_DEVICE_TEST
    for(i=0;i<MEM_DEVICE_TEST_ITEMS_BASE+ddr_start_again&&ret==0;i++)
#else
    for(i=0;i<MEM_DEVICE_TEST_ITEMS_BASE&&ret==0;i++)
#endif
	{
	        ret=mem_test[i]();
	        serial_puts("\nStage ");
	        serial_put_hex(i,8);
	        serial_puts(" Result ");
	        serial_put_hex(ret,32);

	}
	serial_puts("\nDDR Init Finish\n");

//	ddr_start_again=ret?1:ddr_start_again;
	return ret;
}
