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

	normalize_design_to_zero();
	scale_design(512);
	print_triangle_stats();

	create_image("/var/www/html/stl/output.png");
	return EXIT_SUCCESS;

}