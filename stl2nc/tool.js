/* cache of global tool properties so that we don't; need to traverse datastructures for common things in the fast paths */

import * as gcode from './gcode.js';

export let tool_diameter = inch_to_mm(0.25);
export let tool_feedrate = 0.0;
export let tool_plungerate = 0.0;
export let tool_geometry = "";
export let tool_name = "";
export let tool_nr = 0;
export let tool_depth_of_cut = 0.1;
export let tool_stock_to_leave = 0.5;
export let tool_finishing_stepover = 0.1;
export let tool_index = 0;

let high_precision = 0;

function inch_to_mm(inch)
{
    return Math.ceil( (inch * 25.4) *1000)/1000;
}

function mm_to_inch(mm)
{
    return Math.ceil(mm / 25.4 * 1000)/1000;
}

export function set_stepover(so)
{
  tool_finishing_stepover = so;
}



/* ToolRings are created for each tool; they define a set of (X,Y) points relative to the center of
 * where the height of the deisgn will get probed.
 * A tool will have multiple such rings; each ring will be for one specific radius.
 */
class ToolRing {
    constructor (_R)
    {
        this.R = _R;
        this.points = [];

        /* the normal wind directions N / S / E / W*/
        this.points.push(+1.0000 * _R);    this.points.push(+0.0000 * _R);
        this.points.push(+0.0000 * _R);    this.points.push(+1.0000 * _R);
        this.points.push(-1.0000 * _R);    this.points.push(-0.0000 * _R);
        this.points.push(-0.0000 * _R);    this.points.push(-1.0000 * _R);
        
        /* and the halfway points of those */
        
        this.points.push(+0.7071 * _R);    this.points.push(+0.7071 * _R);
        this.points.push(-0.7071 * _R);    this.points.push(+0.7071 * _R);
        this.points.push(-0.7071 * _R);    this.points.push(-0.7071 * _R);
        this.points.push(+0.7071 * _R);    this.points.push(-0.7071 * _R);
        
        if (high_precision == 0 && _R <= 0.5) { return; };
        
        /* and the halfway points again */
        
        this.points.push(+0.9239 * _R);    this.points.push(+0.3827 * _R);
        this.points.push(+0.3827 * _R);    this.points.push(+0.9239 * _R);
        this.points.push(-0.3827 * _R);    this.points.push(+0.9239 * _R);
        this.points.push(-0.9239 * _R);    this.points.push(+0.3827 * _R);

        this.points.push(-0.9239 * _R);    this.points.push(-0.3827 * _R);
        this.points.push(-0.3827 * _R);    this.points.push(-0.9239 * _R);
        this.points.push(+0.3827 * _R);    this.points.push(-0.9239 * _R);
        this.points.push(+0.9239 * _R);    this.points.push(-0.3827 * _R);
    }
}


/* 
 * Basic endmill data structure, has all the basic physical properties/sizes of the endmill
 * as well as the rings for probing
 */
 
const sqrt2 = Math.sqrt(2);
 
class Tool {
  constructor (_number, _diameter, _feedrate, _plungerate, _geometry, _depth_of_cut, _stock_to_leave = 0.0, _stepover = 0.0) 
  {
      this.number = _number;
      this.diameter = 1.0 * _diameter;
      this.feedrate = 1.0 * _feedrate;
      this.plungerate = 1.0 * _plungerate;
      this.geometry = _geometry;
      this.depth_of_cut = 1.0 * _depth_of_cut;
      this.stock_to_leave = 1.0 * _stock_to_leave;
      this.stepover = this.diameter / 10.0;
      this.name = _number.toString();
      this.rings = [];
            
      if (_stepover > 0) {
          this.stepover = 1.0 * _stepover;
      }

      /* now precompute the points to sample for height */      
      let R = _diameter;
      let threshold = 0.4;
      if (high_precision) {
          threshold = 0.25;
      }

    
//      this.rings.push(new ToolRing((_diameter/2)-0.0001));
      
      
      while (R > 0.1) {
            this.rings.push(new ToolRing(R));
            R = R / sqrt2 - 0.0000001;
            if (R < threshold) {
		break;
            }
      }
  }
}



export let tool_library = [];



//   constructor (_number, _diameter, _feedrate, _plungerate, _geometry, _depth_of_cut, _stock_to_leave = 0.0, _stepover = 0.0) 

/* Creates all the tools */
export function tool_factory()
{
    if (tool_library.length > 0) {
        return;
    }
    tool_library.push(new Tool(18,   2, inch_to_mm(20), inch_to_mm(10), "flat", 1.0, 0.1, 0.2));
    tool_library.push(new Tool(28, 0.5, inch_to_mm(50), inch_to_mm(20), "ball", 0.5, 0.0, 0.05)); 
    tool_library.push(new Tool(27,   1, inch_to_mm(50), inch_to_mm(20), "ball", 0.5, 0.0, 0.1));
    tool_library.push(new Tool(102, inch_to_mm(0.125), inch_to_mm(40), inch_to_mm(10), "flat", 1.0, 0.25));
    tool_library.push(new Tool(101, inch_to_mm(0.125), inch_to_mm(40), inch_to_mm(10), "ball", 1.0, 0.25));
    tool_library.push(new Tool(201, inch_to_mm(0.250), inch_to_mm(50), inch_to_mm(20), "flat", 1.0, 0.25));
}


export function set_precision(p)
{
  high_precision = p;
}

export function select_tool(toolnr)
{
    /* reset some of the cached variables */
    gcode.reset_gcode_location();
    tool_factory();
    
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == toolnr) {
            tool_diameter = tool_library[i].diameter;
            tool_feedrate = tool_library[i].feedrate;
            tool_plungerate = tool_library[i].plungerate;
            tool_geometry = tool_library[i].geometry;
            tool_name = tool_library[i].name;
            tool_nr = tool_library[i].number;
            tool_depth_of_cut = tool_library[i].depth_of_cut;
            tool_stock_to_leave = tool_library[i].stock_to_leave;
            tool_finishing_stepover = tool_library[i].stepover;
            tool_index = i;
            return;
        }
    }
    console.log("UNKNOWN TOOL ", toolnr);    
}


