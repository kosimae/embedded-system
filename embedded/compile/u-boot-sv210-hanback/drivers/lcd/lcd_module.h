#ifndef __LCD_MODULE_H__
#define __LCD_MODULE_H__

#define LOW_ACTIVE 0
#define HIGH_ACTIVE 1

#if 0
#define LCD_MODEL     LCD_LTP700WV_RGB
#else
#define LCD_MODEL     LCD_LTE480WV_RGB
#endif

enum
{
  LCD_LTS350Q1,
  LCD_LTT200QV_LTPS,
  LCD_LTS222Q_CPU,
  LCD_LTS222Q_RGB,
  LCD_LTS222Q_SRGB,
  LCD_LTS222Q_SPI,
  LCD_LTP700WV_RGB,
  LCD_LTE480WV_RGB,
  LCD_LTV350QV_RGB,
  LCD_LTM030DK,   // TN portrait WVGA (6410 test module)
  LCD_TL2796,
  LCD_UF24U276A,    // Triple B'd i80 i/F module
  LCD_LMS280GS01,   // Triple B'd RGB i/f module
  LCD_HD,
  LCD_AUO27_SKIP,   // CAM Delta-Structure Module RGB Skip mode
  LCD_AUO27_DUMMY,    // CAM Delta-Structure Module Dummy mode
  LCD_XGA,
  LCD_SXGA,
  LCD_MIPIModule,
  LCD_S6E63M0
};

#define IIC_SDA_GPIO_ADDRESS      (0xE02000C0)  //GPB
#define IIC_SDA_GPIO_BIT        (0)   //GPB[7]
#define IIC_SCL_GPIO_ADDRESS      (0xE02000C0)  //GPB
#define IIC_SCL_GPIO_BIT        (1)   //GPB[4]
#define SDA_GPIO_ADDRESS      (0xE0200040)  //GPB
#define SDA_GPIO_BIT        (7)   //GPB[7]
#define SCL_GPIO_ADDRESS      (0xE0200040)  //GPB
#define SCL_GPIO_BIT        (4)   //GPB[4]
#define CS_GPIO_ADDRESS     (0xE0200040)  //GPB
#define CS_GPIO_BIT       (5)   //GPB[5]
#define BACKLIGHT_GPIO_ADDRESS  (0xE02000A0)  //GPD0
#define BACKLIGHT_GPIO_BIT    (3)   //GPD0[3]
#define RESET_GPIO_ADDRESS    (0xE0200C00)  //GPH0
#define RESET_GPIO_BIT        (6)   //GPH0.6
#define POWER_GPIO_ADDRESS    (0xE0200040)
#define POWER_GPIO_BIT        (6)
#define POWER_GPIO_ADDRESS2   (0xE0200040)
#define POWER_GPIO_BIT2     (7)
#define BACKLIGHT_ACTIVE    HIGH_ACTIVE
#define RESET_ACTIVE      LOW_ACTIVE
#define POWER_ACTIVE      HIGH_ACTIVE

typedef enum
{
  VCLK_RISING_EDGE,VCLK_FALLING_EDGE
}VCLK_EDGE;

typedef enum
{
  HSYNC_NORMAL,HSYNC_INVERT
}HSYNC_P;

typedef enum
{
  VSYNC_NORMAL,VSYNC_INVERT
}VSYNC_P;

typedef enum
{
  VDEN_NORMAL,VDEN_INVERT
}VDEN_P;

typedef enum
{
  DITHER_555,DITHER_565,DITHER_666,DITHER_888
}DITHER_TYPE;

typedef enum
{
  RGB_PARALLEL_16bpp,RGB_PARALLEL_18bpp,RGB_PARALLEL_24bpp,RGB_SERIAL,RGB_DELTA_STRUCTURE,I80_CPU
}INTERFACE_TYPE;

#define IIC_SDA_GPIO_ADDRESS    (0xE02000C0)  //GPB
#define IIC_SDA_GPIO_BIT        (0)   //GPB[7]
#define IIC_SCL_GPIO_ADDRESS    (0xE02000C0)  //GPB
#define IIC_SCL_GPIO_BIT        (1)   //GPB[4]
#define SDA_GPIO_ADDRESS        (0xE0200040)  //GPB
#define SDA_GPIO_BIT            (7)   //GPB[7]
#define SCL_GPIO_ADDRESS          (0xE0200040)  //GPB
#define SCL_GPIO_BIT            (4)   //GPB[4]
#define CS_GPIO_ADDRESS         (0xE0200040)  //GPB
#define CS_GPIO_BIT             (5)   //GPB[5]
#if 1
#define BACKLIGHT_GPIO_ADDRESS  (0xE02000A0)  //GPD0
#define BACKLIGHT_GPIO_BIT      (3)   //GPD0[3]
#define VDD_5V_GPIO_ADDRESS     (0xE0200C60)  //GPH3
#define VDD_5V_GPIO_BIT         (5)   //GPH3[5]
#else
#define BACKLIGHT_GPIO_ADDRESS  (0xE02001C0)  //GPG1
#define BACKLIGHT_GPIO_BIT      (4)   //GPG1[4]
#define VDD_5V_GPIO_ADDRESS     (0xE0200C60)  //GPH3
#define VDD_5V_GPIO_BIT         (5)   //GPH3[5]
#endif
#define RESET_GPIO_ADDRESS      (0xE0200C00)  //GPH0
#define RESET_GPIO_BIT          (6)   //GPH0.6
#define POWER_GPIO_ADDRESS      (0xE0200040)
#define POWER_GPIO_BIT          (6)
#define POWER_GPIO_ADDRESS2     (0xE0200040)
#define POWER_GPIO_BIT2         (7)
#define BACKLIGHT_ACTIVE        HIGH_ACTIVE
#define RESET_ACTIVE            LOW_ACTIVE
#define POWER_ACTIVE            HIGH_ACTIVE
#endif
