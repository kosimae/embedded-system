/*
 * CDS Device Driver
 *  Hanback Electronics Co.,ltd
 * File : cds.c
 * Date : April,2009
 */ 

// 모듈의 헤더파일 선언
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
#define ADC_CHANNEL         0 /* index for s5pC110 9��~R��~R channel adc */

#define CDS_BASE            0xbd
#define IOCTL_CDS_READ      _IOR(CDS_BASE,0,int)

#define DRIVER_AUTHOR       "hanback"	// 모듈의 저작자
#define DRIVER_DESC		      "cds program" // 모듈에 대한 설명

#define	CDS_MAJOR_NUMBER	  301		// 디바이스 주번호
#define	CDS_NAME		        "CDS"		// 디바이스 이름

//Global variable
static unsigned int   cds_usage = 0;
static unsigned long  cds_ioremap;

// define functions...
// 응용 프로그램에서 디바이스를 처음 사용하는 경우를 처리하는 함수
int cds_open (struct inode *inode, struct file *filp)
{
	// 디바이스가 열려 있는지 확인.
	if(cds_usage != 0) return -EBUSY;

  /* Setting GPG2_3 => '1' */
  s3c_gpio_cfgpin(S5PV210_GPG2(3), S3C_GPIO_SFN(2));
  s3c_gpio_setpull(S5PV210_GPG2(3), S3C_GPIO_PULL_UP);

  cds_usage = 1;

	return 0;
}

// 응용 프로그램에서 디바이스를 더이상 사용하지 않아서 닫기를 구현하는 함수
int cds_release (struct inode *inode, struct file *filp)
{
	cds_usage = 0;
	return 0;
}

// 디바이스 드라이버의 읽기를 구현하는 함수
ssize_t cds_read(struct file *inode, char *gdata, size_t length, loff_t *off_what)
{
	int ret, value;

  value = s3c_adc_get_adc_data(ADC_CHANNEL);

	// value가 가리키는 커널 메모리 데이터를 gdata가 가리키는 사용자
	// 메모리 데이터에 n 바이트 만큼 써넣는다.
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

// 파일 오퍼레이션 구조체
// 파일을 열때 open()을 사용한다. open()는 시스템 콜을 호출하여 커널 내부로 들어간다.
// 해당 시스템 콜과 관련된 파일 연산자 구조체 내부의 open에 해당하는 필드가 드라이버 내에서
// cds_open()으로 정의되어 있으므로 cds_open()가 호출된다.
// write, read, release도 마찬가지로 동작한다.
struct file_operations cds_fops = 
{
  .owner		= THIS_MODULE,
  .open		  = cds_open,
  .read		  = cds_read,
  .ioctl    = cds_ioctl,
  .release	= cds_release,
};

// 모듈을 커널 내부로 삽입
// 모듈 프로그램의 핵심적인 목적은 커널 내부로 들어가서 서비스를 제공받는 것이므로
// 커널 내부로 들어가는 init()을 먼저 시작한다.
// 응용 프로그램은 소스 내부에서 정의되지 않은 많은 함수를 사용한다. 그것은 외부
// 라이브러리가 컴파일 과정에서 링크되어 사용되기 때문이다. 모듈 프로그램은 커널
// 내부하고만 링크되기 때문에 커널에서 정의하고 허용하는 함수만을 사용할 수 있다.
int cds_init(void)
{
	int result;

	// 문자 디바이스 드라이버를 등록한다.
	result = register_chrdev(CDS_MAJOR_NUMBER, CDS_NAME, &cds_fops);
	if (result < 0) {  // 등록실패
		printk(KERN_WARNING" %s HBE-SM3-SV210 CDS Device Driver initialized Faild\n",__FUNCTION__);
		return result;
	}
	
	// major 번호를 출력한다.
  printk("HBE-SM3-SV210 CDS Device Driver initialized Ver!! 1.0[%d] \n", CDS_MAJOR_NUMBER);
	return 0;
}

// 모듈을 커널에서 제거	
void cds_exit(void)
{
	// 문자 디바이스 드라이버를 제거한다.
	unregister_chrdev(CDS_MAJOR_NUMBER,CDS_NAME);

	printk("driver: %s DRIVER EXIT\n", CDS_NAME);
}

module_init(cds_init);	// 모듈 적재 시 호출되는 함수
module_exit(cds_exit);	// 모듈 제거 시 호출되는 함수

MODULE_AUTHOR(DRIVER_AUTHOR);	// 모듈의 저작자
MODULE_DESCRIPTION(DRIVER_DESC); // 모듈에 대한 설명
MODULE_LICENSE("Dual BSD/GPL");	 // 모듈의 라이선스 등록

