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
static double maxspeed = 0, minspeed = 500000;

static double speed = 0;

double cuttersize = 2;

static char toollist[8192] = "";

static bool first_coord = true;
static bool spindle_running = false;

static bool need_homing_switches = false;

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

	if (!spindle_running)
		error("Cutting without spindle on\n");

	if (speed > maxspeed)
		maxspeed = speed;
	if (speed < minspeed)
		minspeed = speed;

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
	if (fZ > maxZ)
		maxZ = fZ;
	if (fZ < minZ)
		minZ = fZ;
	if (tZ > maxZ)
		maxZ = tZ;
	if (tZ < minZ)
		minZ = tZ;

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
		if (spindle_running)
			error("Tool change with spindle running\n");
		vprintf("Tool change: %s\n", line + 3);
		strcat(toollist, line+3);
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

	vprintf("Parsing %s\n", filename);
	file = fopen(filename, "r");
	if (!file) {
		error("Error opening file: %s\n", strerror(errno));
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

void print_state(FILE *output)
{
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
}


static void verify_line(const char *key, const char *value)
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
		fgets(line, 4096, file);
		c1 = strchr(line, '\t');
		if (!c1)
			continue;
		*c1 = 0;
		c1++;
		verify_line(line, c1);
	}
	fclose(file);
}
