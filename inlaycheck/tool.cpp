#include "tool.h"

#include <cmath>

double tool::get_height(double R, double depth)
{
    return depth;
}

vbit::vbit(double angle)
{
    _angle = angle;
    _tan = tan(_angle/360.0 * 2 * M_PI / 2);
    _invtan = 1/_tan;
    scanzone = 4; /* 8mm diameter V bit */
    samplesteps = 3;
}

double vbit::get_height(double R, double depth)
{
    return R * _invtan + depth; 
}

double vbit::get_height_static(double R, double depth)
{
    return R * _invtan + depth; 
}

flat::flat(double diameter)
{
    _diameter = diameter;
    _radius = diameter / 2;
    scanzone = diameter / 2 + 0.5;
}

double flat::get_height(double R, double depth)
{
    if (R > _radius )
            return 0;
 
    return depth;           
}