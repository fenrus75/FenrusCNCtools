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
#include <errno.h>
#include <string.h>
#include <math.h>

#include "scene.h"
#include "toolpath.h"

/* SVG path supports relative-to-last coordinates... even across elements */
static double last_X, last_Y;




static void chr_replace(char *line, char a, char r)
{
    char *c;
    do {
        c = strchr(line, a);
        if (c)
            *c = r;
    } while (c);
}

static inline double distance(double x0, double y0, double x1, double y1)
{
    return sqrt( (x1-x0) * (x1-x0) + (y1-y0) * (y1-y0));
}

static void cubic_bezier(class scene *scene,
                         double x0, double y0,
                         double x1, double y1,
                         double x2, double y2,
                         double x3, double y3)
{
    double delta = 0.01;
    double t;
    double lX = x0, lY = y0;
    
    t = 0;
    while (t < 1.0) {
        double nX, nY;
        nX = (1-t)*(1-t)*(1-t)*x0 + 3*(1-t)*(1-t)*t*x1 + 3 * (1-t)*t*t*x2 + t*t*t*x3;
        nY = (1-t)*(1-t)*(1-t)*y0 + 3*(1-t)*(1-t)*t*y1 + 3 * (1-t)*t*t*y2 + t*t*t*y3;
        if (distance(lX,lY, nX,nY) > 1) {
            scene->add_point_to_poly(px_to_mm(nX), px_to_mm(nY));
            lX = nX;
            lY = nY;
        }
        t = t + delta;
    }
    scene->add_point_to_poly(px_to_mm(x3),px_to_mm(y3));
}                    

static void quadratic_bezier(class scene *scene,
                         double x0, double y0,
                         double x1, double y1,
                         double x3, double y3)
{
    double delta = 0.01;
    double t;
    double lX = x0, lY = y0;
    
    t = 0;
    while (t < 1.0) {
        double nX, nY;
        nX = (1-t)*(1-t)*x0 + 2*(1-t)*t*x1 + t*t*x3;
        nY = (1-t)*(1-t)*y0 + 2*(1-t)*t*y1 + t*t*y3;
        if (distance(lX,lY, nX,nY) > 1) {
            scene->add_point_to_poly(px_to_mm(nX), px_to_mm(nY));
            lX = nX;
            lY = nY;
        }
        t = t + delta;
    }
    scene->add_point_to_poly(px_to_mm(x3), px_to_mm(y3));
}                    


/*
<circle cx="440.422" cy="312.878" r="12" stroke="black" stroke-width="1" fill="none" />
*/
static void parse_circle(class scene *scene, char *line)
{
    char *cx, *cy, *r;
    double X,Y,R,phi;
    cx = strstr(line, "cx=\"");
    cy = strstr(line, "cy=\"");
    r = strstr(line, "r=\"");
    if (!cx || !cy || !r) {
        printf("Failed to parse circle: %s\n", line);
        return;
    }
    scene->end_poly();
    scene->set_poly_name("circle");
    X = strtod(cx + 4, NULL);
    Y = strtod(cy + 4, NULL);
    R = strtod(r + 3, NULL);
    phi = 0;
    while (phi < 360) {
        double P;
        P = phi / 360.0 * 2 * M_PI;
        scene->add_point_to_poly(px_to_mm(X + R * cos(P)), px_to_mm(-Y + R * sin(P)));
        phi = phi + 1;
    }
    last_X = X;
    last_Y = Y;
    scene->end_poly();
}

static void push_chunk(class scene *scene, char *chunk, char *line)
{
    char command;
    char *c;
#if 0
    double distance;
#endif
    double arg1 = 0.0, arg2 = 0.0, arg3 = 0.0, arg4 = 0.0, arg5 = 0.0, arg6 = 0.0;
    command = chunk[0];
    c = &chunk[0];
    c++;
    arg1 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg2 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg3 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg4 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg5 = strtod(c, &c);
    if (c && strlen(c) > 0)
        arg6 = strtod(c, &c);
    switch (command) {
        case 'L':
#if 0
            distance = sqrt((arg1 - last_X) * (arg1 - last_X) + (-arg2+last_Y) * (-arg2+last_Y));
            
            /* for long lines, split in 2 */
            if (distance > 4)  {
                printf("Distance %5.2f\n", distance);
                scene->add_point_to_poly((last_X + arg1)/2, (-arg2 - last_Y)/2);
            }
#endif
//            printf("Line         : %5.2f %5.2f\n", arg1, arg2);
            scene->add_point_to_poly(px_to_mm(arg1), px_to_mm(-arg2));
            last_X = arg1;
            last_Y = arg2;
            break;
        case 'M':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            scene->new_poly(px_to_mm(arg1), px_to_mm(-arg2));
            last_X = arg1;
            last_Y = arg2;
            break;
        case 'm':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            arg1 += last_X;
            arg2 += last_Y;
            scene->new_poly(px_to_mm(arg1), px_to_mm(-arg2));
            last_X = arg1;
            last_Y = arg2;
            break;
        case 'H':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            scene->add_point_to_poly(px_to_mm(arg1), px_to_mm(-last_Y));
            last_X = arg1;
            break;
        case 'V':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            scene->add_point_to_poly(px_to_mm(last_X), px_to_mm(-arg1));
            last_Y = arg1;
            break;
        case 'C':
            cubic_bezier(scene, last_X, -last_Y, arg1, -arg2, arg3, -arg4, arg5, -arg6);
            last_X = arg5;
            last_Y = arg6;
            break;
        case 'Q':
            quadratic_bezier(scene, last_X, -last_Y, arg1, -arg2, arg3, -arg4);
            last_X = arg3;
            last_Y = arg4;
            break;
        case 'Z':
        case 'z':
            scene->end_poly();
            break;
        default:
            printf("Unknown command in chunk: %s  (%s)\n", chunk, line);
    }
    
}

static const char *valid = "0123456789.eE- ";

static void strip_str(char *line)
{
    while (strlen(line) > 0 && line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = 0;
    while (strlen(line) > 0 && line[strlen(line)-1] == ' ')
            line[strlen(line)-1] = 0;
}

static void parse_line(class scene *scene, char *line)
{
    char *c;
    char chunk[4095];
    double height;
    
    c = strstr(line, "height=\"");
    if (c) {
        height = strtod(c+8, NULL);
        scene->declare_minY(px_to_mm(-height));
    }
    
    c = strstr(line,"<circle cx=");
    if (c) {
        parse_circle(scene, line);
    }
    c = strstr(line, " d=\"");
    if (c == NULL)
        return;
    c += 4;
    chr_replace(c, '"', 0);
    chr_replace(line, ',', ' ');
//    printf("Line is %s\n", c);
    chunk[0] = 0;
    memset(chunk, 0, sizeof(chunk));
    while (*c) {
        if (strchr(valid, *c)) {
            chunk[strlen(chunk)] = *c;            
        } else {
            strip_str(chunk);
            if (strlen(chunk) > 0)
                push_chunk(scene, chunk, line);
            memset(chunk, 0, sizeof(chunk));
            chunk[strlen(chunk)] = *c;           
        }
        c++;
    }
    strip_str(chunk);
    if (strlen(chunk) > 0)
        push_chunk(scene, chunk, line);
    scene->end_poly();
}

void parse_svg_file(class scene *scene, const char *filename)
{
    size_t n;
    FILE *file;
    
    file = fopen(filename, "r");
    if (!file) {
        printf("Cannot open %s : %s\n", filename, strerror(errno));
        return;
    }
    
    while (!feof(file)) {
        int ret;
        char *line = NULL;
        ret = getline(&line, &n, file);
        if (ret >= 0) {
            parse_line(scene, line);
            free(line);
        }
    }
    fclose(file);
}
