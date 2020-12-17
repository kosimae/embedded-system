#include "system.h"
#include "lcd.h"
#include "lcd_module.h"
#include "gpio.h"
#include "glib.h"
#include "sysc.h"
#include "def.h"

#define _DRAM0_BaseAddress    0x20000000
#define _DRAM_BaseAddress   _DRAM0_BaseAddress
#define CODEC_MEM_ST      (_DRAM_BaseAddress+0x01000000)

static LCD_WINDOW eBgWin, eFgWin, eFg1Win,eFg2Win,eFg3Win;
static IMG_FMT eBgBpp;
static IMG_FMT eFgBpp, eFgArgbBpp;

static bool bIsArgb = false;
static bool bIsRev = false;
static bool bIsAllWin = false;

void TestSimpleDisplay(void)
{
  u32 uFgAddr;
  u32 uHSz, uVSz;
  u32 uLcdFbAddr = CODEC_MEM_ST;

  bIsRev = false;
  bIsAllWin = false;

  eBgWin = 2;
  eFgWin = 2;

  SYSC_UpdateClkInform();

  GPIO_Init();

  printf("debug01\n");
  LCDC_InitLdi(MAIN);
  printf("debug02\n");


  // 1. Set background
  printf("TestSimpleDisplay()==>debug03\n");
  LCDC_InitIp(eBgBpp, uLcdFbAddr, eBgWin, false);
  printf("TestSimpleDisplay()==>debug04\n");
  LCDC_SetWinOnOff(1, eBgWin);
  printf("TestSimpleDisplay()==>debug05\n");
  LCDC_GetFrmSz(&uHSz, &uVSz, eBgWin);
  printf("TestSimpleDisplay()==>debug06\n");

  GLIB_InitInstance(uLcdFbAddr, uHSz, uVSz, eBgBpp);
  GLIB_DrawPattern(uHSz, uVSz);
  GLIB_PutLine(0, 0, 0, uVSz-1, C_BLUE);
  LCDC_Start(); 
  LCDC_WTrigger(eBgWin);

  // 2. Set foreground
  LCDC_GetFbEndAddr(&uFgAddr, eBgWin);
  LCDC_InitWin(eFgBpp, uHSz/2, uVSz/2, 0, 0, uHSz/2, uVSz/2,  60, 80, uFgAddr, eFgWin, false);  

  GLIB_InitInstance(uFgAddr, uHSz/2, uVSz/2, eFgBpp);
  GLIB_DrawPattern(uHSz/2, uVSz/2);
  GLIB_PutLine(0, 0, 0, uVSz-1, C_BLUE);
  LCDC_WTrigger(eFgWin);

  // Test case in which img. size is smaller than LCD size in one window.
  LCDC_SetAllWinOnOff(0);
  LCDC_InitWin(eBgBpp, uHSz/2, uVSz/2, 0, 0, uHSz/2, uVSz/2,  60, 80, uFgAddr, eBgWin, false);  

  GLIB_InitInstance(uLcdFbAddr, uHSz/2, uVSz/2, eBgBpp);
  GLIB_DrawPattern(uHSz/2, uVSz/2);
  LCDC_WTrigger(eBgWin);

}
