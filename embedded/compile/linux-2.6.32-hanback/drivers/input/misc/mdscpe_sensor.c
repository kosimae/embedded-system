//[*]--------------------------------------------------------------------------------------------------[*]
//
//  HBE-SM3V210 Board : MDS-CPE(Compass) Sensor Interface driver (by phs)
//  2010.10.21
//
//[*]--------------------------------------------------------------------------------------------------[*]
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <mach/irqs.h>
#include <asm/system.h>
#include <asm/ioctl.h>

#include <mach/regs-gpio.h>
#include <linux/delay.h>

#include "mdscpe_sensor.h"

#define MDSCPE_MAJOR    300
#define MDSCPE_NAME     "MDS-CPE Compass Sensor"

static int __init       mdscpe_sensor_init			(void);
static void __exit      mdscpe_sensor_exit			(void);
extern struct mdscpe_cal_data caldata;

static int	mdscpe_sensor_open	(struct inode *minode, struct file *mfile)
{
	#if	defined(COMPASS_DEBUG)
		printk("%s\n", __FUNCTION__);
	#endif

	// GPIO Init 
	mdscpe_port_init();
  mdscpe_reset();
	return	0;
}

static ssize_t mdscpe_sensor_write(struct file *file, const char __user *data,
            size_t len, loff_t *ppos)
{
  char buf[10];
  int i;

  for(i=0;i<len;i++) {
  if(get_user(buf[i],data+i))
    return -EFAULT;
  }

  caldata.xoff = buf[1]<<8;   caldata.xoff  += buf[0];
  caldata.yoff = buf[3]<<8;   caldata.yoff  += buf[2];
  caldata.xsc  = buf[5]<<8;   caldata.xsc   += buf[4];
  caldata.ysc  = buf[7]<<8;   caldata.ysc   += buf[6];
  caldata.k    = buf[9]<<8;   caldata.k     += buf[8];

#if 0
  printk("#################################### \n");
  for(i=0;i<len;i++)
    printk("buf[%d] = %x", i, buf[i]);
  printk("#################################### \n");
  printk(" XXoff = %x , YYoff = %x , XXSC = %x, YYSC = %x, KK = %x\n", XXoff, YYoff, XXSC,YYSC,KK);
#endif
}

static ssize_t mdscpe_sensor_release(struct inode *minode, struct file *mfile)
{
	#if	defined(COMPASS_DEBUG)
		printk("%s\n", __FUNCTION__);
	#endif

  return 0;
}

static int mdscpe_sensor_ioctl(struct inode *inode, struct file *flip, unsigned int cmd, unsigned long arg)
{
#if	defined(COMPASS_DEBUG)
  printk("%s\n", __FUNCTION__);
#endif

  short data;
  int size;

  size = _IOC_SIZE(cmd);

  switch(cmd) {
    case SENSOR_CALIB_START:
      mdscpe_calib_init();
      break;
    case SENSOR_CALIB_WRITE:
      write_data_Code(0xe);
      break;
    case SENSOR_CALIB_READ:
      read_data_Code(0xc);
      copy_to_user((void *)arg, (const void *)&caldata, (unsigned long) size);
      break;
    case SENSOR_DATA_READ:
      data = mdscpe_send_command(0x6);
      copy_to_user((void *)arg, (const short *)&data, (unsigned long) size);
      break;
  }
}

struct file_operations mdscpe_fops = {
  .owner    = THIS_MODULE,
  .open     = mdscpe_sensor_open,
  .write    = mdscpe_sensor_write,
  .ioctl    = mdscpe_sensor_ioctl,
  .release  = mdscpe_sensor_release,
};

static int __init	mdscpe_sensor_init	(void)
{
	#if	defined(COMPASS_DEBUG)
		printk("%s\n",__FUNCTION__);
	#endif

  int i = 0;

  int result;

  result = register_chrdev(MDSCPE_MAJOR,MDSCPE_NAME,&mdscpe_fops);
  if(result < 0) {
    printk(KERN_WARNING"Can't get any major\n");
    return result;
  }

  printk("HBE-SM3-SV210 MDS-CPE Device Driver initialized!! Ver 1.0[%d] \n", MDSCPE_MAJOR);
  return 0;
}

static void __exit	mdscpe_sensor_exit	(void)
{
	#if	defined(COMPASS_DEBUG)
		printk("%s\n",__FUNCTION__);
	#endif

  unregister_chrdev(MDSCPE_MAJOR,MDSCPE_NAME);
}

module_init(mdscpe_sensor_init);
module_exit(mdscpe_sensor_exit);

MODULE_AUTHOR("Hanback");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MDS-CPE(Compass Sensor) interface for HBE-SM3V210 Board");
