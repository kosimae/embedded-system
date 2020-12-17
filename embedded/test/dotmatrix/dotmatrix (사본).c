#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#define DRIVER_AUTHOR		"hanback"	
#define DRIVER_DESC		"dotmatrix program" 

#define DOT_MAJOR		244		
#define DOT_NAME 		"DOTMATRIX"	
#define DOT_MODULE_VERSION 	"DOTMATRIX V0.1"
#define DOT_PHY_ADDR		0x88000000	
#define DOT_ADDR_RANGE 		0x1000		

#define NUMSIZE			4		
#define DELAY			2	

//Global variable
static int dot_usage = 0;
static unsigned long  dot_ioremap;
static unsigned short *dot_row_addr,*dot_col_addr;

// Number value
unsigned short font_num[64]= {
	0x7f, 0x41, 0x41, 0x7f,  // 0
	0x40, 0x7f, 0x42, 0x00,  // 1
	0x4f, 0x49, 0x49, 0x79,  // 2
	0x7f, 0x49, 0x49, 0x49,  // 3
	0x10, 0x7f, 0x10, 0x1f,  // 4
	0x79, 0x49, 0x49, 0x4f,  // 5
	0x79, 0x49, 0x49, 0x7f,  // 6
	0x7f, 0x01, 0x01, 0x01,  // 7
	0x7f, 0x49, 0x49, 0x7f,  // 8
	0x7f, 0x49, 0x49, 0x4f,  // 9
        0x7f, 0x09, 0x09, 0x7f,  // A
        0x7f, 0x49, 0x49, 0x7f,  // B
        0X41, 0x41, 0x41, 0x7f,  // C
        0x7f, 0x41, 0x41, 0x7f,  // D
        0x49, 0x49, 0x49, 0x7f,  // E
        0x09, 0x09, 0x09, 0x7f   // F
};

// Delay func.
void m_delay(int num) 
{
	volatile int i,j;
	for(i=0;i<num;i++)
		for(j=0;j<16384;j++);
}

// define functions...
int dot_open(struct inode *minode, struct file *mfile) 
{
	if(dot_usage != 0) return -EBUSY;
	
	dot_ioremap=(unsigned long)ioremap(DOT_PHY_ADDR,DOT_ADDR_RANGE);
	
	dot_row_addr =(unsigned short *)(dot_ioremap+0x40);
	dot_col_addr =(unsigned short *)(dot_ioremap+0x42);
	
	if(!check_mem_region(dot_ioremap, DOT_ADDR_RANGE)) {
		request_mem_region(dot_ioremap, DOT_ADDR_RANGE, DOT_NAME);
	}
	else	printk("driver: unable to register this!\n");
	
	dot_usage = 1;
	return 0;
}

int dot_release(struct inode *minode, struct file *mfile) 
{
	iounmap((unsigned long*)dot_ioremap);

	release_mem_region(dot_ioremap, DOT_ADDR_RANGE);
	dot_usage = 0;
	return 0;
}

ssize_t dot_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) 
{
	volatile int i,j,k;
	int size=0,cnt=0,ret,up_digit=0,quit=1,counter;
	unsigned int second_data, dispdata[20];
	unsigned int init_row=0x100, init_row2=0x008; //Scan value
	
	ret=copy_from_user(&counter, gdata, 4);
	if(ret<0) return -1;

	while(quit) {
		if(cnt == 16) {
			cnt = 0;
			up_digit++;
		}

		if(up_digit == 16) up_digit=0;

		if(up_digit == 0) {
			size = 4;
			for(k=0;k<4;k++) {
				dispdata[k] = font_num[cnt*NUMSIZE+k];
			}
		} else {
			size = 8;
			second_data = up_digit%16;
			for(k=0;k<4;k++) {
				dispdata[k] = font_num[cnt*NUMSIZE+k];
				dispdata[k+4] = font_num[second_data*NUMSIZE+k];
			}
		}
		
		for(i=0;i<100;i++) {
			for(j=0;j<size;j++) {
				if(j<4)	{	// 1*x
					*dot_row_addr = init_row>>j;
					*dot_col_addr = 0x8000|dispdata[j];
					if(cnt<16) m_delay(9);
					else m_delay(3);
				} else { 	// 10*x
					*dot_row_addr = init_row2 >> (j-4);
					*dot_col_addr = 0x8000|dispdata[j];
					m_delay(3);
				}
			}
		}
	
		if(counter == (up_digit*16 + cnt)) {
			quit = 0;
			*dot_col_addr = 0x0;
			break;
		}
		cnt++;
	}
	
	return length;
}

struct file_operations dot_fops = {
	.owner		= THIS_MODULE,
	.write		= dot_write,
	.open		= dot_open,
	.release	= dot_release,
};

int dot_init(void) 
{
	int result;

	result = register_chrdev(DOT_MAJOR, DOT_NAME, &dot_fops);
	
	if(result < 0) { 
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	
	printk("Init Module, Dotmatrix Major Number : %d\n", DOT_MAJOR);
	
	return 0;
}

void dot_exit(void) 
{
	unregister_chrdev(DOT_MAJOR, DOT_NAME);

	printk("driver: %s DRIVER EXIT\n", DOT_NAME);
}

module_init(dot_init);		
module_exit(dot_exit);		

MODULE_AUTHOR(DRIVER_AUTHOR); 	 
MODULE_DESCRIPTION(DRIVER_DESC); 
MODULE_LICENSE("Dual BSD/GPL");	
