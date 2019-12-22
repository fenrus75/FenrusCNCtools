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
int verbose = 0;



int main(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "r:v")) != -1) {
        switch (opt)
	{
			case 'r':
//				resolution = strtoull(optarg, NULL, 10);
//				printf("Setting resolution to %i\n", resolution);
				break;
			case 'v':
				verbose = 1;
				break;
			
			default:
				printf("Usage:\n\ttoolpath <file.svg>\n");
				return EXIT_SUCCESS;
	}
    }

    if (optind == argc) {
        printf("Usage:\n\ttoolpath <file.svg>\n");
        return EXIT_SUCCESS;
    }
    
    set_tool("T102", 0.125, 0.125/2, 0.045, 50, 15);
    set_rippem(15000);
    set_retract_height(0.06);
    enable_finishing_pass();

    for(; optind < argc; optind++) {      
		parse_svg_file(argv[optind]);
		
		process_nesting();
		
		create_toolpaths(-0.125);
		consolidate_toolpaths();
		
		write_svg("output.svg");
		write_gcode("output.nc");
    }
    return EXIT_SUCCESS;
}