#include <regs.h>
#include "./fpga_io.h"

int Getsegcode(int x)
{
  char code;
  switch (x) {
    case 0x0 : code = 0xfc; break;
    case 0x1 : code = 0x60; break;
    case 0x2 : code = 0xda; break;
    case 0x3 : code = 0xf2; break;
    case 0x4 : code = 0x66; break;
    case 0x5 : code = 0xb6; break;
    case 0x6 : code = 0xbe; break;
    case 0x7 : code = 0xe4; break;
    case 0x8 : code = 0xfe; break;
    case 0x9 : code = 0xf6; break;
    case 0xa : code = 0xfa; break;
    case 0xb : code = 0x3e; break;
    case 0xc : code = 0x1a; break;
    case 0xd : code = 0x7a; break;
    case 0xe : code = 0x9e; break;
    case 0xf : code = 0x8e; break;
    default  : code = 0;    break;
  }
  return code;
}

int seg_c_main(void)
{
  int i,j,k;
  char ch;
  unsigned int count=0,temp1,temp2;
  char data[6];
  unsigned int digit[6] ={0x20,0x10,0x08,0x04,0x02,0x01};
  unsigned char * seg_addr_grid, * seg_addr_data;

  /* clear sed */
  seg_addr_grid = (unsigned char *)ADDR_OF_7SEG_GRID;
  seg_addr_data = (unsigned char *)ADDR_OF_7SEG_DATA;

  *seg_addr_grid = 0x00;
  *seg_addr_data = 0x00;

  for(i=0; i<0xf; i++) {
    for(j=0; j<6; j++)
      data[j] = Getsegcode(i);

    for(k=0; k<50; k++) {
      for(j=0; j<6; j++) {
        *seg_addr_grid = ~digit[j];
        *seg_addr_data = data[j];
        udelay(1000);
      }
    }
  }

  /* clear sed */
  *seg_addr_grid = 0x00;
  *seg_addr_data = 0x00;

  return 0;
}
