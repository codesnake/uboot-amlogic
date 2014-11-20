#ifdef CONFIG_ARC_SARDAC_ENABLE

#define IO_CBUS_BASE			  0xc1100000

#define CBUS_REG_OFFSET(reg) ((reg) << 2)
#define CBUS_REG_ADDR(reg)	 (IO_CBUS_BASE + CBUS_REG_OFFSET(reg))

#define WRITE_CBUS_REG(reg, val) __raw_writel(val, CBUS_REG_ADDR(reg))
#define READ_CBUS_REG(reg) (__raw_readl(CBUS_REG_ADDR(reg)))
#define WRITE_CBUS_REG_BITS(reg, val, start, len) \
    WRITE_CBUS_REG(reg,	(READ_CBUS_REG(reg) & ~(((1L<<(len))-1)<<(start)) )| ((unsigned)((val)&((1L<<(len))-1)) << (start)))
#define READ_CBUS_REG_BITS(reg, start, len) \
		((READ_CBUS_REG(reg) >> (start)) & ((1L<<(len))-1))


#define delta_busy()				READ_CBUS_REG_BITS(SAR_ADC_REG0, 30, 1)
#define avg_busy()					READ_CBUS_REG_BITS(SAR_ADC_REG0, 29, 1)
#define sample_busy()				READ_CBUS_REG_BITS(SAR_ADC_REG0, 28, 1)
#define get_fifo_cnt()				READ_CBUS_REG_BITS(SAR_ADC_REG0, 21, 5)
	
#define stop_sample()				WRITE_CBUS_REG_BITS(SAR_ADC_REG0, 1, 14, 1)
#define start_sample()				WRITE_CBUS_REG_BITS(SAR_ADC_REG0, 1, 2, 1)

	
#define enable_sample_engine()		WRITE_CBUS_REG_BITS(SAR_ADC_REG0, 1, 0, 1)
#define disable_sample_engine()		WRITE_CBUS_REG_BITS(SAR_ADC_REG0, 0, 0, 1)
#define get_fifo_sample()			READ_CBUS_REG(SAR_ADC_FIFO_RD)


#define set_idle_mux(mux)	 WRITE_CBUS_REG_BITS(SAR_ADC_DETECT_IDLE_SW, mux, 7, 3)
#define set_detect_mux( mux) WRITE_CBUS_REG_BITS(SAR_ADC_DETECT_IDLE_SW, mux, 23, 3)


#define set_chan_list(list, len)	WRITE_CBUS_REG(SAR_ADC_CHAN_LIST, list | ((len-1)<<24))
#define set_avg_mode(ch, mode, num) do {\
	WRITE_CBUS_REG_BITS(SAR_ADC_AVG_CNTL, num, ch*2, 2);\
	WRITE_CBUS_REG_BITS(SAR_ADC_AVG_CNTL, mode, ch*2+16, 2);\
} while(0)
#define set_sample_mux(ch, mux) do {\
		if (ch < 2)\
			WRITE_CBUS_REG_BITS(SAR_ADC_CHAN_10_SW, mux, ch*16 + 7, 3);\
		else\
			WRITE_CBUS_REG_BITS(SAR_ADC_AUX_SW, mux, (ch-2) * 3 + 8, 3);\
	} while(0)

enum {NO_AVG_MODE = 0,SIMPLE_AVG_MODE,MEDIAN_AVG_MODE};
enum {SAMPLE_NUM_1 = 0,SAMPLE_NUM_2,SAMPLE_NUM_4,SAMPLE_NUM_8};
static unsigned char g_chan_mux[8] = {0,1,2,3,4,5,6,7};


int get_adc_sample_in_arc(int chan)
{
	int count=0;
	
	int value = -1;
	int sum;
	
	set_chan_list(chan, 1);
	set_avg_mode(chan, NO_AVG_MODE, SAMPLE_NUM_8);
	set_sample_mux(chan, g_chan_mux[chan]);
	set_detect_mux(g_chan_mux[chan]);
	set_idle_mux(g_chan_mux[chan]); // for revb
	enable_sample_engine();
	start_sample();

	// Read any CBUS register to delay one clock
	// cycle after starting the sampling engine
	// The bus is really fast and we may miss that it started
	{ count = READ_CBUS_REG(ISA_TIMERE); }
	
	count = 0;

	while (delta_busy() || sample_busy() || avg_busy()){
		if (++count > 10000){
//			dbg_prints("ADC busy error.\n");
			goto adc_sample_end;
			}
	}
	
	stop_sample();
    
	sum = 0;
	count = 0;
	value = get_fifo_sample();

	while (get_fifo_cnt()){
		value = get_fifo_sample() & 0x3ff;
		if ((value != 0x1fe) && (value != 0x1ff)){
			sum += value & 0x3ff;
			count++;
		}
	}
//	value = (count) ? (sum / count) : (-1);
	value = (count) ? sum : (0x87654321);


adc_sample_end:
	
	disable_sample_engine();
	
	return value;
}

#endif //CONFIG_ARC_SARDAC_ENABLE

