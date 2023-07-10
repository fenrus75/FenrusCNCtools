#pragma once

#include "render.h"
#include "tool.h"


extern double find_best_correlation(render *base, render *plug);
extern double save_as_xpm(const char *filename, render *base,render *plug, double offset);
extern void save_as_stl(const char *filename, render *base,render *plug, double offset, bool dobase = true,bool doplug=true, double zoomfactor = 1.0);

extern void find_least_overlap(render *base, render *plug, double depth);
extern void save_overlap_as_stl(const char *filename, render *base,render *plug, double offset, double zoomf);