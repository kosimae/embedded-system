#include <common.h>
#include "./library.h"
#include "./sysc.h"
#include "./gpio.h"

#define CHIPID_BASE       (0xE0000000)
#define SYSCON_BASE     (0xE0100000)

#if 1
#define syscOutp32 Outp32
#define syscInp32 Inp32
#else
#define syscOutp32(a, d) {printf("Outp32(\'h%08x, \'h%08x);\n", a, d); Outp32(a, d);}
#define syscInp32(a) {Inp32(a); u32 d2; d2=Inp32(a); printf("Inp32(\'h%08x); \\ d=0x%08x\n", a, d2);}
#endif

volatile u32 g_uSclkA2M, g_uSclkG3D, g_uSclkMFC;

enum CHIP_ID_SFR    
{
  rPRODUCT_ID     = (u32)(CHIPID_BASE+0x0000), // RO

  rOPERATING_MODE   = (u32)(CHIPID_BASE+0x0004), // RO
  rEFUSE_CFG    = (u32)(CHIPID_BASE+0x0008),
  rEFUSE_CTRL     = (u32)(CHIPID_BASE+0x000c),
  rSPARE_AREA     = (u32)(CHIPID_BASE+0x0010), // RO
  rCHIP_ID0       = (u32)(CHIPID_BASE+0x0014), // RO
  rCHIP_ID1       = (u32)(CHIPID_BASE+0x0018), // RO
};

enum CMU_SFR    
{
  rAPLL_LOCK      = SYSCON_BASE+0x0000,
  rMPLL_LOCK      = SYSCON_BASE+0x0008,
  rEPLL_LOCK      = SYSCON_BASE+0x0010,
  rVPLL_LOCK      = SYSCON_BASE+0x0020,
  // [15:0] Lock time.
  rAPLL_CON       = SYSCON_BASE+0x0100,
  rMPLL_CON       = SYSCON_BASE+0x0108,
  rEPLL_CON       = SYSCON_BASE+0x0110,
  rVPLL_CON       = SYSCON_BASE+0x0120,

  // [31] Enable PLL.
  // [30] PLL is locked.
  // [23:16] M value
  // [13:8] P value
  // [2:0] S value
  rCLK_SRC0       = SYSCON_BASE+0x0200,
  rCLK_SRC1       = SYSCON_BASE+0x0204,
  rCLK_SRC2       = SYSCON_BASE+0x0208,
  rCLK_SRC3       = SYSCON_BASE+0x020c,
  rCLK_SRC4       = SYSCON_BASE+0x0210,
  rCLK_SRC5       = SYSCON_BASE+0x0214,
  rCLK_SRC6       = SYSCON_BASE+0x0218,
  rCLK_SRC_MASK0  = SYSCON_BASE+0x0280,
  rCLK_SRC_MASK1  = SYSCON_BASE+0x0284,
  rCLK_DIV0       = SYSCON_BASE+0x0300,
  rCLK_DIV1       = SYSCON_BASE+0x0304,
  rCLK_DIV2       = SYSCON_BASE+0x0308,
  rCLK_DIV3       = SYSCON_BASE+0x030c,
  rCLK_DIV4       = SYSCON_BASE+0x0310,
  rCLK_DIV5       = SYSCON_BASE+0x0314,
  rCLK_DIV6       = SYSCON_BASE+0x0318,
  rCLK_DIV7       = SYSCON_BASE+0x031C,
  rCLK_GATE_IP0     = SYSCON_BASE+0x0460,
  rCLK_GATE_IP1     = SYSCON_BASE+0x0464,
  rCLK_GATE_IP2     = SYSCON_BASE+0x0468,
  rCLK_GATE_IP3     = SYSCON_BASE+0x046C,
  rCLK_GATE_IP4     = SYSCON_BASE+0x0470,
  rCLK_GATE_BLOCK   = SYSCON_BASE+0x0480,
  rCLK_OUT      = SYSCON_BASE+0x0500,

  // [16:12] CLKSEL
  rCLK_DIV_STAT0    = SYSCON_BASE+0x1000,
  rCLK_DIV_STAT1    = SYSCON_BASE+0x1004,
  rCLK_MUX_STAT0  = SYSCON_BASE+0x1100,
  rCLK_MUX_STAT1  = SYSCON_BASE+0x1104,
  // RESET
  rGLOBAL_SW_RESET  = SYSCON_BASE+0x2000,
  // IEM
  rDCGIDX_MAP0    = SYSCON_BASE+0x3000,
  rDCGIDX_MAP1    = SYSCON_BASE+0x3004,
  rDCGIDX_MAP2    = SYSCON_BASE+0x3008,
  rDCGPERF_MAP0   = SYSCON_BASE+0x3020,
  rDCGPERF_MAP1   = SYSCON_BASE+0x3024,
  rDVCIDX_MAP   = SYSCON_BASE+0x3040,
  rFREQ_CPU     = SYSCON_BASE+0x3060,
  rFREQ_DPM     = SYSCON_BASE+0x3064,
  rDVSEMCLK_EN    = SYSCON_BASE+0x3080,
  rMAXPERF      = SYSCON_BASE+0x3084,
  rAPLL_CON_L8    = SYSCON_BASE+0x3100,
  rAPLL_CON_L7    = SYSCON_BASE+0x3104,
  rAPLL_CON_L6    = SYSCON_BASE+0x3108,
  rAPLL_CON_L5    = SYSCON_BASE+0x310c,
  rAPLL_CON_L4    = SYSCON_BASE+0x3110,
  rAPLL_CON_L3    = SYSCON_BASE+0x3114,
  rAPLL_CON_L2    = SYSCON_BASE+0x3118,
  rAPLL_CON_L1    = SYSCON_BASE+0x311c,
  rCLKDIV_IEM_L8    = SYSCON_BASE+0x3200,
  rCLKDIV_IEM_L7    = SYSCON_BASE+0x3204,
  rCLKDIV_IEM_L6    = SYSCON_BASE+0x3208,
  rCLKDIV_IEM_L5    = SYSCON_BASE+0x320c,
  rCLKDIV_IEM_L4    = SYSCON_BASE+0x3210,
  rCLKDIV_IEM_L3    = SYSCON_BASE+0x3214,
  rCLKDIV_IEM_L2    = SYSCON_BASE+0x3218,
  rCLKDIV_IEM_L1    = SYSCON_BASE+0x321c,
  // MISC
  rONEDRAM_CFG    = SYSCON_BASE+0x6208,
  rDISPLAY_CONTROL      = SYSCON_BASE+0x7008,
  rAUDIO_ENDIAN_SEL = SYSCON_BASE+0x700C,
  rHDMI_PHY_CON0  = SYSCON_BASE+0x7208
};

volatile u32 g_uProductID, g_uPkgMode, g_uMainRev, g_uSubRev; // Chip & Revision inform
volatile u32 g_uAPLL, g_uMPLL, g_uEPLL, g_uVPLL; // SCLKapll,mpll,epll,vpll
volatile u32 g_uARMCLK, g_uHclkMsys, g_uHclkDsys, g_uHclkPsys, g_uPclkMsys, g_uPclkDsys, g_uPclkPsys, g_uOneDRAM; // Core & Bus clocks

void SYSC_UpdateClkInform(void)
{
  // PLLs
  u16 usApllM, usApllP, usApllS, usMpllM, usMpllP, usMpllS, usEpllM, usEpllP, usEpllS, usVpllM, usVpllP, usVpllS;
  // MUXs
  u8 ucMuxApll, ucMuxMpll, ucMuxEpll, ucMuxVpllSrc, ucMuxVpll, ucMuxMsys, ucMuxDsys, ucMuxPsys, ucMuxOneDRAM;
  // DIVIDERs
  u8 ucDivApll, ucDivAM, ucDivOneDRAM,  
     ucDivHclkMsys, ucDivHclkDsys, ucDivHclkPsys, 
     ucDivPclkMsys, ucDivPclkDsys, ucDivPclkPsys;

  u32 uTemp, uVpllFin, uMsysMuxClk, uDsysMuxClk, uPsysMuxClk;

  // clock division ratio 
  uTemp = Inp32(rCLK_DIV0);
  ucDivApll = uTemp & 0x7;  // ARMCLK
  ucDivAM = (uTemp>>4) & 0x7;
  ucDivHclkMsys = (uTemp>>8) & 0x7;
  ucDivPclkMsys = (uTemp>>12) & 0x7;
  ucDivHclkDsys = (uTemp>>16) & 0xf;
  ucDivPclkDsys = (uTemp>>20) & 0x7;
  ucDivHclkPsys = (uTemp>>24) & 0xf;
  ucDivPclkPsys = (uTemp>>28) & 0x7;

  uTemp = Inp32(rCLK_DIV6);
  ucDivOneDRAM = (uTemp>>28) & 0xf;

  uTemp = Inp32(rAPLL_CON);
  usApllM = (uTemp>>16)&0x3ff;
  usApllP = (uTemp>>8)&0x3f;
  usApllS = (uTemp&0x7);

  uTemp = Inp32(rMPLL_CON);
  usMpllM = (uTemp>>16)&0x3ff;
  usMpllP = (uTemp>>8)&0x3f;
  usMpllS = (uTemp&0x7);

  uTemp = Inp32(rEPLL_CON);
  usEpllM = (uTemp>>16)&0x3ff;
  usEpllP = (uTemp>>8)&0x3f;
  usEpllS = (uTemp&0x7);

  uTemp = Inp32(rVPLL_CON);
  usVpllM = (uTemp>>16)&0x3ff;
  usVpllP = (uTemp>>8)&0x3f;
  usVpllS = (uTemp&0x7);

  // APLL out
  uTemp = Inp32(rCLK_SRC0);
  ucMuxApll = uTemp & 0x1;

  if(ucMuxApll) //FOUTapll
  {     
#ifndef FPGA  
    g_uAPLL = ((FIN>>(usApllS-1))/usApllP)*usApllM;
#else // fpga
    g_uAPLL = ((FIN>>(usApllS))/usApllP)*usApllM;
#endif
  }
  else  //FIN
  {
    g_uAPLL = FIN;
  }

  // MPLL out
  ucMuxMpll = (uTemp>>4) & 0x1;
  if(ucMuxMpll) //FOUTmpll
  {     
    g_uMPLL = ((FIN>>(usMpllS))/usMpllP)*usMpllM;
  }
  else  //FIN
  {
    g_uMPLL = FIN;
  }


  // EPLL out
  ucMuxEpll = (uTemp>>8) & 0x1;
  if(ucMuxEpll) //FOUTepll
  {     
    g_uEPLL = ((FIN>>(usEpllS))/usEpllP)*usEpllM;
  }
  else  //FIN
  {
    g_uEPLL = FIN;
  }

  // VPLL out
  ucMuxVpll = (uTemp>>12) & 0x1;
  uTemp = Inp32(rCLK_SRC1);
  ucMuxVpllSrc = (uTemp>>28) & 0x1;
  if (ucMuxVpllSrc)
  {
    uVpllFin = 27000000; // SCLK_HDMI27M : HDMI PHY clock out
  }
  else
  {
    uVpllFin = FIN;
  }
  if(ucMuxVpll) //FOUTvpll
  {     
    g_uVPLL = ((uVpllFin>>(usVpllS))/usVpllP)*usVpllM;
  }
  else  //FINvpll
  {
    g_uVPLL = uVpllFin;
  }

  // MSYS domain & ARMCLK
  uTemp = Inp32(rCLK_SRC0);
  ucMuxMsys = (uTemp>>16) & 0x1;
  if(ucMuxMsys) 
  {     
    uMsysMuxClk = g_uMPLL;
  }
  else  
  {
    uMsysMuxClk = g_uAPLL;
  }

  g_uARMCLK = uMsysMuxClk/(ucDivApll+1);

  // HCLK MSYS & ACLK200 & DMC1 clock
  g_uHclkMsys = g_uARMCLK/(ucDivHclkMsys+1);

  // PCLK MSYS
  g_uPclkMsys = g_uHclkMsys/(ucDivPclkMsys+1);

  // A2M clock
  g_uSclkA2M = g_uAPLL/(ucDivAM+1);


  // DSYS domain
  ucMuxDsys = (uTemp>>20) & 0x1;
  if(ucMuxDsys) 
  {     
    uDsysMuxClk = g_uSclkA2M;
  }
  else  
  {
    uDsysMuxClk = g_uMPLL;
  }
  // HCLK DSYS
  g_uHclkDsys = uDsysMuxClk/(ucDivHclkDsys+1);

  // PCLK DSYS
  g_uPclkDsys = g_uHclkDsys/(ucDivPclkDsys+1);

  // PSYS domain
  ucMuxPsys = (uTemp>>24) & 0x1;
  if(ucMuxPsys) 
  {     
    uPsysMuxClk = g_uSclkA2M;
  }
  else  
  {
    uPsysMuxClk = g_uMPLL;
  }
  // HCLK PSYS
  g_uHclkPsys = uPsysMuxClk/(ucDivHclkPsys+1);

  // PCLK PSYS
  g_uPclkPsys = g_uHclkPsys/(ucDivPclkPsys+1);

  // OneDRAM(DMC0) clock
  uTemp = Inp32(rCLK_SRC6);
  ucMuxOneDRAM = (uTemp>>24) & 0x3;
  switch (ucMuxOneDRAM)
  {
    case 1 : // SCLKMPLL
      g_uOneDRAM = g_uMPLL/(ucDivOneDRAM+1);
      break;
    case 2 : // SCLKEPLL
      g_uOneDRAM = g_uEPLL/(ucDivOneDRAM+1);
      break;
    case 3 : // SCLKVPLL
      g_uOneDRAM = g_uVPLL/(ucDivOneDRAM+1);
      break;
    default : // SCLKA2M
      g_uOneDRAM = g_uSclkA2M/(ucDivOneDRAM+1);
      break;
  }
}

// Get Clock divider
u32 SYSC_GetClkDiv(CLKDIV_ID eId)
{
  u32 uRegAddr;
  u32 uRegMaskData;
  u32 uRegBitOffset;
  u32 uRegTemp;

  switch(eId)
  {
    case eCLKDIV_PCLKPSYS: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0x7; uRegBitOffset = 28; break;
    case eCLKDIV_HCLKPSYS: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0xf; uRegBitOffset = 24; break;
    case eCLKDIV_PCLKDSYS: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0x7; uRegBitOffset = 20; break;
    case eCLKDIV_HCLKDSYS: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0xf; uRegBitOffset = 16; break;
    case eCLKDIV_PCLKMSYS: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0x7; uRegBitOffset = 12; break;
    case eCLKDIV_HCLKMSYS: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0x7; uRegBitOffset = 8; break;
    case eCLKDIV_A2M: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0x7; uRegBitOffset = 4; break;
    case eCLKDIV_APLL: 
      uRegAddr = rCLK_DIV0; uRegMaskData = 0x7; uRegBitOffset = 0; break;

    case eCLKDIV_CSIS: 
      uRegAddr = rCLK_DIV1; uRegMaskData = 0xf; uRegBitOffset = 28; break;
    case eCLKDIV_FIMD: 
      uRegAddr = rCLK_DIV1; uRegMaskData = 0xf; uRegBitOffset = 20; break;
    case eCLKDIV_CAM1:
      uRegAddr = rCLK_DIV1; uRegMaskData = 0xf; uRegBitOffset = 16; break;
    case eCLKDIV_CAM0:
      uRegAddr = rCLK_DIV1; uRegMaskData = 0xf; uRegBitOffset = 12; break;
    case eCLKDIV_FIMC: 
      uRegAddr = rCLK_DIV1; uRegMaskData = 0xf; uRegBitOffset = 8; break;
    case eCLKDIV_TBLK: 
      uRegAddr = rCLK_DIV1; uRegMaskData = 0xf; uRegBitOffset = 0; break;

    case eCLKDIV_MFC: 
      uRegAddr = rCLK_DIV2; uRegMaskData = 0xf; uRegBitOffset = 4; break;
    case eCLKDIV_G3D: 
      uRegAddr = rCLK_DIV2; uRegMaskData = 0xf; uRegBitOffset = 0; break;

    case eCLKDIV_FIMC2: 
      uRegAddr = rCLK_DIV3; uRegMaskData = 0xf; uRegBitOffset = 20; break;
    case eCLKDIV_FIMC1: 
      uRegAddr = rCLK_DIV3; uRegMaskData = 0xf; uRegBitOffset = 16; break;
    case eCLKDIV_FIMC0: 
      uRegAddr = rCLK_DIV3; uRegMaskData = 0xf; uRegBitOffset = 12; break;
    case eCLKDIV_MDNIEPWM:
      uRegAddr = rCLK_DIV3; uRegMaskData = 0x7f; uRegBitOffset = 4; break;
    case eCLKDIV_MDNIE: 
      uRegAddr = rCLK_DIV3; uRegMaskData = 0xf; uRegBitOffset = 0; break;

    case eCLKDIV_UART3: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 28; break;
    case eCLKDIV_UART2: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 24; break;
    case eCLKDIV_UART1: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 20; break;
    case eCLKDIV_UART0: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 16; break;
    case eCLKDIV_MMC3: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 12; break;
    case eCLKDIV_MMC2: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 8; break;
    case eCLKDIV_MMC1: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 4; break;
    case eCLKDIV_MMC0: 
      uRegAddr = rCLK_DIV4; uRegMaskData = 0xf; uRegBitOffset = 0; break;

    case eCLKDIV_PWM: 
      uRegAddr = rCLK_DIV5; uRegMaskData = 0xf; uRegBitOffset = 12; break;
    case eCLKDIV_SPI2: 
      uRegAddr = rCLK_DIV5; uRegMaskData = 0xf; uRegBitOffset = 8; break;
    case eCLKDIV_SPI1: 
      uRegAddr = rCLK_DIV5; uRegMaskData = 0xf; uRegBitOffset = 4; break;
    case eCLKDIV_SPI0: 
      uRegAddr = rCLK_DIV5; uRegMaskData = 0xf; uRegBitOffset = 0; break;

    case eCLKDIV_ONEDRAM: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0xf; uRegBitOffset = 28; break;
    case eCLKDIV_PWI: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0xf; uRegBitOffset = 24; break;
    case eCLKDIV_HPM: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0x7; uRegBitOffset = 20; break;
    case eCLKDIV_COPY:
      uRegAddr = rCLK_DIV6; uRegMaskData = 0x7; uRegBitOffset = 16; break;
    case eCLKDIV_ONENAND: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0x7; uRegBitOffset = 12; break;
    case eCLKDIV_AUDIO2: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0xf; uRegBitOffset = 8; break;
    case eCLKDIV_AUDIO1: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0xf; uRegBitOffset = 4; break;
    case eCLKDIV_AUDIO0: 
      uRegAddr = rCLK_DIV6; uRegMaskData = 0xf; uRegBitOffset = 0; break;

    case eCLKDIV_DPM: 
      uRegAddr = rCLK_DIV7; uRegMaskData = 0x7f; uRegBitOffset = 8; break;
    case eCLKDIV_DVSEM: 
      uRegAddr = rCLK_DIV7; uRegMaskData = 0x7f; uRegBitOffset = 0; break;
    default:
      //DbgSysc(("No appropriate clock divider is found in GetClockDivider()\n\x04"));
      return;
  }
  uRegTemp = syscInp32(uRegAddr);
  return ((uRegTemp>>uRegBitOffset)&uRegMaskData)+1;  // return real divide value. ex) 1/4 -> return 4
}

u32 SYSC_GetClkMuxReg(u32 uRegNum)
{
  u32 uRegTemp;
  uRegTemp = syscInp32(rCLK_SRC0+4*uRegNum);
  return uRegTemp;
}
