#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

static int cutcount = 0;


void pass_split_cut(struct element *e)
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
        
        if (t->type != TYPE_MOVEMENT && state != 0) {
            need_flush = state;
            state = 0;
        }

        if (t->type == TYPE_MOVEMENT && t->is_retract && state != 0) {
            need_flush = state;
            state = 0;
        }
        
        if (state == 0 && t->is_positioning) {
            start = i;
            state = 1;
        }
  
        if (need_flush) {
            struct element *target;
            char buffer[4096];
            sprintf(buffer, "Cut path %i", cutcount++);
            target = new_element(TYPE_CONTAINER, buffer);
            target->is_split_already = true;
            if (t->is_retract)
                move_children_to_element(e, target, start, i);
            else
                move_children_to_element(e, target, start, i - 1);

            i = start;
            start++;
            state = 0;
        }
    }
    if (state == 1) {
            struct element *target;
            target = new_element(TYPE_CONTAINER, "Cut path");
            target->is_split_already = true;
            move_children_to_element(e, target, start, e->children.size() - 1);
            state = 0;
    }

    for (auto i: e->children) {
        if (!i->is_split_already)
            pass_split_cut(i);
    }
}