/*
 * Driver for MT9T111 (UXGA camera) from Samsung Electronics
 * 
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * Copyright (C) 2009, Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>
#include <media/s5k4ba_platform.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "s5k4ba.h"
#include "mt9t111.h"

#define MT9T111_DRIVER_NAME	"MT9T111"

/* Default resolution & pixelformat. plz ref s5k4ba_platform.h */
#define DEFAULT_RES		WVGA	/* Index of resoultion */
#define DEFAUT_FPS_INDEX	MT9T111_15FPS
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10 
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1280 (H) x 1024 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 * Max. pixel clock frequency : 48MHz(upto)
 * Internal PLL (6MHz to 27MHz input frequency)
 */

/* Camera functional setting values configured by user concept */
struct mt9t111_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct mt9t111_state {
	struct s5k4ba_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct mt9t111_userset userset;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
};

static inline struct mt9t111_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct mt9t111_state, sd);
}


#if 1 // mt9t111
static int reg_page_map_set(struct i2c_client *client, const u16 reg)
{
  int ret;
  u16 page;
  static int lastpage = -1; /* PageMap cache value */

  page = (reg >> 8);
  if (page == lastpage)
    return 0;
  if (page > 2)
    return -EINVAL;

  ret = i2c_smbus_write_word_data(client, MT9M111_PAGE_MAP, swab16(page));
  if (!ret)
    lastpage = page;
  return ret;
}

static int mt9t111_reg_read(struct i2c_client *client, u16 command)
{
  struct i2c_msg msg[2];
  u8 buf[2];
  int ret;

  command = swab16(command);

  msg[0].addr  = client->addr;
  msg[0].flags = 0;
  msg[0].len   = 2;
  msg[0].buf   = (u8 *)&command;

  msg[1].addr  = client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len   = 2;
  msg[1].buf   = buf;

  /*
   * if return value of this function is < 0,
   * it mean error.
   * else, under 16bit is valid data.
   */
  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret < 0)
    return ret;

  memcpy(&ret, buf, 2);
  mdelay(1);          // raoudi-20100901
  return swab16(ret);
}

static int mt9t111_reg_write(const struct i2c_client *client, u16 command, u16 data)
{
  struct i2c_msg msg;
  u8 buf[4];
  int ret;

  command = swab16(command);
  data = swab16(data);

  memcpy(buf + 0, &command, 2);
  memcpy(buf + 2, &data,    2);

  msg.addr  = client->addr;
  msg.flags = 0;
  msg.len   = 4;
  msg.buf   = buf;

  /*
   * i2c_transfer return message length,
   * but this function should return 0 if correct case
   * */
  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret >= 0)
    ret = 0;

  mdelay(1);        //raoudi-20100901
  return ret;
}

static int mt9t111_reg_mask_set(const struct i2c_client *client, u16  command, u16  mask, u16  set)
{
  int val = mt9t111_reg_read(client, command);
  if (val < 0)
    return val;

  val &= ~mask;
  val |= set & mask;

  return mt9t111_reg_write(client, command, val);
}

static int mt9m111_reg_set(struct i2c_client *client, const u16 reg, const u16 data)
{
  int ret;

  ret = mt9t111_reg_read(client, reg);
  if (ret >= 0)
    ret = mt9t111_reg_write(client, reg, ret | data);
  return ret;
}

static int mt9m111_reg_clear(struct i2c_client *client, const u16 reg, const u16 data)
{
  int ret;

  ret = mt9t111_reg_read(client, reg);
  return mt9t111_reg_write(client, reg, ret & ~data);
}

static int mt9m111_set_regs(struct i2c_client *sam_client, const struct mt9m111_reg *p_reg)
{
  u32 curReg = 0;
  u16 regVal;
  int status = 0;
  int curPage = -1;
  u8 data[2];
  /* The list is a register number followed by the value */
  while( p_reg[curReg].reg != MT9M111_TERM )
  {
    if (p_reg[curReg].reg == MT9M111_DELAY)
    {
      mdelay(p_reg[curReg].value);
      curReg++;
      continue;
    }
    else
    {
      mt9t111_reg_write(sam_client, p_reg[curReg].reg, p_reg[curReg].value);
    }
    curReg++;
  }
}

static void mt9m111_dump_registers(struct i2c_client *sam_client)
{
  u8 address, value[2] = {0x00, 0x00};
  s32 data;

  printk("Dumping the mt9m111 register state\n");

  printk("Dumping the mt9m111 sensor core registers");
  value[1] = MT9M111_SENSOR_CORE;
  mt9t111_reg_write(sam_client, MT9M111_PAGE_MAP, value);
  for (address = 0; address < 0xff; address++) {
    data = mt9t111_reg_read(sam_client, address);
    printk("register 0x%x contains 0x%x\n", address, data);
  }

  printk("Dumping the mt9m111 color pipeline registers");
  value[1] = MT9M111_COLORPIPE;
  mt9t111_reg_write(sam_client, MT9M111_PAGE_MAP, value);
  for (address = 0; address < 0xff; address++) {
    data = mt9t111_reg_read(sam_client, address);
    printk("register 0x%x contains 0x%x\n", address, data);
  }

  printk("Dumping the mt9m111 camera control registers");
  value[1] = MT9M111_CAMERA_CONTROL;
  mt9t111_reg_write(sam_client, MT9M111_PAGE_MAP, value);
  for (address = 0; address < 0xff; address++) {
    data = mt9t111_reg_read(sam_client, address);
    printk("register 0x%x contains 0x%x\n", address, data);
  }

  printk("mt9m111 register state dump complete\n");
}

static int mt9t111_camera_probe(struct i2c_client *client)
{
  int chipid;

  /*
   * check and show chip ID
   */
  chipid = mt9t111_reg_read(client, 0x0000);

  if (chipid < 0)
    return -EIO;

  switch (chipid) {
    case 0x2680:
      printk("MT9T111 Device Found\n");
      break;
    case 0x2682:
      printk("MT9T112 Device Found\n");
      break;
    default:
      return -ENODEV;
  }


  return 0;
}

static int mt9t111_init_setting(const struct i2c_client *client)
{
  int data, i, ret;
  mt9t111_reg_write(client, 0x001A, 0x0019);  // RESET_AND_MISC_CONTROL
  //msleep(1);
  mt9t111_reg_write(client, 0x001A, 0x0018);  // RESET_AND_MISC_CONTROL

  mt9t111_reg_write(client, 0x0014, 0x2427);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0014, 0x2423);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0014, 0x2145);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0010, 0x0555);  // PLL_DIVIDERS
  mt9t111_reg_write(client, 0x0012, 0x0070);  // PLL_DIVIDERS
  mt9t111_reg_write(client, 0x002A, 0x77CC);  // PLL_P4_P5_P6_DIVIDERS
  mt9t111_reg_write(client, 0x001A, 0x0018);  // RESET_AND_MISC_CONTROL

  mt9t111_reg_write(client, 0x0014, 0x2545);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0014, 0x2547);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0014, 0x3447);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0014, 0x3047);  // PLL_CONTROL
  //mdelay(10);

  mt9t111_reg_write(client, 0x0014, 0x3046);  // PLL_CONTROL
  mt9t111_reg_write(client, 0x0022, 0x01E0);  // VDD_DIS_COUNTER
  mt9t111_reg_write(client, 0x3B84, 0x020B);  // I2C_MASTER_FREQUENCY_DIVIDER
  mt9t111_reg_write(client, 0x002E, 0x0500);  // SENSOR_CLOCK_DIVIDER
  mt9t111_reg_write(client, 0x0016, 0x0400);  // CLOCKS_CONTROL
  mt9t111_reg_write(client, 0x0018, 0x402D);  // STANDBY_CONTROL_AND_STATUS
  mt9t111_reg_write(client, 0x0018, 0x402C);  // STANDBY_CONTROL_AND_STATUS

#if 1
  /* poll to verify out of standby. Must Poll this bit */
  for (i = 0; i < 100; i++) {
    data = mt9t111_reg_read(client, 0x0018);
    if (0x4000 & data)
      break;

    mdelay(10);
  }
#endif

  mt9t111_reg_write(client, 0x0018, 0x0028);  // STANDBY_CONTROL_AND_STATUS
  mt9t111_reg_write(client, 0x001A, 0x0218);  // RESET_AND_MISC_CONTROL
  mt9t111_reg_write(client, 0x001A, 0x0218);  // RESET_AND_MISC_CONTROL
  mt9t111_reg_write(client, 0x001E, 0x0704);  // PAD_SLEW_PAD_CONFIG
  mt9t111_reg_write(client, 0x3084, 0x2409);  // DAC_LD_4_5
  mt9t111_reg_write(client, 0x3092, 0x0A49);  // DAC_LD_18_19
  mt9t111_reg_write(client, 0x3094, 0x4949);  // DAC_LD_20_21
  mt9t111_reg_write(client, 0x3096, 0x4950);  // DAC_LD_22_23
  mt9t111_reg_write(client, 0x0016, 0x02DF);  // CLOCKS_CONTROL
  mt9t111_reg_write(client, 0x098E, 0x68A0);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x082E);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6CA0);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x082E);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x3C20, 0x0000);  // TX_SS_CONTROL
  mt9t111_reg_write(client, 0x316C, 0x0406);  // DAC_TXLO
  mt9t111_reg_write(client, 0x0614, 0x0000);  // SECOND_SCL_SDA_PD
  mt9t111_reg_write(client, 0x098E, 0x6800);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0400);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6802);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0300);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE88E);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x68A0);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x082E);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4802);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4804);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4806);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x060D);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4808);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x080D);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x480A);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0111);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x480C);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x046F);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x480F);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00CC);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4811);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0381);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4813);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x024F);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x481D);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x035B);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x481F);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x05D0);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4825);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0ACF);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C00);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0800);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C02);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0600);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xEC8E);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6CA0);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x082E);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x484A);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0004);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x484C);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0004);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x484E);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x060B);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4850);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x080B);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4852);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0111);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4854);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0027);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4857);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x008C);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4859);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x01F1);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x485B);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00FF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4865);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0664);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4867);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0378);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x486D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0ACF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8A5);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0025);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8A6);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0028);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8A7);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x002D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8A8);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0030);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC844);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC92F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC845);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x009E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC92D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC88C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC930);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC88D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x009E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC92E);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA002);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0010);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA009);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0002);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA00A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0003);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA00C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4846);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0014);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x68AA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0218);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6815);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6817);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0010);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x488E);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0014);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6CAA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x01A2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x364A, 0x0010);   // P_R_P0Q0 <-0x0110(10/20)                   
  mt9t111_reg_write(client, 0x364C, 0xBAED);   // P_R_P0Q1                                   
  mt9t111_reg_write(client, 0x364E, 0x5BD1);   // P_R_P0Q2                                   
  mt9t111_reg_write(client, 0x3650, 0x760C);   // P_R_P0Q3                                   
  mt9t111_reg_write(client, 0x3652, 0x8C91);   // P_R_P0Q4                                   
  mt9t111_reg_write(client, 0x368A, 0xBA09);   // P_R_P1Q0                                   
  mt9t111_reg_write(client, 0x368C, 0x432F);   // P_R_P1Q1                                   
  mt9t111_reg_write(client, 0x368E, 0x31CF);   // P_R_P1Q2                                   
  mt9t111_reg_write(client, 0x3690, 0x87B1);   // P_R_P1Q3                                   
  mt9t111_reg_write(client, 0x3692, 0xE5CE);   // P_R_P1Q4                                   
  mt9t111_reg_write(client, 0x36CA, 0x2312);   // P_R_P2Q0                                   
  mt9t111_reg_write(client, 0x36CC, 0x32AB);   // P_R_P2Q1                                   
  mt9t111_reg_write(client, 0x36CE, 0x3693);   // P_R_P2Q2                                   
  mt9t111_reg_write(client, 0x36D0, 0x7F11);   // P_R_P2Q3                                   
  mt9t111_reg_write(client, 0x36D2, 0x9F16);   // P_R_P2Q4                                   
  mt9t111_reg_write(client, 0x370A, 0xF86C);   // P_R_P3Q0                                   
  mt9t111_reg_write(client, 0x370C, 0x06F0);   // P_R_P3Q1                                   
  mt9t111_reg_write(client, 0x370E, 0xCE4F);   // P_R_P3Q2                                   
  mt9t111_reg_write(client, 0x3710, 0xA653);   // P_R_P3Q3                                   
  mt9t111_reg_write(client, 0x3712, 0xD86D);   // P_R_P3Q4                                   
  mt9t111_reg_write(client, 0x374A, 0x7AF1);   // P_R_P4Q0                                   
  mt9t111_reg_write(client, 0x374C, 0x0273);   // P_R_P4Q1                                   
  mt9t111_reg_write(client, 0x374E, 0xB117);   // P_R_P4Q2                                   
  mt9t111_reg_write(client, 0x3750, 0x9096);   // P_R_P4Q3                                   
  mt9t111_reg_write(client, 0x3752, 0x3119);   // P_R_P4Q4                                   
  mt9t111_reg_write(client, 0x3640, 0x0550);   // P_G1_P0Q0                                  
  mt9t111_reg_write(client, 0x3642, 0x95AC);   // P_G1_P0Q1                                  
  mt9t111_reg_write(client, 0x3644, 0x24D1);   // P_G1_P0Q2                                  
  mt9t111_reg_write(client, 0x3646, 0xBF4B);   // P_G1_P0Q3                                  
  mt9t111_reg_write(client, 0x3648, 0x93D1);   // P_G1_P0Q4                                  
  mt9t111_reg_write(client, 0x3680, 0x04A9);   // P_G1_P1Q0                                  
  mt9t111_reg_write(client, 0x3682, 0xAEAF);   // P_G1_P1Q1                                  
  mt9t111_reg_write(client, 0x3684, 0x11AC);   // P_G1_P1Q2                                  
  mt9t111_reg_write(client, 0x3686, 0x2DB0);   // P_G1_P1Q3                                  
  mt9t111_reg_write(client, 0x3688, 0x4F4E);   // P_G1_P1Q4                                  
  mt9t111_reg_write(client, 0x36C0, 0x0D72);   // P_G1_P2Q0                                  
  mt9t111_reg_write(client, 0x36C2, 0x17EF);   // P_G1_P2Q1                                  
  mt9t111_reg_write(client, 0x36C4, 0x1651);   // P_G1_P2Q2                                  
  mt9t111_reg_write(client, 0x36C6, 0x9490);   // P_G1_P2Q3                                  
  mt9t111_reg_write(client, 0x36C8, 0x8535);   // P_G1_P2Q4                                  
  mt9t111_reg_write(client, 0x3700, 0xA84C);   // P_G1_P3Q0                                  
  mt9t111_reg_write(client, 0x3702, 0xF3F0);   // P_G1_P3Q1                                  
  mt9t111_reg_write(client, 0x3704, 0xAA50);   // P_G1_P3Q2                                  
  mt9t111_reg_write(client, 0x3706, 0x28D3);   // P_G1_P3Q3                                  
  mt9t111_reg_write(client, 0x3708, 0x2FF2);   // P_G1_P3Q4                                  
  mt9t111_reg_write(client, 0x3740, 0x46EF);   // P_G1_P4Q0                                  
  mt9t111_reg_write(client, 0x3742, 0x53D0);   // P_G1_P4Q1                                  
  mt9t111_reg_write(client, 0x3744, 0xC696);   // P_G1_P4Q2                                  
  mt9t111_reg_write(client, 0x3746, 0xFB54);   // P_G1_P4Q3                                  
  mt9t111_reg_write(client, 0x3748, 0x45F8);   // P_G1_P4Q4                                  
  mt9t111_reg_write(client, 0x3654, 0x00F0);   // P_B_P0Q0                                   
  mt9t111_reg_write(client, 0x3656, 0xAE4C);   // P_B_P0Q1                                   
  mt9t111_reg_write(client, 0x3658, 0x0711);   // P_B_P0Q2                                   
  mt9t111_reg_write(client, 0x365A, 0x354C);   // P_B_P0Q3                                   
  mt9t111_reg_write(client, 0x365C, 0xC8F0);   // P_B_P0Q4                                   
  mt9t111_reg_write(client, 0x3694, 0x010A);   // P_B_P1Q0                                   
  mt9t111_reg_write(client, 0x3696, 0xF2EE);   // P_B_P1Q1                                   
  mt9t111_reg_write(client, 0x3698, 0x2A6E);   // P_B_P1Q2                                   
  mt9t111_reg_write(client, 0x369A, 0x1630);   // P_B_P1Q3                                   
  mt9t111_reg_write(client, 0x369C, 0xAE6F);   // P_B_P1Q4                                   
  mt9t111_reg_write(client, 0x36D4, 0x03F2);   // P_B_P2Q0                                   
  mt9t111_reg_write(client, 0x36D6, 0x19EE);   // P_B_P2Q1                                   
  mt9t111_reg_write(client, 0x36D8, 0x2252);   // P_B_P2Q2                                   
  mt9t111_reg_write(client, 0x36DA, 0x4830);   // P_B_P2Q3                                   
  mt9t111_reg_write(client, 0x36DC, 0xB175);   // P_B_P2Q4                                   
  mt9t111_reg_write(client, 0x3714, 0x09CD);   // P_B_P3Q0                                   
  mt9t111_reg_write(client, 0x3716, 0x8570);   // P_B_P3Q1                                   
  mt9t111_reg_write(client, 0x3718, 0x15CE);   // P_B_P3Q2                                   
  mt9t111_reg_write(client, 0x371A, 0x4E91);   // P_B_P3Q3                                   
  mt9t111_reg_write(client, 0x371C, 0x528F);   // P_B_P3Q4                                   
  mt9t111_reg_write(client, 0x3754, 0xC6CB);   // P_B_P4Q0                                   
  mt9t111_reg_write(client, 0x3756, 0x6B11);   // P_B_P4Q1                                   
  mt9t111_reg_write(client, 0x3758, 0xD076);   // P_B_P4Q2                                   
  mt9t111_reg_write(client, 0x375A, 0xC175);   // P_B_P4Q3                                   
  mt9t111_reg_write(client, 0x375C, 0x68F8);   // P_B_P4Q4                                   
  mt9t111_reg_write(client, 0x365E, 0x00D0);   // P_G2_P0Q0                                  
  mt9t111_reg_write(client, 0x3660, 0xA30C);   // P_G2_P0Q1                                  
  mt9t111_reg_write(client, 0x3662, 0x1FF1);   // P_G2_P0Q2                                  
  mt9t111_reg_write(client, 0x3664, 0x906C);   // P_G2_P0Q3                                  
  mt9t111_reg_write(client, 0x3666, 0x8CD1);   // P_G2_P0Q4                                  
  mt9t111_reg_write(client, 0x369E, 0x25CA);   // P_G2_P1Q0                                  
  mt9t111_reg_write(client, 0x36A0, 0x302F);   // P_G2_P1Q1                                  
  mt9t111_reg_write(client, 0x36A2, 0x16CE);   // P_G2_P1Q2                                  
  mt9t111_reg_write(client, 0x36A4, 0xB430);   // P_G2_P1Q3                                  
  mt9t111_reg_write(client, 0x36A6, 0xD80E);   // P_G2_P1Q4                                  
  mt9t111_reg_write(client, 0x36DE, 0x1AD2);   // P_G2_P2Q0                                  
  mt9t111_reg_write(client, 0x36E0, 0x44EE);   // P_G2_P2Q1                                  
  mt9t111_reg_write(client, 0x36E2, 0x0671);   // P_G2_P2Q2                                  
  mt9t111_reg_write(client, 0x36E4, 0x21CE);   // P_G2_P2Q3                                  
  mt9t111_reg_write(client, 0x36E6, 0x96F5);   // P_G2_P2Q4                                  
  mt9t111_reg_write(client, 0x371E, 0x3427);   // P_G2_P3Q0                                  
  mt9t111_reg_write(client, 0x3720, 0x6790);   // P_G2_P3Q1                                  
  mt9t111_reg_write(client, 0x3722, 0xA6F0);   // P_G2_P3Q2                                  
  mt9t111_reg_write(client, 0x3724, 0xE173);   // P_G2_P3Q3                                  
  mt9t111_reg_write(client, 0x3726, 0x53B2);   // P_G2_P3Q4                                  
  mt9t111_reg_write(client, 0x375E, 0xFBAD);   // P_G2_P4Q0                                  
  mt9t111_reg_write(client, 0x3760, 0x55D1);   // P_G2_P4Q1                                  
  mt9t111_reg_write(client, 0x3762, 0xD036);   // P_G2_P4Q2                                  
  mt9t111_reg_write(client, 0x3764, 0x9AB5);   // P_G2_P4Q3                                  
  mt9t111_reg_write(client, 0x3766, 0x59F8);   // P_G2_P4Q4                                  
  mt9t111_reg_write(client, 0x3784, 0x03E0);   // CENTER_COLUMN                              
  mt9t111_reg_write(client, 0x3782, 0x0300);   // CENTER_ROW                                 
  mt9t111_reg_write(client, 0x3210, 0x01B8);   // COLOR_PIPELINE_CONTROL 
  mt9t111_reg_write(client, 0x098E, 0x48B0);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0147);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48B2);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFF82);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48B4);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0046);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48B6);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFF3);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48B8);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0124);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48BA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFF8);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48BC);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0017);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48BE);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFA8);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48C0);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0153);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48C2);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0018);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48C4);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x003F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48C6);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0012);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48C8);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x003B);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48CA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFB3);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48CC);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFF6);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48CE);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0049);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48D0);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFC0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48D2);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFF5);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48D4);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0013);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48D6);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFF7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48D8);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0016);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48DA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFE3);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8F4);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0004);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8F5);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0002);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48F6);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFC29);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48F8);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x801A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48FA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x2931);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48FC);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x67A6);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48FE);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x3DE2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4900);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x901C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4902);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x7FF8);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4904);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0003);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4906);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0027);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4908);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0027);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8F1);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00AC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB839);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8EE);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x005F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8EF);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00C0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8F0);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0064);  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC33);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x003D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE84D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0075);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE84F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC38);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0036);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC39);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x006B);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC3A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0041);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC3B);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x006C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC910);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000C);   //0x0009   //0x0020   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC911);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00FC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x3266, 0x00C4);   // AWB_WEIGHT_TH
  mt9t111_reg_write(client, 0x098E, 0x683F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x01D9);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE847);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE848);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0003);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC02);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC3C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x003C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC3D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0050);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6841);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC949);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0033);   //0x0020  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC94A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0062);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x35A2, 0x0080);   // DARK_COLOR_KILL_CONTROLS
  mt9t111_reg_write(client, 0x35A4, 0x0596);   // BRIGHT_COLOR_KILL_CONTROLS
  mt9t111_reg_write(client, 0x098E, 0x2C05);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x2C07);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x07D0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8F2);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC8F3);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x007F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC34);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0045);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC35);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC36);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0045);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xAC37);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC913);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x686B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0570);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x686D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0580);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C6B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0570);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C6D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0580);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C25);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0008);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C27);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC91C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0096);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC91D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00FF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC91E);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0010);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC91F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0008);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC920);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0010);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC921);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0010);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC922);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0020);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC923);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0018);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC924);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0018);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC925);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0020);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC916);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0014);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC917);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0003);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC918);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0003);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC919);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0020);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC91A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC91B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0009);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x326C, 0x1403);   // APERTURE_PARAMETERS_2D
  mt9t111_reg_write(client, 0x098E, 0xBC02);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC05);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x494B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0020);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x494D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00F0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xDC2A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x001F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xDC2B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0012);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC914);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC915);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00FF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48DC);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48DE);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0096);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48E0);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x001D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48E2);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48E4);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0096);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48E6);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x001D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48E8);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48EA);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0096);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x48EC);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x001D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6821);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x1B58);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE823);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE824);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6C21);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x1B58);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xEC23);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xEC24);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA415);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0004);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE82B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0002);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE81F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0040);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xEC1F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0040);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA80E);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xA807);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0018);   //0x003C  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6815);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6817);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0010);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x682D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x682F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0100);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6831);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0020);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6833);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6835);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00D0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6839);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x012C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6837);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0040);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4815);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x485D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x681B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0004);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE82C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0

  mt9t111_reg_write(client, 0x098E, 0xBC0B);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_0]
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC0C);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_1]
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC0D);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_2]
  mt9t111_reg_write(client, 0x0990, 0x0013);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC0E);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_3]
  mt9t111_reg_write(client, 0x0990, 0x0032);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC0F);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_4]
  mt9t111_reg_write(client, 0x0990, 0x0055);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC10);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_5]
  mt9t111_reg_write(client, 0x0990, 0x006E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC11);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_6]
  mt9t111_reg_write(client, 0x0990, 0x0083);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC12);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_7]
  mt9t111_reg_write(client, 0x0990, 0x0095);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC13);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_8]
  mt9t111_reg_write(client, 0x0990, 0x00A3);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC14);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_9]
  mt9t111_reg_write(client, 0x0990, 0x00B1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC15);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_10]
  mt9t111_reg_write(client, 0x0990, 0x00BC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC16);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_11]
  mt9t111_reg_write(client, 0x0990, 0x00C7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC17);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_12]
  mt9t111_reg_write(client, 0x0990, 0x00D1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC18);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_13]
  mt9t111_reg_write(client, 0x0990, 0x00DA);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC19);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_14]
  mt9t111_reg_write(client, 0x0990, 0x00E2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC1A);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_15]
  mt9t111_reg_write(client, 0x0990, 0x00EA);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC1B);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_16]
  mt9t111_reg_write(client, 0x0990, 0x00F1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC1C);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_17]
  mt9t111_reg_write(client, 0x0990, 0x00F8);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC1D);   // MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_18]
  mt9t111_reg_write(client, 0x0990, 0x00FF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC1E);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_0]
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC1F);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_1]
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC20);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_2]
  mt9t111_reg_write(client, 0x0990, 0x0013);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC21);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_3]
  mt9t111_reg_write(client, 0x0990, 0x0032);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC22);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_4]
  mt9t111_reg_write(client, 0x0990, 0x0055);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC23);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_5]
  mt9t111_reg_write(client, 0x0990, 0x006E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC24);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_6]
  mt9t111_reg_write(client, 0x0990, 0x0083);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC25);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_7]
  mt9t111_reg_write(client, 0x0990, 0x0095);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC26);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_8]
  mt9t111_reg_write(client, 0x0990, 0x00A3);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC27);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_9]
  mt9t111_reg_write(client, 0x0990, 0x00B1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC28);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_10]
  mt9t111_reg_write(client, 0x0990, 0x00BC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC29);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_11]
  mt9t111_reg_write(client, 0x0990, 0x00C7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC2A);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_12]
  mt9t111_reg_write(client, 0x0990, 0x00D1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC2B);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_13]
  mt9t111_reg_write(client, 0x0990, 0x00DA);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC2C);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_14]
  mt9t111_reg_write(client, 0x0990, 0x00E2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC2D);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_15]
  mt9t111_reg_write(client, 0x0990, 0x00EA);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC2E);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_16]
  mt9t111_reg_write(client, 0x0990, 0x00F1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC2F);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_17]
  mt9t111_reg_write(client, 0x0990, 0x00F8);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC30);   // MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_18]
  mt9t111_reg_write(client, 0x0990, 0x00FF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC31);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_0]
  mt9t111_reg_write(client, 0x0990, 0x000C);     //0x0000  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC32);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_1]
  mt9t111_reg_write(client, 0x0990, 0x0010);     //0x0006  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC33);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_2]
  mt9t111_reg_write(client, 0x0990, 0x001F);     //0x0013  // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC34);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_3]
  mt9t111_reg_write(client, 0x0990, 0x0032);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC35);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_4]
  mt9t111_reg_write(client, 0x0990, 0x0055);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC36);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_5]
  mt9t111_reg_write(client, 0x0990, 0x006E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC37);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_6]
  mt9t111_reg_write(client, 0x0990, 0x0083);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC38);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_7]
  mt9t111_reg_write(client, 0x0990, 0x0095);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC39);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_8]
  mt9t111_reg_write(client, 0x0990, 0x00A3);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC3A);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_9]
  mt9t111_reg_write(client, 0x0990, 0x00B1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC3B);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_10]
  mt9t111_reg_write(client, 0x0990, 0x00BC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC3C);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_11]
  mt9t111_reg_write(client, 0x0990, 0x00C7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC3D);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_12]
  mt9t111_reg_write(client, 0x0990, 0x00D1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC3E);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_13]
  mt9t111_reg_write(client, 0x0990, 0x00DA);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC3F);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_14]
  mt9t111_reg_write(client, 0x0990, 0x00E2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC40);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_15]
  mt9t111_reg_write(client, 0x0990, 0x00EA);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC41);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_16]
  mt9t111_reg_write(client, 0x0990, 0x00F1);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC42);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_17]
  mt9t111_reg_write(client, 0x0990, 0x00F8);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC43);   // MCU_ADDRESS [LL_GAMMA_NRCURVE_18]
  mt9t111_reg_write(client, 0x0990, 0x00FF);   // MCU_DATA_0

  mt9t111_reg_write(client, 0x098E, 0x4926);  // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0141);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4928);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0485);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x492A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0700);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC44);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0028);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xBC45);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0028);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3C46);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0012);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3C48);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x01A4);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x326E, 0x00A4);   // LOW_PASS_YUV_FILTER
  mt9t111_reg_write(client, 0x098E, 0x3C4D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFFF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3C4F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xFFFF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB839);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE86F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0470);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xE870);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xEC6F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0470);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xEC70);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0140);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x5C28);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x2000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3005);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00ED);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3007);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00C0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3009);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0089);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x300B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0060);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4403);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x8001);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4411);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0xF41C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xC421);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0018);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3003);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0002);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB002);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0028);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3020);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x557D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3022);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x7D55);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3005);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0185);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3007);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0126);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3009);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0041);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x300B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0033);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x3017);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x2020);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB01D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000B);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB01F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x000F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x440B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x440D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x03C0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB01C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB025);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x003A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB026);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB027);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x004E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB028);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0052);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB029);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0063);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB02A);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0072);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB02B);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0080);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB02C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0090);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB02D);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0098);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB02E);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x00AF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0xB02F);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0982, 0x0000);   // ACCESS_CTL_STAT
  mt9t111_reg_write(client, 0x098A, 0x0351);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3C3C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC640);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xF730);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC4DC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x65FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C0);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xDC67);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0361);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC2DE);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x1DEC);  // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x25FD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C5);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0101);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x01FC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C2);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0371);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD6EC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0C5F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x8402);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x4416);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x4F30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xE703);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD163);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x2714);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0381);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD163);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x2309);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xD604);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFD06);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xD620);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x07FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06D6);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0391);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x05FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06D6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xDE1D);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xE60B);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC407);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30E7);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x02D1);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x6427);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x03A1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x2B7D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0064);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x2726);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x5D27);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x237F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C4);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFC06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xD6FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x03B1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C5);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xD664);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x4FFD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC640);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xF730);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC4E6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x024F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x03C1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFD30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC501);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0101);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFC30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC2FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06D6);  // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x7D06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xCB27);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x03D1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x2EC6);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x40F7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x30C4);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFC06);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC104);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xF306);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD6ED);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x005F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x03E1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x6D00);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x2A01);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x5317);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC0EC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x00FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C2);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x03F1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC1FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C5);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0101);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x01FC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C2);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD06);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC720);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x227F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0401);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C4);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE1D);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xEC25);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC5FC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06D6);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC701);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0411);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFC30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC0FD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06D0);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFC30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC2FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06D2);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xEC25);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0421);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC3BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x953C);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xDE3F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xEE10);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xAD00);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDE1D);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFC06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xCCED);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0431);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3E38);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x3839);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x3C3C);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xBD61);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xD5CE);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x04CD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x1F17);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0211);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0441);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xCC33);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x2E30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xED02);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xCCFF);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFDED);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x00CC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0002);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xBD70);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0451);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x6D18);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE1F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x181F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x8E01);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x10CC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3C52);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30ED);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0018);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0461);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xECA0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC4FD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xBD70);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x2120);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x1ECC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3C52);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30ED);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x00DE);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0471);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x1FEC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xA0BD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x7021);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xCC3C);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x5230);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xED02);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCCFF);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFCED);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0481);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x00CC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0002);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xBD70);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x6D38);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x3839);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3CFC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06EE);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30ED);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0491);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x00BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xA228);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x7D06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xCB27);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x11FC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06EE);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30A3);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0027);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x04A1);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x09FC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06C5);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC300);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x01FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x06C7);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3839);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x098A, 0x0AE0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x37DE);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x1DEC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0C5F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x8402);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x4416);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x4FD7);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x63E6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0BC4);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0AF0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x07D7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x647F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x30C4);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xEC25);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFD30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC5FC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06D6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B00);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC701);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFC30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC0DD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x65FC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C2);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDD67);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30E6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x00BD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B10);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x5203);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x3139);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x3CBD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x776D);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xCC32);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x5C30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC13);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B20);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x8683);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0001);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xBD70);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x21CC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x325E);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30ED);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x00FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x1388);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B30);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x8300);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x01BD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x7021);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x3839);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x3C3C);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3C34);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCE06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x6A1E);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B40);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x0210);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06BD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x8427);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x7E0B);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xD47F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C4);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCC01);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x00FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B50);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C5);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCC00);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFFFD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC640);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xF730);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC4F6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x13A7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B60);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x4F30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xED00);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xCC00);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFFA3);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x00FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C5);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0101);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x01FC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B70);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xED04);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC300);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x01ED);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x046F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06C6);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x80F7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C4);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B80);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xE606);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x4F05);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC306);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x738F);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xEC00);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC530);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xEC04);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0B90);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFD30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC7C6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC0F7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C4);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xCC01);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x00FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C5);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0101);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0BA0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x01FC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C2);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xED02);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x8307);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFF2F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x07CC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x07FF);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xED02);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0BB0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x200C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xEC02);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x83F8);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x012C);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x05CC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xF801);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED02);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xE606);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0BC0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x4F05);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC306);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x7318);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x8FEC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0218);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xED00);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x6C06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xE606);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0BD0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC109);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x25A7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x3838);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x3831);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x098A, 0x8BD8);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0982, 0x0000);   // ACCESS_CTL_STAT
  mt9t111_reg_write(client, 0x0990, 0x0039);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0982, 0x0000);   // ACCESS_CTL_STAT
  mt9t111_reg_write(client, 0x098A, 0x0CD4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3C3C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x3C3C);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xD230);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xED06);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFC06);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD0ED);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x04DC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0CE4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x5DED);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x02DC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x5BED);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x00BD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xD4D0);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x234D);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xDC5B);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0CF4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD0DC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x5DFD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06D2);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFC06);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xD0DD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x53FC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06D2);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xDD55);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D04);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x8640);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xB730);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC4DC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x53FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C0);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDC55);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC2DC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D14);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x61FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C5);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0101);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x7F30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC4DC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x59FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C5);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D24);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC2FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x01FC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C2);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDD59);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x7C06);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCBFE);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06C7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D34);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x08FF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06C7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x7A00);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x5720);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x2FDE);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x1FEC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x3130);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xED06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D44);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x5F4F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xED04);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xD2ED);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x02FC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06D0);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xBDD4);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D54);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD023);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x12CE);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06AD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x1C01);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x01DE);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x1F5F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x4FFD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06D0);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D64);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xEC31);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFD06);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xD25F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xD757);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x3838);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3838);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x393C);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x3C3C);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D74);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3C7F);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C4);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xCCFD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C5);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xD658);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x4FFD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D84);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC640);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xF730);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC4DC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x59FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C5);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0101);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x01FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C0);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0D94);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xDD53);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFC30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC2DD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x55DE);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x1FEC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3330);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x5F4F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0DA4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xED04);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDC55);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xED02);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDC53);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xED00);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xBDD4);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD023);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x2F7F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0DB4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C4);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE1F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xEC33);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC5DC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x59FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC640);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0DC4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xF730);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC4FC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06CC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC501);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0101);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFC30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC2DD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0DD4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x59EC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x33FD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06CC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x7A06);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xCB7A);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0057);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x2030);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xDE1F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0DE4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xEC35);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30ED);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x065F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x4FED);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x04DC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x55ED);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x02DC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x53ED);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0DF4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x00BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xD4D0);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x2410);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDE1F);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x5F4F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDD53);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xEC35);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xDD55);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E04);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xCE06);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xAD1C);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0102);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDC55);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFD06);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xCC5F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD757);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x3838);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E14);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3838);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x39C6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x40F7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C4);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDC53);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC0DC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x55FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E24);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE1F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xEC37);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC501);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0101);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFC30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC2FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E34);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06C7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xEC3B);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xB306);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC723);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x3D7F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C4);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xDC59);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E44);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC5EC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x37FD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x30C7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC640);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xF730);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC4DC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x61FD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C5);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E54);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x0101);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x01FC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x30C2);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDD59);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xEC37);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD06);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCEDC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x5BFD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E64);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06D0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDC5D);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFD06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xD2EC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x3B83);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0001);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC77A);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E74);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06CB);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x7A00);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x5739);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC640);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xF730);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC4DC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x53FD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C0);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E84);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xDC55);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFD30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC2FC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x06C7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFD30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC501);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0101);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0E94);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC2FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06CE);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x5FD7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x5739);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFC06);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC5FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06C7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x8640);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0EA4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xB730);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC4DC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x53FD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C0);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDC55);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC2FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06C7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0EB4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFD30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC501);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0101);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFC30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC2FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06CE);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xDE1F);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xEC39);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0EC4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xB306);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCE23);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x22C6);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x40F7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C4);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDC53);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC0DC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0ED4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x55FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C2);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC5FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C5);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0101);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x01FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C2);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0EE4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFD06);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCE5F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xD757);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x39DE);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x1FEC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x39FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06CE);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x7F30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0EF4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC4EC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x39FE);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06C5);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xBDD4);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x79FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C5);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xDC59);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFD30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F04);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC7C6);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x40F7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x30C4);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDC61);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFD30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC501);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0101);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F14);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC2DD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x597C);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06CB);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x7A00);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x5739);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x3C3C);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x3C3C);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x3C34);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F24);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xBDAD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x15DE);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x198F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC301);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x168F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xE600);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xF705);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x14E6);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F34);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x03F7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0515);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xEC35);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD05);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0EEC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x37FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0510);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC05);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F44);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x5BFD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0512);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xDE37);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xEE08);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xAD00);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30E7);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0ADE);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x198F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F54);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC301);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x178F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xE600);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xF705);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x14E6);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x03F7);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0515);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xDE37);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F64);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xEE08);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xAD00);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x30E7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x09DE);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x198F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC301);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x188F);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xE600);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F74);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xF705);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x14E6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x03F7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0515);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDE37);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xEE08);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xAD00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30E7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F84);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x08CC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x328E);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xED00);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xE60A);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x4FBD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x7021);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCC32);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x6C30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0F94);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xED02);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCCF8);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x00ED);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x00E6);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x094F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x175F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xE608);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0FA4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x4FED);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x04E3);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0684);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x07BD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x706D);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x30C6);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0B3A);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x3539);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x0FB4);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x7E9E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098A, 0x8FB6);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0982, 0x0000);   // ACCESS_CTL_STAT
  mt9t111_reg_write(client, 0x0990, 0x0014);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0982, 0x0000);   // ACCESS_CTL_STAT
  mt9t111_reg_write(client, 0x098A, 0x1000);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xF601);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x85C1);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0326);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x10F6);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0186);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC168);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x2609);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xCC10);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1010);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x18BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x4224);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xBD10);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x1839);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC602);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xF701);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x8AC6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0AF7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1020);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x018B);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE3F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x18CE);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0BED);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xCC00);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x11BD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD700);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xCC0B);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1030);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xEDDD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x3FDE);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x3B18);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xCE0B);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFFCC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0015);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xBDD7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x00CC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1040);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x0BFF);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDD3B);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xDE35);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x18CE);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0C15);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xCC00);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x3FBD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xD700);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1050);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xCC0C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x15DD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x35DE);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x4718);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xCE0C);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x55CC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0015);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xBDD7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1060);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x00CC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0C55);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xDD47);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDE41);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x18CE);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0C6B);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCC00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0DBD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1070);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD700);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCC0C);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x6BDD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x41DE);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x3918);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xCE0C);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x79CC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0023);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1080);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xBDD7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x00CC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0C79);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDD39);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDE31);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x18CE);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0C9D);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xCC00);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1090);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x29BD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xD700);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xCC0C);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x9DDD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x31DE);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x4318);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCE0C);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC7CC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x10A0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x000B);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xBDD7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x00CC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0CC7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDD43);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xCC03);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x51FD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0BF9);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x10B0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xCC10);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xE1FD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0BF7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xCC0B);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x38FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0C0B);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCC04);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x35FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x10C0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x0C31);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCC0F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x1EFD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0C63);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xCC0F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xB4FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0C6D);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xCC0B);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x10D0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x14FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0C85);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xCC0A);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xE0FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0CA7);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xCC04);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x8BFD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0CD1);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x10E0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3930);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x8FC3);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFFF0);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x8F35);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xF606);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xBC4F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xDD59);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x7F30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x10F0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC4FC);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06C1);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFD30);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC5FC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x06C3);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0404);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC701);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1100);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFC30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC0DD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x5BFC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C2);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDD5D);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFC06);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD2ED);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x02FC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1110);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06D0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xED00);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC602);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xBDD6);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xE030);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xEC00);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xD0EC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1120);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x02FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06D2);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC604);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xD757);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDC59);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x2605);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCC00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x01DD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1130);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x597D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0059);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x2707);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC6FF);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30E7);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0C20);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x05D6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x5A30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1140);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xE70C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xF606);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xBA54);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x5454);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x54E7);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0D6C);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0DF6);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06C9);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1150);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC101);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x260B);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xF606);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xBAE7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0EE6);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0CE7);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0F20);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x09E6);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1160);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x0CE7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x0EF6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06BA);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xE70F);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xE60E);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xE10F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x2206);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x7F06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1170);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xC97E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x1353);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xE60E);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xE00F);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xE70F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDE1F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xE62C);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x4F30);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1180);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xED0A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xE60F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x4FEE);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0A02);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x8F30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xE70F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xE10D);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x2404);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1190);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xE60D);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xE70F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xF606);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC9C1);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0126);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x08E6);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0CEB);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x0FD7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x11A0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x5820);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x06E6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0CE0);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0FD7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x58F6);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06CB);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x270D);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC101);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x11B0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x2753);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC102);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x2603);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x7E13);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x4820);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x47FC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06D0);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xDD5F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x11C0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFC06);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xD2DD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x6186);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x40B7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C4);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDC5F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC0DC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x11D0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x61FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C2);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xDC59);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC501);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x017F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C4);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xD658);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x11E0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x4FFD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C5);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC30);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC2FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C7);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x01FC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C0);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xDD53);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x11F0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFC30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC2DD);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x55DC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x53FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x06D0);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDC55);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xD2BD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1200);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x0CD4);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x7E13);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x4B7F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C4);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFC06);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC7FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x30C5);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xFC06);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1210);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xCEFD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C7);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x01FC);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C0);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDD5F);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFC30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC2DD);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x617F);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1220);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C4);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFD30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC5D6);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x584F);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFD30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC7C6);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x40F7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C4);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1230);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xDC59);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFD30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC501);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0101);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xFC30);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xC0DD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x53FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C2);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1240);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xDD55);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE1F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x3C18);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x38EC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x2DCD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xEE37);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xBDD4);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x7930);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1250);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xED0A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x5F6D);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0A2A);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x0153);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x17ED);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x08EC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0AED);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06EC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1260);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x08ED);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x04DC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x55ED);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x02DC);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x53ED);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x00BD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xD4D0);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x2305);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1270);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xBD0E);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x1720);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x8DDE);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x1F3C);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x1838);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xEC2D);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xCDEE);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x2FBD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1280);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD479);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30ED);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0A5F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x6D0A);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x2A01);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x5317);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED08);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xEC0A);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1290);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xED06);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xEC08);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xED04);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xDC55);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xED02);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDC53);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xBDD4);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x12A0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xD024);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x6EC6);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x40F7);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C4);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDC53);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC0DC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x55FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x12B0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x30C2);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xDE1F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xEC2F);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC501);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0101);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFC30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC2FD);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x12C0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06C7);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFC30);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC727);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x07FE);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x06C7);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x08FF);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06C7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC640);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x12D0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xF730);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC4DC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x53FD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30C0);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xDC55);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xFD30);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC2FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06C7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x12E0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFD30);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC501);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x0101);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFC30);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xC2FD);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06CE);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFC06);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC7B3);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x12F0);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06C5);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x2305);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xBD0E);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x9C20);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x52DE);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x1FEC);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x3BB3);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06C7);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1300);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x230A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xEC3B);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFD06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC77A);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x0057);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x203F);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x5FD7);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x5720);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1310);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x3ADE);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x1FEC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x2DFD);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x06C7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xEC2D);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xB306);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xC523);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x06FC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1320);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x06C5);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xFD06);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xC7C6);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x40F7);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C4);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0xDC53);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xFD30);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC0DC);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1330);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x55FD);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x30C2);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xFC06);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xC7FD);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x30C5);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x0101);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x01FC);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C2);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1340);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xFD06);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xCE5F);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xD757);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x2003);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xBD0D);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x717D);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x0057);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x2703);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1350);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x7E11);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x28FC);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0x06D2);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0x30ED);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0x02FC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x06D0);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0xED00);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0xC602);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1360);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0xBDD6);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0xC030);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x0994, 0xEC00);   // MCU_DATA_2
  mt9t111_reg_write(client, 0x0996, 0xFD06);   // MCU_DATA_3
  mt9t111_reg_write(client, 0x0998, 0xD0EC);   // MCU_DATA_4
  mt9t111_reg_write(client, 0x099A, 0x02FD);   // MCU_DATA_5
  mt9t111_reg_write(client, 0x099C, 0x06D2);   // MCU_DATA_6
  mt9t111_reg_write(client, 0x099E, 0x30C6);   // MCU_DATA_7
  mt9t111_reg_write(client, 0x098A, 0x1370);   // PHYSICAL_ADDR_ACCESS
  mt9t111_reg_write(client, 0x0990, 0x103A);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x0992, 0x3539);   // MCU_DATA_1
  mt9t111_reg_write(client, 0x098E, 0x0010);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x1000);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x0003);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0004);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x480C);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x046C);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x4854);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0024);   // MCU_DATA_0

  mt9t111_reg_write(client, 0x098E, 0x8400);   // MCU_ADDRESS
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0

  mt9t111_reg_write(client, 0x098E, 0x6800);   // MCU_ADDRESS [PRI_A_IMAGE_WIDTH]
  mt9t111_reg_write(client, 0x0990, 0x0280);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x6802);   // MCU_ADDRESS [PRI_A_IMAGE_HEIGHT]
  mt9t111_reg_write(client, 0x0990, 0x01E0);   // MCU_DATA_0
  mt9t111_reg_write(client, 0x098E, 0x8400);   // MCU_ADDRESS [SEQ_CMD]
  mt9t111_reg_write(client, 0x0990, 0x0006);   // MCU_DATA_0

  return ret;
}
#endif

/*
 * MT9T111 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int s5k4ba_write(struct v4l2_subdev *sd, u8 addr, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[2];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = reg;

	reg[0] = addr & 0xff;
	reg[1] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return err;	/* Returns here on success */

	/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s: address: 0x%02x, " \
			"value: 0x%02x\n", __func__, \
			reg[0], reg[1]);
		retry++;
		goto again;
	}

	return err;
}

static int s5k4ba_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = {client->addr, 0, length, buf};

	for (i = 0; i < length; i++) {
		buf[i] = i2c_data[i];
	}
	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static int s5k4ba_write_regs(struct v4l2_subdev *sd, unsigned char regs[], 
				int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;

	for (i = 0; i < size; i++) {
		err = s5k4ba_i2c_write(sd, &regs[i], sizeof(regs[i]));
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", \
			__func__);
	}

	return 0;	/* FIXME */
}

static const char *mt9t111_querymenu_wb_preset[] = {
	"WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL
};

static const char *mt9t111_querymenu_effect_mode[] = {
	"Effect Sepia", "Effect Aqua", "Effect Monochrome",
	"Effect Negative", "Effect Sketch", NULL
};

static const char *mt9t111_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};

static struct v4l2_queryctrl mt9t111_controls[] = {
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance in kelvin",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(mt9t111_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(mt9t111_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = (ARRAY_SIZE(mt9t111_querymenu_ev_bias_mode) - 2) / 2,	/* 0 EV */
	},
	{
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(mt9t111_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
};

const char **s5k4ba_ctrl_get_menu(u32 id)
{
	switch (id) {
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return mt9t111_querymenu_wb_preset;

	case V4L2_CID_COLORFX:
		return mt9t111_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return mt9t111_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *s5k4ba_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt9t111_controls); i++)
		if (mt9t111_controls[i].id == id)
			return &mt9t111_controls[i];

	return NULL;
}

static int mt9t111_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt9t111_controls); i++) {
		if (mt9t111_controls[i].id == qc->id) {
			memcpy(qc, &mt9t111_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int mt9t111_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	mt9t111_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, s5k4ba_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * 	freq : in Hz
 * 	flag : not supported for now
 */
static int mt9t111_s_crystal_freq(struct v4l2_subdev *sd, u32 freq, u32 flags)
{
	int err = -EINVAL;

	return err;
}

static int mt9t111_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int mt9t111_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}
static int mt9t111_enum_framesizes(struct v4l2_subdev *sd, \
					struct v4l2_frmsizeenum *fsize)
{
	int err = 0;

	return err;
}

static int mt9t111_enum_frameintervals(struct v4l2_subdev *sd, 
					struct v4l2_frmivalenum *fival)
{
	int err = 0;

	return err;
}

static int mt9t111_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;

	return err;
}

static int mt9t111_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int mt9t111_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dev_dbg(&client->dev, "%s\n", __func__);

	return err;
}

static int s5k4ba_change_resolution(struct v4l2_subdev *sd, \
					enum image_size_s5k4ba image_size)
{
	int i;
	int err = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

#if 0
	switch (image_size) {
	case SQVGA:
		for (i = 0; i < MT9T111_QSVGA_REGS; i++) {
			err = s5k4ba_i2c_write(sd, s5k4ba_qsvga_reg[i], \
						sizeof(s5k4ba_qsvga_reg[i]));
			if (err < 0)
				v4l_info(client, "%s: register set failed\n", \
				__func__);
		}
		break;

	case SVGA:	/* fall through */
	default:
		//printk("s5k4ba_change_resolution : case SVGA");
		for (i = 0; i < MT9T111_SVGA_REGS; i++) {
			err = s5k4ba_i2c_write(sd, s5k4ba_svga_reg[i], \
						sizeof(s5k4ba_svga_reg[i]));
			if (err < 0)
				v4l_info(client, "%s: register set failed\n", \
				__func__);
		}
		break;
	}
#endif
	return err;
}

static int mt9t111_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

#if 0
	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", \
		__func__, param->parm.capture.timeperframe.numerator, \
		param->parm.capture.timeperframe.denominator);

	if (param->parm.capture.capturemode == V4L2_MODE_HIGHQUALITY) {
		err = s5k4ba_change_resolution(sd, SQVGA);
	} else {
		err = s5k4ba_change_resolution(sd, SVGA);
	}
	
	if (err < 0)
		v4l_info(client, "%s: mt9t111_s_parm set failed\n", __func__);
#endif
	return err;
}

static int mt9t111_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct mt9t111_state *state = to_state(sd);
	struct mt9t111_userset userset = state->userset;
	int err = -EINVAL;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;

	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;

	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;

	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}
	
	return err;
}

static int mt9t111_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#ifdef MT9T111_COMPLETE
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s: V4L2_CID_EXPOSURE\n", __func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_ev_bias[ctrl->value], \
			sizeof(s5k4ba_regs_ev_bias[ctrl->value]));
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		dev_dbg(&client->dev, "%s: V4L2_CID_AUTO_WHITE_BALANCE\n", \
			__func__);
		
#if defined CONFIG_ARCH_S3C64XX
		if ((ctrl->value < 0) || (ctrl->value > 6)) {
			dev_dbg(&client->dev, "WB type is invalid:[%d]\n", \
				ctrl->value);
			break;
		}

		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_wb_preset[ctrl->value], \
			sizeof(s5k4ba_regs_wb_preset[ctrl->value]));
#else
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_awb_enable[ctrl->value], \
			sizeof(s5k4ba_regs_awb_enable[ctrl->value]));
#endif
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		dev_dbg(&client->dev, "%s: V4L2_CID_WHITE_BALANCE_PRESET\n", \
			__func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_wb_preset[ctrl->value], \
			sizeof(s5k4ba_regs_wb_preset[ctrl->value]));
		break;

	case V4L2_CID_COLORFX:
		dev_dbg(&client->dev, "%s: V4L2_CID_COLORFX\n", __func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_color_effect[ctrl->value], \
			sizeof(s5k4ba_regs_color_effect[ctrl->value]));
		break;

	case V4L2_CID_CONTRAST:
		dev_dbg(&client->dev, "%s: V4L2_CID_CONTRAST\n", __func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_contrast_bias[ctrl->value], \
			sizeof(s5k4ba_regs_contrast_bias[ctrl->value]));
		break;

	case V4L2_CID_SATURATION:
		dev_dbg(&client->dev, "%s: V4L2_CID_SATURATION\n", __func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_saturation_bias[ctrl->value], \
			sizeof(s5k4ba_regs_saturation_bias[ctrl->value]));
		break;

	case V4L2_CID_SHARPNESS:
		dev_dbg(&client->dev, "%s: V4L2_CID_SHARPNESS\n", __func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_sharpness_bias[ctrl->value], \
			sizeof(s5k4ba_regs_sharpness_bias[ctrl->value]));
		break;

	default:
		dev_err(&client->dev, "%s: no such control\n", __func__);
		break;
	}

	if (err < 0)
		goto out;
	else
		return 0;

out:
	dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);
	return err;
#else
	return 0;
#endif
}

static int mt9t111_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;

#if 0
	v4l_info(client, "%s: camera initialization start\n", __func__);

	for (i = 0; i < MT9T111_INIT_REGS; i++) {
		err = s5k4ba_i2c_write(sd, mt9t111_init_reg[i], \
					sizeof(mt9t111_init_reg[i]));
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", \
			__func__);
	}

	if (err < 0) {
		v4l_err(client, "%s: camera initialization failed\n", \
			__func__);
		return -EIO;	/* FIXME */
	}
#else
  mt9t111_camera_probe(client);
  mt9t111_init_setting(client);
#endif

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time therefor,
 * it is not necessary to be initialized on probe time. except for version checking
 * NOTE: version checking is optional
 */
static int mt9t111_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct mt9t111_state *state = to_state(sd);
	struct s5k4ba_platform_data *pdata;

	dev_info(&client->dev, "fetching platform data\n");

	pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 24000000;	/* 24MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi) {
		state->is_mipi = 0;
		dev_info(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->is_mipi;

	return 0;
}

static const struct v4l2_subdev_core_ops mt9t111_core_ops = {
	.init = mt9t111_init,	/* initializing API */
	.s_config = mt9t111_s_config,	/* Fetch platform data */
	.queryctrl = mt9t111_queryctrl,
	.querymenu = mt9t111_querymenu,
	.g_ctrl = mt9t111_g_ctrl,
	.s_ctrl = mt9t111_s_ctrl,
};

static const struct v4l2_subdev_video_ops mt9t111_video_ops = {
	.s_crystal_freq = mt9t111_s_crystal_freq,
	.g_fmt = mt9t111_g_fmt,
	.s_fmt = mt9t111_s_fmt,
	.enum_framesizes = mt9t111_enum_framesizes,
	.enum_frameintervals = mt9t111_enum_frameintervals,
	.enum_fmt = mt9t111_enum_fmt,
	.try_fmt = mt9t111_try_fmt,
	.g_parm = mt9t111_g_parm,
	.s_parm = mt9t111_s_parm,
};

static const struct v4l2_subdev_ops mt9t111_ops = {
	.core = &mt9t111_core_ops,
	.video = &mt9t111_video_ops,
};

/*
 * s5k4ba_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5k4ba_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct mt9t111_state *state;
	struct v4l2_subdev *sd;

	state = kzalloc(sizeof(struct mt9t111_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, MT9T111_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &mt9t111_ops);

	dev_info(&client->dev, "s5k4ba has been probed\n");
	return 0;
}


static int s5k4ba_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id s5k4ba_id[] = {
	{ MT9T111_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k4ba_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = MT9T111_DRIVER_NAME,
	.probe = s5k4ba_probe,
	.remove = s5k4ba_remove,
	.id_table = s5k4ba_id,
};

MODULE_DESCRIPTION("Samsung Electronics MT9T111 UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");

