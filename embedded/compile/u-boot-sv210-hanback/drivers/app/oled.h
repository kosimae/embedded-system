#ifndef OLED_H
#define OLED_H

#include <common.h>

//--------------------------------------------------------------------------//
// General definitions
//--------------------------------------------------------------------------//
#define	uint  unsigned int
#define	uchar unsigned char

#define ON		1
#define	OFF		0
#define INPUT 		1
#define OUTPUT		0
#define	ENABLE		1
#define	DISABLE		0
#define	SET		1
#define	CLEAR		0
//#define TRUE		1
#define	FALSE		0

//--------------------------------------------------------------------------//
// OLED Interface
//--------------------------------------------------------------------------//
#define	CS_OLED		0x1000		// OLED CS
#define	RESETPIN_OLED	0x800
#define	D_C		0x400
#define WN_OLED		0x200 		// OLED WN : R/W#(WR#)
#define	RN_OLED		0x100		// OLED RN : E(RD#)

#define OLED_CTRL_PORT	PORTG
#define	OLED_DATABUS	PORTA

//--------------------------------------------------------------------------//
// SSD1355 Registers
//--------------------------------------------------------------------------//
#define SET_COLUMN_ADDRESS              0x2A
#define SET_ROW_ADDRESS                 0x2B
#define WRITE_GRAM                      0x2C
#define MEMORY_ACCSEE_CNTL              0x36
#define INTERFACE_PIXEL_FORMAT          0x3A
#define EN_T_EFFECT                     0x35
#define SET_MUX_RATIO                   0xCA
#define SET_DISPLAY_OFFSET              0xC8
#define FUNC_SEL                        0xB3
#define CLOCK_FREQUENCY                 0xD2

#define CONTRAST_RGB                    0xBA
#define WRITE_LUMINANCE                 0x51

#define DISPLAY_ALL_ONOFF               0x29
#define DISPLAY_INVERSE_OFF             0x20
#define DISPLAY_NORMAL                  0x13

#define GAMMA_LUT                       0xBE

#define SET_PHASE_LENGTH                0xCD
#define FIRST_PRECHARGE                 0xBD
#define SECOND_PRECHARGE_PERIOD         0xCE

#define SET_2TH_PRECHARGE_SPEED         0xCE
#define SET_VCOMH                       0xD3

#define SLEEP_MODE_ON                   0x10
#define SLEEP_MODE_OFF                  0x11

#define COMMAD_LOCK                     0xFD
#define SOFT_RESET                      0x01

#define _65K_COLOURS                    0x05
#define _262K_COLOURS                   0x06

#define RGB565(r,g,b) ( (((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3) )

const unsigned char gamma0[] = {
	0x01,0x04,0x07,0x09,
	0x0B,0x0C,0x0D,0x0E,
	0x0F,0x10,0x11,0x12,
	0x13,0x16,0x1d,0x21,
	0x23,0x24,0x2E,0x31,
	0x32,0x37,0x41,0x43,
	0x4c,0x51,0x54,0x61,
	0x68,0x6f,0x75,0x7f,//

	0x01,0x08,0x0E,0x13,
	0x17,0x1a,0x1c,0x1e,
	0x1f,0x20,0x21,0x22,
	0x23,0x25,0x2b,0x30,
	0x31,0x33,0x37,0x39,
	0x41,0x43,0x47,0x50,
	0x52,0x55,0x61,0x63,
	0x64,0x70,0x74,0x7f,//

	0x01,0x04,0x07,0x09,
	0x0B,0x0C,0x0D,0x0E,
	0x0F,0x10,0x11,0x12,
	0x13,0x16,0x1E,0x21,
	0x22,0x25,0x2d,0x30,
	0x32,0x37,0x41,0x43,
	0x4c,0x51,0x56,0x62,
	0x69,0x70,0x77,0x7f
};

//--------------------------------------------------------------------------//
// Screen Related definitions
//--------------------------------------------------------------------------//
#define X_RES			128
#define Y_RES			128

#define MAXCOLOUR		0xFFFF
#define	MAX_TEXTROWS		25

#define BLACK			0x0000
#define WHITE			0xFFFF
#define	RED			0xF800
#define	GREEN			0x07E0
#define	BLUE			0x001F
#define	YELLOW			RED | GREEN

#define	set_full_screen()\
	write_cmd(SET_COLUMN_ADDRESS);\
write_data(0x00);\
write_data(0x7F);\
write_cmd(SET_ROW_ADDRESS);\
write_data(0x00);\
write_data(0x7F)

#define backLight_DIM() \
	write_c(CONTRAST_MASTER); \
write_d(0x07)

#define backLight_BRIGHT() \
	write_c(CONTRAST_MASTER); \
write_d(0x0F)

#endif
