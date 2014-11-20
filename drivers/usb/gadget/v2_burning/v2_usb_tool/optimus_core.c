#include <common.h>
#include "platform.h"
#include "usb_pcd.h"

#include "usb_pcd.c"
#include "platform.c"
#include "dwc_pcd.c"
#include "dwc_pcd_irq.c"

DECLARE_GLOBAL_DATA_PTR;

int v2_usbburning(unsigned timeout)
{
        int cfg = EXT_CLOCK;

#if defined(CONFIG_SILENT_CONSOLE)
        gd->flags &= ~GD_FLG_SILENT;
#endif

        printf("Enter v2 usbburning mode\n");
        set_usb_phy_config(cfg);

        usb_parameter_init(timeout);

        if(usb_pcd_init()) {
                printf("!!!!Fail in usb_pcd_init\n");
                return __LINE__;
        }

#if defined(CONFIG_AML_MESON_8)
        AML_WATCH_DOG_DISABLE(); //disable watchdog
#endif// #if defined(CONFIG_AML_MESON_8)

        while(1)
        {
                //watchdog_clear();		//Elvis Fool
                if(usb_pcd_irq())
                        break;
        }
        return 0;
}

int do_v2_usbtool (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rc = 0;
    unsigned timeout = (2 <= argc) ? simple_strtoul(argv[1], NULL, 0) : 0;

    optimus_work_mode_set(OPTIMUS_WORK_MODE_USB_UPDATE);

    rc = v2_usbburning(timeout);
    close_usb_phy_clock(0);

    return rc;
}


U_BOOT_CMD(
	update,	2,	0,	do_v2_usbtool,
	"Enter v2 usbburning mode",
	"usbburning timeout"
);

