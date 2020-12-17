#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>  

static int dev;
unsigned short button = 0;

// gpio 버튼에서 데이터를 읽어옴
void my_signal_fun(int signum){  
	read(dev,&button,2);
	printf("%d\n",button); 
} 

int main(void){
	
	int Oflags;

	// signal() 함수를 사용하여 프로그램이 운영체제나 raise() 함수에서 인터럽트 신호를
	// 처리할 수 있는 여러 방법 중 하나를 선택할 수 있다.
    	signal(SIGIO, my_signal_fun);
	
	// 디바이스 open
	dev = open("/dev/gpiobutton",O_RDWR);
	if(dev < 0) {
		printf( "Device Open ERROR!\n");
		return -1;
	}
	printf("Please push the GPIO_0 port!\n");
	//read(dev,&button,2);
	//printf("%d\n",button);
	
	// 해당 디바이스 파일에 대한 소유권을 프로세스에 설정해주어야 한다.
	// F_SETOWN 명령으로 설정, getpid()는 동작중인 프로세스
	// 디스크립터를 얻어오는 함수
	fcntl(dev, F_SETOWN, getpid());  
      
	// FGETFL로 장치파일의 플래그 값을 읽어옴
	Oflags = fcntl(dev, F_GETFL);   

	// 시그널 발생이 가능하도록 fcntl함수의 F_GETFL 명령 사용
	fcntl(dev, F_SETFL, Oflags | FASYNC); 
	
	while(1){
		sleep(1);
		printf("%d\n",button);
		button = 0;
	}
	
	close(dev);
	return 0;
}
