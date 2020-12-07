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

bool raw_gcode::is_movement(void) {
    if (gcode_string[0] == '(')
        return false;
    if (gcode_string[0] == 'M')
        return false;
    if (strchr(gcode_string, 'X'))
        return true;
    if (strchr(gcode_string, 'Y'))
        return true;
    if (strchr(gcode_string, 'Z'))
        return true;
    return false;
}

char raw_gcode::get_G_level(char def)
{
    if (gcode_string[0] != 'G')
        return def;
    
    if (strlen(gcode_string) < 2)
        return def;
        
    if (gcode_string[0]=='G' && (gcode_string[1] >= '0' && gcode_string[1] <= '1') && (gcode_string[2] < '0' || gcode_string[2] > '9')) {
        return gcode_string[1];
    }

    return def;        
}

void raw_gcode::set_G_level(char level)
{
    if (!is_movement())
        return;
    /* Easy case : we already have a G level set, we just need to update it */
    if (strlen(gcode_string) > 2) {
        if (gcode_string[0]=='G' && (gcode_string[1] >= '0' && gcode_string[1] <= '1') && (gcode_string[2] < '0' || gcode_string[2] > '9')) {
            gcode_string[1] = level;
            return;
        }
    }
    
    if (gcode_string[0] != 'G') {
        char newstring[4096];
        sprintf(newstring, "G%c%s", level, gcode_string);
        free(gcode_string);
        gcode_string = strdup(newstring);
        return;
    }
}



void raw_gcode::append_gcode(FILE *file)
{
    fprintf(file, "%s\n", gcode_string);
}