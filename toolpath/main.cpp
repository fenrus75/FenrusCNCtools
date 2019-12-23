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

extern "C" {
    #include "toolpath.h"
}

int verbose = 0;
int want_skeleton_path = 0;

void usage(void)
{
	printf("Usage:\n\ttoolpath [-f] [-s] [-t <nr] <file.svg>\n");
	exit(EXIT_SUCCESS);
}


int main(int argc, char **argv)
{
    int opt;
    int tool = 102;
    
    read_tool_lib("toollib.csv");

    while ((opt = getopt(argc, argv, "vfst:")) != -1) {
        switch (opt)
	{
			case 'v':
				verbose = 1;
				break;
			case 'f':
				enable_finishing_pass();
				printf("Finishing pass enabled\n");
				break;
			case 's':
				want_skeleton_path = 1;
				printf("Skeleton path enabled\n");
				break;
			case 't':
				int arg;
				arg = strtoull(optarg, NULL, 10);
				if (have_tool(arg)) {
					tool = arg;
				} else {
					printf("Unknown tool requested\n");
					print_tools();
				}
				break;
			
			default:
				usage();
	}
    }
	
    activate_tool(tool);

    if (optind == argc) {
    	usage();
    }
    
    set_rippem(15000);
    set_retract_height_imperial(0.06);

    for(; optind < argc; optind++) {      
		parse_svg_file(argv[optind]);
		
		process_nesting();
		
		create_toolpaths(inch_to_mm(-0.044));
		consolidate_toolpaths();
		
		write_svg("output.svg");
		write_gcode("output.nc");
    }
    return EXIT_SUCCESS;
}