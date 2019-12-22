/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "fenrus.h"


static int resolution = 512;

int verbose = 0;

int main(int argc, char **argv)
{	
	char *output, *stl;
	int opt;

	while ((opt = getopt(argc, argv, "r:v")) != -1) {
		switch (opt)
		{
			case 'r':
				resolution = strtoull(optarg, NULL, 10);
				printf("Setting resolution to %i\n", resolution);
				break;
			case 'v':
				verbose = 1;
				break;
			
			default:
				printf("Usage:\n\tstl2c2d <file.stl>\n");
				return EXIT_SUCCESS;
		}
	}

	if (optind == argc) {
		printf("Usage:\n\tstl2c2d <file.stl>\n");
		return EXIT_SUCCESS;
	}

	for(; optind < argc; optind++){      
		read_stl_file(argv[optind]);

		normalize_design_to_zero();

		scale_design(resolution);

		print_triangle_stats();

		output = strdup(argv[optind]);
		stl = strstr(output, ".stl");
		if (stl)
			strcpy(stl, ".png");
		else
			output = strdup("output.png");
	

		create_image(output);
		printf("Wrote %s\n", output);
		reset_triangles();
	}
	return EXIT_SUCCESS;

}