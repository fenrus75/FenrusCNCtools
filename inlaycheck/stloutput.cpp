#include "inlay.h"

#include <cstdio>
#include <cstring>

static int triangles = 0;

struct stlheader {
    char sig[80];
    unsigned int triangles;
};

struct triangle {
    float normal[3];
    float point0[3];
    float point1[3];
    float point2[3];
    unsigned short pad;
} __attribute__((packed));


static void write_point(FILE *file, int x, int y, double depth)
{
    struct triangle t;
    
    if (depth == 0)
        return;
        
    memset(&t, 0, sizeof(t));
    t.point0[0] = x;    
    t.point0[1] = y;    
    t.point0[2] = depth;    
    t.point1[0] = x + 1;    
    t.point1[1] = y;    
    t.point1[2] = depth;    
    t.point2[0] = x + 1;    
    t.point2[1] = y + 1;    
    t.point2[2] = depth;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;
    t.point0[0] = x;    
    t.point0[1] = y;    
    t.point0[2] = depth;    
    t.point1[0] = x;    
    t.point1[1] = y + 1;    
    t.point1[2] = depth;    
    t.point2[0] = x + 1;    
    t.point2[1] = y + 1;    
    t.point2[2] = depth;    
    fwrite(&t, sizeof(t), 1, file);
    
    triangles ++;
    
}
static void write_point4(FILE *file, int x, int y, double d00, double d01,double d10,double d11)
{
    struct triangle t;
    double dM = (d00+d01+d10+d11)/4;
    
    if (d00 >= 0 && d01 >= 0 && d10 >= 0 && d11 >= 0)
        return;
        
    memset(&t, 0, sizeof(t));
    t.point0[0] = x;    
    t.point0[1] = y;    
    t.point0[2] = d00;    
    t.point1[0] = x + 1;    
    t.point1[1] = y;    
    t.point1[2] = d10;    
    t.point2[0] = x + 0.5;    
    t.point2[1] = y + 0.5;    
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;

    t.point0[0] = x + 1;    
    t.point0[1] = y + 1;    
    t.point0[2] = d11;    
    t.point1[0] = x + 1;    
    t.point1[1] = y;    
    t.point1[2] = d10;    
    t.point2[0] = x + 0.5;    
    t.point2[1] = y + 0.5;    
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;

    t.point0[0] = x + 1;    
    t.point0[1] = y + 1;    
    t.point0[2] = d11;    
    t.point1[0] = x;    
    t.point1[1] = y + 1;    
    t.point1[2] = d01;    
    t.point2[0] = x + 0.5;    
    t.point2[1] = y + 0.5;    
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;

    t.point0[0] = x;    
    t.point0[1] = y;    
    t.point0[2] = d00;    
    t.point1[0] = x;    
    t.point1[1] = y + 1;    
    t.point1[2] = d01;    
    t.point2[0] = x + 0.5;    
    t.point2[1] = y + 0.5;    
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;
    
}

void save_as_stl(const char *filename, render *base,render *plug, double offset, bool export_base, bool export_plug)
{
    FILE *file;
    int x,y;
    struct stlheader header;
    
    triangles = 0;
    
    printf("Size of triangle %li\n", sizeof(struct triangle));
    
    memset(&header, 0, sizeof(header));
    sprintf(header.sig, "Binary STL file");
    
    
    file = fopen(filename, "w");
    fwrite(&header, 1, sizeof(header), file);
    
    for (y = base->minX; y < base->maxX; y++) {
        for (x =base->minX ; x < base->maxX; x++) {
            double b00 = base->get_height(x,y);
            double b01 = base->get_height(x,y+1);
            double b10 = base->get_height(x+1,y);
            double b11 = base->get_height(x+1,y+1);
            double p = plug->get_height(x,y);
//            double d = p - b - offset;
            
            if (export_base)
                write_point4(file, x, y, b00, b01, b10, b11);
            if (export_plug && p - offset < 0.1) {
                double p00 = plug->get_height(x,y);
                double p01 = plug->get_height(x,y+1);
                double p10 = plug->get_height(x+1,y);
                double p11 = plug->get_height(x+1,y+1);
                write_point4(file, x, y, p00 - offset, p01-offset, p10-offset,p11-offset);
            }
        }
    }
    fseek(file, 0, SEEK_SET);
    header.triangles = triangles;
    fwrite(&header, 1, sizeof(header), file);
    
    fclose(file);
    printf("Exported %i triangles \n", triangles);
}