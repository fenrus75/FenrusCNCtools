/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

extern "C" {
#include <math.h>
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


static inline double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

static double dist3(double X0, double Y0, double Z0, double X1, double Y1, double Z1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0) + (Z1-Z0)*(Z1-Z0));
}

static void flip_triangle_YZ(float *R) 
{
	float x,y,z;
	x = (R)[0];
	y = (R)[1];
	z = (R)[2];

	(R)[0] = x;
	(R)[1] = z;
	(R)[2] = -y;
}

static void flip_triangle_XZ(float *R) 
{
	float x,y,z;
	x = (R)[0];
	y = (R)[1];
	z = (R)[2];

	(R)[0] = z;
	(R)[1] = y;
	(R)[2] = -x;
}
static int read_stl_file(const char *filename, int flip)
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

		if (flip == 1) {
			flip_triangle_YZ(&t.vertex1[0]);
			flip_triangle_YZ(&t.vertex2[0]);
			flip_triangle_YZ(&t.vertex3[0]);
			flip_triangle_YZ(&t.normal[0]);
		}
		if (flip == 2) {
			flip_triangle_XZ(&t.vertex1[0]);
			flip_triangle_XZ(&t.vertex2[0]);
			flip_triangle_XZ(&t.vertex3[0]);
			flip_triangle_XZ(&t.normal[0]);
		}
		push_triangle(t.vertex1, t.vertex2, t.vertex3, t.normal);
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

	if (dist3(X1,Y1,Z1, X2,Y2,Z2) < 0.000001 && !first)
		return;

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

		Z1 = ceil(Z1 * 20) / 20.0;
		Z2 = ceil(Z2 * 20) / 20.0;
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

#define ACC 100.0

static inline double get_height_tool(double X, double Y, double R, bool ballnose)
{	
	double d = 0, dorg;
	double orgR = R;
	double balloffset = 0.0;

	d = fmax(d, get_height(X + 0.0000 * R, Y + 0.0000 * R));

	if (ballnose) {
		balloffset = sqrt(orgR*orgR - R*R) - orgR;
	}

	d = fmax(d, get_height(X + 1.0000 * R, Y + 0.0000 * R) + balloffset);
	d = fmax(d, get_height(X + 0.0000 * R, Y + 1.0000 * R) + balloffset);
	d = fmax(d, get_height(X - 1.0000 * R, Y + 0.0000 * R) + balloffset);
	d = fmax(d, get_height(X - 0.0000 * R, Y - 1.0000 * R) + balloffset);

	dorg = d;
	d = fmax(d, get_height(X + 0.7071 * R, Y + 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X - 0.7071 * R, Y + 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X - 0.7071 * R, Y - 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X + 0.7071 * R, Y - 0.7071 * R) + balloffset);

#if 1
	if (R < 0.6 && fabs(d-dorg) < 0.1)
		return ceil(d*ACC)/ACC;
#endif

	d = fmax(d, get_height(X + 0.9239 * R, Y + 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X + 0.3827 * R, Y + 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X - 0.3872 * R, Y + 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X - 0.9239 * R, Y + 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X - 0.9239 * R, Y - 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X - 0.3827 * R, Y - 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X + 0.3827 * R, Y - 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X + 0.9239 * R, Y - 0.3827 * R) + balloffset);

	R = R / 1.5;

	if (R < 0.4)
		return ceil(d*ACC)/ACC;

	if (ballnose) {
		balloffset = sqrt(orgR*orgR - R*R) - orgR;
	}

	d = fmax(d, get_height(X + 1.0000 * R, Y + 0.0000 * R) + balloffset);
	d = fmax(d, get_height(X + 0.9239 * R, Y + 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X + 0.7071 * R, Y + 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X + 0.3827 * R, Y + 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X + 0.0000 * R, Y + 1.0000 * R) + balloffset);
	d = fmax(d, get_height(X - 0.3872 * R, Y + 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X - 0.7071 * R, Y + 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X - 0.9239 * R, Y + 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X - 1.0000 * R, Y + 0.0000 * R) + balloffset);
	d = fmax(d, get_height(X - 0.9239 * R, Y - 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X - 0.7071 * R, Y - 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X - 0.3827 * R, Y - 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X - 0.0000 * R, Y - 1.0000 * R) + balloffset);
	d = fmax(d, get_height(X + 0.3827 * R, Y - 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X + 0.7071 * R, Y - 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X + 0.9239 * R, Y - 0.3827 * R) + balloffset);

	R = R / 1.5;

	if (R < 0.4)
		return ceil(d*ACC)/ACC;


	if (ballnose)
		balloffset = sqrt(orgR*orgR - R*R) - orgR;

	d = fmax(d, get_height(X + 1.0000 * R, Y + 0.0000 * R) + balloffset);
	d = fmax(d, get_height(X + 0.9239 * R, Y + 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X + 0.7071 * R, Y + 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X + 0.3827 * R, Y + 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X + 0.0000 * R, Y + 1.0000 * R) + balloffset);
	d = fmax(d, get_height(X - 0.3872 * R, Y + 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X - 0.7071 * R, Y + 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X - 0.9239 * R, Y + 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X - 1.0000 * R, Y + 0.0000 * R) + balloffset);
	d = fmax(d, get_height(X - 0.9239 * R, Y - 0.3827 * R) + balloffset);
	d = fmax(d, get_height(X - 0.7071 * R, Y - 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X - 0.3827 * R, Y - 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X - 0.0000 * R, Y - 1.0000 * R) + balloffset);
	d = fmax(d, get_height(X + 0.3827 * R, Y - 0.9239 * R) + balloffset);
	d = fmax(d, get_height(X + 0.7071 * R, Y - 0.7071 * R) + balloffset);
	d = fmax(d, get_height(X + 0.9239 * R, Y - 0.3827 * R) + balloffset);


	return ceil(d*ACC)/ACC;

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

static void create_cutout(class scene *scene, int tool)
{
	Polygon_2 *p;
	double diam = tool_diam(tool);;
	double gradient = 0, circumfence = 0;
	double currentdepth = -scene->get_cutout_depth();
	class inputshape *input;

	input = new(class inputshape);
	input->set_name("Cutout path");
	scene->shapes.push_back(input);

	p = new(Polygon_2);
	p->push_back(Point(-diam/2, -diam/2));
	p->push_back(Point(stl_image_X() + diam/2, -diam/2));
	p->push_back(Point(stl_image_X() + diam/2, stl_image_Y() + diam/2));
	p->push_back(Point(-diam/2, stl_image_Y() + diam / 2));

	for (unsigned int i = 0; i < p->size(); i++) {
		unsigned int next = i + 1;
		if (next >= p->size())
			next = 0;

		class tooldepth * td = new(class tooldepth);
		input->tooldepths.push_back(td);
		td->depth = currentdepth;
		td->toolnr = toolnr;
		td->diameter = get_tool_diameter();
		
		class toollevel *tool = new(class toollevel);
		tool->level = 0;
		tool->offset = get_tool_diameter();
		tool->diameter = get_tool_diameter();
		tool->depth = currentdepth;
		tool->toolnr = toolnr;
		tool->minY = 0;
		tool->name = "Cutout";
		td->toollevels.push_back(tool);

	    Polygon_2 *p2;
		p2 = new(Polygon_2);
		double d1 = currentdepth;
		p2->push_back(Point(CGAL::to_double((*p)[i].x()), CGAL::to_double((*p)[i].y())));
		p2->push_back(Point(CGAL::to_double((*p)[next].x()), CGAL::to_double((*p)[next].y())));
		tool->add_poly_vcarve(p2, d1, d1);
	}			

	for (unsigned int i = 0; i < p->size(); i++) {
		unsigned int next = i + 1;
		if (next >= p->size())
			next = 0;
		circumfence += dist(
					CGAL::to_double((*p)[i].x()), 
					CGAL::to_double((*p)[i].y()), 
					CGAL::to_double((*p)[next].x()), 
					CGAL::to_double((*p)[next].y()));
	}

	if (circumfence == 0)
		return;

	gradient = fabs(get_tool_maxdepth()) / circumfence;
	/*     walk the gradient up until we break the surface */
	while (currentdepth < 0) {
		for (unsigned int i = 0; i < p->size(); i++) {
			unsigned int next = i + 1;
			if (next >= p->size())
				next = 0;

			if (currentdepth > 0)
				break;

			class tooldepth * td = new(class tooldepth);
			input->tooldepths.push_back(td);
			td->depth = currentdepth;
			td->toolnr = toolnr;
			td->diameter = get_tool_diameter();
			class toollevel *tool = new(class toollevel);
			tool->level = 0;
			tool->offset = get_tool_diameter();
			tool->diameter = get_tool_diameter();
			tool->depth = currentdepth;
			tool->toolnr = toolnr;
			tool->minY = 0;
			tool->name = NULL;
			td->toollevels.push_back(tool);

			Polygon_2 *p2;
			p2 = new(Polygon_2);
			double d1 = currentdepth;
			double d2 = gradient * dist(	CGAL::to_double((*p)[i].x()), 
														CGAL::to_double((*p)[i].y()), 
														CGAL::to_double((*p)[next].x()), 
														CGAL::to_double((*p)[next].y()));
			p2->push_back(Point(CGAL::to_double((*p)[i].x()), CGAL::to_double((*p)[i].y())));
			p2->push_back(Point(CGAL::to_double((*p)[next].x()), CGAL::to_double((*p)[next].y())));
			tool->add_poly_vcarve(p2, d1, d1 + d2);
			currentdepth += d2;
					
		}
	}
}

static bool outside_area(double X, double Y, double mX, double mY, double diam)
{
	if (Y > mY)
		Y = mY - Y;

	if (X > mX)
		X = mX - X;

	if (X < 0 && Y < 0 && sqrt(X*X+Y*Y) > diam/2 * 0.90)
		return true;

	return false;

}


static void create_toolpath(class scene *scene, int tool, bool roughing, bool has_cutout, bool even)
{
	double X, Y = 0, maxX, maxY, stepover;
	double maxZ, diam, radius;
	double offset = 0;
	bool ballnose = false;

	double overshoot;

	class inputshape *input;
	toolnr = tool;
	diam = tool_diam(tool);
	maxZ = scene->get_cutout_depth();


	radius = diam / 2;
	/* when roughing, look much wider */
//	if (roughing)
//		radius = diam / 1.9;

	overshoot = diam/2 * 0.9;

	if (!has_cutout) {
		overshoot = overshoot /2;
		if (roughing)
			overshoot = 0;
	}

	Y = -overshoot;
	maxX = stl_image_X() + overshoot;
	maxY = stl_image_Y() + overshoot;

	stepover = get_tool_stepover(toolnr);

	if (!roughing && stepover > 0.2)
		stepover = stepover / 1.42;

	if (!roughing && tool_is_ballnose(tool)) { 
		stepover = stepover / 2;
		ballnose = true;
		if (scene->get_finishing_pass_stepover() > 0)
			stepover = scene->get_finishing_pass_stepover();
	}

	offset = 0;
	if (roughing) 
		offset = scene->get_stock_to_leave();

	if (roughing)
		gcode_set_roughing(1);

	if (even) {
		input = new(class inputshape);
		input->set_name("STL path");
		scene->shapes.push_back(input);
		first = true;
		while (Y < maxY) {
			double prevX;
			X = -overshoot;
			prevX = X;
			while (X < maxX) {
				double d;
				d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;

				if (fabs(d - last_Z) > 0.5 && roughing) {
					X = prevX + stepover / 3;
					d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
					if (fabs(d - last_Z) > 0.5) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
				}

				if (!outside_area(X, Y, stl_image_X(), stl_image_Y(), diam))
					line_to(input, X, Y, d);

				prevX = X;
				X = X + stepover;
			}
			print_progress(100.0 * Y / maxY);
			Y = Y + stepover;
			X = maxX;
			if (!outside_area(X, Y, stl_image_X(), stl_image_Y(), diam)) {
				double d =  -maxZ + offset + get_height_tool(X, Y, radius + offset, ballnose);
				if (fabs(d - last_Z) > 0.1) {
					line_to(input, last_X, last_Y, fmax(last_Z, d));
					line_to(input, X, Y, fmax(last_Z, d));
				}
				line_to(input, X, Y, d);
			}
			prevX = X;
			while (X > -overshoot) {
				double d;
				d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
				if (fabs(d - last_Z) > 0.5 && roughing) {
					X = prevX - stepover / 3;
					d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
					if (fabs(d - last_Z) > 0.5) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
				}

				line_to(input, X, Y, d);

				prevX = X;
				X = X - stepover;
			}

			X = -overshoot;
			print_progress(100.0 * Y / maxY);
			Y = Y + stepover;
			if (Y < maxY && !outside_area(X, Y, stl_image_X(), stl_image_Y(), diam)) {
					double d =  -maxZ + offset + get_height_tool(X, Y, radius + offset, ballnose);
					if (fabs(d - last_Z) > 0.1) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
					line_to(input, X, Y, d);
			}
		}
	}
	if (!even) {
		input = new(class inputshape);
		input->set_name("STL path");
		scene->shapes.push_back(input);
		first = true;
		X = -overshoot;
		while (X < maxX) {
			double prevY;
			Y = -overshoot;
			prevY = Y;
			while (Y < maxY) {
				double d;
				d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
				if (fabs(d - last_Z) > 0.5 && roughing) {
					Y = prevY + stepover / 3;
					d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
					if (fabs(d - last_Z) > 0.5) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
				}


				if (!outside_area(X, Y, stl_image_X(), stl_image_Y(), diam)) {
					line_to(input, X, Y, d);
				}
				prevY = Y;
				Y = Y + stepover;
			}
			print_progress(100.0 * X / maxX);
			X = X + stepover;
			Y = maxY;
			if (!outside_area(X, Y, stl_image_X(), stl_image_Y(), diam) &&  (X < maxX)) {
					double d =  -maxZ + offset + get_height_tool(X, Y, radius + offset, ballnose);
					if (fabs(d - last_Z) > 0.1) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
					line_to(input, X, Y, d);
			}
			prevY = Y;
			while (Y > - overshoot) {
				double d;
				d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
				if (fabs(d - last_Z) > 0.5 && roughing) {
					Y = prevY - stepover / 3;
					d = get_height_tool(X, Y, radius + offset, ballnose) + offset - maxZ;
					if (fabs(d - last_Z) > 0.5) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
				}


				if (!outside_area(X, Y, stl_image_X(), stl_image_Y(), diam))
					line_to(input, X, Y, d);
				prevY = Y;
				Y = Y - stepover;
			}
			print_progress(100.0 * X / maxX);
			X = X + stepover;
			Y = -overshoot;

			if (!outside_area(X, Y, stl_image_X(), stl_image_Y(), diam) &&  (X < maxX)) {
					double d =  -maxZ + offset + get_height_tool(X, Y, radius + offset, ballnose);
					if (fabs(d - last_Z) > 0.1) {
						line_to(input, last_X, last_Y, fmax(last_Z, d));
						line_to(input, X, Y, fmax(last_Z, d));
					}
					line_to(input, X, Y, d);
			}

		}
	}

	printf("                                                          \r");
}


static void process_vertical(class scene *scene, int tool, bool roughing)
{
	double	radius = tool_diam(tool)/2 + 0.001;
	double offset = 0;
	struct line *lines;
	int i, nexti;
	int maxlines = 0;
	class inputshape *input;
	int loopi;

	double maxZ = scene->get_cutout_depth();

	toolnr = tool;
	/* not doing tapered ballnoses */
	if (tool_is_ballnose(toolnr))
		return;

	input = new(class inputshape);
	input->set_name("STL vertical");
	scene->shapes.push_back(input);

	if (roughing) {
		radius += scene->get_stock_to_leave();
		offset = scene->get_stock_to_leave();
	}



	printf("Radius is %5.4f offset is %5.4f  maxZ is %5.4f\n",
			radius, offset, maxZ);

	lines = stl_vertical_triangles(radius);
	if (!lines)
		return;

	i = 0;
	do {
		maxlines++;
		if (lines[i].valid != 1) {
			i++;
			continue;
		}	
		i++;
	} while (lines[i].valid >= 0);

	for (loopi = 0; loopi < maxlines; loopi++) {
		int q = 0, p;
		i = loopi;
		if (lines[i].valid != 1)
			continue;
		nexti = -1;
		p = lines[i].prev;
		first = true;

		while (q < 150 && p >= 0 && lines[p].valid == 1) {
			q++;
			i = p;
			p = lines[i].prev;
		}
		do {
			int j;
			double l, d;
			double lstep;
			double vX,vY;
			double X1,Y1,X2,Y2;
	
			if (lines[i].valid != 1)
				continue;

			first = true;

			X1 = lines[i].X1;
			X2 = lines[i].X2;
			Y1 = lines[i].Y1;
			Y2 = lines[i].Y2;

			vX = X2-X1;
			vY = Y2-Y1;
			l = 0;
			lstep = 0.1 / dist(X1,Y1,X2,Y2);
//			printf("First  lstep %5.4f\n", lstep);
			while (l <= 1) {
				double X,Y;
				X = X1 + l * vX;
				Y = Y1 + l * vY;
				l = l + lstep;

				d = get_height_tool(X, Y, radius, false) + offset - maxZ;
				if (d > 0)
					continue;
				if (fabs(d - last_Z) > 0.2 && !first) {
					line_to(input, last_X, last_Y, fmin(fmax(last_Z, d), 0.1));
					line_to(input, X, Y, fmin(fmax(last_Z, d), 0.1));
				}
				line_to(input, X, Y, d);
//				printf("line %5.4f %5.4f %5.4f\n", X, Y, d);
			}
			d = get_height_tool(X2, Y2, radius, false) + offset - maxZ;
			if (fabs(d - last_Z) > 0.2 && !first && d <= 0) {
					line_to(input, last_X, last_Y, fmin(fmax(last_Z, d), 0.1));
					line_to(input, X2, Y2, fmin(fmax(last_Z, d), 0.1));
			}
			if (d < 0)
				line_to(input, X2, Y2, d);
			lines[i].valid = 0;
			nexti = -1;
			for (j = 0; j < maxlines && nexti == -1; j++ ) {
				if (i == j || lines[j].valid != 1)
					continue;


				if (approx2(lines[i].X2, lines[j].X1) && approx2(lines[i].Y2, lines[j].Y1)) {
					nexti = j;
				}

				if (approx2(lines[i].X2, lines[j].X2) && approx2(lines[i].Y2, lines[j].Y2)) {
					nexti = j;

					double x1,y1,x2,y2;
					x1 = lines[j].X1;
					x2 = lines[j].X2;
					y1 = lines[j].Y1;
					y2 = lines[j].Y2;
					lines[j].X1 = x2;
					lines[j].X2 = x1;
					lines[j].Y1 = y2;
					lines[j].Y2 = y1;
				}

			}
			i = nexti;
		} while (nexti >= 0);
	}
}

void process_stl_file(class scene *scene, const char *filename, int flip)
{
	bool omit_cutout = false;
	bool even = true;

	read_stl_file(filename, flip);
	normalize_design_to_zero();

	if (scene->get_cutout_depth() < 0.01) {
		scene->set_cutout_depth(scene->get_depth());
		printf("Warning: No cutout depth set, using %5.2fmm for the model height\n", scene->get_cutout_depth());
		omit_cutout = true;
	}

	scale_design_Z(scene->get_cutout_depth(), scene->get_z_offset());
	print_triangle_stats();


	for ( int i = scene->get_tool_count() - 1; i >= 0 ; i-- ) {
		activate_tool(scene->get_tool_nr(i));

		printf("Create toolpaths for tool %i \n", scene->get_tool_nr(i));

		tooldepth = get_tool_maxdepth();

		process_vertical(scene, scene->get_tool_nr(i), i < (int)scene->get_tool_count() - 1);

		/* only for the first roughing tool do we need to honor the max tool depth */
		if (i != 0) 
			tooldepth = 5000;

		create_toolpath(scene, scene->get_tool_nr(i), i < (int)scene->get_tool_count() - 1, !omit_cutout, even);

		even = !even;
		if (i == (int)scene->get_tool_count() - 1 && scene->want_finishing_pass()) {
			create_toolpath(scene, scene->get_tool_nr(i), i < (int)scene->get_tool_count() - 1, !omit_cutout, even);

			even = !even;
		}

	}
	if (!omit_cutout) { 
		activate_tool(scene->get_tool_nr(0));
		create_cutout(scene, scene->get_tool_nr(0));
	}
}


