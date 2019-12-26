/*
 * (C) Copyright 2019  -  Arjan van de Ven <arjanvandeven@gmail.com>
 *
 * This file is part of FenrusCNCtools
 *
 * SPDX-License-Identifier: GPL-3.0
 */
#include "tool.h"
extern "C" {
    #include "toolpath.h"
}

void tooldepth::print_as_svg(void)
{
    for (auto i : toollevels) {
        i->print_as_svg();
    }
}


void tooldepth::output_gcode(void)
{
    if (!run_reverse) {
      for (auto i =  toollevels.rbegin(); i != toollevels.rend(); ++i) {
        (*i)->output_gcode();
      }
    } else {
      for (auto i =  toollevels.begin(); i != toollevels.end(); ++i) {
        (*i)->output_gcode();
      }
    }

}
