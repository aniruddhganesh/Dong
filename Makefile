CFLAGS=-Wall -std=c99 -pedantic -I/usr/include/SDL2 -D_REENTRANT -lSDL2

pong: pong.c
	clang $(CFLAGS) -o pong pong.c
