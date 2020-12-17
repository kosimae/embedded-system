
CC = /home/jinkyun/work/week6/arm-2009q3/bin/arm-linux-gcc-4.4.1

APP = fb_bmp

CFLAGS = -DNO_DEBUG -I/home/jinkyun/OpenCV_1_1_0_arm/include/opencv/ -L/home/jinkyun/OpenCV_1_1_0_arm/lib/ -lcxcore -lcv -lhighgui -lcvaux -lml

OBJS = $(APP).o

all: $(APP)

$(APP): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(APP)


