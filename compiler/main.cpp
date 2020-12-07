#include <stdlib.h>

#include "compiler.h"



int main(int argc, char **argv)
{
    class element *element;
    FILE *file;
    
    element = parse_file("input.nc");
    
    element->print_stats();
    
    
    pass_add_g_level(element);
    
    
    file = fopen("output.nc", "w");
    if (!file)
        return EXIT_FAILURE;
        
    element->append_gcode(file);
    fclose(file);
    
    return EXIT_SUCCESS;
}