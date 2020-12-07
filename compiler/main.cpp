#include <stdlib.h>

#include "compiler.h"



int main(int argc, char **argv)
{
    class element *element;
    
    element = parse_file("input.nc");
    
    element->print_stats();
    
    return EXIT_SUCCESS;
}