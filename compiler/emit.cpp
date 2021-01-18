#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>


void emit_TYPE_RAW(FILE *file, struct element *e)
{
    fprintf(file, "%s\n", e->raw_gcode);
}

void emit_TYPE_CONTAINER(FILE *file, struct element *e)
{
    if (e->description)
        fprintf(file,"(CONTAINER: %s)\n", e->description);
}

static bool approx3(double A, double B)
{
    if (fabs(A-B) >= 0.001)
        return false;
    return true;
}


static double lX, lY, lZ, lF;
static char lG;
static bool lValid = false;
void emit_TYPE_MOVEMENT(FILE *file, struct element *e)
{
    char line[4096];
    char frag[1045];
    memset(line, 0, 4096);
    
    if (lG != e->glevel || !lValid) {
        sprintf(frag, "G%c", e->glevel);
        strcat(line, frag);
    }
    
    if (!lValid || !approx3(lX, e->X2)) {
        sprintf(frag, "X%0.3f", e->X2);
        strcat(line, frag);
    }
    if (!lValid || !approx3(lY, e->Y2)) {
        sprintf(frag, "Y%0.3f", e->Y2);
        strcat(line, frag);
    }
    if (!lValid || !approx3(lZ, e->Z2)) {
        sprintf(frag, "Z%0.3f", e->Z2);
        strcat(line, frag);
    }
    if ((!lValid || !approx3(lF, e->feed)) && e->glevel != '0') {
        sprintf(frag, "F%0.0f", e->feed);
        strcat(line, frag);
    }
    fprintf(file, "%s\n", line);
    lX = e->X2;
    lY = e->Y2;
    lZ = e->Z2;
    if (e->glevel != '0')
        lF = e->feed;
    lValid = true;
    lG = e->glevel;
}

void emit_gcode(FILE *file, struct element *e)
{

        
    switch (e->type) {
    case TYPE_RAW:
        emit_TYPE_RAW(file, e);
        lValid = false;
        break;

    case TYPE_CONTAINER:
        emit_TYPE_CONTAINER(file, e);
        break;
    case TYPE_MOVEMENT:
        emit_TYPE_MOVEMENT(file, e);
        break;
    
    default:
        ;
    }
        
    for (auto i: e->children) {
        emit_gcode(file, i);
    }
    if (e->children.size() > 0)
     fprintf(file, "(END GROUP %s)\n", e->description);
}


const char *type2desc[] =
 { "RAW", "Movement", "Container", };
 
 
#define print_flag(f, p) do {  char c[4]; sprintf(c, "."); if (e->is_##f) sprintf(c, "%s", p); printf("%s", c); } while (0)

static void print_flags(struct element *e)
{
  print_flag(vertical, "V");
  print_flag(plunge, "P");
  print_flag(retract, "R");
  print_flag(toolgroup, "T");
  print_flag(g1only, "1");
  print_flag(ring, "r");
  print_flag(from_plunge, "f");
  
  printf("\t");
}

static void __print_tree(struct element *e, int level, int leaf)
{
    int i;
    int is_leaf;
    
    if (e->children.size() == 0 && leaf)
     return;
    
    for (i = 0; i < level; i++)
        printf("\t");
        
    printf("%4i ", e->sequence);
    print_flags(e);
        
    printf("%s\t", type2desc[e->type]);
    if (e->description) {
        printf("%s\t", e->description);
    } 
    
    if (e->type == TYPE_MOVEMENT || e->type == TYPE_RAW)
        printf("%s\t", e->raw_gcode);

    printf("%i", (int) e->children.size());
    printf("\t(%6.2f)\n", e->length);
    is_leaf = 1;
    for (auto i: e->children) {
        if (i->children.size() > 0)
         is_leaf = 0;
    }
    for (auto i: e->children) {
        __print_tree(i, level + 1, is_leaf);
    }
}

void print_tree(struct element *e, int level)
{
    __print_tree( e, level, 0);
}
