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
    if (is_hole)
        color = "red";
    if (is_slotting)
        color = "pink";
    for (auto i : polygons) {
        print_polygon(i, color, inch_to_px(diameter) * 0.125);
#if 1
        if (start_vertex < i->size() && i->size() > 2) {
          svg_circle((*i)[start_vertex].x(), (*i)[start_vertex].y(), inch_to_px(diameter/2), "red", 0.5);
        }
#endif
    }
}

void toolpath::output_gcode(void)
{
  double speed = 1.0;
  double lX = -100000;
  double lY = -100000;
  if (is_slotting) {
    output_gcode_slotting();
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
      
      if (i == 0 && dist(lX,lY, vi.x(), vi.y()) < inch_to_px(diameter * 0.75)) {
        gcode_mill_to(px_to_mm(vi.x()), px_to_mm(vi.y() - get_minY()), inch_to_mm(depth), speed * 0.667);
      }
      gcode_conditional_travel_to(px_to_mm(vi.x()), px_to_mm(vi.y() - get_minY()), inch_to_mm(depth), speed);
      gcode_mill_to(px_to_mm(vi2.x()), px_to_mm(vi2.y() - get_minY()), inch_to_mm(depth), speed);
      lX = vi2.x();
      lY = vi2.y();
    }
  }
}

void toolpath::output_gcode_slotting(void)
{
  double speed = 0.666;
    
  for (auto poly : polygons) {
    if (poly->size() == 2) {
      double distance1, distance2;
      distance1 = dist(gcode_current_X(), gcode_current_Y(), px_to_mm((*poly)[0].x()), px_to_mm((*poly)[0].y() - get_minY()));
      distance2 = dist(gcode_current_X(), gcode_current_Y(), px_to_mm((*poly)[1].x()), px_to_mm((*poly)[1].y() - get_minY()));
      
      if (distance1 < distance2) {
         gcode_conditional_travel_to(px_to_mm((*poly)[0].x()), px_to_mm((*poly)[0].y() - get_minY()), inch_to_mm(depth), speed);
         gcode_mill_to(px_to_mm((*poly)[1].x()), px_to_mm((*poly)[1].y() - get_minY()), inch_to_mm(depth), speed);
      } else {
         gcode_conditional_travel_to(px_to_mm((*poly)[1].x()), px_to_mm((*poly)[1].y() - get_minY()), inch_to_mm(depth), speed);
         gcode_mill_to(px_to_mm((*poly)[0].x()), px_to_mm((*poly)[0].y() - get_minY()), inch_to_mm(depth), speed);
      }
      return;
    }
    for(auto vi = poly->vertices_begin() ; vi != poly->vertices_end() ; ++ vi ) {
      auto vi2 = vi+1;
      if (vi2 == poly->vertices_end())
        vi2 = poly->vertices_begin();
        
      gcode_conditional_travel_to(px_to_mm(vi->x()), px_to_mm(vi->y() - get_minY()), inch_to_mm(depth), speed);
      gcode_mill_to(px_to_mm(vi2->x()), px_to_mm(vi2->y() - get_minY()), inch_to_mm(depth), speed);
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
        
        if (is_hole)
          phi = -phi;
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
