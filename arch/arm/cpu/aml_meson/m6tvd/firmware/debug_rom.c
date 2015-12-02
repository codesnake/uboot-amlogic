#include <asm/arch/romboot.h>
#include <asm/arch/timing.h>
#include <memtest.h>
#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/nand.h>
#ifndef STATIC_PREFIX
#define STATIC_PREFIX
#endif
extern void ipl_memcpy(void*, const void *, __kernel_size_t);
#define memcpy ipl_memcpy
#define get_timer get_utimer
static  char cmd_buf[DEBUGROM_CMD_BUF_SIZE];

void restart_arm(void);

STATIC_PREFIX char * get_cmd(void)
{
    
    int i,j;
    char c;
    i=0;
    
    cmd_buf[0]=0;
    i=0;
    serial_putc('>');
    while(1)
    {
        if(serial_tstc()==0)
            continue;
        c=serial_getc();
        j=0;
        switch(c)
        {
        case '\r':
        case '\n':
            serial_putc('\n');
            return &cmd_buf[0];
        case 0x08:
        case 0x7f:
            if(i>0)
            {
                serial_puts("\b \b");
                cmd_buf[i--]=0;
                cmd_buf[i]=0x20;
            }
            continue;
        case 'w':
        case 'r':
        case 'q':
        case ' ':
        case 'P':
        case 'S':
        case 's':
        case 'M':
        case 'B':
        case '"':
        case ';':
        case 'c':
        case 'm':
        case 'a':
        case 't':
            if(i==sizeof(cmd_buf)-1)
                continue;
            serial_putc(c);
            cmd_buf[i++]=c;
            cmd_buf[i]=0;
            continue;
        default:
            if(i==sizeof(cmd_buf)-1)
                continue;
            if((c>='0' && c<='9')||
               (c>='a' && c<='f'))
            {
                serial_putc(c);
                cmd_buf[i++]=c;
                cmd_buf[i]=0;
                continue;
            }
        }
        serial_put_char(c);
        serial_putc('\n');
        serial_putc('>');
        serial_puts(cmd_buf);
    }
    return &cmd_buf[0];
}
STATIC_PREFIX int get_dword(char * str,unsigned * val)
{
    int c,i;
    *val=0;
    for(i=0;i<8&&*str;i++,str++)
    {
        c=*str;
        if((c>='0' && c<='9'))
            c-='0';
        else if((c>='a' && c<='f'))
            c-='a'-10;
        else
            return -1;
        *val=(*val<<4)|c;
    }
    return 0;
}
STATIC_PREFIX void debug_write_reg(int argc, char * argv[])
{
    unsigned  reg;
    unsigned  val;
    if(argc<3)
    {
        serial_puts("FAIL:Wrong write reg command\n");
        return;
    }
    if(get_dword(argv[1],&reg)||get_dword(argv[2],&val))
    {
        serial_puts("FAIL:Wrong reg addr=");
        serial_puts(argv[1]);
        serial_puts(" or val=");
        serial_puts(argv[2]);
        serial_putc('\n');
        return;
    }
    serial_puts("OK:Write ");
    serial_put_hex(reg,32);
    serial_putc('=');
    serial_put_hex(val,32);
    serial_putc('\n');
    writel(val,reg);
}
STATIC_PREFIX void debug_read_reg(int argc, char * argv[])
{
    unsigned  reg;
    if(argc<2)
    {
        serial_puts("FAIL:Wrong read reg command\n");
        return;
    }
    if(get_dword(argv[1],&reg))
    {
        serial_puts("FAIL:Wrong reg addr=");
        serial_puts(argv[1]);
        serial_putc('\n');
        return;
    }
    serial_puts("OK:Read ");
    serial_put_hex(reg,32);
    serial_putc('=');
    serial_put_hex(readl(reg),32);
    serial_putc('\n');
}
STATIC_PREFIX void memory_pll_init(int argc, char * argv[]);
STATIC_PREFIX void show_setting_addr(int argc, char * argv[])
{
#if 0    
    serial_puts("OK:ADDR=");
    serial_put_hex((unsigned)&__ddr_setting,32);
    serial_putc(' ');
    serial_put_hex((unsigned)&__plls,32);
    serial_putc('\n');
#endif    
}

STATIC_PREFIX void debugrom_ddr_init(int argc, char * argv[])
{
    
    unsigned i=0;
    if(argv[1])
        get_dword(argv[1],&i);
    //ddr_pll_init(&__ddr_setting);
    m6_ddr_init_test(i);
    
}
STATIC_PREFIX void clk_msr(int argc, char * argv[])
{
    
    unsigned id;
    
    serial_puts(argv[1]);
    
    serial_puts("(Mhz)=");
    get_dword(argv[1],&id);
    serial_put_dword(clk_util_clk_msr(id));
    
}
STATIC_PREFIX void start_arc(int argc,char * argv[] )
{
    unsigned addr;
    spi_init();
    get_dword(argv[1],&addr);
    /** copy ARM code*/
    memcpy((void*)0x49008000,(const void*)0x49000000,16*1024);
    writel((0x49008000>>14)&0xf,0xc810001c);
    /** copy ARC code*/
    memcpy((void*)0x49008000,(const void*)addr,16*1024);
    writel(0x1<<4,0xc8100020);
    writel(0x7fffffff,P_AO_RTI_STATUS_REG0);
    serial_puts("start up ARC\n");
//    writel(0x51001,0xc8100030);
    writel(1,P_AO_RTI_STATUS_REG1);
    writel(0x1,0xc1109964);
    writel(0x0,0xc1109964);
    unsigned a,b;
    unsigned timer_base;
    a=b=0x7fffffff;
    serial_puts("ARM is Live\n");
    timer_base=get_timer(0);
    do{
        a=readl(P_AO_RTI_STATUS_REG0);
        if((a&0x80000000)|| ((a==b)&&(get_timer(timer_base)<10000000)))
        {
            continue;
        }
        timer_base=get_timer(0);
        b=a;
        serial_puts("ARM is Live: ");
        serial_put_dword(a);
        switch(a&0xffff)
        {
            case 0: 
                serial_puts("ARM Exit Sleep Mode\n");
                break;
            
        }
       
        
    }while(a);
    
    
}
STATIC_PREFIX void caculate_sum(int argc,char * argv[] )
{
	int i;
	unsigned short sum=0;
	unsigned * magic;
	unsigned addr;
	get_dword(argv[1],&addr);
	volatile unsigned short *buf=(volatile unsigned short*)addr;
	for(i=0;i<0x1b0/2;i++)
	{
		sum^=buf[i];
	}

	for(i=256;i<CHECK_SIZE/2;i++)
	{
		sum^=buf[i];
	}
	buf[0x1b8/2]=sum;
	magic=(unsigned *)&buf[0x1b0/2];
}
STATIC_PREFIX void debugrom_save_to_spi(int argc, char * argv[])
{
    spi_init();
    if(argc!=4)
        return;
    unsigned dest,src,size;
    if(get_dword(argv[1],&dest)||get_dword(argv[2],&src) || get_dword(argv[3],&size))
        return;
    spi_program(dest,src,size);
}
STATIC_PREFIX void memory_pll_init(int argc, char * argv[])
{
    pll_init(&__plls); //running under default freq now . Before we fixed the PLL stable problem
	__udelay(1000);//delay 1 ms , wait pll ready
    serial_init(__plls.uart);

}

STATIC_PREFIX void debugrom_set_start(int argc, char * argv[])
{
    char * p;
    //int i;
    p=argv[1];
    memcpy(&init_script[0],p,sizeof(init_script));
    init_script[sizeof(init_script)-1]=0;
}

STATIC_PREFIX char * debugrom_startup(void)
{
    static int cur =0;
    int ret;
    ret=cur;
    if(cur>sizeof(init_script))
        return NULL;
    if(cur==0)
    {
        memcpy((void*)&cmd_buf[0],(const void *)&init_script[0],sizeof(init_script));
        serial_puts(&cmd_buf[0]);serial_putc('\n');
    }
    for(;cur!=';'&&cmd_buf[cur]!=0;cur++);
    if(cur==';')cmd_buf[cur++]=0;
    return &cmd_buf[ret];
}
STATIC_PREFIX int run_cmd(char * cmd)
{
    int argc;
    char * argv[4]={NULL,NULL,NULL,NULL};
    char * str;
    for(argc=0,str=cmd;argc<4;argc++)
    {
        while(*str==0x20)
            str++;
        if(*str==0)
            break;
        if(*str=='"')
        {
            argv[argc]=++str;
            while(*str!='"'&&*str!=0)
                str++;
            if(*str==0)
                break;
            *str++=0;
            continue;
        }
        argv[argc]=str;
        while(*str!=0x20&&*str!=0)
            str++;
        if(*str==0)
        {
            argc++;
            break;
        }
        *str++=0;
    }
    if(argc==0)
        return 1;
    switch(argv[0][0])
    {
    case 'w':
        debug_write_reg(argc,argv);
        break;
    case 'r':
        debug_read_reg(argc,argv);
        break;
#if (defined AML_DEBUGROM)||(CONFIG_ENABLE_SPL_MORE_CMD)
    case 'P':
        memory_pll_init(argc,argv);
        break;
    case 'S':
        show_setting_addr(argc,argv);
        break;
    case 'M':
        debugrom_ddr_init(argc,argv);
        break;
    case 'm':
        clk_msr(argc,argv);
        break;
    case 'B':
        debugrom_set_start(argc,argv);
        break;
#endif 
#if (defined AML_DEBUGROM)
    case 's':
        debugrom_save_to_spi(argc,argv);
        break;
    case 'c':
        caculate_sum(argc,argv);
        break;
#endif  
    case 'a':
        start_arc(argc,argv);
        break;  
    case 't':
    		restart_arm();
    		break;
    case 'q':
        return 0;
    }
    return 1;
}
#ifdef AML_DEBUGROM
#include <asm/arch/firm/nand.h>

STATIC_PREFIX void nf_cntl_init(const T_ROM_BOOT_RETURN_INFO* bt_info)
{
    nf_pinmux_init();

	//Set NAND to DDR mode , time mode 0 , no adjust
    //	 WRITE_CBUS_REG(NAND_CFG, __hw_setting.nfc_cfg);

	WRITE_CBUS_REG(NAND_CFG, (0<<10) | (0<<9) | (0<<5) | 19);


}

void nf_erase(void)
{
    int i;
    nf_cntl_init(romboot_info);
    for(i=0;i<4;i++)
    {
        NFC_SEND_CMD_CLE(CE0,0x60);
		NFC_SEND_CMD_ALE(CE0,0);
	    NFC_SEND_CMD_ALE(CE0,i);
	    NFC_SEND_CMD_ALE(CE0,0);
	    NFC_SEND_CMD_CLE(CE0,0xd0);
        NFC_SEND_CMD_RB(CE0,0);
    }
    NFC_SEND_CMD_IDLE(CE0,0);     // FIFO cleanup
	NFC_SEND_CMD_IDLE(CE0,0);     // FIFO cleanup
	while(NFC_CMDFIFO_SIZE()>0);   // wait until all command is finished
}
#endif
STATIC_PREFIX void debug_rom(char * file, int line)
{
#ifdef AML_DEBUGROM    
    nf_erase();
#endif    
    //int c;
    serial_puts("Enter Debugrom mode at ");
    serial_puts(file);
    serial_putc(':');
    serial_put_dword(line);
#if (defined AML_DEBUGROM)||(CONFIG_ENABLE_SPL_MORE_CMD) 
    
    char * cmd=debugrom_startup();
    while(cmd&&*cmd)
    {
        run_cmd(cmd);
        cmd=debugrom_startup();
    }
#endif    
    while(run_cmd(get_cmd()))
    {
        ;
    }
}
#ifdef AML_DEBUGROM

void lowlevel_init(void* cur,void * target)
{
    int i;
	
	//Adjust 1us timer base
	WRITE_CBUS_REG_BITS(PREG_CTLREG0_ADDR,CONFIG_CRYSTAL_MHZ,4,5);
	/*
        Select TimerE 1 us base
    */
	clrsetbits_le32(P_ISA_TIMER_MUX,0x7<<8,0x1<<8);
	serial_init((__plls.uart&(~0xfff))|(CONFIG_CRYSTAL_MHZ*1000000/(115200*4)));
//	serial_init_firmware(__plls.uart);
}
void relocate_init(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
    while(1)
        debug_rom(__FILE__,__LINE__);

}
#endif

void restart_arm(void)
{
	  writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
	  //reset A9
	  writel(0x2,P_RESET4_REGISTER);
		while(1){
		}
}
