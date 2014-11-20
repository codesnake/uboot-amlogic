

#ifdef CONFIG_IR_REMOTE_WAKEUP

#define IR_CONTROL_HOLD_LAST_KEY   (1<<6)

unsigned backup_AO_RTI_PIN_MUX_REG;
unsigned backup_AO_IR_DEC_REG0;
unsigned backup_AO_IR_DEC_REG1;
unsigned backup_AO_IR_DEC_LDR_ACTIVE;
unsigned backup_AO_IR_DEC_LDR_IDLE;
unsigned backup_AO_IR_DEC_BIT_0;

/*****************************************************************
**
** func : ir_remote_init
**       in this function will do pin configuration and and initialize for
**       IR Remote hardware decoder mode at 24MHZ on ARC.
**
********************************************************************/
#if 0
void udelay(int i)
{
	int delays = i ;//* 24;
	unsigned base= readl(P_AO_TIMERE_REG);
	//while(((readl(P_AO_TIMERE_REG)-base)&0xffffff) < (  delays&0xffffff)); //reg value is 24bit case
	while(((readl(P_AO_TIMERE_REG)-base)) < (  delays));
}
#endif
void backup_remote_register(void)
{
    backup_AO_RTI_PIN_MUX_REG = readl(P_AO_RTI_PIN_MUX_REG);
    backup_AO_IR_DEC_REG0 = readl(P_AO_IR_DEC_REG0);
    backup_AO_IR_DEC_REG1 = readl(P_AO_IR_DEC_REG1);
    backup_AO_IR_DEC_LDR_ACTIVE = readl(P_AO_IR_DEC_LDR_ACTIVE);
    backup_AO_IR_DEC_LDR_IDLE = readl(P_AO_IR_DEC_LDR_IDLE);
    backup_AO_IR_DEC_BIT_0 = readl(P_AO_IR_DEC_BIT_0);
}

void resume_remote_register(void)
{
	writel(backup_AO_RTI_PIN_MUX_REG,P_AO_RTI_PIN_MUX_REG);
	writel(backup_AO_IR_DEC_REG0,P_AO_IR_DEC_REG0);
	writel(backup_AO_IR_DEC_REG1,P_AO_IR_DEC_REG1);
	writel(backup_AO_IR_DEC_LDR_ACTIVE,P_AO_IR_DEC_LDR_ACTIVE);
	writel(backup_AO_IR_DEC_LDR_IDLE,P_AO_IR_DEC_LDR_IDLE);
	writel(backup_AO_IR_DEC_BIT_0,P_AO_IR_DEC_BIT_0);

	readl(P_AO_IR_DEC_FRAME);//abandon last key
}

static int ir_remote_init_32k_mode(void)
{
    unsigned int control_value,status,data_value;

    int val = readl(P_AO_RTI_PIN_MUX_REG);
		writel((val  | (1<<0)), P_AO_RTI_PIN_MUX_REG);
		
		control_value = 0x600ffe00;
    writel( control_value,P_AO_IR_DEC_REG1 );
    
    control_value = (0xa3f << 12) | 0;

    writel(control_value,P_AO_IR_DEC_REG0);
	
		// no filter
     writel((readl(P_AO_IR_DEC_REG0)& ~(0x7 << 28)) | (0 << 28),P_AO_IR_DEC_REG0);
    
    // SCALE LEADER ACTIVE
    writel((readl(P_AO_IR_DEC_LDR_ACTIVE)& ~(0x3FF << 16)) | ((unsigned)(294*1.1) << 16),P_AO_IR_DEC_LDR_ACTIVE);
    writel((readl(P_AO_IR_DEC_LDR_ACTIVE)& ~(0x3FF << 0)) | ((unsigned)(294*0.9) << 0),P_AO_IR_DEC_LDR_ACTIVE);

    // SCALE LEADER IDLE
    writel((readl(P_AO_IR_DEC_LDR_IDLE)& ~(0x3FF << 16)) | ((unsigned)(147*1.1) << 16),P_AO_IR_DEC_LDR_IDLE);
    writel((readl(P_AO_IR_DEC_LDR_IDLE)& ~(0x3FF << 0)) | ((unsigned)(147*0.9) << 16),P_AO_IR_DEC_LDR_IDLE);


    // SCALE BIT 0 (1.11mS)
    writel((readl(P_AO_IR_DEC_BIT_0)& ~(0x3FF << 16)) | ((unsigned)(36*1.1) << 16),P_AO_IR_DEC_BIT_0);
    writel((readl(P_AO_IR_DEC_BIT_0)& ~(0x3FF << 0))  | ((unsigned)(36*0.9) << 0),P_AO_IR_DEC_BIT_0);


    status = readl(P_AO_IR_DEC_STATUS);
    data_value = readl(P_AO_IR_DEC_FRAME);

    //step 2 : request nec_remote irq  & enable it
    return 0;
}

void init_custom_trigger(void)
{
	ir_remote_init_32k_mode();
}

#if 0
unsigned long cec_rd_reg(unsigned long addr)
{
    unsigned long data;

	writel(addr,P_HDMI_ADDR_PORT);
	writel(addr,P_HDMI_ADDR_PORT);
    data = readl(P_HDMI_DATA_PORT);
    
    return (data);
}

void cec_wr_reg(unsigned long addr, unsigned long data)
{    
    writel(addr, P_HDMI_ADDR_PORT);
    writel(addr, P_HDMI_ADDR_PORT);    
    writel(data, P_HDMI_DATA_PORT);
}

void cec_power_on(void)
{
	/*Enable GPIOD_5*/
	writel((readl(CBUS_REG_ADDR(PREG_PAD_GPIO2_O)) | (1<<21)), CBUS_REG_ADDR(PREG_PAD_GPIO2_O));
	writel((readl(CBUS_REG_ADDR(PREG_PAD_GPIO2_EN_N)) & (~(1<<21))), CBUS_REG_ADDR(PREG_PAD_GPIO2_EN_N));
	
	/*Enable cts_hdmi_sys_clk*/
	writel(((readl(CBUS_REG_ADDR(HHI_HDMI_CLK_CNTL)) & (~((0x7<<9) | 0x7f))) | (1<<8)), CBUS_REG_ADDR(HHI_HDMI_CLK_CNTL));	
}


void remote_cec_hw_reset(void)
{
    //unsigned char index = cec_global_info.my_node_index;
//#ifdef CONFIG_ARCH_MESON6
//    aml_write_reg32(APB_REG_ADDR(HDMI_CNTL_PORT), aml_read_reg32(APB_REG_ADDR(HDMI_CNTL_PORT))|(1<<16));
//#else 
//    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)|(1<<16));
//#endif
    //Enable HDMI Clock Gate 
    writel(readl(P_HHI_HDMI_CLK_CNTL) | (0x1<<8), P_HHI_HDMI_CLK_CNTL);
    writel(readl(P_HHI_GCLK_MPEG2) | (0x1<<4), P_HHI_GCLK_MPEG2);
    
    writel(readl(P_HDMI_CNTL_PORT) | (0x1<<15), P_HDMI_CNTL_PORT);//APB err_en    
    writel(readl(P_HDMI_CNTL_PORT) | (0x1<<16), P_HDMI_CNTL_PORT);//soft reset enable

    cec_wr_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL0, 0xc); //[3]cec_creg_sw_rst [2]cec_sys_sw_rst
    cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x1);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x1);
        
    //mdelay(10);
    //{//Delay some time
    //	int i = 10;
    //	while(i--);
    //}
    cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x0);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x0);
    cec_wr_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL0, 0x0);
//    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)&(~(1<<16)));
//#ifdef CONFIG_ARCH_MESON6
//    aml_write_reg32(APB_REG_ADDR(HDMI_CNTL_PORT), aml_read_reg32(APB_REG_ADDR(HDMI_CNTL_PORT))&(~(1<<16)));
//#else
    //WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)&(~(1<<16)));
    //WRITE_APB_REG(HDMI_DATA_PORT, READ_APB_REG(HDMI_DATA_PORT)|(1<<16));    
//#endif

    writel(P_HDMI_CNTL_PORT, readl(P_HDMI_CNTL_PORT) & (~(0x1<<16)));//soft reset disable
    
    cec_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_H, 0x00 );
    cec_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_L, 0xf0 );

    //cec_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | 0x4);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | (cec_msg.log_addr & 0xf));
    
}

void remote_cec_hw_off(void)
{
    //unsigned char index = cec_global_info.my_node_index;
//#ifdef CONFIG_ARCH_MESON6
//    aml_write_reg32(APB_REG_ADDR(HDMI_CNTL_PORT), aml_read_reg32(APB_REG_ADDR(HDMI_CNTL_PORT))|(1<<16));
//#else 
//    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)|(1<<16));
//#endif
    //enable HDMI Clock Gate 
    //clrbits_le32(P_HHI_SYS_CPU_CLK_CNTL, 1<<4); // disable APB_CLK
    writel(readl(P_HHI_HDMI_CLK_CNTL) | (0x1<<8), P_HHI_HDMI_CLK_CNTL);
    writel(readl(P_HHI_GCLK_MPEG2) | (0x1<<4), P_HHI_GCLK_MPEG2);
     
    writel(readl(P_HDMI_CNTL_PORT) | (0x1<<16), P_HDMI_CNTL_PORT);//soft reset enable
    writel(readl(P_HDMI_CNTL_PORT) | (0x1<<15), P_HDMI_CNTL_PORT);//APB err_en

    cec_wr_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL0, 0xc); //[3]cec_creg_sw_rst [2]cec_sys_sw_rst
    cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x1);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x1);
        
    //mdelay(10);
    //{//Delay some time
    //	int i = 10;
    //	while(i--);
    //}
    cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x0);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x0);
    cec_wr_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL0, 0x0);
//    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)&(~(1<<16)));
//#ifdef CONFIG_ARCH_MESON6
//    aml_write_reg32(APB_REG_ADDR(HDMI_CNTL_PORT), aml_read_reg32(APB_REG_ADDR(HDMI_CNTL_PORT))&(~(1<<16)));
//#else
    //WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)&(~(1<<16)));
    //WRITE_APB_REG(HDMI_DATA_PORT, READ_APB_REG(HDMI_DATA_PORT)|(1<<16));    
//#endif

    writel(P_HDMI_CNTL_PORT, readl(P_HDMI_CNTL_PORT) & (~(0x1<<16)));//soft reset disable
    
    cec_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_H, 0x00 );
    cec_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_L, 0x00 );

    //cec_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | 0x4);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, 0x00);
    //disable HDMI Clock Gate
    writel(readl(P_HHI_HDMI_CLK_CNTL)& ~(0x1<<8), P_HHI_HDMI_CLK_CNTL); 
    writel(readl(P_HHI_GCLK_MPEG2) & ~(0x1<<4), P_HHI_GCLK_MPEG2); 
    
}

//unsigned char remote_cec_ll_rx(unsigned char *msg, unsigned char *len)
unsigned char remote_cec_ll_rx(void)
{

    int i;
    unsigned char data = 0;
    unsigned char ret = 0;  
    unsigned int n = 0;
    unsigned char msg[16];


    unsigned char rx_msg_length = cec_rd_reg(CEC0_BASE_ADDR + CEC_RX_MSG_LENGTH) + 1;

    while (cec_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS) != RX_DONE){
        {//Delay some time
    	    int i = 10;
    	    while(i--);
        }
        n++;
        if(n >= 1000){
            break;
        }
    }
       
    for (i = 0; i < rx_msg_length; i++) {
        msg[i] = cec_rd_reg(CEC0_BASE_ADDR + CEC_RX_MSG_0_HEADER +i);
        //*msg = cec_rd_reg(CEC0_BASE_ADDR + CEC_RX_MSG_0_HEADER +i);
        //msg++;
        cec_msg.buf[i] = cec_rd_reg(CEC0_BASE_ADDR + CEC_RX_MSG_0_HEADER +i);
        //if(msg[i] == 0x44)
        //    cec_msg.test = 0x44;
                    
    }
    //*len = rx_msg_length;
    cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_NO_OP);
    //ret = cec_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS);
      
    //if(RX_DONE == cec_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS)){
    //    cec_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x1);
    //    {//Delay some time
    //	    int i = 10;
    //	    while(i--);
    //    }
    //    cec_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x0);
    //}
    //if((msg[0] == cec_msg.log_addr) && (msg[2] == 0x8f))
    if(msg[0] == cec_msg.log_addr)
        ret = 0x8f;
    remote_cec_hw_reset();    
    return ret;
}
void cec_buf_clear(void)
{
    int i;
    
    for(i = 0; i < 16; i++)
        cec_msg.buf[i] = 0;
}
int remote_cec_ll_tx(unsigned char *msg, unsigned char len)
{
    int i,j;
    int ret = 0;
    unsigned int n = 0;

    while (cec_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_BUSY){
        {//Delay some time
    	    int i = 10;
    	    while(i--);
        }
        n++;
        if(n >= 1000){
            break;
        }
    }
    for (j = 0; j < len; j++) {
        cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_0_HEADER + j, msg[j]);
    }
    cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_LENGTH, len-1);
    cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_REQ_CURRENT);//TX_REQ_NEXT
    //cec_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_REQ_CURRENT);//TX_REQ_NEXT
    ret = cec_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS); 
    
    if(cec_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_ERROR){
        remote_cec_hw_reset();
    }
          
    return ret;
}

void cec_report_physical_address(void)
{
    unsigned char msg[5];
    unsigned char phy_addr_ab = (readl(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = readl(P_AO_DEBUG_REG1) & 0xff;
    //unsigned char phy_addr_ab = 0x20;
    //unsigned char phy_addr_cd = 0x00;            
    msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_REPORT_PHYSICAL_ADDRESS;
    msg[2] = phy_addr_ab;
    msg[3] = phy_addr_cd;
    msg[4] = CEC_PLAYBACK_DEVICE_TYPE;                        
    
    remote_cec_ll_tx(msg, 5);        
}

void cec_report_device_power_status(void)
{
    unsigned char msg[3];
    
    msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_TV_ADDR;
    msg[1] = CEC_OC_REPORT_POWER_STATUS;
    msg[2] = cec_msg.power_status;
    
    remote_cec_ll_tx(msg, 3);
}

void cec_set_stream_path(void)
{
    unsigned char msg[4];
    
    unsigned char phy_addr_ab = (readl(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = readl(P_AO_DEBUG_REG1) & 0xff;
         
    if((hdmi_cec_func_config >> CEC_FUNC_MSAK) & 0x1){    
        if((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1)
        {    
            if ((phy_addr_ab == cec_msg.buf[2]) && (phy_addr_cd == cec_msg.buf[3]) )  {    
                unsigned char msg[4];
                msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
                msg[1] = CEC_OC_ACTIVE_SOURCE;
                msg[2] = phy_addr_ab;
                msg[3] = phy_addr_cd;
                //msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
                //msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;                
                remote_cec_ll_tx(msg, 4);
                //cec_msg.cec_power = 0x1;
            }
        }
    }    
}

//void cec_device_vendor_id(void)
//{
//    unsigned char msg[9];
//    //"PHILIPS"
//    msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
//    msg[1] = CEC_OC_DEVICE_VENDOR_ID;
//    msg[2] = 'P';
//    msg[3] = 'H';
//    msg[4] = 'I';
//    msg[5] = 'L';
//    msg[6] = 'I';
//    msg[7] = 'P';
//    msg[8] = 'S';
//
//    remote_cec_ll_tx(msg, 9);     
//}

void cec_feature_abort(void)
{    
    if(cec_msg.buf[1] != 0xf){
        unsigned char msg[4];
        
        msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_TV_ADDR;
        msg[1] = CEC_OC_FEATURE_ABORT;
        msg[2] = cec_msg.buf[1];
        msg[3] = CEC_UNRECONIZED_OPCODE;
        
        remote_cec_ll_tx(msg, 4);        
    }
}

void cec_menu_status_smp(void)
{
    unsigned char msg[3];
          
    msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_TV_ADDR;
    msg[1] = CEC_OC_MENU_STATUS;
    msg[2] = DEVICE_MENU_ACTIVE;

    remote_cec_ll_tx(msg, 3);     
}

void cec_give_deck_status(void)
{
    unsigned char msg[3];

    msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_TV_ADDR;
    msg[1] = CEC_OC_DECK_STATUS;
    msg[2] = 0x1a;        

    remote_cec_ll_tx(msg, 3);
}

void cec_set_osd_name(void)
{
    unsigned char msg[8];
    
    //"PHILIPS HMP8100"
    msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_TV_ADDR;
    msg[1] = CEC_OC_SET_OSD_NAME;
    msg[2] = 'H';
    msg[3] = 'M';
    msg[4] = 'P';
    msg[5] = '8';
    msg[6] = '1';
    msg[7] = '0';
    
    remote_cec_ll_tx(msg, 8);
}

//void register_cec_rx_msg(unsigned char *msg, unsigned char len )
void cec_handle_message(void)
{
    unsigned char	opcode;
        
    opcode = cec_msg.buf[1];
    
    // process messages from tv polling and cec devices 
    if((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1)
    {    
        switch (opcode) {
        //case CEC_OC_ACTIVE_SOURCE:
        //    //cec_active_source(pcec_message);
        //    //cec_deactive_source(pcec_message);
        //    break;
        //case CEC_OC_INACTIVE_SOURCE:
        //    //cec_deactive_source(pcec_message);
        //    break;
        //case CEC_OC_CEC_VERSION:
        //    cec_report_version(pcec_message);
        //    break;
        //case CEC_OC_DECK_STATUS:
        //    cec_deck_status(pcec_message);
        //    break;
        //case CEC_OC_DEVICE_VENDOR_ID:
        //    cec_device_vendor_id(pcec_message);
        //    break;
        //case CEC_OC_FEATURE_ABORT:
        //    cec_feature_abort(pcec_message);
        //    break;
        //case CEC_OC_GET_CEC_VERSION:
        //    cec_get_version(pcec_message);
        //    break;
        case CEC_OC_GIVE_DECK_STATUS:
            cec_give_deck_status();
            break;
        //case CEC_OC_MENU_STATUS:
        //    cec_menu_status(pcec_message);
        //    break;
        //case CEC_OC_REPORT_PHYSICAL_ADDRESS:
        //    cec_report_phy_addr(pcec_message);
        //    break;
        //case CEC_OC_REPORT_POWER_STATUS:
        //    cec_report_power_status(pcec_message);
        //    break;
        //case CEC_OC_SET_OSD_NAME:
        //    cec_set_osd_name(pcec_message);
        //    break;
        //case CEC_OC_VENDOR_COMMAND_WITH_ID:
        //    cec_vendor_cmd_with_id(pcec_message);
        //    break;
        //case CEC_OC_SET_MENU_LANGUAGE:
        //    cec_set_menu_language(pcec_message);
        //    break;
        case CEC_OC_GIVE_PHYSICAL_ADDRESS:
            cec_report_physical_address();
            break;
        //case CEC_OC_GIVE_DEVICE_VENDOR_ID:
        //     cec_device_vendor_id();
        //    //cec_give_device_vendor_id(pcec_message);
        //    cec_usrcmd_set_device_vendor_id();
            break;
        case CEC_OC_GIVE_OSD_NAME:
            cec_set_osd_name();
            break;
        //case CEC_OC_STANDBY:
        //	//printk("----cec_standby-----");
        //	cec_deactive_source(pcec_message);
        //    cec_standby(pcec_message);
        //    break;
        case CEC_OC_SET_STREAM_PATH:            
            cec_set_stream_path();
            break;
        //case CEC_OC_REQUEST_ACTIVE_SOURCE:
        //    //cec_request_active_source(pcec_message);
        //    cec_usrcmd_set_active_source();
        //    break;
        case CEC_OC_GIVE_DEVICE_POWER_STATUS:
            //cec_msg.cec_power = 0x1;
            cec_report_device_power_status();
            break;
        case CEC_OC_USER_CONTROL_PRESSED:
            if(((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1) && ((hdmi_cec_func_config>>AUTO_POWER_ON_MASK) & 0x1) && (0x40 == cec_msg.buf[2]))
                cec_msg.cec_power = 0x1;
            break;
        //case CEC_OC_USER_CONTROL_RELEASED:
        //    //printk("----cec_user_control_released----");
        //    //cec_user_control_released(pcec_message);
        //    break; 
        //case CEC_OC_IMAGE_VIEW_ON:      //not support in source
        //   cec_usrcmd_set_imageview_on( CEC_TV_ADDR );   // Wakeup TV
        //    break;  
        //case CEC_OC_ROUTING_CHANGE:
        //case CEC_OC_ROUTING_INFORMATION:    	
        //	cec_usrcmd_routing_information(pcec_message);	
        //	break;
        //case CEC_OC_GIVE_AUDIO_STATUS:   	  
        //	cec_report_audio_status();
        //	break;
        case CEC_OC_MENU_REQUEST:
            cec_menu_status_smp();
            break;
        default:
            cec_feature_abort();
            break;
        }
    }
}

unsigned int cec_handler(void)
{
    unsigned int data_msg_num, data_msg_stat;
    unsigned int ret = 0;


    data_msg_stat = cec_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS);
    if (data_msg_stat) {
        if ((data_msg_stat & 0x3) == RX_DONE) {
            data_msg_num = cec_rd_reg(CEC0_BASE_ADDR + CEC_RX_NUM_MSG);
            if (data_msg_num == 1) {
                unsigned char rx_msg[MAX_MSG], rx_len;                
                remote_cec_ll_rx();
                cec_handle_message();
                //remote_cec_ll_rx(rx_msg, &rx_len);
                //register_cec_rx_msg(rx_msg, rx_len);
            } else {
                cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x01);
                cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x00);
                cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_NO_OP);

            }
        } else {
            cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x01);
            cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x00);
            cec_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_NO_OP);
        }
    } 

    //remote_cec_hw_reset();
    //writel(readl(CBUS_REG_ADDR(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR) | (1 << 23)),CBUS_REG_ADDR(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR));             // Clear the interrupt

    return ret;
}


void cec_node_init(void)
{
	int i, bool = 0;
    unsigned char msg[1];
	enum _cec_log_dev_addr_e player_dev[3] = {   CEC_PLAYBACK_DEVICE_1_ADDR,
	    										 CEC_PLAYBACK_DEVICE_2_ADDR,
	    										 CEC_PLAYBACK_DEVICE_3_ADDR,
	    									  };
        
    // Clear CEC Int. state and set CEC Int. mask
    //WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR) | (1 << 23));    // Clear the interrupt
    //WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK) | (1 << 23));            // Enable the hdmi cec interrupt
     for (i = 0; i < 16; i++){
        cec_msg.buf[i] = 0;  
     }
     cec_msg.power_status = 1;
     cec_msg.len = 0; 
     cec_msg.cec_power = 0;  
     cec_msg.test = 0x0; 
        
	for(i = 0; i < 3; i++){
	    msg[0] = (player_dev[i]<<4) | player_dev[i];	     	
		if(TX_DONE == remote_cec_ll_tx(msg, 1)) bool = 1;
		else bool = 0;
		
		if(bool == 0){  // 0 means that no any respond
            // Set Physical address
            cec_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | player_dev[i]);
            cec_msg.log_addr = player_dev[i];
   		    break;		
		}
	}	
}
#endif
#endif
