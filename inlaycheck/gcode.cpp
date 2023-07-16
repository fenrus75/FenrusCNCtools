#include "inlay.h"

#include <cstdio>
#include <cmath>

extern vbit *lastv;

FILE *gcode_file(const char *filename)
{
    FILE *file;
    
    file = fopen(filename, "w");
    if (!file)
        return NULL;
        
    fprintf(file, "G90\n");
    fprintf(file, "G21\n");
    fprintf(file, "G53G0Z-5.000\n");
    fprintf(file, "M05\n");
    fprintf(file, "(TOOL/MILL,0.03, 0.00, 10.00, %4.2f)\n", lastv->_angle/2.0);
    fprintf(file, "M6T102\n");
    fprintf(file, "M03S18000\n");
    fprintf(file, "G1Z2\n");

        
        
    return file;
}

void gcode_close(FILE *file)
{
    fprintf(file, "M05\n");
    fprintf(file, "M02\n");
    fclose(file);
}

static int cx,cy;
static double cz;

static void gcode_lift(FILE *file)
{
    fprintf(file, "G0Z2\n");
    cz = 2;
}


static void gcode_rapid_to(FILE *file, render *plug, int x, int y)
{
    gcode_lift(file);
    fprintf(file, "G0X%0.4fY%0.4f\n", plug->x_to_mm(x + plug->croppedX), plug->y_to_mm(y + plug->croppedY));
    cx = x;
    cy = y;
}

static void gcode_mill_to(FILE *file, render *plug, int x, int y, double z)
{
    if (cz != z) {
        fprintf(file, "G1Z%0.4fF380\n", z);
        cz = z;
    }
    fprintf(file, "G1");
    if (cx != x) {
        fprintf(file, "X%0.4f", plug->x_to_mm(x + plug->croppedX));
        cx = x;
    }
    if (cy != y) {    
        fprintf(file, "Y%0.4f", plug->y_to_mm(y + plug->croppedY));
        cy = y;
    }
    fprintf(file, "F1200\n");
}

static bool is_valid (render *plug, int x, int y)
{
    if (x >= plug->width)
        return false;
    if (y >= plug->height)
        return false;
    if (x < 0)
        return false;
    if (y < 0)
        return false;
    if (!plug->validmap[x + y * plug->width])
        return false;
    if (fabs(plug->valuemap[x + y * plug->width]) < 0.001)
        return false;
    
    return true;
}


static void gcode_snake(FILE *file, render *plug, int sx, int sy, double height)
{
            int dx, dy;
            gcode_rapid_to(file, plug, sx, sy);
            dx = 1;
            dy = 0;
    //        printf("---------\n");
            while (true) {
//                printf("gcode for %i %i    dxdy %i %i\n",sx, sy, dx, dy);
                gcode_mill_to(file, plug, sx, sy, height);
                plug->valuemap[sx + sy *plug->width] = 0;  /* mark as done */
                plug->validmap[sx + sy *plug->width] = false;  /* mark as done */
                
                if (is_valid(plug, sx + dx, sy + dy)) {	
                    sx += dx;
                    sy += dy;
                    continue;
                }
                
                if (is_valid(plug, sx - 1, sy)) {
                    dx = -1;
                    dy = 0;
                    sx += dx;
                    sy += dy;
                    continue;
                }
                if (is_valid(plug, sx + 1, sy)) {
                    dx = 1;
                    dy = 0;
                    sx += dx;
                    sy += dy;
                    continue;
                }
                if (is_valid(plug, sx , sy -1)) {
                    dx = 0;
                    dy = -1;
                    sx += dx;
                    sy += dy;
                    continue;
                }
                if (is_valid(plug, sx, sy + 1)) {
                    dx = 0;
                    dy = 1;
                    sx += dx;
                    sy += dy;
                    continue;
                }
                /* no good neighbors -- stop */
                break;
                
            }
            
            
}

static bool good_start(render *plug, int x, int y)
{
    int neighbors = 0;
    
    if (!plug->validmap[x + y * plug->width])
        return false;
    if (plug->valuemap[x + y * plug->width] == 0)
        return false;
        
    if (x < 1 || y < 1)
        return false;
    if (x >= plug->width - 1)
        return false;
    if (y >= plug->height - 1)
        return false;
        
    if (plug->valuemap[(x - 1) + (y - 1) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x + 0) + (y - 1) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x + 1) + (y - 1) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x - 1) + (y + 0) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x + 1) + (y + 0) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x - 1) + (y + 1) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x + 0) + (y + 1) * plug->width] > 0)
        neighbors++;
    if (plug->valuemap[(x + 1) + (y + 1) * plug->width] > 0)
        neighbors++;
        
//    printf("%i %i has %i neighbors \n", x, y, neighbors);
        
    if (neighbors > 1)
        return false;
    return true;
}

void gcode_writeout_maps(FILE *file, render *plug, double height)
{
    int x,y;
    
    for (y = 0; y < plug->height; y++) {
        for (x = 0; x < plug->width; x++) {
            if (!good_start(plug, x, y))
                continue;

            /* ok we found the starting point of a path */
            gcode_snake(file, plug, x, y, height);
        }
    }
    
    printf("FInding loose ends \n");

    for (y = 0; y < plug->height; y++) {
        for (x = 0; x < plug->width; x++) {
            if (!plug->validmap[x + y * plug->width])
                continue;
            if (plug->valuemap[x + y * plug->width] == 0)
                continue;
                
            /* ok we found the starting point of a path */
            gcode_snake(file, plug, x, y, height);
        }
    }
}

