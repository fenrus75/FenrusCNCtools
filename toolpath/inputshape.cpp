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

static inline double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

double point_snap(double D)
{
    int i = (int)(D * 10 + 0.5);
    return i / 10.0;
}
double point_snap2(double D)
{
    int i = (int)(D * 100 + 0.5);
    return i / 100.0;
}

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

    for (auto i =  tooldepths.rbegin(); i != tooldepths.rend(); ++i)
        (*i)->print_as_svg();

}

void inputshape::output_gcode(int tool)
{
    tool = abs(tool);
    gcode_write_comment("Shape");
    for (auto i =  tooldepths.rbegin(); i != tooldepths.rend(); ++i) {
        if ((*i)->toolnr == tool || tool == 0)
            (*i)->output_gcode();
    }
        
    for (auto i : children)
        i->output_gcode(tool);
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


void inputshape::create_toolpaths(int toolnr, double depth, int finish_pass, int want_optional, double start_inset, double end_inset, bool want_skeleton_path)
{
    int level = 0;
    double diameter = get_tool_diameter();
    double stepover;
    double inset;
    bool reverse;
    
    if (toolnr < 0) {
        reverse = true;
        toolnr = abs(toolnr);
    }
    
    stepover = get_tool_stepover(toolnr);
    
    
    
    if (!polyhole) {
        polyhole = new PolygonWithHoles(poly);
        for (auto i : children)
          polyhole->add_hole(i->poly);
    }
    
    if (!iss) {
        iss =  CGAL::create_interior_straight_skeleton_2(*polyhole);
    }
    
    /* first inset is the radius (half diameter) of the tool, after that increment by stepover */
    inset = start_inset + diameter/2;
    
    /* finish_pass is -1 for all layers above the bottom layer IF finishing is enabled */
    if (finish_pass == -1)
        inset = inset + 0.1;
        
    
    /* less stepover during the bottom finish pass (1) */
    if (finish_pass == 1)
        stepover = stepover / sqrt(2);
        
    class tooldepth * td = new(class tooldepth);
    tooldepths.push_back(td);
    td->depth = depth;
    td->toolnr = toolnr;
    td->diameter = diameter;
    td->run_reverse = reverse;
    
    do {
        class toollevel *tool = new(class toollevel);
        int added = 0;
        int had_exception = 1;
        
        tool->level = level;
        tool->offset = inset;
        tool->diameter = diameter;
        tool->depth = depth;
        tool->toolnr = toolnr;
        tool->run_reverse = reverse;
        tool->minY = minY;
        tool->name = "Pocketing";
        
        if (want_optional && (level > 1) && ((level & 1) == 0))
            tool->is_optional = 1;
            
        PolygonWithHolesPtrVector  offset_polygons;
//        offset_polygons = CGAL::create_interior_skeleton_and_offset_polygons_with_holes_2(inset, *polyhole);

        while (had_exception) {
            had_exception = 0;
            try {
                offset_polygons = arrange_offset_polygons_2(CGAL::create_offset_polygons_2<Polygon_2>(inset,*iss) );
            } catch (...) { had_exception = 1;};
            
            if (had_exception)
                inset = inset + 0.00001;
        }
        
        if (level == 0 && want_skeleton_path) {

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

//            if (!tool->is_optional)
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
 
        td->toollevels.push_back(tool);       
        if (level == 0 || want_optional)
            inset += stepover / 2;
        else
            inset += stepover;
        if (inset > end_inset)
            break;
        level ++;
        
    } while (1);
    if (skeleton.size() > 0 && want_skeleton_path) {
        class toollevel *tool = new(class toollevel);       
        tool->level = level;
        tool->is_slotting = true;
        tool->name = "Bisector Slotting";
        tool->diameter = diameter;
        tool->depth = depth;
        tool->minY = minY;
        td->toollevels.push_back(tool);
        for (auto ss : skeleton) {
            for (auto x = ss->halfedges_begin(); x != ss->halfedges_end(); ++x) {
                    if (x->is_inner_bisector()) {
                        double X1, Y1, X2, Y2;
                        X1 = point_snap(x->vertex()->point().x());
                        Y1 = point_snap(x->vertex()->point().y());
                        X2 = point_snap(x->opposite()->vertex()->point().x());
                        Y2 = point_snap(x->opposite()->vertex()->point().y());
                        if (X1 != X2 || Y1 != Y2) {
                            Polygon_2 *p = new(Polygon_2);
                            p->push_back(Point(X1, Y1));
                            p->push_back(Point(X2, Y2));
                            tool->add_poly(p, false);
                        }
                    }
            } 
        }
    }
}


static double radius_to_depth(double r, double angle)
{
    return -r / tan(angle/360.0 * M_PI);
}

static double depth_to_radius(double d, double angle)
{
    return fabs(d) * tan(angle/360.0 * M_PI);
}

void inputshape::create_toolpaths_vcarve(int toolnr)
{
    double angle = get_tool_angle(toolnr);
    if (!polyhole) {
        polyhole = new PolygonWithHoles(poly);
        for (auto i : children)
          polyhole->add_hole(i->poly);
    }
    
    printf("VCarve toolpath\n");
    
    if (!iss) {
        iss =  CGAL::create_interior_straight_skeleton_2(*polyhole);
    }
    
    class tooldepth * td = new(class tooldepth);
    tooldepths.push_back(td);
    td->toolnr = toolnr;
    
    class toollevel *tool = new(class toollevel);       
    tool->name = "VCarve path";
    tool->toolnr = toolnr;
    tool->minY = minY;
    td->toollevels.push_back(tool);
    
    for (auto x = iss->halfedges_begin(); x != iss->halfedges_end(); ++x) {
            double X1, Y1, X2, Y2, d1, d2;
            X1 = point_snap2(x->vertex()->point().x());
            Y1 = point_snap2(x->vertex()->point().y());
            d1 = distance_from_edge(X1, Y1);
            
            X2 = point_snap2(x->opposite()->vertex()->point().x());
            Y2 = point_snap2(x->opposite()->vertex()->point().y());
            d2 = distance_from_edge(X2, Y2);
            if (x->is_bisector()) {
                if (X1 != X2 || Y1 != Y2) {
                            Polygon_2 *p = new(Polygon_2);
                            p->push_back(Point(X1, Y1));
                            p->push_back(Point(X2, Y2));
                            tool->add_poly_vcarve(p, radius_to_depth(d1, angle), radius_to_depth(d2, angle));
                }
            }
    }
}


void inputshape::consolidate_toolpaths(bool want_inbetween_paths)
{
    unsigned int level;

    if (want_inbetween_paths) {  
      for (auto td : tooldepths) {  
        /* step 1 : eliminate redundant optional paths */	
        /* first for not holes */
        for (level = 0; level + 1 < td->toollevels.size(); level++) {
            if (!td->toollevels[level]->is_optional)
                continue;
            for (auto tp : td->toollevels[level]->toolpaths) {
                double len = 0.0;
                double target = 0.0;
                if (tp->is_hole)
                    continue;
                /* for each toolpath, check if there is a toolpath a level up that is inside this tp */
                for (auto tp2 : td->toollevels[level + 1]->toolpaths) {
                    if (tp2->fits_inside(tp)) {
                        len = len + tp2->length;
                    }
                }
                target = tp->length - 8 * tp->diameter;
                if (target <=0)
                    target = 0.0;
                if (len > target)               
                        tp->polygons.clear();
                
            } 
            for (unsigned int j = 0; j < td->toollevels[level]->toolpaths.size(); j++) {
                if (td->toollevels[level]->toolpaths[j]->polygons.size() == 0) {
                    td->toollevels[level]->toolpaths.erase(td->toollevels[level]->toolpaths.begin() + j);
                    j--;
                }
            }
        }
    

        /* then for holes where "inside" is in the opposite direction */
        for (level = 1; level  < td->toollevels.size(); level++) {
            if (!td->toollevels[level]->is_optional)
                continue;
            for (auto tp : td->toollevels[level]->toolpaths) {
                double len = 0.0;
                double target = 0.0;
                if (!tp->is_hole)
                    continue;
            /* for each toolpath, check if there is a toolpath a level up that is inside this tp */
                for (auto tp2 : td->toollevels[level - 1]->toolpaths) {
                    if (tp2->fits_inside(tp)) {
                        len = len + tp2->length;
                    }
                }               
                target = tp->length - 8 * tp->diameter;
                if (target <=0)
                    target = 0.0;
                if (len > target)               
                        tp->polygons.clear();
            } 
            for (unsigned int j = 0; j < td->toollevels[level]->toolpaths.size(); j++) {
                if (td->toollevels[level]->toolpaths[j]->polygons.size() == 0) {
                    td->toollevels[level]->toolpaths.erase(td->toollevels[level]->toolpaths.begin() + j);
                    j--;
                }
            }
        }
    
 
        /* step 2 : remove empty levels */
        for (level = 0; level + 1 < td->toollevels.size(); level++) {
            if (!td->toollevels[level]->is_optional)
                continue;
            if (td->toollevels[level]->toolpaths.size() == 0) {
                td->toollevels.erase(td->toollevels.begin() + level);
                level--;
            }
        }
      }
    }
    /* step 3 : consolidate outer "rings" into inner rings */
   for (auto td : tooldepths) {  
     for (level = 0; level + 1 < td->toollevels.size(); level++) {
        for (auto tp : td->toollevels[level]->toolpaths) {
            class toolpath *match = NULL;
            int valid = 1;
            /* for each toolpath, check if there is exactly one toolpath a level up that is inside this tp */
            for (auto tp2 : td->toollevels[level + 1]->toolpaths) {
                if (tp2->fits_inside(tp)) {
                    if (match != NULL)
                        valid = 0;
                    match = tp2;
                }
            }   
            
            if (valid && match && tp->is_hole == match->is_hole && tp->is_slotting == match->is_slotting && tp->is_optional == match->is_optional && tp->toolnr == match->toolnr) {
                for (auto p: tp->polygons) {
                    match->add_polygon(p);
                }
                tp->polygons.clear();
            }         
        }        
      }
    }
}


double inputshape::distance_from_edge(double X, double Y)
{
    double bestdist = 1000000000;
    for(auto p = poly.vertices_begin() ; p != poly.vertices_end() ; ++ p ) {
        double d = dist(X, Y, p->x(), p->y());
        if (d < bestdist)
            bestdist= d;
    }
    for (auto c : children) {
        double d = c->distance_from_edge(X, Y);
        if (d < bestdist)
            bestdist = d;
    }
    return bestdist;
}

void inputshape::set_name(const char *n)
{
    name = strdup(n);
}

void inputshape::set_minY(double mY)
{
    minY = mY;
}


class scene * inputshape::scene_from_vcarve(class scene *input, double depth, int toolnr)
{
    class scene *scene;
    double angle = get_tool_angle(toolnr);
    
    if (input)
        scene = input;
    else
        scene = new(class scene);
    scene->set_filename("from scene_from_vcarve");    
    
    /* step 1: clone our poly into the new scene */
    for(auto vi = poly.vertices_begin() ; vi != poly.vertices_end() ; ++ vi ) {
        scene->add_point_to_poly(vi->x(), vi->y());
    }
    scene->end_poly();
    
    /* step 2: and clone all our children as well, but instruct them not to inset by setting toolnr to 0 */
    for (auto i : children)
        scene = i->scene_from_vcarve(scene, 0.0, 0);

        
    if (depth != 0.0 && toolnr > 0) {
        /* step 3: create an inset path */
        double inset;
        int had_exception = 1;
    
        if (!polyhole) {
            polyhole = new PolygonWithHoles(poly);
            for (auto i : children)
            polyhole->add_hole(i->poly);
        }
    
        if (!iss) {
            iss =  CGAL::create_interior_straight_skeleton_2(*polyhole);
        }
    
        /* first inset is the diameter of the Vcutter at the point of max depth */
        inset = 2 * depth_to_radius(depth, angle);
    
        PolygonWithHolesPtrVector  offset_polygons;

        while (had_exception) {
            had_exception = 0;
            try {
                offset_polygons = arrange_offset_polygons_2(CGAL::create_offset_polygons_2<Polygon_2>(inset,*iss) );
            } catch (...) { had_exception = 1;};
            
            if (had_exception)
                inset = inset + 0.00001;
        }
        
        for (auto ply : offset_polygons) {
            Polygon_2 *p;
            p = new(Polygon_2);
            *p = ply->outer_boundary();

            for(auto vi = p->vertices_begin() ; vi != p->vertices_end() ; ++ vi ) {
                scene->add_point_to_poly(vi->x(), vi->y());
            }
            scene->end_poly();

            for(auto hi = ply->holes_begin() ; hi != ply->holes_end() ; ++ hi ) {
                for(auto vi = hi->vertices_begin() ; vi != hi->vertices_end() ; ++ vi ) {
                    scene->add_point_to_poly(vi->x(), vi->y());
                }
                scene->end_poly();
            }
        }
        
        /* step 4: add the inset paths to the new scene */
    }
    
    return scene;
}
