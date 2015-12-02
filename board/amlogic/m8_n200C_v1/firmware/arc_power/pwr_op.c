/*
 * READ ME:
 * This file is in charge of power manager. It need match different power control, PMU(if any) and wakeup condition.
 * We need special this file in customer/board dir to replace the default pwr_op.c.
 *
*/

/*
 * This pwr_op.c is supply for axp202.
*/
#include <i2c.h>
#include <asm/arch/ao_reg.h>
#include <arc_pwr.h>
#include <linux/types.h>

extern void wait_uart_empty();
extern void udelay__(int i);
#ifdef CONFIG_RN5T618
void rn5t618_set_gpio(int gpio, int output);
#endif

#define CONFIG_IR_REMOTE_WAKEUP 1//for M8 MBox
#ifndef CONFIG_CEC_WAKEUP
#define CONFIG_CEC_WAKEUP       1//for CEC function
#endif

#ifdef CONFIG_IR_REMOTE_WAKEUP
#include "irremote2arc.c"
#endif

#ifdef CONFIG_CEC_WAKEUP
#include <cec_tx_reg.h>
#endif
/*
 * i2c clock speed define for 32K and 24M mode
 */
#define I2C_CLK_SOURCE_IN_RESUME 24000000	// 24MHz
#define I2C_CLK_IN_RESUME	100000	// 100K
#define I2C_CLK_DIV_IN_RESUME (I2C_CLK_SOURCE_IN_RESUME/I2C_CLK_IN_RESUME/4)

#define I2C_CLK_SOURCE_IN_SUSPEND 32768	// 32.768KHz
#define I2C_CLK_IN_SUSPEND	1000 // 1KHz
//#define I2C_CLK_DIV_IN_SUSPEND (I2C_CLK_SOURCE_IN_SUSPEND/I2C_CLK_IN_SUSPEND/4)
/* High pulse is about 740us and low pulse is 540us if use above setting.
 * Use following setting can set high pulse to 620us and low pulse to 420us.
 */
#define I2C_CLK_DIV_IN_SUSPEND 6
/*
 * use globle virable to fast i2c speed
 */
volatile static unsigned char exit_reason = 0;

#ifdef CONFIG_RN5T618
#define I2C_RN5T618_ADDR   (0x32 << 1)
#define i2c_pmu_write_b(reg, val)       i2c_pmu_write(reg, val)
#define i2c_pmu_read_b(reg)             (unsigned  char)i2c_pmu_read_12(reg, 1)
#define i2c_pmu_read_w(reg)             (unsigned short)i2c_pmu_read_12(reg, 2)
#endif
#ifdef CONFIG_PWM_VDDEE_VOLTAGE
static  uint32_t aml_read_reg32(uint32_t _reg)
{
    return readl((volatile void *)_reg);
};
static void aml_write_reg32( uint32_t _reg, const uint32_t _value)
{
    writel( _value,(volatile void *)_reg );
};
static void aml_set_reg32_bits(uint32_t _reg, const uint32_t _value, const uint32_t _start, const uint32_t _len)
{
    writel(( (readl((volatile void *)_reg) & ~((( 1L << (_len) )-1) << (_start))) | ((unsigned)((_value)&((1L<<(_len))-1)) << (_start))), (volatile void *)_reg );
}

#endif  /* CONFIG_PWM_VDDEE_VOLTAGE */

volatile static unsigned char vbus_status;

static int gpio_sel0;
static int gpio_mask;

void printf_arc(const char *str)
{
    f_serial_puts(str);
    wait_uart_empty();
}

#define  I2C_CONTROL_REG      (volatile unsigned long *)0xc8100500
#define  I2C_SLAVE_ADDR       (volatile unsigned long *)0xc8100504
#define  I2C_TOKEN_LIST_REG0  (volatile unsigned long *)0xc8100508
#define  I2C_TOKEN_LIST_REG1  (volatile unsigned long *)0xc810050c
#define  I2C_TOKEN_WDATA_REG0 (volatile unsigned long *)0xc8100510
#define  I2C_TOKEN_WDATA_REG1 (volatile unsigned long *)0xc8100514
#define  I2C_TOKEN_RDATA_REG0 (volatile unsigned long *)0xc8100518
#define  I2C_TOKEN_RDATA_REG1 (volatile unsigned long *)0xc810051c

#define  I2C_END               0x0
#define  I2C_START             0x1
#define  I2C_SLAVE_ADDR_WRITE  0x2
#define  I2C_SLAVE_ADDR_READ   0x3
#define  I2C_DATA              0x4
#define  I2C_DATA_LAST         0x5
#define  I2C_STOP              0x6

#ifdef CONFIG_RN5T618
unsigned char hard_i2c_read8(unsigned char SlaveAddr, unsigned char RegAddr)
{    
    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_STOP  << 24)            |
                             (I2C_DATA_LAST << 20)        |  // Read Data
                             (I2C_SLAVE_ADDR_READ << 16)  |
                             (I2C_START << 12)            |
                             (I2C_DATA << 8)              |  // Read RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (RegAddr << 0);
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);   // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);   // Set the start bit
    while( (*I2C_CONTROL_REG) & (1 << 2) ) {}

    return( (unsigned char)((*I2C_TOKEN_RDATA_REG0) & 0xFF) );
}

void hard_i2c_write8(unsigned char SlaveAddr, unsigned char RegAddr, unsigned char Data)
{
    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_STOP << 16)             |
                             (I2C_DATA << 12)             |    // Write Data
                             (I2C_DATA << 8)              |    // Write RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (Data << 8) | (RegAddr << 0);
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);   // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);   // Set the start bit
    while( (*I2C_CONTROL_REG) & (1 << 2) ) {}
}

unsigned short hard_i2c_read8_16(unsigned char SlaveAddr, unsigned char RegAddr)
{
    unsigned short data;
    unsigned int ctrl;

    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_STOP << 28)             |
                             (I2C_DATA_LAST << 24)        |  // Read Data
                             (I2C_DATA << 20)  |
                             (I2C_SLAVE_ADDR_READ << 16)  |
                             (I2C_START << 12)            |
                             (I2C_DATA << 8)              |  // Read RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (RegAddr << 0);
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);    // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);     // Set the start bit
    while (1) {
        ctrl = (*I2C_CONTROL_REG);
        if (ctrl & (1 << 3)) {          // error case
            return 0;    
        }
        if (!(ctrl & (1 << 2))) {       // controller becomes idle
            break;    
        }
    }

    data = *I2C_TOKEN_RDATA_REG0;
    return data & 0xffff;
}

void i2c_pmu_write(unsigned char reg, unsigned char val)
{
    return hard_i2c_write8(I2C_RN5T618_ADDR, reg, val);
}

unsigned short i2c_pmu_read_12(unsigned int reg, int size)
{
    unsigned short val;
    if (size == 1) {
        val = hard_i2c_read8(I2C_RN5T618_ADDR, reg);
    } else {
        val = hard_i2c_read8_16(I2C_RN5T618_ADDR, reg);    
    }
    return val;
}
#endif

extern void delay_ms(int ms);
void init_I2C()
{
	unsigned v,reg;
	//struct aml_i2c_reg_ctrl* ctrl;

		//save gpio intr setting
	gpio_sel0 = readl(0xc8100084);
	gpio_mask = readl(0xc8100080);

	if(!(readl(0xc8100080) & (1<<8)))//kernel enable gpio interrupt already?
	{
		writel(readl(0xc8100084) | (1<<18) | (1<<16) | (0x3<<0),0xc8100084);
		writel(readl(0xc8100080) | (1<<8),0xc8100080);
		writel(1<<8,0xc810008c); //clear intr
	}

	f_serial_puts("i2c init\n");


	//1. set pinmux
	v = readl(P_AO_RTI_PIN_MUX_REG);
	//master
	v |= ((1<<5)|(1<<6));
	v &= ~((1<<2)|(1<<24)|(1<<1)|(1<<23));
	//slave
	// v |= ((1<<1)|(1<<2)|(1<<3)|(1<<4));
	writel(v,P_AO_RTI_PIN_MUX_REG);
	udelay__(10000);


	reg = readl(P_AO_I2C_M_0_CONTROL_REG);
	reg &= 0xCFC00FFF;
	reg |= (I2C_CLK_DIV_IN_RESUME <<12);             // at 24MHz, i2c speed to 100KHz
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
	writel(0,P_AO_I2C_M_0_TOKEN_LIST0);
	writel(0,P_AO_I2C_M_0_TOKEN_LIST1);
//	delay_ms(20);
//	delay_ms(1);
	udelay__(1000);

#ifdef CONFIG_PLATFORM_HAS_PMU
    v = i2c_pmu_read_b(0x0000);                 // read version
    if (v == 0x00 || v == 0xff)
	{
        serial_put_hex(v, 8);
        f_serial_puts(" Error: I2C init failed!\n");
    }
	else
	{
		serial_put_hex(v, 8);
		f_serial_puts("Success.\n");
	}
#endif

#ifndef CONFIG_NO_32K_XTAL
    //pwm_out_rtc_32k();
#endif
}

#ifdef CONFIG_RN5T618
static unsigned char reg_ldo     = 0;
static unsigned char reg_ldo_rtc = 0;
static unsigned char dcdc1_ctrl  = 0;
static unsigned char charge_timeout = 0;

void rn5t618_set_bits(unsigned char addr, unsigned char bit, unsigned char mask)
{
    unsigned char val;
    val = i2c_pmu_read_b(addr);
    val &= ~(mask);
    val |=  (bit & mask);
    i2c_pmu_write_b(addr, val);
}

#define power_off_vddio_ao28()  rn5t618_set_bits(0x0044, 0x00, 0x01)    // LDO1
#define power_on_vddio_ao28()   rn5t618_set_bits(0x0044, 0x01, 0x01)
#define power_off_vddio_ao18()  rn5t618_set_bits(0x0044, 0x00, 0x02)    // LDO2
#define power_on_vddio_ao18()   rn5t618_set_bits(0x0044, 0x02, 0x02)
#define power_off_vcc18()       rn5t618_set_bits(0x0044, 0x00, 0x04)    // LDO3
#define power_on_vcc18()        rn5t618_set_bits(0x0044, 0x04, 0x04)
#define power_off_vcc28()       rn5t618_set_bits(0x0044, 0x00, 0x08)    // LDO4
#define power_on_vcc28()        rn5t618_set_bits(0x0044, 0x08, 0x08)
#define power_off_avdd18()      rn5t618_set_bits(0x0044, 0x00, 0x10)    // LDO5
#define power_on_avdd18()       rn5t618_set_bits(0x0044, 0x10, 0x10)

#define LDO2_BIT                0x02
#define LDO3_BIT                0x04
#define LDO4_BIT                0x08
#define LDO5_BIT                0x10

#define MODE_PWM                1
#define MODE_PSM                2
#define MODE_AUTO               0

inline void power_off_ddr15() 
{
    rn5t618_set_bits(0x0030, 0x00, 0x01);    // DCDC3
}
inline void power_on_ddr15() 
{
    rn5t618_set_bits(0x0030, 0x01, 0x01);
}

int get_charging_state()
{
#ifdef CONFIG_ALWAYS_POWER_ON
    return 1;
#else
    unsigned char status;
    status = i2c_pmu_read_b(0x00bd);
    return (status & 0xc0) ? 1 : 0;
#endif
}

void rn5t618_shut_down()
{
    unsigned char reg_coulomb[4];
    unsigned char reg_save[4]={0};
    unsigned char flag;
    int save_coulomb, curr_coulomb;

    flag = i2c_pmu_read_b(0x0001);
    if (flag > 0x06) {                                      // OTP version is larger than RN5T618F
        /*
         * save coulomb registers before shut down
         */
        reg_coulomb[0] = i2c_pmu_read_b(0x00f3);
        reg_coulomb[1] = i2c_pmu_read_b(0x00f4);
        reg_coulomb[2] = i2c_pmu_read_b(0x00f5);
        reg_coulomb[3] = i2c_pmu_read_b(0x00f6);
        flag = i2c_pmu_read_b(0x0007);
        printf_arc("save flag:");
        serial_put_hex(flag, 8);
        printf_arc("\n");
        i2c_pmu_write_b(0x00ff, 0x01);                      // change to register banck 1
        printf_arc("curr coulomb:");
        serial_put_hex(reg_coulomb[0], 8);
        serial_put_hex(reg_coulomb[1], 8);
        serial_put_hex(reg_coulomb[2], 8);
        serial_put_hex(reg_coulomb[3], 8);
        printf_arc("\n");
        curr_coulomb = (reg_coulomb[3] <<  0) | 
                       (reg_coulomb[2] <<  8) | 
                       (reg_coulomb[1] << 16) | 
                       (reg_coulomb[0] << 24);
        if (flag & 0x40) {                                  // already saved
            reg_save[0] = i2c_pmu_read_b(0x00bd);
            reg_save[0] = i2c_pmu_read_b(0x00bf);
            reg_save[0] = i2c_pmu_read_b(0x00c1);
            reg_save[0] = i2c_pmu_read_b(0x00c3);
            printf_arc("already saved coulomb:");
            serial_put_hex(reg_save[0], 8);
            serial_put_hex(reg_save[1], 8);
            serial_put_hex(reg_save[2], 8);
            serial_put_hex(reg_save[3], 8);
            printf_arc("\n");
            save_coulomb = (reg_save[3] <<  0) | 
                           (reg_save[2] <<  8) | 
                           (reg_save[1] << 16) | 
                           (reg_save[0] << 24);
            curr_coulomb += save_coulomb;
        }
        reg_coulomb[0] = (curr_coulomb >> 24) & 0xff;
        reg_coulomb[1] = (curr_coulomb >> 16) & 0xff;
        reg_coulomb[2] = (curr_coulomb >>  8) & 0xff;
        reg_coulomb[3] = (curr_coulomb >>  0) & 0xff;
        printf_arc("save coulomb:"); 
        serial_put_hex(reg_coulomb[0], 8);
        serial_put_hex(reg_coulomb[1], 8);
        serial_put_hex(reg_coulomb[2], 8);
        serial_put_hex(reg_coulomb[3], 8);
        printf_arc("\n");
        i2c_pmu_write_b(0x00bd, reg_coulomb[0]);
        i2c_pmu_write_b(0x00bf, reg_coulomb[1]);
        i2c_pmu_write_b(0x00c1, reg_coulomb[2]);
        i2c_pmu_write_b(0x00c3, reg_coulomb[3]);
        i2c_pmu_write_b(0x00ff, 0x00);                     // back to banck 0;
        rn5t618_set_bits(0x0007, 0x40, 0x60);              // set flag
        rn5t618_set_bits(0x00EF, 0x08, 0x08);              // clear coulomb 
    }

#ifdef CONFIG_RESET_TO_SYSTEM
    rn5t618_set_bits(0x0007, 0x00, 0x01);                   // clear flag
#endif
    rn5t618_set_gpio(0, 1);
    rn5t618_set_gpio(1, 1);
    udelay__(100 * 1000);
    rn5t618_set_bits(0x00EF, 0x00, 0x10);                     // disable coulomb counter
    rn5t618_set_bits(0x00E0, 0x00, 0x01);                     // disable fuel gauge 
    rn5t618_set_bits(0x000f, 0x00, 0x01);
    rn5t618_set_bits(0x000E, 0x01, 0x01);
    while (1);
}

int find_idx(int start, int target, int step, int size)
{
    int i = 0;  
    do { 
        if (start >= target) {
            break;    
        }    
        start += step;
        i++; 
    } while (i < size);
    return i;
}

void rn5t618_set_dcdc_voltage(int dcdc, int voltage)
{
    int addr;
    int idx_to;
    addr = 0x35 + dcdc;
    idx_to = find_idx(6000, voltage * 10, 125, 256);            // step is 12.5mV
    i2c_pmu_write_b(addr, idx_to);
#if 1
    printf_arc("set dcdc");
    serial_put_hex(addr-36, 4);
    wait_uart_empty();
    printf_arc(" to 0x");
    serial_put_hex(idx_to, 8);
    wait_uart_empty();
    printf_arc("\n");
#endif
}

void rn5t618_set_dcdc_mode(int dcdc, int mode)
{
    int addr = 0x2C + (dcdc - 1) * 2;
    unsigned char bits = (mode << 4) & 0xff; 

    rn5t618_set_bits(addr, bits, 0x30);
    udelay__(50);
}

static unsigned char gpio_dir = 0xff;
static unsigned char gpio_out = 0;

void rn5t618_set_gpio(int gpio, int output)
{
    int val = output ? 1 : 0;
    if (gpio < 0 || gpio > 3) {
        return ;
    }
    if (gpio_dir == 0xff) {
        gpio_dir = i2c_pmu_read_b(0x0090);
    }
    if (!gpio_out) {
        gpio_out = i2c_pmu_read_b(0x0091);    
    }
    gpio_out &= ~(1 << gpio);
    gpio_out |= (val << gpio);
    i2c_pmu_write_b(0x0091, gpio_out);                      // set output
    gpio_dir |= (1 << gpio);
    i2c_pmu_write_b(0x0090, gpio_dir);                      // set output mode 
    udelay__(50);
}

void rn5t618_get_gpio(int gpio, unsigned char *val)
{
    int value;
    if (gpio < 0 || gpio > 3) {
        return ;
    }
    value = i2c_pmu_read_b(0x0097);                         // read status
    *val = (value & (1 << gpio)) ? 1 : 0;
}

#endif

#ifdef CONFIG_RN5T618
int pmu_get_battery_voltage(void)
{
    unsigned char val[2];
    int result;

    result = i2c_pmu_read_w(0x006A);
    val[0] = result & 0xff;
    val[1] = (result >> 8) & 0xff;
    result = (val[0] << 4) | (val[1] & 0x0f);
    result = (result * 5000) / 4096;                        // resolution: 1.221mV

    return result | (val[0] << 24 | val[1] << 16);
}

#if defined(CONFIG_ENABLE_PMU_WATCHDOG) || defined(CONFIG_RESET_TO_SYSTEM)
void pmu_feed_watchdog(unsigned int flags)
{
    i2c_pmu_write_b(0x0013, 0x00);                      // clear watch dog IRQ
}
#endif /* CONFIG_ENABLE_PMU_WATCHDOG */

void rn5t618_power_off_at_24M()
{
    i2c_pmu_write_b(0x66, 0x29);                                        // select vbat channel
#if 0
    rn5t618_get_gpio(1, &vbus_status);
    if (!vbus_status) {
    //  rn5t618_set_gpio(1, 1);                                         // close boost
    }
#endif
#ifdef CONFIG_POWEROFF_VCCX2
	rn5t618_set_gpio(1, 1);                                             // close vccx2
#endif
    rn5t618_set_gpio(0, 1);                                             // close vccx3
    udelay__(500);

#if defined(CONFIG_VDDAO_VOLTAGE_CHANGE)
#if CONFIG_VDDAO_VOLTAGE_CHANGE
    rn5t618_set_dcdc_voltage(2, CONFIG_VDDAO_SUSPEND_VOLTAGE);
#endif
#endif
#if defined(CONFIG_DCDC_PFM_PMW_SWITCH)
#if CONFIG_DCDC_PFM_PMW_SWITCH
    rn5t618_set_dcdc_mode(2, MODE_PSM);
    printf_arc("dc2 set to PSM\n");
    rn5t618_set_dcdc_mode(3, MODE_PSM);
    printf_arc("dc3 set to PSM\n");
#endif
#endif
    reg_ldo     = i2c_pmu_read_b(0x0044);
    reg_ldo_rtc = i2c_pmu_read_b(0x0045);
    dcdc1_ctrl  = i2c_pmu_read_b(0x002c);
    reg_ldo &= ~(LDO3_BIT | LDO4_BIT);
    i2c_pmu_write_b(0x0044, reg_ldo);                                   // close LDO3 & 4

    dcdc1_ctrl &= ~(0x01);                                              // close DCDC1, vcck
    i2c_pmu_write_b(0x002c, dcdc1_ctrl);

#if defined(CONFIG_ENABLE_PMU_WATCHDOG) || defined(CONFIG_RESET_TO_SYSTEM)
    i2c_pmu_write_b(0x000b, 0x0c);                      // time out to 1s
    i2c_pmu_write_b(0x0013, 0x00);                      // clear watch dog IRQ
    i2c_pmu_write_b(0x0012, 0x40);                      // enable watchdog
#endif
    printf_arc("enter 32K\n");
}

void rn5t618_power_on_at_24M()                                          // need match power sequence of  power_off_at_24M
{
    printf_arc("enter 24MHz. reason:");

#if defined(CONFIG_ENABLE_PMU_WATCHDOG) || defined(CONFIG_RESET_TO_SYSTEM)
    i2c_pmu_write_b(0x0012, 0x00);                      // disable watchdog
    i2c_pmu_write_b(0x000b, 0x01);                      // disable watchdog 
    i2c_pmu_write_b(0x0013, 0x00);                      // clear watch dog IRQ
#endif

    serial_put_hex(exit_reason, 32);
    wait_uart_empty();
    printf_arc("\n");


    rn5t618_set_gpio(3, 1);                                             // should open LDO1.2v before open VCCK
    udelay__(6 * 1000);                                                 // must delay 25 ms before open vcck
    dcdc1_ctrl |= 0x01;
    i2c_pmu_write_b(0x002c, dcdc1_ctrl);                                // open DCDC1, vcck

    reg_ldo |= (LDO3_BIT | LDO4_BIT);
    i2c_pmu_write_b(0x0044, reg_ldo);
	
#if defined(CONFIG_DCDC_PFM_PMW_SWITCH)
#if CONFIG_DCDC_PFM_PMW_SWITCH
    rn5t618_set_dcdc_mode(3, MODE_PWM);
    printf_arc("dc3 set to pwm\n");
    rn5t618_set_dcdc_mode(2, MODE_PWM);
    printf_arc("dc2 set to pwm\n");
#endif
#endif
#if defined(CONFIG_VDDAO_VOLTAGE_CHANGE)
#if CONFIG_VDDAO_VOLTAGE_CHANGE
    rn5t618_set_dcdc_voltage(2, CONFIG_VDDAO_VOLTAGE);
#endif
#endif

#ifdef CONFIG_POWEROFF_VCCX2
	rn5t618_set_gpio(1, 0);
#endif

#if 0
    if (!vbus_status) {
        //rn5t618_set_gpio(1, 0);
    }
#endif
    if (charge_timeout & 0x20) {
        printf_arc("charge timeout detect, reset charger\n");
        rn5t618_set_bits(0x00C5, 0x00, 0x20);                   // clear flag
        rn5t618_set_bits(0x00B3, 0x00, 0x03);
        udelay__(10 * 1000);
        rn5t618_set_bits(0x00B3, 0x03, 0x03);
    }
    rn5t618_set_gpio(3, 0);                                     // close ldo 1.2v when vcck is opened 
}

void rn5t618_power_off_at_32K_1()
{
    unsigned int reg;                               // change i2c speed to 1KHz under 32KHz cpu clock
    //unsigned int sleep_flag = readl(P_AO_RTI_STATUS_REG2);

	reg  = readl(P_AO_I2C_M_0_CONTROL_REG);
	reg &= 0xCFC00FFF;
	if  (readl(P_AO_RTI_STATUS_REG2) == 0x87654321) {
    	reg |= (I2C_CLK_DIV_IN_SUSPEND << 12);              // suspended from uboot 
    } else {
		reg |= (I2C_CLK_DIV_IN_SUSPEND << 12);               // suspended from kernel
    }
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
	writel(0,P_AO_I2C_M_0_SLAVE_ADDR);
	udelay__(10);

    reg_ldo &= ~(LDO5_BIT);
    i2c_pmu_write_b(0x0044, reg_ldo);                   // close LDO5, AVDD1.8

    reg_ldo_rtc &= ~(0x10);
    i2c_pmu_write_b(0x0045, reg_ldo_rtc);               // close ext DCDC 3.3v
}

void rn5t618_power_on_at_32K_1()        // need match power sequence of  power_off_at_32K_1
{
    unsigned int    reg;

    reg_ldo_rtc |= 0x10;
    i2c_pmu_write_b(0x0045, reg_ldo_rtc);               // open ext DCDC 3.3v

    reg_ldo |= (LDO5_BIT);
    i2c_pmu_write_b(0x0044, reg_ldo);                   // open LDO5, AVDD1.8

	reg  = readl(P_AO_I2C_M_0_CONTROL_REG);
	reg &= 0xCFC00FFF;
	reg |= (I2C_CLK_DIV_IN_RESUME << 12);
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
	writel(0,P_AO_I2C_M_0_SLAVE_ADDR);
	udelay__(10);
	
}

void rn5t618_power_off_at_32K_2()       // If has nothing to do, just let null
{
    // TODO: add code here
}

void rn5t618_power_on_at_32K_2()        // need match power sequence of power_off_at_32k
{
    // TODO: add code here
}
#endif


#ifdef CONFIG_PWM_POWER
#ifdef CONFIG_PWM_VDDEE_VOLTAGE

static int vcck_pwm_on(void)
{
    //aml_set_reg32_bits(P_PREG_PAD_GPIO2_EN_N, 0, 29, 1);
    //set GPIODV 28 to PWM D
    aml_set_reg32_bits(P_PERIPHS_PIN_MUX_3, 1, 26, 1);
    
    /* set  pwm_d regs */
    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 0, 16, 7);  //pwm_d_clk_div
    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 0, 6, 2);  //pwm_d_clk_sel
    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 1, 23, 1);  //pwm_d_clk_en
    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 1, 1, 1);  //enable pwm_d
    
    return 0;
}
#if 0
static int vcck_pwm_off(void)
{
    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 0, 1, 1);  //disable pwm_d
    return 0;
}
#endif

static int pwm_duty_cycle_set(int duty_high,int duty_total)
{
    int pwm_reg=0;

    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 0, 16, 7);  //pwm_d_clk_div
    if(duty_high > duty_total){
        printf_arc("error: duty_high larger than duty_toral !!!\n");
        return -1; 
    }
    aml_write_reg32(P_PWM_PWM_D, (duty_high << 16) | (duty_total-duty_high));
    udelay__(100000);

    pwm_reg = aml_read_reg32(P_PWM_PWM_D);
#if 1
    printf_arc("##### P_PWM_PWM_D value = ");
    serial_put_hex(pwm_reg, 32);
    printf_arc("\n");
#endif
    return 0;
}

int n200C_pwm_set_vddEE_voltage(int voltage)
{
    printf_arc("n200C_pwm_set_vddEE_voltage\n");
    
    int duty_high = 0;
    //int duty_high_tmp = 0;
    //int tmp1,tmp2,tmp3;
    vcck_pwm_on();
    printf_arc("## VDDEE voltage = 0x");
    serial_put_hex(voltage, 16);
    printf_arc("\n");

    //duty_high = (28-(voltage-860)/10);
    duty_high = (28 - (((voltage-860)*103) >> 10) );



#if 1
    printf_arc("##### duty_high = 0x");
    serial_put_hex(duty_high, 16);
    printf_arc("\n");

#endif
    pwm_duty_cycle_set(duty_high,28);
    return 0;
}
#endif 

void power_off_at_24M()
{

#ifdef CONFIG_PWM_VDDEE_VOLTAGE
    n200C_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_SUSPEND_VOLTAGE);    
#endif
//close vcck GPIAO_3
    clrbits_le32(P_AO_GPIO_O_EN_N, (1<<3) | (1<<19) | (1<<2));
    setbits_le32(P_AO_GPIO_O_EN_N, (1<<18));



//    clrbits_le32(P_AO_GPIO_O_EN_N, (1<<14));
//    clrbits_le32(P_AO_GPIO_O_EN_N, (1<<30));
    printf_arc("suspend stay 24M...\n");
    wait_uart_empty();
}
void power_on_at_24M()                                          // need match power sequence of  power_off_at_24M
{
    printf_arc("enter 24MHz. reason:");
    serial_put_hex(exit_reason, 32);
    printf_arc("\n");
    wait_uart_empty();
#ifdef CONFIG_PWM_VDDEE_VOLTAGE
    n200C_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_VOLTAGE);    
#endif

    clrbits_le32(P_AO_GPIO_O_EN_N, (1<<3) | (1<<2) | (1<<18));
    setbits_le32(P_AO_GPIO_O_EN_N, (1<<19));

/*
	clrbits_le32(P_PERIPHS_PIN_MUX_7, 1<<31);
    clrbits_le32(P_PERIPHS_PIN_MUX_3, (1 << 22));
	setbits_le32(P_PERIPHS_PIN_MUX_9, 1<<19);
    
    // PWM_E,  wifi_32k as RTC clk
    clrbits_le32(P_PWM_MISC_REG_EF, (0x7f<<8) | (3<<4));
    setbits_le32(P_PWM_MISC_REG_EF, ((1 << 15) | (0<<8) | (0<<4) | (1<<0)));
//    writel(0x8001, P_PWM_MISC_REG_EF);
    writel(0x16d016d, P_PWM_PWM_E);   //pwm duty
    */
    
//    clrbits_le32(P_AO_GPIO_O_EN_N, (1<<14));
//    setbits_le32(P_AO_GPIO_O_EN_N, (1<<30));
}
#endif

unsigned int detect_key(unsigned int flags)
{
#ifdef CONFIG_RN5T618
    int delay_cnt   = 0;
    int power_status;
    int prev_status;
    int battery_voltage;
    int low_bat_cnt = 0;
#endif
    int ret = FLAG_WAKEUP_PWRKEY;

#ifdef CONFIG_IR_REMOTE_WAKEUP
#ifndef CONFIG_NO_32K_XTAL
    //backup the remote config (on arm)
    backup_remote_register();
    //set the ir_remote to 32k mode at ARC
    init_custom_trigger();
#endif
#ifdef CONFIG_CEC_WAKEUP
    udelay__(10000);
    if(hdmi_cec_func_config & 0x1){
        cec_power_on();
        cec_msg.log_addr = 4;
        remote_cec_hw_reset();
        cec_node_init();
    }
#endif
#endif

//    writel(readl(P_AO_GPIO_O_EN_N)|(1 << 3),P_AO_GPIO_O_EN_N);
//    writel(readl(P_AO_RTI_PULL_UP_REG)|(1 << 3)|(1<<19),P_AO_RTI_PULL_UP_REG);
#ifdef CONFIG_RN5T618
	prev_status = get_charging_state();
#endif
    do {
        /*
         * when extern power status has changed, we need break
         * suspend loop and resume system.
         */
#ifdef CONFIG_RN5T618
	    power_status = get_charging_state();
        if ((flags == 0x87654321) && (!power_status)) {      // suspend from uboot
            ret = FLAG_WAKEUP_PWROFF;
            exit_reason = -1;
            break;
        }
        if (power_status ^ prev_status) {
            exit_reason = 1;
            break;
        }
        delay_cnt++;

    #if defined(CONFIG_ENABLE_PMU_WATCHDOG) || defined(CONFIG_RESET_TO_SYSTEM)
        pmu_feed_watchdog(flags);
    #endif
        if (delay_cnt >= 3000) {
            if (flags != 0x87654321 && !power_status) {
                /*
                 * when battery voltage is too low but no power supply and suspended from kernel,
                 * we need to break suspend loop to resume system, then system will shut down
                 */
                battery_voltage = pmu_get_battery_voltage();
                if (((battery_voltage & 0xffff) < 3480) && (battery_voltage & 0xffff)) {
                    low_bat_cnt++;
                    if (low_bat_cnt >= 3) {
                        exit_reason = (battery_voltage & 0xffff0000) | 2;
                        break;
                    }
                } else {
                    low_bat_cnt = 0;
                }
                if ((readl(0xc8100088) & (1<<8))) {        // power key
                    exit_reason = 3;
                    break;
                }
            } else if (power_status) {
                charge_timeout = i2c_pmu_read_b(0x00c5);
                if (charge_timeout & 0x20) {
                    exit_reason = 4;
                    break;
                } else {
                    charge_timeout = 0;
                }
                if ((readl(0xc8100088) & (1<<8))) {        // power key
                    exit_reason = 5;
                    break;
                }
            }
            delay_cnt = 0;
        }
#endif

#ifdef CONFIG_IR_REMOTE_WAKEUP
		if(readl(P_AO_RTI_STATUS_REG2) == 0x4853ffff){
            exit_reason = 10;
			break;
		}
		if(remote_detect_key()){
			exit_reason = 6;
			break;
		}
#endif
#ifdef CONFIG_CEC_WAKEUP
        if(hdmi_cec_func_config & 0x1){
          cec_handler();	
          if(cec_msg.cec_power == 0x1){  //cec power key
                break;
            }
        }
#endif

	    if((readl(P_AO_RTC_ADDR1) >> 12) & 0x1) {
            exit_reason = 7;
			ret = FLAG_WAKEUP_ALARM;
            break;
        }

#ifdef CONFIG_BT_WAKEUP
        if(readl(P_PREG_PAD_GPIO0_I)&(0x1<<16)){
			exit_reason = 8;
            ret = FLAG_WAKEUP_BT;
			break;
		}
#endif
#ifdef CONFIG_WIFI_WAKEUP
			if ((flags != 0x87654321) &&(readl(P_AO_GPIO_O_EN_N)&(0x1<<22)))
			if(readl(P_PREG_PAD_GPIO0_I)&(0x1<<21)){
				exit_reason = 9;
				ret = FLAG_WAKEUP_WIFI;
				break;
			}
#endif

    } while (1);   //(!(readl(0xc8100088) & (1<<8)));            // power key

	writel(1<<8,0xc810008c);
	writel(gpio_sel0, 0xc8100084);
	writel(gpio_mask,0xc8100080);

#ifdef CONFIG_IR_REMOTE_WAKEUP
#ifndef CONFIG_NO_32K_XTAL
	resume_remote_register();
#endif
#endif

    return ret;
}


void arc_pwr_register(struct arc_pwr_op *pwr_op)
{
//    printf_arc("%s\n", __func__);
#ifdef CONFIG_RN5T618
	pwr_op->power_off_at_24M    = rn5t618_power_off_at_24M;
	pwr_op->power_on_at_24M     = rn5t618_power_on_at_24M;

	pwr_op->power_off_at_32K_1  = rn5t618_power_off_at_32K_1;
	pwr_op->power_on_at_32K_1   = rn5t618_power_on_at_32K_1;
	pwr_op->power_off_at_32K_2  = rn5t618_power_off_at_32K_2;
	pwr_op->power_on_at_32K_2   = rn5t618_power_on_at_32K_2;

	pwr_op->power_off_ddr15     = 0;//rn5t618_power_off_ddr15;
	pwr_op->power_on_ddr15      = 0;//rn5t618_power_on_ddr15;

	pwr_op->shut_down           = rn5t618_shut_down;

	pwr_op->detect_key          = rn5t618_detect_key;
 #endif
 #ifdef CONFIG_PWM_POWER
	pwr_op->power_off_at_24M    = power_off_at_24M;
	pwr_op->power_on_at_24M     = power_on_at_24M;
#endif
	pwr_op->detect_key          = detect_key;
 
}


