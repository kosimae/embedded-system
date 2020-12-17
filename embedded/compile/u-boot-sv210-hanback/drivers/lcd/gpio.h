#ifndef __GPIO_H__
#define __GPIO_H__

#include <common.h>
#include "./def.h"


#define   Low_Level   0x0
#define   High_Level    0x1
#define   Falling_Edge  0x2
#define   Rising_Edge   0x3
#define   Both_Edge   0x4

typedef enum GPIO_Id
{
  eGPIO_A0 = 0x000,       // muxing bit : 4,    offset : 0,     
  eGPIO_A1 = 0x020,       // muxing bit : 4,    offset : 0x02*0x10    
  eGPIO_B = 0x040,               // muxing bit : 4,     offset : 0x04*0x10
  eGPIO_C0 = 0x060,       // muxing bit : 4,    offset : 0x06*0x10
  eGPIO_C1 = 0x080,       // muxing bit : 4,    offset : 0x08*0x10
  eGPIO_D0 = 0x0A0,       // muxing bit : 4,    offset : 0x0A*0x10
  eGPIO_D1 = 0x0C0,       // muxing bit : 4,    offset : 0x0C*0x10
  eGPIO_E0 = 0x0E0,       // muxing bit : 4,    offset : 0x0E*0x10
  eGPIO_E1 = 0x100,       // muxing bit : 4,    offset : 0x10*0x10  
  eGPIO_F0 = 0x120,       // muxing bit : 4,    offset : 0x12*0x10
  eGPIO_F1 = 0x140,       // muxing bit : 4,    offset : 0x14*0x10
  eGPIO_F2 = 0x160,       // muxing bit : 4,    offset : 0x16*0x10
  eGPIO_F3 = 0x180,       // muxing bit : 4,    offset : 0x18*0x10
  eGPIO_G0 = 0x1A0,       // muxing bit : 4,    offset : 0x1A*0x10
  eGPIO_G1 = 0x1C0,       // muxing bit : 4,    offset : 0x1C*0x10
  eGPIO_G2 = 0x1E0,       // muxing bit : 4,    offset : 0x1E*0x10
  eGPIO_G3 = 0x200,       // muxing bit : 4,    offset : 0x20*0x10

  eGPIO_I = 0x220,               // muxing bit : 4,     offset : 0x22*0x10
  eGPIO_J0 = 0x240,       // muxing bit : 4,    offset : 0x24*0x10
  eGPIO_J1 = 0x260,       // muxing bit : 4,    offset : 0x26*0x10
  eGPIO_J2 = 0x280,       // muxing bit : 4,    offset : 0x28*0x10
  eGPIO_J3 = 0x2A0,       // muxing bit : 4,    offset : 0x2A*0x10
  eGPIO_J4 = 0x2C0,       // muxing bit : 4,    offset : 0x2C*0x10

  eGPIO_MP0_1 = 0x2E0,        // muxing bit : 4,    offset : 0x2E*0x10
  eGPIO_MP0_2 = 0x300,        // muxing bit : 4,    offset : 0x30*0x10
  eGPIO_MP0_3 = 0x320,        // muxing bit : 4,    offset : 0x32*0x10
  eGPIO_MP0_4 = 0x340,        // muxing bit : 4,    offset : 0x34*0x10
  eGPIO_MP0_5 = 0x360,        // muxing bit : 4,    offset : 0x36*0x10
  eGPIO_MP0_6 = 0x380,        // muxing bit : 4,    offset : 0x38*0x10
  eGPIO_MP0_7 = 0x3A0,        // muxing bit : 4,    offset : 0x3A*0x10

  eGPIO_MP1_0 = 0x3C0,       // muxing bit : 4,    offset : 0x3C*0x10
  eGPIO_MP1_1 = 0x3E0,        // muxing bit : 4,    offset : 0x3E*0x10
  eGPIO_MP1_2 = 0x400,        // muxing bit : 4,    offset : 0x40*0x10
  eGPIO_MP1_3 = 0x420,        // muxing bit : 4,    offset : 0x42*0x10
  eGPIO_MP1_4 = 0x440,        // muxing bit : 4,    offset : 0x44*0x10
  eGPIO_MP1_5 = 0x460,        // muxing bit : 4,    offset : 0x46*0x10
  eGPIO_MP1_6 = 0x480,        // muxing bit : 4,    offset : 0x48*0x10
  eGPIO_MP1_7 = 0x4A0,        // muxing bit : 4,    offset : 0x4A*0x10
  eGPIO_MP1_8 = 0x4C0,        // muxing bit : 4,    offset : 0x4C*0x10

  eGPIO_MP2_0 = 0x4E0,        // muxing bit : 4,    offset : 0x4E*0x10
  eGPIO_MP2_1 = 0x500,        // muxing bit : 4,    offset : 0x50*0x10
  eGPIO_MP2_2 = 0x520,        // muxing bit : 4,    offset : 0x52*0x10
  eGPIO_MP2_3 = 0x540,        // muxing bit : 4,    offset : 0x54*0x10
  eGPIO_MP2_4 = 0x560,        // muxing bit : 4,    offset : 0x56*0x10
  eGPIO_MP2_5 = 0x580,        // muxing bit : 4,  , offset : 0x58*0x10
  eGPIO_MP2_6 = 0x5A0,        // muxing bit : 4,    offset : 0x5A*0x10
  eGPIO_MP2_7 = 0x5C0,        // muxing bit : 4,    offset : 0x5C*0x10
  eGPIO_MP2_8 = 0x5E0,        // muxing bit : 4,    offset : 0x5E*0x10

  eETC0 = 0x608,        // offset(3bit) : 0x608
  eETC1 = 0x628,        // offset(3bit) : 0x628
  eETC2 = 0x648,        // offset(3bit) : 0x648
  eETC3 = 0x668,        // offset(3bit) : 0x668
  eETC4 = 0x688,        // offset(3bit) : 0x688

  eGPIO_H0 = 0xC00,       // muxing bit : 4,    offset : 0xC0*0x10
  eGPIO_H1 = 0xC20,       // muxing bit : 4,    offset : 0xC2*0x10
  eGPIO_H2 = 0xC40,       // muxing bit : 4,    offset : 0xC4*0x10
  eGPIO_H3 = 0xC60,       // muxing bit : 4,    offset : 0xC6*0x10

}
GPIO_eId;

void GPIO_SetFunctionAll(GPIO_eId Id, u32 uValue0);
void GPIO_SetPullUpDownAll(GPIO_eId Id, u32 uValue);

void GPIO_SetDSAll(GPIO_eId Id, u32 uValue);


#endif
