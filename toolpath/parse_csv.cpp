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
#include <errno.h>
#include <string.h>
#include <math.h>

#include "scene.h"
extern "C" {
#include "toolpath.h"
}

/* we need to remember previous coordinates for bezier curves */
static double last_X, last_Y, last_Z;
static bool first = true;
static double tooldepth = 0.1;

static int toolnr;

#ifndef FINE
static const double detail_threshold = 0.2;
#else
static const double detail_threshold = 0.1;
#endif

static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

static double dist3(double X0, double Y0, double Z0, double X1, double Y1, double Z1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0) + (Z1-Z0) * (Z1-Z0));
}

static void chr_replace(char *line, char a, char r)
{
    char *c;
    do {
        c = strchr(line, a);
        if (c)
            *c = r;
    } while (c);
}

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


static void circle(class inputshape *input, double X, double Y, double Z, double R)
{
    double phi;
	first = true;
    phi = 0;
    while (phi <= 360) {
        double P;
        P = phi / 360.0 * 2 * M_PI;
		if (first || dist(X + R * cos(P), Y + R * sin(P), last_X, last_Y) > 0.2)
			line_to(input, X + R * cos(P), Y + R * sin(P), Z );
        phi = phi + 1;
    }
	first = true;
}

static void sphere(class inputshape *input, double X, double Y, double Z, double R)
{
	double z = - R;
	double so = get_tool_stepover(toolnr);
	double maxz;

	maxz = so;
	if (maxz > tooldepth / 2)
		maxz = tooldepth / 2;
	
	while (z < -0.00001) {
		double r;
		double deltaZ, z2;

		r  = sqrt(R * R - z * z);

		printf("z is %5.2f,  R is %5.2f  r is %5.2f\n", z, R, r);

		circle(input, X, Y, Z + z, r);
		r += so;
		if (r > R)
			r = R;

		z2 = sqrt(R*R - r*r);
		deltaZ = z2 - z;
		if (deltaZ > maxz)
			deltaZ = maxz;

		if (deltaZ < 0.01)
			deltaZ = 0.01;

		printf("DeltaZ is %5.2f,    r = %5.2f, z = %5.2f\n", deltaZ, r, z);
		z += deltaZ;
	}
	first = true;	
}


static void cubic_bezier(class inputshape *inputshape,
                         double x0, double y0, double z0,
                         double x1, double y1, double z1,
                         double x2, double y2, double z2,
                         double x3, double y3, double z3)
{
    double delta = 0.01;
    double t;
    double lX = x0, lY = y0, lZ = z0;
    
    if (dist(x0,y0,x3,y3) < 150)
            delta = 1/40.0;
    if (dist(x0,y0,x3,y3) < 50)
            delta = 1/20.0;
    if (dist(x0,y0,x3,y3) < 10)
            delta = 1/10.0;
    if (dist(x0,y0,x3,y3) < 5)
            delta = 1/5.0;

    if (dist(x0,y0,x3,y3) < detail_threshold)
            delta = 1.0;

	/* bad stuff happens if we end up and an exect multiple of 1 */
	delta += 0.000013121;
	delta = delta / 4;
    t = 0;
    while (t < 1.0) {
        double nX, nY, nZ;
        nX = (1-t)*(1-t)*(1-t)*x0 + 3*(1-t)*(1-t)*t*x1 + 3 * (1-t)*t*t*x2 + t*t*t*x3;
        nY = (1-t)*(1-t)*(1-t)*y0 + 3*(1-t)*(1-t)*t*y1 + 3 * (1-t)*t*t*y2 + t*t*t*y3;
        nZ = (1-t)*(1-t)*(1-t)*z0 + 3*(1-t)*(1-t)*t*z1 + 3 * (1-t)*t*t*z2 + t*t*t*z3;
        if (dist3(lX,lY,lZ,nX,nY,nZ) >= detail_threshold) {
            line_to(inputshape, nX, nY, nZ);
            lX = nX;
            lY = nY;
			lZ = nZ;
        }
        t = t + delta;
    }

	line_to(inputshape, x3, y3, z3);
}                    

static void quadratic_bezier(class inputshape *inputshape,
                         double x0, double y0, double z0,
                         double x1, double y1, double z1,
                         double x3, double y3, double z3)
{
    double delta = 0.01;
    double t;
    double lX = x0, lY = y0, lZ = z0;

    
    if (dist(x0,y0,x3,y3) < 150)
            delta = 1/40.0;
    if (dist(x0,y0,x3,y3) < 50)
            delta = 1/20.0;
    if (dist(x0,y0,x3,y3) < 10)
            delta = 1/10.0;
    if (dist(x0,y0,x3,y3) < 5)
            delta = 1/5.0;

    if (dist(x0,y0,x3,y3) < detail_threshold)
            delta = 1.0;

	delta += 0.0000131313;
	delta = delta / 4;
    
    t = 0;
    while (t < 1.0) {
        double nX, nY, nZ;
        nX = (1-t)*(1-t)*x0 + 2*(1-t)*t*x1 + t*t*x3;
        nY = (1-t)*(1-t)*y0 + 2*(1-t)*t*y1 + t*t*y3;
        nZ = (1-t)*(1-t)*z0 + 2*(1-t)*t*z1 + t*t*z3;
        if (dist3(lX,lY,lZ, nX,nY,nZ) >= detail_threshold) {
            line_to(inputshape, nX, nY, nZ);
            lX = nX;
            lY = nY;
			lZ = nZ;
        }
        t = t + delta;
    }
	line_to(inputshape, x3, y3, z3);
}                    

static void parse_line(class inputshape *inputshape, char *line)
{
    char *c;

	char *chunk = line;
	int argcount = 0;

    double arg1 = 0.0, arg2 = 0.0, arg3 = 0.0, arg4 = 0.0, arg5 = 0.0, arg6 = 0.0, arg7 = 0.0, arg8 = 0.0, arg9 = 0.0;

	chr_replace(line, ',', ' ');
	chr_replace(line, '\n', 0);
    c = &chunk[0];
    arg1 = strtod(c, &c); argcount++;
    if (c && strlen(c) > 0) {
        arg2 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg3 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg4 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg5 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg6 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg7 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg8 = strtod(c, &c); argcount++;
	}
    if (c && strlen(c) > 0) {
        arg9 = strtod(c, &c); argcount++;
	}

    switch (argcount) {
        case 3:
			line_to(inputshape, arg1, arg2, arg3);
            break;
		case 4:
			sphere(inputshape, arg1, arg2, arg3, arg4);
			break;
        case 9:
            cubic_bezier(inputshape, last_X, last_Y, last_Z, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            break;
        case 6:
            quadratic_bezier(inputshape, last_X, last_Y, last_Z, arg1, arg2, arg3, arg4, arg5, arg6);
            break;
		case 0:
            first = true;
            break;
        default:
            printf("Unparsable line %s\n", line);
    }
    
}



void parse_csv_file(class scene *scene, const char *filename, int tool)
{
    size_t n;
    FILE *file;

	toolnr = tool;
	tooldepth = get_tool_maxdepth();

	printf("Using tool %i with max depth of cut %5.2fmm\n", toolnr, tooldepth);

	class inputshape *input;

	input = new(class inputshape);

	input->set_name("Manual path");

	scene->shapes.push_back(input);

	printf("Opening %s\n", filename);
    
    file = fopen(filename, "r");
    if (!file) {
        printf("Cannot open %s : %s\n", filename, strerror(errno));
        return;
    }
    
    while (!feof(file)) {
        int ret;
        char *line = NULL;
        ret = getline(&line, &n, file);
        if (ret >= 0) {
            parse_line(input, line);
            free(line);
        }
    }
    fclose(file);
}
