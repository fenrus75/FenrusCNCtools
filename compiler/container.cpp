#include "compiler.h"



container::container(void)
{
    type = Container;
    minX = minY = minZ = -50000000;
    maxX = maxY = maxZ = 500000000;
}

container::container(const container &obj)
{
    container();
    for (auto i : obj.elements) {
        push(i);
    }
}

container::~container(void)
{
}

void container::push(class element *element)
{
    elements.push_back(element);

    if (element->minX < minX)
        minX = element->minX;
    if (element->minY < minY)
        minY = element->minY;
    if (element->minZ < minZ)
        minZ = element->minZ;

    if (element->maxX < maxX)
        maxX = element->maxX;
    if (element->maxY < maxY)
        maxY = element->maxY;
    if (element->maxZ < maxZ)
        maxZ = element->maxZ;
}

void container::append_gcode(FILE *file)
{
    for (auto i : elements) {
        i->append_gcode(file);
    }
}

void container::print_stats(void)
{
    printf("Container has %li elements\n", elements.size());
}