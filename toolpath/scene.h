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


#include "tool.h"

class input_shape;

class scene {
public:
        scene() {
            minX = 0;
            minY = 0;
            maxX = 0;
            maxY = 0;
            _want_finishing_pass = false;
            _want_inbetween_paths = true;
            _want_skeleton_paths = false;
			_want_inlay = false;
            shape = NULL;
			cutout = NULL;
			inlay_plug = NULL;
            filename = "unknown";            
			cutout_depth = 0;
			depth = 0;	
			stock_to_leave = 0.1;
	    z_offset = 0;
        }
        
        scene(const char *filename);
        
        double get_minX(void);
        double get_minY(void);
        double get_maxX(void);
        double get_maxY(void);
        
        void declare_minY(double Y);
        void force_minY(double Y) { minY = Y;}
        void declare_maxX(double X) { if (X > maxX) maxX = X;};

        void new_poly(double X, double Y);
        void set_poly_name(const char *n);
        void add_point_to_poly(double X, double Y);
        void end_poly(void);
        
        void push_tool(int toolnr);
        void drop_tool(int toolnr);
        bool has_tool(int toolnr);
        void set_default_tool(int toolnr);
        
        void write_naked_svg(void);
        void write_svg(const char *filename);
        void write_gcode(const char *filename, const char *description);
        void write_naked_gcode(void);

        void process_nesting(void);
        void create_toolpaths(void);
        
        void enable_finishing_pass(void);
        bool want_finishing_pass(void);

        void enable_inbetween_paths(void);
        bool want_inbetween_paths(void);

        void enable_inlay(void) { _want_inlay = true; };
        bool want_inlay(void) { return _want_inlay; };

		void set_z_offset(double d) { z_offset = d; };
		double get_z_offset(void) { return z_offset; };

		void set_cutout_depth(double d) { cutout_depth = d; };
		double get_cutout_depth(void) { return cutout_depth; };

		void set_stock_to_leave(double d) { stock_to_leave = d; };
		double get_stock_to_leave(void) { return stock_to_leave; };

		void set_depth(double d) { depth = d; };
		double get_depth(void) { return depth; };

        void enable_skeleton_paths(void);
        bool want_skeleton_paths(void);
        void set_filename(const char *f) { filename = strdup(f);};
        
        double distance_from_edge(double X, double Y, bool exclude_zero);

		class scene * clone_scene(class scene *input, int mirror, double Xadd);

		class scene *inlay_plug;

		bool fits_inside(Polygon_2 *poly);

		void optimize_cutout(void);

        vector<class inputshape *> shapes;
        
private:
        vector<int> toollist;
		class inputshape * cutout;

        class inputshape *shape;


        double minX, minY, maxX, maxY;
		double z_offset;
		double stock_to_leave;
        bool _want_finishing_pass;
        bool _want_inbetween_paths;
        bool _want_skeleton_paths;
		bool _want_inlay;
        const char *filename;
		double cutout_depth;
		double depth;
        
        void consolidate_toolpaths(void);
        void flatten_nesting(void);

        
};


extern void parse_svg_file(class scene * scene, const char *filename);


#endif