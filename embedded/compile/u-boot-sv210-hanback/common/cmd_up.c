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

#if defined(CONFIG_CMD_UP)
#define UP_CMD_BOOTLOADER_FILENAME    "empos3-sv210/u-boot.bin"
#define UP_CMD_KERNEL_FILENAME        "empos3-sv210/zImage"
#define UP_CMD_ROOTFS_FILENAME        "empos3-sv210/rootfs_android.yaffs2"
#define UP_CMD_LOGO_FILENAME          "empos3-sv210/hanback_logo.bmp"

#define UP_CMD_KERNEL_TEST_FILENAME   "empos3-sv210/zImage"
#define UP_CMD_ROOTFS_TEST_FILENAME   "empos3-sv210/rootfs_android-test.yaffs2"

#define UP_CMD_RESET                  "reset;"
#define UP_CMD_KERNEL_BOOT            "nand read c0008000 80000 3c0000;bootm c0008000;"
#define UP_CMD_DEFAULT_MAC            "00:0b:a8:"

int do_up(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
  int i, j, dev, ret;
  char *cmd, *s, *temp1, *temp2;
  int quiet = 0;
  const char *quiet_str = getenv("quiet");
  char up_cmd_str[8192], up_all_cmd_str[8192], up_cmd_mac[17];
  const char *mac;
	size_t 		retlen = 0;
  eth_download_cmd_t up_download_cmd;

  /* at least two arguments please */
  if (argc < 2)
    goto usage;

  if (quiet_str)
    quiet = simple_strtoul(quiet_str, NULL, 0) != 0;

  cmd = argv[1];


  up_download_cmd.test_ok = 0;
  up_download_cmd.up_ok = 1;

  /* env setting : ipaddr and serverip */
  setenv ("serverip", TFTPSERVERIP);
  setenv ("ipaddr", IPADDR);

  if(cmd[0] == '-')
  {
    if(strlen(cmd) > 1 && strlen(cmd) < 6) {
      for(i = 0; i < strlen(cmd) -1; i++){
        switch (cmd[i+1]) {
          case 'b':
            up_download_cmd.kind = UBOOT_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_BOOTLOADER_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            break;

          case 'k':
            up_download_cmd.kind = KERNEL_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_KERNEL_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            break;

          case 'r':
            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            up_download_cmd.kind = ROOTFS_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_ROOTFS_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            break;

          case 'l':
            up_download_cmd.kind = LOGO_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_LOGO_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            break;

          case 'a':
            if(strlen(cmd) > 2)
              goto usage;

            up_download_cmd.kind = UBOOT_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_BOOTLOADER_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            strcpy(up_all_cmd_str, up_cmd_str);

            up_download_cmd.kind = KERNEL_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_KERNEL_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            up_download_cmd.kind = ROOTFS_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_ROOTFS_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            up_download_cmd.kind = LOGO_CMD;
            strcpy(up_download_cmd.bootfile, UP_CMD_LOGO_FILENAME);
            ret = ethdown_common(&up_download_cmd);
            if(ret < 0)
              return -1;
            parsing_command(up_cmd_str, &up_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            parse_string_outer(up_all_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
            return 1;

          default:
            printf("\n Bad option [%s]\n", cmd);
            goto usage;
        }
      }
    } else {
      goto usage;
    }
  } else {
    if (strcmp(cmd, "test") == 0) {
      if (argc > 3)
        goto usage;

      up_download_cmd.kind = UBOOT_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_BOOTLOADER_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcpy(up_all_cmd_str, up_cmd_str);

      up_download_cmd.kind = KERNEL_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_KERNEL_TEST_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      nand_dump_temp(NAND_ROOTFS_ADDRESS);
      up_download_cmd.kind = ROOTFS_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_ROOTFS_TEST_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      up_download_cmd.kind = LOGO_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_LOGO_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      strcat(up_all_cmd_str, UP_CMD_RESET);
      parse_string_outer(up_all_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
      return 1;

    } else if (strcmp(cmd, "pp") == 0) {
      if (argc < 3)
        goto usage;

      mac = argv[2];

      if(strlen(mac) < 6)
        goto usage;

      strcpy(up_cmd_mac, UP_CMD_DEFAULT_MAC);

      for (j=0; j < 3; j++) {
        strncat(up_cmd_mac, &mac[j*2], 2);
        if (j != 2)
          strcat(up_cmd_mac, ":");

      }
      up_download_cmd.kind = UBOOT_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_BOOTLOADER_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcpy(up_all_cmd_str, up_cmd_str);

      up_download_cmd.kind = KERNEL_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_KERNEL_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      nand_dump_temp(NAND_ROOTFS_ADDRESS);
      up_download_cmd.kind = ROOTFS_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_ROOTFS_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      up_download_cmd.kind = LOGO_CMD;
      strcpy(up_download_cmd.bootfile, UP_CMD_LOGO_FILENAME);
      ret = ethdown_common(&up_download_cmd);
      if(ret < 0)
        return -1;
      parsing_command(up_cmd_str, &up_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      setenv ("serverip", DEFAULT_SERVERIP);
      setenv ("ipaddr", DEFAULT_IPADDR);

      strcat(up_all_cmd_str, "set ethaddr ");
      strcat(up_all_cmd_str, up_cmd_mac);
      strcat(up_all_cmd_str, ";saveenv;");
      strcat(up_all_cmd_str, UP_CMD_RESET);
      parse_string_outer(up_all_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
      return 1;
    }
  }

  parse_string_outer(up_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
  return 1;

usage:
  printf("Usage:\n%s\n", cmdtp->usage);
  return 1;
}

U_BOOT_CMD(
	up, 3, 1, do_up,
  "up  - Board image Download and Write\n",
  " [Options] - write options images\n"
  " Options :\n"
  "  -b : write bootloader image\n"
  "  -k : write kernel image\n"
  "  -r : write root filesystem image\n"
  "  -l : write logo image\n"
  "  -a : write all images[bootloader/kernel/rootfs/logo] \n"
  "  example) up -bk \n"
  "up pp [mac] - write last images [bootloader/kernel/rootfs/logo/mac]\n"
  "  example) up pp 031301 \n"
  "up test - write test images [bootloader/kernel/rootfs/logo]\n"
);

#endif // CONFIG_CMD_ONENAND

