/*
 * (C) Copyright 2020  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */

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
