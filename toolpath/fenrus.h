
#ifndef __INCLUDE_GUARD_FENRUS_H__
#define __INCLUDE_GUARD_FENRUS_H__

#include "toolpath.h"

struct triangle {
	float vertex[3][3];
	float normal[3];
	float minX, minY, maxX, maxY;
	int status;
	int vertical;
};

#define BUCKETSIZE 64
#define L2BUCKETSIZE 64
struct bucket {
	float minX, minY, maxX, maxY;
	int triangles[BUCKETSIZE];
	int status;
};

struct l2bucket {
	double minX, minY, maxX, maxY;
	struct bucket *buckets[L2BUCKETSIZE];
};

struct line {
	double X1, Y1, X2, Y2;
	double nX, nY;
	int valid;

	int prev, next;
};

extern void set_max_triangles(int count);
extern void push_triangle(float v1[3], float v2[3], float v3[3], float norm[3]);
extern void normalize_design_to_zero(void);
extern void scale_design(double newsize);
extern void scale_design_Z(double newsize, double z_offset);
extern void print_triangle_stats(void);
extern double stl_image_X(void);
extern double stl_image_Y(void);
extern double scale_Z(void);
extern double get_height(double X, double Y);
extern void reset_triangles(void);
extern struct line * stl_vertical_triangles(double radius);

#endif