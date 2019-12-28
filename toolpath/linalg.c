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
#include <string.h>
#include <errno.h>
#include <math.h>

#include "toolpath.h"


/*
 * Distance from a point to a vector
 * 
 * See https://www.youtube.com/watch?v=n4EJnyQgbK8 for the math behind this
 */

static double dist(double X0, double Y0, double X1, double Y1)
{
  return sqrt((X1-X0)*(X1-X0) + (Y1-Y0)*(Y1-Y0));
}
 
 
double distance_point_from_vector(double X1, double Y1, double X2, double Y2, double pX, double pY)
{
    double x2, y2; /* vector relative to X1/Y1 */
    double l; /* V1 = (X1, Y1) + l (x2, y2)   and V2 = (pX,pY) + k (-y2, x2) */
    
    double iX, iY;
    
    double t1, t2;
    x2 = X2-X1;
    y2 = Y2-Y1;
    
    /* find the intersection point, e.g. l and k such that V1 and V2 intersect */
    
    /* 
     * this means
     * X1 + l * x2 = pX + k * -y2
     * Y1 + l * y2 = pY + k * x2
     *
     * so k = (Y1 + l * y2 - pY) / x2
     * but also
     *    k = (X1 + l * x2 - pX) / -y2
     *
     * X1 + l * x2 = pX - y2 * (Y1 + l * y2 - pY) / x2
     * l * x2 + y2 (Y1 + l * y2 -pY) / x2 = pX-X1
     * l * x2 + y2/x2 * Y1 + y2/x2 *l *y2 - y2 * pY/x2 = pX - X1
     * l * (x2 + y2 * y2 / x2) = pX - X1 - y2/x2 * Y1 + y2 / x2 * pY
     
     * (Y1 + l * y2 -pY) / x2 + (X1 + l * x2 -pX) / y2 = 0
     * Y1/x2 + l * y2 - pY/x2 + X1/y2 + l * x2/y2 - pX/y2 = 0
     * l * (y2/x2 + x2/y2) = - Y1/x2 + pY/x2 -X1/y2 + pX/y2
     
     */
     
     t1 = y2 * y2 + x2 * x2;
     t2 = - Y1*y2 + y2 * pY*x2*x2 - x2 * X1 + x2 * pX;
     l = t2/t1;
//     printf("x2 %5.2f  y2 %5.2f\n", x2, y2);
//     printf("l is %5.2f  t1 %5.2f  t2 %5.2f\n", l, t1, t2);
     
     iX = X1 + l * x2;
     iY = Y1 + l * y2;
     
     if (l >= 0 && l <= 1)
       return dist(pX,pY, iX, iY);
     
     /* if l < 0 or l > 1, the intersect point is actually outside our vector and one of the two edges
        is the shortest distance instead */
     return fmin(dist(pX,pY,X1,X2), dist(pX,pY,X2,Y2));
}
