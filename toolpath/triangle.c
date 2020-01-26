/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "fenrus.h"



static int maxtriangle = 0;
static int maxbuckets = 0;
static int current = 0;
static struct triangle *triangles;
static struct bucket *buckets;
static int nrbuckets = 0;


static float minX = 100000;
static float maxX = -100000;
static float minY = 100000;
static float maxY = -100000;
static float minZ = 100000;
static float maxZ = -100000;

void reset_triangles(void)
{
	free(triangles);
	triangles = NULL;
	current = 0;
	maxtriangle = 0;
	minX = 100000;
	minY = 100000;
	minZ = 100000;
	maxX = -100000;
	maxY = -100000;
	maxZ = -100000;
}

void set_max_triangles(int count)
{
	maxtriangle = count;
	triangles = realloc(triangles, count * sizeof(struct triangle));
}

void set_max_buckets(int count)
{
	maxbuckets = count;
	buckets = realloc(buckets, count * sizeof(struct bucket));
}

struct bucket *allocate_bucket(void)
{
	int j;
	if (nrbuckets >= maxbuckets - 1)
		set_max_buckets(nrbuckets + 16);

	nrbuckets ++;
	for (j = 0; j < BUCKETSIZE; j++)
		buckets[nrbuckets - 1].triangles[j] = -1;
	return &buckets[nrbuckets - 1];
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
		triangles[i].minY = fminf(triangles[i].minY,         triangles[i].vertex[2][1]);

		triangles[i].maxY = fmaxf(triangles[i].vertex[0][1], triangles[i].vertex[1][1]);
		triangles[i].maxY = fmaxf(triangles[i].maxY,         triangles[i].vertex[2][1]);
	}

	maxX *= factor;
	maxY *= factor;
	maxZ *= factor;
}

void scale_design_Z(double newheight)
{
	double factor ;
	int i;

	normalize_design_to_zero();

	factor = newheight / maxZ;
		

	
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
		triangles[i].minY = fminf(triangles[i].minY,         triangles[i].vertex[2][1]);

		triangles[i].maxY = fmaxf(triangles[i].vertex[0][1], triangles[i].vertex[1][1]);
		triangles[i].maxY = fmaxf(triangles[i].maxY,         triangles[i].vertex[2][1]);
	}

	maxX *= factor;
	maxY *= factor;
	maxZ *= factor;
}

void make_buckets(void)
{
	int i;
	double slop = fmax(stl_image_X(), stl_image_Y())/50;
	double maxslop = slop * 2;

	for (i = 0; i < current; i++) {
		int j;
		double Xmax, Ymax, Xmin, Ymin;
		double rXmax, rYmax, rXmin, rYmin;
		int bucketptr = 0;
		struct bucket *bucket;
		if (triangles[i].status)
			continue;

		bucket = allocate_bucket();
		Xmax = triangles[i].maxX;
		Xmin = triangles[i].minX;
		Ymax = triangles[i].maxY;
		Ymin = triangles[i].minY;

		rXmax = Xmax + slop;
		rYmax = Ymax + slop;
		rXmin = Xmin - slop;
		rYmin = Ymin - slop;

		bucket->triangles[bucketptr++] = i;
		triangles[i].status = 1;
	
		for (j = i + 1; j < current && bucketptr < BUCKETSIZE; j++)	{
			if (triangles[j].maxX <= rXmax && triangles[j].maxY <= rYmax && triangles[j].minY >= rYmin &&  triangles[j].minX >= rXmin) {
				Xmax = fmax(Xmax, triangles[j].maxX);
				Ymax = fmax(Ymax, triangles[j].maxY);
				Xmin = fmin(Xmin, triangles[j].minX);
				Ymin = fmin(Ymin, triangles[j].minY);
				bucket->triangles[bucketptr++] = j;
				triangles[j].status = 1;				
			}				
		}

		if (bucketptr >= BUCKETSIZE -5)
			slop = slop * 0.9;
		if (bucketptr < BUCKETSIZE / 8)
			slop = fmin(slop * 1.1, maxslop);
		if (bucketptr < BUCKETSIZE / 2)
			slop = fmin(slop * 1.05, maxslop);

		bucket->minX = Xmin;
		bucket->minY = Ymin;
		bucket->maxX = Xmax;
		bucket->maxY = Ymax;
	}
	printf("Created %i buckets\n", nrbuckets);
}

double stl_image_X(void)
{
	return maxX;
}

double stl_image_Y(void)
{
	return maxY;
}

double scale_Z(void)
{
	return 255.0 / maxZ;
}

void print_triangle_stats(void)
{
	int i;
	double sum = 0;
	printf("Number of triangles in file   : %i\n", current);
/*
	printf("Span of the design	      : (%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f) \n",
			minX, minY, minZ, maxX, maxY, maxZ);
*/
	printf("Image size                    : %5.2f  x %5.2f mm\n", stl_image_X(), stl_image_Y());
	printf("Image size                    : %5.2f\" x %5.2f\"\n", mm_to_inch(stl_image_X()), mm_to_inch(stl_image_Y()));


	if (current > 10000)
		printf("Large number of triangles, this may take some time\n");
	for (i = 0; i < current; i++) {
		double size;
		size = fmax(triangles[i].maxX-triangles[i].minX, triangles[i].maxY-triangles[i].minY);
		sum += size;
	}
	printf("Average triangle size: %5.2f\n", sum / current);
	make_buckets();
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


static float calc_Z(float X, float Y, int index)
{
	float det = (triangles[index].vertex[1][1] - triangles[index].vertex[2][1]) * 
		    (triangles[index].vertex[0][0] - triangles[index].vertex[2][0]) + 
                    (triangles[index].vertex[2][0] - triangles[index].vertex[1][0]) * 
		    (triangles[index].vertex[0][1] - triangles[index].vertex[2][1]);

	float l1 = ((triangles[index].vertex[1][1] - triangles[index].vertex[2][1]) * (X - triangles[index].vertex[2][0]) + (triangles[index].vertex[2][0] - triangles[index].vertex[1][0]) * (Y - triangles[index].vertex[2][1])) / det;
	float l2 = ((triangles[index].vertex[2][1] - triangles[index].vertex[0][1]) * (X - triangles[index].vertex[2][0]) + (triangles[index].vertex[0][0] - triangles[index].vertex[2][0]) * (Y - triangles[index].vertex[2][1])) / det;
	float l3 = 1.0f - l1 - l2;

	return l1 * triangles[index].vertex[0][2] + l2 * triangles[index].vertex[1][2] + l3 * triangles[index].vertex[2][2];
}

double get_height_old(double X, double Y)
{
	double value = 0;
	int i;
	for (i = 0; i < current; i++) {
		double newZ;

		/* first a few quick bounding box checks */
		if (triangles[i].minX > X)
			continue;
		if (triangles[i].minY > Y)
			continue;
		if (triangles[i].maxX < X)
			continue;
		if (triangles[i].maxY < Y)
			continue;

		/* then a more expensive detailed triangle test */
		if (!within_triangle(X, Y, i))
			continue;
		/* now calculate the Z height within the triangle */
		newZ = calc_Z(X, Y, i);

		value = fmax(newZ, value);
	}
	return value;
}

double get_height(double X, double Y)
{
	double value = 0;
	int i, b, j;

	if (nrbuckets == 0)
		make_buckets();

	for (b = 0; b < nrbuckets; b++) {

		if (buckets[b].minX > X)
			continue;
		if (buckets[b].minY > Y)
			continue;
		if (buckets[b].maxX < X)
			continue;
		if (buckets[b].maxY < Y)
			continue;

		for (j = 0; j < BUCKETSIZE; j++) {
			double newZ;
			i = buckets[b].triangles[j];
			if (i < 0)
				break;

			/* first a few quick bounding box checks */
			if (triangles[i].minX > X)
				continue;
			if (triangles[i].minY > Y)
				continue;
			if (triangles[i].maxX < X)
				continue;
			if (triangles[i].maxY < Y)
				continue;
	
			/* then a more expensive detailed triangle test */
			if (!within_triangle(X, Y, i))
				continue;
			/* now calculate the Z height within the triangle */
			newZ = calc_Z(X, Y, i);

			value = fmax(newZ, value);
		}
	}
	return value;
}

