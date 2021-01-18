#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


double retractZ;

void pass_plunge_detect(struct element *e)
{   
    bool prev_plunge = false;
    for (auto i: e->children) {
        pass_plunge_detect(i);
        if (prev_plunge && i->type == TYPE_CONTAINER)
            i->is_from_plunge = true;
            
        prev_plunge = i->is_plunge;
    }
    
    if (!e->is_vertical)
        return;
        
    if (e->Z1 >= retractZ -0.0001)
        e->is_plunge = true;
}