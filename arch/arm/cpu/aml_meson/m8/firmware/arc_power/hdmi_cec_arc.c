/**************************************************
 *           HDMI CEC uboot code                  *
 *                                                *
 **************************************************/
#ifdef CONFIG_CEC_WAKEUP
#include <cec_tx_reg.h>
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef CONFIG_NO_32K_XTAL

static void gpiox_10_pin_mux_mask(void){
    //GPIOX_10 pin mux masked expect PWM_E
    writel(readl(P_PERIPHS_PIN_MUX_3) & (~(1<<22)), P_PERIPHS_PIN_MUX_3 ); //0x202f bit[22]: XTAL_32K_OUT 
    writel(readl(P_PERIPHS_PIN_MUX_3) & (~(1<< 8)), P_PERIPHS_PIN_MUX_3 ); //0x202f bit[ 8]: Tsin_D0_B
    writel(readl(P_PERIPHS_PIN_MUX_4) & (~(1<<23)), P_PERIPHS_PIN_MUX_4 ); //0x2030 bit[23]: SPI_MOSI
    writel(readl(P_PERIPHS_PIN_MUX_6) & (~(1<<17)), P_PERIPHS_PIN_MUX_6 ); //0x2032 bit[17]: UART_CTS_B
    writel(readl(P_PERIPHS_PIN_MUX_7) & (~(1<<31)), P_PERIPHS_PIN_MUX_7 ); //0x2033 bit[31]: PWM_VS
    writel(readl(P_PERIPHS_PIN_MUX_9) |   (1<<19) , P_PERIPHS_PIN_MUX_9 ); //0x2035 bit[19]: PWM_E
}

static void pwm_e_config(void){
    //PWM E config
    writel(0x25ff25fe, P_PWM_PWM_E ); //0x21b0 bit[31:16]: PWM_E_HIGH counter
                                      //0x21b0 bit[15: 0]: PWM_E_LOW counter
    writel(readl(P_PWM_MISC_REG_EF) |   ( 0x1<<15), P_PWM_MISC_REG_EF ); //0x21b2 bit[15]   : PWM_E_CLK_EN
    writel(readl(P_PWM_MISC_REG_EF) & (~(0x3f<<8)), P_PWM_MISC_REG_EF ); //0x21b2 bit[14: 8]: PWM_E_CLK_DIV
    writel(readl(P_PWM_MISC_REG_EF) |   ( 0x2<<4 ), P_PWM_MISC_REG_EF ); //0x21b2 bit[5 : 4]: PWM_E_CLK_SEL :0x2 sleect fclk_div4:637.5M
    writel(readl(P_PWM_MISC_REG_EF) |   ( 0x1<<0 ), P_PWM_MISC_REG_EF ); //0x21b2 bit[0]    : PWM_E_EN
}

void pwm_out_rtc_32k(void){
    gpiox_10_pin_mux_mask(); //enable PWM_E pin mux
    pwm_e_config();  //PWM E config   
    //f_serial_puts("Set PWM_E out put RTC 32K!\n");
}

#endif

int cec_strlen(char *p)
{
  int i=0;
  while(*p++)i++;
  return i;
}

void *cec_memcpy(void *memto, const void *memfrom, unsigned int size)
{
    if((memto == NULL) || (memfrom == NULL))
        return NULL;
    char *tempfrom = (char *)memfrom;
    char *tempto = (char *)memto;
    while(size -- > 0)
        *tempto++ = *tempfrom++;
    return memto;
}

#define waiting_aocec_free() \
        do{\
            unsigned long cnt = 0;\
            while(readl(P_AO_CEC_RW_REG) & (1<<23))\
            {\
                if(5000 == cnt++)\
                {\
                    break;\
                }\
            }\
        }while(0)

unsigned long cec_rd_reg(unsigned long addr)
{
    unsigned long data32;
    waiting_aocec_free();
    data32  = 0;
    data32 |= 0     << 16;  // [16]     cec_reg_wr
    data32 |= 0     << 8;   // [15:8]   cec_reg_wrdata
    data32 |= addr  << 0;   // [7:0]    cec_reg_addr
    writel(data32, P_AO_CEC_RW_REG);
    waiting_aocec_free();
    data32 = ((readl(P_AO_CEC_RW_REG)) >> 24) & 0xff;
    return (data32);
} /* cec_rd_reg */

void cec_wr_reg (unsigned long addr, unsigned long data)
{
    unsigned long data32;
    waiting_aocec_free();
    data32  = 0;
    data32 |= 1     << 16;  // [16]     cec_reg_wr
    data32 |= data  << 8;   // [15:8]   cec_reg_wrdata
    data32 |= addr  << 0;   // [7:0]    cec_reg_addr
    writel(data32, P_AO_CEC_RW_REG);
    waiting_aocec_free();
} /* aocec_wr_only_reg */

void cec_power_on(void)
{
	/*Enable GPIOD_5*/
	//writel((readl(CBUS_REG_ADDR(PREG_PAD_GPIO2_O)) | (1<<21)), CBUS_REG_ADDR(PREG_PAD_GPIO2_O));
	//writel((readl(CBUS_REG_ADDR(PREG_PAD_GPIO2_EN_N)) & (~(1<<21))), CBUS_REG_ADDR(PREG_PAD_GPIO2_EN_N));
	
	/*Enable cts_hdmi_sys_clk*/
	//writel(((readl(CBUS_REG_ADDR(HHI_HDMI_CLK_CNTL)) & (~((0x7<<9) | 0x7f))) | (1<<8)), CBUS_REG_ADDR(HHI_HDMI_CLK_CNTL));	
}

void cec_arbit_bit_time_set(unsigned bit_set, unsigned time_set){//11bit:bit[10:0]
    switch(bit_set){
    case 3:
        //3 bit
        cec_wr_reg(AO_CEC_TXTIME_4BIT_BIT7_0, time_set & 0xff);
        cec_wr_reg(AO_CEC_TXTIME_4BIT_BIT10_8, (time_set >> 8) & 0x7);
        break;
        //5 bit
    case 5:
        cec_wr_reg(AO_CEC_TXTIME_2BIT_BIT7_0, time_set & 0xff);
        cec_wr_reg(AO_CEC_TXTIME_2BIT_BIT10_8, (time_set >> 8) & 0x7);
        //7 bit
	case 7:
        cec_wr_reg(AO_CEC_TXTIME_17MS_BIT7_0, time_set & 0xff);
        cec_wr_reg(AO_CEC_TXTIME_17MS_BIT10_8, (time_set >> 8) & 0x7);
        break;
    default:
        break;
    }
}


void remote_cec_hw_reset(void)
{
#ifdef CONFIG_N200C_AOCEC_CRYSTAL_24M  
    //pwm_out_rtc_32k();  //enable RTC 32k
#endif

    //unsigned long data32;
    // Assert SW reset AO_CEC
    //data32  = 0;
    //data32 |= 0 << 1;   // [2:1]    cntl_clk: 0=Disable clk (Power-off mode); 1=Enable gated clock (Normal mode); 2=Enable free-run clk (Debug mode).
    //data32 |= 1 << 0;   // [0]      sw_reset: 1=Reset
    writel(0x1, P_AO_CEC_GEN_CNTL);
    // Enable gated clock (Normal mode).
    writel(readl(P_AO_CEC_GEN_CNTL) | (1<<1), P_AO_CEC_GEN_CNTL);
    // Release SW reset
    writel(readl(P_AO_CEC_GEN_CNTL) & ~(1<<0), P_AO_CEC_GEN_CNTL);

    // Enable all AO_CEC interrupt sources
    //writel(readl(P_AO_CEC_GEN_CNTL) | 0x6, P_AO_CEC_GEN_CNTL);
    //cec_wr_reg(CEC_CLOCK_DIV_H, 0x00 );
    //cec_wr_reg(CEC_CLOCK_DIV_L, 0x02 );
    cec_wr_reg(CEC_LOGICAL_ADDR0, (0x1 << 4) | (cec_msg.log_addr & 0xf));
    cec_rd_reg(CEC_LOGICAL_ADDR0);

    //Cec arbitration 3/5/7 bit time set.
    cec_arbit_bit_time_set(3, 0x118);
    cec_arbit_bit_time_set(5, 0x000);
    cec_arbit_bit_time_set(7, 0x2aa);   
}

unsigned char remote_cec_ll_rx(void)
{
    int i;
    unsigned char rx_msg_length = cec_rd_reg(CEC_RX_MSG_LENGTH) + 1;
       
    for (i = 0; i < rx_msg_length; i++) {
        cec_msg.buf[cec_msg.rx_write_pos].msg[i] = cec_rd_reg(CEC_RX_MSG_0_HEADER +i);                   
    }
  
    cec_wr_reg(CEC_RX_MSG_CMD,  RX_NO_OP);
    
    remote_cec_hw_reset();    
    return 0;
}
void cec_buf_clear(void)
{
    int i;
    
    for(i = 0; i < 16; i++)
        cec_msg.buf[cec_msg.rx_read_pos].msg[i] = 0;
}
int remote_cec_ll_tx(unsigned char *msg, unsigned char len)
{
    int i;
	//writel(P_AO_DEBUG_REG0, (readl(P_AO_DEBUG_REG0) | (1 << 4)));
	for (i = 0; i < len; i++) {
	    cec_wr_reg(CEC_TX_MSG_0_HEADER + i, msg[i]);
	}
	cec_wr_reg(CEC_TX_MSG_LENGTH, len-1);
	cec_wr_reg(CEC_TX_MSG_CMD, TX_REQ_CURRENT);//TX_REQ_NEXT
    return 0;
}

int ping_cec_ll_tx(unsigned char *msg, unsigned char len)
{
    int i;
    int ret = 0;
    unsigned int n = 15;

	for (i = 0; i < len; i++) {
	    cec_wr_reg(CEC_TX_MSG_0_HEADER + i, msg[i]);
	}
	cec_wr_reg(CEC_TX_MSG_LENGTH, len-1);
	cec_wr_reg(CEC_TX_MSG_CMD, TX_REQ_CURRENT);//TX_REQ_NEXT
	ret = cec_rd_reg(CEC_RX_MSG_STATUS); 
		
    while ( (n--) && (cec_rd_reg(CEC_TX_MSG_STATUS) != TX_DONE) ){     
        udelay__(2000);
    }

	if(cec_rd_reg(CEC_TX_MSG_STATUS) == TX_DONE){
        ret = TX_DONE;
	    cec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
	}

	if(cec_rd_reg(CEC_TX_MSG_STATUS) == TX_ERROR){
	    remote_cec_hw_reset();
	    ret = TX_ERROR;
	}
    return ret;
}

void cec_imageview_on(void)
{
    unsigned char msg[2];
  
    msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_TV_ADDR;
    msg[1] = CEC_OC_IMAGE_VIEW_ON;
    
    ping_cec_ll_tx(msg, 2);
}

void cec_report_physical_address(void)
{
    unsigned char msg[5];
    unsigned char phy_addr_ab = (readl(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = readl(P_AO_DEBUG_REG1) & 0xff;
          
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
            cec_imageview_on();
            if ((phy_addr_ab == cec_msg.buf[cec_msg.rx_read_pos].msg[2]) && (phy_addr_cd == cec_msg.buf[cec_msg.rx_read_pos].msg[3]) )  {    
                unsigned char msg[4];
                msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
                msg[1] = CEC_OC_ACTIVE_SOURCE;
                msg[2] = phy_addr_ab;
                msg[3] = phy_addr_cd;
               
                remote_cec_ll_tx(msg, 4);
            }
        }
    }    
}

void cec_device_vendor_id(void)
{
    unsigned char msg[5];
    
    //00-00-00   (hex) There is no init,need usrer set.
    msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_DEVICE_VENDOR_ID;
    msg[2] = 0x00;
    msg[3] = 0x00;
    msg[4] = 0x00;

    remote_cec_ll_tx(msg, 5);     
}

void cec_feature_abort(void)
{    
    if(cec_msg.buf[cec_msg.rx_read_pos].msg[1] != 0xf){
        unsigned char msg[4];
        
        msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_TV_ADDR;
        msg[1] = CEC_OC_FEATURE_ABORT;
        msg[2] = cec_msg.buf[cec_msg.rx_read_pos].msg[1];
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
    unsigned char msg[16];
    unsigned char osd_len = cec_strlen(CEC_OSD_NAME);
    
    msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_TV_ADDR;
    msg[1] = CEC_OC_SET_OSD_NAME;
    cec_memcpy(&msg[2], CEC_OSD_NAME, osd_len);
    
    remote_cec_ll_tx(msg, osd_len + 2);
}

void cec_get_version(void)
{
    unsigned char dest_log_addr = cec_msg.log_addr & 0xf;
    unsigned char msg[3];

    if (0xf != dest_log_addr) {
        msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_TV_ADDR;
        msg[1] = CEC_OC_CEC_VERSION;
        msg[2] = CEC_VERSION_14A;
        remote_cec_ll_tx(msg, 3);
    }
}

unsigned int cec_handle_message(void)
{
    unsigned char	opcode;
    //if( (readl(P_AO_DEBUG_REG0) & (1 << 4)) || (!cec_msg.buf[cec_msg.rx_read_pos].msg[0]) )
    //    return 1;
    opcode = cec_msg.buf[cec_msg.rx_read_pos].msg[1];
    
    // process messages from tv polling and cec devices 
    if((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1)
    {    
        switch (opcode) {
        case CEC_OC_GET_CEC_VERSION:
            cec_get_version();
            break;
        case CEC_OC_GIVE_DECK_STATUS:
            cec_give_deck_status();
            break;
        case CEC_OC_GIVE_PHYSICAL_ADDRESS:
            cec_report_physical_address();
            break;
        case CEC_OC_GIVE_DEVICE_VENDOR_ID:
            cec_device_vendor_id();
            break;
        case CEC_OC_GIVE_OSD_NAME:
            cec_set_osd_name();
            break;
        case CEC_OC_SET_STREAM_PATH:            
            cec_set_stream_path();
            break;
        case CEC_OC_GIVE_DEVICE_POWER_STATUS:
            cec_report_device_power_status();
            break;
        case CEC_OC_USER_CONTROL_PRESSED:
            if(((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1) && ((hdmi_cec_func_config>>AUTO_POWER_ON_MASK) & 0x1) && (0x40 == cec_msg.buf[cec_msg.rx_read_pos].msg[2]))
                cec_msg.cec_power = 0x1;
            break;
        case CEC_OC_MENU_REQUEST:
            cec_menu_status_smp();
            break;
        default:
            break;
        }

        (cec_msg.rx_read_pos == cec_msg.rx_buf_size - 1) ? (cec_msg.rx_read_pos = 0) : (cec_msg.rx_read_pos++);
        cec_buf_clear();

    }
    return 0;
}

unsigned int cec_handler(void)
{   
    switch (cec_rd_reg(CEC_RX_MSG_STATUS)){
    case RX_DONE:
        remote_cec_ll_rx();
        (cec_msg.rx_write_pos == cec_msg.rx_buf_size - 1) ? (cec_msg.rx_write_pos = 0) : (cec_msg.rx_write_pos++);
        break;
    case RX_ERROR:
        remote_cec_hw_reset();
        break;
    default:
        break;
    }

    if( cec_msg.rx_read_pos != cec_msg.rx_write_pos){
        
        cec_handle_message();
        
        switch(cec_rd_reg(CEC_TX_MSG_STATUS)){
        case TX_DONE:
            cec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
            //writel(P_AO_DEBUG_REG0, (readl(P_AO_DEBUG_REG0) & ~(1 << 4)));
            cec_buf_clear();
            (cec_msg.rx_read_pos == cec_msg.rx_buf_size - 1) ? (cec_msg.rx_read_pos = 0) : (cec_msg.rx_read_pos++);
            break;
        case TX_ERROR:
    	    remote_cec_hw_reset();
    	    //writel(P_AO_DEBUG_REG0, (readl(P_AO_DEBUG_REG0) & ~(1 << 4)));
    	    break;
    	 default:
    	    break;
    	}
    }

    return 0;
}

void cec_node_init(void)
{
	int i, bool = 0;
    unsigned char msg[1];
	enum _cec_log_dev_addr_e player_dev[3] = {   CEC_PLAYBACK_DEVICE_1_ADDR,
	    										 CEC_PLAYBACK_DEVICE_2_ADDR,
	    										 CEC_PLAYBACK_DEVICE_3_ADDR,
	    									  };
    repeat = 3;
    cec_buf_clear();
    cec_msg.rx_read_pos = 0;
    cec_msg.rx_write_pos = 0;
    cec_msg.rx_buf_size = 16;
    
    cec_msg.power_status = 1;
    cec_msg.len = 0; 
    cec_msg.cec_power = 0;  
    cec_msg.test = 0x0;   
    //msg[0] = 0x44;
    //ping_cec_ll_tx(msg, 1);
    //writel(P_AO_DEBUG_REG0, (readl(P_AO_DEBUG_REG0) & ~(1 << 4)));
	for(i = 0; i < 3; i++){
	    msg[0] = (player_dev[i]<<4) | player_dev[i];	     	
		//if(TX_DONE == ping_cec_ll_tx(msg, 1)) bool = 1;
		//else bool = 0;
		
		if(bool == 0){
		    // 0 means that no any respond
            // Set Physical address
            cec_wr_reg(CEC_LOGICAL_ADDR0, (0x1 << 4) | player_dev[i]);
            cec_msg.log_addr = player_dev[i];
   		    break;		
		}
	}
	remote_cec_hw_reset();
	//writel(P_AO_DEBUG_REG0, (readl(P_AO_DEBUG_REG0) & ~(1 << 4)));
}

#endif
