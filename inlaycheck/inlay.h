#pragma once

#include "render.h"
#include "tool.h"


extern double find_best_correlation(render *base, render *plug);
extern void save_as_xpm(const char *filename, render *base,render *plug, double offset);
extern void save_as_stl(const char *filename, render *base,render *plug, double offset);