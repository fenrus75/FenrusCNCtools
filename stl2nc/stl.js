
let zoffset = 0.0;

function inch_to_mm(inch)
{
    return Math.ceil( (inch * 25.4) *1000)/1000;
}

function mm_to_inch(mm)
{
    return Math.ceil(mm / 25.4 * 1000)/1000;
}

/* take a stored ieee float into a JS number */
function data_f32_to_number(data, offset)
{
    let u32value = (data.charCodeAt(offset + 0)    )  + (data.charCodeAt(offset + 1)<<8) + 
                   (data.charCodeAt(offset + 2)<<16)  + (data.charCodeAt(offset + 3)<<24);     
    let sign = (u32value & 0x80000000)?-1:1; // TODO: Does STL support negative numbers at all? 
    let mant = (u32value & 0x7FFFFF);
    let exp  = (u32value >> 23) & 0xFF;
    
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


class Triangle {
  /* creates a triangle straight from binary STL data */
  constructor (data, offset)
  {
    /* basic vertices */
    this.vertex = new Array(3);
    this.vertex[0] = new Array(3);
    this.vertex[1] = new Array(3);
    this.vertex[2] = new Array(3);
    /* and the bounding box */
    this.minX = 6000000000.0;
    this.minY = 6000000000.0;
    this.maxX = -6000000000.0;
    this.maxY = -6000000000.0;
    this.minZ = 600000000.0;
    this.maxZ = -600000000.0;
    /* status = 0 -> add to the bucket hierarchy later, status = 1 -> don't add (again) */
    this.status = 0;
    
    
    /* first 12 bytes are the surface normal vector, only Z we'll use in a bit */
    
    this.vertex[0][0] = data_f32_to_number(data, offset + 12);
    this.vertex[0][1] = data_f32_to_number(data, offset + 16);
    this.vertex[0][2] = data_f32_to_number(data, offset + 20);

    this.vertex[1][0] = data_f32_to_number(data, offset + 24);
    this.vertex[1][1] = data_f32_to_number(data, offset + 28);
    this.vertex[1][2] = data_f32_to_number(data, offset + 32);

    this.vertex[2][0] = data_f32_to_number(data, offset + 36);
    this.vertex[2][1] = data_f32_to_number(data, offset + 40);
    this.vertex[2][2] = data_f32_to_number(data, offset + 44);
    
    if (orientation == 0) {
        let Znorm =  data_f32_to_number(data, offset + 8);
        /* if the normal vector of the triangle points downwards, we'll never see it so we mark it as not interesting */
        if (Znorm < 0) {
            this.status = 1;
        }
    }
    
    /* Front */
    if (orientation == 1) {
        let x,y,z;
        let Ynorm =  data_f32_to_number(data, offset + 4);
        if (Ynorm > 0) {
            this.status = 1;
        }
        for (let i = 0; i < 3; i++) {
            x = this.vertex[i][0];
            y = this.vertex[i][1];
            z = this.vertex[i][2];
            this.vertex[i][0] = x;
            this.vertex[i][1] = z;
            this.vertex[i][2] = -y;
        }
    }
    /* Back */
    if (orientation == 3) {
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
    
    /* and now set the various min/max variables */
   
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
  
  /* returns 1 if (X,Y) is within the triangle */
  /* in the math sense, this is true if the three determinants are not of different signs */
  /* a determinant of 0 means on the line, and does not count as opposing sign in any way */
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


        if (det1 < 0) { has_neg = 1; };
        if (det2 < 0) { has_neg = 1; };
        if (det3 < 0) { has_neg = 1; };

        if (det1 > 0) { has_pos = 1; };
        if (det2 > 0) { has_pos = 1; };
        if (det3 > 0) { has_pos = 1; };
        
        /* check if we have opposing signs */
        if (has_neg && has_pos)
            return 0;
        return 1;

  }
  /* for (X,Y) inside the triangle (see previous function), calculate the Z of the intersection point */
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

class TrianglePNG {
  /* creates a triangle straight from PNG data */
  constructor (X1,Y1,Z1)
  {
    /* basic vertices */
    this.vertex = new Array(3);
    this.vertex[0] = new Array(3);
    this.vertex[1] = new Array(3);
    this.vertex[2] = new Array(3);
    /* and the bounding box */
    this.minX = 6000000000.0;
    this.minY = 6000000000.0;
    this.maxX = -6000000000.0;
    this.maxY = -6000000000.0;
    this.minZ = 600000000.0;
    this.maxZ = -600000000.0;
    /* status = 0 -> add to the bucket hierarchy later, status = 1 -> don't add (again) */
    this.status = 0;
    
    
    /* first 12 bytes are the surface normal vector, only Z we'll use in a bit */
    
    this.vertex[0][0] = 0;
    this.vertex[0][1] = 0;
    this.vertex[0][2] = 0;

    this.vertex[1][0] = X1;
    this.vertex[1][1] = 0;
    this.vertex[1][2] = Z1;

    this.vertex[2][0] = X1;
    this.vertex[2][1] = Y1;
    this.vertex[2][2] = Z1;
    
    /* and now set the various min/max variables */
   
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
  
  /* returns 1 if (X,Y) is within the triangle */
  /* in the math sense, this is true if the three determinants are not of different signs */
  /* a determinant of 0 means on the line, and does not count as opposing sign in any way */
  within_triangle(X, Y)
  {
        return 1;
  }
  /* for (X,Y) inside the triangle (see previous function), calculate the Z of the intersection point */
  calc_Z(X, Y)
  {
      let x,y,z;
      x = Math.floor(this.xyscale * X);
      y = this.image.height - Math.floor(this.xyscale * Y) - 1;
      z = this.depth * this.image.data[x*4 + 1 + y * this.image.width * 4] / pixeldivider;

      
      return z;
      
  }

}


let triangles = []
let buckets = []
let l2buckets = []


/*
 * An STL file is supposed to be in the "all positive" quadrant, but there's no rule
 * for how far away from the zero point the design is. This function just moves the
 * whole design so that everything is positive, but that the zero point is really 
 * the bottom left point.
 *
 * NOTE: This takes a "zoffset" option, that allows for a %age of the design to
 * "sink" into the Z plane, so you can get negative values. That's ok since 
 * the height functions later will ignore negative Z's
 *
 * This function does not update the bounding box; right after this function
 * the design will be scaled and the final bounding box is updated there.
 */
function normalize_design_to_zero()
{	
    let len = triangles.length;
    
    let offset = (global_maxZ - global_minZ) * zoffset / 100.0;
    
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

        triangles[i].vertex[0][2] -= offset;
        triangles[i].vertex[1][2] -= offset;
        triangles[i].vertex[2][2] -= offset;
    }

    global_maxX -= global_minX;    
    global_maxY -= global_minY;   
    global_maxZ -= global_minZ;    
    global_maxZ -= offset;
    global_minX = 0.0;
    global_minY = 0.0;
    global_minZ = 0.0;
}


/*
 * Scale the design so that it fits in the stock of desired_width (X)  x desired_hight (Y) x desired_depth (Z)
 */
function scale_design(desired_width, desired_height, desired_depth)
{	
    let len = triangles.length;
    
    let factor = 1.0;
    
    normalize_design_to_zero();
    
    /* calculate the three different scale factors to make the design fit in the respective axis */
    let factorD = desired_depth / global_maxZ; 
    let factorW = desired_width / global_maxX; 
    let factorH = desired_height / global_maxY; 

    /* and the smallest scale factor ensures that the design fits in the stock */
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
        
        /* if we're entirely below the surface due to a Z offset, mark it for skipping */
        if (triangles[i].maxZ <= 0) {
            triangles[i].status = 1;
        } 
    }

    global_maxX *= factor;    
    global_maxY *= factor;   
    global_maxZ *= factor;    
}

/* helper inlined later; determinant is used to say if a point is to the left of a line */
function  point_to_the_left(X, Y, AX, AY, BX, BY)
{
	return (BX-AX)*(Y-AY) - (BY-AY)*(X-AX);
}


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

		l2bucket.minX = Xmin - 0.001;
		l2bucket.minY = Ymin - 0.001;
		l2bucket.maxX = Xmax + 0.001;
		l2bucket.maxY = Ymax + 0.001;
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

            		value = Math.max(newZ, value);
                }
            }
        }
	return value - global_maxZ + offset;
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


/*
 * This function takes the raw data of an STL file, processes this data into ascii or binary triangles
 * and sets up the various data structures (buckets etc) so that no other part of the code really
 * has to know anything about STL... just about triangles that are already organized in a bucket
 * hierarchy.
 */
export function process_data(data, desired_width, desired_height, desired_depth)
{
    let len = data.length;
    
    var start;
    
    start = Date.now();
    
    reset_stl_state();
    
    if (len < 84) {
        document.getElementById('list').innerHTML = "STL file too short";
        return; 
    }
    
    if (data[0] == 's' && data[1] == 'o' && data[2] == 'l' && data[3] == 'i' && data[4] == 'd') {
        console.log("ASCII STL detected");
        return process_data_ascii(data, desired_width, desired_height, desired_depth);
    }
    
    let total_triangles = (data.charCodeAt(80)) + (data.charCodeAt(81)<<8) + (data.charCodeAt(82)<<16) + (data.charCodeAt(83)<<24); 
    console.log("Triangle count " + total_triangles);
    console.log("Data length " + data.length);
    
    if (84 + total_triangles * 50  != data.length) {
        document.getElementById('list').innerHTML  = "Length mismatch " + data.length + " " + total_triangles;
        return;
    }
    
    console.log("Start of parsing at " + (Date.now() - start), " orientation is ", orientation);
    
    for (let i = 0; i < total_triangles; i++) {
        let T = new Triangle(data, 84 + i * 50);
        triangles.push(T);
    }

    console.log("End of parsing at " + (Date.now() - start));

    scale_design(desired_width, desired_height, desired_depth);    
    update_gui_actuals(desired_depth);	
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));

    console.log("Scale " + (Date.now() - start));
    
//    document.getElementById('list').innerHTML  = "Number of triangles " + total_triangles + "mX " + global_maxX + " mY " + global_maxY;
}


let pixeldivider = 256.0;

function png_get_pixel_value(image, X, Y)
{
  let _y = image.height - Y - 1; /* PNG files are upside down compared to our zero point */
  return image.bitmap[X + _y * image.lineLen] / pixeldivider;
}

export function process_data_PNG(image, desired_width, desired_height, desired_depth)
{
    let start = Date.now();
    let xscale, yscale;
    let xyscale;
    
    reset_stl_state();
    pixeldivider = 256.0;
    if (image.depth == 16)
        pixeldivider = 256 * 256.0;

    console.log("Processing PNG image of size ", image.width, "x", image.height, " ", pixeldivider);
    
    xscale = 1.0* image.width / desired_width;
    yscale = 1.0* image.height / desired_height;
    xyscale = Math.min(xscale, yscale, desired_depth);

    let T = new TrianglePNG(desired_width, desired_height, desired_depth);
    T.image = image;
    T.xyscale = xyscale;
    T.depth = desired_depth;
    triangles.push(T);

    console.log("End of parsing at " + (Date.now() - start));
    console.log("Number of triangles ", triangles.length);

    update_gui_actuals(desired_depth);	
    console.log("Making buckets");
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));

    console.log("Scale " + (Date.now() - start));
    
//    document.getElementById('list').innerHTML  = "Number of triangles " + total_triangles + "mX " + global_maxX + " mY " + global_maxY;
}

function process_data_ascii(data, desired_width, desired_height, desired_depth)
{
    let len = data.length;

    let lines = data.split('\n');    
    var start;
    
    start = Date.now();
    
    triangles = [];
    buckets = [];
    l2buckets = [];
    global_minX = 600000000.0;
    global_minY = 600000000.0;
    global_minZ = 600000000.0;
    global_maxX = -600000000.0;
    global_maxY = -600000000.0;
    global_maxZ = -600000000.0;
    
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

    scale_design(desired_width, desired_height, desired_depth);    
    update_gui_actuals(desired_depth);	
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));

    console.log("Scale " + (Date.now() - start));
}

export function set_zoffset(offset) {
    console.log("Setting zoffset to " + offset);
    if (offset >= 0 && offset <= 100) {
        zoffset = offset;
    }
}

export function reset_stl_state() {
     global_minX = 600000000.0;
     global_minY = 600000000.0;
     global_minZ = 600000000.0;
     global_maxX = -600000000.0;
     global_maxY = -600000000.0;
     global_maxZ = -600000000.0;
     triangles = [];
     buckets = [];
     l2buckets = [];
}

export function update_gui_actuals(desired_depth)
{
    let link;
    
    link  = document.getElementById('resultwidth')
    if (gui_is_metric) {
        link.innerHTML = Math.ceil(Math.max(0,global_maxX)*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(Math.max(0, global_maxX)) + "\"";
    }

    link  = document.getElementById('resultheight')
    if (gui_is_metric) {
        link.innerHTML = Math.ceil(Math.max(0, global_maxY)*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(Math.max(0, global_maxY)) + "\"";
    }
    link  = document.getElementById('resultdepth')
    if (gui_is_metric) {
        link.innerHTML = Math.floor(Math.max(global_maxZ, 0)*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(Math.max(0, global_maxZ)) +"\"";
    }
    if (global_maxZ < -500000000.0)
        return;
    link  = document.getElementById('basedepth')
    if (gui_is_metric) {
        link.innerHTML = Math.floor(Math.max(desired_depth - global_maxZ, 0)*10)/10 + "mm";
    } else {
        link.innerHTML = mm_to_inch(Math.max(0, desired_depth - global_maxZ)) +"\"";
    }
}


let gui_is_metric = 1;
export function set_metric(metric, desired_depth)
{
    gui_is_metric = metric;
    update_gui_actuals(desired_depth);
}

export function set_orientation(o)
{
    orientation = o;
    console.log("orientation is ", o);
}