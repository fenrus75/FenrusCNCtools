/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "scene.h"

extern "C" {
    #include "toolpath.h"
}

int verbose = 0;

double cutout_depth = 0;

static double depth;

void usage(void)
{
	printf("Usage:\n\ttoolpath [options] <file.svg>\n");
	printf("\t--verbose         (-v)    verbose output\n");
	printf("\t--finish-pass     (-f)	add a finishing pass\n");
	printf("\t--skeleton        (-s)    reduce slotting\n");
    printf("\t--inbetween       (-i)    ensure no ridges left over\n");
	printf("\t--inlay			(-n)    create an inlay/plug design\n");
	printf("\t--library <file>  (-l)	load CC .csv tool file\n");
	printf("\t--tool <number>   (-t)	use tool number <number> \n");
	printf("\t--depth <inch>    (-d)    set cutting depth in inches\n");
	printf("\t--Depth <mm>      (-D)    set cutting depth in mm\n");
	printf("\t--cutout <inch>   (-c)    cut out the outer geometry to depth <inch>\n");
	exit(EXIT_SUCCESS);
}

static struct option long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument,       0, 'v'},
          {"finish-pass", no_argument,       0, 'f'},
          {"skeleton", no_argument,       0, 's'},
          {"inbetween", no_argument,       0, 'i'},
          {"inlay", no_argument,       0, 'n'},
          {"library",    required_argument, 0, 'l'},
          {"tool",    required_argument, 0, 't'},
          {"cutout",  required_argument, 0, 'c'},
          {"depth",    required_argument, 0, 'd'},
          {"Depth",    required_argument, 0, 'D'},
		  {"help",	no_argument, 0, 'h'},
          {0, 0, 0, 0}
        };

int main(int argc, char **argv)
{
    int opt;
    int tool = 102;
	int option_index;
    
    class scene *scene;
    
    scene = new(class scene);
    
    read_tool_lib("toollib.csv");
    
    depth = inch_to_mm(0.044);

    while ((opt = getopt_long(argc, argv, "vfsil:t:d:D:", long_options, &option_index)) != -1) {
        switch (opt)
		{
			case 'v':
				verbose = 1;
				break;
			case 'f':
				scene->enable_finishing_pass();
				printf("Finishing pass enabled\n");
				break;
			case 's':
				scene->enable_skeleton_paths();
				printf("Skeleton path enabled\n");
				break;
			case 'i':
				scene->enable_inbetween_paths();
				printf("Inbetween paths enabled\n");
				break;
			case 'n':
				scene->enable_inlay();
				printf("Creating inlay plug\n");
				break;
			case 'l':
				read_tool_lib(optarg);
				break;	
			case 'd': /* inch */
				depth = inch_to_mm(strtod(optarg, NULL));
				printf("Depth set to %5.2fmm\n", depth);
				break;
			case 'D': /* metric mm*/
				depth = strtod(optarg, NULL);
				break;
			case 'c': /* inch */
				cutout_depth = inch_to_mm(strtod(optarg, NULL));
				printf("Enabling cutout to depth %5.2fmm\n", cutout_depth);
				break;
			case 't':
				int arg;
				arg = strtoull(optarg, NULL, 10);
				if (have_tool(arg)) {
					tool = arg;
					scene->push_tool(tool);
				} else {
					printf("Unknown tool requested\n");
					print_tools();
				}
				break;
			
			case 'h':
			default:
				usage();
		}
    }
	
    if (optind == argc) {
    	usage();
    }
    
    set_rippem(15000);
    set_retract_height_imperial(0.06);
    scene->set_default_tool(tool);

    for(; optind < argc; optind++) {      
		parse_svg_file(scene, argv[optind]);
		scene->set_filename(argv[optind]);
		
		scene->process_nesting();
		
		scene->create_toolpaths(-depth);
		
		scene->write_svg("output.svg");
		scene->write_gcode("output.nc");
		if (scene->inlay_plug)
			scene->inlay_plug->write_svg("inlay.svg");
    }
    
    return EXIT_SUCCESS;
}