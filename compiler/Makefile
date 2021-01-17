all: compiler


OBJS := main.o parser.o  emit.o pass_raw_to_movement.o stats.o pass_vertical_G0.o lib.o pass_split_by_tool.o pass_split_g1.o pass_bounding_box.o

%.o : %.cpp Makefile compiler.h
	    @echo "Compiling: $< => $@"
	    @g++ $(CFLAGS) -O3  -march=native -frounding-math -ffunction-sections -fno-common -Wno-address-of-packed-member -Wall -W -g2 -Wno-unused-variable -Wno-unused-parameter  -c $< -o $@



compiler: Makefile $(OBJS)
	g++ $(CFLAGS) $(OBJS) -o compiler
	
clean:
	rm -f *.o *~ compiler
	