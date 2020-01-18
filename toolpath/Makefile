all: toolpath toolpath-fine


OBJS := linalg.o tooldepth.o toollib.o gcode.o toolpath.o inputshape.o main.o scene.o toollevel.o svg.o parse_svg.o

FOBJS := linalg.o tooldepth.fo toollib.fo gcode.o toolpath.fo inputshape.fo main.fo scene.fo toollevel.fo svg.o parse_svg.fo


%.o : %.c toolpath.h Makefile
	    @echo "Compiling: $< => $@"
	    @gcc $(CFLAGS) -march=native  -ffunction-sections  -Wall -W -O3 -flto -g2 -c $< -o $@

%.o : %.cpp toolpath.h print.h tool.h Makefile scene.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3 -flto  -march=native -frounding-math -ffunction-sections -fno-common -Wall -W -g2 -c $< -o $@

%.fo : %.cpp toolpath.h print.h tool.h Makefile scene.h
	    @echo "Compiling: $< => $@ (fine)"
	    @g++ $(CFLAGS) -O3 -flto -DFINE  -march=native -frounding-math -ffunction-sections -fno-common -Wall -W -g2 -c $< -o $@

toolpath: Makefile $(OBJS)
	g++ -g -O3 -flto $(OBJS) -o toolpath -lCGAL -lgmp -lCGAL_Core -lmpfr

toolpath-fine: Makefile $(FOBJS)
	g++ -g -O3 -flto $(FOBJS) -DFINE  -o toolpath-fine -lCGAL -lgmp -lCGAL_Core -lmpfr
	
la_test: Makefile la_test.o linalg.o
	gcc la_test.o linalg.o -lm -o la_test
	
clean:
	rm -f *.o *~ DEADJOE toolpath