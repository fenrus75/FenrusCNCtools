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
    return D;
    int i = (int)(D * 1000 + 0.5);
    return i / 1000.0;
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
    area = fabs(CGAL::to_double(poly.area()));
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

#if 1
    for (auto i : skeleton)
        print_straight_skeleton(*i);
    if (iss)
        print_straight_skeleton(*iss);
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
      if (CGAL::to_double(v1.x()) == X && CGAL::to_double(v1.y()) == Y)
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
      if (CGAL::to_double(v1.x()) == CGAL::to_double(v2.x()) && CGAL::to_double(v1.y()) == CGAL::to_double(v2.y())) {
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
    bool reverse = false;
    
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
			K k;
            for (auto ply : offset_polygons) {
                SsPtr pp = CGAL::create_interior_straight_skeleton_2(ply->outer_boundary().vertices_begin(),
                    ply->outer_boundary().vertices_end(),
                    ply->holes_begin(), ply->holes_end(), k);            
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
                        X1 = point_snap(CGAL::to_double(x->vertex()->point().x()));
                        Y1 = point_snap(CGAL::to_double(x->vertex()->point().y()));
                        X2 = point_snap(CGAL::to_double(x->opposite()->vertex()->point().x()));
                        Y2 = point_snap(CGAL::to_double(x->opposite()->vertex()->point().y()));
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

void inputshape::create_toolpaths_cutout(int toolnr, double depth)
{
	/* Step 1: Create an outside bounding box */
	double currentdepth = -fabs(depth);
	double gradient = 0;
	double circumfence = 0;
	double outset = 0;
	auto bbox = poly.bbox();
    Polygon_2 *boxpoly = new(Polygon_2);

#if 0
    K k;
	auto out = compute_outer_frame_margin(poly.vertices_begin(), poly.vertices_end(), tool_diam(toolnr), k);
	if (out) {
		outset += CGAL::to_double(*out);
	}
#endif
	outset += 4 * tool_diam(toolnr);
	outset += 40;

	vprintf("Cutout Outset is %5.2f mm\n", outset);

	boxpoly->push_back(Point(bbox.xmin() - outset, bbox.ymin() - outset));
	boxpoly->push_back(Point(bbox.xmin() - outset, bbox.ymax() + outset));
	boxpoly->push_back(Point(bbox.xmax() + outset, bbox.ymax() + outset));
	boxpoly->push_back(Point(bbox.xmax() + outset, bbox.ymin() - outset));

	boxpoly->reverse_orientation();
	
	/* Step 2: Create a polyhole with the coutout box as holes */
    auto ph = new PolygonWithHoles(*boxpoly);
	poly.reverse_orientation();
	ph->add_hole(poly);

	/* Step 3: Create an ISS */
	auto ciss = CGAL::create_interior_straight_skeleton_2(*ph);

	/* Step 4: Inset the ISS by tool radius */
	PolygonWithHolesPtrVector  offset_polygons;
	offset_polygons = arrange_offset_polygons_2(CGAL::create_offset_polygons_2<Polygon_2>(get_tool_diameter()/2, *ciss) );


	/* Step 5: The hole perimiter is now our path for the tool */

		for (auto ply : offset_polygons) {        
			for(auto hi = ply->holes_begin() ; hi != ply->holes_end() ; ++ hi ) {
    	    	Polygon_2 *p;

				if (currentdepth > 0)
					break;
				p = new(Polygon_2);
            	*p = *hi;
				for (unsigned int i = 0; i < p->size(); i++) {
					unsigned int next = i + 1;
					if (next >= p->size())
						next = 0;

					class tooldepth * td = new(class tooldepth);
					tooldepths.push_back(td);
					td->depth = currentdepth;
					td->toolnr = toolnr;
					td->diameter = get_tool_diameter();
					class toollevel *tool = new(class toollevel);
					tool->level = 0;
					tool->offset = get_tool_diameter();
					tool->diameter = get_tool_diameter();
					tool->depth = currentdepth;
					tool->toolnr = toolnr;
					tool->minY = minY;
					tool->name = "Cutout bottom";
					td->toollevels.push_back(tool);

	    	    	Polygon_2 *p2;
					p2 = new(Polygon_2);
					double d1 = currentdepth;
					double d2 = gradient * dist(	CGAL::to_double((*p)[i].x()), 
														CGAL::to_double((*p)[i].y()), 
														CGAL::to_double((*p)[next].x()), 
														CGAL::to_double((*p)[next].y()));
					p2->push_back(Point(CGAL::to_double((*p)[i].x()), CGAL::to_double((*p)[i].y())));
					p2->push_back(Point(CGAL::to_double((*p)[next].x()), CGAL::to_double((*p)[next].y())));
					tool->add_poly_vcarve(p2, d1, d1 + d2);
					
				}
	        }
		}


	/*	   calculate gradient of dZ/mm */

	for (auto ply : offset_polygons) {        
		for(auto hi = ply->holes_begin() ; hi != ply->holes_end() ; ++ hi ) {
        	Polygon_2 *p;
            p = new(Polygon_2);
            *p = *hi;
			for (unsigned int i = 0; i < p->size(); i++) {
				unsigned int next = i + 1;
				if (next >= p->size())
					next = 0;
				circumfence += dist(
					CGAL::to_double((*p)[i].x()), 
					CGAL::to_double((*p)[i].y()), 
					CGAL::to_double((*p)[next].x()), 
					CGAL::to_double((*p)[next].y()));
			}
        }
	}
	vprintf("Circumfence is %5.2f mm\n", circumfence);
	if (circumfence == 0)
		return;
	gradient = fabs(get_tool_maxdepth()) / circumfence;
	/*     walk the gradient up until we break the surface */
	while (currentdepth < 0) {
		for (auto ply : offset_polygons) {        
			for(auto hi = ply->holes_begin() ; hi != ply->holes_end() ; ++ hi ) {
    	    	Polygon_2 *p;

				if (currentdepth > 0.1)
					break;
				p = new(Polygon_2);
            	*p = *hi;
				for (unsigned int i = 0; i < p->size(); i++) {
					unsigned int next = i + 1;
					if (next >= p->size())
						next = 0;

					if (currentdepth > 0.1)
						break;

					class tooldepth * td = new(class tooldepth);
					tooldepths.push_back(td);
					td->depth = currentdepth;
					td->toolnr = toolnr;
					td->diameter = get_tool_diameter();
					class toollevel *tool = new(class toollevel);
					tool->level = 0;
					tool->offset = get_tool_diameter();
					tool->diameter = get_tool_diameter();
					tool->depth = currentdepth;
					tool->toolnr = toolnr;
					tool->minY = minY;
					tool->name = "Cutout ramp";
					td->toollevels.push_back(tool);

	    	    	Polygon_2 *p2;
					p2 = new(Polygon_2);
					double d1 = currentdepth;
					double d2 = gradient * dist(	CGAL::to_double((*p)[i].x()), 
														CGAL::to_double((*p)[i].y()), 
														CGAL::to_double((*p)[next].x()), 
														CGAL::to_double((*p)[next].y()));
					p2->push_back(Point(CGAL::to_double((*p)[i].x()), CGAL::to_double((*p)[i].y())));
					p2->push_back(Point(CGAL::to_double((*p)[next].x()), CGAL::to_double((*p)[next].y())));
					tool->add_poly_vcarve(p2, d1, d1 + d2);
					currentdepth += d2;
					
				}
	        }
		}
	}
}


bool should_vcarve(class inputshape *shape, double X1, double Y1, double X2, double Y2, double is_inner, double is_bisect) 
{
	if (is_inner)
		return true;

    for (unsigned int i = 0; i < shape->poly.size(); i++) {
        double BC_x, BC_y, BA_x, BA_y;
        double BC_l, BA_l;
        double phi;
		unsigned int a, b, c;
        if (i == 0)
          a = shape->poly.size() - 1;
        else
          a = i - 1;
        b = i;
        c = i + 1;
        if (c >= shape->poly.size())
          c = 0;
          
        BA_x = CGAL::to_double((shape->poly)[a].x()) - CGAL::to_double((shape->poly)[b].x());
        BA_y = CGAL::to_double((shape->poly)[a].y()) - CGAL::to_double((shape->poly)[b].y());

        BC_x = CGAL::to_double((shape->poly)[c].x()) - CGAL::to_double((shape->poly)[b].x());
        BC_y = CGAL::to_double((shape->poly)[c].y()) - CGAL::to_double((shape->poly)[b].y());
        
        BA_l = sqrt(BA_x * BA_x + BA_y * BA_y);
        BC_l = sqrt(BC_x * BC_x + BC_y * BC_y);
        
        phi = acos( (BA_x * BC_x + BA_y * BC_y) / (BA_l * BC_l));
		phi = 360 / 2 / M_PI * phi;

//		printf("Angle is %5.2f\n", phi);
		if (approx4((shape->poly)[b].x(),X1) && approx4(shape->poly[b].y(), Y1) && phi < 120)
			return true;
		if (approx4((shape->poly)[b].x(),X2) && approx4(shape->poly[b].y(), Y2) && phi < 120)
			return true;
    }
	return false;
}

void inputshape::create_toolpaths_vcarve(int toolnr, double maxdepth)
{
    double angle = get_tool_angle(toolnr);
    if (!polyhole) {
        polyhole = new PolygonWithHoles(poly);
        for (auto i : children)
          polyhole->add_hole(i->poly);
    }
    
//    printf("VCarve toolpath\n");
    
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
            X1 = point_snap2(CGAL::to_double(x->vertex()->point().x()));
            Y1 = point_snap2(CGAL::to_double(x->vertex()->point().y()));
            
            X2 = point_snap2(CGAL::to_double(x->opposite()->vertex()->point().x()));
            Y2 = point_snap2(CGAL::to_double(x->opposite()->vertex()->point().y()));
#if 0
            if (x->is_inner_bisector()) {
#else
            if (x->is_bisector() || x->is_inner_bisector()) {
#endif
                d1 = radius_to_depth(parent->distance_from_edge(X1, Y1, false), angle);
                d2 = radius_to_depth(parent->distance_from_edge(X2, Y2, false), angle);
                
                /* four cases to deal with */
#if 1                
                /* case 1: d1 and d2 are both ok wrt max depth */
                if (d1 >= maxdepth && d2 >= maxdepth && should_vcarve(this, X1, Y1, X2, Y2, x->is_inner_bisector(), x->is_bisector())) {
//                    printf(" CASE 1 \n");
                    if (X1 != X2 || Y1 != Y2) { 
                            Polygon_2 *p = new(Polygon_2);
                            p->push_back(Point(X1, Y1));
                            p->push_back(Point(X2, Y2));
                            tool->diameter = fmax(tool->diameter, -d1 * 2);
                            tool->diameter = fmax(tool->diameter, -d2 * 2);
                            tool->add_poly_vcarve(p, d1, d2);
                    }
                }
#endif                
#if 1
                /* case 2: d1 and d2 are both not ok wrt max depth */
                if (d1 < maxdepth && d2 < maxdepth && should_vcarve(this, X1, Y1, X2, Y2, x->is_inner_bisector(), x->is_bisector())) {
                  if (X1 != X2 || Y1 != Y2) { 
                    double x1,y1,x2,y2,x3,y3,x4,y4;
                    double r1, r2;
                    
                    r1 = depth_to_radius(fabs(maxdepth) - fabs(d1), angle);
                    r2 = depth_to_radius(fabs(maxdepth) - fabs(d2), angle); 
                    
//                    printf("D2R1  %5.2f   ->   %5.2f    %5.3f, %5.3f \n", depth_to_radius(d1, angle), r1,X1,Y1);
//                    printf("D2R2  %5.2f   ->   %5.2f    %5.3f, %5.3f \n", depth_to_radius(d2, angle), r2,X2,Y2);
                    int ret = 0;
                    
                    ret += lines_tangent_to_two_circles(X1, Y1, r1, 
                                X2, Y2, r2,
                                0,
                                &x1, &y1, &x2, &y2);
                    ret += lines_tangent_to_two_circles(X1, Y1, r1, 
                                X2, Y2, r2,
                                1,
                                &x3, &y3, &x4, &y4);
                                
                    if (ret == 0) {
                                
                      Polygon_2 *p = new(Polygon_2);
                      if (isnan(x1) || isnan(x2) || isnan(y1) || isnan(y2)) {
                        printf("%5.5f,%5.5f   -> %5.5f,%5.5f\n", X1, Y1, X2, Y2);
                        printf("d1 %5.2f   d2 %5.2f\n", d1, d2);
                        printf("d2r %5.5f  %5.5f\n", depth_to_radius(fabs(d1) - fabs(maxdepth),angle), depth_to_radius(fabs(d2) - fabs(maxdepth), angle));
                        printf("x1 %5.2f y1 %5.2f   x2 %5.2f  y2 %5.2f\n", x1, y1, x2, y2);
                        printf("x3 %5.2f y3 %5.2f   x4 %5.2f  y4 %5.2f\n", x3, y3, x4, y4);
                      }
//                      printf("generate %5.5f %5.5f\n", x1,y1);
//                      printf("generate %5.5f %5.5f\n", x2,y2);

					  p->push_back(Point(x1, y1));
    	              p->push_back(Point(x2, y2));
                      tool->diameter = depth_to_radius(maxdepth, angle) * 2;
                      tool->add_poly_vcarve(p, maxdepth, maxdepth);
                    
                      Polygon_2 *p2 = new(Polygon_2);
					  p2->push_back(Point(x3, y3));
        	          p2->push_back(Point(x4, y4));
                      tool->diameter = depth_to_radius(maxdepth, angle) * 2;
                      tool->add_poly_vcarve(p2, maxdepth, maxdepth);
                    }
                
//                    printf(" CASE 2 \n");
                  }
                }
#endif
#if 1
                /* case 3: d1 is not ok but d2 is ok */
                /* if we swap 1 and 2 we're at case 4 so swap and cheap out */
                if (d1 < maxdepth && d2 >= maxdepth) {
                    double t;
                    
                    t = X1; X1 = X2; X2 = t;
                    t = Y1; Y1 = Y2; Y2 = t;
                    t = d1; d1 = d2; d2 = t;
                    
//                    printf(" CASE 3 \n");
                }
#endif
#if 1                
                /* case 4: d1 is ok d2 is not ok */
                if (d1 >= maxdepth && d2 < maxdepth) {
				  bool exclusioncase = false;
				  if (d1 == 0 && !should_vcarve(this, X1, Y1, X2, Y2, x->is_inner_bisector(), x->is_bisector()))
						exclusioncase = true; 
                  if ( (X1 != X2 || Y1 != Y2) && !exclusioncase) { 
                    double x1,y1,x2,y2,x3,y3,x4,y4;
                    int ret = 0;
//                    printf(" CASE 4 \n");
                    double Xm, Ym; /* midpoint of the vector at the place it crosses the depth */
                    double ratio;
                    
                    x1 = X2 - X1;
                    y1 = Y2 - Y1;
                    
                    ratio = (d1 - maxdepth) / (d1 - d2);
                    Xm = X1 + ratio * x1;
                    Ym = Y1 + ratio * y1;
//                    printf("Point 1 (%5.2f,%5.2f) at %5.2f\n", X1, Y1, d1);
//                    printf("Point 2 (%5.2f,%5.2f) at %5.2f\n", X2, Y2, d2);
//                    printf("Point M (%5.2f,%5.2f) at %5.2f\n", Xm, Ym, d1 + ratio * (d2-d1));

                    /* From X1 to Xm is business as usual */
                    Polygon_2 *p = new(Polygon_2);
                    p->push_back(Point(X1, Y1));
                    p->push_back(Point(Xm, Ym));
                    tool->diameter = fmax(tool->diameter, -d1 * 2);
                    tool->diameter = fmax(tool->diameter, -fabs(maxdepth) * 2);
                    tool->add_poly_vcarve(p, d1, maxdepth);


                    /* and from Xm to X2 is like case 2 */
                    ret += lines_tangent_to_two_circles(Xm, Ym, 0, 
                                X2, Y2, depth_to_radius(fabs(maxdepth) - fabs(d2), angle),
                                0,
                                &x1, &y1, &x2, &y2);
                    ret += lines_tangent_to_two_circles(Xm, Ym, 0, 
                                X2, Y2, depth_to_radius(fabs(maxdepth) - fabs(d2), angle),
                                1, 
                                &x3, &y3, &x4, &y4);
                      if (isnan(x1) || isnan(x2) || isnan(y1) || isnan(y2)) {
                        printf("%5.5f,%5.5f   -> %5.5f,%5.5f\n", X1, Y1, X2, Y2);
                        printf("d1 %5.2f   d2 %5.2f\n", d1, d2);
                        printf("d2r %5.5f  %5.5f\n", depth_to_radius(fabs(d1) - fabs(maxdepth),angle), depth_to_radius(fabs(d2) - fabs(maxdepth), angle));
                        printf("x1 %5.2f y1 %5.2f   x2 %5.2f  y2 %5.2f\n", x1, y1, x2, y2);
                        printf("x3 %5.2f y3 %5.2f   x4 %5.2f  y4 %5.2f\n", x3, y3, x4, y4);
                      }
                                
                    Polygon_2 *p3 = new(Polygon_2);
//                    printf("x1 %5.2f y1 %5.2f   x2 %5.2f  y2 %5.2f\n", x1, y1, x2, y2);
//                    printf("x3 %5.2f y3 %5.2f   x4 %5.2f  y4 %5.2f\n", x3, y3, x4, y4);
                    p3->push_back(Point(x1, y1));
                    p3->push_back(Point(x2, y2));
                    tool->diameter = depth_to_radius(maxdepth, angle) * 2;
                    if (ret == 0)
                        tool->add_poly_vcarve(p3, maxdepth, maxdepth);
                    
                    Polygon_2 *p2 = new(Polygon_2);
                    p2->push_back(Point(x3, y3));
                    p2->push_back(Point(x4, y4));
                    tool->diameter = depth_to_radius(maxdepth, angle) * 2;
                    if (ret == 0)
                        tool->add_poly_vcarve(p2, maxdepth, maxdepth);
                
                    }
                    
                    
                }
#endif
            }
    }
}


static void create_inlayplug_path(class scene *scene, class toollevel *tool, int toolnr, double X1, double Y1, double X2, double Y2, double maxdepth)
{	
	double d1, d2;
	double angle;
	/* line too long, split the difference */
	if (dist(X1,Y1,X2,Y2) > 1) {
		create_inlayplug_path(scene, tool, toolnr, X1, Y1, (X1+X2)/2, (Y1+Y2)/2, maxdepth);
		create_inlayplug_path(scene, tool, toolnr, (X1+X2)/2, (Y1+Y2)/2, X2, Y2, maxdepth);
		return;
	}
	angle = get_tool_angle(toolnr);
	d1 = radius_to_depth(scene->distance_from_edge(X1, Y1, true)/2, angle);
	d2 = radius_to_depth(scene->distance_from_edge(X2, Y2, true)/2, angle);

	d1 = fmax(d1, maxdepth);
	d2 = fmax(d2, maxdepth);

	d1 = maxdepth;
	d2 = maxdepth;
	/* if there's too much difference in distance, split the line */
#if 0
	if (fabs(d1 - d2) > 0.5) {
		create_inlayplug_path(scene, tool, toolnr, X1, Y1, (X1+X2)/2, (Y1+Y2)/2, maxdepth);
		create_inlayplug_path(scene, tool, toolnr, (X1+X2)/2, (Y1+Y2)/2, X2, Y2, maxdepth);
		return;
    }
#endif
	Polygon_2 *p2 = new(Polygon_2);
    p2->push_back(Point(X1, Y1));
    p2->push_back(Point(X2, Y2));
	tool->add_poly_vcarve(p2, d1, d2);
}

void inputshape::create_toolpaths_inlayplug(int toolnr, double maxdepth)
{
	/* create td/tool */
	class tooldepth * td = new(class tooldepth);
	tooldepths.push_back(td);
	td->depth = maxdepth;
	td->toolnr = toolnr;
	td->diameter = get_tool_diameter();
	class toollevel *tool = new(class toollevel);
	tool->level = 0;
	tool->offset = get_tool_diameter();
	tool->diameter = get_tool_diameter();
	tool->depth = maxdepth;
	tool->toolnr = toolnr;
	tool->minY = minY;
	tool->name = "Inlay walk";

	td->toollevels.push_back(tool);

	for (unsigned int i = 0; i < poly.size(); i++) {
		unsigned int next = i + 1;
		if (next >= poly.size())
			next = 0;
		double X1, Y1, X2, Y2;
		X1 = CGAL::to_double(poly[i].x());
		Y1 = CGAL::to_double(poly[i].y());
		X2 = CGAL::to_double(poly[next].x());
		Y2 = CGAL::to_double(poly[next].y());

		create_inlayplug_path(parent, tool, toolnr, X1, Y1, X2, Y2, maxdepth);
	}
    for (auto i : children)
        i->create_toolpaths_inlayplug(toolnr, maxdepth);

	
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


double inputshape::distance_from_edge(double X, double Y, bool exclude_zero)
{
    double bestdist = 1000000000;
    unsigned int i;
    
    for (i = 0; i < poly.size(); i++) {
        unsigned int next = i + 1;
        if (next >= poly.size())
            next = 0;
            
        double d = distance_point_from_vector(CGAL::to_double(poly[i].x()), CGAL::to_double(poly[i].y()), CGAL::to_double(poly[next].x()), CGAL::to_double(poly[next].y()), X, Y);
//        if (d < 20) {
//             printf("-----------\n");
//            double d = distance_point_from_vector(poly[i].x(), poly[i].y(), poly[next].x(), poly[next].y(), X, Y);
//            printf("Point %5.4f %5.4f\n", X, Y);
//            printf("Vector %5.4f,%5.4f -> %5.4f,%5.4f\n", poly[i].x(), poly[i].y(), poly[next].x(), poly[next].y());
//            printf("Distance is %6.4f\n", d);
//            printf("-----------\n");
//        }
		if (exclude_zero && d < 0.0000001) 
			d = 1000000000;
        if (d < bestdist) {
//            printf("New best distance %5.2f   %5.2f,%5.2f -> %5.2f,%5.2f\n", d, X,Y, poly[i].x(), poly[i].y());
            bestdist= d;
        }
    }
    for (auto c : children) {
        double d = c->distance_from_edge(X, Y, exclude_zero);
		if (exclude_zero && d < 0.0000001) 
			d = 1000000000;
        if (d < bestdist) {
            bestdist = d;
        }
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
    for (auto i: children)
        i->set_minY(mY);
}



class scene * inputshape::clone_scene(class scene *input, int mirror, double Xadd)
{
    class scene *scene;
    
    if (input)
        scene = input;
    else
        scene = new(class scene);

    scene->declare_minY(minY);
    scene->set_filename("from clone_scene");    
    
    /* step 1: clone our poly into the new scene */
    for(auto vi = poly.vertices_begin() ; vi != poly.vertices_end() ; ++ vi ) {
		if (mirror)
	        scene->add_point_to_poly(Xadd - CGAL::to_double(vi->x()), CGAL::to_double(vi->y()));
		else
    	    scene->add_point_to_poly(CGAL::to_double(vi->x()), CGAL::to_double(vi->y()));
    }
    scene->end_poly();
    
    /* step 2: and clone all our children as well, but instruct them not to inset by setting toolnr to 0 */
    for (auto i : children)
        scene = i->clone_scene(scene, mirror, Xadd);

    return scene;
}

