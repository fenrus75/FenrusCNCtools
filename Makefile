all: stl2png stl2png.exe

OBJS := stl.o main.o triangle.o image.o
WOBJS := stl.wo main.wo triangle.wo image.wo

%.o : %.c fenrus.h Makefile
	    @gcc $(CFLAGS) -Wno-address-of-packed-member -flto -ffunction-sections  -Wall -W -O3 -g -c $< -o $@

%.wo : %.c fenrus.h Makefile
	    @x86_64-w64-mingw32-gcc -Wno-address-of-packed-member -Wall -W -O3 -g -c $< -o $@


stl2png: Makefile fenrus.h $(OBJS)
	gcc -flto $(OBJS) -o stl2png -lm -lpng
	
stl2png.exe: Makefile fenrus.h $(WOBJS)
	x86_64-w64-mingw32-gcc -static $(WOBJS) -o stl2png.exe -lm -lpng -lz
	

clean:
	@rm -f *~ stl2png *.o *.wo stl2png.exe