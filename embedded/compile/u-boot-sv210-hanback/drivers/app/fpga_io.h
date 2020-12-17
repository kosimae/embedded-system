#ifndef __FPGA_IO_H__
#define __FPGA_IO_H__

#include <common.h>
#include <command.h>

#define mdelay(n)       udelay((n)*1000)

/* FPGA TEXTLCD ADDRESS */
#define ADDR_OF_LCD         0x88000010
/* FPGA LED ADDRESS */
#define ADDR_OF_LED         0x88000020
/* FPGA 7-SEGMENT ADDRESS */
#define ADDR_OF_7SEG_GRID   0x88000030
#define ADDR_OF_7SEG_DATA   0x88000032
/* FPGA DOT LED ADDRESS */
#define DOTLED_ROW          0x88000040
#define DOTLED_COL          0x88000042
/* FPGA PIEZO ADDRESS */
#define ADDR_OF_BUZ         0x88000050
/* FPGA DIP SWITCH ADDRESS */
#if defined(CONFIG_EMPOS3SV210)
#define ADDR_OF_DIP_CTL     0x88000060
#endif
#define ADDR_OF_DIP_DATA    0x88000062
/* FPGA KEYPAD ADDRESS */
#define ADDR_OF_KEY_LD      0x88000072
#define ADDR_OF_KEY_RD      0x88000070

#endif
