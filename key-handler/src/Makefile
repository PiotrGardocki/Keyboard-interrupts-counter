obj-m += key-handler.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

.PHONY : clean

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules
	-insmod key-handler.ko
clean:
	-rmmod key-handler.ko
	-rm -f /dev/key-handler
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
