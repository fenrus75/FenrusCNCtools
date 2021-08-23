/* cache of global tool properties so that we don't; need to traverse datastructures for common things in the fast paths */

import * as gcode from './gcode.js';

export let tool_diameter = inch_to_mm(0.25);
let tool_feedrate = 0.0;
let tool_plungerate = 0.0;
let tool_geometry = "";
let tool_name = "";
export let tool_rippem = 24000;
export let tool_nr = 0;
export let tool_depth_of_cut = 0.1;
export let tool_stock_to_leave = 0.5;
export let tool_finishing_stepover = 0.3175;
export let tool_index = 0;
export let chipload = 0.025;
export let tool_roughing_feedrate = 0.0;

let precision = 1.0;
let manual_stepover = 0;

let material_multiplier = 1.0;

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
  if (manual_stepover > 0) {
    return;
  }
  tool_finishing_stepover = so;
  console.log("Setting stepover to ", tool_finishing_stepover);
}

export function set_stepover_gui(so)
{
  tool_finishing_stepover = so;
  manual_stepover = 1;
  console.log("Setting stepover to (gui)", tool_finishing_stepover, manual_stepover);
}

export function set_chipload(so)
{
  chipload = so;
}

/* ToolRings are created for each tool; they define a set of (X,Y) points relative to the center of
 * where the height of the deisgn will get probed.
 * A tool will have multiple such rings; each ring will be for one specific radius.
 */
class ToolRing {
    constructor (_R, is_radius)
    {
        this.R = _R;
        this.points = [];

        /* the normal wind directions N / S / E / W*/
        this.points.push(+1.0000 * _R);    this.points.push(+0.0000 * _R);
        this.points.push(+0.0000 * _R);    this.points.push(+1.0000 * _R);
        this.points.push(-1.0000 * _R);    this.points.push(-0.0000 * _R);
        this.points.push(-0.0000 * _R);    this.points.push(-1.0000 * _R);
        
        let angle = 0.2 * Math.random();
        let delta = 6.28 / 5;
        if (_R > 1) { delta = delta / 2; };
        if (_R > 2) { delta = delta / 1.75; };
        if (_R > 4) { delta = delta / 1.75; };
        if (_R > 6) { delta = delta / 1.75; };
        
        if (_R > 8 && is_radius == 0) { delta = delta * 4; };
        
        if (precision > 0)
          delta = delta / precision;
        
        if (is_radius > 0)
            delta = delta / 2.1;
        while (angle < 2 * 3.1415) {
          this.points.push(Math.sin(angle) * _R);    this.points.push(Math.cos(angle) * _R);
          angle = angle + delta;
        }        
    }
}


function print_rings(nr, rings)
{
	const ringcount = rings.length;
        console.log("Tool nr ", nr);
	for (let i = 0; i < ringcount ; i++) {
          const pointcount = rings[i].points.length;
          for (let p = 0; p < pointcount; p+=2) {
            console.log(rings[i].points[p] + ", " + rings[i].points[p+1] + ", ");
          } 
	}

}
/* 
 * Basic endmill data structure, has all the basic physical properties/sizes of the endmill
 * as well as the rings for probing
 */
 
const sqrt2 = Math.sqrt(2);
 
class Tool {
  constructor (_number, _diameter, _feedrate, _plungerate, _geometry, _depth_of_cut, _stock_to_leave = 0.0, _stepover = 0.0, flutes = 2, maxrpm = 24000) 
  {
      this.number = _number;
      this.diameter = 1.0 * _diameter;
      this.feedrate = 1.0 * _feedrate;
      this.roughing_feedrate = this.feedrate;
      this.roughing_woc = this.diameter / 2;
      this.rippem = maxrpm;
      this.flutes = flutes;
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
      let threshold = 0.3;
      if (precision > 0 ) {
          threshold = threshold / precision;
      }

    
      this.rings.push(new ToolRing((_diameter/2)-0.0001, 1));
      
      
      while (R > 0.1) {
            this.rings.push(new ToolRing(R, 0));
            if (R > 0.4) {
              R = R / sqrt2 - 0.0000001;
            } else {
              R = R - 0.1;
            }
            if (R < threshold) {
		break;
            }
      }
      
//      print_rings(_number, this.rings);
  }
  
  set_chipload(target_mm) 
  {
      let best_woc = 0;
      let best_rpm = 0;
      let best_feed = 0;    
      let best_mrr = 0;
      let woc = 0.01;
      let rpm = 6000;
      if (this.rippem < 6000) {
          this.rippem = 24000;
      }
      for (rpm = 6000; rpm <= this.rippem; rpm += 1000) {
        for (woc = 0.06 * this.diameter; woc <= this.diameter/4; woc += 0.0005) {
          let adjusted = target_mm;
          if (this.roughing_woc < this.diameter / 2) {
            adjusted = target_mm *
              this.diameter / (
                    2 * Math.sqrt(this.diameter * woc - woc*woc)
                  );
          }
          let rate = adjusted * this.flutes * rpm;
          
          if (rate > this.feedrate) {
            console.log("Rate exceeds ", rate, " " , this.feedrate);
            continue;
          }
          rate = Math.round(rate/100) * 100;
          
          let mrr = rate * woc;
          
          if (mrr > best_mrr) {
              best_mrr = mrr;
              best_feed = rate;
              best_woc = woc;
              best_rpm = rpm;
          }
          if (mrr == best_mrr && woc < best_woc) {
              best_feed = rate;
              best_woc = woc;
              best_rpm = rpm;
          }
        }
      }
      
      this.roughing_feedrate = best_feed;
      this.roughing_woc = best_woc;
      this.rippem = best_rpm;
      
      if (this.geometry != "flat") {
        this.roughing_feedrate = this.feedrate;
        this.rippem = 24000;
      }
      
      console.log("Feedrate set to ", best_feed, " for WOC of ", best_woc, " at rpm ", best_rpm);
      let link  = document.getElementById('resultfns');
      best_woc = Math.round(best_woc * 1000) / 1000;
      link.innerHTML = best_rpm + " rpm @ "+ best_feed + "mm/min, WOC is " + best_woc + " mm";
   }
}



export let tool_library = [];



//   constructor (_number, _diameter, _feedrate, _plungerate, _geometry, _depth_of_cut, _stock_to_leave = 0.0, _stepover = 0.0, flutes = 3) 

/* Creates all the tools */
export function tool_factory()
{
    if (tool_library.length > 0) {
        return;
    }
    tool_library.push(new Tool(666,   12.7, 5000, 500, "flat", 25, 0.25, 0.2,3));
    tool_library.push(new Tool(18,   2.0, inch_to_mm(20), inch_to_mm(10), "flat", 1.0, 0.1, 0.2));
    tool_library.push(new Tool(19,   1.5, inch_to_mm(15), inch_to_mm(7), "flat", 1.0, 0.1, 0.2));
    tool_library.push(new Tool(27,   1.0, inch_to_mm(200), inch_to_mm(30), "ball", 0.5, 0.0, 0.1));
    tool_library.push(new Tool(28,   0.5, inch_to_mm(200), inch_to_mm(30), "ball", 0.5, 0.0, 0.05)); 
    tool_library.push(new Tool(91,  inch_to_mm(0.50),  inch_to_mm(70), inch_to_mm(20), "flat", 2.0, 0.3));
    tool_library.push(new Tool(101, inch_to_mm(0.125), inch_to_mm(140), inch_to_mm(20), "ball", 1.0, 0.25));
    tool_library.push(new Tool(99,  inch_to_mm(0.125/2), inch_to_mm(140), inch_to_mm(20), "ball", 1.0, 0.25));
    tool_library.push(new Tool(102, inch_to_mm(0.125), inch_to_mm(75), inch_to_mm(50), "flat", 1.0, 0.25));
    tool_library.push(new Tool(201, inch_to_mm(0.250), inch_to_mm(180), inch_to_mm(20), "flat", 1.0, 0.25, 0.0, 3));
    tool_library.push(new Tool(78, inch_to_mm(0.250), inch_to_mm(180), inch_to_mm(20), "flat", 5.0, 0.25, 0.0, 2));
    tool_library.push(new Tool(278, inch_to_mm(0.250), inch_to_mm(180), inch_to_mm(20), "flat", 5.0, 0.25, 0.0, 1));
}


export function set_precision(p)
{
  precision = p;
}

export function select_tool(toolnr)
{
    /* reset some of the cached variables */
    gcode.reset_gcode_location();
    tool_factory();
    
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == toolnr) {
            tool_diameter = tool_library[i].diameter;
            tool_plungerate = tool_library[i].plungerate * Math.sqrt(material_multiplier);
            tool_geometry = tool_library[i].geometry;
            tool_name = tool_library[i].name;
            tool_nr = tool_library[i].number;
            tool_depth_of_cut = tool_library[i].depth_of_cut * Math.sqrt(material_multiplier);
            tool_stock_to_leave = tool_library[i].stock_to_leave * Math.sqrt(material_multiplier);
            tool_library[i].set_chipload(chipload);
            if (tool_library[i].rippem > 0) {
              tool_rippem = tool_library[i].rippem;
            } else {
              tool_rippem = 24000;
            }
            tool_feedrate = tool_library[i].feedrate;
            if (tool_library[i].roughing_feedrate > 0) {
              tool_feedrate = tool_library[i].roughing_feedrate;
            }
            tool_roughing_feedrate = tool_library[i].roughing_feedrate;
            if (manual_stepover == 0) {
              tool_finishing_stepover = tool_library[i].stepover;
            }
            tool_index = i;
            return;
        }
    }
    console.log("UNKNOWN TOOL ", toolnr);    
}

export function woc_from_chipload(toolnr)
{
    /* reset some of the cached variables */
    gcode.reset_gcode_location();
    tool_factory();
    
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == toolnr) {
            tool_library[i].set_chipload(chipload);
            return tool_library[i].roughing_woc;
        }
    }
    console.log("UNKNOWN TOOL ", toolnr);    
    return 0;
}



/* 2D distance function */
function dist(x1,y1,x2,y2)
{
	return Math.sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
} 

/* 3D distance function */
function dist3(x1,y1,z1,x2,y2,z2)
{
	return Math.sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)+(z1-z2)*(z1-z2));
}

/* 
 * Helper to calculate what speed to mill at, which
 * is either feedrate or plungerate, depending on if the horizontal
 * or vertical element of the move dominates
 */
export function toolspeed3d(cX, cY, cZ, X, Y, Z)
{
	let horiz = dist(cX, cY, X, Y);
	let d = dist3(cX, cY, cZ, X, Y, Z);
	let vert = cZ - Z;
	let time_horiz, time_vert;

	time_horiz = horiz / tool_feedrate;

	/* if we're milling up or only slightly down, feedrate dominates by definition */
	if (vert <= horiz) {
            return tool_feedrate;
	}

	
	/* scenario 1: feedrate dominates */
	if (time_horiz > 0.000001) {
		/* check if the effective plungerate is below max plung rate */
		if (vert / time_horiz < tool_plungerate) {
			return tool_feedrate;
		}
	}

	/* when we get here, plunge rate dominates */
	return tool_plungerate;
}

/*
 * Calculate how much higher than the center point the bit geometry is at distance R
 *
 * this obviously is a different formula for different endmill types 
 */

export function geometry_at_distance(R)
{
    if (tool_geometry == "ball") {
        let orgR = radius();
	return orgR - Math.sqrt(orgR*orgR - R*R);
    }
    
    return 0;
}

export function radius()
{
  return tool_diameter / 2;
}

export function name()
{
  return tool_name;
}

export function plungerate()
{
  return tool_plungerate;
}

export function custom_rippem(value)
{
    tool_factory();
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == 666) {
            tool_library[i].rippem = value;
            console.log("Max RPM set to ", tool_library[i].rippem);
        }
    }
    return 0;
}

export function custom_diameter(value)
{
    tool_factory();
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == 666) {
            tool_library[i].diameter = value;
            console.log("Diameter set to ", tool_library[i].diameter);
        }
    }
    return 0;
}

export function custom_feedrate(value)
{
    tool_factory();
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == 666) {
            tool_library[i].feedrate = value;
            console.log("Feedrate set to ", tool_library[i].feedrate);
        }
    }
    return 0;
}

export function custom_plungerate(value)
{
    tool_factory();
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == 666) {
            tool_library[i].plungerate = value;
            console.log("Plungerate set to ", tool_library[i].plungerate);
        }
    }
    return 0;
}

export function custom_flutes(value)
{
    tool_factory();
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == 666) {
            tool_library[i].flutes = value;
            console.log("Flutes set to ", tool_library[i].flutes);
        }
    }
    return 0;
}

export function custom_doc(value)
{
    tool_factory();
    for (let i = 0; i < tool_library.length; i++) {
        if (tool_library[i].number == 666) {
            tool_library[i].depth_of_cut = value;
            console.log("DOC set to ", tool_library[i].depth_of_cut);
        }
    }
    return 0;
}

