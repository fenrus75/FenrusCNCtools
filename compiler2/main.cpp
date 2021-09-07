#include <stdlib.h>

#include "compiler.h"


double retractZ = 0;


void optimization_passes(struct element *e)
{
    pass_raw_to_movement(e);
    pass_bounding_box(e);
    pass_vertical_G0(e);
    pass_positioning(e, NULL);
    
    pass_split_cut(e);
    pass_bounding_box(e);
    
    pass_dependencies(e);

    retractZ = e->maxZ;
    
    print_tree(e, 0);
}
int main(int argc, char **argv)
{
    struct element *element;
    FILE *file;
    
    element = parse_file("input.nc");
    
    
    optimization_passes(element);
    
    file = fopen("output.nc", "w");
    emit_gcode(file, element);
    fclose(file);
    
    
    return EXIT_SUCCESS;
}