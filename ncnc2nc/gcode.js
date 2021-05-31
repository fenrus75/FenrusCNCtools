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

let gcode_content = [];
let gcode_string = "";

let safe_retract_height = 2.0;
let rippem = 18000.0;

let split_gcode = 0;
let slow_m3 = 0;

export function set_split_gcode(v)
{
    split_gcode = v;
}

export function set_router(v)
{
    if (v > 0) {
        slow_m3 = 0;
    } else {
        slow_m3 = 1;
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
function approx2(A, B) { 
	if (Math.abs(A-B) < 0.01) 
		return 1; 
	return 0; 
}


function gcode_write(str)
{
    gcode_string = gcode_string + str + "\n";
}

function gcode_comment(str)
{
    gcode_string = gcode_string + "(" + str + ")\n";
}


let gcode_cX = 0.0;
let gcode_cY = 0.0;
let gcode_cZ = 0.0;
let gcode_cF = 0.0;

let gcode_G = "9";

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
    gcode_retract_count = 0;
    reset_gcode_location();
}


/* In gcode we want numbers to always have 2 decimals */
function gcode_float2str(value)
{
    return value.toFixed(2);
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
    gcode_G="0";
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
    gcode_G = "0";
}


/* 
 * Mill from the current location to (X,Y,Z)
 */

export function gcode_mill_to_3D(X, Y, Z)
{
	let toolspeed;
	let command;
	
	command = "1";
	
	/* check if we're within rounding */
	if (approx2(gcode_cX, X) && approx2(gcode_cY, Y) && approx2(gcode_cZ, Z)) {
	    return;
        }

	/* if all we do is straight go up, we can use G0 instead of G1 for speed */
	if (approx2(gcode_cX, X) && approx2(gcode_cY, Y) && (Z > gcode_cZ)) {
	    command = "0";
        }


	toolspeed = tool.toolspeed3d(gcode_cX, gcode_cY, gcode_cZ, X, Y, Z);

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
        
        if (command == gcode_G) {
            gcode_write(sX + sY + sZ + sF);        
        } else {
            gcode_write("G" + command + sX + sY + sZ + sF);        
        }
        
        gcode_G = command;
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
	toolspeed = tool.plungerate();
	
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
        gcode_G = "1";

}


function gcode_write_toolchange()
{
    if (gcode_first_toolchange == 0) {
        gcode_retract();
        gcode_write("M5");
    }
    gcode_write("M6 T" + tool.name());
    gcode_write("M3 S" + rippem.toString());
    if (slow_m3 > 0) {
        gcode_write("G4P5.0");
    }
    gcode_write("G0 X0Y0");
    gcode_G = "0";
    gcode_cX = 0.0;
    gcode_cY = 0.0;
    gcode_cF = -1.0;
    gcode_retract();
    gcode_first_toolchange = 0;    
}


export function gcode_change_tool(toolnr)
{
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

export function distance_from_current(X, Y, Z)
{
    return dist3(gcode_cX, gcode_cY, gcode_cZ, X, Y, Z);
}