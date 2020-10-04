NCOPT Global Variables
======================

We cannot currently autodoc these as sphinx_js only extracts 
functions and classes automatically


State trackers
--------------

It is necessary to track the bounding box of where the gcode goes, this is handled by;
global_maxX, global_maxY, global_maxZ

A secondary set tries to track only places where moves happen 
to distinguish the height where the final retract happens into
global_maxZlevel
global_minX, global_minY, global_minZ


Volume Tracking
---------------

To make safe optimisations it is necessary to know where the cutter has been.
The space is represented by the array2D state variables.

We have a 2D array, indexed by X,Y in work coordinates, in blocks of 5mm x 5mm..

This 2D array contains a list of points that got visited in each of these blocks.

This is used to quickly look up if a point previously was visited and what the Z was
in the previous visit.

xoffset
yoffset
array_xmax
array_ymax
array2D

GCode Stateful Tracking
-----------------------

GCode G is a stateful command set, 
where we need to track both the level of the G code as well as X Y Z and F
since commands we read or write are incremental.

These include;
currentx, currenty, currentz, currentf
glevel


Output GCode
------------

Output gcode is collected into the outputtext (array?)


Enable Flags and Counters
-------------------------

Feature enable flags and some feature action counters are maintained such as;
optimization_retract

count_retract

