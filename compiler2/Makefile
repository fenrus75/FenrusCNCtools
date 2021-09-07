all: compiler


OBJS := main.o parser.o  emit.o stats.o lib.o  pass_bounding_box.o pass_raw_to_movement.o pass_vertical_G0.cpp pass_positioning.cpp pass_split_cut.cpp pass_dependencies.cpp

%.o : %.cpp Makefile compiler.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -g -O0  -march=native -frounding-math -ffunction-sections -fno-common -Wno-address-of-packed-member -Wall -W -g2 -Wno-unused-variable -Wno-unused-parameter  -c $< -o $@



compiler: Makefile $(OBJS)
	g++ $(CFLAGS) $(OBJS) -o compiler
	
clean:
	rm -f *.o *~ compiler
	