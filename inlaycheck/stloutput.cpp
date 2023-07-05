#include "inlay.h"

#include <cstdio>
#include <cstring>
#include <cmath>

static int triangles = 0;

static bool *cache;

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


static void write_point(FILE *file, int x, int y, double d00, double d01,double d10, double d11, bool filter)
{
    struct triangle t;
    
    if (d00 >= 0 && d01 >= 0 && d10 >= 0 && d11 >= 0 && filter)
        return;
        
    memset(&t, 0, sizeof(t));
    t.point0[0] = x;    
    t.point0[1] = y;    
    t.point0[2] = d00;    
    t.point1[0] = x + 1;    
    t.point1[1] = y;    
    t.point1[2] = d01;    
    t.point2[0] = x + 1;    
    t.point2[1] = y + 1;    
    t.point2[2] = d11;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;
    t.point0[0] = x;    
    t.point0[1] = y;    
    t.point0[2] = d00;    
    t.point1[0] = x;    
    t.point1[1] = y + 1;    
    t.point1[2] = d01;    
    t.point2[0] = x + 1;    
    t.point2[1] = y + 1;    
    t.point2[2] = d11;    
    fwrite(&t, sizeof(t), 1, file);
    
    triangles ++;
    
}
static void write_point4(FILE *file, int x, int y, double d00, double d01,double d10,double d11, bool filter)
{
    struct triangle t;
    double dM = (d00+d01+d10+d11)/4;
    
    if (d00 >= 0 && d01 >= 0 && d10 >= 0 && d11 >= 0 && filter)
        return;
        
    if (fabs(d00-d01)<0.001 && fabs(d00-d10)<0.001 && fabs(d00-d11)<0.0010)
        return write_point(file, x,y,d00,d01,d10,d11, filter);
        
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




double plugtop(double d)
{
    if (d < 0)
        return 0.0;
    return d;
}

bool should_emit_plug(double d00, double d01,double d10,double d11, double offset)
{
    if (d00 - offset < 0)
        return true;
    if (d01 - offset < 0)
        return true;
    if (d10 - offset < 0)
        return true;
    if (d11 - offset < 0)
        return true;
        
        
    return false;
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
    
    cache = (bool *)calloc(base->width,base->height * sizeof(bool));
    
    
    
    file = fopen(filename, "w");
    fwrite(&header, 1, sizeof(header), file);
    
    for (y = base->minX; y < base->maxX; y++) {
        for (x =base->minX ; x < base->maxX; x++) {
            double b00 = base->get_height(x,y);
            double b01 = base->get_height(x,y+1);
            double b10 = base->get_height(x+1,y);
            double b11 = base->get_height(x+1,y+1);
            double p00 = plug->get_height(x,y);
            double p01 = plug->get_height(x,y+1);
            double p10 = plug->get_height(x+1,y);
            double p11 = plug->get_height(x+1,y+1);
//            double d = p - b - offset;
            
            if (export_base)
                write_point4(file, x, y, b00, b01, b10, b11,true);
            if (export_plug && should_emit_plug(p00, p01, p10, p11, offset)) {
                write_point4(file, x, y, p00 - offset, p01-offset, p10-offset,p11-offset, false);
                write_point4(file, x, y, plugtop(p00 - offset), plugtop(p01-offset), plugtop(p10-offset),plugtop(p11-offset), false);
                
            }
        }
    }
    fseek(file, 0, SEEK_SET);
    header.triangles = triangles;
    fwrite(&header, 1, sizeof(header), file);
    
    fclose(file);
    free(cache);
    printf("Exported %i triangles \n", triangles);
}