
CC = /home/jinkyun/embedded/compile/arm-2009q3/bin/arm-linux-gcc-4.4.1
CXX = /home/jinkyun/embedded/compile/arm-2009q3/bin/arm-linux-g++

APP = term_test

CFLAGS = -DNO_DEBUG -I/home/jinkyun/embedded/compile/OpenCV_1_1_0_arm/include/opencv/ -L/home/jinkyun/embedded/compile/OpenCV_1_1_0_arm/lib/ -lcxcore -lcv -lhighgui -lcvaux -lml -Wall -Wconversion -O3 -fPIC

OBJS = $(APP).o svm.o

all: $(APP)

svm.o: svm.cpp svm.h
	$(CXX) -Wall -Wconversion -static -O3 -fPIC -c svm.cpp

$(APP): $(APP).o svm.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(APP)


