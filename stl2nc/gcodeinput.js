/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/


/*
 * GCode stateful tracking. GCode G is a stateful command set, 
 * where we need to track both the level of the G code as well as X Y Z and F
 * since commands we read or write are incremental.
 */
 
import * as stl from './stl.js';
 
let currentx = 0.0;
let currenty = 0.0;
let currentz = 0.0;
let currentf = 0.0;
let currentfout = 0.0;

let glevel = '0';
let gout = '';
let metric = 1;

let safeZ = 0.0;

let filename = "output.nc";

let dryrun = 1;



function inch_to_mm(inch)
{
    return Math.ceil( (inch * 25.4) *1000)/1000;
}

function mm_to_inch(mm)
{
    return Math.ceil(mm / 25.4 * 1000)/1000;
}

function to_mm(inch)
{
	if (metric)
		return inch;
	return inch_to_mm(inch);
}


/* Variable into which output gcode is collected */
let outputtext = "";

function approx2(A, B) { 
	if (Math.abs(A-B) < 0.0005) 
		return 1; 
	return 0; 
}



function mm_to_coord(mm)
{
	if (metric > 0)
		return mm;
	return mm / 25.4;
}

/**
 * Add "line" to the pending gcode output pile
 * Manipulates global outputtext and gout variables
 * 
 * @param {Number} line 	Line number of g-code being processed
 */
function emit_output(line)
{
	outputtext = outputtext + line;
	if (!line.includes("\n")) {
		outputtext = outputtext + "\n";
	}
	if (line[0] == 'G') {
		gout= line[0] + line[1];
	}
}


function FtoString(rate)
{
	return "" + rate.toFixed(3);
}


/**
 * Handle a "G0" (rapid move) command in the input gcode.
 * Handling involves updating the global bounding box
 * and then writing the output.
 * 
 * Manipulates global vars global_max(X/Y/Z) and global_min(X/Y/Z)
 * 
 * @param {Number} x	X co-ordinate
 * @param {Number} y	Y co-ordinate
 * @param {Number} z	Z co-ordinate
 * @param {Number} line	Line number in g-code
l */
function G0(x, y, z) 
{
	let s = "G0";

	if (z < safeZ - 0.01) {
		z = z + get_height(x, y);
	}

	if (gout == "G0")
		s = "";
	if (x != currentx)
		s += "X" + FtoString(x);
	if (y != currenty)
		s += "Y" + FtoString(y);
	if (z != currentz)
		s += "Z" + FtoString(z);
		

	emit_output(s);

	gout = "G0";
	currentx = x;
	currenty = y;
	currentz = z;
}


/* Handle a "G1" (cutting move) in the gcode input.
 *
 */
 
function emitG1(x, y, z, feed) 
{

	let s = "G1";
	
	let thisout = s;
	if (gout == thisout)
		s = "";
	if (x != currentx)
		s += "X" + FtoString(x);
	if (y != currenty)
		s += "Y" + FtoString(y);
	s += "Z" + FtoString(z);
	if (feed != currentf && thisout != "G0") {
		s += "F" + Math.round(feed);
	}
	
	gout = thisout;
	emit_output(s);
}

function dist2(X1, Y1, X2, Y2) 
{
	return Math.sqrt((X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2));
}
function dist3(X1, Y1, Z1, X2, Y2, Z2) 
{
	return Math.sqrt((X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2) +(Z2-Z1)*(Z2-Z1));
}


let pX, pY, pZ, pF, pV = 0;
let ppX, ppY, ppZ, ppF, ppV = 0;
function bufferG1(x, y, z, feed)
{
	if (ppV > 0 && pV > 0) {
		let d1 = dist2(ppX, ppY, pX, pY);
		let d2 = dist2(ppX, ppY, x, y);
		if (d2 > 0 && to_mm(d2) < 0.5) {
			let l = d1/d2;
			let newZ = ppZ + l * (z - ppZ);
			if (approx2(to_mm(newZ), to_mm(pZ))) {
				pX = x;
				pY = y;
				pZ = z;
				return;
			}
		}
	}
	
	if (ppV > 0) {
		emitG1(ppX, ppY, ppZ, feed);
		currentx = ppX;
		currenty = ppY;
		currentz = ppZ;
		currentf = feed;
	}
	
	ppX = pX; ppY = pY; ppZ = pZ; ppF = feed; ppV = pV;
	pX = x; pY = y; pZ = z; pF = feed; pV = 1;
}

function flush_buffer()
{
	if (ppV > 0) {
		emitG1(ppX, ppY, ppZ, ppF);
		currentx = ppX;
		currenty = ppY;
		currentz = ppZ;
		currentf = ppF;
		ppV = 0;
	}
	if (pV > 0) {
		emitG1(pX, pY, pZ, pF);
		currentx = pX;
		currenty = pY;
		currentz = pZ;
		currentf = pF;
		pV = 0;
	}
}

function get_height(x, y)
{
	if (metric > 0) {
		return stl.get_height(x, y);
	}
	
	return mm_to_inch(stl.get_height(inch_to_mm(x), inch_to_mm(y)));
}

function G1(x, y, z, feed) 
{
	let l = 0;
	let d = dist2(currentx, currenty, x, y);
	if (d <= 0.025) {
		let newZ = z + get_height(x, y);
		/* retracts need to still go all the way up */
		if (z >= safeZ - 0.01)
			newZ = z;
		emitG1(x,y,newZ,feed);
		currentx = x;
		currenty = y;
		currentz = z;
		currentf = feed;
		return;
	}
	let stepsize = 0.025 / d;
	if (metric == 0)
	 	stepsize = mm_to_inch(stepsize);
	let cX = currentx;
	let cY = currenty;
	let cZ = currentz;
	
	let dX = (x - currentx);
	let dY = (y - currenty);
	let dZ = (z - currentz);
	
	while (l < 1.0) {
		let newX = cX + dX * l;
		let newY = cY + dY * l;
		let newZ = cZ + dZ * l;
		
		if (newZ <= 0) {
			newZ += get_height(newX, newY);
		}
		
		bufferG1(newX, newY, newZ, feed);
		l = l + stepsize;
	}
	
	flush_buffer();
	let newZ = z + get_height(x, y);
	if (z > 0)
		newZ = z;
	emitG1(x,y,newZ,feed);
	
	currentx = x;
	currenty = y;
	currentz = z;
	currentf = feed;
}


/*
 * We got a line with X Y Z etc coordinates */
function handle_XYZ_line(line)
{
	let newX, newY, newZ, newF;
	newX = currentx;
	newY = currenty;
	newZ = currentz;
	newF = currentf;
	let idx;
	
	
	/* parse the x/y/z/f */	
	idx = line.indexOf("X");
	if (idx >= 0) {
		newX = parseFloat(line.substring(idx + 1));	
	}
	idx = line.indexOf("Y");
	if (idx >= 0)
		newY = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("Z");
	if (idx >= 0)
		newZ = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("F")
	if (idx >= 0) {
		newF = parseFloat(line.substring(idx + 1));	
		if (newF == 0) /* Bug in F360 as of Oct/2020  */
			newF == currentf;
	}

	/* arc commands are pass through .. nothing smart we can do */
	if (glevel == '2' || glevel == '3') {
		emit_output(line);
		currentx = newX;
		currenty = newY;
		currentz = newZ;
		currentf = -1.0;
		currentfout = -1.0;
		return;
	}
	
	/* G0/G1 we can do clever things with */	
	if (glevel == '1') 
		G1(newX, newY, newZ, newF, line);
	else
		G0(newX, newY, newZ, line);
}

function handle_XYZ_line_scan(line)
{
	let newX, newY, newZ, newF;
	newX = currentx;
	newY = currenty;
	newZ = currentz;
	newF = currentf;
	let idx;
	
	
	/* parse the x/y/z/f */	
	idx = line.indexOf("X");
	if (idx >= 0) {
		newX = parseFloat(line.substring(idx + 1));	
	}
	idx = line.indexOf("Y");
	if (idx >= 0)
		newY = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("Z");
	if (idx >= 0)
		newZ = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("F")
	if (idx >= 0) {
		newF = parseFloat(line.substring(idx + 1));	
		if (newF == 0) /* Bug in F360 as of Oct/2020  */
			newF == currentf;
	}

	if (newZ > safeZ)
		safeZ = newZ;
}

/*
 * Lines that start with G0/1 matter, all others we just pass through 
 */
function handle_G_line(line)
{
	if (line.includes("G20")) {
		metric = 0;
		console.log("Switching to empire units");
	}
	if (line.includes("G21")) {
		metric = 1;
		console.log("Using metric systemk");
	}
		
	if (line[1] == '3') glevel = '3';
	if (line[1] == '2') glevel = '2';
	if (line[1] == '1') glevel = '1';
	if (line[1] == '0') glevel = '0';
	
	if ((line[1] == '1' || line[1] == '0') && (line[2] < '0' || line[2] > '9'))
		return handle_XYZ_line(line);

	/* G line we don't need to process, just pass through */
	currentf = -1.0;
	currentfout = -1.0;
	emit_output(line);
}

function handle_G_line_scan(line)
{
	if (line.includes("G20")) {
		metric = 0;
		console.log("Switching to empire units");
	}
	if (line.includes("G21")) {
		metric = 1;
		console.log("Using metric system");
	}
		
	if (line[1] == '3') glevel = '3';
	if (line[1] == '2') glevel = '2';
	if (line[1] == '1') glevel = '1';
	if (line[1] == '0') glevel = '0';
	
	if ((line[1] == '1' || line[1] == '0') && (line[2] < '0' || line[2] > '9'))
		return handle_XYZ_line_scan(line);

	/* G line we don't need to process, just pass through */
	currentf = -1.0;
	currentfout = -1.0;
}

/* 
 * Process one line of gcode input, demultiplex the different commands 
 */
function process_line(line)
{
	if (line == "")
		return;
	let code = " ";
	code = line[0];
	
	switch (code) {
		case 'X':
		case 'Y':
		case 'Z':
			handle_XYZ_line(line);
			break;
		case 'G':
			handle_G_line(line);
			break;
		default:
			emit_output(line);
	}
}

function process_line_scan(line)
{
	if (line == "")
		return;
	let code = " ";
	code = line[0];
	
	switch (code) {
		case 'X':
		case 'Y':
		case 'Z':
			handle_XYZ_line_scan(line);
			break;
		case 'G':
			handle_G_line_scan(line);
			break;
	}
}


function line_range(lines, start, stop)
{
	for (let i = start; i < stop; i++)
		process_line(lines[i]);
		
	let elem = document.getElementById("Bar");
        elem.style.width = (Math.round(stop/lines.length * 100)) + "%";
        
        if (stop == lines.length) {
	        var link = document.getElementById('download')
	        link.innerHTML = 'save gcode';
	        link.href = "#";
	        link.download = filename;
	        link.href = "data:text/plain;base64," + btoa(outputtext);
	        var pre = document.getElementById('outputpre')
	        pre.innerHTML = outputtext;
	        console.log("Printing gcode");
	}

}


/**
 * Main processing function which works through the supplied gcode, inspects
 * and modifies it as necessary
 * 
 * @param {string} data 
 */
export function process_data(data)
{
    let lines = data.split("\n")
    let len = lines.length;
    
    var start;
    
    currentx = 0.0;
    currenty = 0.0;
    currentz = 0.0;
    currentf = 0.0;

    glevel = '0';
    let stepsize = 1;
    
    console.log("Data is " + lines[0] + " size  " + lines.length);
    
    start = Date.now();
    
    console.log("Start of gcode pre-parsing at " + (Date.now() - start));
    for (let i = 0; i < len; i ++) {
    	process_line_scan(lines[i]);
    }
    
    console.log("Retract height is ", safeZ);

    console.log("Start of gcode parsing at " + (Date.now() - start));

    currentx = 0.0;
    currenty = 0.0;
    currentz = 0.0;
    currentf = 0.0;

    glevel = '0';

    outputtext = "";

    stepsize = Math.round((lines.length + 1)/100);
    if (stepsize < 1)
    	stepsize = 1;
    for (let i = 0; i < len; i += stepsize) {
    	let end = i + stepsize;
    	if (end > lines.length)
    		end = lines.length;
	setTimeout(line_range, 0, lines, i, end);
    }
    
    console.log("End of parsing at " + (Date.now() - start));
}


