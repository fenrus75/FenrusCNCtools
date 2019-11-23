all: stl2c2d

stl2c2d: Makefile stl.c stl2c2d.c fenrus.h
	gcc $(CFLAGS) -O3 -Wall -W stl.c stl2c2d.c -o stl2c2d
	
