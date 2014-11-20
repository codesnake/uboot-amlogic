/*Power manager for M6TV*/
//#include <asm/arch/io.h>
#include <asm/arch/uart.h>
#include <asm/arch/reg_addr.h>

extern void f_serial_puts(const char *s);
extern void wait_uart_empty();
extern void udelay(int i);

/*Inorder to save power for core1.1v, 
we need let A9 enter auto gate off & decrease core input clk*/
//unsigned int hhi_sys_cpu_clk_cntl;
void power_down_core(void)
{
	f_serial_puts("power down core.\n");
	wait_uart_empty();
	//Enable the auto clock gate off which be pre-setting @ os_api.c
//	writel(readl(0xc1100000 + 0x1078*4)| 1<<1,0xc1100000 + 0x1078*4);
//	udelay(1);
//	writel(readl(0xc1100000 + 0x1078*4)& (~(1<<1)),0xc1100000 + 0x1078*4);
	
	//Set A9 clk for 24M/128
//	hhi_sys_cpu_clk_cntl = readl(0xc1100000 + 0x1067*4);
//	writel(readl(0xc1100000 + 0x1067*4) & (~(3<<0)) | (3<<2) | (1<<7) | (0x3f<<8),0xc1100000 + 0x1067*4);
}

void power_up_core(void)
{
	f_serial_puts("power up core.\n");
	wait_uart_empty();
	//Just store P_HHI_SYS_CPU_CLK_CNTL. The A9 core will be reset later.
//	writel(hhi_sys_cpu_clk_cntl,0xc1100000 + 0x1067*4);
}


void power_off_at_24M()
{	
	power_down_core();
	
	writel(readl(P_AO_GPIO_O_EN_N) & (~(1<<25)),P_AO_GPIO_O_EN_N);
	udelay(200);
	writel(readl(P_AO_GPIO_O_EN_N) & (~(1<<9)),P_AO_GPIO_O_EN_N);
	udelay(200);
	writel(readl(P_AO_GPIO_O_EN_N) & (~(1<<21)),P_AO_GPIO_O_EN_N);
	udelay(200);
	writel(readl(P_AO_GPIO_O_EN_N) & (~(1<<5)),P_AO_GPIO_O_EN_N);
	
	/*power down EE 1.1v*/
	writel(readl(P_AO_GPIO_O_EN_N) & (~(1<<11)) & (~(1<<27)),P_AO_GPIO_O_EN_N);

}
void power_on_at_24M()
{
	/*power up EE 1.1v*/
	writel(readl(P_AO_GPIO_O_EN_N) | (1<<27),P_AO_GPIO_O_EN_N);
	
	writel(readl(P_AO_GPIO_O_EN_N) | (1<<21),P_AO_GPIO_O_EN_N);
	udelay(200);
	writel(readl(P_AO_GPIO_O_EN_N) | (1<<5),P_AO_GPIO_O_EN_N);
	udelay(24000000);
	writel(readl(P_AO_GPIO_O_EN_N) | (1<<25),P_AO_GPIO_O_EN_N);
	udelay(200);
	writel(readl(P_AO_GPIO_O_EN_N) | (1<<9),P_AO_GPIO_O_EN_N);
	
	power_up_core();
}

void power_off_at_32k()
{
}

void power_on_at_32k()
{
}

