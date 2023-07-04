#include "inlay.h"

#include <cstdio>

/* one shot cache of the problem point -- allows for quick check */
static int breakX = 0,breakY = 0;

static int early_exit_count = 0;
static int total_count = 0;

static int step = 1;

double correlate(render *base, render *plug, double limit)
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
//    printf("Hit at %i %i for depth %5.2f\n", breakX, breakY, best_so_far);
    return best_so_far;
}


void find_best_correlation(render *base, render *plug)
{
    double best_so_far = -50;
    
    int x,y;
    
    int bestx, besty;
    
    step  = 3;
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
    
    step = 1;
    for (y = besty - 5 ; y < besty + 5 ; y += step) {
        for (x = bestx - 5; x < bestx + 5 ; x += step) {
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
}
