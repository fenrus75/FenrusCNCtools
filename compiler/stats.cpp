#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

int stat_pass_raw_to_movement;
int stat_pass_vertical_G0;


void print_stats(void)
{
    printf("Pass statistics\n");
    printf("\traw_to_movement      : %i\n", stat_pass_raw_to_movement);
    printf("\tvertical_G0          : %i\n", stat_pass_vertical_G0);
}