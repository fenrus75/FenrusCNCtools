========================
NCOPT javascript library
========================

The library is currently a single file ncopt.js


Structure
---------

The library uses a group of global variables to handle input, processing and 
output of the modified gcode

:doc:`ncopt_implementation/globals`

Invocation
----------

The processing logic is called by process_data

.. js:autofunction:: process_data

In outline the code;

* loads the supplied gcode and splits it into the lines array

* initialises the global state variables

* performs a dry-run through the gcode to ?

* performs the production run through the code to optimise it


Some example JSDoc extracted autofunction docs
----------------------------------------------

The JSDoc strings have been updated a little to support autofunction docs in Sphinx

.. js:autofunction:: allocate_2D_array

.. js:autofunction:: emit_output

.. js:autofunction:: G0

