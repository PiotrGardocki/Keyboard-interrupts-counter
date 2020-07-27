obj-m += key-handler.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	insmod key-handler.ko
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rmmod key-handler.ko
