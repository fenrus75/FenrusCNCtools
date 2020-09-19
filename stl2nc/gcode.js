/*
 * GCODE helper library 
 * (should move into its own .js file at some point once I figure out how that works *
 *
 * Consists of a set of very low level functions for gcode plumbing, and all functions
 * are "direct" in that they emit gcode instantly, there's nothing doing operations ordering
 * after this.
 *
 * The most useful general functions are
 * gcode_retract()   -- get the endmill to a safe-to-move height 
 * gcode_travel_to(X,Y) -- retracts (if needed) and then fast-moves to the location (X,Y)    (G0 in gcode speak)
 * gcode_mill_to_3D(X, Y, Z) -- mill from the current location to (X, Y, Z)     (G1 in gcode speak)
 * gcode_mill_down_to(Z) -- go straight down, first at G0 speed until the bit gets close to the material, then at plunge speed
 * gcode_select_tool(tool nr) -- look up a tool in the database and select it -- does not write out gcode
 * gcode_change_tool(tool nr) -- change a tool (internal caches) and write out the gcode for the toolchange   
 */

import * as tool from './tool.js';

export let gcode_content = [];
export let gcode_string = "";

let safe_retract_height = 2.0;
let rippem = 18000.0;

let split_gcode = 0;

export function set_split_gcode(v)
{
    split_gcode = v;
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

function gcode_write(str)
{
    gcode_string = gcode_string + str + "\n";
}

export function gcode_write_raw(str)
{
    gcode_string = gcode_string + str;
}

function gcode_comment(str)
{
    gcode_string = gcode_string + "(" + str + ")\n";
}


export let gcode_cX = 0.0;
export let gcode_cY = 0.0;
export let gcode_cZ = 0.0;
export let gcode_cF = 0.0;

export function reset_gcode_location()
{
    gcode_cX = -40000;
    gcode_cY = -40000;
    gcode_cZ = -40000;
    gcode_cF = -40000;
}

export function reset_gcode()
{
    gcode_string = "";
    gcode_content = [];
}


/* In gcode we want numbers to always have 4 decimals */
function gcode_float2str(value)
{
    return value.toFixed(4);
}

/* Standard header to write at the start of the gcode */
export function gcode_header(filename)
{
    gcode_write("%");
    gcode_write("G21"); /* milimeters not imperials */
    gcode_write("G90"); /* all relative to work piece zero */
    gcode_write("G0X0Y0Z" + gcode_float2str(safe_retract_height));
    gcode_cZ = safe_retract_height;
    gcode_comment("Created by STL2NC");
    gcode_comment("FILENAME: " + filename);
    gcode_first_toolchange = 1;
    gcode_retract_count = 0;
}

/* And close off the gcode by turning off the spindle/etc */
export function gcode_footer()
{
    gcode_write("M5");
    gcode_write("M30");
    gcode_comment("END");
    gcode_write("%");
    gcode_content.push(gcode_string);
    gcode_string = "";
}


/* 
 * before the first time setting up the tool we don't have to turn off the spindle since
 * it's not on yet
 */

let gcode_first_toolchange = 1;

export let gcode_retract_count = 0;

export function gcode_retract()
{
    gcode_write("G0Z" + gcode_float2str( safe_retract_height));
    gcode_cZ = safe_retract_height;
    gcode_retract_count += 1;
}


/*
* Moves the bit to some (X,Y) location at retract height. 
 * This will first retract the bit if it is not already retracted,
 * and then use the gcode G0 fast move command to go to (X,Y)
 */
export function gcode_travel_to(X, Y)
{
    if (gcode_cZ < safe_retract_height) {
        gcode_retract();
    }
    
    if (approx4(gcode_cX,X) && approx4(gcode_cY,Y)) {
        return;
    }
    let sX = "";
    let sY = "";
    
    if (!approx4(gcode_cX,X)) {
        sX = "X" + gcode_float2str(X);
    }
    if (!approx4(gcode_cY,Y)) {
        sY = "Y" + gcode_float2str(Y);
    }
    
    gcode_write("G0" + sX + sY);
    gcode_cX = X;
    gcode_cY = Y;
}

/* 
 * Internal helper to calculate what speed to mill at, which
 * is either feedrate or plungerate, depending on if the horizontal
 * or vertical element of the move dominates
 */
function toolspeed3d(cX, cY, cZ, X, Y, Z)
{
	let horiz = dist(cX, cY, X, Y);
	let d = dist3(cX, cY, cZ, X, Y, Z);
	let vert = cZ - Z;
	let time_horiz, time_vert;

	time_horiz = horiz / tool.tool_feedrate;

	/* if we're milling up, feedrate dominates by definition */
	if (vert <= 0) {
            return tool.tool_feedrate;
	}

	
	/* scenario 1: feedrate dominates */
	if (time_horiz > 0.000001) {
		/* check if the effective plungerate is below max plung rate */
		if (vert / time_horiz < tool.tool_plungerate) {
			return tool.tool_feedrate;
		}
	}

	/* when we get here, plunge rate dominates */
	return tool.tool_plungerate;
}

/* 
 * Mill from the current location to (X,Y,Z)
 */

export function gcode_mill_to_3D(X, Y, Z)
{
	let toolspeed;
	let command;
	
	command = "1";

	/* if all we do is straight go up, we can use G0 instead of G1 for speed */
	if (approx4(gcode_cX, X) && approx4(gcode_cY, Y) && (Z > gcode_cZ)) {
	    command = "0";
        }


	toolspeed = toolspeed3d(gcode_cX, gcode_cY, gcode_cZ, X, Y, Z);

	let sX = "";
	let sY = "";
	let sZ = "";
	let sF = "";

        if (gcode_cX != X) {
            sX = "X" + gcode_float2str(X);
	    gcode_cX = X;
	}
        if (gcode_cY != Y) {
            sY = "Y" + gcode_float2str(Y);
	    gcode_cY = Y;
	}

        if (gcode_cZ != Z) {
            sZ = "Z" + gcode_float2str(Z);
            gcode_cZ = Z;
        }

	toolspeed = Math.ceil(toolspeed /10)*10;

        if (gcode_cF != toolspeed && command == '1') {
            sF = "F" + toolspeed.toString();
        }
        
	if (command == '1') {
	    gcode_cF = toolspeed;
        }
        
        gcode_write("G" + command + sX + sY + sZ + sF);        
}

/* 
 * Go straight down after a retract in the fastest possible way (faster tham mill_to_3D) by
 * doing a combination of G0 and G1 moves
 */
export function gcode_mill_down_to(Z)
{
	let toolspeed;
	let sZ;
	let sF;

        if (gcode_cZ == Z)
            return;
	toolspeed = tool.tool_plungerate;
	
	let Z2 = Z + tool.tool_depth_of_cut * 1.5;
	
	if (Z2 < gcode_cZ) {
            sZ = "Z" + gcode_float2str(Z2);
	    gcode_write("G0"+sZ);
	}
	

        sZ = "Z" + gcode_float2str(Z);
        
        toolspeed = Math.floor(toolspeed);

        sF = "F" + toolspeed.toString();
        
        gcode_write("G1" + sZ + sF);        
        gcode_cZ = Z;
        gcode_cF = toolspeed;

}


export function gcode_write_toolchange()
{
    if (gcode_first_toolchange == 0) {
        gcode_retract();
        gcode_write("M5");
    }
    gcode_write("M6 T" + tool.tool_name);
    gcode_write("M3 S" + rippem.toString());
    gcode_write("G0 X0Y0");
    gcode_cX = 0.0;
    gcode_cY = 0.0;
    gcode_cF = -1.0;
    gcode_retract();
    gcode_first_toolchange = 0;    
}


export function gcode_change_tool(toolnr)
{
    console.log("CHANGE_TOOL", toolnr);
    tool.select_tool(toolnr);
    gcode_write_toolchange();
}



/* update the gcode in the GUI */
export function update_gcode_on_website(filename)
{
    var pre = document.getElementById('outputpre')
    pre.innerHTML = gcode_content[0];
    
    if (split_gcode == 1) {
        var link = document.getElementById('download')
        link.innerHTML = 'Download roughing-' + filename+".nc";
        link.href = "#";
        link.download = "roughing-" + filename + ".nc";
        link.href = "data:text/plain;base64," + btoa(gcode_content[0]);
        link = document.getElementById('download2')
        link.innerHTML = 'Download finishing-' + filename+".nc";
        link.href = "#";
        link.download = "finishing-" + filename + ".nc";
        link.href = "data:text/plain;base64," + btoa(gcode_content[1]);
    } else {
        var link = document.getElementById('download')
        link.innerHTML = 'Download ' + filename+".nc";
        link.href = "#";
        link.download = filename + ".nc";
        link.href = "data:text/plain;base64," + btoa(gcode_content[0]);
    }
}

