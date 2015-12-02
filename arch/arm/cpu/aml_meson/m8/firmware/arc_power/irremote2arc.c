
#ifdef CONFIG_IR_REMOTE_WAKEUP

#define IR_POWER_KEY    0xe51afb04
#define IR_POWER_KEY_MASK 0xffffffff
unsigned int kk[] = {
        0xe51afb04,
};
#define IR_CONTROL_HOLD_LAST_KEY   (1<<6)
typedef struct reg_remote
{
        int reg;
        unsigned int val;
}reg_remote;

typedef enum
{
        DECODEMODE_NEC = 0,
        DECODEMODE_DUOKAN = 1,
        DECODEMODE_RCMM ,
        DECODEMODE_SONYSIRC,
        DECODEMODE_SKIPLEADER ,
        DECODEMODE_MITSUBISHI,
        DECODEMODE_THOMSON,
        DECODEMODE_TOSHIBA,
        DECODEMODE_RC5,
        DECODEMODE_RC6,
        DECODEMODE_COMCAST,
        DECODEMODE_SANYO,
        DECODEMODE_MAX
}ddmode_t;
#define CONFIG_END 0xffffffff
/*
  bit0 = 1120/31.25 = 36 
  bit1 = 2240 /31.25 = 72
  2500 /31.25  = 80 
  ldr_idle = 4500  /31.25 =144
  ldr active = 9000 /31.25 = 288 
*/
static const reg_remote RDECODEMODE_NEC[] ={
        {P_AO_MF_IR_DEC_LDR_ACTIVE,320<<16 |260<<0},
        {P_AO_MF_IR_DEC_LDR_IDLE, 200<<16 | 120<<0}, 
        {P_AO_MF_IR_DEC_LDR_REPEAT,100<<16 |70<<0},
        {P_AO_MF_IR_DEC_BIT_0,50<<16|20<<0 },
        {P_AO_MF_IR_DEC_REG0,3<<28|(0xFA0<<12)}, 
        {P_AO_MF_IR_DEC_STATUS,(100<<20)|(45<<10)},
        {P_AO_MF_IR_DEC_REG1,0x600fdf00},
        {P_AO_MF_IR_DEC_REG2,0x0},
        {P_AO_MF_IR_DEC_DURATN2,0},
        {P_AO_MF_IR_DEC_DURATN3,0},
        {CONFIG_END,            0 }
};
static const reg_remote RDECODEMODE_DUOKAN[] =
{
	{P_AO_MF_IR_DEC_LDR_ACTIVE,477<<16 | 400<<0}, // NEC leader 9500us,max 477: (477* timebase = 31.25) = 9540 ;min 400 = 8000us
	{P_AO_MF_IR_DEC_LDR_IDLE, 248<<16 | 202<<0}, // leader idle
	{P_AO_MF_IR_DEC_LDR_REPEAT,130<<16|110<<0},  // leader repeat
	{P_AO_MF_IR_DEC_BIT_0,60<<16|48<<0 }, // logic '0' or '00'
	{P_AO_MF_IR_DEC_REG0,3<<28|(0xFA0<<12)|0x13},  // sys clock boby time.base time = 20 body frame 108ms
	{P_AO_MF_IR_DEC_STATUS,(111<<20)|(100<<10)},  // logic '1' or '01'
	{P_AO_MF_IR_DEC_REG1,0x9f40}, // boby long decode (8-13)
	{P_AO_MF_IR_DEC_REG2,0x0},  // hard decode mode
	{P_AO_MF_IR_DEC_DURATN2,0},
	{P_AO_MF_IR_DEC_DURATN3,0},
	{CONFIG_END,            0      }
};

static const reg_remote *remoteregsTab[] =
{
	RDECODEMODE_NEC,
	RDECODEMODE_DUOKAN,
};
void setremotereg(const reg_remote *r)
{
	writel(r->val, r->reg);
}
int set_remote_mode(int mode){
	const reg_remote *reg;
	reg = remoteregsTab[mode];
	while(CONFIG_END != reg->reg)
		setremotereg(reg++);
	return 0;
}
unsigned backup_AO_RTI_PIN_MUX_REG;
unsigned backup_AO_IR_DEC_REG0;
unsigned backup_AO_IR_DEC_REG1;
unsigned backup_AO_IR_DEC_LDR_ACTIVE;
unsigned backup_AO_IR_DEC_LDR_IDLE;
unsigned backup_AO_IR_DEC_BIT_0;
unsigned bakeup_P_AO_IR_DEC_LDR_REPEAT;

/*****************************************************************
**
** func : ir_remote_init
**       in this function will do pin configuration and and initialize for
**       IR Remote hardware decoder mode at 32kHZ on ARC.
**
********************************************************************/

void backup_remote_register(void)
{
	backup_AO_RTI_PIN_MUX_REG = readl(P_AO_RTI_PIN_MUX_REG);
    backup_AO_IR_DEC_REG0 = readl(P_AO_MF_IR_DEC_REG0);
    backup_AO_IR_DEC_REG1 = readl(P_AO_MF_IR_DEC_REG1);
    backup_AO_IR_DEC_LDR_ACTIVE = readl(P_AO_MF_IR_DEC_LDR_ACTIVE);
    backup_AO_IR_DEC_LDR_IDLE = readl(P_AO_MF_IR_DEC_LDR_IDLE);
    backup_AO_IR_DEC_BIT_0 = readl(P_AO_MF_IR_DEC_BIT_0);
    bakeup_P_AO_IR_DEC_LDR_REPEAT = readl(P_AO_MF_IR_DEC_LDR_REPEAT);
}

void resume_remote_register(void)
{
	writel(backup_AO_RTI_PIN_MUX_REG,P_AO_RTI_PIN_MUX_REG);
	writel(backup_AO_IR_DEC_REG0,P_AO_MF_IR_DEC_REG0);
	writel(backup_AO_IR_DEC_REG1,P_AO_MF_IR_DEC_REG1);
	writel(backup_AO_IR_DEC_LDR_ACTIVE,P_AO_MF_IR_DEC_LDR_ACTIVE);
	writel(backup_AO_IR_DEC_LDR_IDLE,P_AO_MF_IR_DEC_LDR_IDLE);
	writel(backup_AO_IR_DEC_BIT_0,P_AO_MF_IR_DEC_BIT_0);
	writel(bakeup_P_AO_IR_DEC_LDR_REPEAT,P_AO_MF_IR_DEC_LDR_REPEAT);

	readl(P_AO_MF_IR_DEC_FRAME);//abandon last key
}

static int ir_remote_init_32k_mode(void)
{
    unsigned int status,data_value;
    int val = readl(P_AO_RTI_PIN_MUX_REG);
    writel((val  | (1<<0)), P_AO_RTI_PIN_MUX_REG);
    set_remote_mode(DECODEMODE_NEC);
    status = readl(P_AO_MF_IR_DEC_STATUS);
    data_value = readl(P_AO_MF_IR_DEC_FRAME);

    //step 2 : request nec_remote irq  & enable it
    return 0;
}

void init_custom_trigger(void)
{
	ir_remote_init_32k_mode();
}

int remote_detect_key(){
    unsigned power_key;
    if(((readl(P_AO_MF_IR_DEC_STATUS))>>3) & 0x1){
	power_key = readl(P_AO_MF_IR_DEC_FRAME);
	if((power_key&IR_POWER_KEY_MASK) == kk[DECODEMODE_NEC]){
	    return 1;
        }
    }
    return 0;
}
#endif
