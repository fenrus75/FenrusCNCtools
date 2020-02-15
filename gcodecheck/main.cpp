#include <stdio.h>
#include <stdlib.h>

#include "gcodecheck.h"

int main(int argc, char **argv)
{
	int iter = 0;
	if (argc < 2) {
		printf("Usage:\n\tgcodecheck <filename>\n");
		exit(0);
	}
	
	read_gcode(argv[1]);

	return 0;
}
