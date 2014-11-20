/* usb pcd driver */
/*
 * (C) Copyright 2010 Amlogic, Inc
 *
 * Victor Wan, victor.wan@amlogic.com, 
 * 2010-03-24 @ Shanghai
 *
 */
#include "platform.h"
#include "usb_ch9.h"
#include "dwc_pcd.h"
#include "dwc_pcd_irq.h"
#include "usb_pcd.h"
#include "config.h"

#define DRIVER_VENDOR_ID	0x1B8E  //Amlogic's VerdorID
#define DRIVER_PRODUCT_ID	0xC003
#define DRIVER_VERSION       0x0100



#define STRING_MANUFACTURER	1
#define STRING_PRODUCT		2
#define STRING_SERIAL		3
#define STRING_CONFIG		4
#define STRING_INTERFACE	5
static const struct usb_device_descriptor
device_desc = {
	sizeof device_desc,		//__u8  bLength;
        USB_DT_DEVICE,			//__u8  bDescriptorType;
#ifdef USE_FULL_SPEED
        __constant_cpu_to_le16(0x0110),	//__u16 bcdUSB;
#else
	__constant_cpu_to_le16(0x0200),	//__u16 bcdUSB;
#endif
        USB_CLASS_PER_INTERFACE,	//__u8  bDeviceClass;
        0,				//__u8  bDeviceSubClass;
        0,				//__u8  bDeviceProtocol;
        64,				//__u8  bMaxPacketSize0;
        __constant_cpu_to_le16(DRIVER_VENDOR_ID),	//__u16 idVendor;
        __constant_cpu_to_le16(DRIVER_PRODUCT_ID),	//__u16 idProduct;
        __constant_cpu_to_le16(0x0007),	//__u16 bcdDevice;
        STRING_MANUFACTURER,		//__u8  iManufacturer;
        STRING_PRODUCT,			//__u8  iProduct;
        0,//STRING_SERIAL,			//__u8  iSerialNumber;
        1				//__u8  bNumConfigurations;
};
#define INTF_CONFIG_DESC_LEN  23
#define TOTAL_CONFIG_DESC_LEN	(INTF_CONFIG_DESC_LEN + USB_DT_CONFIG_SIZE)
static const struct usb_config_descriptor
config_desc = {
	USB_DT_CONFIG_SIZE,		//__u8  bLength;
        USB_DT_CONFIG,			//__u8  bDescriptorType;
        TOTAL_CONFIG_DESC_LEN,				//__u16 wTotalLength;
        1,				//__u8  bNumInterfaces;
        1,			//__u8  bConfigurationValue;
        0,//STRING_CONFIG,			//__u8  iConfiguration;
        USB_CONFIG_ATT_ONE | 
        USB_CONFIG_ATT_SELFPOWER,	//__u8  bmAttributes;
        1				//__u8  MaxPower;
};
/*
static struct usb_interface_descriptor
intf_desc = {
	9,//sizeof intf_desc,		//__u8  bLength;
	USB_DT_INTERFACE,		//__u8  bDescriptorType;
					//
	0,				//__u8  bInterfaceNumber;
	0,				//__u8  bAlternateSetting;
	2,				//__u8  bNumEndpoints;
	USB_CLASS_MASS_STORAGE,		//__u8  bInterfaceClass;
	USB_SC_SCSI,			//__u8  bInterfaceSubClass;
	USB_PR_BULK,			//__u8  bInterfaceProtocol;
	0//STRING_INTERFACE		//__u8  iInterface;
};*/
static const unsigned char intf_desc[INTF_CONFIG_DESC_LEN]={
	0x09,// length
	0x04,//USB_DT_INTERFACE
	0x00,//bInterfaceNumber
	0x00,//bAlternateSetting
	0x02,//bNumEndpoints
	0xFF,//bInterfaceClass, 0xFF =  USB_CLASS_VENDOR_SPEC
	0x00,//bInterfaceSubClass
	0x00,//bInterfaceProtocol
	0x00,//iInterface
	
	0x07,//Length
	0x05,//USB_DT_ENDPOINT
	0x80 | BULK_IN_EP_NUM,// 1 -- IN
	0x02,// Bulk
#ifndef USE_FULL_SPEED
	0x00,//
	0x02,// 64 bytes MPS
#else
	0x40,//
	0x00,// 64 bytes MPS
#endif
	0x00,
	
	0x07,//Length
	0x05,//USB_DT_ENDPOINT
	0x00 | BULK_OUT_EP_NUM,// 2 -- OUT
	0x02,// Bulk
#ifndef USE_FULL_SPEED
	0x00,//
	0x02,// 64 bytes MPS
#else
	0x40,//
	0x00,// 64 bytes MPS
#endif
	0x00	
};

#define DT_STRING_ID_LEN  4
static const  char dt_string_id[DT_STRING_ID_LEN]={
	DT_STRING_ID_LEN,
	USB_DT_STRING,
	0x09,
	0x04,
};
#define DT_STRING_VID_LEN 16
static const char dt_string_vid[DT_STRING_VID_LEN]={
	DT_STRING_VID_LEN,
	USB_DT_STRING,
	'A',
	0,
	'm',
	0,
	'l',
	0,
	'o',
	0,
	'g',
	0,
	'i',
	0,
	'c',
	0
};
#define DT_STRING_PID_LEN 16
static const char dt_string_pid[DT_STRING_PID_LEN]={
	DT_STRING_PID_LEN,
	USB_DT_STRING,
	'M',
	0,
	'3',
	0,
	'-',
	0,
	'C',
	0,
	'H',
	0,
	'I',
	0,
	'P',
	0
};
#if 0
#define DT_STRING_SERIAL_LEN	18
static const char dt_string_serial[DT_STRING_SERIAL_LEN]={
	DT_STRING_SERIAL_LEN,
	USB_DT_STRING,
	'2',
	0,
	'0',
	0,
	'1',
	0,
	'1',
	0,
	'0',
	0,
	'4',
	0,
	'0',
	0,
	'6',
	0
};
#endif
int usb_pcd_init(void)
{
	return dwc_core_init();
}

static unsigned int need_check_timeout;
static unsigned int time_out_val;

void usb_parameter_init(void)
{
	need_check_timeout = 0;
	/* clear utimer */
	need_check_timeout=get_timer(need_check_timeout)?get_timer(need_check_timeout):1;
	time_out_val = USB_ROM_CONN_TIMEOUT; // wait PC GetDescriptor command 
}

int usb_pcd_irq(void)
{
	
	if(need_check_timeout){
		if(get_timer(need_check_timeout) > time_out_val){
			dwc_otg_power_off_phy();
			return 2;// return to other device boot
		}
	}

	return dwc_otg_irq();
}

#define buff ((char*)PCD_BUFF)
void start_bulk_transfer(pcd_struct_t *_pcd)
{
	_pcd->bulk_lock = 1; // TODO : add lock code.
	dwc_otg_ep_req_start(_pcd,_pcd->bulk_out?BULK_OUT_EP_NUM:BULK_IN_EP_NUM);
}
/**
 * This functions delegates the setup command to the gadget driver.
 */
void do_gadget_setup( pcd_struct_t *_pcd, struct usb_ctrlrequest * ctrl)
{
	int			value;
	u16			w_index = ctrl->wIndex;
	u16			w_value = ctrl->wValue;
	u16			w_length = ctrl->wLength;
	
	/* usually this just stores reply data in the pre-allocated ep0 buffer,
	 * but config change events will also reconfigure hardware. */
	switch (ctrl->bRequest) {

	  case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
								USB_RECIP_DEVICE))
			break;
		//time_out_val = USB_ROM_DRIVER_TIMEOUT;// Wait SetConfig (PC install driver OK)
		need_check_timeout = 0;
		switch (w_value >> 8) {
			case USB_DT_DEVICE:
				USB_DBG("--get device descriptor\n\n");
				value = min(w_length, (u16) sizeof device_desc);
				usb_memcpy(buff, (char*)&device_desc, value);
				_pcd->buf = buff;
				_pcd->length = value;
				break;
			case USB_DT_DEVICE_QUALIFIER:
				USB_DBG("--get device qualifier\n\n");
				break;
			case USB_DT_OTHER_SPEED_CONFIG: 
				DBG("--get other speed configuration descriptor\n");
				DBG("--other speed configuration descriptor length :%d\n", value);
				break;
			case USB_DT_CONFIG:
				USB_DBG("--get configuration descriptor: size %d\n",w_length);
				if(w_length > USB_DT_CONFIG_SIZE)
				{					
					value = TOTAL_CONFIG_DESC_LEN;
					usb_memcpy(buff+USB_DT_CONFIG_SIZE,(void*)&intf_desc[0],INTF_CONFIG_DESC_LEN);
				}
				else
					value = w_length;
				usb_memcpy(buff, (void*)&config_desc, USB_DT_CONFIG_SIZE);
				_pcd->buf = buff;
				_pcd->length = value;
				USB_DBG("--configuration descriptor length :%d\n\n", value);			
				break;
			case USB_DT_STRING:
				USB_DBG("--get string descriptor: id: %d\n",w_value & 0xff);
				switch(w_value & 0xff){
					case 0: // IDs
						usb_memcpy(buff,(void*)dt_string_id,DT_STRING_ID_LEN);
						break;
					case 1: // STRING_MANUFACTURER
						usb_memcpy(buff,(void*)dt_string_vid,DT_STRING_VID_LEN);

						break;
					case 2://STRING_PRODUCT
						usb_memcpy(buff,(void*)dt_string_pid,DT_STRING_PID_LEN);
						break;
#if 0
					case 3://STRING_SERIAL
						usb_memcpy(buff,(void*)dt_string_serial,DT_STRING_SERIAL_LEN);
						break;
#endif
					default:
						USB_ERR("Error string id!\n");
						buff[0] = 0;
						break;	
					}
				_pcd->buf = buff;
				_pcd->length = buff[0];
				USB_DBG("--get string descriptor: return length: %d\n\n",_pcd->length);
				break;
			}
			break;

	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		USB_DBG("--set configuration\n\n");
		_pcd->buf = 0;
		_pcd->length = 0;
		_pcd->request_config = 1;   /* Configuration changed */
		//need_check_timeout = 0;
		break;
	
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		USB_DBG("--get configuration\n\n");
		buff[0] = 1;
		_pcd->buf = buff;
		_pcd->length = 1;
		break;

	default:
		USB_ERR("--unknown control req %02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	w_index = 0; //remove compile warning
	return ;
}
/**
 * This functions delegates vendor ctrl request.
 */
void do_vendor_request( pcd_struct_t *_pcd, struct usb_ctrlrequest * ctrl)
{
	int			value =0;
	u16			w_index = ctrl->wIndex;
	u16			w_value = ctrl->wValue;
	u16			w_length = ctrl->wLength;
	
	switch (ctrl->bRequest) {

	  case AM_REQ_WRITE_MEM:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
		USB_DBG("--am req write memory\n");
		value = (w_value << 16) + w_index;
		USB_DBG("addr = 0x%08X, size = %d\n\n",value,w_length);
		_pcd->buf = (char *)value; // copy to dst memory directly
		_pcd->length = w_length;
	  	break;

	  case AM_REQ_READ_MEM:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
		value = (w_value << 16) + w_index;

		usb_memcpy((char *)buff,(char*)value,w_length);
		
		_pcd->buf = buff;
		_pcd->length = w_length;
	  	break;

	  case AM_REQ_READ_AUX:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
		unsigned int data = 0;
		value = (w_value << 16) + w_index;

		//data = _lr(value);
		*(unsigned int *)buff = data;
		
		_pcd->buf = buff;
		_pcd->length = w_length;
	  	break;

	  case AM_REQ_FILL_MEM:
	  case AM_REQ_WRITE_AUX:
	  case AM_REQ_MODIFY_MEM:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
		_pcd->buf = buff;
		_pcd->length = w_length;
	  	break;

	  case AM_REQ_RUN_IN_ADDR:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
		value = (w_value << 16) + w_index;
		USB_DBG("--am req run in addr %p\n\n",value);		
		_pcd->buf = buff;
		_pcd->length = w_length;
	  	break;
		
	  case AM_REQ_WR_LARGE_MEM:
	  	value = 1;
	  case AM_REQ_RD_LARGE_MEM:
	  	USB_DBG("--am req large %s mem \n\n",value?"write":"read");

	  	_pcd->bulk_len = w_value;	// block length
	  	_pcd->bulk_num = w_index; // number of block
	  	_pcd->buf = buff;
	  	_pcd->length = w_length; 
		break;
		
	  case AM_REQ_IDENTIFY_HOST:
	  	buff[0] = USB_ROM_VER_MAJOR;
	  	buff[1] = USB_ROM_VER_MINOR;
	  	buff[2] = USB_ROM_STAGE_MAJOR;
	  	buff[3] = USB_ROM_STAGE_MINOR;
	  	_pcd->buf = buff;
	  	_pcd->length = w_length;
	  	need_check_timeout = 0;
		break;
	case AM_REQ_TPL_CMD:
	  case AM_REQ_TPL_STAT:
	  	serial_puts("w_length=");
		serial_put_hex(w_length, 32);
		_pcd->buf = buff;
		_pcd->length = w_length;
		break;
	  default:
		USB_ERR("--unknown vendor req %02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		break;
	}

	return;
}

extern int usb_run_command (const char *cmd, char *buffer);
/*
 * This function will be called after a whole SETUP-OUT-IN transfer.
 */
void do_vendor_out_complete( pcd_struct_t *_pcd, struct usb_ctrlrequest * ctrl)
{
	int			value = 0;
	u16			w_index = ctrl->wIndex;
	u16			w_value = ctrl->wValue;
	u16			w_length = ctrl->wLength;
	void (*fp)(void);
	char * buf; 

	//USB_DBG("do_vendor_out_complete()\n");
	switch (ctrl->bRequest) {

	  case AM_REQ_WRITE_MEM:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
			USB_DBG("--am req write memory completed\n\n");
	  	break;
		
	  case AM_REQ_FILL_MEM:
	  	buf = _pcd->buf;
		unsigned int addr,i;
		for(i = 0; i < _pcd->length; i+=8){
			addr = *((unsigned int *)&buf[i]) ;
			value = *((unsigned int *)&buf[i+4]) ;
			*(unsigned int*)addr = value;
		}
	  	break;

	  case AM_REQ_WRITE_AUX:
	  	buf = _pcd->buf;
		unsigned int data =0; 

		data = *((unsigned int *)&buf[0]) ; //reg value
		value = (w_value << 16) + w_index; //aux reg

		//_sr(data,value);
	  	break;	  	

	  case AM_REQ_MODIFY_MEM:
	  	do_modify_memory(w_value,_pcd->buf);
	  	break;		
		
	  case AM_REQ_RUN_IN_ADDR:
	  	if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_VENDOR |
				USB_RECIP_DEVICE))
				break;
	  	value = (w_value << 16) + w_index;
	  	USB_DBG("run addr = 0x%08X\n",value);
	  	fp = (void(*)(void))value;
	  	dwc_otg_power_off_phy();
	  	fp();
	  	break;

	  case AM_REQ_WR_LARGE_MEM:
	  	value = 1; // is_out = 1
	  case AM_REQ_RD_LARGE_MEM:
	  	_pcd->bulk_out = value; // read or write
	  	_pcd->bulk_buf = (char *)(*(unsigned int*)buff); // board address
	  	_pcd->bulk_data_len = (*(unsigned int*) &buff[4]); // data length
	  	start_bulk_transfer(_pcd);
	  	break;
	  case AM_REQ_TPL_CMD:
		  /* this is an example for any command */
		if(w_index == 1){/* assume subcode is 0 */
			char cmd[CMD_BUFF_SIZE];
			memcpy(cmd, buff, CMD_BUFF_SIZE);
			usb_run_command(cmd, buff);
		}
		break;
	  default:
		USB_ERR("--unknown vendor req comp %02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}
	w_length = 0;//remove compile warning
	return;
}
/*
 * This function will be called after a whole bulk out transfer.
 */
void do_bulk_complete( pcd_struct_t *_pcd)
{
	_pcd->bulk_lock = 0;
	_pcd->bulk_num--;
	_pcd->bulk_data_len -= _pcd->xfer_len;

	if(_pcd->bulk_num)
	{
		_pcd->bulk_buf += _pcd->bulk_len;
		start_bulk_transfer(_pcd);
	}
		
}

static void do_modify_memory(u16 opcode,char * inbuff)
{
	unsigned int *mem,*mem2;
	unsigned int data,mask;

	mem = *(unsigned int**)&inbuff[0];
	data = *(unsigned int*)&inbuff[4];
	mask = *(unsigned int*)&inbuff[8];
	mem2= *(unsigned int**)&inbuff[12];

	switch(opcode){
	  case 0: //*mem = data
	  	*mem = data;
		break;

	  case 1:// *mem = (data & mask)
	  	*mem = data & mask;
		break;

	  case 2:// *mem =(*mem | mask)
	  	*mem = *mem | mask;
		break;

	  case 3:// *mem = (data & (~mask))
	  	*mem =  (data & (~mask));
		break;

	  case 4:// *mem = (data & mask) |(*mem & ~mask)
	  	*mem = (data & mask) |(*mem & ~mask);
		break;	  

	  case 5:// *mem = *mem2
	  	*mem = *mem2;
		break;

	  case 6:// *mem = (*mem2 & mask)
	  	*mem = (*mem2 & mask);
		break;

	  case 7:// while(data--) {*mem++ = *mem2++}
	  	while(data--) {
			*mem++ = *mem2++;
		}
		break;
		
	  default:
	  	break;
	}
		
}
