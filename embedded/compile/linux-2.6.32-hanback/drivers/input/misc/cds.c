/*
 * CDS Device Driver
 *  Hanback Electronics Co.,ltd
 * File : cds.c
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

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);
#define ADC_CHANNEL         0 /* index for s5pC110 9�~R�~R channel adc */

#define CDS_BASE            0xbd
#define IOCTL_CDS_READ      _IOR(CDS_BASE,0,int)

#define DRIVER_AUTHOR       "hanback"	// ����� ������
#define DRIVER_DESC		      "cds program" // ��⿡ ���� ����

#define	CDS_MAJOR_NUMBER	  301		// ����̽� �ֹ�ȣ
#define	CDS_NAME		        "CDS"		// ����̽� �̸�

//Global variable
static unsigned int   cds_usage = 0;
static unsigned long  cds_ioremap;

// define functions...
// ���� ���α׷����� ����̽��� ó�� ����ϴ� ��츦 ó���ϴ� �Լ�
int cds_open (struct inode *inode, struct file *filp)
{
	// ����̽��� ���� �ִ��� Ȯ��.
	if(cds_usage != 0) return -EBUSY;

  /* Setting GPG2_3 => '1' */
  s3c_gpio_cfgpin(S5PV210_GPG2(3), S3C_GPIO_SFN(2));
  s3c_gpio_setpull(S5PV210_GPG2(3), S3C_GPIO_PULL_UP);

  cds_usage = 1;

	return 0;
}

// ���� ���α׷����� ����̽��� ���̻� ������� �ʾƼ� �ݱ⸦ �����ϴ� �Լ�
int cds_release (struct inode *inode, struct file *filp)
{
	cds_usage = 0;
	return 0;
}

// ����̽� ����̹��� �б⸦ �����ϴ� �Լ�
ssize_t cds_read(struct file *inode, char *gdata, size_t length, loff_t *off_what)
{
	int ret, value;

  value = s3c_adc_get_adc_data(ADC_CHANNEL);

	// value�� ����Ű�� Ŀ�� �޸� �����͸� gdata�� ����Ű�� �����
	// �޸� �����Ϳ� n ����Ʈ ��ŭ ��ִ´�.
	ret=copy_to_user(gdata,&value,4);

	if(ret<0) return -1;
	return length;
}

static int cds_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
  int value = 0;
  int size;

  size = _IOC_SIZE(cmd);

  switch (cmd)
  {
    case IOCTL_CDS_READ:
      value = s3c_adc_get_adc_data(ADC_CHANNEL);
      copy_to_user((void *)arg, (const void *)&value, (unsigned long) size);
//      mdelay(100);
//      printk("value=%d\n", value);
  }
  return 0;
}

// ���� ���۷��̼� ����ü
// ������ ���� open()�� ����Ѵ�. open()�� �ý��� ���� ȣ���Ͽ� Ŀ�� ���η� ����.
// �ش� �ý��� �ݰ� ���õ� ���� ������ ����ü ������ open�� �ش��ϴ� �ʵ尡 ����̹� ������
// cds_open()���� ���ǵǾ� �����Ƿ� cds_open()�� ȣ��ȴ�.
// write, read, release�� ���������� �����Ѵ�.
struct file_operations cds_fops = 
{
  .owner		= THIS_MODULE,
  .open		  = cds_open,
  .read		  = cds_read,
  .ioctl    = cds_ioctl,
  .release	= cds_release,
};

// ����� Ŀ�� ���η� ����
// ��� ���α׷��� �ٽ����� ������ Ŀ�� ���η� ���� ���񽺸� �����޴� ���̹Ƿ�
// Ŀ�� ���η� ���� init()�� ���� �����Ѵ�.
// ���� ���α׷��� �ҽ� ���ο��� ���ǵ��� ���� ���� �Լ��� ����Ѵ�. �װ��� �ܺ�
// ���̺귯���� ������ �������� ��ũ�Ǿ� ���Ǳ� �����̴�. ��� ���α׷��� Ŀ��
// �����ϰ� ��ũ�Ǳ� ������ Ŀ�ο��� �����ϰ� ����ϴ� �Լ����� ����� �� �ִ�.
int cds_init(void)
{
	int result;

	// ���� ����̽� ����̹��� ����Ѵ�.
	result = register_chrdev(CDS_MAJOR_NUMBER, CDS_NAME, &cds_fops);
	if (result < 0) {  // ��Ͻ���
		printk(KERN_WARNING" %s HBE-SM3-SV210 CDS Device Driver initialized Faild\n",__FUNCTION__);
		return result;
	}
	
	// major ��ȣ�� ����Ѵ�.
  printk("HBE-SM3-SV210 CDS Device Driver initialized Ver!! 1.0[%d] \n", CDS_MAJOR_NUMBER);
	return 0;
}

// ����� Ŀ�ο��� ����	
void cds_exit(void)
{
	// ���� ����̽� ����̹��� �����Ѵ�.
	unregister_chrdev(CDS_MAJOR_NUMBER,CDS_NAME);

	printk("driver: %s DRIVER EXIT\n", CDS_NAME);
}

module_init(cds_init);	// ��� ���� �� ȣ��Ǵ� �Լ�
module_exit(cds_exit);	// ��� ���� �� ȣ��Ǵ� �Լ�

MODULE_AUTHOR(DRIVER_AUTHOR);	// ����� ������
MODULE_DESCRIPTION(DRIVER_DESC); // ��⿡ ���� ����
MODULE_LICENSE("Dual BSD/GPL");	 // ����� ���̼��� ���

