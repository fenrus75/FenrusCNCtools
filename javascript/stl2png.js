/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/

let desired_resolution = 1024;

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
function Triangle(data, offset)
{
    this.vertex = new Array(3);
    this.vertex[0] = new Array(3);
    this.vertex[1] = new Array(3);
    this.vertex[2] = new Array(3);
    this.minX = 6000000000;
    this.minY = 6000000000;
    this.maxX = -6000000000;
    this.maxY = -6000000000;
    this.status = 0;
    
    this.vertex[0][0] = data_f32_to_number(data, offset + 12);
    this.vertex[0][1] = -data_f32_to_number(data, offset + 16);
    this.vertex[0][2] = data_f32_to_number(data, offset + 20);

    this.vertex[1][0] = data_f32_to_number(data, offset + 24);
    this.vertex[1][1] = -data_f32_to_number(data, offset + 28);
    this.vertex[1][2] = data_f32_to_number(data, offset + 32);

    this.vertex[2][0] = data_f32_to_number(data, offset + 36);
    this.vertex[2][1] = -data_f32_to_number(data, offset + 40);
    this.vertex[2][2] = data_f32_to_number(data, offset + 44);
    
    if (orientation == 1) {
        let x,y,z;
        for (let i = 0; i < 3; i++) {
            x = this.vertex[i][0];
            y = this.vertex[i][1];
            z = this.vertex[i][2];
            this.vertex[i][0] = x;
            this.vertex[i][1] = -z;
            this.vertex[i][2] = -y;
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


let triangles = []
let buckets = []
let l2buckets = []

function normalize_design_to_zero()
{	
    let len = triangles.length;
    
    for (let i = 0; i < len; i++) {
        t = triangles[i];
        t.vertex[0][0] -= global_minX;
        t.vertex[1][0] -= global_minX;
        t.vertex[2][0] -= global_minX;

        t.vertex[0][1] -= global_minY;
        t.vertex[1][1] -= global_minY;
        t.vertex[2][1] -= global_minY;


        t.vertex[0][2] -= global_minZ;
        t.vertex[1][2] -= global_minZ;
        t.vertex[2][2] -= global_minZ;
    }

    global_maxX -= global_minX;    
    global_maxY -= global_minY;   
    global_maxZ -= global_minZ;    
    global_minX = 0;
    global_minY = 0;
    global_minZ = 0;
}

function scale_design(newsize)
{	
    let len = triangles.length;
    
    let factor = 1;
    
    normalize_design_to_zero();
    
    if (global_maxX < 0.1)
        alert(global_maxX);
    if (global_maxY < 0.1)
        alert(global_maxY);
    factor = newsize / global_maxX;
    factor = Math.min(factor, newsize / global_maxY);
    
    
    for (let i = 0; i < len; i++) {
        t = triangles[i];
        t.vertex[0][0] *= factor;
        t.vertex[0][1] *= factor;
        t.vertex[0][2] *= factor;

        t.vertex[1][0] *= factor;
        t.vertex[1][1] *= factor;
        t.vertex[1][2] *= factor;

        t.vertex[2][0] *= factor;
        t.vertex[2][1] *= factor;
        t.vertex[2][2] *= factor;

        t.minX = Math.min(t.vertex[0][0], t.vertex[1][0]); 
        t.minX = Math.min(t.minX, 	  t.vertex[2][0]); 
        t.minY = Math.min(t.vertex[0][1], t.vertex[1][1]); 
        t.minY = Math.min(t.minY, 	  t.vertex[2][1]); 
        t.minZ = Math.min(t.vertex[0][2], t.vertex[1][2]); 
        t.minZ = Math.min(t.minZ, 	  t.vertex[2][2]); 
    
        t.maxX = Math.max(t.vertex[0][0], t.vertex[1][0]); 
        t.maxX = Math.max(t.maxX, 	  t.vertex[2][0]); 
        t.maxY = Math.max(t.vertex[0][1], t.vertex[1][1]); 
        t.maxY = Math.max(t.maxY, 	  t.vertex[2][1]); 
    }

    global_maxX *= factor;    
    global_maxY *= factor;   
    global_maxZ *= factor;    
}

function  point_to_the_left(X, Y, AX, AY, BX, BY)
{
	return (BX-AX)*(Y-AY) - (BY-AY)*(X-AX);
}


function within_triangle(X, Y, t)
{
	let det1, det2, det3;

	let has_pos = 0, has_neg = 0;
	
	det1 = point_to_the_left(X, Y, t.vertex[0][0], t.vertex[0][1], t.vertex[1][0], t.vertex[1][1]);
	det2 = point_to_the_left(X, Y, t.vertex[1][0], t.vertex[1][1], t.vertex[2][0], t.vertex[2][1]);
	det3 = point_to_the_left(X, Y, t.vertex[2][0], t.vertex[2][1], t.vertex[0][0], t.vertex[0][1]);

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


function calc_Z(X, Y, t)
{
	let det = (t.vertex[1][1] - t.vertex[2][1]) * 
		    (t.vertex[0][0] - t.vertex[2][0]) + 
                    (t.vertex[2][0] - t.vertex[1][0]) * 
		    (t.vertex[0][1] - t.vertex[2][1]);

	let l1 = ((t.vertex[1][1] - t.vertex[2][1]) * (X - t.vertex[2][0]) + (t.vertex[2][0] - t.vertex[1][0]) * (Y - t.vertex[2][1])) / det;
	let l2 = ((t.vertex[2][1] - t.vertex[0][1]) * (X - t.vertex[2][0]) + (t.vertex[0][0] - t.vertex[2][0]) * (Y - t.vertex[2][1])) / det;
	let l3 = 1.0 - l1 - l2;

	return l1 * t.vertex[0][2] + l2 * t.vertex[1][2] + l3 * t.vertex[2][2];
}

function Bucket(lead_triangle)
{
    this.minX = global_maxX;
    this.maxX = 0;
    this.minY = global_maxY;
    this.maxY = 0;
    
    this.triangles = []
    this.status = 0;
}

function L2Bucket(lead_triangle)
{
    this.minX = global_maxX;
    this.maxX = 0;
    this.minY = global_maxY;
    this.maxY = 0;
    
    this.buckets = []
    this.status = 0;
}


function make_buckets()
{
	let i;
	let slop = Math.max(global_maxX, global_maxY)/50;
	let maxslop = slop * 2;
	let len = triangles.length

	for (i = 0; i < len; i++) {
		let j;
		let reach;
		let Xmax, Ymax, Xmin, Ymin;
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
	
		for (j = i + 1; j < reach && bucket.triangles.length < 64; j++)	{
			if (triangles[j].status == 0 && triangles[j].maxX <= rXmax && triangles[j].maxY <= rYmax && triangles[j].minY >= rYmin &&  triangles[j].minX >= rXmin) {
				Xmax = Math.max(Xmax, triangles[j].maxX);
				Ymax = Math.max(Ymax, triangles[j].maxY);
				Xmin = Math.min(Xmin, triangles[j].minX);
				Ymin = Math.min(Ymin, triangles[j].minY);
				bucket.triangles.push(triangles[j]);
				triangles[j].status = 1;				
			}				
		}
                let bucketr = bucket.triangles.length
		if (bucketptr >= 64 -5)
			slop = slop * 0.9;
		if (bucketptr < 64 / 8)
			slop = Math.min(slop * 1.1, maxslop);
		if (bucketptr < 64 / 2)
			slop = Math.min(slop * 1.05, maxslop);

		bucket.minX = Xmin - 0.001; /* subtract a little to cope with rounding */
		bucket.minY = Ymin - 0.001;
		bucket.maxX = Xmax + 0.001;
		bucket.maxY = Ymax + 0.001;
		buckets.push(bucket);
	}
	console.log("Made " + buckets.length + " buckets\n");
	
	slop = Math.max(global_maxX, global_maxY)/10;
	maxslop = slop * 2;

	let nrbuckets = buckets.length
	for (i = 0; i < nrbuckets; i++) {
		let j;
		let Xmax, Ymax, Xmin, Ymin;
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
	
		for (j = i + 1; j < nrbuckets && l2bucket.buckets.length < 64; j++)	{
			if (buckets[j].status == 0 && buckets[j].maxX <= rXmax && buckets[j].maxY <= rYmax && buckets[j].minY >= rYmin &&  buckets[j].minX >= rXmin) {
				Xmax = Math.max(Xmax, buckets[j].maxX);
				Ymax = Math.max(Ymax, buckets[j].maxY);
				Xmin = Math.min(Xmin, buckets[j].minX);
				Ymin = Math.min(Ymin, buckets[j].minY);
				l2bucket.buckets.push(buckets[j]);
				buckets[j].status = 1;				
			}				
		}

		if (bucketptr >= 64 - 4)
			slop = slop * 0.9;
		if (bucketptr < 64 / 8)
			slop = Math.min(slop * 1.1, maxslop);
		if (bucketptr < 64 / 2)
			slop = Math.min(slop * 1.05, maxslop);

		l2bucket.minX = Xmin;
		l2bucket.minY = Ymin;
		l2bucket.maxX = Xmax;
		l2bucket.maxY = Ymax;
		l2buckets.push(l2bucket);
	}
	console.log("Created " + l2buckets.length + " L2 buckets\n");
	
}

function get_height(X, Y)
{
	let value = 0;
	let i;
	let k;
	
	let l2bl = l2buckets.length;
	
	for (k = 0; k < l2bl; k++) {
	
	let bl = l2buckets[k].buckets.length;
	l2bucket = l2buckets[k];
	let j;
	if (l2bucket.minX > X)
		        continue;
        if (l2bucket.minY > Y)
                        continue;
        if (l2bucket.maxX < X)
            		continue;
        if (l2bucket.maxY < Y)
                        continue;

	
	for (j =0 ; j < bl; j++) {
	        bucket = l2bucket.buckets[j];
        	let len = bucket.triangles.length;

                if (bucket.minX > X)
		        continue;
                if (bucket.minY > Y)
                        continue;
            	if (bucket.maxX < X)
            		continue;
                if (bucket.maxY < Y)
                        continue;

        	for (i = 0; i < len; i++) {
        	        let newZ;
        	        let t = bucket.triangles[i];
	    	

            		// first a few quick bounding box checks 
	        	if (t.minX > X)
		                continue;
                        if (t.minY > Y)
                                continue;

            		if (t.maxX < X)
            		        continue;
			
                        if (t.maxY < Y)
                                continue;


                        /* then a more expensive detailed triangle test */
                        if (!within_triangle(X, Y, t)) {
                                continue;
                        }
                        /* now calculate the Z height within the triangle */
                        newZ = calc_Z(X, Y, t);

            		value = Math.max(newZ, value);
                }
        }
        }
	return value;
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
    
    let total_triangles = (data.charCodeAt(80)) + (data.charCodeAt(81)<<8) + (data.charCodeAt(82)<<16) + (data.charCodeAt(83)<<24); 
    
    if (84 + total_triangles * 50  != data.length) {
        document.getElementById('list').innerHTML  = "Length mismatch " + data.length + " " + total_triangles;
        return;
    }
//    document.getElementById('list').innerHTML  = "Parsing STL file";
    
    console.log("Start of parsing at " + (Date.now() - start));
    
    for (let i = 0; i < total_triangles; i++) {
        T = new Triangle(data, 84 + i * 50);
        triangles.push(T);
    }

    console.log("End of parsing at " + (Date.now() - start));

    scale_design(desired_resolution);    
    make_buckets();
    console.log("End of buckets at " + (Date.now() - start));

    console.log("Scale " + (Date.now() - start));
    
//    document.getElementById('list').innerHTML  = "Number of triangles " + total_triangles + "mX " + global_maxX + " mY " + global_maxY;
}


function  calculate_line(data, Y, elem, context, pixels) 
{
    for (let X = 0; X < global_maxX; X++) {
            let c = 255 * (get_height(X, Y) / global_maxZ);
            data[(X + Y * global_maxX) * 4 + 0 ] = c;
            data[(X + Y * global_maxX) * 4 + 1 ] = c;
            data[(X + Y * global_maxX) * 4 + 2 ] = c;
            data[(X + Y * global_maxX) * 4 + 3 ] = 255;
    }
    elem.style.width = (Math.round(Y/global_maxY * 100)) + "%";
    if ((Y & 15) == 0) {
        context.putImageData(pixels, 0, 0);
    }
    if (Y == global_maxY -1) {
        context.putImageData(pixels, 0, 0);
        var link = document.getElementById('download')
        link.innerHTML = 'download image';
        link.href = "#";
        link.download = "output.png";
        link.href = canvas.toDataURL('image/png');
    }
}

function calculate_image() 
{
    var canvas = document.querySelector('canvas');
    var context = canvas.getContext('2d');
    canvas.width = global_maxX;
    canvas.height = global_maxY;
    
    
    global_maxX = Math.ceil(global_maxX);
    global_maxY = Math.ceil(global_maxY);
    
    var pixels = context.getImageData(
        0, 0, global_maxX, global_maxY
      );
      
    var elem = document.getElementById("Bar");
      
    var data = pixels.data;
    for (let Y = 0; Y < global_maxY; Y++) {
//            calculate_line(data, Y, elem, context, pixels);
        setTimeout(calculate_line, 0, data, Y, elem, context, pixels);
    }
    context.putImageData(pixels, 0, 0);
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
    let news = Math.floor(parseFloat(val));
    if (news > 0) {
        desired_resolution = news;
    }
}

function load(evt) 
{
    var start;
    start =  Date.now();
    if (evt.target.readyState == FileReader.DONE) {
        process_data(evt.target.result);    
        console.log("End of data processing " + (Date.now() - start));
        start = Date.now();
        calculate_image();
        console.log("Image calculation " + (Date.now() - start));
    }    
}

function handle(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
        var reader = new FileReader();
        reader.onloadend = load;
        reader.readAsBinaryString(f);
    }
}


document.getElementById('files').onchange = handle;
