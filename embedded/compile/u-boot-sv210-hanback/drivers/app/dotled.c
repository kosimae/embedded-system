#include <regs.h>
#include "./fpga_io.h"

int dotled_test(void)
{
  unsigned short row_data;
  int i,j,k;

  unsigned short *addr_row, *addr_col;

  addr_row = (unsigned short *)DOTLED_ROW;
  addr_col = (unsigned short *)DOTLED_COL;

  for(i=0; i < 10; i++) {
    *addr_row = 0x01 << i;
    *addr_col = 0x8000 | 0x7f;
    mdelay(100);
  }

  for(j=0; j < 10; j++) {
    *addr_row = 0x200 >> j;
    *addr_col = 0x8000 | 0x7f;
    mdelay(100);
  }

  *addr_row = 0x0000;

  return 0;
}
