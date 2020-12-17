#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <termios.h>
#include <unistd.h>
#include <linux/fb.h> 	/* for fb_var_screeninfo, FBIOGET_VSCREENINFO*/
#include <unistd.h>
#include <signal.h>  
#include <linux/input.h>
#include <errno.h>
#include <sys/poll.h>

#include "cv.h"
#include "highgui.h"
#include "bmp_header.h"
#include "svm.h"

#define RGB565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define FBDEV_FILE "/dev/fb0"
#define CAMERA_DEVICE "/dev/camera"
#define FILE_NAME "face_image.jpg"
#define SIZE 240 		// 화면에 띄울 얼굴 영역 이미지의 가로세로 크기
#define EVENT_BUF_NUM 64

// mnist
struct svm_node *x;
int max_nr_attr = 64;
struct svm_model *model;
int predict_probability = 0;
static char *line = NULL;
static int max_line_len;

//svm predict
int print_null(const char *s, ...) { return 0; }
static int (*info)(const char *fmt, ...) = &printf;

 
#define FBDEV_FILE "/dev/fb0"
#define TURN     1
#define STONE_CURSOR    4
#define BOARD_WIDTH     9
#define BOARD_HEIGHT    9

unsigned int turn = TURN;
unsigned int clock_count;
unsigned int sec_saved;
unsigned int sec_present;
struct timeval val;
struct tm *ptm;

// 터치 받을 때 시간 넘어가면 통과할 때 사용
int poll_state;
struct pollfd    poll_events;
int x_detected, y_detected;        // touch 받아서 detecting한 좌표 저장하는 변수

// facedetect
static CvMemStorage *storage = 0;
static CvHaarClassifierCascade *cascade = 0;
const char *cascade_xml = "haarcascade_frontalface_alt2.xml";

// screen
int screen_width;
int screen_height;
int bits_per_pixel;
int line_length;
struct fb_var_screeninfo fbvar;
struct fb_fix_screeninfo fbfix;
int mem_size;
unsigned char *pfbmap;

// 파일 지정자
int fb_fd;
int event_fd;

// 카메라 이미지
unsigned short cis_rgb[320 * 240 * 2]; 

// keyboard
static struct termios initial_settings, new_settings;
static int peek_character = -1;

// gpiobutton
static int dev_gpio;
unsigned short button = 0;

// game
int mode = 0;
int mode_copy = -1;
int color = 0;
int sy = 0, sx = 0;

// gpiobutton function
void my_signal_fun(int signum){  
	read(dev_gpio,&button,2);
	usleep(500000);
}

// 플레이어의 정보를 저장하기 위한 구조체
#define MAX_LEN 100
struct _node{
	char name[MAX_LEN];
	int score;
	int save;
	int hard;
	struct _node *next;
}node;

// list을 만들기 위한 head, tail 선언
struct _node *head, *tail;

// event가 발생했을 때 값을 저장할 변수
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

// mnist에서 input file을 작성하는 함수
int write_input_file(float *input){
	int x, y;
	FILE *mnist_input_f;
	if ((mnist_input_f = fopen("mnist_input.txt", "w")) == NULL){
		return 0;
	}

	fprintf(mnist_input_f, "%d", 3);

	for (y = 0; y < 28; y++){
		for (x = 0; x < 28; x++){
			if (input[y * 28 + x] > 0){
				fprintf(mnist_input_f, " %d:%f", (y * 28 + x), input[y * 28 + x]);
			}
		}
		//printf("\n");
	}
	fclose(mnist_input_f);
	return 1;
}

//input : resize 후 colorimage 3channel
IplImage *Color_to_Grayscale_Resize(IplImage *img){
	int grayVal;
	IplImage *imgGray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	IplImage *small_img = cvCreateImage(cvSize(28, 28), 8, 1);
	int x, y = 0;
	cvCvtColor(img, imgGray, CV_BGR2GRAY);
	cvResize(imgGray, small_img, CV_INTER_LINEAR);
	cvReleaseImage(&imgGray);
	return small_img;
}

// 배열의 값을 0~1 사이의 값으로 변환
float *Normalization(IplImage *src, float *dst){
	int x, y, min, max;
	min = 255;
	max = 0;

	// 배열에서 최솟값을 구함
	for (y = 0; y < src->height; y++){
		for (x = 0; x < src->width; x++){
			if (min > src->imageData[x + y * src->widthStep]){
				min = src->imageData[x + y * src->widthStep];
			}
		}
	}

	for (y = 0; y < src->height; y++){
		for (x = 0; x < src->width; x++){
			src->imageData[x + y * src->widthStep] -= min;
		}
	}

	// 배열에서 최댓값을 구함
	for (y = 0; y < src->height; y++){
		for (x = 0; x < src->width; x++){
			// printf("%3d ", src->imageData[x + y * src->widthStep]);
			if (max < src->imageData[x + y * src->widthStep]){
				max = src->imageData[x + y * src->widthStep];
			}
		}
		// printf("\n");
	}
	// printf("MAX: %d\n", max);

	for (y = 0; y < src->height; y++){
		for (x = 0; x < src->width; x++){
			if ((float)(src->imageData[x + y * src->widthStep]) / (float)max > 0.35){
				dst[(y * src->widthStep) + x] = 0;
			}else{
				dst[(y * src->widthStep) + x] = 1 - (float)(src->imageData[x + y * src->widthStep]) / (float)max;
			}
			// printf("%4.3f ", dst[x + y * src->widthStep]);
		}
		// printf("\n");
	}
	return dst;
} //bigger than 100 ->0

// 파일에서 한 줄씩 문자열을 읽는 함수
static char *readline(FILE *input){
	int len;

	if (fgets(line, max_line_len, input) == NULL)
		return NULL;

	while (strrchr(line, '\n') == NULL){
		max_line_len *= 2;
		line = (char *)realloc(line, max_line_len);
		len = (int)strlen(line);
		if (fgets(line + len, max_line_len - len, input) == NULL)
			break;
	}
	return line;
}

// 오류가 발생했을 때 메시지를 출력하는 함수
void exit_input_error(int line_num){
	fprintf(stderr, "Wrong input format at line %d\n", line_num);
	exit(1);
}

// mnist에서 파일을 입력받아 인식한 값을 출력하는 함수
int predict(FILE *input, FILE *output){
	int correct = 0;
	int total = 0;
	double error = 0;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

	int svm_type = svm_get_svm_type(model);
	int nr_class = svm_get_nr_class(model);
	double *prob_estimates = NULL;
	int j;

	int player_num = 0;

	if (predict_probability){
		if (svm_type == NU_SVR || svm_type == EPSILON_SVR)
			info("Prob. model for test data: target value = predicted value + z,\nz: Laplace distribution e^(-|z|/sigma)/(2sigma),sigma=%g\n", svm_get_svr_probability(model));
		else
		{
			int *labels = (int *)malloc(nr_class * sizeof(int));
			svm_get_labels(model, labels);
			prob_estimates = (double *)malloc(nr_class * sizeof(double));
			fprintf(output, "labels");

			for (j = 0; j < nr_class; j++)
				fprintf(output, " %d", labels[j]);

			fprintf(output, "\n");
			free(labels);
		}
	}

	max_line_len = 1024;
	line = (char *)malloc(max_line_len * sizeof(char));

	while (readline(input) != NULL){
		int i = 0;
		double target_label, predict_label;
		char *idx, *val, *label, *endptr;
		int inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0

		label = strtok(line, " \t\n");
		if (label == NULL) // empty line
			exit_input_error(total + 1);

		target_label = strtod(label, &endptr);
		if (endptr == label || *endptr != '\0')
			exit_input_error(total + 1);

		while (1)
		{
			if (i >= max_nr_attr - 1) // need one more for index = -1
			{
				max_nr_attr *= 2;
				x = (struct svm_node *)realloc(x, max_nr_attr * sizeof(struct svm_node));
			}

			idx = strtok(NULL, ":");
			val = strtok(NULL, " \t");

			if (val == NULL)
				break;

			errno = 0;
			x[i].index = (int)strtol(idx, &endptr, 10);

			if (endptr == idx || errno != 0 || *endptr != '\0' || x[i].index <= inst_max_index)
				exit_input_error(total + 1);
			else
				inst_max_index = x[i].index;

			errno = 0;
			x[i].value = strtod(val, &endptr);

			if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
				exit_input_error(total + 1);

			++i;
		}
		x[i].index = -1;

		if (predict_probability && (svm_type == C_SVC || svm_type == NU_SVC))
		{
			predict_label = svm_predict_probability(model, x, prob_estimates);
			fprintf(output, "%g", predict_label);

			for (j = 0; j < nr_class; j++)
				fprintf(output, " %g", prob_estimates[j]);
			fprintf(output, "\n");
		}
		else
		{
			predict_label = svm_predict(model, x);
			fprintf(output, "%.17g\n", predict_label);
		}

		player_num = (int)predict_label;

		if (predict_label == target_label)
			++correct;

		error += (predict_label - target_label) * (predict_label - target_label);
		sump += predict_label;
		sumt += target_label;
		sumpp += predict_label * predict_label;
		sumtt += target_label * target_label;
		sumpt += predict_label * target_label;
		++total;
	}

	if (svm_type == NU_SVR || svm_type == EPSILON_SVR)
	{
        	info("Mean squared error = %g (regression)\n", error / total);
        	info("Squared correlation coefficient = %g (regression)\n",
          	   ((total * sumpt - sump * sumt) * (total * sumpt - sump * sumt)) /
               	 	((total * sumpp - sump * sump) * (total * sumtt - sumt * sumt)));
    	}	
    	else
        	//	info("Accuracy = %g%% (%d/%d) (classification)\n",
        	//		(double)correct/total*100,correct,total);
        	if (predict_probability)
        		free(prob_estimates);

    	return player_num;
}

// touch event가 발생했을 때 좌표를 저장하는 함수
int GetTouch(void) {
	int i;
	size_t read_bytests;
	struct input_event event_bufts[EVENT_BUF_NUM];
	int x = 0, y = 0;

	// event device open
	if ((event_fd = open("/dev/input/event2", O_RDONLY)) < 0) {
		printf("open error");
		exit(1);
	}

	poll_events.fd        = event_fd;
	poll_events.events    = POLLIN | POLLERR;          // 수신된 자료가 있는지, 에러가 있는지
	poll_events.revents   = 0;

	poll_state = poll(                                    // poll()을 호출하여 event 발생 여부 확인
		(struct pollfd*)&poll_events,       // event 등록 변수
		1,   // 체크할 pollfd 개수
		10    // time out 시간
	);

	// POLLIN event가 발생했을 때 실행
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

		// 0~16000 사이의 값으로 저장되는 x, y의 값을
		// 스크린의 800x480 크기로 변환
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

// 스크린을 사용하기 위한 설정
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

// ms delay 함수
void m_delay(int num) {
	volatile int i,j;
	for(i=0;i<num;i++)
		for(j=0;j<16384;j++);
}

// 스크린에 image을 출력
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

// lplImage의 image을 RGB565 형식으로 변환
void cvIMG2RGB565(IplImage *img, unsigned short *cv_rgb, int ex, int ey, int width)
{
	int x, y;
	unsigned char r, g, b;

	for (y = 0; y < ey; y++){
		for (x = 0; x < ex; x++){
			b = (img->imageData[(y * img->widthStep) + x * 3]);
			g = (img->imageData[(y * img->widthStep) + x * 3 + 1]);
			r = (img->imageData[(y * img->widthStep) + x * 3 + 2]);
			//cv_rgb[y * 320 + x] = (unsigned short)RGB565(r, g, b);
			cv_rgb[y * width + x] = (unsigned short)RGB565(r, g, b);
		}
	}
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

// RGB 형태의 image을 lplImage 형태로 변환
void RGB2cvIMG(unsigned short *rgb, IplImage *img, int ex, int ey){
	int x, y;

	for (y = 0; y < ey; y++){
		for (x = 0; x < ex; x++){
			(img->imageData[(y * img->widthStep) + x * 3]) = (rgb[y * ex + x] & 0x1F) << 3;               //b
			(img->imageData[(y * img->widthStep) + x * 3 + 1]) = ((rgb[y * ex + x] & 0x07E0) >> 5) << 2;  //g
			(img->imageData[(y * img->widthStep) + x * 3 + 2]) = ((rgb[y * ex + x] & 0xF800) >> 11) << 3; //r
		}
	}
}

// touch event가 발생했을 때 event가 발생한 좌표 주위의 스크린에 검정색을 출력
void draw_circle(int sx, int sy){
	int coor_x, coor_y;
	int screen_width;
	unsigned short *ptr;
	screen_width = fbvar.xres;

	int r = 0;
	int g = 0;
	int b = 0;
	int pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);;

	for (coor_y = 0; coor_y < 15; coor_y++){
		ptr = (unsigned short *)pfbmap + (screen_width * sy + sx) + (screen_width * coor_y);
		for (coor_x = 0; coor_x < 15; coor_x++){
			*ptr++ = pixel;
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

// mnist 입력을 위한 정사각형 rgb을 초기화
void fill_rgb(unsigned short *data_rgb, int r, int g, int b){
	int x, y;

	int rows = 300;
	int cols = 300;

	for(y=0; y<rows; y++){
		for(x=0; x<cols; x++){
			data_rgb[y*cols + x] = (unsigned short)RGB565(r, g, b);

		}
	}
}

// mnist 입력을 위한 정사각형에 touch event가 발생하면 배열에 값을 저장
void draw_rgb(unsigned short *data_rgb, int sx, int sy){
	int x, y;
	int xx, yy;

	int rows = 300;
	int cols = 300;

	for(y=0; y<15; y++){
		for(x=0; x<15; x++){
			xx = sx+x;
			yy = sy+y;
	
			if(xx>250 && xx<550 && yy > 90 && yy < 390){
				data_rgb[(yy-90)*cols + (xx-250)] = (unsigned short)RGB565(0, 0, 0);
			}	
		}
	}
}


//--------------------------------------------------
//unsigned short *player1_rgb, unsigned short *player2_rgb 매개변수 추가
//--------------------------------------------------

int detect_and_draw(IplImage *img, unsigned short *player1_rgb, unsigned short *player2_rgb){
	int ret = 0;

	double scale = 1.3;

	CvSize x = cvSize(cvRound(img->width / scale), cvRound(img->height / scale)); // Round: 반올림
	IplImage *gray = cvCreateImage(cvSize(img->width, img->height), 8, 1); // 그레이스케일 이미지를 저장할 Mat
	IplImage *small_img = cvCreateImage(x, 8, 1);                          // resize하여 적당한 크기로 변환할 이미지를 저장할 Mat
	int i, j;

	cvCvtColor(img, gray, CV_BGR2GRAY); // 입력 이미지 img를 그레이스케일로 변환하여 gray에 저장

	cvResize(gray, small_img, CV_INTER_LINEAR); // gray를 resize하여 small_img에 저장

	cvEqualizeHist(small_img, small_img); // 평활화

	cvClearMemStorage(storage); 

	// player
	IplImage *player1 = cvCreateImage(cvSize(SIZE, SIZE), 8, 3);
	IplImage *player2 = cvCreateImage(cvSize(SIZE, SIZE), 8, 3);

	int ix, iy;
	if (cascade){                                // 학습 모델(.xml)이 정상적으로 불러와진 경우
		double t = (double)cvGetTickCount(); // 시간 측정

		// face detecting
		CvSeq *faces = cvHaarDetectObjects(gray, cascade, storage, 1.1, 2, 0 /*CV_HAAR_DO_CANNY_PRUNING*/, cvSize(30, 30)); // 얼굴 인식
		// 한 명이 인식된 경우에만 실행
		if (faces->total == 2){
			// 영상에서 인식하는 사람 수만큼 반복문 실행
			for (i = 0; i < (faces ? faces->total : 0); i++){
				// faces->total: face.sizes()와 같음. 인식된 얼굴 수
			
				CvRect *faceRegion = (CvRect *)cvGetSeqElem(faces, i); // 얼굴 영역

				IplImage *crop = cvCreateImage(cvSize(faceRegion->width + 60, faceRegion->height + 60), 8, 3);

				for (iy = 0; iy < crop->height; iy++){
					for (ix = 0; ix < crop->width; ix++){
						int sy = iy + faceRegion->y - 30;
						int sx = ix + faceRegion->x - 30;

						if (sy >= 0 && sy < img->height && sx >= 0 && sx < img->width){
							crop->imageData[(iy * crop->widthStep) + ix * 3 + 0] = img->imageData[(sy * img->widthStep) + sx * 3 + 0];
							crop->imageData[(iy * crop->widthStep) + ix * 3 + 1] = img->imageData[(sy * img->widthStep) + sx * 3 + 1];
							crop->imageData[(iy * crop->widthStep) + ix * 3 + 2] = img->imageData[(sy * img->widthStep) + sx * 3 + 2];
						}
						else{
							crop->imageData[(iy * crop->widthStep) + ix * 3 + 0] = 255;
							crop->imageData[(iy * crop->widthStep) + ix * 3 + 1] = 255;
							crop->imageData[(iy * crop->widthStep) + ix * 3 + 2] = 255;
						}
					}
				}

				// player1
				if (i == 0){
					cvResize(crop, player1, CV_INTER_LINEAR);
					cvIMG2RGB565(player1, player1_rgb, player1->width, player1->height, player1->width);
					//fb_display(player1_rgb, 435, 120, player1->width, player1->height);
					//cvSaveImage("player1.jpg", player1);
				}

				// player2
				if (i == 1){
					cvResize(crop, player2, CV_INTER_LINEAR);
					cvIMG2RGB565(player2, player2_rgb, player2->width, player2->height, player2->width);
					//fb_display(player2_rgb, 435, 270, player2->width, player2->height);
					//cvSaveImage("player2.jpg", player2);
				}

				ret++; // 얼굴 수 update
			}
		}
		else{
			printf("not two peoples\n");
		}
	}

	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);
	cvReleaseImage(&player1);
	cvReleaseImage(&player2);

	return ret;
}
// data에 있는 데이터를 bmpdata에 저장
void write_bmp(unsigned short *bmpdata, char *data, int cols, int rows){
	int i, j, k, t;
	int r, g, b;
	int pixel;

	for(j=0;j<rows;j++){
		k = j*cols*3;
		t = (rows -1 - j)*cols;
		for(i=0;i<cols;i++){
			b = *(data + (k + i*3));
			g = *(data + (k + i*3+1));
			r = *(data + (k + i*3+2));
			pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
			bmpdata[t+i] = pixel;
		}
	}
}

// bmp 파일을 닫는 함수 
void close_bmp(char **pDib){
	free(*pDib);
}

// 게임을 할 때 출력하는 사진과 위치를 지정
void color_position(int mode){
	switch(mode){
	case 0:
		color = 0;
		sy = 0;
		sx = 0;
		break;
	case 1:
		color = 1;
		sy = 0;
		sx = 0;
		break;
	case 2:
		color = 1;
		sy = 0;
		sx = 1;
		break;
	case 3:
		color = 1;
		sy = 0;
		sx = 2;
		break;
	case 4:
		color = 1;
		sy = 0;
		sx = 3;
		break;
	case 5:
		color = 1;
		sy = 1;
		sx = 0;
		break;
	case 6:
		color = 1;
		sy = 1;
		sx = 1;
		break;
	case 7:
		color = 1;
		sy = 1;
		sx = 2;
		break;
	case 8:
		color = 1;
		sy = 1;
		sx = 3;
		break;
	case 9:
		color = 1;
		sy = 2;
		sx = 0;
		break;
	case 10:
		color = 1;
		sy = 2;
		sx = 1;
		break;
	case 11:
		color = 1;
		sy = 2;
		sx = 2;
		break;
	case 12:
		color = 1;
		sy = 2;
		sx = 3;
		break;
	case 13:
		color = 1;
		sy = 3;
		sx = 0;
		break;
	case 14:
		color = 1;
		sy = 3;
		sx = 1;
		break;
	case 15:
		color = 1;
		sy = 3;
		sx = 2;
		break;
	case 16:
		color = 1;
		sy = 3;
		sx = 3;
		break;
	case 17:
		color = 2;
		sy = 0;
		sx = 0;
		break;
	case 18:
		color = 2;
		sy = 0;
		sx = 1;
		break;
	case 19:
		color = 2;
		sy = 0;
		sx = 2;
		break;
	case 20:
		color = 2;
		sy = 0;
		sx = 3;
		break;
	case 21:
		color = 2;
		sy = 1;
		sx = 0;
		break;
	case 22:
		color = 2;
		sy = 1;
		sx = 1;
		break;
	case 23:
		color = 2;
		sy = 1;
		sx = 2;
		break;
	case 24:
		color = 2;
		sy = 1;
		sx = 3;
		break;
	case 25:
		color = 2;
		sy = 2;
		sx = 0;
		break;
	case 26:
		color = 2;
		sy = 2;
		sx = 1;
		break;
	case 27:
		color = 2;
		sy = 2;
		sx = 2;
		break;
	case 28:
		color = 2;
		sy = 2;
		sx = 3;
		break;
	case 29:
		color = 2;
		sy = 3;
		sx = 0;
		break;
	case 30:
		color = 2;
		sy = 3;
		sx = 1;
		break;
	case 31:
		color = 2;
		sy = 3;
		sx = 2;
		break;
	case 32:
		color = 2;
		sy = 3;
		sx = 3;
		break;
	}

	switch(sx){
	case 0:
		sx = 40;
		break;

	case 1:
		sx = 240;
		break;
	case 2:
		sx = 440;
		break;
	case 3:
		sx = 640;
		break;
	default:
		break;
	}

	switch(sy){
	case 0:
		sy = 0;
		break;

	case 1:
		sy = 120;
		break;
	case 2:
		sy = 240;
		break;
	case 3:
		sy = 360;
		break;
	default:
		break;
	}
}

// list을 초기화 하는 함수
void init_list(){
	head = (struct _node *)calloc(1,sizeof(struct _node));
	tail = (struct _node *)calloc(1,sizeof(struct _node));
	head->next = tail;
	tail->next = tail;
} 

// list에 node을 추가하는 함수
// node에 저장된 score을 기준으로 오름차순으로 저장
struct _node *insert_node(char *file_name, char *file_score){
	struct _node *s, *p, *r;
	p = head;
	s = p->next;
	
	int score = atoi(file_score);

	// score의 점수가 작거나 다음 node가 tail이 아니면 이동
	while(s->score >= score && s!=tail){
		p = p->next;
		s = s->next;
	}

	// memory allocation
	r = (struct _node *)calloc(1, sizeof(struct _node));
	r->score = score;
	
	int k;
	for(k=0; k<strlen(file_name); k++){
		r->name[k] = file_name[k];
	}

	p->next = r;
	r->next = s;
	
	return r;
}

// list의 개수를 num개로 지정
void cut_node(int num){
	struct _node *s, *p;
	p = head;
	s = p->next;
	int flag = 0;

	int j;
	for(j=0; j<num; j++){
		if(s==tail){
			flag = 1;
			break;
		}
		
		p = p->next;
		s = s->next;
	} 

	if(flag == 0){
		p->next = tail;
	}
}

// score.txt에 저장된 점수와 이름을 사용하여 list을 생성
void make_list(){
	char *filename = "score.txt";

	FILE *fp;
	fp = fopen(filename, "r");

	char str[MAX_LEN];
	char name[MAX_LEN];
	char score[MAX_LEN];
	
	int j, k;

	init_list();

	// score.txt에 저장된 5명의 이름과 점수를 읽음
	for(j=0; j<5; j++){
		// str에 문자열 저장
		// '\n'까지 read
		if(fgets(str, MAX_LEN, fp) == NULL){
			break;
		}

		// str이 '\n'로 끝나기 때문에 '\n'을 search하여 '\0'로 변환
		for(k=0; k<MAX_LEN; k++){
			if(str[k] == '\n'){
				name[k] = '\0';
				break;
			}else{
				name[k] = str[k];
			}	
		}

		if(fgets(str, MAX_LEN, fp) == NULL){
			break;
		}
		for(k=0; k<MAX_LEN; k++){
			if(str[k] == '\n'){
				score[k] = '\0';
				break;
			}else{
				score[k] = str[k];
			}	
		}

		// list에 추가
		insert_node(name, score);
	}

	fclose(fp);
}

// list에 저장된 node의 이름과 점수를 출력
void print_node(){
	struct _node *p;
	p = head->next;

	while(p!=tail){
		printf("%d : %s\n", p->score, p->name);
		p = p->next;
	}
}

// 스크린에 알파벳을 출력하는 함수
// putText 함수를 구현한 것
void print_alphabet(char *str, int size, int sx, int sy, int step){
	int coor_x, coor_y;
	int screen_width;
	unsigned short *ptr;

	screen_width = fbvar.xres;

	unsigned short bmpdata[80*80];
	unsigned short reimage[size*size];
	char *pData, *data;
	int cols = 0, rows = 0;

	char *filename = NULL;

	// str의 알파벳마다 bmp 파일을 read
	int j;
	for(j=0; j<strlen(str); j++){
		if(str[j] == 'a') filename = "a.bmp";
		if(str[j] == 'b') filename = "b.bmp";
		if(str[j] == 'c') filename = "c.bmp";
		if(str[j] == 'd') filename = "d.bmp";
		if(str[j] == 'e') filename = "e.bmp";
		if(str[j] == 'f') filename = "f.bmp";
		if(str[j] == 'g') filename = "g.bmp";
		if(str[j] == 'h') filename = "h.bmp";
		if(str[j] == 'i') filename = "i.bmp";
		if(str[j] == 'j') filename = "j.bmp";
		if(str[j] == 'k') filename = "k.bmp";
		if(str[j] == 'l') filename = "l.bmp";
		if(str[j] == 'm') filename = "m.bmp";
		if(str[j] == 'n') filename = "n.bmp";
		if(str[j] == 'o') filename = "o.bmp";
		if(str[j] == 'p') filename = "p.bmp";
		if(str[j] == 'q') filename = "q.bmp";
		if(str[j] == 'r') filename = "r.bmp";
		if(str[j] == 's') filename = "s.bmp";
		if(str[j] == 't') filename = "t.bmp";
		if(str[j] == 'u') filename = "u.bmp";
		if(str[j] == 'v') filename = "v.bmp";
		if(str[j] == 'w') filename = "w.bmp";
		if(str[j] == 'x') filename = "x.bmp";
		if(str[j] == 'y') filename = "y.bmp";
		if(str[j] == 'z') filename = "z.bmp";
		if(str[j] == '0') filename = "0.bmp";
		if(str[j] == '1') filename = "1.bmp";
		if(str[j] == '2') filename = "2.bmp";
		if(str[j] == '3') filename = "3.bmp";
		if(str[j] == '4') filename = "4.bmp";
		if(str[j] == '5') filename = "5.bmp";
		if(str[j] == '6') filename = "6.bmp";
		if(str[j] == '7') filename = "7.bmp";
		if(str[j] == '8') filename = "8.bmp";
		if(str[j] == '9') filename = "9.bmp";
		if(str[j] == ' ') return;

		read_bmp(filename, &pData, &data, &cols, &rows);
		write_bmp(bmpdata, data, cols, rows);
		close_bmp(&pData);

		// image을 size x size 크기로 resize
		int x, y;
		for(coor_y=0; coor_y < size; coor_y++){
			for(coor_x = 0; coor_x < size; coor_x++){
				x = coor_x*80/size;
				y = coor_y*80/size;
				reimage[coor_y*size+coor_x] = bmpdata[y*cols+x];
			}
		}

		// 출력하는 범위가 스크린에서 벗어나면 함수 종료
		if(sy>=0 && sy+size < 480 && sx>=0 && sx+(j*step)+size < 800){

		}else{
			return;
		}

		// 스크린에 알파벳 또는 숫자를 출력
		for(coor_y=0; coor_y<size; coor_y++) {
			ptr = (unsigned short*)pfbmap + (screen_width*sy+(sx+j*step)) + (screen_width * coor_y);
			for (coor_x = 0; coor_x < size; coor_x++){
				if(reimage[coor_y*size+coor_x] != (unsigned short)RGB565(255, 255, 255))
					*ptr++ = reimage[coor_y*size+coor_x];
				else
					*ptr++;
			}
		}	
	}
}

// 변환된 랭킹을 기준으로 score.txt을 작성하는 함수
void make_file(){
	char *filename = "score.txt";

	FILE *fp;
	fp = fopen(filename, "w");
	char str[20];

	struct _node *p;
	p = head->next;

	while(p!=tail){
		fputs(p->name, fp);		// 파일에 이름을 저장
		fputs("\n", fp);		// read에서 '\n'을 단위로 읽기 때문에 추가
		sprintf(str, "%d", p->score);	// 정수를 문자열로 변환
		fputs(str, fp);			// 파일에 점수를 저장
		fputs("\n", fp);

		p = p->next;
	}

	fclose(fp);
}

// 랭킹에서 알파벳을 출력하는 함수
void print_alphabet_node(){
	struct _node *p;
	p = head->next;
	char score[MAX_LEN];
	char str[MAX_LEN];
	
	int j=0;
	int k=0;

	for(j=0; j<MAX_LEN; j++){
		score[j] = ' ';
	}

	// 글자의 크기와 간격
	int size = 25;
	int step = 25;

	j=0;
	while(p!=tail){
		sprintf(str, "%d", p->score);
	
		for(k=0; k<MAX_LEN; k++){
			score[j] = ' ';
		}

		for(k=0; k<strlen(str); k++){
			score[k] = str[k];
		}

		// 스크린에 점수를 출력
		if(j == 0) print_alphabet(score, size, 300, 130, step);
		if(j == 1) print_alphabet(score, size, 300, 175, step);
		if(j == 2) print_alphabet(score, size, 300, 225, step);
		if(j == 3) print_alphabet(score, size, 300, 270, step);
		if(j == 4) print_alphabet(score, size, 300, 315, step);

		// 스크린에 이름을 출력
		if(j == 0) print_alphabet(p->name, size, 400, 130, step);
		if(j == 1) print_alphabet(p->name, size, 400, 175, step);
		if(j == 2) print_alphabet(p->name, size, 400, 225, step);
		if(j == 3) print_alphabet(p->name, size, 400, 270, step);
		if(j == 4) print_alphabet(p->name, size, 400, 315, step);

		p = p->next;

		j++;
		if(j == 5){
			break;
		}
	}
}

int main(int argc, char **argv){
	int i,j,k,t;
	int fbfd, fd;
	int ret = 0;
	int optlen = strlen("--cascade=");
	unsigned short ch = 0;
	CvCapture *capture = 0;
	IplImage *image = NULL;

	// /dev/mem 파일을 열고 파일 기술자를 fd에 저장
	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0){
		perror("mem open fail\n");
		exit(1);
	}

	// /dev/fb0 파일을 열고 파일 기술자를 fbfd에 저장
	if ((fbfd = open(FBDEV_FILE, O_RDWR)) < 0){
		printf("Failed to open: %s\n", FBDEV_FILE);
		exit(-1);
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar) < 0){
		perror("fbdev ioctl");
		exit(1);
	}

	if (fbvar.bits_per_pixel != 16){
		fprintf(stderr, "bpp is not 16\n");
		exit(1);
	}

	// screen
	pfbmap = (unsigned char *)mmap(0, fbvar.xres * fbvar.yres * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

	if ((unsigned)pfbmap < 0){
		perror("mmap failed...");
		exit(1);
	}

	if (argc > 1 && strncmp(argv[1], "--cascade=", optlen) == 0){
		cascade_xml = argv[1] + optlen;
	}

	cascade = (CvHaarClassifierCascade *)cvLoad(cascade_xml, 0, 0, 0);

	// cascade을 열지 못했을 때 메시지 출력
	if (!cascade){
		fprintf(stderr, "ERROR: Could not load classifier cascade\n");
		fprintf(stderr, "Usage : ./facedetect -- cascade=[CASCADE_PATH/FILENAME]\n");
		return -1;
	}

	storage = cvCreateMemStorage(0);

	// camera device을 열고 파일 기술자를 dev에 저장
	int dev_camera = 0;
	dev_camera = open(CAMERA_DEVICE, O_RDWR);
	if (dev_camera < 0){
		printf("Error: cannot open %s.\n", CAMERA_DEVICE);
		exit(1);
	}

	// image의 크기를 320x240, color로 설정
	image = cvCreateImage(cvSize(320, 240), IPL_DEPTH_8U, 3);

	printf("1.3M CIS Camera FaceDetect Program -ver.20100721\n");
	
	//printf("init keyboard\n");
	init_keyboard();


	/////////////////////////////////////////////////////////////////////////
	// gpiobutton setting
	int Oflags;

    	signal(SIGIO, my_signal_fun);
	
	dev_gpio = open("/dev/gpiobutton",O_RDWR);
	if(dev_gpio < 0) { 
		printf( "GPIO Open ERROR!\n");
       		return -1;
	}

	fcntl(dev_gpio, F_SETOWN, getpid());  
      
        Oflags = fcntl(dev_gpio, F_GETFL);   

        fcntl(dev_gpio, F_SETFL, Oflags | FASYNC);

	//////////////////////////////////////////////////////////////
	int dev_dotmatrix = 0;
	dev_dotmatrix = open("/dev/dotmatrix", O_WRONLY);
	if(dev_dotmatrix < 0){
		printf( "Dotmatrix Open ERROR!\n");
       		return -1;
	}

	////////////////////////////////////////////////////////////////
	int dev_segment = 0;
	if((dev_segment=open("/dev/segment",O_RDWR|O_SYNC)) <= 0){
		printf("FND open fail\n");
		exit(1);
	}

	ioctl(dev_segment, 1, NULL, NULL);
	int time = 0;
	int ptime = 0;
	int m, s, ms;
	int temp = 0;

	///////////////////////////////////////////////////////////////////////////
	// BMP setting
	char *pData_ground,*data_ground;
	char *pData_blue,*data_blue;
	char *pData_red,*data_red;

	int cols=0, rows=0;
	int gcols = 0, grows = 0;
	int rcols = 0, rrows = 0;
	int bcols = 0, brows = 0;

	char r,g,b;
	unsigned short bmpdata_ground[800*480];
	unsigned short bmpdata_blue[800*480];
	unsigned short bmpdata_red[800*480];

	printf("read bmp\n");
	read_bmp("ground.bmp",&pData_ground,&data_ground, &gcols, &grows);
	write_bmp(bmpdata_ground, data_ground, gcols, grows);
	close_bmp(&pData_ground);

	read_bmp("default1.bmp",&pData_blue,&data_blue, &bcols, &brows);
	write_bmp(bmpdata_blue, data_blue, bcols, brows);
	close_bmp(&pData_blue);

	read_bmp("hit1.bmp",&pData_red,&data_red, &rcols, &rrows);
	write_bmp(bmpdata_red, data_red, rcols, rrows);
	close_bmp(&pData_red);

	// fps와 크게 관련없이 사용할 bmp 저장
	char *pData, *data;
	unsigned short bmpdata[800*480];

	///////////////////////////////////////////////////////////////
	// keypad
	int dev_event3;
	size_t read_bytes;
	struct fb_var_screeninfo fbvar;
	struct fb_fix_screeninfo fbfix;

	struct input_event event_buf[EVENT_BUF_NUM];
	if ((dev_event3 = open("/dev/input/event3", O_RDONLY)) < 0){
       		printf("application : keypad driver open fail!\n");
        	exit(1);
	}


	if((fbfd = open(FBDEV_FILE, O_RDWR))<0){
		printf("%s: open error\n", FBDEV_FILE);
		exit(1);
	}
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar) < 0){
		printf("%s: ioctl error - FBIOGET_VSCREENINFO \n",FBDEV_FILE);
		exit(1);
	}
	if(ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix)){
		printf("%s: ioctl error - FBIOGET_FSCREENINFO \n",
		FBDEV_FILE);
		exit(1);
	}
	if(fbvar.bits_per_pixel != 16){
		fprintf(stderr, "bpp is not 16\n");
		exit(1);
	}

	int tsfd;
	int screen_width;
	int screen_height;
	int bits_per_pixel;
	int line_length;
	int coor_x, coor_y;
	int mem_size;

	screen_width = fbvar.xres; 		// 스크린의 픽셀 폭
	screen_height = fbvar.yres; 		// 스크린의 픽셀 높이
	bits_per_pixel = fbvar.bits_per_pixel; 	// 픽셀 당 비트 개수
	line_length = fbfix.line_length; 	// 한개 라인 당 바이트 개수
	mem_size = line_length * screen_height;
	//pfbmap_bmp = (unsigned short *)mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	unsigned short *ptr;

	// save image
	unsigned short player1_rgb[SIZE*SIZE*2];
	unsigned short player2_rgb[SIZE*SIZE*2];

	for(j=0; j<SIZE*SIZE*2; j++){
		player1_rgb[j] = 255;
		player2_rgb[j] = 255;
	}

	// mnist
	unsigned short mnist_rgb[300*300*2];
	fill_rgb(mnist_rgb, 255, 255, 255);

	int mnist_val = 0;
	FILE *input, *output;
	float mnist_input[28 * 28] = {0};
	IplImage *mnist_ipl = cvCreateImage(cvSize(300, 300), 8, 3);
	IplImage *small_mnist = NULL;


	int phase = 0;
	int key = 0;	// keypad value
	int score = 0;	
	int delay = 0;	// time delay
	int mode = 0;	
	int difficulty = 10;
	
	int play_time = 100;
	time = 100;

	int player = 0;
	char *player1_name;
	char *player2_name;
	int player1_score = 0;
	int player2_score = 0;
	int index = 0;

	int qwerty_flag = 0;
	char input_key = ' ';
	char input_key_copy = ' ';

	// 반복문을 실행하기 전 
	// 화면을 검정색으로 클리어 
	Fill_Background(0x0000);

	// 파일을 읽어 리스트에 저장
	init_list();	// 리스트 초기화
	make_list();	// 파일을 읽어 리스트에 저장
	struct _node *p1, *p2;
	
	// memory allocation
	p1 = (struct _node *)calloc(1, sizeof(struct _node));
	p2 = (struct _node *)calloc(1, sizeof(struct _node));
	
	// touch event
	int tx = 0, ty = 0;
	int tx_copy = 0, ty_copy = 0;

	// 키보드 이미지에서 입력받아 저장
	char buf[16];
	for(coor_y = 0; coor_y<16; coor_y++)
		buf[coor_y] = ' ';
	buf[15] = '\0';

	// alphabet
	// print_alphabet("abcdefg", 80,  120, 40, 80);
	
	//printf("print_node\n");
	print_node();
	char p1score[MAX_LEN];
	char p2score[MAX_LEN];

	for(j=0; j<MAX_LEN; j++){
		p1score[j] = ' ';
		p2score[j] = ' ';
	}
	char strscore[MAX_LEN];

	phase = 0;
	delay = 0;

	while (ch != 'q'){
		// 키보드 입력이 발생하면 문자를 읽음.
		if (kbhit()){
			ch = readch();
		}

		// cover image 출력
		if(phase == 0){
			read_bmp("cover.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			// print image
			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
				for (coor_x = 0; coor_x < cols; coor_x++)
					*ptr++ = bmpdata[coor_x + coor_y*cols];
			}

			if(button == 1){
				button = 0;
				phase = 1;
				Fill_Background(0x0000);
			}

		}

		// face detect
		// gpiobutton을 누르면 face detect을 수행
		if(phase == 1){
			// 카메라에서 영상을 읽어 cis_rgb에 저장
			write(dev_camera, NULL, 1);
			read(dev_camera, cis_rgb, 320 * 240 * 2);

			// 화면에 cis_rgb을 출력(왼쪽 실시간 영상)
			fb_display(cis_rgb, 40, 120, 320, 240);

			// 얼굴 인식
			if (button == 1){
				button = 0;

				// rgb를 iplimage로 변환
				RGB2cvIMG(cis_rgb, image, 320, 240);

				if (image){
					ret = detect_and_draw(image, player1_rgb, player2_rgb);
					if (ret > 0){
						printf("save\n");
						phase = 2;
					}
					else{
						printf("Don't facedetect\n");
					}
				}
								// 1회만 실시
				
				Fill_Background(0x0000);
			}
		}

		// face detect 결과로 얻은 2명의 사진을 출력
		// 화면을 터치하면 키보드 구간으로 이동
		if(phase == 2){
			// player image
			fb_display(player1_rgb, 120, 120, SIZE, SIZE);
			fb_display(player2_rgb, 480, 120, SIZE, SIZE);
			
			// 터치 이벤트 발생
			if(GetTouch() != -1) {
				if(x_detected!=-1 && y_detected!=-1) {

					// player1의 사진을 터치
					if(x_detected > 120 && x_detected < 360 && y_detected > 120 && y_detected < 360){
						Fill_Background(0x0000);	
						player = 1;
						phase = 3;
					}

					// player2의 사진을 터치
					if(x_detected > 480 && x_detected < 720 && y_detected > 120 && y_detected < 360){
						Fill_Background(0x0000);	
						player = 2;
						phase = 3;
					}
				}
			}

			// 플레이어 1, 2의 save가 모두 1이 되면 player1 start 버튼으로 이동
			if(p1->save == 1 && p2->save == 1){
				phase = 8;
			}		
		}

		// keyboard image 출력
		// gpiobutton을 누르면 phase 4로 이동
		if(phase == 3){
			read_bmp("qwerty.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
				
			}

			// phase2에서의 터치 이벤트가 phase4에 영향을 주지 않도록 하기 위한 조치
			if(button == 1){
				index = 0;
				tx_copy = -1;
				ty_copy = -1;
				button = 0;
				phase = 4;
			}
		}

		// touch가 발생하면 해당하는 위치의 알파벳을 buf에 저장하면서 화면에 출력
		if(phase == 4){
			if(GetTouch() != -1) {
				if(x_detected!=-1 && y_detected!=-1) {
					tx = x_detected;
					ty = y_detected;

					int key_width = 55;
					int key_height = 80;
	
					// 한 번만 클릭할 때 사용하기 draw에서는 기존의 방식 채택 
					// touch event가 한 번 누를 때 다수의 입력을 받기 때문에
					// 한 곳을 누르고 있을 때 한 번만 인식되도록 함
					if(tx > tx_copy - 10 && tx < tx_copy + 10 && ty > ty_copy - 10 && ty < ty_copy + 10){

					}else{
						// q
						if(tx > 76 && tx < 76+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'q';
						}
		
						// w		
						else if(tx > 142 && tx < 142+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'w';
						}

						// e
						else if(tx > 207 && tx < 207+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'e';
						}

						// r
						else if(tx > 274 && tx < 274+key_width && ty > 154 && ty < 154+key_height){
								input_key = 'r';
						}

						// t
						else if(tx > 339 && tx < 339+key_width && ty > 154 && ty < 154+key_height){
							input_key = 't';
						}

						// y
						else if(tx > 405 && tx < 405+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'y';
						}

						// u
						else if(tx > 470 && tx < 470+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'u';
						}

						// i
						else if(tx > 536 && tx < 536+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'i';
						}

						// o
						else if(tx > 601 && tx < 601+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'o';
						}

						// p
						else if(tx > 668 && tx < 668+key_width && ty > 154 && ty < 154+key_height){
							input_key = 'p';
						}

						// a
						else if(tx > 109 && tx < 109+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'a';
						}

						// s
						else if(tx > 176 && tx < 176+key_width && ty > 258 && ty < 258+key_height){
							input_key = 's';
						}

						// d
						else if(tx > 241 && tx < 241+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'd';
						}

						// f
						else if(tx > 306 && tx < 306+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'f';
						}

						// g
						else if(tx > 372 && tx < 372+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'g';
						}

						// h
						else if(tx > 437 && tx < 437+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'h';
						}

						// j
						else if(tx > 502 && tx < 502+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'j';
						}

						// k
						else if(tx > 570 && tx < 570+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'k';
						}

						// l
						else if(tx > 634 && tx < 634+key_width && ty > 258 && ty < 258+key_height){
							input_key = 'l';
						}

						// z
						else if(tx > 176 && tx < 176+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'z';
						}

						// x
						else if(tx > 241 && tx < 241+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'x';
						}

						// c
						else if(tx > 306 && tx < 306+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'c';
						}

						// v
						else if(tx > 372 && tx < 372+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'v';
						}

						// b
						else if(tx > 437 && tx < 437+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'b';
						}

						// n
						else if(tx > 502 && tx < 502+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'n';
						}

						// m
						else if(tx > 570 && tx < 570+key_width && ty > 360 && ty < 360+key_height){
							input_key = 'm';
						}

						// enter
						// 이름을 저장한 후 buf을 초기화하고 mnist로 이동
						else if(tx > 646 && tx < 723 && ty > 360 && ty < 360+key_height){
							if(player == 1){
								p1->save = 1;
								memcpy(p1->name, buf, 16);
								//printf("player1 : %s\n", p1->name);
							}

							if(player == 2){
								p2->save = 1;
								memcpy(p2->name, buf, 16);
								//printf("player2 : %s\n", p2->name);
							}

							for(j=0; j<15; j++){
								buf[j] = ' ';
							}	
							buf[15] = '\0';
							
							index = 0;
							phase = 5;
						}

						else{
							input_key = ' ';
						}

						if(input_key != ' '){
							// bmpdata에 저장되어 있는 키보드 이미지를 출력
							for(coor_y=0; coor_y<480; coor_y++) {
								ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
								for (coor_x = 0; coor_x < 800; coor_x++)
									*ptr++ = bmpdata[coor_y*800+coor_x];
							}

							buf[index] = input_key;

							index++;
							if(index >= 7){
								index = 0;
							}

							// 스크린에 알파벳 출력
							print_alphabet(buf, 80,  120, 40, 80);
						}
					}		

					tx_copy = tx;
					ty_copy = ty;
				}
			}
		}

		// mnist
		if(phase == 5){
			// 스크린을 검정색으로 초기화
			Fill_Background(0x0000);

			// mnist_rgb을 흰색으로 초기화
			fill_rgb(mnist_rgb, 255, 255, 255);
			// 가운데 정사각형 영역을 흰색으로 채움
			for(coor_y = 0+90; coor_y < 300+90; coor_y++){
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
		
				for(coor_x = 0; coor_x < 250; coor_x++){
					*ptr++;
				}
				
				for(coor_x = 0+250; coor_x < 300+250; coor_x++){
					*ptr++ = (unsigned short)RGB565(255, 255, 255);
				}
			}
			phase = 99;	
		}

		if(phase == 99){
			if(GetTouch() != -1) {
				if(x_detected!=-1 && y_detected!=-1) {
					if(x_detected > 250 && x_detected < 550 && y_detected > 90 && y_detected < 390){
						// 터치한 위치의 스크린에 출력
						draw_circle(x_detected, y_detected);

						// 터치한 위치에 해당하는 영역에 검정색을 채움
						draw_rgb(mnist_rgb, x_detected, y_detected);
					}else{
						phase = 5;
					}	
				}
			}

			if(button == 1){
				Fill_Background(0x0000);
				button = 0;
				phase = 6;
			}
		}
	
		// Loading image 출력
		if(phase == 6){
			read_bmp("loading.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			// print image
			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
				for (coor_x = 0; coor_x < cols; coor_x++)
					*ptr++ = bmpdata[coor_x + coor_y*cols];
			}
			phase = 7;
		}

		if(phase == 7){
			RGB2cvIMG(mnist_rgb, mnist_ipl, 300, 300);
			small_mnist = Color_to_Grayscale_Resize(mnist_ipl);
			Normalization(small_mnist, &mnist_input);
			write_input_file(&mnist_input);

			predict_probability = atoi("mnist_input.txt");

			input = fopen("mnist_input.txt", "r");
			if (input == NULL)
			{
				fprintf(stderr, "can't open input file %s\n", "mnist_input.txt");
 			 	exit(1);
   			}

    			output = fopen("mnist_output.file", "w");
    			if (output == NULL)
    			{
        			fprintf(stderr, "can't open output file %s\n", "mnist_output.file");
        			exit(1);
    			}
    			if ((model = svm_load_model("mnist.model")) == 0)
    			{
        			fprintf(stderr, "can't open model file %s\n", "mnist.model");
        			exit(1);
   			}

  			x = (struct svm_node *)malloc(max_nr_attr * sizeof(struct svm_node));
   			if (predict_probability)
    			{
        			if (svm_check_probability_model(model) == 0)
        			{
            				fprintf(stderr, "Model does not support probabiliy estimates\n");
            				exit(1);
        			}
     			}
    			else
   			{
        			if (svm_check_probability_model(model) != 0)
           			 	info("Model supports probability estimates, but disabled in prediction.\n");
    			}

    			mnist_val = predict(input, output);
   			printf("mnist val : %d\n", mnist_val);
   			svm_free_and_destroy_model(&model);
 					
    			free(x);
    			free(line);
    			fclose(input);
    			fclose(output);

			Fill_Background(0x0000);

			// mnist가 끝나면 값을 저장한 후 이동
			if(mnist_val>=1 && mnist_val <= 3){
				if(player == 1){
					p1->hard = mnist_val;
				}
				if(player == 2){
					p2->hard = mnist_val;
				}
				phase = 2;
			}else{
				// 지정된 난이도와 다르게 인식하면 다시 mnist 수행
				phase = 5;
			}
		}	

		// player1 start 버튼 출력
		if(phase == 8){
			read_bmp("player1_start.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
			}

			// 터치 이벤트 발생
			if(GetTouch() != -1) {
				if(x_detected!=-1 && y_detected!=-1) {
					//printf("y : %d\n", y_detected);
					//printf("x : %d\n\n", x_detected);

					// player1의 사진을 터치
					if(x_detected > 265 && x_detected < 520 && y_detected > 310 && y_detected < 420){
						player = 1;
						phase = 10;
					}
				}
			}
		}

		// player2 start 버튼 출력
		if(phase == 9){
			read_bmp("player2_start.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
			}

			// 터치 이벤트 발생
			if(GetTouch() != -1) {
				if(x_detected!=-1 && y_detected!=-1) {
					//printf("y : %d\n", y_detected);
					//printf("x : %d\n\n", x_detected);

					// player1의 사진을 터치
					if(x_detected > 265 && x_detected < 520 && y_detected > 310 && y_detected < 420){
						player = 2;
						phase = 10;
					}
				}
			}
		}

		// 3, 2, 1, start을 화면에 출력
		if(phase == 10){
			read_bmp("count3.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
			}

			sleep(1);

			read_bmp("count2.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
			}

			sleep(1);

			read_bmp("count1.bmp",&pData,&data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
			}

			sleep(1);
			
			phase = 11;
		}

		// 게임 설정
		if(phase == 11){
			if(player == 1){
				if(p1->hard == 1){
					difficulty = 14;
				}
				if(p1->hard == 2){
					difficulty = 12;
				}
				if(p1->hard == 3){
					difficulty = 10;
				}
			}

			if(player == 2){
				if(p2->hard == 1){
					difficulty = 14;
				}
				if(p2->hard == 2){
					difficulty = 12;
				}
				if(p2->hard == 3){
					difficulty = 10;
				}
			}

			mode = 0;
			score = 0;
			time = 100;
			delay = 0;

			phase = 12;
		}

		// 게임 실행
		if(phase == 12){
			// keypad 입력
			read_bytes = read(dev_event3, event_buf, (sizeof(struct input_event) * EVENT_BUF_NUM));

        		for (i = 0; i < (read_bytes / sizeof(struct input_event)); i++){
            			if ((event_buf[i].type == EV_KEY) && (event_buf[i].value == 0)){
					key = event_buf[i].code;
				}
            		}
			
			// key의 입력에 따른 mode의 변화
			// 기본 화면일 때 3frame 출력 후 랜덤하게 다음 이미지로 이동
			if(mode == 0){
				delay++;
				if(delay == 3){
					delay = 0;
					mode = (rand()%16)+1;
				}
			}

			// 1~16 사이일 때 mode와 key가 같으면 정답 칸으로 이동
			// N프레임 이내에 누르지 못하면 MODE 0로 이동
			if(mode >= 1 && mode <=16){
				if(mode == key){
					mode = mode + 16;
					delay = 0;

					if(difficulty == 14){	
						score++;
					}
					if(difficulty == 12){
						score+=2;
					}
					if(difficulty == 10){
						score+=3;
					}
					if(difficulty == 8){
						score+=4;
					}
					if(difficulty == 6){
						score+=5;
					}
				}

				delay++;
				if(delay == difficulty){
					delay = 0;
					mode = 0;
				}
			}
	
			// 정답 화면에서 3프레임 이후 모드 0으로 이동
			if(mode > 16){
				delay++;
				if(delay == 3){
					delay = 0;
					mode = 0;
				}
			}

			// color, sy, sx setting
			color_position(mode);

			if(mode != mode_copy){
				// print bmp
				for(coor_y=0; coor_y<480; coor_y++) {
					ptr = (unsigned short*)pfbmap + (screen_width*coor_y);
					for ( coor_x = 0; coor_x < 800; coor_x++)
						*ptr++ = bmpdata_ground[coor_x + coor_y*800];
				}	
	
				if(color == 1){
					for(coor_y=10; coor_y<brows-10; coor_y++) {
						ptr = (unsigned short*)pfbmap + (screen_width * sy + sx) + (screen_width * 	coor_y);
						for (coor_x = 0; coor_x < 10; coor_x++)
							*ptr++;
	
						for (coor_x = 10; coor_x < bcols-10; coor_x++){
							if(bmpdata_blue[coor_x + coor_y*bcols] == RGB565(255, 255, 255)){
								*ptr++;
							}else{
								*ptr++ = bmpdata_blue[coor_x + coor_y*bcols];
							}
						}
					}
				}
	
				if(color == 2){
					for(coor_y=10; coor_y<rrows-10; coor_y++) {
						ptr = (unsigned short*)pfbmap + (screen_width * sy + sx) + (screen_width * 	coor_y);
						for (coor_x = 0; coor_x < 10; coor_x++)
							*ptr++;
	
						for ( coor_x = 10; coor_x < rcols-10; coor_x++){
							if(bmpdata_red[coor_x + coor_y*rcols] == RGB565(255, 255, 255)){
								*ptr++;
							}else{
								*ptr++ = bmpdata_red[coor_x + coor_y*rcols];
							}
						}
					}
				}
			}

			// print score in dot matrix 
			write(dev_dotmatrix, &score, 4);

			// 시간이 지날수록 이동하는 속도가 점점 빨라짐
			time--;
			if(time == play_time/3 * 2){
				difficulty-=2;
			}
			if(time == play_time/3){
				difficulty-=2;
			}

			// 시간이 0이 되면 다음 페이지로 이동
			if(time == 0){
				phase = 13;

			}

			// print time in segment은
			for(j = 0; j<10; j++){
				write(dev_segment, &time, 4);
			}

			mode_copy = mode;
		}

		// 플레이어의 이름과 점수를 node에 추가
		if(phase == 13){
			char sc[10];

			if(player == 1){
				p1->score = score;
				sprintf(sc, "%d", p1->score);
				insert_node(p1->name, sc);
				phase = 9;
			}
			if(player == 2){
				p2->score = score;
				sprintf(sc, "%d", p2->score);
				insert_node(p2->name, sc);
				phase = 14;
			}
		}

		//  list의 개수를 5개로 줄이고 점수에 따라 다른 phase로 이동
		if(phase == 14){
			cut_node(5);

			Fill_Background(0x0000);
			if(p1->score > p2->score){
				phase = 15;
			}
			if(p1->score == p2->score){
				phase = 16;
			}
			if(p1->score < p2->score){
				phase = 17;
			}

			print_node();
		}

		// player1이 이기면 player1의 사진과 이름을 스크린에 출력
		if(phase == 15){
			read_bmp("winner.bmp", &pData, &data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
				
			}

			fb_display(player1_rgb, 300, 120, SIZE, SIZE);
			print_alphabet(p1->name, 80, 300, 360, 80);
			//printf("p1->name : %s\n", p1->name);
			print_alphabet("winner", 80, 160, 40, 80);

			phase = 18;
		}

		// 점수가 같으면 player1, 2의 사진과 이름을 스크린에 출력
		if(phase == 16){
			read_bmp("winner.bmp", &pData, &data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
				
			}

			fb_display(player1_rgb, 120, 120, SIZE, SIZE);
			fb_display(player2_rgb, 480, 120, SIZE, SIZE);
			print_alphabet(p1->name, 80, 120, 360, 80);
			print_alphabet(p2->name, 80, 480, 360, 80);
			print_alphabet("draw", 80, 250, 40, 80);

			phase = 18;
		}

		// player2의 점수가 더 높으면 player2의 사진과 이름을 스크린에 출력
		if(phase == 17){
			read_bmp("winner.bmp", &pData, &data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
				
			}

			fb_display(player2_rgb, 300, 120, SIZE, SIZE);
			print_alphabet("winner", 80, 160, 40, 80);
			print_alphabet(p2->name, 80, 300, 360, 80);

			phase = 18;
		}

		// 스크린에 winner의 사진을 출력한 후 gpio button을 누르면 다음 페이지로 이동
		if(phase == 18){
			if(button == 1){
				button = 0;	
				phase = 19;
			}
		}

		// 스크린에 랭킹을 출력
		if(phase == 19){
			// 배경화면을 읽고 스크린에 출력
			read_bmp("rank.bmp", &pData, &data, &cols, &rows);
			write_bmp(bmpdata, data, cols, rows);
			close_bmp(&pData);

			for(coor_y=0; coor_y<rows; coor_y++) {
				ptr = (unsigned short*)pfbmap + (screen_width * coor_y);
					for (coor_x = 0; coor_x < cols; coor_x++)
						*ptr++ = bmpdata[coor_x + coor_y*cols];
				
			}

			// list에서 이름과 점수를 읽어 스크린에 알파벳과 숫자를 출력
			print_alphabet_node();

			// 현재 저장된 list을 기반으로 score.txt을 작성
			make_file();

			phase = 20;
		}

		// touch event에 따라 restart, end을 결정
		if(phase == 20){
			// 터치 이벤트 발생
			if(GetTouch() != -1) {
				if(x_detected!=-1 && y_detected!=-1) {
					// restart
					if(x_detected > 180 && x_detected < 625 && y_detected > 360 && y_detected < 393){
						phase = 0;
					}

					// end
					if(x_detected > 180 && x_detected < 625 && y_detected > 405 && y_detected < 435){
						break;
					}
				}
			}
		}
	}

	Fill_Background(0x0000);

	cvReleaseImage(&image);
	close_keyboard();
	return 0;
}
