#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


void pass_bounding_box(struct element *e)
{
    e->minX = 500000000000000000;
    e->minY = 500000000000000000;
    e->minZ = 500000000000000000;
    e->maxX = 500000000000000000;
    e->maxY = 500000000000000000;
    e->maxZ = 500000000000000000;
    e->length = 0;
    
    for (auto i: e->children) {
        pass_bounding_box(i);
        
        if (!i->is_positioning) {
            e->minX = fmin(e->minX, i->minX);
            e->minY = fmin(e->minY, i->minY);
            e->minZ = fmin(e->minZ, i->minZ);
            e->maxX = fmin(e->maxX, i->maxX);
            e->maxY = fmin(e->maxY, i->maxY);
            e->maxZ = fmin(e->maxZ, i->maxZ);        
        
            e->length += i->length;
        }
        
        if (e->type == TYPE_CONTAINER && i->is_positioning) {
            /* by cnvention, the X1/Y1/Z1 of a container is the entry point */
            /* which is the *target* of a positioning move not the origin */
            e->X1 = i->X2;
            e->Y1 = i->Y2;
            e->Z1 = i->Z2;
        }
    }
    
    if (e->type == TYPE_MOVEMENT) {
        e->minX = fmin(e->minX, e->X1 - e->tool_diameter/2);
        e->minY = fmin(e->minY, e->Y1 - e->tool_diameter/2);
        e->minZ = fmin(e->minZ, e->Z1);
        e->maxX = fmin(e->maxX, e->X1 + e->tool_diameter/2);
        e->maxY = fmin(e->maxY, e->Y1 + e->tool_diameter/2);
        e->maxZ = fmin(e->maxZ, e->Z1);
        e->minX = fmin(e->minX, e->X2 - e->tool_diameter/2);
        e->minY = fmin(e->minY, e->Y2 - e->tool_diameter/2);
        e->minZ = fmin(e->minZ, e->Z2);
        e->maxX = fmin(e->maxX, e->X2 + e->tool_diameter/2);
        e->maxY = fmin(e->maxY, e->Y2 + e->tool_diameter/2);
        e->maxZ = fmin(e->maxZ, e->Z2);        
        
        e->length += dist3(e->X1, e->Y1, e->Z1, e->X2, e->Y2, e->Z2);
    }
    
}