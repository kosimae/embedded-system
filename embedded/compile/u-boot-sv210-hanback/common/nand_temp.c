/*
 * common/nand_temp.c
 *
 *  $Id: cmd_android.c,v 1.2 2009/11/21 00:11:42 dark0351 Exp $
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
 * temporary code for nand access Function
 *
 */

#include <common.h>
#include <command.h>

#include <regs.h>

#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

#if defined(CONFIG_CMD_NANDTEMP)
#include <nand.h>
int _nand_dump(nand_info_t *nand, unsigned int off)
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

int nand_dump_temp(ulong off)
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

  ret = _nand_dump(nand, off);
}

#endif // CONFIG_CMD_NANDTEMP

