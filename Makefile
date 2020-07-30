.PHONY : clean

all:
	make -C key-handler/src
	make -C key-handler-api/src
clean:
	make -C key-handler-api/src clean
	make -C key-handler/src clean
