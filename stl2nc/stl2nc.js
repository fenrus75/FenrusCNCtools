/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/

"use strict";

let desired_depth = 12;
let desired_width = 120;
let desired_height = 120;
let zoffset = 0.0;
let safe_retract_height = 2.0;
let rippem = 18000;
let filename = "";
let gui_is_metric = 1;

let finishing_endmill = 27;
let roughing_endmill = 201;

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

function dist3sq(x1,y1,z1,x2,y2,z2)
{
	return ((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)+(z1-z2)*(z1-z2));
}

/* returns 1 if A and B are within 4 digits behind the decimal */
function approx4(A, B) { 
	if (Math.abs(A-B) < 0.0002) 
		return 1; 
	return 0; 
}


function data_f32_to_number(data, offset)
{
    let u32value = (data.charCodeAt(offset + 0)    )  + (data.charCodeAt(offset + 1)<<8) + 
                   (data.charCodeAt(offset + 2)<<16)  + (data.charCodeAt(offset + 3)<<24);     
    let sign = (u32value & 0x80000000)?-1:1;
    let mant = (u32value & 0x7FFFFF);
    let exp  = (u32value >> 23) & 0xFF;
    
//    console.log("sign " + sign + " mant " + mant + " exp " + exp + "\n");
//    console.log("u32value " + u32value.toString(16));
    
    let value = 0.0;
    switch (exp) {
        case 0:
            break;
        case 0xFF:
            value = NaN;
            break;
        default:
            exp = exp - 127; /* minus the bias */
            mant += 0x800000; /* silent leading 1 */
            value = sign * (mant / 8388608.0) * Math.pow(2, exp);
            break;
            
    }
    return value;
}


let global_minX = 600000000;
let global_minY = 600000000;
let global_minZ = 600000000;
let global_maxX = -600000000;
let global_maxY = -600000000;
let global_maxZ = -600000000;

let orientation = 0;

class Triangle {
  constructor (data, offset)
  {
    this.vertex = new Array(3);
    this.vertex[0] = new Array(3);
    this.vertex[1] = new Array(3);
    this.vertex[2] = new Array(3);
    this.minX = 6000000000.0;
    this.minY = 6000000000.0;
    this.maxX = -6000000000.0;
    this.maxY = -6000000000.0;
    this.minZ = 600000000.0;
    this.maxZ = -600000000.0;
    this.status = 0;
    
    this.vertex[0][0] = data_f32_to_number(data, offset + 12);
    this.vertex[0][1] = data_f32_to_number(data, offset + 16);
    this.vertex[0][2] = data_f32_to_number(data, offset + 20);

    this.vertex[1][0] = data_f32_to_number(data, offset + 24);
    this.vertex[1][1] = data_f32_to_number(data, offset + 28);
    this.vertex[1][2] = data_f32_to_number(data, offset + 32);

    this.vertex[2][0] = data_f32_to_number(data, offset + 36);
    this.vertex[2][1] = data_f32_to_number(data, offset + 40);
    this.vertex[2][2] = data_f32_to_number(data, offset + 44);
    
    if (orientation == 1) {
        let x,y,z;
        for (let i = 0; i < 3; i++) {
            x = this.vertex[i][0];
            y = this.vertex[i][1];
            z = this.vertex[i][2];
            this.vertex[i][0] = x;
            this.vertex[i][1] = -z;
            this.vertex[i][2] = y;
        }
    }
    if (orientation == 2) {
        let x,y,z;
        for (let i = 0; i < 3; i++) {
            x = this.vertex[i][0];
            y = this.vertex[i][1];
            z = this.vertex[i][2];
            this.vertex[i][0] = y;
            this.vertex[i][1] = -z;
            this.vertex[i][2] = -x;
        }
    }
   
    this.minX = Math.min(this.vertex[0][0], this.vertex[1][0]); 
    this.minX = Math.min(this.minX,         this.vertex[2][0]); 
    
    this.minY = Math.min(this.vertex[0][1], this.vertex[1][1]); 
    this.minY = Math.min(this.minY,         this.vertex[2][1]); 

    this.minZ = Math.min(this.vertex[0][2], this.vertex[1][2]); 
    this.minZ = Math.min(this.minZ,         this.vertex[2][2]); 
    
    this.maxX = Math.max(this.vertex[0][0], this.vertex[1][0]); 
    this.maxX = Math.max(this.maxX,         this.vertex[2][0]); 
    
    this.maxY = Math.max(this.vertex[0][1], this.vertex[1][1]); 
    this.maxY = Math.max(this.maxY,         this.vertex[2][1]); 
    
    this.maxZ = Math.max(this.vertex[0][2], this.vertex[1][2]); 
    this.maxZ = Math.max(this.maxZ,         this.vertex[2][2]); 
    
    global_minX = Math.min(global_minX, this.minX);
    global_minY = Math.min(global_minY, this.minY);
    global_minZ = Math.min(global_minZ, this.minZ);
    global_maxX = Math.max(global_maxX, this.maxX);
    global_maxY = Math.max(global_maxY, this.maxY);
    global_maxZ = Math.max(global_maxZ, this.maxZ);
  }
  within_triangle(X, Y)
  {
  
 /*
        if (this.minX > X)
            return 0;
         
        if (this.minY > Y)
            return 0;
        if (this.maxX < X)
            return 0;
        if (this.maxY < Y)
            return 0;
*/
	let det1, det2, det3;

	let has_pos = 0, has_neg = 0;
	
	det1 = point_to_the_left(X, Y, this.vertex[0][0], this.vertex[0][1], this.vertex[1][0], this.vertex[1][1]);
	det2 = point_to_the_left(X, Y, this.vertex[1][0], this.vertex[1][1], this.vertex[2][0], this.vertex[2][1]);
	det3 = point_to_the_left(X, Y, this.vertex[2][0], this.vertex[2][1], this.vertex[0][0], this.vertex[0][1]);

//	has_neg = (det1 < 0) || (det2 < 0) || (det3 < 0);
        if (det1 < 0) { has_neg = 1; };
        if (det2 < 0) { has_neg = 1; };
        if (det3 < 0) { has_neg = 1; };
//	has_pos = (det1 > 0) || (det2 > 0) || (det3 > 0);
        if (det1 > 0) { has_pos = 1; };
        if (det2 > 0) { has_pos = 1; };
        if (det3 > 0) { has_pos = 1; };
        
        if (has_neg && has_pos)
            return 0;
        return 1;

	return !(has_neg && has_pos);
  }
   calc_Z(X, Y)
  {
	let det = (this.vertex[1][1] - this.vertex[2][1]) * 
		    (this.vertex[0][0] - this.vertex[2][0]) + 
                    (this.vertex[2][0] - this.vertex[1][0]) * 
		    (this.vertex[0][1] - this.vertex[2][1]);

	let l1 = ((this.vertex[1][1] - this.vertex[2][1]) * (X - this.vertex[2][0]) + (this.vertex[2][0] - this.vertex[1][0]) * (Y - this.vertex[2][1])) / det;
	let l2 = ((this.vertex[2][1] - this.vertex[0][1]) * (X - this.vertex[2][0]) + (this.vertex[0][0] - this.vertex[2][0]) * (Y - this.vertex[2][1])) / det;
	let l3 = 1.0 - l1 - l2;

	return l1 * this.vertex[0][2] + l2 * this.vertex[1][2] + l3 * this.vertex[2][2];
  }

}

class Triangle_Ascii
{
  constructor (line1, line2, line3, line4, line5, line6, line7)
  {
    this.vertex = new Array(3);
    this.vertex[0] = new Array(3);
    this.vertex[1] = new Array(3);
    this.vertex[2] = new Array(3);
    this.minX = 6000000000.0;
    this.minY = 6000000000.0;
    this.maxX = -6000000000.0;
    this.maxY = -6000000000.0;
    this.status = 0;
    
    line3 = line3.trim();
    line4 = line4.trim();
    line5 = line5.trim();
    
    let split3 = line3.split(" ");
    let split4 = line4.split(" ");
    let split5 = line5.split(" ");
    
    this.vertex[0][0] = parseFloat(split3[1]);
    this.vertex[0][1] = parseFloat(split3[2]);
    this.vertex[0][2] = parseFloat(split3[3]);

    this.vertex[1][0] = parseFloat(split4[1]);
    this.vertex[1][1] = parseFloat(split4[2]);
    this.vertex[1][2] = parseFloat(split4[3]);

    this.vertex[2][0] = parseFloat(split5[1]);
    this.vertex[2][1] = parseFloat(split5[2]);
    this.vertex[2][2] = parseFloat(split5[3]);

    
    if (orientation == 1) {
        let x,y,z;
        for (let i = 0; i < 3; i++) {
            x = this.vertex[i][0];
            y = this.vertex[i][1];
            z = this.vertex[i][2];
            this.vertex[i][0] = x;
            this.vertex[i][1] = -z;
            this.vertex[i][2] = y;
        }
    }
    if (orientation == 2) {
        let x,y,z;
        for (let i = 0; i < 3; i++) {
            x = this.vertex[i][0];
            y = this.vertex[i][1];
            z = this.vertex[i][2];
            this.vertex[i][0] = y;
            this.vertex[i][1] = -z;
            this.vertex[i][2] = -x;
        }
    }
   
    this.minX = Math.min(this.vertex[0][0], this.vertex[1][0]); 
    this.minX = Math.min(this.minX,         this.vertex[2][0]); 
    
    this.minY = Math.min(this.vertex[0][1], this.vertex[1][1]); 
    this.minY = Math.min(this.minY,         this.vertex[2][1]); 

    this.minZ = Math.min(this.vertex[0][2], this.vertex[1][2]); 
    this.minZ = Math.min(this.minZ,         this.vertex[2][2]); 
    
    this.maxX = Math.max(this.vertex[0][0], this.vertex[1][0]); 
    this.maxX = Math.max(this.maxX,         this.vertex[2][0]); 
    
    this.maxY = Math.max(this.vertex[0][1], this.vertex[1][1]); 
    this.maxY = Math.max(this.maxY,         this.vertex[2][1]); 
    
    this.maxZ = Math.max(this.vertex[0][2], this.vertex[1][2]); 
    this.maxZ = Math.max(this.maxZ,         this.vertex[2][2]); 
    
    global_minX = Math.min(global_minX, this.minX);
    global_minY = Math.min(global_minY, this.minY);
    global_minZ = Math.min(global_minZ, this.minZ);
    global_maxX = Math.max(global_maxX, this.maxX);
    global_maxY = Math.max(global_maxY, this.maxY);
    global_maxZ = Math.max(global_maxZ, this.maxZ);
  } 
  within_triangle(X, Y)
  {
	let det1, det2, det3;

	let has_pos = 0, has_neg = 0;
	
	det1 = point_to_the_left(X, Y, this.vertex[0][0], this.vertex[0][1], this.vertex[1][0], this.vertex[1][1]);
	det2 = point_to_the_left(X, Y, this.vertex[1][0], this.vertex[1][1], this.vertex[2][0], this.vertex[2][1]);
	det3 = point_to_the_left(X, Y, this.vertex[2][0], this.vertex[2][1], this.vertex[0][0], this.vertex[0][1]);

//	has_neg = (det1 < 0) || (det2 < 0) || (det3 < 0);
        if (det1 < 0) { has_neg = 1; };
        if (det2 < 0) { has_neg = 1; };
        if (det3 < 0) { has_neg = 1; };
//	has_pos = (det1 > 0) || (det2 > 0) || (det3 > 0);
        if (det1 > 0) { has_pos = 1; };
        if (det2 > 0) { has_pos = 1; };
        if (det3 > 0) { has_pos = 1; };
        
        if (has_neg && has_pos)
            return 0;
        return 1;

	return !(has_neg && has_pos);
  }
  calc_Z(X, Y)
  {
	let det = (this.vertex[1][1] - this.vertex[2][1]) * 
		    (this.vertex[0][0] - this.vertex[2][0]) + 
                    (this.vertex[2][0] - this.vertex[1][0]) * 
		    (this.vertex[0][1] - this.vertex[2][1]);

	let l1 = ((this.vertex[1][1] - this.vertex[2][1]) * (X - this.vertex[2][0]) + (this.vertex[2][0] - this.vertex[1][0]) * (Y - this.vertex[2][1])) / det;
	let l2 = ((this.vertex[2][1] - this.vertex[0][1]) * (X - this.vertex[2][0]) + (this.vertex[0][0] - this.vertex[2][0]) * (Y - this.vertex[2][1])) / det;
	let l3 = 1.0 - l1 - l2;

	return l1 * this.vertex[0][2] + l2 * this.vertex[1][2] + l3 * this.vertex[2][2];
  }
}


let triangles = []
let buckets = []
let l2buckets = []

function normalize_design_to_zero()
{	
    let len = triangles.length;
    
    for (let i = 0; i < len; i++) {
        triangles[i].vertex[0][0] -= global_minX;
        triangles[i].vertex[1][0] -= global_minX;
        triangles[i].vertex[2][0] -= global_minX;

        triangles[i].vertex[0][1] -= global_minY;
        triangles[i].vertex[1][1] -= global_minY;
        triangles[i].vertex[2][1] -= global_minY;


        triangles[i].vertex[0][2] -= global_minZ;
        triangles[i].vertex[1][2] -= global_minZ;
        triangles[i].vertex[2][2] -= global_minZ;
    }

    global_maxX -= global_minX;    
    global_maxY -= global_minY;   
    global_maxZ -= global_minZ;    
    global_minX = 0.0;
    global_minY = 0.0;
    global_minZ = 0.0;
}

function scale_design(desired_depth)
{	
    let len = triangles.length;
    
    let factor = 1;
    
    normalize_design_to_zero();
    
    let factorD = desired_depth / global_maxZ; 
    let factorW = desired_width / global_maxX; 
    let factorH = desired_height / global_maxY; 
    
    factor = Math.min(factorD, Math.min(factorW, factorH));
    
    
    for (let i = 0; i < len; i++) {
        triangles[i].vertex[0][0] *= factor;
        triangles[i].vertex[0][1] *= factor;
        triangles[i].vertex[0][2] *= factor;

        triangles[i].vertex[1][0] *= factor;
        triangles[i].vertex[1][1] *= factor;
        triangles[i].vertex[1][2] *= factor;

        triangles[i].vertex[2][0] *= factor;
        triangles[i].vertex[2][1] *= factor;
        triangles[i].vertex[2][2] *= factor;

        triangles[i].minX = Math.min(triangles[i].vertex[0][0], triangles[i].vertex[1][0]); 
        triangles[i].minX = Math.min(triangles[i].minX, 	triangles[i].vertex[2][0]); 
        triangles[i].minY = Math.min(triangles[i].vertex[0][1], triangles[i].vertex[1][1]); 
        triangles[i].minY = Math.min(triangles[i].minY, 	triangles[i].vertex[2][1]); 
        triangles[i].minZ = Math.min(triangles[i].vertex[0][2], triangles[i].vertex[1][2]); 
        triangles[i].minZ = Math.min(triangles[i].minZ, 	triangles[i].vertex[2][2]); 
    
        triangles[i].maxX = Math.max(triangles[i].vertex[0][0], triangles[i].vertex[1][0]); 
        triangles[i].maxX = Math.max(triangles[i].maxX, 	triangles[i].vertex[2][0]); 
        triangles[i].maxY = Math.max(triangles[i].vertex[0][1], triangles[i].vertex[1][1]); 
        triangles[i].maxY = Math.max(triangles[i].maxY, 	triangles[i].vertex[2][1]); 
        triangles[i].maxZ = Math.max(triangles[i].vertex[0][2], triangles[i].vertex[1][2]); 
        triangles[i].maxZ = Math.max(triangles[i].maxZ, 	triangles[i].vertex[2][2]); 
    }

    global_maxX *= factor;    
    global_maxY *= factor;   
    global_maxZ *= factor;    
}

function  point_to_the_left(X, Y, AX, AY, BX, BY)
{
	return (BX-AX)*(Y-AY) - (BY-AY)*(X-AX);
}


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
function make_buckets()
{
	let i;
	let slop = Math.min(Math.max(global_maxX, global_maxY)/100, 1);
	let maxslop = slop * 2;
	let len = triangles.length
	
	console.log("Slop is ", slop);

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
	console.log("L2 Slop is ", slop);
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
	
}

function get_height(X, Y, value = -global_maxZ, offset = 0.0)
{
        value = value + global_maxZ - offset;        

	let l2bl = l2buckets.length;
	
	for (let k = 0; k < l2bl; k++) {
	
            let l2bucket = l2buckets[k];

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

            let bl = l2buckets[k].buckets.length;
	
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

		let bucket = l2bucket.buckets[j];
        	let len = l2bucket.buckets[j].triangles.length;
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
	return value - global_maxZ + offset;
}

function get_heighest_point(X, Y, radius, value = -global_maxZ)
{
        value += global_maxZ;
        
        let minX = X - radius;
        let maxX = X + radius;
        let minY = Y - radius;
        let maxY = Y + radius;
//        let value = 0.0;
	let l2bl = l2buckets.length;
	
	for (let k = 0; k < l2bl; k++) {
	
            let l2bucket = l2buckets[k];

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

            let bl = l2buckets[k].buckets.length;
	
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

		let bucket = l2bucket.buckets[j];
        	let len = l2bucket.buckets[j].triangles.length;
        	for (let i = 0; i < len; i++) {
        	        let newZ;
        	        //let t = bucket.triangles[i];

                        let bt = bucket.triangles[i];

            		// first a few quick bounding box checks 
	        	if (bt.minX > maxX)
                            continue;
                        if (bt.minY > maxY)
                            continue;
                        if (bt.maxX < minX)
                            continue;
                        if (bt.maxY < minY)
                            continue;
                        if (bt.maxZ <= value)
                            continue;
                            
                        
                            
                        for (let q = 0; q < 3; q++) {
                            /* check if this point of the triangle is the heighest */
                            if (bt.maxZ == bt.vertex[q][2]) {
                                let active_R = dist(X, Y, bt.vertex[q][0], bt.vertex[q][1]);
                                if (active_R <= radius) {
                                    let offset =  -geometry_at_distance(active_R);
                                    if (value < bt.maxZ + offset) {
                                        value = bt.maxZ + offset;
                                        cache_prev_X = bt.vertex[q][0];
                                        cache_prev_Y = bt.vertex[q][1];
                                    }
                                }
                            }
                        }                        
                }
            }
        }
	return value - global_maxZ;
}



function process_data(data)
{
    let len = data.length;
    
    var start;
    
    start = Date.now();
    
    triangles = [];
    buckets = [];
    l2buckets = [];
    global_minX = 600000000;
    global_minY = 600000000;
    global_minZ = 600000000;
    global_maxX = -600000000;
    global_maxY = -600000000;
    global_maxZ = -600000000;
    
    if (len < 84) {
        document.getElementById('list').innerHTML = "STL file too short";
        return; 
    }
    
    if (data[0] == 's' && data[1] == 'o' && data[2] == 'l' && data[3] == 'i' && data[4] == 'd') {
        console.log("ASCII STL detected");
        return process_data_ascii(data);
    }
    
    let total_triangles = (data.charCodeAt(80)) + (data.charCodeAt(81)<<8) + (data.charCodeAt(82)<<16) + (data.charCodeAt(83)<<24); 
    
    if (84 + total_triangles * 50  != data.length) {
        document.getElementById('list').innerHTML  = "Length mismatch " + data.length + " " + total_triangles;
        return;
    }
//    document.getElementById('list').innerHTML  = "Parsing STL file";
    
    console.log("Start of parsing at " + (Date.now() - start));
    
    for (let i = 0; i < total_triangles; i++) {
        let T = new Triangle(data, 84 + i * 50);
        triangles.push(T);
    }

    console.log("End of parsing at " + (Date.now() - start));

    scale_design(desired_depth);    
    update_gui_actuals();	
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));

    console.log("Scale " + (Date.now() - start));
    
//    document.getElementById('list').innerHTML  = "Number of triangles " + total_triangles + "mX " + global_maxX + " mY " + global_maxY;
}

function process_data_ascii(data)
{
    let len = data.length;

    let lines = data.split('\n');    
    var start;
    
    start = Date.now();
    
    triangles = [];
    buckets = [];
    l2buckets = [];
    global_minX = 600000000;
    global_minY = 600000000;
    global_minZ = 600000000;
    global_maxX = -600000000;
    global_maxY = -600000000;
    global_maxZ = -600000000;
    
    if (len < 84) {
        document.getElementById('list').innerHTML = "STL file too short";
        return; 
    }
    
    console.log("Start of parsing at " + (Date.now() - start));
    
    let lineslen = lines.length;
    console.log("Total lines count " + lineslen);
    
    for (let i = 1; i < lineslen + 6; i+= 7) {
        let line1 = lines[i];
        let line2 = lines[i + 1];
        let line3 = lines[i + 2];
        let line4 = lines[i + 3];
        let line5 = lines[i + 4];
        let line6 = lines[i + 5];
        let line7 = lines[i + 6];
        if (typeof line1 === 'undefined')
            continue;
        if (typeof line2 === 'undefined')
            continue;
        if (typeof line3 === 'undefined')
            continue;
        if (typeof line4 === 'undefined')
            continue;
        if (typeof line5 === 'undefined')
            continue;
        if (typeof line6 === 'undefined')
            continue;
        if (typeof line7 === 'undefined')
            continue;
        let T = new Triangle_Ascii(line1, line2, line3, line4, line5, line6, line7);
        triangles.push(T);
    }

    console.log("End of parsing at " + (Date.now() - start));

    scale_design(desired_resolution);    
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));

    console.log("Scale " + (Date.now() - start));
    
//    document.getElementById('list').innerHTML  = "Number of triangles " + total_triangles + "mX " + global_maxX + " mY " + global_maxY;
}

function inch_to_mm(inch)
{
    return Math.ceil( (inch * 25.4) *1000)/1000;
}

function mm_to_inch(mm)
{
    return Math.ceil(mm / 25.4 * 1000)/1000;
}


let tool_diameter = inch_to_mm(0.25);
let tool_feedrate = 0.0;
let tool_plungerate = 0.0;
let tool_geometry = "";
let tool_name = "";
let tool_nr = 0;
let tool_depth_of_cut = 0.1;
let tool_stock_to_leave = 0.5;


let gcode_string = "";

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

function gcode_float2str(value)
{
    return value.toFixed(4);
}

function gcode_header()
{
    gcode_write("%");
    gcode_write("G21"); /* milimeters not imperials */
    gcode_write("G90"); /* all relative to work piece zero */
    gcode_write("G0X0Y0Z" + gcode_float2str(safe_retract_height));
    gcode_cZ = safe_retract_height;
    gcode_comment("FILENAME: " + filename);
}

function gcode_footer()
{
    gcode_write("M5");
    gcode_write("M30");
    gcode_comment("END");
    gcode_write("%");
}

let gcode_first_toolchange = 1;

let gcode_retract_count = 0;
function gcode_retract()
{
    gcode_write("G0Z" + gcode_float2str( safe_retract_height));
    gcode_cZ = safe_retract_height;
    gcode_retract_count += 1;
}

function gcode_travel_to(X, Y)
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

function toolspeed3d(cX, cY, cZ, X, Y, Z)
{
	let horiz = dist(cX, cY, X, Y);
	let d = dist3(cX, cY, cZ, X, Y, Z);
	let vert = cZ - Z;
	let time_horiz, time_vert;

	time_horiz = horiz / tool_feedrate;

	/* if we're milling up, feedrate dominates by definition */
	if (vert <= 0) {
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

function gcode_mill_to_3D(X, Y, Z)
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


function gcode_write_toolchange()
{
    if (gcode_first_toolchange == 0) {
        gcode_retract();
        gcode_write("M5");
    }
    gcode_write("M6 T" + tool_name);
    gcode_write("M3 S" + rippem.toString());
    gcode_write("G0 X0Y0");
    gcode_cX = 0.0;
    gcode_cY = 0.0;
    gcode_cF = -1.0;
    gcode_retract();
    gcode_first_toolchange = 0;    
}

function gcode_select_tool(toolnr)
{
    /* reset some of the cached variables */
    gcode_cF = -1; 
    cache_prev_X = -50000;
    cache_prev_Y = -50000;
    /* TODO: NEED TOOL DATABASE */
    if (toolnr == 201) {
        tool_diameter = inch_to_mm(0.25);
        tool_feedrate = inch_to_mm(50);
        tool_plungerate = inch_to_mm(10);
        tool_geometry = "flat"
        tool_nr = toolnr;
        tool_name = toolnr.toString()
        tool_depth_of_cut = inch_to_mm(0.039);
        tool_stock_to_leave = 0.5;
        return;
    }    
    if (toolnr == 101) {
        tool_diameter = inch_to_mm(0.125);
        tool_feedrate = inch_to_mm(30);
        tool_plungerate = inch_to_mm(10);
        tool_geometry = "ball"
        tool_nr = toolnr;
        tool_name = toolnr.toString()
        tool_depth_of_cut = inch_to_mm(0.039);
        tool_stock_to_leave = 0.25;
        return;
    }    
    if (toolnr == 102) {
        tool_diameter = inch_to_mm(0.125);
        tool_feedrate = inch_to_mm(30);
        tool_plungerate = inch_to_mm(10);
        tool_geometry = "flat"
        tool_nr = toolnr;
        tool_name = toolnr.toString()
        tool_depth_of_cut = inch_to_mm(0.039);
        tool_stock_to_leave = 0.25;
        return;
    }    
    if (toolnr == 27) {
        tool_diameter = 1;
        tool_feedrate = inch_to_mm(30);
        tool_plungerate = inch_to_mm(10);
        tool_geometry = "ball"
        tool_nr = toolnr;
        tool_name = toolnr.toString()
        tool_stock_to_leave = 0.0;
        return;
    }    
    if (toolnr == 18) {
        tool_diameter = 1;
        tool_feedrate = inch_to_mm(20);
        tool_plungerate = inch_to_mm(10);
        tool_geometry = "flatl"
        tool_nr = toolnr;
        tool_name = toolnr.toString()
        tool_stock_to_leave = 0.1;
        return;
    }    
    console.log("UNKNOWN TOOL");    
}

function gcode_change_tool(toolnr)
{
    gcode_select_tool(toolnr);
    gcode_write_toolchange();
}

class Segment {
  constructor()
  {
    this.X1 = -1.0;
    this.Y1 = -1.0;
    this.Z1 = -1.0;
    this.X2 = -1.0;
    this.Y2 = -1.0;
    this.Z2 = -1.0;
    this.direct_mill_distance = 0.0001;
  }
}


let levels = [];


class Level {
  constructor(tool_number)
  {
    this.tool = tool_number;
    this.paths = [];
  }
}

function push_segment(X1, Y1, Z1, X2, Y2, Z2, level = 0.0, direct_mill = 0.0001)
{
    if (X1 == X2 && Y1 == Y2 && Z1 == Z2) {
        return;
    }

    for (let lev = 0; lev <= level; lev++) {
        if (typeof(levels[lev]) == "undefined") {
            levels[lev] = new Level();
            levels[lev].tool = tool_nr;
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

        if (prev.Y1== Y1 && prev.Z1 == prev.Z2 && Z1 == Z2 && prev.X2 == X1 && prev.Z1 == Z1 && Y1 == Y2) {
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
    
    levels[level].paths.push(seg);
}

function push_segment_multilevel(X1, Y1, Z1, X2, Y2, Z2, direct_mill = 0.0001)
{
    let z1 = Z1;
    let z2 = Z2;
    let l = 0;
    let mult = 0.5;
    let divider = 1/tool_depth_of_cut;
    
    let total_buckets = Math.ceil(global_maxZ / (tool_depth_of_cut/2)) + 2
    
    if (X1 == X2 && Y1 == Y2 && Z1 == Z2) {
        return;
    }
    
//    let jump = Math.abs(Math.ceil((global_maxZ + Math.min(Z1,Z2)) / tool_depth_of_cut));
//    console.log("Jump is ", jump, "Z1 = ",Z1," Z2 = ", Z2);
    
//    console.log("X1 ", X1, " Y1 ", Y1, " Z1 ", Z1, " X2 ", X2, " Y2 ", Y2, " Z2 ", Z2);
            
    while (z1 < 0 || z2 < 0) {
        if (l != 0) {
            let dZ = -Math.min(z1, z2) / (tool_depth_of_cut/2);
            dZ = Math.round(dZ);
            l = total_buckets - dZ;
        }
        push_segment(X1, Y1, z1, X2, Y2, z2, l, direct_mill);
        z1 = Math.ceil( (z1 + mult * tool_depth_of_cut) * divider) / divider;
        z2 = Math.ceil( (z2 + mult * tool_depth_of_cut) * divider) / divider;
        
        /* deal with rounding artifacts/boundary conditions elsewhere */
        if (z1 < -tool_depth_of_cut/2) {
            z1 = z1 - 0.001;
        }
        if (z2 < -tool_depth_of_cut/2) {
            z2 = z2 - 0.001;
        }
        z1 = Math.max(z1, z2);
        z2 = Math.max(z1, z2);
        l = l + 1;
        mult = 1.0;
    }         
}


let ACC = 100.0;

function geometry_at_distance(R)
{
    if (tool_geometry == "ball") {
        let orgR = tool_diameter / 2;
	return orgR - Math.sqrt(orgR*orgR - R*R);
    }
    
    return 0;
}

let cache_prev_X = -5000.0;
let cache_prev_Y = -5000.0;

function update_height(height, X, Y, offset)
{

    let prevheight = height;
    let newheight = get_height(X, Y, height, offset) + offset;
    
    if (newheight > prevheight) {
        cache_prev_X = X;
        cache_prev_Y = Y;
        height = newheight;
    }

    return height;


//    return Math.max(height, get_height(X, Y) + offset);

}

function get_height_tool(X, Y, R)
{	
	let d = -global_maxZ, dorg;
	let balloffset = 0.0;
	let r;
	


	/* we track the previous heighest point and make sure we check that early */	
	/*
	r = dist(X, Y, cache_prev_X, cache_prev_Y)
	if (r <= R) {
        	balloffset = -geometry_at_distance(r);
        	d = update_height(d, cache_prev_X, cache_prev_Y, balloffset);
        }
        */
        /* get_heighest_point takes endmill geometry offset into account already */	
//	d = get_heighest_point(X, Y, R, d);

	d = update_height(d, X + 0.0000 * R, Y + 0.0000 * R, 0);
 
	balloffset = -geometry_at_distance(R);

	d = update_height(d, X + 1.0000 * R, Y + 0.0000 * R,  balloffset);
	d = update_height(d, X + 0.0000 * R, Y + 1.0000 * R,  balloffset);
	d = update_height(d, X - 1.0000 * R, Y + 0.0000 * R,  balloffset);
	d = update_height(d, X - 0.0000 * R, Y - 1.0000 * R,  balloffset);

	dorg = d;
	d = update_height(d, X + 0.7071 * R, Y + 0.7071 * R,  balloffset);
	d = update_height(d, X - 0.7071 * R, Y + 0.7071 * R,  balloffset);
	d = update_height(d, X - 0.7071 * R, Y - 0.7071 * R,  balloffset);
	d = update_height(d, X + 0.7071 * R, Y - 0.7071 * R,  balloffset);

	if (R < 0.6 && Math.abs(d-dorg) < 0.1)
		return Math.ceil(d*ACC)/ACC;

	d = update_height(d, X + 0.9239 * R, Y + 0.3827 * R,  balloffset);
	d = update_height(d, X + 0.3827 * R, Y + 0.9239 * R,  balloffset);
	d = update_height(d, X - 0.3872 * R, Y + 0.9239 * R,  balloffset);
	d = update_height(d, X - 0.9239 * R, Y + 0.3827 * R,  balloffset);
	d = update_height(d, X - 0.9239 * R, Y - 0.3827 * R,  balloffset);
	d = update_height(d, X - 0.3827 * R, Y - 0.9239 * R,  balloffset);
	d = update_height(d, X + 0.3827 * R, Y - 0.9239 * R,  balloffset);
	d = update_height(d, X + 0.9239 * R, Y - 0.3827 * R,  balloffset);

	R = R / 1.5;

	if (R < 0.4)
		return Math.ceil(d*ACC)/ACC;

	balloffset = -geometry_at_distance(R);

	d = update_height(d, X + 1.0000 * R, Y + 0.0000 * R,  balloffset);
	d = update_height(d, X + 0.9239 * R, Y + 0.3827 * R,  balloffset);
	d = update_height(d, X + 0.7071 * R, Y + 0.7071 * R,  balloffset);
	d = update_height(d, X + 0.3827 * R, Y + 0.9239 * R,  balloffset);
	d = update_height(d, X + 0.0000 * R, Y + 1.0000 * R,  balloffset);
	d = update_height(d, X - 0.3872 * R, Y + 0.9239 * R,  balloffset);
	d = update_height(d, X - 0.7071 * R, Y + 0.7071 * R,  balloffset);
	d = update_height(d, X - 0.9239 * R, Y + 0.3827 * R,  balloffset);
	d = update_height(d, X - 1.0000 * R, Y + 0.0000 * R,  balloffset);
	d = update_height(d, X - 0.9239 * R, Y - 0.3827 * R,  balloffset);
	d = update_height(d, X - 0.7071 * R, Y - 0.7071 * R,  balloffset);
	d = update_height(d, X - 0.3827 * R, Y - 0.9239 * R,  balloffset);
	d = update_height(d, X - 0.0000 * R, Y - 1.0000 * R,  balloffset);
	d = update_height(d, X + 0.3827 * R, Y - 0.9239 * R,  balloffset);
	d = update_height(d, X + 0.7071 * R, Y - 0.7071 * R,  balloffset);
	d = update_height(d, X + 0.9239 * R, Y - 0.3827 * R,  balloffset);

	R = R / 1.5;

	if (R < 0.4)
		return Math.ceil(d*ACC)/ACC;

	balloffset = -geometry_at_distance(R);

	d = update_height(d, X + 1.0000 * R, Y + 0.0000 * R,  balloffset);
	d = update_height(d, X + 0.9239 * R, Y + 0.3827 * R,  balloffset);
	d = update_height(d, X + 0.7071 * R, Y + 0.7071 * R,  balloffset);
	d = update_height(d, X + 0.3827 * R, Y + 0.9239 * R,  balloffset);
	d = update_height(d, X + 0.0000 * R, Y + 1.0000 * R,  balloffset);
	d = update_height(d, X - 0.3872 * R, Y + 0.9239 * R,  balloffset);
	d = update_height(d, X - 0.7071 * R, Y + 0.7071 * R,  balloffset);
	d = update_height(d, X - 0.9239 * R, Y + 0.3827 * R,  balloffset);
	d = update_height(d, X - 1.0000 * R, Y + 0.0000 * R,  balloffset);
	d = update_height(d, X - 0.9239 * R, Y - 0.3827 * R,  balloffset);
	d = update_height(d, X - 0.7071 * R, Y - 0.7071 * R,  balloffset);
	d = update_height(d, X - 0.3827 * R, Y - 0.9239 * R,  balloffset);
	d = update_height(d, X - 0.0000 * R, Y - 1.0000 * R,  balloffset);
	d = update_height(d, X + 0.3827 * R, Y - 0.9239 * R,  balloffset);
	d = update_height(d, X + 0.7071 * R, Y - 0.7071 * R,  balloffset);
	d = update_height(d, X + 0.9239 * R, Y - 0.3827 * R,  balloffset);


	return Math.ceil(d*ACC)/ACC;

}


function segments_to_gcode(maxlook = 1)
{
    for (let lev = levels.length - 1; lev >= 0; lev--) {
        let start = 0;
        /* force the first sgement to never be a hit to a previous <whatever> including previous layer */
        gcode_cX = -4000.0;
        gcode_cY = -4000.0;
        gcode_cZ = -4000.0;
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
            
            if (dist3(gcode_cX, gcode_cY, gcode_cZ, segm.X1, segm.Y1, segm.Z1) < segm.direct_mill_distance) {
                if (dist3(gcode_cX, gcode_cY, gcode_cZ, segm.X1, segm.Y1, segm.Z1) > 0.00001) {
                    gcode_mill_to_3D(segm.X1, segm.Y1, segm.Z1);            
                }
                found_one = 1;
                gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);            
                
                levels[lev].paths.splice(seg, 1);
                start = seg;
                break;
                
            }
         }
         
         if (found_one == 0) {
            /* step 2: we didn't find any, so just pick the first one */
            let segm = levels[lev].paths[0];
            /* go up and horizontal to the start of the segment */
            gcode_travel_to(segm.X1, segm.Y1);
            /* plunge */
            gcode_mill_to_3D(segm.X1, segm.Y1, segm.Z1);
            /* and mill */
            gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);            
            levels[lev].paths.splice(0, 1);
            start = 0;
        }
      }
    }
    levels = [];
    console.log("Segments_to_gcode " + (Date.now() - startdate));
    console.log("Total retract count", gcode_retract_count);
    console.log("Total halfway count", halfway_counter);
}

function segments_to_gcode_quick()
{
    for (let lev = levels.length - 1; lev >= 0; lev--) {
        for (let seg = 0; seg < levels[lev].paths.length; seg++) {
            let segm = levels[lev].paths[seg];
            
            if (dist3(gcode_cX, gcode_cY, gcode_cZ, segm.X1, segm.Y1, segm.Z1) > segm.direct_mill_distance) {
                gcode_travel_to(segm.X1, segm.Y1);
                gcode_mill_to_3D(segm.X1, segm.Y1, segm.Z1);
//                console.log("cx ", gcode_cX, " X1 ", segm.X1);
//                console.log("cy ", gcode_cY, " X1 ", segm.Y1);
//                console.log("cz ", gcode_cZ, " X1 ", segm.Z1);
            } else {
                if (dist3(gcode_cX, gcode_cY, gcode_cZ, segm.X1, segm.Y1, segm.Z1) > 0.00001) {
                    gcode_mill_to_3D(segm.X1, segm.Y1, segm.Z1);            
                }
            }
            gcode_mill_to_3D(segm.X2, segm.Y2, segm.Z2);            
        }
    }
    levels = [];
    console.log("Segments_to_gcode " + (Date.now() - startdate));
    console.log("Total retract count", gcode_retract_count);
    console.log("Total halfway count", halfway_counter);
}

let prev_pct = 0;


function roughing_zig(X, deltaY)
{
       let Y = -tool_diameter / 2;
        
        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, 2 * tool_diameter / 2) + tool_stock_to_leave;;
        let maxY = global_maxY + tool_diameter / 2;
        
//        gcode_travel_to(X, 0);
        while (Y <= maxY) {
            /* for roughing we look 2x the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, 2 * tool_diameter / 2) + tool_stock_to_leave;
            if (Math.abs(prevZ - Z) > 0.6) {
                halfway_counter += 1;
                let halfY = (Y + prevY) / 2;
                let halfZ = get_height_tool(X, halfY, 2 * tool_diameter / 2) + tool_stock_to_leave;
                push_segment_multilevel(prevX, prevY, prevZ, X, halfY, halfZ, tool_diameter * 0.7);
                prevY = halfY;
                prevZ = halfZ;
            } 
            

            push_segment_multilevel(prevX, prevY, prevZ, X, Y, Z, tool_diameter * 0.7);
            
            
            prevY = Y;
            prevZ = Z;
            
            if (Y == maxY) 
            {
                break;
            }
            Y = Y + deltaY;
            if (Y > maxY) 
            {
                Y = maxY;
            }
        }    
        let pct = (Math.round(X/global_maxX * 100));
        if (pct > prev_pct + 10 || pct > 99) {
            var elem = document.getElementById("BarRoughing");
            elem.style.width = pct + "%";
            prev_pct = pct;
        }
    
 }
 
function roughing_zag(X, deltaY)
{
        let Y = global_maxY + tool_diameter / 2;
        
        let newZ = get_height_tool(X, Y, 2 * tool_diameter / 2) + tool_stock_to_leave;;
        let prevX = X;
        let prevY = Y;
        let minY = -tool_diameter / 2;
        
        let prevZ = newZ;
        
//        gcode_travel_to(X, 0);
        while (Y >= minY) {
            /* for roughing we look 2x the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, 2 * tool_diameter / 2) + tool_stock_to_leave;

            if (Math.abs(prevZ - Z) > 0.6) {
                halfway_counter += 1;
                let halfY = (Y + prevY) / 2;
                let halfZ = get_height_tool(X, halfY, 2 * tool_diameter / 2) + tool_stock_to_leave;
                push_segment_multilevel(prevX, prevY, prevZ, X, halfY, halfZ, tool_diameter * 0.7);
                prevY = halfY;
                prevZ = halfZ;
            } 
            

            push_segment_multilevel(prevX, prevY, prevZ, X, Y, Z, tool_diameter * 0.7);
            
            
            prevY = Y;
            prevZ = Z;
            
            if (Y == minY) {
                break;
            }
            Y = Y - deltaY;
            if (Y  <= minY) {
                Y = minY;
            }
        }
        let pct = (Math.round(X/global_maxX * 100));
        if (pct > prev_pct + 10 || pct > 99) {
            var elem = document.getElementById("BarRoughing");
            elem.style.width = pct + "%";
            prev_pct = pct;
        }
    
        
}


function cutout_box1()
{
    let minX = -tool_diameter / 2;
    let minY = -tool_diameter / 2;
    let maxX = global_maxX + tool_diameter / 2;
    let maxY = global_maxY + tool_diameter / 2;
    
    let maxZ = -global_maxZ + tool_depth_of_cut * 0.5;
    
    push_segment_multilevel(minX, minY, maxZ, minX, maxY, maxZ, tool_diameter / 1.9);
    push_segment_multilevel(minX, maxY, maxZ, maxX, maxY, maxZ, tool_diameter / 1.9);
    push_segment_multilevel(maxX, maxY, maxZ, maxX, minY, maxZ, tool_diameter / 1.9);
    push_segment_multilevel(maxX, minY, maxZ, minX, minY, maxZ, tool_diameter / 1.9);
}

function cutout_box2()
{
    let minX = -tool_diameter / 2;
    let minY = -tool_diameter / 2;
    let maxX = global_maxX + tool_diameter / 2;
    let maxY = global_maxY + tool_diameter / 2;
    
    let maxZ = -global_maxZ;
    
    push_segment(minX, minY, maxZ, minX, maxY, maxZ, 0);
    push_segment(minX, maxY, maxZ, maxX, maxY, maxZ, 0);
    push_segment(maxX, maxY, maxZ, maxX, minY, maxZ, 0);
    push_segment(maxX, minY, maxZ, minX, minY, maxZ, 0);
}


function roughing_zig_zag(tool)
{
    gcode_select_tool(tool);
    setTimeout(gcode_change_tool, 0, tool);
    
    let deltaX = tool_diameter / 2;
    let deltaY = tool_diameter / 4;
    let X = 0;
    let lastX = 0;
    
    if (deltaY > 0.5) {
        deltaY = 0.5;
    }

    setTimeout(cutout_box1, 0);    
    
    while (X <= global_maxX) {

        setTimeout(roughing_zig, 0, X, deltaY);        
        
        if (X == global_maxX) {
            break;
        }
        X = X + deltaX;
        if (X > global_maxX) {
            X = global_maxX;
        }


        setTimeout(roughing_zag, 0, X, deltaY);
        
        
        if (X == global_maxX) {
            break;
        }
        X = X + deltaX;
        if (X > global_maxX) {
            X = global_maxX;
            
        }

    }
    
    setTimeout(segments_to_gcode, 0, 50);
    setTimeout(cutout_box2, 0);    
    setTimeout(segments_to_gcode, 0);
    
}

let halfway_counter = 0;
function finishing_zig(Y, deltaX)
{
        let X = -tool_diameter / 2;
        let maxX = global_maxX + tool_diameter / 2;
        
        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, tool_diameter / 2);
        
//        gcode_travel_to(X, 0);
        while (X <= maxX) {
            let Z = get_height_tool(X, Y, tool_diameter / 2);
            
            if (Math.abs(prevZ - Z) > 0.6) {
                halfway_counter += 1;
                let halfX = (X + prevX) / 2;
                let halfZ = get_height_tool(halfX, Y, tool_diameter / 2);
                push_segment(prevX, prevY, prevZ, halfX, Y, halfZ, 0, 5);
                prevX = halfX;
                prevZ = halfZ;
            } 
            
            push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
            prevX = X;
            prevZ = Z;
            
            if (X == maxX) 
            {
                break;
            }
            X = X + deltaX;
            if (X > maxX) 
            {
                X = maxX;
            }
        }
    

}
let prev_pct2 = 0;
function finishing_zag(Y, deltaX)
{
        let X = global_maxX + tool_diameter / 2;
        let minX = -tool_diameter / 2;
        
        let newZ = get_height_tool(X, Y, tool_diameter / 2) ;
        let prevY = Y;
        let prevX = global_maxX;
        
        let prevZ = newZ;
        
//        gcode_travel_to(X, 0);
        while (X >= minX) {
            /* for roughing we look 2x the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, tool_diameter / 2);
            if (Math.abs(prevZ - Z) > 0.6) {
                halfway_counter += 1;
                let halfX = (X + prevX) / 2;
                let halfZ = get_height_tool(halfX, Y, tool_diameter / 2);
                push_segment(prevX, prevY, prevZ, halfX, Y, halfZ, 0, 5);
                prevX = halfX;
                prevZ = halfZ;
            } 

            push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
            prevX = X;
            prevZ = Z;
            
            if (X == minX) 
            {
                break;
            }
            X = X - deltaX;
            if (X  <= minX) 
            {
                X = minX;
            }
        }
        let pct = (Math.round(Y/global_maxY * 100));
        if (pct > prev_pct2 + 5 || pct > 99) {
            if (pct > 100) {
                pct = 100;
            }
            if (pct < 0) {
                pct = 0;
            }
            var elem = document.getElementById("BarFinishing");
            elem.style.width = pct + "%";
            prev_pct2 = pct;
        }
}

function finishing_zig_zag(tool)
{

    setTimeout(gcode_change_tool, 0, tool);
    gcode_select_tool(tool);
    
    let minY = -tool_diameter / 2;
    let maxY = global_maxY + tool_diameter / 2;
    
    
    let deltaX = tool_diameter / 10;
    let deltaY = tool_diameter / 10;
    let Y = -minY;
    let lastY = 0;
    
    
    if (deltaX > 0.5) {
        deltaX = 0.5;
    }
    if (deltaX < 0.15) {
        deltaX = Math.min(0.15, tool_diameter/2);
    }
    
    if (deltaY < 0.15) {
        deltaY = 0.15;
    }
    
    console.log("dX ", deltaX, "  dY ", deltaY);
    while (Y <= maxY) {
 
        setTimeout(finishing_zig, 0, Y, deltaX);       
        
        
        if (Y == maxY) {
            break;
        }
        Y = Y + deltaY;
        if (Y > maxY) {
            Y = maxY;
        }


        setTimeout(finishing_zag, 0, Y, deltaX);       

        
        
        if (Y == maxY) {
            break;
        }
        Y = Y + deltaY;
        if (Y > maxY) {
            Y = maxY;
            
        }

    }
    
    
    setTimeout(segments_to_gcode_quick, 0);
}

let startdate;

function update_gcode_on_website()
{
    var pre = document.getElementById('outputpre')
    pre.innerHTML = gcode_string;
    var link = document.getElementById('download')
    link.innerHTML = 'Download ' + filename+".nc";
    link.href = "#";
    link.download = filename + ".nc";
    link.href = "data:text/plain;base64," + btoa(gcode_string);
    gcode_string = "";
    console.log("Gcode calculation " + (Date.now() - startdate));
}


function reset_globals()
{
    prev_pct = 0;
    prev_pct2 = 0;
    global_minX = 600000000;
    global_minY = 600000000;
    global_minZ = 600000000;
    global_maxX = -600000000;
    global_maxY = -600000000;
    global_maxZ = -600000000;
    triangles = [];
    buckets = [];
    l2buckets = [];
    gcode_string = "";
    gcode_cX = 0.0;
    gcode_cY = 0.0;
    gcode_cZ = 0.0;
    gcode_cF = 0.0;
    gcode_first_toolchange = 1;
    gcode_retract_count = 0;
    levels = [];
    halfway_counter = 0;
}

function calculate_image() 
{
    
    startdate = Date.now();
    gcode_header();    

    roughing_zig_zag(roughing_endmill);

    finishing_zig_zag(finishing_endmill);

    setTimeout(gcode_footer, 0);
    setTimeout(update_gcode_on_website, 0);
    
}

function RadioB(val)
{
    if (val == "top") {
        orientation = 0;
    };
    if (val == "front") {
        console.log("set front orientation")
        orientation = 1;
    };
    if (val == "side") {
        console.log("set side orientation")
        orientation = 2;
    };
}

function SideB(val)
{
    let news = parseFloat(val);
    if (news > 0) {
        desired_depth = news;
    }
}


function OffsetB(val)
{
    let news = Math.floor(parseFloat(val));
    if (news >= 0 && news < 100) {
        console.log("Setting zoffset to " + val);
        zoffset = news;
    }
}

function load(evt) 
{
    var start;
    start =  Date.now();
    
    
    if (evt.target.readyState == FileReader.DONE) {
        reset_globals();        
        process_data(evt.target.result);    
        console.log("End of data processing " + (Date.now() - start));
        start = Date.now();
        calculate_image();
        console.log("Image calculation " + (Date.now() - start));
    }    
}


function basename(path) 
{
     return path.replace(/.*\//, '');
}

function handle(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
        let fn = basename(f.name);
        if (fn != "" && fn.includes(".stl")) {
                filename = fn;
        }
        var reader = new FileReader();
        reader.onloadend = load;
        reader.readAsBinaryString(f);
    }
}

function handle_roughing(val)
{
    roughing_endmill = Math.floor(parseFloat(val));;
}

function handle_finishing(val)
{
    finishing_endmill = Math.floor(parseFloat(val));;
}


function handle_depth(val)
{
    let news = parseFloat(val);
    if (news > 0) {
        if (gui_is_metric) {
            desired_depth = news;
        } else {
            desired_depth = inch_to_mm(news);
        }
    }
    console.log("Setting depth to ", desired_depth);
}

function handle_width(val)
{
    let news = parseFloat(val);
    if (news > 0) {
        if (gui_is_metric) {
            desired_width = news;
        } else {
            desired_width = inch_to_mm(news);
        }
    }
    console.log("Setting width to ", desired_width);
}

function handle_height(val)
{
    let news = parseFloat(val);
    if (news > 0) {
        if (gui_is_metric) {
            desired_height = news;
        } else {
            desired_height = inch_to_mm(news);
        }
    }
    console.log("Setting height to ", desired_height);
}


function update_gui_dimensions()
{
    let link;
    let linkmm;
    
    link  = document.getElementById('width')
    linkmm = document.getElementById('widthmm')
    if (gui_is_metric) {
        link.value = desired_width;
        linkmm.innerHTML = "mm";
    } else {
        link.value = mm_to_inch(desired_width);
        linkmm.innerHTML = "inch";
    }

    link  = document.getElementById('height')
    linkmm = document.getElementById('heightmm')
    if (gui_is_metric) {
        link.value = desired_height;
        linkmm.innerHTML = "mm";
    } else {
        link.value = mm_to_inch(desired_height);
        linkmm.innerHTML = "inch";
    }
    link  = document.getElementById('depth')
    linkmm = document.getElementById('depthmm')
    if (gui_is_metric) {
        link.value = desired_depth;
        linkmm.innerHTML = "mm";
    } else {
        link.value = mm_to_inch(desired_depth);
        linkmm.innerHTML = "inch";
    }
}

function update_gui_actuals()
{
    let link;
    
    link  = document.getElementById('resultwidth')
    if (gui_is_metric) {
        link.innerHTML = Math.ceil(global_maxX*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(global_maxX) + "\"";
    }

    link  = document.getElementById('resultheight')
    if (gui_is_metric) {
        link.innerHTML = Math.ceil(global_maxY*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(global_maxY) + "\"";
    }
    link  = document.getElementById('resultdepth')
    if (gui_is_metric) {
        link.innerHTML = Math.ceil(global_maxZ*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(global_maxZ) +"\"";
    }
}

function handle_metric(val)
{
    if (val == "imperial") {
        gui_is_metric = 0;
    } 
    if (val == "metric") {
        gui_is_metric = 1;
    } 
    update_gui_dimensions();
}


document.getElementById('files').onchange = handle;
