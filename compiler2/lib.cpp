#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int globalsequence;

/* This function replaces a series of elements in the parents' children vector with a new element that has
 * these elements as its children. Effectively this moves the <start,end> range of children one hierarchy level
 * deeper 
 */
void move_children_to_element(struct element *parent, struct element *target, int startindex, int endindex)
{
    int i;
    
    /* Step 1: Add a copy of all elements in the range to the target */    
    for (i = startindex; i <= endindex; i++)
        target->children.push_back(parent->children[i]);
        
    /* Step 2: Replace the first element with the target */
    
    parent->children[startindex] = target;
    
    /* Step 3: remove the rest */    
    if (startindex + 1 <= endindex)
        parent->children.erase(parent->children.begin() + startindex + 1, parent->children.begin() + endindex + 1);
        
    if (target->children.size() > 0) {
        struct element *e = target->children[0];
        if (e->type != TYPE_MOVEMENT)
            target->is_barrier = true;
    }
}


double dist3(double X1, double Y1, double Z1, double X2, double Y2, double Z2)
{
    return sqrt((X2-X1)*(X2-X1) + (Y2-Y1)*(Y2-Y1) + (Z2-Z1)*(Z2-Z1));
}

void declare_A_depends_on_B(struct element *A, struct element *B)
{
    A->depends_on.push_back(B);
    B->dependents.push_back(A);
    A->depends_refcount++;    
}

bool elements_intersect(struct element *A, struct element *B)
{
    if (A->maxX < B->minX)
        return false;
    if (B->maxX < A->minX)
        return false;
    if (A->maxY < B->minY)
        return false;
    if (B->maxY < A->minY)
        return false;
        
    /* for now, don't look any deeper */
    return true;
}