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


//#define CONFIG_IR_REMOTE_WAKEUP 1//for M8 MBox

#ifdef CONFIG_IR_REMOTE_WAKEUP
#include "irremote2arc.c"
#endif

/*
 * i2c clock speed define for 32K and 24M mode only for M8B
 */
#define I2C_CLK_SOURCE_IN_RESUME 24000000	// 24MHz
#define I2C_CLK_IN_RESUME	100000	// 100K
#define I2C_CLK_DIV_IN_RESUME (I2C_CLK_SOURCE_IN_RESUME/I2C_CLK_IN_RESUME)
#define I2C_CLK_HIGH_DIV_IN_RESUME (I2C_CLK_DIV_IN_RESUME/2)
#define I2C_CLK_LOW_DIV_IN_RESUME (I2C_CLK_DIV_IN_RESUME/4)

#define I2C_CLK_SOURCE_IN_SUSPEND 32768	// 32.768KHz
#define I2C_CLK_IN_SUSPEND	1000 // 1KHz
#define I2C_CLK_DIV_IN_SUSPEND (I2C_CLK_SOURCE_IN_SUSPEND/I2C_CLK_IN_SUSPEND)
//#define I2C_CLK_HIGH_DIV_IN_SUSPEND (I2C_CLK_DIV_IN_SUSPEND/2)
//#define I2C_CLK_LOW_DIV_IN_SUSPEND (I2C_CLK_DIV_IN_SUSPEND/4)
/* High pulse is about 740us and low pulse is 540us if use above setting.
 * Use following setting can set high pulse to 540us.
 */
#define I2C_CLK_HIGH_DIV_IN_SUSPEND 10
#define I2C_CLK_LOW_DIV_IN_SUSPEND 8

/*
 * use globle virable to fast i2c speed
 */
static unsigned char exit_reason = 0;
extern void wait_uart_empty(void);
extern void udelay__(int i);

#ifdef CONFIG_AML1218
#define AML1218_DCDC1                1
#define AML1218_DCDC2                2
#define AML1218_DCDC3                3
#define AML1218_BOOST                4
#define I2C_AML1218_ADDR                (0x35 << 1)
#define i2c_pmu_write_b(reg, val)       hard_i2c_write168(I2C_AML1218_ADDR, reg, val); 
#define i2c_pmu_write_w(reg, val)       hard_i2c_write1616(I2C_AML1218_ADDR, reg, val); 
#define i2c_pmu_read_b(reg)             (unsigned  char)hard_i2c_read168(I2C_AML1218_ADDR, reg)
#define i2c_pmu_read_w(reg)             (unsigned short)hard_i2c_read1616(I2C_AML1218_ADDR, reg)

/*
 * Amlogic PMU suspend/resume interface
 */
#define PWR_UP_SW_ENABLE_ADDR       0x82
#define PWR_DN_SW_ENABLE_ADDR       0x84

#define   AML1218_POWER_EXT_DCDC_VCCK_BIT   11
#define   AML1218_POWER_LDO6_BIT            8
#define   AML1218_POWER_LDO5_BIT            7
#define   AML1218_POWER_LDO4_BIT            6
#define   AML1218_POWER_LDO3_BIT            5

#define   AML1218_POWER_LDO2_BIT            4
#define   AML1218_POWER_DC4_BIT             0
#define   AML1218_POWER_DC3_BIT             3
#define   AML1218_POWER_DC2_BIT             2
#define   AML1218_POWER_DC1_BIT             1
#endif  /* CONFIG_AML1218 */
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

#endif  /* CONFIG_AML1218 */

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

#ifdef CONFIG_AML1218
#define I2C_WAIT_CNT        (24 * 8 * 1000)
int hard_i2c_check_error(void)
{
    if (*I2C_CONTROL_REG & 0x08) {
        printf_arc("-- i2c error, CTRL:");
        serial_put_hex(*I2C_CONTROL_REG, 32);
        printf_arc("\n");
        return -1;
    }
    return 0;
}

int hard_i2c_wait_complete(void)
{
    int delay = 0;

    while (delay < I2C_WAIT_CNT) {
        if (!((*I2C_CONTROL_REG) & (1 << 2))) {     // idle
            break;
        }
        delay++;
    }
    if (delay >= I2C_WAIT_CNT) {
        printf_arc("i2c timeout\n");
    }
    return hard_i2c_check_error();
}

unsigned short hard_i2c_read1616(unsigned char SlaveAddr, unsigned short RegAddr)
{
    unsigned short data;

    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_DATA_LAST << 28)   |
                             (I2C_DATA << 24)        |  // Read Data
                             (I2C_SLAVE_ADDR_READ << 20)  |
                             (I2C_START << 16)            |
                             (I2C_DATA << 12)  |
                             (I2C_DATA << 8)              |  // Read RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);
    (*I2C_TOKEN_LIST_REG1) = I2C_STOP;

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (RegAddr << 0);
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);    // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);     // Set the start bit
    hard_i2c_wait_complete();

    data = *I2C_TOKEN_RDATA_REG0;
    return data & 0xffff;
}

unsigned char hard_i2c_read168(unsigned char SlaveAddr, unsigned short RegAddr)
{
    unsigned char data;

    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_STOP << 28)             |
                             (I2C_DATA_LAST << 24)        |  // Read Data
                             (I2C_SLAVE_ADDR_READ << 20)  |
                             (I2C_START << 16)            |
                             (I2C_DATA << 12)             |
                             (I2C_DATA << 8)              |  // Read RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);
    (*I2C_TOKEN_LIST_REG1) = I2C_END;

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (RegAddr << 0); 
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);   // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);   // Set the start bit

    hard_i2c_wait_complete();

    data = *I2C_TOKEN_RDATA_REG0 & 0xff;
    return data;
}

void hard_i2c_write1616(unsigned char SlaveAddr, unsigned short RegAddr, unsigned short Data)
{
    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_STOP  << 24)            |
                             (I2C_DATA << 20)             |    // Write Data
                             (I2C_DATA << 16)             |    // Write Data
                             (I2C_DATA << 12)             |
                             (I2C_DATA << 8)              |    // Write RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);
    (*I2C_TOKEN_LIST_REG1) = I2C_END;

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (Data << 16) | (RegAddr << 0); 
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);   // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);   // Set the start bit

    hard_i2c_wait_complete();
}

void hard_i2c_write168(unsigned char SlaveAddr, unsigned short RegAddr, unsigned char Data)
{
    // Set the I2C Address
    (*I2C_SLAVE_ADDR) = ((*I2C_SLAVE_ADDR) & ~0xff) | SlaveAddr;
    // Fill the token registers
    (*I2C_TOKEN_LIST_REG0) = (I2C_STOP  << 20)            |
                             (I2C_DATA << 16)             |    // Write Data
                             (I2C_DATA << 12)             |
                             (I2C_DATA << 8)              |    // Write RegAddr
                             (I2C_SLAVE_ADDR_WRITE << 4)  |
                             (I2C_START << 0);
    (*I2C_TOKEN_LIST_REG1) = I2C_END;

    // Fill the write data registers
    (*I2C_TOKEN_WDATA_REG0) = (Data << 16) | (RegAddr << 0); 
    // Start and Wait
    (*I2C_CONTROL_REG) &= ~(1 << 0);   // Clear the start bit
    (*I2C_CONTROL_REG) |= (1 << 0);   // Set the start bit

    hard_i2c_wait_complete();
}

int find_idx(int start, int target, int step, int size)
{
    int i = 0; 
    do { 
        if ((start - step) < target) {
            break;    
        }    
        start -= step;
        i++; 
    } while (i < size);
    return i;
}
#endif  /* CONFIG_AML1218 */ 

extern void delay_ms(int ms);

void init_I2C()
{
	unsigned v,reg;

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
	reg |= (I2C_CLK_HIGH_DIV_IN_RESUME <<12);             // at 24MHz, i2c speed to 100KHz
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
	writel((1<<28)|(I2C_CLK_LOW_DIV_IN_RESUME<<16),P_AO_I2C_M_0_SLAVE_ADDR);
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
}

#ifdef CONFIG_AML1218
static unsigned char otg_status = 0;

void aml1218_set_bits(unsigned short addr, unsigned char bit, unsigned char mask)
{
    unsigned char val;
    val = i2c_pmu_read_b(addr);
    val &= ~(mask);
    val |=  (bit & mask);
    i2c_pmu_write_b(addr, val);
}

void aml1218_set_pfm(int dcdc, int en)
{
    unsigned char val;
    if (dcdc < 1 || dcdc > 4 || en > 1 || en < 0) {
        return ;    
    }
    switch(dcdc) {
    case AML1218_DCDC1:
        val = i2c_pmu_read_b(0x003b);
        if (en) {
            val |=  (1 << 5);                                   // pfm mode
        } else {
            val &= ~(1 << 5);                                   // pwm mode
        }
        i2c_pmu_write_b(0x003b, val);
        break;
    case AML1218_DCDC2:
        val = i2c_pmu_read_b(0x0044);
        if (en) {
            val |=  (1 << 5);    
        } else {
            val &= ~(1 << 5);   
        }
        i2c_pmu_write_b(0x0044, val);
        break;
    case AML1218_DCDC3:
        val = i2c_pmu_read_b(0x004e);
        if (en) {
            val |=  (1 << 7);    
        } else {
            val &= ~(1 << 7);   
        }
        i2c_pmu_write_b(0x004e, val);
        break;
    case AML1218_BOOST:
        val = i2c_pmu_read_b(0x0028);
        if (en) {
            val |=  (1 << 0);    
        } else {
            val &= ~(1 << 0);   
        }
        i2c_pmu_write_b(0x0028, val);
        break;
    default:
        break;
    }
     udelay__(1000); 
}

int aml1218_set_gpio(int gpio, int val)
{
#if 0
    unsigned char data;

    if (pin <= 0 || pin > 3 || val > 1 || val < 0) {
        printf_arc("ERROR, invalid input value\n");
        return -1;
    }
    if (val < 2) {
        data = ((val ? 1 : 0) << (pin));
    } else {
        return -1;
    }
    aml1218_set_bits(0x0013, data, (1 << pin));
    udelay__(50);
    return 0;
#else
    unsigned int data;

    if (gpio > 4 || gpio <= 0) {
        return -1;
    }

    data = (1 << (gpio + 11));
    if (val) {
        i2c_pmu_write_w(PWR_DN_SW_ENABLE_ADDR, data);    
    } else {
        i2c_pmu_write_w(PWR_UP_SW_ENABLE_ADDR, data);    
    }
    udelay__(100);
    return 0;
#endif
}

int aml1218_get_battery_voltage()
{
    unsigned short val; 
    int result = 0;
    
    val = i2c_pmu_read_w(0x00AF); 
    
    result = (val * 4800) / 4096;
    
    return result;
}

int aml1218_get_charge_status(void)
{
    unsigned short val = 0;

    val = i2c_pmu_read_b(0x00E0);
    if (val & 0x18) {                // DCIN & VBUS are OK
        return 1;    
    } else {
        return 0;
    }
}

// modify by endy
void aml_pmu_power_ctrl(int on, int bit_mask)
{
    unsigned short addr = on ? PWR_UP_SW_ENABLE_ADDR : PWR_DN_SW_ENABLE_ADDR;
    i2c_pmu_write_w(addr, (unsigned short)bit_mask);
    udelay__(100);
}

#define power_off_ao18()            aml_pmu_power_ctrl(0, 1 << AML1218_POWER_LDO3_BIT)
#define power_on_ao18()             aml_pmu_power_ctrl(1, 1 << AML1218_POWER_LDO3_BIT)
#define power_off_vcc18()           aml_pmu_power_ctrl(0, 1 << AML1218_POWER_LDO4_BIT)
#define power_on_vcc18()            aml_pmu_power_ctrl(1, 1 << AML1218_POWER_LDO4_BIT) 
#define power_off_vcc_cam()         aml_pmu_power_ctrl(0, 1 << AML1218_POWER_LDO6_BIT)
#define power_on_vcc_cam()          aml_pmu_power_ctrl(1, 1 << AML1218_POWER_LDO6_BIT)

#define power_off_vcc28()           aml_pmu_power_ctrl(0, 1 << AML1218_POWER_LDO5_BIT)
#define power_on_vcc28()            aml_pmu_power_ctrl(1, 1 << AML1218_POWER_LDO5_BIT) 

#define power_off_vcck()            aml_pmu_power_ctrl(0, 1 << AML1218_POWER_EXT_DCDC_VCCK_BIT)  
#define power_on_vcck()             aml_pmu_power_ctrl(1, 1 << AML1218_POWER_EXT_DCDC_VCCK_BIT)

#define power_off_vcc33()           aml_pmu_power_ctrl(0, 1 << AML1218_POWER_DC3_BIT)  
#define power_on_vcc33()            aml_pmu_power_ctrl(1, 1 << AML1218_POWER_DC3_BIT)
#define power_off_vcc50()           aml_pmu_power_ctrl(0, 1 << AML1218_POWER_DC4_BIT)
#define power_on_vcc50()            aml_pmu_power_ctrl(1, 1 << AML1218_POWER_DC4_BIT)

static char format_buf[12] = {};
static char *format_dec_value(unsigned int val)
{
    unsigned int tmp, i = 11;

    for (i = 0; i < 12; i++) {
        format_buf[i] = 0;
    }
    i = 11;
    while (val) {
        tmp = val % 10; 
        val /= 10;
        format_buf[--i] = tmp + '0';
    }
    return format_buf + i;
}

static void print_voltage_info(char *voltage_prefix, int voltage_idx, unsigned int voltage, unsigned int reg_from, unsigned int reg_to, unsigned int addr)
{
    printf_arc(voltage_prefix);
    if (voltage_idx >= 0) {
        serial_put_hex(voltage_idx, 8);
    }
    printf_arc(" set to ");
    printf_arc(format_dec_value(voltage));
    printf_arc(", register from 0x");
    serial_put_hex(reg_from, 16);
    printf_arc(" to 0x");
    serial_put_hex(reg_to, 16);
    printf_arc(", addr:0x");
    serial_put_hex(addr, 16);
    printf_arc("\n");
}

int aml1218_set_dcdc_voltage(int dcdc, int voltage)
{
    int addr;
    int idx_to;
    int range    = 64;
    int step     = 19;
    int start    = 1881;
    int idx_cur;
    int val;

    if (dcdc > 3 || dcdc < 0) {
        return -1;    
    }
    addr = 0x34+(dcdc-1)*9;
    if (dcdc == 3) {
        step     = 50; 
        range    = 64; 
        start    = 3600;
    }
    idx_cur  = i2c_pmu_read_b(addr);
    idx_to   = find_idx(start, voltage, step, range);
#if 1
    print_voltage_info("DCDC", dcdc, voltage, idx_cur, idx_to << 1, addr);
#endif
    val = idx_cur;
    idx_cur = (idx_cur & 0x7e) >> 1;

    step = idx_cur - idx_to;
    if (step < 0) {
        step = -step;    
    }
    val &= ~0x7e;
    val |= (idx_to << 1);
    i2c_pmu_write_b(addr, val);
    udelay__(20 * step);

    return 0;
}

void aml1218_power_off_at_24M()
{
    aml1218_set_bits(0x012f, 0x00, 0x10);                               // power off hdmi 5v 
    otg_status = i2c_pmu_read_b(0x0019);
    i2c_pmu_write_b(0x0019, 0x10);                                      // cut usb output
    power_off_vcc50();
    udelay__(100);
    aml1218_set_gpio(1, 1);                                             // close vccx3
    aml1218_set_gpio(2, 1);                                             // close vccx2
    udelay__(500);

#if defined(CONFIG_VDDAO_VOLTAGE_CHANGE)
#if CONFIG_VDDAO_VOLTAGE_CHANGE
    aml1218_set_dcdc_voltage(1, CONFIG_VDDAO_SUSPEND_VOLTAGE);
#endif
#endif
#if defined(CONFIG_DCDC_PFM_PMW_SWITCH)
#if CONFIG_DCDC_PFM_PMW_SWITCH
  //aml1218_set_pfm(2, 1);
  //printf_arc("dc2 set to PFM\n");
#endif
#endif

    printf_arc("uvlo to 3.21v\n");
    aml1218_set_bits(0x0061, 0x00, 0x01);
    aml1218_set_bits(0x0062, 0x00, 0xc0);           // set UVLO to 3.21v

    aml1218_set_bits(0x0035, 0x00, 0x07);                               // set DCDC OCP to 1.5A to protect DCDC
    //aml1218_set_bits(0x003e, 0x00, 0x07);                               // set DCDC OCP to 1.5A to protect DCDC
    aml1218_set_bits(0x0047, 0x03, 0x07);                               // set DCDC3

    power_off_vcc_cam();                                                // close LDO6
    power_off_vcc28();                                                  // close LDO5
    power_off_vcck();                                                   // close DCDC1, vcck
    printf_arc("enter 32K\n");
}

void aml1218_power_on_at_24M()
{
    unsigned char val;
    printf_arc("enter 24MHz. reason:");
    aml1218_set_gpio(2, 0);                                     // open vccx2

    serial_put_hex(exit_reason, 32);
    wait_uart_empty();
    printf_arc("\n");

    //aml1218_set_gpio(3, 1);                                             // should open LDO1.2v before open VCCK
    udelay__(6 * 1000);                                                 // must delay 25 ms before open vcck

    power_on_vcck();                                                    // open DCDC1, vcck
    power_on_vcc28();                                                   // open LDO5, VCC2.8
    power_on_vcc_cam();
    
    printf_arc("uvlo to 2.95v\n");
    aml1218_set_bits(0x0061, 0x01, 0x01);
    aml1218_set_bits(0x0062, 0x80, 0xc0);           // set UVLO to 2.95v
#if defined(CONFIG_DCDC_PFM_PMW_SWITCH)
#if CONFIG_DCDC_PFM_PMW_SWITCH
  //aml1218_set_pfm(2, 0);
  //printf_arc("dc2 set to pwm\n");
#endif
#endif
#if defined(CONFIG_VDDAO_VOLTAGE_CHANGE)
#if CONFIG_VDDAO_VOLTAGE_CHANGE
    aml1218_set_dcdc_voltage(1, CONFIG_VDDAO_VOLTAGE);
#endif
#endif

    //aml1218_set_gpio(3, 0);                                     // close ldo 1.2v when vcck is opened
    aml1218_set_bits(0x001A, 0x00, 0x06);
    val = i2c_pmu_read_b(0x00d6);
    if (val & 0x01) {
        printf_arc("I saw boost fault:");
        serial_put_hex(val, 8);
        printf_arc("\n");
        power_off_vcc50();
    }
    udelay__(2 * 1000);
    printf_arc("open boost\n");
    power_on_vcc50();
    udelay__(1000);
    aml1218_set_bits(0x1A, 0x06, 0x06);
    i2c_pmu_write_b(0x0019, otg_status);

    aml1218_set_bits(0x012f, 0x10, 0x10);                               // power off hdmi 5v 
    val = i2c_pmu_read_b(0x00E1);
    if (val & 0x02) {
        printf_arc("extern power plugged, set dcdc current limit to 1.5A\n");
        aml1218_set_bits(0x003b, 0x00, 0x40);
        aml1218_set_bits(0x0044, 0x00, 0x40);
        i2c_pmu_write_b(0x011d, 0x02);
        i2c_pmu_write_b(0x011f, 0x02);

        aml1218_set_bits(0x0035, 0x00, 0x07);
        //aml1218_set_bits(0x003e, 0x00, 0x07);
        aml1218_set_bits(0x0047, 0x00, 0x07);
        aml1218_set_bits(0x004f, 0x08, 0x08);
    } else {
        printf_arc("extern power unpluged, set dcdc current limit to 2A\n");
        aml1218_set_bits(0x003b, 0x40, 0x40);
        aml1218_set_bits(0x0044, 0x40, 0x40);
        i2c_pmu_write_b(0x011d, 0x00);
        i2c_pmu_write_b(0x011f, 0x00);

        aml1218_set_bits(0x004f, 0x00, 0x08);
        aml1218_set_bits(0x0035, 0x04, 0x07);
        //aml1218_set_bits(0x003e, 0x04, 0x07);
        aml1218_set_bits(0x0047, 0x04, 0x07);
    }
}

void aml1218_power_off_at_32K_1()
{
    unsigned int reg;                               // change i2c speed to 1KHz under 32KHz cpu clock

    reg  = readl(P_AO_I2C_M_0_CONTROL_REG);
    reg &= 0xCFC00FFF;
    if  (readl(P_AO_RTI_STATUS_REG2) == 0x87654321) {
        reg |= (I2C_CLK_HIGH_DIV_IN_SUSPEND << 12);              // suspended from uboot 
    } else {
        reg |= (I2C_CLK_HIGH_DIV_IN_SUSPEND << 12);               // suspended from kernel
    }
    writel(reg,P_AO_I2C_M_0_CONTROL_REG);
    writel((1<<28)|(I2C_CLK_LOW_DIV_IN_SUSPEND<<16),P_AO_I2C_M_0_SLAVE_ADDR);
    udelay__(10);

//  power_off_vcc18();                                  // close LDO4, vcc1.8v
//  power_off_vcc33();                                  // close DCDC3, VCC3.3v
}

void aml1218_power_on_at_32K_1()
{
    unsigned int    reg;

//  power_on_vcc33();                                   // open ext DCDC 3.3v
//  power_on_vcc18();                                   // open LDO4, vcc1.8v

    reg  = readl(P_AO_I2C_M_0_CONTROL_REG);
    reg &= 0xCFC00FFF;
    reg |= (I2C_CLK_HIGH_DIV_IN_RESUME << 12);
    writel(reg,P_AO_I2C_M_0_CONTROL_REG);
    writel((1<<28)|(I2C_CLK_LOW_DIV_IN_RESUME<<16),P_AO_I2C_M_0_SLAVE_ADDR);
    udelay__(10);
}

void aml1218_power_off_at_32K_2()
{
       // TODO: add code here
}

void aml1218_power_on_at_32K_2()
{
       // TODO: add code here
}


void aml1218_shut_down()
{
    i2c_pmu_write_b(0x0019, 0x10);                              // cut usb output 
    i2c_pmu_write_w(0x0084, 0x0001);                            // close boost
    udelay__(10 * 1000);
    aml1218_set_gpio(1, 1);
    aml1218_set_gpio(2, 1);
    aml1218_set_gpio(3, 1);
    udelay__(100 * 1000);
    i2c_pmu_write_b(0x0081, 0x20);                                      // soft power off
}
#endif  /* CONFIG_AML1218 */

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

int m8b_pwm_set_vddEE_voltage(int voltage)
{
    printf_arc("m8b_pwm_set_vddEE_voltage\n");
    
    int duty_high = 0;
    //int duty_high_tmp = 0;
    //int tmp1,tmp2,tmp3;
    vcck_pwm_on();
    printf_arc("## VDDEE voltage = 0x");
    serial_put_hex(voltage, 16);
    printf_arc("\n");

    //duty_high = (28-(voltage-860)/10);
    duty_high = (28 - (((voltage-860)*103) >> 10) );

#if 0
    tmp1 = (voltage-860)*103;
    tmp2 = tmp1 >> 10;
    tmp3 = 28 - tmp2;

    printf_arc("### tmp1 = 0x");
    serial_put_hex(tmp1, 16);
    printf_arc(" tmp2 = 0x");
    serial_put_hex(tmp2, 16);
    printf_arc(" tmp3 = 0x");
    serial_put_hex(tmp3, 16);
    printf_arc("\n");

    duty_high_tmp = tmp3;
    printf_arc("##### duty_high_tmp = 0x");
    serial_put_hex(duty_high_tmp, 16);
    printf_arc("\n");
    printf_arc("\n");
#endif

#if 1
    printf_arc("##### duty_high = 0x");
    serial_put_hex(duty_high, 16);
    printf_arc("\n");

#endif
    pwm_duty_cycle_set(duty_high,28);
    return 0;
}

void m8b_pwm_power_off_at_24M(void)
{
    m8b_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_SUSPEND_VOLTAGE);    
}

void m8b_pwm_power_on_at_24M(void)
{
    m8b_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_VOLTAGE);
}
    
void m8b_pwm_power_off_at_32K_1(void)
{
    m8b_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_SUSPEND_VOLTAGE); 
}
    
void m8b_pwm_power_on_at_32K_1(void)
{
    m8b_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_VOLTAGE);
}
#endif 
unsigned int detect_key(unsigned int flags)
{
#ifdef CONFIG_AML1218
    int delay_cnt   = 0;
    int power_status;
    int prev_status;
    int battery_voltage;
    int low_bat_cnt = 0;
#endif
    int ret = FLAG_WAKEUP_PWRKEY;

#ifdef CONFIG_IR_REMOTE_WAKEUP
    //backup the remote config (on arm)
    backup_remote_register();
    //set the ir_remote to 32k mode at ARC
    init_custom_trigger();
#endif

    writel(readl(P_AO_GPIO_O_EN_N)|(1 << 3),P_AO_GPIO_O_EN_N);
    //writel(readl(P_AO_RTI_PULL_UP_REG)|(1 << 3)|(1<<19),P_AO_RTI_PULL_UP_REG);

#ifdef CONFIG_AML1218
    prev_status = aml1218_get_charge_status();
#endif
    do {
        /*
         * when extern power status has changed, we need break
         * suspend loop and resume system.
         */
#ifdef CONFIG_AML1218
        power_status = aml1218_get_charge_status();
        if (power_status ^ prev_status) {
            if (flags == 0x87654321) {      // suspend from uboot
                ret = FLAG_WAKEUP_PWROFF;
            }
            exit_reason = -1;
            break;
        }
        if (power_status ^ prev_status) {
            exit_reason = 1;
            break;
        }
        delay_cnt++;

    #if defined(CONFIG_ENABLE_PMU_WATCHDOG) || defined(CONFIG_RESET_TO_SYSTEM)
        //pmu_feed_watchdog(flags);
    #endif
        if (delay_cnt >= 3000) {
            if (flags != 0x87654321 && !power_status) {
                /*
                 * when battery voltage is too low but no power supply and suspended from kernel,
                 * we need to break suspend loop to resume system, then system will shut down
                 */
                battery_voltage = aml1218_get_battery_voltage();
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
            }
            delay_cnt = 0;
        }
#endif

#ifdef CONFIG_IR_REMOTE_WAKEUP
        if(readl(P_AO_RTI_STATUS_REG2) == 0x4853ffff){
            break;
        }
        if(remote_detect_key()){
            exit_reason = 6;
            break;
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

    } while (!(readl(0xc8100088) & (1<<8)));            // power key

    writel(1<<8,0xc810008c);
    writel(gpio_sel0, 0xc8100084);
    writel(gpio_mask,0xc8100080);

#ifdef CONFIG_IR_REMOTE_WAKEUP
    resume_remote_register();
#endif
    return ret;
}

void arc_pwr_register(struct arc_pwr_op *pwr_op)
{
 #ifdef CONFIG_AML1218
 	pwr_op->power_off_at_24M    = aml1218_power_off_at_24M;
	pwr_op->power_on_at_24M     = aml1218_power_on_at_24M;
	pwr_op->power_off_at_32K_1  = aml1218_power_off_at_32K_1;
	pwr_op->power_on_at_32K_1   = aml1218_power_on_at_32K_1;
	pwr_op->power_off_at_32K_2  = aml1218_power_off_at_32K_2;
	pwr_op->power_on_at_32K_2   = aml1218_power_on_at_32K_2;
	pwr_op->power_off_ddr15     = 0;//aml1218_power_off_ddr15;
	pwr_op->power_on_ddr15      = 0;//aml1218_power_on_ddr15;
	pwr_op->shut_down           = aml1218_shut_down;
#endif
 #ifdef CONFIG_PWM_VDDEE_VOLTAGE
    pwr_op->power_off_at_24M    = m8b_pwm_power_off_at_24M;
    pwr_op->power_on_at_24M     = m8b_pwm_power_on_at_24M;
    pwr_op->power_off_at_32K_1  = 0;//m8b_pwm_power_off_at_32K_1;
    pwr_op->power_on_at_32K_1   = 0;//m8b_pwm_power_on_at_32K_1;
#endif
	pwr_op->detect_key			= detect_key;

}


