all: compiler


OBJS := main.o parser.o raw_gcode.o container.o pass_add_g_level.o

%.o : %.cpp Makefile compiler.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3  -march=native -frounding-math -ffunction-sections -fno-common -Wno-address-of-packed-member -Wall -W -g2 -c $< -o $@



compiler: Makefile $(OBJS)
	g++ $(CFLAGS) $(OBJS) -o compiler
	
clean:
	rm -f *.o *~ compiler
	