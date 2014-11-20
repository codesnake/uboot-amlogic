// ----------------------------------------------------
// GPIO Usage
// ----------------------------------------------------
// gpioH[8:0]   // DEBUG OUT[15:7]
// card[6:0]    // DEBUG OUT[6:0]
// gpioAO_8     // DEBUG OUT VALID
// ------------------------------------
// gpioY[9:6]   // knobs[19:16]
// gpioX[11:0]  // knobs[15:4]
// gpioAO[13:10]// knobs[3:0]
// ----------------------------------------------------
// gpioAO_7     // DONE
// gpioAO[6:5]  // TEST_OUT[1:0]
// ----------------------------------------------------
// gpioAO_4     // test_sel[3]
// gpioAO_3     // test_sel[2]
// gpioAO_2     // test_sel[1]
// gpioAO_1     // test_sel[0]
// ----------------------------------------------------
//
// boot_18      //              SPI
// boot_12      //              SPI
// boot_14      //              SPI
// boot_13      //              SPI
// gpioAO_0     // trst_n


// boot_6
#define CLR_TEST_DONE_FLAG  writel(readl(P_AO_GPIO_O_EN_N) & ~(1 << (7 + 16)), P_AO_GPIO_O_EN_N)
#define SET_TEST_DONE_FLAG  writel(readl(P_AO_GPIO_O_EN_N) |  (1 << (7 + 16)), P_AO_GPIO_O_EN_N)

// {1'b1,`AO_GPIO_I}:              PRDATA_int  <= {test_n_gpio_i,rtc_gpo_i,rtc_gpi_i,15'h0,gpio_i[13:0]};
#define GET_TEST_SEL        ((readl(P_AO_GPIO_I) >> 1) & 0x0F);    

// assign          mod_pad_gpio1_i[16:0]   = gpioY_i_tm[16:0];
// assign          mod_pad_gpio0_i[21:0]   = gpioX_i_tm[21:0];
// gpioAO[13:10]
#define GET_KNOB_VALUE  (((readl(P_AO_GPIO_I) >> 10) & 0xF) | ((readl(P_PREG_PAD_GPIO0_I) & 0xFFF) << 4) | (((readl(P_PREG_PAD_GPIO1_I) >> 6) & 0xf) << 16))
// --------------------------------------------------------


// ----------------------------------------------------
// GPIO Usage
// ----------------------------------------------------
// gpioH[8:0]   // DEBUG OUT[15:7]
// card[6:0]    // DEBUG OUT[6:0]
// gpioAO_8     // DEBUG OUT VALID
// ------------------------------------
// gpioY[9:6]   // knobs[19:16]
// gpioX[11:0]  // knobs[15:4]
// gpioAO[13:10]// knobs[3:0]
// ----------------------------------------------------
// gpioAO_7     // DONE
// gpioAO[6:5]  // TEST_OUT[1:0]
// ----------------------------------------------------
// gpioAO_4     // test_sel[3]
// gpioAO_3     // test_sel[2]
// gpioAO_2     // test_sel[1]
// gpioAO_1     // test_sel[0]
// ----------------------------------------------------
//
// boot_18      //              SPI
// boot_12      //              SPI
// boot_14      //              SPI
// boot_13      //              SPI
// gpioAO_0     // trst_n

// assign          mod_pad_gpio3_i[18:0]   = boot_i_tm[18:0];
// boot[5:4]
void SET_TEST_OUT(int a)    
{
    writel((readl(P_AO_GPIO_O_EN_N) & ~(0x3 << (5 + 16))) | (((a)&0x3) << (5 + 16)), P_AO_GPIO_O_EN_N);
}

// 
// wire    [9:0]   gpioH_gpio_o            =    mod_pad_gpio3_o[28:19];
// assign          mod_pad_gpio0_i[28:22]  = card_i_tm[6:0];
void SET_DEBUG_LEVEL(int a)
{
    writel((readl(P_PREG_PAD_GPIO3_O) & ~(0x1FF << 19)) | (((a) >> 7) << 19),    P_PREG_PAD_GPIO3_O);
    writel((readl(P_PREG_PAD_GPIO0_O) & ~(0x7F << 22)) | ((((a) & 0x7F) << 22)), P_PREG_PAD_GPIO0_O);
}

// assign          mod_pad_gpio3_i[18:0]   = boot_i_tm[18:0];
// boot_7
void CLR_DEBUG_VALID(void)
{
    writel(readl(P_AO_GPIO_O_EN_N) & ~(1 << (8 + 16)), P_AO_GPIO_O_EN_N);
}

void SET_DEBUG_VALID(void)
{
    writel(readl(P_AO_GPIO_O_EN_N) | (1 << (8 + 16)), P_AO_GPIO_O_EN_N);
}

void ddr_debug ( int  data )
{
        SET_DEBUG_LEVEL(data & 0xffff);
        SET_DEBUG_VALID();
        CLR_DEBUG_VALID();
        SET_DEBUG_LEVEL((data >> 16) & 0xffff);
        SET_DEBUG_VALID();
        CLR_DEBUG_VALID();
}

void output_debug_level(int data)
{
    SET_DEBUG_LEVEL(data);
    SET_DEBUG_VALID();
    CLR_DEBUG_VALID();
}

