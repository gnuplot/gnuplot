This is gnuplot version 5.4 -- binary distribution for Windows
===========================================================================

gnuplot is a command-line driven interactive function plotting utility
for Linux, OSX, Windows, VMS, and many other platforms.  The software is
copyrighted but freely distributed (i.e., you don't have to pay for it).
It was originally intended as graphical program to allow scientists
and students to visualize mathematical functions and data.

gnuplot handles both curves (2 dimensions) and surfaces (3 dimensions).
Surfaces can be plotted as a mesh fitting the specified function, floating
in the 3-d coordinate space, or as a contour plot on the x-y plane.
For 2-d plots, there are also many plot styles including lines, points,
boxes, heat maps, stacked histograms, and contoured projections of 3D data.
Graphs may be labeled with arbitrary labels and arrows, axis labels,
a title, date and time, and a key.


Getting started
---------------

The new gnuplot user should begin by reading the general information
available by typing `help` after running gnuplot. Then read about the
`plot` command (type `help plot`).  The manual for gnuplot (which is a
nicely formatted version of the on-line help information) is available
as a PDF document.

You can find loads of test and sample scripts in the 'demo' directory.
Try executing `test` and `load "all.dem"` or have a look at the online
version of the demos at
  http://www.gnuplot.info/screenshots/index.html#demos


License
-------

See the Copyright file for copyright conditions.

The "GNU" in gnuplot is NOT related to the Free Software Foundation,
the naming is just a coincidence (and a long story; see the gnuplot FAQ
for details). Thus gnuplot is not covered by the GPL (GNU Public License)
copyleft, but rather by its own copyright statement, included in all source
code files. However, some of the associated drivers and support utilities
are dual-licensed.


gnuplot binaries
----------------

* wgnuplot.exe:  GUI version and the default gnuplot executable.

* gnuplot.exe:  Text (console) mode version of the gnuplot executable with full
  pipe functionality as on other platforms. In contrast to wgnuplot.exe, this
  program can also accept commands on stdin (standard input) and print messages
  on stdout (standard output). It replaces a program pgnuplot.exe used by some
  earlier gnuplot versions.  gnuplot.exe can be used as a graph engine by 
  3rd party applications like Octave (www.octave.org).

* runtime library files
  Runtime library files (e.g. libfreetype-6.dll) that are required for gnuplot
  are included in the package.  Licenses of these runtime libraries can be
  found in the 'license' directory.


Interactive Terminals
---------------------

gnuplot on Windows offers three different interactive terminal types:  windows,
wxt, and qt.  The later two are also available on other platforms.  All three
produce high quality output using antialiasing and oversampling with hinting
and support all modern gnuplot features.  Differences can be found in the
export formats available for saving and clipboard, printing support and the
behaviour in persist mode.  Also wxt uses the same drawing code as the pngcairo
and pdfcairo terminals, which allows for non-interactive saving of graphs.  The
windows terminal's graph windows can be docked to the wgnuplot text window.

By default, gnuplot on Windows will use the wxt terminal.  If you prefer, you
can change this by setting the GNUTERM environment variable.  See below on how
to change environment variables. Alternatively, you can add
    set term windows / wxt / qt
to your gnuplot.ini, see `help startup`.


Installation
------------

gnuplot comes with its own installer, which will basically do the following,
provided you check the corresponding options:

* Extract this package (or parts thereof) in a directory of your choice, e.g.
  C:\Program Files\gnuplot etc.

* Create shortcut icons to wgnuplot on your desktop.  Additionally, a menu is
  added to the startup menu with links to the programs, help and documentation,
  gnuplot's internet site and the demo scripts.

* The extensions *.gp *.gpl *.plt will be associated with application wgnuplot.

* The path to the gnuplot binaries is added to the PATH environment variable.
  That way you can start gnuplot by typing `gnuplot' or `wgnuplot' on a command
  line.

* gnuplot is added to the shortcuts of the Windows explorer "Run" Dialog.
  To start wgnuplot simply press Windows-R and execute `wgnuplot'.

* You may select your default terminal of preference (wxt/windows/qt) and the
  installer will update the GNUTERM environment variable accordingly.
  Alternatively, you can later add
    set term windows
  or
    set term wxt
  to your gnuplot.ini, see `help startup`.

* If you install the demo scripts, the directory containing the demos is
  included in the GNUPLOT_LIB search path, see below.

Customisation:
On startup, gnuplot executes the gnuplot.ini script from the user's
application data directory %APPDATA% if found, see `help startup`. The wgnuplot
text window and the windows terminal load and save settings from/to wgnuplot.ini
located in the appdata directory, see `help wgnuplot.ini`.


Fonts
-----

graphical text window (wgnuplot.exe):
  You can change the font of the terminal window by selecting "Options..." -
  "Choose Font..." via the toolbar or the context (right-click) menu. We
  strongly encourage you to use a modern outline font like e.g. "Consolas"
  instead of the old "Terminal" raster font, which was the default until
  gnuplot version 4.4. Make sure to "Update wgnuplot.ini" to make this change
  permanent.

console window (gnuplot.exe):
  If extended characters do not display correctly you might have to change
  the console font to a non-raster type like e.g. "Consolas" or "Lucida
  Console". You can do this using the "Properties" dialog of the console
  window.


Encodings
---------

On Windows, gnuplot version 5.2 or later support command line input using all
encodings supported by gnuplot, including UTF-8, see `help encoding`.
By default gnuplot will use an encoding which matches the system's ANSI codepage,
if supported.  We recommend to add `set encoding utf8` to your gnuplot.ini file,
see below.  Note that while Unicode input on the command line is limited to the
Basic Multilingual Plane (BMP), scripts may contain all characters (as in
previous versions).


Localisation
------------

gnuplot supports localised versions of the menu and help files. By default,
gnuplot tries to load wgnuplot-XX.mnu and wgnuplot-XX.chm, where XX is a two
character language code. Currently, only English (default) and Japanese (ja)
are supported, but you are invited to contribute.

You can enforce a certain language by adding
  Language=XX
to your wgnuplot.ini. This file is located in your %APPDATA% directory. If you
would like to have mixed settings, e.g., English menus but Japanese help texts,
you could add the following statements to your wgnuplot.ini:
  HelpFile=wgnuplot-ja.chm
  MenuFile=wgnuplot.mnu

Please note that currently there's no way to change the language setting from
within gnuplot.


Environmental variables
-----------------------

For a list of environment variables supported, type `help environment`
in gnuplot. Below, we list some important ones:

* If GNUTERM is defined, it is used as the name of the terminal type to be
  used. This overrides any terminal type sensed by gnuplot on start-up, but is
  itself overridden by the gnuplot.ini start-up file (see `help startup`) and,
  of course, by later explicit changes.

* Variable GNUPLOT_LIB may be used to define additional search directories for
  data and command files. The variable may contain a single directory name, or
  a list of directories separated by a path separator ';'. The contents of
  GNUPLOT_LIB are appended to the `loadpath`, but not saved with the `save`
  or `save set` commands. See 'help loadpath' for more details.


Known bugs
----------

Please see and use

    http://sourceforge.net/p/gnuplot/bugs/

for an up-to-date bug tracking system.

--------------------------------------------------------------------------------

The gnuplot team, January 2022
