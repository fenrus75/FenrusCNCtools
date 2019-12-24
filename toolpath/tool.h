/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef __INCLUDE_GUARD_TOOL_H_
#define __INCLUDE_GUARD_TOOL_H_

#include <vector>
#include <boost/shared_ptr.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/create_offset_polygons_from_polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>

#include <CGAL/Polygon_2.h>
#include <CGAL/create_offset_polygons_2.h>


using namespace std;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K ;
typedef K::Point_2                   Point ;
typedef CGAL::Polygon_2<K>           Polygon_2 ;
typedef CGAL::Straight_skeleton_2<K> Ss ;
typedef boost::shared_ptr<Polygon_2> PolygonPtr ;
typedef boost::shared_ptr<Ss> SsPtr ;
typedef std::vector<PolygonPtr> PolygonPtrVector ;
typedef CGAL::Polygon_with_holes_2<K> PolygonWithHoles ;
typedef boost::shared_ptr<PolygonWithHoles> PolygonWithHolesPtr ;
typedef std::vector<PolygonWithHolesPtr> PolygonWithHolesPtrVector;
typedef CGAL::Polygon_with_holes_2<K> Polygon_with_holes ;

class inputshape;
typedef class inputshape inputshape;

class toolpath {
public:
    toolpath() {
        level = 0;
        is_hole = false;
        is_slotting = false;
        is_optional = false;
        diameter = 0;
        start_vertex = 0;
        depth = 0;
        toolnr = 0;
    }

    int level;
    int toolnr;

    double diameter;
    double depth;
    
    unsigned int start_vertex;
    bool is_hole;
    bool is_slotting;    
    bool is_optional;
    
    double length;
    
    void add_polygon(Polygon_2 *poly);
    bool fits_inside(class toolpath *shape);
    double distance_from(double X, double Y);


    void print_as_svg(const char *color);
    void output_gcode(void);
    void recalculate();

    vector<Polygon_2*> polygons;
private:
    void output_gcode_slotting(void);
};

class toollevel {
public:

    toollevel() {
        level = 0;
        offset = 0.0;
        is_optional = false;
        is_slotting = false;
        diameter = 0.0;
        name = "unknown";
        depth = 0.0;
        toolnr = 0;
    }
    int level;
    int toolnr;
    double offset; /* offset used to create this level */
    const char *name;
    double diameter;
    double depth;
    
    void add_poly(Polygon_2 *poly, bool is_hole);
    vector<class toolpath*> toolpaths;
    
    /* for in-between paths, if a path has even deeper paths it will be optimized go away eventually */
    /* so that the only places the half inbetween paths stay is at the very inner cuts */
    bool is_optional;
    
    /* the slotting level runs at a different speed and should not merge */
    bool is_slotting;
    
    void print_as_svg(void);
    void output_gcode(void);
    
    void sort_if_slotting(void);
};

class inputshape {
public:
    inputshape() {
        level = 0;
        polyhole = NULL;
        iss = NULL;
    }
    void set_level(int _level);
    void add_child(class inputshape *child);
    void wipe_children(void);
    void update_stats(void);
    void add_point(double X, double Y);
    void close_shape(void);

    void fix_orientation(void);
    void print_as_svg(void);
    void output_gcode(int tool);
    
    bool fits_inside(class inputshape *shape);


    void create_toolpaths(int toolnr, double depth, int finish_pass, int is_optional, double start_inset, double end_inset);
    void consolidate_toolpaths(void);

    double area;
    int level;
    vector<class inputshape*> children;

private:

    PolygonWithHoles *polyhole;
    vector<SsPtr>	skeleton;
    SsPtr iss;
    
    vector<class toollevel*> toolpaths;
    
    double bbX1, bbY1, bbX2, bbY2;

    
    Polygon_2 poly;    
    
};

extern int want_skeleton_path;
extern int want_inbetween_paths;


#endif