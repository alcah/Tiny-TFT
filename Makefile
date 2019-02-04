# Makefile for ttft
.POSIX:
CC = cc
CFLAGS = -W

all: ttft clock
ttft: main.c
	$(CC) -o ttft $(CFLAGS) -pthread -lSDL2 main.c

clock: examples/ttft-clock.c
	$(CC) -o examples/clock $(CFLAGS) -lm examples/ttft-clock.c

clean:
	rm -f ttft examples/clock
