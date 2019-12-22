/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "toolpath.h"

static FILE *output;

static int maxX, maxY, minX, minY;
static double svgscale;

void svg_line(double X1, double Y1, double X2, double Y2, const char *color, double width)
{
	fprintf(output, "<line x1=\"%5.2f\" y1=\"%5.2f\" x2=\"%5.2f\" y2=\"%5.2f\" stroke=\"%s\" stroke-width=\"%f\"/>\n", 
		svgscale * (mm_to_px(X1) - minX), 
		svgscale * (maxY-mm_to_px(Y1)), 
		svgscale * (mm_to_px(X2) - minX), 
		svgscale * (maxY-mm_to_px(Y2)),
		color,
		width);
}

void svg_circle(double X1, double Y1, double radius, const char * color, double width)
{
	fprintf(output, "<circle cx=\"%5.2f\" cy=\"%5.2f\" r=\"%5.2f\"  stroke=\"%s\" stroke-width=\"%f\" />\n", 
		svgscale * (mm_to_px(X1) - minX), 
		svgscale * (maxY-mm_to_px(Y1)), 
		svgscale * mm_to_px(radius), 
		color,
		mm_to_px(width));
}

void set_svg_bounding_box(int X1, int Y1, int X2, int Y2)
{
	minX = mm_to_px(X1);
	minY = mm_to_px(Y1);
	maxX = mm_to_px(X2);
	maxY = mm_to_px(Y2);
}

void write_svg_header(const char *filename, double scale)
{
	output = fopen(filename, "w");
	if (!output) {
		printf("Cannot write to %s (%s)\n", filename, strerror(errno));
		return;
	}
	
	svgscale = scale;

	fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	fprintf(output, "<svg width=\"%ipx\" height=\"%ipx\" xmlns=\"http://www.w3.org/2000/svg\">\n", 
			(int)(scale * (maxX - minX) + 0.99), 
			(int)(scale * (maxY - minY) + 0.99));
			
	fprintf(output, "<defs>\n<marker id=\"arrow\" markerWidth=\"5\" markerHeight=\"5\" refX=\"0\" refY=\"1.5\" orient=\"auto\" markerUnits=\"strokeWidth\">\n<path d=\"M0,0 L0,4 L4.5,1.5 z\" fill=\"#f00\" />\n</marker></defs>");
}

void write_svg_footer(void)
{
	fprintf(output, "</svg>\n");
	fclose(output);
	output = NULL;
}
