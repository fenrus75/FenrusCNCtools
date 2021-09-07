#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_positioning(struct element *e, struct element *prev)
{
    if (prev && e->type == TYPE_MOVEMENT && prev->is_retract) {
        e->is_positioning = true;
    }

    struct element *p = NULL;
    for (auto i: e->children) {
        pass_positioning(i, p);
        p = i;
    }
}