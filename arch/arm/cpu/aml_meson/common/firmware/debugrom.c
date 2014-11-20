#define  CONFIG_AMLROM_SPL      1
#define  CONFIG_SPL_DEBUGROM    1
#include <uartpin.c>
#include <serial.c>
#include <pinmux.c>
#include <sdpinmux.c>
#include <timer.c>
#include <timming.c>
#include <memtest.c>
#include <pll.c>
#include <ddr.c>
#include <sdio.c>
#include <debug_rom.c>
#ifdef CONFIG_ACS
#include <storage.c>
#include <acs.c>
#endif

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
    //Adjust 1us timer base
    timer_init();

    serial_init(UART_CONTROL_SET(CONFIG_BAUDRATE,CONFIG_CRYSTAL_MHZ*1000000));
    serial_put_dword(get_utimer(0));
    AML_WATCH_DOG_DISABLE();//disable Watchdog
    debug_rom(__FILE__,__LINE__);
    return 0;
}
