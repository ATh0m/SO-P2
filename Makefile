CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=gnu99 -g -O2 -Wall -Wextra $(shell pkg-config --cflags fuse3)
LDLIBS = $(shell pkg-config --libs fuse3)

fat16: fat16.o fat16_fuse.o

fat16.o: fat16.c fat16.h

fat16_fuse.o: fat16_fuse.c fat16_fuse.h

clean:
	$(RM) *.o *~ fat16

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

.PHONY: device mount unmount
