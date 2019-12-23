#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

extern "C" {
    #include "toolpath.h"
}

#include <vector>

using namespace std;

struct tool {
    int number;
    const char *name;
    double diameter_inch;
    double depth_inch;
    double feedrate_ipm;
    double plungerate_ipm;
};

static vector<struct tool *> tools;

static struct tool * current;

/* 
0 number,
1 vendor,
2 model,
3 URL,
4 name,
5 type,
6 diameter,
7 cornerradius,
8 flutelength,
9 shaftdiameter,
10 angle,
11 numflutes,
12 stickout,
13 coating,
14 metric,
15 notes,
16 machine,
17 material,
18 plungerate,
19 feedrate,
20 rpm,
21 depth,
22 cutpower,
23 finishallowance,
24 3dstepover,
25 3dfeedrate,
26 3drpm
*/
static void push_word(char *word, int level)
{
    if (!current)
        current = (struct tool *)calloc(1, sizeof(struct tool));
        
    if (level == 0)
        current->number = strtoull(word, NULL, 10);
    if (level == 4)
        current->name = strdup(word);
    if (level == 6)
        current->diameter_inch = strtod(word, NULL);
    if (level == 21)
        current->depth_inch = strtod(word, NULL);
    if (level == 19)
        current->feedrate_ipm = strtod(word, NULL);
    if (level == 18)
        current->plungerate_ipm = strtod(word, NULL);
}

static void print_tool(struct tool *current)
{
    printf("Tool %i (%s)\n", current->number, current->name);
    printf("\tDiameter     : %5.3f\"  (%5.1f mm)\n", current->diameter_inch, inch_to_mm(current->diameter_inch));
    printf("\tDepth of cut : %5.3f\"  (%5.1f mm)\n", current->depth_inch, inch_to_mm(current->depth_inch));
    printf("\tFeedrate     : %5.0f ipm (%5.0f mmpm)\n", current->feedrate_ipm, ipm_to_metric(current->feedrate_ipm));
    printf("\tPlungerate   : %5.0f ipm (%5.0f mmpm)\n", current->plungerate_ipm, ipm_to_metric(current->plungerate_ipm));
}

void print_tools(void)
{
    for (auto tool : tools)
        print_tool(tool);
}

static void finish_tool(void)
{
    tools.push_back(current);
    if (verbose) {
        print_tool(current);
    }
    current = NULL;
}

static void parse_line(char *line)
{
    char word[4096];
    int item = 0;
    int quotelevel = 0;
    char *c;
    memset(word, 0, 4096);
    
    c = line;
    while (c && *c != 0) {
        if ( ((*c) == ',' && quotelevel == 0) || (*c) == '\n') {
            push_word(word, item);
            item++;
            memset(word, 0, 4096);
        } else {
            word[strlen(word)] = *c;
        }
        c++;
    }
    
    finish_tool();
    
}


void read_tool_lib(const char *filename)
{
    size_t n;
    FILE *file;
    char *line = NULL;
    int linenr = 0;
    
    file = fopen(filename, "r");
    if (!file) {
        printf("Cannot open tool file %s: %s\n", filename, strerror(errno));
    }
    
    while (!feof(file)) {
        int ret;
        
        n = 0;
        ret = getline(&line, &n, file);
        if (ret <= 0)
            break;
        if (linenr > 0)
            parse_line(line);
        free(line);
        line = NULL;
        linenr++;
    }
    fclose(file);
    
}

int have_tool(int nr)
{
    for (auto tool : tools)
        if (tool->number == nr)
            return 1;
    return 0;
}

void activate_tool(int nr)
{
    char toolnr[32];
    for (auto tool : tools)
        if (tool->number == nr) {
            sprintf(toolnr, "T%i", tool->number);
            print_tool(tool);
            set_tool_imperial(toolnr, tool->diameter_inch, tool->diameter_inch/2, tool->depth_inch, tool->feedrate_ipm, tool->plungerate_ipm);
        }
}

