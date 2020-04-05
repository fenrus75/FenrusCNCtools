/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/


let xoffset = 0;
let yoffset = 0;
let array_xmax;
let array_ymax;
let global_maxX = 0;
let global_maxY = 0;
let global_minX = 5000000;
let global_minY = 5000000;
let currentx = 0;
let currenty = 0;
let currentz = 0;
let currentf = 0;
let diameter = 0;
let array2D;
let glevel = 0;


let scalefactor = 3.0;

let maxload = 1500;

let imgdata;
let imgwidth = 0;
let imgheight = 0;
let imgcanvas;
let imgpixels;

function update_scalefactor()
{
	let scalex = 1024 / (global_maxX - global_minX);
	let scaley = 1024 / (global_maxY - global_minY);
	
	scalefactor = Math.min(scalex, scaley);
}

function setpixel(x,y, newc, newr)
{
	if (y < 0)
		return;
	if (x < 0)
		return;
	if (x >= imgwidth)
		return;
	if (y >= imgheight)
		return;
	let pos = (x + y * imgwidth) * 4;  
	if (imgdata[pos + 3] == 0) {
		imgdata[pos + 0 ] = newr;
		imgdata[pos + 1 ] = newc;
		imgdata[pos + 2 ] = newc;
		imgdata[pos + 3 ] = 255;
	} else {
		if (imgdata[pos + 1] > newc) {
			imgdata[pos + 0 ] = newr;
			imgdata[pos + 1 ] = newc;
			imgdata[pos + 2 ] = newc;
		imgdata[pos + 3 ] = 255;
		}
	}

}

function declare_point_load(X, Y, load)
{
	x = Math.round((X - global_minX) * scalefactor);
	y = Math.round((Y - global_minY) * scalefactor);
	
	if (x > imgwidth || y > imgheight) {
//		console.log("OOB " + x + " " + y + "\n");
		return;
	}
		
	y = imgheight - y - 1;
		
		
//	if (load > maxload)
//		console.log("Load exceeds " + maxload + " : " + load + " \n");
	
	newc = (load / maxload) ;
	if (newc < 0) newc = 0;
	if (newc > 1) newc = 1;
//	newc = newc * newc;
	newc = (1 - newc) * 255;
	
	newr = newc;
	if (newr < 128) newr = 128;
	
	setpixel(x, y, newc, newr);	
	setpixel(x-1, y, newc, newr);	
	setpixel(x+1, y, newc, newr);	
	setpixel(x, y-1, newc, newr);	
	setpixel(x, y+1, newc, newr);	
}


function allocate_2D_array(minX, minY, maxX, maxY) {
	xoffset = - Math.floor(minX / 5.0);
	yoffset = - Math.floor(minY / 5.0);
	array_xmax = yoffset + Math.ceil(maxX / 5.0) + 4
	array_ymax = yoffset + Math.ceil(maxY / 5.0) + 4
	
	var x, y;
//	console.log("maxX " + maxX + " " + array_xmax + "   maxY " + maxY + " " + array_ymax + "\n");
	
	array2D = new Array(array_ymax + 1)
	for (y = 0; y <= array_ymax; y++) {
		array2D[y] = new Array(array_xmax + 1)
		for (x = 0; x <= array_xmax; x++) {
			array2D[y][x] = [];
		}	
	}
	

	update_scalefactor();
	imgcanvas = document.querySelector('canvas');
	imgcontext = imgcanvas.getContext('2d');	
	imgwidth = Math.ceil(scalefactor * (global_maxX - global_minX));
	imgheight = Math.ceil(scalefactor * (global_maxY - global_minY));
//	console.log("maxX " + maxX + " " + imgwidth + "   maxY " + maxY + " " + imgheight + "\n");
	imgpixels = imgcontext.createImageData(imgwidth, imgheight);
	imgdata = imgpixels.data;
	
      	
}


function Move(fromX, fromY, fromZ, toX, toY, toZ, D) {
	this.X1 = fromX;
	this.Y1 = fromY;
	this.Z1 = fromZ;
	
	this.X2 = toX;
	this.Y2 = toY;
	this.Z2 = toZ;
	
	this.minX = Math.min(this.X1, this.X2) - D/1.99;
	this.minY = Math.min(this.Y1, this.Y2) - D/1.99;
	this.maxX = Math.max(this.X1, this.X2) + D/1.99;
	this.maxY = Math.max(this.Y1, this.Y2) + D/1.99;
	
	this.diameter = D;
}

function record_move(fromX, fromY, fromZ, toX, toY, toZ) 
{
	var move = new Move(fromX, fromY, fromZ, toX, toY, toZ, diameter);
	
	var minx, miny, maxx, maxy;

	
	var limit = 7 + diameter / 2;

	/* https://github.com/svaarala/duktape-wiki/blob/master/Performance.md */
	/* Store frequently accessed values in local variables instead of looking them up from the global object or other objects. */

	var curx = currentx;
	var cury = currenty;
	var xoff = xoffset;
	var yoff = yoffset;
	
	minx = Math.floor((move.minX - limit)/5.0) + xoff;
	miny = Math.floor((move.minY - limit)/5.0) + xoff;
	maxx = Math.ceil((move.maxX + limit)/5.0) + yoff;
	maxy = Math.ceil((move.maxY + limit)/5.0) + yoff;
	
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx > array_xmax) maxx = array_xmax;
	if (maxy > array_ymax) maxy = array_ymax;
	

	for (y = miny; y <= maxy; y++) {
		Y = (y - yoff) * 5.0; 
		for (x = minx; x <= maxx; x++) {
			X = (x - xoff) * 5.0;
			var d = distance_point_from_vector(curx, cury, toX, toY, X, Y, limit);
			if (d <= limit) {
				array2D[y][x].push(move);
			}
		}	
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
 * calculate the distance of a point to a vector to see if it is within "threshold"
 *
 * if after a cheap calculation it's already known to be within "threshold,
 * a more expensive detailed calculation will be skipped.
 */
function distance_point_from_vector(X1, Y1, X2, Y2, pX,  pY, threshold)
{
	var bestD = Math.min(dist(pX,pY,X1,Y1), dist(pX,pY,X2,Y2));
	
	if (bestD < threshold)
		return bestD;
	
	var x2 = 0.0;
	var y2 = 0.0;	
	var l = 0.0;
    
	var iX, iY;
	var t1, t2;
		
	x2 = X2-X1;
	y2 = Y2-Y1;

     	t1 = y2 * y2 + x2 * x2;
	t2 = - Y1*y2 + y2 * pY - x2 * X1 + x2 * pX;
     	l = t2/t1;
     
	iX = X1 + l * x2;
     	iY = Y1 + l * y2;

	if (l >= 0 && l <= 1)
		bestD = Math.min(bestD, dist(pX,pY, iX, iY));
     
	return bestD;
}

var depthcache = -1;

/*
 * returns the depth at a specific X,Y given past cuts.
 * once a cut at "threshold" depth is found, the search is stopped
 * as the objective has been reached
 */
function depth_at_XY(X, Y, threshold)
{
	var depth = 2;
	var m;
	
	
	var ax, ay;
	
	ax = Math.round(X/5.0) + xoffset;
	ay = Math.round(Y/5.0) + yoffset;
	
	if (ax < 0) ax = 0;
	if (ay < 0) ay = 0;
	if (ax >= array_xmax) ax = array_xmax - 1;
	if (ay >= array_ymax) ay = array_ymax - 1;
	
	var arr = array2D[ay][ax];

	/* one shot cache */
	if (depthcache >= 0 && depthcache < array2D[ay][ax].length) {
		m = arr[depthcache];
		var d;
		var r = m.diameter / 2;
		d = distance_point_from_vector(m.X1, m.Y1, m.X2, m.Y2, X, Y, r);		
		if (d <= r && m.Z1 <= threshold) {
			return m.Z1;
		}
	}
	
	var len = arr.length -1;
	
	for (var i = len; i >= 0; i--) {
		m = arr[i];
		var d;


		if (X < m.minX)
			continue;
		if (X > m.maxX)
			continue;
		if (Y < m.minY)
			continue;
		if (Y > m.maxY)
			continue;

		if (depth <= m.Z1)
			continue;

		var r = m.diameter/2;
		d = distance_point_from_vector(m.X1, m.Y1, m.X2, m.Y2, X, Y, r);	

		if (d  > r)
			continue;


		depth = Math.min(depth, m.Z1);
		if (depth <= threshold) {
			depthcache = i;
			return depth;
		}
	}

	if (depth > 0)
		depth = 0;

	return depth;
}

/* calculates cutter engagement at a specific depth, as a ratio of max depth of cut */
function point_load(X, Y, Z)
{
	var Z2;
	var delta;

	Z2 = depth_at_XY(X, Y, Z);

	if (Z2 <= Z) {
		return 0;
	}
	delta = Math.abs(Z2 - Z);
	
	return delta;
}

var sampleY = [0.995,   0.7778, 0.5556, 0.3333, 0.1111, -0.1111, -0.3333, -0.5556, -0.7778, -0.995];
var sampleX = [0.09991, 0.629,  0.832,  0.943,  0.994,   0.994,   0.943,   0.832,   0.629,   0.099911];

/* 
 * calculates the average cutter engagement over a path.
 *
 */

function area_load(X1, Y1, Z1, X2, Y2, Z2, feed)
{
	var vx, vy, vz;
	var nx,ny;
	var len;
	var l = 0, prevl = 0;
	var maxl = dist3(X1,Y1,Z1,X2,Y2,Z2);
	var sum = 0;
	var count = 0;

	var stepsize = diameter / 4;

	if (dist(X1,Y1,X2,Y2)/10 < stepsize)
		stepsize = dist(X1, Y1, X2, Y2) / 10;

	if (stepsize < 0.1)
		stepsize = 0.1;
		
	stepsize = 0.1;

	var R = diameter / 2;

	/* should not happen but would cause math to go NaN */
	if (approx4(X1,X2) && approx4(Y1,Y2)) {
		return 1;
	};

	vx = X2-X1;
	vy = Y2-Y1;
	vz = Z2-Z1;
	len = Math.sqrt(vx*vx+vy*vy+vz*vz);
	vx = vx / len;
	vy = vy / len;
	vz = vz / len;

	nx = -vy;
	ny = vx;
	len = Math.sqrt(nx*nx+ny*ny);
	nx = nx / len;
	ny = ny / len;
	
	var sX = sampleX;
	var sY = sampleY;

	do {
		var i;
		var local = 0;
		var pl0 =  point_load(X1 + (l + R * sX[0]) * vx + R * sY[0] * nx, Y1 + (l + R * sX[0]) * vy + R * sY[0] * ny, Z1  + l * vz);
		var pl9 =  point_load(X1 + (l + R * sX[9]) * vx + R * sY[9] * nx, Y1 + (l + R * sX[9]) * vy + R * sY[9] * ny, Z1  + l * vz);
		local += pl0;
		local += pl9;
		
		/* if both extremes are at full load, shortcut for performance and assume all are */
		if (1) {
			for (i = 1; i < 9; i++) {
				local += point_load(X1 + (l + R * sX[i]) * vx + R * sY[i] * nx, Y1 + (l + R * sX[i]) * vy + R * sY[i] * ny, Z1  + l * vz);
			}
		} else {
			local += 8;
		}
		
		declare_point_load(X1 + l * vx, Y1 + l * vy, local / 10.0 * feed);

		sum += local;
		count += 10;
		
		prevl = l;
		if (l < maxl - stepsize || l == maxl) {
		    l = l + stepsize;
		} else { 
			if (l != maxl)
				l = maxl;
			else
				l = l + stepsize;
		}
	} while (l <= maxl);

	if (count > 0)
		sum = sum / count;

	return sum;
	
}
let dryrun = 1;

let lastmillline = "";

function targetload(depth, ipm)
{
	return 2 * (depth * 25.4) * ipm * 25.4;
}

function tool_change(mline)
{
	let diam = 0;
	let angle = 0;
	let nr = 0;
	let newtarget = 0;
	
	if (lastmillline != "") {
		fields = lastmillline.split(",");
		diam = parseFloat(fields[1]);
		angle = 2 * parseFloat(fields[4]);
	}
	
	mline = mline.replace("M6T","");
	mline = mline.replace("M6 T","");
	mline = mline.replace("T","");
	nr = parseInt(mline);
	
	if (angle > 0) diam = 0; /* vbits */
	
	if (nr == 301) {
		angle = 90;
		diam = 0;
	}
	if (nr == 302) {
		angle = 60;
		diam = 0;
	}
	
	if (nr == 102 || nr == 1) {
		diam = 1.0/8 * 25.4;
		newtarget = targetload(0.03, 45);
	}
	if (nr == 112) {
		diam = 1.0/16 * 25.4;
		newtarget = targetload(0.02, 35);
	}
	if (nr == 201 || nr == 251) {
		diam = 1.0/4 * 25.4;
		newtarget = targetload(0.04, 60);
	}
	
	if (newtarget == 0) { /* unknown -> scale to 1/4" */
		newtarget = targetload(0.04, 60) * diam / (0.25 * 25.4);
	}
	
	
	diameter = diam;
	maxload = newtarget;
	console.log("New tool: " + nr + " diameter " + diameter + " max load " + maxload + "\n");
	
}

function G1(x, y, z, feed) {

	global_maxX = Math.max(global_maxX, x);
	global_maxY = Math.max(global_maxY, y);
	global_minX = Math.min(global_minX, x);
	global_minY = Math.min(global_minY, y);
	
	if (dryrun > 0)
		return;
		
	if (currentz == z && diameter > 0 && glevel == 1) {
		area_load(currentx, currenty, currentz, x, y, z, feed);
		record_move(currentx, currenty, currentz, x, y, z);	
	}
	currentx = x;
	currenty = y;
	currentz = z;
	currentf = feed;
}

function handle_M_line(line)
{
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
	if (line.includes("stockMax:")) {
		l = line.replace("(stockMax:","");
		global_maxX = parseFloat(l);
		let idx = l.indexOf(",");
		l = l.substring(idx + 1);
		global_maxY = parseFloat(l);
//		console.log("new maX/Y " + global_maxX+ " " + global_maxY + "\n");		
	}
	
	if (line.includes("(TOOL/MILL,")) {
		lastmillline = line;
		l = line.replace("(TOOL/MILL,","");
		diameter = parseFloat(l);
	}
	
}

function handle_XYZ_line(line)
{
	let newX, newY, newZ, newF;
	newX = currentx;
	newY = currenty;
	newZ = currentz;
	newF = currentf;
	let idx;
	
	
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
	if (idx >= 0)
		newF = parseFloat(line.substring(idx + 1));	
		
	G1(newX, newY, newZ, newF);
}

function get_phi(rX,rY,aX,aY)
{
	aX -= rX;
	aY -= rY;
	let len = Math.sqrt(aX * aX + aY * aY);
	aX /= len;
	aY /= len;
	let phi = Math.atan2(aY,aX);
	return phi;
}

function handle_arc_line(line)
{
	let newX, newY, newZ, newF;
	let newI, newJ, newK, newRx, newRy;
	newX = currentx;
	newY = currenty;
	newZ = currentz;
	newF = currentf;
	newI = 0;
	newJ = 0;
	newK = 0;
	let idx;
	let clockwise = 1;
	
	let startphi, endphi;
	let deltaphi = 0.01;
	
	if (line.includes("G3")) {
		delphaphi = -0.1;
		clockwise = 0;
	}
	
	
	idx = line.indexOf("X");
	if (idx >= 0)
		newX = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("Y");
	if (idx >= 0)
		newY = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("Z");
	if (idx >= 0)
		newZ = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("I");
	if (idx >= 0)
		newI = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("J");
	if (idx >= 0)
		newJ = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("K");
	if (idx >= 0)
		newK = parseFloat(line.substring(idx + 1));	
	idx = line.indexOf("F")
	if (idx >= 0)
		newF = parseFloat(line.substring(idx + 1));	
		
	newRx = currentx + newI;
	newRy = currenty + newJ;
	
	start_phi = get_phi(newRx, newRy, currentx, currenty);
	end_phi = get_phi(newRx, newRy, newX, newY);	
	
	let radius = dist(newRx, newRy, newX, newY);
	
	if (clockwise) {
	
		if (start_phi < end_phi) {
			start_phi += 2 * 3.1415;
		}
	
		let phi = start_phi;
		while (phi > end_phi) {
			G1(newRx + radius * Math.cos(phi), newRy + radius * Math.sin(phi), newZ, newF);
			phi -= deltaphi;
		}
	} else {
		if (start_phi > end_phi) {
			start_phi -= 2 * 3.1415;
		}
	
		let phi = start_phi;
		while (phi < end_phi) {
			G1(newRx + radius * Math.cos(phi), newRy + radius * Math.sin(phi), newZ, newF);
			phi += deltaphi;
		}
	}
	G1(newX, newY, newZ, newF);	
		
//	console.log("ARC X Y Z I J K " + newX + " " + newY + " " + newZ + " " + newI + " " + newJ + "  phi " + start_phi + " -> " + end_phi + " \n");
//	G1(newX, newY, newZ, newF);
}

function handle_G_line(line)
{
	if (line[1] == '1') glevel = 1;
	if (line[1] == '0') glevel = 0;
	if (line[1] == '1' || line[1] == '0')
		return handle_XYZ_line(line);
		
	if (line[1] == '2' || line[1] == '3')
		if (line[2] != '0' && line[2] != '1')
			return handle_arc_line(line);
}

function process_line(line)
{
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
		
		
		default:
			console.log("Unknown code for line " + line + "\n");
	}
}


function line_range(lines, start, stop)
{
	for (let i = start; i < stop; i++)
		process_line(lines[i]);
		
	imgcontext.putImageData(imgpixels, 0, 0);    

	let elem = document.getElementById("Bar");
        elem.style.width = (Math.round(stop/lines.length * 100)) + "%";
}


function process_data(data)
{
    let lines = data.split("\n")
    let len = lines.length;
    
    var start;
    
    array2D = [];
    xoffset = 0;
    yoffset = 0;
    global_maxX = 0;
    global_maxY = 0;
    global_minX = 5000000;
    global_minY = 5000000;
    currentx = 0;
    currenty = 0;
    currentz = 0;
    currentf = 0;
    diameter = 0;
    glevel = 0;
    let stepsize = 1;
    
    console.log("Data is " + lines[0] + " size  " + lines.length);
    
    start = Date.now();
    
    console.log("Start of parsing at " + (Date.now() - start));

    /* first a quick dryrun to find the size of the work*/
    dryrun = 1;    
    for (let i = 0; i < len; i++) {
    	process_line(lines[i]);
    }
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
    imgcontext.putImageData(imgpixels, 0, 0);    

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

function handle(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
        var reader = new FileReader();
        reader.onloadend = load;
        reader.readAsText(f);
    }
}


document.getElementById('files').onchange = handle;
