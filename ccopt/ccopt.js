/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/



"use strict;"

/*
 * GCode stateful tracking. GCode G is a stateful command set, 
 * where we need to track both the level of the G code as well as X Y Z and F
 * since commands we read or write are incremental.
 */
let currentx = 0.0;
let currenty = 0.0;
let currentz = 50.0;
let parserz = 50.0;
let currentf = 0.0;
let parserf = 0.0;
let glevel = '0';
let gout = '';
let metric = 1;

let filename = "output.nc";


let magicX = -500000;
let magicY = -500000;
let slowNext = false;

let pendingZ = 0.0;
let hasPendingZ = false;

/* Variable into which output gcode is collected */
let outputtext = "";


function mm_to_coord(mm)
{
	if (metric > 0)
		return mm;
	return mm / 25.4;
}


function flushPendingZ() {
	if (!hasPendingZ)
		return;
	_G0(currentx, currenty, pendingZ);
	hasPendingZ = false;
}

let points = [];

function Point(X, Y, Z) {
	this.X = X;
	this.Y = Y;
	this.Z = Z;
}


function get_and_update_depth(X, Y, newZ)
{
	for (var i = 0; i < points.length; i++) {
		if (approx3(points[i].x, X) && approx3(points[i].y, Y)) {
			var z = points[i].z;
			if (newZ < points[i].z) {
				points[i].z = newZ;
			}
			return z;
		}
	}
	points.push(new Point(X, Y, newZ));
	if (points.length > 32) {
		points.shift();
	}
	return 5002000;
}


let trainSize = 0;
let trainX1 = 0.0;
let trainY1 = 0.0;
let trainX2 = 0.0;
let trainY2 = 0.0;
let trainZ = -500000.0;
let trainX = [];
let trainY = [];
let trainValid = false;

function startTrain(X, Y, Z)
{
	trainSize = 1;
	trainX1 = X;
	trainY1 = Y;
	trainX2 = X;
	trainY2 = Y;
	trainZ = Z;
	trainX[0] = X;
	trainY[0] = Y;
	trainValid = true;
}

function wipeTrain()
{
	trainSize = 0;
	trainValid = false;
}

function addPointToTrain(X, Y)
{
	if (trainValid == false) {
		trainSize = 0;
		trainX1 = X;
		trainY1 = Y;
	}
	trainValid = true;
	trainX[trainSize] = X;
	trainY[trainSize] = Y;
	trainSize = trainSize + 1;
	trainX2 = X;
	trainY2 = Y;
}

function retraceTrain()
{
	if (!trainValid)
		return;
	if (trainSize < 2)
		return;
	if (trainZ != currentz)
		return;
	if (trainX2 != currentx || trainY2 != currenty )
		return;

	let newZ = currentz  /* + 0.02; */
	/* _G0(currentx, currenty, newZ);  go up a teensy bit to not scratch the finish */
	for (var i = trainSize - 1; i>= 0; i--) {
		_G0(trainX[i], trainY[i], newZ);
	}
}

function approx3(A, B)
{
        if (!metric) {
                return 0;
        }
        if (Math.abs(A - B) < 0.001) {
                return 1;
        }
        return 0;
}

function dist2(x1,y1,x2,y2)
{
        return Math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
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
	if (metric) {
		return "" + rate.toFixed(2);
	} else {
		return "" + rate.toFixed(3);
	}
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
 */
function G0(x, y, z) 
{
	let s = "G0";


	if (hasPendingZ && trainX1 == x && trainY1 == y && trainX2 == currentx && trainY2 == currenty && trainValid) {
		retraceTrain();
	}
	
	wipeTrain();

	if (approx3(x, currentx) && approx3(y, currenty)) {
		/*
		 * Two cases: Z goes up or Z goes down(or equal) from current
		 * If Z goes up, we defer the movement into a "pending" state,
		 * and we group multiple such up movements into one actual G0 command.
		 *
		 * If Z goes down, we cancel any defered up movements and just go down.
		 */
		if (z > currentz) {
			pendingZ = z;
			hasPendingZ = true;
			return;
		}
		if (z <= currentz) {
			hasPendingZ = false;
			get_and_update_depth(x, y, z);
		}
	}

	flushPendingZ();

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
	slowNext = false;
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
 */
function _G0(x, y, z) 
{
	let s = "G0";

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
 * This involves 
 * - updating the bounding box.
 * - applying the retract optimization in case the G1 only goes straight up
 * - applying the rapidtop optimization when the move is at or above the level for rapids
 * - applying the rapidplunge optimization in case this goes straight down 
 */
 
function G1(x, y, z, feed) 
{
	let s = "G1";
	
	
	
	if (approx3(x, currentx) && approx3(y, currenty)) {
		/* We're vertical only movement */
		magicX = x;
		magicY = y;
		slowNext = false;
		
                /*
                 * Two cases: Z goes up or Z goes down(or equal) from current
                 * If Z goes up, we defer the movement into a "pending" state,
                 * and we group multiple such up movements into one actual G0 command.
                 *
                 * If Z goes down, we cancel any defered up movements and just go down.
                 */
                 		
		if (z > currentz) {
			pendingZ = z;
			hasPendingZ = true;
			return;	
		}
		if (z <= currentz) {
			startTrain(x,y,z);
			hasPendingZ = false;
			
			let prevdepth = get_and_update_depth(x, y, z) + 0.1;
			if (prevdepth < z) {
                                prevdepth = z;
                        }
                        /* if we've been deeper before, just G0 there */
                        if (prevdepth < currentz) {
                                _G0(x,y,prevdepth);
                        }
			
		}		
		
	} else {
		if (slowNext) {
			feed = feed / 2;
			slowNext = false;
			magicX = x;
			magicY = y;
			if (trainX2 == currentx && trainY2 == currenty && trainZ == z) 
				addPointToTrain(x, y);
		} else {
			if (x == magicX && y == magicY) {
				slowNext = true;
			}
		}
	}
	flushPendingZ();
	
	
	if (dist2(currentx, currenty, x, y) < 0.0125 && currentz == z) {
		/* travel distance less than half the step size; just don't emit */
		return;
	}


	if (s == gout)
		s = "";
	if (x != currentx)
		s += "X" + FtoString(x);
	if (y != currenty)
		s += "Y" + FtoString(y);
	if (z != currentz)
		s += "Z" + FtoString(z);
	if (feed != currentf) {
		s += "F" + feed.toFixed(1);
	}
	
	emit_output(s);

	currentx = x;
	currenty = y;
	currentz = z;
	currentf = feed;
}

function handle_M_line(line)
{
	emit_output(line);
	currentf = -500000000;
	currentx = -500000000;
	currenty = -500000000;
	currentz = 50;
}

/*
 * We got a line with X Y Z etc coordinates 
 */
function handle_XYZ_line(line)
{
	let newX, newY, newZ, newF;
	newX = currentx;
	newY = currenty;
	newZ = parserz;
	newF = parserf;
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
		parserz = newZ;
		parserf = newF;
		return;
	}
	
	/* G0/G1 we can do clever things with */	
	if (glevel == '1') 
		G1(newX, newY, newZ, newF);
	else
		G0(newX, newY, newZ);
		
	parserz = newZ;
	parserf = newF;
}

function tool_change(line)
{
	emit_output(line);
}

/*
 * Lines that start with G0/1 matter, all others we just pass through 
 */
function handle_G_line(line)
{
	if (line.includes("G20"))
		metric = 0;
	if (line.includes("G21"))
		metric = 1;
		
	if (line[1] == '3') glevel = '3';
	if (line[1] == '2') glevel = '2';
	if (line[1] == '1') glevel = '1';
	if (line[1] == '0') glevel = '0';
	
	if ((line[1] == '1' || line[1] == '0') && (line[2] < '0' || line[2] > '9'))
		return handle_XYZ_line(line);

	/* G line we don't need to process, just pass through */
	currentf = -1.0;
	currentx = -5000000;
	currenty = -5000000;
	currentz = 50;
	emit_output(line);
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
		case 'M':
			flushPendingZ();
			wipeTrain();
			points = [];;
			handle_M_line(line);
			break;
		case 'X':
		case 'Y':
		case 'Z':
			handle_XYZ_line(line);
			break;
		case 'G':
			handle_G_line(line);
			break;
		case 'T':
			flushPendingZ();
			wipeTrain();
			points = [];;
			tool_change(line);
			break;
		case ' ': 
		case '':
		case '%':
		case '(':
		case 'S': /* no special handling of S lines yet */
			flushPendingZ();
			wipeTrain();
			emit_output(line);
			points = [];;
			break;
		
		default:
			flushPendingZ();
			points = [];;
			wipeTrain();
			emit_output(line);
			if (line != "" && line != " ") {
				console.log("Unknown code for line -" + line + "-\n");
			}
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
	}

}


/**
 * Main processing function which works through the supplied gcode, inspects
 * and modifies it as necessary
 * 
 * @param {string} data 
 */
function process_data(data)
{
    let lines = data.split("\n")
    let len = lines.length;
    
    var start;
    
    currentx = -500000.0;
    currenty = -500000.0;
    currentz = 50;
    currentf = -555550.0;
    glevel = 'X';
    let stepsize = 1;
    
    console.log("Data is " + lines[0] + " size  " + lines.length);
    
    start = Date.now();
    
    console.log("Start of parsing at " + (Date.now() - start));


    outputtext = "";
    
    len = lines.length;

    for (let i = 0; i < len; i++) {
    	process_line(lines[i]);
    }

    console.log("End of parsing at " + (Date.now() - start));
	        var link = document.getElementById('download')
	        link.innerHTML = 'save gcode';
	        link.href = "#";
	        link.download = filename;
	        link.href = "data:text/plain;base64," + btoa(outputtext);
	        var pre = document.getElementById('outputpre')
	        pre.innerHTML = outputtext;
}


function load(evt) 
{
    var start;
    start =  Date.now();
    if (evt.target.readyState == FileReader.DONE) {
        process_data(evt.target.result); console.log("End of data processing " + (Date.now() - start));
    }    
}

function basename(path) {
     return path.replace(/.*\//, '');
}

function handle(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
    	let fn = basename(f.name);
    	if (fn != "" && fn.includes(".nc")) {
    		filename = fn.replace(".nc", "-out.nc");
    	}
        var reader = new FileReader();
        reader.onloadend = load;
        reader.readAsText(f);
    }
}

function Speedlimit(value)
{
    let newlimit = parseFloat(value);
    
    if (newlimit > 0 && newlimit < 500) {
    	speedlimit = newlimit;
    }
}


document.getElementById('files').onchange = handle;

