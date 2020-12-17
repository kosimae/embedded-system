#
# Makefile for Dotmatrix Device Driver
# Hanback Electronics Co.,ltd
# Date : September 30,2009
#

CC     = /home/jinkyun/work/week6/arm-2009q3/bin/arm-linux-gcc-4.4.1

obj-m := dotmatrix.o

KDIR := /home/jinkyun/work/week6/linux-2.6.32-hanback

PWD := $(shell pwd)

TEST_TARGET = dot_test
TEST_SRCS = $(TEST_TARGET).c

default:$(TEST_TARGET)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	$(CC) $(TEST_SRCS) -static -o $(TEST_TARGET)
	rm -f default

clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f .*.cmd
	rm -f $(TEST_TARGET)
