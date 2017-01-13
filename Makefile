CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra

default: main

main: main.o fat16.o fat16_fuse.o

fat16.o: fat16.c fat16.h

fat16_fuse.o: fat16_fuse.c fat16_fuse.h
	gcc -Wall fuse.c $(shell pkg-config fuse3 --cflags --libs) -o fuse

clean:
	$(RM) main *.o

.PHONY: clean

# Debug section.

device: 
	dd if=/dev/zero of=fs_image.raw count=10240 bs=1024
	mkfs.vfat -F 16 -n SO2017 -v fs_image.raw

mount:
	mkdir -p fs_root
	mount -t msdos -o loop,rw,showexec,umask=0 fs_image.raw fs_root

unmount:
	umount fs_root

docker:
	docker start -ia SO-P2

docker-create:
	docker run -it --name SO-P2 --privileged=true -v $(shell pwd):/root -w /root ubuntu

cleanall: clean
	$(RM) -r fs_root fs_image.raw

.PHONY: device mount unmount docker docker-create cleanall
