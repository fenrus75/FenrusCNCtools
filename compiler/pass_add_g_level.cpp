#include "compiler.h"

void pass_add_g_level(class element *element)
{
    class container *container;
    
    if (element->type != Container)
        return;
    container = (class container *) element;
    
    
    char current_G_level = '0';
    std::vector<class element *> *elements = container->get_vector();
    
    for (auto i : *elements) {
        if (i->type == Raw_gcode) {
            class raw_gcode *raw = (class raw_gcode*)i;
            current_G_level = raw->get_G_level(current_G_level);
            raw->set_G_level(current_G_level);            
        }
    }

}