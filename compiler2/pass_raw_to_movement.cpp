#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

static double cX, cY, cZ, cF;
static bool cValid = false;
static char cG = '0';

static double tool_diameter = 0.0;

static void handle_XYZ(struct element *e, const char *c2) 
{
    double nX = cX;
    double nY = cY;
    double nZ = cZ;
    double nF = cF;
    char *c;
    
    c = (char *)c2;
    
    
    while (c && strlen(c) > 0) {
        if (c[0] == ' ') {
            c++;
            continue;
        }
        
        if (c[0] == 'X') {
            nX = strtod(c + 1, &c);
            continue;
        }
        if (c[0] == 'Y') {
            nY = strtod(c + 1, &c);
            continue;
        }
        if (c[0] == 'Z') {
            nZ = strtod(c + 1, &c);
            continue;
        }
        if (c[0] == 'F') {
            nF = strtod(c + 1, &c);
            continue;
        }
        printf("Invalid option in -%s- out of %s\n", c, c2);
        cValid = false;
        return;
    }
    
    e->tool_diameter = tool_diameter;
    
    if (cValid) {
        e->glevel = cG;
        e->X1 = cX;
        e->Y1 = cY;
        e->Z1 = cZ;
        e->X2 = nX;
        e->Y2 = nY;
        e->Z2 = nZ;
        e->feed = nF;
        e->type = TYPE_MOVEMENT;
        stat_pass_raw_to_movement++;
    }
    
    if (cZ == 1000)
        e->is_positioning = true;
    
    cX = nX;
    cY = nY;
    cZ = nZ;
    cF = nF;
    cValid = true;
}

static void handle_G(struct element *e, const char *c) 
{
    if (strlen(c) < 2)
        return;
        
    /* bit change goes to high Z */
    if (strstr(c, "G53G0Z")) {
        cZ = 1000;
        e->is_vertical = true;
    }
    /* for now, only handle G0/G1 */
    if (c[1] <'0' || c[1] > '1') {
        cValid = false;
        return;
    }
    /* check the second digit to make sure we only look at single digit G levels */
    if (strlen(c) > 2 && c[2] >= '0' && c[2] <= '9') {
        cValid = false;
        return;
    }
        
    cG = c[1];
    handle_XYZ(e, c + 2);
}

static void handle_comment(struct element *e, const char *line)
{
    double d;
    
    e->is_barrier = true;
    const char *c;
    c = strstr(line, "(TOOL/MILL,");
    if (!c)
        return;
    c+=11;
    d = strtod(c, NULL);
    tool_diameter = d;
}


void pass_raw_to_movement(struct element *e)
{
    if (e->type == TYPE_RAW) {
        if (e->raw_gcode[0] == 'G')
            handle_G(e, e->raw_gcode);
        if (e->raw_gcode[0] == 'M') {
            e->is_barrier = true;
        }
        if (e->raw_gcode[0] == '(') {
            handle_comment(e, e->raw_gcode);
        }
        if (e->raw_gcode[0] == 'X' || e->raw_gcode[0] == 'Y' || e->raw_gcode[0] == 'Z')
            handle_XYZ(e, e->raw_gcode);
        
    }


    for (auto i: e->children) {
        pass_raw_to_movement(i);
    }
}