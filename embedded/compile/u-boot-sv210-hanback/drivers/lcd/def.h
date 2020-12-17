#ifndef __DEF_H__
#define __DEF_H__

typedef unsigned char   bool;

#define false 0
#define true 1

#if 0
typedef enum
{
        PAL1, PAL2, PAL4, PAL8,
        RGB8, ARGB8, RGB16, ARGB16, RGB18, RGB24, RGB30, ARGB24,
        YC420, YC422, // Non-interleave
        CRYCBY, CBYCRY, YCRYCB, YCBYCR, YUV444 // Interleave
} CSPACE;
#endif

typedef enum
{
  //FIMD
  RGB565,
  RGB666,
  RGB888,
  ARGB8888,         // 32BPP Display
  PRE_ARGB8888,
  ARGB1888,         // 25BPP Display
  ARGB1887,         // 24BPP Display
  ARGB1666,         // 19BPP Display
  ARGB1555,         // 16BPP Display
  ARGB4444,

  RGB1555,          // 16BPP Display
  ARGB1444,         // 13BPP Display
  ARGB1232,         // 8BPP Display
  PAL8,           // Palette 8BPP
  PAL4,           // Palette 4BPP
  PAL2,           // Palette 2BPP
  PAL1,

  //FIMC
  YCBCR420_2P_CRCB_LE,    // 2Plane, CRCB Little Endian  (Cr3Cb3Cr2Cb2Cr1Cb1Cr0Cb0)
  YCBCR420_2P_CBCR_LE,    // 2Plane, CBCR Little Endian (Cb3Cr3Cb2Cr2Cb1Cr1Cb0Cr0)
  YCBCR420_2P_CRCB_BE,    // 2Plane, CRCB Big Endian  (Cr0Cb0Cr1Cb1Cb2Cr2Cb3Cr3)
  YCBCR420_2P_CBCR_BE,    // 2Plane, CBCR Big Endian (Cb0Cr0Cb1Cr1Cb2Cr2Cb3Cr3)
  YCBCR420_3P,        // 3Plane

  YCBCR422_1P_YCBYCR,   // 1Plane,  Data Stored Order (Y3Cb1Y2Cr1Y1Cb0Y0Cr0)
  YCBCR422_1P_CBYCRY,   // 1Plane,  Data Stored Order (Cb1Y3Cr1Y2Cb0Y1Cr0Y0)
  YCBCR422_1P_YCRYCB,   // 1Plane,  Data Stored Order (Y3Cr1Y2Cb1Y1Cr0Y0Cb0)
  YCBCR422_1P_CRYCBY,   // 1Plane,  Data Stored Order (Cr1Y3Cb1Y2Cr0Y1Cb0Y0)
  YCBCR422_2P_CRCB_LE,    // 2Plane, CRCB Little Endian  (Cr3Cb3Cr2Cb2Cr1Cb1Cr0Cb0) 
  YCBCR422_2P_CBCR_LE,    // 2Plane, CBCR Little Endian (Cb3Cr3Cb2Cr2Cb1Cr1Cb0Cr0)
  YCBCR422_2P_CRCB_BE,    // 2Plane, CRCB Big Endian  (Cr0Cb0Cr1Cb1Cb2Cr2Cb3Cr3)
  YCBCR422_2P_CBCR_BE,    // 2Plane, CBCR Big Endian (Cb0Cr0Cb1Cr1Cb2Cr2Cb3Cr3)
  YCBCR422_3P,        // 3Plane

  YCBCR444_2P_CRCB_LE,    // 2Plane, CRCB Little Endian  (Cr3Cb3Cr2Cb2Cr1Cb1Cr0Cb0) 
  YCBCR444_2P_CBCR_LE,    // 2Plane, CBCR Little Endian (Cb3Cr3Cb2Cr2Cb1Cr1Cb0Cr0)
  YCBCR444_2P_CRCB_BE,    // 2Plane, CRCB Big Endian  (Cr0Cb0Cr1Cb1Cb2Cr2Cb3Cr3)
  YCBCR444_2P_CBCR_BE,    // 2Plane, CBCR Big Endian (Cb0Cr0Cb1Cr1Cb2Cr2Cb3Cr3)
  YCBCR444_3P,        // 3Plane

  EXT_RGB565,       // Extension RGB565 by FIMC(Input DMA Path) R&B[2:0] <= R&B[7:5], G[1:0] <= G[7:6]
  EXT_RGB666,       // Extension RGB666 by FIMC(Input DMA Path) R&G&B[1:0] <= R&G&B[7:6]

  BayerRGB8,          // Mipi CSI Raw RGB Data
  BayerRGB10,         // Mipi CSI Raw RGB Data
  BayerRGB12,         // Mipi CSI Raw RGB Data

  // Definition of the Tile Image & Interlaced Image
  YCBCR420_2P_CRCB_LE_IB,   // Interlaced Bob Image ( for VP Little Endian Type )
  YCBCR420_2P_CRCB_LE_IW,   // Interlaced Weave ( for VP  Little Endian Type)
  TILE_YCBCR420_2P_CRCB_LE,   // 64byte X 32line Tile Image (for VP Little Endian Type , MFC Decoder )
  TILE_YCBCR420_2P_CRCB_LE_IB,  // 64byte X 32line Tile Image & Interlaced Bob Image (for VP Little Endian Type)
  TILE_YCBCR420_2P_CRCB_LE_IW,  // 64byte X 32line Tile Image & Interlaced Weave Image (for VP Little Endian Type)  
  TILE16_YCBCR420_2P_CRCB_LE_IB,  // 16byte X 16line Tile Image ( for MFC Encoder )

  //The Definition of the previous devices
  NV12_VP_BE,       // Only Used for VP,  (Y0Y1Y2Y3Y4Y5Y6Y7)(Cb0Cr0Cb1Cr1Cb2Cr2Cb3Cr3)
  NV12_IB_VP_BE,        // 
  NV12_IW_VP_BE,        //
  TILE_NV12_VP_BE,
  TILE_NV12_IB_VP_BE,
  TILE_NV12_IW_VP_BE,

  RGB8,
  ARGB8 = ARGB1232,

  RGB16_IB,

  RGB16_IW,
  RGB24_IB,
  RGB24_IW,


  RGB16 = RGB565,
  RGB18 = RGB666,
  RGB24 = RGB888, 
  ARGB16 = ARGB1555,
  ARGB18 = ARGB1666,
  ARGB24 = ARGB1887,
  ARGB25 = ARGB1888,

  CRYCBY  = YCBCR422_1P_CRYCBY,
  CBYCRY  = YCBCR422_1P_CBYCRY,
  YCRYCB  = YCBCR422_1P_YCRYCB,
  YCBYCR  = YCBCR422_1P_YCBYCR,
  NV12  = YCBCR420_2P_CRCB_LE,  // VP Little Endian Type
  NV16  = YCBCR422_2P_CRCB_LE,
  NV24  = YCBCR444_2P_CRCB_LE,
  YC420 = YCBCR420_3P,
  YC422 = YCBCR422_3P,
  YC444 = YCBCR444_3P,

  NV12_IB   = YCBCR420_2P_CRCB_LE_IB, // Interlaced Bob Image
  NV12_IW = YCBCR420_2P_CRCB_LE_IW, // Interlaced Weave Image
  TILE_NV12 = TILE_YCBCR420_2P_CRCB_LE,
  TILE_NV12_IB  = TILE_YCBCR420_2P_CRCB_LE_IB,
  TILE_NV12_IW  = TILE_YCBCR420_2P_CRCB_LE_IW,
  TILE16_NV12 = TILE16_YCBCR420_2P_CRCB_LE_IB,
  ARGB1665
} IMG_FMT;

#define QCIF_HSIZE    176
#define QCIF_VSIZE    144
#define CIF_HSIZE   352
#define CIF_VSIZE   288
#define QVGA_HSIZE    320
#define QVGA_VSIZE    240
#define QVGA_PORT_HSIZE 240
#define QVGA_PORT_VSIZE 320
#define VGA_HSIZE   640
#define VGA_VSIZE   480
#define WVGA_HSIZE  800
#define WVGA_VSIZE  480
#define WVGA_PORT_HSIZE 480
#define WVGA_PORT_VSIZE 800
#define SVGA_HSIZE    800
#define SVGA_VSIZE    600
#define WSVGA_HSIZE 1024
#define WSVGA_VSIZE 600
#define XGA_HSIZE   1024
#define XGA_VSIZE   768
#define SXGA_HSIZE    1280
#define SXGA_VSIZE    1024
#define UXGA_HSIZE    1600
#define UXGA_VSIZE    1200
#define WUXGA_HSIZE 1920
#define WUXGA_VSIZE 1200
#define QXGA_HSIZE    2048
#define QXGA_VSIZE    1536
#define SD480_HSIZE 720
#define SD480_VSIZE 480
#define SD576_HSIZE 720
#define SD576_VSIZE 576
#define HD720_HSIZE 1280
#define HD720_VSIZE 720
#define HD1080_HSIZE  1920
#define HD1080_VSIZE  1080
#define USER_DEFINE_SIZE_HSIZE  1920
#define USER_DEFINE_SIZE_VSIZE  1088

#define Assert(a) {if (!(a)) {printf("\n %s(line %d)\n", __FILE__, __LINE__); return -1;}}

#define Outp32(addr, data)	(*(volatile u32 *)(addr) = (data))
#define Outp16(addr, data)	(*(volatile u16 *)(addr) = (data))
#define Outp8(addr, data)	  (*(volatile u8 *)(addr) = (data))
#define Inp32(addr)		(*(volatile u32 *)(addr))
#define Inp16(addr)		(*(volatile u16 *)(addr))
#define Inp8(addr)		(*(volatile u8 *)(addr))

#define DRAM_BASE_PA_START      (0x20000000)
#define FRAMEBUFFER_OFFSET      (0x07F00000)
#define FRAMEBUFFER_PA_START    (DRAM_BASE_PA_START + FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_SIZE  (0x00100000)

#define LCD_FRAMEBUFFER_ST      FRAMEBUFFER_PA_START

#endif
