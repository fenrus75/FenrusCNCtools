#include "inlay.h"

#include <cstdio>
#include <cstring>
#include <cmath>

static int triangles = 0;

static double zoomfactor = 1;


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


#define zoom(D) ((D) * zoomfactor)


static void write_point(FILE *file, int x, int y, double d00, double d01,double d10, double d11, bool filter, int width, double * cache)
{
    struct triangle t;
    
    if (d00 >= 0 && d01 >= 0 && d10 >= 0 && d11 >= 0 && filter)
        return;

    if (cache && cache[x + y * width] > 900) {
        cache[x + y * width] = d00;
        return;
    }

    memset(&t, 0, sizeof(t));
    t.point0[0] = zoom(x);    
    t.point0[1] = zoom(y);    
    t.point0[2] = d00;    
    t.point1[0] = zoom(x + 1);
    t.point1[1] = zoom(y);    
    t.point1[2] = d01;    
    t.point2[0] = zoom(x + 1);    
    t.point2[1] = zoom(y + 1);    
    t.point2[2] = d11;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;
    t.point0[0] = zoom(x);    
    t.point0[1] = zoom(y);    
    t.point0[2] = d00;    
    t.point1[0] = zoom(x);    
    t.point1[1] = zoom(y + 1);    
    t.point1[2] = d01;    
    t.point2[0] = zoom(x + 1);    
    t.point2[1] = zoom(y + 1);    
    t.point2[2] = d11;    
    fwrite(&t, sizeof(t), 1, file);
    
    triangles ++;

}

static void flush_cache(double *cache, FILE *file, int width, int height)
{
    int x, y;
    
    if (!cache)
        return;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            struct triangle t;
            int x2;
            double c = cache[x + y * width];
            if (c > 900)
                continue;
            x2 = x;
            while (x2 < width -1) {
                if (fabs(cache[x2 + 1 + y * width] - c) < 0.0001) 
                    x2++;
                else
                    break;
            }
            /* now we know we have a run from x to x2 (where x could be x2) */
            memset(&t, 0, sizeof(t));
            t.point0[0] = zoom(x);    
            t.point0[1] = zoom(y);    
            t.point0[2] = c;    
            t.point1[0] = zoom(x2 + 1);    
            t.point1[1] = zoom(y);    
            t.point1[2] = c;    
            t.point2[0] = zoom(x2 + 1);    
            t.point2[1] = zoom(y + 1);
            t.point2[2] = c;    
            fwrite(&t, sizeof(t), 1, file);
            triangles++;
            t.point0[0] = zoom(x);
            t.point0[1] = zoom(y);
            t.point0[2] = c;    
            t.point1[0] = zoom(x);
            t.point1[1] = zoom(y + 1);
            t.point1[2] = c;    
            t.point2[0] = zoom(x2 + 1);
            t.point2[1] = zoom(y + 1);
            t.point2[2] = c;    
            fwrite(&t, sizeof(t), 1, file);
    
            triangles ++;
            x = x2;
            
        }
    }
}

static void write_point4(FILE *file, int x, int y, double d00, double d01,double d10,double d11, bool filter, int width, double *cache)
{
    struct triangle t;
    double dM = (d00+d01+d10+d11)/4;
    
    if (d00 >= 0 && d01 >= 0 && d10 >= 0 && d11 >= 0 && filter)
        return;
        
    if (fabs(d00-d01)<0.001 && fabs(d00-d10)<0.001 && fabs(d00-d11)<0.0010)
        return write_point(file, x,y,d00,d01,d10,d11, filter, width, cache);
        
    memset(&t, 0, sizeof(t));
    t.point0[0] = zoom(x);
    t.point0[1] = zoom(y);    
    t.point0[2] = d00;    
    t.point1[0] = zoom(x + 1);    
    t.point1[1] = zoom(y);    
    t.point1[2] = d10;    
    t.point2[0] = zoom(x + 0.5);
    t.point2[1] = zoom(y + 0.5);
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;

    t.point0[0] = zoom(x + 1);
    t.point0[1] = zoom(y + 1);
    t.point0[2] = d11;    
    t.point1[0] = zoom(x + 1);
    t.point1[1] = zoom(y);
    t.point1[2] = d10;    
    t.point2[0] = zoom(x + 0.5);
    t.point2[1] = zoom(y + 0.5);
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;

    t.point0[0] = zoom(x + 1);    
    t.point0[1] = zoom(y + 1);    
    t.point0[2] = d11;    
    t.point1[0] = zoom(x);    
    t.point1[1] = zoom(y + 1);    
    t.point1[2] = d01;    
    t.point2[0] = zoom(x + 0.5);    
    t.point2[1] = zoom(y + 0.5);    
    t.point2[2] = dM;    
    fwrite(&t, sizeof(t), 1, file);
    triangles++;

    t.point0[0] = zoom(x);    
    t.point0[1] = zoom(y);    
    t.point0[2] = d00;    
    t.point1[0] = zoom(x);    
    t.point1[1] = zoom(y + 1);    
    t.point1[2] = d01;    
    t.point2[0] = zoom(x + 0.5);    
    t.point2[1] = zoom(y + 0.5);    
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

void save_as_stl(const char *filename, render *base,render *plug, double offset, bool export_base, bool export_plug, double zoomf)
{
    FILE *file;
    int x,y;
    struct stlheader header;
    double *basecache = NULL, *plugcache1 = NULL, *plugcache2 = NULL;
    
    zoomfactor = zoomf;
//    printf("Zoomfactor %5.2f\n", zoomfactor);
   
    triangles = 0;
    
//    printf("Size of triangle %li\n", sizeof(struct triangle));
    
    memset(&header, 0, sizeof(header));
    sprintf(header.sig, "Binary STL file");
    
    if (export_base) {
        basecache = (double *)calloc(base->width,base->height * sizeof(double));
        for (x = 0; x < base->width * base->height ; x++) basecache[x] = 1000;
    }
    if (export_plug) {
        plugcache1 = (double *)calloc(base->width,base->height * sizeof(double));
        for (x = 0; x < base->width * base->height ; x++) plugcache1[x] = 1000;
        plugcache2 = (double *)calloc(base->width,base->height * sizeof(double));
        for (x = 0; x < base->width * base->height ; x++) plugcache2[x] = 1000;
    }
    
    
    
    file = fopen(filename, "w");
    fwrite(&header, 1, sizeof(header), file);
    
    if (base->maxX > base->height)
        base->maxY = base->height;
    
    for (y = base->minY - 2/zoomf; y < base->maxY + 2/zoomf; y++) {
        for (x =base->minX - 2/zoomf ; x < base->maxX + 2/zoomf; x++) {
            if (y > base->height || x > base->width || x < 0 || y < 0)
                continue;
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
                write_point4(file, x, y, b00, b01, b10, b11,false, base->width, basecache);
            if (export_plug && should_emit_plug(p00, p01, p10, p11, offset)) {
                write_point4(file, x, y, p00 - offset, p01-offset, p10-offset,p11-offset, false, base->width, plugcache1);
                write_point4(file, x, y, plugtop(p00 - offset), plugtop(p01-offset), plugtop(p10-offset),plugtop(p11-offset), false, base->width, plugcache2);
                
            }
        }
    }
    flush_cache(basecache, file, base->width, base->height);
    flush_cache(plugcache1, file, base->width, base->height);
    flush_cache(plugcache2, file, base->width, base->height);
    fseek(file, 0, SEEK_SET);
    header.triangles = triangles;
    fwrite(&header, 1, sizeof(header), file);
    
    fclose(file);
    if (export_base)
        free(basecache);
    if (export_plug) {
        free(plugcache1);
        free(plugcache2);
    }
//    printf("Exported %i triangles \n", triangles);
}