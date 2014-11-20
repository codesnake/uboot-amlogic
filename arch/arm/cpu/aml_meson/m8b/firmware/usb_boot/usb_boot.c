#include <asm/arch/reboot.h>

#include "platform.h"
#include "usb_pcd.h"

#include "usb_pcd.c"
#include "platform.c"
#include "dwc_pcd.c"
#include "dwc_pcd_irq.c"
#include "burn_func.c"


int usb_boot(int clk_cfg)
{
	int cfg = INT_CLOCK;
	if(clk_cfg)
		cfg = EXT_CLOCK;
	set_usb_phy_config(cfg);

	usb_parameter_init();
		
	if(usb_pcd_init())
		return 0;

	while(1)
	{
		//watchdog_clear();		//Elvis Fool
		if(usb_pcd_irq())
			break;
	}
	return 0;
}


