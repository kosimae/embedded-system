#ifndef __LCD_H__
#define __LCD_H__

#include <common.h>


#define LCD_FRM_RATE     60

enum
{
  LCD_HCLK,
  LCD_SCLK 
};

#if 0
#define LCD_VCLKSRC  LCD_SCLK
#else
#define LCD_VCLKSRC  LCD_HCLK
#endif

#define CS_SETUP_TIME           (0xf&0xf)
#define WR_SETUP_TIME          (0xf&0xf)
#define WR_ACT_TIME              (0xf&0xf)
#define WR_HOLD_TIME            (0xf&0xf)

typedef enum 
{
    ALPHA_4BIT, ALPHA_8BIT
}ALPHA_WIDTH;

typedef enum
{
  LCD_eXXTI = 0,LCD_eXusbXTI = 1,LCD_eMPLL = 6,LCD_eEPLL = 7, LCD_eVPLL =8, LCD_eHCLK = 10,
}LCD_SEL_CLKSRC;

typedef enum
{
  WIN0=0, WIN1=1, WIN2=2, WIN3=3, WIN4=4, ALLWIN
}LCD_WINDOW;

typedef enum 
{
  MAIN, SUB
} CPUIF_LDI;

typedef enum
{
  LCDC_BUF_SINGLE,LCDC_BUF_DOUBLE, LCDC_BUF_TRIPLE
}LCDC_AUTO_BUFFER_MODE;

typedef enum 
{
  IN_POST, IN_CIM
} LCD_LOCAL_INPUT;

typedef enum
{
  LOCAL_IF_RGB24,
  LOCAL_IF_YCBCR444
} LOCAL_IF_TYPE;

typedef struct
{ 
  u32 m_uLcdHSz, m_uLcdVSz;
  u32 m_uLcdStX, m_uLcdStY;

  u32 m_uImgStAddr[5], m_uDoubleBufImgStAddr[5],m_uTripleBufImgStAddr[5];
  u32 m_uFbStAddr[5], m_uFbEndAddr[5];
  u32 m_uDoubleBufStAddr[5], m_uDoubleBufEndAddr[5];
  u32 m_uTripleBufStAddr[5], m_uTripleBufEndAddr[5];  
  u32 m_uFrmHSz[5], m_uFrmVSz[5];
  u32 m_uViewHSz[5], m_uViewVSz[5];
  u32 m_uPageWidth[5], m_uOffsetSz[5];

  u32 m_uBppMode;

  u32 m_uBytes, m_uBits;
  u32 m_uBitSwpMode,m_uBytSwpMode, m_uHawSwpMode, m_uWordSwpMode;

  u32 m_uMaxBurstLen;
  u32 m_uDitherMode;

  LCDC_AUTO_BUFFER_MODE m_bAutoBuf;

  u32 m_uScanMode; // 0: Progressive, 1: Interlace 

  bool m_bFifoIn[3]; // false: DMA Path, true: Local Path
  LCD_LOCAL_INPUT m_eLocalIn[3]; // 0: CIM, 1: POST
  LOCAL_IF_TYPE m_uLocalInColorSpace[3];
#if 0
#ifdef LDI_INIT_GPIO
#else
  SPI oSpi;
#endif
#endif
}LCDC;

//void IEIP_Select(MDNIE_SEL eSel);


void LCDC_InitGpioMux(CPUIF_LDI eLdiMode);
void LCDC_InitGpioMuxSel(u32 uLCDmodel, CPUIF_LDI eLdiMode);
//void LCDC_InitGpioDualLCD(LCD_SEL_DLCD eSelLCD);
//void LCDC_InitGpioDualSel(u32 uLCDmodel, LCD_SEL_DLCD eSelLCD);
void LCDC_InitLdi(CPUIF_LDI eLdiMode);  
void LCDC_InitLdiSel(u32 uLCDmodel,CPUIF_LDI eLdiMode);
void LCDC_InitIp(IMG_FMT eBpp, u32 uFbAddr, LCD_WINDOW eWin, bool bIsDoubleBuf);
//void LCDC_InitIpDualFs(IMG_FMT eBpp, u32 uFbAddr, LCD_WINDOW eWinA,LCD_WINDOW eWinB, bool bIsDoubleBuf,LCD_SCAN_TYPE eScan);
//void LCDC_InitIpDualLs(IMG_FMT eBpp, u32 uFbAddr, LCD_WINDOW eWinA,LCD_WINDOW eWinB, bool bIsDoubleBuf,LCD_SCAN_TYPE eScan);
//void LCDC_InitIpDualLsSel(u32 uLCDmodel, IMG_FMT eBpp, u32 uFbAddr, LCD_WINDOW eWinA, LCD_WINDOW eWinB, bool bIsDoubleBuf, LCD_SCAN_TYPE eScan);

//LCDC_InitIpDualp is temporary code for debug 800x480 landscape line split test
//void LCDC_InitIpDualp(IMG_FMT eBpp, u32 uFbAddr,u32 uHsize, LCD_WINDOW eWinA,LCD_WINDOW eWinB, bool bIsDoubleBuf,LCD_SCAN_TYPE eScan);

void LCDC_InitBase(void); 
void LCDC_InitBaseDualp(u32 uHsize);  //LCDC_InitBaseDualp is temporary code for debug 800x480 landscape line split test
void LCDC_InitBaseDualFs(void);
void LCDC_InitBaseDualLs(void);
void LCDC_InitBaseDualLsSel(u32 uLCDmodel);
void LCDC_Start(void);
void LCDC_Stop(void); // Direct off
void LCDC_PerFrameOff(void);

//void LCDC_InitItuBase(ITU_R_STANDARD eItu, u32 uFrameHsz, u32 uFrameVsz, SCAN_MODE eScanMode);
//void LCDC_SetItuMode(ITU_R_STANDARD eItu, SCAN_MODE eScanMode);

void LCDC_InitWin(IMG_FMT eBpp, u32 uFrameH, u32 uFrameV, u32 uX_Frame, u32 uY_Frame,  u32 uViewH, u32 uViewV,
        u32 uX_Lcd, u32 uY_Lcd, u32 uFbAddr, LCD_WINDOW eWin, bool bIsDoubleBuf); // For WIN0

// In Fifo Input mode, Interface Data Format is RGB 
void LCDC_SetWinFrmBfAddr(u32 uFbAddr, LCD_WINDOW eWin,bool bIsDoubleBuf);
void LCDC_SetWinShadowP(LCD_WINDOW eWin, u32 eEn);
//void LCDC_SetLocalEn(LCD_LOCAL_EN eLocalEn, LCD_WINDOW eWin);

void LCDC_SetVpOnOff(u32 uOnOff);
void LCDC_SetFifoInSrcFmt(LCD_WINDOW eWin, IMG_FMT eBpp);
void LCDC_SetWinOnOff(u32 uOnOff, LCD_WINDOW eWin);
void LCDC_SetAllWinOnOff(u32 uOnOff);

void LCDC_SetViewPosOnLcd(u32 uX_Lcd, u32 uY_Lcd, LCD_WINDOW eWin);
void LCDC_SetViewPosOnFrame(u32 uX_Frame, u32 uY_Frame, LCD_WINDOW eWin); // For WIN0~4
void LCDC_SetViewPosOnLcddelay(u32 uX_Lcd, u32 uY_Lcd, LCD_WINDOW eWin); // Only for test shadow protect
void LCDC_SetFreeRun(u32 uFreeEn);
//void LCDC_SetRGBSkip(RGBSKIP_EN eRGBskipEn);
//void LCDC_SetRGBdummy(DUMMY_POSITION ePosition, RGBDUMMY_EN eDummyEn);


void LCDC_GetDoubleBufAddr(u32* uFbAddr, u32 uWhichAddr, LCD_WINDOW eWin);
void LCDC_GetBufIdx(u32* uBufNum, LCD_WINDOW eWin);
void LCDC_SetActiveFrameBuf(u32 uWhichBuf, LCD_WINDOW eWin);  
void LCDC_EnableAutoBuf(LCDC_AUTO_BUFFER_MODE uMode, LCD_WINDOW eWin);
//void LCDC_ChangePaletteClk(PAL_CLK ePalClkSrc); 
void LCDC_SetPaletteCtrl(IMG_FMT eBpp, LCD_WINDOW eWin);
void LCDC_SetPaletteData(u32 uIdx, u32 uPalData, LCD_WINDOW eWin);
void LCDC_SetColorMapValue(u32 uVal, LCD_WINDOW eWin);
void LCDC_SetColorMapOnOff(u32 uOnOff, LCD_WINDOW eWin);
void LCDC_SetAllWinColorMapOnOff(u32 uOnOff);
void LCDC_SetAlpha(u32 uA0R, u32 uA0G, u32 uA0B, u32 uA1R, u32 uA1G, u32 uA1B, LCD_WINDOW eWin);
void LCDC_SetAlphaClear(LCD_WINDOW eWin);
//void LCDC_SetBlendingType(BLENDING_APPLIED_UNIT eBldPix, BLENDING_SELECT_MODE eBldMode, LCD_WINDOW eWin);
void LCDC_SetBlendingEquation(u32 uBackAlphaValFactor, u32 uForeAlphaValFactor, u32 uBackDataFactor, u32 uForeDataFactor, LCD_WINDOW eWin);
void LCDC_SetAlphaValWidth(ALPHA_WIDTH eAlphaValWidth);
//void LCDC_SetRgbFormat(PNR_MODE ePnr);
//void LCDC_SetRGBOrder(RGB_ORDER eEvenOrder, RGB_ORDER eOddOrder);
//void LCDC_SetByteSwap(SWAP_MODE eBitSwap, SWAP_MODE eByteSwap, SWAP_MODE eHawSwap, SWAP_MODE eWordSwap, LCD_WINDOW eWin); 
//void LCDC_SetBurstLength(BURST_MODE eBurstLength, LCD_WINDOW eWin);
void LCDC_SetColorKeyOnOff(u32 uOnOff, LCD_WINDOW eWin);
void LCDC_SetAllWinColorKeyOnOff(u32 uOnOff);
//void LCDC_SetColorKey(u32 uMaskKey, u32 uColorKey, COLOR_KEY_DIRECTION eDir, bool bIsBlending, LCD_WINDOW eWin);
void LCDC_SetColorKeyBlackOut(u32 uOnOff, LCD_WINDOW eWin);
//void LCDC_SetScanMode(SCAN_MODE eScanMode);
//void LCDC_SetOutputPath(LCD_LOCAL_OUTPUT eOutLocal);
//void LCDC_SetWBFrameSkip(LCD_WB_SKIP eSkip);
//void LCDC_SetDualDisplay(LCD_DUAL_METHOD eSelMethod, LCD_SEL_DUAL eSelDual,LCD_DUAL_VDEN_EN eSelVden);
//void LCDC_SetDualDisplaySel(u32 uLCDmodel, LCD_DUAL_METHOD eSelMethod, LCD_SEL_DUAL eSelDual, LCD_DUAL_VDEN_EN eSelVden);
void LCDC_SetFifoOut(void);
void LCDC_SetClkVal(u8 uClkVal);
void LCDC_SetrScFmtInFifo(IMG_FMT eBpp, LCD_WINDOW eWin);
void LCDC_GetLcdSz(u32* uLcdHSz, u32* uLcdVSz);
void LCDC_GetFrmSz(u32* uFrmHSz, u32* uFrmVSz, LCD_WINDOW eWin);
void LCDC_GetFbStAddr(u32* uFbAddr, LCD_WINDOW eWin);
void LCDC_GetFbEndAddr(u32* uFbAddr, LCD_WINDOW eWin);
void LCDC_GetImgStAddr(u32* uImgStAddr, LCD_WINDOW eWin);
u32 LCDC_GetLineCount(void);
void LCDC_InitIpForFifoIn(LCD_LOCAL_INPUT eInLocal, LCD_WINDOW eWin);
void LCDC_InitWinForFifoIn(LCD_LOCAL_INPUT eInLocal, LCD_WINDOW eWin);
void LCDC_InitWinForFifoIn1(LCD_LOCAL_INPUT eLocalIn, LCD_WINDOW eWin, LOCAL_IF_TYPE eLocalInCS);
void LCDC_InitWinForFifoIn2(LCD_LOCAL_INPUT eLocalIn, LCD_WINDOW eWin, LOCAL_IF_TYPE eLocalInCS, u32 uLcdHsize, u32 uLcdVsize);
void LCDC_InitWinForFifoIn3(LCD_LOCAL_INPUT eLocalIn, LCD_WINDOW eWin, u32 uLcdHsize, u32 uLcdVsize, u32 uX_Lcd, u32 uY_Lcd);
void LCDC_InitWinForFifoIn4 (LOCAL_IF_TYPE eLocalInCS, LCD_WINDOW eWin, u32 uLcdHsize, u32 uLcdVsize, u32 uX_Lcd, u32 uY_Lcd);
void LCDC_SetAlphaClear(LCD_WINDOW eWin);
void LCDC_WTrigger(LCD_WINDOW eWin);

void LCDC_SetALC(void);

/////////////////////////////////////////////////////////////////////////////////////
//// Image Enhancement
/////////////////////////////////////////////////////////////////////////////////////// 
void LCDC_SetGammaOnOff(u32 uEnable);
void LCDC_SetGammaValue(u8 * pLUT);
void LCDC_SetColorGainOnOff(u32 uEnable);
void LCDC_SetColorGainValue(u32 uCGRed, u32 uCGGreen, u32 uCGBlue);
void LCDC_SetHUEOnOff(u32 uEnable);
//void LCDC_SetHUE_CSC_control(HUE_CSC_MODE eWide, HUE_CSC_MODE eEq);
void LCDC_SetHUE_Coef(u16 eCBG0_P,u16 eCBG0_N,u16 eCBG1_P,u16 eCBG1_N,u16 eCRG0_P,u16 eCRG0_N,u16 eCRG1_P,u16 eCRG1_N);
void LCDC_SetHUE_Offset(u16 eOffsetIn,u16 eOffsetOut);
void LCDC_SetALPSOnOff(u32 uEnable);
void LCDC_SetALPSsize(u32 uHsz, u32 uVsz);
//void LCDC_SetALPSLux(CONTROL_LUX uLux);
void LCDC_SetALPSLutValue(u8* pLow_R_LUT, u8* pLow_G_LUT, u8* pLow_B_LUT, u8* pHigh_R_LUT, u8* pHigh_G_LUT, u8* pHigh_B_LUT);
void LCDC_SetALPSLutFullValue(u32* pLow_LUT, u32* pHigh_LUT);
void LCDC_SetDitheringOnOff(u32 uEnable);
//void LCDC_SetDitheringValue(DITHER_MODE eRedDitherBit, DITHER_MODE eGreenDitherBit, DITHER_MODE eBlueDitherBit);

/////////////////////////////////////////////////////////////////////////////////////
//// CPU-IF LCD
/////////////////////////////////////////////////////////////////////////////////////// 
void LCDC_Trigger(void);  
//void LCDC_UnmaskHwTrigger(TRIGGER_SRC eSrc);
//void LCDC_SetCpuIfDataFormat(CPUIF_LDI eLdiMode, CPUIF_DATA_FORMAT eFormat);
void LCDC_SetCpuIfTiming(u8 ucCSSetUpTime, u8 ucWrSetUpTime, u8 ucWrActTime, u8 ucWrHoldTime, CPUIF_LDI eLdi);
bool LCDC_IsFrameDone(void);
//void LCDC_SetFrameSkipRate(CPU_FRAME_SKIP eFrmSkip, CPUIF_LDI eLdi);
//void LCDC_SetAutoCmdRate(CPU_AUTO_CMD_RATE eCmdRate, CPUIF_LDI eLdi);
void LCDC_DisplayPartialArea(u32 uSrcStX, u32 uSrcStY, u32 uDstStX, u32 uDstStY, 
      u32 uPartialHSz, u32 uPartialVsz, u32 uSrcFbStAddr, LCD_WINDOW eWin,CPUIF_LDI eCpuIf);
//void LCDC_SetTriggerMode(TRIGGER_MODE eTrig);

// Manual
void LCDC_PutCmdToLdiByManual(u32 uData, CPUIF_LDI eLdiMode);
void LCDC_PutPayLoad(u32 uData, CPUIF_LDI eLdiMode);        //Add by NSH for MIPI DSIM
void LCDC_PutDataToLdiByManual(u32 uData, CPUIF_LDI eLdiMode);
void LCDC_GetCmdFromLdiByManual(u32* uCmd, CPUIF_LDI eLdiMode );
void LCDC_GetDataFromLdiByManual(u32* uData, CPUIF_LDI eLdiMode );
// Normal/ Auto/ NormalNAuto way
//void LCDC_PutCmdToLdi(u32 uData[], u32 uDataNum, CPUIF_LDI eLdiMode, CPU_COMMAND_MODE eCmdMode);
//void LCDC_PutDataToLdi(u32 uData[], u32 uDataNum, CPUIF_LDI eLdiMode, CPU_COMMAND_MODE eCmdMode);
//void LCDC_PutDataOrCmdToLdi(u32 uData[], u32 uDataNum, CPUIF_LDI eLdiMode, CPU_COMMAND_MODE eCmdMode, bool IsCmd);  


//void LCDC_InitStartPosOnLcdI80If(CPUIF_LDI eLdiMode);
// uclkDir: 1 -> divieded clk. uClkDir: 0 -> direct clk.
void LCDC_GetClkValAndClkDir(u32 uLcdHSz, u32 uLcdVSz, u32* uClkVal, u32* uClkDir);
void LCDC_InitWinRegs(LCD_WINDOW eWin);
void LCDC_WriteCmd(u32 uAddr, u32 uData);
void LCDC_SetRsOnOff(u32 uOnOff);
void LCDC_SetnWE(u32 uLowHigh);
void LCDC_InitStartPosOnLcd(CPUIF_LDI eLdiMode);
void LCDC_InitStartPosLCDSel(u32 uLCDmodel, CPUIF_LDI eLdiMode);
//void LCD_InitInt(LCDC_INT eLcdInt, LCDC_INT_SYS eLcdIntSys, LCDC_FIFO_LEVEL eFifoIntLevel, LCDC_FIFO_INT_SEL eFifoIntSel, LCDC_FRAME_INT_SEL eFIntSel );
void LCDC_DisableInterrupt(void);
void LCDC_EnableInterrupt(void);
//void LCDC_EnableAllWinFifoLvlInt(LCDC_FIFO_LEVEL eFifoLvl);
//void LCDC_EnableAllWinFrameInt(LCDC_FRAME_INT_SEL eIntSel);
//void LCDC_SetAllWinIntCnt(LCDC_FIFO_LEVEL eFifoLvl, LCDC_FRAME_INT_SEL eIntSel);
void LCDC_DisableIntFifoLevel(LCD_WINDOW eWin);
void LCDC_DisableFifoInterrupt(void);
void LCDC_ClearPending(u32 uIntFlag);
void LCDC_GetIntFlag(u32* pFlag);


//void LCDC_SetITUInterface(LCD_ITU_INTERLACE eITU_SEL);
//void LCDC_SetITUInterface_656(LCD_ITU_INTERLACE eITU_SEL);
#endif
