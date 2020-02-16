#pragma once

extern void read_gcode(const char *filename);
extern void print_state(FILE *output);

extern void verify_fingerprint(const char *filename);

extern int verbose;
extern int errorcode;

#define vprintf(...) do { if (verbose) printf(__VA_ARGS__); } while (0)

#define error(...) do { fprintf(stderr, __VA_ARGS__); errorcode++;} while (0)

