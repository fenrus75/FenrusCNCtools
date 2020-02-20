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
#include <string.h>
#include <errno.h>
#include <math.h>
#include <vector>

extern "C" {
#include "toolpath.h"
}

#include "gcode.h"

static bool want_adaptive = false;

static std::vector<struct line *> lines;
static const char *tool_name = "T201";
static int current_tool_nr = -499;
static double tool_diameter = 6;
static double tool_stepover = 3;
static double tool_maxdepth = 1;
static double tool_feedrate = 0;
static double tool_plungerate = 0;
static double tool_angle = 0;
static int tool_nr = 0;
static double rippem = 10000;
static double safe_retract_height = 2;
static int want_separate;

static char stored_filename[8192];
static FILE *gcode;
static int retract_count;
static int mill_count;
/* in mm */
static double cX, cY, cZ, cS;
static int has_current = 0;
static int am_roughing = 0;

/* for vcarving */

static double prevX1, prevY1, prevX2, prevY2;
static int prev_valid;

static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}
static double dist3(double X0, double Y0, double Z0, double X1, double Y1, double Z1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0) + (Z1-Z0)*(Z1-Z0));
}

static double radius_at_depth(double Z)
{
	double radius = tool_diameter / 2;
	if (tool_angle == 0)
		return tool_diameter / 2;
	return fmin(depth_to_radius(Z, tool_angle), radius);	
}

static void record_motion_XYZ(double fX, double fY, double fZ, double tX, double tY, double tZ)
{
	struct line *point;

	if (!want_adaptive)
		return;

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

	point->tool = current_tool_nr;
	point->toolradius = tool_diameter / 2;
	point->toolangle = tool_angle;

	lines.push_back(point);
//	printf("XYZ movement from %5.2f,%5.2f to %5.2f,%5.2f\n", currentX, currentY, X, Y);
}

static char *double_to_str(double X)
{
	static char buffer[128];
	char buf[96];
	int x;
	int i;
	x = (int)(X * 10000);

	if (X < 0)
		sprintf(buf, "%06i", x);
	else
		sprintf(buf, "%05i", x);

	strcpy(buffer, buf);
	i = strlen(buf) - 4;
	buffer[i] = '.';
	strcpy(buffer + i + 1, buf + i);
	//printf("Double %5.4f to string %s \n", X, buffer);
	return buffer;
}

void set_tool_imperial(const char *name, int nr, double diameter_inch, double stepover_inch, double maxdepth_inch, double feedrate_ipm, double plungerate_ipm)
{
    tool_name = strdup(name);
    tool_diameter = inch_to_mm(diameter_inch);
    tool_stepover = inch_to_mm(stepover_inch);
    tool_maxdepth = inch_to_mm(maxdepth_inch);
    tool_feedrate = ipm_to_metric(feedrate_ipm);
    tool_plungerate = ipm_to_metric(plungerate_ipm);
	tool_angle = get_tool_angle(nr);
	tool_nr = nr;
    prev_valid = 0;
	has_current = 0;
}

void set_tool_metric(const char *name, int nr, double diameter_mm, double stepover_mm, double maxdepth_mm, double feedrate_metric, double plungerate_metric)
{
    tool_name = strdup(name);
    tool_diameter = diameter_mm;
    tool_stepover = stepover_mm;
    tool_maxdepth = maxdepth_mm;
    tool_feedrate = feedrate_metric;
    tool_plungerate = plungerate_metric;
	tool_angle = get_tool_angle(nr);
	tool_nr = nr;
    prev_valid = 0;
	has_current = 0;
}

double get_tool_diameter(void)
{
    return tool_diameter;
}

double get_tool_stepover_gcode(void)
{
    return tool_stepover;
}

double get_tool_maxdepth(void)
{
    return tool_maxdepth;
}

void set_rippem(double _rippem)
{
    rippem = _rippem;
}

void set_retract_height_imperial(double _rh_inch)
{
    safe_retract_height = inch_to_mm(_rh_inch);
}
void set_retract_height_metric(double _rh_mm)
{
    safe_retract_height = _rh_mm;
}

double get_retract_height_metric(void)
{
    return safe_retract_height;
}

static int iter = 0;
void write_gcode_header(const char *filename)
{
	char actual_filename[8392];
	if (strlen(stored_filename) == 0) {
		char *c;
		strncpy(stored_filename, filename,8000);
		c = strstr(stored_filename, ".nc");
		if (c) *c = 0;	
	}
	strcpy(actual_filename, filename);

	if (want_separate) {
		sprintf(actual_filename, "%s-%i-%i.nc", stored_filename, ++iter, tool_nr);
	}

	

    gcode = fopen(actual_filename, "w");
    if (!gcode) {
        printf("Cannot open %s for gcode output: %s\n", filename, strerror(errno));
        return;
    }
    fprintf(gcode, "%%\n");
    fprintf(gcode, "G21\n"); /* milimeters not imperials */
    fprintf(gcode, "G90\n"); /* all relative to work piece zero */
    fprintf(gcode, "G0X0Y0Z%5.4f\n", safe_retract_height);
    cZ = safe_retract_height;
    fprintf(gcode, "(FILENAME: %s)\n", filename);
}

void gcode_plunge_to(double Z, double speedratio)
{
    fprintf(gcode, "G1");
    if (cZ != Z)
        fprintf(gcode,"Z%5.4f", Z);
    if (cS != speedratio)
        fprintf(gcode, "F%i", (int)(speedratio * tool_plungerate) );
    cZ = Z;
    cS = speedratio * tool_plungerate;
    prev_valid = 0;
	has_current = 1;
    fprintf(gcode, "\n");
}

void gcode_retract(void)
{
//    printf("retract\n");
    fprintf(gcode, "G0");
    if (cZ != safe_retract_height)
        fprintf(gcode,"Z%s", double_to_str(safe_retract_height));
    cZ = safe_retract_height;
    fprintf(gcode, "\n");
    retract_count++;
    prev_valid = 0;
	has_current = 1;
}

static double gcode_depth_at_XY(double X, double Y)
{
	double depth = 2;
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

		d = distance_point_from_vector_ll(lines[i]->X1, lines[i]->Y1, lines[i]->X2, lines[i]->Y2, X, Y, &l);	

		if (d  > lines[i]->toolradius)
			continue;

		baseZ = lines[i]->Z1 + l * (lines[i]->Z2 - lines[i]->Z1);

		if (lines[i]->toolangle > 0.01) {
			adjust -= radius_to_depth(d, lines[i]->toolangle);

		}

//		vprintf("XY %5.4f %5.4f    line %5.4f,%5.4f -> %5.4f,%5.4f tool %i at dist %5.4f   est Z %5.4f + %5.4f = %5.4f\n",
//			X, Y, lines[i]->X1, lines[i]->Y1, lines[i]->X2, lines[i]->Y2, lines[i]->tool, d, baseZ, adjust, baseZ + adjust);

		baseZ += adjust;
		depth = fmin(depth, baseZ);
	}

	if (depth > 0)
		depth = 0;

	return depth;
}
static double gcode_point_load(double X, double Y, double Z)
{
	double Z2;
	double delta;

	Z2 = gcode_depth_at_XY(X, Y);

	if (Z2 <= Z)
		return 0;
	delta = fabs(Z2 - Z);
	return delta / tool_maxdepth;
}

static double gcode_area_load(double X1, double Y1, double Z1, double X2, double Y2, double Z2, double *lout)
{
	double vx, vy, vz;
	double nx,ny;
	double len;
	double l = 0, prevl = 0;
	double maxl = dist3(X1,Y1,Z1,X2,Y2,Z2);
	double sum = 0;
	double count = 0;

	double stepsize = tool_stepover / 2;

	if (dist(X1,Y1,X2,Y2)/10 < stepsize)
		stepsize = dist(X1, Y1, X2, Y2) / 10;

	if (stepsize < 0.1)
		stepsize = 0.1;

	#define SAMPLES 10
	double sampleY[SAMPLES] = {0.995, 0.7778, 0.5556, 0.3333, 0.1111, -0.1111, -0.3333, -0.5556, -0.7778, -0.995};
	double sampleX[SAMPLES] = {0.09991, 0.629, 0.832, 0.943, 0.994, 0.994, 0.943, 0.832, 0.629, 0.099911};
	double R = tool_diameter / 2;

	*lout = 10000;

	if (approx4(X1,X2) && approx4(Y1,Y2)) {
		return 1;
	};

	vx = X2-X1;
	vy = Y2-Y1;
	vz = Z2-Z1;
	len=sqrt(vx*vx+vy*vy+vz*vz);
	vx = vx / len;
	vy = vy / len;
	vz = vz / len;

	nx = -vy;
	ny = vx;
	len=sqrt(nx*nx+ny*ny);
	nx = nx / len;
	ny = ny / len;

	do {
		int i;
		double local = 0;
		for (i = 0; i < SAMPLES; i++) {
			local += gcode_point_load(X1 + (l + R * sampleX[i]) * vx + R * sampleY[i] * nx, Y1 + (l + R * sampleX[i]) * vy + R * sampleY[i] * ny, Z1  + l * vz);
		}

		if (lout && count > 0 && prevl >= 0.01 && l != maxl) {
			double avg1, avg2;
			avg1 = sum / count;
			avg2 = local / SAMPLES;

//			printf("Avg1 %5.4f   avg2  %5.4f  l %5.4f  prevl %5.4f   X1 %5.4f  Y1 %5.4f  X2 %5.4f  Y2 %5.4f\n", avg1, avg2, l, prevl, X1, Y1, X2, Y2);

			if (fabs(avg1-avg2) > 0.1) {
				/* we're more than points% off... lets stop right here */
				*lout = prevl;
				return avg1;
			}
		}
		
		sum += local;
		count += SAMPLES;


		prevl = l;
		if (l < maxl - stepsize || l == maxl) {
		    l = l + stepsize;
		} else { 
			if (l != maxl)
				l = maxl;
			else
				l = l + stepsize;
		}
	} while (l <= maxl);

	if (count > 0)
		sum = sum / count;

	if (lout)
		*lout = 10000;

	return sum;
	
}


void gcode_mill_to(double X, double Y, double Z, double speedratio)
{
	char comment[4095];
    if (cZ != Z) {
        gcode_plunge_to(Z, speedratio);
	}

	/* slow start and stop for long distances */
	if (speedratio == 1.0 && dist(cX,cY,X,Y) >= 3 * tool_diameter && !want_adaptive) {
		double vX,vY, len;
		vX = X - cX;
		vY = Y - cY;
		len = sqrt(vX*vX + vY * vY);
		vX /= len;
		vY /= len;

		gcode_mill_to(cX + vX * tool_diameter/2, cY + vY * tool_diameter/2, Z, 0.66);
		gcode_mill_to(X - vX * tool_diameter/2, Y - vY * tool_diameter/2, Z, 1.01);
		gcode_mill_to(X, Y, Z, 0.66);
		return;
	} 

	if (want_adaptive) {
		double vX,vY, len;
		vX = X - cX;
		vY = Y - cY;
		len = sqrt(vX*vX + vY * vY);
		vX /= len;
		vY /= len;

		double load = gcode_area_load(cX, cY, cZ, X, Y, Z, &len);

		if (len != 10000) {
			gcode_mill_to(cX + len * vX, cY + len * vY, Z, speedratio);
			gcode_mill_to(X, Y, Z, speedratio);
			return;
		}
		
		if (load > 0) {
			speedratio = 1/(load * 2);
			if (speedratio < 0.5)
				speedratio = 0.5;
			if (speedratio > 1.2)
				speedratio = 1.2;
		}
		if (verbose) {
			sprintf(comment, "Area load for next move is %5.4f with speed ratio %5.4f", load, speedratio);
			gcode_write_comment(comment);
		}
	}
	

	/* slow down for round corners */
	if (dist(cX,cY,X,Y) < 0.3 * tool_diameter && speedratio > 0.66 && !want_adaptive)
		speedratio = 0.66;

    fprintf(gcode, "G1");
    if (cX != X)
        fprintf(gcode,"X%s", double_to_str(X));
    if (cY != Y)
        fprintf(gcode,"Y%s", double_to_str(Y));
    if (cZ != Z)
		fprintf(gcode,"Z%s", double_to_str(Z));
	if (cS != speedratio * tool_feedrate)
		fprintf(gcode, "F%i", (int)(speedratio * tool_feedrate));

	record_motion_XYZ(cX,cY,cZ, X,Y,Z);
    cX = X;
    cY = Y;
    cZ = Z;
	has_current = 1;
    cS = speedratio * tool_feedrate;
    fprintf(gcode, "\n");
    mill_count++;
    prev_valid = 0;
}

void gcode_vmill_to(double X, double Y, double Z, double speedratio)
{
	double d;
	double toolspeed;
	char command = '1';

	/* if all we do is straight go up, we can use G0 instead of G1 for speed */
	if (approx4(cX, X) && approx4(cY, Y) && (Z > cZ))
		command = '0';


//    printf("VMill to %5.4f %5.4f %5.4f\n", X, Y, Z);

	/* slow start and stop for long distances */
	if (speedratio == 1.0 && dist(cX,cY,X,Y) >= 1.5 * tool_diameter && approx4(cZ,Z)) {
		double vX,vY, len;
		vX = X - cX;
		vY = Y - cY;
		len = sqrt(vX*vX + vY * vY);
		vX /= len;
		vY /= len;

		gcode_vmill_to(cX + vX * tool_diameter/2, cY + vY * tool_diameter/2, Z, 0.66);
		gcode_vmill_to(X - vX * tool_diameter/2, Y - vY * tool_diameter/2, Z, 1.01);
		gcode_vmill_to(X, Y, Z, 0.66);
		return;
	} 

    fprintf(gcode, "G%c", command);
    prevX1 = cX;
    prevY1 = cY;
    prevX2 = X;
    prevY2 = Y;

	d = dist3(cX,cY,cZ,X,Y,Z);

	toolspeed = dist(cX,cY,X,Y) / d * tool_feedrate + fabs(cZ-Z) / d * tool_plungerate;

	/* slow down for round corners */
	if (dist(cX,cY,X,Y) < 0.5 * tool_diameter && speedratio > 0.7 && !am_roughing)
		speedratio = 0.66;
	record_motion_XYZ(cX,cY,cZ, X,Y,Z);


    if (cX != X) {
        fprintf(gcode,"X%s", double_to_str(X));
	    cX = X;
	}
    if (cY != Y) {
        fprintf(gcode,"Y%s", double_to_str(Y));
	    cY = Y;
	}


	toolspeed = ceil(speedratio * toolspeed /10)*10;

    if (cZ != Z)
        fprintf(gcode,"Z%s", double_to_str(Z));
    if (cS != toolspeed && command == '1')
        fprintf(gcode, "F%i", (int)(toolspeed));
        
    prev_valid = 1;
	has_current = 1;
    cZ = Z;
	if (command == '1')
	    cS = toolspeed;
    fprintf(gcode, "\n");
    mill_count++;
}

void gcode_travel_to(double X, double Y)
{
    char buffer[256];
    sprintf(buffer,"Travel distance %5.4fmm", dist(X, Y, cX, cY));
    gcode_write_comment(buffer);
    if (cZ < safe_retract_height)
        gcode_retract();
    fprintf(gcode, "G0");
    if (cX != X)
        fprintf(gcode,"X%s", double_to_str(X));
    if (cY != Y)
        fprintf(gcode,"Y%s", double_to_str(Y));
    cX = X;
    cY = Y;
    fprintf(gcode, "\n");
    prev_valid = 0;
}

void gcode_conditional_travel_to(double X, double Y, double Z, double speed)
{
    if (cX == X && cY == Y && cZ == Z)
        return;
        
    /* rounding error handling: if we're within 0.01 mm just mill to it */
    if (cZ == Z && dist(X,Y,cX,cY) < 0.07) {
        gcode_vmill_to(X, Y, Z, speed);
        return;
    }
    if (cX !=X || cY != Y)
     gcode_travel_to(X, Y);
    if (Z > 0)
        gcode_plunge_to(Z, speed);
        
}

void gcode_vconditional_travel_to(double X, double Y, double Z, double speed, double nextX, double nextY, double nextZ)
{
    double pX, pY;
    if (approx4(cX, X) && approx4(cY, Y) && approx4(cZ,Z))
        return;

//    printf("Travel to %5.4f %5.4f %5.4f\n", X, Y, Z);
    if (dist3(X,Y,Z,cX,cY,cZ) < 0.05) {
        gcode_vmill_to(X, Y, Z, speed);
        return;
    }

// if both soure and target are above 0, consider just millting to it */
    if (dist3(X,Y,Z,cX,cY,cZ) < 20 && Z > 0 && cZ > 0) {
        gcode_vmill_to(X, Y, Z, speed);
        return;
    }
        

    /* can we just go back */
    if (Z == nextZ && prev_valid && dist(prevX1, prevY1, X,Y) < 0.005) {
         gcode_write_comment("Going back");
         gcode_vmill_to(X, Y, Z, speed);
    }  else
    /* we have cases where due to math precision, it's easier to go back a bit over the existing line  */
    if (Z == nextZ && prev_valid && vector_intersects_vector(prevX1, prevY1, prevX2, prevY2, X, Y, nextX, nextY, &pX, &pY)) {
         if (dist(pX,pY, cX,cY) + dist(pX,pY, X,Y) < fabs(3 * Z)) {
            gcode_write_comment("Split X toolpath");
            gcode_vmill_to(pX,pY, Z, speed);
            gcode_vmill_to(X,Y, Z, speed);
//          printf("HAVE GOOD PATH %5.2fx%5.2f \n", pX, pY);
//          printf("D1 is %5.4f\n", dist(pX,pY, cX,cY));
//          printf("D2 is %5.4f\n", dist(pX,pY, X,Y));
         } else {
//          printf("line 1: %5.2f,%5.2f -> %5.2f,%5.2f\n", prevX1, prevY1, prevX2, prevY2);
//          printf("line 2: %5.2f,%5.2f -> %5.2f,%5.2f\n", X, Y, nextX, nextY);
//          printf("HAVE LONG PATH %5.2fx%5.2f \n", pX, pY);
//          printf("D1 is %5.2f\n", dist(pX,pY, cX,cY));
//          printf("D2 is %5.2f\n", dist(pX,pY, X,Y));
         }
    }
        
    if (cX !=X || cY != Y)
     gcode_travel_to(X, Y);
    if (Z <= 0)
        gcode_plunge_to(Z, speed);
        
}


int gcode_vconditional_would_retract(double X, double Y, double Z, double speed, double nextX, double nextY, double nextZ)
{
    double pX, pY;
    if (cX == X && cY == Y && cZ == Z)
        return 0;

    if (dist3(X,Y,Z,cX,cY,cZ) < 0.07 || speed < 0) {
        return 0;
    }
    /* can we just go back */
    if (Z == nextZ && prev_valid && dist(prevX1, prevY1, X,Y) < 0.005) {
		 return 0;
    }  else
    /* we have cases where due to math precision, it's easier to go back a bit over the existing line  */
    if (Z == nextZ && prev_valid && vector_intersects_vector(prevX1, prevY1, prevX2, prevY2, X, Y, nextX, nextY, &pX, &pY)) {
         if (dist(pX,pY, cX,cY) + dist(pX,pY, X,Y) < fabs(3 * Z)) {
			return 0;
         }
    }
        
    if (cX !=X || cY != Y)
     return 1;
    return 0;
}



void gcode_write_comment(const char *comment)
{
    fprintf(gcode, "(%s)\n", comment);
}

void write_gcode_footer(void)
{
    gcode_retract();
    fprintf(gcode, "M5\n");
    fprintf(gcode, "M30\n");
    fprintf(gcode, "(END)\n");
    fprintf(gcode, "%%\n");
    fclose(gcode);
    vprintf("There were %i retracts in the file and %i milling toolpaths\n", retract_count, mill_count);
}

double gcode_current_X(void)
{
    return cX;
}

double gcode_current_Y(void)
{
    return cY;
}

static int first_time = 1;
void gcode_tool_change(int toolnr)
{
 if (toolnr == current_tool_nr) 
   return;
 if (!first_time) {
  gcode_retract();
  if (!want_separate)
	  fprintf(gcode, "M5\n");
 }
 current_tool_nr = toolnr;
 activate_tool(toolnr); 
 if (want_separate && !first_time) {
		write_gcode_footer();
		write_gcode_header(stored_filename);
 }
 fprintf(gcode, "M6 T%i\n", abs(toolnr));
 fprintf(gcode, "M3 S%i\n", (int)rippem);  
 fprintf(gcode, "G0 X0Y0\n");
 first_time = 0;
    prev_valid = 0;
	has_current = 0;

	cX = 0;
	cY = 0;
	cZ = 500000;	
}

int gcode_has_current(void)
{
	return has_current;
}

void gcode_reset_current(void)
{
	has_current = 0;
	cS = 0;
	cX = -500000;
	cY = -500000;
	cZ = -500000;	
}

void gcode_set_roughing(int value)
{
	am_roughing = value;
}

void gcode_want_separate_files(void)
{
	want_separate = 1;
}

void gcode_want_adaptive(void)
{
	want_adaptive = true;
}