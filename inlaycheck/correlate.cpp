#include "inlay.h"

#include <cstdio>

/* one shot cache of the problem point -- allows for quick check */
static int breakX = 0,breakY = 0;

static int early_exit_count = 0;
static int total_count = 0;

static int step = 1;

static double correlate(render *base, render *plug, double limit)
{
    double delta;
    
    int x, y;
    
    total_count++;

    delta = plug->get_height(breakX+step, breakY) - base->get_height(breakX+step, breakY);
    if (delta <= limit) {
        early_exit_count++;
        breakX+=step;
        return delta;
    } 
    delta = plug->get_height(breakX, breakY) - base->get_height(breakX, breakY);
    if (delta <= limit) {
        early_exit_count++;
        return delta;
    } 
    delta = plug->get_height(breakX, breakY+step) - base->get_height(breakX, breakY+step);
    if (delta <= limit) {
        early_exit_count++;
        breakY+=step;
        return delta;
    } 
    double best_so_far = delta;
    
    
    for (y = base->minY-plug->height/2; y < base->maxY + plug->height/2; y++) {
        int limitX = base->maxX + plug->width/2;
        for (x = base->minX -plug->width/2; x < limitX; x++) {
            
//            printf("Trying %i,%i\n", x,y);
            delta = plug->get_height(x, y) - base->get_height(x, y);
            
            if (delta <= limit) { /* we failed */
//                printf("Fail at %i,%i   %5.2f vs %5.2f \n", x,y, delta, limit);
                breakX = x;
                breakY = y;
                return delta;
            }
            
            if (delta < best_so_far) {
                breakX = x;
                breakY = y;
                best_so_far = delta;
            }
        }
    }
//    printf("Hit at %i %i for depth %5.2f\n", breakX, breakY, best_so_far);
    return best_so_far;
}


double find_best_correlation(render *base, render *plug)
{
    double best_so_far = 0.001;
    
    int x,y;
    
    int bestx = 0, besty = 0;
    
    printf("Finding location of plug in base \n");
    
    step  = base->pixels_per_mm/4;
    if (step < 1)
        step = 1;
    for (y = -plug->height/4 ; y < + plug->height/4 ; y += step) {
//        printf("y is %i / %i\n", y, base->height);
	//printf("   %i/%i early exits\n", early_exit_count, total_count);
        for (x = -plug->width/4; x < +plug->width/4 ; x += step) {
            plug->set_offsets(x, y);
            double v = correlate(base, plug, best_so_far);
            if (v > best_so_far) {
                printf("Found best so far: (%i, %i) at %5.2f\n", x, y, v);
                best_so_far = v;
                bestx = x;
                besty = y;
            }
        }
    }


    /* fine tuning phase, we know we're pretty close */
    printf("Finding location of plug in base phase 2\n");
    
    step = 1;
    for (y = besty - base->pixels_per_mm/2 ; y < besty + base->pixels_per_mm/2 ; y += step) {
        for (x = bestx - base->pixels_per_mm/2; x < bestx + base->pixels_per_mm/2 ; x += step) {
            plug->set_offsets(x, y);
            double v = correlate(base, plug, best_so_far);
            if (v > best_so_far) {
                printf("Found best so far: (%i, %i) at %5.2f\n", x, y, v);
                best_so_far = v;
                bestx = x;
                besty = y;
            }
        }
    }
    printf("%i/%i early exits\n", early_exit_count, total_count);
    
    plug->set_offsets(bestx, besty);
    return best_so_far;
}


void save_as_xpm(const char *filename, render *base,render *plug, double offset)
{
    FILE *file;
    int x,y;
    double lowest_d = 5;
    
    
    printf("Offset is %5.2f\n", offset);
    file = fopen(filename, "w");
    
    fprintf(file, "! XPM2\n");
    fprintf(file, "%i %i 6 1\n", base->width, base->height);
    fprintf(file, "a c #FFFFFF\n");
    fprintf(file, "r c #FF0000\n");
    fprintf(file, "g c #00FF00\n");
    fprintf(file, "l c #88FF88\n");
    fprintf(file, "b c #0000FF\n");
    fprintf(file, "B c #8080FF\n");

    for (y = base->height-1; y >= 0; y--) {
        for (x =0; x <base->width; x++) {
            double b = base->get_height(x,y);
            double p = plug->get_height(x,y);
            double d = p - b - offset;
            
            if (b == 0) {
                continue;
            }
            /* the plug sticks out, we have a gap, color it red */
            if (p - offset > 0) {
                continue;
            }
            if (d < lowest_d)
                lowest_d = d;
            
        }
    }
    
//    printf("Lowest d is %5.2f\n", lowest_d);
    lowest_d += 0.02; /* cope with rounding errors */
    

    for (y = base->height-1; y >= 0; y--) {
        for (x =0; x <base->width; x++) {
            double b = base->get_height(x,y);
            double p = plug->get_height(x,y);
            double d = p - b - offset;
//                printf("x %i y %i  b %5.2f  p %5.2f   ox %i, oy %i  oZ %5.2f\n", x,y,b,p, plug->offsetX, plug->offsetY, plug->offsetZ);
            
            if (b == 0) {
                fprintf(file, "a");
                continue;
            }
            /* the plug sticks out, we have a gap, color it red */
            if (p - offset > 0) {
            
                fprintf(file, "r");
                continue;
            }
            /* we nearly touch */
            if (d <= lowest_d) {
                fprintf(file, "b");
                continue;
            }
            /* we nearly touch */
            if (d <= lowest_d + 0.1) {
                fprintf(file, "B");
                continue;
            }
            if (d < 0.5) {
                fprintf(file, "g");
                continue;
            }
            fprintf(file, "l");
            
        }
        fprintf(file, "\n");
    }
    fclose(file);
}