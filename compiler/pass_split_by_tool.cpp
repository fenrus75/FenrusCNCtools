#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_split_by_tool(struct element *e)
{
    unsigned int start = 0;
    unsigned int i;
    struct element *target;
    
    
    if (e->children.size() < 1)
        return;
        
    target = new_element(TYPE_CONTAINER, "Tool grouping");
    target->is_toolgroup = true;
    
    for (i = 0; i < e->children.size(); i++) {
        struct element *t;
        t = e->children[i];
        
        if (t->type != TYPE_RAW)
            continue;
        if (strncmp(t->raw_gcode, "M6T", 3) == 0 && i > start) {
            move_children_to_element(e, target, start, i - 1);
            target = new_element(TYPE_CONTAINER, "Tool grouping");
            target->is_toolgroup = true;

            i = start;
            start++;
        }
        
    }
    move_children_to_element(e, target, start, e->children.size() -1 );

    for (auto i: e->children) {
        if (!i->is_toolgroup)
            pass_split_by_tool(i);
    }
}