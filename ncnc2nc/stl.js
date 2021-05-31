/* NOT ACTUAL ABOUT STL files, but actual gcode ;-) */

let zoffset = 0.0;

function inch_to_mm(inch)
{
    return Math.ceil( (inch * 25.4) *1000)/1000;
}

function mm_to_inch(mm)
{
    return Math.ceil(mm / 25.4 * 1000)/1000;
}


let global_minX = 600000000.0;
let global_minY = 600000000.0;
let global_minZ = 600000000.0;
let global_maxX = -600000000.0;
let global_maxY = -600000000.0;
let global_maxZ = -600000000.0;

let orientation = 0;

export function get_work_width()
{
    return global_maxX;
}
export function get_work_height()
{
    return global_maxY;
}
export function get_work_depth()
{
    return global_maxZ;
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

let loglimit = 0;

class Triangle {
  /* creates a triangle straight from binary STL data */
  constructor (X1, Y1, Z1, X2, Y2, Z2, diameter)
  {
    /* basic vertices */
    this.X1 = X1;
    this.X2 = X2;
    this.Y1 = Y1;
    this.Y2 = Y2;
    this.Z1 = Z1;
    this.Z2 = Z2;
    this.radius = diameter / 2;

    /* and the bounding box */
    this.minX = 6000000000.0;
    this.minY = 6000000000.0;
    this.maxX = -6000000000.0;
    this.maxY = -6000000000.0;
    this.minZ = 600000000.0;
    this.maxZ = -600000000.0;
    /* status = 0 -> add to the bucket hierarchy later, status = 1 -> don't add (again) */
    this.status = 0;
    
    
    this.minX = Math.min(X1 - diameter/2, X2 - diameter/2); 
    this.minY = Math.min(Y1 - diameter/2, Y2 - diameter/2); 
    this.minZ = Math.min(Z1, 2); 

    this.maxX = Math.max(X1 + diameter/2, X2 + diameter/2); 
    this.maxY = Math.max(Y1 + diameter/2, Y2 + diameter/2); 
    this.maxZ = Math.max(Z1, 2); 
    
    
    global_minX = Math.min(global_minX, this.minX);
    global_minY = Math.min(global_minY, this.minY);
    global_minZ = Math.min(global_minZ, this.minZ);
    global_maxX = Math.max(global_maxX, this.maxX);
    global_maxY = Math.max(global_maxY, this.maxY);
    global_maxZ = Math.max(global_maxZ, this.maxZ);
  }
  
  /* returns 1 if (X,Y) is within the triangle */
  /* in the math sense, this is true if the three determinants are not of different signs */
  /* a determinant of 0 means on the line, and does not count as opposing sign in any way */
  within_triangle(X, Y)
  {
	let x2 = 0.0;
	let y2 = 0.0;	
	let l = 0.0;
    
	let iX, iY;
	let t1, t2;
	
	/* check the ends first, if we're within the endmill radius we're within */
	if (dist2(this.X1, this.Y1, X, Y) <= this.radius)
		return 1;
	if (dist2(this.X2, this.Y2, X, Y) <= this.radius)
		return 1;
		
	x2 = this.X2-this.X1;
	y2 = this.Y2-this.Y1;

     	t1 = y2 * y2 + x2 * x2;
	t2 = - this.Y1*y2 + y2 * Y - x2 * this.X1 + x2 * X;
     	l = t2/t1;
     
	iX = this.X1 + l * x2;
     	iY = this.Y1 + l * y2;


	if (l >= 0 && l <= 1 && dist2(iX, iY, X, Y) <= this.radius)
		return 1;
     
	return 0;

  }
  /* for (X,Y) inside the triangle (see previous function), calculate the Z of the intersection point */
  calc_Z(X, Y)
  {
	let x2 = 0.0;
	let y2 = 0.0;	
	let l = 0.0;
    
	let iX, iY;
	let t1, t2;
	
	let newZ = 10.0;
	
	
	/* check the ends first, if we're within the endmill radius we're within */
	if (dist2(this.X1, this.Y1, X, Y) <= this.radius)
		newZ = Math.min(newZ, this.Z1);
	if (dist2(this.X2, this.Y2, X, Y) <= this.radius)
		newZ = Math.min(newZ, this.Z2);
		
	if (newZ < 0)
		return newZ;
		
	/* the following is a crime against both geometry and linear algebra but.. it kind of gets close */
	x2 = this.X2-this.X1;
	y2 = this.Y2-this.Y1;

     	t1 = y2 * y2 + x2 * x2;
	t2 = - this.Y1*y2 + y2 * Y - x2 * this.X1 + x2 * X;
     	l = t2/t1;
     
	iX = this.X1 + l * x2;
     	iY = this.Y1 + l * y2;

	if (l >= 0 && l <= 1) {
		let dZ = this.Z2 - this.Z1;
		return this.Z1 + l * dZ;
	}

	if (loglimit ++ < 100)
		console.log("Should not get here");
     
	return 0;
  }

}

let triangles = []
let buckets = []
let l2buckets = []



/*
 * "Bucket" is a collection of nearby triangles that have a joint bounding box.
 * This allows for a peformance optimization, if the desired point is outside of
 * the buckets bounding box, all triangles in the bucket can be skipped */
 
class Bucket {
  constructor (lead_triangle)
  {
    this.minX = global_maxX;
    this.maxX = 0.0;
    this.minY = global_maxY;
    this.maxY = 0.0;
    this.maxZ = 0.0;
    
    this.triangles = []
    this.status = 0;
  }
}

/*
 * An L2Bucket is a "bucket of buckets" to allow whole groups of buckets to
 * be skipped in our searches
 */
class L2Bucket {
  constructor (lead_triangle)
  {
    this.minX = global_maxX;
    this.maxX = 0.0;
    this.minY = global_maxY;
    this.maxY = 0.0;
    this.maxZ = 0.0;
    
    this.buckets = []
    this.status = 0;
  }
}


let BUCKET_SIZE=64;
let BUCKET_SIZE2=32;
/* 
 * Turn a list of triangles into a 2 level hierarchy of buckets/
 *
 * general algorithm: pick the first unassigned triangle. Define a "slop" area around
 * it, and find more triangles to go into the same bucket that are within this slop area, 
 * until the bucket is full.
 */
function make_buckets()
{
	let i;
	let slop = Math.min(Math.max(global_maxX, global_maxY)/100.0, 1.5);
	let maxslop = slop * 2;
	let len = triangles.length
	
	for (i = 0; i < len; i++) {
		let j;
		let reach;
		let Xmax, Ymax, Xmin, Ymin, Zmax;
		let rXmax, rYmax, rXmin, rYmin;
		let bucketptr = 0;
		let bucket;
		if (triangles[i].status > 0)
			continue;

		bucket = new Bucket();
		Xmax = triangles[i].maxX;
		Xmin = triangles[i].minX;
		Ymax = triangles[i].maxY;
		Ymin = triangles[i].minY;

		rXmax = Xmax + slop;
		rYmax = Ymax + slop;
		rXmin = Xmin - slop;
		rYmin = Ymin - slop;

		bucket.triangles.push(triangles[i]);
		triangles[i].status = 1;

		reach = len;
		if (reach > i + 50000)
			reach = i + 50000;
	
		for (j = i + 1; j < reach && bucket.triangles.length < BUCKET_SIZE; j++)	{
			if (triangles[j].status == 0 && triangles[j].maxX <= rXmax && triangles[j].maxY <= rYmax && triangles[j].minY >= rYmin &&  triangles[j].minX >= rXmin) {
				Xmax = Math.max(Xmax, triangles[j].maxX);
				Ymax = Math.max(Ymax, triangles[j].maxY);
				Zmax = Math.max(Zmax, triangles[j].maxZ);
				Xmin = Math.min(Xmin, triangles[j].minX);
				Ymin = Math.min(Ymin, triangles[j].minY);
				bucket.triangles.push(triangles[j]);
				triangles[j].status = 1;				
			}				
		}
                let bucketr = bucket.triangles.length
		if (bucketptr >= BUCKET_SIZE -5)
			slop = slop * 0.9;
		if (bucketptr < BUCKET_SIZE / 8)
			slop = Math.min(slop * 1.1, maxslop);
		if (bucketptr < BUCKET_SIZE / 2)
			slop = Math.min(slop * 1.05, maxslop);

		bucket.minX = Xmin - 0.001; /* subtract a little to cope with rounding */
		bucket.minY = Ymin - 0.001;
		bucket.maxX = Xmax + 0.001;
		bucket.maxY = Ymax + 0.001;
                bucket.maxZ = Zmax;
		buckets.push(bucket);
	}
	console.log("Made " + buckets.length + " buckets\n");
	
	slop = Math.min(Math.max(global_maxX, global_maxY)/20, 8);
	maxslop = slop * 2;

	let nrbuckets = buckets.length
	for (i = 0; i < nrbuckets; i++) {
		let j;
		let Xmax, Ymax, Xmin, Ymin, Zmax;
		let rXmax, rYmax, rXmin, rYmin;
		let bucketptr = 0;
		let l2bucket;

		if (buckets[i].status > 0)
			continue;

		l2bucket = new L2Bucket();
		Xmax = buckets[i].maxX;
		Xmin = buckets[i].minX;
		Ymax = buckets[i].maxY;
		Ymin = buckets[i].minY;

		rXmax = Xmax + slop;
		rYmax = Ymax + slop;
		rXmin = Xmin - slop;
		rYmin = Ymin - slop;

                l2bucket.buckets.push(buckets[i]);
		buckets[i].status = 1;
	
		for (j = i + 1; j < nrbuckets && l2bucket.buckets.length < BUCKET_SIZE2; j++)	{
			if (buckets[j].status == 0 && buckets[j].maxX <= rXmax && buckets[j].maxY <= rYmax && buckets[j].minY >= rYmin &&  buckets[j].minX >= rXmin) {
				Xmax = Math.max(Xmax, buckets[j].maxX);
				Ymax = Math.max(Ymax, buckets[j].maxY);
				Xmin = Math.min(Xmin, buckets[j].minX);
				Ymin = Math.min(Ymin, buckets[j].minY);
				Zmax = Math.max(Zmax, buckets[j].maxZ);
				l2bucket.buckets.push(buckets[j]);
				buckets[j].status = 1;				
			}				
		}

		if (bucketptr >= BUCKET_SIZE2 - 4)
			slop = slop * 0.9;
		if (bucketptr < BUCKET_SIZE2 / 8)
			slop = Math.min(slop * 1.1, maxslop);
		if (bucketptr < BUCKET_SIZE2 / 2)
			slop = Math.min(slop * 1.05, maxslop);

		l2bucket.minX = Xmin;
		l2bucket.minY = Ymin;
		l2bucket.maxX = Xmax;
		l2bucket.maxY = Ymax;
		l2bucket.maxZ = Zmax;
		l2buckets.push(l2bucket);
	}
	console.log("Created " + l2buckets.length + " L2 buckets\n");
	
	/* and garbage collect the rest */
	triangles = [];
}


/* Given an X and Y coordinate, find the height of the STL design.
 * This function operates by traversing the bucket hierarchy 
 * and only inspecting triangles whose lowest bucket bounding box 
 * indicates that there might be a hit.
 *
 * the function takes a "value" optional argument, which is 
 * the lowest value of height that is searched for; this is used
 * for repeated searches for the same tool, so that whole buckets
 * can be skipped if it's known they cannot add to a higher "height"
 *
 * "offset" is an additional offset, used for compensating for tool geometry
 */
export function get_height(X, Y, value = -global_maxZ, offset = 0.0)
{
        value = value + global_maxZ - offset;        

	const l2bl = l2buckets.length;
	
	for (var k = 0; k < l2bl; k++) {
	
            const l2bucket = l2buckets[k];

            if (l2bucket.minX > X)
		        continue;
            if (l2bucket.minY > Y)
                        continue;
            if (l2bucket.maxX < X)
            		continue;
            if (l2bucket.maxY < Y)
                        continue;
       
            if (l2bucket.maxZ <= value)
                    continue;

            const bl = l2buckets[k].buckets.length;
	
            for (let j =0 ; j < bl; j++) {
	        
                if (l2bucket.buckets[j].minX > X)
                    continue;
                if (l2bucket.buckets[j].minY > Y)
                    continue;
                if (l2bucket.buckets[j].maxX < X)
                    continue;
                if (l2bucket.buckets[j].maxY < Y) 
		    continue;
		    

		if (l2bucket.buckets[j].maxZ <= value)
		    continue;

		const bucket = l2bucket.buckets[j];
        	const len = l2bucket.buckets[j].triangles.length;
        	for (let i = 0; i < len; i++) {
        	        let newZ;
        	        //let t = bucket.triangles[i];
	    	

            		// first a few quick bounding box checks 
	        	if (bucket.triangles[i].minX > X)
                            continue;
                        if (bucket.triangles[i].minY > Y)
                            continue;
                        if (bucket.triangles[i].maxX < X)
                            continue;
                        if (bucket.triangles[i].maxY < Y)
                            continue;
                        if (bucket.triangles[i].maxZ <= value)
                            continue;

                        /* then a more expensive detailed triangle test */
                        if (!bucket.triangles[i].within_triangle(X, Y)) {
                                continue;
                        }
                        /* now calculate the Z height within the triangle */
                        newZ = bucket.triangles[i].calc_Z(X, Y);

            		value = Math.min(newZ, value);
                }
            }
        }
	return value;
}

/*
 * Batch version of get_height(), instead of a specific (X,Y) it takes a center point
 * and an array of offset pairs to this center point. In all other ways the function is the
 * same as get_height(). The returned value is the highest point for any of the pairs in the array.
 *
 * Reason for this array version is computational efficiency for the common case of needing
 * the max of a set of nearby points.
 *
 * Only use nearby points, for the general case the computational behavior of this function
 * is dreadful... one ends up looking at all triangles not just close ones
 */
export function get_height_array(minX, minY, maxX, maxY, _X, _Y, arr, value = -global_maxZ, offset = 0.0)
{
        value = value + global_maxZ - offset;        

	const l2bl = l2buckets.length;
	const points = arr.length / 2;
	
	for (var k = 0; k < l2bl; k++) {
	
            const l2bucket = l2buckets[k];

            if (l2bucket.minX > maxX)
		        continue;
            if (l2bucket.minY > maxY)
                        continue;
            if (l2bucket.maxX < minX)
            		continue;
            if (l2bucket.maxY < minY)
                        continue;
       
            if (l2bucket.maxZ <= value)
                    continue;

            const bl = l2buckets[k].buckets.length;
	
            for (let j =0 ; j < bl; j++) {
	        
                if (l2bucket.buckets[j].minX > maxX)
                    continue;
                if (l2bucket.buckets[j].minY > maxY)
                    continue;
                if (l2bucket.buckets[j].maxX < minX)
                    continue;
                if (l2bucket.buckets[j].maxY < minY) 
		    continue;
		    

		if (l2bucket.buckets[j].maxZ <= value)
		    continue;

		const bucket = l2bucket.buckets[j];
        	const len = l2bucket.buckets[j].triangles.length;
        	
        	for (let p = 0; p < points; p++) {
        	    let X = _X + arr[p * 2];
        	    let Y = _Y + arr[p * 2 + 1];
        	    for (let i = 0; i < len; i++) {
        	        let newZ;
        	        //let t = bucket.triangles[i];
	    	

            		// first a few quick bounding box checks 
	        	if (bucket.triangles[i].minX > X)
                            continue;
                        if (bucket.triangles[i].minY > Y)
                            continue;
                        if (bucket.triangles[i].maxX < X)
                            continue;
                        if (bucket.triangles[i].maxY < Y)
                            continue;
                        if (bucket.triangles[i].maxZ <= value)
                            continue;

                        /* then a more expensive detailed triangle test */
                        if (!bucket.triangles[i].within_triangle(X, Y)) {
                                continue;
                        }
                        /* now calculate the Z height within the triangle */
                        newZ = bucket.triangles[i].calc_Z(X, Y);

            		value = Math.max(newZ, value);
                    }
                }
            }
        }
	return value - global_maxZ + offset;
}


let currentx = 0.0;
let currenty = 0.0;
let currentz = 0.0;
let currentf = 0.0;
let currentfout = 0.0;
let diameter = 0.0;

let glevel = '0';
let gout = '';
let metric = 1;


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
 * Handle a "G0" (rapid move) command in the input gcode.
 * Handling involves updating the global bounding 
 * 
 * @param {Number} x	X co-ordinate
 * @param {Number} y	Y co-ordinate
 * @param {Number} z	Z co-ordinate
 * @param {Number} line	Line number in g-code
 */
function G0(x, y, z) 
{
	currentx = x;
	currenty = y;
	currentz = z;
}


/* Handle a "G1" (cutting move) in the gcode input.
 *
 */
 
function dist2(X1, Y1, X2, Y2) 
{
	return Math.sqrt((X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2));
}
function dist3(X1, Y1, Z1, X2, Y2, Z2) 
{
	return Math.sqrt((X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2) +(Z2-Z1)*(Z2-Z1));
}

function doG1(X1, Y1, Z1, X2, Y2, Z2, recurse)
{

	let dX, dY, dZ;
	
	dX = X2-X1;
	dY = Y2-Y1;
	dZ = Z2-Z1;
	
	/* If Z varies too much, we break the G segment into 2 */
	if (Math.abs(dZ) > 0.1 && dist2(X1,Y1,X2,Y2) > 0.01 && recurse < 5) {
		doG1(X1, Y1, Z1, X1 + dX/2, Y1 + dY/2, Z1 + dZ/2, recurse + 1);
		doG1(X1 + dX/2, Y1 + dY/2, Z1 + dZ/2, X2, Y2, Z2, recurse + 1);
		return;
	}

        let T = new Triangle(X1, Y1, Z1, X2, Y2, Z2, diameter);
        triangles.push(T);

}

function G1(x, y, z, feed) 
{
	let l = 0;

	let X1 = currentx;
	let Y1 = currenty;
	let Z1 = currentz;
	let X2 = x;
	let Y2 = y;
	let Z2 = z;
	
	doG1(X1, Y1, Z1, X2, Y2, Z2, 0);
	
	
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
}

function handle_comment(line)
{
	if (line.includes("TOOL/MILL")) {
		let linesplit = line.split(",")
		let diastr = linesplit[1];
		diameter = parseFloat(diastr);
		console.log("Setting Diameter to ", diameter);
	}
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
		case '(':
			handle_comment(line);
			break;
		default:
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
    
    console.log("Start of gcode parsing at " + (Date.now() - start));

    stepsize = Math.round((lines.length + 1)/100);
    if (stepsize < 1)
    	stepsize = 1;
    for (let i = 0; i < len; i += 1) {
            process_line(lines[i]);
    }
    
    console.log("End of parsing at " + (Date.now() - start));
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));
}




/*
 * This function takes the raw data of an STL file, processes this data into ascii or binary triangles
 * and sets up the various data structures (buckets etc) so that no other part of the code really
 * has to know anything about STL... just about triangles that are already organized in a bucket
 * hierarchy.
 */
export function process_data_legacy(data)
{
    let len = data.length;
    
    var start;
    
    start = Date.now();
    
    reset_stl_state();
    
    console.log("Start of parsing at " + (Date.now() - start));
    
    for (let i = 0; i < total_triangles; i++) {
        let T = new Triangle(data, 84 + i * 50);
        triangles.push(T);
    }

    console.log("End of parsing at " + (Date.now() - start));

    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));
}

export function reset_stl_state() {
     global_minX = 600000000.0;
     global_minY = 600000000.0;
     global_minZ = 600000000.0;
     global_maxX = -600000000.0;
     global_maxY = -600000000.0;
     global_maxZ = -600000000.0;
     orientation = 0;
     triangles = [];
     buckets = [];
     l2buckets = [];
}

