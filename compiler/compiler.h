#pragma once

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <vector>

#define TYPE_RAW	0
#define TYPE_MOVEMENT	1
#define TYPE_CONTAINER  2

struct element;

struct element {
    int type;
    int sequence;
    
    /* bounding box */
    double minX, minY, minZ, maxX, maxY, maxZ;

    double X1, Y1, Z1, X2, Y2, Z2;
    double feed;
    
    double length;
    

    bool is_retract;    
    bool is_toolgroup;
    bool is_g1only;
    bool is_ring;
    bool is_vertical;

    char glevel;    
    char *raw_gcode;
    const char *description;
    
    std::vector<struct element *>children;
};



extern int globalsequence;

static inline struct element * new_element(int type, const char *description)
{
    struct element *e;
    
    e = (struct element*)calloc(1, sizeof(struct element));
    e->type = type;
    
    if (description)
    
    e->description = strdup(description);
    e->sequence = globalsequence++;
    
    return e;
}

extern struct element *parse_file(const char *filename);


extern void move_children_to_element(struct element *parent, struct element *target, int startindex, int endindex);


extern void emit_gcode(FILE *file, struct element *e);
extern void print_tree(struct element *e, int level);
extern void print_stats(void);


extern int stat_pass_raw_to_movement;
extern int stat_pass_vertical_G0;
extern int stat_pass_split_rings;

extern void pass_raw_to_movement(struct element *e);
extern void pass_vertical_G0(struct element *e);
extern void pass_split_by_tool(struct element *e);
extern void pass_split_g1(struct element *e);
extern void pass_bounding_box(struct element *e);
extern void pass_split_rings(struct element *e);



extern double dist3(double X1, double Y1, double Z1, double X2, double Y2, double Z2);
