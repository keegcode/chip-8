CC = gcc
CFLAGS = -W -Wall -Wextra -pedantic -g
LFLAGS = -lSDL2

chip8:main.c 
	gcc $(CFLAGS) -o chip8 main.c $(LFLAGS)
