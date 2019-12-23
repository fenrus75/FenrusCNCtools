all: toolpath


OBJS := toollib.o gcode.o toolpath.o inputshape.o main.o data.o toollevel.o svg.o parse_svg.o


%.o : %.c toolpath.h Makefile
	    @echo "Compiling: $< => $@"
	    @gcc $(CFLAGS) -march=native -flto -ffunction-sections  -Wall -W -O3 -g -c $< -o $@

%.o : %.cpp toolpath.h print.h tool.h Makefile
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3 -flto -march=native -ffunction-sections -fno-common -Wall -W -g -c $< -o $@

toolpath: Makefile $(OBJS)
	g++ -O2 $(OBJS) -o toolpath -lCGAL -lgmp -lCGAL_Core -lmpfr
	
clean:
	rm -f *.o *~ DEADJOE toolpath