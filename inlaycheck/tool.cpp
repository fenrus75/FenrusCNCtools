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
    scanzone = 4; /* 8mm diameter V bit */
}

double vbit::get_height(double R, double depth)
{
    return fabs(R) / _tan + depth; 
    return 0;
}

flat::flat(double diameter)
{
    _diameter = diameter;
    scanzone = diameter / 2 + 0.5;
}

double flat::get_height(double R, double depth)
{
    if (R > _diameter / 2)
            return 0;
 
    return depth;           
}