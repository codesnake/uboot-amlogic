/*
 * (C) Copyright 2012
 * Amlogic. Inc. zongdong.jiao@amlogic.com
 *
 * This file is used to prefetch/varify/compare HDCP keys
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <asm/byteorder.h>
#ifdef CONFIG_MESON_SECURE_HDCP
#include <asm/arch/trustzone.h>
#endif

#define HDCP_KEY_SIZE                   308
#define HDCP_IP_KEY_SIZE                288
#define TX_HDCP_DKEY_OFFSET             0x400

#if 0
extern ssize_t uboot_key_init(void);
extern int nandkey_provider_register(void);
extern int key_set_version(char *device);
extern ssize_t uboot_key_read(char *keyname, char *keydata);
#endif
extern ssize_t uboot_key_get(char *device,char *key_name, char *key_data,int key_data_len,int ascii_flag);
extern int cmd_efuse(int argc, char * const argv[], char *buf);
extern unsigned long hdmi_hdcp_rd_reg(unsigned long addr);
extern void hdmi_hdcp_wr_reg(unsigned long addr, unsigned long data);

// store prefetch RAW data
static unsigned char hdcp_keys_prefetch[HDCP_KEY_SIZE] = { 0x00 };

static unsigned char hdcp_keys_reformat[HDCP_IP_KEY_SIZE] = { 0x00 };

// copy the fetched data into HDMI IP
#ifndef CONFIG_MESON_SECURE_HDCP
static int init_hdcp_ram(unsigned char * dat, unsigned int pre_clear)
{
    int i, j;
    char value;
    void hdmi_hdcp_wr_reg(unsigned long addr, unsigned long data);
    unsigned int ram_addr;

    memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));
    // adjust the HDCP key's KSV & DPK position
    memcpy(&hdcp_keys_reformat[0], &hdcp_keys_prefetch[8], HDCP_IP_KEY_SIZE-8);   // DPK
    memcpy(&hdcp_keys_reformat[280], &hdcp_keys_prefetch[0], 5);   // KSV
    j = 0;
    for (i = 0; i < HDCP_IP_KEY_SIZE - 3; i++) {  // ignore 3 zeroes in reserved KSV
        value = hdcp_keys_reformat[i];
        ram_addr = TX_HDCP_DKEY_OFFSET+j;
        hdmi_hdcp_wr_reg(ram_addr, value);
        j = ((i % 7) == 6) ? j + 2: j + 1;
    }
    if(pre_clear == 1)
        hdmi_hdcp_wr_reg(TX_HDCP_DKEY_OFFSET, 0xa);

    return 1;
}
#endif

// Prefetch the HDCP keys data from nand, efuse, etc
static int do_hdcp_prefetch(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    int ret=-1, prefetch_flag = 0;
#ifdef CONFIG_MESON_SECURE_HDCP
	struct hdcp_hal_api_arg arg;
	char hdcpname[]="hdcp";
#endif

    if (argc == 1)
        return cmd_usage(cmdtp);

#ifndef CONFIG_MESON_SECURE_HDCP
    init_hdcp_ram(hdcp_keys_prefetch, 1);

    printf("hdcp get form storage medium: %s\n", argv[1]);
    if(!strncmp(argv[1], "nand", strlen("nand")) || !strncmp(argv[1], "emmc", strlen("emmc"))) {
#if defined(CONFIG_SECURITYKEY)
        ret = uboot_key_get(argv[1], "hdcp", hdcp_keys_prefetch, 308, 0);
#endif
        if(ret >= 0)
            prefetch_flag = 1;
        else
            printf("prefetch hdcp keys from %s failed\n", argv[1]);
    }
    else if(!strncmp(argv[1], "efuse", strlen("efuse"))) {
        const char *Argv[3] = {"efuse", "read", "hdcp"};
        int Argc = 3;
        ret = cmd_efuse(Argc, Argv, hdcp_keys_prefetch);
        if(!ret)
            prefetch_flag = 1;
        else
            printf("prefetch hdcp keys from %s failed\n", argv[1]);
    }

    if(prefetch_flag == 1) {
        init_hdcp_ram(hdcp_keys_prefetch, 0);
        memset(hdcp_keys_reformat, 0, sizeof(hdcp_keys_reformat));  // clear the buffer to prevent reveal keys
        ret = 1;
    }
	return 1;
#else
	arg.namelen = sizeof(hdcpname);
	arg.name_phy_addr = hdcpname;
	arg.datalen = 308;
	ret = meson_trustzone_hdcp(&arg);
	if(ret<0){
		printf("prefetch hdcp set fail!\n");
		return -1;
	}
	else{
		printf("prefech hdcp set success!\n");
		return 1;
	}
#endif
}

static cmd_tbl_t cmd_hdcp_sub[] = {
	U_BOOT_CMD_MKENT(prefetch, 2, 1, do_hdcp_prefetch, "", ""),
};

static int do_hdcp(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_hdcp_sub[0], ARRAY_SIZE(cmd_hdcp_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	hdcp, 3, 0, do_hdcp,
	"HDCP sub-system",
    "prefetch [device] - prefetch hdcp keys from nand, efuse or others\n"
);

