#include <regs.h>
#include "./fpga_io.h"

int led_test(void)
{
  int i;
  unsigned short * led_addr;
  unsigned char mask;
  unsigned char blink_count=8;

  led_addr = (unsigned short *)ADDR_OF_LED;

  /* clear led */
  *led_addr = 0x00;

  mask = 0x01;

  printf("move right to left\n");
  /* move right to left */
  for (i=0; i < blink_count; i++) {
    *led_addr = 0x100 | (mask << i);
    mdelay(100);
  }

  /* clear led */
  *led_addr = 0x00;
  mask = 0x80;

  printf("move left to right...\n");
  /* move left to right */
  for (i=0; i < blink_count; i++) {
    *led_addr = 0x100 | (mask >> i);
    mdelay(100);
  }

  /* clear led */
  *led_addr = 0x00;
  return 0;
}

