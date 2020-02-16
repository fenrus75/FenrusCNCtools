/*
 * (C) Copyright 2020  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>


#include "gcodecheck.h"

int verbose = 0;
int errorcode = 0;
int quiet = 1;

void usage(void)
{
	printf("Usage:\n\tgcodecheck [options] <file.nc>\n");
	printf("\t--verbose         	(-v)    verbose output\n");
	printf("\t--library <file>  	(-l)	load CC .csv tool file\n");
	exit(EXIT_SUCCESS);
}

static struct option long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument,       0, 'v'},
          {"library",    required_argument, 0, 'l'},
          {0, 0, 0, 0}
        };

int main(int argc, char **argv)
{
    int opt;
	int option_index;

	read_tool_lib("toollib.csv");


    while ((opt = getopt_long(argc, argv, "vhl:", long_options, &option_index)) != -1) {
        switch (opt)
		{
			case 'v':
				verbose = 1;
				break;
			case 'l':
				read_tool_lib(optarg);
				break;	
			case 'h':
			default:
				usage();
		}
    }
	
    if (optind == argc) {
    	usage();
    }
    
	for(; optind < argc; optind++) {      
		char filename[8192];
		char *c;
		read_gcode(argv[optind]);
		strcpy(filename, argv[optind]);
		c = strstr(filename, ".nc");
		if (!c)
			continue;
		strcpy(c, ".fingerprint");

		if (access(filename, R_OK) == 0) {
			verify_fingerprint(filename);
		} else {
			FILE *output;
			output = fopen(filename, "w");
			print_state(output);
			fclose(output);
		}

	}
	return errorcode;
}
