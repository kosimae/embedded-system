#include <common.h>
#include "./def.h"
#include "./library.h"
#include "./lcd_module.h"
#include "./gpio.h"

#define LCDM_SDA_HIGH     Outp32(SDA_GPIO_ADDRESS+4,(Inp32(SDA_GPIO_ADDRESS+4) |= (1<<SDA_GPIO_BIT)))
#define LCDM_SDA_LOW      Outp32(SDA_GPIO_ADDRESS+4,(Inp32(SDA_GPIO_ADDRESS+4) &= ~(1<<SDA_GPIO_BIT)))
#define LCDM_SCL_HIGH     Outp32(SCL_GPIO_ADDRESS+4,(Inp32(SCL_GPIO_ADDRESS+4) |= (1<<SCL_GPIO_BIT)))
#define LCDM_SCL_LOW      Outp32(SCL_GPIO_ADDRESS+4,(Inp32(SCL_GPIO_ADDRESS+4) &= ~(1<<SCL_GPIO_BIT)))
#define LCDM_CS_HIGH      Outp32(CS_GPIO_ADDRESS+4,(Inp32(CS_GPIO_ADDRESS+4) |=(1<<CS_GPIO_BIT)))
#define LCDM_CS_LOW     Outp32(CS_GPIO_ADDRESS+4,(Inp32(CS_GPIO_ADDRESS+4) &= ~(1<<CS_GPIO_BIT)))
#define LCDM_BACKLIGHT_HIGH   Outp32(BACKLIGHT_GPIO_ADDRESS+4,(Inp32(BACKLIGHT_GPIO_ADDRESS+4) |= (1<<BACKLIGHT_GPIO_BIT)))
#define LCDM_BACKLIGHT_LOW  Outp32(BACKLIGHT_GPIO_ADDRESS+4,(Inp32(BACKLIGHT_GPIO_ADDRESS+4) &= ~(1<<BACKLIGHT_GPIO_BIT)))
#define LCDM_RESET_HIGH   Outp32(RESET_GPIO_ADDRESS+4,(Inp32(RESET_GPIO_ADDRESS+4) |= (1<<RESET_GPIO_BIT)))
#define LCDM_RESET_LOW    Outp32(RESET_GPIO_ADDRESS+4,(Inp32(RESET_GPIO_ADDRESS+4) &= ~(1<<RESET_GPIO_BIT)))
#define LCDM_IIC_SCL_HIGH      Outp32(SCL_GPIO_ADDRESS+4,(Inp32(SCL_GPIO_ADDRESS+4) |= (1<<SCL_GPIO_BIT)))
#define LCDM_IIC_SCL_LOW      Outp32(SCL_GPIO_ADDRESS+4,(Inp32(SCL_GPIO_ADDRESS+4) &= ~(1<<SCL_GPIO_BIT)))
#define LCDM_IIC_SDA_HIGH      Outp32(SDA_GPIO_ADDRESS+4,(Inp32(SDA_GPIO_ADDRESS+4) |= (1<<SDA_GPIO_BIT)))
#define LCDM_IIC_SDA_LOW      Outp32(SDA_GPIO_ADDRESS+4,(Inp32(SDA_GPIO_ADDRESS+4) &= ~(1<<SDA_GPIO_BIT)))

#define LCDM_VDD_5V_HIGH   Outp32(VDD_5V_GPIO_ADDRESS+4,(Inp32(VDD_5V_GPIO_ADDRESS+4) |= (1<<VDD_5V_GPIO_BIT)))
#define LCDM_VDD_5V_LOW  Outp32(VDD_5V_GPIO_ADDRESS+4,(Inp32(VDD_5V_GPIO_ADDRESS+4) &= ~(1<<VDD_5V_GPIO_BIT)))

u32 gLCDM_HSIZE;
u32 gLCDM_VSIZE;  
u32 gVCLK_EDGE_DETECT;
u32 gHSYNC_POLARITY;
u32 gVSYNC_POLARITY;
u32 gVDEN_POLARITY;
u32 gLCDM_INTERFACE_TYPE;
u32 gDITHERING_FORMAT;
u32 gMODULE_VBPDE;
u32 gMODULE_VBPD;
u32 gMODULE_VFPD;
u32 gMODULE_VSPW;
u32 gMODULE_VFPDE;      
u32 gMODULE_HBPD;         
u32 gMODULE_HFPD;        
u32 gMODULE_HSPW;

void LCDM_BackLightOn(void)
{
  if(!BACKLIGHT_GPIO_ADDRESS)
    return;

  if(BACKLIGHT_ACTIVE == HIGH_ACTIVE)
    LCDM_BACKLIGHT_HIGH;
  else
    LCDM_BACKLIGHT_LOW;
}

void LCDM_BackLightOff(void)
{
  if(!BACKLIGHT_GPIO_ADDRESS)
    return;

  if(BACKLIGHT_ACTIVE == HIGH_ACTIVE)
    LCDM_BACKLIGHT_LOW;
  else
    LCDM_BACKLIGHT_HIGH;
}

void LCDM_ResetOn(void)
{
  if(!RESET_GPIO_ADDRESS)
    return;

  if(RESET_ACTIVE == HIGH_ACTIVE)
    LCDM_RESET_HIGH;
  else
    LCDM_RESET_LOW;
}

void LCDM_ResetOff(void)
{
  if(!RESET_GPIO_ADDRESS)
    return;

  if(RESET_ACTIVE == HIGH_ACTIVE)
    LCDM_RESET_LOW;
  else
    LCDM_RESET_HIGH;
}

void LCDM_Reset(u32 uMilli) 
{
  LCDM_ResetOn();
  //LCDM_DelayMS(uMilli);
  LCDM_ResetOff();
  //LCDM_DelayMS(uMilli);
}

void LCDM_InitDISPConGPIO(void)
{
  if(gLCDM_INTERFACE_TYPE == RGB_PARALLEL_16bpp 
      || gLCDM_INTERFACE_TYPE == RGB_PARALLEL_18bpp 
      || gLCDM_INTERFACE_TYPE == RGB_PARALLEL_24bpp 
      || gLCDM_INTERFACE_TYPE == RGB_DELTA_STRUCTURE 
      || gLCDM_INTERFACE_TYPE == RGB_SERIAL)
  {
    printf("LCDM_InitDISPConGPIO()==>debug01\n");
    GPIO_SetFunctionAll(eGPIO_F0, 0x22222222);  //set GPF0 as LVD_HSYNC,VSYNC,VCLK,VDEN,VD[3:0]
    GPIO_SetPullUpDownAll(eGPIO_F0,0x0);      //set pull-up,down disable
    GPIO_SetFunctionAll(eGPIO_F1, 0x22222222);  //set GPF1 as VD[11:4]
    GPIO_SetPullUpDownAll(eGPIO_F1,0x0);      //set pull-up,down disable
    GPIO_SetFunctionAll(eGPIO_F2, 0x22222222);  //set GPF2 as VD[19:12]
    GPIO_SetPullUpDownAll(eGPIO_F2,0x0);      //set pull-up,down disable
    GPIO_SetFunctionAll(eGPIO_F3, 0x00002222);  //set GPF3 as VD[23:20]
    GPIO_SetPullUpDownAll(eGPIO_F3,0x0);      //set pull-up,down disable

    printf("LCDM_InitDISPConGPIO()==>debug02\n");
    //--------- S5PV210 EVT0 needs MAX drive strength ---------//
    GPIO_SetDSAll(eGPIO_F0,0xffffffff);     //set GPF0 drive strength max by WJ.KIM(09.07.17)
    GPIO_SetDSAll(eGPIO_F1,0xffffffff);     //set GPF1 drive strength max by WJ.KIM(09.07.17)
    GPIO_SetDSAll(eGPIO_F2,0xffffffff);     //set GPF2 drive strength max by WJ.KIM(09.07.17)
    GPIO_SetDSAll(eGPIO_F3,0x3ff);          //set GPF3 drive strength max by WJ.KIM(09.07.17)
    //-----------------------------------------------------------//
    printf("LCDM_InitDISPConGPIO()==>debug03\n");
  }
}

void LCDM_InitLdiSpec(u32 eLcdModel)
{
  if(eLcdModel == LCD_LTE480WV_RGB) 
  {
    gLCDM_HSIZE = WVGA_HSIZE;
    gLCDM_VSIZE = WVGA_VSIZE;
    gVCLK_EDGE_DETECT =VCLK_FALLING_EDGE;
    gHSYNC_POLARITY = HSYNC_INVERT;
    gVSYNC_POLARITY = VSYNC_INVERT;
    gVDEN_POLARITY  = VDEN_NORMAL;
    gLCDM_INTERFACE_TYPE= RGB_PARALLEL_24bpp;
    gDITHERING_FORMAT=  DITHER_888;
    gMODULE_VBPDE   =6;
    gMODULE_VBPD    =    (6); // 1 line back porch
    gMODULE_VFPD    =    (4); // 1 line front porch
    gMODULE_VSPW    =    (0); // 1 Line pulse width
    gMODULE_VFPDE   =4;
    gMODULE_HBPD    =    (12); // 2 CLK back porch
    gMODULE_HFPD    =    (7); // 5 CLK front porch
    gMODULE_HSPW    =    (2); // 2 CLK pulse width
  }
  else if(eLcdModel == LCD_LTP700WV_RGB)
  {
    gLCDM_HSIZE = WVGA_HSIZE;
    gLCDM_VSIZE = WVGA_VSIZE;
    gVCLK_EDGE_DETECT=  VCLK_FALLING_EDGE;
    gHSYNC_POLARITY = HSYNC_INVERT;
    gVSYNC_POLARITY = VSYNC_INVERT;
    gVDEN_POLARITY  = VDEN_NORMAL;
    gLCDM_INTERFACE_TYPE =  RGB_PARALLEL_24bpp;
    gDITHERING_FORMAT = DITHER_888;
    gMODULE_VBPDE   = 0;
    gMODULE_VBPD    =       (7-1);
    gMODULE_VFPD    =       (5-1);
    gMODULE_VSPW    =       (1-1);
    gMODULE_VFPDE   = 0 ;
    gMODULE_HBPD    =       (13-1);
    gMODULE_HFPD    =       (8-1);      
    gMODULE_HSPW    =       (3-1);
  }
  else
  {
    Assert(0);
  }
}


void LCDM_InitGPIO(u32 reg, u32 bitpos)
{
  u32 uDataValue;

  if(!reg)
    return;

  //output setting
  uDataValue = Inp32((reg));
  uDataValue = (uDataValue & ~(0xf<<bitpos*4)) | (1<<bitpos*4);
  Outp32((reg), uDataValue);

  //pull down disable
  uDataValue = Inp32((reg+0x8));
  uDataValue = (uDataValue & ~(0x3<<bitpos*2));
  Outp32((reg+0x8), uDataValue);

  //max drive strength
  uDataValue = Inp32((reg+0xc));
  uDataValue = ((uDataValue & ~(0x3<<bitpos*2)) | (0x3<<bitpos*2));
  Outp32((reg+0xc), uDataValue);

}

void LCDM_InitLdi(u32 uLdiModule, u32 uLdiMode)
{
    u32* pReg;

    LCDM_InitGPIO(SDA_GPIO_ADDRESS,SDA_GPIO_BIT);
    LCDM_InitGPIO(SCL_GPIO_ADDRESS,SCL_GPIO_BIT);
    LCDM_InitGPIO(CS_GPIO_ADDRESS,CS_GPIO_BIT);
    LCDM_InitGPIO(IIC_SDA_GPIO_ADDRESS,IIC_SDA_GPIO_BIT);
    LCDM_InitGPIO(IIC_SCL_GPIO_ADDRESS,IIC_SCL_GPIO_BIT);
    LCDM_InitGPIO(BACKLIGHT_GPIO_ADDRESS,BACKLIGHT_GPIO_BIT);
    LCDM_InitGPIO(RESET_GPIO_ADDRESS,RESET_GPIO_BIT);
    LCDM_InitGPIO(POWER_GPIO_ADDRESS,POWER_GPIO_BIT);
    LCDM_InitGPIO(POWER_GPIO_ADDRESS2,POWER_GPIO_BIT2);
    LCDM_InitGPIO(VDD_5V_GPIO_ADDRESS,VDD_5V_GPIO_BIT);

    LCDM_SDA_HIGH;
    LCDM_SCL_HIGH;
    LCDM_CS_HIGH;
    LCDM_IIC_SCL_HIGH;
    LCDM_IIC_SDA_HIGH;

    /* VDD 5V Enable */
    LCDM_VDD_5V_HIGH;

    printf("LCDM_InitLdi()==>debug01\n");
    LCDM_BackLightOn();
    printf("LCDM_InitLdi()==>debug02\n");
    LCDM_ResetOff();
    printf("LCDM_InitLdi()==>debug03\n");

    LCDM_InitDISPConGPIO();
    printf("LCDM_InitLdi()==>debug04\n");
    LCDM_InitLdiSpec(uLdiModule);
    printf("LCDM_InitLdi()==>debug05\n");

    LCDM_Reset(10);
}

