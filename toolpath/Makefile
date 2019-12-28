all: toolpath


OBJS := linalg.o tooldepth.o toollib.o gcode.o toolpath.o inputshape.o main.o scene.o toollevel.o svg.o parse_svg.o


%.o : %.c toolpath.h Makefile
	    @echo "Compiling: $< => $@"
	    @gcc $(CFLAGS) -march=native  -ffunction-sections  -Wall -W -O3 -g -c $< -o $@

%.o : %.cpp toolpath.h print.h tool.h Makefile scene.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3  -march=native -ffunction-sections -fno-common -Wall -W -g -c $< -o $@

toolpath: Makefile $(OBJS)
	g++ -O2 $(OBJS) -o toolpath -lCGAL -lgmp -lCGAL_Core -lmpfr
	
clean:
	rm -f *.o *~ DEADJOE toolpath