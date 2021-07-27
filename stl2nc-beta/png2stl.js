/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/

import * as gcodetile from './gcodetile.js';
"use strict";


let pixeldivider = 256.0;

function get_pixel_value(image, X, Y)
{
  return image.bitmap[x + y * image.width] / pixeldivider;
}

function png_loaded(evt)
{
  console.log("PNG loaded" + evt);
  
  let image = document.getElementById("image");
  image.src = evt.target.result;
  var canvas = document.createElement('canvas');
  canvas.width = image.width;
  canvas.height = image.height;
  canvas.getContext('2d').drawImage(image, 0, 0, image.width, image.height);  
  let pixeldata = canvas.getContext('2d').getImageData(0, 0, image.width, image.height);
  console.log(pixeldata);
}


function basename(path) 
{
     return path.replace(/.*\//, '');
}

export function handle_png(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
        let fn = basename(f.name);
        if (fn != "" && fn.includes(".nc")) {
                filename = fn;
        }
        var reader = new FileReader();
        reader.onload = png_loaded;
        reader.readAsDataURL(f);
    }
}

document.getElementById('pngfiles').onchange = handle_png;

