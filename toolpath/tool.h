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
        is_vcarve = false;
        run_reverse = false;
        diameter = 0;
        start_vertex = 0;
        depth = 0;
        depth2 = 0;
        toolnr = 0;
        minY = 0;
    }

    int level;
    int toolnr;

    double diameter;
    double depth;
    double depth2; /* for vcarving */
    
    unsigned int start_vertex;
    bool is_hole;
    bool is_slotting;    
    bool is_optional;
    bool is_vcarve;
    bool run_reverse;
    
    double length;
    double minY;
    
    void add_polygon(Polygon_2 *poly);
    bool fits_inside(class toolpath *shape);
    double distance_from(double X, double Y);


    void print_as_svg(const char *color);
    void output_gcode(void);
    void recalculate();
    double get_minY(void);

    vector<Polygon_2*> polygons;
private:
    void output_gcode_slotting(void);
    void output_gcode_reverse(void);
    void output_gcode_vcarve(void);
};

class toollevel {
public:

    toollevel() {
        level = 0;
        offset = 0.0;
        minY = 0.0;
        is_optional = false;
        is_slotting = false;
        run_reverse = false;
        diameter = 0.0;
        name = "unknown";
        depth = 0.0;
        toolnr = 0;
    }
    
    double get_minY(void) { return minY;};
    int level;
    int toolnr;
    double offset; /* offset used to create this level */
    const char *name;
    double diameter;
    double depth;
    double minY;
    
    void add_poly(Polygon_2 *poly, bool is_hole);
    void add_poly_vcarve(Polygon_2 *poly, double depth1, double depth2);
    vector<class toolpath*> toolpaths;
    
    /* for in-between paths, if a path has even deeper paths it will be optimized go away eventually */
    /* so that the only places the half inbetween paths stay is at the very inner cuts */
    bool is_optional;
    
    /* the slotting level runs at a different speed and should not merge */
    bool is_slotting;
    
    /* run the toolpath outside in */
    bool run_reverse;
    
    void print_as_svg(void);
    void output_gcode(void);
    
    void sort_if_slotting(void);
};

class tooldepth {
public:

    tooldepth() {
        level = 0;
        is_slotting = false;
        run_reverse = false;
        diameter = 0.0;
        name = "unknown";
        depth = 0.0;
        toolnr = 0;
    }
    int level;
    int toolnr;
    const char *name;
    double diameter;
    double depth;
    
    void add_poly(Polygon_2 *poly, bool is_hole);
    vector<class toollevel*> toollevels;
    
    /* the slotting level runs at a different speed and should not merge */
    bool is_slotting;
    /* run outside in */
    bool run_reverse;
    
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
        name = "unknown";
        minY = 0;
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


    void create_toolpaths(int toolnr, double depth, int finish_pass, int is_optional, double start_inset, double end_inset, bool _want_skeleton_path);
    void create_toolpaths_vcarve(int toolnr);
    void consolidate_toolpaths(bool _want_inbetween_paths);

    double area;
    int level;
    vector<class inputshape*> children;
    
    void set_name(const char *n);
    void set_minY(double mY);
    
    class scene * scene_from_vcarve(class scene *input, double depth, int toolnr);

private:
    const char *name;

    PolygonWithHoles *polyhole;
    vector<SsPtr>	skeleton;
    SsPtr iss;
    
    vector<class tooldepth*> tooldepths;
    
    double bbX1, bbY1, bbX2, bbY2;
    double minY;

    
    Polygon_2 poly;    
    double distance_from_edge(double X, double Y);
};


#include "scene.h"

extern void parse_svg_file(class scene * scene, const char *filename);


#endif