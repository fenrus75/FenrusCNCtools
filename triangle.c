#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "fenrus.h"



static int maxtriangle = 0;
static int current = 0;
static struct triangle *triangles;


static float minX = 100000;
static float maxX = -100000;
static float minY = 100000;
static float maxY = -100000;
static float minZ = 100000;
static float maxZ = -100000;

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

	minX = fminf(minX, v1[0]);
	minX = fminf(minX, v2[0]);
	minX = fminf(minX, v3[0]);
	maxX = fmaxf(maxX, v1[0]);
	maxX = fmaxf(maxX, v2[0]);
	maxX = fmaxf(maxX, v3[0]);

	minY = fminf(minY, v1[1]);
	minY = fminf(minY, v2[1]);
	minY = fminf(minY, v3[1]);
	maxY = fmaxf(maxY, v1[1]);
	maxY = fmaxf(maxY, v2[1]);
	maxY = fmaxf(maxY, v3[1]);

	minZ = fminf(minZ, v1[2]);
	minZ = fminf(minZ, v2[2]);
	minZ = fminf(minZ, v3[2]);
	maxZ = fmaxf(maxZ, v1[2]);
	maxZ = fmaxf(maxZ, v2[2]);
	maxZ = fmaxf(maxZ, v3[2]);


	current++;
}


void print_triangle_stats(void)
{
	printf("Space allocated for triangles : %i\n", maxtriangle);
	printf("Actual number of triangles    : %i\n", current);
	printf("Span of the design	      : (%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f) \n",
			minX, minY, minZ, maxX, maxY, maxZ);
}