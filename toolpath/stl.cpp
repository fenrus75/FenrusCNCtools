/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

extern "C" {
#include "fenrus.h"
#include "toolpath.h"
}
#include "scene.h"

struct stltriangle {
	float normal[3];
	float vertex1[3];
	float vertex2[3];
	float vertex3[3];
	uint16_t attribute;
} __attribute__((packed));

static int toolnr;
static double tooldepth = 0.1;

static void print_stl_triangle(struct stltriangle *t)
{
	printf("\t(%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f)  %i\n",
		t->vertex1[0], t->vertex1[1], t->vertex1[2],
		t->vertex2[0], t->vertex2[1], t->vertex2[2],
		t->vertex3[0], t->vertex3[1], t->vertex3[2],
		t->attribute);
}


static int read_stl_file(const char *filename)
{
	FILE *file;
	char header[80];
	uint32_t trianglecount;
	int ret;
	uint32_t i;

	file = fopen(filename, "rb");
	if (!file) {
		printf("Failed to open file %s: %s\n", filename, strerror(errno));
		return -1;
	}
	
	ret = fread(header, 1, 80, file);
	if (ret <= 0) {
		printf("STL file too short\n");
		fclose(file);
		return -1;
	}
	ret = fread(&trianglecount, 1, 4, file);
	set_max_triangles(trianglecount);

	for (i = 0; i < trianglecount; i++) {
		struct stltriangle t;
		ret = fread(&t, 1, sizeof(struct stltriangle), file);
		if (ret < 1)
			break;
		push_triangle(t.vertex1, t.vertex2, t.vertex3);
	}

	fclose(file);
	return 0;
}

static double last_X,  last_Y, last_Z;
static bool first;

static void line_to(class inputshape *input, double X2, double Y2, double Z2)
{
	double X1 = last_X, Y1 = last_Y, Z1 = last_Z;
	unsigned int depth = 0;

	last_X = X2;
	last_Y = Y2;
	last_Z = Z2;

	if (first) {
		first = false;
		return;
	}

	while (Z1 < -0.000001 || Z2 < -0.00001) {
//		printf("at depth %i    %5.2f %5.2f %5.2f -> %5.2f %5.2f %5.2f\n", depth, X1, Y1, Z1, X2, Y2, Z2);
		depth++;
		while (input->tooldepths.size() <= depth) {
				class tooldepth * td = new(class tooldepth);
				td->depth = Z1;
				td->toolnr = toolnr;
				td->diameter = get_tool_diameter();
				input->tooldepths.push_back(td);				
		}

		if (input->tooldepths[depth]->toollevels.size() < 1) {
					class toollevel *tool = new(class toollevel);
					tool->level = 0;
					tool->offset = get_tool_diameter();
					tool->diameter = get_tool_diameter();
					tool->depth = Z1;
					tool->toolnr = toolnr;
					tool->minY = 0;
					tool->name = "Manual toolpath";
					input->tooldepths[depth]->toollevels.push_back(tool);
		}
		Polygon_2 *p2;
		p2 = new(Polygon_2);
		p2->push_back(Point(X1, Y1));
		p2->push_back(Point(X2, Y2));
		input->tooldepths[depth]->toollevels[0]->add_poly_vcarve(p2, Z1, Z2);
		
		Z1 += tooldepth;
		Z2 += tooldepth;
	}
}

static inline double get_height_tool(double X, double Y, double R)
{	
	double d;

	d = get_height(X, Y);
	d = fmax(d, get_height(X - R, Y));
	d = fmax(d, get_height(X + R, Y));
	d = fmax(d, get_height(X, Y + R));
	d = fmax(d, get_height(X, Y - R));
	return d;
}

static void create_toolpath(class scene *scene, int tool)
{
	double X, Y = 0, maxX, maxY, stepover;
	double maxZ, diam;
	class inputshape *input;
	toolnr = tool;
	diam = tool_diam(tool);
	maxZ = scene->get_cutout_depth();

	input = new(class inputshape);
	input->set_name("STL path");
	scene->shapes.push_back(input);

	Y = 0;
	maxX = stl_image_X();
	maxY = stl_image_Y();
	stepover = get_tool_stepover(toolnr);
	while (Y < maxY) {
		X = 0;
		first = true;
		while (X < maxX) {
			double d;
			d = get_height_tool(X, Y, diam);

			line_to(input, X, Y, -maxZ + d);
			X = X + stepover;
		}
		printf("Y is %5.2f\n", Y);
		Y = Y + stepover;
	}
}

void process_stl_file(class scene *scene, const char *filename)
{
	read_stl_file(filename);
	normalize_design_to_zero();
	if (scene->get_cutout_depth() < 0.01) {
		printf("Error: No cutout depth set\n");
	}
	scale_design_Z(scene->get_cutout_depth());
	print_triangle_stats();

	create_toolpath(scene, 201);
}


