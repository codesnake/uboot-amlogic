#include <config.h>
#include <asm/arch/romboot.h>
#include <asm/arch/timing.h>
#include <asm/arch/io.h>
#include <asm/arch/sdio.h>
#include "nfio.c"
//#include <asm/arch/firm/config.h>

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
#include <aml_a9_cache.c>
#endif //defined(CONFIG_AML_SPL_L1_CACHE_ON)

/**
**/
// for CMD_SEND
#define     repeat_package_times_bit  24
#define     use_int_window_bit        21
#define     use_int_window_bit        21
#define     cmd_send_data_bit         20
#define     check_busy_on_dat0_bit    19
#define     response_crc7_from_8_bit  18
#define     response_have_data_bit 17
#define     response_do_not_have_crc7_bit 16
#define     cmd_response_bits_bit 8
#define     cmd_command_bit       0
// for SDIO_CONFIG
#define     sdio_write_CRC_ok_status_bit 29
#define     sdio_write_Nwr_bit        23
#define     m_endian_bit              21
#define     bus_width_bit             20   // 0 1-bit, 1-4bits
#define     data_latch_at_negedge_bit 19
#define     response_latch_at_negedge_bit 18
#define     cmd_argument_bits_bit 12
#define     cmd_out_at_posedge_bit 11
#define     cmd_disable_CRC_bit   10
#define     cmd_clk_divide_bit    0

// SDIO_STATUS_IRQ
#define     sdio_timing_out_count_bit   19
#define     arc_timing_out_int_en_bit   18
#define     amrisc_timing_out_int_en_bit 17
#define     sdio_timing_out_int_bit      16
#define     sdio_status_info_bit        12
#define     sdio_set_soft_int_bit       11
#define     sdio_soft_int_bit           10
#define     sdio_cmd_int_bit             9
#define     sdio_if_int_bit              8
#define     sdio_data_write_crc16_ok_bit 7
#define     sdio_data_read_crc16_ok_bit  6
#define     sdio_cmd_crc7_ok_bit  5
#define     sdio_cmd_busy_bit     4
#define     sdio_status_bit       0

// SDIO_IRQ_CONFIG
#define     force_halt_bit           30
#define     sdio_force_read_back_bit 24
#define     disable_mem_halt_bit     22
#define     sdio_force_output_en_bit 16
#define     soft_reset_bit           15
#define     sdio_force_enable_bit    14
#define     sdio_force_read_data_bit  8
#define     sdio_if_int_config_bit 6
#define     arc_soft_int_en_bit    5
#define     arc_cmd_int_en_bit     4
#define     arc_if_int_en_bit      3
#define     amrisc_soft_int_en_bit 2
#define     amrisc_cmd_int_en_bit  1
#define     amrisc_if_int_en_bit   0


// for SDIO_MULT_CONFIG
#define     data_catch_finish_point_bit 16
#define     response_read_index_bit     12
#define     data_catch_readout_en_bit    9
#define     write_read_out_index_bit     8
#define     data_catch_level_bit   6
#define     stream_8_bits_mode_bit 5
#define     stream_enable_bit      4
#define     ms_sclk_always_bit     3
#define     ms_enable_bit_bit      2
#define     SDIO_port_sel_bit      0


#define ERROR_NONE                  0
#define ERROR_GO_IDLE1              1
#define ERROR_GO_IDLE2              2
#define ERROR_APP55_1               3
#define ERROR_ACMD41                4
#define ERROR_APP55_2               5
#define ERROR_VOLTAGE_VALIDATION    6
#define ERROR_SEND_CID1             7
#define ERROR_SEND_RELATIVE_ADDR    8
#define ERROR_SEND_CID2             9
#define ERROR_SELECT_CARD           10
#define ERROR_APP55_RETRY3          11
#define ERROR_SEND_SCR              12
#define ERROR_READ_BLOCK            13
#define ERROR_STOP_TRANSMISSION     14
#define ERROR_MAGIC_WORDS           15
#define ERROR_CMD1                  16
#define ERROR_MMC_SWITCH_BUS        17
#define ERROR_MMC_SWITCH_PART       18

//#define VOLTAGE_VALIDATION_RETRY    0x8000
//#define APP55_RETRY                 3

#define CARD_TYPE_SD		0
#define CARD_TYPE_SDHC		1
#define CARD_TYPE_MMC		2
#define CARD_TYPE_EMMC		3

/* 250ms of timeout */
#define CLK_DIV                     250  /* (200/((31+1)*2)=0.390625MHz),this define is for SD_CLK in Card Identification Stage */

#define VOLTAGE_VALIDATION_RETRY    0x8000
#define APP55_RETRY                 3

#define TIMEOUT_SHORT       (250*1000)  /* 250ms */
#define TIMEOUT_DATA        (300*1000)  /* 300ms (TIMEOUT_SHORT+ (READ_SIZE*8)/10000000) */
#ifndef CONFIG_SDIO_BUFFER_SIZE
#define CONFIG_SDIO_BUFFER_SIZE 64*1024
#endif
#define NO_DELAY_DATA 0

static inline int wait_busy(unsigned timeout)
{
    unsigned this_timeout = TIMERE_GET();
    unsigned r;
    WRITE_CBUS_REG(SDIO_STATUS_IRQ,0x1fff<<sdio_timing_out_count_bit);
    while (TIMERE_SUB(TIMERE_GET(),this_timeout)<timeout) {
        r = READ_CBUS_REG(SDIO_STATUS_IRQ);
        if ((r >> sdio_timing_out_count_bit) == 0)
            WRITE_CBUS_REG(SDIO_STATUS_IRQ,0x1fff<<sdio_timing_out_count_bit);
        if(((r >> sdio_cmd_busy_bit) & 0x1) == 0) {
            return 0;
        }
    }  
    return -1;          
}


static inline short sdio_send_cmd(unsigned cmd,unsigned arg,unsigned time,short err)
{
    writel(arg,P_CMD_ARGUMENT);
    writel(cmd,P_CMD_SEND);
    if (wait_busy(time))
    {
        return err;
    }
    return 0;
}
 
STATIC_PREFIX int sdio_read(unsigned target,unsigned size,unsigned por_sel)
{
   unsigned addr,cur_size,read_size;

   unsigned SD_boot_type;
   int error;
   unsigned cmd_clk_divide;
   unsigned arg, bus_width;
   cmd_clk_divide=__plls.sdio_cmd_clk_divide;
   SD_boot_type=sdio_get_port(por_sel);
   unsigned card_type=(romboot_info->ext>>4)&0xf;
   unsigned switch_status[16];

//register.h: #define SDIO_AHB_CBUS_CTRL 0x2318
//#define SDIO_AHB_CBUS_CTRL          (volatile unsigned long *)0xc1108c60   
#define SDIO_AHB_CBUS_CTRL_ADDR          (volatile unsigned long *)0xc1108c60   
    serial_put_dword(clk_util_clk_msr(7));
//    *SDIO_AHB_CBUS_CTRL&=~1;
    *SDIO_AHB_CBUS_CTRL_ADDR&=~1;
   //set clk to trnsfer clk rate and bus width
   WRITE_CBUS_REG(SDIO_CONFIG,(2 << sdio_write_CRC_ok_status_bit) |
                      (2 << sdio_write_Nwr_bit) |
                      (3 << m_endian_bit) |
                      (39 << cmd_argument_bits_bit) |
                      (0 << cmd_out_at_posedge_bit) |
                      (0 << cmd_disable_CRC_bit) |
                      (NO_DELAY_DATA << response_latch_at_negedge_bit) |
                      ((cmd_clk_divide-1)  << cmd_clk_divide_bit));

   WRITE_CBUS_REG(SDIO_MULT_CONFIG,SD_boot_type);
   __udelay(200);
   
   bus_width = 0;
    if((SD_boot_type != 2) || (card_type != CARD_TYPE_EMMC)){
	    serial_puts("check SD_boot_type:0x");
        serial_put_dec(SD_boot_type);
        serial_puts("\t card_type:0x");
        serial_put_dec(card_type);
        serial_puts("\n");
        goto DATA_READ;        
    }
    
#if 0    
    //send cmd6 to switch Highspeed
#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
        memset((unsigned char *)&switch_status, 0, 64);
        invalidate_dcache_range((unsigned char *)&switch_status, (unsigned char *)&switch_status+64);
#endif  
    
    WRITE_CBUS_REG(SDIO_M_ADDR, (unsigned char *)&switch_status);
    
    WRITE_CBUS_REG(SDIO_EXTENSION,(64*8 + (16 - 1)) << 16);    
    
    if(card_type == CARD_TYPE_EMMC){
        arg = ((0x3 << 24) |(185 << 16) |(1 << 8));  //emmc
    }
    else{
        arg = (1 << 31) | 0xffffff;
    	arg &= ~(0xf << (0 * 4));
    	arg |= 1 << (0 * 4);
    }
	
	//serial_puts("***CMD6 switch highspeed here\n");
	error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
	      (0 << repeat_package_times_bit) | // packet number
	      (1 << use_int_window_bit) | // will disable interrupt
          (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
          (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
          (1 << response_have_data_bit) | // RESPONSE has data receive
          (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
          ((0x40+6) << cmd_command_bit)  
			,arg
			,TIMEOUT_DATA
			,ERROR_MMC_SWITCH_BUS);
			
	if(error){
	    serial_puts("###CMD6 switch Highspeed failed error:0x");
        serial_put_dec(error);
        serial_puts("\n");
        goto DATA_READ;
	}	
    __udelay(200); 
#endif

    //switch width bus 4bit here
    if(card_type == CARD_TYPE_EMMC){
#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
        memset((unsigned char *)&switch_status, 0, 64);
        invalidate_dcache_range((unsigned char *)&switch_status, (unsigned char *)&switch_status+64);
#endif  
        
        WRITE_CBUS_REG(SDIO_M_ADDR, (unsigned char *)&switch_status);        
        WRITE_CBUS_REG(SDIO_EXTENSION,(64*8 + (16 - 1)) << 16);    
        
        arg = ((0x3 << 24) |(183 << 16) |(1 << 8));  
    	
    	//serial_puts("***CMD6 switch width here\n");
        //send cmd6 to switch Highspeed
    	error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
    	      (0 << repeat_package_times_bit) | // packet number
    	      (1 << use_int_window_bit) | // will disable interrupt
              (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
              (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
              (1 << response_have_data_bit) | // RESPONSE has data receive
              (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
              ((0x40+6) << cmd_command_bit)  
    			,arg
    			,TIMEOUT_DATA
    			,ERROR_MMC_SWITCH_BUS);
    			
    	if(error){
    	    serial_puts("###CMD6 switch Highspeed failed error:0x");
            serial_put_dec(error);
            serial_puts("\n");
            goto DATA_READ;
    	}	
        __udelay(5000); 
            
    }
    else{                    
        //send APP55 cmd 	

        WRITE_CBUS_REG(SDIO_EXTENSION, 0); 
    	arg = (0x1234)<<16;
    	    
        //serial_puts("***CMD55 app cmd here\n");    
    	error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
    	      (0 << repeat_package_times_bit) | // packet number
              (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
              (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
              (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
              ((0x40+55) << cmd_command_bit)  
    			,arg
    			,TIMEOUT_DATA
    			,ERROR_MMC_SWITCH_BUS);
    	
    	 //send ACMD6 to switch width 			
        WRITE_CBUS_REG(SDIO_EXTENSION, 0);             
    
    	arg = 2;
    	
        //serial_puts("***ACMD6 to switch width here\n");
    
    	error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
    	      (0 << repeat_package_times_bit) | // packet number
              (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
              (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
              (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
              ((0x40+6) << cmd_command_bit)  
    			,arg
    			,TIMEOUT_DATA
    			,ERROR_MMC_SWITCH_BUS);
    			
    	if(error){
    	    serial_puts("***ACMD6 switch width failed error:0x");
            serial_put_dec(error);
            serial_puts("\n");
            goto DATA_READ;
    	}
    }		
	
	
    setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
	writel(((1<<8) | (1<<9)),P_SDIO_STATUS_IRQ);    
    WRITE_CBUS_REG(SDIO_CONFIG,(2 << sdio_write_CRC_ok_status_bit) |
                      (2 << sdio_write_Nwr_bit) |
                      (3 << m_endian_bit) |
                      (39 << cmd_argument_bits_bit) |
                      (0 << cmd_out_at_posedge_bit) |
                      (1 << bus_width_bit) |                                          
                      (0 << cmd_disable_CRC_bit) |
                      (NO_DELAY_DATA << response_latch_at_negedge_bit) |
                      ((3)   << cmd_clk_divide_bit));	
                      
    WRITE_CBUS_REG(SDIO_MULT_CONFIG,SD_boot_type);
    bus_width = 1;	
   __udelay(5000);                   	
        
DATA_READ:   
   size=(size+511)&(~(511));
   for(addr=READ_SIZE,read_size=0;read_size<size;addr+=CONFIG_SDIO_BUFFER_SIZE)
   {
      
      cur_size=(size-read_size)>CONFIG_SDIO_BUFFER_SIZE?CONFIG_SDIO_BUFFER_SIZE:(size-read_size);

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
        invalidate_dcache_range(read_size+target,read_size+target+cur_size);
#endif  

      WRITE_CBUS_REG(SDIO_M_ADDR,read_size+target);
      if(bus_width == 0)
        WRITE_CBUS_REG(SDIO_EXTENSION,((1 << (9 + 3)) + (16 - 1)) << 16);
      else
        WRITE_CBUS_REG(SDIO_EXTENSION,((1 << (9 + 3)) + (16 - 1)*4) << 16);
        
      if((card_type==CARD_TYPE_SDHC)||(card_type==CARD_TYPE_EMMC))
            arg=addr>>9;
      else
        arg=addr;
      error=sdio_send_cmd((((cur_size >> 9) - 1) << repeat_package_times_bit) | // packet number
           (0 << cmd_send_data_bit) | // RESPONSE has CRC7
           (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
           (1 << use_int_window_bit) | // will disable interrupt
           (1 << response_have_data_bit) | // RESPONSE has data receive
           (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
           ((0x40+18) << cmd_command_bit),arg,TIMEOUT_SHORT,ERROR_READ_BLOCK);
           
    	if(error){
    	    serial_puts("CMD18 switch width failed error:0x");
            serial_put_dec(error);
            serial_puts("\n");
            return error;
    	}
//--------------------------------------------------------------------------------
// send CMD12 -- STOP_TRANSMISSION
      error=sdio_send_cmd((1 << check_busy_on_dat0_bit) | // RESPONSE is R1b
          (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
          (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
          (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
          ((0x40+12) << cmd_command_bit),0,TIMEOUT_DATA,ERROR_STOP_TRANSMISSION);
        if(error){
            serial_puts("CMD12 switch width failed error:0x");
            serial_put_dec(error);
            serial_puts("\n");
            return error;
        }

        read_size+=cur_size;
    }
    return 0;
    
}

STATIC_PREFIX short sdio_init(unsigned dev)
{
    unsigned SD_boot_type, temp;  // bits [9:8]
    unsigned card_type = CARD_TYPE_SD;
    short error;
    SD_boot_type=enable_sdio(dev);
    
    setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
    
    writel(  (2 << sdio_write_CRC_ok_status_bit) |
             (2 << sdio_write_Nwr_bit) |
             (3 << m_endian_bit) |
             (39 << cmd_argument_bits_bit) |
             (0 << cmd_out_at_posedge_bit) |
             (0 << cmd_disable_CRC_bit) |
             (NO_DELAY_DATA << response_latch_at_negedge_bit) |
             (CLK_DIV  << cmd_clk_divide_bit)
             , P_SDIO_CONFIG);

    writel(SD_boot_type,P_SDIO_MULT_CONFIG);

    unsigned cid;
    unsigned retry, retry_app_cmd;


    // SDIO communication
    //--------------------------------------------------------------------------------
    // send CMD0 -- GO_IDLE_STATE
    setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
    writel(((1<<8) | (1<<9)),P_SDIO_STATUS_IRQ);
    error=sdio_send_cmd(((0 << cmd_response_bits_bit) | // NO_RESPONSE
           (0x40 << cmd_command_bit)),0,TIMEOUT_SHORT,ERROR_GO_IDLE1);
    if(error)
        return error;
    writel(0,P_CMD_ARGUMENT);// delay some time for sdio_card ready
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    writel(0,P_CMD_ARGUMENT);
    
    //distinguish sd sdhc card
    error=sdio_send_cmd(((0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
           (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
           ((0x40+8) << cmd_command_bit)),0x1aa,TIMEOUT_SHORT,-1);
    if (error == 0)
    {
        writel(1<<8,P_SDIO_MULT_CONFIG);
        if((readl(P_CMD_ARGUMENT)&0x1ff) == 0x1aa)
            card_type = CARD_TYPE_SDHC;
        else
            card_type = CARD_TYPE_SD;
    }
    else
    {
        //--------------------------------------------------------------------------------
        // send CMD0 -- GO_IDLE_STATE
        setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
        writel(((1<<8) | (1<<9)),P_SDIO_STATUS_IRQ);
         error=sdio_send_cmd(((0 << cmd_response_bits_bit) | // NO_RESPONSE
           (0x40 << cmd_command_bit)),0,TIMEOUT_SHORT,ERROR_GO_IDLE2);
        if(error)
            return error;
    }

    //--------------------------------------------------------------------------------
    // Voltage validation
    retry = 0;
    retry_app_cmd = 0;
    while (retry < APP55_RETRY) {
        
        //--------------------------------------------------------------------------------
        // send CMD55 -- APP_CMD
         error=sdio_send_cmd(((0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
                (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                ((0x40+55) << cmd_command_bit)),0,TIMEOUT_SHORT,-1);
    
        if (error == 0) {
            break;
        }

        retry++;
        retry_app_cmd++;
        //--------------------------------------------------------------------------------
        // send CMD0 -- GO_IDLE_STATE
        setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
        writel(((1<<8) | (1<<9)),P_SDIO_STATUS_IRQ);
        error=sdio_send_cmd(((0 << cmd_response_bits_bit) | // NO_RESPONSE
           (0x40 << cmd_command_bit)),0,TIMEOUT_SHORT,ERROR_GO_IDLE1);
        if(error)
            return error;
    }
    unsigned time_cal=get_utimer(0);
    while (retry < VOLTAGE_VALIDATION_RETRY) {
        if(retry_app_cmd >= APP55_RETRY)
        {
            //--------------------------------------------------------------------------------
            // send CMD1 -- MMC_SEND_OP_COND for voltage validation
            setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
            writel(((1<<8) | (1<<9)),P_SDIO_STATUS_IRQ);

            error=sdio_send_cmd(((1 << response_do_not_have_crc7_bit) |
                    (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                    ((0x40+1) << cmd_command_bit)
                    ),0x40FF8000,TIMEOUT_SHORT,ERROR_CMD1);
            if(error)
                return error;
            writel(1<<8,P_SDIO_MULT_CONFIG);
//            *SDIO_MULT_CONFIG = 1<<8;
            temp = readl(P_CMD_ARGUMENT);
            if (temp & (1 << 31)) {
                if((temp & (1 << 30)))
                    card_type = CARD_TYPE_EMMC;
                else
                    card_type = CARD_TYPE_MMC;
                break;
            }

            retry++;
            continue;
        }

        //--------------------------------------------------------------------------------
        // send ACMD41 -- SD_SEND_OP_COND for voltage validation
        writel(0x8300,P_SDIO_EXTENSION);
        if(card_type == CARD_TYPE_SDHC)
            error=sdio_send_cmd(((1 << response_do_not_have_crc7_bit) |
                (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                ((0x40+41) << cmd_command_bit)
               ),0x40200000,TIMEOUT_SHORT,ERROR_ACMD41);
        else
            error=sdio_send_cmd(((1 << response_do_not_have_crc7_bit) |
                (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                ((0x40+41) << cmd_command_bit)
               ),0x200000,TIMEOUT_SHORT,ERROR_ACMD41);
        
        if(error)
            return error;
        /* check response */
        writel(1<<8,P_SDIO_MULT_CONFIG);
        temp = readl(P_CMD_ARGUMENT);
        if (temp & (1 << 31)) {
            if(!(temp & (1 << 30)))
                card_type = CARD_TYPE_SD;
            break;
        }

        //--------------------------------------------------------------------------------
        // send CMD55 -- APP_CMD
        error=sdio_send_cmd(((0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
                        (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                        ((0x40+55) << cmd_command_bit)),0,TIMEOUT_SHORT,ERROR_APP55_2);
        if(error)
            return error;
        retry++;
    }
    serial_put_dword(retry);
    serial_put_dword(get_utimer(time_cal));
    if (retry >= VOLTAGE_VALIDATION_RETRY) {
        return ERROR_VOLTAGE_VALIDATION;
    }

    //--------------------------------------------------------------------------------
    // send CMD2 -- ALL_SEND_cid
    error=sdio_send_cmd((
        (1 << response_crc7_from_8_bit) | // RESPONSE CRC7 is internal
        (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
        (133 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+120(respnse)+7(crc)-1 data
        ((0x40+2) << cmd_command_bit)
        ),0,TIMEOUT_SHORT,ERROR_SEND_CID1);
        if(error)
            return error;
    //--------------------------------------------------------------------------------
    // send CMD3 -- SEND_RELATIVE_ADDR
    error=sdio_send_cmd((
        (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
        (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
        ((0x40+3) << cmd_command_bit)
       ),0,TIMEOUT_SHORT,ERROR_SEND_RELATIVE_ADDR);
        if(error)
            return error;
    writel(1<<8,P_SDIO_MULT_CONFIG);
    if((card_type == CARD_TYPE_MMC) || (card_type == CARD_TYPE_EMMC))
        cid = 1<<16;
    else
        cid = readl(P_CMD_ARGUMENT) ;

    //--------------------------------------------------------------------------------
    // send CMD9 -- SEND_cid
    writel(1<<8,P_SDIO_MULT_CONFIG);
    error=sdio_send_cmd((
        (1 << response_crc7_from_8_bit) | // RESPONSE CRC7 is internal
        (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
        (133 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+120(respnse)+7(crc)-1 data
        ((0x40+9) << cmd_command_bit)
       ),cid,TIMEOUT_SHORT,ERROR_SEND_CID2);
        if(error)
            return error;
    writel(1<<8,P_SDIO_MULT_CONFIG);

    //--------------------------------------------------------------------------------
    // send CMD7 -- SELECT/DESELECT_CARD

    error=sdio_send_cmd((
        (1 << check_busy_on_dat0_bit) | // RESPONSE is R1b
        (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
        (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
        (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
        ((0x40+7) << cmd_command_bit)
       ),cid,TIMEOUT_SHORT,ERROR_SELECT_CARD);
        if(error)
            return error;
    if((card_type != CARD_TYPE_MMC) && (card_type != CARD_TYPE_EMMC))
    {
        //-----------------------------------------------------------------<a name=""></a>---------------
        // send CMD55 -- APP_CMD
        error=sdio_send_cmd((
                    (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
                    (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                    ((0x40+55) << cmd_command_bit)
                  ),cid,TIMEOUT_SHORT,ERROR_APP55_RETRY3);
        if(error)
            return error;
        //--------------------------------------------------------------------------------
        // send ACMD51 -- SEND_SCR
        writel(0x80000000,P_SDIO_M_ADDR);
        writel(79<<16,P_SDIO_EXTENSION);

        error=sdio_send_cmd(
            (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
            (1 << use_int_window_bit) | // will disable interrupt
            (1 << response_have_data_bit) | // RESPONSE has information on data
            (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
            ((0x40+51) << cmd_command_bit),cid,TIMEOUT_SHORT,ERROR_SEND_SCR);
        if(error)
            return error;
    }

    romboot_info->ext|=(romboot_info->ext&(0xf<<4))|((card_type&0xf)<<4);
    return ERROR_NONE;
}



#ifdef CONFIG_MESON_TRUSTZONE

#define print(format)  serial_puts(format)

#define MMC_DATA_BUF1   0x12000000
#define MMC_DATA_BUF2    0x13000000

#define SIZE_1M   0x100000
#define SIZE_1K 	1024

#define     MMC_BOOT_DEVICE_SIZE            (0x4*SIZE_1M)
#define     MMC_BOOT_PARTITION_RESERVED     (32*SIZE_1M) // 32MB

#define READ_PORT 		(POR_2ND_SDIO_C<<2)

#define MMC_STORAGE_MAGIC		"mmc_storage"
#define MMC_STORAGE_AREA_SIZE		(256*1024)
#define MMC_STORAGE_AREA_COUNT	 2
#define MMC_STORAGE_OFFSET		(MMC_BOOT_DEVICE_SIZE+MMC_BOOT_PARTITION_RESERVED \
								+0x200000)

#define MMC_STORAGE_MAGIC_SIZE	16
#define MMC_STORAGE_AREA_HEAD_SIZE	(MMC_STORAGE_MAGIC_SIZE+4*6)
#define MMC_STORAGE_AREA_VALID_SIZE	(MMC_STORAGE_AREA_SIZE-MMC_STORAGE_AREA_HEAD_SIZE)

#define MMC_STORAGE_DEFAULT_ADDR	 0x061e0000

#define MMC_STORAGE_DEFAULT_SIZE	(128*1024)

#define MMC_STORAGE_MEM_ADDR		MMC_STORAGE_DEFAULT_ADDR 
#define MMC_STORAGE_MEM_SIZE		MMC_STORAGE_DEFAULT_SIZE

struct mmc_storage_head_t{
	unsigned char magic[MMC_STORAGE_MAGIC_SIZE];
	unsigned int magic_checksum;
	unsigned int version;
	unsigned int tag;
	unsigned int checksum; //data checksum
	unsigned int timestamp;
	unsigned int reserve;
	unsigned int data[MMC_STORAGE_AREA_VALID_SIZE];
};

STATIC_PREFIX unsigned int mmckey_calculate_checksum(unsigned char *buf,unsigned int lenth)
{
	unsigned int checksum = 0;
	unsigned int cnt;
	for(cnt=0;cnt<lenth;cnt++){
		checksum += buf[cnt];
	}
	return checksum;
}

STATIC_PREFIX int sdio_read_off_size(unsigned  target,unsigned off, unsigned size, unsigned por_sel)
{
   unsigned addr,cur_size,read_size;

   unsigned SD_boot_type;
   int error;
   unsigned cmd_clk_divide;
   unsigned arg, bus_width;
   cmd_clk_divide=__plls.sdio_cmd_clk_divide;
   SD_boot_type=sdio_get_port(por_sel);
   unsigned card_type=(romboot_info->ext>>4)&0xf;
   unsigned switch_status[16];

//register.h: #define SDIO_AHB_CBUS_CTRL 0x2318
//#define SDIO_AHB_CBUS_CTRL		  (volatile unsigned long *)0xc1108c60	 
#define SDIO_AHB_CBUS_CTRL_ADDR          (volatile unsigned long *)0xc1108c60   
	serial_put_dword(clk_util_clk_msr(7));
//	  *SDIO_AHB_CBUS_CTRL&=~1;
	*SDIO_AHB_CBUS_CTRL_ADDR&=~1;
   //set clk to trnsfer clk rate and bus width
   WRITE_CBUS_REG(SDIO_CONFIG,(2 << sdio_write_CRC_ok_status_bit) |
					  (2 << sdio_write_Nwr_bit) |
					  (3 << m_endian_bit) |
					  (39 << cmd_argument_bits_bit) |
					  (0 << cmd_out_at_posedge_bit) |
					  (0 << cmd_disable_CRC_bit) |
					  (NO_DELAY_DATA << response_latch_at_negedge_bit) |
					  ((cmd_clk_divide-1)  << cmd_clk_divide_bit));

   WRITE_CBUS_REG(SDIO_MULT_CONFIG,SD_boot_type);
   __udelay(200);
   
   bus_width = 0;
	if((SD_boot_type != 2) || (card_type != CARD_TYPE_EMMC)){
		serial_puts("check SD_boot_type:0x");
		serial_put_dec(SD_boot_type);
		serial_puts("\t card_type:0x");
		serial_put_dec(card_type);
		serial_puts("\n");
		goto DATA_READ; 	   
	}
	
	//switch width bus 4bit here
	if(card_type == CARD_TYPE_EMMC){
#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
		memset((unsigned char *)&switch_status, 0, 64);
		invalidate_dcache_range((unsigned char *)&switch_status, (unsigned char *)&switch_status+64);
#endif  
		
		WRITE_CBUS_REG(SDIO_M_ADDR, (unsigned char *)&switch_status);		 
		WRITE_CBUS_REG(SDIO_EXTENSION,(64*8 + (16 - 1)) << 16);    
		
		arg = ((0x3 << 24) |(183 << 16) |(1 << 8));  
		
		//serial_puts("***CMD6 switch width here\n");
		//send cmd6 to switch Highspeed
		error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
			  (0 << repeat_package_times_bit) | // packet number
			  (1 << use_int_window_bit) | // will disable interrupt
			  (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
			  (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
			  (1 << response_have_data_bit) | // RESPONSE has data receive
			  (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
			  ((0x40+6) << cmd_command_bit)  
				,arg
				,TIMEOUT_DATA
				,ERROR_MMC_SWITCH_BUS);
				
		if(error){
			serial_puts("###CMD6 switch Highspeed failed error:0x");
			serial_put_dec(error);
			serial_puts("\n");
			goto DATA_READ;
		}	
		__udelay(5000); 
			
	}
	else{					 
		//send APP55 cmd	

		WRITE_CBUS_REG(SDIO_EXTENSION, 0); 
		arg = (0x1234)<<16;
			
		//serial_puts("***CMD55 app cmd here\n");	 
		error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
			  (0 << repeat_package_times_bit) | // packet number
			  (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
			  (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
			  (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
			  ((0x40+55) << cmd_command_bit)  
				,arg
				,TIMEOUT_DATA
				,ERROR_MMC_SWITCH_BUS);
		
		 //send ACMD6 to switch width			
		WRITE_CBUS_REG(SDIO_EXTENSION, 0);			   
	
		arg = 2;
		
		//serial_puts("***ACMD6 to switch width here\n");
	
		error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
			  (0 << repeat_package_times_bit) | // packet number
			  (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
			  (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
			  (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
			  ((0x40+6) << cmd_command_bit)  
				,arg
				,TIMEOUT_DATA
				,ERROR_MMC_SWITCH_BUS);
				
		if(error){
			serial_puts("***ACMD6 switch width failed error:0x");
			serial_put_dec(error);
			serial_puts("\n");
			goto DATA_READ;
		}
	}		
	
	
	setbits_le32(P_SDIO_IRQ_CONFIG,1<<soft_reset_bit);
	writel(((1<<8) | (1<<9)),P_SDIO_STATUS_IRQ);	
	WRITE_CBUS_REG(SDIO_CONFIG,(2 << sdio_write_CRC_ok_status_bit) |
					  (2 << sdio_write_Nwr_bit) |
					  (3 << m_endian_bit) |
					  (39 << cmd_argument_bits_bit) |
					  (0 << cmd_out_at_posedge_bit) |
					  (1 << bus_width_bit) |										  
					  (0 << cmd_disable_CRC_bit) |
					  (NO_DELAY_DATA << response_latch_at_negedge_bit) |
					  ((3)	 << cmd_clk_divide_bit));	
					  
	WRITE_CBUS_REG(SDIO_MULT_CONFIG,SD_boot_type);
	bus_width = 1;	
   __udelay(5000);						
		
DATA_READ:	 
   size=(size+511)&(~(511));
   for(addr=off,read_size=0;read_size<size;addr+=CONFIG_SDIO_BUFFER_SIZE)
   {
	  
	  cur_size=(size-read_size)>CONFIG_SDIO_BUFFER_SIZE?CONFIG_SDIO_BUFFER_SIZE:(size-read_size);

#if defined(CONFIG_AML_SPL_L1_CACHE_ON)
		invalidate_dcache_range(read_size+target,read_size+target+cur_size);
#endif  

	  WRITE_CBUS_REG(SDIO_M_ADDR,read_size+target);
	  if(bus_width == 0)
		WRITE_CBUS_REG(SDIO_EXTENSION,((1 << (9 + 3)) + (16 - 1)) << 16);
	  else
		WRITE_CBUS_REG(SDIO_EXTENSION,((1 << (9 + 3)) + (16 - 1)*4) << 16);
		
	  if((card_type==CARD_TYPE_SDHC)||(card_type==CARD_TYPE_EMMC))
			arg=addr>>9;
	  else
		arg=addr;
	  error=sdio_send_cmd((((cur_size >> 9) - 1) << repeat_package_times_bit) | // packet number
		   (0 << cmd_send_data_bit) | // RESPONSE has CRC7
		   (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
		   (1 << use_int_window_bit) | // will disable interrupt
		   (1 << response_have_data_bit) | // RESPONSE has data receive
		   (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
		   ((0x40+18) << cmd_command_bit),arg,TIMEOUT_SHORT,ERROR_READ_BLOCK);
		   
		if(error){
			serial_puts("CMD18 switch width failed error:0x");
			serial_put_dec(error);
			serial_puts("\n");
			return error;
		}
//--------------------------------------------------------------------------------
// send CMD12 -- STOP_TRANSMISSION
	  error=sdio_send_cmd((1 << check_busy_on_dat0_bit) | // RESPONSE is R1b
		  (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
		  (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
		  (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
		  ((0x40+12) << cmd_command_bit),0,TIMEOUT_DATA,ERROR_STOP_TRANSMISSION);
		if(error){
			serial_puts("CMD12 switch width failed error:0x");
			serial_put_dec(error);
			serial_puts("\n");
			return error;
		}

		read_size+=cur_size;
	}
	return 0;
	
}


static inline void init_magic(unsigned char * magic)
{
	unsigned char * source_magic = MMC_STORAGE_MAGIC;
	int i=0;
	
	for(i = 0; i < 11; i++){
		magic[i] = source_magic[i];
	}
	
	return;
}

static int check_data(void* cmp, char part_flag)
{
	struct mmc_storage_head_t * head = (struct mmc_storage_head_t * )cmp;
	int addr =  0;
	int ret =0;

	if(part_flag == 0){
		addr = MMC_DATA_BUF1 + MMC_STORAGE_AREA_HEAD_SIZE;
	}else{
		addr = MMC_DATA_BUF2 + MMC_STORAGE_AREA_HEAD_SIZE;
	}	

	if(head->checksum != mmckey_calculate_checksum(&(head->data[0]),MMC_STORAGE_AREA_VALID_SIZE)){
		ret = -1;
		print("mmc check storage check_sum failed\n");
	}

	return ret;

}

static int check_magic(void * cmp, unsigned char * magic)
{
	struct mmc_storage_head_t * head = (struct mmc_storage_head_t * )cmp;
	int ret =0, i = 0;

	for(i = 0; i < 11; i++ ){
		if(head->magic[i] != magic[i]){
			ret = -1;
			serial_puts("mmc  storage check_magic : failed\n");
			break;
		}
	}

	/*if(head->magic_checksum != mmckey_calculate_checksum(&(head->magic),MMC_STORAGE_MAGIC_SIZE)){
		ret = -1;
	}*/
	
	return ret ;
}

STATIC_PREFIX int sdio_switch_partition(void)
{
   int error;
   unsigned card_type=(romboot_info->ext>>4)&0xf;
   
    if(card_type != CARD_TYPE_EMMC){
        serial_puts("No need switch partition for none emmc device\n");
        return ERROR_NONE;
    }

	if (card_type == CARD_TYPE_EMMC){
	    
     	error=sdio_send_cmd((0 << check_busy_on_dat0_bit) | // RESPONSE is R1
    	      (0 << repeat_package_times_bit) | // packet number
    	      (1 << use_int_window_bit) | // will disable interrupt
              (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
              (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
              (0 << response_have_data_bit) | // RESPONSE has data receive
              (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
              ((0x40+6) << cmd_command_bit)  
    			,0x03b30000 //normal area
    			,TIMEOUT_DATA
    			,ERROR_MMC_SWITCH_PART);

    	if(error){
    	    serial_puts("###eMMC switch to usr partition failed error:0x");
            serial_put_dec(error);
            serial_puts("\n");
            return error;
    	}	
    	
    	serial_puts("###eMMC switch to usr partition sucess\n");
    	
        __udelay(10000);   //delay 10ms;        
    }

    return ERROR_NONE;
}


STATIC_PREFIX int sdio_secure_storage_get(void)
{
	struct mmc_storage_head_t  *part0_head = MMC_DATA_BUF1, *part1_head = MMC_DATA_BUF2;
	unsigned char magic[MMC_STORAGE_MAGIC_SIZE];
	char part0_valid = 0,part1_valid = 0;
	unsigned valid_addr=0;
	int ret = 0;

	unsigned card_type=(romboot_info->ext>>4)&0xf;
	
	if(card_type == CARD_TYPE_EMMC){
	    ret = sdio_switch_partition();
	    if(ret){
	        serial_puts("###eMMC switch to usr partition failed\n");
	    }
	}	
	init_magic(&magic[0]);
		
	ret = sdio_read_off_size(MMC_DATA_BUF1,MMC_STORAGE_OFFSET,sizeof(struct mmc_storage_head_t),READ_PORT);
	if(ret){
		print("mmc read storage head failed\n");
		part0_valid = 1;
	}	
	
	ret = sdio_read_off_size(MMC_DATA_BUF2,(MMC_STORAGE_OFFSET+MMC_STORAGE_AREA_SIZE),sizeof(struct mmc_storage_head_t),READ_PORT);
	if(ret){
		print("mmc read storage head failed\n");
		part1_valid = 1;
	}

	ret = check_magic(part0_head,&magic[0]);
	if(ret){
		print("mmc read storage check magic name failed\n");
		part0_valid = 1;
	}
	
	ret = check_magic(part1_head,&magic[0]);
	if(ret){
		print("mmc read storage check magic name failed\n");
		part1_valid = 1;
	}

	ret = 	check_data(part0_head,0);
	if(ret){
		print("mmc read storage check data  failed\n");
		part0_valid = 1;
	}

	ret = 	check_data(part1_head,1);
	if(ret){
		print("mmc read storage check data  failed\n");
		part1_valid = 1;
	}
	
	if((!part0_valid)&&(!part1_valid)){
		
		print("mmc read storage here : \n");
		if(part0_head->timestamp >= part1_head->timestamp){	
			print("mmc read storage part0_valid : \n");
			valid_addr = MMC_STORAGE_OFFSET ;
		}else{	
			print("mmc read storage part1_valid : \n");
			valid_addr = MMC_STORAGE_OFFSET + MMC_STORAGE_AREA_SIZE;
		}
	}

	if(valid_addr == 0){
		if(!part0_valid){
			print("mmc read storage part0_valid : \n");
			valid_addr = MMC_STORAGE_OFFSET ;
		}
		if(!part1_valid){
			print("mmc read storage part1_valid : \n");
			valid_addr = MMC_STORAGE_OFFSET + MMC_STORAGE_AREA_SIZE;
		}
	}
	
	if(valid_addr){
		ret = sdio_read_off_size((MMC_STORAGE_DEFAULT_ADDR-MMC_STORAGE_AREA_HEAD_SIZE),valid_addr,sizeof(struct mmc_storage_head_t),READ_PORT);
		if(ret){
			print("mmc read storage head failed\n");
			ret = -1;
		}
	}

	serial_puts("sdio_secure_storage_get : finished\n");

	return ret;
}


#endif

