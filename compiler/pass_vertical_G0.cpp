#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_vertical_G0(struct element *e)
{
    if (e->type == TYPE_MOVEMENT && e->X1 == e->X2 && e->Y1 == e->Y2 && e->Z1 < e->Z2) {
        if (e->glevel == '1') {
             e->glevel = '0';
             stat_pass_vertical_G0++;   
        } 
        e->is_retract = true;
    }


    for (auto i: e->children) {
        pass_vertical_G0(i);
    }
}