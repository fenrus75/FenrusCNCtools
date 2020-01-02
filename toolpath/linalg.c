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

//static FILE *dump = NULL;

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
//     printf("t1 %6.4f  t2 %6.4f   l %6.4f\n", t1, t2, l);
//     printf("x2 %5.2f  y2 %5.2f\n", x2, y2);
//     printf("l is %5.2f  t1 %5.2f  t2 %5.2f\n", l, t1, t2);
     
     iX = X1 + l * x2;
     iY = Y1 + l * y2;
     
     if (l >= 0 && l <= 1)
       return dist(pX,pY, iX, iY);
       
     
     
     /* if l < 0 or l > 1, the intersect point is actually outside our vector and one of the two edges
        is the shortest distance instead */
//     printf("Point does not hit vector  %5.2f\n", fmin(dist(pX,pY,X1,X2), dist(pX,pY,X2,Y2)));
     return fmin(dist(pX,pY,X1,Y1), dist(pX,pY,X2,Y2));
}


/* Creating 2 vectors that are outer tangents to two circles, as needed for limited depth v-carving */
/* Math based on https://en.wikipedia.org/wiki/Tangent_lines_to_circles */

/* Circle 1 is allowed to have a radius of 0 (point) */

/* If R1 > R2 then we flip the inputs so that always R1 <= R2*/

/* there are two answers, select = 0 and select = 1 return them */

int lines_tangent_to_two_circles(double X1, double Y1, double R1, double X2, double Y2, double R2, int select, double *pX1, double *pY1, double *pX2, double *pY2)
{
     double dr;
     
     double length;
     double Dcenter;
     double phi;
     
     double l;
     
     double ux, uy; /* unit vector from x1/y1 to x2/y2 */
     double ox, oy; /* vector orthogonal to this unit vector */
     
     double vx, vy, ovx, ovy;
     
     if (R2 < R1) {
//        printf("FLIP\n");
        return lines_tangent_to_two_circles(X2,Y2,R2,X1,Y1,R1, select, pX1, pY1, pX2, pY2);
     }
     *pX1 = 0;
     *pX2 = 0;
     *pY1 = 0;
     *pY2 = 0; 
     if (select <= 0)
       select = -1;
     else
       select = 1;
 
//    if (!dump) {    
//     dump = fopen("linalg.svg", "w");
//     fprintf(dump, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
//     fprintf(dump, "<svg width=\"200px\" height=\"200px\" xmlns=\"http://www.w3.org/2000/svg\">\n");
//   }
   
   
//   fprintf(dump, "<circle cx=\"%5.3f\" cy=\"%5.3f\" r=\"%5.3f\" fill=\"blue\" stroke-width=\"0.2\" />\n", X1, Y1, R1);
//   fprintf(dump, "<circle cx=\"%5.3f\" cy=\"%5.3f\" r=\"%5.3f\" fill=\"red\" stroke-width=\"0.2\" />\n", X2, Y2, R2);
    


     Dcenter = dist(X1, Y1, X2, Y2);
     
     
     /* calculate unit vector */
     ux = (X2-X1)/Dcenter;
     uy = (Y2-Y1)/Dcenter;
     
     /* vector of length 1 orthogonal to this unit vector */
     ox = -uy;
     oy = ux;
     
//     printf("(ox, oy) is (%5.2f, %5.2f)\n", ox, oy);
//     printf("(ux, uy) is (%5.2f, %5.2f)\n", ux, uy);
     
     
     /* most of the math below assumes the first circle is a point, we just offset the line by "dr" later to correct */
     dr = R2 - R1;
     
     /* length is the distance from the center of C1 to the point where the vector is tangent to C2 */
     /* dr^2 + length^2 = Dcenter^2 => length = sqrt(Dcenter^2 - dr^2) */
     
     double delta;
     
     delta = Dcenter * Dcenter - dr*dr;
     
     if (delta > -0.001 && delta <=0)
      delta = 0;
      
     
     if (delta <= 0) {
      if (R1 == 0) {
         *pX1 = X1;
         *pX2 = X1;
         *pY1 = Y1;
         *pY2 = Y1;
         return 0;
      }
      //printf("DC %5.6f   dr %5.6f   R1 %5.2f  R2 %5.2f\n", Dcenter, dr, R1, R2);
      //printf("NAN  (%5.3f,%5.3f)@%5.3f -> (%5.3f,%5.3f)@%5.3f\n",X1,Y1,R1,X2,Y2,R2);
      //fflush(dump);
      return -1;
     }
     length = sqrt(delta);
     
//     printf("length is %5.2f\n", length);
     
     /* phi is the angle at C1 */
     phi = asin(dr/Dcenter);
 //    printf("phi is %5.2f which is %5.2f degrees\n", phi, phi /2  / 3.1415 * 360.0);
     
     /* so now we know the angle and "length" so we can compute the distance along u and how high on o the new point is */
     
//     printf("cos phi is %5.2f   sin phi is %5.2f\n", cos(phi), sin(phi));
     
     vx = ux * cos(phi) * length + select * sin(phi) * length * ox;
     vy = uy * cos(phi) * length + select * sin(phi) * length * oy;
     
//     printf("(vx, vy) is (%5.2f, %5.2f)\n", vx, vy);
 
          
     ovx = -vy;
     ovy = vx;
     
     l = sqrt(ovx*ovx + ovy*ovy);
     ovx /= l;
     ovy /= l;
 
     *pX1 = X1 + select * R1 * ovx;
     *pY1 = Y1 + select * R1 * ovy;
     
     *pX2 = X1 + vx + select * R1 * ovx;
     *pY2 = Y1 + vy + select * R1 * ovy;

//    fprintf(dump, "<line x1=\"%5.3f\" y1=\"%5.3f\" x2=\"%5.3f\" y2=\"%5.3f\" stroke=\"black\" stroke-width=\"0.2\"/>\n", *pX1, *pY1, *pX2, *pY2);
//    fflush(dump);
     
  
//     printf("----\n");    
     return 0;
}