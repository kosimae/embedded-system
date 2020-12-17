#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
//#include <mach/gpio-bank.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <asm/uaccess.h>
//#include <asm/ioctl.h>

#define DRIVER_AUTHOR 		"hanback" 		
#define DRIVER_DESC 		"GPIO-BUTTON Program" 	
#define GPIO_MAJOR 		249 				
#define GPIO_NAME 		"GPIO-BUTTON" 		
#define GPIO_MODULE_VERSION 	"GPIO-BUTTON V0.1"
#define IRQ_BUTTON 		IRQ_EINT11 


//Global variable
unsigned long gpl1;
static irqreturn_t button_interrupt(int irq, void *dev_id);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static unsigned short button = 1;
static volatile int ev_press = 0;
static struct fasync_struct *button_async;

// button interrupt handler
static irqreturn_t button_interrupt(int irq, void *dev_id){
	// wake_up_interruptible() 함수를 호출하면
	// wait queue를 등록한 프로세스가 깨어나 다시 실행을 재개한다.
	wake_up_interruptible(&wait_queue);
	ev_press = 1;         

	// 어플에 SIGIO를 보낸다.
	kill_fasync (&button_async, SIGIO, POLL_IN);
	
	return IRQ_HANDLED;
}

// 응용 프로그램에서 디바이스를 처음 사용하는 경우를 처리하는 함수
int gpio_open(struct inode *minode, struct file *mfile){
	int res;

	// set interrupt type
	// button을 눌렀을 때, edge rising일 때 인터럽트 실행
	set_irq_type(IRQ_BUTTON, IRQ_TYPE_EDGE_RISING);
	res = request_irq(IRQ_BUTTON,button_interrupt,IRQF_DISABLED,"GPIO",NULL);
	
	if(res < 0)
		printk(KERN_ERR "%s: Request for IRQ %d failed\n",__FUNCTION__,IRQ_BUTTON);
	else {
		printk("%s: Request for IRQ %d successed\n",__FUNCTION__,IRQ_BUTTON);
	}
	return 0;
}

// 응용 프로그램에서 디바이스를 더이상 사용하지 않아서 닫기를 구현하는 함수
int gpio_release(struct inode *minode, struct file *mfile){
	free_irq(IRQ_BUTTON,NULL);
	return 0;
}

ssize_t gpio_read(struct file *inode, char *gdata, size_t length, loff_t *off_what){
	int ret=0;
	// 휴면상태로 프로세스 상태 변경
	// wait_queue 이벤트가 실행될 때까지 기다린다.
	wait_event_interruptible(wait_queue, ev_press);
 	
	//interruptible_sleep_on(&wait_queue);
	
	// 커널 가상 주소에서 유저 가상 주소로 데이터 복사
	// button에서 gdata로 2byte만큼 데이터 복사
	ret=copy_to_user(gdata,&button,2);
	
	ev_press = 0; 
	if(ret<0) 
		return -1;
	return length;
}


static int gpio_fasync (int fd, struct file *filp, int on){  
	printk("driver: gpio_fasync\n");  
	return fasync_helper (fd, filp, on, &button_async);  
}  
  
// 파일 오퍼레이션
// 파일 시스템을 통해 디바이스 드라이버와 프로그램을 연결시켜주는 구조체
struct file_operations gpio_fops = {
	.owner = THIS_MODULE,
	.open = gpio_open,
	.read = gpio_read,
	.release = gpio_release,
	.fasync  =  gpio_fasync,
};

// 모듈을 커널 내부로 삽입
int gpio_init(void){
	int result;
	int err;

	result = register_chrdev(GPIO_MAJOR, GPIO_NAME, &gpio_fops);
	if(result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}

	err = gpio_request(S5PV210_GPH1(3), "GPH1");
	if(err){
		printk(KERN_ERR "%s:gpio request error(%d)!!\n", __FUNCTION__, err);
	}
	else{
		s3c_gpio_cfgpin(S5PV210_GPH1(3), (0xf<<12));
		s3c_gpio_setpull(S5PV210_GPH1(3), S3C_GPIO_PULL_NONE);
	}
	printk("Init Module, GPIO-BUTTON Major Number : %d\n",GPIO_MAJOR);
	return 0;
}

// 모듈을 커널에서 제거
void gpio_exit(void){
	unregister_chrdev(GPIO_MAJOR, GPIO_NAME);
	printk("driver: %s DRIVER EXIT\n", GPIO_NAME);
}

module_init(gpio_init);
module_exit(gpio_exit); 
MODULE_AUTHOR(DRIVER_AUTHOR); 
MODULE_DESCRIPTION(DRIVER_DESC); 
MODULE_LICENSE("Dual BSD/GPL"); 



