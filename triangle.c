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
	triangles[current].vertex[0][1] = v1[1];
	triangles[current].vertex[0][2] = v1[2];

	triangles[current].vertex[1][0] = v2[0];
	triangles[current].vertex[1][1] = v2[1];
	triangles[current].vertex[1][2] = v2[2];

	triangles[current].vertex[2][0] = v3[0];
	triangles[current].vertex[2][1] = v3[1];
	triangles[current].vertex[2][2] = v3[2];

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

void normalize_design_to_zero(void)
{
	int i;
	for (i = 0; i < current; i++) {
		triangles[i].vertex[0][0] -= minX;
		triangles[i].vertex[1][0] -= minX;
		triangles[i].vertex[2][0] -= minX;

		triangles[i].vertex[0][1] -= minY;
		triangles[i].vertex[1][1] -= minY;
		triangles[i].vertex[2][1] -= minY;

		triangles[i].vertex[0][2] -= minZ;
		triangles[i].vertex[1][2] -= minZ;
		triangles[i].vertex[2][2] -= minZ;
	}

	maxX = maxX - minX;
	minX = 0;
	maxY = maxY - minY;
	minY = 0;
	maxZ = maxZ - minZ;
	minZ = 0;
}
void scale_design(double newsize)
{
	double factor ;
	int i;

	normalize_design_to_zero();

	factor = newsize / maxX;
	factor = fmin(factor, newsize / maxY);
		

	
	for (i = 0; i < current; i++) {
		triangles[i].vertex[0][0] *= factor;
		triangles[i].vertex[1][0] *= factor;
		triangles[i].vertex[2][0] *= factor;

		triangles[i].vertex[0][1] *= factor;
		triangles[i].vertex[1][1] *= factor;
		triangles[i].vertex[2][1] *= factor;

		triangles[i].vertex[0][2] *= factor;
		triangles[i].vertex[1][2] *= factor;
		triangles[i].vertex[2][2] *= factor;

		triangles[i].minX = fminf(triangles[i].vertex[0][0], triangles[i].vertex[1][0]);
		triangles[i].minX = fminf(triangles[i].minX,         triangles[i].vertex[2][0]);

		triangles[i].maxX = fmaxf(triangles[i].vertex[0][0], triangles[i].vertex[1][0]);
		triangles[i].maxX = fmaxf(triangles[i].maxX,         triangles[i].vertex[2][0]);

		triangles[i].minY = fminf(triangles[i].vertex[0][1], triangles[i].vertex[1][1]);
		triangles[i].minY = fminf(triangles[i].minX,         triangles[i].vertex[2][1]);

		triangles[i].maxY = fmaxf(triangles[i].vertex[0][1], triangles[i].vertex[1][1]);
		triangles[i].maxY = fmaxf(triangles[i].maxY,         triangles[i].vertex[2][1]);
	}

	maxX *= factor;
	maxY *= factor;
	maxZ *= factor;
}

int image_X(void)
{
	return maxX + 0.999;
}

int image_Y(void)
{
	return maxY + 0.999;
}

double scale_Z(void)
{
	return 255.0 / maxZ;
}
void print_triangle_stats(void)
{
	printf("Space allocated for triangles : %i\n", maxtriangle);
	printf("Actual number of triangles    : %i\n", current);
	printf("Span of the design	      : (%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f) \n",
			minX, minY, minZ, maxX, maxY, maxZ);
	printf("Image size                    : %i x %i \n", image_X(), image_Y());
}

static double point_to_the_left(double X, double Y, double AX, double AY, double BX, double BY)
{
	return (BX-AX)*(Y-AY) - (BY-AY)*(X-AX);
}


static int within_triangle(double X, double Y, int i)
{
	double det1, det2, det3;

	int has_pos, has_neg;

	det1 = point_to_the_left(X, Y, triangles[i].vertex[0][0], triangles[i].vertex[0][1], triangles[i].vertex[1][0], triangles[i].vertex[1][1]);
	det2 = point_to_the_left(X, Y, triangles[i].vertex[1][0], triangles[i].vertex[1][1], triangles[i].vertex[2][0], triangles[i].vertex[2][1]);
	det3 = point_to_the_left(X, Y, triangles[i].vertex[2][0], triangles[i].vertex[2][1], triangles[i].vertex[0][0], triangles[i].vertex[0][1]);

	has_neg = (det1 < 0) || (det2 < 0) || (det3 < 0);
	has_pos = (det1 > 0) || (det2 > 0) || (det3 > 0);

	return !(has_neg && has_pos);

}

double get_height(double X, double Y)
{
	double value = 0;
	int i;
	for (i = 0; i < current; i++) {
		double newZ;
#if 0
		if (triangles[i].minX > X - 1)
			continue;
		if (triangles[i].minY > Y - 1)
			continue;
		if (triangles[i].maxX < X + 1)
			continue;
		if (triangles[i].maxY < Y + 1)
			continue;
#endif
		if (!within_triangle(X, Y, i))
			continue;

		newZ = (fmax(triangles[i].vertex[0][2], triangles[i].vertex[1][2]) + fmin(triangles[i].vertex[0][2], triangles[i].vertex[1][2]))/2;

;

		value = fmax(newZ, value);
	}
	return value;
}

