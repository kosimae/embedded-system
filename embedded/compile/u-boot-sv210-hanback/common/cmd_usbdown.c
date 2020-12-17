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

#include <regs.h>

#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

#if defined(CONFIG_CMD_USBDOWN)

#if defined(CONFIG_S3C2412) || defined(CONFIG_S3C2442)
#include "../cpu/s3c24xx/usbd-fs.h"
#elif defined(CONFIG_S3C2443) || defined(CONFIG_S3C2450) || defined(CONFIG_S3C2416)
#include "../cpu/s3c24xx/usbd-hs.h"
#elif defined(CONFIG_S3C6400) || defined(CONFIG_S3C6410) || defined(CONFIG_S3C6430)
#include "../cpu/s3c64xx/usbd-otg-hs.h"
#elif defined(CONFIG_S5PC100)
#include "../cpu/s5pc1xx/usbd-otg-hs.h"
#elif defined(CONFIG_S5PC110)
#include "../cpu/s5pc11x/usbd-otg-hs.h"
#elif defined(CONFIG_S5P6440)
#include "../cpu/s5p64xx/usbd-otg-hs.h"
#elif defined(CONFIG_S5P6442)
#include "../cpu/s5p644x/usbd-otg-hs.h"
#else
#error "* CFG_ERROR : you have to setup right Samsung CPU configuration"
#endif

#define USBDOWN_CMD_NAND_ERASE   "nand erase "
#define USBDOWN_CMD_NAND_WRITE   "nand write "
#define USBDOWN_CMD_NAND_WRITE_YAFFS2  "nand write.yaffs "
#define USBDOWN_CMD_COMMAND_DIV  ";"

/* Bootloader images Download and Fusing Command */
#define USBDOWN_CMD_BOOTLOADER_SIZE   0x80000
/* Kernel images Download and Fusing Command */
#define USBDOWN_CMD_KERNEL_SIZE      0x3c0000
/* Root Filesystem images Download and Fusing Command */
#define USBDOWN_CMD_ROOTFS_SIZE      0xf9c0000
/* Boot Logo images Download and Fusing Command */
#define USBDOWN_CMD_LOGO_SIZE        0x200000

#define USBDOWN_CMD_RESET            "reset;"
#define USBDOWN_CMD_KERNEL_BOOT      "nand read c0008000 80000 3c0000;bootm c0008000;"
#define USBDOWN_CMD_DEFAULT_MAC    "00:0b:a8:"

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

#define	PRINTF(fmt,args...)	printf (fmt ,##args)

static const char pszMe[] = "usbd: ";

extern otg_dev_t otg;

typedef struct
{
  u32 kind;
  u32 download_address;
  u32 download_size;
  u32 start_address;
  u32 erase_size;
} __attribute__ ((packed)) usb_download_cmd_t;


#if 0 // temporary code for nand strage action
#include <nand.h>
static int nand_dump(nand_info_t *nand, unsigned int off)
{
  int i;
  u_char *buf, *p;
  struct nand_chip *chip = nand->priv;
  size_t len = nand->writesize;

  buf = malloc(nand->writesize + nand->oobsize);

  if (!buf) {
    puts("No memory for page buffer\n");
    return 1;
  }

  /* off &= ~(nand->writesize - 1); */

  i = nand_read(nand, off, (size_t *)&len, (u_char *)buf);

  /* if (i < 0) {
   *     printf("Error (%d) reading page %08x\n", i, off);
   *         free(buf);
   *             return 1;
   *               } */

#if 0
  printf("Page %08x dump:\n", off);
#endif

  i = nand->writesize >> 4;
  p = buf;

#if 0
  while (i--) {
    printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x"
        "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
        p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
        p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    p += 16;
  }
#endif

#if 0
  puts("OOB:\n");
#endif

  p = chip->oob_poi;
  i = nand->oobsize >> 3;

#if 0
  while (i--) {
    printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
        p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
    p += 8;
  }
#endif
  free(buf);

  return 0;
}


static int nand_dump_temp(ulong off)
{
  int ret;
  nand_info_t *nand;
  /* the following commands operate on the current device */
  if (nand_curr_device < 0 || nand_curr_device >= CFG_MAX_NAND_DEVICE ||
      !nand_info[nand_curr_device].name) {
    puts("\nno devices available\n");
    return 1;
  }
  nand = &nand_info[nand_curr_device];

  ret = nand_dump(nand, off);
}
#endif

otg_dev_t android_usbd_download(unsigned long dwn_address)
{
	otg_dev_t	nOtg;

	//s3c_usbd_dn_addr = DOWNLOAD_BASE_ADDRESS;
	s3c_usbd_dn_addr = dwn_address;
	s3c_receive_done = 0;

	s3c_usbctl_init();
	s3c_usbc_activate();

  if(dwn_address == DOWNLOAD_UBOOT_ADDRESS)
	  printf("Now, Waiting for Bootloader image downloading.\n");
  else if(dwn_address == DOWNLOAD_KERNEL_ADDRESS)
	  printf("Now, Waiting for Kernel image downloading.\n");
  else if(dwn_address == DOWNLOAD_ROOTFS_ADDRESS)
	  printf("Now, Waiting for Root Filesystem image downloading.\n");
  else if(dwn_address == DOWNLOAD_LOGO_ADDRESS)
	  printf("Now, Waiting for Logo image downloading.\n");
    
	printf("Press the 'USB Port => Download' button.\n");

	while (1) {
		if (S3C_USBD_DETECT_IRQ()) {
			s3c_udc_int_hndlr();
			S3C_USBD_CLEAR_IRQ();
		}

		if (s3c_receive_done)
			break;

		if (serial_tstc()) {
			serial_getc();
			break;
		}
	}

#if 0 // by netmania
	nOtg.dn_addr 		= s3c_usbd_dn_addr;
	nOtg.dn_filesize 	= s3c_usbd_dn_cnt >> 18;
	s3c_usbd_dn_cnt 	= (nOtg.dn_filesize << 18) + 0x40000;
	nOtg.dn_filesize 	= s3c_usbd_dn_cnt;
#endif
	/* when operation is done, usbd must be stopped */
	s3c_usb_stop();

	return nOtg;
}

static void parsing_command(char *command, usb_download_cmd_t usb_download_cmd)
{
  char cmd_temp1[256], cmd_temp2[256];

  strcpy(command, USBDOWN_CMD_NAND_ERASE);

  sprintf(cmd_temp1, "%x", usb_download_cmd.start_address);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(cmd_temp2, " ");
  strcat(command, cmd_temp2);

  sprintf(cmd_temp1, "%x", usb_download_cmd.erase_size);

  strcpy(cmd_temp2, cmd_temp1);
  strcat(cmd_temp2, USBDOWN_CMD_COMMAND_DIV);
  strcat(command, cmd_temp2);

  if(usb_download_cmd.kind == ROOTFS_CMD)
    strcat(command, USBDOWN_CMD_NAND_WRITE_YAFFS2);
  else
    strcat(command, USBDOWN_CMD_NAND_WRITE);

  sprintf(cmd_temp1, "%x", usb_download_cmd.download_address);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(cmd_temp2, " ");
  strcat(command, cmd_temp2);

  sprintf(cmd_temp1, "%x", usb_download_cmd.start_address);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(command, cmd_temp2);
  strcat(command, " ");

  sprintf(cmd_temp1, "%x", usb_download_cmd.download_size);
  strcpy(cmd_temp2, cmd_temp1);
  strcat(command, cmd_temp2);
  strcat(command, USBDOWN_CMD_COMMAND_DIV);
  //printf("%s\n", command);
}

int do_usbdown(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
  int i, j, dev, ret;
  char *cmd, *s, *temp1, *temp2;
  int quiet = 0;
  const char *quiet_str = getenv("quiet");
  char up_cmd_str[8192], up_all_cmd_str[8192], up_cmd_mac[17];
  const char *mac;
  otg_dev_t	nOtg;
	size_t 		retlen = 0;
  usb_download_cmd_t usb_download_cmd;

  /* at least two arguments please */
  if (argc < 2)
    goto usage;

  if (quiet_str)
    quiet = simple_strtoul(quiet_str, NULL, 0) != 0;

  cmd = argv[1];

  if(cmd[0] == '-')
  {
    if(strlen(cmd) > 1 && strlen(cmd) < 6) {
      for(i = 0; i < strlen(cmd) -1; i++){
        switch (cmd[i+1]) {
          case 'b':
	          android_usbd_download(DOWNLOAD_UBOOT_ADDRESS);
            if((otg.dn_filesize - 10) > 0x7C000 || otg.dn_filesize == 0)
            {
              printf("Invalid Bootloader Image\n");
              return -1;
            }
            usb_download_cmd.kind = UBOOT_CMD;
            usb_download_cmd.download_address = DOWNLOAD_UBOOT_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_BOOTLOADER_SIZE;
            usb_download_cmd.start_address = NAND_UBOOT_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_BOOTLOADER_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            break;

          case 'k':
            android_usbd_download(DOWNLOAD_KERNEL_ADDRESS);
            if((otg.dn_filesize - 10) > 0x3C0000 || (otg.dn_filesize - 10) < 0x180000 || otg.dn_filesize == 0)
            {
              printf("Invalid Kernel Image\n");
              return -1;
            }
            usb_download_cmd.kind = KERNEL_CMD;
            usb_download_cmd.download_address = DOWNLOAD_KERNEL_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_KERNEL_SIZE;
            usb_download_cmd.start_address = NAND_KERNEL_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_KERNEL_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            break;

          case 'r':
            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            android_usbd_download(DOWNLOAD_ROOTFS_ADDRESS);
            if((otg.dn_filesize - 10) < 0x1800000 || otg.dn_filesize == 0)
            {
              printf("Invalid Root Filesystem Image\n");
              return -1;
            }
            usb_download_cmd.kind = ROOTFS_CMD;
            usb_download_cmd.download_address = DOWNLOAD_ROOTFS_ADDRESS;
            usb_download_cmd.download_size = (otg.dn_filesize - 10);
            usb_download_cmd.start_address = NAND_ROOTFS_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_ROOTFS_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            break;

          case 'l':
            android_usbd_download(DOWNLOAD_LOGO_ADDRESS);
            if((otg.dn_filesize - 10) > 0x200000 || otg.dn_filesize == 0)
            {
              printf("Invalid Logo Image\n");
              return -1;
            }
            usb_download_cmd.kind = LOGO_CMD;
            usb_download_cmd.download_address = DOWNLOAD_LOGO_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_LOGO_SIZE;
            usb_download_cmd.start_address = NAND_LOGO_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_LOGO_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            break;

          case 'a':
            if(strlen(cmd) > 2)
              goto usage;

	          android_usbd_download(DOWNLOAD_UBOOT_ADDRESS);
            if((otg.dn_filesize - 10) > 0x7C000 || otg.dn_filesize == 0)
            {
              printf("Invalid Bootloader Image\n");
              return -1;
            }
            usb_download_cmd.kind = UBOOT_CMD;
            usb_download_cmd.download_address = DOWNLOAD_UBOOT_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_BOOTLOADER_SIZE;
            usb_download_cmd.start_address = NAND_UBOOT_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_BOOTLOADER_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcpy(up_all_cmd_str, up_cmd_str);
            printf("\n");

            android_usbd_download(DOWNLOAD_KERNEL_ADDRESS);
            if((otg.dn_filesize - 10) > 0x3C0000 || (otg.dn_filesize - 10) < 0x200000 || otg.dn_filesize == 0)
            {
              printf("Invalid Kernel Image\n");
              return -1;
            }
            usb_download_cmd.kind = KERNEL_CMD;
            usb_download_cmd.download_address = DOWNLOAD_KERNEL_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_KERNEL_SIZE;
            usb_download_cmd.start_address = NAND_KERNEL_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_KERNEL_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);
            printf("\n");

            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            android_usbd_download(DOWNLOAD_ROOTFS_ADDRESS);
            if((otg.dn_filesize - 10) < 0x1800000 || otg.dn_filesize == 0)
            {
              printf("Invalid Root Filesystem Image\n");
              return -1;
            }

            usb_download_cmd.kind = ROOTFS_CMD;
            usb_download_cmd.download_address = DOWNLOAD_ROOTFS_ADDRESS;
            usb_download_cmd.download_size = (otg.dn_filesize - 10);
            usb_download_cmd.start_address = NAND_ROOTFS_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_ROOTFS_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);
            printf("\n");

            android_usbd_download(DOWNLOAD_LOGO_ADDRESS);
            if((otg.dn_filesize - 10) > 0x200000 || otg.dn_filesize == 0)
            {
              printf("Invalid Logo Image\n");
              return -1;
            }
            usb_download_cmd.kind = LOGO_CMD;
            usb_download_cmd.download_address = DOWNLOAD_LOGO_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_LOGO_SIZE;
            usb_download_cmd.start_address = NAND_LOGO_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_LOGO_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
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
#if 1
    if (strcmp(cmd, "test") == 0) {
      if (argc > 3)
        goto usage;
	          android_usbd_download(DOWNLOAD_UBOOT_ADDRESS);
            if((otg.dn_filesize - 10) > 0x7C000 || otg.dn_filesize == 0)
            {
              printf("Invalid Bootloader Image\n");
              return -1;
            }
            usb_download_cmd.kind = UBOOT_CMD;
            usb_download_cmd.download_address = DOWNLOAD_UBOOT_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_BOOTLOADER_SIZE;
            usb_download_cmd.start_address = NAND_UBOOT_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_BOOTLOADER_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcpy(up_all_cmd_str, up_cmd_str);
            printf("\n");

            android_usbd_download(DOWNLOAD_KERNEL_ADDRESS);
            if((otg.dn_filesize - 10) > 0x3C0000 || (otg.dn_filesize - 10) < 0x200000 || otg.dn_filesize == 0)
            {
              printf("Invalid Kernel Image\n");
              return -1;
            }
            usb_download_cmd.kind = KERNEL_CMD;
            usb_download_cmd.download_address = DOWNLOAD_KERNEL_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_KERNEL_SIZE;
            usb_download_cmd.start_address = NAND_KERNEL_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_KERNEL_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);
            printf("\n");

            nand_dump_temp(NAND_ROOTFS_ADDRESS);
            android_usbd_download(DOWNLOAD_ROOTFS_ADDRESS);
            if((otg.dn_filesize - 10) < 0x1800000 || otg.dn_filesize == 0)
            {
              printf("Invalid Root Filesystem Image\n");
              return -1;
            }

            usb_download_cmd.kind = ROOTFS_CMD;
            usb_download_cmd.download_address = DOWNLOAD_ROOTFS_ADDRESS;
            usb_download_cmd.download_size = (otg.dn_filesize - 10);
            usb_download_cmd.start_address = NAND_ROOTFS_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_ROOTFS_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);
            printf("\n");

            android_usbd_download(DOWNLOAD_LOGO_ADDRESS);
            if((otg.dn_filesize - 10) > 0x200000 || otg.dn_filesize == 0)
            {
              printf("Invalid Logo Image\n");
              return -1;
            }
            usb_download_cmd.kind = LOGO_CMD;
            usb_download_cmd.download_address = DOWNLOAD_LOGO_ADDRESS;
            usb_download_cmd.download_size = USBDOWN_CMD_LOGO_SIZE;
            usb_download_cmd.start_address = NAND_LOGO_ADDRESS;
            usb_download_cmd.erase_size = USBDOWN_CMD_LOGO_SIZE;
            parsing_command(up_cmd_str, usb_download_cmd);
            strcat(up_all_cmd_str, up_cmd_str);

            strcat(up_all_cmd_str, USBDOWN_CMD_RESET);
            parse_string_outer(up_all_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
            return 1;

    } else if (strcmp(cmd, "pp") == 0) {
      if (argc < 3)
        goto usage;

      mac = argv[2];

      if(strlen(mac) < 6)
        goto usage;

      strcpy(up_cmd_mac, USBDOWN_CMD_DEFAULT_MAC);

      for (j=0; j < 3; j++) {
        strncat(up_cmd_mac, &mac[j*2], 2);
        if (j != 2)
          strcat(up_cmd_mac, ":");

      }
      

      android_usbd_download(DOWNLOAD_UBOOT_ADDRESS);
      if((otg.dn_filesize - 10) > 0x7C000 || otg.dn_filesize == 0)
      {
        printf("Invalid Bootloader Image\n");
        return -1;
      }
      usb_download_cmd.kind = UBOOT_CMD;
      usb_download_cmd.download_address = DOWNLOAD_UBOOT_ADDRESS;
      usb_download_cmd.download_size = USBDOWN_CMD_BOOTLOADER_SIZE;
      usb_download_cmd.start_address = NAND_UBOOT_ADDRESS;
      usb_download_cmd.erase_size = USBDOWN_CMD_BOOTLOADER_SIZE;
      parsing_command(up_cmd_str, usb_download_cmd);
      strcpy(up_all_cmd_str, up_cmd_str);
      printf("\n");

      android_usbd_download(DOWNLOAD_KERNEL_ADDRESS);
      if((otg.dn_filesize - 10) > 0x3C0000 || (otg.dn_filesize - 10) < 0x200000 || otg.dn_filesize == 0)
      {
        printf("Invalid Kernel Image\n");
        return -1;
      }
      usb_download_cmd.kind = KERNEL_CMD;
      usb_download_cmd.download_address = DOWNLOAD_KERNEL_ADDRESS;
      usb_download_cmd.download_size = USBDOWN_CMD_KERNEL_SIZE;
      usb_download_cmd.start_address = NAND_KERNEL_ADDRESS;
      usb_download_cmd.erase_size = USBDOWN_CMD_KERNEL_SIZE;
      parsing_command(up_cmd_str, usb_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);
      printf("\n");

      nand_dump_temp(NAND_ROOTFS_ADDRESS);
      android_usbd_download(DOWNLOAD_ROOTFS_ADDRESS);
      if((otg.dn_filesize - 10) < 0x1800000 || otg.dn_filesize == 0)
      {
        printf("Invalid Root Filesystem Image\n");
        return -1;
      }

      usb_download_cmd.kind = ROOTFS_CMD;
      usb_download_cmd.download_address = DOWNLOAD_ROOTFS_ADDRESS;
      usb_download_cmd.download_size = (otg.dn_filesize - 10);
      usb_download_cmd.start_address = NAND_ROOTFS_ADDRESS;
      usb_download_cmd.erase_size = USBDOWN_CMD_ROOTFS_SIZE;
      parsing_command(up_cmd_str, usb_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);
      printf("\n");

      android_usbd_download(DOWNLOAD_LOGO_ADDRESS);
      if((otg.dn_filesize - 10) > 0x200000 || otg.dn_filesize == 0)
      {
        printf("Invalid Logo Image\n");
        return -1;
      }
      usb_download_cmd.kind = LOGO_CMD;
      usb_download_cmd.download_address = DOWNLOAD_LOGO_ADDRESS;
      usb_download_cmd.download_size = USBDOWN_CMD_LOGO_SIZE;
      usb_download_cmd.start_address = NAND_LOGO_ADDRESS;
      usb_download_cmd.erase_size = USBDOWN_CMD_LOGO_SIZE;
      parsing_command(up_cmd_str, usb_download_cmd);
      strcat(up_all_cmd_str, up_cmd_str);

      strcat(up_all_cmd_str, "set ethaddr ");
      strcat(up_all_cmd_str, up_cmd_mac);
      strcat(up_all_cmd_str, ";saveenv;");
      strcat(up_all_cmd_str, USBDOWN_CMD_RESET);
      parse_string_outer(up_all_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
      return 1;

    }
#endif
  }

  parse_string_outer(up_cmd_str, FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
  return 1;

usage:
  printf("Usage:\n%s\n", cmdtp->usage);
  return 1;
}

U_BOOT_CMD(
	usbdown, 3, 1, do_usbdown,
  "usbdown  - Board Image Download and Write\n",
  " [Options] - write options images\n"
  " Options :\n"
  "  -b : write bootloader image\n"
  "  -k : write kernel image\n"
  "  -r : write rootfilesystem image\n"
  "  -l : write logo image\n"
#if 0
  "  -a : write all images[bootloader/kernel/rootfs/logo] \n"
  "  example) usbdown -bk \n"
  "usbdown pp [mac] - write last images [bootloader/kernel/rootfs/logo/mac]\n"
  "  example) usbdown pp 031301 \n"
#endif
);

#endif // CONFIG_CMD_ONENAND

