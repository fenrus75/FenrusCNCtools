#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fenrus.h"



static int maxtriangle = 0;
static int current = 0;
static struct triangle *triangles;



void set_max_triangles(int count)
{
	maxtriangle = count;
	triangles = reallocarray(triangles, count, sizeof(struct triangle));
}


void push_triangle(float v1[3], float v2[3], float v3[3])
{
	if (current >= maxtriangle)
		set_max_triangles(current + 16);

	triangles[current].vertex[0][0] = v1[0];
	triangles[current].vertex[0][0] = v1[1];
	triangles[current].vertex[0][0] = v1[2];

	triangles[current].vertex[1][0] = v2[0];
	triangles[current].vertex[1][0] = v2[1];
	triangles[current].vertex[1][0] = v2[2];

	triangles[current].vertex[2][0] = v3[0];
	triangles[current].vertex[2][0] = v3[1];
	triangles[current].vertex[2][0] = v3[2];

	current++;
}


void print_triangle_stats(void)
{
	printf("Space allocated for triangles : %i\n", maxtriangle);
	printf("Actual number of triangles    : %i\n", current);
}