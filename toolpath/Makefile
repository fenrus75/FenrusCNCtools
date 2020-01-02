all: toolpath


OBJS := linalg.o tooldepth.o toollib.o gcode.o toolpath.o inputshape.o main.o scene.o toollevel.o svg.o parse_svg.o


%.o : %.c toolpath.h Makefile
	    @echo "Compiling: $< => $@"
	    @gcc $(CFLAGS) -march=native  -ffunction-sections  -Wall -W -O2 -g2 -c $< -o $@

%.o : %.cpp toolpath.h print.h tool.h Makefile scene.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O2  -march=native -frounding-math -ffunction-sections -fno-common -Wall -W -g2 -c $< -o $@

toolpath: Makefile $(OBJS)
	g++ -g -O2 $(OBJS) -o toolpath -lCGAL -lgmp -lCGAL_Core -lmpfr
	
la_test: Makefile la_test.o linalg.o
	gcc la_test.o linalg.o -lm -o la_test
	
clean:
	rm -f *.o *~ DEADJOE toolpath