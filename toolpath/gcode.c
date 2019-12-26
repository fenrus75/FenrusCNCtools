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

#include "toolpath.h"


static const char *tool_name = "T201";
static double tool_diameter = 6;
static double tool_stepover = 3;
static double tool_maxdepth = 1;
static double tool_feedrate = 0;
static double tool_plungerate = 0;
static double rippem = 10000;
static double safe_retract_height = 2;

static int want_finishing_pass;

static FILE *gcode;
static int retract_count;
static int mill_count;
/* in mm */
static double cX, cY, cZ, cS;

void set_tool_imperial(const char *name, double diameter_inch, double stepover_inch, double maxdepth_inch, double feedrate_ipm, double plungerate_ipm)
{
    tool_name = strdup(name);
    tool_diameter = inch_to_mm(diameter_inch);
    tool_stepover = inch_to_mm(stepover_inch);
    tool_maxdepth = inch_to_mm(maxdepth_inch);
    tool_feedrate = ipm_to_metric(feedrate_ipm);
    tool_plungerate = ipm_to_metric(plungerate_ipm);
}

void set_tool_metric(const char *name, double diameter_mm, double stepover_mm, double maxdepth_mm, double feedrate_metric, double plungerate_metric)
{
    tool_name = strdup(name);
    tool_diameter = diameter_mm;
    tool_stepover = stepover_mm;
    tool_maxdepth = maxdepth_mm;
    tool_feedrate = feedrate_metric;
    tool_plungerate = plungerate_metric;
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

void enable_finishing_pass(void)
{
  want_finishing_pass = 1;
}

int get_finishing_pass(void)
{
  return want_finishing_pass;
}

void write_gcode_header(const char *filename)
{
    gcode = fopen(filename, "w");
    if (!gcode) {
        printf("Cannot open %s for gcode output: %s\n", filename, strerror(errno));
        return;
    }
    fprintf(gcode, "%%\n");
    fprintf(gcode, "G21\n"); /* milimeters not imperials */
    fprintf(gcode, "G90\n"); /* all relative to work piece zero */
    fprintf(gcode, "G0X0Y0Z%5.4f\n", safe_retract_height);
    cZ = safe_retract_height;
    fprintf(gcode, "M6 %s\n", tool_name);
    fprintf(gcode, "M3 S%i\n", (int)rippem);
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
    fprintf(gcode, "\n");
}

void gcode_retract(void)
{
    fprintf(gcode, "G0");
    if (cZ != safe_retract_height)
        fprintf(gcode,"Z%5.4f", safe_retract_height);
    cZ = safe_retract_height;
    fprintf(gcode, "\n");
    retract_count++;
}

void gcode_mill_to(double X, double Y, double Z, double speedratio)
{

    if (cZ != Z)
        gcode_plunge_to(Z, speedratio);
    fprintf(gcode, "G1");
    if (cX != X)
        fprintf(gcode,"X%5.4f", X);
    if (cY != Y)
        fprintf(gcode,"Y%5.4f", Y);
    if (cZ != Z)
        fprintf(gcode,"Z%5.4f", Z);
    if (cS != speedratio * tool_feedrate)
        fprintf(gcode, "F%i", (int)(speedratio * tool_feedrate));
    cX = X;
    cY = Y;
    cZ = Z;
    cS = speedratio * tool_feedrate;
    fprintf(gcode, "\n");
    mill_count++;
}

void gcode_vmill_to(double X, double Y, double Z, double speedratio)
{

    fprintf(gcode, "G1");
    if (cX != X)
        fprintf(gcode,"X%5.4f", X);
    if (cY != Y)
        fprintf(gcode,"Y%5.4f", Y);
    if (cZ != Z)
        fprintf(gcode,"Z%5.4f", Z);
    if (cS != speedratio * tool_feedrate)
        fprintf(gcode, "F%i", (int)(speedratio * tool_feedrate));
    cX = X;
    cY = Y;
    cZ = Z;
    cS = speedratio * tool_feedrate;
    fprintf(gcode, "\n");
    mill_count++;
}

void gcode_travel_to(double X, double Y)
{

    if (cZ < safe_retract_height)
        gcode_retract();
    fprintf(gcode, "G0");
    if (cX != X)
        fprintf(gcode,"X%5.4f", X);
    if (cY != Y)
        fprintf(gcode,"Y%5.4f", Y);
    cX = X;
    cY = Y;
    fprintf(gcode, "\n");
}

static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

void gcode_conditional_travel_to(double X, double Y, double Z, double speed)
{
    if (cX == X && cY == Y && cZ == Z)
        return;
        
    /* rounding error handling: if we're within 0.01 mm just mill to it */
    if (cZ == Z && dist(X,Y,cX,cY) < 0.07) {
        gcode_mill_to(X, Y, Z, speed);
        return;
    }
    gcode_travel_to(X, Y);
    if (Z > 0)
        gcode_plunge_to(Z, speed);
        
}

void gcode_vconditional_travel_to(double X, double Y, double Z, double speed)
{
    if (cX == X && cY == Y && cZ == Z)
        return;
        
    gcode_travel_to(X, Y);
    if (Z > 0)
        gcode_plunge_to(Z, speed);
        
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
    printf("There were %i retracts in the file and %i milling toolpaths\n", retract_count, mill_count);
}

double gcode_current_X(void)
{
    return cX;
}

double gcode_current_Y(void)
{
    return cY;
}

void gcode_tool_change(int toolnr)
{
 gcode_retract();
 fprintf(gcode, "M5\n");
 activate_tool(toolnr); 
 fprintf(gcode, "M6 T%i\n", toolnr);
 fprintf(gcode, "M3 S%i\n", (int)rippem);  
}