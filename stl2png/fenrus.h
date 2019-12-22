
#ifndef __INCLUDE_GUARD_FENRUS_H__
#define __INCLUDE_GUARD_FENRUS_H__

struct triangle {
	float vertex[3][3];
	float minX, minY, maxX, maxY;
};

extern int read_stl_file(char *filename);

extern void set_max_triangles(int count);
extern void push_triangle(float v1[3], float v2[3], float v3[3]);
extern void print_triangle_stats(void);
extern void normalize_design_to_zero(void);
extern void scale_design(double newsize);
extern int image_X(void);
extern int image_Y(void);
extern double scale_Z(void);
extern double get_height(double X, double Y);
extern void create_image(char *filename);
extern void reset_triangles(void);
#endif