/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/


/*
 * GCode stateful tracking. GCode G is a stateful command set, 
 * where we need to track both the level of the G code as well as X Y Z and F
 * since commands we read or write are incremental.
 */
 
let currentxin = 0.0;
let currentyin = 0.0;
let currentzin = 0.0;
let currentfin = 0.0;
let currentxout = 0.0;
let currentyout = 0.0;
let currentzout = 0.0;
let currentfout = 0.0;

let glevel = '0';
let gout = '';
let metric = 1;

let filename = "output";

let dryrun = 1;

let minX = 0;
let minY = 0;
let maxX = 40000000;
let maxY = 40000000;

let boundX1 = 600000000;
let boundY1 = 600000000;
let boundX2 = -6000000000;
let boundY2 = -6000000000;
let boundZ2 = -6000000000;

let tilemaxX = 0;
let tilemaxY = 0;
let tileminX = 0;
let tileminY = 0;

let cutmargin = 4;


function segment_in_box(x1, y1, x2, y2)
{
	if (x1 < tileminX - cutmargin) return 0;
	if (x1 > tilemaxX + cutmargin) return 0;
	if (x2 < tileminX - cutmargin) return 0;
	if (x2 > tilemaxX + cutmargin) return 0;
	if (y1 < tileminY - cutmargin) return 0;
	if (y1 > tilemaxY + cutmargin) return 0;
	if (y2 < tileminY - cutmargin) return 0;
	if (y2 > tilemaxY + cutmargin) return 0;
	return 1;
}



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



function emitG1(glevel, x, y, z, feed) 
{

	let s = "G" + glevel;
	let thisout = s;
	if (gout == thisout)
		s = "";
	if (x != currentxout)
		s += "X" + FtoString(x);
	if (y != currentyout)
		s += "Y" + FtoString(y);
	if (z != currentzout)
		s += "Z" + FtoString(z);
	if (feed != currentfout && thisout != "G0") {
		s += "F" + Math.round(feed);
		currentfout = feed;
	}
	
	if (thisout == "G0")
		currentfout = -1;
	
	gout = thisout;
	
	currentxout = x;
	currentyout = y;
	currentzout = z;

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


let pX1, pX2, pY1, pY2, pZ1, pZ2, pF = 100, pG, pV = 0;


function retract()
{
	emitG1("0", currentxout, currentyout, boundZ2);
}

function flush_buffer()
{
	if (pV > 0) {
		if ( (currentxout != pX1) || (currentyout != pY1) || (currentzout != pZ1)) {
			retract();
			emitG1("0", pX1, pY1, currentzout, pF);
			emitG1("1", pX1, pY1, pZ1, pF);
		}
		emitG1(pG, pX2, pY2, pZ2, pF);
		pV = 0;
	}
}


function bufferG1(glevel, x1, y1, z1, x2, y2, z2, feed)
{
	if (!segment_in_box(x1, y1, x2, y2)) {
		flush_buffer();
		return;
	}

	if (pV > 0) {
		if (pX2 != x1 || pY2 != y1 || pZ2 != z1 || pG != glevel || pF != feed)
			flush_buffer();
	}
	if (pV == 0) {
		pX1 = x1;
		pX2 = x2;
		pY1 = y1;
		pY2 = y2;
		pZ1 = z1;
		pZ2 = z2;
		pF = feed;
		pG = glevel;
		pV = 1;
		return;
	}
	
	pX2 = x2;
	pY2 = y2;
	pZ2 = z2;
//	console.log("Buffer holds (", pX1, ",", pY1, ") -> (", pX2, ",", pY2, ")  PV ", pV);
	pV = 1;
}


function G1(glevel, x, y, z, feed) 
{
	let l = 0;
	let d = dist2(currentxin, currentyin, x, y);
	if (d <= 0.025) {
		flush_buffer();
		bufferG1(glevel, currentxin - minX, currentyin - minY, currentzin, x - minX, y - minY ,z, feed);
		flush_buffer();
		currentxin = x;
		currentyin = y;
		currentzin = z;
		currentfin = feed;
		return;
	}
	let stepsize = 0.025 / d;
	let cX = currentxin;
	let cY = currentyin;
	let cZ = currentzin;
	
	let dX = (x - currentxin);
	let dY = (y - currentyin);
	let dZ = (z - currentzin);
	
	let oldX = cX;
	let oldY = cY;
	let oldZ = cZ;
	
	while (l < 1.0) {
		let newX = cX + dX * l;
		let newY = cY + dY * l;
		let newZ = cZ + dZ * l;
		
		bufferG1(glevel, oldX - minX, oldY - minY, oldZ, newX - minX, newY - minY, newZ, feed);
		oldX = newX;
		oldY = newY;
		oldZ = newZ;
		l = l + stepsize;
	}
	flush_buffer();
	bufferG1(glevel, oldX - minX, oldY - minY, oldZ, x - minX, y - minY, z, feed);
	
	flush_buffer();
	
	currentxin = x;
	currentyin = y;
	currentzin = z;
	currentfin = feed;
}


/*
 * We got a line with X Y Z etc coordinates 
 */
function handle_XYZ_line(line)
{
	let newX, newY, newZ, newF;
	newX = currentxin;
	newY = currentyin;
	newZ = currentzin;
	newF = currentfin;
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
			newF == currentfin;
	}

	if (glevel == '2' || glevel == '3') {
		console.log("ARC DETECTED");
	}
	
	/* G0/G1 we can do clever things with */	
	if (glevel == '1' || glevel == '0')  {
		G1(glevel, newX, newY, newZ, newF, line);
	}
}

function handle_XYZ_line_scan(line)
{
	let newX, newY, newZ, newF;
	newX = currentxin;
	newY = currentyin;
	newZ = currentzin;
	newF = currentfin;
	let idx;
	
	
	/* parse the x/y/z/f */	
	idx = line.indexOf("X");
	if (idx >= 0) {
		newX = to_mm(parseFloat(line.substring(idx + 1)));	
	}
	idx = line.indexOf("Y");
	if (idx >= 0)
		newY = to_mm(parseFloat(line.substring(idx + 1)));	
	idx = line.indexOf("Z");
	if (idx >= 0)
		newZ = parseFloat(line.substring(idx + 1));	
		
	if (newX > boundX2)
		boundX2 = newX;
	if (newX < boundX1)
		boundX1 = newX;
	if (newY > boundY2)
		boundY2 = newY;
	if (newY < boundY1)
		boundY1 = newY;
	if (newZ > boundZ2)
		boundZ2 = newZ;


	currentxin = newX;
	currentyin = newY;
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
		console.log("Using metric system");
	}
		
	if (line[1] == '3') glevel = '3';
	if (line[1] == '2') glevel = '2';
	if (line[1] == '1') glevel = '1';
	if (line[1] == '0') glevel = '0';
	
	if ((line[1] == '1' || line[1] == '0') && (line[2] < '0' || line[2] > '9'))
		return handle_XYZ_line(line);

	/* G line we don't need to process, just pass through */
	currentfin = -1.0;
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
		console.log("Using metric systemk");
	}
		
	if (line[1] == '3') glevel = '3';
	if (line[1] == '2') glevel = '2';
	if (line[1] == '1') glevel = '1';
	if (line[1] == '0') glevel = '0';
	
	if ((line[1] == '1' || line[1] == '0') && (line[2] < '0' || line[2] > '9'))
		return handle_XYZ_line_scan(line);
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


function do_tile(lines)
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
export function process_data(filename, data, tile_width, tile_height, overcut)
{
    let lines = data.split("\n")
    let len = lines.length;
    
    var start;
    
    currentxin = 0.0;
    currentyin = 0.0;
    currentzin = 0.0;
    currentfin = 0.0;
    currentxout = -50000.0;
    currentyout = -50000.0;
    currentzout = -50000.0;
    currentfout = -50000.0;
    
    boundX1 = 0.0;
    boundX2 = -60000000.0;
    boundY1 = 0.0;
    boundY2 = -60000000.0;

    glevel = '0';
    let stepsize = 1;
    
    console.log("Data is " + lines[0] + " size  " + lines.length);
    
    start = Date.now();
    
    console.log("Start of gcode parsing at " + (Date.now() - start));

    outputtext = "";


    for (let i = 0; i < len; i += 1) {
    	process_line_scan(lines[i]);
    }

    console.log("Bounding box: (",boundX1," , ", boundY1, ") x (", boundX2, " , ", boundY2, ")   maxZ ", boundZ2);
    
    let tilesX = Math.ceil(boundX2 / tile_width);
    let tilesY = Math.ceil(boundY2 / tile_height);

    link  = document.getElementById('resultwidth')
    link.innerHTML =  tilesX;
    link  = document.getElementById('resultheight')
    link.innerHTML =  tilesY;
    console.log("Number of tiles: ", tilesX, " x ", tilesY);
    
    for (let tY = 0; tY < tilesY; tY++) {
	    for (let tX = 0; tX < tilesX; tX++) {
	    	minX = (tX * tile_width) - overcut;
	    	maxX = (tX + 1) * tile_width + overcut;
	    	minY = tY * tile_height - overcut;
	    	maxY = (tY + 1) * tile_height + overcut;
	    	
	    	tilemaxX = tile_width + 2 * overcut;
	    	tilemaxY = tile_height + 2 * overcut;
	    	tileminX = 0;
	    	tileminY = 0;
	    	
	    	
		for (let i = 0; i < len; i += stepsize) {
		    process_line(lines[i]);
		}

	        var link = document.getElementById('download_'+(tX+1)+"_"+(tY+1))
	        link.innerHTML = 'save gcode for tile ' + (tX + 1) + ' x ' + (tY + 1) + "<br>";
	        link.href = "#";
	        link.download = filename+"_tile_"+(tX+1)+"_"+(tY+1)+".nc";
	        link.href = "data:text/plain;base64," + btoa(outputtext);
	        var pre = document.getElementById('outputpre')
	        pre.innerHTML = outputtext;
	        console.log("Printing gcode ");
	        outputtext = "";

	   }
    }
    
    console.log("End of parsing at " + (Date.now() - start));
}

