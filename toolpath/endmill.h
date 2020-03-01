#pragma once


extern "C" {
	#include "toolpath.h"
}

class endmill {
public:
	endmill() {
		toolnr = 0;
		angle = 0;
		name = "";
		svgcolor = "black";
		depth_of_cut = 0;
		feedrate = 0;
		plungerate = 0;
		printed = false;
	};

	double get_diameter(void) 		{ return diameter; };
	double get_angle(void)  		{ return angle; };
	double get_depth_of_cut(void)  	{ return depth_of_cut; };
	double get_feedrate(void)	  	{ return feedrate; };
	double get_plungerate(void)	  	{ return plungerate; };


	void set_diameter_mm(double d) 	{ diameter = d; };
	void set_diameter_inch(double d) { diameter = inch_to_mm(d); };

	void set_angle(double d)		{ angle = d; };

	void set_depth(double d)		{ depth_of_cut = d; };
	void set_depth_inch(double d)	{ depth_of_cut = inch_to_mm(d); };

	void set_feedrate(double d)		{ feedrate = d; };
	void set_feedrate_ipm(double d)	{ feedrate = ipm_to_metric(d); };

	void set_plungerate(double d)		{ plungerate = d; };
	void set_plungerate_ipm(double d)	{ plungerate = ipm_to_metric(d); };

	void set_name(const char *c)	{ name = strdup(c); };
	void set_tool_nr(int n)			{ toolnr = n; };

	int get_tool_nr(void)			{ return toolnr; };
	const char *get_tool_name(void)	{ return name; };
	const char *get_svgcolor(void)  { return svgcolor; };


/* virtual methods that differ per endmill type */

	virtual double get_stepover(void)   { return diameter / 2; };
	virtual bool is_vbit(void)			{ return false; };
	virtual bool is_ballnose(void)		{ return false; };
	virtual void print(void);
private:
	int toolnr;
	double diameter;
	double angle;
	const char *name;
    const char *svgcolor;
    double depth_of_cut;
    double feedrate;
    double plungerate;
	bool printed;
};


class endmill_vbit : public endmill {
public:
	virtual bool is_vbit(void)			{ return true; };
	
};

class endmill_ballnose : public endmill {
public:
	virtual bool is_ballnose(void)		{ return true; };
};

