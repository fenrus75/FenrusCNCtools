all: stl2png

OBJS := stl.o main.o triangle.o image.o

%.o : %.c fenrus.h Makefile
	    @gcc $(CFLAGS) -Wno-address-of-packed-member -flto -ffunction-sections  -Wall -W -O3 -g -c $< -o $@


stl2png: Makefile fenrus.h $(OBJS)
	gcc -flto $(OBJS) -o stl2png -lm -lpng
	

clean:
	@rm -f *~ stl2png *.o