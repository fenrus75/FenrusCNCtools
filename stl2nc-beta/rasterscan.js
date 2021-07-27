import * as tool from './tool.js';
import * as gcode from './gcode.js';
import * as stl from './stl.js';
import * as segment from './segment.js';

let ACC = 50.0;

let precision = 1.0;

let roughing_endmill = 0;
let finishing_endmill = 0;
let split_gcode = 0;



function update_height(height, X, Y, offset)
{

    let prevheight = height;
    let newheight = stl.get_height(X, Y, height, offset) + offset;
    
    
    if (newheight > prevheight) {
//        newheight = Math.ceil(newheight*ACC)/ACC
        height = newheight;
    }

    return height;


//    return Math.max(height, get_height(X, Y) + offset);

}

function update_height_array(height, rr, X, Y, arr,  offset)
{

    let prevheight = height;
    let newheight = stl.get_height_array(X - rr,  Y - rr, X + rr, Y + rr, X, Y, arr, height, offset) + offset;
    
    newheight = Math.ceil(newheight*ACC)/ACC
    
    if (newheight > prevheight) {
        height = newheight;
    }

    return height;
}


let first = 1;

/*
 * Given the current tool, find how low the tool can go before it hits the model.
 */
function get_height_tool(X, Y, maxR)
{	
	let d = -stl.get_work_depth(), dorg;
	let balloffset = 0.0;

	d = update_height(d, X, Y, 0);
	
	const ringcount = tool.tool_library[tool.tool_index].rings.length;
	for (let i = 0; i < ringcount ; i++) {
	    let rr = tool.tool_library[tool.tool_index].rings[i].R;
	    if (rr > maxR) { 
	        continue;
            }
	    balloffset = -tool.geometry_at_distance(rr);
	    d = update_height_array(d, rr, X, Y, tool.tool_library[tool.tool_index].rings[i].points,  balloffset);
	}
	first = 0;
	return Math.ceil(d*ACC)/ACC;

}



/* last percentage the gui was updated */
let prev_pct = 0;

/* 
          zeg
      --------->
     ^          |
 zig |          | zag
     |          + 
     <-----------
          zog

*/

let roughing_overcut = 1.5;

/* one raster direction for roughing, front to back */
function roughing_zig(X, startY, endY, deltaY)
{
        let Y = startY;
        
        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;;
        
        while (Y <= endY) {
            /* for roughing we look roughing_overcutx the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;
            /* 45 degree max ramp down */
            if (Z - prevZ < -deltaY) {
                Z = prevZ + prevY - Y;
            }

            segment.push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
            prevY = Y;
            prevZ = Z;
            
            if (Y == endY) 
            {
                break;
            }
            Y = Y + deltaY;
            if (Y > endY) 
            {
                Y = endY;
            }
        }    
}

function roughing_zag(X, startY, endY, deltaY)
{
        let Y = startY;

        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;;
        
        while (Y >= endY) {
            /* for roughing we look roughing_overcutx the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;
            /* 45 degree max ramp down */
            if (Z - prevZ < -deltaY) {
                Z = prevZ - deltaY;
            }

            segment.push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
            prevY = Y;
            prevZ = Z;
            
            if (Y == endY) 
            {
                break;
            }
            Y = Y - deltaY;
            if (Y < endY) 
            {
                Y = endY;
            }
        }    
}

function roughing_zeg(Y, startX, endX, deltaX)
{
        let X = startX;


        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;;
        
        while (X <= endX) {
            /* for roughing we look roughing_overcutx the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;
            /* 45 degree max ramp down */
            if (Z - prevZ < -deltaX) {
                Z = prevZ - deltaX;
            }

            segment.push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
            prevX = X;
            prevZ = Z;
            
            if (X == endX) 
            {
                break;
            }
            X = X + deltaX;
            if (X > endX) 
            {
                X = endX;
            }
        }    
}

function roughing_zog(Y, startX, endX, deltaX)
{
        let X = startX;

        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;;
        
        while (X >= endX) {
            /* for roughing we look roughing_overcutx the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, roughing_overcut * tool.radius()) + tool.tool_stock_to_leave;
            /* 45 degree max ramp down */
            if (Z - prevZ < -deltaX) {
                Z = prevZ - deltaX;
            }

            segment.push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
            prevX = X;
            prevZ = Z;
            
            if (X == endX) 
            {
                break;
            }
            X = X - deltaX;
            if (X < endX) 
            {
                X = endX;
            }
        }    
}




/* 
 * create a "cutout" toolpath around the model
 *
 * This is done to give the finishing endmill space to manouver in safely outside of the model
 *
 * box1 leaves a little "tab" behind to help work holding.
 * box2 cuts away this tab
 */

function cutout_box1()
{
    let minX = -tool.radius();
    let minY = -tool.radius();
    let maxX = stl.get_work_width() + tool.radius();
    let maxY = stl.get_work_height() + tool.radius();
    
    let maxZ = -stl.get_work_depth() + tool.tool_depth_of_cut * 0.5;
    
    console.log("BOX1");
    
    segment.push_segment_multilevel(minX, minY, maxZ, minX, maxY, maxZ, tool.tool_diameter / 1.9);
    segment.push_segment_multilevel(minX, maxY, maxZ, maxX, maxY, maxZ, tool.tool_diameter / 1.9);
    segment.push_segment_multilevel(maxX, maxY, maxZ, maxX, minY, maxZ, tool.tool_diameter / 1.9);
    segment.push_segment_multilevel(maxX, minY, maxZ, minX, minY, maxZ, tool.tool_diameter / 1.9);
    console.log("BOX1 done");
}

function cutout_box2()
{
    let minX = -tool.radius();
    let minY = -tool.radius();
    let maxX = stl.get_work_width() + tool.radius();
    let maxY = stl.get_work_height() + tool.radius();
    
    let maxZ = -stl.get_work_depth();
    
    segment.push_segment(minX, minY, maxZ, minX, maxY, maxZ, 0);
    segment.push_segment(minX, maxY, maxZ, maxX, maxY, maxZ, 0);
    segment.push_segment(maxX, maxY, maxZ, maxX, minY, maxZ, 0);
    segment.push_segment(maxX, minY, maxZ, minX, minY, maxZ, 0);
}

function reset_progress_bars()
{
    prev_pct = 0;
    prev_pct2 = 0;
    var elem = document.getElementById("BarRoughing");
    elem.style.width = "0%";
    var elem = document.getElementById("BarFinishing");
    elem.style.width = "0%";

}


/* roughing pass */
function roughing_zig_zag(_tool, width, height)
{
    tool.select_tool(_tool);
    setTimeout(gcode.gcode_change_tool, 0, _tool);
    
    setTimeout(reset_progress_bars, 0);
    
    
    let woc = tool.woc_from_chipload(_tool);

    
    if (_tool == 0) {
        return;
    }
    
    console.log("roughing tool diameter is ", tool.tool_diameter);
    console.log("wxh ", width, " ", height)
    
    let stepsize = tool.tool_diameter / 8;
    if (woc > 0) {
        stepsize = woc;
    }
    console.log("Stepsize is ", stepsize, "mm");
    
    let midX = width / 2;
    let midY = height / 2;
    
    let offsetX = width/2 + tool.tool_diameter/1.75;
    let offsetY = height/2 + tool.tool_diameter/1.75;
     
    
    let deltaXY = tool.tool_diameter / 10;
    if (deltaXY > 0.5) {
        deltaXY = 0.5;
    }

//    setTimeout(cutout_box1, 0);    
    
    while (offsetX > 0 && offsetY > 0) {

        setTimeout(roughing_zig, 0, midX - offsetX, midY - offsetY - stepsize, midY + offsetY, deltaXY);
        setTimeout(roughing_zeg, 0, midY + offsetY, midX - offsetX, midX + offsetX, deltaXY);
        setTimeout(roughing_zag, 0, midX + offsetX, midY + offsetY, midY - offsetY, deltaXY);
        setTimeout(roughing_zog, 0, midY - offsetY, midX + offsetX, midX - offsetX + stepsize, deltaXY);
        
        if (offsetX == 0) { break; };
        if (offsetY == 0) { break; };
        
        offsetX -= stepsize;
        offsetY -= stepsize;
        if (offsetX < 0) { offsetX = 0;};
        if (offsetY < 0) { offsetY = 0;};
        
    }
    
    setTimeout(segment.segments_to_gcode, 0, 50);
//    setTimeout(cutout_box2, 0);    
    setTimeout(segment.segments_to_gcode, 0);
    
}

let halfway_counter = 0;
/* left-to-right finishing pass */
function finishing_zig(Y, deltaX)
{
        let X = -tool.radius();
        let maxX = stl.get_work_width() + tool.radius();
        let threshold = 0.6;
        if (precision != 0) {
            threshold = threshold / precision;
        }
        
        let prevX = X;
        let prevY = Y;
        let prevZ = get_height_tool(X, Y, tool.radius());
        
//        gcode_travel_to(X, 0);
        while (X <= maxX) {
            let Z = get_height_tool(X, Y, tool.radius());
            
            if (Math.abs(prevZ - Z) > threshold) {
                halfway_counter += 1;
                let halfX = (X + prevX) / 2;
                let halfZ = get_height_tool(halfX, Y, tool.radius());
                segment.push_segment(prevX, prevY, prevZ, halfX, Y, halfZ, 0, 5);
                prevX = halfX;
                prevZ = halfZ;
            } 
            
            segment.push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
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

/* right to left finishing pass */
function finishing_zag(Y, deltaX)
{
        let X = stl.get_work_width() + tool.radius();
        let minX = -tool.radius();
        
        let newZ = get_height_tool(X, Y, tool.radius()) ;
        let prevY = Y;
        let prevX = stl.get_work_width();
        
        let prevZ = newZ;
        let threshold = 0.6;
        if (precision != 0) {
            threshold = threshold / precision
        }
        
//        gcode_travel_to(X, 0);
        while (X >= minX) {
            /* for roughing we look 2x the tool diameter as a stock-to-leave measure */
            let Z = get_height_tool(X, Y, tool.radius());
            if (Math.abs(prevZ - Z) > threshold) {
                halfway_counter += 1;
                let halfX = (X + prevX) / 2;
                let halfZ = get_height_tool(halfX, Y, tool.radius());
                segment.push_segment(prevX, prevY, prevZ, halfX, Y, halfZ, 0, 5);
                prevX = halfX;
                prevZ = halfZ;
            } 

            segment.push_segment(prevX, prevY, prevZ, X, Y, Z, 0, 5);
            
            
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
        let pct = (Math.round(Y/stl.get_work_height() * 100));
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


/* Finishing pass */
function finishing_zig_zag(_tool)
{

    setTimeout(gcode.gcode_change_tool, 0, _tool);
    tool.select_tool(_tool);
    
    if (_tool == 0) {
        return;
    }
    
        
    let minY = -tool.radius();
    let maxY = stl.get_work_height() + tool.radius();
    
    
    let deltaX = tool.tool_diameter / 10;
    let deltaY = tool.tool_diameter / 10;
    let Y = minY;
    
    deltaX = tool.tool_finishing_stepover;
    deltaY = tool.tool_finishing_stepover;
    
    if (precision > 0) {
        deltaX = deltaX / precision;
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
    
    
    setTimeout(segment.segments_to_gcode_quick, 0);
}

let startdate;

/* The main director function, this takes a design-in-buckets and goes all the way to gcode */
export function calculate_gcode(filename, desired_width, desired_height) 
{
    halfway_counter = 0;
    tool.tool_factory();
        
    startdate = Date.now();
    gcode.gcode_header(filename);    

    roughing_zig_zag(roughing_endmill, desired_width, desired_height);
    
    if (split_gcode == 1) {
        setTimeout(gcode.gcode_footer, 0);
        setTimeout(gcode.gcode_header, 0);
    }

    finishing_zig_zag(finishing_endmill);

    setTimeout(gcode.gcode_footer, 0);
    setTimeout(gcode.update_gcode_on_website, 0, filename);
    
}

export function set_precision(p)
{
    precision = p;
    tool.set_precision(p);
    ACC = 50 * p;
}

export function set_roughing_endmill(m)
{
    roughing_endmill = Math.floor(parseFloat(m));
    console.log("Roughing endmill is ", roughing_endmill);
}
export function set_finishing_endmill(m)
{
    finishing_endmill = Math.floor(parseFloat(m));
    tool.select_tool(finishing_endmill);
    tool.set_stepover(tool.tool_diameter / 10);
}

export function set_split_gcode(split)
{
    split_gcode = split;
    gcode.set_split_gcode(split);
}