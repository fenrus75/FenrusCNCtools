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



int main(int argc, char **argv)
{	
	char *output, *stl;
	if (argc <= 1) {
		printf("Usage:\n\tstl2c2d <file.stl>\n");
		return EXIT_SUCCESS;
	}

	read_stl_file(argv[1]);

	normalize_design_to_zero();
	scale_design(512);
	print_triangle_stats();

	output = strdup(argv[1]);
	stl = strstr(output, ".stl");
	if (stl)
		strcpy(stl, ".png");
	else
		output = strdup("output.png");
	

	create_image(output);
	printf("Wrote %s\n", output);
	return EXIT_SUCCESS;

}