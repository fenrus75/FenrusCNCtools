#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <algorithm>

#include "endmill.h"


void endmill::print(void)
{
	if (!printed || verbose) {
	    qprintf("Tool %i (%s)\n", get_tool_nr(), get_tool_name());
	    qprintf("\tDiameter     : %5.3f\"  (%5.1f mm)\n", mm_to_inch(get_diameter()), get_diameter());
	    qprintf("\tDepth of cut : %5.3f\"  (%5.1f mm)\n", mm_to_inch(get_depth_of_cut()), get_depth_of_cut());
	    qprintf("\tFeedrate     : %5.0f ipm (%5.0f mmpm)\n", mm_to_inch(get_feedrate()), get_feedrate());
	    qprintf("\tPlungerate   : %5.0f ipm (%5.0f mmpm)\n", mm_to_inch(get_plungerate()), get_plungerate());
	}
	printed = true;
}

double endmill::geometry_at_distance(double R)
{
	if (R < 0)
		return 0;
	return 0;
}

double endmill::distance_of_geometry(double R)
{
	if (R < 0)
		return 0;
	return 0;
}

double endmill_vbit::geometry_at_distance(double R)
{
	return R / tan(get_angle()/360.0 * M_PI);;
}


double endmill_vbit::distance_of_geometry(double R)
{
	return fabs(R) * tan(get_angle()/360.0 * M_PI); 
}

double endmill_ballnose::geometry_at_distance(double R)
{
	double orgR = get_diameter() / 2;
	return orgR - sqrt(orgR*orgR - R*R);
}


double endmill_ballnose::distance_of_geometry(double R)
{
	double orgR = get_diameter() / 2;
	double remain = orgR - R;
	if (R < 0)
		return 0;
	if (remain < 0 || remain > R)
		return orgR;

	return sqrt(orgR*orgR - remain*remain);
}
