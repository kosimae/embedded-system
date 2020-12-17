/*
 * Proximity Device Driver
 *  Hanback Electronics Co.,ltd
 * File : proximity.c
 * Date : April,2009
 */ 

// ����� ������� ����
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
#define ADC_CHANNEL  1 /* index for s5pC110 9�~R�~R channel adc */

#define PROXIMITY_BASE          0xbc
#define IOCTL_PROXIMITY_READ   _IOR(PROXIMITY_BASE,0,int)

#define PROXIMITY_PWM_CH 1
#define PERIOD_NS 35714                         /* 1000000000 / 35714 => 28 Khz */
#define DUTY_NS 1000*18                         /* 1000(1 us) * 18 => 18 us */

static struct pwm_device *proximity_pwm;
static unsigned int proximity_period_ns;
static unsigned int proximity_duty_ns;

#define DRIVER_AUTHOR       "hanback"	          // ����� ������
#define DRIVER_DESC		      "proximity program" // ��⿡ ���� ����

#define	PROXIMITY_MAJOR_NUMBER	  302		              // ����̽� �ֹ�ȣ
#define	PROXIMITY_NAME		        "PROXIMITY"		      // ����̽� �̸�

#define TIMER_INTERVAL		  20

//Global variable
static unsigned int proximity_usage = 0;

// define functions...
// ���� ���α׷����� ����̽��� ó�� ����ϴ� ��츦 ó���ϴ� �Լ�
int proximity_open (struct inode *inode, struct file *filp)
{
  int ret;
	// ����̽��� ���� �ִ��� Ȯ��.
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

// ���� ���α׷����� ����̽��� ���̻� ������� �ʾƼ� �ݱ⸦ �����ϴ� �Լ�
int proximity_release (struct inode *inode, struct file *filp)
{
  pwm_config(proximity_pwm, 0, proximity_period_ns);
  pwm_disable(proximity_pwm);
  pwm_free(proximity_pwm);
	proximity_usage = 0;
	return 0;
}

// ����̽� ����̹��� �б⸦ �����ϴ� �Լ�
ssize_t proximity_read(struct file *inode, char *gdata, size_t length, loff_t *off_what)
{
	int ret, value;

  value = s3c_adc_get_adc_data(ADC_CHANNEL);

  //printk("adc = %d \n",value);

	// value�� ����Ű�� Ŀ�� �޸� �����͸� gdata�� ����Ű�� �����
	// �޸� �����Ϳ� n ����Ʈ ��ŭ ��ִ´�.
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

// ���� ���۷��̼� ����ü
// ������ ���� open()�� ����Ѵ�. open()�� �ý��� ���� ȣ���Ͽ� Ŀ�� ���η� ����.
// �ش� �ý��� �ݰ� ���õ� ���� ������ ����ü ������ open�� �ش��ϴ� �ʵ尡 ����̹� ������
// proximity_open()���� ���ǵǾ� �����Ƿ� proximity_open()�� ȣ��ȴ�.
// write, read, release�� ���������� �����Ѵ�.
struct file_operations proximity_fops = 
{
  .owner		= THIS_MODULE,
  .open		  = proximity_open,
  .read		  = proximity_read,
  .ioctl    = proximity_ioctl,
  .release	= proximity_release,
};

// ����� Ŀ�� ���η� ����
// ��� ���α׷��� �ٽ����� ������ Ŀ�� ���η� ���� ���񽺸� �����޴� ���̹Ƿ�
// Ŀ�� ���η� ���� init()�� ���� �����Ѵ�.
// ���� ���α׷��� �ҽ� ���ο��� ���ǵ��� ���� ���� �Լ��� ����Ѵ�. �װ��� �ܺ�
// ���̺귯���� ������ �������� ��ũ�Ǿ� ���Ǳ� �����̴�. ��� ���α׷��� Ŀ��
// �����ϰ� ��ũ�Ǳ� ������ Ŀ�ο��� �����ϰ� ����ϴ� �Լ����� ����� �� �ִ�.
int proximity_init(void)
{
	int result;

	// ���� ����̽� ����̹��� ����Ѵ�.
	result = register_chrdev(PROXIMITY_MAJOR_NUMBER, PROXIMITY_NAME, &proximity_fops);
	if (result < 0) {  // ��Ͻ���
		printk(KERN_WARNING"%s HBE-SM3-SV210 PROXIMITY Device Driver initialized Faild\n", __FUNCTION__);
		return result;
	}
	
	// major ��ȣ�� ����Ѵ�.
  printk("HBE-SM3-SV210 PROXIMITY Device Driver initialized Ver!! 1.0[%d] \n", PROXIMITY_MAJOR_NUMBER);
	return 0;
}

// ����� Ŀ�ο��� ����	
void proximity_exit(void)
{
	// ���� ����̽� ����̹��� �����Ѵ�.
	unregister_chrdev(PROXIMITY_MAJOR_NUMBER,PROXIMITY_NAME);

	printk("driver: %s DRIVER EXIT\n", PROXIMITY_NAME);
}

module_init(proximity_init);	// ��� ���� �� ȣ��Ǵ� �Լ�
module_exit(proximity_exit);	// ��� ���� �� ȣ��Ǵ� �Լ�

MODULE_AUTHOR(DRIVER_AUTHOR);	// ����� ������
MODULE_DESCRIPTION(DRIVER_DESC); // ��⿡ ���� ����
MODULE_LICENSE("Dual BSD/GPL");	 // ����� ���̼��� ���

