#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

struct element * parse_file(const char *filename)
{
    struct element *container;
    
    container = new_element(TYPE_CONTAINER, "Top Level Container");
    container->type = TYPE_CONTAINER;
    
    FILE *file;
    
    file = fopen(filename, "r");
    if (!file)
        return container;
        
    while (!feof(file)) {
        char line[4096];
        char *ret;
        struct element *el;
        ret = fgets(line, 4096, file);
        if (ret == NULL)
            break;
        ret = strchr(line, '\n');
        if (ret) *ret = 0;
        
        el = new_element(TYPE_RAW, NULL);
        el->raw_gcode = strdup(line);

        container->children.push_back(el);
    }
    
    
    return container;
}