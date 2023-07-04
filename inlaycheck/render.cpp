#include "inlay.h"


#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

extern "C" {
#include <png.h>
}

render::render(const char *filename)
{
    
    width_mm = 0;
    height_mm = 0;
    width = 0;
    height = 0;
    pixels_per_mm = 8;
    pixels = NULL;
    tool = NULL;    
    cX = 0; cY = 0; cZ = 50;
    deepest = 0;
    
    fname = strdup(filename);
    
    
}

void render::load(void)
{
    FILE *file;
    char *line;
    int lines = 0;
    file = fopen(fname, "rm");
    if (!file)
        return;
    while (!feof(file)) {
        size_t s = 0;
        line = NULL;
        if (getline(&line, &s, file) < 0)
            break;
        parse_line(line);
        free(line);       
        lines++;
    }
    fclose(file);
    printf("Read %i lines\n", lines);
}

int render::mm_to_x(double X)
{
    return X * ratio_x;
}

int render::mm_to_y(double Y)
{
    return Y * ratio_y;
}

double render::x_to_mm(int x) 
{
    return x / ratio_x;
}

double render::y_to_mm(int y)
{
    return y / ratio_y;
}

void render::update_pixel(int x, int y, double H)
{
    if (x < 0 || y < 0)
        return;
    if (x >= width || y >= height)
        return;
    if (pixels[y * width + x] > H)
        pixels[y * width + x] = H;
    if (H < deepest)
        deepest = H;
}

void render::update_pixel(double X, double Y, double H)
{
    update_pixel(mm_to_x(X), mm_to_y(Y), H);
}

void render::setup_canvas(void)
{
    width = 0.99999 + pixels_per_mm * width_mm;
    height = 0.99999 + pixels_per_mm * height_mm;
    
    ratio_x = width / width_mm;
    ratio_y = height / height_mm;
    
    pixels = (double *)calloc(sizeof(double), width * height);
}


void render::parse_line(const char *line)
{
//    (stockMax:127.00mm
    if (strncmp(line, "(stockMax ", 9) == 0) {
        const char *c;
        char *d;
        c =line +10;
        width_mm = strtod(c, &d);
        if (d)
            height_mm = strtod(d + 3, NULL);
        printf("STOCKMAX %5.2f, %5.2f\n", width_mm, height_mm);
        
        setup_canvas();
        return;
    }
// (TOOL/MILL,1.98, 0.00, 0.00, 0.00)

    if (strncmp(line, "(TOOL/MILL,", 11) == 0) {
        const char *c;
        char *d;
        double diameter, angle = 0;
        c = line + 11;
        diameter = strtod(c, &d);
        if (d)
            strtod(d + 1, &d);
        if (d)
            strtod(d + 1, &d);
        if (d)
            angle = 2 *strtod(d + 1, &d);
        printf("Tool : D = %5.2f,  Angle = %5.2f\n", diameter, angle);
        
        if (angle > 0)
            tool = new vbit(angle);
        else
            tool = new flat(diameter);
            
        return;
    }    
    if (line[0] == 'G' || line[0] == 'X' || line[0] == 'Y' || line[0] == 'Z') {
        parse_g_line(line);
        return;
    }
    
    if (line[0] == 'M' || line[0] =='T') {
        /*nothing to do with M or T or comment lines for now but reset Z*/
        cZ = 0;
        return;
    }

    if (line[0] == 'M' || line[0] =='T' || line[0] == '(') {
        /*nothing to do with M or T or comment lines for now */
        return;
    }
    
    printf("Unknown line %s\n", line);
    
}

void render::parse_g_line(const char *line)
{
    char *c;
    double nX, nY, nZ;
    
    nX = cX;
    nY = cY;
    nZ = cZ;
    
    c = (char *)line;
    while (strlen(c) > 0) {
        if (*c == 'G') {
            c++;
            strtoull(c, &c, 10);
            continue;
        }
        if (*c == 'X') {
            nX = strtod(c +1, &c);
            continue;
        }
        if (*c == 'Y') {
            nY = strtod(c +1, &c);
            continue;
        }
        if (*c == 'Z') {
            nZ = strtod(c +1, &c);
            continue;
        }
        if (*c == 'F') {
            strtod(c +1, &c);
            continue;
        }
        if (*c == ' ' || *c == '\n') {
            c++;
            continue;
        }
        printf("Line is %s  %s\n",c, line);
    }
    movement(cX, cY, cZ, nX, nY, nZ);
    cX = nX;
    cY = nY;
    cZ = nZ;
}

void render::movement(double X1, double Y1, double Z1, double X2, double Y2,double Z2)
{
    double len;
    
    double stepX, stepY, stepZ, steps;
    int i;
    len = sqrt( (X1-X2)*(X1-X2) + (Y1 - Y2)*(Y1 - Y2) + (Z1 - Z2)*(Z1 - Z2));
    
    if (Z1 > 0 && Z2 > 0)
        return;
    
    steps = len * pixels_per_mm;
    
    stepX = (X2-X1) / steps;
    stepY = (Y2-Y1) / steps;
    stepZ = (Z2-Z1) / steps;
    
    tooltouch(X1, Y1, Z1);
    
    for (i = 0; i < steps; i++) {
        X1 += stepX;
        Y1 += stepY;
        Z1 += stepZ;
        tooltouch(X1, Y1, Z1);
    }
    tooltouch(X2, Y2, Z2);
    
    cX = X2;
    cY = Y2;
    cZ = Z2;
}

void render::tooltouch(double X, double Y, double Z)
{
    int startx, starty;
    int maxX, maxY;
    int x,y;
    if (Z > 0)
        return;
        
    if (!tool)
        return;
    
    startx = mm_to_x(X - tool->scanzone);       
    maxX = mm_to_x(X + tool->scanzone) + 1;
    starty = mm_to_y(Y - tool->scanzone);       
    maxY = mm_to_y(Y + tool->scanzone) + 1;
    
    for (y = starty; y <= maxY; y++) {
        for (x = startx; x < maxX; x++) {
            double R;
            double dX, dY;
            dX = x_to_mm(x)-X;
            dY= y_to_mm(y)-Y;
            R = sqrt(dX*dX+dY*dY);
            update_pixel(x, y, tool->get_height(R, Z));
        }
    }
        
}

void render::save_as_pgm(const char *filename)
{
     FILE *file;
     int x,y;
     file = fopen(filename, "w");
     fprintf(file, "P2\n");
     fprintf(file, "%i %i\n", width, height);
     fprintf(file, "255 \n");
     
     for (y = height - 1 ; y >= 0; y--) {
         for (x = 0; x < width; x++) {
             double d;
             int c;
             d = pixels[y * width + x];
             d = 255 * (deepest -d)/deepest;
             c = round(d);
             if (c < 0) c =0;
             if (c > 255) c=255;
             fprintf(file, "%i ", c);
         }
         fprintf(file, "\n");
     }
     
     fclose(file);
}