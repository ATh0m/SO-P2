CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra

default: main

main: main.c fat16.o
	$(CC) $(CFLAGS) -o main main.c fat16.o

fat16.o: fat16.c fat16.h
	$(CC) $(CFLAGS) -c fat16.c

clean:
	-rm main *.o

.PHONY: clean

# Debug section.

device: 
	dd if=/dev/zero of=fs_image.raw count=10240 bs=1024
	mkfs.fat -F 16 -n SO2015 -v fs_image.raw

mount:
	hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount fs_image.raw
	mkdir -p fs_root
	mount -t msdos /dev/disk3 fs_root/

unmount:
	umount fs_root
	diskutil eject /dev/disk3

cleanall: clean
	-rm -rf fs_root fs_image.raw

.PHONY: device mount unmount cleanall
