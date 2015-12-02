#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/uart.h>
#include <amlogic/aml_pmu_common.h>

extern void hard_i2c_init(void);
extern unsigned char hard_i2c_read8(unsigned char SlaveAddr, unsigned char RegAddr);
extern void hard_i2c_write8(unsigned char SlaveAddr, unsigned char RegAddr, unsigned char Data);
extern unsigned char hard_i2c_read168(unsigned char SlaveAddr, unsigned short RegAddr);
extern void hard_i2c_write168(unsigned char SlaveAddr, unsigned short RegAddr, unsigned char Data); 

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef CONFIG_VDDAO_VOLTAGE
#define CONFIG_VDDAO_VOLTAGE 1200
#endif

#ifdef CONFIG_AW_AXP20
    #define DEVID 0x68
    #define PMU_NAME    "axp202"
#elif defined CONFIG_PMU_ACT8942
    #define DEVID 0xb6
    #define PMU_NAME    "act8942"
#elif defined CONFIG_AML_PMU
    #define DEVID 0x6A
    #define PMU_NAME    "aml1212"
#elif defined CONFIG_RN5T618
    #define DEVID 0x64
    #define PMU_NAME    "rn5t618"
#elif defined CONFIG_AML1216
    #define DEVID 0x6A
    #define PMU_NAME    "aml1216"
#elif defined CONFIG_AML1218
    #define DEVID 0x6A
    #define PMU_NAME    "aml1218"
#else
    #warning no PMU device is selected
    #define PMU_NAME    "NONE"
#endif

static char format_buf[12] = {};
static char *format_dec_value(uint32_t val)
{
    uint32_t tmp, i = 11;

    memset(format_buf, 0, 12);
    while (val) {
        tmp = val % 10; 
        val /= 10;
        format_buf[--i] = tmp + '0';
    }
    return format_buf + i;
}

static void print_voltage_info(char *voltage_prefix, int voltage_idx, uint32_t voltage, uint32_t reg_from, uint32_t reg_to, uint32_t addr)
{
    serial_puts(voltage_prefix);
    if (voltage_idx >= 0) {
        serial_put_hex(voltage_idx, 8);
    }
    serial_puts(" v:");
    serial_puts(format_dec_value(voltage));
    serial_puts(", 0x[");
    serial_put_hex(addr, 8);
    serial_puts("]: ");
    serial_put_hex(reg_from, 8);
    serial_puts(" -> ");
    serial_put_hex(reg_to, 8);
    serial_puts("\n");
}

#define PWM_PRE_DIV 0
struct vcck_pwm {
    int          vcck_voltage;
    unsigned int pwm_value;
};

static struct vcck_pwm vcck_pwm_table[] = {
    {1401, 0x0000001c},  
    {1384, 0x0001001b},  
    {1367, 0x0002001a},  
    {1350, 0x00030019},  
    {1333, 0x00040018},  
    {1316, 0x00050017},  
    {1299, 0x00060016},  
    {1282, 0x00070015},  
    {1265, 0x00080014},  
    {1248, 0x00090013},  
    {1232, 0x000a0012},  
    {1215, 0x000b0011},  
    {1198, 0x000c0010},  
    {1181, 0x000d000f},  
    {1164, 0x000e000e},  
    {1147, 0x000f000d},  
    {1130, 0x0010000c},  
    {1113, 0x0011000b},  
    {1096, 0x0012000a},  
    {1079, 0x00130009},  
    {1062, 0x00140008},  
    {1045, 0x00150007},  
    {1028, 0x00160006},  
    {1011, 0x00170005},  
    { 994, 0x00180004},  
    { 977, 0x00190003},  
    { 960, 0x001a0002},  
    { 943, 0x001b0001},  
    { 926, 0x001c0000}
};

static int vcck_set_default_voltage(int voltage)
{
    int i = 0;
    unsigned int misc_cd;

    // set PWM_C pinmux 
    setbits_le32(P_PERIPHS_PIN_MUX_2, (1 << 2));
    clrbits_le32(P_PERIPHS_PIN_MUX_1, (1 << 29));
    misc_cd = readl(P_PWM_MISC_REG_CD);

    if (voltage > vcck_pwm_table[0].vcck_voltage) {
        serial_puts("vcck voltage too large:");
        serial_puts(format_dec_value(voltage));
        return -1;    
    }
    for (i = 0; i < ARRAY_SIZE(vcck_pwm_table) - 1; i++) {
        if (vcck_pwm_table[i].vcck_voltage     >= voltage && 
            vcck_pwm_table[i + 1].vcck_voltage <  voltage) {
            break;    
        }
    }
    serial_puts("set vcck voltage to "); 
    serial_puts(format_dec_value(voltage));
    serial_puts(", pwm_value from 0x");
    serial_put_hex(readl(P_PWM_PWM_C), 32);
    serial_puts(" to 0x");
    serial_put_hex(vcck_pwm_table[i].pwm_value, 32);
    serial_puts("\n");
    writel(vcck_pwm_table[i].pwm_value, P_PWM_PWM_C);
    writel(((misc_cd & ~(0x7f << 8)) | ((1 << 15) | (PWM_PRE_DIV << 8) | (1 << 0))), P_PWM_MISC_REG_CD);
    return 0;
}

#ifdef CONFIG_AW_AXP20
void axp20_set_bits(uint32_t add, uint8_t bits, uint8_t mask)
{
    uint8_t val;
    val = hard_i2c_read8(DEVID, add);
    val &= ~(mask);                                         // clear bits;
    val |=  (bits & mask);                                  // set bits;
    hard_i2c_write8(DEVID, add, val);
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

int axp20_set_dcdc_voltage(int dcdc, int voltage)
{
    int addr, size, val;
    int idx_to, idx_cur, mask;

    addr = (dcdc == 2 ? 0x23 : 0x27);
    size = (dcdc == 2 ? 64 : 128);
    mask = (dcdc == 2 ? 0x3f : 0x7f);
    idx_to = find_idx(700, voltage, 25, size);                  // step is 25mV
    idx_cur = hard_i2c_read8(DEVID, addr);
    print_voltage_info("DC", dcdc, voltage, idx_cur, idx_to, addr);
    val = idx_cur;
    while (idx_cur != idx_to) {
        if (idx_cur < idx_to) {                                 // adjust to target voltage step by step
            idx_cur++;    
        } else {
            idx_cur--;
        }
        val &= ~mask;
        val |= idx_cur; 
        hard_i2c_write8(DEVID, addr, val);
        __udelay(100);                                          // atleast delay 100uS
    }
    __udelay(1 * 1000);
}

static int find_ldo4(int voltage) 
{
    int table[] = {
        1250, 1300, 1400, 1500, 1600, 1700, 1800, 1900,
        2000, 2500, 2700, 2800, 3000, 3100, 3200, 3300
    };
    int i = 0;

    for (i = 0; i < 16; i++) {
        if (table[i] >= voltage) {
            return i;    
        }    
    }
    return i;
}

static void axp20_ldo_voltage(int ldo, int voltage)
{
    int addr, size, start, step;
    int idx_to, idx_cur, mask;
    int shift;

    switch (ldo) {
    case 2:
        addr  = 0x28; 
        size  = 16;
        step  = 100;
        mask  = 0xf0;
        start = 1800;
        shift = 4;
        break;
    case 3:
        addr  = 0x29;
        size  = 128;
        step  = 25;
        mask  = 0x7f;
        start = 700;
        shift = 0;
        break;
    case 4:
        addr  = 0x28;
        mask  = 0x0f;
        shift = 0;
        break;
    }
    if (ldo != 4) {
        idx_to = find_idx(start, voltage, step, size);
    } else {
        idx_to = find_ldo4(voltage); 
    }
    idx_cur = hard_i2c_read8(DEVID, addr);
    print_voltage_info("LDO", ldo, voltage, idx_cur, idx_to, addr);
    idx_cur &= ~mask;
    idx_cur |= (idx_to << shift);
    hard_i2c_write8(DEVID, addr, idx_cur);
}

static void axp20_power_init(int init_mode)
{
    hard_i2c_write8(DEVID, 0x80, 0xe4);
#ifdef CONFIG_VCCK_VOLTAGE
    vcck_set_default_voltage(CONFIG_VCCK_VOLTAGE);
#endif

#ifdef CONFIG_DISABLE_LDO3_UNDER_VOLTAGE_PROTECT
    axp20_set_bits(0x81, 0x00, 0x04); 
#endif
#ifdef CONFIG_VDDAO_VOLTAGE
    axp20_set_dcdc_voltage(3, CONFIG_VDDAO_VOLTAGE);
#endif
#ifdef CONFIG_VDDIO_AO 
    axp20_ldo_voltage(2, CONFIG_VDDIO_AO);
#endif
#ifdef CONFIG_AVDD2V5 
    axp20_ldo_voltage(3, CONFIG_AVDD2V5);
#endif
#ifdef CONFIG_AVDD3V3 
    axp20_ldo_voltage(4, CONFIG_AVDD3V3);
#endif
#ifdef CONFIG_CONST_PWM_FOR_DCDC
    axp20_set_bits(0x80, 0x06, 0x06);
#endif
#ifdef CONFIG_DDR_VOLTAGE
    /*
     * must use direct register write...
     */
    hard_i2c_write8(DEVID, 0x23, (CONFIG_DDR_VOLTAGE - 700) / 25);
#endif
}
#endif

#ifdef CONFIG_AML_PMU
#define AML_PMU_DCDC1       1
#define AML_PMU_DCDC2       2
static unsigned int dcdc1_voltage_table[] = {                   // voltage table of DCDC1
    2000, 1980, 1960, 1940, 1920, 1900, 1880, 1860, 
    1840, 1820, 1800, 1780, 1760, 1740, 1720, 1700, 
    1680, 1660, 1640, 1620, 1600, 1580, 1560, 1540, 
    1520, 1500, 1480, 1460, 1440, 1420, 1400, 1380, 
    1360, 1340, 1320, 1300, 1280, 1260, 1240, 1220, 
    1200, 1180, 1160, 1140, 1120, 1100, 1080, 1060, 
    1040, 1020, 1000,  980,  960,  940,  920,  900,  
     880,  860,  840,  820,  800,  780,  760,  740
};

static unsigned int dcdc2_voltage_table[] = {                   // voltage table of DCDC2
    2160, 2140, 2120, 2100, 2080, 2060, 2040, 2020,
    2000, 1980, 1960, 1940, 1920, 1900, 1880, 1860, 
    1840, 1820, 1800, 1780, 1760, 1740, 1720, 1700, 
    1680, 1660, 1640, 1620, 1600, 1580, 1560, 1540, 
    1520, 1500, 1480, 1460, 1440, 1420, 1400, 1380, 
    1360, 1340, 1320, 1300, 1280, 1260, 1240, 1220, 
    1200, 1180, 1160, 1140, 1120, 1100, 1080, 1060, 
    1040, 1020, 1000,  980,  960,  940,  920,  900
};

int find_idx_by_voltage(int voltage, unsigned int *table)
{
    int i;

    /*
     * under this section divide(/ or %) can not be used, may cause exception
     */
    for (i = 0; i < 64; i++) {
        if (voltage >= table[i]) {
            break;    
        }
    }
    if (voltage == table[i]) {
        return i;    
    }
    return i - 1;
}

void aml_pmu_set_voltage(int dcdc, int voltage)
{
    int idx_to = 0xff;
    int idx_cur;
    unsigned char val;
    unsigned char addr;
    unsigned int *table;

    if (dcdc < 0 || dcdc > AML_PMU_DCDC2 || voltage > 2100 || voltage < 840) {
        return ;                                                // current only support DCDC1&2 voltage adjust
    }
    if (dcdc == AML_PMU_DCDC1) {
        addr  = 0x2f; 
        table = dcdc1_voltage_table;
    } else if (dcdc = AML_PMU_DCDC2) {
        addr  = 0x38;
        table = dcdc2_voltage_table;
    }
    val = hard_i2c_read168(DEVID, addr);
    idx_cur = ((val & 0xfc) >> 2);
    idx_to = find_idx_by_voltage(voltage, table);
    print_voltage_info("DC", dcdc, voltage, idx_cur, idx_to, addr);
    while (idx_cur != idx_to) {
        if (idx_cur < idx_to) {                                 // adjust to target voltage step by step
            idx_cur++;    
        } else {
            idx_cur--;
        }
        val &= ~0xfc;
        val |= (idx_cur << 2);
        hard_i2c_write168(DEVID, addr, val);
        __udelay(100);                                          // atleast delay 100uS
    }
}

static void aml1212_power_init(int init_mode)
{
#ifdef CONFIG_VCCK_VOLTAGE
    vcck_set_default_voltage(CONFIG_VCCK_VOLTAGE);
#endif
#ifdef CONFIG_VDDAO_VOLTAGE
    aml_pmu_set_voltage(AML_PMU_DCDC1, CONFIG_VDDAO_VOLTAGE);
#endif
#ifdef CONFIG_DDR_VOLTAGE
    aml_pmu_set_voltage(AML_PMU_DCDC2, CONFIG_DDR_VOLTAGE);
#endif
}
#endif      /* CONFIG_AML_1212  */

#ifdef CONFIG_RN5T618
void rn5t618_set_bits(uint32_t add, uint8_t bits, uint8_t mask)
{
    uint8_t val;
    val = hard_i2c_read8(DEVID, add);
    val &= ~(mask);                                         // clear bits;
    val |=  (bits & mask);                                  // set bits;
    hard_i2c_write8(DEVID, add, val);
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

int rn5t618_set_dcdc_voltage(int dcdc, int voltage)
{
    int addr;
    int idx_to, idx_cur;
    addr = 0x35 + dcdc;
    idx_to = find_idx(6000, voltage * 10, 125, 256);            // step is 12.5mV
    idx_cur = hard_i2c_read8(DEVID, addr);
    print_voltage_info("DC", dcdc, voltage, idx_cur, idx_to, addr);
    hard_i2c_write8(DEVID, addr, idx_to);
    __udelay(5 * 1000);
}

#define LDO_RTC1        10 
#define LDO_RTC2        11
int rn5t618_set_ldo_voltage(int ldo, int voltage)
{
    int addr;
    int idx_to, idx_cur;
    int start = 900;

    switch (ldo) {
    case LDO_RTC1:
        addr  = 0x56;
        start = 1700;
        break;
    case LDO_RTC2:
        addr  = 0x57;
        start = 900;
        break;
    case 1:
    case 2:
    case 4:
    case 5:
        start = 900;
        addr  = 0x4b + ldo;
        break;
    case 3:
        start = 600;
        addr  = 0x4b + ldo;
        break;
    default:
        serial_puts("wrong LDO value\n");
        break;
    }
    idx_to = find_idx(start, voltage, 25, 128);                 // step is 25mV
    idx_cur = hard_i2c_read8(DEVID, addr);
    print_voltage_info("LDO", ldo, voltage, idx_cur, idx_to, addr);
    hard_i2c_write8(DEVID, addr, idx_to);
    __udelay(5 * 100);
}

void rn5t618_power_init(int init_mode)
{
    unsigned char val;

    hard_i2c_read8(DEVID, 0x0b);                                // clear watch dog 
    rn5t618_set_bits(0xB3, 0x40, 0x40);
    rn5t618_set_bits(0xB8, 0x02, 0x1f);                         // set charge current to 300mA

    if (init_mode == POWER_INIT_MODE_NORMAL) {
    #ifdef CONFIG_VCCK_VOLTAGE
        rn5t618_set_dcdc_voltage(1, CONFIG_VCCK_VOLTAGE);       // set cpu voltage
    #endif
    #ifdef CONFIG_VDDAO_VOLTAGE
        rn5t618_set_dcdc_voltage(2, CONFIG_VDDAO_VOLTAGE);      // set VDDAO voltage
    #endif
    } else if (init_mode == POWER_INIT_MODE_USB_BURNING) {
        /*
         * if under usb burning mode, keep VCCK and VDDEE
         * as low as possible for power saving and stable issue
         */
        rn5t618_set_dcdc_voltage(1, 900);                       // set cpu voltage
        rn5t618_set_dcdc_voltage(2, 950);                       // set VDDAO voltage
    }
#ifdef CONFIG_DDR_VOLTAGE
    rn5t618_set_dcdc_voltage(3, CONFIG_DDR_VOLTAGE);            // set DDR voltage
#endif
    val = hard_i2c_read8(DEVID, 0x30);
    val |= 0x01;
    hard_i2c_write8(DEVID, 0x30, val);                          // Enable DCDC3--DDR
#ifdef CONFIG_VDDIO_AO28
    rn5t618_set_ldo_voltage(1, CONFIG_VDDIO_AO28);              // VDDIO_AO28
#endif
#ifdef CONFIG_VDDIO_AO18
    rn5t618_set_ldo_voltage(2, CONFIG_VDDIO_AO18);              // VDDIO_AO18
#endif
#ifdef CONFIG_VCC1V8
    rn5t618_set_ldo_voltage(3, CONFIG_VCC1V8);                  // VCC1.8V 
#endif
#ifdef CONFIG_VCC2V8
    rn5t618_set_ldo_voltage(4, CONFIG_VCC2V8);                  // VCC2.8V 
#endif
#ifdef CONFIG_AVDD1V8
    rn5t618_set_ldo_voltage(5, CONFIG_AVDD1V8);                 // AVDD1.8V 
#endif
#ifdef CONFIG_VDD_LDO
    rn5t618_set_ldo_voltage(LDO_RTC1, CONFIG_VDD_LDO);          // VDD_LDO
#endif
#ifdef CONFIG_RTC_0V9
    rn5t618_set_ldo_voltage(LDO_RTC2, CONFIG_RTC_0V9);          // RTC_0V9 
#endif
}
#endif

#ifdef CONFIG_AML1216

#define AML1216_DCDC1                1
#define AML1216_DCDC2                2
#define AML1216_DCDC3                3
#define AML1216_BOOST                4

#define AML1216_LDO1                 5
#define AML1216_LDO2                 6
#define AML1216_LDO3                 7
#define AML1216_LDO4                 8
#define AML1216_LDO5                 9

static unsigned int VDDEE_voltage_table[] = {                   // voltage table of VDDEE
    1184, 1170, 1156, 1142, 1128, 1114, 1100, 1086, 
    1073, 1059, 1045, 1031, 1017, 1003, 989, 975, 
    961,  947,  934,  920,  906,  892,  878, 864, 
    850,  836,  822,  808,  794,  781,  767, 753 
};

int find_idx_by_vddEE_voltage(int voltage, unsigned int *table)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (voltage >= table[i]) {
            break;    
        }
    }
    if (voltage == table[i]) {
        return i;    
    }
    return i - 1;
}

int aml1216_set_vddEE_voltage(int voltage)
{
    int addr = 0x005d;
    int idx_to, idx_cur;
    unsigned current;
    unsigned char val;
 
    val = hard_i2c_read168(DEVID, addr);
    idx_cur = ((val & 0xfc) >> 2);
    idx_to = find_idx_by_vddEE_voltage(voltage, VDDEE_voltage_table);

    current = idx_to*5; 

    print_voltage_info("EE", -1, voltage, idx_cur, idx_to, addr);
    
    val &= ~0xfc;
    val |= (idx_to << 2);

    hard_i2c_write168(DEVID, addr, val);
    __udelay(5 * 100);
}

void aml1216_set_bits(uint32_t add, uint8_t bits, uint8_t mask)
{
    uint8_t val;
    val = hard_i2c_read168(DEVID, add);
    val &= ~(mask);                                         // clear bits;
    val |=  (bits & mask);                                  // set bits;
    hard_i2c_write168(DEVID, add, val);
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

void aml1216_set_pfm(int dcdc, int en)
{
    unsigned char val;
    unsigned char bit, addr;

    if (dcdc < 1 || dcdc > AML1216_DCDC3 || en > 1 || en < 0) {
        return ;    
    }
    switch(dcdc) {
    case AML1216_DCDC1:
        addr = 0x3b;
        bit  = 5;
        break;

    case AML1216_DCDC2:
        addr = 0x44;
        bit  = 5;
        break;

    case AML1216_DCDC3:
        addr = 0x4e;
        bit  = 7;
        break;

    case AML1216_BOOST:
        addr = 0x28;
        bit  = 0;
        break;

    default:
        break;
    }
    if (en) {
        val = 1 << bit;
    } else {
        val = 0 << bit;    
    }
    aml1216_set_bits(addr, val, (1 << bit));
    __udelay(100);
}

int aml1216_set_dcdc_voltage(int dcdc, int voltage)
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
    idx_cur  = hard_i2c_read168(DEVID, addr);
    idx_to   = find_idx(start, voltage, step, range);
    print_voltage_info("DC", dcdc, voltage, idx_cur, idx_to << 2, addr);
    val = idx_cur;
    idx_cur >>= 2;
    while (idx_cur != idx_to) {
        if (idx_cur < idx_to) {                                 // adjust to target voltage step by step
            idx_cur++;    
        } else {
            idx_cur--;
        }
        val &= ~0xfc;
        val |= (idx_cur << 2);
        hard_i2c_write168(DEVID, addr, val);
        __udelay(100);                                          // atleast delay 100uS
    }
    __udelay(100);                         // wait a moment
    return 0;
}

static int ldo2_voltage[] = {
    3330, 3290, 3240, 3190,  3140, 3090, 3040, 2990, 
    2940, 2890, 2840, 2790,  2740, 2690, 2640, 2600, 
    2560, 2510, 2460, 2410,  2360, 2310, 2260, 2210, 
    2160, 2110, 2060, 2010,  1960, 1910, 1860, 1820, 
    2200, 2160, 2110, 2060,  2010, 1960, 1910, 1860, 
    1810, 1760, 1710, 1660,  1610, 1560, 1510, 1460, 
    1420, 1380, 1330, 1280,  1230, 1180, 1130, 1080,
    1030,  980,  930,  880,   830,  780,  730,  680
};

static int ldo3_voltage[] = {
    3640, 3600, 3550, 3500,  3450, 3400, 3350, 3300, 
    3250, 3200, 3150, 3100,  3050, 3000, 2950, 2900, 
    2800, 2760, 2710, 2660,  2610, 2560, 2510, 2460, 
    2410, 2360, 2310, 2260,  2210, 2160, 2110, 2070, 
    2250, 2220, 2170, 2120,  2070, 2020, 1970, 1920, 
    1870, 1820, 1770, 1720,  1670, 1620, 1570, 1530, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720, 680
};

static int ldo4_voltage[] = {
    3640, 3600, 3550, 3500,  3450, 3400, 3350, 3300,
    3250, 3200, 3150, 3100,  3050, 3000, 2950, 2900, 
    2800, 2760, 2710, 2660,  2610, 2560, 2510, 2460, 
    2410, 2360, 2310, 2260,  2210, 2160, 2110, 2070, 
    2250, 2220, 2170, 2120,  2070, 2020, 1970, 1920, 
    1870, 1820, 1770, 1720,  1670, 1620, 1570, 1530, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720,  680
};

static int ldo5_voltage[] = {
    3320, 3270, 3220, 3170,  3120, 3070, 3020, 2970, 
    2920, 2870, 2820, 2770,  2720, 2670, 2620, 2570, 
    2530, 2500, 2450, 2400,  2350, 2300, 2250, 2200, 
    2150, 2100, 2050, 2000,  1950, 1900, 1850, 1810, 
    2200, 2140, 2090, 2040,  1990, 1940, 1890, 1840, 
    1790, 1740, 1690, 1640,  1590, 1540, 1490, 1440, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720,  680
};

static int ldo6_voltage[] = {
    3410, 3380, 3330, 3280,  3230, 3180, 3130, 3080, 
    3030, 2980, 2940, 2900,  2860, 2820, 2780, 2700, 
    2600, 2550, 2490, 2450,  2410, 2370, 2330, 2290, 
    2250, 2210, 2170, 2130,  2090, 2050, 2010, 1950, 
    2010, 1970, 1930, 1890,  1850, 1810, 1770, 1730, 
    1690, 1650, 1610, 1570,  1530, 1500, 1460, 1420, 
    1260, 1220, 1180, 1140,  1100, 1060, 1020,  980, 
     940,  900,  860,  820,   780,  740,  700,  660
};

static int ldo7_voltage[] = {
    3930, 3870, 3820, 3770,  3720, 3660, 3610, 3570, 
    3620, 3570, 3520, 3470,  3420, 3370, 3320, 3270, 
    3010, 2960, 2910, 2860,  2810, 2760, 2710, 2660, 
    2610, 2560, 2510, 2460,  2410, 2360, 2310, 2260, 
    2300, 2250, 2200, 2150,  2100, 2050, 2000, 1950, 
    1990, 1940, 1890, 1840,  1790, 1740, 1690, 1640, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720,  680
};

struct ldo_attr {
    int ldo;
    int addr;
    int mask;
    int size;
    int *voltage_table;
};

static struct ldo_attr aml1216_ldo_attr[] = {
    {
        .ldo  = 2,
        .addr = 0x62,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo2_voltage,
    }, 
    {
        .ldo  = 3,
        .addr = 0x65,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo3_voltage,
    }, 
    {
        .ldo  = 4,
        .addr = 0x68,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo4_voltage,
    }, 
    {
        .ldo  = 5,
        .addr = 0x6b,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo5_voltage,
    }, 
    {
        .ldo  = 6,
        .addr = 0x6e,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo6_voltage,
    }, 
    {
        .ldo  = 7,
        .addr = 0x71,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo7_voltage,
    }, 
};

int aml1216_set_ldo_voltage(int ldo, int voltage)
{
    int idx_cur, idx_to, i, j;
    struct ldo_attr *attr = aml1216_ldo_attr;

    for (i = 0; i < ARRAY_SIZE(aml1216_ldo_attr); i++) {
        if (attr[i].ldo == ldo) {
            break;
        } 
    }
    if (i >= ARRAY_SIZE(aml1216_ldo_attr)) {
        serial_puts("Wrong LDO value:");
        serial_put_hex(ldo, 8);
        serial_puts("\n");
        return -1;
    }

    for (j = 0; j < attr[i].size - 1; j++) {
        if (attr[i].voltage_table[j] >= voltage && attr[i].voltage_table[j + 1] <= voltage) {
            break;    
        }
    }
    if (j >= attr[i].size - 1) {
        serial_puts("Wrong voltage value:");
        serial_put_hex(voltage, 32);
        serial_puts("\n");
        return -1;
    }

    idx_to = j;
    idx_cur = hard_i2c_read168(DEVID, attr[i].addr);
    
    print_voltage_info("LDO", ldo, voltage, idx_cur, idx_to, attr[i].addr);
    aml1216_set_bits(attr[i].addr, (uint8_t)idx_to, attr[i].mask);
    __udelay(5 * 100);
}

void aml1216_check_vbat(int init)
{
    unsigned char val1, val2, val3;

    if (init) {
        val1 = hard_i2c_read168(DEVID, 0x0087);
        val2 = hard_i2c_read168(DEVID, 0x0088);
        val3 = hard_i2c_read168(DEVID, 0x0089);
        serial_puts("-- fault status: ");
        serial_put_hex(val1, 8);
        serial_put_hex(val2, 8);
        serial_put_hex(val3, 8);
        serial_puts("\n");
        __udelay(10 * 1000);
        hard_i2c_write168(DEVID, 0x009B, 0x0c);//enable auto_sample and accumulate IBAT measurement
        hard_i2c_write168(DEVID, 0x009C, 0x10);
        hard_i2c_write168(DEVID, 0x009D, 0x04);//close force charge and discharge sample mask
        hard_i2c_write168(DEVID, 0x009E, 0x08);//enable VBAT measure result average 4 samples
        hard_i2c_write168(DEVID, 0x009F, 0x20);//enable IBAT measure result average 4 samples
        hard_i2c_write168(DEVID, 0x009A, 0x20);
        hard_i2c_write168(DEVID, 0x00B8, 0x00);
        hard_i2c_write168(DEVID, 0x00A9, 0x8f);

        hard_i2c_write168(DEVID, 0x00A0, 0x01);//select auto-sampling timebase is 2ms
        hard_i2c_write168(DEVID, 0x00A1, 0x15);//set the IBAT measure threshold and enable auto IBAT +VBAT_in_active sample
        hard_i2c_write168(DEVID, 0x00C9, 0x06);// open DCIN_OK and USB_OK IRQ
    }
    __udelay(80000);
    val1 = hard_i2c_read168(DEVID, 0x00af);
    val2 = hard_i2c_read168(DEVID, 0x00b0);
    serial_puts("\n-- vbat: 0x");
    serial_put_hex(val2, 8);
    serial_put_hex(val1, 8);
    serial_puts("\n");
}

void aml1216_power_init(int init_mode)
{
    aml1216_set_bits(0x001b, 0x06, 0x06);                           // Enable DCDC1 fault
//    aml1216_set_bits(0x004f, 0x08, 0x08);                           // David Wang, DCDC limit
    aml1216_set_bits(0x001c, 0x06, 0x06);
    aml1216_set_bits(0x0045, 0x08, 0x08);
    aml1216_set_bits(0x003c, 0x08, 0x08);
    aml1216_set_bits(0x0121, 0x04, 0x04);
    aml1216_set_bits(0x011f, 0x04, 0x04);
    aml1216_set_bits(0x011d, 0x04, 0x04);
    aml1216_set_bits(0x003c, 0x00, 0x02);                           // open LDO8 for RTC power

    aml1216_check_vbat(1);
    if (init_mode == POWER_INIT_MODE_NORMAL) {
#ifdef CONFIG_VCCK_VOLTAGE
        aml1216_set_dcdc_voltage(1, CONFIG_VCCK_VOLTAGE);           // set cpu voltage
        __udelay(2000);
#endif

#ifdef CONFIG_DDR_VOLTAGE
        aml1216_set_dcdc_voltage(2, CONFIG_DDR_VOLTAGE);
        __udelay(2000);              
        hard_i2c_write168(DEVID, 0x0082, 0x04);                     // open DCDC2 
        __udelay(2000);              
#endif

#ifdef CONFIG_VDDAO_VOLTAGE
        aml1216_set_vddEE_voltage(CONFIG_VDDAO_VOLTAGE); 
        __udelay(2000);
#endif

#ifdef CONFIG_VCC3V3
        //aml1216_set_dcdc_voltage(3, CONFIG_VCC3V3);
        __udelay(2000);
#endif

#ifdef CONFIG_IOREF_1V8
        aml1216_set_ldo_voltage(2, CONFIG_IOREF_1V8);
        __udelay(2000);
#endif

#ifdef CONFIG_VDDIO_AO18 
        aml1216_set_ldo_voltage(3, CONFIG_VDDIO_AO18);
        __udelay(2000);
#endif

#ifdef CONFIG_VCC1V8
        aml1216_set_ldo_voltage(4, CONFIG_VCC1V8);
        __udelay(2000);
#endif

#ifdef CONFIG_VCC2V8
        aml1216_set_ldo_voltage(5, CONFIG_VCC2V8);
        __udelay(2000);
#endif

#ifdef CONFIG_VCC_CAM
        aml1216_set_ldo_voltage(6, CONFIG_VCC_CAM);
        hard_i2c_write168(DEVID, 0x83, 0x01);                           // open LDO6
#endif

#ifdef CONFIG_VDDIO_AO28
        aml1216_set_ldo_voltage(7, CONFIG_VDDIO_AO28);
        __udelay(2000);
#endif
    } else if (init_mode == POWER_INIT_MODE_USB_BURNING) {
        /*
         * if under usb burning mode, keep VCCK and VDDEE
         * as low as possible for power saving and stable issue
         */
        aml1216_set_dcdc_voltage(1, 900);                       // set cpu voltage                      
        aml1216_set_vddEE_voltage(950);                         // set VDDEE voltage
    }
    aml1216_set_pfm(1, 0);                                      // DC1 ~ 3 to fix PWM
    aml1216_set_pfm(2, 0);
    aml1216_set_pfm(3, 0);
    aml1216_check_vbat(0);
}
#endif

#ifdef CONFIG_AML1218

#define AML1218_DCDC1                1
#define AML1218_DCDC2                2
#define AML1218_DCDC3                3
#define AML1218_BOOST                4

#define AML1218_LDO1                 5
#define AML1218_LDO2                 6
#define AML1218_LDO3                 7
#define AML1218_LDO4                 8
#define AML1218_LDO5                 9

static unsigned int VDDEE_voltage_table[] = {                   // voltage table of VDDEE
    1184, 1170, 1156, 1142, 1128, 1114, 1100, 1086, 
    1073, 1059, 1045, 1031, 1017, 1003, 989, 975, 
    961,  947,  934,  920,  906,  892,  878, 864, 
    850,  836,  822,  808,  794,  781,  767, 753 
};

int find_idx_by_vddEE_voltage(int voltage, unsigned int *table)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (voltage >= table[i]) {
            break;    
        }
    }
    if (voltage == table[i]) {
        return i;    
    }
    return i - 1;
}

int aml1218_set_vddEE_voltage(int voltage)
{
    int addr = 0x005d;
    int idx_to, idx_cur;
    unsigned current;
    unsigned char val;
 
    val = hard_i2c_read168(DEVID, addr);
    idx_cur = ((val & 0x7c) >> 2);
    idx_to = find_idx_by_vddEE_voltage(voltage, VDDEE_voltage_table);

    current = idx_to*5; 

    print_voltage_info("VCCK", -1, voltage, idx_cur, idx_to, addr);
    
    val &= ~0x7c;
    val |= (idx_to << 2);

    hard_i2c_write168(DEVID, addr, val);
    __udelay(5 * 100);
}

void aml1218_set_bits(uint32_t add, uint8_t bits, uint8_t mask)
{
    uint8_t val;
    val = hard_i2c_read168(DEVID, add);
    val &= ~(mask);                                         // clear bits;
    val |=  (bits & mask);                                  // set bits;
    hard_i2c_write168(DEVID, add, val);
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

void aml1218_set_pfm(int dcdc, int en)
{
    unsigned char val;
    unsigned char bit, addr;

    if (dcdc < 1 || dcdc > AML1218_DCDC3 || en > 1 || en < 0) {
        return ;    
    }
    switch(dcdc) {
    case AML1218_DCDC1:
        addr = 0x3b;
        bit  = 5;
        break;

    case AML1218_DCDC2:
        addr = 0x44;
        bit  = 5;
        break;

    case AML1218_DCDC3:
        addr = 0x4e;
        bit  = 7;
        break;

    case AML1218_BOOST:
        addr = 0x28;
        bit  = 0;
        break;

    default:
        break;
    }
    if (en) {
        val = 1 << bit;
    } else {
        val = 0 << bit;    
    }
    aml1218_set_bits(addr, val, (1 << bit));
    __udelay(100);
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
    idx_cur  = hard_i2c_read168(DEVID, addr);
    idx_to   = find_idx(start, voltage, step, range);
    if (dcdc == 1) {
        print_voltage_info("EE", dcdc, voltage, idx_cur, idx_to << 1, addr);
    } else {
        print_voltage_info("DC", dcdc, voltage, idx_cur, idx_to << 1, addr);
    }
    val = idx_cur;
    idx_cur = (idx_cur & 0x7e) >> 1;

    step = idx_cur - idx_to;
    if (step < 0) {
        step = -step;    
    }
    val &= ~0x7e;
    val |= (idx_to << 1);
    hard_i2c_write168(DEVID, addr, val);
    __udelay(20 * step);

    return 0;
}

static int ldo2_voltage[] = {
    3330, 3290, 3240, 3190,  3140, 3090, 3040, 2990, 
    2940, 2890, 2840, 2790,  2740, 2690, 2640, 2600, 
    2560, 2510, 2460, 2410,  2360, 2310, 2260, 2210, 
    2160, 2110, 2060, 2010,  1960, 1910, 1860, 1820, 
    2200, 2160, 2110, 2060,  2010, 1960, 1910, 1860, 
    1810, 1760, 1710, 1660,  1610, 1560, 1510, 1460, 
    1420, 1380, 1330, 1280,  1230, 1180, 1130, 1080,
    1030,  980,  930,  880,   830,  780,  730,  680
};

static int ldo3_voltage[] = {
    3640, 3600, 3550, 3500,  3450, 3400, 3350, 3300, 
    3250, 3200, 3150, 3100,  3050, 3000, 2950, 2900, 
    2800, 2760, 2710, 2660,  2610, 2560, 2510, 2460, 
    2410, 2360, 2310, 2260,  2210, 2160, 2110, 2070, 
    2250, 2220, 2170, 2120,  2070, 2020, 1970, 1920, 
    1870, 1820, 1770, 1720,  1670, 1620, 1570, 1530, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720, 680
};

static int ldo4_voltage[] = {
    3640, 3600, 3550, 3500,  3450, 3400, 3350, 3300,
    3250, 3200, 3150, 3100,  3050, 3000, 2950, 2900, 
    2800, 2760, 2710, 2660,  2610, 2560, 2510, 2460, 
    2410, 2360, 2310, 2260,  2210, 2160, 2110, 2070, 
    2250, 2220, 2170, 2120,  2070, 2020, 1970, 1920, 
    1870, 1820, 1770, 1720,  1670, 1620, 1570, 1530, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720,  680
};

static int ldo5_voltage[] = {
    3320, 3270, 3220, 3170,  3120, 3070, 3020, 2970, 
    2920, 2870, 2820, 2770,  2720, 2670, 2620, 2570, 
    2530, 2500, 2450, 2400,  2350, 2300, 2250, 2200, 
    2150, 2100, 2050, 2000,  1950, 1900, 1850, 1810, 
    2200, 2140, 2090, 2040,  1990, 1940, 1890, 1840, 
    1790, 1740, 1690, 1640,  1590, 1540, 1490, 1440, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720,  680
};

static int ldo6_voltage[] = {
    3410, 3380, 3330, 3280,  3230, 3180, 3130, 3080, 
    3030, 2980, 2940, 2900,  2860, 2820, 2780, 2700, 
    2600, 2550, 2490, 2450,  2410, 2370, 2330, 2290, 
    2250, 2210, 2170, 2130,  2090, 2050, 2010, 1950, 
    2010, 1970, 1930, 1890,  1850, 1810, 1770, 1730, 
    1690, 1650, 1610, 1570,  1530, 1500, 1460, 1420, 
    1260, 1220, 1180, 1140,  1100, 1060, 1020,  980, 
     940,  900,  860,  820,   780,  740,  700,  660
};

static int ldo7_voltage[] = {
    3930, 3870, 3820, 3770,  3720, 3660, 3610, 3570, 
    3620, 3570, 3520, 3470,  3420, 3370, 3320, 3270, 
    3010, 2960, 2910, 2860,  2810, 2760, 2710, 2660, 
    2610, 2560, 2510, 2460,  2410, 2360, 2310, 2260, 
    2300, 2250, 2200, 2150,  2100, 2050, 2000, 1950, 
    1990, 1940, 1890, 1840,  1790, 1740, 1690, 1640, 
    1410, 1370, 1320, 1270,  1220, 1170, 1120, 1070, 
    1020,  970,  920,  870,   820,  770,  720,  680
};

struct ldo_attr {
    int ldo;
    int addr;
    int mask;
    int size;
    int *voltage_table;
};

static struct ldo_attr aml1218_ldo_attr[] = {
    {
        .ldo  = 2,
        .addr = 0x62,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo2_voltage,
    }, 
    {
        .ldo  = 3,
        .addr = 0x65,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo3_voltage,
    }, 
    {
        .ldo  = 4,
        .addr = 0x68,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo4_voltage,
    }, 
    {
        .ldo  = 5,
        .addr = 0x6b,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo5_voltage,
    }, 
    {
        .ldo  = 6,
        .addr = 0x6e,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo6_voltage,
    }, 
    {
        .ldo  = 7,
        .addr = 0x71,
        .mask = 0x3f,
        .size = 64,
        .voltage_table = ldo7_voltage,
    }, 
};

int aml1218_set_ldo_voltage(int ldo, int voltage)
{
    int idx_cur, idx_to, i, j;
    struct ldo_attr *attr = aml1218_ldo_attr;

    for (i = 0; i < ARRAY_SIZE(aml1218_ldo_attr); i++) {
        if (attr[i].ldo == ldo) {
            break;
        } 
    }
    if (i >= ARRAY_SIZE(aml1218_ldo_attr)) {
        serial_puts("Wrong LDO value:");
        serial_put_hex(ldo, 8);
        serial_puts("\n");
        return -1;
    }

    for (j = 0; j < attr[i].size - 1; j++) {
        if (attr[i].voltage_table[j] >= voltage && attr[i].voltage_table[j + 1] <= voltage) {
            break;    
        }
    }
    if (j >= attr[i].size - 1) {
        serial_puts("Wrong voltage value:");
        serial_put_hex(voltage, 32);
        serial_puts("\n");
        return -1;
    }

    idx_to = j;
    idx_cur = hard_i2c_read168(DEVID, attr[i].addr);
    
    print_voltage_info("LDO", ldo, voltage, idx_cur, idx_to, attr[i].addr);
    aml1218_set_bits(attr[i].addr, (uint8_t)idx_to, attr[i].mask);
    __udelay(5 * 100);
}

void aml1218_check_vbat(int init)
{
    unsigned char val1, val2, val3;

    if (init) {
        val1 = hard_i2c_read168(DEVID, 0x0087);
        val2 = hard_i2c_read168(DEVID, 0x0088);
        val3 = hard_i2c_read168(DEVID, 0x0089);
        serial_puts("-- fault status: ");
        serial_put_hex(val1, 8);
        serial_put_hex(val2, 8);
        serial_put_hex(val3, 8);
        serial_puts("\n");
        __udelay(10 * 1000);
        hard_i2c_write168(DEVID, 0x009B, 0x0c);//enable auto_sample and accumulate IBAT measurement
        hard_i2c_write168(DEVID, 0x009C, 0x10);
        hard_i2c_write168(DEVID, 0x009D, 0x04);//close force charge and discharge sample mask
        hard_i2c_write168(DEVID, 0x009E, 0x08);//enable VBAT measure result average 4 samples
        hard_i2c_write168(DEVID, 0x009F, 0x20);//enable IBAT measure result average 4 samples
        hard_i2c_write168(DEVID, 0x009A, 0x20);
        hard_i2c_write168(DEVID, 0x00B8, 0x00);
        hard_i2c_write168(DEVID, 0x00A9, 0x8f);

        hard_i2c_write168(DEVID, 0x00A0, 0x01);//select auto-sampling timebase is 2ms
        hard_i2c_write168(DEVID, 0x00A1, 0x15);//set the IBAT measure threshold and enable auto IBAT +VBAT_in_active sample
        hard_i2c_write168(DEVID, 0x00C9, 0x06);// open DCIN_OK and USB_OK IRQ
    }
    __udelay(80000);
    val1 = hard_i2c_read168(DEVID, 0x00af);
    val2 = hard_i2c_read168(DEVID, 0x00b0);
    serial_puts("\n-- vbat: 0x");
    serial_put_hex(val2, 8);
    serial_put_hex(val1, 8);
    serial_puts("\n");
}

void aml1218_power_init(int init_mode)
{
    aml1218_set_pfm(1, 0);                                      // DC1 ~ 3 to fix PWM
    aml1218_set_pfm(2, 0);
    aml1218_set_pfm(3, 0);

    // According David Wang
    aml1218_set_bits(0x0038, 0x08, 0x08);                       // DCDC ov threshold adjust
    aml1218_set_bits(0x0041, 0x08, 0x08);
    aml1218_set_bits(0x004b, 0x40, 0x40);

    aml1218_set_bits(0x0037, 0x08, 0x08);                       // ya xie zhendang
    aml1218_set_bits(0x0039, 0x02, 0x02);
    aml1218_set_bits(0x0042, 0x02, 0x02);

    hard_i2c_write168(DEVID, 0x0220, 0xff);                         // reset audio 
    hard_i2c_write168(DEVID, 0x0221, 0xff);

    aml1218_set_bits(0x0140, 0x08, 0x1f);                           // enable ramp control, 10us/step
    aml1218_set_bits(0x0141, 0x08, 0x1f);                           // enable ramp control, 10us/step
    aml1218_set_bits(0x0142, 0x08, 0x1f);                           // enable ramp control, 10us/step

    aml1218_set_bits(0x0033, 0x00, 0x70);                           // test
    aml1218_set_bits(0x001b, 0x06, 0x46);                           // Enable DCDC1 & 2 fault
    aml1218_set_bits(0x001c, 0x06, 0x06);
    aml1218_set_bits(0x0045, 0x08, 0x08);
    aml1218_set_bits(0x003c, 0x08, 0x08);
    hard_i2c_write168(DEVID, 0x0121, 0x12);                         // enable DC3 oc
    hard_i2c_write168(DEVID, 0x004d, 0x00);
    aml1218_set_bits(0x011f, 0x04, 0x04);
    aml1218_set_bits(0x011d, 0x04, 0x04);
    aml1218_set_bits(0x003c, 0x00, 0x02);                           // open LDO8 for RTC power
    aml1218_check_vbat(1);
    if (init_mode == POWER_INIT_MODE_NORMAL) {
#ifdef CONFIG_VCCK_VOLTAGE
        aml1218_set_vddEE_voltage(CONFIG_VCCK_VOLTAGE); 
        __udelay(2000);
#endif

#ifdef CONFIG_DDR_VOLTAGE
        aml1218_set_dcdc_voltage(2, CONFIG_DDR_VOLTAGE);
        __udelay(2000);              
        hard_i2c_write168(DEVID, 0x0082, 0x04);                     // open DCDC2 
        __udelay(2000);              
#endif

#ifdef CONFIG_VDDAO_VOLTAGE
        aml1218_set_dcdc_voltage(1, CONFIG_VDDAO_VOLTAGE); 
        __udelay(2000);
#endif

#ifdef CONFIG_VCC3V3
        //aml1218_set_dcdc_voltage(3, CONFIG_VCC3V3);
        __udelay(2000);
#endif

#ifdef CONFIG_IOREF_1V8
        aml1218_set_ldo_voltage(2, CONFIG_IOREF_1V8);
        __udelay(2000);
#endif

#ifdef CONFIG_VCC2V8 
        aml1218_set_ldo_voltage(3, CONFIG_VCC2V8);
        __udelay(2000);
#endif

#ifdef CONFIG_DVDD_1V8 
        aml1218_set_ldo_voltage(4, CONFIG_DVDD_1V8);
        __udelay(2000);
#endif

#ifdef CONFIG_VCC2V5 
        aml1218_set_ldo_voltage(5, CONFIG_VCC2V5);
        __udelay(2000);
#endif

#ifdef CONFIG_VCC_CAM
        aml1218_set_ldo_voltage(6, CONFIG_VCC_CAM);
        hard_i2c_write168(DEVID, 0x83, 0x01);                           // open LDO6
#endif

#ifdef CONFIG_VDDIO_AO28
        aml1218_set_ldo_voltage(7, CONFIG_VDDIO_AO28);
        __udelay(2000);
#endif
    } else if (init_mode == POWER_INIT_MODE_USB_BURNING) {
        /*
         * if under usb burning mode, keep VCCK and VDDEE
         * as low as possible for power saving and stable issue
         */
        aml1218_set_dcdc_voltage(1, 900);                       // set cpu voltage                      
        aml1218_set_vddEE_voltage(950);                         // set VDDEE voltage
    }
    aml1218_check_vbat(0);
}
#endif


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
static int vcck_pwm_off(void)
{
    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 0, 1, 1);  //disable pwm_d
    return 0;
}

static int pwm_duty_cycle_set(int duty_high,int duty_total)
{
    int pwm_reg=0;

    aml_set_reg32_bits(P_PWM_MISC_REG_CD, 0, 16, 7);  //pwm_d_clk_div
    if(duty_high > duty_total){
        serial_puts("error: duty_high larger than duty_toral !!!\n");
        return -1; 
    }
    aml_write_reg32(P_PWM_PWM_D, (duty_high << 16) | (duty_total-duty_high));
    __udelay(100000);
    pwm_reg = aml_read_reg32(P_PWM_PWM_D);
    //serial_puts("##### P_PWM_PWM_D value = ");
    //serial_put_hex(pwm_reg, 32);
    //serial_puts("\n");

    return 0;
}

int m8b_pwm_set_vddEE_voltage(int voltage)
{

    int duty_high = 0;
    vcck_pwm_on();


    duty_high = 28-(voltage-860)/10;

#if 1
    serial_puts("##### VDDEE voltage = 0x");
    serial_put_hex(voltage, 16);
    serial_puts("\n");
#endif
    pwm_duty_cycle_set(duty_high,28);

}
#endif

void power_init(int init_mode)
{

    hard_i2c_init();
    
    __udelay(1000);
    serial_puts("PMU:"PMU_NAME"\n");
#ifdef CONFIG_AW_AXP20
    axp20_power_init(init_mode);
#elif defined CONFIG_PMU_ACT8942
    if(CONFIG_VDDAO_VOLTAGE <= 1200)
        hard_i2c_write8(DEVID, 0x21, (CONFIG_VDDAO_VOLTAGE - 600) / 25);
    else if(CONFIG_VDDAO_VOLTAGE <= 2400)
        hard_i2c_write8(DEVID, 0x21, ((CONFIG_VDDAO_VOLTAGE - 1200) / 50) + 0x18);
    else
        hard_i2c_write8(DEVID, 0x21, ((CONFIG_VDDAO_VOLTAGE - 2400) / 100) + 0x30);
#elif defined CONFIG_AML_PMU
    aml1212_power_init(init_mode); 
#elif defined CONFIG_RN5T618
    rn5t618_power_init(init_mode);
#elif defined CONFIG_AML1216
    aml1216_power_init(init_mode);
#elif defined CONFIG_AML1218
    aml1218_power_init(init_mode);
#elif defined (CONFIG_PWM_DEFAULT_VCCK_VOLTAGE) && defined (CONFIG_VCCK_VOLTAGE)
    vcck_set_default_voltage(CONFIG_VCCK_VOLTAGE);
#elif defined CONFIG_PWM_VDDEE_VOLTAGE
    m8b_pwm_set_vddEE_voltage(CONFIG_PWM_VDDEE_VOLTAGE);
#endif
    __udelay(1000);
}
