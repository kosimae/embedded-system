#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "./def.h"

#define Outp32(addr, data)  (*(volatile u32 *)(addr) = (data))
#define Outp16(addr, data)  (*(volatile u16 *)(addr) = (data))
#define Outp8(addr, data) (*(volatile u8 *)(addr) = (data))
#define Inp32(addr)     (*(volatile u32 *)(addr))
#define Inp32_(addr, data) (data = (*(volatile u32 *)(addr))) //jhy20090326 for code merge

//#define Assert(a) {if (!(a)) {printf("\n %s(line %d)\n", __FILE__, __LINE__); exit(0);}}
#define Assert(x) printf("error\n");

//// Multimedia
#define LCDC_BASE       (0xF8000000)
#endif
