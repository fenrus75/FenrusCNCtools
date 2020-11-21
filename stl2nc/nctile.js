/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/

import * as gcodetile from './gcodetile.js';
"use strict";

let desired_width = 120.0;
let desired_height = 120.0;
let desired_overlap = 0;
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


function load_gcode(evt) 
{
    var start;
    start =  Date.now();
    
    
    if (evt.target.readyState == FileReader.DONE) {
        gcodetile.process_data(filename, evt.target.result, desired_width, desired_height, desired_overlap);    
        console.log("End of gcode processing " + (Date.now() - start));
    }    
}



function basename(path) 
{
     return path.replace(/.*\//, '');
}

export function handle_gcode(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
        let fn = basename(f.name);
        if (fn != "" && fn.includes(".nc")) {
                filename = fn;
        }
        var reader = new FileReader();
        reader.onloadend = load_gcode;
        reader.readAsBinaryString(f);
    }
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

export function handle_overlap(val)
{
    let news = parseFloat(val);
    if (news > 0) {
        if (gui_is_metric) {
            desired_overlap = news;
        } else {
            desired_overlap = inch_to_mm(news);
        }
    }
    console.log("Setting overlap to ", desired_overlap);
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

    link  = document.getElementById('overlap')
    linkmm = document.getElementById('overlapmm')
    if (gui_is_metric) {
        link.value = desired_overlap;
        linkmm.innerHTML = "mm";
    } else {
        link.value = mm_to_inch(desired_overlap);
        linkmm.innerHTML = "inch";
    }
}


export function handle_metric(val)
{
    if (val == "imperial") {
        gui_is_metric = 0;
    } 
    if (val == "metric") {
        gui_is_metric = 1;
    } 
    update_gui_dimensions();
}

document.getElementById('gcodefiles').onchange = handle_gcode;

/* Glue to handle html vs javascript modules */
window.handle_metric = handle_metric;
window.handle_width = handle_width;
window.handle_height = handle_height;
window.handle_overlap = handle_overlap;

