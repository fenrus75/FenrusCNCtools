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
static int maxl2buckets = 0;
static int current = 0;
static int nrvertical;
static struct triangle *triangles;

static struct bucket *buckets;
static int nrbuckets = 0;
static struct l2bucket *l2buckets;
static int nrl2buckets = 0;


static float minX = 100000;
static float maxX = -100000;
static float minY = 100000;
static float maxY = -100000;
static float minZ = 100000;
static float maxZ = -100000;


static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}

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

void set_max_l2buckets(int count)
{
	maxl2buckets = count;
	l2buckets = realloc(l2buckets, count * sizeof(struct l2bucket));
}

struct bucket *allocate_bucket(void)
{
	int j;
	if (nrbuckets >= maxbuckets - 1)
		set_max_buckets(nrbuckets + 16);

	nrbuckets ++;
	for (j = 0; j < BUCKETSIZE; j++)
		buckets[nrbuckets - 1].triangles[j] = -1;
	buckets[nrbuckets - 1].status = 0;
	return &buckets[nrbuckets - 1];
}

struct l2bucket *allocate_l2bucket(void)
{
	int j;
	if (nrbuckets >= maxl2buckets - 1)
		set_max_l2buckets(nrl2buckets + 16);

	nrl2buckets ++;
	for (j = 0; j < L2BUCKETSIZE; j++)
		l2buckets[nrl2buckets - 1].buckets[j] = NULL;
	return &l2buckets[nrl2buckets - 1];
}


void push_triangle(float v1[3], float v2[3], float v3[3], float norm[3])
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

	triangles[current].normal[0] = norm[0];
	triangles[current].normal[1] = norm[1];
	triangles[current].normal[2] = norm[2];



	if (fabs(norm[2]) < 0.001 && fabs(norm[0])+fabs(norm[1]) > 0.01) {
		triangles[current].vertical = 1;
		nrvertical++;
	}


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

void normalize_design_to_offset(double offsetpct)
{
	int i;
	double Zadj;

	Zadj = ((100 - offsetpct) * minZ + offsetpct * maxZ) / 100;

	for (i = 0; i < current; i++) {
		triangles[i].vertex[0][0] -= minX;
		triangles[i].vertex[1][0] -= minX;
		triangles[i].vertex[2][0] -= minX;

		triangles[i].vertex[0][1] -= minY;
		triangles[i].vertex[1][1] -= minY;
		triangles[i].vertex[2][1] -= minY;

		triangles[i].vertex[0][2] -= Zadj;
		triangles[i].vertex[1][2] -= Zadj;
		triangles[i].vertex[2][2] -= Zadj;
	}

	maxX = maxX - minX;
	minX = 0;
	maxY = maxY - minY;
	minY = 0;
	maxZ = maxZ - Zadj;
	minZ = minZ - Zadj;
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

void scale_design_Z(double newheight, double z_offset)
{
	double factor;
	int i;

	normalize_design_to_offset(100.0 * z_offset / newheight);

	factor = (newheight) / (maxZ);

	
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
		int reach;
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

		reach = current;
		if (reach > i + 50000)
			reach = i + 50000;
	
		for (j = i + 1; j < reach && bucketptr < BUCKETSIZE; j++)	{
			if (triangles[j].status == 0 && triangles[j].maxX <= rXmax && triangles[j].maxY <= rYmax && triangles[j].minY >= rYmin &&  triangles[j].minX >= rXmin) {
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

		bucket->minX = Xmin - 0.001; /* subtract a little to cope with rounding */
		bucket->minY = Ymin - 0.001;
		bucket->maxX = Xmax + 0.001;
		bucket->maxY = Ymax + 0.001;
	}
	printf("Created %i buckets\n", nrbuckets);
	slop = fmax(stl_image_X(), stl_image_Y())/10;
	maxslop = slop * 2;

	for (i = 0; i < nrbuckets; i++) {
		int j;
		double Xmax, Ymax, Xmin, Ymin;
		double rXmax, rYmax, rXmin, rYmin;
		int bucketptr = 0;
		struct l2bucket *l2bucket;

		if (buckets[i].status)
			continue;

		l2bucket = allocate_l2bucket();
		Xmax = buckets[i].maxX;
		Xmin = buckets[i].minX;
		Ymax = buckets[i].maxY;
		Ymin = buckets[i].minY;

		rXmax = Xmax + slop;
		rYmax = Ymax + slop;
		rXmin = Xmin - slop;
		rYmin = Ymin - slop;

		l2bucket->buckets[bucketptr++] = &buckets[i];
		buckets[i].status = 1;
	
		for (j = i + 1; j < nrbuckets && bucketptr < L2BUCKETSIZE; j++)	{
			if (buckets[j].status == 0 && buckets[j].maxX <= rXmax && buckets[j].maxY <= rYmax && buckets[j].minY >= rYmin &&  buckets[j].minX >= rXmin) {
				Xmax = fmax(Xmax, buckets[j].maxX);
				Ymax = fmax(Ymax, buckets[j].maxY);
				Xmin = fmin(Xmin, buckets[j].minX);
				Ymin = fmin(Ymin, buckets[j].minY);
				l2bucket->buckets[bucketptr++] = &buckets[j];
				buckets[j].status = 1;				
			}				
		}

		if (bucketptr >= L2BUCKETSIZE - 4)
			slop = slop * 0.9;
		if (bucketptr < L2BUCKETSIZE / 8)
			slop = fmin(slop * 1.1, maxslop);
		if (bucketptr < L2BUCKETSIZE / 2)
			slop = fmin(slop * 1.05, maxslop);

		l2bucket->minX = Xmin;
		l2bucket->minY = Ymin;
		l2bucket->maxX = Xmax;
		l2bucket->maxY = Ymax;
	}
	printf("Created %i L2 buckets\n", nrl2buckets);
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
	vprintf("      of which are vertical   : %i\n", nrvertical);
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
	int i, b, j, b2;

	if (nrbuckets == 0)
		make_buckets();

	for (b2 = 0; b2 < nrl2buckets; b2++) {
		if (l2buckets[b2].minX > X)
			continue;
		if (l2buckets[b2].minY > Y)
			continue;
		if (l2buckets[b2].maxX < X)
			continue;
		if (l2buckets[b2].maxY < Y)
			continue;


		for (b = 0; b < L2BUCKETSIZE; b++) {
			if (l2buckets[b2].buckets[b] == NULL)
				break;

			if (l2buckets[b2].buckets[b]->minX > X)
				continue;
			if (l2buckets[b2].buckets[b]->minY > Y)
				continue;
			if (l2buckets[b2].buckets[b]->maxX < X)
				continue;
			if (l2buckets[b2].buckets[b]->maxY < Y)
				continue;

			for (j = 0; j < BUCKETSIZE; j++) {
				double newZ;
				i = l2buckets[b2].buckets[b]->triangles[j];
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
	}
#if 0
	if (value < 0)
		value = 0;
#endif

	return value;
}


static struct line *lines;
static struct line *outlines;
static int linecount = 0;

static void push_line(double X1, double Y1, double X2, double Y2, double nX, double nY) 
{
	int i;
	for (i = 0; i < linecount; i++) {
		if (approx4(X1, lines[i].X1) && approx4(X2, lines[i].X2) &&  approx4(Y1, lines[i].Y1) &&  approx4(Y2, lines[i].Y2)) {
			lines[i].count++;
			return;
		}
	}
	double len;
	len=sqrt(nX*nX+nY*nY);
	if (fabs(len) < 0.01)
		return;


	lines[linecount].X1 = X1;
	lines[linecount].X2 = X2;
	lines[linecount].Y1 = Y1;
	lines[linecount].Y2 = Y2;
	/* make sure nX/nY are normalized to 1 */
	lines[linecount].nX = nX/len;
	lines[linecount].nY = nY/len;
	lines[linecount].valid = 1;
	lines[linecount].count = 1;
	linecount++;
}

static void do_outlines(double distance)
{
	int i;
	for (i = 0; i < linecount; i++) {
		double mX, mY;
		double vX,vY, len;
		double l1,l2;
		int match = 0;
		int j;

		if (lines[i].valid != 1)
			continue;

		mX = (lines[i].X1 + lines[i].X2)/2;
		mY = (lines[i].Y1 + lines[i].Y2)/2;
		vX = lines[i].X2 - mX;
		vY = lines[i].Y2 - mY;
		len = sqrt(vX*vX + vY*vY);
		if (fabs(len) < 0.01)
			continue;
		vX = vX/len;
		vY = vY/len;

		l1 = dist(mX, mY, lines[i].X1, lines[i].Y1);
		l2 = dist(mX, mY, lines[i].X2, lines[i].Y2);

		mX = mX + distance * lines[i].nX;
		mY = mY + distance * lines[i].nY;
		double oldl1 = l1;

		/* lets go find a match for X1/Y1 */
		for (j = 0 ; j < linecount; j++) {
			if (lines[j].valid != 1 || i == j)
				continue;
			if ( (approx3(lines[i].X1,lines[j].X1) || approx3(lines[i].X1,lines[j].X2)) && 			
				 (approx3(lines[i].Y1,lines[j].Y1) || approx3(lines[i].Y1,lines[j].Y2))) {
				double m2X, m2Y, v2X, v2Y;

				outlines[i].prev = j;
				
				m2X = (lines[j].X1 + lines[j].X2)/2;
				m2Y = (lines[j].Y1 + lines[j].Y2)/2;
				v2X = lines[j].X2 - m2X;
				v2Y = lines[j].Y2 - m2Y;
				len = sqrt(v2X*v2X + v2Y*v2Y);
				if (fabs(len) < 0.0000001)
					continue;
				v2X = v2X/len;
				v2Y = v2Y/len;
				m2X = m2X + distance * lines[j].nX;
				m2Y = m2Y + distance * lines[j].nY;


				/* solve for l1:
					mX + l1 * vX == m2X + k1 * v2X
					mY + l1 * vY == m2Y + k1 * v2Y

					k1 = (mY + l1 * vY - m2Y)/v2Y
					l1 * vX == m2X + (mY + l1 * vY - m2Y)/v2Y * v2X - mX
					l1 (vX - vY/v2Y * v2X) == m2X + v2X * mY / v2Y - m2Y * v2X/v2Y - mX

					or

					k1 = (mX + l1 * vX - m2X)/v2X;
					l1 (vY - vX/v2X * v2Y) == m2Y + v2Y * mX / v2X - m2X * v2Y/v2X - mY
				
				 */

				if (fabs(v2Y) > 0.01 || fabs(v2X < 0.0001)) {
					double left, right;
					left = vX - vY/v2Y * v2X;
					right = m2X + v2X * mY / v2Y - m2Y * v2X/v2Y - mX;
					if (left != 0)
						l1 = - right / left;
				} else {
					double left, right;
					left = vY - vX/v2X * v2Y;
					right = m2Y + v2Y * mX / v2X - m2X * v2Y/v2X - mY;
					if (left != 0)
						l1 = - right / left;
				}
				match++;
//				printf("l1 %5.2f   oldl1 %5.2f\n", l1, oldl1);
				if (l1/oldl1 > 3) {
					l1 = - 10;
				}
			}
		
		}


		double oldl2 = l2;

		/* lets go find a match for X2/Y2 */
		for (j = 0 ; j < linecount; j++) {
			if (!lines[j].valid || i == j)
				continue;
			if ( (approx4(lines[i].X2,lines[j].X1) || approx4(lines[i].X2,lines[j].X2)) && 			
				 (approx4(lines[i].Y2,lines[j].Y1) || approx4(lines[i].Y2,lines[j].Y2))) {
				double m2X, m2Y, v2X, v2Y;

				outlines[i].next = j;
				
				m2X = (lines[j].X1 + lines[j].X2)/2;
				m2Y = (lines[j].Y1 + lines[j].Y2)/2;
				v2X = lines[j].X2 - m2X;
				v2Y = lines[j].Y2 - m2Y;
				len = sqrt(v2X*v2X + v2Y*v2Y);
				if (fabs(len) < 0.0000001)
					continue;
				v2X = v2X/len;
				v2Y = v2Y/len;
				m2X = m2X + distance * lines[j].nX;
				m2Y = m2Y + distance * lines[j].nY;


				/* solve for l1:
					mX + l1 * vX == m2X + k1 * v2X
					mY + l1 * vY == m2Y + k1 * v2Y

					k1 = (mY + l1 * vY - m2Y)/v2Y
					l1 * vX == m2X + (mY + l1 * vY - m2Y)/v2Y * v2X - mX
					l1 (vX - vY/v2Y * v2X) == m2X + v2X * mY / v2Y - m2Y * v2X/v2Y - mX

					or

					k1 = (mX + l1 * vX - m2X)/v2X;
					l1 (vY - vX/v2X * v2Y) == m2Y + v2Y * mX / v2X - m2X * v2Y/v2X - mY
				
				 */

				if (fabs(v2Y) > 0.11 || fabs(v2X < 0.0001)) {
					double left, right;
					left = vX - vY/v2Y * v2X;
					right = m2X + v2X * mY / v2Y - m2Y * v2X/v2Y - mX;
					if (left != 0)
						l2 = right / left;
				} else {
					double left, right;
					left = vY - vX/v2X * v2Y;
					right = m2Y + v2Y * mX / v2X - m2X * v2Y/v2X - mY;
					if (left != 0)
						l2 = right / left;
				}
				match++;
				if (l2/oldl2 > 3) 
					l2 = -10;
			}
		
		}

		outlines[i].X1 = mX - (l1) * vX;
		outlines[i].X2 = mX + (l2) * vX;
		outlines[i].Y1 = mY - (l1) * vY;
		outlines[i].Y2 = mY + (l2) * vY;
		outlines[i].count = lines[i].count;
		if (l1 > 0 && l2 > 0) {
			outlines[i].valid = 1;
		} else {
			outlines[i].valid = 0;
		}
	}
}

void cleanup_outlines(void)
{
	int i;
	for (i = 0; i < linecount; i++) {
		int p;
		if (outlines[i].valid != 1)
			continue;
		if (outlines[i].count < 2)
			continue;
		p = outlines[i].prev;
		if (outlines[p].valid == 1)
			outlines[p].count++;
		p = outlines[i].next;
		if (outlines[p].valid == 1)
			outlines[p].count++;
	}
	for (i = 0; i < linecount; i++) {
		if (outlines[i].valid != 1)
			continue;
		if (outlines[i].count < 2)
			outlines[i].valid = 0;
	}
}

struct line * stl_vertical_triangles(double radius)
{
	int i;
	FILE *output;
	if (lines)
		free(lines);

	if (nrvertical == 0 || nrvertical > 20000)
		return NULL;

	nrvertical += 8;
	lines = calloc(sizeof(struct line), nrvertical + 5);
	if (outlines)
		free(outlines);
	linecount = 0;

	outlines = calloc(sizeof(struct line), nrvertical + 5);
	for (i = 0; i < nrvertical + 1; i++) {
		outlines[i].valid = -1;
		outlines[i].X1 = -4000;
		outlines[i].Y1 = -4000;
		outlines[i].X2 = -4000;
		outlines[i].Y2 = -4000;
		lines[i].X1 = -4000;
		lines[i].Y1 = -4000;
		lines[i].X2 = -4000;
		lines[i].Y2 = -4000;
		outlines[i].prev = -1;
		outlines[i].next = -1;
	}
#if 0
	push_line(0, 0, 0, stl_image_Y(), 1, 0);
	push_line(0, stl_image_Y(), stl_image_X(), stl_image_Y(), 0, -1);
	push_line(stl_image_X(), stl_image_Y(), stl_image_X(), 0, -1, 0);
	push_line(stl_image_X(), 0, 0, 0, 0, 1);
	push_line(0, 0, 0, stl_image_Y(), 1, 0);
	push_line(0, stl_image_Y(), stl_image_X(), stl_image_Y(), 0, -1);
	push_line(stl_image_X(), stl_image_Y(), stl_image_X(), 0, -1, 0);
	push_line(stl_image_X(), 0, 0, 0, 0, 1);
#endif
	for (i = 0; i < current; i++) {
		double X1, Y1, X2, Y2;

		if (!triangles[i].vertical)
			continue;
		X1 = triangles[i].minX;
		Y1 = -1000000;
		X2 = triangles[i].maxX;
		Y2 = -1000000;
		if (X1 == triangles[i].vertex[0][0])
			Y1 = triangles[i].vertex[0][1];
		else if (X2 == triangles[i].vertex[0][0])
				Y2 = triangles[i].vertex[0][1];
		if (X1 == triangles[i].vertex[1][0])
			Y1 = triangles[i].vertex[1][1];
		else if (X2 == triangles[i].vertex[1][0])
			Y2 = triangles[i].vertex[1][1];
		if (X1 == triangles[i].vertex[2][0])
			Y1 = triangles[i].vertex[2][1];
		else if (X2 == triangles[i].vertex[2][0])
				Y2 = triangles[i].vertex[2][1];

		if (Y1 > -100000 && Y2 > -100000) 
			push_line(X1, Y1, X2, Y2, triangles[i].normal[0], triangles[i].normal[1]);
	}

	do_outlines(radius);
	cleanup_outlines();

	if (verbose) {
		output = fopen("lines.svg", "w");
		fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
		fprintf(output, "<svg width=\"50px\" height=\"50px\" xmlns=\"http://www.w3.org/2000/svg\">\n");


		for (i = 0; i < linecount; i++)
			if (lines[i].valid == 1)
				fprintf(output, "<line x1=\"%5.4f\" y1=\"%5.4f\" x2=\"%5.4f\" y2=\"%5.4f\"  stroke=\"purple\" stroke-width=\"0.3\"/>\n", lines[i].X1, lines[i].Y1, lines[i].X2, lines[i].Y2);
		for (i = 0; i < linecount; i++)
			if (outlines[i].valid == 1)
				fprintf(output, "<line x1=\"%5.4f\" y1=\"%5.4f\" x2=\"%5.4f\" y2=\"%5.4f\"  stroke=\"green\" stroke-width=\"0.3\"/>\n", outlines[i].X1, outlines[i].Y1, outlines[i].X2, outlines[i].Y2);
			else
				fprintf(output, "<line x1=\"%5.4f\" y1=\"%5.4f\" x2=\"%5.4f\" y2=\"%5.4f\"  stroke=\"red\" stroke-width=\"0.3\"/>\n", outlines[i].X1, outlines[i].Y1, outlines[i].X2, outlines[i].Y2);
		fprintf(output, "</svg>\n");
		fclose(output);
	}
	return outlines;
}