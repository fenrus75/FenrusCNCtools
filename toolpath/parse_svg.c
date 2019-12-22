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

static void cubic_bezier(double x0, double y0,
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
            add_point_to_poly(nX, nY);
            lX = nX;
            lY = nY;
        }
        t = t + delta;
    }
    add_point_to_poly(x3,y3);
}                    

static void quadratic_bezier(double x0, double y0,
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
            add_point_to_poly(nX, nY);
            lX = nX;
            lY = nY;
        }
        t = t + delta;
    }
    add_point_to_poly(x3,y3);
}                    

static void push_chunk(char *chunk)
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
                add_point_to_poly((last_X + arg1)/2, (-arg2 - last_Y)/2);
            }
#endif
//            printf("Line         : %5.2f %5.2f\n", arg1, arg2);
            add_point_to_poly(arg1, -arg2);
            last_X = arg1;
            last_Y = arg2;
            break;
        case 'M':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            new_poly(arg1, -arg2);
            last_X = arg1;
            last_Y = arg2;
            break;
        case 'H':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            add_point_to_poly(arg1, -last_Y);
            last_X = arg1;
            break;
        case 'V':
//            printf("Start of poly: %5.2f %5.2f\n", arg1, arg2);
            add_point_to_poly(last_X, -arg1);
            last_Y = arg1;
            break;
        case 'C':
            cubic_bezier(last_X, -last_Y, arg1, -arg2, arg3, -arg4, arg5, -arg6);
            last_X = arg5;
            last_Y = arg6;
            break;
        case 'Q':
            quadratic_bezier(last_X, -last_Y, arg1, -arg2, arg3, -arg4);
            last_X = arg3;
            last_Y = arg4;
            break;
        case 'Z':
            end_poly();
            break;
        default:
            printf("Unknown command in chunk: %s\n", chunk);
    }
    
}

static const char *valid = "0123456789. ";

static void strip_str(char *line)
{
    while (strlen(line) > 0 && line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = 0;
    while (strlen(line) > 0 && line[strlen(line)-1] == ' ')
            line[strlen(line)-1] = 0;
}

static void parse_line(char *line)
{
    char *c;
    char chunk[4095];
    double height;
    
    c = strstr(line, "height=\"");
    if (c) {
        height = strtod(c+8, NULL);
        declare_minY(-height);
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
                push_chunk(chunk);
            memset(chunk, 0, sizeof(chunk));
            chunk[strlen(chunk)] = *c;           
        }
        c++;
    }
    strip_str(chunk);
    if (strlen(chunk) > 0)
        push_chunk(chunk);
    end_poly();
}

void parse_svg_file(const char *filename)
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
            parse_line(line);
            free(line);
        }
    }
    fclose(file);
}
