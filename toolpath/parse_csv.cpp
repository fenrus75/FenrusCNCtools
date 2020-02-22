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

static double pxratio = 25.4 / 96.0;
static inline double px_to_mm2(double px) { return pxratio * px ; };

/* we need to remember previous coordinates for bezier curves */
static double last_X, last_Y, last_Z;
static double start_X, start_Y, start_Z;
static bool first = true;
static double tooldepth = 0.1;
static double prio = 0;
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

	printf("LINE %5.4f %5.4f %5.4f -> %5.4f %5.4f %5.4f\n", X1, Y1, Z1, X2, Y2, Z2);

	if (first) {
		first = false;
		printf("FIRST\n");
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
		input->tooldepths[depth]->toollevels[0]->add_poly_vcarve(p2, Z1, Z2, prio);
		
		Z1 += tooldepth;
		Z2 += tooldepth;
	}
}


static void circle(class inputshape *input, double X, double Y, double Z, double R)
{
    double phi;
	first = true;
    phi = 0;
	prio = R;
    while (phi <= 360) {
        double P;
        P = phi / 360.0 * 2 * M_PI;
		if (first || dist(X + R * cos(P), Y + R * sin(P), last_X, last_Y) > 0.2 || phi >= 359)
			line_to(input, X + R * cos(P), Y + R * sin(P), Z );
        phi = phi + 1;
    }
	first = true;
	prio = 0;
}

static void sphere(class scene *scene, double X, double Y, double Z, double R)
{
	double z = - R;
	double so = get_tool_stepover(toolnr);
	double maxz;

	class inputshape *input;

	input = new(class inputshape);
	input->set_name("Sphere path");
	scene->shapes.push_back(input);

	maxz = so;
	if (maxz > tooldepth / 2)
		maxz = tooldepth / 2;
	
	while (z < -0.00001) {
		double r;
		double deltaZ, z2;

		r  = sqrt(R * R - z * z);

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

static bool parse_line(class scene *scene, class inputshape *inputshape, char *line)
{
    char *c;

	char *chunk = line;
	int argcount = 0;
	bool need_new_input = false;

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
			sphere(scene, arg1, arg2, arg3, arg4);
			need_new_input = true;
			break;
        case 9:
            cubic_bezier(inputshape, last_X, last_Y, last_Z, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            break;
        case 6:
            quadratic_bezier(inputshape, last_X, last_Y, last_Z, arg1, arg2, arg3, arg4, arg5, arg6);
            break;
		case 0:
            first = true;
			need_new_input = true;
            break;
        default:
            printf("Unparsable line %s with count %i\n", line, argcount);
    }
    return need_new_input;
}


static double svgheight = 0;

static inline double distance(double x0, double y0, double x1, double y1)
{
    return sqrt( (x1-x0) * (x1-x0) + (y1-y0) * (y1-y0));
}


/*
<circle cx="440.422" cy="312.878" r="12" stroke="black" stroke-width="1" fill="none" />
*/
static void parse_circle(class inputshape *input, char *line, double depth)
{
    char *cx, *cy, *r;
    double X,Y,R,phi;
    cx = strstr(line, "cx=\"");
    cy = strstr(line, "cy=\"");
    r = strstr(line, "r=\"");
    if (!cx || !cy || !r) {
        printf("Failed to parse circle: %s\n", line);
        return;
    }
    X = strtod(cx + 4, NULL);
    Y = strtod(cy + 4, NULL);
    R = strtod(r + 3, NULL);
    phi = 0;
	first = true;
    while (phi < 360) {
        double P;
        P = phi / 360.0 * 2 * M_PI;
		
        line_to(input, px_to_mm2(X + R * cos(P)), px_to_mm2(svgheight -Y + R * sin(P)), depth);
        phi = phi + 1;
    }
    last_X = X;
    last_Y = Y;
}

static double get_depth_ratio(char *c) 
{
	int a;
	printf("Depthratio -%s-\n", c);

	if (strlen(c) < 6)
		return 0;
	c[7] = 0;

	a = strtoull(c, NULL, 16);
	a = a & 255;
	a = 255 - a;
	printf("ST a = %i\n",a);
	return a / 255.0;		
}

static void push_chunk(class inputshape *input, char *chunk, char *line, double depth)
{
    char command;
    char *c;
	double Z;
	double adder  = 0;

    double arg1 = 0.0, arg2 = 0.0, arg3 = 0.0, arg4 = 0.0, arg5 = 0.0, arg6 = 0.0;
    command = chunk[0];
    c = &chunk[0];
    c++;
    arg1 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg2 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg3 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg4 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg5 = strtod(c, &c);
    if (c && strlen(c) > 0)
		arg6 = strtod(c, &c);

	Z = depth;
	printf("COMMAND %c last_Z %5.4f\n", command, last_Z);
    switch (command) {
#if 0
		case 'l':
            arg1 += last_X;
            arg2 += last_Y;
			/* fall through */
#endif
        case 'L':
//            printf("Linee         : %5.2f %5.2f\n", arg1, arg2);

			line_to(input, px_to_mm2(arg1), px_to_mm2(svgheight - arg2), Z);
            break;
        case 'M':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            last_X = px_to_mm2(arg1);
            last_Y = px_to_mm2(arg2);
			last_Z = depth;
			start_X = last_X;
			start_Y = last_Y;
			start_Z = last_Z;
			first = false;
            break;
        case 'm':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);

            last_X = px_to_mm2(arg1);
            last_Y = px_to_mm2(svgheight - arg2) + last_Y;
			last_Z = depth;
			start_X = last_X;
			start_Y = last_Y;
			start_Z = last_Z;
			first = false;
            break;
		case 'h':
            adder = last_X;
			/* fall through */
        case 'H':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
			line_to(input, px_to_mm2(arg1) + adder, last_Y, Z);
            break;
		case 'v':
			/* fall through */
			adder = last_Y;
        case 'V':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
			line_to(input, last_X, px_to_mm2(-arg1) + adder, Z);
            break;
#if 0
		case 'c':
			arg1 += last_X;
			arg2 += last_Y;
			arg3 += last_X;
			arg4 += last_Y;
			arg5 += last_X;
			arg6 += last_Y;
			/* fall through */
#endif
#if 0
        case 'C':
            cubic_bezier(input, last_X, -last_Y, arg1, -arg2, arg3, -arg4, arg5, -arg6);
            last_X = arg5;
            last_Y = arg6;
            break;
        case 'Q':
            quadratic_bezier(input, last_X, -last_Y, px_to_mm2(arg1), -px_to_mm2(arg2), px_to_mm2(arg3), -px_to_mm2(arg4));
            break;
#endif
        case 'Z':
        case 'z':
			line_to(input, start_X, start_Y, start_Z);
            first = true;
            break;
        default:
            printf("Unknown command in chunk: %s  (%s)\n", chunk, line);
    }
    
}

static const char *valid = "0123456789.eE- ";

static void strip_str(char *line)
{
    while (strlen(line) > 0 && line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = 0;
    while (strlen(line) > 0 && line[strlen(line)-1] == ' ')
            line[strlen(line)-1] = 0;
}

static double depthratio = 1.0;

static void parse_svg_line(class inputshape *input, char *line, double depth)
{
    char *c;
    char chunk[4095];
    double height;
    
    c = strstr(line, "height=\"");
    if (c && svgheight == 0) {
        height = strtod(c+8, NULL);
//        scene->declare_minY(px_to_mm2(-height));
        svgheight = height;
		printf("SETTING HEIGHT TO %5.4f\n", svgheight);
    }

	if (strstr(line, "inkscape:document-units=\"mm\"") != NULL) {
		pxratio = 1.0;
		printf("Switching to mm\n");
	}

	c = strstr(line, "stroke:#");
	if (c) {
		printf("STROKE\n");
		depthratio = get_depth_ratio(c + 8);
	}
		
    /*
    c = strstr(line, "width=\"");
    if (c) {
        width = strtod(c+7, NULL);
        scene->declare_maxX(px_to_mm2(width));
        printf("MAX X %5.2f\n", width);
    }
    */
    
    c = strstr(line,"<circle cx=");
    if (c) {
        parse_circle(input, line, depth * depthratio);
    }
    c = strstr(line, " d=\"");
    if (c == NULL)
        return;
    c += 4;
    chr_replace(c, '"', 0);
    chr_replace(line, ',', ' ');
//    printf("Line is %s\n", c);
    chunk[0] = 0;
    memset(chunk, 0, sizeof(chunk));
    while (*c) {
        if (strchr(valid, *c)) {
            chunk[strlen(chunk)] = *c;            
        } else {
            strip_str(chunk);
            if (strlen(chunk) > 0)
                push_chunk(input, chunk, line, depth * depthratio);
            memset(chunk, 0, sizeof(chunk));
            chunk[strlen(chunk)] = *c;           
        }
        c++;
    }
    strip_str(chunk);
    if (strlen(chunk) > 0)
        push_chunk(input, chunk, line, depth * depthratio);
}

static void parse_direct_svg(class scene *scene, const char *filename, int tool)
{
	class inputshape *input;

	input = new(class inputshape);
	input->set_name("Manual path");
	scene->shapes.push_back(input);

    FILE *file;
    
    file = fopen(filename, "r");
    if (!file) {
        printf("Cannot open %s : %s\n", filename, strerror(errno));
        return;
    }

	last_Z = -scene->get_depth();    
    while (!feof(file)) {
        char * ret;
        char line[40960];
        ret = fgets(&line[0], sizeof(line), file);
        if (ret) {
            parse_svg_line(input, line, -scene->get_depth());
        }
    }
    fclose(file);

	
}

void parse_csv_file(class scene *scene, const char *filename, int tool)
{
    FILE *file;

	toolnr = tool;
	tooldepth = get_tool_maxdepth();

	qprintf("Using tool %i with max depth of cut %5.2fmm\n", toolnr, tooldepth);

	if (strstr(filename, ".svg") != NULL) {
		parse_direct_svg(scene, filename, tool);
		return;
	}

	class inputshape *input;

	input = new(class inputshape);
	input->set_name("Manual path");
	scene->shapes.push_back(input);

	qprintf("Opening %s\n", filename);
    
    file = fopen(filename, "r");
    if (!file) {
        printf("Cannot open %s : %s\n", filename, strerror(errno));
        return;
    }
    
    while (!feof(file)) {
        char * ret;
        char line[40960];
	ret = fgets(line, sizeof(line), file);
        if (ret) {
			if (parse_line(scene, input, line)) {
				input = new(class inputshape);
				input->set_name("Manual path");
				scene->shapes.push_back(input);
			}
        }
    }
    fclose(file);
}
