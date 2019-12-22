/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>

#include <png.h>

#include "fenrus.h"

extern int verbose;


void create_image(char *filename)
{
	int maxX, maxY;
	double scale;
	unsigned char *pixels;
	FILE *file;
	int x, y;

	png_structp png;
	png_infop info;

	file = fopen(filename, "wb");
	if (!file)
		return;

	maxX = image_X();
	maxY = image_Y();
	scale = scale_Z();

	pixels = calloc(maxX, maxY);

	for (y = 0; y < maxY; y++) {
		for (x = 0; x < maxX; x++) {
			pixels[x + y * maxX] = scale * get_height(x, y);
		}
		if (verbose) {
			printf("\rLine %i",y);
			fflush(stdout);
		}
	}


	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info = png_create_info_struct(png);

	png_init_io(png, file);
	png_set_IHDR(png, info, maxX, maxY, 8, PNG_COLOR_TYPE_GRAY,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	for (y = 0; y < maxY; y++)
		png_write_row(png, &pixels[(maxY - y - 1) * maxX]);
	png_write_end(png, NULL);

	fclose(file);
}