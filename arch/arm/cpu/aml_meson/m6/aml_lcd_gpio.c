#include <common.h>
#include <asm/arch/register.h>
#include <asm/arch/lcd_reg.h>
#include <asm/arch/aml_lcd_gpio.h>

static const char* aml_lcd_gpio_type_table[]={
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
	"GPIOE_0",
	"GPIOE_1",
	"GPIOE_2",
	"GPIOE_3",
	"GPIOE_4",
	"GPIOE_5",
	"GPIOE_6",
	"GPIOE_7",
	"GPIOE_8",
	"GPIOE_9",
	"GPIOE_10",
	"GPIOE_11",
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
	"GPIOX_22",
	"GPIOX_23",
	"GPIOX_24",
	"GPIOX_25",
	"GPIOX_26",
	"GPIOX_27",
	"GPIOX_28",
	"GPIOX_29",
	"GPIOX_30",
	"GPIOX_31",
	"GPIOX_32",
	"GPIOX_33",
	"GPIOX_34",
	"GPIOX_35",
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
	"GPIOD_0",
	"GPIOD_1",
	"GPIOD_2",
	"GPIOD_3",
	"GPIOD_4",
	"GPIOD_5",
	"GPIOD_6",
	"GPIOD_7",
	"GPIOD_8",
	"GPIOD_9",
	"GPIOC_0",
	"GPIOC_1",
	"GPIOC_2",
	"GPIOC_3",
	"GPIOC_4",
	"GPIOC_5",
	"GPIOC_6",
	"GPIOC_7",
	"GPIOC_8",
	"GPIOC_9",
	"GPIOC_10",
	"GPIOC_11",
	"GPIOC_12",
	"GPIOC_13",
	"GPIOC_14",
	"GPIOC_15",
	"CARD_0",
	"CARD_1",
	"CARD_2",
	"CARD_3",
	"CARD_4",
	"CARD_5",
	"CARD_6",
	"CARD_7",
	"CARD_8",
	"GPIOB_0",
	"GPIOB_1",
	"GPIOB_2",
	"GPIOB_3",
	"GPIOB_4",
	"GPIOB_5",
	"GPIOB_6",
	"GPIOB_7",
	"GPIOB_8",
	"GPIOB_9",
	"GPIOB_10",
	"GPIOB_11",
	"GPIOB_12",
	"GPIOB_13",
	"GPIOB_14",
	"GPIOB_15",
	"GPIOB_16",
	"GPIOB_17",
	"GPIOB_18",
	"GPIOB_19",
	"GPIOB_20",
	"GPIOB_21",
	"GPIOB_22",
	"GPIOB_23",
	"GPIOA_0",
	"GPIOA_1",
	"GPIOA_2",
	"GPIOA_3",
	"GPIOA_4",
	"GPIOA_5",
	"GPIOA_6",
	"GPIOA_7",
	"GPIOA_8",
	"GPIOA_9",
	"GPIOA_10",
	"GPIOA_11",
	"GPIOA_12",
	"GPIOA_13",
	"GPIOA_14",
	"GPIOA_15",
	"GPIOA_16",
	"GPIOA_17",
	"GPIOA_18",
	"GPIOA_19",
	"GPIOA_20",
	"GPIOA_21",
	"GPIOA_22",
	"GPIOA_23",
	"GPIOA_24",
	"GPIOA_25",
	"GPIOA_26",
	"GPIOA_27",
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
	
	if ((gpio>=GPIOZ_0) && (gpio<=GPIOZ_12)) {	//GPIOZ_0~12
		gpio_bit = gpio - GPIOZ_0 + 16;
		gpio_bank = PREG_PAD_GPIO6_EN_N;
	}
	else if ((gpio>=GPIOE_0) && (gpio<=GPIOE_11)) {	//GPIOE_0~11
		gpio_bit = gpio - GPIOE_0;
		gpio_bank = PREG_PAD_GPIO6_EN_N;
	}
	else if ((gpio>=GPIOY_0) && (gpio<=GPIOY_15)) {	//GPIOY_0~15
		gpio_bit = gpio - GPIOY_0;
		gpio_bank = PREG_PAD_GPIO5_EN_N;
	}
	else if ((gpio>=GPIOX_0) && (gpio<=GPIOX_31)) {	//GPIOX_0~31
		gpio_bit = gpio - GPIOX_0;
		gpio_bank = PREG_PAD_GPIO4_EN_N;
	}
	else if ((gpio>=GPIOX_32) && (gpio<=GPIOX_35)) {	//GPIOX_32~35
		gpio_bit = gpio - GPIOX_32 + 20;
		gpio_bank = PREG_PAD_GPIO3_EN_N;
	}
	else if ((gpio>=BOOT_0) && (gpio<=BOOT_17)) {	//BOOT_0~17
		gpio_bit = gpio - BOOT_0;
		gpio_bank = PREG_PAD_GPIO3_EN_N;
	}
	else if ((gpio>=GPIOD_0) && (gpio<=GPIOD_9)) {	//GPIOD_0~9
		gpio_bit = gpio - GPIOD_0 + 16;
		gpio_bank = PREG_PAD_GPIO2_EN_N;
	}
	else if ((gpio>=GPIOC_0) && (gpio<=GPIOC_15)) {	//GPIOC_0~15
		gpio_bit = gpio - GPIOC_0;
		gpio_bank = PREG_PAD_GPIO2_EN_N;
	}
	else if ((gpio>=CARD_0) && (gpio<=CARD_8)) {	//CARD_0~8
		gpio_bit = gpio - CARD_0 + 23;
		gpio_bank = PREG_PAD_GPIO5_EN_N;
	}
	else if ((gpio>=GPIOB_0) && (gpio<=GPIOB_23)) {	//GPIOB_0~23
		gpio_bit = gpio - GPIOB_0;
		gpio_bank = PREG_PAD_GPIO1_EN_N;
	}
	else if ((gpio>=GPIOA_0) && (gpio<=GPIOA_27)) {	//GPIOA_0~27
		gpio_bit = gpio - GPIOA_0;
		gpio_bank = PREG_PAD_GPIO0_EN_N;
	}
	else if ((gpio>=GPIOAO_0) && (gpio<=GPIOAO_11)) {	//GPIOAO_0~11
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
