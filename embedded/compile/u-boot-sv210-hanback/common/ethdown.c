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

#if defined(CONFIG_CMD_ETHDOWN)

long ethdown_download_common(eth_download_cmd_t *p_eth_download_cmd)
{
  long down_filesize = -1;
  load_addr = p_eth_download_cmd->download_address;
  copy_filename (BootFile, p_eth_download_cmd->bootfile, sizeof(BootFile));

  if(p_eth_download_cmd->up_ok)
  {
    setenv ("serverip", TFTPSERVERIP);
    setenv ("ipaddr", IPADDR);
  }

  if ((down_filesize = NetLoop(TFTP)) < 0) {
    return -1;
  }

  if(p_eth_download_cmd->up_ok)
  {
    setenv ("serverip", DEFAULT_SERVERIP);
    setenv ("ipaddr", DEFAULT_IPADDR);
  }

  return down_filesize;
}

void parsing_command(char *command, eth_download_cmd_t *p_eth_download_cmd)
{
  char cmd_temp1[256], cmd_temp2[256];

  strcpy(command, ETHDOWN_CMD_NAND_ERASE);

  sprintf(cmd_temp1, "%x", p_eth_download_cmd->start_address);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(cmd_temp2, " ");
  strcat(command, cmd_temp2);

  sprintf(cmd_temp1, "%x", p_eth_download_cmd->erase_size);

  strcpy(cmd_temp2, cmd_temp1);
  strcat(cmd_temp2, ETHDOWN_CMD_COMMAND_DIV);
  strcat(command, cmd_temp2);

  if(p_eth_download_cmd->kind == ROOTFS_CMD)
    strcat(command, ETHDOWN_CMD_NAND_WRITE_YAFFS2);
  else
    strcat(command, ETHDOWN_CMD_NAND_WRITE);

  sprintf(cmd_temp1, "%x", p_eth_download_cmd->download_address);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(cmd_temp2, " ");
  strcat(command, cmd_temp2);

  sprintf(cmd_temp1, "%x", p_eth_download_cmd->start_address);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(command, cmd_temp2);
  strcat(command, " ");

  if(p_eth_download_cmd->kind == LOGO_CMD)
    sprintf(cmd_temp1, "%x", ETHDOWN_CMD_LOGO_SIZE);
  else
    sprintf(cmd_temp1, "%x", p_eth_download_cmd->download_size);

  strcpy(cmd_temp2, cmd_temp1);
  strcat(command, cmd_temp2);
  strcat(command, ETHDOWN_CMD_COMMAND_DIV);
}

int ethdown_common(eth_download_cmd_t *p_eth_download_cmd)
{
  long down_filesize = -1;
  switch (p_eth_download_cmd->kind)
  {
    case UBOOT_CMD:
      p_eth_download_cmd->download_address = DOWNLOAD_UBOOT_ADDRESS;
      p_eth_download_cmd->download_size = ETHDOWN_CMD_BOOTLOADER_SIZE;
      p_eth_download_cmd->start_address = NAND_UBOOT_ADDRESS;
      p_eth_download_cmd->erase_size = ETHDOWN_CMD_BOOTLOADER_SIZE;
      //strcpy(p_eth_download_cmd->bootfile, ETHDOWN_CMD_BOOTLOADER_FILENAME);

      printf("Start Bootloader image download. \n");
      down_filesize = ethdown_download_common(p_eth_download_cmd);

      if(down_filesize > 0x7C000 || down_filesize < 0)
      {
        printf("Invalid Bootloader Image\n");
        return -1;
      }
      break;

    case KERNEL_CMD:
      p_eth_download_cmd->download_address = DOWNLOAD_KERNEL_ADDRESS;
      p_eth_download_cmd->download_size = ETHDOWN_CMD_KERNEL_SIZE;
      p_eth_download_cmd->start_address = NAND_KERNEL_ADDRESS;
      p_eth_download_cmd->erase_size = ETHDOWN_CMD_KERNEL_SIZE;
      //strcpy(p_eth_download_cmd->bootfile, ETHDOWN_CMD_KERNEL_FILENAME);

      printf("Start Kernel image download. \n");
      down_filesize = ethdown_download_common(p_eth_download_cmd);

      if(down_filesize > 0x3C0000 || down_filesize < 0x180000 || down_filesize < 0)
      {
        printf("Invalid Kernel Image\n");
        return -1;
      }
      break;

    case ROOTFS_CMD:
      p_eth_download_cmd->download_address = DOWNLOAD_ROOTFS_ADDRESS;
      p_eth_download_cmd->start_address = NAND_ROOTFS_ADDRESS;
      p_eth_download_cmd->erase_size = ETHDOWN_CMD_ROOTFS_SIZE;
      //strcpy(p_eth_download_cmd->bootfile, ETHDOWN_CMD_ROOTFS_FILENAME);

      printf("Start Root Filesystem  image download. \n");

      down_filesize = ethdown_download_common(p_eth_download_cmd);

      if(down_filesize < 0x1800000 || down_filesize < 0)
      {
        printf("Invalid Root Filesystem Image\n");
        return -1;
      }
      p_eth_download_cmd->download_size = down_filesize;
      break;
 
    case LOGO_CMD:
      p_eth_download_cmd->download_address = DOWNLOAD_LOGO_ADDRESS;
      p_eth_download_cmd->start_address = NAND_LOGO_ADDRESS;
      p_eth_download_cmd->erase_size = ETHDOWN_CMD_LOGO_SIZE;

      printf("Start LOGO image download. \n");

      down_filesize = ethdown_download_common(p_eth_download_cmd);

      if(down_filesize < 0x100000 || down_filesize < 0)
      {
        printf("Invalid LOGO Image\n");
        return -1;
      }
      p_eth_download_cmd->download_size = down_filesize;
      break;
   
    default:
      printf("\n invalid command \n");
      return -1;
      break;
  }
  return 1;
}
#endif // CONFIG_CMD_ETHDOWN

