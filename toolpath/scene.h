/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef __INCLUDE_GUARD_SCENE_H_
#define __INCLUDE_GUARD_SCENE_H_
#include <string.h>

using namespace std;

#include <vector>

class input_shape;

class scene {
public:
        scene() {
            minX = 1000000;
            minY = 1000000;
            maxX = -1000000;
            maxY = -1000000;
            _want_finishing_pass = false;
            _want_inbetween_paths = false;
            _want_skeleton_paths = false;
            shape = NULL;
            filename = "unknown";            
        }
        
        scene(const char *filename);
        
        double get_minX(void);
        double get_minY(void);
        double get_maxX(void);
        double get_maxY(void);
        
        void declare_minY(double Y);

        void new_poly(double X, double Y);
        void set_poly_name(const char *n);
        void add_point_to_poly(double X, double Y);
        void end_poly(void);
        
        void push_tool(int toolnr);
        void set_default_tool(int toolnr);
        
        void write_svg(const char *filename);
        void write_gcode(const char *filename);

        void process_nesting(void);
        void create_toolpaths(double depth);
        
        void enable_finishing_pass(void);
        bool want_finishing_pass(void);

        void enable_inbetween_paths(void);
        bool want_inbetween_paths(void);

        void enable_skeleton_paths(void);
        bool want_skeleton_paths(void);
        void set_filename(const char *f) { filename = strdup(f);};
        
        class scene * scene_from_vcarve(class scene *input, double depth, int toolnr);


        
private:
        vector<int> toollist;
        vector<class inputshape *> shapes;

        class inputshape *shape;
        double minX, minY, maxX, maxY;
        bool _want_finishing_pass;
        bool _want_inbetween_paths;
        bool _want_skeleton_paths;
        const char *filename;
        
        void consolidate_toolpaths(void);
        void flatten_nesting(void);
};


extern void parse_svg_file(class scene * scene, const char *filename);


#endif