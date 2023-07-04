all: inlay

inlay: Makefile inlay.cpp inlay.h render.cpp render.h tool.h tool.cpp
	g++ -g -O2 -Wall inlay.cpp render.cpp tool.cpp -o inlay
	
	
clean:
	rm -f a.out inlay *.o *~
	