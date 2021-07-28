import * as gcode from './gcode.js';
import * as tool from './tool.js';
import * as stl from './stl.js';

/* 
 * Segment API section
 *
 * Segments are moves you want to mill (X1,Y1,X1) -> (X2, Y2, Z2)
 * each segment has an (optional) tolerance where the tolerance is used
 * to avoid retracts if an adjacent segment is within this distance
 *
 * Two fundamental ways to create segments:
 *
 * push_segment(X1, Y1, Z1, X2, Y2, Z2) -- uncomplicated creation/queueing of a segment
 * push_segment_multilevel(X1,Y1,Z1, X2,Y2,Z2) -- takes the active tools max depth of cut into account and will dynamically create multiple segments
 * 
 * Two ways to create gcode from the segment data:
 * segments_to_gcode()  -- creates gcode while optimizing for fewest retracts (good for roughing etc)
 * segments_to_gcode_quick() -- computationally simpler option, it just creates exactly in order of queueing (good for finishing that has no natural retracts)
 */

class Segment {
  constructor()
  {
    this.X1 = -1.0;
    this.Y1 = -1.0;
    this.Z1 = -1.0;
    this.X2 = -1.0;
    this.Y2 = -1.0;
    this.Z2 = -1.0;
    this.want_cooling_retract = false;
    this.direct_mill_distance = 0.0001;
  }
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


/* returns 1 if A and B are within 4 digits behind the decimal */
function approx4(A, B) { 
	if (Math.abs(A-B) < 0.0002) 
		return 1; 
	return 0; 
}


/* 
 * Internally the segment tooling has a set of "levels" where 0 is the deepest. In the gcode generation, the levels
 * will be written top down (so high to low numerical). Think of a "level" as one depth of cut "layer" so that
 * if a multilevel segment is created, each layer of this per depth of cut is stored in a separate level.
 * Levels are what makes sure we cut the higher paths before the lower paths.
 */
let levels = [];


class Level {
  constructor(tool_number)
  {
    this.tool = tool_number;
    this.paths = [];
  }
}

/* 
 * Create a segment and push it, not honoring any depth-of-cut.
 * If this segment is just an extension of the previously pushed segment,
 * the segments may get merged into one larger segment for efficiency.
 */
export function push_segment(X1, Y1, Z1, X2, Y2, Z2, level = 0, direct_mill = 0.0001, cooling = false)
{
    if (X1 == X2 && Y1 == Y2 && Z1 == Z2) {
        return;
    }

    for (let lev = 0; lev <= level; lev++) {
        if (typeof(levels[lev]) == "undefined") {
            levels[lev] = new Level();
            levels[lev].tool = tool.tool_nr;
            levels[lev].paths = [];
        }
    }

//    console.log("X1 ", X1, " Y1 ", Y1, " Z1 ", Z1, " X2 ", X2, " Y2 ", Y2, " Z2 ", Z2, " level ", level);

    /* if the new segment is just an extension of the previous... merge them */    
    if (levels[level].paths.length > 0) {
        let prev = levels[level].paths[levels[level].paths.length - 1];
        if (prev.X1 == X1 && prev.Z1 == prev.Z2 && Z1 == Z2 && approx4(prev.Y2,Y1) && prev.Z1 == Z1 && X1 == X2) {
            levels[level].paths[levels[level].paths.length - 1].Y2 = Y2;
            return;
        }

        if (prev.Y1== Y1 && prev.Z1 == prev.Z2 && Z1 == Z2 && approx4(prev.X2, X1) && prev.Z1 == Z1 && Y1 == Y2) {
            levels[level].paths[levels[level].paths.length - 1].X2 = X2;
            return;
        }
    }
    
    let seg = new Segment();
    seg.X1 = X1;
    seg.Y1 = Y1;
    seg.Z1 = Z1;
    seg.X2 = X2;
    seg.Y2 = Y2;
    seg.Z2 = Z2;
    seg.direct_mill_distance = direct_mill;
    if (cooling) {
        seg.want_cooling_retract = true;
    }
    
    levels[level].paths.push(seg);
}

/*
 * Create a segment for a milling move, while keeping max Depth of Cut into account.
 * This means that multiple segments (each at a higher level) may be generated
 * as part of one API call. The higher-than-the-lowest levels are rounded such
 * that there is a higher chance of merging these higher levels together.
 */

export function push_segment_multilevel(X1, Y1, Z1, X2, Y2, Z2, direct_mill = 0.0001, skip_bottom = 0)
{
    let z1 = Z1;
    let z2 = Z2;
    let l = 0;
    let mult = 0.5;
    let divider = 1/tool.tool_depth_of_cut;
    
    let total_buckets = Math.ceil(stl.get_work_height() / (tool.tool_depth_of_cut/2)) + 2
    
    if (X1 == X2 && Y1 == Y2 && Z1 == Z2) {
        return;
    }
    
//    let jump = Math.abs(Math.ceil((global_maxZ + Math.min(Z1,Z2)) / tool.tool_depth_of_cut));
//    console.log("Jump is ", jump, "Z1 = ",Z1," Z2 = ", Z2);
    
//    console.log("X1 ", X1, " Y1 ", Y1, " Z1 ", Z1, " X2 ", X2, " Y2 ", Y2, " Z2 ", Z2);
            
    while (z1 < 0 || z2 < 0) {
        if (l != 0) {
            let dZ = -Math.min(z1, z2) / (tool.tool_depth_of_cut/2);
            dZ = Math.round(dZ);
            l = total_buckets - dZ;
        }
        if (skip_bottom < 1 || l > 0) {
            push_segment(X1, Y1, z1, X2, Y2, z2, l, direct_mill);
        }
        z1 = Math.ceil( (z1 + mult * tool.tool_depth_of_cut) * divider) / divider;
        z2 = Math.ceil( (z2 + mult * tool.tool_depth_of_cut) * divider) / divider;
        
        /* deal with rounding artifacts/boundary conditions elsewhere */
        if (z1 < -tool.tool_depth_of_cut/2) {
            z1 = z1 - 0.001;
        }
        if (z2 < -tool.tool_depth_of_cut/2) {
            z2 = z2 - 0.001;
        }
        z1 = Math.max(z1, z2);
        z2 = Math.max(z1, z2);
        l = l + 1;
        mult = 1.0;
    }         
}

/* Create gcode from the queued segments, while trying to avoid retracts.
 * maxlook is how hard to work on trying to avoid retracts.
 */
export function segments_to_gcode(maxlook = 1)
{
    console.log("S_T_G levels ", levels.length);
    for (let lev = levels.length - 1; lev >= 0; lev--) {
        let start = 0;
        /* force the first sgement to never be a hit to a previous <whatever> including previous layer */
        gcode.reset_gcode_location();
        while (levels[lev].paths.length > 0) {
          let found_one = 0;
          let max = levels[lev].paths.length;
          if (max > maxlook) {
              max = maxlook;
          }
          
          if (max > 1 && lev == 0) {
              max = 1;
          }
          if (start >= max) {
              start = max - 1;
          }
          
          /* step 1: try to find a segement that doesn't cause us to do a retract */
          for (let seg = start; seg < max; seg++) {
            let segm = levels[lev].paths[seg];
            
            if (gcode.distance_from_current(segm.X1, segm.Y1, segm.Z1) < segm.direct_mill_distance) {
                if (gcode.distance_from_current(segm.X1, segm.Y1, segm.Z1) > 0.00001) {
                    gcode.gcode_mill_to_3D(segm.X1, segm.Y1, segm.Z1);            
                }
                found_one = 1;
                gcode.gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);            
                
                levels[lev].paths.splice(seg, 1);
                start = seg;
                break;
                
            }
         }
         

          /* step 1: try to find a segement that is very nearby */
          for (let seg = 0; found_one == 0 && seg < max; seg++) {
            let segm = levels[lev].paths[seg];
            
            if (dist3(gcode.gcode_cX, gcode.gcode_cY, gcode.gcode_cZ, segm.X1, segm.Y1, segm.Z1) <= 2 * tool.tool_diameter) {
                found_one = 1;
                /* go up and horizontal to the start of the segment */
                if (dist3(gcode.gcode_cX, gcode.gcode_cY, gcode.gcode_cZ, segm.X1, segm.Y1, segm.Z1) >= 0.001)  {
                    gcode.gcode_travel_to(segm.X1, segm.Y1);
                }
                /* plunge */
                gcode.gcode_mill_down_to(segm.Z1);
                /* and mill */
                gcode.gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);            
                
                levels[lev].paths.splice(seg, 1);
                start = seg;
                break;
                
            }
         }
         
         if (found_one == 0) {
            /* step 2: we didn't find any, so just pick the first one */
            let segm = levels[lev].paths[0];
            /* go up and horizontal to the start of the segment */
            gcode.gcode_travel_to(segm.X1, segm.Y1);
            /* plunge */
            gcode.gcode_mill_down_to(segm.Z1);
            /* and mill */
            gcode.gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);            
            levels[lev].paths.splice(0, 1);
            start = 0;
        }
      }
    }
    levels = [];
}

/* computationally more efficient way to generate gcode from the queued segments,
 * but will not try to optimize for fewer retracts... useful for finishing passes
 */
export function segments_to_gcode_quick()
{
    for (let lev = levels.length - 1; lev >= 0; lev--) {
        for (let seg = 0; seg < levels[lev].paths.length; seg++) {
            let segm = levels[lev].paths[seg];
            
            if (dist3(gcode.gcode_cX, gcode.gcode_cY, gcode.gcode_cZ, segm.X1, segm.Y1, segm.Z1) > segm.direct_mill_distance) {
                gcode.gcode_travel_to(segm.X1, segm.Y1);
                /* plunge */
                gcode.gcode_mill_down_to(segm.Z1);
            } else {
                if (dist3(gcode.gcode_cX, gcode.gcode_cY, gcode.gcode_cZ, segm.X1, segm.Y1, segm.Z1) > 0.00001) {
                    gcode.gcode_mill_to_3D(segm.X1, segm.Y1, segm.Z1);            
                }
            }
            gcode.gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);
            if (segm.want_cooling_retract) {
                gcode.gcode_cooling_retract(segm.Z2);
            }
        }
    }
    levels = [];
    console.log("Total retract count", gcode.gcode_retract_count);
}

export function reset_segment()
{
    levels = [];
}