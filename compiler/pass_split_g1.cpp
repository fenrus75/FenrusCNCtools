#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_split_g1(struct element *e)
{
    unsigned int start = 0;
    unsigned int i;
    int state = 0;
    
    
    if (e->children.size() < 1)
        return;
        
    
    for (i = 0; i < e->children.size(); i++) {
        int need_flush = 0;
        struct element *t;
        t = e->children[i];
        
        if (t->type != TYPE_MOVEMENT) {
            need_flush = state;
            state = 0;
        }
        
        if (t->glevel != '1') {
            need_flush = state;
            state = 0;
        }
        
        if (state == 0 && t->glevel == '1') {
            start = i;
            state = 1;
        }
        
        if (need_flush) {
            struct element *target;
            target = new_element(TYPE_CONTAINER, "G1 path");
            target->is_g1only = true;
            move_children_to_element(e, target, start, i - 1);

            i = start;
            start++;
            state = 0;
        }
    }
    if (state == 1) {
            struct element *target;
            target = new_element(TYPE_CONTAINER, "G1 path");
            target->is_g1only = true;
            move_children_to_element(e, target, start, e->children.size() - 1);
            state = 0;
    }

    for (auto i: e->children) {
        if (!i->is_g1only)
            pass_split_g1(i);
    }
}