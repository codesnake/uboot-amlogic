#ifndef _ARC_PWR_H_
#define _ARC_PWR_H_

/*Wake up source flag which be writen to AO_STATUS_REG2*/

#define FLAG_WAKEUP_PWRKEY      0x1234abcd //IR, power key, low power, adapter plug in/out and so on, are all use this flag. 
#define FLAG_WAKEUP_ALARM       0x12345678
#define FLAG_WAKEUP_WIFI        0x12340001
#define FLAG_WAKEUP_BT          0x12340002
#define FLAG_WAKEUP_PWROFF      0x12340003

struct arc_pwr_op
{
	void (*power_off_at_24M)(void);
	void (*power_on_at_24M)(void);

	void (*power_off_at_32K_1)(void);
	void (*power_on_at_32K_1)(void);
	void (*power_off_at_32K_2)(void);
	void (*power_on_at_32K_2)(void);

	void (*power_off_ddr15)(void);
	void (*power_on_ddr15)(void);

	void (*shut_down)(void);

	unsigned int (*detect_key)(unsigned int);
};

/*You can add param here for more */
struct ARC_PARAM
{
	unsigned int serial_disable; //0 enable output, 1 disable output
};

extern struct ARC_PARAM *arc_param;

extern void ddr_self_refresh(void);
extern void ddr_resume(void);

#define readl(a) (*(volatile unsigned int *)(a))
#define writel(v,a) (*(volatile unsigned int *)(a) = (v))

#define setbits_le32(reg,mask)  writel(readl(reg)|(mask),reg)
#define clrbits_le32(reg,mask)  writel(readl(reg)&(~(mask)),reg)
#define clrsetbits_le32(reg,clr,mask)  writel((readl(reg)&(~(clr)))|(mask),reg)
#define writel_reg_bits(reg, val, start, len) \
  writel(((readl(reg) & ~(((1L<<(len))-1)<<(start))) | ((unsigned int)(val) << (start))), reg)


#endif
