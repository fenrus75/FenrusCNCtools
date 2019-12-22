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



int main(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "vf")) != -1) {
        switch (opt)
	{
			case 'v':
				verbose = 1;
				break;
			case 'f':
				enable_finishing_pass();
				printf("Finishing pass enabled\n");
				break;
				
			
			default:
				printf("Usage:\n\ttoolpath [-f] <file.svg>\n");
				return EXIT_SUCCESS;
	}
    }

    if (optind == argc) {
        printf("Usage:\n\ttoolpath [-f] <file.svg>\n");
        return EXIT_SUCCESS;
    }
    
    set_tool_imperial("T102", 0.125, 0.125/2, 0.045, 50, 15);
    set_rippem(15000);
    set_retract_height_imperial(0.06);

    for(; optind < argc; optind++) {      
		parse_svg_file(argv[optind]);
		
		process_nesting();
		
		create_toolpaths(inch_to_mm(-0.125));
		consolidate_toolpaths();
		
		write_svg("output.svg");
		write_gcode("output.nc");
    }
    return EXIT_SUCCESS;
}