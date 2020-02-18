/*
 * (C) Copyright 2020  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <vector>

#include "gcodecheck.h"


static std::vector<struct line *> lines;
static int metric = 1;
static int absolute = 1;

static char gcommand=' ';

static double currentX;
static double currentY;
static double currentZ;

static double minX = 1000, minY = 1000, minZ = 1000, maxX, maxY, maxZ;
static double maxspeed = 0, minspeed = 500000;

static double speed = 0;

double cuttersize = 2;

static char toollist[8192] = "";

static bool first_coord = true;
static bool spindle_running = false;

static bool need_homing_switches = false;

static int toolnr = 0;
static double diameter;
static double angle;
static double speedlimit, plungelimit;

static double to_mm(double x)
{
	if (metric)
		return x;
	else
		return x * 25.4;
}

static double radius_at_depth(double Z)
{
	double radius = diameter / 2;
	if (angle == 0)
		return diameter / 2;
	return fmin(depth_to_radius(Z, angle), radius);	
}

static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}
static double dist3(double X0, double Y0, double Z0, double X1, double Y1, double Z1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0) + (Z1-Z0)*(Z1-Z0));
}


static void speed_check(double X1, double Y1, double Z1, double X2, double Y2, double Z2, int line)
{
	static int warned = 0;
	double d = dist3(X1,Y1,X1, X2,Y2,Z2);
	double t,w,h;
	if (speed == 0 || d == 0 || warned > 0)
		return;

	if (nospeedcheck)
		return;
	t = d / speed;	
	w = dist(X1,Y1,X2,Y2);
	h = fabs(Z1-Z2);

	if (w/t > speedlimit) {
		error("Gcode line %i: Feed limit exceed %5.4f > %5.4f mmpmin (%5.4f > %5.4f ipm) with tool %i\n",
			line, w/t , speedlimit, mm_to_inch(w/t) , mm_to_inch(speedlimit), toolnr);
		warned++;
	}
	if (h/t > plungelimit) {
		error("Gcode line %i: Plunge limit exceed %5.4f > %5.4f mmpmin (%5.4f > %5.4f ipm) with tool %i\n",
			line, h/t , plungelimit, mm_to_inch(h/t) , mm_to_inch(plungelimit), toolnr);
		warned++;
	}
	
}

static void record_motion_XYZ(double fX, double fY, double fZ, double tX, double tY, double tZ, int line)
{
	struct line *point;

	if (gcommand == 0 && fZ >= 0 && tZ >=0) {
		first_coord = true;
		return;
	}

	if (!spindle_running)
		error("Cutting without spindle on\n");

	if (gcommand == 1)
		speed_check(fX,fY,fZ, tX,tY,tZ, line);

	if (speed > maxspeed)
		maxspeed = speed;
	if (speed < minspeed)
		minspeed = speed;

	maxX = fmax(maxX, fX + radius_at_depth(fZ));
	minX = fmin(minX, fX - radius_at_depth(fZ));
	maxX = fmax(maxX, tX + radius_at_depth(tZ));
	minX = fmin(minX, tX - radius_at_depth(tZ));
	maxY = fmax(maxY, fY + radius_at_depth(fZ));
	minY = fmin(minY, fY - radius_at_depth(fZ));
	maxY = fmax(maxY, tY + radius_at_depth(tZ));
	minY = fmin(minY, tY - radius_at_depth(tZ));
	maxZ = fmax(maxZ, fZ);
	minZ = fmin(minZ, fZ);
	maxZ = fmax(maxZ, tZ);
	minZ = fmin(minZ, tZ);

	point = (struct line*)calloc(sizeof(struct line), 1);
	point->X1 = fX;
	point->X2 = tX;
	point->Y1 = fY;
	point->Y2 = tY;
	point->Z1 = fZ;
	point->Z2 = tZ;

	point->minX = fmin(fX - radius_at_depth(fZ), tX - radius_at_depth(tZ));
	point->maxX = fmax(fX + radius_at_depth(fZ), tX + radius_at_depth(tZ));
	point->minY = fmin(fY - radius_at_depth(fZ), tY - radius_at_depth(tZ));
	point->maxY = fmax(fY + radius_at_depth(fZ), tY + radius_at_depth(tZ));

	point->tool = toolnr;
	point->toolradius = diameter / 2;
	point->toolangle = angle;

	lines.push_back(point);
//	printf("XYZ movement from %5.2f,%5.2f to %5.2f,%5.2f\n", currentX, currentY, X, Y);
}

static int xyzline(char *line, int nr)
{
	char *c;
	double X,Y,Z;

	while (line[0] == ' ')
		line++;

	if (absolute == 1) {
		X = currentX;
		Y = currentY;
		Z = currentZ;
	} else {
		X = 0.0;
		Y = 0.0;
		Z = 0.0;
	}
	if (strlen(line) == 0)
		return 1;
	c = line;
	
	while (strlen(c) > 0) {
		char *c2;

		if (c[0] == 'X') {
			X = to_mm(strtod(c+1, &c2));
			c = c2;
		} else if (c[0] == 'Y') {
			Y = to_mm(strtod(c+1, &c2));
			c = c2;
		} else if (c[0] == 'Z') {
			Z = to_mm(strtod(c+1, &c2));
			c = c2;
		} else if (c[0] == 'F') {
			speed = strtod(c+1, &c2);
			c = c2;
		} else {
			printf("Unknown XYZ command: %s\n", line);
			*c = 0;
		}
	}

	if (absolute == 0) {
		X += currentX;
		Y += currentY;
		Z += currentZ;
	}

	if (!first_coord)
		record_motion_XYZ(currentX, currentY, currentZ, X, Y, Z, nr);

	currentX = X;
	currentY = Y;
	currentZ = Z;
	first_coord = false;
	return 1;
}


static int gline(char *line, int nr)
{
	int code;
	int handled = 0;
	code = strtoull(&line[1], NULL, 10);

	if (code == 0 && line[1] == '0') {
		gcommand = 0;
		handled = 1;
		xyzline(line + 2, nr);
	}
	if (code == 1 && line[1] == '1') {
		gcommand = 1;
		handled = 1;
		xyzline(line + 2, nr);
	}
	
	if (code == 20) {
		vprintf("Switching to imperial\n");
		metric = 0;
		handled = 1;
	}
	if (code == 21) {
		vprintf("Switching to metric\n");
		metric = 1;
		handled = 1;
	}
	if (code == 90) {
		vprintf("Switching to absolute mode\n");
		absolute = 1;
		handled = 1;
	}
	if (code == 91) {
		vprintf("Switching to relative mode\n");
		absolute = 0;
		handled = 1;
	}

	if (code == 53) { /* G53 is "absolute move once line" which is for bit changes/etc */
		handled = 1;
		first_coord = true;
		need_homing_switches = true;
	}


	if (handled == 0) {
		printf("Unhandled G code: %s\n", line);
	}
	return handled;
}


static int mline(char *line)
{
	int code;
	int handled = 0;
	code = strtoull(&line[1], NULL, 10);

	if (code == 2) {
		vprintf("Program end\n");
		handled = 1;
	}
	if (code == 3) {
		vprintf("Spindle Start\n");
		spindle_running = true;
		handled = 1;
	}
	if (code == 4) {
		vprintf("Spindle Start\n");
		spindle_running = true;
		handled = 1;
	}
	if (code == 5) {
		vprintf("Spindle Stop\n");
		spindle_running = false;
		handled = 1;
	}
	if (code == 6) {
		char *c;
		int t;
		if (spindle_running)
			error("Tool change with spindle running\n");
		vprintf("Tool change: %s\n", line + 3);
		strcat(toollist, line+3);
		c = line + 3;
		if (*c == 'T') c++;
		t = strtoull(c, NULL, 10);
		activate_tool(t);
		handled = 1;
	}
	if (code == 30) {
		vprintf("Program end\n");
		handled = 1;
	}

	
	if (handled == 0) {
		printf("Unhandled M code: %s\n", line);
	}
	return handled;
}

static void parse_line(char *line, int nr)
{
	int handled = 0;
	char *c;
	c = strchr(line, '\n');
	if (c) *c = 0;

	if (line[0] == '%')
		return;
	if (line[0] == '(')
		return;

	if (line[0] == 'G') {
		handled += gline(line, nr);
	}
	if (line[0] == 'M') {
		handled += mline(line);
	}
	if (line[0] == 'X') {
		handled += xyzline(line, nr);
	}
	if (line[0] == 'Y') {
		handled += xyzline(line, nr);
	}
	if (line[0] == 'Z') {
		handled += xyzline(line, nr);
	}
	if (line[0] == 'F') {
		handled += xyzline(line, nr);
	}


	if (strlen(line) == 0 )
		handled = 1;
	if (handled == 0)
		printf("Line is %s \n", line);
}

void read_gcode(const char *filename)
{
	FILE *file;
	int linenr = 0;

	vprintf("Parsing %s\n", filename);
	file = fopen(filename, "r");
	if (!file) {
		error("Error opening file: %s\n", strerror(errno));
	}
	while (!feof(file)) {
		char line[8192];
		line[0] = 0;
		if (!fgets(line, 8192, file))
			break;
		linenr++;
		if (line[0] != 0)
			parse_line(line, linenr);
	}
	fclose(file);
}

double depth_at_XY(double X, double Y)
{
	double depth = maxZ;
	unsigned int i;
	
	for (i = 0; i < lines.size(); i++) {
		double d;
		double l;
		double baseZ;
		double adjust = 0;
		if (X < lines[i]->minX)
			continue;
		if (X > lines[i]->maxX)
			continue;
		if (Y < lines[i]->minY)
			continue;
		if (Y > lines[i]->maxY)
			continue;

		d = distance_point_from_vector(lines[i]->X1, lines[i]->Y1, lines[i]->X2, lines[i]->Y2, X, Y, &l);	

		if (d  > lines[i]->toolradius)
			continue;

		baseZ = lines[i]->Z1 + l * (lines[i]->Z2 - lines[i]->Z1);

		if (lines[i]->toolangle > 0.01) {
			adjust -= radius_to_depth(d, lines[i]->toolangle);

		}

		vprintf("XY %5.4f %5.4f    line %5.4f,%5.4f -> %5.4f,%5.4f tool %i at dist %5.4f   est Z %5.4f + %5.4f = %5.4f\n",
			X, Y, lines[i]->X1, lines[i]->Y1, lines[i]->X2, lines[i]->Y2, lines[i]->tool, d, baseZ, adjust, baseZ + adjust);

		baseZ += adjust;
		depth = fmin(depth, baseZ);
	}

	if (depth > 0)
		depth = 0;

	return depth;
}

void print_state(FILE *output)
{
	double X, Y;
	fprintf(output, "minX\t%5.4f\n", minX);
	fprintf(output, "maxX\t%5.4f\n", maxX);
	fprintf(output, "minY\t%5.4f\n", minY);
	fprintf(output, "maxY\t%5.4f\n", maxY);
	fprintf(output, "minZ\t%5.4f\n", minZ);
	fprintf(output, "maxZ\t%5.4f\n", maxZ);
	fprintf(output, "tools\t%s\n", toollist);
	fprintf(output, "minspeed\t%5.4f\n", minspeed);
	fprintf(output, "maxspeed\t%5.4f\n", maxspeed);
	if (need_homing_switches)
		fprintf(output, "homing\tyes\n");
	else
		fprintf(output, "homing\tno\n");

	Y = minY;
	while (Y <= maxY) {
		X = minX;
		while (X <= maxX) {

			fprintf(output, "point\t%5.4f\t%5.4f\t%5.4f\n", X, Y, depth_at_XY(X, Y));

			X += 1.0;
		}
		Y += 1.0;
	}
}


static void verify_line(const char *key, char *value)
{
	double valD = strtod(value, NULL);


	if (strcmp(key, "minX") == 0) {
		if (fabs(valD-minX) > 0.01)
			error("min X deviates from reference  %5.4f vs %5.4f\n", valD, minX);
		return;
	}
	if (strcmp(key, "maxX") == 0) {
		if (fabs(valD-maxX) > 0.01)
			error("max X deviates from reference  %5.4f vs %5.4f\n", valD, maxX);
		return;
	}
	if (strcmp(key, "minY") == 0) {
		if (fabs(valD-minY) > 0.01)
			error("min Y deviates from reference  %5.4f vs %5.4f\n", valD, minY);
		return;
	}
	if (strcmp(key, "maxY") == 0) {
		if (fabs(valD-maxY) > 0.01)
			error("max Y deviates from reference  %5.4f vs %5.4f\n", valD, maxY);
		return;
	}
	if (strcmp(key, "minZ") == 0) {
		if (fabs(valD-minZ) > 0.01)
			error("min Z deviates from reference  %5.4f vs %5.4f\n", valD, minZ);
		return;
	}
	if (strcmp(key, "maxZ") == 0) {
		if (fabs(valD-maxZ) > 0.01)
			error("max Z deviates from reference  %5.4f vs %5.4f\n", valD, maxZ);
		return;
	}
	if (strcmp(key, "minspeed") == 0) {
		if (valD < 0.9 * minspeed)
			error("Minimum speed deviates from reference  %5.4f vs %5.4f\n", valD, minspeed);
		return;
	}
	if (strcmp(key, "maxspeed") == 0) {
		if (valD > 1.01 * maxspeed)
			error("Maximum speed deviates from reference  %5.4f vs %5.4f\n", valD, maxspeed);
		return;
	}
	if (strcmp(key, "tools") == 0) {
		if (strcmp(value, toollist) != 0)
			error("Tool list deviates from reference  %s vs %s\n", value, toollist);
		return;
	}
	if (strcmp(key, "homing") == 0) {
		bool home = false;
		if (strcmp(value, "yes") == 0)
			home = true;
		if (home != need_homing_switches)
			error("Homing switches setting deviates from reference\n");
		return;
	}
	if (strcmp(key, "point") == 0) {
		char *c;
		double X,Y,Z, Z2;
		c = value;
		X = strtod(c, &c);
		Y = strtod(c, &c);
		Z = strtod(c, &c);
		Z2 = depth_at_XY(X, Y);
		if (fabs(Z - Z2) > 0.01) {
			error("Content failure, expected a depth of %5.4f at %5.4f,%5.4f but got %5.4f\n",
				Z, X, Y, Z2);
		}
		return;
	}


	printf("Unhandled key %s \n", key);
}

void verify_fingerprint(const char *filename)
{
	FILE *file;
	file = fopen(filename, "r");
	if (!file) {
		error("Error opening reference file %s", filename);
		return;
	}

	while (!feof(file)) {
		char line[4096];
		char *c1;
		line[0] = 0;
		if (!fgets(line, 4096, file))
			break;
		c1 = strchr(line, '\n');
		if (c1) *c1 = 0;
		c1 = strchr(line, '\t');
		if (!c1)
			continue;
		*c1 = 0;
		c1++;
		verify_line(line, c1);
	}

	fclose(file);
}

#define unused(x)  do { if (x != x) exit(0); } while (0) 
void set_tool_imperial(const char *name, int nr, double diameter_inch, double stepover_inch, double maxdepth_inch, double feedrate_ipm, double plungerate_ipm)
{
	unused(stepover_inch);
	unused(name);
	unused(maxdepth_inch);

	vprintf("Switching to tool %i\n", nr);
	toolnr = nr;
	diameter = inch_to_mm(diameter_inch);
	speedlimit = ipm_to_metric(feedrate_ipm);
	plungelimit = ipm_to_metric(plungerate_ipm);
	angle = get_tool_angle(toolnr);

}
