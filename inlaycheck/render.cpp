#include "inlay.h"

#include <sys/param.h>


#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>


vbit *lastv = NULL;


render::render(const char *filename)
{
    
    width_mm = 0;
    height_mm = 0;
    width = 0;
    height = 0;
    pixels_per_mm = 32;
    pixels = NULL;
    tool = NULL;    
    bestpixels = NULL;
    cX = 0; cY = 0; cZ = 50;
    deepest = 0;
    offsetX = 0; offsetY = 0;
    validmap = NULL;
    valuemap = NULL;
    
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
    return x * invratio_x;
}

double render::y_to_mm(int y)
{
    return y * invratio_y;
}

void render::update_pixel(int x, int y, double H)
{
    if (H >= 0)
        return;
/*        
    if (x < 0 || y < 0)
        return;
    if (x >= width || y >= height)
        return;
*/
    if (pixels[y * width + x] > H)
        pixels[y * width + x] = H;
    if (H < deepest)
        deepest = H;
        
    minX = MIN(minX, x);
    minY = MIN(minY, y);
    maxX = MAX(maxX, x);
    maxY = MAX(maxY, y);
}

void render::update_pixel(double X, double Y, double H)
{
    update_pixel(mm_to_x(X), mm_to_y(Y), H);
}

void render::setup_canvas(void)
{
    width = 0.99999 + pixels_per_mm * width_mm;
    height = 0.99999 + pixels_per_mm * height_mm;
    
    minX = width;
    minY = height;
    maxX = 0;
    maxY = 0;
    
    ratio_x = width / width_mm;
    ratio_y = height / height_mm;
    invratio_x = 1/ratio_x;
    invratio_y = 1/ratio_y;
    
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
    if (strncmp(line, "(stockMin ", 9) == 0) {
        const char *c;
        char *d;
        c =line +10;
        strtod(c, &d); /* width */
        if (d)
            strtod(d + 3, &d); /* height */
        if (d) 
            depth_mm = strtod(d + 3, NULL); /* height */
        printf("STOCK Depth  %5.2f\n", depth_mm);
        
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
        
        if (angle > 0) {
            lastv = new vbit(angle);
            tool = lastv;
        } else {
            tool = new flat(diameter);
        }
            
        return;
    }    
    if (line[0] == 'G' || line[0] == 'X' || line[0] == 'Y' || line[0] == 'Z') {
        parse_g_line(line);
        return;
    }
    
    if (line[0] == 'M' || line[0] =='T') {
        const char *c;
        /* nothing much to do with M or T or comment lines for now but reset Z*/
        
        /* but we do want to find the tool number */
        c = strchr(line, 'T');
        if (c)
            tool->number = strtoll(c+1, NULL, 10);
        
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
    
    if (strstr(line, "G53"))
        return;
    
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
    
    steps = len * pixels_per_mm * tool->samplesteps + 1;
    
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
    
    if (maxY >= height)
        maxY = height - 1;
    if (maxX >= width)
        maxX = width - 1;
    if (starty < 0)
        starty = 0;
    if (startx < 0)
        startx = 0;
    
    for (y = starty; y <= maxY; y++) {
        int offset = y * width;
        for (x = startx; x < maxX; x++) {
            double R;
            double dX, dY;
            
            if (pixels[x + offset] <= Z)
                continue;
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
     
     if (deepest < depth_mm)
         deepest = depth_mm;
     
     for (y = height - 1 ; y >= 0; y--) {
         for (x = 0; x < width; x++) {
             double d;
             int c;
             d = pixels[y * width + x] + offsetZ;
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


void render::cut_out(void)
{
    int x,y;
    double threshold;
    
    double cutout = 100;

     if (deepest < depth_mm)
         deepest = depth_mm;
    
    threshold = deepest + 0.0001; /* deal with rounding artifacts*/
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (pixels[y * width + x] > threshold)
                pixels[y * width + x] = cutout;
            else
                break;
        }
        for (x = width - 1; x >= 0; x--) {
            if (pixels[y * width + x] > threshold)
                pixels[y * width + x] = cutout;
            else
                break;
        }
    }
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            if (pixels[y * width + x] > threshold)
                pixels[y * width + x] = cutout;
            else
                break;
        }
        for (y = height - 1; y >= 0; y--) {
            if (pixels[y * width + x] > threshold)
                pixels[y * width + x] = cutout;
            else
                break;
        }
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (pixels[y * width + x] >= cutout)
                pixels[y * width + x] = deepest;
        }
    }
}


void render::crop(void)
{
    int new_width, new_height;
    double *newpixels;
    int x,y;
    
    printf("Cropping (%i %i) x (%i %i)\n", minX, minY, maxX, maxY);
    
    croppedX = minX;
    croppedY = minY;
    
    new_width = maxX - minX + 1;
    new_height = maxY - minY + 1;
    
    newpixels = (double *)calloc(sizeof(double), new_width * new_height);
    
    for (y = 0; y < new_height; y++) {
        if (y + minY >= height)
            continue;
            
        for (x = 0; x < new_width; x++) {
            if (x + minX >= width)
                continue;
            newpixels[y * new_width + x] = pixels[ (y + minY) * width + (x + minX)];  
        }
    }
 
     width = new_width;
     height = new_height;
     free(pixels);
     pixels = newpixels;
     minX = 0;
     minY = 0;
     maxX = width;
     maxY = height;   
}

void render::set_offsets(int x, int y)
{
    offsetX = x;
    offsetY = y;
}

double render::get_height(int x, int y) 
{
    x -= offsetX;
    y -= offsetY;
    
    if (x < 0 || y < 0)
        return -offsetZ;
    if (x >= width)
        return -offsetZ;
    if (y >= height)
        return -offsetZ;
    return pixels[y * width + x];
}
double render::get_best_height(int x, int y) 
{
    x -= offsetX;
    y -= offsetY;
    
    if (x < 0 || y < 0)
        return -offsetZ;
    if (x >= width)
        return -offsetZ;
    if (y >= height)
        return -offsetZ;
    if (!bestpixels)
        return -offsetZ;
    return bestpixels[y * width + x];
}

void render::flip_over(void)
{
    double *newpixels;
    int x,y;
    
    newpixels = (double *)calloc(sizeof(double), width * height);
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            newpixels[y * width + (width - x - 1)] = - pixels[y * width + x];
        }
    }
    free(pixels);
    pixels = newpixels;
    
    
    if (bestpixels) {
        newpixels = (double *)calloc(sizeof(double), width * height);
    
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                newpixels[y * width + (width - x - 1)] = - bestpixels[y * width + x];
            }
        }
        free(bestpixels);
        bestpixels = newpixels;
    }
    if (offsetZ == 0) 
        offsetZ = depth_mm;
    else
        offsetZ = 0;
}

void render::set_best_height(int x, int y, double H) 
{
    x -= offsetX;
    y -= offsetY;
    
    if (x < 0 || y < 0)
        return;
    if (x >= width)
        return;
    if (y >= height)
        return;
        
    if (!bestpixels)
        bestpixels = (double *) calloc(width * height, sizeof(double));
        
    if (H < 0)
        H = 0;
    bestpixels[y * width + x] = H;
}

void render::swap_best(void)
{
    double *p;
    if (!bestpixels)
        return;
    p = pixels;
    pixels = bestpixels;
    bestpixels = p;
}

bool render::tooltouch_valid(int cx, int cy, double Z)
{
    int startx, starty;
    int maxX, maxY;
    int x,y;
    double X, Y;
    bool retval = true;
    double gains = 0.0;
    if (Z > 0)
        return true;
        
    if (!tool)
        return false;
        
    X = x_to_mm(cx);
    Y = y_to_mm(cy);
    
    startx = cx - mm_to_x(tool->scanzone);       
    maxX = cx + mm_to_x(tool->scanzone) + 1;
    starty = cy - mm_to_y(tool->scanzone);       
    maxY = cy +  mm_to_y(tool->scanzone) ;


#if 0
    if (bestpixels[cx+cy * width >= 0])  {
        retval = false;
        validmap[cx + cy * width] = false;
        return false;
    }
#endif
//    startx = cx ; maxX = cx+1;starty=cy;maxY=cy;
    
    if (maxY >= height)
        maxY = height - 1;
    if (maxX >= width)
        maxX = width - 1;
    if (starty < 0)
        starty = 0;
    if (startx < 0)
        startx = 0;
    
    for (y = starty; y <= maxY; y++) {
        int offset = y * width;
        for (x = startx; x < maxX; x++) {
            double R;
            double dX, dY;
            
            if (pixels[x + offset] <= Z)
                continue;
                
            dX = x_to_mm(x)-X;
            dY= y_to_mm(y)-Y;
            R = sqrt(dX*dX+dY*dY);
            
            double H = lastv->get_height_static(R, Z);
            

            if (H < bestpixels[x + y * width]) {
                retval = false;
                validmap[cx + cy * width] = false;
                return false;
            }
            if (retval && H < pixels[x + y * width])  {
                gains  += pixels[x + y * width] - H;
//                printf("x,y %i,%i  H is %5.2f  pixels is %5.2f  gains is %5.2f\n",x,y,H, pixels[x + y * width],gains);
//               gains += pixels[x + y * width] - bestpixels[x + y * width];
            }
                
        }
    }
    
    if (retval)
        valuemap[cx + cy * width] = gains;
    else 
        validmap[cx + cy * width] = false;
        
    return retval;
}


void render::make_validmap(double Depth)
{
    int x,y;
    if (!validmap)
        validmap = (bool *)calloc(sizeof(bool), width * height);
    if (!valuemap)
        valuemap = (double *)calloc(sizeof(double), width * height);
    
    if (!lastv)
        return;
        
    printf("Making validmap\n");
    
    tool = lastv;
    
    /* first we set all places to valid, and all value to 0 */
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            validmap[x + y * width] = true;
            valuemap[x + y * width] = 0.0;
        }
    }
    
    /* find all the valid pixels where the V bit can work without damaging the design -- and where there is value */
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
           tooltouch_valid(x, y, Depth);
        }
    }
    
    /* now we need to filter the valuemap -- we only want to do values that are adjacent to invalid pixels */
    for (y = 1; y < height -1; y++) {
        for (x = 1; x < width - 1; x++) {
            if (valuemap[x + y * width] == 0)
               continue;
            if (!validmap[x - 1 + (y-1) * width])
               continue;               
            if (!validmap[x + 0 + (y-1) * width])
               continue;
            if (!validmap[x + 1 + (y-1) * width])
               continue;
            if (!validmap[x - 1 + y * width])
               continue;
            if (!validmap[x + 0 + y * width])
               continue;
            if (!validmap[x + 1 + y * width])
               continue; 
            if (!validmap[x - 1 + (y+1) * width])
               continue;
            if (!validmap[x + 0 + (y+1) * width])
               continue;
            if (!validmap[x + 1 + (y+1) * width])
               continue;
               

            /* if we get there no neighbor is invalid */
            valuemap[x + y * width] = 0;               
            
        }
    }
    
    
}

void render::export_validmap(const char *filename)
{
     FILE *file;
     int x,y;
     printf("Saving valuemap to %s\n", filename);
     file = fopen(filename, "w");
     
     double maxvalue = 0.001;
     fprintf(file, "P2\n");
     fprintf(file, "%i %i\n", width, height);
     fprintf(file, "255 \n");
     
     if (deepest < depth_mm)
         deepest = depth_mm;
     
     for (y = height - 1 ; y >= 0; y--) {
         for (x = 0; x < width; x++) {
                 if (fabs(valuemap[x + y * width]) > maxvalue)
                    maxvalue = fabs(valuemap[x + y * width]);
         }
     }


     for (y = height - 1 ; y >= 0; y--) {
         for (x = 0; x < width; x++) {
             int c = 0;
             if (validmap[x + y * width]) {
                 c = 255;
                 if (valuemap[x + y * width] != 0)
                    c= 64 + 64 * fabs(valuemap[x + y* width]/maxvalue);
             }
                
             fprintf(file, "%i ", c);
         }
         fprintf(file, "\n");
     }
     
     fclose(file);

}