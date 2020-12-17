/* linux/arch/arm/mach-s5pv210/button-smdkv210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Button Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <plat/map-base.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-gpio.h>
#include <mach/regs-irq.h>
#include <linux/gpio.h>

static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{
	if (irq == IRQ_EINT4)
		printk(KERN_INFO "XEINT 4 Button Interrupt occure\n");
#if defined(CONFIG_MACH_EMPOS3) || defined(CONFIG_MACH_ADKSV210)
	else if (irq == IRQ_EINT11)
		printk(KERN_INFO "XEINT 11 Button Interrupt occure\n");
#else
	else if (irq == IRQ_EINT(31))
		printk(KERN_INFO "XEINT 31 Button Interrupt occure\n");
#endif
	else
		printk(KERN_INFO "%d Button Interrupt occure\n", irq);

	return IRQ_HANDLED;
}

#if defined(CONFIG_MACH_ADKSV210)
static irqreturn_t
s3c_16_button_interrupt(int irq, void *dev_id)
{
    printk(" 16 Button Interrupt occure\n");

      return IRQ_HANDLED;
}

static irqreturn_t
s3c_17_button_interrupt(int irq, void *dev_id)
{
    printk(" 17 Button Interrupt occure\n");

      return IRQ_HANDLED;
}

static irqreturn_t
s3c_18_button_interrupt(int irq, void *dev_id)
{
    printk(" 18 Button Interrupt occure\n");

      return IRQ_HANDLED;
}
#endif

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= IRQF_SHARED ,
	.handler	= s3c_button_interrupt,
};

#if defined(CONFIG_MACH_ADKSV210)
static struct irqaction s3c_16_button_irq = {
  .name   = "s3c 16 button Tick",
  .flags    = IRQF_SHARED ,
  .handler  = s3c_16_button_interrupt,
};

static struct irqaction s3c_17_button_irq = {
  .name   = "s3c 17 button Tick",
  .flags    = IRQF_SHARED ,
  .handler  = s3c_17_button_interrupt,
};

static struct irqaction s3c_18_button_irq = {
  .name   = "s3c 18 button Tick",
  .flags    = IRQF_SHARED ,
  .handler  = s3c_18_button_interrupt,
};
#endif

static unsigned int s3c_button_gpio_init(void)
{
	u32 err;
#ifndef CONFIG_REGULATOR
	err = gpio_request(S5PV210_GPH0(4), "GPH0");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5PV210_GPH0(4), (0xf << 16));
		s3c_gpio_setpull(S5PV210_GPH0(4), S3C_GPIO_PULL_NONE);
	}
#endif
#if defined(CONFIG_MACH_EMPOS3) || defined(CONFIG_MACH_ADKSV210)
  /* XEINT[11] Setting */
	err = gpio_request(S5PV210_GPH1(3), "GPH1");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5PV210_GPH1(3), (0xf << 12));
		s3c_gpio_setpull(S5PV210_GPH1(3), S3C_GPIO_PULL_NONE);
	}
#else
	err = gpio_request(S5PV210_GPH3(7), "GPH3");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5PV210_GPH3(7), (0xf << 28));
		s3c_gpio_setpull(S5PV210_GPH3(7), S3C_GPIO_PULL_NONE);
	}
#endif

#if defined(CONFIG_MACH_ADKSV210)
  /* XEINT[16] Setting */
  err = gpio_request(S5PV210_GPH2(0), "GPH2");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH2(0), (0xf << 0));
    s3c_gpio_setpull(S5PV210_GPH2(0), S3C_GPIO_PULL_NONE);
  }

  /* XEINT[17] Setting */
  err = gpio_request(S5PV210_GPH2(1), "GPH2");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH2(1), (0xf << 4));
    s3c_gpio_setpull(S5PV210_GPH2(1), S3C_GPIO_PULL_NONE);
  }

  /* XEINT[18] Setting */
  err = gpio_request(S5PV210_GPH2(2), "GPH2");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH2(2), (0xf << 8));
    s3c_gpio_setpull(S5PV210_GPH2(2), S3C_GPIO_PULL_NONE);
  }
#endif
	return err;
}

static int __init s3c_button_init(void)
{
	printk(KERN_INFO "SMDKC110 Button init function \n");

	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __func__);
		return 0;
	}
#ifndef CONFIG_REGULATOR
	set_irq_type(IRQ_EINT4, IRQF_TRIGGER_FALLING);
	set_irq_wake(IRQ_EINT4, 1);
	setup_irq(IRQ_EINT4, &s3c_button_irq);
#endif

#if defined(CONFIG_MACH_EMPOS3) || defined(CONFIG_MACH_ADKSV210)
  /* XEINT[11] Setting */
	set_irq_type(IRQ_EINT11, IRQ_TYPE_EDGE_FALLING);
	set_irq_wake(IRQ_EINT11, 1);
	setup_irq(IRQ_EINT11, &s3c_button_irq);
#else
	set_irq_type(IRQ_EINT(31), IRQ_TYPE_EDGE_FALLING);
	set_irq_wake(IRQ_EINT(31), 1);
	setup_irq(IRQ_EINT(31), &s3c_button_irq);
#endif

#if defined(CONFIG_MACH_ADKSV210)
  /* XEINT[16] Setting */
  set_irq_type(IRQ_EINT(16), IRQF_TRIGGER_FALLING);
  set_irq_wake(IRQ_EINT(16), 1);
  setup_irq(IRQ_EINT(16), &s3c_16_button_irq);
  /* XEINT[17] Setting */
  set_irq_type(IRQ_EINT(17), IRQF_TRIGGER_FALLING);
  set_irq_wake(IRQ_EINT(17), 1);
  setup_irq(IRQ_EINT(17), &s3c_16_button_irq);
  /* XEINT[18] Setting */
  set_irq_type(IRQ_EINT(18), IRQF_TRIGGER_FALLING);
  set_irq_wake(IRQ_EINT(18), 1);
  setup_irq(IRQ_EINT(18), &s3c_16_button_irq);
#endif

	return 0;
}
late_initcall(s3c_button_init);
