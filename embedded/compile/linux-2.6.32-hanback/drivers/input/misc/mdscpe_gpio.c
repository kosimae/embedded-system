//[*]----------------------------------------------------------------------------------------------[*]
//
//  S5PV210 gpio Controll drive ( by phs )
//  2010.10.21
//
//[*]----------------------------------------------------------------------------------------------[*]

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/fs.h>

#include <mach/irqs.h>
#include <asm/system.h>

#include <linux/delay.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank.h>

#include "mdscpe_sensor.h"

#define	GPG2CON					        (*(unsigned long *)S5PV210_GPG2CON)
#define	GPG2DAT					        (*(unsigned long *)S5PV210_GPG2DAT)
#define	GPG2PUD					        (*(unsigned long *)S5PV210_GPG2PUD)

#define	GPIO_I2C_SDA_CON_PORT	  GPG2CON
#define	GPIO_I2C_SDA_DAT_PORT	  GPG2DAT
#define	GPIO_SDA_PIN			      6         // GPG2[6]

#define	GPIO_I2C_CLK_CON_PORT	  GPG2CON
#define	GPIO_I2C_CLK_DAT_PORT	  GPG2DAT
#define	GPIO_CLK_PIN			      5         // GPG2[5]

#define	GPIO_I2C_RST_CON_PORT	  GPG2CON
#define	GPIO_I2C_RST_DAT_PORT	  GPG2DAT
#define	GPIO_RST_PIN			      4         // GPG2[4]

#define	GPIO_CON_PORT_MASK		  0xF
#define	GPIO_CON_PORT_OFFSET	  0x4

#define	GPIO_CON_INPUT			    0x0
#define	GPIO_CON_OUTPUT			    0x1

#define TIMEOUT_VAL             0x10000
#define	DELAY_TIME				      12

#define	HIGH					          1
#define	LOW						          0

#define SSI_ADDR  0b01000000      // default address of SSI module

static int SSI_Error = 0;
struct mdscpe_cal_data caldata; 

static	void			gpio_i2c_sda_port_control(unsigned char inout);
static	void			gpio_i2c_clk_port_control(unsigned char inout);
static	void			gpio_i2c_rst_port_control(unsigned char inout);

static	void			gpio_i2c_set_sda			(unsigned char hi_lo);
static	void			gpio_i2c_set_clk			(unsigned char hi_lo);
static	void			gpio_i2c_set_rst			(unsigned char hi_lo);
                                        	
static	void			gpio_i2c_start			  (void);
static	void			gpio_i2c_stop			    (void);

/*---------------------------------------------------------------------- */
static	void			gpio_i2c_sda_port_control	(unsigned char inout)
{
	GPIO_I2C_SDA_CON_PORT &=  (unsigned long)(~(GPIO_CON_PORT_MASK << (GPIO_SDA_PIN * GPIO_CON_PORT_OFFSET)));
	GPIO_I2C_SDA_CON_PORT |=  (unsigned long)( (inout << (GPIO_SDA_PIN * GPIO_CON_PORT_OFFSET)));
}

static	void			gpio_i2c_clk_port_control	(unsigned char inout)
{
	GPIO_I2C_CLK_CON_PORT &=  (unsigned long)(~(GPIO_CON_PORT_MASK << (GPIO_CLK_PIN * GPIO_CON_PORT_OFFSET)));
	GPIO_I2C_CLK_CON_PORT |=  (unsigned long)( (inout << (GPIO_CLK_PIN * GPIO_CON_PORT_OFFSET)));
}

static	void			gpio_i2c_rst_port_control	(unsigned char inout)
{
	GPIO_I2C_RST_CON_PORT &=  (unsigned long)(~(GPIO_CON_PORT_MASK << (GPIO_RST_PIN * GPIO_CON_PORT_OFFSET)));
	GPIO_I2C_RST_CON_PORT |=  (unsigned long)( (inout << (GPIO_RST_PIN * GPIO_CON_PORT_OFFSET)));
}

static	unsigned char	gpio_i2c_get_clk		(void)
{
	return	GPIO_I2C_CLK_DAT_PORT & (HIGH << GPIO_CLK_PIN) ? 1 : 0;
}

static	void			gpio_i2c_set_sda		(unsigned char hi_lo)
{
	if(hi_lo)	{
      GPIO_I2C_SDA_DAT_PORT |= (HIGH << GPIO_SDA_PIN);
	}
	else		{
		GPIO_I2C_SDA_DAT_PORT &= ~(HIGH << GPIO_SDA_PIN);
		gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);
	}
}

static	void			gpio_i2c_set_clk		(unsigned char hi_lo)
{
	if(hi_lo)	{
		GPIO_I2C_CLK_DAT_PORT |= (HIGH << GPIO_CLK_PIN);
	}
	else		{
		GPIO_I2C_CLK_DAT_PORT &= ~(HIGH << GPIO_CLK_PIN);
		gpio_i2c_clk_port_control(GPIO_CON_OUTPUT);
	}
}

static	void			gpio_i2c_set_rst		(unsigned char hi_lo)
{
	if(hi_lo)	{
		GPIO_I2C_RST_DAT_PORT |= (HIGH << GPIO_RST_PIN);
	}
	else		{
		GPIO_I2C_RST_DAT_PORT &= ~(HIGH << GPIO_RST_PIN);
		gpio_i2c_rst_port_control(GPIO_CON_OUTPUT);
  }
}

/*********************************************
 *
 * Set bit of 8-bit data
 *    Input: data to be set
 *           bits possition
 * Return: data
 *
 * *********************************************/
char setbit8 (char data, char bits)
{
  char i, j;
  i = data;
  j = bits;
  return (i |= (1 << j));
}

void SSI1_Start(void)
{
  gpio_i2c_set_clk(LOW);                      // Clock = LOW
  gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
  gpio_i2c_set_sda(HIGH);                     // DATA = HIGH

  gpio_i2c_set_clk(HIGH);                     // Clock = High

  /* Delay 2ms for Module Compass Wake up */
  mdelay(2);

  gpio_i2c_set_sda(LOW);                      // Data = Low
  udelay(DELAY_TIME);       

  gpio_i2c_set_clk(LOW);                      // Clock = Low
  udelay(DELAY_TIME);

  gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
  udelay(DELAY_TIME);

  gpio_i2c_set_sda(HIGH);                     // RISING EDGE WHEN SCL1 = 1
  udelay(DELAY_TIME);

  gpio_i2c_set_clk(LOW);                      // Clock = Low
  udelay(DELAY_TIME);

  gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
}

char SSI1_Sendcode(char SSI_Adr, char SSI_command, char SSI_RW)
{
  int i;
  char command;

  command = SSI_command | SSI_Adr;

  if (SSI_RW == 1) command |= 1;

  gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);   // Data = Output

  for(i=8;i>0;i--)
  {
    if ((command & 0x80) == 0)
      gpio_i2c_set_sda(LOW);                    // DATA = Low
    else
      gpio_i2c_set_sda(HIGH);                   // DATA = HIGH;

    command <<= 1;
    gpio_i2c_set_clk(HIGH);                     // Clock = HIGH

    udelay(DELAY_TIME);
    gpio_i2c_set_clk(LOW);                      // Clock = LOW
    udelay(DELAY_TIME);
  }

  /* Check ACK, Set Port To Input */
  gpio_i2c_sda_port_control(GPIO_CON_INPUT);    // Data = INPUT
  gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
  udelay(DELAY_TIME);

  if ((GPIO_I2C_SDA_DAT_PORT & 0x40)) {         // Data Check
    SSI_Error = setbit8(SSI_Error,0);
  }

  gpio_i2c_set_clk(LOW);                        // Clock = LOW

  udelay(DELAY_TIME);

  return (SSI_Error);
}

/*********************************************
 *
 * Receive data from SSI module 1
 * Return: 16 bits data
 *
 *********************************************/
int SSI1_rdata(void)
{
  int temp=0,i,j;
  char SSI_Checksum;
  int cnt=0;

  for(i=2;i>0;i--)
  {
   gpio_i2c_sda_port_control(GPIO_CON_INPUT);     // Data = INPUT

    /* Read 16 bits data */
    for(j=8;j>0;j--)
    {
      gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
      udelay(DELAY_TIME);
      temp <<= 1;
      cnt = (GPIO_I2C_SDA_DAT_PORT & 0x40);       // Data Signal Check
      if(cnt)
        temp |= 1;
      gpio_i2c_set_clk(LOW);                      // Clock = HIGH
      udelay(DELAY_TIME);
    }

    /* ouput ACK */
   gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);    // DATA = OUTPUT 
   gpio_i2c_set_sda(LOW);                         // DATA = Low
   gpio_i2c_set_clk(HIGH);                        // Clock = High
   udelay(DELAY_TIME);
   gpio_i2c_set_clk(LOW);                         // Clock = Low
   udelay(DELAY_TIME);
  }

   gpio_i2c_sda_port_control(GPIO_CON_INPUT);     // DATA = INPUT

   /* Read 8 bits Checksum */
   for(i=8;i>0;i--)
   {
     gpio_i2c_set_clk(HIGH);                      // Clock = High
     udelay(DELAY_TIME);

     SSI_Checksum <<= 1;
     if((GPIO_I2C_SDA_DAT_PORT & 0x40))           // Data Signal Check
       SSI_Checksum |= 1;
     gpio_i2c_set_clk(LOW);                       // Clock = LOW
     udelay(DELAY_TIME);
   }

   /* ouput ACK */
   gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);    // DATA = OUTPUT
   gpio_i2c_set_sda(HIGH);                        // DATA  = HIGH
   gpio_i2c_set_clk(HIGH);                        // Clock = HIGH
   udelay(DELAY_TIME);

   gpio_i2c_set_clk(LOW);                         // Clock = LOW
   udelay(110);

   return temp;
}

/*********************************************
 *
 * Write 10 bytes data to SSI module 1 Xoff, Yoff, XSC, YSC, K
 * Return: none
 *
 *********************************************/

void write_calib_data(void)
{
  int i,j;

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);   // DATA = OUTPUT

    for(j=0;j<8;j++)
    {
      if((caldata.xoff & 0x8000) != 0)
        gpio_i2c_set_sda(HIGH);                   // DATA = HIGH
      else
        gpio_i2c_set_sda(LOW);                    // DATA = LOW

      gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
      udelay(DELAY_TIME);

      gpio_i2c_set_clk(LOW);                      // Clock = LOW
      udelay(DELAY_TIME);
      caldata.xoff <<= 1;
    }

    /* check ACK, set port to input */
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);    // DATA = INPUT
    gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
    udelay(DELAY_TIME);

    if ((GPIO_I2C_SDA_DAT_PORT & 0x40))           // Data Signal Check
    {
      SSI_Error = setbit8(SSI_Error,0);
    }

    gpio_i2c_set_clk(LOW);                        // Clock = LOW
    udelay(DELAY_TIME);
  }

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);   // DATA = OUTPUT

    for(j=0;j<8;j++)
    {
      if((caldata.yoff & 0x8000) != 0)
        gpio_i2c_set_sda(HIGH);                   // DATA = HIGH
      else
        gpio_i2c_set_sda(LOW);                    // DATA = LOW

      gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
      udelay(DELAY_TIME);

      gpio_i2c_set_clk(LOW);                      // Clock = LOW
      udelay(DELAY_TIME);
      caldata.yoff <<= 1;
    }

    /* check ACK, set port to input */
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);    // DATA = INPUT
    gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
    udelay(DELAY_TIME);

    if ((GPIO_I2C_SDA_DAT_PORT & 0x40))
    {
      SSI_Error = setbit8(SSI_Error,0);
    }
    gpio_i2c_set_clk(LOW);                        // Clock = LOW
    udelay(DELAY_TIME);
  }

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);   // DATA = OUTPUT

    for(j=0;j<8;j++)
    {
      if((caldata.xsc & 0x8000) != 0)
        gpio_i2c_set_sda(HIGH);                   // DATA = HIGH
      else
        gpio_i2c_set_sda(LOW);                    // DATA = LOW

      gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
      udelay(DELAY_TIME);

      gpio_i2c_set_clk(LOW);                      // Clock = LOW
      udelay(DELAY_TIME);
      caldata.xsc <<= 1;
    }

    /* check ACK, set port to input */
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);    // DATA = INPUT
    gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
    udelay(DELAY_TIME);

    if ((GPIO_I2C_SDA_DAT_PORT & 0x40))
    {
      SSI_Error = setbit8(SSI_Error,0);
    }
    gpio_i2c_set_clk(LOW);                        // Clock = LOW
    udelay(DELAY_TIME);
  }

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);   // DATA = OUTPUT

    for(j=0;j<8;j++)
    {
      if((caldata.ysc & 0x8000) != 0)
        gpio_i2c_set_sda(HIGH);                   // DATA = HIGH
      else
        gpio_i2c_set_sda(LOW);                    // DATA = LOW

      gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
      udelay(DELAY_TIME);

      gpio_i2c_set_clk(LOW);                      // Clock = LOW
      udelay(DELAY_TIME);
      caldata.ysc <<= 1;
    }

    /* check ACK, set port to input */
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);    // DATA = INPUT
    gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
    udelay(DELAY_TIME);

    if ((GPIO_I2C_SDA_DAT_PORT & 0x40))
    {
      SSI_Error = setbit8(SSI_Error,0);
    }
    gpio_i2c_set_clk(LOW);                        // Clock = LOW
    udelay(DELAY_TIME);
  }

  gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);     // DATA = OUTPUT

  for(j=0;j<8;j++)
  {
    if((caldata.k & 0x8000) != 0)
      gpio_i2c_set_sda(HIGH);                     // DATA = HIGH
    else
      gpio_i2c_set_sda(LOW);                      // DATA = LOW

    gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                        // Clock = LOW
    udelay(DELAY_TIME);
    caldata.k <<= 1;
  }

  /* check ACK, set port to input */
  gpio_i2c_sda_port_control(GPIO_CON_INPUT);      // DATA = INPUT
  gpio_i2c_set_clk(HIGH);                         // Clock = HIGH
  udelay(DELAY_TIME);

  if ((GPIO_I2C_SDA_DAT_PORT & 0x40))
  {
    SSI_Error = setbit8(SSI_Error,0);
  }
  gpio_i2c_set_clk(LOW);                          // Clock = LOW
  udelay(DELAY_TIME);

  gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);     // DATA = OUTPUT

  for(j=0;j<8;j++)
  {
    if((caldata.k & 0x8000) != 0)
      gpio_i2c_set_sda(HIGH);                     // DATA = HIGH
    else
      gpio_i2c_set_sda(LOW);                      // DATA = LOW

    gpio_i2c_set_clk(HIGH);                       // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                        // Clock = LOW
    udelay(DELAY_TIME);
    caldata.k <<= 1;
  }

  /* check ACK, set port to input */
  gpio_i2c_sda_port_control(GPIO_CON_INPUT);      // DATA = INPUT
  gpio_i2c_set_clk(HIGH);                         // Clock = HIGH
  udelay(DELAY_TIME);

  if ((GPIO_I2C_SDA_DAT_PORT & 0x40))
  {
    SSI_Error = setbit8(SSI_Error,0);
  }
  gpio_i2c_set_clk(LOW);                          // Clock = LOW
  udelay(DELAY_TIME);
}

void write_data_Code(char code)
{
  /* Start SSI module */
  SSI1_Start();

  /* Send read Pressure from SSI module */
  SSI_Error = SSI1_Sendcode(SSI_ADDR,code,1);

  write_calib_data();
}

/*********************************************
 *
 * Receive data from SSI module 1
 * Return: 10 bytes data Xoff, Yoff, XSC, YSC, K
 *
 *********************************************/

void read_calib_data(void)
{
  int i, j;

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);  // DATA = INPUT

    for(j=0;j<8;j++)
    {
      gpio_i2c_set_clk(HIGH);                   // Clock = HIGH
      udelay(DELAY_TIME);
      caldata.xoff <<= 1;
      if((GPIO_I2C_SDA_DAT_PORT & 0x40))
        caldata.xoff |= 1;

      gpio_i2c_set_clk(LOW);                    // Clock = Low
      udelay(DELAY_TIME);
    }

    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
    gpio_i2c_set_sda(LOW);                      // DATA = LOW
    gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                      // Clock = Low
    udelay(DELAY_TIME);
  }

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);  // DATA = INPUT

    for(j=0;j<8;j++)
    {
      gpio_i2c_set_clk(HIGH);                   // Clock = HIGH
      udelay(DELAY_TIME);
      caldata.yoff <<= 1;
      if((GPIO_I2C_SDA_DAT_PORT & 0x40))
        caldata.yoff |= 1;

      gpio_i2c_set_clk(LOW);                    // Clock = Low
      udelay(DELAY_TIME);
    }

    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
    gpio_i2c_set_sda(LOW);                      // DATA = LOW
    gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                      // Clock = Low
    udelay(DELAY_TIME);
  }

  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);  // DATA = INPUT

    for(j=0;j<8;j++)
    {
      gpio_i2c_set_clk(HIGH);                   // Clock = HIGH
      udelay(DELAY_TIME);
      caldata.xsc <<= 1;
      if((GPIO_I2C_SDA_DAT_PORT & 0x40))
        caldata.xsc |= 1;

      gpio_i2c_set_clk(LOW);                    // Clock = Low
      udelay(DELAY_TIME);
    }

    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
    gpio_i2c_set_sda(LOW);                      // DATA = LOW
    gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                      // Clock = Low
    udelay(DELAY_TIME);
  }


  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);  // DATA = INPUT

    for(j=0;j<8;j++)
    {
      gpio_i2c_set_clk(HIGH);                   // Clock = HIGH
      udelay(DELAY_TIME);
      caldata.ysc <<= 1;
      if((GPIO_I2C_SDA_DAT_PORT & 0x40))
        caldata.ysc |= 1;

      gpio_i2c_set_clk(LOW);                    // Clock = Low
      udelay(DELAY_TIME);
    }

    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
    gpio_i2c_set_sda(LOW);                      // DATA = LOW
    gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                      // Clock = Low
    udelay(DELAY_TIME);
  }


  for(i=0;i<2;i++)
  {
    gpio_i2c_sda_port_control(GPIO_CON_INPUT);  // DATA = INPUT

    for(j=0;j<8;j++)
    {
      gpio_i2c_set_clk(HIGH);                   // Clock = HIGH
      udelay(DELAY_TIME);
      caldata.k <<= 1;
      if((GPIO_I2C_SDA_DAT_PORT & 0x40))
        caldata.k |= 1;

      gpio_i2c_set_clk(LOW);                    // Clock = Low
      udelay(DELAY_TIME);
    }

    gpio_i2c_sda_port_control(GPIO_CON_OUTPUT); // DATA = OUTPUT
    gpio_i2c_set_sda(LOW);                      // DATA = LOW
    gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
    udelay(DELAY_TIME);

    gpio_i2c_set_clk(LOW);                      // Clock = Low
    udelay(DELAY_TIME);
  }

}


void read_data_Code(char code)
{
  /* Start SSI module */
  SSI1_Start();

  /* Send read Pressure from SSI module */
  SSI_Error = SSI1_Sendcode(SSI_ADDR,code,1);

  read_calib_data();

#ifdef  COMPASS_DEBUG
  printk(" xoff = %x , yoff = %x , xsc = %x, ysc = %x, k = %x\n", 
          caldata.xoff, caldata.yoff, caldata.xsc,caldata.ysc,caldata.k);
#endif
}

unsigned int mdscpe_send_command(char code)
{
  int timeout = 0;
  int data =0;

  /* Start SSI module */
  SSI1_Start();

  /* Send read data from SSI module */
  SSI_Error = SSI1_Sendcode(SSI_ADDR,code,1);   
  gpio_i2c_sda_port_control(GPIO_CON_INPUT);    // DATA = INPUT

  /* wait for data ready */
  /* GPG2_6 : Data Read.. */
  mdelay(10);
  while ((GPIO_I2C_SDA_DAT_PORT & 0x40) && timeout < TIMEOUT_VAL)
  {
    timeout++;
  }

  if(timeout == TIMEOUT_VAL)
  {
    SSI_Error = setbit8(SSI_Error,0);
    printk("%s: Ack Signal Not Responding...\n",__FUNCTION__);
    return -1;
  } else {
      mdelay(30);
  }

  /* output ACK */
  gpio_i2c_set_clk(HIGH);                     // Clock = HIGH
  udelay(DELAY_TIME);
  gpio_i2c_set_clk(LOW);                      // Clock = Low

  udelay(DELAY_TIME);

  data = (unsigned int)SSI1_rdata();

  if(code == 0x6)  return data;
}

void mdscpe_reset(void)
{
  gpio_i2c_set_clk(LOW);                      // Reset Signal = HIGH
  gpio_i2c_set_rst(HIGH);                     // Reset Signal = HIGH
  gpio_i2c_set_rst(LOW);                      // Reset Signal = LOW
  mdelay(25);                                 // Waiting for 25ms
  gpio_i2c_set_rst(HIGH);                     // Reset Signal = HIGH
  mdelay(100);                                // Waiting for 100ms
}

void mdscpe_calib_init()
{
  int i;

  printk("Compass Calibarion Start. \n");
  printk("Comapss must be turned by very slow at Horizontal Level for at least 360\n");
  mdscpe_send_command(0x0);                   // Start Calibration
  for(i=0;i<60;i++) {
    mdelay(50);
    mdscpe_send_command(0x2);                 //  
  }
  mdscpe_send_command(0x4);                   // End Calibration
  printk("Compass Calibarion End. \n");
}

void 			mdscpe_port_init			(void)
{
  // RESET(4) = HIGH, Clock(5) = HIGH, DATA(6) = LOW
	GPG2CON &= (~0x0FFF0000);	GPG2CON |= 0x00110000;	
	GPG2PUD &= (~0x0000FF00);	GPG2PUD |= 0x0000AA00;	
}
