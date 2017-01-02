CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra

default: main

main: main.c fat16.o
	$(CC) $(CFLAGS) -o main main.c fat16.o

fat16.o: fat16.c fat16.h
	$(CC) $(CFLAGS) -c fat16.c

clean:
	rm main *.o

.PHONY: clean
