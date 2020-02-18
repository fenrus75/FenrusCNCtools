all: gcodecheck


OBJS := main.o gcode.o ../toolpath/toollib.o linalg.o
WOBJS := main.wo gcode.wo ../toolpath/toollib.wo linalg.wo



%.o : %.c gcodecheck.h Makefile
	    @echo "Compiling: $< => $@"
	    @gcc $(CFLAGS) -march=native  -ffunction-sections  -Wall -W -O3 -flto -g2 -c $< -o $@

%.o : %.cpp gcodecheck.h Makefile
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3   -march=native -frounding-math -ffunction-sections -fno-common -Wno-address-of-packed-member -Wall -W -g2 -c $< -o $@

%.wo : %.cpp gcodecheck.h Makefile
	    @echo "Compiling: $< => $@ (windows)"
	    @x86_64-w64-mingw32-g++ -I/usr/mingw/include -march=westmere  -L/usr/mingw/lib -Wno-address-of-packed-member -Wall -W -O2 -g -c $< -o $@

%.wo : %.c gcodecheck.h Makefile
	    @echo "Compiling: $< => $@ (windows)"
	    @x86_64-w64-mingw32-gcc -I/usr/mingw/include -march=westmere  -L/usr/mingw/lib -Wno-address-of-packed-member -Wall -W -O2 -g -c $< -o $@


gcodecheck: Makefile $(OBJS)
	g++ -g -O3 $(OBJS) -o gcodecheck 

gcodecheck.exe: Makefile $(WOBJS)
	x86_64-w64-mingw32-g++ -static -O3 $(WOBJS) -o gcodecheck.exe
	x86_64-w64-mingw32-strip gcodecheck.exe 

clean:
	rm -f *.o *.wo *.fo *~ DEADJOE gcodecheck gcodecheck.exe gcodecheck-fine
	ccache -C