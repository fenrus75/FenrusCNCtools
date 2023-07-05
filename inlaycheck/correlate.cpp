#include "inlay.h"

#include <cstdio>

/* one shot cache of the problem point -- allows for quick check */
static int breakX = 0,breakY = 0;

static int early_exit_count = 0;
static int total_count = 0;

static int step = 1;

static double correlate(render *base, render *plug, double limit)
{
    double best_so_far = 100;
    double delta;
    
    int x, y;
    
    total_count++;

    delta = plug->get_height(breakX, breakY) - base->get_height(breakX, breakY);
    if (delta <= limit) {
        early_exit_count++;
        return delta;
    } 
    delta = plug->get_height(breakX+step, breakY) - base->get_height(breakX+step, breakY);
    if (delta <= limit) {
        early_exit_count++;
        breakX+=step;
        return delta;
    } 
    
    
    for (y = -plug->height/2; y < base->height + plug->height/2; y++) {
        for (x = -plug->width/2; x < base->width + plug->width/2; x++) {
            
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
    printf("Hit at %i %i for depth %5.2f\n", breakX, breakY, best_so_far);
    return best_so_far;
}


double find_best_correlation(render *base, render *plug)
{
    double best_so_far = -50;
    
    int x,y;
    
    int bestx = 0, besty = 0;
    
    printf("Finding location of plug in base \n");
    
    step  = base->pixels_per_mm/4;
    for (y = -plug->height/4 ; y < base->height + plug->height/4 ; y += step) {
//        printf("y is %i / %i\n", y, base->height);
	//printf("   %i/%i early exits\n", early_exit_count, total_count);
        for (x = -plug->width/4; x < base->width +plug->width/4 ; x += step) {
            plug->set_offsets(x, y);
            double v = correlate(base, plug, best_so_far);
            if (v > best_so_far) {
                if (v > 0.01)
                    printf("Found best so far: (%i, %i) at %5.2f\n", x, y, v);
                best_so_far = v;
                bestx = x;
                besty = y;
            }
        }
    }


    best_so_far = -50;
    printf("Finding location of plug in base phase 2\n");
    
    step = 1;
    for (y = besty - base->pixels_per_mm ; y < besty + base->pixels_per_mm ; y += step) {
        for (x = bestx - base->pixels_per_mm; x < bestx + base->pixels_per_mm ; x += step) {
            plug->set_offsets(x, y);
            double v = correlate(base, plug, best_so_far);
            if (v > best_so_far) {
                if (v > 0.01)
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
    
    file = fopen(filename, "w");
    
    fprintf(file, "! XPM2\n");
    fprintf(file, "%i %i 5 1\n", base->width, base->height);
    fprintf(file, "a c #FFFFFF\n");
    fprintf(file, "r c #FF0000\n");
    fprintf(file, "g c #00FF00\n");
    fprintf(file, "l c #88FF88\n");
    fprintf(file, "b c #0000FF\n");
    
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
    
    printf("Lowest d is %5.2f\n", lowest_d);
    lowest_d += 0.01;
    

    for (y = base->height-1; y >= 0; y--) {
        for (x =0; x <base->width; x++) {
            double b = base->get_height(x,y);
            double p = plug->get_height(x,y);
            double d = p - b - offset;
            
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
            if (d < 1) {
                fprintf(file, "g");
                continue;
            }
            fprintf(file, "l");
            
        }
        fprintf(file, "\n");
    }
    fclose(file);
}