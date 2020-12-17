#include "oled.h"
#include "io.h"
#include "oledlogo.h"

#define ADDR_OF_OLED 0x88000000
#define mdelay(n) ({unsigned long msec=(n); while (msec--) udelay(1000);})

extern const unsigned short lstar0[];
extern const char FONT5X7[][5];
extern const char FONT8X15[][8][2];

//==================================================
//==================================================
void write_cmd(unsigned char cmd)
{
	__REG16(ADDR_OF_OLED) &= ~0xFF;

	__REG16(ADDR_OF_OLED) &= ~D_C;
	__REG16(ADDR_OF_OLED) &= ~CS_OLED;
	__REG16(ADDR_OF_OLED) &= ~WN_OLED;

	__REG16(ADDR_OF_OLED) |= cmd;

	__REG16(ADDR_OF_OLED) |= WN_OLED;
	__REG16(ADDR_OF_OLED) |= D_C;
	__REG16(ADDR_OF_OLED) |= CS_OLED;
}

void write_data(unsigned char data)
{
	__REG16(ADDR_OF_OLED) &= ~0xFF;

	__REG16(ADDR_OF_OLED) |= D_C;
	__REG16(ADDR_OF_OLED) &= ~CS_OLED;
	__REG16(ADDR_OF_OLED) &= ~WN_OLED;

	__REG16(ADDR_OF_OLED) |= data;

	__REG16(ADDR_OF_OLED) |= WN_OLED;
	__REG16(ADDR_OF_OLED) &= ~D_C;
	__REG16(ADDR_OF_OLED) |= CS_OLED;
}

void SetAddr(unsigned char x, unsigned char y)
{
	write_cmd(SET_COLUMN_ADDRESS); //colum аж╪р
	write_data(x);
	write_data(0x7f);

	write_cmd(SET_ROW_ADDRESS);
	write_data(y);
	write_data(0x7f);

	write_cmd(WRITE_GRAM);
}

//==================================================
// Plot one point
// at x,y with pixel color
//==================================================
void PutPixel(unsigned char x, unsigned char y, unsigned short color)
{
	SetAddr(x,y);
	write_data(color >> 8);
	write_data(color);
}

void Reset_SSD1355(void)
{
	__REG16(ADDR_OF_OLED) &= ~RESETPIN_OLED;
	udelay(2);
	__REG16(ADDR_OF_OLED) |= RESETPIN_OLED;
	udelay(2);
}

void initOLED(void)
{
	unsigned char j;

	__REG16(ADDR_OF_OLED) &= ~0xFF;
	__REG16(ADDR_OF_OLED) |= RN_OLED;
	__REG16(ADDR_OF_OLED) |= RESETPIN_OLED;
	__REG16(ADDR_OF_OLED) |= CS_OLED;
	__REG16(ADDR_OF_OLED) |= WN_OLED;
	__REG16(ADDR_OF_OLED) |= D_C;

	Reset_SSD1355();

	write_cmd(SOFT_RESET);

	write_cmd(COMMAD_LOCK);
	write_data(0x12);

	write_cmd(COMMAD_LOCK);
	write_data(0xB3);

	write_cmd(SLEEP_MODE_ON);

	write_cmd(CLOCK_FREQUENCY);
	write_data(0x10);

	write_cmd(SET_MUX_RATIO);
	write_data(0x7f);  //127

	write_cmd(SET_DISPLAY_OFFSET);
	write_data(0x00);

	write_cmd(MEMORY_ACCSEE_CNTL);
	write_data(0x88);
	write_data(0x01);

	write_cmd(INTERFACE_PIXEL_FORMAT);
	write_data(_65K_COLOURS);

	write_cmd(EN_T_EFFECT);
	write_data(0x00);

	write_cmd(FUNC_SEL);
	write_data(0x03);

	write_cmd(CONTRAST_RGB);
	write_data(0xc3);
	write_data(0x55);
	write_data(0x87);

	write_cmd(WRITE_LUMINANCE);
	write_data(0xF0);

	write_cmd(GAMMA_LUT);

	for(j=0;j<96;j++)
		write_data(gamma0[j]);

	write_cmd(SET_PHASE_LENGTH);
	write_data(0x32);

	write_cmd(FIRST_PRECHARGE);
	write_data(0x09);

	write_cmd(SECOND_PRECHARGE_PERIOD);
	write_data(0x0b);

	write_cmd(SET_2TH_PRECHARGE_SPEED);
	write_data(0x03);

	write_cmd(SET_VCOMH);
	write_data(0x04);

	write_cmd(DISPLAY_ALL_ONOFF);

	write_cmd(DISPLAY_INVERSE_OFF);

	write_cmd(DISPLAY_NORMAL);

	write_cmd(SLEEP_MODE_OFF);
}

void OLED_DrawImage()
{
	unsigned short i,j,w_count,h_count,k=16384;

	w_count=128;
	h_count=128;

	for(i=0; i<h_count;i++)
		for(j=w_count; j>0; j--)
			PutPixel(j,i,oledlogo[k--]);
}
