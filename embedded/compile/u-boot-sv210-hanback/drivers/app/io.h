#ifndef IO_H
#define IO_H

#define readb(c)	(*(volatile u8  *)(c))
#define writeb(c, v)	(*(volatile u8  *)(c) = (v))

#define __REG8 *(volatile unsigned char *)
#define __REG16 *(volatile unsigned short *)

#define SWAP32(A)   ((((A)&0x000000ff)<<24) | (((A)&0x0000ff00)<<8) | (((A)&0x00ff0000)>>8) | (((A)&0xff000000)>>24))
#endif
