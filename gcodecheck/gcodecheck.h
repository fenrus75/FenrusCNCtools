#pragma once

#include <math.h>

struct line {
	double X1, Y1, Z1;
	double X2, Y2, Z2;

	double toolradius;
	double toolangle;
	int tool;

	double minX, maxX, minY, maxY;
};

struct point {
	double X, Y, Z;
};


extern void read_gcode(const char *filename);
extern void print_state(FILE *output);

extern void verify_fingerprint(const char *filename);
extern double distance_point_from_vector(double X1, double Y1, double X2, double Y2, double pX, double pY, double *LL);

extern int verbose;
extern int errorcode;
extern int nospeedcheck;
#define vprintf(...) do { if (verbose) printf(__VA_ARGS__); } while (0)

#define error(...) do { fprintf(stderr, __VA_ARGS__); errorcode++;} while (0)

static inline double inch_to_mm(double inch) { return 25.4 * inch; };
static inline double mm_to_inch(double inch) { return inch / 25.4; };

static inline double ipm_to_metric(double inch) { return 25.4 * inch; };

static inline double depth_to_radius(double d, double angle) { return fabs(d) * tan(angle/360.0 * M_PI); }
static inline double radius_to_depth(double r, double angle) { return -r / tan(angle/360.0 * M_PI); }

extern "C" {
extern void set_tool_imperial(const char *name, int nr, double diameter_inch, double stepover_inch, double maxdepth_inch, double feedrate_ipm, double plungerate_ipm);
extern void read_tool_lib(const char *filename);
extern void activate_tool(int nr);
extern double get_tool_angle(int toolnr);

}