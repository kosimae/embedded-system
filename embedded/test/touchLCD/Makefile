#DIPSW Device Driver Makfile
CC = /home/jinkyun/work/week6/arm-2009q3/bin/arm-linux-gcc-4.4.1
obj-m := camera.o
KDIR := /home/jinkyun/work/week6/linux-2.6.32-hanback
PWD := $(shell pwd)

TEST_TARGET = touchLCD
TEST_SRCS = $(TEST_TARGET).c

default:$(TEST_TARGET)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	$(CC) $(TEST_SRCS) -o $(TEST_TARGET)
	rm -f default
clean:
	rm -f *.ko *.o *.mod.* .*.cmd
	rm -rf $(TEST_TARGET) Module.* module* .tmp_versions
