#include <stdlib.h>

#include "compiler.h"




void optimization_passes(struct element *e)
{
    pass_split_by_tool(e);
    pass_raw_to_movement(e);
    

    pass_vertical_G0(e);
    
    pass_split_g1(e);
    
    pass_split_rings(e);
    
    pass_bounding_box(e);
    retractZ = e->maxZ;
    
    pass_plunge_detect(e);

    print_stats();
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