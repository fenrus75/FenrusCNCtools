#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fenrus.h"



int main(int argc, char **argv)
{	
	if (argc <= 1) {
		printf("Usage:\n\tstl2c2d <file.stl>\n");
		return EXIT_SUCCESS;
	}

	read_stl_file(argv[1]);

	print_triangle_stats();
	return EXIT_SUCCESS;
}