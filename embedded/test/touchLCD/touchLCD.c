#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <termios.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // open/close
#include <fcntl.h> // O_RDWR
#include <sys/ioctl.h> // ioctl
#include <sys/mman.h> // mmap PROT_
#include <linux/fb.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include "bmp_header.h"		// bmp structure 

#define RGB565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

// touch 관련 변수 정의
#define EVENT_BUF_NUM 2
#define Y_WIDTH 1275
#define X_WIDTH 797.5
#define X_OFFSET 4920-X_WIDTH/2
#define Y_OFFSET 13150+Y_WIDTH/2

int x_detected, y_detected;        // touch 받아서 detecting한 좌표 저장하는 변수
 
#define FBDEV_FILE "/dev/fb0"
#define TURN     1
#define STONE_CURSOR    4
#define BOARD_WIDTH     9
#define BOARD_HEIGHT    9

// LCD 관련 정의
#define LCD_FINIT 0
#define LCD_SINIT 1
#define LCD_PRINT 2
 
// LCD 화면 프레임 저장 배열
unsigned short frame[384000];
unsigned short csframe[384000];
unsigned short backframe[384000];

// 바둑판 각 좌표별 상태 저장 이중 배열
unsigned int board_status[BOARD_HEIGHT][BOARD_WIDTH] = { { 0 } };


unsigned int turn = TURN;
unsigned int clock_count;
unsigned int sec_saved;
unsigned int sec_present;
struct timeval val;
struct tm *ptm;

// keyboard
static struct termios initial_settings, new_settings;
static int peek_character = -1;

// 터치 받을 때 시간 넘어가면 통과할 때 사용
int poll_state;
struct pollfd    poll_events;

// screen
int screen_width;
int screen_height;
int bits_per_pixel;
int line_length;
struct fb_var_screeninfo fbvar;
struct fb_fix_screeninfo fbfix;
int mem_size;

// 파일 지정자
int fb_fd;
int event_fd;

unsigned char *pfbmap;

// TextLCD 에 표시되는 값들에 관한 정보 저장 구조체
struct strcommand_varible strcommand;
struct strcommand_varible {
	char rows;
	char nfonts;
	char display_enable;
	char cursor_enable;
	char nblink;
	char set_screen;
	char set_rightshit;
	char increase;
	char nshift;
	char pos;
	char command;
	char strlength;
	char buf[16];
};

struct strcommand_varible strcommand;
struct input_event event_buf[EVENT_BUF_NUM];

// 키보드 입력 설정
void init_keyboard(){
	tcgetattr(0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
}

// 더 이상 키보드를 사용하지 않을 때 사용
void close_keyboard(){
	tcsetattr(0, TCSANOW, &initial_settings);
}

// 키보드 입력이 발생했을 때 1을 반환
int kbhit(){
	char ch;
	int nread;

	if (peek_character != -1)
		return 1;

	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	if (nread == 1){
		peek_character = ch;
		return 1;
	}

	return 0;
}

// 키보드에서 문자를 읽음
int readch(){
	char ch;

	if (peek_character != -1){
		ch = peek_character;
		peek_character = -1;
		return ch;
	}
	read(0, &ch, 1);
	return ch;
}


// ms delay 함수
void m_delay(int num) {
	volatile int i,j;
	for(i=0;i<num;i++)
		for(j=0;j<16384;j++);
}

int GetTouch(void) {
	int i;
	size_t read_bytests;
	struct input_event event_bufts[EVENT_BUF_NUM];
	int x, y;

	if ((event_fd = open("/dev/input/event2", O_RDONLY)) < 0) {
		printf("open error");
		exit(1);
	}

	poll_events.fd        = event_fd;
	poll_events.events    = POLLIN | POLLERR;        // 수신된 자료가 있는지, 에러가 있는지
	poll_events.revents   = 0;

	poll_state = poll(                               // poll()을 호출하여 event 발생 여부 확인
		(struct pollfd*)&poll_events,      	 // event 등록 변수
		1,   // 체크할 pollfd 개수
		10    // time out 시간
	);

	if(poll_events.revents == POLLIN) {
		read_bytests = read(event_fd, event_bufts, ((sizeof(struct input_event))*EVENT_BUF_NUM));

		for (i = 0; i<(read_bytests / sizeof(struct input_event)); i++) {
			switch (event_bufts[i].type) {
			case EV_ABS:
				switch (event_bufts[i].code) {
				case ABS_X:
					y = event_bufts[i].value;
					break;

				case ABS_Y:
					x = event_bufts[i].value;
					break;
			
				default:
					break;
				}
			break;

			default:
				break;

			}
		}

		if(x>0 && x<16000 && y>0 && y<16000){
			x_detected = x * 800 / 16000;
			y_detected = 480 - (y * 480 / 16000);

		}else{
			y_detected = -1;
			x_detected = -1;
		}

	        close(event_fd);
	}

	else {
		close(event_fd);
		return -1;
	}
	return 0;
}

void LCDinit(int inp) {
	unsigned short *ptr;
	int coor_y;
	int coor_x;
	int i;
	int j = 0;
	int cols = 800, rows = 480;

	if (access(FBDEV_FILE, F_OK)) {
		printf("%s: access error\n", FBDEV_FILE);
		exit(1);
	}

	if ((fb_fd = open(FBDEV_FILE, O_RDWR)) < 0) {
		printf("%s: open error\n", FBDEV_FILE);
		exit(1);
	}

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbvar)) {
		printf("%s: ioctl error - FBIOGET_VSCREENINFO \n",
			FBDEV_FILE);
		exit(1);
	}

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbfix)) {
		printf("%s: ioctl error - FBIOGET_FSCREENINFO \n",FBDEV_FILE);
		exit(1);
	}

    	screen_width = fbvar.xres; // 스크린의 픽셀 폭
	screen_height = fbvar.yres; // 스크린의 픽셀 높이
	bits_per_pixel = fbvar.bits_per_pixel; // 픽셀 당 비트 개수
	line_length = fbfix.line_length; // 한개 라인 당 바이트 개수
	mem_size = screen_width * screen_height * 2;

	pfbmap = (unsigned char *)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
}

// device driver들 로드
void init_devices() {
	strcommand.rows = 0;
	strcommand.nfonts = 0;
	strcommand.display_enable = 1;
	strcommand.cursor_enable = 0;
	strcommand.nblink = 0;
	strcommand.set_screen = 0;
	strcommand.set_rightshit = 1;
	strcommand.increase = 1;
	strcommand.nshift = 0;
	strcommand.pos = 10;
	strcommand.command = 1;
	strcommand.strlength = 16;
}

// 배경을 하나의 color로 채움.
void Fill_Background(unsigned short color){
	int x, y;

	for (y = 0; y < 480; y++){
		for (x = 0; x < 800; x++){
			*(unsigned short *)(pfbmap + (x) * 2 + (y) * 800 * 2) = color;
		}
	}
}

void draw_circle(int sx, int sy){
	int coor_x, coor_y;
	int screen_width;
	unsigned short *ptr;
	screen_width = fbvar.xres;

	for (coor_y = 0; coor_y < 10; coor_y++){
		ptr = (unsigned short *)pfbmap + (screen_width * sy + sx) + (screen_width * coor_y);
		for (coor_x = 0; coor_x < 10; coor_x++){
			*ptr++ = (unsigned short)RGB565(255, 255, 255);
		}
	}
}

// bmp file을 읽어서 저장
void read_bmp(char *filename, char **pDib, char **data, int *cols, int *rows){
	BITMAPFILEHEADER bmpHeader;
	BITMAPINFOHEADER *bmpInfoHeader;
	unsigned int size;
	unsigned char ID[2];
	int nread;
	FILE *fp;
	fp = fopen(filename,"rb");
	
	// 파일을 열지 못했을 경우 함수 종료
	if(fp == NULL) {
		printf("ERROR\n");
		return;
	}

	// bmp file인지 확인. 시작 지점이 B, M으로 시작하지 않으면 함수 종료
	ID[0] = fgetc(fp);
	ID[1] = fgetc(fp);
	if(ID[0] != 'B' && ID[1] != 'M') {
		fclose(fp);
		return;
	}

	// fp에서 bitmapfileheader의 크기만큼 read
	nread = fread(&bmpHeader.bfSize,1,sizeof(BITMAPFILEHEADER),fp);
	size = bmpHeader.bfSize - sizeof(BITMAPFILEHEADER);
	*pDib = (unsigned char *)malloc(size);

	// fp에서 size의 크기만큼 pDib에 저장
	fread(*pDib,1,size,fp);
	bmpInfoHeader = (BITMAPINFOHEADER *)*pDib;
	
	// 24bit bitmap file인지 확인
	if(24 != bmpInfoHeader->biBitCount){
		printf("It supports only 24bit bmp!\n");
		fclose(fp);
		return;
	}

	// 가로, 세로의 길이, image data을 저장
	*cols = bmpInfoHeader->biWidth;
	*rows = bmpInfoHeader->biHeight;
	*data = (char *)(*pDib + bmpHeader.bfOffBits - sizeof(bmpHeader)-2);
	fclose(fp);
}

// bmp을 닫는 함수
void close_bmp(char **pDib){
	free(*pDib);
}

void fill_rgb(unsigned short *data_rgb, int r, int g, int b){
	int x, y;

	int rows = 480;
	int cols = 800;

	for(y=0; y<rows; y++){
		for(x=0; x<cols; x++){
			data_rgb[y*cols + x] = (unsigned short)RGB565(r, g, b);

		}
	}
}

void draw_rgb(unsigned short *data_rgb, int sx, int sy){
	int x, y;
	int xx, yy;

	int rows = 480;
	int cols = 800;

	for(y=0; y<10; y++){
		for(x=0; x<10; x++){
			xx = sx+x;
			yy = sy+y;
	
			if(xx<800 && yy<480){
				data_rgb[yy*cols + xx] = (unsigned short)RGB565(0, 0, 0);
			}	
		}
	}
}

int fb_display(unsigned short *rgb, int sx, int sy, int width, int height){
	int coor_x, coor_y;
	int screen_width;
	unsigned short *ptr;

	screen_width = fbvar.xres;

	for (coor_y = 0; coor_y < height; coor_y++){
		ptr = (unsigned short *)pfbmap + (screen_width * sy + sx) + (screen_width * coor_y);
		for (coor_x = 0; coor_x < width; coor_x++){
			*ptr++ = rgb[coor_x + coor_y * width];
		}
	}
	return 0;
}

int main(void) {
	int i = 0, j = 0;
	unsigned int led_cnt;
	unsigned int seg_count;
	unsigned short ch = 0;
     
	init_devices();
	LCDinit(LCD_FINIT);
	init_keyboard();

	unsigned short data_rgb[800*480*2];
	fill_rgb(data_rgb, 255, 255, 255);
	
	Fill_Background(0x0000);
	while (ch != 'q') {
		// 키보드 입력이 발생하면 문자를 읽음.
		if (kbhit()){
			ch = readch();
		}

		if(GetTouch() != -1) {
			if(x_detected!=-1 && y_detected!=-1) {
				//printf("y : %d\n", y_detected);
				//printf("x : %d\n\n", x_detected);

				int mg = 50;
				if(x_detected > mg && x_detected < 800 - mg && y_detected > mg && y_detected < 480 - mg){
					// 터치한 위치의 스크린에 출력
					draw_circle(x_detected, y_detected);

					// 터치한 위치에 해당하는 영역에 검정색을 채움
					draw_rgb(data_rgb, x_detected, y_detected);
				}
			}
		}
	}
	// 화면을 검정으로 클리어
	Fill_Background(0x0000);

	ch = 'd';

	// rgb에 저장한 파일을 화면에 출력 
	fb_display(data_rgb, 0, 0, 800, 480);


	/////////////////////////////////////////////////////////////////////////////////////////////
	// mnist 할 거면 여기서 하기 


	// q을 누르면 배경을 검정으로 채우고 종료 
	while (ch != 'q') {
		// 키보드 입력이 발생하면 문자를 읽음.
		if (kbhit()){
			ch = readch();
		}
	}

	Fill_Background(0x0000);

	close_keyboard();

	return 0;
} 
