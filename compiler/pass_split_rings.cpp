#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_split_rings(struct element *e)
{
    struct element *target;
    
    for (auto i: e->children) {
        if (!i->is_ring)
            pass_split_rings(i);
    }

    
    if (e->children.size() < 1)
        return;
        
    if (!e->is_g1only)
        return;
        
    for (unsigned int i = 0; i < e->children.size(); i++) {
        struct element *t;
        t = e->children[i];
        if (t->Z1 != t->Z2)
            continue;
        
        for (unsigned int j = i + 1; j < e->children.size(); j++) {
            struct element *t2;
            t2 = e->children[j];
            
            /* rings must be on the same Z */
            if (t2->Z1 != t2->Z2)
                break;
            if (t2->Z1 != t->Z1)
                break;
            if (t2->type != TYPE_MOVEMENT)
                break;
                
            if (t->X1 == t2->X2 && t->Y1 == t2->Y2) {
                target = new_element(TYPE_CONTAINER, "G1 ring");
                target->is_ring = true;
                move_children_to_element(e, target, i, j);

            }
        }
        
    }

}