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
