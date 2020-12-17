typedef struct {
	// unsigned short bfType; // BM 이라 써있으면 BMP
	unsigned int bfSize; 		// 이미지 크기
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int bfOffBits; 		// 이미지 데이터가 있는 곳의 포인터
}BITMAPFILEHEADER;

typedef struct {
	unsigned int biSize; 		//현 구조체의 크기
	unsigned int biWidth; 		//이미지의 가로 크기
	unsigned int biHeight; 		//이미지의 세로 크기
	unsigned short biPlanes; 	//플레인수
	unsigned short biBitCount; 	//비트 수
	unsigned int biCompression; 	//압축 유무
	unsigned int biSizeImage; 	//이미지 크기
	unsigned int biXPelsPerMeter; 	//미터당 가로 픽셀
	unsigned int biYPelsPerMeter; 	//미터당 세로 픽셀
	unsigned int biClrUsed; 	//컬러 사용 유무
	unsigned int biClrImportant; 	//중요하게 사용하는 색
}BITMAPINFOHEADER;

typedef struct {
	unsigned char rgbBlue;
	unsigned char rgbGreen;
	unsigned char rgbRed;
	unsigned char rgbReserved;
}RGBQUAD;

typedef struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
}BITMAPINFO;
