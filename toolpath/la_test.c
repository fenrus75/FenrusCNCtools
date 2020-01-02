

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern int lines_tangent_to_two_circles(double X1, double Y1, double R1, double X2, double Y2, double R2, int select, double *pX1, double *pY1, double *pX2, double *pY2);
extern double distance_point_from_vector(double X1, double Y1, double X2, double Y2, double pX, double pY);

int main(int argc, char **argv)
{
    FILE *file;
    double X1, Y1, X2, Y2;
    double R1, R2;
    int ret = 0;
    
    
    double x1,y1,x2,y2;
    double x3,y3,x4,y4;
    
    
    X1 = 90.0430; Y1 = 96.3930;
    X2 = 11.5570; Y2 = 96.3930;
    
    printf("DISTANCE = %5.2f\n",
        distance_point_from_vector(X1, Y1, X2, Y2, 90.0435, 11.5565));
    
    


    X1 = 143.15; Y1 = 105.89; R1 = 8.08893;
    
    X2 = 142.70; Y2 = 105.56; R2 = 12.17702;
    
    ret += lines_tangent_to_two_circles(X1, Y1, R1, X2, Y2, R2, 0, &x1, &y1, &x2, &y2);
    ret += lines_tangent_to_two_circles(X1, Y1, R1, X2, Y2, R2, 1, &x3, &y3, &x4, &y4);
        
    if (ret != 0) {
            printf("Invalid result\n");
    }
        
    file = fopen("output.svg", "w");
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    fprintf(file, "<svg width=\"20px\" height=\"20px\" xmlns=\"http://www.w3.org/2000/svg\">\n");
    fprintf(file, "<circle cx=\"%5.3f\" cy=\"%5.3f\" r=\"%5.3f\" fill=\"blue\" stroke-width=\"0.2\" />\n", X1, Y1, R1);
    fprintf(file, "<circle cx=\"%5.3f\" cy=\"%5.3f\" r=\"%5.3f\" fill=\"blue\" stroke-width=\"0.2\" />\n", X2, Y2, R2);
    

    fprintf(file, "<line x1=\"%5.3f\" y1=\"%5.3f\" x2=\"%5.3f\" y2=\"%5.3f\" stroke=\"black\" stroke-width=\"0.2\"/>\n", x1, y1, x2, y2);
    fprintf(file, "<line x1=\"%5.3f\" y1=\"%5.3f\" x2=\"%5.3f\" y2=\"%5.3f\" stroke=\"black\" stroke-width=\"0.2\"/>\n", x3, y3, x4, y4);

    fprintf(file, "</svg>\n");
    fclose(file);
        
    printf(" (%5.4f,%5.4f) -> (%5.4f, %5.4f)\n", x1, y1, x2, y2);
    printf(" (%5.4f,%5.4f) -> (%5.4f, %5.4f)\n", x3, y3, x4, y4);
    
}