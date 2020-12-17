/*
 * common/cmd_android.c
 *
 *  $Id: cmd_android.c,v 1.2 2009/11/21 00:11:42 dark0351 Exp $
 *
 * (C) Copyright 2009
 * Byungjae Lee, Samsung Erectronics, bjlee@samsung.com.
 *	- support for S3C2412, S3C2443, S3C6400, S5PC100 and S5PC110
 *
 * (C) Copyright SAMSUNG Electronics
 *      wc7.jang  <wc7.jang@samsung.com>
 *      - add Android Command fo S5PC100 and S5PC110
 *
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

/*
 * Android Installation Functions
 *
 */

#include <common.h>
#include <command.h>
#include <net.h>

#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

#include "ethdown.h"

#define ETHDOWN_CMD_BOOTLOADER_FILENAME "u-boot.bin"
#define ETHDOWN_CMD_KERNEL_FILENAME     "zImage"
#define ETHDOWN_CMD_ROOTFS_FILENAME     "rootfs_android.yaffs2"
#define ETHDOWN_CMD_LOGO_FILENAME       "hanback_logo.bmp"

#if defined(CONFIG_CMD_ETHDOWN)
int do_ethdown(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
  int i, j, dev, ret;
  char *cmd, *s, *temp1, *temp2;
  int quiet = 0;
  const char *quiet_str = getenv("quiet");
  char up_cmd_str[8192], up_all_cmd_str[8192], up_cmd_mac[17];
  const char *mac;
  size_t 		retlen = 0;
  eth_download_cmd_t eth_download_cmd;

  /* at least two arguments please */
  if (argc < 2)
    goto usage;

  if (quiet_str)
    quiet = simple_strtoul(quiet_str, NULL, 0) != 0;

  cmd = argv[1];

  eth_download_cmd.up_ok = 0;

  if(cmd[0] == '-')
  {
    if(strlen(cmd) > 1 && strlen(cmd) < 6) {
      for(i = 0; i < strlen(cmd) -1; i++){
        switch (cmd[i+1]) {
          case 'b':
            eth_download_cmd.kind = UBOOT_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_BOOTLOADER_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            break;

          case 'k':
            eth_download_cmd.kind = KERNEL_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_KERNEL_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            break;

          case 'r':
            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            eth_download_cmd.kind = ROOTFS_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_ROOTFS_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            break;

          case 'l':
            eth_download_cmd.kind = LOGO_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_LOGO_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            break;

          case 'a':
            if(strlen(cmd) > 2)
              goto usage;
            eth_download_cmd.kind = UBOOT_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_BOOTLOADER_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            strcpy(up_all_cmd_str, up_cmd_str);

            eth_download_cmd.kind = KERNEL_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_KERNEL_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            eth_download_cmd.kind = ROOTFS_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_ROOTFS_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            eth_download_cmd.kind = LOGO_CMD;
            strcpy(eth_download_cmd.bootfile, ETHDOWN_CMD_LOGO_FILENAME);
            ret = ethdown_common(&eth_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &eth_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            parse_string_outer(up_all_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
            return 1;
            //break;
          default:
            printf("\n Bad option [%s]\n", cmd);
            goto usage;
        }
      }
    } else {
      goto usage;
    }
  } else {
  }

  parse_string_outer(up_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
  return 1;

usage:
  printf("Usage:\n%s\n", cmdtp->usage);
  return 1;
}

U_BOOT_CMD(
	ethdown, 3, 1, do_ethdown,
  "ethdown  - Board image Download and Write\n",
  " [Options] - write options images\n"
  " Options :\n"
  "  -b : write bootloader image\n"
  "  -k : write kernel image\n"
  "  -r : write root filesystem image\n"
  "  -l : write logo image\n"
  "  -a : write all images[bootloader/kernel/rootfs/logo] \n"
  "  example) ethdown -bk \n"
);
#endif // CONFIG_CMD_ETHDOWN

