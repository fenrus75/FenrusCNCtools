
#ifndef __INCLUDE_GUARD_FENRUS_H__
#define __INCLUDE_GUARD_FENRUS_H__

struct triangle {
	float vertex[3][3];
};

extern int read_stl_file(char *filename);

extern void set_max_triangles(int count);
extern void push_triangle(float v1[3], float v2[3], float v3[3]);
extern void print_triangle_stats(void);

#endif