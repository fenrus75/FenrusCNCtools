all: inlay

inlay: Makefile inlay.cpp inlay.h render.cpp render.h tool.h tool.cpp correlate.cpp
	g++ -g -O2 -flto -Wall inlay.cpp render.cpp tool.cpp correlate.cpp -o inlay
	
	
clean:
	rm -f a.out inlay *.o *~
	