CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=gnu99 -g -O2 -Wall -Wextra $(shell pkg-config --cflags fuse3) -Wno-unused-parameter -Wno-unused-result
LDLIBS = $(shell pkg-config --libs fuse3)

fat16: fat16.o fat16_fuse.o

fat16.o: fat16.c fat16.h

fat16_fuse.o: fat16_fuse.c fat16_fuse.h

clean:
	$(RM) *.o *~ fat16

.PHONY: clean
