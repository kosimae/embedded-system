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

#if defined(CONFIG_CMD_ETHDOWN)

#define ETHDOWN_CMD_NAND_ERASE   "nand erase "
#define ETHDOWN_CMD_NAND_WRITE   "nand write "
#define ETHDOWN_CMD_NAND_WRITE_YAFFS2  "nand write.yaffs "
#define ETHDOWN_CMD_COMMAND_DIV  ";"

/* Bootloader images Download and Fusing Command */
#define ETHDOWN_CMD_BOOTLOADER_SIZE   0x80000
/* Kernel images Download and Fusing Command */
#define ETHDOWN_CMD_KERNEL_SIZE      0x3c0000
/* Root Filesystem images Download and Fusing Command */
#define ETHDOWN_CMD_ROOTFS_SIZE      0xf9c0000
/* Boot Logo images Download and Fusing Command */
#define ETHDOWN_CMD_LOGO_SIZE        0x200000

#define ETHDOWN_CMD_RESET            "reset;"
#define ETHDOWN_CMD_KERNEL_BOOT      "nand read c0008000 80000 3c0000;bootm c0008000;"
#define ETHDOWN_CMD_DEFAULT_MAC    "00:0b:a8:"


#define ANDROID_OK	1
#define ANDROID_FAIL	0

#define DOWNLOAD_BASE_ADDRESS	0x24000000
#define NAND_BASE_ADDRESS 0x0

#define UBOOT_ADDRESS     0x0
#define KERNEL_ADDRESS    0x80000
#define ROOTFS_ADDRESS    0x440000
#define LOGO_ADDRESS      0x8000000

#define DOWNLOAD_UBOOT_ADDRESS		DOWNLOAD_BASE_ADDRESS + UBOOT_ADDRESS
#define DOWNLOAD_KERNEL_ADDRESS		DOWNLOAD_BASE_ADDRESS + KERNEL_ADDRESS
#define DOWNLOAD_ROOTFS_ADDRESS		DOWNLOAD_BASE_ADDRESS + ROOTFS_ADDRESS
#define DOWNLOAD_LOGO_ADDRESS     DOWNLOAD_BASE_ADDRESS + LOGO_ADDRESS

#define NAND_UBOOT_ADDRESS  NAND_BASE_ADDRESS
#define NAND_KERNEL_ADDRESS NAND_BASE_ADDRESS + KERNEL_ADDRESS
#define NAND_ROOTFS_ADDRESS NAND_BASE_ADDRESS + ROOTFS_ADDRESS
#define NAND_LOGO_ADDRESS   NAND_BASE_ADDRESS + 0xFE00000

#define UBOOT_CMD 0x01
#define KERNEL_CMD 0x02
#define ROOTFS_CMD 0x03
#define LOGO_CMD 0x04

#if 1
#define TFTPSERVERIP  "192.168.123.100"
#define IPADDR        "192.168.123.165"
#else
#define TFTPSERVERIP  "192.168.0.100"
#define IPADDR        "192.168.0.165"
#endif

#define DEFAULT_SERVERIP  "192.168.0.160"
#define DEFAULT_IPADDR    "192.168.0.165"

#define	PRINTF(fmt,args...)	printf (fmt ,##args)

typedef struct
{
  u32 kind;
  u32 download_address;
  u32 download_size;
  u32 start_address;
  u32 erase_size;
  char bootfile[256];
  u32 test_ok;
  u32 up_ok;
} __attribute__ ((packed)) eth_download_cmd_t;

#endif // CONFIG_CMD_ETHDOWN

