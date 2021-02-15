/*
    Copyright (C) 2020 -- Arjan van de Ven
    
    Licensed under the terms of the GPL-3.0 license
*/


let filename = "output.nc";

let filenames = []
let filecontent = []


let add_go_to_zero = 0;

function process_data(data)
{
    let lines = data.split("\n")
    let len = lines.length;
    
    let toolchanges = 0;
    let phase = 0
    
    let header = ""
    let footer = ""
    
    console.log("Data is " + lines[0] + " size  " + lines.length);
    
    start = Date.now();
    
    console.log("Start of parsing at " + (Date.now() - start));

    /* first a quick dryrun to find the header and footer text*/
    dryrun = 1;    
    for (let i = 0; i < len; i++) {
    	line = lines[i];
    	
        /* weird preproc issue in CC where it does an S without an M3 */
    	if (line[0] == "S" && ! line.includes("M3")) {
    	    line = "M3" + line;
    	}
    	
    	if (line == "M05") {
    		phase = 1;
    		toolchanges = toolchanges + 1;
    		filecontent[toolchanges] = "";
	}
	if (line == "M02") {
	        phase = 2;
        }
        
        if (line.includes("M06") || line.includes("M6") || line.includes("M0 ")) {
            let index = line.indexOf("T");
            let bit = "";
            if (line.includes("M0 ")) {
    		toolchanges = toolchanges + 1;
    		filecontent[toolchanges] = "";
            }
            if (index >= 0) {
                bit = line.substring(index + 1);
                console.log("TOOL CHANGE" , line, " to bit ", bit);
                filenames[toolchanges] = filename + "-" + toolchanges.toString() + "-" + bit + ".nc";
            }
                
        }
        
        if (phase == 0) {
            header = header + line + "\n";
        }
        
        if (phase == 1) {
             filecontent[toolchanges] = filecontent[toolchanges] + line + "\n";
        }
        
        if (phase == 2) {
            footer = footer + line + "\n";
        }
    }
    
    let zero = "";
    if (add_go_to_zero) {
        zero = "G0X0Y0\n";
    }
    
    for (let i = 0; i <= toolchanges; i++) {
            if (typeof(filenames[i]) == "undefined") {
                console.log("NO FILENAME ", i);
                continue;
            };
            console.log("Length at ", i, " is ", filecontent[i].length);
            var link = document.getElementById('download' + i.toString())
            link.innerHTML = 'Download ' + filenames[i];
            link.href = "#";
            link.download = filenames[i];
            link.href = "data:text/plain;base64," + btoa(header + filecontent[i] + zero + footer);
    }

    console.log("Number of tool changes " + toolchanges);
    console.log("End of parsing at " + (Date.now() - start));
    console.log("Size of header ", header.length);
    console.log("Size of footer ", footer.length);
}


function load(evt) 
{
    var start;
    start =  Date.now();
    if (evt.target.readyState == FileReader.DONE) {
        process_data(evt.target.result); console.log("End of data processing " + (Date.now() - start));
    }    
}

function basename(path) {
     return path.replace(/.*\//, '');
}

function handle(e) 
{
    var files = this.files;
    for (var i = 0, f; f = files[i]; i++) {
    	fn = basename(f.name);
    	if (fn != "" && fn.includes(".nc")) {
    		filename = fn.replace(/\.nc$/,"");
    	}
        var reader = new FileReader();
        reader.onloadend = load;
        reader.readAsText(f);
    }
}

function Speedlimit(value)
{
    let newlimit = parseFloat(value);
    
    if (newlimit > 0 && newlimit < 500) {
    	speedlimit = newlimit;
    }
}

function set_zero(value)
{
    if (value) {
        add_go_to_zero = 1;
    } else {
        add_go_to_zero = 0;
    }
}


document.getElementById('files').onchange = handle;
window.set_zero = set_zero;
