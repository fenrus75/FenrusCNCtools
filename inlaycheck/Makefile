all: inlay

%.o : %.cpp Makefile inlay.h render.h tool.h  
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3  -march=native -frounding-math -ffast-math -fno-common -Wno-address-of-packed-member -Wno-unused-but-set-variable -flto -Wall -g2 -c $< -o $@  -lpthread

OBJS := inlay.o render.o tool.o correlate.o stloutput.o  gcode.o
inlay: Makefile inlay.h render.h tool.h  $(OBJS)
	g++ -g -O2 -flto -Wall $(OBJS) -o inlay
	
	
clean:
	rm -f a.out inlay *.o *~
	