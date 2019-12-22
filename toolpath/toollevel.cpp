/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include "tool.h"
extern "C" {
    #include "toolpath.h"
}

static inline double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

void toollevel::print_as_svg(void)
{
    char color[32], color2[32];
    int c;
    int xi = 0;
    
    c = level * 16;
    if (c > 200)
        c = 200;
        
        
    sprintf(color, "#%02x%02x%02x", c, c, c);
    sprintf(color2, "#%02x%02x%02x", c, c, 255);
    if (is_slotting) {
        sprintf(color, "purple\" marker-end=\"url(#arrow)");
    }
    for (auto i : toolpaths) {
        c = xi * 8;
        if (c > 255) c = c & 255;
        if (is_slotting)
            sprintf(color, "#%02x%02x%02x", c, 128, 0);
        i->print_as_svg(color);
        xi++;
    }
}


static double sortX, sortY;
static bool compare_path(class toolpath *A, class toolpath *B)
{
    return (A->distance_from(sortX, sortY) < B->distance_from(sortX, sortY));
}

void toollevel::output_gcode(void)
{
    vector<class toolpath*> worklist;    
    
    worklist = toolpaths;
    gcode_write_comment(name);
    sortX = gcode_current_X();
    sortY = gcode_current_Y() + get_minY();
        
    sort(worklist.begin(), worklist.end(), compare_path);
    
    while (worklist.size() > 0) {
        worklist[0]->output_gcode();
        worklist.erase(worklist.begin());
        sortX = gcode_current_X();
        sortY = gcode_current_Y() + get_minY();
        
        sort(worklist.begin(), worklist.end(), compare_path);
    }
}


void toollevel::add_poly(Polygon_2 *poly, bool is_hole)
{
    class toolpath *tp;
    
    tp = new(class toolpath);
    tp->add_polygon(poly);
    tp->is_hole = is_hole;
    tp->is_slotting = is_slotting;
    tp->diameter = diameter;
    tp->depth = depth;
    tp->recalculate();
    
    /* check if the same path is already there if we're slotting */
    if (is_slotting) {
        for (auto tp : toolpaths) {
            if (tp->polygons.size() > 1)
                continue;
            Polygon_2 *p2;
            p2 = tp->polygons[0];
            if ( (*p2)[0].x() == (*poly)[0].x() && (*p2)[0].y() == (*poly)[0].y() &&
                (*p2)[1].x() == (*poly)[1].x() && (*p2)[1].y() == (*poly)[1].y())
                    return;
            if ( (*p2)[0].x() == (*poly)[1].x() && (*p2)[0].y() == (*poly)[1].y() &&
                (*p2)[1].x() == (*poly)[0].x() && (*p2)[1].y() == (*poly)[0].y())
                    return;
#if 1
            if (dist((*p2)[0].x(), (*p2)[0].y(), (*poly)[0].x(), (*poly)[0].y()) < 0.15 &&
                dist((*p2)[1].x(), (*p2)[1].y(), (*poly)[1].x(), (*poly)[1].y()) < 0.15)
                   return;
            if (dist((*p2)[0].x(), (*p2)[0].y(), (*poly)[0].x(), (*poly)[0].y()) < 0.15 &&
                dist((*p2)[0].x(), (*p2)[0].y(), (*poly)[1].x(), (*poly)[1].y()) < 0.15)
                   return;
#endif            
        }
    }

    toolpaths.push_back(tp);    
}

void toollevel::sort_if_slotting(void)
{
    if (!is_slotting)
        return;

    /* to be implemented */        
}
