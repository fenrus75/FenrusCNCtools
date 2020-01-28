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
    const char *color;
    
    color = tool_svgcolor(toolnr);
    
    for (auto i : toolpaths) {
        i->print_as_svg(color);
    }
}


static double sortX, sortY;
static bool compare_path(class toolpath *A, class toolpath *B)
{
	if (A->distance_from(sortX, sortY) > 0.1 && B->distance_from(sortX, sortY) > 0.1) {
		if (fabs(A->depth - B->depth) > 0.1) {
			if (A->depth > B->depth)
				return true;
			if (A->depth < B->depth)
				return false;
		}
	}
	if (A->priority < B->priority)
		return true;
	if (A->priority > B->priority)
		return false;
    return (A->distance_from(sortX, sortY) < B->distance_from(sortX, sortY));
}

static class toolpath *clone_tp(class toolpath *tp1)
{
	class toolpath *newtp;
	newtp = new(class toolpath);
	newtp->level = tp1->level;
	newtp->minY = tp1->minY;
	newtp->is_optional = tp1->is_optional;
	newtp->is_hole = tp1->is_hole;
	newtp->is_slotting = tp1->is_slotting;
	newtp->is_single = tp1->is_single;
	newtp->is_vcarve = tp1->is_vcarve;
	newtp->run_reverse = tp1->run_reverse;
	newtp->diameter = tp1->diameter;
	newtp->depth = tp1->depth;
	newtp->depth2 = tp1->depth2;
	newtp->toolnr = tp1->toolnr;
	return newtp;
}

static class toolpath *can_merge(class toolpath *tp1, class toolpath *tp2)
{
	double X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
	double X3 = 0, Y3 = 0, X4 = 0, Y4 = 0;
	double x2,y2,x4,y4;
	double ox2, oy2, ox4, oy4;
	double len;

	if (tp1->depth != tp2->depth) 
		return NULL;
	if (tp1->depth2 != tp2->depth2) 
		return NULL;
	if (tp1->depth != tp1->depth2)
		return NULL;
	if (tp1->polygons.size() > 1)
		return NULL;
	if (tp2->polygons.size() > 1)
		return NULL;
	if (!tp1->is_single)
		return NULL;
	if (!tp2->is_single)
		return NULL;
	if (!tp1->is_vcarve)
		return NULL;
	if (!tp2->is_vcarve)
		return NULL;
	if (tp1->toolnr != tp2->toolnr)
		return NULL;

	for (auto p : tp1->polygons) {
			X1 = CGAL::to_double((*p)[0].x());
			Y1 = CGAL::to_double((*p)[0].y());
			X2 = CGAL::to_double((*p)[1].x());
			Y2 = CGAL::to_double((*p)[1].y());
	}
	for (auto p : tp2->polygons) {
			X3 = CGAL::to_double((*p)[0].x());
			Y3 = CGAL::to_double((*p)[0].y());
			X4 = CGAL::to_double((*p)[1].x());
			Y4 = CGAL::to_double((*p)[1].y());
	}

	x2 = X2 - X1;
	y2 = Y2 - Y1;
	x4 = X4 - X3;
	y4 = Y4 - Y3;

	len = sqrt(x2 * x2 + y2 * y2);
	ox2 = x2 / len;
	oy2 = y2 / len;

	len = sqrt(x4 * x4 + y4 * y4);
	ox4 = x4 / len;
	oy4 = y4 / len;

	if ((approx3(ox2, ox4) && approx3(oy2, oy4)) || (len <= 0.00001)) {
		/* same start point same unit vector -> pick the longest*/
		if (approx4(X1,X3) && approx4(Y1,Y3)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			if (dist(X1,Y1,X2,Y2) > dist(X3,Y3,X4,Y4)) {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X2, Y2));
			} else {
				p->push_back(Point(X3, Y3));
			    p->push_back(Point(X4, Y4));
			}
			newtp->add_polygon(p);
			return newtp;
		}

		/* consecutive paths */
		if (approx4(X1,X4) && approx4(Y1,Y4)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			p->push_back(Point(X2, Y2));
		    p->push_back(Point(X3, Y3));
			newtp->add_polygon(p);
			return newtp;
		}


		/* consecutive paths */
		if (approx4(X2,X3) && approx4(Y2,Y3)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			p->push_back(Point(X1, Y1));
		    p->push_back(Point(X4, Y4));
			newtp->add_polygon(p);
			return newtp;
		}

		/* same end point same unit vector */
		if (approx4(X2,X4) && approx4(Y2,Y4)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			if (dist(X1,Y1,X2,Y2) > dist(X3,Y3,X4,Y4)) {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X2, Y2));
			} else {
				p->push_back(Point(X3, Y3));
			    p->push_back(Point(X4, Y4));
			}
			newtp->add_polygon(p);
			return newtp;
		}

		/* if X3,Y3 is a point on the first vector and same unit vector ... */
		if (distance_point_from_vector(X1,Y1,X2,Y2,X3,Y3) < 0.0000001) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			if (dist(X1,Y1,X2,Y2) > dist(X1,Y1,X4,Y4)) {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X2, Y2));
			} else {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X4, Y4));
			}
			newtp->add_polygon(p);
			return newtp;			
		}
	}

	if ( (approx3(ox2, -ox4) && approx3(oy2, -oy4)) || (len <= 0.0001)) {
		if (approx4(X1,X3) && approx4(Y1,Y3)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			p->push_back(Point(X2, Y2));
			p->push_back(Point(X4, Y4));
			newtp->add_polygon(p);
			return newtp;
		}

		if (approx4(X1,X4) && approx4(Y1,Y4)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			if (dist(X1,Y1,X2,Y2) > dist(X3,Y3,X4,Y4)) {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X2, Y2));
			} else {
				p->push_back(Point(X3, Y3));
			    p->push_back(Point(X4, Y4));
			}
			newtp->add_polygon(p);
			return newtp;
		}

		if (approx4(X2,X3) && approx4(Y2,Y3)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			if (dist(X1,Y1,X2,Y2) > dist(X3,Y3,X4,Y4)) {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X2, Y2));
			} else {
				p->push_back(Point(X3, Y3));
			    p->push_back(Point(X4, Y4));
			}
			newtp->add_polygon(p);
			return newtp;
		}

		/* same end point opposing unit vector */
		if (approx4(X2,X4) && approx4(Y2,Y4)) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			p->push_back(Point(X1, Y1));
		    p->push_back(Point(X3, Y3));
			newtp->add_polygon(p);
			return newtp;
		}

#if 1
		/* if X4,Y4 is a point on the first vector and same unit vector ... */
		if (distance_point_from_vector(X1,Y1,X2,Y2,X4,Y4) < 0.0000001) {
			class toolpath *newtp = clone_tp(tp1);
			Polygon_2 *p = new(Polygon_2);
			if (dist(X1,Y1,X2,Y2) > dist(X1,Y1,X3,Y3)) {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X2, Y2));
			} else {
				p->push_back(Point(X1, Y1));
			    p->push_back(Point(X3, Y3));
			}
			newtp->add_polygon(p);
			return newtp;			
		}
#endif
	}


	return NULL;
}


void toollevel::consolidate(void)
{
	unsigned int i, j;
	class toolpath *tp;

	if (toolpaths.size() < 2)
		return; /* nothing to consolidate */

	/* first, we do +1 and +2 as that's a common case */

	for (i = 0; i < toolpaths.size() - 2; i++) {
		tp = can_merge(toolpaths[i], toolpaths[i + 1]);
		if (tp) {
			toolpaths[i] = tp;
			toolpaths.erase(toolpaths.begin() + i + 1);
			if (i >= 2)
				i -= 2;	
//			vprintf("Can merge %i, %i \n", i, i+1);
			continue;
		}
		tp = can_merge(toolpaths[i], toolpaths[i + 2]);
		if (tp) {
			toolpaths[i] = tp;
			toolpaths.erase(toolpaths.begin() + i + 2);
			if (i >= 2)
				i -= 2;	
//			vprintf("Can merge2 %i, %i \n", i, i+2);
			continue;
		}
	}

	if (no_sort)
		return;
	/* now the O(N^2) part */

	for (i = 0; i < toolpaths.size(); i++) {
	  for (j = 0; j < toolpaths.size(); j++) {
		if (i == j)
			continue;
		tp = can_merge(toolpaths[i], toolpaths[j]);
		if (tp && i != j) {
			toolpaths[i] = tp;
			toolpaths.erase(toolpaths.begin() + j);
//			vprintf("Can merge square %i, %i \n", i, j);
			if (i >= 2)
				i -= 2;	
			if (j >= 2)
				j -= 2;	
			continue;
		}
      }
	}
}


void toollevel::trim_intersects(void)
{
	unsigned int i, j;

	if (toolpaths.size() < 2)
		return; /* nothing to intersect */

	if (!toolpaths[0]->is_vcarve)
		return;

	for (i = 0; i < toolpaths.size(); i++) {
		int count = 0;
		double min_l = 1.0;
		double max_l = 0;

		double X1, Y1, X2, Y2;

		for (auto p : toolpaths[i]->polygons) {
				X1 = CGAL::to_double((*p)[0].x());
				Y1 = CGAL::to_double((*p)[0].y());
				X2 = CGAL::to_double((*p)[1].x());
				Y2 = CGAL::to_double((*p)[1].y());
		}
		for (j = 0; j < toolpaths.size(); j++) {
			double this_l;
			double X3 = 0,Y3 = 0,X4 = 0,Y4 = 0;
			if (i == j)
				continue;

			for (auto p : toolpaths[j]->polygons) {
				X3 = CGAL::to_double((*p)[0].x());
				Y3 = CGAL::to_double((*p)[0].y());
				X4 = CGAL::to_double((*p)[1].x());
				Y4 = CGAL::to_double((*p)[1].y());
			}
			if (vector_intersects_vector_l(X1,Y1,X2,Y2,X3,Y3,X4,Y4, &this_l)) {
				count++;
				min_l = fmin(min_l, this_l);
				max_l = fmax(max_l, this_l);
//				printf("Intersect %i with l %5.3f  (%5.4f - %5.4f)\n", count, this_l, min_l, max_l);
			}
		}
		if (count >= 2 && max_l >= 0.90 && min_l < 0.10) {
			class toolpath *newtp = clone_tp(toolpaths[i]);
			Polygon_2 *p = new(Polygon_2);
			vector_apply_l(&X1,&Y1, &X2, &Y2, min_l, max_l);
			p->push_back(Point(X1, Y1));
		    p->push_back(Point(X2, Y2));
			newtp->add_polygon(p);
			toolpaths[i] = newtp;
//			printf("Reducing toolpath %i due to %i intersects   (%5.2f - %5.2f)\n", i, count, min_l, max_l);
		}
	}
}

void toollevel::output_gcode(void)
{
    vector<class toolpath*> worklist;    


//	vprintf("Work size before consolidate  %i\n", (int)toolpaths.size());
	consolidate();
//	vprintf("Work size before trim_intersect  %i\n", (int)toolpaths.size());
#if 1
	if (!no_sort) {
		trim_intersects(); /* This is unproven correct so far */
		consolidate();
		consolidate();
	}
#endif
//	vprintf("Work size after trim_intersect  %i\n", (int)toolpaths.size());
    worklist = toolpaths;

#if 0
	for (unsigned int i = 0; i < worklist.size(); i++) {
		class toolpath *tp = worklist[i];
		for (auto p : tp->polygons) {
			double x1,y1,x2,y2;
			double ox, oy, len;
			x1 = CGAL::to_double((*p)[0].x());
			y1 = CGAL::to_double((*p)[0].y());
			x2 = CGAL::to_double((*p)[1].x());
			y2 = CGAL::to_double((*p)[1].y());
			ox = x2-x1;
			oy = y2-y1;
			len = sqrt(ox*ox + oy*oy);
			ox = ox / len;
			oy = oy / len;
			printf("%4i: %5.4f,%5.4f --> %5.4f, %5.4f     uv %5.8f,%5.8f  length %5.4f\n", i, x1, y1, x2, y2, ox, oy, len);
		}
    }
#endif
	if (name)
	    gcode_write_comment(name);
    sortX = gcode_current_X();
    sortY = gcode_current_Y() + get_minY();
        
	if (!no_sort)
	    sort(worklist.begin(), worklist.end(), compare_path);
    
    while (worklist.size() > 0) {
		bool zero_retracts;

		zero_retracts = worklist[0]->output_gcode_vcarve_would_retract();
		if (worklist.size() > 1 && zero_retracts && !worklist[1]->output_gcode_vcarve_would_retract()) {
			worklist[1]->output_gcode();
	        worklist.erase(worklist.begin() + 1);
		} if (worklist.size() > 2 && zero_retracts && !worklist[2]->output_gcode_vcarve_would_retract()) {
			worklist[2]->output_gcode();
	        worklist.erase(worklist.begin() + 2);
		} else {
			worklist[0]->output_gcode();
	        worklist.erase(worklist.begin());
		}
	        sortX = gcode_current_X();
	        sortY = gcode_current_Y() + get_minY();
        
		if (!no_sort)
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
    tp->is_optional = is_optional;
    tp->run_reverse = run_reverse;
    tp->diameter = diameter;
    tp->depth = depth;
    tp->recalculate();
    tp->toolnr = toolnr;
    tp->minY = minY;
    
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
            if (dist(CGAL::to_double((*p2)[0].x()), CGAL::to_double((*p2)[0].y()), CGAL::to_double((*poly)[0].x()), CGAL::to_double((*poly)[0].y())) < 0.15 &&
                dist(CGAL::to_double((*p2)[1].x()), CGAL::to_double((*p2)[1].y()), CGAL::to_double((*poly)[1].x()), CGAL::to_double((*poly)[1].y())) < 0.15)
                   return;
            if (dist(CGAL::to_double((*p2)[0].x()), CGAL::to_double((*p2)[0].y()), CGAL::to_double((*poly)[0].x()), CGAL::to_double((*poly)[0].y())) < 0.15 &&
                dist(CGAL::to_double((*p2)[0].x()), CGAL::to_double((*p2)[0].y()), CGAL::to_double((*poly)[1].x()), CGAL::to_double((*poly)[1].y())) < 0.15)
                   return;
#endif            
        }
    }

    toolpaths.push_back(tp);    
}

void toollevel::add_poly_vcarve(Polygon_2 *poly, double depth1, double depth2, double prio, const char *color)
{
    /* check if the same path is already there if we're slotting */
#if 1
	if (!no_sort) {
		for (auto tp : toolpaths) {
			if (tp->polygons.size() > 1) {
				continue;
			}
            Polygon_2 *p2;
            p2 = tp->polygons[0];
            if ( (*p2)[0].x() == (*poly)[0].x() && (*p2)[0].y() == (*poly)[0].y() &&
                (*p2)[1].x() == (*poly)[1].x() && (*p2)[1].y() == (*poly)[1].y())
                    return;
            if ( (*p2)[0].x() == (*poly)[1].x() && (*p2)[0].y() == (*poly)[1].y() &&
                (*p2)[1].x() == (*poly)[0].x() && (*p2)[1].y() == (*poly)[0].y())
                    return;
	    }
	}
#endif
    class toolpath *tp;
    tp = new(class toolpath);
    tp->add_polygon(poly);
    tp->depth = depth1;
    tp->depth2 = depth2;
    tp->recalculate();
    tp->toolnr = toolnr;
    tp->is_vcarve = true;
    tp->minY = minY;
    tp->diameter = diameter;
    tp->is_single = true;
	tp->color = color;
	tp->priority = prio;
    toolpaths.push_back(tp);    
}

void toollevel::sort_if_slotting(void)
{
    if (!is_slotting)
        return;

    /* to be implemented */        
}
