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


static vector<class inputshape *> shapes;

static class inputshape *shape;
static double minX = 10000, minY = 10000, maxY = -10000, maxX = -10000;



static bool compare_shape(class inputshape *A, class inputshape *B)
{
  return (fabs(A->area) < fabs(B->area));
}


double get_minX(void)
{
  return minX;
}
double get_minY(void)
{
  return minY;
}
double get_maxY(void)
{
  return maxY;
}

void declare_minY(double Y)
{
  minY = fmin(Y, minY);
}

void new_poly(double X, double Y)
{
  end_poly();

  shape = new(class inputshape);
  
  add_point_to_poly(X, Y);
}

void add_point_to_poly(double X, double Y)
{
  if (!shape)
    shape = new(class inputshape);
    
  shape->add_point(X, Y);
  minX = fmin(X, minX);
  minY = fmin(Y, minY);
  maxX = fmax(X, maxX);
  maxY = fmax(Y, maxY);
}

void end_poly(void)
{
  if (shape) {
    shape->close_shape();
    shapes.push_back(shape);
  }
  shape = NULL;
  sort(shapes.begin(), shapes.end(), compare_shape);
}


void write_svg(const char *filename)
{

  printf("Work size: %5.2f x %5.2f inch\n", mm_to_inch(maxX-minX), mm_to_inch(maxY - minY));
  set_svg_bounding_box(minX, minY, maxX, maxY);
  write_svg_header(filename, 1.0);

  for (auto i : shapes) {
    i->print_as_svg();
  }
  write_svg_footer();
}

void write_gcode(const char *filename)
{
  write_gcode_header(filename);
  
  for (auto i : shapes) {
    i->output_gcode();
  }
  
  write_gcode_footer();
}


/* input: arbitrary nested vector of shapes */
/* output: odd/even split, max nesting level is 1 */
static void flatten_nesting(void)
{
  unsigned int i, j, z;
  for (i = 0; i < shapes.size(); i++) {
    for (j = 0; j < shapes[i]->children.size(); j++) {
      for (z = 0; z < shapes[i]->children[j]->children.size(); z++) {
        shapes.push_back(shapes[i]->children[j]->children[z]);
      }
      shapes[i]->children[j]->wipe_children();
    }
  }
  
}

/* input: a vector of shapes, output: nested shapes are properly parented */
/* This is an O(N^2) algorithm for now */
void process_nesting(void)
{
  unsigned int i;
  /* first sort so that the vector is smallest first */
  /* invariant thus is that a shape can only be inside later shapes in the vector */
  /* in order of nesting */
  sort(shapes.begin(), shapes.end(), compare_shape);

  for (i = 0; i < shapes.size(); i++) {
    unsigned int j;
    for (j = i + 1; j < shapes.size(); j++) {
      if (shapes[i]->fits_inside(shapes[j])) {
        shapes[j]->add_child(shapes[i]);
        shapes.erase(shapes.begin() + i);
        i--;
        break;
      }
    }
  }
  
  flatten_nesting();

  for (i = 0; i < shapes.size(); i++) {
      shapes[i]->set_level(0);
      shapes[i]->fix_orientation();
  }
}

void create_toolpaths(int tool, double depth)
{
  double currentdepth = depth;
  double depthstep;
  double surplus;
  int finish = 0;
  activate_tool(tool);
  
  depthstep = get_tool_maxdepth();

  int rounds = (int)ceilf(-depth / depthstep);
  surplus = rounds * depthstep + depth;
  
  /* if we have spare height, split evenly between first and last cut */
  depthstep = depthstep - surplus / 2;


  if (get_finishing_pass()) {
    /* finishing rules: deepest cut is small */
    depthstep = fmin(depthstep, 0.25);
    finish  = 1;
  }

  while (currentdepth < 0) {
    for (auto i : shapes)
      i->create_toolpaths(currentdepth, finish, want_inbetween_paths);
    currentdepth += depthstep;
    depthstep = get_tool_maxdepth();
    if (finish)
      finish = -1;
  }
}

void consolidate_toolpaths()
{
  for (auto i : shapes)
    i->consolidate_toolpaths();

}

