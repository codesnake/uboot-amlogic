//#include <config.h>
#include <io.h>
#include <uart.h>
#include <reg_addr.h>
#include <ao_reg.h>
//#include <i2c.h>
#include <arc_pwr.h>

#define AML_I2C_CTRL_CLK_DELAY_MASK			0x3ff
#define AML_I2C_SLAVE_ADDR_MASK				0xff
#define AML_I2C_SLAVE_ADDR_MASK_7BIT   (0x7F)
#define AML_I2C_MAX_TOKENS		8
#define ETIMEDOUT 110
#define EIO       111

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

