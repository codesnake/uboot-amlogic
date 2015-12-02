#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/uart.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/pctl.h>
#include <asm/arch/dmc.h>
#include <asm/arch/ddr.h>
#include <asm/arch/memtest.h>
#include <asm/arch/pctl.h>

#define AML_I2C_CTRL_CLK_DELAY_MASK			0x3ff
#define AML_I2C_SLAVE_ADDR_MASK				0xff
#define AML_I2C_SLAVE_ADDR_MASK_7BIT   (0x7F)
#define AML_I2C_MAX_TOKENS		8
#define ETIMEDOUT 110
#define EIO       111

/*
 * if not defined DDR and AO suspend voltage, define a default value for them
 */
#ifndef CONFIG_DDR_VOLTAGE              // ddr voltage for resume
#define CONFIG_DDR_VOLTAGE              1500
#endif

#ifndef CONFIG_VDDAO_VOLTAGE            // VDDAO voltage for resume
#define CONFIG_VDDAO_VOLTAGE            1200
#endif

#ifndef CONFIG_DCDC_PFM_PMW_SWITCH
#define CONFIG_DCDC_PFM_PMW_SWITCH      1
#endif

/*
 * i2c clock speed define for 32K and 24M mode
 */
#define I2C_SUSPEND_SPEED    6                  // speed = 8KHz / I2C_SUSPEND_SPEED
#define I2C_RESUME_SPEED    60                  // speed = 6MHz / I2C_RESUME_SPEED

#define 	I2C_IDLE		0
#define 	I2C_RUNNING	1

struct aml_i2c {
	unsigned int		cur_slave_addr;
	unsigned char		token_tag[AML_I2C_MAX_TOKENS];
	unsigned int 		msg_flags;
}i2c;

#define I2C_M_TEN		       0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		       0x0001	/* read data, from slave to master */
#define I2C_M_NOSTART		   0x4000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR 0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	 0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		 0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		 0x0400	/* length will be first received byte */

typedef unsigned short __u16;
typedef unsigned char __u8;

struct i2c_msg {
	__u16 addr;	/* slave address			*/
	__u16 flags;
	__u16 len;		/* msg length				*/
	__u8 *buf;		/* pointer to msg data			*/
};

enum aml_i2c_token {
	TOKEN_END,
	TOKEN_START,
	TOKEN_SLAVE_ADDR_WRITE,
	TOKEN_SLAVE_ADDR_READ,
	TOKEN_DATA,
	TOKEN_DATA_LAST,
	TOKEN_STOP
};

struct aml_i2c_platform{
	unsigned int 		wait_count;/*i2c wait ack timeout = 
											wait_count * wait_ack_interval */
	unsigned int 		wait_ack_interval;
	unsigned int 		wait_read_interval;
	unsigned int 		wait_xfer_interval;
	unsigned int 		master_no;
	unsigned int		use_pio;/*0: hardware i2c, 1: manual pio i2c*/
	unsigned int		master_i2c_speed;
};

struct aml_i2c_platform g_aml_i2c_plat = {
    .wait_count         = 1000000,
    .wait_ack_interval  = 5,
    .wait_read_interval = 5,
    .wait_xfer_interval = 5,
    .master_no          = 0,
    .use_pio            = 0,
    .master_i2c_speed   = 160
};

#define v_outs(s,v) serial_puts(s);serial_put_dword(v);

static int aml_i2c_do_address(unsigned int addr)
{
	i2c.cur_slave_addr = addr & AML_I2C_SLAVE_ADDR_MASK_7BIT;
	writel(readl(P_AO_I2C_M_0_SLAVE_ADDR)&(~AML_I2C_SLAVE_ADDR_MASK), P_AO_I2C_M_0_SLAVE_ADDR);
	writel(readl(P_AO_I2C_M_0_SLAVE_ADDR)|(i2c.cur_slave_addr<<1), P_AO_I2C_M_0_SLAVE_ADDR);
//	writel((i2c.cur_slave_addr<<1), P_AO_I2C_M_0_SLAVE_ADDR);
	return 0;
}
static void  aml_i2c_clear_token_list(void)
{	
	int i;
	writel(0, P_AO_I2C_M_0_TOKEN_LIST0);
	writel(0, P_AO_I2C_M_0_TOKEN_LIST1);
	for(i=0; i<AML_I2C_MAX_TOKENS; i++)
	  	i2c.token_tag[i] = TOKEN_END;
}

static void aml_i2c_set_token_list(void)
{
	int i;
	unsigned int token_reg=0;	
	for(i=0; i<AML_I2C_MAX_TOKENS; i++)
		token_reg |= i2c.token_tag[i]<<(i*4);
	writel(token_reg, P_AO_I2C_M_0_TOKEN_LIST0);
}

#if 0
void udelay(int i)
{
	int delays = i ;//* 24;
	unsigned base= readl(P_AO_TIMERE_REG);
	//while(((readl(P_AO_TIMERE_REG)-base)&0xffffff) < (  delays&0xffffff)); //reg value is 24bit case
	while(((readl(P_AO_TIMERE_REG)-base)) < (  delays));
}
#else
void udelay(int i)
{
    int delays = 0;
    for(delays=0;delays<i;delays++)
    {
        asm("mov r0,r0");
    }
}
#endif

static void aml_i2c_start_token_xfer()
{
	writel(readl(P_AO_I2C_M_0_CONTROL_REG)&(~1), P_AO_I2C_M_0_CONTROL_REG);
	writel(readl(P_AO_I2C_M_0_CONTROL_REG)|1, P_AO_I2C_M_0_CONTROL_REG);
	udelay(g_aml_i2c_plat.wait_xfer_interval);
}

static void aml_i2c_stop()
{
	aml_i2c_clear_token_list();
	i2c.token_tag[0]=TOKEN_STOP;
	aml_i2c_set_token_list();
	aml_i2c_start_token_xfer();
}

static int aml_i2c_check_error()
{
    if (readl(P_AO_I2C_M_0_CONTROL_REG)&(1<<3))
		return -EIO;
	else
		return 0;
}

/*poll status*/
static int aml_i2c_wait_ack()
{
	int i;
	for(i=0; i<g_aml_i2c_plat.wait_count; i++) {
		udelay(g_aml_i2c_plat.wait_ack_interval);
	if (((readl(P_AO_I2C_M_0_CONTROL_REG)&(1<<2))>>2)==I2C_IDLE)
			return aml_i2c_check_error();
	}
	return -ETIMEDOUT;			
}

#define min_t(a,b) ((a) < (b) ? (a):(b))
	
static void aml_i2c_get_read_data(unsigned char *buf, unsigned len)
{
	int i;
	unsigned long rdata0 = readl(P_AO_I2C_M_0_RDATA_REG0); //pmaster->i2c_token_rdata_0;
	unsigned long rdata1 = readl(P_AO_I2C_M_0_RDATA_REG1); //pmaster->i2c_token_rdata_1;

	for(i=0; i< min_t(len, AML_I2C_MAX_TOKENS>>1); i++)
		*buf++ = (rdata0 >> (i*8)) & 0xff;

	for(; i< min_t(len, AML_I2C_MAX_TOKENS); i++) 
		*buf++ = (rdata1 >> ((i - (AML_I2C_MAX_TOKENS>>1))*8)) & 0xff;
}

static int aml_i2c_read(unsigned char *buf,unsigned len) 
{
	int i;
	int ret;
	unsigned rd_len;
	int tagnum=0;

	aml_i2c_clear_token_list();
	
	if(!(i2c.msg_flags & I2C_M_NOSTART)){
		i2c.token_tag[tagnum++]=TOKEN_START;
		i2c.token_tag[tagnum++]=TOKEN_SLAVE_ADDR_READ;

		aml_i2c_set_token_list();

		aml_i2c_start_token_xfer();

		udelay(g_aml_i2c_plat.wait_ack_interval);
		
		ret = aml_i2c_wait_ack();
		if(ret<0)
			return ret;	
		aml_i2c_clear_token_list();
	}
	
	while(len){
		tagnum = 0;
		rd_len = min_t(len,AML_I2C_MAX_TOKENS);
		if(rd_len == 1)
			i2c.token_tag[tagnum++]=TOKEN_DATA_LAST;
		else{
			for(i=0; i<rd_len-1; i++)
				i2c.token_tag[tagnum++]=TOKEN_DATA;
			if(len > rd_len)
				i2c.token_tag[tagnum++]=TOKEN_DATA;
			else
				i2c.token_tag[tagnum++]=TOKEN_DATA_LAST;
		}
		aml_i2c_set_token_list();
		aml_i2c_start_token_xfer();

		udelay(g_aml_i2c_plat.wait_ack_interval);
		
		ret = aml_i2c_wait_ack();
		if(ret<0)
			return ret;	
		
		aml_i2c_get_read_data( buf, rd_len);
		len -= rd_len;
		buf += rd_len;

		udelay(g_aml_i2c_plat.wait_read_interval);
		aml_i2c_clear_token_list();
	}
	return 0;
}

static void aml_i2c_fill_data(unsigned char *buf, unsigned len)
{
	int i;
	unsigned int wdata0 = 0;
	unsigned int wdata1 = 0;

	for(i=0; i< min_t(len, AML_I2C_MAX_TOKENS>>1); i++)
		wdata0 |= (*buf++) << (i*8);

	for(; i< min_t(len, AML_I2C_MAX_TOKENS); i++)
		wdata1 |= (*buf++) << ((i - (AML_I2C_MAX_TOKENS>>1))*8); 

	writel(wdata0, P_AO_I2C_M_0_WDATA_REG0);
	writel(wdata1, P_AO_I2C_M_0_WDATA_REG1);
}

static int aml_i2c_write(unsigned char *buf, unsigned len) 
{
	int i;
	int ret;
	unsigned wr_len;
	int tagnum=0;

	aml_i2c_clear_token_list();
	
	if(!(i2c.msg_flags & I2C_M_NOSTART)){
		i2c.token_tag[tagnum++]=TOKEN_START;
		i2c.token_tag[tagnum++]=TOKEN_SLAVE_ADDR_WRITE;
	}
	while(len){
		wr_len = min_t(len, AML_I2C_MAX_TOKENS-tagnum);
		for(i=0; i<wr_len; i++)
			i2c.token_tag[tagnum++]=TOKEN_DATA;
		
		aml_i2c_set_token_list();
		
		aml_i2c_fill_data(buf, wr_len);
		
		aml_i2c_start_token_xfer();

		len -= wr_len;
		buf += wr_len;
		tagnum = 0;

		ret = aml_i2c_wait_ack();
		if(ret<0)
			return ret;
		
		aml_i2c_clear_token_list();
    	}
	return 0;
}
int do_msgs(struct i2c_msg *msgs,int num)
{
	int i,ret;
	struct i2c_msg* p;
	ret = 0;
	for (i = 0; !ret && i < num; i++) {
		p = &msgs[i];
	
		ret = aml_i2c_do_address( p->addr);
		if (ret || !p->len)
		{
			continue;
		}
		if (p->flags & I2C_M_RD)
		{
				ret = aml_i2c_read(p->buf, p->len);
		}
		else{
			ret = aml_i2c_write(p->buf, p->len);
		}
	}
	
	aml_i2c_stop();

	/* Return the number of messages processed, or the error code*/
	if (ret == 0)
		return num;
	else 
		return ret;
}
#ifdef CONFIG_AW_AXP20
#define POWER_OFF_AVDD25
#define POWER_OFF_AVDD33
#define POWER_OFF_VDDIO

#define I2C_AXP202_ADDR   (0x68 >> 1)
#define AXP_I2C_ADDR           0x34
typedef __u8                   uint8_t;
#define AXP_OCV_BUFFER0        (0xBC)
void i2c_axp202_write(unsigned char reg, unsigned char val)
{
    unsigned char buff[2];
    buff[0] = reg;
    buff[1] = val;

	struct i2c_msg msg[] = {
        {
        .addr  = I2C_AXP202_ADDR,
        .flags = 0,
        .len   = 2,
        .buf   = buff,
        }
    };

    if (do_msgs(msg, 1) < 0) {
        f_serial_puts("i2c transfer failed\n");
    }
}
unsigned char i2c_axp202_read(unsigned char reg)
{
    unsigned char val = 0;
    struct i2c_msg msgs[] = {
        {
            .addr = I2C_AXP202_ADDR,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        {
            .addr = I2C_AXP202_ADDR,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = &val,
        }
    };

    if (do_msgs(msgs, 2)< 0) {
       f_serial_puts("<error>");
    }

    return val;
}
int axp_reads(int reg, int len, uint8_t *val)
{
    int ret;
    struct i2c_msg msgs[] = {
        {
            .addr = AXP_I2C_ADDR,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        {
            .addr = AXP_I2C_ADDR,
            .flags = I2C_M_RD,
            .len = len,
            .buf = val,
        }
    };
    ret = do_msgs(msgs, 2);
    if (ret < 0) {
       f_serial_puts("<error>");
       wait_uart_empty();
        return ret;
    }
    return 0;
}
int axp_get_ocv(void)
{
    int battery_ocv;
    uint8_t v[2];
    axp_reads(AXP_OCV_BUFFER0, 2, v);
    battery_ocv = (((v[0] << 4) + (v[1] & 0x0f)) * 2253) >> 11;
    return battery_ocv;
}
#endif//CONFIG_AW_AXP20

#ifdef CONFIG_ACT8942QJ233_PMU
#define I2C_ACT8942QJ233_ADDR   (0x5B)
void i2c_act8942_write(unsigned char reg, unsigned char val)
{
    unsigned char buff[2];
    buff[0] = reg;
    buff[1] = val;

	struct i2c_msg msg[] = {
        {
        .addr  = I2C_ACT8942QJ233_ADDR,
        .flags = 0,
        .len   = 2,
        .buf   = buff,
        }
    };

    if (do_msgs(msg, 1) < 0) {
        f_serial_puts("i2c transfer failed\n");
    }
}
unsigned char i2c_act8942_read(unsigned char reg)
{
    unsigned char val = 0;
    struct i2c_msg msgs[] = {
        {
            .addr = I2C_ACT8942QJ233_ADDR,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        {
            .addr = I2C_ACT8942QJ233_ADDR,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = &val,
        }
    };

    if (do_msgs(msgs, 2)< 0) {
       f_serial_puts("<error>");
    }

    return val;
}
#endif//CONFIG_ACT8942QJ233_PMU

#ifdef CONFIG_AML_PMU

#define I2C_AML_PMU_ADDR   (0x6a >> 1)
#define i2c_pmu_write_b(reg, val)       i2c_pmu_write_12(reg, 1, val)
#define i2c_pmu_write_w(reg, val)       i2c_pmu_write_12(reg, 2, val)
#define i2c_pmu_read_b(reg)             (unsigned  char)i2c_pmu_read_12(reg, 1)
#define i2c_pmu_read_w(reg)             (unsigned short)i2c_pmu_read_12(reg, 2)

void printf_arc(const char *str)
{
    f_serial_puts(str);
    wait_uart_empty();
}

void i2c_pmu_write_12(unsigned int reg, int size, unsigned short val)
{
	unsigned char buff[4];
	buff[0] = reg & 0xff;
	buff[1] = (reg >> 8) & 0x0f;
	buff[2] = val & 0xff;

    if (size == 2) {
    	buff[3] = (val >> 8) & 0xff;
    }

	struct i2c_msg msg[] = {
        {
        .addr  = I2C_AML_PMU_ADDR,
        .flags = 0,
        .len   = 2 + size,
        .buf   = buff,
        }
    };

    if (do_msgs(msg, 1) < 0) {
        printf_arc("i2c write failed\n");
    }
}

unsigned short i2c_pmu_read_12(unsigned int reg, int size)
{
	unsigned char buff[2];
    unsigned short val = 0;
	buff[0] = reg & 0xff;
	buff[1] = (reg >> 8) & 0x0f;
    
    struct i2c_msg msgs[] = {
        {
            .addr = I2C_AML_PMU_ADDR,
            .flags = 0,
            .len = 2,
            .buf = buff,
        },
        {
            .addr = I2C_AML_PMU_ADDR,
            .flags = I2C_M_RD,
            .len = size,
            .buf = &val,
        }
    };

    if (do_msgs(msgs, 2)< 0) {
       printf_arc("i2c read error\n");
    }

    return val;
}

#endif//CONFIG_AML_PMU

extern void delay_ms(int ms);
void init_I2C()
{
	unsigned v,speed,reg;
	struct aml_i2c_reg_ctrl* ctrl;

	//1. set pinmux
	v = readl(P_AO_RTI_PIN_MUX_REG);
	//master
	v |= ((1<<5)|(1<<6));
	//slave
	// v |= ((1<<1)|(1<<2)|(1<<3)|(1<<4));
	writel(v,P_AO_RTI_PIN_MUX_REG);

	//-------------------------------	
	//reset
//	writel(readl(P_AO_RTI_GEN_CNTL_REG0)|(3<<18),P_AO_RTI_GEN_CNTL_REG0);
//	writel(readl(P_AO_RTI_GEN_CNTL_REG0)&(~(3<<18)),P_AO_RTI_GEN_CNTL_REG0);
//	delay_ms(20);
	//---------------------------------
	//config	  
 	//v=20; //(32000/speed) >>2) speed:400K
	reg = readl(P_AO_I2C_M_0_CONTROL_REG);
	reg &= 0xFFC00FFF;
	reg |= (I2C_RESUME_SPEED <<12);             // at 24MHz, i2c speed to 100KHz
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
//	delay_ms(20);
	delay_ms(1);
#ifdef CONFIG_ACT8942QJ233_PMU
	v = i2c_act8942_read(0);
	if(v == 0xFF || v == 0)
#endif
#ifdef CONFIG_AW_AXP20
	v = i2c_axp202_read(0x12);
	if(v != 0x5F )
#endif
#ifdef CONFIG_AML_PMU
	v = i2c_pmu_read_b(0x007f);                 // read PMU version
	if(v == 0xff || v == 0)
#endif
	{
        serial_put_hex(v, 8);
        f_serial_puts(" Error: I2C init failed!\n");
    }

}

/***************************/
/*******AXP202 PMU*********/
/**************************/
#ifdef CONFIG_AW_AXP20
#define POWER20_DCDC_MODESET        (0x80)
#define POWER20_DC2OUT_VOL          (0x23)
#define POWER20_DC3OUT_VOL          (0x27)
#define POWER20_LDO24OUT_VOL        (0x28)
#define POWER20_LDO3OUT_VOL         (0x29)

unsigned char vddio;
unsigned char vdd25;
unsigned char avdd33;
unsigned char _3gvcc;
unsigned char ddr15_reg12;//reg=0x12
unsigned char ddr15_reg23;//reg=0x23
unsigned char dcdc_reg;
/*
void dump_pmu_reg()
{
	int i,data;
	for(i=0;i<0x95;i++)
	{
		data=i2c_axp202_read(i);		
		serial_put_hex(i,8);
		serial_puts("	");
		serial_put_hex(data,8);
		serial_puts("\n");
	}

}
*/
void power_off_avdd25()
{
	unsigned char data;
	vdd25 = i2c_axp202_read(0x12);
	data = vdd25 & (~(1<<6));//ldo3
	i2c_axp202_write(0x12,data);
	
	udelay(100);
}

void power_on_avdd25()
{
	unsigned char data;
	data = vdd25 | 1<<6;//ldo3
	i2c_axp202_write(0x12,data);
	
	udelay(100);
}

void power_off_vddio()
{
	unsigned char data;
	vddio = i2c_axp202_read(0x12);
	data = vddio & 0xfe;//EXTEN
	i2c_axp202_write(0x12,data);

	udelay(100);

}

void power_on_vddio()
{
	unsigned char data;

	data = vddio | 0x01;//EXTEN
	i2c_axp202_write(0x12,data);

	udelay(100);

}

void power_off_avdd33()
{
	unsigned char data;
	avdd33 = i2c_axp202_read(0x12);
	data = avdd33 & 0xf7;
	i2c_axp202_write(0x12,data);

	udelay(100);

}

void power_on_avdd33()
{
	unsigned char data;

	data = avdd33 | 0x08;
	i2c_axp202_write(0x12,data);

	udelay(100);

}


void power_off_3gvcc()
{
	unsigned char data;
	_3gvcc = i2c_axp202_read(0x91);
	data = _3gvcc & 0x0f;//low or hight is enable
	//data = _3gvcc | 0x07;
	i2c_axp202_write(0x91,data);

	udelay(100);

}

void power_on_3gvcc()
{
	unsigned char data;

	data = _3gvcc;// | 0xa0;
	//data = _3gvcc & 0xf8;
	i2c_axp202_write(0x91,data);

	udelay(100);

}

void power_off_ddr15()
{	
	i2c_axp202_write(0x12,i2c_axp202_read(0x12) & 0xef);	
	udelay(100);
}

void power_on_ddr15()
{	
	i2c_axp202_write(0x12,i2c_axp202_read(0x12) | 0x10);	
	udelay(100);
}

unsigned char get_charging_state()
{
	return (i2c_axp202_read(0x0) & 0xa0);
}

void shut_down()
{
	i2c_axp202_write(0x32,i2c_axp202_read(0x32) | 0x80);
}

void dc_dc_pwm_switch(unsigned int flag)
{
	unsigned char data;
	if(flag)
	{
		//data = i2c_axp202_read(0x80);
		//data |= (unsigned char)(3<<1);
		i2c_axp202_write(0x80,dcdc_reg);
	}
	else//PFM
	{
		dcdc_reg = i2c_axp202_read(0x80);
		data = dcdc_reg & (unsigned char)(~(3<<1));
		i2c_axp202_write(0x80,data);
	}
	udelay(100);//>1ms@32k
}

int find_idx(int start, int target, int step, int length)
{
    int i = 0;
    do {
        if (start >= target) {
            break;    
        }    
        start += step;
        i++;
    } while (i < length);
    return i;
}

void axp20_dcdc_voltage(int dcdc, int target) 
{
    int idx_to, idx_cur;
    int addr, val, mask;
    if (dcdc == 2) {
        idx_to = find_idx(700, target, 25, 64);
        addr   = POWER20_DC2OUT_VOL; 
        mask   = 0x3f;
    } else if (dcdc == 3) {
        idx_to = find_idx(700, target, 25, 128);
        addr   = POWER20_DC3OUT_VOL; 
        mask   = 0x7f;
    }
    val = i2c_axp202_read(addr);
    idx_cur = val & mask;
    
#if 0                                                           // for debug
	f_serial_puts(" voltage set from 0x");
	serial_put_hex(idx_cur, 8);
    wait_uart_empty();
    f_serial_puts(" to 0x");
	serial_put_hex(idx_to, 8);
    wait_uart_empty();
    f_serial_puts("\n");
#endif

    while (idx_cur != idx_to) {
        if (idx_cur < idx_to) {                                 // adjust to target voltage step by step
            idx_cur++;    
        } else {
            idx_cur--;
        }
        val &= ~mask;
        val |= (idx_cur);
        i2c_axp202_write(addr, val);
        udelay(100);                                            // atleast delay 100uS
    }
}

int ldo4_voltage_table[] = { 1250, 1300, 1400, 1500, 1600, 1700,
				   1800, 1900, 2000, 2500, 2700, 2800,
				   3000, 3100, 3200, 3300 };

extern void wait_uart_empty();
int check_all_regulators(void)
{
	int ret = 0;
	unsigned char reg_data, val;
	
	f_serial_puts("Check all regulator\n");
	
#ifdef CONFIG_CONST_PWM_FOR_DCDC
	//check work mode for DCDC2 & DCDC3
	reg_data = i2c_axp202_read(POWER20_DCDC_MODESET);
	if(!((reg_data&(1<<1) )&&(reg_data&(1<<2) )))
	{
		f_serial_puts("Use constant PWM for DC-DC2 & DC-DC3. But the register is 0x");
		serial_put_hex(reg_data, 8);
		f_serial_puts(" before\n");
 		wait_uart_empty();
		reg_data |= ((1<<1)|(1<<2));
		i2c_axp202_write(POWER20_DCDC_MODESET, reg_data);	//use constant PWM for DC-DC2 & DC-DC3
		udelay(10000);
		ret = 1;
	}
#endif

#ifdef CONFIG_DISABLE_LDO3_UNDER_VOLTAGE_PROTECT
	reg_data = i2c_axp202_read(0x81);	//check switch for  LDO3 under voltage protect
	if(reg_data & (1<<2))
	{
		f_serial_puts("Disable LDO3 under voltage protect. But the register is 0x");
		serial_put_hex(reg_data, 8);
		f_serial_puts(" before\n");
 		wait_uart_empty();
		reg_data &= ~(1<<2);	//disable LDO3 under voltage protect
		i2c_axp202_write(0x81, reg_data);	
		udelay(10000);
		ret = 1;
	}
#endif

	//check for LDO2(VDDIO_AO)
#ifdef CONFIG_LDO2_VOLTAGE
#if ((CONFIG_LDO2_VOLTAGE<1800) || (CONFIG_LDO2_VOLTAGE>3300))
#error CONFIG_LDO2_VOLTAGE not in the range 1800~3300mV
#endif
	val = (CONFIG_LDO2_VOLTAGE-1800)/100;
	reg_data = i2c_axp202_read(POWER20_LDO24OUT_VOL);
	if(((reg_data>>4)&0xf)	!= val)
	{
		val = (reg_data & 0xf0) | (val<<4);
		i2c_axp202_write(POWER20_LDO24OUT_VOL, val);	//set LDO2(VDDIO_AO) to 3.000V
		f_serial_puts("Set LDO2(VDDIO_AO) register to 0x");
		serial_put_hex(val, 8);
		f_serial_puts(". But the register is 0x");
		serial_put_hex(reg_data, 8);
		f_serial_puts(" before\n");
 		wait_uart_empty();
		udelay(10000);
		ret = 1;
	}
#endif

	//check for LDO4(AVDD3.3V)
#ifdef CONFIG_LDO4_VOLTAGE
#if ((CONFIG_LDO4_VOLTAGE<1250) || (CONFIG_LDO4_VOLTAGE>3300))
#error CONFIG_LDO4_VOLTAGE not in the range 1250~3300mV
#endif
	for(val = 0; val < sizeof(ldo4_voltage_table); val++){
		if(CONFIG_LDO4_VOLTAGE <= ldo4_voltage_table[val]){
			break;
		}
	}
	reg_data = i2c_axp202_read(POWER20_LDO24OUT_VOL);
	if((reg_data&0xf) != val)
	{
		i2c_axp202_write(POWER20_LDO24OUT_VOL, val);	//set LDO4(AVDD3.3V) to 3.300V
		f_serial_puts("Set LDO4(AVDD3.3V) register to 0x");
		serial_put_hex(val, 8);
		f_serial_puts(". But the register is 0x");
		serial_put_hex(reg_data, 8);
		f_serial_puts(" before\n");
 		wait_uart_empty();
		udelay(10000);
		ret = 1;
	}
#endif

#ifdef CONFIG_LDO3_VOLTAGE
#if ((CONFIG_LDO3_VOLTAGE<700) || (CONFIG_LDO3_VOLTAGE>3500))
#error CONFIG_LDO3_VOLTAGE not in the range 700~3500mV
#endif
	val = (CONFIG_LDO3_VOLTAGE -700)/25;
	//check for LDO3(AVDD2.5V)
	reg_data = i2c_axp202_read(POWER20_LDO3OUT_VOL);
	if(reg_data != 0x48)
	{
		i2c_axp202_write(POWER20_LDO3OUT_VOL, 0x48);	//set LDO3(AVDD2.5V) to 2.500V;
		f_serial_puts("Set  LDO3(AVDD2.5V) register to 0x");
		serial_put_hex(val, 8);
		f_serial_puts(". But the register is 0x");
		serial_put_hex(reg_data, 8);
		f_serial_puts(" before\n");
 		wait_uart_empty();
		udelay(10000);
		ret = 1;
	}
#endif
	return ret;
}
#endif//CONFIG_AW_AXP20

/**************************/
/**************************/
#ifdef CONFIG_ACT8942QJ233_PMU
void vddio_off()
{
	unsigned char reg3;
	//To disable the regulator, set ON[ ] to 1 first then clear it to 0.
	reg3 = i2c_act8942_read(0x42);	//add by Elvis for cut down VDDIO
	reg3 |= (1<<7);	
	i2c_act8942_write(0x42,reg3);
	reg3 = i2c_act8942_read(0x42);
	reg3 &= ~(1<<7);	
	i2c_act8942_write(0x42,reg3);
}

void vddio_on()
{
	unsigned char reg3;
	//To enable the regulator, clear ON[ ] to 0 first then set to 1.
	reg3 = i2c_act8942_read(0x42);	//add by Elvis, Regulator3 Enable for VDDIO
	reg3 &= ~(1<<7);	
	i2c_act8942_write(0x42,reg3);
	reg3 = i2c_act8942_read(0x42);
	reg3 |= (1<<7);	
	i2c_act8942_write(0x42,reg3);
}

//Regulator7 Disable for cut down HDMI_VCC
void reg7_off()
{
	unsigned char data;
	//To disable the regulator, set ON[ ] to 1 first then clear it to 0.
	data = i2c_act8942_read(0x65);
	data |= (1<<7);	
	i2c_act8942_write(0x65,data);
	data = i2c_act8942_read(0x65);
	data &= ~(1<<7);	
	i2c_act8942_write(0x65,data);
}

//Regulator7 Enable for power on HDMI_VCC
void reg7_on()
{
	unsigned char data;
	//To enable the regulator, clear ON[ ] to 0 first then set to 1.
	data = i2c_act8942_read(0x65);
	data &= ~(1<<7);	
	i2c_act8942_write(0x65,data);
	data = i2c_act8942_read(0x65);
	data |= (1<<7);	
	i2c_act8942_write(0x65,data);
}
//Regulator7 Disable for cut down HDMI_VCC
void reg5_off()
{
	unsigned char data;
	//To disable the regulator, set ON[ ] to 1 first then clear it to 0.
	data = i2c_act8942_read(0x55);
	data |= (1<<7);	
	i2c_act8942_write(0x55,data);
	data = i2c_act8942_read(0x55);
	data &= ~(1<<7);	
	i2c_act8942_write(0x55,data);
}

//Regulator7 Enable for power on HDMI_VCC
void reg5_on()
{
	unsigned char data;
	//To enable the regulator, clear ON[ ] to 0 first then set to 1.
	data = i2c_act8942_read(0x55);
	data &= ~(1<<7);	
	i2c_act8942_write(0x55,data);
	data = i2c_act8942_read(0x55);
	data |= (1<<7);	
	i2c_act8942_write(0x55,data);
}
//Regulator7 Disable for cut down HDMI_VCC
void reg6_off()
{
	unsigned char data;
	//To disable the regulator, set ON[ ] to 1 first then clear it to 0.
	data = i2c_act8942_read(0x61);
	data |= (1<<7);	
	i2c_act8942_write(0x61,data);
	data = i2c_act8942_read(0x61);
	data &= ~(1<<7);	
	i2c_act8942_write(0x61,data);
}

//Regulator7 Enable for power on HDMI_VCC
void reg6_on()
{
	unsigned char data;
	//To enable the regulator, clear ON[ ] to 0 first then set to 1.
	data = i2c_act8942_read(0x61);
	data &= ~(1<<7);	
	i2c_act8942_write(0x61,data);
	data = i2c_act8942_read(0x61);
	data |= (1<<7);	
	i2c_act8942_write(0x61,data);
}
#endif //CONFIG_ACT8942QJ233_PMU

#ifdef CONFIG_AML_PMU
/*
 * Amlogic PMU suspend/resume interface
 */
#define OTP_GEN_CONTROL0_ADDR       0x17
#define OTP_BOOST_CONTROL_ADDR      0x19
#define GEN_CNTL1_ADDR              0x81
#define PWR_UP_SW_ENABLE_ADDR       0x82
#define PWR_DN_SW_ENABLE_ADDR       0x84
#define SP_CHARGER_STATUS2_ADDR     0xE0
#define SP_CHARGER_STATUS3_ADDR     0xE1
#define GPIO_OUT_CTRL_ADDR          0xC3

#define AML_PMU_DCDC1               0
#define AML_PMU_DCDC2               1
#define AML_PMU_DCDC3               2
#define AML_PMU_BOOST               3
#define AML_PMU_LDO1                4
#define AML_PMU_LDO2                5
#define AML_PMU_LDO3                6
#define AML_PMU_LDO4                7
#define AML_PMU_LDO5                8

//Separate PMU from arc_pwr.c 
//todo...
#define   AML1212_POWER_EXT_DCDC_VCCK_BIT   11
#define   AML1212_POWER_LDO5_BIT            7
#define   AML1212_POWER_LDO4_BIT            6
#define   AML1212_POWER_LDO3_BIT            5
#define   AML1212_POWER_LDO2_BIT            4
#define   AML1212_POWER_DC4_BIT             0
#define   AML1212_POWER_DC3_BIT             3
#define   AML1212_POWER_DC2_BIT             2
#define   AML1212_POWER_DC1_BIT             1

unsigned char vbus_status;
unsigned char pmu_version = 0;

/*
 * fake functions for suspend interface
 */
void power_off_avdd25(void) {}
void power_on_avdd25(void) {}
void power_off_avdd33(void) {}
void power_on_avdd33(void) {}
void power_off_vddio(void) {}
void power_on_vddio(void) {}
void dc_dc_pwm_switch(int pfm) {}
void check_all_regulators(void) {}

void aml_pmu_power_ctrl(int on, int bit_mask)
{
    unsigned char addr = on ? PWR_UP_SW_ENABLE_ADDR : PWR_DN_SW_ENABLE_ADDR;
    i2c_pmu_write_w(addr, (unsigned short)bit_mask);
    udelay(100);
}

#define power_off_avdd25_aml()      aml_pmu_power_ctrl(0, 1 << AML1212_POWER_LDO5_BIT)
#define power_on_avdd25_aml()       aml_pmu_power_ctrl(1, 1 << AML1212_POWER_LDO5_BIT)
#define power_off_avdd30()          aml_pmu_power_ctrl(0, 1 << AML1212_POWER_LDO3_BIT)
#define power_on_avdd30()           aml_pmu_power_ctrl(1, 1 << AML1212_POWER_LDO3_BIT)
#define power_off_avdd18()          aml_pmu_power_ctrl(0, 1 << AML1212_POWER_LDO4_BIT)
#define power_on_avdd18()           aml_pmu_power_ctrl(1, 1 << AML1212_POWER_LDO4_BIT) 
#define power_off_vcc33()           aml_pmu_power_ctrl(0, 1 << AML1212_POWER_DC3_BIT)  
#define power_on_vcc33()            aml_pmu_power_ctrl(1, 1 << AML1212_POWER_DC3_BIT)
#define power_off_vcc50()           aml_pmu_power_ctrl(0, 1 << AML1212_POWER_DC4_BIT)
#define power_on_vcc50()            aml_pmu_power_ctrl(1, 1 << AML1212_POWER_DC4_BIT)
#define power_off_vcck12()          aml_pmu_power_ctrl(0, 1 << AML1212_POWER_EXT_DCDC_VCCK_BIT)
#define power_on_vcck12()           aml_pmu_power_ctrl(1, 1 << AML1212_POWER_EXT_DCDC_VCCK_BIT)

inline void power_off_ddr15(void)
{
    aml_pmu_power_ctrl(0, 1 << AML1212_POWER_DC2_BIT);
}

inline void power_on_ddr15(void) 
{
    aml_pmu_power_ctrl(1, 1 << AML1212_POWER_DC2_BIT);
}

void power_off_3gvcc()
{
    // nothing todo, not used right now
}

void power_on_3gvcc()
{
    // nothing todo, not used right now
}

void aml_pmu_set_gpio(int gpio, int val)
{
    unsigned int data;

    if (gpio > 4 || gpio <= 0) {
        return;    
    }

    data = (1 << (gpio + 11));
    if (val) {
        i2c_pmu_write_w(PWR_DN_SW_ENABLE_ADDR, data);    
    } else {
        i2c_pmu_write_w(PWR_UP_SW_ENABLE_ADDR, data);    
    }
    udelay(100);
}

unsigned char get_charging_state()
{
    unsigned char status;

    status = i2c_pmu_read_b(SP_CHARGER_STATUS2_ADDR);
    if (status & 0x18) {
        return 1;                                               // CHG_DCIN_OK | CHG_VBUS_OK 
    } else {
        return 0;                                               // not charging
    }
}

/*
 * work around for can not stop charge
 */
unsigned char get_charge_end_det()
{
    unsigned char status;
    status = i2c_pmu_read_b(0x00);
    if (status & 0x80) {
        return 0;    
    }
    status = i2c_pmu_read_b(SP_CHARGER_STATUS3_ADDR);
    if (!(status & 0x08)) {
        return 1;
    } else {
        return 0;
    }
}

void aml_pmu_set_charger(int en)
{
    unsigned char reg;
    unsigned char addr;
    unsigned char mask;

    en &= 0x01;
    if (pmu_version <= 2 && pmu_version) {
        addr = 0x0017;
        mask = 0x01;
    } else if (pmu_version == 0x03) {
        en <<= 7;
        addr = 0x0011;
        mask = 0x80;
    }
    reg = i2c_pmu_read_b(addr);
    reg &= ~mask;
    reg |= en;
    i2c_pmu_write_b(addr, reg);
}

void shut_down()
{
    unsigned char sw_shut_down = (1 << 5);                      // PMU move to OFF state

    aml_pmu_set_gpio(1, 1);
    i2c_pmu_write_b(0x0019, 0x10);                              // cut usb output 
    i2c_pmu_write_w(0x0084, 0x0001);                            // close boost
    i2c_pmu_write_b(0x0078, 0x04);                              // close LDO6 before power off
  //i2c_pmu_write_b(0x00f4, 0x04);                              // according David Wang, for charge cannot stop
    aml_pmu_set_charger(0);                                     // close charger before power off
    if (pmu_version == 0x03) {
        i2c_pmu_write_b(0x004a, 0x00);
    }
    i2c_pmu_write_b(GEN_CNTL1_ADDR, sw_shut_down);
}

void cut_usb_out(void)                                          // cut usb_power out path
{
    i2c_pmu_write_b(OTP_BOOST_CONTROL_ADDR, 0x10);              // disable boost output 
    udelay(100);
}

int find_idx(int start, int target, int step)
{
    int i = 0;
    for (i = 0; i < 64; i++) {
        start -= step;
        if (target > start) {
            break;    
        }
    }
    return i;
}

void aml_pmu_set_voltage(int dcdc, int voltage)
{
    int idx_to = 0xff;
    int idx_cur;
    unsigned char val;
    unsigned char addr;

    if (dcdc < 0 || dcdc > AML_PMU_DCDC2 || voltage > 2160 || voltage < 760) {
        return ;                                                // current only support DCDC1&2 voltage adjust
    }
    if (dcdc == AML_PMU_DCDC1) {
        addr   = 0x2f; 
        idx_to = find_idx(2000, voltage, 20);                   // voltage index of DCDC1 
    } else if (dcdc = AML_PMU_DCDC2) {
        addr  = 0x38;
        idx_to = find_idx(2160, voltage, 20);                   // voltage index of DCDC2
    }
    val = i2c_pmu_read_b(addr);
    idx_cur = ((val & 0xfc) >> 2);
#if 0                                                           // for debug
	printf_arc("voltage set from 0x");
	serial_put_hex(idx_cur, 8);
    wait_uart_empty();
    printf_arc(" to 0x");
	serial_put_hex(idx_to, 8);
    wait_uart_empty();
    printf_arc("\n");
#endif
    while (idx_cur != idx_to) {
        if (idx_cur < idx_to) {                                 // adjust to target voltage step by step
            idx_cur++;    
        } else {
            idx_cur--;
        }
        val &= ~0xfc;
        val |= (idx_cur << 2);
        i2c_pmu_write_b(addr, val);
        udelay(100);                                            // atleast delay 100uS
    }
}

int aml_pmu_get_voltage(void)
{
    unsigned short buf;
    int tmp;

    i2c_pmu_write_b(0x009A, 0x04);
    udelay(100);
    buf = i2c_pmu_read_w(0x00AF);
    tmp = buf & 0xfff;
    if (pmu_version == 0x02 || pmu_version == 0x01) {
        tmp = tmp * 7200 / 2048;        // LSB of VBAT ADC is 3.51mV
    } else if (pmu_version == 0x03) {
        tmp = tmp * 4800 / 2048;    
    } else {
        tmp = 0;
    }
    return tmp;
}

int aml_pmu_get_current(void)
{
    unsigned short buf;
    unsigned int tmp;

    i2c_pmu_write_b(0x009A, 0x40);
    udelay(100);
    buf = i2c_pmu_read_w(0x00AB);
    tmp = buf & 0xfff; 
    if (tmp & 0x800) {                                              // complement code
        tmp = (tmp ^ 0xfff) + 1;
    }
    return (tmp * 4000) / 2048;                                  // LSB of IBAT ADC is 1.95mA
}

void aml_pmu_set_pfm(int dcdc, int en)
{
    unsigned char val;
    if (dcdc < 0 || dcdc > AML_PMU_DCDC3 || en > 1 || en < 0) {
        return ;    
    }
    switch(dcdc) {
    case AML_PMU_DCDC1:
        val = i2c_pmu_read_b(0x0036);
        if (en) {
            val |=  (1 << 4);                                   // pfm mode
        } else {
            val &= ~(1 << 4);                                   // pwm mode
        }
        i2c_pmu_write_b(0x0036, val);
        break;

    case AML_PMU_DCDC2:
        val = i2c_pmu_read_b(0x003f);
        if (en) {
            val |=  (1 << 4);    
        } else {
            val &= ~(1 << 4);   
        }
        i2c_pmu_write_b(0x003f, val);
        break;

#if 0       // not used right now, to save code size
    case AML_PMU_DCDC3:
        val = i2c_pmu_read_b(0x0049);
        if (en) {
            val |=  (1 << 7);    
        } else {
            val &= ~(1 << 7);   
        }
        i2c_pmu_write_b(0x0049, val);
        break;

    case AML_PMU_BOOST:
        val = i2c_pmu_read_b(0x0028);
        if (en) {
            val |=  (1 << 0);    
        } else {
            val &= ~(1 << 0);   
        }
        i2c_pmu_write_b(0x0028, val);
        break;
#endif

    default:
        break;
    }
    udelay(100);
}
#endif          /* CONFIG_AML_PMU */

//Separate PMU from arc_pwr.c 
//todo...
inline void power_off_at_24M()
{
#ifdef CONFIG_AW_AXP20
#ifdef POWER_OFF_3GVCC
	power_off_3gvcc();
#endif	
#ifdef POWER_OFF_AVDD33
	power_off_avdd33();
#endif
#endif          /* CONFIG_AW_AXP20 */

#ifdef CONFIG_AML_PMU
    pmu_version = i2c_pmu_read_b(0x007f);
    vbus_status = i2c_pmu_read_b(0x0019);
    cut_usb_out();                                  // cut usb boost out path, make usb as input
    printf_arc("cut_usb_out\n");
    aml_pmu_set_gpio(1, 1);
    printf_arc("close gpio1\n");
    power_off_vcc50();
    printf_arc("close boost\n");
    aml_pmu_set_gpio(2, 1);
    printf_arc("close gpio2\n");
    aml_pmu_set_gpio(3, 1);
    printf_arc("close gpio3\n");
    if (!get_charging_state()) {
    #if CONFIG_DCDC_PFM_PMW_SWITCH
        aml_pmu_set_pfm(AML_PMU_DCDC1, 1);
        printf_arc("DCDC1 to pfm\n");
        aml_pmu_set_pfm(AML_PMU_DCDC2, 1);
        printf_arc("DCDC2 to pfm\n");
    #endif
    }
#endif /* CONFIG_AML_PMU */
}

#ifdef CONFIG_AML_PMU
//change register for remember I have disabled charge. 
unsigned char status_charge_flag;
#endif
inline void power_on_at_24M()
{
#ifdef CONFIG_AW_AXP20
#ifdef POWER_OFF_AVDD33
	power_on_avdd33();
#endif
#ifdef POWER_OFF_3GVCC
	power_on_3gvcc();
#endif
#endif      /* CONFIG_AW_AXP20 */

#ifdef CONFIG_AML_PMU
    unsigned char status;
    int charge_state = get_charging_state();
    unsigned char charge_en = 0x01;
    /*
     * Work around for can't stop charging issue, when detected CHG_END_DET bit,
     * disable AUTO_CHG_EN bit and this bit shoud be open at least 1s later
     */
    printf_arc("power on 24m\n");
    status = i2c_pmu_read_b(SP_CHARGER_STATUS3_ADDR);
    if (!(status & 0x08) && charge_state) {
        status_charge_flag  = i2c_pmu_read_b(0x79);
        status_charge_flag &= 0x80;
        if ((aml_pmu_get_current() < 150) && (status_charge_flag !=0x80)) {
            printf_arc("CHG_END_DET, disable AUTO_CHG_EN\n");
            charge_en &= ~(0x01);
            aml_pmu_set_charger(0);
            status_charge_flag  = i2c_pmu_read_b(0x79);
            status_charge_flag |= 0x80;                             // remember I have disabled charge 
            i2c_pmu_write_b(0x79, status_charge_flag);
        }
    }
#if CONFIG_DCDC_PFM_PMW_SWITCH
    aml_pmu_set_pfm(AML_PMU_DCDC1, 0);
    printf_arc("dcdc1 to pwm\n");
    aml_pmu_set_pfm(AML_PMU_DCDC2, 0);
    printf_arc("dcdc2 to pwm\n");
#endif
    aml_pmu_set_gpio(3, 0);
    printf_arc("open gpio3\n");
    aml_pmu_set_gpio(2, 0);
    printf_arc("open gpio2\n");
    udelay(500 * 1000);               				// wait 10ms before applay power to boost 
    power_on_vcc50();
    printf_arc("open boost\n");
    if (!charge_state) {
        status  = i2c_pmu_read_b(0x79); //change register for charge flag.
        status &= ~0x80;                            // clear flag 
        i2c_pmu_write_b(0x79, status);
    }
    udelay(4000);
    if (!(charge_en & 0x01)) {
        charge_en |= 0x01;
        printf_arc("charger was disabled, re-open it\n");
        wait_uart_empty();
        aml_pmu_set_charger(1);
    }

    i2c_pmu_write_b(0x0019, vbus_status);           // restore  vbus status 
    printf_arc("restore boost\n");
#endif /* CONFIG_AML_PMU */
}

inline void power_off_at_32K_1()
{
    unsigned int reg;                               // change i2c speed to 1KHz under 32KHz cpu clock
    unsigned int sleep_flag = readl(P_AO_RTI_STATUS_REG2);
	reg  = readl(P_AO_I2C_M_0_CONTROL_REG);
	reg &= 0xFFC00FFF;
    if (sleep_flag == 0x87654321) {                 // suspended in uboot
        reg |= (12) << 12;
    } else {
    	reg |= (I2C_SUSPEND_SPEED << 12);           // suspended from kernel
    }
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
	udelay(100);

#ifdef CONFIG_AW_AXP20
#ifdef POWER_OFF_AVDD25
	power_off_avdd25();
#endif
#ifdef POWER_OFF_VDDIO
	power_off_vddio();
#endif
#endif  /* CONFIG_AW_AXP20 */

#ifdef CONFIG_AML_PMU
    /*
     * close LDO3(2.8V) LDO4(1.8V) and LDO5(2.5V) at one time to save suspend/resume time
     */
    aml_pmu_power_ctrl(0,  
                      (1 << AML1212_POWER_LDO3_BIT) | 
                      (1 << AML1212_POWER_LDO4_BIT) | 
                      (1 << AML1212_POWER_LDO5_BIT));
    power_off_vcc33();
    power_off_vcck12();
#endif /* CONFIG_AML_PMU */
}

inline void power_on_at_32k_1()//need match the power_off_at_32k
{
    unsigned int    reg;
#ifdef CONFIG_AW_AXP20
#ifdef POWER_OFF_VDDIO
	power_on_vddio();
#endif
#ifdef POWER_OFF_AVDD25
	power_on_avdd25();
#endif
#endif

#ifdef CONFIG_AML_PMU
    power_on_vcc33();               // vcc3 must be open before core power

    /*
     * open LDO3(2.8V) LDO4(1.8V) and LDO5(2.5V) at one time to save suspend/resume time
     */
    aml_pmu_power_ctrl(1, 
                      (1 << AML1212_POWER_LDO3_BIT) | 
                      (1 << AML1212_POWER_LDO4_BIT) | 
                      (1 << AML1212_POWER_LDO5_BIT));
    power_on_vcck12();

#endif /* CONFIG_AML_PMU */
	reg  = readl(P_AO_I2C_M_0_CONTROL_REG);
	reg &= 0xFFC00FFF;
	reg |= (I2C_RESUME_SPEED << 12);
	writel(reg,P_AO_I2C_M_0_CONTROL_REG);
	udelay(100);
}

inline void power_off_at_32K_2()//If has nothing to do, just let null
{
#ifdef CONFIG_AW_AXP20
#if CONFIG_DCDC_PFM_PMW_SWITCH 
	dc_dc_pwm_switch(0);
#endif
#endif  /* CONFIG_AW_AXP20 */

#ifdef CONFIG_AML_PMU
    // add other code here
#endif /* CONFIG_AML_PMU */
}

inline void power_on_at_32k_2()//need match the power_off_at_32k
{
#ifdef CONFIG_AW_AXP20
#if CONFIG_DCDC_PFM_PMW_SWITCH 
	dc_dc_pwm_switch(1);
#endif
#endif  /* CONFIG_AW_AXP20 */

#ifdef CONFIG_AML_PMU
    // add other code here
#endif /* CONFIG_AML_PMU */
}

