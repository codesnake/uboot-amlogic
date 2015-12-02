#include <config.h>
#include <asm/arch/romboot.h>
#include <asm/arch/timing.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/io.h>
#include <asm/arch/sdio.h>
#include "nfio.c"
//#include <asm/arch/firm/config.h>


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
   unsigned arg;
   cmd_clk_divide=__plls.sdio_cmd_clk_divide;
   SD_boot_type=sdio_get_port(por_sel);
   unsigned card_type=(romboot_info->ext>>4)&0xf;

//register.h: #define SDIO_AHB_CBUS_CTRL 0x2318
//#define SDIO_AHB_CBUS_CTRL          (volatile unsigned long *)0xc1108c60   
#define SDIO_AHB_CBUS_CTRL_ADDR          (volatile unsigned long *)0xc1108c60   
    //serial_put_dword(clk_util_clk_msr(7));
	serial_puts("CLK81=");
	serial_put_dec(clk_util_clk_msr(7));
	serial_puts("Mhz\n");
//*SDIO_AHB_CBUS_CTRL&=~1;
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

    //--------------------------------------------------------------------------------
    // send CMD17 -- READ_BLOCK
   WRITE_CBUS_REG(SDIO_MULT_CONFIG,SD_boot_type);
   size=(size+511)&(~(511));
   for(addr=READ_SIZE,read_size=0;read_size<size;addr+=CONFIG_SDIO_BUFFER_SIZE)
   {
      
      cur_size=(size-read_size)>CONFIG_SDIO_BUFFER_SIZE?CONFIG_SDIO_BUFFER_SIZE:(size-read_size);


      WRITE_CBUS_REG(SDIO_M_ADDR,read_size+target);
      read_size+=cur_size;

      WRITE_CBUS_REG(SDIO_EXTENSION,((1 << (9 + 3)) + (16 - 1)) << 16);
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
        if(error)
            return error;
//--------------------------------------------------------------------------------
// send CMD12 -- STOP_TRANSMISSION
      error=sdio_send_cmd((1 << check_busy_on_dat0_bit) | // RESPONSE is R1b
          (0 << response_crc7_from_8_bit) | // RESPONSE CRC7 is normal
          (0 << response_do_not_have_crc7_bit) | // RESPONSE has CRC7
          (45 << cmd_response_bits_bit) | // RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
          ((0x40+12) << cmd_command_bit),0,TIMEOUT_DATA,ERROR_STOP_TRANSMISSION);
        if(error)
            return error;
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

