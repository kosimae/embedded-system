#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> 	/* for O_RDONLY */
#include <sys/mman.h> 	/* for mmap */
#include <linux/fb.h> 	/* for fb_var_screeninfo, FBIOGET_VSCREENINFO*/
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "bmp_header.h"		// bmp structure 
#define FBDEV_FILE "/dev/fb0"	// screen device

// 입력이 정확하지 않을 때 출력 
void usage(){
	printf("\n====================================================\n");
	printf("\nUsage: ./fb_bmp [FILE.bmp]\n");
	printf("\n====================================================\n");
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

int main(int argc, char *argv[]){
	int i,j,k,t;
	int fbfd,tsfd;
	int screen_width;
	int screen_height;
	int bits_per_pixel;
	int line_length;
	int coor_x, coor_y;
	int cols=0, rows=0;
	int mem_size;
	char *pData,*data;
	char r,g,b;
	unsigned short bmpdata[800*480];
	unsigned short pixel;
	unsigned short *pfbmap;
	unsigned short *ptr;
	struct fb_var_screeninfo fbvar;
	struct fb_fix_screeninfo fbfix;
	printf("Frame buffer Application - Bitmap\n");
	
	// ./fb_bmp (filename).bmp의 형태로 프로그램 실행
	// argc가 2가 아니면 메시지 출력 후 프로그램 종료
	if(argc != 2){
		usage();
		return 0;
	}

	// (filename).bmp을 읽어 read_bmp 함수 실행
	read_bmp(argv[1],&pData,&data, &cols, &rows);
	printf("Bitmap : cols = %d, rows = %d\n", cols, rows);

	// data에 저장되어 있는 bmp data을 unsigned short 형태로 변환
	for(j=0;j<rows;j++){
		k = j*cols*3;
		t = (rows -1 - j)*cols;
		for(i=0;i<cols;i++){
			b = *(data + (k + i*3));
			g = *(data + (k + i*3+1));
			r = *(data + (k + i*3+2));

			// RGB 형태로 변환
			pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
			bmpdata[t+i] = pixel;
		}
	}

	close_bmp(&pData);

	// screen device open
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

	screen_width = fbvar.xres; 		// 스크린의 픽셀 폭
	screen_height = fbvar.yres; 		// 스크린의 픽셀 높이
	bits_per_pixel = fbvar.bits_per_pixel; 	// 픽셀 당 비트 개수
	line_length = fbfix.line_length; 	// 한개 라인 당 바이트 개수
	mem_size = line_length * screen_height;
	pfbmap = (unsigned short *)
	mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);

	if((unsigned)pfbmap == (unsigned)-1){
		perror("fbdev mmap");
		exit(1);
	}

	// 배경을 black으로 clear
	for(coor_y=0; coor_y<480; coor_y++) {
		ptr = (unsigned short*)pfbmap + (screen_width*coor_y);
		for ( coor_x = 0; coor_x < 800; coor_x++)
			*ptr++ = 0x0000; // black
	}

	// bmpdata을 스크린에 출력
	for(coor_y=0; coor_y<rows; coor_y++) {
		ptr = (unsigned short*)pfbmap + (screen_width*coor_y);
		for ( coor_x = 0; coor_x < cols; coor_x++)
			*ptr++ = bmpdata[coor_x + coor_y*cols];
	}

	munmap(pfbmap, mem_size);
	close(fbfd);
	return 0;
}
