#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <algorithm>

#include "endmill.h"
extern "C" {
    #include "toolpath.h"
}

static const char *colors[8] = {"darkgray", "red", "green", "blue", "purple", "orange", "yellow", "cyan"};
static int colornr;

#include <vector>

using namespace std;

struct tool {
    int number;
    const char *name;
    const char *svgcolor;
    double diameter_inch;
    double depth_inch;
    double feedrate_ipm;
    double plungerate_ipm;
    double angle;
    bool is_vcarve;
	bool is_ballnose;
	bool printed;
};

static vector<class endmill *> endmills;

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
    if (level == 6) {
		current->diameter_inch = strtod(word, NULL);
	}
	if (level == 5) {
		if (strstr(word, "vee")) 
			current->is_vcarve = true;
		if (strstr(word, "ball"))
			current->is_ballnose = true;
	}
    if (level == 10) {
        current->angle = strtod(word, NULL);
    }
    if (level == 21)
        current->depth_inch = strtod(word, NULL);
    if (level == 19)
        current->feedrate_ipm = strtod(word, NULL);
    if (level == 18)
        current->plungerate_ipm = strtod(word, NULL);
}

static bool compare_endmills(class endmill *A, class endmill *B)
{
	if (A->get_diameter() > B->get_diameter())
		return true;
	if (A->get_diameter() < B->get_diameter())
		return false;
	return (A->get_tool_nr() < B->get_tool_nr());
}

void print_tools(void)
{
    for (auto endmill : endmills)
        endmill->print();
}

static void finish_tool(void)
{
	class endmill *mill;
    current->svgcolor = colors[colornr++];
	if (colornr >= 8)
		colornr = 0;

	if (current->is_vcarve) {
		mill = new(class endmill_vbit);
	} else if (current->is_ballnose) {
		mill = new(class endmill_ballnose);
	} else {
		mill = new(class endmill);
	}

	mill->set_tool_nr(current->number);
	mill->set_name(current->name);
	mill->set_diameter_inch(current->diameter_inch);
	mill->set_depth_inch(current->depth_inch);
	mill->set_feedrate_ipm(current->feedrate_ipm);
	mill->set_plungerate_ipm(current->plungerate_ipm);
	mill->set_angle(current->angle);

	endmills.push_back(mill);

    if (verbose) {
        mill->print();
    }
	free(current);
    current = NULL;
}

const char * tool_svgcolor(int toolnr)
{
    toolnr = abs(toolnr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == toolnr)
            return endmill->get_svgcolor();
    return "black";
}

double tool_diam(int toolnr)
{
    toolnr = abs(toolnr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == toolnr)
            return endmill->get_diameter();
    return 0;
}


double get_tool_stepover(int toolnr)
{
    toolnr = abs(toolnr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == toolnr)
            return endmill->get_stepover();
    return 0.125;
}

int tool_is_vcarve(int toolnr)
{
    toolnr = abs(toolnr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == toolnr)
			return endmill->is_vbit();
    return 0;
}

int tool_is_ballnose(int toolnr)
{
    toolnr = abs(toolnr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == toolnr)
			return endmill->is_ballnose();
    return 0;
}

double get_tool_angle(int toolnr)
{
    toolnr = abs(toolnr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == toolnr)
            return endmill->get_angle();
    return 0;
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
    FILE *file;
    char line[40960];
    int linenr = 0;
    
    file = fopen(filename, "r");
    if (!file) {
		if (verbose)
	        printf("Cannot open tool file %s: %s\n", filename, strerror(errno));
		return;
    }
    
    while (!feof(file)) {
        char * ret;
        
        ret = fgets(&line[0], sizeof(line), file);
        if (!ret)
            break;
        if (linenr > 0)
            parse_line(line);
        linenr++;
    }
    fclose(file);
	sort(endmills.begin(), endmills.end()  , compare_endmills);    
}

int have_tool(int nr)
{
    nr = abs(nr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == nr)
            return 1;
    return 0;
}


const char * get_tool_name(int nr)
{
    nr = abs(nr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == nr)
            return endmill->get_tool_name();
    return "<none>";
}

int first_tool(void)
{
    for (auto endmill : endmills)
		return endmill->get_tool_nr();
    return -1;
}


int next_tool(int nr)
{
	bool prevhit = false;
    nr = abs(nr);
    for (auto endmill : endmills) {
		if (prevhit)
			return endmill->get_tool_nr();
        if (endmill->get_tool_nr() == nr)
            prevhit = true;
	}
    return -1;
}

void activate_tool(int nr)
{
    char toolnr[32];
    nr = abs(nr);
    for (auto endmill : endmills)
        if (endmill->get_tool_nr() == nr) {
            sprintf(toolnr, "T%i", endmill->get_tool_nr());
            endmill->print();
            set_tool_metric(toolnr, nr, endmill->get_diameter(), endmill->get_stepover(), endmill->get_depth_of_cut(), endmill->get_feedrate(), 
				endmill->get_plungerate());
        }
}

