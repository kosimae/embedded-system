#include "./system.h"
#include "./sysc.h"
#include "./library.h"
#include "./lcd.h"
#include "./gpio.h"
#include "./def.h"
#include "./lcd_module.h"

#define VIDOUT_RGBIF      (0<<26)

#define LcdcOutp32(offset, x) Outp32(LCDC_BASE+offset, x)
#define LcdcInp32(offset, x) {x=Inp32(LCDC_BASE+offset);}

enum LCDC_SFR
{
  VIDCON0   = 0x00000,
  VIDCON1   = 0x00004, 
  VIDCON2   = 0x00008,
  VIDCON3   = 0x0000C,    //new(v6.0)
  VIDTCON0    = 0x00010,
  VIDTCON1    = 0x00014, 
  VIDTCON2    = 0x00018,
  VIDTCON3    = 0x0001c,
  WINCON0   = 0x00020,
  WINCON1   = 0x00024,
  WINCON2   = 0x00028,
  WINCON3   = 0x0002C,
  WINCON4       = 0x00030,
  SHADOWCON = 0x00034,    //new(v6.0)
  VIDOSD0A    = 0x00040,
  VIDOSD0B    = 0x00044,
  VIDOSD0C    = 0x00048,
  VIDOSD1A    = 0x00050,
  VIDOSD1B    = 0x00054,
  VIDOSD1C    = 0x00058,
  VIDOSD1D    = 0x0005C,
  VIDOSD2A    = 0x00060,
  VIDOSD2B    = 0x00064,
  VIDOSD2C    = 0x00068,
  VIDOSD2D    = 0x0006c,
  VIDOSD3A    = 0x00070,
  VIDOSD3B    = 0x00074,
  VIDOSD3C    = 0x00078,
  VIDOSD4A    = 0x00080,
  VIDOSD4B    = 0x00084,
  VIDOSD4C    = 0x00088,

  VIDW00ADD0B0  = 0x000A0,
  VIDW00ADD0B1  = 0x000A4,
  VIDW00ADD0B2  = 0x020A0,    // v6.2 for triple buffer
  VIDW01ADD0B0  = 0x000A8,
  VIDW01ADD0B1  = 0x000AC,
  VIDW01ADD0B2  = 0x020A8,    // v6.2 for triple buffer
  VIDW02ADD0B0  = 0x000B0,    //new(v6.0) win2~4 have double buffer
  VIDW02ADD0B1  = 0x000B4,
  VIDW02ADD0B2  = 0x020B0,    // v6.2 for triple buffer
  VIDW03ADD0B0  = 0x000B8,
  VIDW03ADD0B1  = 0x000BC,
  VIDW03ADD0B2  = 0x020B8,    // v6.2 for triple buffer
  VIDW04ADD0B0  = 0x000C0,
  VIDW04ADD0B1  = 0x000C4,
  VIDW04ADD0B2  = 0x020C0,    // v6.2 for triple buffer

  VIDW00ADD1B0  = 0x000D0,
  VIDW00ADD1B1  = 0x000D4,
  VIDW00ADD1B2  = 0x020D0,    // v6.2 for triple buffer 
  VIDW01ADD1B0  = 0x000D8,
  VIDW01ADD1B1  = 0x000DC,
  VIDW01ADD1B2  = 0x020D8,  // v6.2 for triple buffer 
  VIDW02ADD1B0  = 0x000E0,    //new(v6.0) win2~4 have double buffer
  VIDW02ADD1B1  = 0x000E4,
  VIDW02ADD1B2  = 0x020E0, // v6.2 for triple buffer  
  VIDW03ADD1B0  = 0x000E8,
  VIDW03ADD1B1  = 0x000EC,
  VIDW03ADD1B2  = 0x020E8,// v6.2 for triple buffer 
  VIDW04ADD1B0  = 0x000F0,
  VIDW04ADD1B1  = 0x000F4,
  VIDW04ADD1B2  = 0x020F0,// v6.2 for triple buffer 

  VIDW00ADD2      = 0x00100,
  VIDW01ADD2      = 0x00104,
  VIDW02ADD2      = 0x00108,
  VIDW03ADD2      = 0x0010C,
  VIDW04ADD2      = 0x00110,

  VP1TCON0    = 0x00118,    //new(v6.0)
  VP1TCON1    = 0x0011c,

  VIDINTCON0    = 0x00130,
  VIDINTCON1    = 0x00134,
  W1KEYCON0   = 0x00140,
  W1KEYCON1   = 0x00144,
  W2KEYCON0   = 0x00148,
  W2KEYCON1   = 0x0014C,
  W3KEYCON0   = 0x00150,
  W3KEYCON1   = 0x00154,
  W4KEYCON0   = 0x00158,
  W4KEYCON1   = 0x0015C,

  W1KEYALPHA  = 0x00160,    //new(v6.0)
  W2KEYALPHA  = 0x00164,
  W3KEYALPHA  = 0x00168,
  W4KEYALPHA  = 0x0016C,

  DITHMODE    = 0x00170,

  WIN0MAP   = 0x00180,
  WIN1MAP   = 0x00184,
  WIN2MAP   = 0x00188,
  WIN3MAP   = 0x0018C,
  WIN4MAP   = 0x00190,

  WPALCON_H   = 0x0019C,
  WPALCON_L   = 0x001A0,  
  TRIGCON   = 0x001A4,

  ITUIFCON0   = 0x001A8, 

  I80IFCONA0    = 0x001B0,
  I80IFCONA1    = 0x001B4,
  I80IFCONB0    = 0x001B8,
  I80IFCONB1    = 0x001BC,

  COLORGAINCON  = 0x001C0,    //new(v6.0)

  VESFRCON0   = 0x001C4,    //new(v6.0)
  VESFRCON1   = 0x001C8,  
  VESFRCON2   = 0x001CC,

  LDI_CMDCON0 = 0x001D0,
  LDI_CMDCON1 = 0x001D4,

  SIFCCON0    = 0x001E0,
  SIFCCON1    = 0x001E4,
  SIFCCON2    = 0x001E8,

  HUECOEF00   = 0x001EC,    //new(v6.0)
  HUECOEF01   = 0x001F0,
  HUECOEF10   = 0x001F4,  
  HUECOEF11   = 0x001F8,
  HUEOFFSET   = 0x001FC,

  VIDW0ALPHA0 = 0x00200,
  VIDW0ALPHA1 = 0x00204,
  VIDW1ALPHA0 = 0x00208,
  VIDW1ALPHA1 = 0x0020C,
  VIDW2ALPHA0 = 0x00210,
  VIDW2ALPHA1 = 0x00214,
  VIDW3ALPHA0 = 0x00218,
  VIDW3ALPHA1 = 0x0021C,
  VIDW4ALPHA0 = 0x00220,
  VIDW4ALPHA1 = 0x00224,
  BLENDEQ1    = 0x00244,
  BLENDEQ2    = 0x00248,
  BLENDEQ3    = 0x0024C,
  BLENDEQ4    = 0x00250,
  BLENDCON    = 0x00260,

  W0RTQOSCON  = 0x00264,    //new(v6.0)
  W1RTQOSCON  = 0x00268,
  W2RTQOSCON  = 0x0026C,
  W3RTQOSCON  = 0x00270,
  W4RTQOSCON  = 0x00274,
  GPOUTCON0   = 0x00278,
  DUALCON   = 0x0027C,    //new(v6.0) need to change Reg. name (Temporary name now 09.Feb.16th)

  LDI_CMD0    = 0x00280,
  LDI_CMD1    = 0x00284,
  LDI_CMD2    = 0x00288,
  LDI_CMD3    = 0x0028C,
  LDI_CMD4    = 0x00290,
  LDI_CMD5    = 0x00294,
  LDI_CMD6    = 0x00298,
  LDI_CMD7    = 0x0029C,
  LDI_CMD8    = 0x002A0,
  LDI_CMD9    = 0x002A4,
  LDI_CMD10   = 0x002A8,
  LDI_CMD11   = 0x002AC,

  GAMMALUT_01_00  = 0x0037C,    //new(v6.0)
  GAMMALUT_03_02  = 0x00380,
  GAMMALUT_05_04  = 0x00384,
  GAMMALUT_07_06  = 0x00388,
  GAMMALUT_09_08  = 0x0038C,
  GAMMALUT_11_10  = 0x00390,
  GAMMALUT_13_12  = 0x00394,
  GAMMALUT_15_14  = 0x00398,
  GAMMALUT_17_16  = 0x0039C,
  GAMMALUT_19_18  = 0x003A0,
  GAMMALUT_21_20  = 0x003A4,
  GAMMALUT_23_22  = 0x003A8,
  GAMMALUT_25_24  = 0x003AC,
  GAMMALUT_27_26  = 0x003B0,
  GAMMALUT_29_28  = 0x003B4,
  GAMMALUT_31_30  = 0x003B8,
  GAMMALUT_33_32  = 0x003BC,
  GAMMALUT_35_34  = 0x003C0,
  GAMMALUT_37_36  = 0x003C4,
  GAMMALUT_39_38  = 0x003C8,
  GAMMALUT_41_40  = 0x003CC,
  GAMMALUT_43_42  = 0x003D0,
  GAMMALUT_45_44  = 0x003D4,
  GAMMALUT_47_46  = 0x003D8,
  GAMMALUT_49_48  = 0x003DC,
  GAMMALUT_51_50  = 0x003E0,
  GAMMALUT_53_52  = 0x003E4,
  GAMMALUT_55_54  = 0x003E8,
  GAMMALUT_57_56  = 0x003EC,
  GAMMALUT_59_58  = 0x003F0,
  GAMMALUT_61_60  = 0x003F4,
  GAMMALUT_63_62  = 0x003F8,
  GAMMALUT_xx_64  = 0x003FC,

  W0PDATA01_L   = 0x022c0,
  W0PDATA23_L   = 0x022c4,
  W0PDATA45_L   = 0x022c8,
  W0PDATA67_L   = 0x022cc,
  W0PDATA89_L   = 0x022d0,
  W0PDATAAB_L   = 0x022d4,
  W0PDATACD_L   = 0x022d8,
  W0PDATAEF_L   = 0x022dc,
  W0PDATA01_H   = 0x022e0,
  W0PDATA23_H   = 0x022e4,
  W0PDATA45_H   = 0x022e8,
  W0PDATA67_H   = 0x022ec,
  W0PDATA89_H   = 0x022f0,
  W0PDATAAB_H   = 0x022f4,
  W0PDATACD_H   = 0x022f8,
  W0PDATAEF_H   = 0x022fc,
  W1PDATA01_L   = 0x02300,
  W1PDATA23_L   = 0x02304,
  W1PDATA45_L   = 0x02308,
  W1PDATA67_L   = 0x0230c,
  W1PDATA89_L   = 0x02310,
  W1PDATAAB_L   = 0x02314,
  W1PDATACD_L   = 0x02318,
  W1PDATAEF_L   = 0x0231c,
  W1PDATA01_H   = 0x02320,
  W1PDATA23_H   = 0x02324,
  W1PDATA45_H   = 0x02328,
  W1PDATA67_H   = 0x0232c,
  W1PDATA89_H   = 0x02330,
  W1PDATAAB_H   = 0x02334,
  W1PDATACD_H   = 0x02338,
  W1PDATAEF_H   = 0x0233c,  
  W2PDATA01_L   = 0x02340,
  W2PDATA23_L   = 0x02344,
  W2PDATA45_L   = 0x02348,
  W2PDATA67_L   = 0x0234c,
  W2PDATA89_L   = 0x02350,
  W2PDATAAB_L   = 0x02354,
  W2PDATACD_L   = 0x02358,
  W2PDATAEF_L   = 0x0235c,
  W2PDATA01_H   = 0x02360,
  W2PDATA23_H   = 0x02364,
  W2PDATA45_H   = 0x02368,
  W2PDATA67_H   = 0x0236c,
  W2PDATA89_H   = 0x02370,
  W2PDATAAB_H   = 0x02374,
  W2PDATACD_H   = 0x02378,
  W2PDATAEF_H   = 0x0237c,
  W3PDATA01_L   = 0x02380,
  W3PDATA23_L   = 0x02384,
  W3PDATA45_L   = 0x02388,
  W3PDATA67_L   = 0x0238c,
  W3PDATA89_L   = 0x02390,
  W3PDATAAB_L   = 0x02394,
  W3PDATACD_L   = 0x02398,
  W3PDATAEF_L   = 0x0239c,
  W3PDATA01_H   = 0x023a0,
  W3PDATA23_H   = 0x023a4,
  W3PDATA45_H   = 0x023a8,
  W3PDATA67_H   = 0x023ac,
  W3PDATA89_H   = 0x023b0,
  W3PDATAAB_H   = 0x023b4,
  W3PDATACD_H   = 0x023b8,
  W3PDATAEF_H   = 0x023bc,
  W4PDATA01_L   = 0x023c0,
  W4PDATA23_L   = 0x023c4,
  W4PDATA45_L   = 0x023c8,
  W4PDATA67_L   = 0x023cc,
  W4PDATA89_L   = 0x023d0,
  W4PDATAAB_L   = 0x023d4,
  W4PDATACD_L   = 0x023d8,
  W4PDATAEF_L   = 0x023dc,
  W4PDATA01_H   = 0x023e0,
  W4PDATA23_H   = 0x023e4,
  W4PDATA45_H   = 0x023e8,
  W4PDATA67_H   = 0x023ec,
  W4PDATA89_H   = 0x023f0,
  W4PDATAAB_H   = 0x023f4,
  W4PDATACD_H   = 0x023f8,
  W4PDATAEF_H   = 0x023fc,


  W0PRAMSTART   = 0x02400,
  W1PRAMSTART       = 0x02800,
  W2PRAMSTART       = 0x02c00,
  W3PRAMSTART       = 0x03000,
  W4PRAMSTART       = 0x03400,

  VELUT1_00     = 0x04800,

  VELUT2_000      = 0x05000
};

#define DSI_DISABLE       (0<<30)
#define DSI_ENABLE        (1<<30)

#define VID_PROGRESSIVE     (0<<29)
#define VID_INTERLACE     (1<<29)

#define VIDOUT_RGBIF      (0<<26)
#define VIDOUT_TV       (1<<26)    // v6.0's case this is Write Back path
#define VIDOUT_I80IF0     (2<<26)
#define VIDOUT_I80IF1     (3<<26)
#define VIDOUT_TVNRGBIF         (4<<26)   // v6.0's case this is RGB(LCD) and WriteBack dual path (TV = WB)
#define VIDOUT_TVNI80IF0    (6<<26) 
#define VIDOUT_TVNI80IF1    (7<<26) 

// VIDW0ALPHA0, VIDW0ALPHA1 (VIDeo Window 0 ALPHA register)
#define ALPHA0_R(n)       (((n)&0xFF)<<16)
#define ALPHA0_G(n)       (((n)&0xFF)<<8)
#define ALPHA0_B(n)       (((n)&0xFF)<<0)
#define ALPHA1_R(n)       (((n)&0xFF)<<16)
#define ALPHA1_G(n)       (((n)&0xFF)<<8)
#define ALPHA1_B(n)       (((n)&0xFF)<<0)

// VIDOSD1C, VIDOSD2C, VIDOSD3C, VIDOSD4C       new(v6.0)
// (VIDeo window 1,2,3,4 position control C register)
#define ALPHA0_R_H(n)     (((n)&0xF)<<20)
#define ALPHA0_G_H(n)     (((n)&0xF)<<16)
#define ALPHA0_B_H(n)     (((n)&0xF)<<12)
#define ALPHA1_R_H(n)     (((n)&0xF)<<8)
#define ALPHA1_G_H(n)     (((n)&0xF)<<4)
#define ALPHA1_B_H(n)     (((n)&0xF)<<0)

// VIDW1ALPHA0, VIDW1ALPHA1 (VIDeo Window 1 ALPHA register)
// VIDW2ALPHA0, VIDW2ALPHA1 (VIDeo Window 2 ALPHA register)
// VIDW3ALPHA0, VIDW3ALPHA1 (VIDeo Window 3 ALPHA register)
// VIDW4ALPHA0, VIDW4ALPHA1 (VIDeo Window 4 ALPHA register)
#define ALPHA0_R_L(n)     (((n)&0xF)<<16)
#define ALPHA0_G_L(n)     (((n)&0xF)<<8)
#define ALPHA0_B_L(n)     (((n)&0xF)<<0)
#define ALPHA1_R_L(n)     (((n)&0xF)<<16)
#define ALPHA1_G_L(n)     (((n)&0xF)<<8)
#define ALPHA1_B_L(n)     (((n)&0xF)<<0)

// New feature of FIMD v6.0 (Image Enhancement Block)
// BLENDEQ1, BLENDEQ2, BLENDEQ3, BLENDEQ4 (BLENDing EQuation)
#define Q_FUNC(n)       (((n)&0xF)<<18)
#define P_FUNC(n)       (((n)&0xF)<<12)
#define B_FUNC(n)       (((n)&0xF)<<6)
#define A_FUNC(n)       (((n)&0xF)<<0)

// WINCON0, 1, 2, 3, 4 SFR's
#define LIMIT_DISABLE     (0<<29)   //new(v6.0)
#define LIMIT_ENABLE      (1<<29)
#define EQ601         (0<<28)
#define EQ709         (1<<28)
#define WIDE            (0<<26)
#define NARROW          (3<<26)
#define PATH_DMA        (0<<22)
#define PATH_LOCAL        (1<<22)

#define WBUF0         (0<<20)
#define WBUF1         (1<<20)
#define WBUF_MASK       (1<<20)
#define WBUFAUTO_DISABLE    (0<<19)
#define WBUFAUTO_ENABLE   (1<<19)
#define WBUFAUTO_MASK         (1<<19)

#define BITSWP_DISABLE      (0<<18)
#define BITSWP_ENABLE     (1<<18)
#define BYTSWP_DISABLE      (0<<17)
#define BYTSWP_ENABLE     (1<<17)
#define HAWSWP_DISABLE        (0<<16)
#define HAWSWP_ENABLE     (1<<16)
#define WRDSWP_DISABLE          (0<<15)
#define WRDSWP_ENABLE     (1<<15)

#define DOUBLE_BUFFER_MODE  (0<<14)
#define TRIPLE_BUFFER_MODE  (1<<14)

#define IN_LOCAL_RGB      (0<<13)
#define IN_LOCAL_YUV      (1<<13)
#define INRGB_MASK        (1<<13)

#define MAX_BURSTLEN_16WORD (0<<9)
#define MAX_BURSTLEN_8WORD  (1<<9)
#define MAX_BURSTLEN_4WORD  (2<<9)

#define ALPHA_MUL_DISABLE   (0<<7)
#define ALPHA_MUL_ENABLE    (1<<7)

#define BLD_PIX_PLANE     (0<<6)
#define BLD_PIX_PIXEL     (1<<6)
#define BLD_PIX_MASK      (1<<6)

#define BPPMODE_F_1BPP      (0<<2)
#define BPPMODE_F_2BPP      (1<<2)
#define BPPMODE_F_4BPP      (2<<2)
#define BPPMODE_F_8BPP_PAL  (3<<2)
#define BPPMODE_F_8BPP_NOPAL  (4<<2) // A:1 R:2 G:3 B:2
#define BPPMODE_F_16BPP_565 (5<<2)
#define BPPMODE_F_16BPP_A555  (6<<2)
#define BPPMODE_F_16BPP_I555  (7<<2)
#define BPPMODE_F_18BPP_666 (8<<2)
#define BPPMODE_F_18BPP_A665  (9<<2)
#define BPPMODE_F_19BPP_A666  (0xa<<2)
#define BPPMODE_F_24BPP_888 (0xb<<2)
#define BPPMODE_F_24BPP_A887  (0xc<<2)
#define BPPMODE_F_25BPP_A888  (0xd<<2)
#define BPPMODE_F_13BPP_A444  (0xe<<2)
#define BPPMODE_F_32BPP_8888  (0xd<<2)
#define BPPMODE_F_16BPP_4444  (0xe<<2)
#define BPPMODE_F_15BPP_555 (0xf<<2)
#define BPPMODE_F_MASK    (0xf<<2)

// W1KEYCON0, W2KEYCON0, W3KEYCON0, W4KEYCON0(Color Key Control Register)
#define KEYBLACKOUT_DISABLE   (0<<31)
#define KEYBLACKOUT_ENABLE    (1<<31)
#define KEYBLEN_F_DISABLE     (0<<26)
#define KEYBLEN_F_ENABLE      (1<<26)
#define KEYEN_F_DISABLE         (0<<25)
#define KEYEN_F_ENABLE      (1<<25)
#define DIRCON_MATCH_FG_IMAGE   (0<<24)   //display pixel from BG img
#define DIRCON_MATCH_BG_IMAGE (1<<24)   //display pixel from FG img

// DITHMODE (DITHering MODE register)
#define RDITHPOS_8BIT     (0<<5)
#define RDITHPOS_6BIT     (1<<5)
#define RDITHPOS_5BIT     (2<<5)
#define GDITHPOS_8BIT     (0<<3)
#define GDITHPOS_6BIT     (1<<3)
#define GDITHPOS_5BIT     (2<<3)
#define BDITHPOS_8BIT     (0<<1)
#define BDITHPOS_6BIT     (1<<1)
#define BDITHPOS_5BIT     (2<<1)
#define RGB_DITHPOS_MASK    (0x3f<<1)
#define DITHERING_DISABLE   (0<<0)
#define DITHERING_ENABLE    (1<<0)

// WIN0MAP, WIN1MAP, WIN2MAP, WIN3MAP, WIN4MAP (WINdow color (MAP) control)
#define COLOR_MAP_DISABLE   (0<<24)
#define COLOR_MAP_ENABLE    (1<<24)

// TRIGCON (I80/RGB LCDC_Trigger CONtrol)
// LCDC_Trigger
#define W4_SW_TRIGGER     (1<<26)
#define W4_SW_TRIG_MODE   (1<<25)
#define W3_SW_TRIGGER     (1<<21)
#define W3_SW_TRIG_MODE   (1<<20)
#define W2_SW_TRIGGER     (1<<16)
#define W2_SW_TRIG_MODE   (1<<15)
#define W1_SW_TRIGGER     (1<<11)
#define W1_SW_TRIG_MODE   (1<<10)
#define W0_SW_TRIGGER     (1<<6)
#define W0_SW_TRIG_MODE   (1<<5)
#define I80_SW_TRIGGER      (1<<1)
#define I80_SW_TRIG_MODE    (1<<0)
#define SW_TRIGGER        (1<<1)    //need to be changed with src code(08.12.10)
#define SW_TRIG_MODE      (1<<0)    //need to be changed with src code(08.12.10)

// CPU Interface Data Format (SUB LDI)
#define SUB_16_MODE     (0<<23)
#define SUB_16PLUS2_MODE    (1<<23)
#define SUB_9PLUS9_MODE       (2<<23)
#define SUB_16PLUS8_MODE    (3<<23)
#define SUB_18_MODE         (4<<23)
#define SUB_8PLUS8_MODE   (5<<23)

// CPU Interface Data Format (MAIN LDI)
#define MAIN_16_MODE      (0<<20)
#define MAIN_16PLUS2_MODE   (1<<20)
#define MAIN_9PLUS9_MODE    (2<<20)
#define MAIN_16PLUS8_MODE   (3<<20)
#define MAIN_18_MODE      (4<<20)
#define MAIN_8PLUS8_MODE    (5<<20)   // new(v6.0)

// VIDCON0 (CPU I/F Masking)
#define LCD_OUT_MASKING       ~(0x3<<26)
#define SUB_DATA_MASKING    ~(0x7<<23)
#define MAIN_DATA_MASKING   ~(0x7<<20)

#define PNRMODE_RGB_P     (0<<17)
#define PNRMODE_BGR_P     (1<<17)
#define PNRMODE_RGB_S     (2<<17)
#define PNRMODE_BGR_S     (3<<17)
#define PNRMODE_MASK      (3<<17)

#define CLKVALUP_ALWAYS       (0<<16)
#define CLKVALUP_ST_FRM       (1<<16)

#define CLKVAL_F(n)       (((n)&0xFF)<<6)

#define VCLKFREE_NORMAL   (0<<5) 
#define VCLKFREE_FREERUN    (1<<5) 

#define CLKDIR_F(n)       (((n)&0x1)<<4)
#define CLKDIR_DIRECT     (0<<4)
#define CLKDIR_DIVIDED      (1<<4)
 
#define CLKSEL_F_HCLK     (0<<2)
#define CLKSEL_F_SCLK         (1<<2)    // LCD video Clock(fr SYSCON)
 
#define ENVID_DISABLE     (0<<1)
#define ENVID_ENABLE      (1<<1)

#define ENVID_F_DISABLE     (0<<0)
#define ENVID_F_ENABLE      (1<<0)

// VIDTCON0 (VIDeo Time CONtrol 0 register)
#define VBPDE(n)          (((n)&0xFF)<<24)
#define VBPD(n)         (((n)&0xFF)<<16)
#define VFPD(n)         (((n)&0xFF)<<8)
#define VSPW(n)         (((n)&0xFF)<<0)

// VIDTCON1 (VIDeo Time CONtrol 1 register)
#define VFPDE(n)          (((n)&0xFF)<<24)
#define HBPD(n)         (((n)&0xFF)<<16)
#define HFPD(n)         (((n)&0xFF)<<8)
#define HSPW(n)         (((n)&0xFF)<<0)

// VIDTCON2 (VIDeo Time CONtrol 2 register)
#define LINEVAL(n)        (((n)&0x7FF)<<11)
#define HOZVAL(n)       (((n)&0x7FF)<<0)

// VIDTCON3 (VIDeo Time CONtrol 3 register)
#define VSYNC_DISABLE     (0<<31)
#define VSYNC_ENABLE      (1<<31)
#define FRM_DISABLE       (0<<29)
#define FRM_ENABLE        (1<<29)
#define INVFRM_HIGH       (0<<28)
#define INVFRM_LOW        (1<<28)
#define FRMVRATE(n)       (((n)&0xF)<<24)
#define FRMVFPD(n)        (((n)&0xFF)<<8)
#define FRMVSPW(n)        (((n)&0xFF)<<0)

// WINCON1,2,3,4 BLEND_SELECT_MODE
#define BLEND_ALPHA0_PLANE    (0<<1)
#define BLEND_ALPHA1_PLANE    (1<<1)
#define BLEND_AEN_PIXEL     (0<<1)
#define BLEND_DATA_PIXEL    (1<<1)

#define ALPHA_SEL_0       ~(1<<1)   // 

#define BLEND_COLORKEY_AREA (0<<1)    // ???

#define BLEND_SEL_MASK      (1<<1)

#define ENWIN_F_DISABLE       (0<<0)
#define ENWIN_F_ENABLE      (1<<0)

// VIDOSD0A, VIDOSD1A, VIDOSD2A,VIDOSD3A, VIDOSD4A
// (VIDeo window 0, 1, 2, 3, 4 position control A register)
#define OSD_LTX_F(n)        (((n)&0x7FF)<<11)
#define OSD_LTY_F(n)        (((n)&0x7FF)<<0)

// VIDOSD0B, VIDOSD1B, VIDOSD2B,VIDOSD3B, VIDOSD4B
// (VIDeo window 0, 1, 2, 3, 4 position control B register)
#define OSD_RBX_F(n)      (((n)&0x7FF)<<11)
#define OSD_RBY_F(n)      (((n)&0x7FF)<<0)

// VIDOSD0C, VIDOSD1D, VIDOSD2D
// (VIDeo window 0 position control C & window 1,2 position control D register)
#define OSDSIZE(n)        (((n)&0xFFFFFF)<<0)

// VIDINTCON0 (VIDeo INTerrupt CONtrol 0 register)
// (LCD INTERRUPT CONTROL REGISTER)
#define INTVPEN_DISABLE     (0<<26)   //new(v6.0)
#define INTVPEN_ENABLE      (1<<26)
#define SYSMAINCON_DISABLE    (0<<19)
#define SYSMAINCON_ENABLE   (1<<19)
#define SYSSUBCON_DISABLE   (0<<18)
#define SYSSUBCON_ENABLE    (1<<18)
#define I80IFDONE_DISABLE   (0<<17)
#define I80IFDONE_ENABLE    (1<<17)
#define FRAMESEL0_BACK      (0<<15)
#define FRAMESEL0_VSYNC       (1<<15)
#define FRAMESEL0_ACTIVE    (2<<15)
#define FRAMESEL0_FRONT       (3<<15)
#define FRAMESEL1_NONE      (0<<13)
#define FRAMESEL1_BACK      (1<<13)
#define FRAMESEL1_VSYNC       (2<<13)
#define FRAMESEL1_FRONT   (3<<13)
#define INTFRMEN_DISABLE    (0<<12)
#define INTFRMEN_ENABLE       (1<<12)
#define FIFOSEL_WIN4      (1<<11)   // enable FIFO Interrupt of WIN4
#define FIFOSEL_WIN3      (1<<10)
#define FIFOSEL_WIN2      (1<<9)
#define FIFOSEL_WIN1      (1<<6)
#define FIFOSEL_WIN0      (1<<5)
#define FIFOSEL_ALL       (FIFOSEL_WIN0 | FIFOSEL_WIN1 | FIFOSEL_WIN2 | FIFOSEL_WIN3 | FIFOSEL_WIN4)
#define FIFOLEVEL_25      (0<<2)
#define FIFOLEVEL_50      (1<<2)
#define FIFOLEVEL_75      (2<<2)
#define FIFOLEVEL_EMPTY     (3<<2)
#define FIFOLEVEL_FULL      (4<<2)
#define INTFIFOEN_DISABLE   (0<<1)
#define INTFIFOEN_ENABLE    (1<<1)
#define INTEN_DISABLE     (0<<0)
#define INTEN_ENABLE      (1<<0)

// VIDCON1 (VIDeo main CONtrol 1 register)
#define IVCLK_FALL_EDGE     (0<<7)
#define IVCLK_RISE_EDGE     (1<<7)
#define IHSYNC_NORMAL     (0<<6)
#define IHSYNC_INVERT     (1<<6)
#define IVSYNC_NORMAL     (0<<5)
#define IVSYNC_INVERT     (1<<5)
#define IVDEN_NORMAL      (0<<4)
#define IVDEN_INVERT      (1<<4)

// VIDCON2 (VIDeo main CONtrol 2 register)
#define RGB_SKIP_DISABLE    (0<<27)   // new(v6.0)
#define RGB_SKIP_ENABLE   (1<<27)
#define RGB_DUMMY_LAST      (0<<25)
#define RGB_DUMMY_FIRST   (1<<25)
#define RGB_DUMMY_DISABLE   (0<<24)
#define RGB_DUMMY_ENABLE    (1<<24)

#define EN601_DISABLE     (0<<23)
#define EN601_ENABLE      (1<<23)
#define EN656_DISABLE     (0<<22)
#define EN656_ENABLE      (1<<22)

#define RGB_ORDER_E_RGB   (0<<19)   // new(v6.0)
#define RGB_ORDER_E_GBR   (1<<19)
#define RGB_ORDER_E_BRG   (2<<19)
#define RGB_ORDER_E_BGR   (4<<19)
#define RGB_ORDER_E_RBG   (5<<19)
#define RGB_ORDER_E_GRB   (6<<19)

#define RGB_ORDER_O_RGB   (0<<16)   // new(v6.0)
#define RGB_ORDER_O_GBR   (1<<16)
#define RGB_ORDER_O_BRG   (2<<16)
#define RGB_ORDER_O_BGR   (4<<16)
#define RGB_ORDER_O_RBG   (5<<16)
#define RGB_ORDER_O_GRB   (6<<16)

extern u32 gLCDM_HSIZE;
extern u32 gLCDM_VSIZE; 
extern u32 gVCLK_EDGE_DETECT;
extern u32 gHSYNC_POLARITY;
extern u32 gVSYNC_POLARITY;
extern u32 gVDEN_POLARITY;
extern u32 gLCDM_INTERFACE_TYPE;
extern u32 gDITHERING_FORMAT;
extern u32 gMODULE_VBPDE;
extern u32 gMODULE_VBPD;
extern u32 gMODULE_VFPD;
extern u32 gMODULE_VSPW;
extern u32 gMODULE_VFPDE;     
extern u32 gMODULE_HBPD;         
extern u32 gMODULE_HFPD;        
extern u32 gMODULE_HSPW;

LCDC oLcdc;

void LCDC_Start(void)
{
  u32 uLcdCon;

  LcdcInp32(VIDCON0, uLcdCon);
  uLcdCon |= 0x3<<0;
  LcdcOutp32(VIDCON0, uLcdCon);
}

void LCDC_Stop(void)  // Direct off
{
  u32 uLcdCon;
  u32 uSfr;

  LcdcInp32(VIDCON0, uLcdCon);
  uLcdCon &=~ (3<<0);
  LcdcOutp32(VIDCON0, uLcdCon);
}

void LCDC_InitIp(IMG_FMT eBpp, u32 uFbAddr, LCD_WINDOW eWin, bool bIsDoubleBuf)
{
  LCDC_InitBase();
  LCDC_InitWin(eBpp, oLcdc.m_uLcdHSz, oLcdc.m_uLcdVSz, 0, 0, oLcdc.m_uLcdHSz, oLcdc.m_uLcdVSz,
      0, 0, uFbAddr, eWin, bIsDoubleBuf);
}

void LCDC_InitBase(void)
{
  u32 uVidconReg=0;
  u32 uVidCon2Reg=0;
  u32 uTmpReg;
  u32 uVidOutFormat;
  u32 uLcdCon;
  u32 uClkVal, uClkDir;
  u32 uVlkcPolarity,uHsyncPolarity,uVsyncPolarity,uVdenPolarity;
  LCD_WINDOW eWin;
  u32 i;

  oLcdc.m_bAutoBuf = false;
  oLcdc.m_uScanMode = 0; // progressive mode
  oLcdc.m_bFifoIn[0] = false;
  oLcdc.m_bFifoIn[1] = false;
  oLcdc.m_bFifoIn[2] = false;

  oLcdc.m_uLcdHSz = gLCDM_HSIZE;
  oLcdc.m_uLcdVSz = gLCDM_VSIZE;

  if(gLCDM_INTERFACE_TYPE == RGB_PARALLEL_24bpp || 
      gLCDM_INTERFACE_TYPE == RGB_PARALLEL_18bpp || 
      gLCDM_INTERFACE_TYPE == RGB_PARALLEL_16bpp || 
      gLCDM_INTERFACE_TYPE == RGB_DELTA_STRUCTURE || 
      gLCDM_INTERFACE_TYPE == RGB_SERIAL)
    uVidOutFormat = VIDOUT_RGBIF;
  else
  {
    LcdcInp32(VIDCON0,uTmpReg);
    uTmpReg = (uTmpReg>>26) & 0x3;
    if(uTmpReg == 0x3)
      uVidOutFormat = VIDOUT_I80IF1;
    else
      uVidOutFormat = VIDOUT_I80IF0;
  }

  switch(gDITHERING_FORMAT)
  {
    case DITHER_888:
      oLcdc.m_uDitherMode = RDITHPOS_8BIT|GDITHPOS_8BIT|BDITHPOS_8BIT;
      break;
    case DITHER_666:
      oLcdc.m_uDitherMode = RDITHPOS_6BIT|GDITHPOS_6BIT|BDITHPOS_6BIT;
      break;
    case DITHER_565:
      oLcdc.m_uDitherMode = RDITHPOS_5BIT|GDITHPOS_6BIT|BDITHPOS_5BIT;
      break;
    case DITHER_555:
      oLcdc.m_uDitherMode = RDITHPOS_5BIT|GDITHPOS_5BIT|BDITHPOS_5BIT;
      break;
  }

  oLcdc.m_uDitherMode &= ~DITHERING_ENABLE;

  if(gVCLK_EDGE_DETECT == VCLK_RISING_EDGE)
    uVlkcPolarity = 0x0 | IVCLK_RISE_EDGE;
  else
    uVlkcPolarity = 0x0 | IVCLK_FALL_EDGE;

  if(gHSYNC_POLARITY == HSYNC_INVERT)
    uHsyncPolarity = 0x0 | IHSYNC_INVERT;
  else
    uHsyncPolarity = 0x0 | IHSYNC_NORMAL;

  if(gVSYNC_POLARITY == VSYNC_INVERT)
    uVsyncPolarity = 0x0 | IVSYNC_INVERT;
  else
    uVsyncPolarity = 0x0 | IVSYNC_NORMAL;

  if(gVDEN_POLARITY == VDEN_INVERT)
    uVdenPolarity = 0x0 | IVDEN_INVERT;
  else
    uVdenPolarity = 0x0 | IVDEN_NORMAL;

  LcdcOutp32(VIDCON1, uVlkcPolarity | uHsyncPolarity | uVsyncPolarity | uVdenPolarity);   
  LcdcOutp32(VIDTCON0, VBPDE(gMODULE_VBPDE) | VBPD(gMODULE_VBPD) | VFPD(gMODULE_VFPD) | VSPW(gMODULE_VSPW));
  LcdcOutp32(VIDTCON1, VFPDE(gMODULE_VFPDE) | HBPD(gMODULE_HBPD) | HFPD(gMODULE_HFPD) | HSPW(gMODULE_HSPW));
  LcdcOutp32(DITHMODE, oLcdc.m_uDitherMode); // Fixed Dithering Matrix

  LcdcOutp32(VIDTCON2, LINEVAL(oLcdc.m_uLcdVSz-1) | HOZVAL(oLcdc.m_uLcdHSz-1));
  LCDC_Stop();

  // Check up LCD to turn off
  while (1)
  {
    LcdcInp32(VIDCON0, uLcdCon);
    if( (uLcdCon&0x03) == 0 ) // checking whether disable the video output and the Display control signal or not.
      break;
  }

  LCDC_GetClkValAndClkDir(oLcdc.m_uLcdHSz, oLcdc.m_uLcdVSz, &uClkVal, &uClkDir);
  printf("LCDC_InitBase()==========================> uClkVal=%d, uClkDir=%d\n", uClkVal, uClkDir);

  if(gLCDM_INTERFACE_TYPE==RGB_SERIAL)
  {
    if(LCD_VCLKSRC == LCD_HCLK)
    {
      uVidconReg = VID_PROGRESSIVE | uVidOutFormat | SUB_16_MODE | MAIN_16_MODE | PNRMODE_RGB_S | CLKVALUP_ALWAYS |
        CLKVAL_F(uClkVal) | VCLKFREE_NORMAL | CLKDIR_F(uClkDir) | CLKSEL_F_HCLK |
        ENVID_DISABLE | ENVID_F_DISABLE;
    }
    else
    {
      uVidconReg = VID_PROGRESSIVE | uVidOutFormat | SUB_16_MODE | MAIN_16_MODE | PNRMODE_RGB_S | CLKVALUP_ALWAYS |
        CLKVAL_F(uClkVal) | VCLKFREE_NORMAL | CLKDIR_F(uClkDir) | CLKSEL_F_SCLK |
        ENVID_DISABLE | ENVID_F_DISABLE;
    }
  }
  else
  {
    if(LCD_VCLKSRC == LCD_HCLK)
    {
      uVidconReg = VID_PROGRESSIVE | uVidOutFormat | SUB_16_MODE | MAIN_16_MODE | PNRMODE_RGB_P | CLKVALUP_ALWAYS |
        CLKVAL_F(uClkVal) | VCLKFREE_NORMAL | CLKDIR_F(uClkDir) | CLKSEL_F_HCLK |
        ENVID_DISABLE | ENVID_F_DISABLE;
    }
    else
    {
      uVidconReg = VID_PROGRESSIVE | uVidOutFormat | SUB_16_MODE | MAIN_16_MODE | PNRMODE_RGB_P | CLKVALUP_ALWAYS |
        CLKVAL_F(uClkVal) | VCLKFREE_NORMAL | CLKDIR_F(uClkDir) | CLKSEL_F_SCLK |
        ENVID_DISABLE | ENVID_F_DISABLE;
    }
  }

  if(gLCDM_INTERFACE_TYPE==RGB_DELTA_STRUCTURE)
  {
    uVidCon2Reg = RGB_ORDER_E_GBR | RGB_ORDER_O_RGB | RGB_SKIP_ENABLE;
    LcdcOutp32(VIDCON2, uVidCon2Reg);
  }

  LcdcOutp32(VIDCON0, uVidconReg);

  LcdcOutp32(VIDINTCON0, FRAMESEL0_BACK | FRAMESEL1_NONE | INTFRMEN_DISABLE |
      FIFOSEL_WIN0 | FIFOLEVEL_25 | INTFIFOEN_DISABLE | INTEN_DISABLE);

  LCDC_SetAllWinOnOff(0); // Turn all windows off
  LCDC_SetAllWinColorMapOnOff(0); // Turn all windows color map off
  LCDC_SetAllWinColorKeyOnOff(0); // Turn all windows Color Key off

  for (i=1; i<5; i++)
  {
    eWin =
      (i == 1) ? WIN1 :
      (i == 2) ? WIN2 :
      (i == 3) ? WIN3 : WIN4;

    LCDC_SetAlpha(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, eWin);
  }
}

void LCDC_InitWin(IMG_FMT eBpp, u32 uFrameH, u32 uFrameV, u32 uX_Frame, u32 uY_Frame,  u32 uViewH, u32 uViewV,
    u32 uX_Lcd, u32 uY_Lcd, u32 uFbAddr, LCD_WINDOW eWin, LCDC_AUTO_BUFFER_MODE bIsDoubleBuf) // For WIN0
{
  u32 uOffset;
  u32 uLineVal;
  u32 uBurstSize;

  printf("LCDC_InitWin()==>uFrameH=%d, uFrameV=%d\n", uFrameH, uFrameV);
  oLcdc.m_uFrmHSz[eWin] = uFrameH;
  oLcdc.m_uFrmVSz[eWin] = uFrameV;

  oLcdc.m_uViewHSz[eWin] = uViewH;
  oLcdc.m_uViewVSz[eWin] = uViewV;

  oLcdc.m_uLcdStX = uX_Lcd;
  oLcdc.m_uLcdStY = uY_Lcd;

  uOffset = oLcdc.m_uFrmHSz[eWin] - oLcdc.m_uViewHSz[eWin];

  oLcdc.m_uBytSwpMode = BYTSWP_DISABLE;
  oLcdc.m_uHawSwpMode = HAWSWP_DISABLE;

  oLcdc.m_uMaxBurstLen = MAX_BURSTLEN_16WORD;

  printf("LCDC_InitWin()==>debug01\n");
  if (eBpp == PAL1)
  {
  printf("LCDC_InitWin()==>debug02\n");
    oLcdc.m_uBytes = 1;
    oLcdc.m_uBits = 4;
    Assert( !(uOffset%32) ); // Must keep to word-alignment
    oLcdc.m_uBppMode = BPPMODE_F_1BPP;

    oLcdc.m_uMaxBurstLen = MAX_BURSTLEN_4WORD;
  }
  else if (eBpp == PAL2)
  {
  printf("LCDC_InitWin()==>debug03\n");
    oLcdc.m_uBytes = 1;
    oLcdc.m_uBits = 4;
    Assert( !(uOffset%16) ); // Must keep to word-alignment
    oLcdc.m_uBppMode = BPPMODE_F_2BPP;

    oLcdc.m_uMaxBurstLen = MAX_BURSTLEN_4WORD;
  }
  else if (eBpp == PAL4)
  {
  printf("LCDC_InitWin()==>debug04\n");
    oLcdc.m_uBytes = 1;
    oLcdc.m_uBits = 2;

    Assert( !(uOffset%8) ); // Must keep to word-alignment
    oLcdc.m_uBppMode = BPPMODE_F_4BPP;

    oLcdc.m_uMaxBurstLen = MAX_BURSTLEN_4WORD;
  }
  else if (eBpp == PAL8)
  {
  printf("LCDC_InitWin()==>debug05\n");
    oLcdc.m_uBytes = 1;
    oLcdc.m_uBits = 1;
    Assert( !(uOffset%4) ); // Must keep to word-alignment
    oLcdc.m_uBppMode = BPPMODE_F_8BPP_PAL;
    oLcdc.m_uBytSwpMode = BYTSWP_ENABLE;
  }
  else if (eBpp == ARGB8) // A232
  {
  printf("LCDC_InitWin()==>debug06\n");
    oLcdc.m_uBytes = 1;
    oLcdc.m_uBits = 1;
    Assert( !(uOffset%2) ); // Must keep to word-alignment

    oLcdc.m_uBppMode = BPPMODE_F_8BPP_NOPAL;
    oLcdc.m_uHawSwpMode = BYTSWP_ENABLE;
  }
  else if (eBpp == RGB16)
  {
  printf("LCDC_InitWin()==>debug07\n");
    oLcdc.m_uBytes = 2;
    oLcdc.m_uBits = 1;
    Assert( !(uOffset%2) ); // Must keep to word-alignment

    oLcdc.m_uBppMode = BPPMODE_F_16BPP_565;
    oLcdc.m_uHawSwpMode = HAWSWP_ENABLE;
    oLcdc.m_uWordSwpMode = WRDSWP_DISABLE;
  }
  else if (eBpp == ARGB16)
  {
  printf("LCDC_InitWin()==>debug08\n");
    oLcdc.m_uBytes = 2;
    oLcdc.m_uBits = 1;
    Assert( !(uOffset%2) ); // Must keep to word-alignment

    oLcdc.m_uBppMode = BPPMODE_F_16BPP_A555;
    oLcdc.m_uHawSwpMode = HAWSWP_ENABLE;
  }
  else if (eBpp == RGB18)
  {
  printf("LCDC_InitWin()==>debug09\n");
    oLcdc.m_uBytes = 4;
    oLcdc.m_uBits = 1;

    oLcdc.m_uWordSwpMode = WRDSWP_ENABLE;
    oLcdc.m_uBppMode = BPPMODE_F_18BPP_666;
  }
  else if (eBpp == RGB24)
  {
  printf("LCDC_InitWin()==>debug10\n");
    oLcdc.m_uBytes = 4;
    oLcdc.m_uBits = 1;

    oLcdc.m_uWordSwpMode = WRDSWP_ENABLE;
    oLcdc.m_uBppMode = BPPMODE_F_24BPP_888;
  }
  else if (eBpp == ARGB24)
  {
  printf("LCDC_InitWin()==>debug10\n");
    oLcdc.m_uBytes = 4;
    oLcdc.m_uBits = 1;

    oLcdc.m_uWordSwpMode = WRDSWP_ENABLE;
    oLcdc.m_uBppMode = BPPMODE_F_24BPP_A887;
  }

  else if (eBpp == ARGB25)
  {
  printf("LCDC_InitWin()==>debug11\n");
    oLcdc.m_uBytes = 4;
    oLcdc.m_uBits = 1;
    oLcdc.m_uBppMode = BPPMODE_F_25BPP_A888;
  }

  else if (eBpp == ARGB8888)
  {
  printf("LCDC_InitWin()==>debug12\n");
    oLcdc.m_uBytes = 4;
    oLcdc.m_uBits = 1;
    oLcdc.m_uBppMode = BPPMODE_F_32BPP_8888;
  }
  else
  {
  printf("LCDC_InitWin()==>debug13\n");
    Assert(0);
  }

  printf("LCDC_InitWin()==>debug14\n");
  // Get offsetsize, pagewidth and lineVal
  if (oLcdc.m_uFrmHSz[eWin] >= oLcdc.m_uViewHSz[eWin])
  {
  printf("LCDC_InitWin()==>debug15\n");
    oLcdc.m_uOffsetSz[eWin] = uOffset*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uPageWidth[eWin] = oLcdc.m_uViewHSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;

    uBurstSize =
      (oLcdc.m_uMaxBurstLen == MAX_BURSTLEN_16WORD) ? 16*4 :
      (oLcdc.m_uMaxBurstLen == MAX_BURSTLEN_8WORD) ? 8*4 : 4*4;

    // Offset size must be more than the burst size
    Assert(oLcdc.m_uOffsetSz[eWin] == 0 || !(oLcdc.m_uOffsetSz[eWin]%4) );
    Assert(oLcdc.m_uPageWidth[eWin] > uBurstSize); // Page width must be more than burst size and be word-aligned
    Assert( !( (oLcdc.m_uPageWidth[eWin]+oLcdc.m_uOffsetSz[eWin])%4 ) );

    uLineVal = oLcdc.m_uViewVSz[eWin] - 1;
  }
  else
  {
  printf("LCDC_InitWin()==>debug16\n");
    Assert(0);
  }

  oLcdc.m_uImgStAddr[eWin] = uFbAddr;

  oLcdc.m_uFbStAddr[eWin] = uFbAddr + (oLcdc.m_uFrmHSz[eWin]*uY_Frame + uX_Frame)*oLcdc.m_uBytes/oLcdc.m_uBits;
  oLcdc.m_uFbEndAddr[eWin] = oLcdc.m_uFbStAddr[eWin] + (oLcdc.m_uOffsetSz[eWin]+oLcdc.m_uPageWidth[eWin])*(uFrameV);  // ref.VIDWxxADD1

  //   v6.0's all windows support double buffer
  if ( bIsDoubleBuf == LCDC_BUF_SINGLE)
  {
  printf("LCDC_InitWin()==>debug17\n");
    oLcdc.m_uDoubleBufImgStAddr[eWin] = oLcdc.m_uImgStAddr[eWin];
    oLcdc.m_uDoubleBufStAddr[eWin] = oLcdc.m_uFbStAddr[eWin];
    oLcdc.m_uDoubleBufEndAddr[eWin] = oLcdc.m_uFbEndAddr[eWin];

    oLcdc.m_uTripleBufImgStAddr[eWin] = oLcdc.m_uDoubleBufImgStAddr[eWin];
    oLcdc.m_uTripleBufStAddr[eWin] = oLcdc.m_uDoubleBufStAddr[eWin] ;
    oLcdc.m_uTripleBufEndAddr[eWin] = oLcdc.m_uDoubleBufEndAddr[eWin];
  }
  else if ( bIsDoubleBuf == LCDC_BUF_DOUBLE )
  {
  printf("LCDC_InitWin()==>debug18\n");
    oLcdc.m_uDoubleBufImgStAddr[eWin] = oLcdc.m_uImgStAddr[eWin] + oLcdc.m_uFrmHSz[eWin]*oLcdc.m_uFrmVSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uDoubleBufStAddr[eWin] = oLcdc.m_uFbStAddr[eWin] + oLcdc.m_uFrmHSz[eWin]*oLcdc.m_uFrmVSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uDoubleBufEndAddr[eWin] = oLcdc.m_uDoubleBufStAddr[eWin] + (oLcdc.m_uOffsetSz[eWin]+oLcdc.m_uPageWidth[eWin])*(uFrameV);

    oLcdc.m_uTripleBufImgStAddr[eWin] = oLcdc.m_uDoubleBufImgStAddr[eWin];
    oLcdc.m_uTripleBufStAddr[eWin] = oLcdc.m_uDoubleBufStAddr[eWin] ;
    oLcdc.m_uTripleBufEndAddr[eWin] = oLcdc.m_uDoubleBufEndAddr[eWin];

  }
  else if ( bIsDoubleBuf == LCDC_BUF_TRIPLE)
  {
  printf("LCDC_InitWin()==>debug19\n");
    oLcdc.m_uDoubleBufImgStAddr[eWin] = oLcdc.m_uImgStAddr[eWin] + oLcdc.m_uFrmHSz[eWin]*oLcdc.m_uFrmVSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uDoubleBufStAddr[eWin] = oLcdc.m_uFbStAddr[eWin] + oLcdc.m_uFrmHSz[eWin]*oLcdc.m_uFrmVSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uDoubleBufEndAddr[eWin] = oLcdc.m_uDoubleBufStAddr[eWin] + (oLcdc.m_uOffsetSz[eWin]+oLcdc.m_uPageWidth[eWin])*(uFrameV);

    oLcdc.m_uTripleBufImgStAddr[eWin] = oLcdc.m_uDoubleBufImgStAddr[eWin] + oLcdc.m_uFrmHSz[eWin]*oLcdc.m_uFrmVSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uTripleBufStAddr[eWin] = oLcdc.m_uDoubleBufStAddr[eWin] + oLcdc.m_uFrmHSz[eWin]*oLcdc.m_uFrmVSz[eWin]*oLcdc.m_uBytes/oLcdc.m_uBits;
    oLcdc.m_uTripleBufEndAddr[eWin] = oLcdc.m_uTripleBufStAddr[eWin] + (oLcdc.m_uOffsetSz[eWin]+oLcdc.m_uPageWidth[eWin])*(uFrameV);

  }
  else
  {
  printf("LCDC_InitWin()==>debug20\n");
    Assert(0);
  }

  // For back-ward compatibility LCDC V4.0
  // WIN0 do not support Blending Equation
  if ( eWin!=WIN0)
  {
    LCDC_SetBlendingEquation(0, 0, 3, 2, eWin);
  }
  LCDC_SetAlphaValWidth(ALPHA_8BIT);
  LCDC_InitWinRegs(eWin);

  LCDC_SetWinOnOff(1, eWin);  // Turn on window's ENWIN_F
}

void LCDC_InitWinRegs(LCD_WINDOW eWin)
{
  u32 uWinConXReg;
  u32 uFrmBufStAddrReg;
  u32 uFrmBufEndAddrReg;
  u32 uFrmDBufStAddrReg;
  u32 uFrmDBufEndAddrReg; 
  u32 uFrmDBufStAddrRegVal;
  u32 uFrmDBufEndAddrRegVal;
  u32 uFrmTBufStAddrReg;
  u32 uFrmTBufEndAddrReg; 
  u32 uOffsetPageWidthReg;
  u32 uOsdLeftPosReg;
  u32 uOsdRightPosReg;
  u32 uWinConXRegVal;
  u32 uAlphaRegH;
  u32 uAlpha0RegL;
  u32 uAlpha1RegL;
  u32 uLcdEndX, uLcdEndY;

  uWinConXReg =
    (eWin == WIN0) ? WINCON0 :
    (eWin == WIN1) ? WINCON1 :
    (eWin == WIN2) ? WINCON2 :
    (eWin == WIN3) ? WINCON3 : WINCON4;

  uFrmBufStAddrReg =
    (eWin == WIN0) ? VIDW00ADD0B0 :
    (eWin == WIN1) ? VIDW01ADD0B0:
    (eWin == WIN2) ? VIDW02ADD0B0 :
    (eWin == WIN3) ? VIDW03ADD0B0 : VIDW04ADD0B0;

  uFrmBufEndAddrReg =
    (eWin == WIN0) ? VIDW00ADD1B0 :
    (eWin == WIN1) ? VIDW01ADD1B0 :
    (eWin == WIN2) ? VIDW02ADD1B0 :
    (eWin == WIN3) ? VIDW03ADD1B0 : VIDW04ADD1B0;

  uFrmDBufStAddrReg =
    (eWin == WIN0) ? VIDW00ADD0B1 :
    (eWin == WIN1) ? VIDW01ADD0B1:
    (eWin == WIN2) ? VIDW02ADD0B1 :
    (eWin == WIN3) ? VIDW03ADD0B1 : VIDW04ADD0B1;

  uFrmDBufEndAddrReg =
    (eWin == WIN0) ? VIDW00ADD1B1 :
    (eWin == WIN1) ? VIDW01ADD1B1 :
    (eWin == WIN2) ? VIDW02ADD1B1 :
    (eWin == WIN3) ? VIDW03ADD1B1 : VIDW04ADD1B1;

  uFrmTBufStAddrReg =
    (eWin == WIN0) ? VIDW00ADD0B2 :
    (eWin == WIN1) ? VIDW01ADD0B2:
    (eWin == WIN2) ? VIDW02ADD0B2 :
    (eWin == WIN3) ? VIDW03ADD0B2 : VIDW04ADD0B2;

  uFrmTBufEndAddrReg =
    (eWin == WIN0) ? VIDW00ADD1B2 :
    (eWin == WIN1) ? VIDW01ADD1B2 :
    (eWin == WIN2) ? VIDW02ADD1B2 :
    (eWin == WIN3) ? VIDW03ADD1B2 : VIDW04ADD1B2;

  uOffsetPageWidthReg =
    (eWin == WIN0) ? VIDW00ADD2 :
    (eWin == WIN1) ? VIDW01ADD2 :
    (eWin == WIN2) ? VIDW02ADD2 :
    (eWin == WIN3) ? VIDW03ADD2 : VIDW04ADD2;

  uOsdLeftPosReg =
    (eWin == WIN0) ? VIDOSD0A :
    (eWin == WIN1) ? VIDOSD1A :
    (eWin == WIN2) ? VIDOSD2A :
    (eWin == WIN3) ? VIDOSD3A : VIDOSD4A;

  uOsdRightPosReg =
    (eWin == WIN0) ? VIDOSD0B :
    (eWin == WIN1) ? VIDOSD1B :
    (eWin == WIN2) ? VIDOSD2B :
    (eWin == WIN3) ? VIDOSD3B : VIDOSD4B;

  uWinConXRegVal = 0;

  if (eWin == WIN0)
  {
    if (!oLcdc.m_bFifoIn[0]) // Input path is DMA
    {
      uWinConXRegVal =
        PATH_DMA | WBUF0 | WBUFAUTO_DISABLE | BITSWP_DISABLE |
        oLcdc.m_uBytSwpMode | oLcdc.m_uHawSwpMode|oLcdc.m_uWordSwpMode|
        oLcdc.m_uMaxBurstLen |oLcdc.m_uBppMode | ENWIN_F_DISABLE;

    }

    else // Input path is Local
    {
      if(oLcdc.m_uLocalInColorSpace[0] == LOCAL_IF_YCBCR444)
      {
        uWinConXRegVal =
          PATH_LOCAL | WBUF0 | WBUFAUTO_DISABLE | BITSWP_DISABLE | BYTSWP_DISABLE | 
          HAWSWP_DISABLE | WRDSWP_DISABLE |IN_LOCAL_YUV |
          oLcdc.m_uMaxBurstLen | oLcdc.m_uBppMode | ENWIN_F_DISABLE;
      }
      else
      {
        uWinConXRegVal =
          PATH_LOCAL | WBUF0 | WBUFAUTO_DISABLE | BITSWP_DISABLE | BYTSWP_DISABLE | 
          HAWSWP_DISABLE | WRDSWP_DISABLE |IN_LOCAL_RGB |
          oLcdc.m_uMaxBurstLen | oLcdc.m_uBppMode | ENWIN_F_DISABLE;
      }
    }
    LcdcOutp32(VIDOSD0C, OSDSIZE(oLcdc.m_uViewHSz[0]*oLcdc.m_uViewVSz[0]));

    // Set WIN0 alpha value
    LcdcOutp32(VIDW0ALPHA0, ALPHA0_R(0xFF) | ALPHA0_G(0xFF) | ALPHA0_B(0xFF));
    LcdcOutp32(VIDW0ALPHA1, ALPHA1_R(0xFF) | ALPHA1_G(0xFF) | ALPHA1_B(0xFF));
  }
  else  // eWin == WIN2 OR WIN3 OR WIN4
  {
    if (eWin == WIN2)
    {
      if (!oLcdc.m_bFifoIn[2]) // Input path is DMA
      {
        uWinConXRegVal =
          PATH_DMA | WBUF0 | WBUFAUTO_DISABLE | BITSWP_DISABLE |
          oLcdc.m_uBytSwpMode | oLcdc.m_uHawSwpMode|oLcdc.m_uWordSwpMode|
          oLcdc.m_uMaxBurstLen | BLD_PIX_PLANE | oLcdc.m_uBppMode |
          BLEND_ALPHA0_PLANE | ENWIN_F_DISABLE;
      }
      else // Input path is Local
      {
        Assert(oLcdc.m_eLocalIn[2] == IN_POST || oLcdc.m_eLocalIn[2] == IN_CIM);          

        if(oLcdc.m_uLocalInColorSpace[2] == LOCAL_IF_YCBCR444)
        {
          uWinConXRegVal = 
            PATH_LOCAL| WBUF0 | WBUFAUTO_DISABLE | BITSWP_DISABLE | BYTSWP_DISABLE |
            HAWSWP_DISABLE |WRDSWP_DISABLE | IN_LOCAL_YUV |
            oLcdc.m_uMaxBurstLen | BLD_PIX_PLANE | oLcdc.m_uBppMode |
            BLEND_ALPHA0_PLANE | ENWIN_F_DISABLE;
        }
        else
        {
          uWinConXRegVal = 
            PATH_LOCAL| WBUF0 | WBUFAUTO_DISABLE | BITSWP_DISABLE | BYTSWP_DISABLE |
            HAWSWP_DISABLE|WRDSWP_DISABLE| IN_LOCAL_RGB |
            oLcdc.m_uMaxBurstLen | BLD_PIX_PLANE | oLcdc.m_uBppMode |
            BLEND_ALPHA0_PLANE | ENWIN_F_DISABLE;   
        }
      }
      LcdcOutp32(VIDOSD2D, OSDSIZE(oLcdc.m_uViewHSz[2]*oLcdc.m_uViewVSz[2]));       
    }
    else
    {
      uWinConXRegVal = 
        PATH_DMA | WBUF0 | WBUFAUTO_DISABLE |         
        BITSWP_DISABLE | oLcdc.m_uBytSwpMode | oLcdc.m_uHawSwpMode |oLcdc.m_uWordSwpMode|
        oLcdc.m_uMaxBurstLen | BLD_PIX_PLANE | oLcdc.m_uBppMode | 
        BLEND_ALPHA0_PLANE | ENWIN_F_DISABLE;
    }
    // Set WIN1, WIN2, WIN3, WIN4 alpha Value
    uAlphaRegH =
      (eWin == WIN1) ? VIDOSD1C :
      (eWin == WIN2) ? VIDOSD2C :
      (eWin == WIN3) ? VIDOSD3C : VIDOSD4C;
    uAlpha0RegL = 
      (eWin == WIN1) ? VIDW1ALPHA0 :
      (eWin == WIN2) ? VIDW2ALPHA0 :
      (eWin == WIN3) ? VIDW3ALPHA0 : VIDW4ALPHA0;
    uAlpha1RegL = 
      (eWin == WIN1) ? VIDW1ALPHA1 :
      (eWin == WIN2) ? VIDW2ALPHA1 :
      (eWin == WIN3) ? VIDW3ALPHA1 : VIDW4ALPHA1;
    LcdcOutp32(uAlphaRegH,
        ALPHA0_R_H(0xF) | ALPHA0_G_H(0xF) | ALPHA0_B_H(0xF) |
        ALPHA1_R_H(0xF) | ALPHA1_G_H(0xF) | ALPHA1_B_H(0xF));
    LcdcOutp32(uAlpha0RegL, ALPHA0_R_L(0xF) | ALPHA0_G_L(0xF) | ALPHA0_B_L(0xF));
    LcdcOutp32(uAlpha1RegL, ALPHA1_R_L(0xF) | ALPHA1_G_L(0xF) | ALPHA1_B_L(0xF));
  }

  LcdcOutp32(uFrmBufStAddrReg, oLcdc.m_uFbStAddr[eWin]);
  LcdcOutp32(uFrmBufEndAddrReg, oLcdc.m_uFbEndAddr[eWin]);
  LcdcOutp32(uFrmDBufStAddrReg, oLcdc.m_uDoubleBufStAddr[eWin]);
  LcdcOutp32(uFrmDBufEndAddrReg, oLcdc.m_uDoubleBufEndAddr[eWin]);  
  LcdcOutp32(uFrmTBufStAddrReg, oLcdc.m_uTripleBufStAddr[eWin]);
  LcdcOutp32(uFrmTBufEndAddrReg, oLcdc.m_uTripleBufEndAddr[eWin]);
  LcdcOutp32(uOffsetPageWidthReg, (oLcdc.m_uOffsetSz[eWin]<<13) | oLcdc.m_uPageWidth[eWin]);

  uLcdEndX = oLcdc.m_uLcdStX + oLcdc.m_uViewHSz[eWin] - 1;
  uLcdEndY = oLcdc.m_uLcdStY + oLcdc.m_uViewVSz[eWin] - 1;

  if (oLcdc.m_uScanMode == 1) // Interlace mode
  {
    // In interlace mode, Left Top Y(oLcdc.m_uLcdStY) and Right Bottom Y(uLcdEndY) must be divided by 2.
    // And, Right Bottom Y must be rounded down
    LcdcOutp32(uOsdLeftPosReg, OSD_LTX_F(oLcdc.m_uLcdStX) | (OSD_LTY_F(oLcdc.m_uLcdStY)>>1));
    LcdcOutp32(uOsdRightPosReg, OSD_RBX_F(uLcdEndX) | (OSD_RBY_F(uLcdEndY)>>1));
  }
  else // Progressive mode
  {
    LcdcOutp32(uOsdLeftPosReg, OSD_LTX_F(oLcdc.m_uLcdStX) | OSD_LTY_F(oLcdc.m_uLcdStY));
    LcdcOutp32(uOsdRightPosReg, OSD_RBX_F(uLcdEndX) | OSD_RBY_F(uLcdEndY));
  }
}

// Window 0~4 Setting
void LCDC_SetAlpha(u32 uA0R, u32 uA0G, u32 uA0B, u32 uA1R, u32 uA1G, u32 uA1B, LCD_WINDOW eWin)
{
  u32 uWin0A0RVal;
  u32 uWin0A0GVal;
  u32 uWin0A0BVal;
  u32 uWin0A1RVal;
  u32 uWin0A1GVal;
  u32 uWin0A1BVal;
  u32 uA0RLVal, uA0RHVal;
  u32 uA0GLVal, uA0GHVal;
  u32 uA0BLVal, uA0BHVal;
  u32 uA1RLVal, uA1RHVal;
  u32 uA1GLVal, uA1GHVal;
  u32 uA1BLVal, uA1BHVal;
  u32 uAlphaRegH, uAlpha0RegL;
  u32 uAlpha1RegL; 

  // WIN0 alpha
  if(eWin == WIN0)
  {
    uWin0A0RVal = uA0R & 0xff;
    uWin0A0GVal = uA0G & 0xff;
    uWin0A0BVal = uA0B & 0xff;
    uWin0A1RVal = uA1R & 0xff;
    uWin0A1GVal = uA1G & 0xff;
    uWin0A1BVal = uA1B & 0xff;
    LcdcOutp32(VIDW0ALPHA0, ALPHA0_R(uWin0A0RVal) | ALPHA0_G(uWin0A0GVal) | ALPHA0_B(uWin0A0BVal));
    LcdcOutp32(VIDW0ALPHA1, ALPHA1_R(uWin0A1RVal) | ALPHA1_G(uWin0A1GVal) | ALPHA1_B(uWin0A1BVal));
  }
  // WIN1 or WIN2 or WIN3 or WIN4 alpha
  else  //((eWin==WIN1)||(eWin==WIN2)||(eWin==WIN3)||(eWin==WIN4))
  {
    uA0RLVal = uA0R & 0x0f;
    uA0RHVal = ((uA0R & 0xf0)>>4) & 0xf;    
    uA0GLVal = uA0G & 0x0f;
    uA0GHVal = ((uA0G & 0xf0)>>4) & 0xf;
    uA0BLVal = uA0B & 0x0f;
    uA0BHVal = ((uA0B & 0xf0)>>4) & 0xf;    
    uA1RLVal = uA1R & 0x0f;
    uA1RHVal = ((uA1R & 0xf0)>>4) & 0xf;    
    uA1GLVal = uA1G & 0x0f;
    uA1GHVal = ((uA1G & 0xf0)>>4) & 0xf;    
    uA1BLVal = uA1B & 0x0f;
    uA1BHVal = ((uA1B & 0xf0)>>4) & 0xf;

    uAlphaRegH =
      (eWin == WIN1) ? VIDOSD1C :
      (eWin == WIN2) ? VIDOSD2C :
      (eWin == WIN3) ? VIDOSD3C : VIDOSD4C;
    LcdcOutp32(uAlphaRegH,
        ALPHA0_R_H(uA0RHVal) | ALPHA0_G_H(uA0GHVal) | ALPHA0_B_H(uA0BHVal) |
        ALPHA1_R_H(uA1RHVal) | ALPHA1_G_H(uA1GHVal) | ALPHA1_B_H(uA1BHVal));

    uAlpha0RegL = 
      (eWin == WIN1) ? VIDW1ALPHA0 :
      (eWin == WIN2) ? VIDW2ALPHA0 :
      (eWin == WIN3) ? VIDW3ALPHA0 : VIDW4ALPHA0;
    LcdcOutp32(uAlpha0RegL, ALPHA0_R_L(uA0RLVal) | ALPHA0_G_L(uA0GLVal) | ALPHA0_B_L(uA0BLVal));

    uAlpha1RegL= 
      (eWin == WIN1) ? VIDW1ALPHA1 :
      (eWin == WIN2) ? VIDW2ALPHA1 :
      (eWin == WIN3) ? VIDW3ALPHA1 : VIDW4ALPHA1;
    LcdcOutp32(uAlpha1RegL, ALPHA1_R_L(uA1RLVal) | ALPHA1_G_L(uA1GLVal) | ALPHA1_B_L(uA1BLVal));
  }

}

void LCDC_SetAlphaValWidth(ALPHA_WIDTH eAlphaValWidth)
{
  if(eAlphaValWidth == ALPHA_4BIT)
  {
    LcdcOutp32(BLENDCON, 0x0);
  }
  else // (eAlphaValWidth == ALPHA_8BIT)
  {
    LcdcOutp32(BLENDCON, 0x1);
  }
}

void LCDC_SetWinOnOff(u32 uOnOff, LCD_WINDOW eWin)
{
  u32 uWinTemp;
  u32 uWinConReg;

  uWinConReg =
    (eWin == WIN0) ? WINCON0 :
    (eWin == WIN1) ? WINCON1 :
    (eWin == WIN2) ? WINCON2 :
    (eWin == WIN3) ? WINCON3 : WINCON4;

  LcdcInp32(uWinConReg, uWinTemp);

  if (uOnOff) // Turn OSD on
  {
    uWinTemp |= ENWIN_F_ENABLE;
  }
  else // Turn OSD off
  {
    uWinTemp &= ~ENWIN_F_ENABLE;
  }

  LcdcOutp32(uWinConReg, uWinTemp);

}

void LCDC_SetAllWinOnOff(u32 uOnOff)
{
  u32 uWinTemp[5];


  LcdcInp32(WINCON0, uWinTemp[0]);
  LcdcInp32(WINCON1, uWinTemp[1]);
  LcdcInp32(WINCON2, uWinTemp[2]);
  LcdcInp32(WINCON3, uWinTemp[3]);
  LcdcInp32(WINCON4, uWinTemp[4]);


  if (uOnOff)
  {
    LcdcOutp32(WINCON0, uWinTemp[0]|ENWIN_F_ENABLE);
    LcdcOutp32(WINCON1, uWinTemp[1]|ENWIN_F_ENABLE);
    LcdcOutp32(WINCON2, uWinTemp[2]|ENWIN_F_ENABLE);
    LcdcOutp32(WINCON3, uWinTemp[3]|ENWIN_F_ENABLE);
    LcdcOutp32(WINCON4, uWinTemp[4]|ENWIN_F_ENABLE);
  }
  else
  {
    LcdcOutp32(WINCON0, uWinTemp[0]&~ENWIN_F_ENABLE);
    LcdcOutp32(WINCON1, uWinTemp[1]&~ENWIN_F_ENABLE);
    LcdcOutp32(WINCON2, uWinTemp[2]&~ENWIN_F_ENABLE);
    LcdcOutp32(WINCON3, uWinTemp[3]&~ENWIN_F_ENABLE);
    LcdcOutp32(WINCON4, uWinTemp[4]&~ENWIN_F_ENABLE);
  }
}

void LCDC_SetAllWinColorMapOnOff(u32 uOnOff)
{
  u32 uColorMapVal;
  u32 uColorMapReg;
  u32 i;

  for (i=0; i<5; i++)
  {
    uColorMapReg =
      (i == 0) ? WIN0MAP :
      (i == 1) ? WIN1MAP :
      (i == 2) ? WIN2MAP :
      (i == 3) ? WIN3MAP : WIN4MAP;

    LcdcInp32(uColorMapReg, uColorMapVal);

    if (uOnOff)
    {
      LcdcOutp32(uColorMapReg, COLOR_MAP_ENABLE|uColorMapVal);
    }
    else
    {
      LcdcOutp32(uColorMapReg, ~COLOR_MAP_ENABLE&uColorMapVal);
    }
  }
}

void LCDC_SetAllWinColorKeyOnOff(u32 uOnOff)
{
  u32 uColorKeyReg, uColorKeyVal;
  u32 i;

  for (i=1; i<5; i++)
  {
    uColorKeyReg =
      (i == 1) ? W1KEYCON0 :
      (i == 2) ? W2KEYCON0 :
      (i == 3) ? W3KEYCON0 : W4KEYCON0;

    LcdcInp32(uColorKeyReg, uColorKeyVal);

    if (uOnOff)
    {
      LcdcOutp32(uColorKeyReg, uColorKeyVal | KEYEN_F_ENABLE);
    }
    else
    {
      LcdcOutp32(uColorKeyReg, uColorKeyVal & KEYBLEN_F_DISABLE & KEYEN_F_DISABLE);
    }
  }

}

void LCDC_GetClkValAndClkDir(u32 uLcdHSz, u32 uLcdVSz, u32* uClkVal, u32* uClkDir)
{
  double dTmpVal;
  u32 uTmpClkVal;
  u32 uTotalTime;
  u32 uLCD_VCLK;
  double dFrmRate;
  u32 uClkDiv,uClkSelReg;
  u32 uFrameRate = LCD_FRM_RATE;

  uLcdHSz+= gMODULE_HBPD +gMODULE_HFPD +gMODULE_HSPW +3;
  uLcdVSz+= gMODULE_VBPD +gMODULE_VFPD +gMODULE_VSPW +3;

  if (gLCDM_INTERFACE_TYPE == I80_CPU) 
    uTotalTime = WR_ACT_TIME + WR_HOLD_TIME;
  else
    uTotalTime = 1;

  if(LCD_VCLKSRC == LCD_HCLK)
  {
    uClkSelReg = (u32)LCD_eHCLK ;
    uClkDiv = 1;
  }
  else
  {

    uClkDiv = SYSC_GetClkDiv(eCLKDIV_FIMD);
    uClkSelReg = SYSC_GetClkMuxReg(1);
    uClkSelReg = (uClkSelReg >>20) & 0xf ;
    printf("LCDC_GetClkValAndClkDir()==> uClkDiv=%d, uClkSelReg=%x, uClkSelReg=%x\n", uClkDiv, uClkSelReg, uClkSelReg);
  }

  uLCD_VCLK =
    (uClkSelReg == (u32)LCD_eHCLK) ?    g_uHclkDsys :
    (uClkSelReg == (u32)LCD_eXXTI) ?  FIN / uClkDiv :
    (uClkSelReg == (u32)LCD_eXusbXTI) ?   FIN / uClkDiv :
    (uClkSelReg == (u32)LCD_eMPLL) ?  g_uMPLL / uClkDiv :
    (uClkSelReg == (u32)LCD_eEPLL) ?  g_uEPLL  / uClkDiv  :
#if 0
    (uClkSelReg == (u32)LCD_eVPLL) ?  g_uVPLL  / uClkDiv  : Assert(0);
#else
    (uClkSelReg == (u32)LCD_eVPLL) ?  g_uVPLL  / uClkDiv  : 0;
#endif

    switch(uClkSelReg)
    {
      case LCD_eHCLK: printf("LCD VCLK Source is HCLK "); break;
      case LCD_eXXTI: printf("LCD VCLK Source is XXIT "); break;
      case LCD_eXusbXTI: printf("LCD VCLK Source is XusbXTI "); break;
      case LCD_eMPLL: printf("LCD VCLK Source is MPLL "); break;
      case LCD_eEPLL: printf("LCD VCLK Source is EPLL "); break;
      case LCD_eVPLL: printf("LCD VCLK Source is VPLL "); break;
      default:printf("??? ");
    }

    printf("LCDC_GetClkValAndClkDir()==> g_uHclkDsys=%d uLCD_VCLK=%d\n", g_uHclkDsys, uLCD_VCLK);
    printf("with %3.3fMHz\n",(double)uLCD_VCLK/1000000);
    printf("Output VCLK need to be %3.3fMHz\n",(double)(uTotalTime*uLcdHSz*uLcdVSz*uFrameRate)/1000000);


    dTmpVal = (double)(uLCD_VCLK)/(uTotalTime*uLcdHSz*uLcdVSz*uFrameRate) - 1;

    *uClkVal = (int)(dTmpVal+0.5);
    *uClkDir = (*uClkVal<1) ? 0 : 1;
    uTmpClkVal = *uClkVal;

    dFrmRate = (double)((u32)(uLCD_VCLK))/((uTmpClkVal+1)*uLcdHSz*uLcdVSz);
    printf("Frame Rate: %f\t ClkVal: %d\n", dFrmRate, (int)dTmpVal);
}

void LCDC_WTrigger(LCD_WINDOW eWin)
{
  u32 uTrigCtrl;

  LcdcInp32(TRIGCON, uTrigCtrl);

  if (oLcdc.m_bAutoBuf != LCDC_BUF_SINGLE)
  {
    switch(eWin)
    {
      case WIN0 :
        LcdcOutp32(TRIGCON, uTrigCtrl | W0_SW_TRIG_MODE | W0_SW_TRIGGER);       
        break;

      case WIN1 :
        LcdcOutp32(TRIGCON, uTrigCtrl | W1_SW_TRIG_MODE | W1_SW_TRIGGER);       
        break;

      case WIN2 :
        LcdcOutp32(TRIGCON, uTrigCtrl | W2_SW_TRIG_MODE | W2_SW_TRIGGER);       
        break;

      case WIN3 :
        LcdcOutp32(TRIGCON, uTrigCtrl | W3_SW_TRIG_MODE | W3_SW_TRIGGER);       
        break;

      case WIN4 :
        LcdcOutp32(TRIGCON, uTrigCtrl | W4_SW_TRIG_MODE | W4_SW_TRIGGER);       
        break;

    }

  }

  else
  {
#if ((LCD_MODEL == LCD_LTS222Q_CPU) ||  (LCD_MODEL == LCD_MIPIModule) || (LCD_MODEL == LCD_UF24U276A))
    LcdcOutp32(TRIGCON, uTrigCtrl | I80_SW_TRIG_MODE | I80_SW_TRIGGER);
#endif
  }

}

void LCDC_SetBlendingEquation(u32 uBackAlphaValFactor, u32 uForeAlphaValFactor, u32 uBackDataFactor, u32 uForeDataFactor, LCD_WINDOW eWin)
{
  u32 uBlendEqReg =
    (eWin == WIN1) ? BLENDEQ1 :
    (eWin == WIN2) ? BLENDEQ2 :
    (eWin == WIN3) ? BLENDEQ3 :
    (eWin == WIN4) ? BLENDEQ4 :
#if 0
    Assert(0);
#else
    0;
#endif

  LcdcOutp32(uBlendEqReg, Q_FUNC(uBackAlphaValFactor)|P_FUNC(uForeAlphaValFactor)|
      B_FUNC(uBackDataFactor)|A_FUNC(uForeDataFactor));

}

void LCDC_InitLdi(CPUIF_LDI eLdiMode)
{
  LCDM_InitLdi(LCD_MODEL,eLdiMode);
}

void LCDC_GetFrmSz(u32* uFrmHSz, u32* uFrmVSz, LCD_WINDOW eWin)
{
  *uFrmHSz = oLcdc.m_uFrmHSz[eWin];
  *uFrmVSz = oLcdc.m_uFrmVSz[eWin];
}

void LCDC_GetFbEndAddr(u32* uFbAddr, LCD_WINDOW eWin) 
{ 
  *uFbAddr = oLcdc.m_uFbEndAddr[eWin]; 
}
