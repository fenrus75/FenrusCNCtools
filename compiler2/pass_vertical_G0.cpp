#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_vertical_G0(struct element *e)
{
    if (e->type == TYPE_MOVEMENT && e->X1 == e->X2 && e->Y1 == e->Y2 && e->Z1 < e->Z2 && e->Z2 >= retractZ) {
        e->is_retract = true;
    }
    if (e->type == TYPE_MOVEMENT && e->X1 == e->X2 && e->Y1 == e->Y2) {
        e->is_vertical = true;
    }


    for (auto i: e->children) {
        pass_vertical_G0(i);
    }
}