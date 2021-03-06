/*
 * Proximity Device Driver
 *  Hanback Electronics Co.,ltd
 * File : proximity.c
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

#include <linux/pwm.h>

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);
#define ADC_CHANNEL  1 /* index for s5pC110 9��~R��~R channel adc */

#define PROXIMITY_BASE          0xbc
#define IOCTL_PROXIMITY_READ   _IOR(PROXIMITY_BASE,0,int)

#define PROXIMITY_PWM_CH 1
#define PERIOD_NS 35714                         /* 1000000000 / 35714 => 28 Khz */
#define DUTY_NS 1000*18                         /* 1000(1 us) * 18 => 18 us */

static struct pwm_device *proximity_pwm;
static unsigned int proximity_period_ns;
static unsigned int proximity_duty_ns;

#define DRIVER_AUTHOR       "hanback"	          // 모듈의 저작자
#define DRIVER_DESC		      "proximity program" // 모듈에 대한 설명

#define	PROXIMITY_MAJOR_NUMBER	  302		              // 디바이스 주번호
#define	PROXIMITY_NAME		        "PROXIMITY"		      // 디바이스 이름

#define TIMER_INTERVAL		  20

//Global variable
static unsigned int proximity_usage = 0;

// define functions...
// 응용 프로그램에서 디바이스를 처음 사용하는 경우를 처리하는 함수
int proximity_open (struct inode *inode, struct file *filp)
{
  int ret;
	// 디바이스가 열려 있는지 확인.
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

// 응용 프로그램에서 디바이스를 더이상 사용하지 않아서 닫기를 구현하는 함수
int proximity_release (struct inode *inode, struct file *filp)
{
  pwm_config(proximity_pwm, 0, proximity_period_ns);
  pwm_disable(proximity_pwm);
  pwm_free(proximity_pwm);
	proximity_usage = 0;
	return 0;
}

// 디바이스 드라이버의 읽기를 구현하는 함수
ssize_t proximity_read(struct file *inode, char *gdata, size_t length, loff_t *off_what)
{
	int ret, value;

  value = s3c_adc_get_adc_data(ADC_CHANNEL);

  //printk("adc = %d \n",value);

	// value가 가리키는 커널 메모리 데이터를 gdata가 가리키는 사용자
	// 메모리 데이터에 n 바이트 만큼 써넣는다.
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

// 파일 오퍼레이션 구조체
// 파일을 열때 open()을 사용한다. open()는 시스템 콜을 호출하여 커널 내부로 들어간다.
// 해당 시스템 콜과 관련된 파일 연산자 구조체 내부의 open에 해당하는 필드가 드라이버 내에서
// proximity_open()으로 정의되어 있으므로 proximity_open()가 호출된다.
// write, read, release도 마찬가지로 동작한다.
struct file_operations proximity_fops = 
{
  .owner		= THIS_MODULE,
  .open		  = proximity_open,
  .read		  = proximity_read,
  .ioctl    = proximity_ioctl,
  .release	= proximity_release,
};

// 모듈을 커널 내부로 삽입
// 모듈 프로그램의 핵심적인 목적은 커널 내부로 들어가서 서비스를 제공받는 것이므로
// 커널 내부로 들어가는 init()을 먼저 시작한다.
// 응용 프로그램은 소스 내부에서 정의되지 않은 많은 함수를 사용한다. 그것은 외부
// 라이브러리가 컴파일 과정에서 링크되어 사용되기 때문이다. 모듈 프로그램은 커널
// 내부하고만 링크되기 때문에 커널에서 정의하고 허용하는 함수만을 사용할 수 있다.
int proximity_init(void)
{
	int result;

	// 문자 디바이스 드라이버를 등록한다.
	result = register_chrdev(PROXIMITY_MAJOR_NUMBER, PROXIMITY_NAME, &proximity_fops);
	if (result < 0) {  // 등록실패
		printk(KERN_WARNING"%s HBE-SM3-SV210 PROXIMITY Device Driver initialized Faild\n", __FUNCTION__);
		return result;
	}
	
	// major 번호를 출력한다.
  printk("HBE-SM3-SV210 PROXIMITY Device Driver initialized Ver!! 1.0[%d] \n", PROXIMITY_MAJOR_NUMBER);
	return 0;
}

// 모듈을 커널에서 제거	
void proximity_exit(void)
{
	// 문자 디바이스 드라이버를 제거한다.
	unregister_chrdev(PROXIMITY_MAJOR_NUMBER,PROXIMITY_NAME);

	printk("driver: %s DRIVER EXIT\n", PROXIMITY_NAME);
}

module_init(proximity_init);	// 모듈 적재 시 호출되는 함수
module_exit(proximity_exit);	// 모듈 제거 시 호출되는 함수

MODULE_AUTHOR(DRIVER_AUTHOR);	// 모듈의 저작자
MODULE_DESCRIPTION(DRIVER_DESC); // 모듈에 대한 설명
MODULE_LICENSE("Dual BSD/GPL");	 // 모듈의 라이선스 등록

