#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"

raw_gcode::raw_gcode(const raw_gcode &obj)
{
    raw_gcode(obj.gcode_string);
}

raw_gcode::raw_gcode(const char *str)
{
    type = Raw_gcode;
    minX = minY = minZ = maxX = maxY = maxZ = 0;

    gcode_string = strdup(str);
}

raw_gcode::~raw_gcode(void)
{
    free(gcode_string);
}


void raw_gcode::append_gcode(FILE *file)
{
    fprintf(file, "%s\n", gcode_string);
}