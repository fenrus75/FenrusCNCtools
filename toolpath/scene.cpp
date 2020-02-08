/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include "tool.h"

#include "scene.h"

extern "C" {
  #include "toolpath.h"
}


static inline double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

static bool compare_shape(class inputshape *A, class inputshape *B)
{
  return (fabs(A->area) < fabs(B->area));
}


double scene::get_minX(void)
{
  return minX;
}
double scene::get_minY(void)
{
  return minY;
}
double scene::get_maxY(void)
{
  return maxY;
}

void scene::declare_minY(double Y)
{
  minY = fmin(Y, minY);
}

void scene::new_poly(double X, double Y)
{
  end_poly();

  shape = new(class inputshape);
  shape->parent = this;
  shape->set_z_offset(z_offset);
  shape->set_stock_to_leave(stock_to_leave);
  
  add_point_to_poly(X, Y);
}

void scene::set_poly_name(const char *n)
{
  if (!shape) {
    shape = new(class inputshape);
    shape->parent = this;
	shape->set_z_offset(z_offset);
	shape->set_stock_to_leave(stock_to_leave);
  }
  shape->set_name(n);
}

void scene::add_point_to_poly(double X, double Y)
{
  if (!shape) { 
    shape = new(class inputshape);
    shape->parent = this;
	shape->set_z_offset(z_offset);
	shape->set_stock_to_leave(stock_to_leave);
  }
    
  shape->add_point(X, Y);
  minX = fmin(X, minX);
  minY = fmin(Y, minY);
  maxX = fmax(X, maxX);
  maxY = fmax(Y, maxY);
}

void scene::end_poly(void)
{
  if (shape) {
    shape->close_shape();
    shapes.push_back(shape);
  }
  shape = NULL;
  sort(shapes.begin(), shapes.end(), compare_shape);
}

void scene::push_tool(int toolnr)
{
	if (has_tool(toolnr))
		return;
	toollist.push_back(toolnr);
}

void scene::drop_tool(int toolnr)
{
	for (unsigned int i = 0; i < toollist.size(); i++) {
		if (toollist[i] == toolnr) {
			toollist.erase(toollist.begin() + i);
			return;
		}
	}
}

bool scene::has_tool(int toolnr)
{
	for (unsigned int i = 0; i < toollist.size(); i++) {
		if (toollist[i] == toolnr) {
			return true;
		}
	}
	return false;
}

void scene::set_default_tool(int toolnr)
{
  if (toollist.size() > 0)
    return;
  toollist.push_back(toolnr);
}

void scene::write_naked_svg()
{

  for (auto i : shapes) {
    i->print_as_svg();
  }
}

void scene::write_svg(const char *filename)
{

  vprintf("Work size: %5.2f x %5.2f inch\n", mm_to_inch(maxX), mm_to_inch(maxY-minY));
// printf("%5.2f,%5.2f  x %5.2f, %5.2f\n", minX,minY, maxX,maxY);
  set_svg_bounding_box(minX, minY, maxX, maxY);
  write_svg_header(filename, 1.0);

  write_naked_svg();
  for (auto i : shapes) {
    i->print_as_svg();
  }
  write_svg_footer();
}

void scene::write_naked_gcode()
{
  unsigned int j;
  unsigned int start = 0;

  if (tool_is_vcarve(toollist[0])) {
	  gcode_tool_change(toollist[1]);
	  start = 1;
  } else {
	  gcode_tool_change(toollist[0]);
  }
		
  
  for (j = start; j < toollist.size() ; j++) {
    for (auto i : shapes) {
      i->output_gcode(toollist[j]);
    }
	if (cutout) {
		gcode_reset_current();
		cutout->output_gcode(toollist[j]);
	}
    if (j < toollist.size() - 1) {
          gcode_tool_change(toollist[j + 1]);
    }
    
  }

  if (tool_is_vcarve(toollist[0])) {
      gcode_tool_change(toollist[0]);
      for (auto i : shapes) {
        i->output_gcode(toollist[0]);
      }
  }
}


void scene::write_gcode(const char *filename, const char *description)
{
  printf("Writing gcode for %s to %s\n", description, filename);
  gcode_reset_current();
  activate_tool(toollist[0]);
  write_gcode_header(filename);

  write_naked_gcode();
  
  write_gcode_footer();
}


/* input: arbitrary nested vector of shapes */
/* output: odd/even split, max nesting level is 1 */
void scene::flatten_nesting(void)
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
void scene::process_nesting(void)
{
  unsigned int i;
  /* first sort so that the vector is smallest first */
  /* invariant thus is that a shape can only be inside later shapes in the vector */
  /* in order of nesting */
  sort(shapes.begin(), shapes.end(), compare_shape);

  /* if we are doing a cutout we need to remove the biggest shape, that's the cutout */
  if (cutout_depth != 0 && cutout == NULL) {
	  i = shapes.size() - 1;
	  cutout = shapes[i];
	  shapes.erase(shapes.begin() + i);
	  cutout->is_cutout = true;
  }

  vprintf("Scene has %i objects\n", (int)shapes.size());


  for (i = 0; i < shapes.size(); i++) {
    unsigned int j;
    for (j = i + 1; j < shapes.size(); j++) {
      if (shapes[i]->fits_inside(shapes[j])) {
//        printf("Adding shape %s (%5.2f) to %s (%5.2f) \n", shapes[i]->name, shapes[i]->area, shapes[j]->name, shapes[j]->area);
        shapes[j]->add_child(shapes[i]);
        shapes.erase(shapes.begin() + i);
        i--;
        break;
      }
    }
  }
  
  flatten_nesting();

  for (i = 0; i < shapes.size(); i++) {
      shapes[i]->set_minY(get_minY());
      shapes[i]->set_level(0);
      shapes[i]->fix_orientation();
  }
}

void scene::create_toolpaths(void)
{
  double currentdepth;
  double depthstep;
  double surplus;
  int finish = 0;
  int toolnr = 0;
  int tool;

  depth = -fabs(depth);

  printf("Creating toolpath for depth %5.2f with offset %5.2f\n", depth, z_offset);

  if (want_inlay()) {
		inlay_plug = clone_scene(NULL, 1, maxX);
	    inlay_plug->set_depth(depth / 1.25);
		inlay_plug->create_toolpaths(); 
  }
  
  vprintf("create_toolpaths with depth %5.2f\n", depth);

  /* cutout toolpath first */
	if (cutout) {
		int toolnr = 0;
		if (tool_is_vcarve(toollist[0]) && toollist.size() > 1)
			toolnr = 1;
		cutout->create_toolpaths_cutout(toollist[toolnr], - fabs(cutout_depth), want_finishing_pass());
	}  
  
  tool = toollist.size() -1;
  while (tool >= 0) {
    double start, end;
    currentdepth = depth;
    toolnr = toollist[tool];  
    activate_tool(toolnr);
  
    depthstep = get_tool_maxdepth();

    int rounds = (int)ceilf(-depth / depthstep);
    surplus = rounds * depthstep + depth;
  
    /* if we have spare height, split evenly between first and last cut */
    depthstep = depthstep - surplus / 2;


    if (want_finishing_pass() && !tool_is_vcarve(toollist[tool])) {
      /* finishing rules: deepest cut is small, 2x stock_to_leave */
      depthstep = fmin(depthstep, stock_to_leave * 2);
      finish  = 1;
    }
    
    start = 0;
    end = 60000000;
    
    /* we want courser tools to not get within the stepover of the finer tool */
	/* but actually that's a mess, we'll leave 0.1mm stock to leave and that's it */
    if (tool < (int)toollist.size() -1)
//      start = get_tool_stepover(toollist[tool+1]);
		start = 0.1;
    
    /* if tool 0 is a vcarve bit, tool 1 needs to start at radius at depth of cut */
    /* and all others need an offset */
    if (tool > 0 and tool_is_vcarve(toollist[0])) {
      double angle = get_tool_angle(toollist[0]);
      if (tool == 1)
        start = 0;
      start += depth_to_radius(depth, angle); 
    }
      
    if (tool > 0)
      end = tool_diam(toollist[tool-1])/2 + 0.2;
      
    if (tool == 1 && tool_is_vcarve(toollist[0]))
      end = 600000000;
      
      
    if (tool_is_vcarve(toolnr) && tool == 0) {
		double stock_to_leave = 0;
		while (currentdepth <= -z_offset) {
          	for (auto i : shapes)
            	i->create_toolpaths_vcarve(toolnr, currentdepth, stock_to_leave);
			currentdepth += depthstep;
			depthstep = get_tool_maxdepth();
			if (want_finishing_pass())
				stock_to_leave = 0.1;
        }
    } else {
      vprintf("Tool %i goes from %5.2f mm to %5.2f mm\n", toolnr, start, end);
	  bool inbetween = want_inbetween_paths();
      while (currentdepth < - z_offset - 0.00000001) {
	    
	    	/* we want courser tools to not get within the stepover of the finer tool */
		    if (tool < (int)toollist.size() -1)
	      start = get_tool_stepover(toollist[tool+1]);
    
		    /* if tool 0 is a vcarve bit, tool 1 needs to start at radius at depth of cut */
		    /* and all others need an offset */
		    if (tool > 0 and tool_is_vcarve(toollist[0])) {
		      double angle = get_tool_angle(toollist[0]);
		      if (tool == 1)
		        start = 0;
		      start += depth_to_radius(currentdepth, angle);
		    }


        for (auto i : shapes)
          i->create_toolpaths(toolnr, currentdepth, finish, inbetween, start, end, _want_skeleton_paths);
        currentdepth += depthstep;
        depthstep = get_tool_maxdepth();
        if (finish)
          finish = -1;
		inbetween = false;
      }
    }
    
    tool--;
  }
  consolidate_toolpaths();
}
void scene::consolidate_toolpaths(void)
{
  for (auto i : shapes)
    i->consolidate_toolpaths(_want_inbetween_paths);

}

void scene::enable_finishing_pass(void)
{
  _want_finishing_pass = true;
}

bool scene::want_finishing_pass(void)
{
  return _want_finishing_pass;
}

void scene::enable_inbetween_paths(void)
{
  _want_inbetween_paths = true;
}

bool scene::want_inbetween_paths(void)
{
  return _want_inbetween_paths;
}

void scene::enable_skeleton_paths(void)
{
  _want_skeleton_paths = true;
}

bool scene::want_skeleton_paths(void)
{
  return _want_skeleton_paths;
}


scene::scene(const char *filename)
{
       minX = 1000000;
       minY = 1000000;
       maxX = -1000000;
       maxY = -1000000;
       _want_finishing_pass = false;
       _want_inbetween_paths = false;
       _want_skeleton_paths = false;
       shape = NULL;
       filename = strdup(filename);
       parse_svg_file(this, filename);
}

double scene::distance_from_edge(double X, double Y, bool exclude_zero)
{
  double d = 1000000000;
  for (auto i : shapes) {
       d = fmin(d, i->distance_from_edge(X, Y, exclude_zero));
  }
  return d;
}

static Polygon_2 *cutout_clone_split(Polygon_2 *poly, double threshold)
{
	Polygon_2 *clone = NULL;

	clone = new(Polygon_2);

	for (unsigned int i = 0; i < poly->size(); i++) {
		unsigned int next = i + 1;
		double X1, X2, Y1, Y2;
		if (next >= poly->size())
			next = 0;

		X1 = CGAL::to_double((*poly)[i].x());
		Y1 = CGAL::to_double((*poly)[i].y());
		X2 = CGAL::to_double((*poly)[next].x());
		Y2 = CGAL::to_double((*poly)[next].y());

		double d = dist(X1,Y1,X2,Y2);

		clone->push_back(Point(X1, Y1));
		
		if (d > threshold) {
				clone->push_back(Point((X1+X2)/2, (Y1+Y2)/2));
		}
	}
	return clone;
}


static double poly_length(Polygon_2 *poly)
{
	double len = 0.0;

//	return CGAL::to_double(poly->area());

	for (unsigned int i = 0; i < poly->size(); i++) {
		unsigned int next = i + 1;
		double X1, X2, Y1, Y2;
		if (next >= poly->size())
			next = 0;

		X1 = CGAL::to_double((*poly)[i].x());
		Y1 = CGAL::to_double((*poly)[i].y());
		X2 = CGAL::to_double((*poly)[next].x());
		Y2 = CGAL::to_double((*poly)[next].y());

		len += dist(X1,Y1,X2,Y2);
	}

	return len;
}

static Polygon_2 *clone_mutate(Polygon_2 *poly, double alpha)
{
	unsigned int target;

	Polygon_2 *clone = NULL;

	clone = new(Polygon_2);
	
	target = rand() % poly->size();

	double dX = 0;
	double dY = 0;

	int element = rand() % 8;
	switch (element) {
		case 0:
			dX = alpha;
			break;
		case 1:
			dX = -alpha;
			break;
		case 2:
			dY = alpha;
			break;
		case 3:
			dY = -alpha;
			break;

		case 4:
			dX = alpha/1.4;
			dY = alpha/1.4;
			break;
		case 5:
			dX = -alpha/1.4;
			dY = -alpha/1.4;
			break;
		case 6:
			dX = -alpha/1.4;
			dY = alpha/1.4;
			break;
		case 7:
			dX = alpha/1.4;
			dY = -alpha/1.4;
			break;
	}


	for (unsigned int i = 0; i < poly->size(); i++) {
		double X1, Y1;

		X1 = CGAL::to_double((*poly)[i].x());
		Y1 = CGAL::to_double((*poly)[i].y());

		if (i == target) {
			X1 += dX;
			Y1 += dY;
		}

		clone->push_back(Point(X1, Y1));
	}
	return clone;
}

static bool poly_valid(Polygon_2 *poly, class scene *scene)
{
	if (!poly->is_simple())
		return false;
	return scene->fits_inside(poly);
}


#define ITER 800
#define ALPHA 2
static Polygon_2 * optimize_bb(class scene *scene, Polygon_2 *boundingbox, bool quick, double best_bb_length) 
{
	int maxiter = ITER;
	int i = ITER;
	double alpha;
	time_t target;

	alpha = ALPHA;

	target = time(NULL) + 5;

	if (quick) {
		maxiter = ITER * 2;
		i = maxiter;
		target = time(NULL) + 2;
		alpha = 1;
	}


	while (i > 0) {
		Polygon_2 *next;
		double len;
		i--;

		if (time(NULL) > target)
			break;

		next = clone_mutate(boundingbox, alpha);
		alpha = alpha * 0.99;
		len = poly_length(next);

		if (best_bb_length <= len) {
			delete next;
			continue;
		}
		if (!poly_valid(next, scene)) {
			delete next;
			continue;
		}
		delete boundingbox;
		boundingbox = next;
		if (len <= best_bb_length || (rand() % 400) == 21) {
			alpha *= 2;
			if (alpha < 0.1)
				alpha = 0.1;
			if (alpha > 8 && quick == 0)
				alpha = 8;
			if (alpha > 16 && quick == 1)
				alpha = 16;
			if (!quick)
				i = maxiter;
		}
		best_bb_length = len;
	}
	vprintf("Winner with %5.2f  q %i\n", best_bb_length, quick);

	return boundingbox;
}


void scene::optimize_cutout(void) 
{
	Polygon_2 *boundingbox;

	return;

	boundingbox = cutout_clone_split(&cutout->poly, 1000);
	unsigned int iter = 0;
	double best_bb_length;
	do {
		double prev;
		best_bb_length = poly_length(boundingbox);

		prev = best_bb_length;
		boundingbox = cutout_clone_split(boundingbox, 4);

		boundingbox = optimize_bb(this, boundingbox, false, best_bb_length);

//		if (iter % 2 == 1 )
//			boundingbox = optimize_bb(this, boundingbox, false, best_bb_length);

		best_bb_length = poly_length(boundingbox);

		vprintf("BB opt from %5.2f to %5.2f at size %i\n", prev, best_bb_length, (int)boundingbox->size());

		if (prev - best_bb_length  < 0.5)
			break;

		if (boundingbox->size() > 128)
			break;

		iter++;
	} while (1);

	boundingbox = optimize_bb(this, boundingbox, false, best_bb_length);

	best_bb_length = poly_length(boundingbox);

	vprintf("Final bb opt %5.2f with size %i\n", best_bb_length, (int)boundingbox->size());

	cutout->poly = *boundingbox;

}


class scene * scene::clone_scene(class scene *input, int mirror, double Xadd)
{
  class scene *scene = input;
  double xmin = 500000, ymin = 500000, xmax = 0, ymax = 0;
//  printf("SCENE_FROM_VCARVE for depth %5.2f\n", depth);

  if (!scene) {
	scene = new(class scene);
	scene->set_z_offset(-inch_to_mm(0.1));
	scene->set_stock_to_leave(stock_to_leave);
  }
  for (auto i : shapes)
    scene = i->clone_scene(scene, mirror, Xadd);

  sort(scene->shapes.begin(), scene->shapes.end(), compare_shape);
  for (unsigned int i = 0; i < scene->shapes.size() ; i++ ) {
		auto bbox = scene->shapes[i]->get_bbox();
		xmin = fmin(bbox.xmin(), xmin);
		ymin = fmin(bbox.ymin(), ymin);
		xmax = fmax(bbox.xmax(), xmax);
		ymax = fmax(bbox.ymax(), ymax);
  }

  double outset;

  if (cutout_depth) {
	  outset = 7;
	  scene->new_poly(         xmin - outset, ymin - outset);
	  scene->add_point_to_poly(xmin - outset, ymax + outset);
	  scene->add_point_to_poly(xmax + outset, ymax + outset);
	  scene->add_point_to_poly(xmax + outset, ymin - outset);
	  scene->end_poly();

	  scene->cutout = scene->shapes[scene->shapes.size() - 1];
	  scene->set_cutout_depth(cutout_depth);
	  scene->shapes.erase(scene->shapes.begin() + scene->shapes.size() - 1);
	  scene->cutout->is_cutout = true;
	  scene->cutout->set_cutout_offset(5);

	  scene->optimize_cutout();
  }

  /* box for area clearance */
  outset = 10;

  scene->new_poly(         xmin - outset, ymin - outset);
  scene->add_point_to_poly(xmin - outset, ymax + outset);
  scene->add_point_to_poly(xmax + outset, ymax + outset);
  scene->add_point_to_poly(xmax + outset, ymin - outset);
  scene->end_poly();


  scene->force_minY(minY);

  if (scene->toollist.size() == 0)
		scene->toollist = toollist;

  scene->process_nesting();    
  return scene;
}


bool scene::fits_inside(Polygon_2 *bound)
{
   for (auto inp : shapes) {
		for(auto vi = inp->poly.vertices_begin() ; vi != inp->poly.vertices_end() ; ++ vi ) {
			if (bound->bounded_side(*vi) == CGAL::ON_UNBOUNDED_SIDE) {
			       return false;
			}
		}
	}
   return true;
}
