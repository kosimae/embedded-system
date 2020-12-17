#include <common.h>
#include "../app/io.h"
#include "../lcd/def.h"

#if defined(CONFIG_EMPOS3SV210)
#define ST16C554_UART_ABASE 0xA0040000  /* Quad Uart Port A */
#define ST16C554_UART_BBASE 0x38000040
#define ST16C554_UART_CBASE 0x38000080
#define ST16C554_UART_DBASE 0x380000C0
#elif defined(CONFIG_SM3SV210)
#define ST16C554_UART_ABASE 0xA0040000  /* Quad Uart Port A */
#define ST16C554_UART_BBASE 0x38000040
#define ST16C554_UART_CBASE 0x38000080
#define ST16C554_UART_DBASE 0x380000C0
#endif

#define ST16C554_UART_BASE ST16C554_UART_ABASE

#define ST16C554_RBR   0x00  /* Receive Buffer Register (read only) */
#define ST16C554_THR   0x00  /* Transmit Holding Register (write only) */
#define ST16C554_IER   (0x04 <<1)  /* Interrupt Enable Register (read/write) */
#define ST16C554_IIR   (0x08 <<1)  /* Interrupt ID Register (read only) */
#define ST16C554_FCR   (0x08 <<1)  /* FIFO Control Register (write only) */
#define ST16C554_LCR   (0x0C <<1)  /* Line Control Register (read/write) */
#define ST16C554_MCR   (0x10 <<1)  /* Modem Control Register (read/write) */
#define ST16C554_LSR   (0x14 <<1)  /* Line Status Register (read only) */
#define ST16C554_MSR   (0x18 <<1)  /* Reserved */
#define ST16C554_SPR   (0x1C <<1)  /* Scratch Pad Register (read/write) */
#define ST16C554_DLL   (0x00 <<1)  /* Divisor Latch Low Register (DLAB = 1) (read/write) */
#define ST16C554_DLH   (0x04 <<1)  /* Divisor Latch High Register (DLAB = 1) (read/write) */

/* ST16C554 Serial baudrate */
#define BAUD_RATE_115200   (0x01)
#define BAUD_RATE_57600    (0x02)
#define BAUD_RATE_38400    (0x03)
#define BAUD_RATE_19200    (0x06)
#define BAUD_RATE_9600     (0x0c)

int MOD(int num, int dev){              // num % dev    툴체인에 문제가 있어서  함수를 직접 구현하였다.
  int remainder = num;

  for(;;)
  {
    if(remainder<dev)
      return remainder;

    remainder -= dev;
  }

  return remainder;
}

int DEV(int num, int dev){              // num / dev    툴체인에 문제가 있어서  함수를 직접 구현하였다.
  int remainder = num;
  int mok =0;

  for(;;)
  {
    if(remainder<dev)
      return mok;

    mok++;
    remainder -= dev;
  }
  return mok;
}

int Power(int num, int cnt){

  long retval=num;
  cnt--;

  while (cnt--){
    retval *= num;
  }
  return retval;
}

bool DecToLong(char *s, long *retval){
  long remainder;
  if (!s || !s[0]) return false;

  for (*retval=0; *s; s++){
    if (*s < '0' || *s > '9') return false;
    remainder = *s - '0';
    *retval = *retval * 10 + remainder;
  }

  return true;
}

void ST16C554_SerialInit(void)
{
  __REG8(ST16C554_UART_BASE + ST16C554_LCR) = 0x80; // DLAB set=latch registers, DLAB clear=일반 포트.
  __REG8(ST16C554_UART_BASE + ST16C554_DLL) = 0x08; // baud rate 설정.
  __REG8(ST16C554_UART_BASE + ST16C554_DLH) = 0x00;

  __REG8(ST16C554_UART_BASE + ST16C554_LCR) = 0x03; // 8-bit, 1 stop, no parity 세팅.
  __REG8(ST16C554_UART_BASE + ST16C554_FCR) = 0x07; // Reset tx, rx FIFO. clear. FIFO enable
  __REG8(ST16C554_UART_BASE + ST16C554_IER) = 0x40; // UART Enable Interrupt
  // Transmit Shift Register, Transmit Holding Register, FIFO에
  // 데이타가 없을때까지 기다린다.
  while(! __REG8(ST16C554_UART_BASE + ST16C554_LSR) & 0x40 );
  __REG8(ST16C554_UART_BASE +ST16C554_IER) = 0x40; // UART Unit Enable
  __REG8(ST16C554_UART_BASE +ST16C554_FCR) = 0x07; // RESETTF, RESETRF, TRFIFOE
  __REG8(ST16C554_UART_BASE +ST16C554_LCR) = 0x03; // i stop, 8bit data, no parity
  return;
}

void ST16C554_PutChar(const char ch)
{
    // FIFO에 데이타가 없을때까지 기다린다.
    while ((__REG8(ST16C554_UART_BASE + ST16C554_LSR) & 0x00000020) == 0 );

    __REG8(ST16C554_UART_BASE + ST16C554_THR) = ((u32)ch & 0xFF);

    if (ch=='\n') ST16C554_PutChar('\r'); // c=='\n'이면, 실제로는 "\n\r"을 출력.
}

void ST16C554_PutString(const char *ch) //문잘열을 출력하기 위해 구현해 놓았지만 printf문이 있어 거의 사용하지 않는다.
{
    while(*ch != '\0')
	ST16C554_PutChar(*ch++);
}

int ST16C554_GetChar(char * ch) //시리얼로 입력된 데이타를 가져 온다.
{
  if((__REG8(ST16C554_UART_BASE + ST16C554_LSR) & 0x00000001) == 0){ // FIFO에 데이타가 있을때.
    return 0;
  } else {
    //(volatile char)*ch = __REG8(ST16C554_UART_BASE + ST16C554_RBR);
    *ch = __REG8(ST16C554_UART_BASE + ST16C554_RBR);
    return 1;
  }
}

void ST16C554_PrintChar(char *fmt, char c)
{
    ST16C554_PutChar(c);
    return;
}

void ST16C554_PrintDec(char *fmt, int l)
{
    int	i, j;
    char	c, *s=fmt, tol[10];
    bool	flag0=false, flagl=false;	// "%08lx"에서 '0', 'l'의 존재 여부.
    long	flagcnt=0;					// "%08lx"에서 "8"을 찾아서 long형으로.
    bool	leading_zero=true;			// long형의 data를 출력하기 위한 변수.
    long	divisor, result, remainder;

    // fmt의 "%08lx"에서 '0', '8', 'l'을 해석.
    for (i=0; (c=s[i]); i++){
	if (c=='d') break;
	else if (c>='1' && c<='9'){
	    for (j=0; s[i]>='0' && s[i]<='9'; j++){
		tol[j] = s[i++];
	    }
	    tol[j] = '\0';
	    i--;
	    DecToLong(tol, &flagcnt);
	}
	else if (c=='0') flag0=true;
	else if (c=='l') flagl=true;
	else continue;
    }

    // 위의 flag에 따라 출력.
    if (flagcnt){
	if (flagcnt>9) flagcnt=9;

	//remainder = l%(Power(10, flagcnt));	// flagcnt보다 윗자리의 수는 걸러냄. 199에 flagcnt==2이면, 99만.
	remainder = MOD(l,Power(10, flagcnt));

	for (divisor=Power(10, flagcnt-1); divisor>0; divisor/=10){

	    //result = remainder / divisor;
	    result = DEV(remainder,divisor);

	    //remainder %= divisor;
	    remainder =MOD(remainder, divisor);

	    if (result!=0 || divisor==1) leading_zero = false;

	    if (leading_zero==true){
		if (flag0)	ST16C554_PutChar('0');
		else		ST16C554_PutChar(' ');
	    }
	    else ST16C554_PutChar((char)(result)+'0');
	}
    } else {
	remainder = l;

	for (divisor=1000000000; divisor>0; divisor/=10){
	    //result = remainder/divisor;
	    result = DEV(remainder,divisor);

	    //remainder %= divisor;
	    remainder =MOD(remainder, divisor);

	    if (result!=0 || divisor==1) leading_zero = false;
	    if (leading_zero==false) ST16C554_PutChar((char)(result)+'0');
	}
    }

    return;
}

void ST16C554_PrintHex(char *fmt, int l)
{
    int		i, j;
    char	c, *s=fmt, tol[10];
    bool	flag0=false, flagl=false;	// flags.
    long	flagcnt=0;
    bool	leading_zero=true;
    char	uHex, lHex;
    int		cnt;						// "%5x"의 경우 5개만 출력하도록 출력한 개수.

    // fmt의 "%08lx"에서 '0', '8', 'l'을 해석.
    for (i=0; (c=s[i]); i++){
	if (c=='x') break;
	else if (c>='1' && c<='9'){
	    for (j=0; s[i]>='0' && s[i]<='9'; j++){
		tol[j] = s[i++];
	    }
	    tol[j] = '\0';
	    i--;
	    DecToLong(tol, &flagcnt);
	}
	else if (c=='0') flag0=true;
	else if (c=='l') flagl=true;
	else continue;
    }

    s = (char *)(&l);
    l = SWAP32(l);		// little, big endian에 따라서.(big이 출력하기 쉬워 순서를 바꿈)

    // 위의 flag에 따라 출력.
    if (flagcnt){
	if (flagcnt&0x01){	// flagcnt가 홀수 일때, upper를 무시, lower만 출력.
	    c = s[(8-(flagcnt+1))/2]; // 홀수 일때 그 위치를 포함하는 곳의 값을 가져 옵니다.

	    // lower 4 bits를 가져와서 ascii code로.
	    lHex = ((c>>0)&0x0f);
	    if (lHex!=0) leading_zero=false;
	    if (lHex<10) lHex+='0';
	    else         lHex+='A'-10;

	    // lower 4 bits 출력.
	    if (leading_zero){
		if (flag0) ST16C554_PutChar('0');
		else       ST16C554_PutChar(' ');
	    }
	    else ST16C554_PutChar(lHex);

	    flagcnt--;
	}

	// byte단위의 data를 Hex로 출력.
	for (cnt=0, i=(8-flagcnt)/2; i<4; i++){
	    c = s[i];

	    // get upper 4 bits and lower 4 bits.
	    uHex = ((c>>4)&0x0f);
	    lHex = ((c>>0)&0x0f);

	    // upper 4 bits and lower 4 bits to '0'~'9', 'A'~'F'.
	    // upper 4 bits를 ascii code로.
	    if (uHex!=0) leading_zero = false;
	    if (uHex<10) uHex+='0';
	    else         uHex+='A'-10;

	    // upper 4 bits 출력.
	    if (leading_zero){
		if (flag0) ST16C554_PutChar('0');
		else       ST16C554_PutChar(' ');
	    }
	    else ST16C554_PutChar(uHex);

	    // lower 4 bits를 ascii code로.
	    if (lHex!=0) leading_zero = false;
	    if (lHex<10) lHex+='0';
	    else         lHex+='A'-10;

	    // lower 4 bits 출력.
	    if (leading_zero){
		if (flag0) ST16C554_PutChar('0');
		else       ST16C554_PutChar(' ');
	    }
	    else ST16C554_PutChar(lHex);
	}
    }
    else {
	for (i=0; i<4; i++){
	    c = s[i];

	    // get upper 4 bits and lower 4 bits.
	    uHex = ((c>>4)&0x0f);
	    lHex = ((c>>0)&0x0f);

	    // upper 4 bits and lower 4 bits to '0'~'9', 'A'~'F'.
	    if (uHex!=0) leading_zero = false;
	    if (uHex<10) uHex+='0';
	    else         uHex+='A'-10;
	    if (!leading_zero) ST16C554_PutChar(uHex);

	    if (lHex!=0 || i==3) leading_zero = false;
	    if (lHex<10) lHex+='0';
	    else         lHex+='A'-10;
	    if (!leading_zero) ST16C554_PutChar(lHex);
	}
    }
    return;
}

void ST16C554_PrintString(char *fmt, char *s)
{
    if (!fmt || !s) return;
    while (*s) ST16C554_PutChar(*s++);
    return;
}

void ST16C554_printf(char *fmt, ...)
{
  int	i;
  va_list args;
  char	*s=fmt;
  char	format[10];		// fmt의 인자가 "%08lx"라면, "08l"를 임시로 기록.

  va_start(args, fmt);

  while (*s){
    if (*s=='%'){
      s++;
      // s에서 "%08lx"형식을 가져와 format에 기록. 나중에 출력함수에 넘겨줌.
      format[0] = '%';
      for (i=1; i<10;){
        if (*s=='c' || *s=='d' || *s=='x' || *s=='s' || *s=='%'){
          format[i++] = *s;
          format[i] = '\0';
          break;
        }
        else {
          format[i++] = *s++;
        }
      }
      // "%s", "%c", "%d", "%x"를 찾아 출력할 함수 호출.
      switch (*s++){
        case 'c' :
          ST16C554_PrintChar(format, va_arg(args, int));
          break;
        case 'd' :
          ST16C554_PrintDec(format, va_arg(args, int));
          break;
        case 'x' :
          ST16C554_PrintHex(format, va_arg(args, int));
          break;
        case 's' :
          ST16C554_PrintString(format, va_arg(args, char *));
          break;
        case '%' :
          ST16C554_PrintChar("%c", '%');
          break;
      }
    }
    else {
      ST16C554_PrintChar("%c", *s);
      s++;
    }
  }
  va_end(args);
  return;
}

#define TRUE    1
#define FALSE   0

#define RHR	0x00    /* Receive Holding Register */
#define THR	0x00    /* Transmit Holding Register */
#define IER	0x04    /* Interrupt Enable Register */
#define FCR	0x08    /* FIFO control Register */
#define ISR	0x08    /* Interrupt Status Register */
#define LCR	0x0c    /* Line control Register */
#define MCR	0x10    /* Modem control Register */
#define LSR	0x14    /* Line Status Register */
#define MSR	0x18    /* Modem Status Register */
#define SCR	0x1c    /* Scratch pad Register */

/* This two offsets are used for defining the baud rate */
#define DIVLSB 0x00    /* Divisor LSB latch address */
#define DIVMSB 0x04     /* Divisor MSB latch address */

#define LSR_FIFOE       (1 << 7)        /* FIFO Error Status */
#define LSR_TEMT        (1 << 6)        /* Transmitter Empty */
#define LSR_TDRQ        (1 << 5)        /* Transmit Data Request */
#define LSR_BI          (1 << 4)        /* Break Interrupt */
#define LSR_FE          (1 << 3)        /* Framing Error */
#define LSR_PE          (1 << 2)        /* Parity Error */
#define LSR_OE          (1 << 1)        /* Overrun Error */
#define LSR_DR          (1 << 0)        /* Data Ready */


/*\
  * Program table for baud rate
  * This represents the LSB and MSB divisor latch data
\*/

char baud_table[8][2] = {
                        { 0x80, 0x01 },	/* 300 */
                        { 0x60, 0x00 },	/* 1200 */
                        { 0x30, 0x00 },	/* 2400 */
                        { 0x0c, 0x00 },	/* 9600 */
                        { 0x06, 0x00 },	/* 19K */
                        { 0x03, 0x00 },	/* 38K */
                        { 0x02, 0x00 },	/* 56K */
                        { 0x01, 0x00 }	/* 116K */
};

/* Baud Rates */
#define _COM_300_	0
#define _COM_1200_	1
#define _COM_2400_	2
#define _COM_9600_	3
#define _COM_19K_	4
#define _COM_38K_	5
#define _COM_56K_	6
#define _COM_115K_	7

/* Parity */
#define _COM_NOPARITY_		0
#define _COM_ODDPARITY_		1
#define _COM_EVENPARITY_	2

/* Stopbits */
#define _COM_STOP1_                     0
#define _COM_STOP2_                     1
#define _COM_STOP1_5_           1

/* word length */
#define _COM_CHR5_      0
#define _COM_CHR6_      1
#define _COM_CHR7_      2
#define _COM_CHR8_      3

/* word length */
#define _COM_FIFO1_                     0
#define _COM_FIFO4_                     1
#define _COM_FIFO8_                     2
#define _COM_FIFO14_            3

#define _COM_PORT1_             0x38000000

#define USE_STUART0
#undef  USE_STUART01
#undef  USE_STUART02
#undef  USE_STUART03

#if defined(USE_STUART0)
static const unsigned long base = 0x38000000;
#elif defined(USE_STUART1)
static const unsigned long base = 0x08400000;
#elif defined(USE_STUART2)
static const unsigned long base = 0x08500000;
#elif defined(USE_STUART3)
static const unsigned long base = 0x08600000;
#endif

#define uart_put(p, v)	writeb(base + (p), v)
#define uart_get(p)		readb(base + (p))

/*\
  *This function checks the existence of a port.
  * It is very simple. Take the port address then write to the scratch pad
  * at the read it back. if the data read back the same as one that was written then
  * return TRUE else return FALSE.
\*/
int
check_port(com_port)
unsigned long com_port;
{
        int i;

        printf("Checking for port %xH\n", com_port);
        /* Write 1010 1010(0xaa) to scratch pad */
        printf("Writing AAH in %xH\n", com_port);
        writeb(com_port + SCR, 0xaa);

        /* read it back. if it same then return TRUE */
        i = readb(com_port + SCR);

        printf("Read back %xH from %xH\n", i, com_port);

        if(i == 0xaa)
                return TRUE;
        else
                return FALSE;
}


/*\
 * This is the work horse function which actually setups the UART.
 * It needs to know every thing
 \*/
int
init_uart(port, baud, parity, data, stop, fifo, trigger)
int port, baud, parity, data, stop, fifo, trigger;
{
        char lcr_byte;

        /* Set divisor latch */
        writeb(port + LCR, 0x80);

        printf("Devisor Latch is %2xH %2xH (High Low)\n", baud_table[baud][1], baud_table[baud][0]);
        writeb(port + DIVLSB, baud_table[baud][0]);
        writeb(port + DIVMSB, baud_table[baud][1]);

        /* Reset to normal Programming */
        /* Program the lcr_byte for the above parameters */
        lcr_byte = 0x00;
        lcr_byte = data;                /* Set the bit0 & bit1 for word length */
        lcr_byte |= stop;               /* Set the bit2 for stop bit */
        if(parity != _COM_NOPARITY_) {
                lcr_byte |= 1 << 4;     /* Set the bit3 for parity */
                if(parity == _COM_EVENPARITY_)
                        lcr_byte |= 1 << 5;     /* Set the bit4 for EVEN parity */
        }
        printf("LCR byte is %2xH\n", lcr_byte);
        /* Program LCR */
        writeb(port + LCR, lcr_byte);

        if(fifo) {
                char fifo_byte;

                printf("Programming FIFOs without DMA mode\n");

                /* Have to first set the fifo enable */
                fifo_byte = 0x01;
                writeb(port + FCR, fifo_byte);

                /* Now program the FIFO */
                fifo_byte = 0x07;       /* set bit0 - FIFO enable, Reset RCVR and XMIT FIFO */
                fifo_byte |= trigger << 7;      /* set bit6 and bit7 with the trigger level */


                /* Program FCR */
                writeb(port + FCR, fifo_byte);
                if(!(readb(port + ISR) & 0xc0)) {
                        printf("This port %4xH does not have FIFOs\n");
                        printf("Hence did not program Enable FIFOs\n");
                }
        }

        /* Program IER */
        printf("Programming IER for interrupt on bit0 RCV holding Register\n");
        writeb(port + IER, 0x01);

        return TRUE;
}

void uart_test(void)
{
        init_uart(_COM_PORT1_ ,_COM_115K_,_COM_NOPARITY_, _COM_CHR8_,_COM_STOP1_,TRUE,_COM_FIFO1_);
}

/*\
  * It first generates a random number for the data size to be generated.
  * Then generates a random data whose length is equal to the data size.
  * It puts it out on both the ports and polls for the interrupt to occur.
  * It reads both the ports unitl all characters are received OR a timeout
  * has occured. It then prints out the error Message if any.
  * This loop is done for ever.
\*/
void test552(p1, p2)
unsigned long p1, p2;
{
        int i, j, c, w, n;
        unsigned char outbuf[20], inbuf1[20], inbuf2[20];
        unsigned char pbuf[200];
        unsigned long timeout, pass;

        printf("ST16C552 External Loop Test Beginning\n");
        printf("        Testing ports %4x and %4x\n\n", p1, p2) ;
        printf("        Programing ports for 56K,8 bit,no parity,1 stop bit,FIFO trigger level 01\n");
        printf("        This program uses POLLED mode for testing\n");
        printf("        Press Cntrl-C to stop the testing and quit\n");
        printf("        Note: The ports will remain at the above settings after the TEST\n");

        /* Programming ports for 8 bits, no parity, 56K baud, FIFO enabled at level 01 */
        /* Program first port */
        printf("        Programming port %x4\n", p1);
        init_uart(p1,_COM_115K_,_COM_NOPARITY_, _COM_CHR8_,_COM_STOP1_,TRUE,_COM_FIFO1_);

#if 0
        /* Program Second Port */
        printf("        Programming port %x4\n",p2);
        init_uart(p2,_COM_115K_,_COM_NOPARITY_, _COM_CHR8_,_COM_STOP1_,TRUE,_COM_FIFO1_);
#endif

        printf("Starting test\n");
        for (pass = 1 ; ; pass++) {
                /* generate random size for data */
                //n = rand() ;
                n = 6;
                n += n >> 8 ;
                n &= 0x0f ;
                /* Make sure we never get a 0 as the random size data */
                if(n != 0x0f)
                        n++;
                /* generate random data */
                for (w = 0 ; w < n ; w++) {
                        //c = rand() ;
                        c = 7;
                        c += c >> 8 ;
                        c &= 0xff ;
                        c |= 0x01 ; /* no NULLs allowed */
                        outbuf[w] = c ;
                }
                outbuf[w] = NULL;
                printf("******** Pass %10ld Sending %d *********\n", pass, n) ;

                /* Transmitt the data */
                for (i = 0 ; i < n ; i++ ) {
                        writeb(p1, outbuf[i]) ;
                        //writeb(p2, outbuf[i]) ;
                }
                printf("DEBUG--01\n");

                /* loop waiting for intr pending */
                for ( i = 0;;i++ ) {
                        //if ((~(readb(p1+ISR)) & 0x01) && (~(readb(p2+ISR)) & 0x01))
                        if ((~(readb(p1+ISR)) & 0x01))
                        break;
                }
                printf("DEBUG--02\n");

                /* receive data until all has been received OR timeout */
                timeout = 0x0008F ;
                for (i = j = 0; ((i < 20) && (j < 20));) {
                        if (readb(p1+LSR) & 0x01) inbuf1[i++] = readb(p1) ;
                        //c = rand() ;
                        c = 'c';
                        c += c >> 8 ;
                        c &= 0x001f ;
                        c++ ;
                        #if 0
                        for (; c != 0; c--) ;
                        if (readb(p2+LSR) & 0x01) inbuf2[j++] = readb(p2) ;
                        #endif
                        if (timeout-- == 0) break ;
                }

                #if 0
                /* If timed out then print message else comparse data */
                if(timeout == 0 )
                        printf("Timed out on Ports\n");
                else {
                        inbuf1[i] = inbuf2[j] = NULL;
                        /* compare results */
                        if (StrCmp(outbuf, inbuf1) || ( i != n)) {
                                printf("\nError:%04x Sent: ", p2) ;
                                for ( w = 0; w < n; w++ )
                                        printf(" %02x", outbuf[w]) ;
                                printf("\n%04x Received:", p1) ;
                                for ( w = 0; w < i; w++ )
                                        printf(" %02x", inbuf1[w]) ;
                                printf("\n") ;
                        }

                        if (StrCmp(outbuf, inbuf2) || ( j != n )) {
                                printf("\nError:%04x Sent: ", p1);
                                for ( w = 0; w < n; w++ )
                                        printf(" %02x", outbuf[w]) ;
                                printf("\n%04x Received:", p2) ;
                                for ( w = 0; w < j; w++ )
                                        printf(" %02x", inbuf2[w]) ;
                                printf("\n") ;
                        }
                }
                #endif
        }
}



/*\
  * This is the test mode.
  * It gets the address of the ports checks to see if they are there.
  * Note: If a driver already exits I am not sure how to temporarily remove it.
  * Well we will worry about it later.
  * Wam the user to remove any drivers that are on the ports.
  * Especially the mouse driver.
  * pass the address to the test552 routine.
\*/
int test_mode()
{
        int i, j, k;    /* generic variables */
        //char port1[10], port2[10];
        char port1, port2;
        unsigned long pt1, pt2; /* this are the integer port numbers */

        void test552();

        printf("WARNING: This program will not work if the ports to be tested\n");
        printf("                                        have drivers installed in them. e.g Mouse driver\n");
        printf("                                        Please remove the drivers before doing this test.\n");

        while(TRUE) {
                printf("First Port Address (In HEX) > ");
                #if 0
                scanf("%s", port1);
                pt1 = strtol(port1, NULL, 16);
                fflush(stdin);
                #else
                //port1 = uart_getc();
                //pt1 = StrToInt(port1);
                #endif

                pt1 = _COM_PORT1_;
                pt2 = 0x38000000;
                /*\
                  * Check if this port exits. else loop
                 \*/
                 if(check_port(pt1))
                        break;
                 printf("Error: Port %xH does not exist. Try again\n", pt1);
        }

#if 0
        while(TRUE) {
                printf("Second Port Address (In HEX) > ");
                #if 0
                scanf("%s", port2);
                pt2 = strtol(port2, NULL, 16);
                fflush(stdin);
                #else
                port2 = uart_getc();
                pt2 = StrToInt(port1);
                #endif

                /*\
                  * Check if this port exits. else loop
                 \*/
                 if(check_port(pt2))
                        break;
                 printf("Error: Port %4xH does not exist. Try again\n", pt2);
        }
#endif

        /* Test 554 with the two port addresses */
        test552(pt1, pt2);

        return TRUE;
}

int st16c554_putc(int ch)
{
        printf("base=%x\n", base);
        if(ch == '\n') st16c554_putc('\r');
        // FIFO에 데이타가 없을때까지 기다린다.
        while (!(uart_get(LSR) & LSR_TDRQ));
                uart_put(THR, ch);
                printf("ch=%x\n", ch);
        return ch;
}

int st16c554_getc(void)
{
        int ch = -1;
        if (uart_get(LSR) & LSR_DR)
                ch = uart_get(RHR);
                printf("LSR's Value=%x\n", uart_get(LSR));
        if (ch == '\r') ch = '\n';
        return ch;
}

int st16c554_puts(const char *s){
        const char *t = s;
        while (*t) st16c554_putc(*t++);
        return t - s;
}

int st16c554_putb(int ch){
        while (!(uart_get(LSR) & LSR_TDRQ));
        uart_put(THR, ch);
        return ch;
}

int st16c554_getb(void){
        int ch = -1;
        if (uart_get(LSR) & LSR_DR)
                ch = uart_get(RHR);
        return ch;
}

int st16c554_write(const void *buff, int count){
        const char *s = buff;
        while (count-- > 0){
                while (!(uart_get(LSR) & LSR_TDRQ));
                uart_put(THR, *s++);
        }
        return count;
}

#if 0
int getc(void){
        return st16c554_getc();
}

int putc(int ch){
        return st16c554_putc(ch);
}

int putchar(int ch){
        return st16c554_putc(ch);
}

int puts(const char *s){
        const char *t = s;
        while (*t)
                st16c554_putc(*t++);
        st16c554_putc('\n');
        return t-s+1;
}
#endif
