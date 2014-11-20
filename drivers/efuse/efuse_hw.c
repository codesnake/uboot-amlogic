#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <amlogic/efuse.h>
#include "efuse_regs.h"

#ifdef EFUSE_DEBUG
unsigned debug_buf[EFUSE_DWORDS] = {0};	
#endif

void __efuse_write_byte( unsigned long addr, unsigned long data )
{
#ifndef CONFIG_MESON_TRUSTZONE	
#ifdef EFUSE_DEBUG
	char *p = (char*)debug_buf;
	p[addr] = data;
	return;
#endif		
	//printf("addr=%d, data=%x\n", addr, data);
    unsigned long auto_wr_is_enabled = 0;

	//set efuse PD=0
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 0, 27, 1);

    if ( READ_EFUSE_REG( P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_ENABLE_BIT ) )
    {
        auto_wr_is_enabled = 1;
    }
    else
    {
        /* temporarily enable Write mode */
        WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_ON,
            CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
    }

#if defined(CONFIG_AML_MESON_8)
	unsigned int byte_sel = addr % 4;
	addr = addr / 4;

    /* write the address */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
        CNTL1_BYTE_ADDR_BIT, CNTL1_BYTE_ADDR_SIZE );

	//auto write byte select (0-3), for m8
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL3, byte_sel,
        CNTL1_AUTO_WR_START_BIT, 2 );
#else
	/* write the address */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
        CNTL1_BYTE_ADDR_BIT, CNTL1_BYTE_ADDR_SIZE );
#endif

    /* set starting byte address */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
        CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
        CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );

    /* write the byte */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, data,
        CNTL1_BYTE_WR_DATA_BIT, CNTL1_BYTE_WR_DATA_SIZE );
    /* start the write process */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
        CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
        CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
    /* dummy read */
    READ_EFUSE_REG(P_EFUSE_CNTL1 );

    while ( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_BUSY_BIT ) )
    {
        udelay(1);
    }

    /* if auto write wasn't enabled and we enabled it, then disable it upon exit */
    if (auto_wr_is_enabled == 0 )
    {
        WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_OFF,
            CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
    }

	//set efuse PD=1
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 1, 27, 1);

#endif   // endif trustzone    
}

void __efuse_read_dword( unsigned long addr, unsigned long *data )
{
#ifndef CONFIG_MESON_TRUSTZONE	
#ifdef EFUSE_DEBUG
	unsigned *p =debug_buf;
	*data = p[addr>>2];
	return;
#endif		

#if defined(CONFIG_AML_MESON_8)
	addr = addr / 4;	//each address have 4 bytes in m8
#endif
    unsigned long auto_rd_is_enabled = 0;
    
	//set efuse PD=0
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 0, 27, 1);

    if( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_ENABLE_BIT ) )
    {
        auto_rd_is_enabled = 1;
    }
    else
    {
        /* temporarily enable Read mode */
        WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
            CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
    }

    /* write the address */    
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
        CNTL1_BYTE_ADDR_BIT,  CNTL1_BYTE_ADDR_SIZE );
    	
    /* set starting byte address */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
        CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );	
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
        CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
   /* start the read process */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
        CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );      
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
        CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
      
    /* dummy read */
    READ_EFUSE_REG( P_EFUSE_CNTL1 );
    while ( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_BUSY_BIT ) )
    {
        udelay(1);
    }
    /* read the 32-bits value */
    ( *data ) = READ_EFUSE_REG( P_EFUSE_CNTL2 );    
    /* if auto read wasn't enabled and we enabled it, then disable it upon exit */
    if ( auto_rd_is_enabled == 0 )
    {
        WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
            CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
    }

	//set efuse PD=1
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 1, 27, 1);

    //printf("__efuse_read_dword: addr=%ld, data=0x%lx\n", addr, *data);
#endif   // end if trustzone    
}

int efuse_init(void)
{	
#ifndef CONFIG_MESON_TRUSTZONE	
    /* disable efuse encryption */
    WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL4, CNTL4_ENCRYPT_ENABLE_OFF,
        CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE );
#if defined(CONFIG_M6) || defined(CONFIG_AML_MESON_8)
	// clear power down bit
	WRITE_EFUSE_REG_BITS(P_EFUSE_CNTL1, CNTL1_PD_ENABLE_OFF, 
			CNTL1_PD_ENABLE_BIT, CNTL1_PD_ENABLE_SIZE);
#endif        

#ifdef EFUSE_DEBUG	
#ifdef CONFIG_M6
/*	char *p = debug_buf;
	// licence	
	p[0] = 0xbf;
	p[1] = 0xff;
	p[2] = 0x00;	
	//rsa
	p[8] = 0xaf;
	p[9] = 0x32;
	p[10] = 0x76;
	p[135] = 0xb2;	*/
#elif defined(CONFIG_M3)
	char *p = debug_buf;
	p[0] = p[60] = 0x02;
	p[1] = p[61] = 0x03;
	p[2] = p[62] = 0x00;
	p[3] = p[63] = 0xA3;
#endif
#endif
#endif   // endif trustzone
    return 0;
}
