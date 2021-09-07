#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


void pass_dependencies(struct element *e)
{
    unsigned int i, j;
    
    
    if (e->children.size() < 1)
        return;
        
    
    for (i = 0; i < e->children.size(); i++) {
        struct element *first;
        first = e->children[i];
        
        /* 
         * Warning: O(N^2) complexity here
         * 
         * We will try to cut this down by honoring barriers and aborting early as often as possible
         */
        for (j = i + 1; i < e->children.size(); j++) {
            struct element *second;
            second = e->children[j];
            
            /* cut the chain on barriers */
            if (second->is_barrier || second->type != TYPE_CONTAINER) {
                declare_A_depends_on_B(second, first);
                break;
            }
            
            if (elements_intersect(first, second) || first->is_barrier)
                declare_A_depends_on_B(second, first);
            
            
        }
    }
}