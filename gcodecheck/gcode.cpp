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

struct line {
	double X1, Y1, Z1;
	double X2, Y2, Z2;
};

struct point {
	double X, Y, Z;
};


static std::vector<struct line *> lines;
static int metric = 1;
static int absolute = 1;

static char gcommand=' ';

static double currentX;
static double currentY;
static double currentZ;

static double minX = 1000, minY = 1000, minZ = 1000, maxX, maxY, maxZ;


static double speed = 0;

double cuttersize = 2;

static bool first_coord = true;

static double to_mm(double x)
{
	if (metric)
		return x;
	else
		return x * 25.4;
}

static void record_motion_XYZ(double fX, double fY, double fZ, double tX, double tY, double tZ)
{
	struct line *point;

	if (gcommand == 0 && fZ >= 0 && tZ >=0) {
		first_coord = true;
		return;
	}

	if (fX > maxX)
		maxX = fX;
	if (fX < minX)
		minX = fX;
	if (tX > maxX)
		maxX = tX;
	if (tX < minX)
		minX = tX;
	if (fY > maxY)
		maxY = fY;
	if (fY < minY)
		minY = fY;
	if (tY > maxY)
		maxY = tY;
	if (tY < minY)
		minY = tY;

	point = (struct line*)calloc(sizeof(struct line), 1);
	point->X1 = fX;
	point->X2 = tX;
	point->Y1 = fY;
	point->Y2 = tY;
	point->Z1 = fZ;
	point->Z2 = tZ;

	lines.push_back(point);
//	printf("XYZ movement from %5.2f,%5.2f to %5.2f,%5.2f\n", currentX, currentY, X, Y);
}

static int xyzline(char *line)
{
	char *c;
	double X,Y,Z;

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
		record_motion_XYZ(currentX, currentY, currentZ, X, Y, Z);

	currentX = X;
	currentY = Y;
	currentZ = Z;
	first_coord = false;
	return 1;
}


static int gline(char *line)
{
	int code;
	int handled = 0;
	code = strtoull(&line[1], NULL, 10);

	if (code == 0 && line[1] == '0') {
		gcommand = 0;
		handled = 1;
		xyzline(line + 2);
	}
	if (code == 1 && line[1] == '1') {
		gcommand = 1;
		handled = 1;
		xyzline(line + 2);
	}
	
	if (code == 20) {
		printf("Switching to imperial\n");
		metric = 0;
		handled = 1;
	}
	if (code == 21) {
		printf("Switching to metric\n");
		metric = 1;
		handled = 1;
	}
	if (code == 90) {
		printf("Switching to absolute mode\n");
		absolute = 1;
		handled = 1;
	}
	if (code == 91) {
		printf("Switching to relative mode\n");
		absolute = 0;
		handled = 1;
	}

	if (code == 53) { /* G53 is "absolute move once line" which is for bit changes/etc */
		handled = 1;
		first_coord = true;
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
		printf("Program end\n");
		handled = 1;
	}
	if (code == 3) {
		printf("Spindle Start\n");
		handled = 1;
	}
	if (code == 4) {
		printf("Spindle Start\n");
		handled = 1;
	}
	if (code == 5) {
		printf("Spindle Stop\n");
		handled = 1;
	}
	if (code == 6) {
		printf("Tool change: %s\n", line + 3);
		handled = 1;
	}
	if (code == 30) {
		printf("Program end\n");
		handled = 1;
	}

	
	if (handled == 0) {
		printf("Unhandled M code: %s\n", line);
	}
	return handled;
}

static void parse_line(char *line)
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
		handled += gline(line);
	}
	if (line[0] == 'M') {
		handled += mline(line);
	}
	if (line[0] == 'X') {
		handled += xyzline(line);
	}
	if (line[0] == 'Y') {
		handled += xyzline(line);
	}
	if (line[0] == 'Z') {
		handled += xyzline(line);
	}
	if (line[0] == 'F') {
		handled += xyzline(line);
	}


	if (strlen(line) == 0 )
		handled = 1;
	if (handled == 0)
		printf("Line is %s \n", line);
}

void read_gcode(const char *filename)
{
	char *line;
	FILE *file;

	printf("Parsing %s\n", filename);
	file = fopen(filename, "r");
	if (!file) {
		printf("Error opening file: %s\n", strerror(errno));
	}
	while (!feof(file)) {
		size_t ret;
		line = NULL;
		if (getline(&line, &ret, file) < 0)
			break;
		if (ret > 0 && line)
			parse_line(line);
		free(line);
	}
	fclose(file);
}
