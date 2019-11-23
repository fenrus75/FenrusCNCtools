#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "fenrus.h"

struct stltriangle {
	float normal[3];
	float vertex1[3];
	float vertex2[3];
	float vertex3[3];
	uint16_t attribute;
} __attribute__((packed));


void print_stl_triangle(struct stltriangle *t)
{
	printf("\t(%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f) - (%5.1f, %5.1f, %5.1f)  %i\n",
		t->vertex1[0], t->vertex1[1], t->vertex1[2],
		t->vertex2[0], t->vertex2[1], t->vertex2[2],
		t->vertex3[0], t->vertex3[1], t->vertex3[2],
		t->attribute);
}


int read_stl_file(char *filename)
{
	FILE *file;
	char header[80];
	uint32_t trianglecount;
	int ret;
	uint32_t i;

	file = fopen(filename, "rb");
	if (!file) {
		printf("Failed to open file %s: %s\n", filename, strerror(errno));
		return -1;
	}
	
	ret = fread(header, 1, 80, file);
	if (ret <= 0) {
		printf("STL file too short\n");
		fclose(file);
		return -1;
	}
	ret = fread(&trianglecount, 1, 4, file);
	set_max_triangles(trianglecount);

	for (i = 0; i < trianglecount; i++) {
		struct stltriangle t;
		ret = fread(&t, 1, sizeof(struct stltriangle), file);
		if (ret < 1)
			break;
		push_triangle(t.vertex1, t.vertex2, t.vertex3);
	}

	fclose(file);
	return 0;
}
