#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include <algorithm>


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
    if (e->raw_gcode)
      fprintf(file, "%s\n", e->raw_gcode);
    else
      fprintf(file, "%s\n", line);
    
    lX = e->X2;
    lY = e->Y2;
    lZ = e->Z2;
    if (e->glevel != '0')
        lF = e->feed;
    lValid = true;
    lG = e->glevel;
}

static double currentX, currentY, currentZ;


/* Sorting rules
 * All emitted items come last
 * Then sort by least depends-on
 * THen sort by travel distance
 * Then sort by sequence number
 */
static bool compare_elements_for_sort(struct element *A, struct element *B)
{
  if (A->has_been_emitted && ! B->has_been_emitted)
    return false;
  if (B->has_been_emitted && ! A->has_been_emitted)
    return true;
    
  if (A->depends_refcount < B->depends_refcount)
    return true;
  if (A->depends_refcount > B->depends_refcount)
    return false;
    
  double dA = dist3(currentX, currentY, currentZ, A->X1, A->Y1, A->Z1);
  double dB = dist3(currentX, currentY, currentZ, B->X1, B->Y1, B->Z1);
    
  if (dA < dB)
    return true;
  if (dA > dB)
    return false;

  if (A->sequence < B->sequence)
    return true;

  return false;
    
}

static void sort_children(struct element *e)
{
  std::sort(e->children.begin(), e->children.end(), compare_elements_for_sort);
}

void __emit_gcode(FILE *file, struct element *e, int level)
{

        
    switch (e->type) {
    case TYPE_RAW:
        emit_TYPE_RAW(file, e);
        lValid = false;
        break;

    case TYPE_CONTAINER:
        currentX = e->X1;
        currentY = e->Y1;
        currentZ = e->Z1;
        emit_TYPE_CONTAINER(file, e);
        break;
    case TYPE_MOVEMENT:
        currentX = e->X2;
        currentY = e->Y2;
        currentZ = e->Z2;
        emit_TYPE_MOVEMENT(file, e);
        break;
    
    default:
        ;
    }
    e->has_been_emitted = true;
    
    for (auto q: e->dependents) {    
      q->depends_refcount--;
    }
    
    if (e->type == TYPE_CONTAINER || e->type == TYPE_RAW)
      printf("Emiting container %i (%s)\n", e->sequence, e->description);
      
    if (level == 0) {

      sort_children(e);
    
      if (e->children.size() > 0) {
        struct element *i;
        i = e->children[0];
      
        while (!i->has_been_emitted) {
#if 0        
          printf("emitting sequence %i at distance %5.2f\n", i->sequence, dist3(currentX, currentY, currentZ, i->X1, i->Y1, i->Z1));
          for (unsigned int pp = 1; pp < e->children.size() && pp < 3; pp++) {
              struct element *qq = e->children[pp];
              if (!qq->has_been_emitted && qq->depends_refcount < 5)
                printf("    versus emitting sequence %i at distance %5.2f  RC %li\n", qq->sequence, dist3(currentX, currentY, currentZ, qq->X1, qq->Y1, qq->Z1), qq->depends_refcount);
          }
#endif        
          __emit_gcode(file, i, level + 1);
          sort_children(e);
          i = e->children[0];
        }
      }
    } else {
      for (auto i: e->children)
        __emit_gcode(file, i, level + 1);
    }
      
    if (e->description)
      fprintf(file, "(END GROUP %s)\n", e->description);
}

void emit_gcode(FILE *file, struct element *e)
{
  __emit_gcode(file, e, 0);
}


const char *type2desc[] =
 { "RAW", "Movement", "Container", };
 
 
#define print_flag(f, p) do {  char c[4]; sprintf(c, "."); if (e->is_##f) sprintf(c, "%s", p); printf("%s", c); } while (0)

static void print_flags(struct element *e)
{
  print_flag(barrier, "B");
  print_flag(vertical, "V");  
  print_flag(retract, "R");  
  print_flag(positioning, "P");
  printf("\t");
}

static void __print_tree(struct element *e, int level, int leaf)
{
    int i;
    int is_leaf;
    
//    if (e->children.size() == 0 && leaf)
//     return;
    
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

    printf("%i\t", (int) e->children.size());
    if (e->type == TYPE_CONTAINER || e->type == TYPE_RAW) 
      printf("RC:%li\t", e->depends_refcount);
    if (e->depends_refcount == 1)
      printf("DEPS:%i\t", e->depends_on[0]->sequence);
    if (e->type == TYPE_CONTAINER) {
      printf("BB:%1.2fx%1.2f-%1.2fx%1.2f\t", e->minX, e->minY, e->maxX, e->maxY);
    }
    printf("(%6.2f)\n", e->length);
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
