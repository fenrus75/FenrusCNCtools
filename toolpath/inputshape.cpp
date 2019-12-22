/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include "tool.h"

/* todo: get rid of this addiction to print.h */
#include "print.h"

void inputshape::set_level(int _level)
{
    level = _level;
    
    for (auto i : children)
        i->set_level(_level + 1);
        
    fix_orientation();
}

void inputshape::add_child(class inputshape *child)
{
    children.push_back(child);
    child->set_level(level + 1);
}

void inputshape::wipe_children(void)
{
    children.clear();
}

void inputshape::update_stats(void)
{
    area = fabs(poly.area());
}

void inputshape::print_as_svg(void)
{
    const char *color = "black";
    if (level == 1)
        color = "green";
    if (level == 2)
        color = "blue";
    if (level == 3)
        color = "red";
    print_polygon(poly, color);

#if 0
    for (auto i : skeleton)
        print_straight_skeleton(*i);
#endif
    for (auto i : children)
        i->print_as_svg();

    for (auto i : toolpaths)
        i->print_as_svg();

}

void inputshape::output_gcode(void)
{
    gcode_write_comment("Shape");
    for (auto i =  toolpaths.rbegin(); i != toolpaths.rend(); ++i)
        (*i)->output_gcode();
        
    for (auto i : children)
        i->output_gcode();
}
void inputshape::add_point(double X, double Y)
{
    if (poly.size() > 0) {
      auto v1 = poly[0];
      if (v1.x() == X && v1.y() == Y)
          return;
    }
    poly.push_back( Point(X, Y));
}

void inputshape::close_shape(void)
{
    /* check if first and last point are the same.. if they are, pop the last */
    if (poly.size() > 1 && !poly.is_simple()) {
      auto v1 = poly[0];
      auto v2 = poly[poly.size() - 1];
      if (v1.x() == v2.x() && v1.y() == v2.y()) {
          poly.erase(poly.vertices_end() - 1);
      }
    }

    if (!poly.is_simple())
        cout << "Poly is not simple!!" << std::endl;

    /* CGAL wants the points in the opposite orientation than SVG */    
    fix_orientation();
    
    update_stats();
}


/* desired end goal: Even level orientation, counterclockwise,  Odd level orientation, clockwise */
void inputshape::fix_orientation(void)
{
    if (level & 1) {
        /* odd */
        if (poly.orientation() == CGAL::COUNTERCLOCKWISE)
            poly.reverse_orientation();
    } else {
        /* even */
        if (poly.orientation() == CGAL::CLOCKWISE)
            poly.reverse_orientation();
    }
    for (auto i : children)
      i->fix_orientation();
}

bool inputshape::fits_inside(class inputshape *shape)
{
   for(auto vi = poly.vertices_begin() ; vi != poly.vertices_end() ; ++ vi )
     if (shape->poly.bounded_side(*vi) == CGAL::ON_UNBOUNDED_SIDE) {
       return false;
     }
   return true;
}


void inputshape::create_toolpaths(double depth, int finish_pass)
{
    int level = 0;
    double diameter = get_tool_diameter();
    double stepover = get_tool_stepover();
    double inset;
    if (!polyhole) {
        polyhole = new PolygonWithHoles(poly);
        for (auto i : children)
          polyhole->add_hole(i->poly);
    }
    
    /* first inset is the radius (half diameter) of the tool, after that increment by stepover */
    inset = diameter/2;
    
    /* finish_pass is -1 for all layers above the bottom layer IF finishing is enabled */
    if (finish_pass == -1)
        inset = inset + 0.1;
        
    
    /* less stepover during the bottom finish pass (1) */
    if (finish_pass == 1)
        stepover = stepover / sqrt(2);
        
    do {
        class toollevel *tool = new(class toollevel);
        int added = 0;
        
        tool->level = level;
        tool->offset = inset;
        tool->diameter = diameter;
        tool->depth = depth;
                
        PolygonWithHolesPtrVector  offset_polygons;
        offset_polygons = CGAL::create_interior_skeleton_and_offset_polygons_with_holes_2(inset, *polyhole);
        
        if (level == 0) {

            for (auto ply : offset_polygons) {
                SsPtr pp = CGAL::create_interior_straight_skeleton_2(ply->outer_boundary().vertices_begin(),
                    ply->outer_boundary().vertices_end(),
                    ply->holes_begin(), ply->holes_end());            
                skeleton.push_back(pp);                
            }
        }
        for (auto ply : offset_polygons) {        
            Polygon_2 *p;
            p = new(Polygon_2);
            *p = ply->outer_boundary();
            p->reverse_orientation();
            tool->add_poly(p, false);
            added++;

            for(auto hi = ply->holes_begin() ; hi != ply->holes_end() ; ++ hi ) {
              Polygon_2 *p;
              p = new(Polygon_2);
              *p = *hi;
              p->reverse_orientation();
              tool->add_poly(p, true);
              added++;
            }
        }
        
        if (!added)
                break; /* fixme: memleak */
 
        toolpaths.push_back(tool);       
        inset += stepover;
        level ++;
        
    } while (1);
    
    if (skeleton.size() > 0) {
        double lX = 0, lY = 0;
        class toollevel *tool = new(class toollevel);       
        tool->level = level;
        tool->is_slotting = true;
        tool->name = "Bisector Slotting";
        tool->diameter = diameter;
        tool->depth = depth;
        toolpaths.push_back(tool);
        for (auto ss : skeleton) {
            for (auto x = ss->halfedges_begin(); x != ss->halfedges_end(); ++x) {
                    if (x->is_inner_bisector()) {
                        Polygon_2 *p = new(Polygon_2);
                        if (lX == x->vertex()->point().x() && lY == x->vertex()->point().y()) { 
                            p->push_back(Point(x->vertex()->point().x(), x->vertex()->point().y()));
                            p->push_back(Point(x->opposite()->vertex()->point().x(), x->opposite()->vertex()->point().y()));
                            lX = x->opposite()->vertex()->point().x();
                            lY = x->opposite()->vertex()->point().y();
                        } else {
                            p->push_back(Point(x->opposite()->vertex()->point().x(), x->opposite()->vertex()->point().y()));
                            p->push_back(Point(x->vertex()->point().x(), x->vertex()->point().y()));
                            lX = x->vertex()->point().x();
                            lY = x->vertex()->point().y();
                        }
                        tool->add_poly(p, false);
                    }
            } 
        }
    }
}

void inputshape::consolidate_toolpaths(void)
{
    unsigned int level;
    
    /* skip level 0; we want the final contour cut to be special */
    for (level = 0; level + 1 < toolpaths.size(); level++) {
        for (auto tp : toolpaths[level]->toolpaths) {
            class toolpath *match = NULL;
            int valid = 1;
            /* for each toolpath, check if there is exactly e toolpath a level up that is inside this tp */
            for (auto tp2 : toolpaths[level + 1]->toolpaths) {
                if (tp2->fits_inside(tp)) {
                    if (match != NULL)
                        valid = 0;
                    match = tp2;
                }
            }   
            
            if (valid && match && tp->is_hole == match->is_hole && tp->is_slotting == match->is_slotting) {
                for (auto p: tp->polygons) {
                    match->add_polygon(p);
                }
                tp->polygons.clear();
            }         
        }        
    }
   for (auto x : toolpaths)
       x->sort_if_slotting();
}