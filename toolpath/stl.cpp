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
static double cur_X, cur_Y, cur_Z;
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
		cur_X = X2;
		cur_Y = Y2;
		cur_Z = Z2;
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
					tool->no_sort = true;
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

/*
sin/cos circle table:
Angle  0.00     X 1.0000   Y 0.0000
Angle 22.50     X 0.9239   Y 0.3827
Angle 45.00     X 0.7071   Y 0.7071
Angle 67.50     X 0.3827   Y 0.9239
Angle 90.00     X 0.0000   Y 1.0000
Angle 112.50     X -0.3827   Y 0.9239
Angle 135.00     X -0.7071   Y 0.7071
Angle 157.50     X -0.9239   Y 0.3827
Angle 180.00     X -1.0000   Y 0.0000
Angle 202.50     X -0.9239   Y -0.3827
Angle 225.00     X -0.7071   Y -0.7071
Angle 247.50     X -0.3827   Y -0.9239
Angle 270.00     X -0.0000   Y -1.0000
Angle 292.50     X 0.3827   Y -0.9239
Angle 315.00     X 0.7071   Y -0.7071
Angle 337.50     X 0.9239   Y -0.3827
*/
static inline double get_height_tool(double X, double Y, double R)
{	
	double d = 0;

	d = fmax(d, get_height(X + 0.0000 * R, Y + 0.0000 * R));

	d = fmax(d, get_height(X + 1.0000 * R, Y + 0.0000 * R));
	d = fmax(d, get_height(X + 0.9239 * R, Y + 0.3827 * R));
	d = fmax(d, get_height(X + 0.7071 * R, Y + 0.7071 * R));
	d = fmax(d, get_height(X + 0.3827 * R, Y + 0.9239 * R));
	d = fmax(d, get_height(X + 0.0000 * R, Y + 1.0000 * R));
	d = fmax(d, get_height(X - 0.3872 * R, Y + 0.9239 * R));
	d = fmax(d, get_height(X - 0.7071 * R, Y + 0.7071 * R));
	d = fmax(d, get_height(X - 0.9239 * R, Y + 0.3827 * R));
	d = fmax(d, get_height(X - 1.0000 * R, Y + 0.0000 * R));
	d = fmax(d, get_height(X - 0.9239 * R, Y - 0.3827 * R));
	d = fmax(d, get_height(X - 0.7071 * R, Y - 0.7071 * R));
	d = fmax(d, get_height(X - 0.3827 * R, Y - 0.9239 * R));
	d = fmax(d, get_height(X - 0.0000 * R, Y - 1.0000 * R));
	d = fmax(d, get_height(X + 0.3827 * R, Y - 0.9239 * R));
	d = fmax(d, get_height(X + 0.7071 * R, Y - 0.7071 * R));
	d = fmax(d, get_height(X + 0.9239 * R, Y - 0.3827 * R));

	R = R / 1.5;

	if (R < 1)
		return d;

	d = fmax(d, get_height(X + 1.0000 * R, Y + 0.0000 * R));
	d = fmax(d, get_height(X + 0.9239 * R, Y + 0.3827 * R));
	d = fmax(d, get_height(X + 0.7071 * R, Y + 0.7071 * R));
	d = fmax(d, get_height(X + 0.3827 * R, Y + 0.9239 * R));
	d = fmax(d, get_height(X + 0.0000 * R, Y + 1.0000 * R));
	d = fmax(d, get_height(X - 0.3872 * R, Y + 0.9239 * R));
	d = fmax(d, get_height(X - 0.7071 * R, Y + 0.7071 * R));
	d = fmax(d, get_height(X - 0.9239 * R, Y + 0.3827 * R));
	d = fmax(d, get_height(X - 1.0000 * R, Y + 0.0000 * R));
	d = fmax(d, get_height(X - 0.9239 * R, Y - 0.3827 * R));
	d = fmax(d, get_height(X - 0.7071 * R, Y - 0.7071 * R));
	d = fmax(d, get_height(X - 0.3827 * R, Y - 0.9239 * R));
	d = fmax(d, get_height(X - 0.0000 * R, Y - 1.0000 * R));
	d = fmax(d, get_height(X + 0.3827 * R, Y - 0.9239 * R));
	d = fmax(d, get_height(X + 0.7071 * R, Y - 0.7071 * R));
	d = fmax(d, get_height(X + 0.9239 * R, Y - 0.3827 * R));

	R = R / 1.5;

	if (R < 1)
		return d;

	d = fmax(d, get_height(X + 1.0000 * R, Y + 0.0000 * R));
	d = fmax(d, get_height(X + 0.9239 * R, Y + 0.3827 * R));
	d = fmax(d, get_height(X + 0.7071 * R, Y + 0.7071 * R));
	d = fmax(d, get_height(X + 0.3827 * R, Y + 0.9239 * R));
	d = fmax(d, get_height(X + 0.0000 * R, Y + 1.0000 * R));
	d = fmax(d, get_height(X - 0.3872 * R, Y + 0.9239 * R));
	d = fmax(d, get_height(X - 0.7071 * R, Y + 0.7071 * R));
	d = fmax(d, get_height(X - 0.9239 * R, Y + 0.3827 * R));
	d = fmax(d, get_height(X - 1.0000 * R, Y + 0.0000 * R));
	d = fmax(d, get_height(X - 0.9239 * R, Y - 0.3827 * R));
	d = fmax(d, get_height(X - 0.7071 * R, Y - 0.7071 * R));
	d = fmax(d, get_height(X - 0.3827 * R, Y - 0.9239 * R));
	d = fmax(d, get_height(X - 0.0000 * R, Y - 1.0000 * R));
	d = fmax(d, get_height(X + 0.3827 * R, Y - 0.9239 * R));
	d = fmax(d, get_height(X + 0.7071 * R, Y - 0.7071 * R));
	d = fmax(d, get_height(X + 0.9239 * R, Y - 0.3827 * R));


	return d;
}

static void print_progress(double pct) {
	char line[] = "----------------------------------------";
	int i;
	int len = strlen(line);
	for (i = 0; i < len; i++ ) {
		if (i * 100.0 / len < pct)
			line[i] = '#';
	}
	printf("Progress =[%s]=     \r", line);
	fflush(stdout);
}

static void create_toolpath(class scene *scene, int tool, bool roughing)
{
	double X, Y = 0, maxX, maxY, stepover;
	double maxZ, diam;
	double offset = 0;

	class inputshape *input;
	toolnr = tool;
	diam = tool_diam(tool);
	maxZ = scene->get_cutout_depth();


	Y = -diam/2;
	maxX = stl_image_X() + diam/2;
	maxY = stl_image_Y() + diam/2;
	stepover = get_tool_stepover(toolnr);

	if (!roughing && stepover > 0.2)
		stepover = stepover / 1.42;

	offset = 0;
	if (roughing) 
		offset = scene->get_stock_to_leave();

	if (roughing || scene->want_finishing_pass()) {
		input = new(class inputshape);
		input->set_name("STL path");
		scene->shapes.push_back(input);
		while (Y < maxY) {
			X = -diam/2;
			first = true;
			while (X < maxX) {
				double d;
				d = get_height_tool(X, Y, diam + offset);

				line_to(input, X, Y, -maxZ + d + offset);

				X = X + stepover;
			}
			print_progress(100.0 * Y / maxY);
			Y = Y + stepover;
			line_to(input, X, Y, -maxZ + offset + get_height_tool(X, Y, diam + offset));

			while (X > -diam/2) {
				double d;
				d = get_height_tool(X, Y, diam + offset);

				line_to(input, X, Y, -maxZ + d + offset);

				X = X - stepover;
			}
			print_progress(100.0 * Y / maxY);
			Y = Y + stepover;
		}
	}
	if (!roughing) {
		input = new(class inputshape);
		input->set_name("STL path");
		scene->shapes.push_back(input);
		X = -diam/2;
		while (X < maxX) {
			Y = -diam/2;
			first = true;
			while (Y < maxY) {
				double d;
				d = get_height_tool(X, Y, diam + offset);

				line_to(input, X, Y, -maxZ + d + offset);

				Y = Y + stepover;
			}
			print_progress(100.0 * X / maxX);
			X = X + stepover;
			line_to(input, X, Y, -maxZ + offset + get_height_tool(X, Y, diam + offset));

			while (Y > - diam/2) {
				double d;
				d = get_height_tool(X, Y, diam + offset);

				line_to(input, X, Y, -maxZ + d + offset);
				Y = Y - stepover;
			}
			print_progress(100.0 * X / maxX);
			X = X + stepover;
		}
	}

	printf("                                                          \r");
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

	for ( int i = scene->get_tool_count() - 1; i >= 0 ; i-- ) {
		activate_tool(scene->get_tool_nr(i));
		printf("Create toolpaths for tool %i \n", scene->get_tool_nr(i));

		/* only for the first roughing tool do we need to honor the max tool depth */
		if (i != 0) {
			tooldepth = 5000;
		} else {
			tooldepth = get_tool_maxdepth();
		}
		create_toolpath(scene, scene->get_tool_nr(i), i < (int)scene->get_tool_count() - 1);
	}
}


