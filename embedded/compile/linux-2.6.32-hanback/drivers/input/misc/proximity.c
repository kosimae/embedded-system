/*
 * Proximity Device Driver
 *  Hanback Electronics Co.,ltd
 * File : proximity.c
 * Date : April,2009
 */ 

// ¸ğµâÀÇ Çì´õÆÄÀÏ ¼±¾ğ
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <asm/fcntl.h>
#include <linux/ioport.h>

#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>

#include <linux/pwm.h>

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);
#define ADC_CHANNEL  1 /* index for s5pC110 9â–~Râ–~R channel adc */

#define PROXIMITY_BASE          0xbc
#define IOCTL_PROXIMITY_READ   _IOR(PROXIMITY_BASE,0,int)

#define PROXIMITY_PWM_CH 1
#define PERIOD_NS 35714                         /* 1000000000 / 35714 => 28 Khz */
#define DUTY_NS 1000*18                         /* 1000(1 us) * 18 => 18 us */

static struct pwm_device *proximity_pwm;
static unsigned int proximity_period_ns;
static unsigned int proximity_duty_ns;

#define DRIVER_AUTHOR       "hanback"	          // ¸ğµâÀÇ ÀúÀÛÀÚ
#define DRIVER_DESC		      "proximity program" // ¸ğµâ¿¡ ´ëÇÑ ¼³¸í

#define	PROXIMITY_MAJOR_NUMBER	  302		              // µğ¹ÙÀÌ½º ÁÖ¹øÈ£
#define	PROXIMITY_NAME		        "PROXIMITY"		      // µğ¹ÙÀÌ½º ÀÌ¸§

#define TIMER_INTERVAL		  20

//Global variable
static unsigned int proximity_usage = 0;

// define functions...
// ÀÀ¿ë ÇÁ·Î±×·¥¿¡¼­ µğ¹ÙÀÌ½º¸¦ Ã³À½ »ç¿ëÇÏ´Â °æ¿ì¸¦ Ã³¸®ÇÏ´Â ÇÔ¼ö
int proximity_open (struct inode *inode, struct file *filp)
{
  int ret;
	// µğ¹ÙÀÌ½º°¡ ¿­·Á ÀÖ´ÂÁö È®ÀÎ.
	if(proximity_usage != 0) return -EBUSY;

  proximity_period_ns = PERIOD_NS;
  proximity_duty_ns = DUTY_NS;
  proximity_pwm = pwm_request(PROXIMITY_PWM_CH, "proximity");

  if (IS_ERR(proximity_pwm)) {
    printk("unable to request PWM for proximity\n");
    ret = PTR_ERR(proximity_pwm);
  }
//    printk("got pwm for proximity\n");

  pwm_config(proximity_pwm, proximity_duty_ns, proximity_period_ns);
  pwm_enable(proximity_pwm);

  proximity_usage = 1;

	return 0;

err_pwm:
  pwm_free(proximity_pwm);
  return ret;
}

// ÀÀ¿ë ÇÁ·Î±×·¥¿¡¼­ µğ¹ÙÀÌ½º¸¦ ´õÀÌ»ó »ç¿ëÇÏÁö ¾Ê¾Æ¼­ ´İ±â¸¦ ±¸ÇöÇÏ´Â ÇÔ¼ö
int proximity_release (struct inode *inode, struct file *filp)
{
  pwm_config(proximity_pwm, 0, proximity_period_ns);
  pwm_disable(proximity_pwm);
  pwm_free(proximity_pwm);
	proximity_usage = 0;
	return 0;
}

// µğ¹ÙÀÌ½º µå¶óÀÌ¹öÀÇ ÀĞ±â¸¦ ±¸ÇöÇÏ´Â ÇÔ¼ö
ssize_t proximity_read(struct file *inode, char *gdata, size_t length, loff_t *off_what)
{
	int ret, value;

  value = s3c_adc_get_adc_data(ADC_CHANNEL);

  //printk("adc = %d \n",value);

	// value°¡ °¡¸®Å°´Â Ä¿³Î ¸Ş¸ğ¸® µ¥ÀÌÅÍ¸¦ gdata°¡ °¡¸®Å°´Â »ç¿ëÀÚ
	// ¸Ş¸ğ¸® µ¥ÀÌÅÍ¿¡ n ¹ÙÀÌÆ® ¸¸Å­ ½á³Ö´Â´Ù.
	ret=copy_to_user(gdata,&value,4);

	if(ret<0) return -1;

	return length;
}

static int proximity_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
  int value, size;

  size = _IOC_SIZE(cmd);

  switch (cmd)
  {
    case IOCTL_PROXIMITY_READ:
      value = s3c_adc_get_adc_data(ADC_CHANNEL);
//      mdelay(100);
      //printk("value=%d\n", value);
      copy_to_user((void *)arg, (const void *)&value, (unsigned long) size);
      break;
  }
  return 0;
}

// ÆÄÀÏ ¿ÀÆÛ·¹ÀÌ¼Ç ±¸Á¶Ã¼
// ÆÄÀÏÀ» ¿­¶§ open()À» »ç¿ëÇÑ´Ù. open()´Â ½Ã½ºÅÛ ÄİÀ» È£ÃâÇÏ¿© Ä¿³Î ³»ºÎ·Î µé¾î°£´Ù.
// ÇØ´ç ½Ã½ºÅÛ Äİ°ú °ü·ÃµÈ ÆÄÀÏ ¿¬»êÀÚ ±¸Á¶Ã¼ ³»ºÎÀÇ open¿¡ ÇØ´çÇÏ´Â ÇÊµå°¡ µå¶óÀÌ¹ö ³»¿¡¼­
// proximity_open()À¸·Î Á¤ÀÇµÇ¾î ÀÖÀ¸¹Ç·Î proximity_open()°¡ È£ÃâµÈ´Ù.
// write, read, releaseµµ ¸¶Âù°¡Áö·Î µ¿ÀÛÇÑ´Ù.
struct file_operations proximity_fops = 
{
  .owner		= THIS_MODULE,
  .open		  = proximity_open,
  .read		  = proximity_read,
  .ioctl    = proximity_ioctl,
  .release	= proximity_release,
};

// ¸ğµâÀ» Ä¿³Î ³»ºÎ·Î »ğÀÔ
// ¸ğµâ ÇÁ·Î±×·¥ÀÇ ÇÙ½ÉÀûÀÎ ¸ñÀûÀº Ä¿³Î ³»ºÎ·Î µé¾î°¡¼­ ¼­ºñ½º¸¦ Á¦°ø¹Ş´Â °ÍÀÌ¹Ç·Î
// Ä¿³Î ³»ºÎ·Î µé¾î°¡´Â init()À» ¸ÕÀú ½ÃÀÛÇÑ´Ù.
// ÀÀ¿ë ÇÁ·Î±×·¥Àº ¼Ò½º ³»ºÎ¿¡¼­ Á¤ÀÇµÇÁö ¾ÊÀº ¸¹Àº ÇÔ¼ö¸¦ »ç¿ëÇÑ´Ù. ±×°ÍÀº ¿ÜºÎ
// ¶óÀÌºê·¯¸®°¡ ÄÄÆÄÀÏ °úÁ¤¿¡¼­ ¸µÅ©µÇ¾î »ç¿ëµÇ±â ¶§¹®ÀÌ´Ù. ¸ğµâ ÇÁ·Î±×·¥Àº Ä¿³Î
// ³»ºÎÇÏ°í¸¸ ¸µÅ©µÇ±â ¶§¹®¿¡ Ä¿³Î¿¡¼­ Á¤ÀÇÇÏ°í Çã¿ëÇÏ´Â ÇÔ¼ö¸¸À» »ç¿ëÇÒ ¼ö ÀÖ´Ù.
int proximity_init(void)
{
	int result;

	// ¹®ÀÚ µğ¹ÙÀÌ½º µå¶óÀÌ¹ö¸¦ µî·ÏÇÑ´Ù.
	result = register_chrdev(PROXIMITY_MAJOR_NUMBER, PROXIMITY_NAME, &proximity_fops);
	if (result < 0) {  // µî·Ï½ÇÆĞ
		printk(KERN_WARNING"%s HBE-SM3-SV210 PROXIMITY Device Driver initialized Faild\n", __FUNCTION__);
		return result;
	}
	
	// major ¹øÈ£¸¦ Ãâ·ÂÇÑ´Ù.
  printk("HBE-SM3-SV210 PROXIMITY Device Driver initialized Ver!! 1.0[%d] \n", PROXIMITY_MAJOR_NUMBER);
	return 0;
}

// ¸ğµâÀ» Ä¿³Î¿¡¼­ Á¦°Å	
void proximity_exit(void)
{
	// ¹®ÀÚ µğ¹ÙÀÌ½º µå¶óÀÌ¹ö¸¦ Á¦°ÅÇÑ´Ù.
	unregister_chrdev(PROXIMITY_MAJOR_NUMBER,PROXIMITY_NAME);

	printk("driver: %s DRIVER EXIT\n", PROXIMITY_NAME);
}

module_init(proximity_init);	// ¸ğµâ ÀûÀç ½Ã È£ÃâµÇ´Â ÇÔ¼ö
module_exit(proximity_exit);	// ¸ğµâ Á¦°Å ½Ã È£ÃâµÇ´Â ÇÔ¼ö

MODULE_AUTHOR(DRIVER_AUTHOR);	// ¸ğµâÀÇ ÀúÀÛÀÚ
MODULE_DESCRIPTION(DRIVER_DESC); // ¸ğµâ¿¡ ´ëÇÑ ¼³¸í
MODULE_LICENSE("Dual BSD/GPL");	 // ¸ğµâÀÇ ¶óÀÌ¼±½º µî·Ï

