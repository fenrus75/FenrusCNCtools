This code turns an SVG file into a gcode toolpath where it basically area
clears inside the shapes

NOTE: The SVG parser is extremely limited and only really works on SVG files
exported from Carbide Create. Over time I'll work on expanding the SVG
import capability

This is very early stage software and really only meant to experiment with
toolpath generation techniques etc etc... so meant for prototyping &
research not production


Requirements
------------
The tool is developed on Linux. The main external requirement is CGAL
(libcgal-dev or equivalent)



Todo list (what I know is wrong or what I want to add)
------------------------------------------------------

Things to fix
* SVG parser is extremely hard coded/limited; using a library can improve
  this but will add another dependency


Things to experiment with
* The slotting toolpaths should likely be full speed at half depth instead
* An "edge only" toolpath at half depth to reduce fuzz at the edges of the
  cut



Done list
---------
* Units are a mess internally. APIs are often in imperial inches,
  internal datastructures are in PX but the gcode side is in milimeters.
  --> Over time converge on using MM everywhere with helpers for conversion
