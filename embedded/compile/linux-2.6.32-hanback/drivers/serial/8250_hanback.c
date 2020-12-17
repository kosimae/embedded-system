/*
 *  linux/drivers/serial/8250_hanback.c
 *
 *  Written by Paul B Schroeder < pschroeder "at" uplogix "dot" com >
 *  Based on 8250_boca.
 *
 *  Copyright (C) 2005 Russell King.
 *  Data taken from include/asm-i386/serial.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <mach/map.h>
#include <asm/mach/irq.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/s5pv210.h>

#define PORT(_irq, _mapbase)				\
	{						\
		.irq		= _irq,			\
		.mapbase	= _mapbase,		\
		.uartclk	= 14745600,		\
		.iotype		= UPIO_MEM,		\
		.flags		= UPF_IOREMAP,		\
	}

static struct plat_serial8250_port exar_data[] = {
#if defined(CONFIG_MACH_EMPOS3)
  PORT(IRQ_EINT(28), HANBACK_ST16C554_UART0),
	PORT(IRQ_EINT(29), HANBACK_ST16C554_UART1),
	PORT(IRQ_EINT(30), HANBACK_ST16C554_UART2),
	PORT(IRQ_EINT(31), HANBACK_ST16C554_UART3),
#else /* CONFIG_MACH_SM3SV210 */
  PORT(IRQ_EINT(5), HANBACK_ST16C554_UART0),
	PORT(IRQ_EINT(7), HANBACK_ST16C554_UART1),
	PORT(IRQ_EINT(16), HANBACK_ST16C554_UART2),
	PORT(IRQ_EINT(17), HANBACK_ST16C554_UART3),
#endif
#if CONFIG_SERIAL_8250_EXAR_ST16C550_HANBACK
	PORT(IRQ_EINT(12), HANBACK_ST16C550_UART0),
#endif
	{ },
};

static struct platform_device exar_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_EXAR_ST16C554,
	.dev			= {
		.platform_data	= exar_data,
	},
};

static int __init exar_init(void)
{
  u32 err;
#if defined(CONFIG_MACH_EMPOS3)
  /* XEINT[28] Setting */
  err = gpio_request(S5PV210_GPH3(4), "GPH3");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH3(4), (0xf << 16));
    s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(28), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(28), 1);

  /* XEINT[29] Setting */
  err = gpio_request(S5PV210_GPH3(5), "GPH3");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH3(5), (0xf << 20));
    s3c_gpio_setpull(S5PV210_GPH3(5), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(29), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(29), 1);

  /* XEINT[30] Setting */
  err = gpio_request(S5PV210_GPH3(6), "GPH3");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH3(6), (0xf << 24));
    s3c_gpio_setpull(S5PV210_GPH3(6), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(30), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(30), 1);

  /* XEINT[31] Setting */
  err = gpio_request(S5PV210_GPH3(7), "GPH3");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH3(7), (0xf << 28));
    s3c_gpio_setpull(S5PV210_GPH3(7), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(31), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(31), 1);
#else /* CONFIG_MACH_SM3SV210 */
  /* XEINT[5] Setting */
  err = gpio_request(S5PV210_GPH0(5), "GPH0");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH0(5), (0xf << 20));
    s3c_gpio_setpull(S5PV210_GPH0(5), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(5), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(5), 1);

  /* XEINT[7] Setting */
  err = gpio_request(S5PV210_GPH0(7), "GPH0");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH0(7), (0xf << 28));
    s3c_gpio_setpull(S5PV210_GPH0(7), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(7), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(7), 1);

  /* XEINT[16] Setting */
  err = gpio_request(S5PV210_GPH2(0), "GPH2");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH2(0), (0xf << 0));
    s3c_gpio_setpull(S5PV210_GPH2(0), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(16), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(16), 1);

  /* XEINT[17] Setting */
  err = gpio_request(S5PV210_GPH2(1), "GPH2");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH2(1), (0xf << 4));
    s3c_gpio_setpull(S5PV210_GPH2(1), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(17), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(17), 1);
#endif

#if CONFIG_SERIAL_8250_EXAR_ST16C550_HANBACK
  /* XEINT[12] Setting */
  err = gpio_request(S5PV210_GPH1(4), "GPH1");
  if (err) {
    printk(KERN_INFO "gpio request error : %d\n", err);
  } else {
    s3c_gpio_cfgpin(S5PV210_GPH1(4), (0xf << 16));
    s3c_gpio_setpull(S5PV210_GPH1(4), S3C_GPIO_PULL_NONE);
  }
  set_irq_type(IRQ_EINT(12), IRQ_TYPE_EDGE_RISING);
  set_irq_wake(IRQ_EINT(12), 1);
#endif

	return platform_device_register(&exar_device);
}

module_init(exar_init);

MODULE_AUTHOR("Paul B Schroeder");
MODULE_DESCRIPTION("8250 serial probe module for Exar cards");
MODULE_LICENSE("GPL");
