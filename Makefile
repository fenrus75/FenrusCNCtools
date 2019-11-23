all: stl2c2d

stl2c2d: Makefile stl.c main.c fenrus.h triangle.c image.c
	gcc $(CFLAGS) -O3 -Wall -W -Wno-address-of-packed-member -flto -ffunction-sections stl.c main.c triangle.c image.c -o stl2c2d -lm -lpng
	

clean:
	rm -f *~ stl2c2d