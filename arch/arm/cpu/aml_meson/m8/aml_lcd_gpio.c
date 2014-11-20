#include <common.h>
#include <asm/arch/register.h>
#include <asm/arch/lcd_reg.h>
#include <asm/arch/aml_lcd_gpio.h>

static const char* aml_lcd_gpio_type_table[]={
	"GPIOAO_0",
	"GPIOAO_1",
	"GPIOAO_2",
	"GPIOAO_3",
	"GPIOAO_4",
	"GPIOAO_5",
	"GPIOAO_6",
	"GPIOAO_7",
	"GPIOAO_8",
	"GPIOAO_9",
	"GPIOAO_10",
	"GPIOAO_11",
	"GPIOAO_12",
	"GPIOAO_13",
	"GPIOZ_0",
	"GPIOZ_1",
	"GPIOZ_2",
	"GPIOZ_3",
	"GPIOZ_4",
	"GPIOZ_5",
	"GPIOZ_6",
	"GPIOZ_7",
	"GPIOZ_8",
	"GPIOZ_9",
	"GPIOZ_10",
	"GPIOZ_11",
	"GPIOZ_12",
	"GPIOZ_13",
	"GPIOZ_14",
	"GPIOH_0",
	"GPIOH_1",
	"GPIOH_2",
	"GPIOH_3",
	"GPIOH_4",
	"GPIOH_5",
	"GPIOH_6",
	"GPIOH_7",
	"GPIOH_8",
	"GPIOH_9",
	"BOOT_0",
	"BOOT_1",
	"BOOT_2",
	"BOOT_3",
	"BOOT_4",
	"BOOT_5",
	"BOOT_6",
	"BOOT_7",
	"BOOT_8",
	"BOOT_9",
	"BOOT_10",
	"BOOT_11",
	"BOOT_12",
	"BOOT_13",
	"BOOT_14",
	"BOOT_15",
	"BOOT_16",
	"BOOT_17",
	"BOOT_18",
	"CARD_0",
	"CARD_1",
	"CARD_2",
	"CARD_3",
	"CARD_4",
	"CARD_5",
	"CARD_6",
	"GPIODV_0",
	"GPIODV_1",
	"GPIODV_2",
	"GPIODV_3",
	"GPIODV_4",
	"GPIODV_5",
	"GPIODV_6",
	"GPIODV_7",
	"GPIODV_8",
	"GPIODV_9",
	"GPIODV_10",
	"GPIODV_11",
	"GPIODV_12",
	"GPIODV_13",
	"GPIODV_14",
	"GPIODV_15",
	"GPIODV_16",
	"GPIODV_17",
	"GPIODV_18",
	"GPIODV_19",
	"GPIODV_20",
	"GPIODV_21",
	"GPIODV_22",
	"GPIODV_23",
	"GPIODV_24",
	"GPIODV_25",
	"GPIODV_26",
	"GPIODV_27",
	"GPIODV_28",
	"GPIODV_29",
	"GPIOY_0",
	"GPIOY_1",
	"GPIOY_2",
	"GPIOY_3",
	"GPIOY_4",
	"GPIOY_5",
	"GPIOY_6",
	"GPIOY_7",
	"GPIOY_8",
	"GPIOY_9",
	"GPIOY_10",
	"GPIOY_11",
	"GPIOY_12",
	"GPIOY_13",
	"GPIOY_14",
	"GPIOY_15",
	"GPIOY_16",
	"GPIOX_0",
	"GPIOX_1",
	"GPIOX_2",
	"GPIOX_3",
	"GPIOX_4",
	"GPIOX_5",
	"GPIOX_6",
	"GPIOX_7",
	"GPIOX_8",
	"GPIOX_9",
	"GPIOX_10",
	"GPIOX_11",
	"GPIOX_12",
	"GPIOX_13",
	"GPIOX_14",
	"GPIOX_15",
	"GPIOX_16",
	"GPIOX_17",
	"GPIOX_18",
	"GPIOX_19",
	"GPIOX_20",
	"GPIOX_21",
	"GPIO_TEST_N",
	"GPIO_MAX",
}; 

int aml_lcd_gpio_name_map_num(const char *name)
{
	int i;
	
	for(i = 0; i < GPIO_MAX; i++) {
		if(!strcmp(name, aml_lcd_gpio_type_table[i]))
			break;
	}
	if (i == GPIO_MAX) {
		printf("wrong gpio name %s, i=%d\n", name, i);
		i = -1;
	}
	return i;
}

int aml_lcd_gpio_set(int gpio, int flag)
{
	int gpio_bank, gpio_bit;
	
	if ((gpio>=GPIOAO_0) && (gpio<=GPIOAO_13)) {
		printf("don't support GPIOAO Port yet\n");
		return -2;
	}
	else if ((gpio>=GPIOZ_0) && (gpio<=GPIOZ_14)) {
		gpio_bit = gpio - GPIOZ_0 + 17;
		gpio_bank = PREG_PAD_GPIO1_EN_N;	//0x200f
	}
	else if ((gpio>=GPIOH_0) && (gpio<=GPIOH_9)) {
		gpio_bit = gpio - GPIOH_0 + 19;
		gpio_bank = PREG_PAD_GPIO3_EN_N;	//0x2015
	}
	else if ((gpio>=BOOT_0) && (gpio<=BOOT_18)) {
		gpio_bit = gpio - BOOT_0;
		gpio_bank = PREG_PAD_GPIO3_EN_N;	//0x2015
	}
	else if ((gpio>=CARD_0) && (gpio<=CARD_6)) {
		gpio_bit = gpio - CARD_0 + 22;
		gpio_bank = PREG_PAD_GPIO0_EN_N;	//0x200c
	}
	else if ((gpio>=GPIODV_0) && (gpio<=GPIODV_29)) {
		gpio_bit = gpio - GPIODV_0;
		gpio_bank = PREG_PAD_GPIO2_EN_N;	//0x2012
	}
	else if ((gpio>=GPIOY_0) && (gpio<=GPIOY_16)) {
		gpio_bit = gpio - GPIOY_0;
		gpio_bank = PREG_PAD_GPIO1_EN_N;	//0x200f
	}
	else if ((gpio>=GPIOX_0) && (gpio<=GPIOX_21)) {
		gpio_bit = gpio - GPIOX_0;
		gpio_bank = PREG_PAD_GPIO0_EN_N;	//0x200c
	}
	else if (gpio==GPIO_TEST_N) {
		printf("don't support GPIOAO Port yet\n");
		return -2;
	}
	else {
		printf("Wrong GPIO Port number: %d\n", gpio);
		return -1;
	}
	
	if (flag == LCD_GPIO_OUTPUT_LOW) {
		WRITE_LCD_CBUS_REG_BITS(gpio_bank+1, 0, gpio_bit, 1);
		WRITE_LCD_CBUS_REG_BITS(gpio_bank, 0, gpio_bit, 1);
	}
	else if (flag == LCD_GPIO_OUTPUT_HIGH) {
		WRITE_LCD_CBUS_REG_BITS(gpio_bank+1, 1, gpio_bit, 1);
		WRITE_LCD_CBUS_REG_BITS(gpio_bank, 0, gpio_bit, 1);
	}
	else {
		WRITE_LCD_CBUS_REG_BITS(gpio_bank, 1, gpio_bit, 1);
	}
	return 0;
}
