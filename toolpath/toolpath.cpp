/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include "tool.h"
#include "print.h"
static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

void toolpath::print_as_svg(const char *color)
{
    double width = diameter * 0.333;
//    if (is_hole)
//        color = "red";
//    if (is_slotting)
//        color = "pink";
//    if (is_optional) {
//        color = "purple";
//    }
    for (auto i : polygons) {
        print_polygon(i, color, width);
        for (unsigned int j = 0; j < i->size(); j++)
          svg_circle((*i)[j].x(), (*i)[j].y(), width/2, color, 0);
#if 0
        if (start_vertex < i->size() && i->size() > 2) {
          svg_circle((*i)[start_vertex].x(), (*i)[start_vertex].y(), diameter/2, "red", 0.5);
        }
#endif
    }
}

void toolpath::output_gcode(void)
{
  double speed = 1.0;
  double lX = -100000;
  double lY = -100000;
  if (is_vcarve) {
    output_gcode_vcarve();
    return;
  }
  if (is_slotting) {
    output_gcode_slotting();
    return;
  }
  
  if (run_reverse) {
    output_gcode_reverse();
    return;
  }
    
  for (auto poly : polygons) {
    unsigned int i;
    for (i = 0; i < poly->size(); i++) {
      int current;
      int next;
      current = (i + start_vertex) % poly->size();
      next = (current + 1) % poly->size();
      
      auto vi = (*poly)[current];
      auto vi2 = (*poly)[next];
      
      if (i == 0 && dist(lX,lY, vi.x(), vi.y()) < diameter * 0.75) {
        gcode_mill_to(vi.x(), vi.y() - get_minY(), depth, speed * 0.5);
      }
      gcode_conditional_travel_to(vi.x(), vi.y() - get_minY(), depth, speed);
      gcode_mill_to(vi2.x(), vi2.y() - get_minY(), depth, speed);
      lX = vi2.x();
      lY = vi2.y();
    }
    speed = 1.0;
  }
}

void toolpath::output_gcode_vcarve(void)
{
  double speed = 1.0;
    
  for (auto poly : polygons) {
    if (depth > depth2) {
      gcode_vconditional_travel_to((*poly)[0].x(), (*poly)[0].y() - get_minY(), depth, speed);
      gcode_vmill_to((*poly)[1].x(), (*poly)[1].y() - get_minY(), depth2, speed);
    } else {
      gcode_vconditional_travel_to((*poly)[1].x(), (*poly)[1].y() - get_minY(), depth2, speed);
      gcode_vmill_to((*poly)[0].x(), (*poly)[0].y() - get_minY(), depth, speed);
    }
    speed = 1.0;
  }
}

void toolpath::output_gcode_reverse(void)
{
  double speed = 1.0;
  double lX = -100000;
  double lY = -100000;
  
  if (level == 0)
    speed = 0.5;
    
  for (auto ppoly =  polygons.rbegin(); ppoly != polygons.rend(); ++ppoly) {  
    auto poly = *ppoly;
    unsigned int i;
    for (i = 0; i < poly->size(); i++) {
      int current;
      int next;
      current = (i + start_vertex) % poly->size();
      next = (current + 1) % poly->size();
      
      auto vi = (*poly)[current];
      auto vi2 = (*poly)[next];
      
      if (i == 0 && dist(lX,lY, vi.x(), vi.y()) < diameter * 0.75) {
        gcode_mill_to(vi.x(), vi.y() - get_minY(), depth, speed * 0.5);
      }
      gcode_conditional_travel_to(vi.x(), vi.y() - get_minY(), depth, speed);
      gcode_mill_to(vi2.x(), vi2.y() - get_minY(), depth, speed);
      lX = vi2.x();
      lY = vi2.y();
    }
    speed = 1.0;
  }
}

void toolpath::output_gcode_slotting(void)
{
  double speed = 0.5;
    
  for (auto poly : polygons) {
    if (poly->size() == 2) {
      double distance1, distance2;
      distance1 = dist(gcode_current_X(), gcode_current_Y(), (*poly)[0].x(), (*poly)[0].y() - get_minY());
      distance2 = dist(gcode_current_X(), gcode_current_Y(), (*poly)[1].x(), (*poly)[1].y() - get_minY());
      
      if (distance1 < distance2) {
         gcode_conditional_travel_to((*poly)[0].x(), (*poly)[0].y() - get_minY(), depth, speed);
         gcode_mill_to((*poly)[1].x(), (*poly)[1].y() - get_minY(), depth, speed);
      } else {
         gcode_conditional_travel_to((*poly)[1].x(), (*poly)[1].y() - get_minY(), depth, speed);
         gcode_mill_to((*poly)[0].x(), (*poly)[0].y() - get_minY(), depth, speed);
      }
      return;
    }
    for(auto vi = poly->vertices_begin() ; vi != poly->vertices_end() ; ++ vi ) {
      auto vi2 = vi+1;
      if (vi2 == poly->vertices_end())
        vi2 = poly->vertices_begin();
        
      gcode_conditional_travel_to(vi->x(), vi->y() - get_minY(), depth, speed);
      gcode_mill_to(vi2->x(), vi2->y() - get_minY(), depth, speed);
    }
  }
}

double toolpath::distance_from(double X, double Y)
{
  double d = 100000000000;
  
  if (is_slotting) {
    for (auto poly : polygons) {
      for (auto vi = poly->vertices_begin() ; vi != poly->vertices_end() ; ++ vi ) {
          double di;
          di = dist(X, Y, vi->x(), vi->y());
          d = fmin(d, di);
      }
    }
  } else {
    for (auto poly : polygons) {
      auto vi = (*poly)[start_vertex];
      double di;
      di = dist(X, Y, vi.x(), vi.y());
      d = fmin(d, di);
    }
  }
  return d;
}

void toolpath::add_polygon(Polygon_2 *poly)
{
    polygons.push_back(poly);
}

bool toolpath::fits_inside(class toolpath *shape)
{
   for (auto spoly : shape->polygons) {
      for (auto poly : polygons) {
       for(auto vi = poly->vertices_begin() ; vi != poly->vertices_end() ; ++ vi ) {
          if (spoly->bounded_side(*vi) == CGAL::ON_UNBOUNDED_SIDE) {
             return false;
          }
       }  

     }
   }
   return true;
}


void toolpath::recalculate()
{
    /* figure out which vertex is the favorite point to start cutting.*/
    /* criteria prio 1: Largest angle wins */
    /* tie breaker: maximize the shortest length of the two vectors to the point */
    
    unsigned a, b, c;
    unsigned int i;
    
    
    int best_i = -10;
    double angle_i = -400;
    double length_i = -400;
    
    length = 0.0;
    for (i = 0; i < polygons[0]->size(); i++) {
      unsigned int next = i + 1;
      if (next >= polygons[0]->size())
        next = 0;
      length = length + dist( (*polygons[0])[i].x(), (*polygons[0])[i].y(), (*polygons[0])[next].x(), (*polygons[0])[next].y());
    }
    
    for (i = 0; i < polygons[0]->size(); i++) {
        double BC_x, BC_y, BA_x, BA_y;
        double BC_l, BA_l;
        double phi;
        if (i == 0)
          a = polygons[0]->size() - 1;
        else
          a = i - 1;
        b = i;
        c = i + 1;
        if (c >= polygons[0]->size())
          c = 0;
          
        BA_x = (*polygons[0])[a].x() - (*polygons[0])[b].x();
        BA_y = (*polygons[0])[a].y() - (*polygons[0])[b].y();

        BC_x = (*polygons[0])[c].x() - (*polygons[0])[b].x();
        BC_y = (*polygons[0])[c].y() - (*polygons[0])[b].y();
        
        BA_l = sqrt(BA_x * BA_x + BA_y * BA_y);
        BC_l = sqrt(BC_x * BC_x + BC_y * BC_y);
        
        phi = acos( (BA_x * BC_x + BA_y * BC_y) / (BA_l * BC_l));
        
//        if (!is_hole)
//          phi = -phi;
        if ( (phi > angle_i) || ( (phi == angle_i) && (length_i < fmin(BA_l,BC_l)))) {
            angle_i = phi;
            best_i = i;
            length_i = fmin(BA_l, BC_l);
        }
    }
//    printf("BEst vertex is %i with angle %5.2f\n", best_i, angle_i);
    if (best_i >= 0)
      start_vertex = best_i;
}
