#include <regs.h>
#include <common.h>
#include <asm/io.h>
#include "./fpga_io.h"

#define GPIO_LED_OUTPUT 0x01
#define GPIO_LED_HIGH   0x1
#define GPIO_LED_LOW    0x0

void gpio_led_init_gpio_bit(char bit, char value)
{
  int temp;

  temp = __raw_readl(GPJ0CON);

  if (value == 1)
    temp |= (1 << (bit * 4));
  else
    temp &= ~(1 << (bit * 4));

  __raw_writel(temp,GPJ0CON);
}

void gpio_led_set_gpio_bit(char bit, char value)
{
  int temp;

  temp = __raw_readl(GPJ0DAT);

  if (value == 1)
    temp |= (1 << bit);
  else
    temp &= ~(1 << bit);

  __raw_writel(temp,GPJ0DAT);
}

int gpio_led_test(void)
{
  int i;
  unsigned short * led_addr;
  unsigned char mask;
  unsigned char blink_count=8;

  /* LED[0] ~ LED[7] : GPJ0(0) ~ GPJ0(7) setting output */
  for(i=0; i<8; i++)
  {
    gpio_led_init_gpio_bit(i, GPIO_LED_OUTPUT);
  }

  /* clear led */
  for(i=0; i<8; i++)
  {
    gpio_led_set_gpio_bit(i, GPIO_LED_LOW);
  }

  printf("move right to left\n");
  /* move right to left */
  for (i=0; i < blink_count; i++) {
    gpio_led_set_gpio_bit(i, GPIO_LED_HIGH);
    mdelay(100);
    gpio_led_set_gpio_bit(i, GPIO_LED_LOW);
  }

  /* clear led */  /* clear led */
  for(i=0; i<8; i++)
  {
    gpio_led_set_gpio_bit(i, GPIO_LED_LOW);
  }

  printf("move left to right...\n");
  /* move left to right */
  for (i=blink_count; i >= 0; i--) {
    gpio_led_set_gpio_bit(i, GPIO_LED_HIGH);
    mdelay(100);
    gpio_led_set_gpio_bit(i, GPIO_LED_LOW);
  }

  /* clear led */
  for(i=0; i<8; i++)
  {
    gpio_led_set_gpio_bit(i, GPIO_LED_LOW);
  }

  return 0;
}
