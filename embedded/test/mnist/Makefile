
CC = /home/jinkyun/work/week6/arm-2009q3/bin/arm-linux-gcc-4.4.1
CXX = /home/jinkyun/work/week6/arm-2009q3/bin/arm-linux-g++

APP = mnist

CFLAGS = -DNO_DEBUG -I/home/jinkyun/OpenCV_1_1_0_arm/include/opencv/ -L/home/jinkyun/OpenCV_1_1_0_arm/lib/ -lcxcore -lcv -lhighgui -lcvaux -lml -Wall -Wconversion -O3 -fPIC

OBJS = $(APP).o svm.o

all: $(APP)

svm.o: svm.cpp svm.h
	$(CXX) -Wall -Wconversion -static -O3 -fPIC -c svm.cpp

$(APP): mnist.o svm.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OBJS) $(APP)


