import * as stl from './stl.js';



let global_maxY = 0;
let global_maxX = 0;
let pixelsize = 0.0;

function  calculate_line(data, Y, elem, context, pixels, outputfilename) 
{
    let zdiv = 0.0;
    
    zdiv = stl.get_work_depth();
    for (let X = 0; X < global_maxX; X++) {
            let d = 1 + stl.get_height(pixelsize * (X + 0.5), pixelsize * (Y + 0.5)) / zdiv;
            let c = 255 * d;
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
        var link = document.getElementById('downloadpng')
        link.innerHTML = 'Download image ' + outputfilename + ".png";
        link.href = "#";
        link.download = outputfilename + ".png";
        link.href = canvas.toDataURL('image/png');
    }
}

export function calculate_png_image(filename) 
{
    let canvas = document.querySelector('canvas');
    let context = canvas.getContext('2d');
    
    
    
    let ratio = stl.get_work_width() / stl.get_work_height();
    
    if (ratio >= 1) {
        global_maxX = 1024;
        global_maxY = 1024 / ratio;
    } else {
        global_maxX = 1024 * ratio;
        global_maxY = 1024;
    }
    pixelsize = Math.max(stl.get_work_width(), stl.get_work_height()) / 1024;
    
    
    global_maxX = Math.ceil(global_maxX);
    global_maxY = Math.ceil(global_maxY);
    canvas.width = global_maxX;
    canvas.height = global_maxY;
    
    console.log("PNG is ", global_maxX, "x", global_maxY, " while pixelsize is ", pixelsize);
    
    let pixels = context.getImageData(
        0, 0, global_maxX, global_maxY
      );
      
    let elem = document.getElementById("BarPNG");
      
    let data = pixels.data;
    for (let Y = 0; Y < global_maxY; Y++) {
//            calculate_line(data, Y, elem, context, pixels);
        setTimeout(calculate_line, 0, data, Y, elem, context, pixels, filename);
    }
    context.putImageData(pixels, 0, 0);
}
