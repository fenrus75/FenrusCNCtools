/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/



/* 
 * Global state trackers that track the bounding box of where the gcode goes */
let global_maxX = 0.0;
let global_maxY = 0.0;
let global_maxZ = 0.0;
/*
 * maxZlevel is different from maxZ in that it tries to track only places where moves happen 
 * to distinguish the height where the final retract happens into
 */
let global_maxZlevel = -500000.0;
let global_minX = 5000000.0;
let global_minY = 5000000.0;
let global_minZ = 5000000.0;


/*
 * array2D state variables. We have a 2D array, indexed by X,Y in work coordinates, in
 * blocks of 5mm x 5mm..
 * This 2D array contains a list of points that got visited in each of these blocks.
 *
 * This is used to quickly look up if a point previously was visited and what the Z was
 * in the previous visit.
 */
let xoffset = 0.0;
let yoffset = 0.0;
let array_xmax;
let array_ymax;
let array2D;



/*
 * GCode stateful tracking. GCode G is a stateful command set, 
 * where we need to track both the level of the G code as well as X Y Z and F
 * since commands we read or write are incremental.
 */
let currentx = 0.0;
let currenty = 0.0;
let currentz = 0.0;
let currentf = 0.0;
let currentfout = 0.0;
let glevel = '0';
let gout = '';
let metric = 1;

let diameter = 0.0;
let filename = "output.nc";

let dryrun = 1;
let isF360 = 0;


/* Variable into which output gcode is collected */
let outputtext = "";


/* optimization enable flags and matching counters */
let optimization_retract = 1;
let optimization_rapidtop = 1;
let optimization_rapidplunge = 1;

let count_retract = 0;
let count_rapidtop = 0;
let count_rapidplunge = 0;



function mm_to_coord(mm)
{
	if (metric > 0)
		return mm;
	return mm / 25.4;
}

/**
 * Allocate the 2D array of 5x5mm blocks.
 *
 * This array is used for storing "points", which are destinations that the gcode has visited
 * and their matching height.
 *
 * This is used to find (for plunges) what the previous deepest point was for that given (X,Y)
 * location.
 * 
 * Manipulates global array2D array
 * 
 * @param {Number} minX		Description of param
 * @param {Number} minY		Description of param
 * @param {Number} maxX		Description of param
 * @param {Number} maxY		Description of param
 */
function allocate_2D_array(minX, minY, maxX, maxY) {
	/* arrays are nicer if they start at 0 so calculate an offset */
	xoffset = - Math.floor(minX / 5.0);
	yoffset = - Math.floor(minY / 5.0);
	array_xmax = yoffset + Math.ceil(maxX / 5.0) + 4
	array_ymax = yoffset + Math.ceil(maxY / 5.0) + 4
	
	var x, y;

	array2D = new Array(array_ymax + 1)
	for (y = 0; y <= array_ymax; y++) {
		array2D[y] = new Array(array_xmax + 1)
		for (x = 0; x <= array_xmax; x++) {
			array2D[y][x] = [];
		}	
	}
	
}


function Position(X, Y, Z) {
	this.X = X;
	this.Y = Y;
	this.Z = Z;
}

/* 
 * For every move (G1) record the (X,Y,Z) so that in the future we can find the
 * deepest Z given an (X,Y)
 *
 */
function record_move_to(X, Y, Z) 
{
	/* 
	 * While doing the initial pass through the gcode, we don't have the 
	 * array yet to store Positionse into 
	 */
	if (dryrun > 0) {
		return;
	}
	var move = new Position(X, Y, Z);
	
	var minx, miny;
	
	minx = Math.floor(X/5.0) + xoffset;
	miny = Math.floor(Y/5.0) + yoffset;
	
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (minx >= array_xmax) minx = array_xmax - 1;
	if (miny >= array_ymax) miny = array_ymax - 1;
	array2D[miny][minx].push(move);
}

/*
 * returns the depth at a specific X,Y given past cuts.
 *
 * This is the simplified version that only look at end-points of cuts, not
 * the whole path long the gcode.
 */
function depth_at_XY(X, Y)
{
	var depth = global_maxZ;
	var m;
	
	
	var ax, ay;
	
	ax = Math.floor(X/5.0) + xoffset;
	ay = Math.floor(Y/5.0) + yoffset;
	
	if (ax < 0) ax = 0;
	if (ay < 0) ay = 0;
	if (ax >= array_xmax) ax = array_xmax - 1;
	if (ay >= array_ymax) ay = array_ymax - 1;
	
	var arr = array2D[ay][ax];
	
	var len = arr.length -1;
	
	for (var i = len; i >= 0; i--) {
		m = arr[i];
		var d;

		if (X == m.X && Y == m.Y)
			depth = Math.min(depth, m.Z);
	}

	return depth;
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


function g_of_line(line)
{
	if (line[0] == 'G')
		return line[0] + line[1];
	return "G" + glevel;	
}

function FtoString(rate)
{
	rate = Math.round(rate * 10000) / 10000;
	return "" + rate;
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
 */
function G0(x, y, z, line) 
{

	global_maxX = Math.max(global_maxX, x);
	global_maxY = Math.max(global_maxY, y);
	global_maxZ = Math.max(global_maxZ, z);
	global_minX = Math.min(global_minX, x);
	global_minY = Math.min(global_minY, y);
	global_minZ = Math.min(global_minZ, z);
	/* If there are rapids below the total maxZ... that counts */
	if ((currentx != x || currenty != y) && z < global_maxZ) {
		global_maxZlevel = Math.max(global_maxZlevel, z);
	}
	
	let s = "G0";

	if (gout == "G0")
		s = "";
	if (x != currentx)
		s += "X" + FtoString(x);
	if (y != currenty)
		s += "Y" + FtoString(y);
	if (z != currentz)
		s += "Z" + FtoString(z);
		


	if (gout == "G0" && g_of_line(line) == "G0") {
		emit_output(line);
		
	} else { 
		emit_output(s);
	}
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
 
function G1(x, y, z, feed, line) 
{
	let orgfeed = feed;
	global_maxX = Math.max(global_maxX, x);
	global_maxY = Math.max(global_maxY, y);
	global_maxZ = Math.max(global_maxZ, z);
	if (currentx != x || currenty != y) {
		global_maxZlevel = Math.max(global_maxZlevel, z);
	}
	global_minX = Math.min(global_minX, x);
	global_minY = Math.min(global_minY, y);
	global_minZ = Math.min(global_minZ, z);
	
	let s = "G1";
	
	if (optimization_retract == 1 && x == currentx && y == currenty && z > currentz) {
		/* This is a move straight up -- we can just use G0*/
		s = "G0";
		count_retract++;
	}
	
	if (optimization_rapidtop == 1 && z >= global_maxZlevel && currentz >= global_maxZlevel) {
		/* we're cutting at or above the level where we want to do only rapid moves */
		s = "G0";
		count_rapidtop++;
	}
	
	if (optimization_rapidplunge == 1 && dryrun == 0) {
		/* Are we pluging straight down ?*/
		if (currentx == x && currenty == y && z < currentz) {
			/* Find the previous deepest point at this (X, Y) ad back off by 0.2mm */
			let targetZ = depth_at_XY(x, y) + mm_to_coord(0.2);
			/* and make sure that this isn't deeper than we want to go in the first place */
			targetZ = Math.max(targetZ, z);
			/* if this Z is above our estination... rapid to there as an extra command */
			if (targetZ < currentz) {
				let s2 = "G0Z" + targetZ.toFixed(4);
				gout = "G0";
				emit_output(s2);
				currentz = targetZ;
				count_rapidplunge++;
			}
		}
		/* We want to remember this (X,Y) with this depth for the future */
		record_move_to(x, y, z);
	}
	
	
	let thisout = s;
	if (x != currentx)
		s += " X" + FtoString(x);
	if (y != currenty)
		s += " Y" + FtoString(y);
	if (z != currentz)
		s += " Z" + FtoString(z);
	if (feed != currentfout && thisout != "G0") {
		s += " F" + Math.round(feed);
		currentfout = feed;
	}
	

	if (gout == thisout && g_of_line(line) == thisout && currentf == feed) {
		emit_output(line);
		
	} else { 
		gout = thisout;
		emit_output(s);
	}

	currentx = x;
	currenty = y;
	currentz = z;
	currentf = feed;
}

function handle_M_line(line)
{
	emit_output(line);
	/* lines we can ignore */
	if (line == "M05") {
		return;
	}
	if (line == "M02")
		return;
		
	if (line.includes("M6"))
		return tool_change(line);
}

function handle_comment(line)
{
	emit_output(line);
	if (line.includes("Fusion 360")) {
		isF360 = 1;
	}
	/* Carbide Create tells us the size of the stock nicely in comments */
	if (line.includes("stockMax:")) {
		l = line.replace("(stockMax:","");
		global_maxX = parseFloat(l);
		let idx = l.indexOf(",");
		l = l.substring(idx + 1);
		global_maxY = parseFloat(l);
		idx = l.indexOf(",");
		l = l.substring(idx + 1);
		global_maxZ = parseFloat(l);
		global_maxZlevel = global_maxZ + 0.0001;
//		console.log("new maX/Y " + global_maxX+ " " + global_maxY + "\n");		
	}
	
	if (line.includes("(TOOL/MILL,")) {
		lastmillline = line;
		l = line.replace("(TOOL/MILL,","");
		diameter = parseFloat(l);
	}
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


	/* update the bounding box */
	global_maxX = Math.max(global_maxX, newX);
	global_maxY = Math.max(global_maxY, newY);
	global_maxZ = Math.max(global_maxZ, newZ);
	
	if ((newX != currentx || newY != currenty) && glevel != '0') {
		global_maxZlevel = Math.max(global_maxZlevel, newZ);
	}
	
	global_minX = Math.min(global_minX, newX);
	global_minY = Math.min(global_minY, newY);
	global_minZ = Math.min(global_minZ, newZ);

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
	currentfout = -1.0;
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
			handle_M_line(line);
			break;
		case '%':
		case '(':
			handle_comment(line);
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
			tool_change(line);
			break;
		case 'S': /* no special handling of S lines yet */
			emit_output(line);
			break;
		case ' ': /* F360 somehow emits blank lies */
		case '':
			handle_comment(line);
			break;
		
		
		default:
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

function log_dimensions()
{
	console.log("Detected dimensions:");
	console.log("    X : ", global_minX, " - ", global_maxX);
	console.log("    Y : ", global_minX, " - ", global_maxY);
	console.log("    Z : ", global_minZ, " - ", global_maxZ, "   with movement at ", global_maxZlevel);
}

function log_optimizations()
{
	console.log("Optimization statistic");
	console.log("   retract-to-G0 optimization      :", count_retract);
	console.log("   rapids at top of design         :", count_rapidtop);
	console.log("   rapid plunge                    :", count_rapidplunge);
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
    
    array2D = [];
    xoffset = 0.0;
    yoffset = 0.0;
    global_maxX = -500000.0;
    global_maxY = -500000.0;
    global_maxZ = -500000.0;
    global_maxZlevel = -500000.0;
    global_minX = 5000000.0;
    global_minY = 5000000.0;
    global_minZ = 5000000.0;
    currentx = 0.0;
    currenty = 0.0;
    currentz = 0.0;
    currentf = 0.0;
    currentfout = 0.0;
    diameter = 0.0;
    glevel = '0';
    let stepsize = 1;
    
    console.log("Data is " + lines[0] + " size  " + lines.length);
    
    start = Date.now();
    
    console.log("Start of parsing at " + (Date.now() - start));

    /* first a quick dryrun to find the size of the work*/
    dryrun = 1;    
    for (let i = 0; i < len; i++) {
    	process_line(lines[i]);
    }
    outputtext = "";
    count_retract = 0;
    count_rapidtop = 0;
    count_rapidplunge = 0;
    
    if (global_maxZlevel < 0.04) {
    	global_maxZlevel = global_maxZ;
    }
    
    log_dimensions();
    allocate_2D_array(global_minX, global_minY, global_maxX, global_maxY);
    /* and then for real */
    dryrun = 0;    
    stepsize = Math.round((lines.length + 1)/100);
    if (stepsize < 1)
    	stepsize = 1;
    for (let i = 0; i < len; i += stepsize) {
    	let end = i + stepsize;
    	if (end > lines.length)
    		end = lines.length;
	setTimeout(line_range, 0, lines, i, end);
    }
    
//    for (let i = 0; i < len; i++) {
//    	process_line(lines[i]);
//    }

    setTimeout(log_optimizations, 0);
    console.log("End of parsing at " + (Date.now() - start));
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


function optRetract(value)
{
	if (value == true) {
		optimization_retract = 1;
	}
	if (value == false) {
		optimization_retract = 0;
	}
	console.log("optimization_retract is set to ", optimization_retract);
}

function optRapidTop(value)
{
	if (value == true) {
		optimization_rapidtop = 1;
	}
	if (value == false) {
		optimization_rapidtop = 0;
	}
	console.log("optimization_rapidtop is set to ", optimization_rapidtop);
}

function optRapidPlunge(value)
{
	if (value == true) {
		optimization_rapidplunge = 1;
	}
	if (value == false) {
		optimization_rapidplunge = 0;
	}
	console.log("optimization_rapidplunge is set to ", optimization_rapidplunge);
}

window.optRetract = optRetract;
window.optRapidTop = optRapidTop;
window.optRapidPlunge = optRapidPlunge;
