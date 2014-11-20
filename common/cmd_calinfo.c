#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <amlogic/efuse.h>

int do_calinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char buf[EFUSE_BYTES];
    unsigned char ver, tmp;
    int print = 0;
#if defined(CONFIG_AML_MESON_8)

	memset(buf, 0, sizeof(buf));
	
	if(argc <= 2){
        if (!strcmp(argv[1], "-p")) {
            print = 1;    
        }
		#define THERMAL_CAL_FLAG_OFF 503
		#define CAL_VER_OFF          505
		#define CVBS_FLAG_OFF 505
		printf("Module:                    Calibrated?\n");
		printf("---------------------------------------\n");
		memcpy(buf, efuse_dump(), EFUSE_BYTES);
        tmp  = ((buf[CAL_VER_OFF] & 0xf0) >> 4);
        ver  = (tmp & 0x08) >> 3;
        ver |= (tmp & 0x04) >> 1;
        ver |= (tmp & 0x02) << 1;
        ver |= (tmp & 0x01) << 3;
        if (print) {
            printf("tmp:0x%x, ver:0x%x\n", tmp, ver);
        }
		printf("thermal_sensor                ");
		if (buf[THERMAL_CAL_FLAG_OFF] & 1 << 7 && ((ver >= 0x05) || (ver == 0x02)))
			printf("Yes\n");
		else
			printf("No\n");
		printf("cvbs                          ");
		if ((ver == 0x01) || (ver == 0x02) || (ver >= 0x05))
			printf("Yes\n");
		else
			printf("No\n");
	}
#endif

	return 0;
	
}

U_BOOT_CMD(
	calinfo,	2,	1,	do_calinfo,
	"calinfo print the chip calibration info",
	"calinfo\n"
	"print the chip calibration info;\n"
);
