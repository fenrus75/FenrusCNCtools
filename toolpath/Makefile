all: toolpath 


OBJS := parse_csv.o linalg.o tooldepth.o toollib.o gcode.o toolpath.o inputshape.o main.o scene.o toollevel.o svg.o parse_svg.o stl.o triangle.o

FOBJS := parse_csv.o linalg.o tooldepth.fo toollib.fo gcode.o toolpath.fo inputshape.fo main.fo scene.fo toollevel.fo svg.o parse_svg.fo stl.o triangle.o

WOBJS := parse_csv.wo linalg.wo tooldepth.wo toollib.wo gcode.wo toolpath.wo inputshape.wo main.wo scene.wo toollevel.wo svg.wo parse_svg.wo stl.wo triangle.wo 


%.o : %.c toolpath.h Makefile
	    @echo "Compiling: $< => $@"
	    @gcc $(CFLAGS) -march=native  -ffunction-sections  -Wall -W -O3 -flto -g2 -c $< -o $@

%.o : %.cpp toolpath.h print.h tool.h Makefile scene.h fenrus.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3   -march=native -frounding-math -ffunction-sections -fno-common -Wno-address-of-packed-member -Wall -W -g2 -c $< -o $@

gcode.o : gcode.cpp toolpath.h print.h tool.h Makefile scene.h fenrus.h
	    @echo "Kompiling: $< => $@"
	    @g++ $(CFLAGS) -O0   -march=native -frounding-math -ffunction-sections -fno-common -Wno-address-of-packed-member -Wall -W -g2 -c $< -o $@

%.fo : %.cpp toolpath.h print.h tool.h Makefile scene.h fenrus.h
	    @echo "Compiling: $< => $@ (fine)"
	    @g++ $(CFLAGS) -O3 -flto -DFINE  -march=native -frounding-math -ffunction-sections -fno-common -Wall -W -g2 -c $< -o $@

%.wo : %.cpp toolpath.h print.h tool.h Makefile scene.h fenrus.h
	    @echo "Compiling: $< => $@ (windows)"
	    @x86_64-w64-mingw32-g++ -I/usr/mingw/include -march=westmere  -L/usr/mingw/lib -Wno-address-of-packed-member -Wall -W -O2 -g -c $< -o $@

%.wo : %.c toolpath.h print.h tool.h Makefile scene.h fenrus.h
	    @echo "Compiling: $< => $@ (windows)"
	    @x86_64-w64-mingw32-gcc -I/usr/mingw/include -march=westmere  -L/usr/mingw/lib -Wno-address-of-packed-member -Wall -W -O2 -g -c $< -o $@


toolpath: Makefile $(OBJS)
	g++ -g -O3 $(OBJS) -o toolpath -lCGAL -lgmp -lCGAL_Core -lmpfr  -lboost_thread

toolpath.exe: Makefile $(WOBJS)
	x86_64-w64-mingw32-g++ -static -O3 $(WOBJS) -o toolpath.exe -L/usr/mingw/lib  -lmpfr -lgmp -lboost_thread
	x86_64-w64-mingw32-strip toolpath.exe 

toolpath-fine: Makefile $(FOBJS)
	g++ -g -O3 -flto $(FOBJS) -DFINE  -o toolpath-fine -lCGAL -lgmp -lCGAL_Core -lmpfr
	
la_test: Makefile la_test.o linalg.o
	gcc la_test.o linalg.o -lm -o la_test
	
clean:
	rm -f *.o *.wo *.fo *~ DEADJOE toolpath toolpath.exe toolpath-fine
	ccache -C