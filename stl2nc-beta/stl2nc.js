/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/

import * as gcode from './gcode.js';
import * as tool from './tool.js';
import * as stl from './stl.js';
import * as segment from './segment.js';
import * as raster from './rasterscan.js';
import * as png from './png.js';

"use strict";

let desired_depth = 12.0;
let desired_width = 120.0;
let desired_height = 120.0;
let filename = "";
let gui_is_metric = 1;


/* Clear all global state so that you can load a new file */
function reset_globals()
{
    stl.reset_stl_state();
    gcode.reset_gcode_location()
    gcode.reset_gcode();
    segment.reset_segment();
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


function inch_to_mm(inch)
{
    return Math.ceil( (inch * 25.4) *1000)/1000;
}

function mm_to_inch(mm)
{
    return Math.ceil(mm / 25.4 * 1000)/1000;
}







/*
 * Series of onChange handers called by the HTML gui
 */

export function RadioB(val)
{
    if (val == "top") {
        console.log("set top orientation")
        stl.set_orientation(0);
    };
    if (val == "front") {
        console.log("set front orientation")
        stl.set_orientation(1);
    };
    if (val == "back") {
        console.log("set front orientation")
        stl.set_orientation(3);
    };
    if (val == "side") {
        console.log("set side orientation")
        stl.set_orientation(2);
    };
}

export function SideB(val)
{
    let news = parseFloat(val);
    if (news > 0) {
        desired_depth = news;
    }
}


function OffsetB(val)
{
    stl.set_zoffset(Math.floor(parseFloat(val)));
}

function load(evt) 
{
    var start;
    start =  Date.now();
    
    
    if (evt.target.readyState == FileReader.DONE) {
        reset_globals();        
        stl.process_data(evt.target.result, desired_width, desired_height, desired_depth);    
        console.log("End of data processing " + (Date.now() - start));
        start = Date.now();
        png.calculate_png_image(filename);
        raster.calculate_gcode(filename, desired_width, desired_height);
        console.log("Image calculation " + (Date.now() - start));
    }    
}


function basename(path) 
{
     return path.replace(/.*\//, '');
}

function png_decoded()
{
  let image = document.getElementById("image");
  var canvas = document.createElement('canvas');
  canvas.width = image.width;
  canvas.height = image.height;
  console.log("W/H ", image.width, " x ", image.height);
  canvas.getContext('2d').drawImage(image, 0, 0, image.width, image.height);  
  let pixeldata = canvas.getContext('2d').getImageData(0, 0, image.width, image.height);
  
  stl.process_data_PNG(pixeldata, desired_width, desired_height, desired_depth);    
  let start = Date.now();
  png.calculate_png_image(filename);
  raster.calculate_gcode(filename);
  console.log("Image calculation " + (Date.now() - start));
}

function png_loaded(evt)
{
  console.log("PNG loaded" + evt, evt.target.readyState);
  
  let image = document.getElementById("image");
  image.src = evt.target.result;
  image.decode().then(png_decoded);
}

export function handle(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
        let fn = basename(f.name);
        if (fn != "" && fn.includes(".stl")) {
                filename = fn;
        }
        if (fn != "" && fn.includes(".STL")) {
                filename = fn;
        }

        console.log("fn is ", fn)
        console.log("filename is ", filename)
        if (fn.includes(".png")) {
            console.log("Loading PNG file", fn);
            filename = fn;
            var reader = new FileReader();
            reader.onload = png_loaded;
            reader.readAsDataURL(f);
        } else {
            var reader = new FileReader();
            reader.onloadend = load;
            reader.readAsBinaryString(f);
        }
    }
}

export function handle_roughing(val)
{
    raster.set_roughing_endmill(val);
}

export function handle_stepover(val)
{
    let news = parseFloat(val);
    if (gui_is_metric) {
        tool.set_stepover_gui(news);
    } else {
        tool.set_stepover_gui(inch_to_mm(news));
    }
    
}

export function handle_finishing(val)
{
    raster.set_finishing_endmill(val);
    update_gui_dimensions();
}


export function handle_depth(val)
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

export function handle_width(val)
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

export function handle_height(val)
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

    link  = document.getElementById('stepover')
    linkmm = document.getElementById('stepovermm')
    if (gui_is_metric) {
        link.value = tool.tool_finishing_stepover
        linkmm.innerHTML = "mm";
    } else {
        link.value = mm_to_inch(tool.tool_finishing_stepover);
        linkmm.innerHTML = "inch";
    }

    link  = document.getElementById('chipload')
    linkmm = document.getElementById('chiploadmm')
    if (gui_is_metric) {
        link.value = tool.tool_finishing_stepover
        linkmm.innerHTML = "mm";
    } else {
        link.value = mm_to_inch(tool.chipload);
        linkmm.innerHTML = "inch";
    }
}


export function handle_metric(val)
{
    if (val == "imperial") {
        gui_is_metric = 0;
        stl.set_metric(0, desired_depth);
    } 
    if (val == "metric") {
        gui_is_metric = 1;
        stl.set_metric(1, desired_depth);
    } 
    update_gui_dimensions();
}

export function handle_precision(val)
{
    raster.set_precision(parseFloat(val));
    let link  = document.getElementById('rangeoutput');
    link.innerHTML = val;
    
}

export function handle_bitsetter(val)
{
    if (val == "bitsetter") {
        raster.set_split_gcode(0);
    }  else {
        raster.set_split_gcode(1);
    } 
}

export function handle_router(val)
{
    if (val == "router") {
        gcode.set_router(1);
    }  else {
        gcode.set_router(0);
    } 
}


export function handle_chipload(val)
{
    if (gui_is_metric) {
        tool.set_chipload(val);
    } else {
        tool.set_chipload(inch_to_mm(val));
    }
}

export function handle_custom_rippem(val)
{
  tool.custom_rippem(val);
}
export function handle_custom_diameter(val)
{
  tool.custom_diameter(val);
}
export function handle_custom_feedrate(val)
{
  tool.custom_feedrate(val);
}
export function handle_custom_plungerate(val)
{
  tool.custom_plungerate(val);
}
export function handle_custom_flutes(val)
{
  tool.custom_flutes(val);
}
export function handle_custom_doc(val)
{
  tool.custom_doc(val);
}


document.getElementById('files').onchange = handle;

/* Glue to handle html vs javascript modules */
window.handle_metric = handle_metric;
window.handle_bitsetter = handle_bitsetter;
window.handle_router = handle_router;
window.handle_stepover = handle_stepover;
window.handle_precision = handle_precision;
window.handle_width = handle_width;
window.handle_height = handle_height;
window.handle_depth = handle_depth;
window.handle_roughing = handle_roughing;
window.handle_finishing = handle_finishing;
window.handle_chipload = handle_chipload;
window.OffsetB = OffsetB;
window.RadioB = RadioB;

window.handle_custom_rippem = handle_custom_rippem;
window.handle_custom_diameter = handle_custom_diameter;
window.handle_custom_feedrate = handle_custom_feedrate;
window.handle_custom_plungerate = handle_custom_plungerate;
window.handle_custom_flutes = handle_custom_flutes;
window.handle_custom_doc = handle_custom_doc;
