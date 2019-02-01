# Makefile for ttft


ttft: main.c
	cc -o ttft -lSDL2 -pthread main.c

clock: examples/ttft-clock.c
	cc -o examples/clock -lm examples/ttft-clock.c
