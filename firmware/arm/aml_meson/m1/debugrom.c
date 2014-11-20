#define FIRMWARE_IN_ONE_FILE
#define STATIC_PREFIX static inline
//#define CONFIG_ENABLE_MEM_DEVICE_TEST
#define AML_DEBUGROM
#include <uartpin.c>
#include <pinmux.c>
#include <serial.c>
#include <timer.c>
#include <timming.c>
#include <memtest.c>
#include <ddr.c>
#include <pll.c>

#include <sdio.c>
#include <spiwrite.c>
#include <debug_rom.c>


