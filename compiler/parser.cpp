#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

class element * parse_file(const char *filename)
{
    class container *container = new class container();
    
    FILE *file;
    
    file = fopen(filename, "r");
    if (!file)
        return container;
        
    while (!feof(file)) {
        char line[4096];
        char *ret;
        ret = fgets(line, 4096, file);
        if (ret == NULL)
            break;
        ret = strchr(line, '\n');
        if (ret) *ret = 0;

        container->push(new class raw_gcode(line));
    }
    
    
    return container;
}