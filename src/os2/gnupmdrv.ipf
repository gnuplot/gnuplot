:userdoc.
:docprof toc=12.

:h1 res=5. Extended Help
:p.This program is a display interface for Gnuplot.
:p.When it starts up, it spawns a new session which contains 
the :hp2.GNUPLOT:ehp2. program. This new session provides the usual
Gnuplot command line.

:h1 res=103.  Help Menu Help
:i1 id=mabout. Help menu
:i2 refid=mabout. About 
:p.The :hp2.About:ehp2. menu item displays the About box, which
just identifies the program.

:h1 res=100. Options Menu Help
:i1 id=mopt. Options menu
:i2 refid=mopt. The options menu
:p.The :hp2.Options:ehp2. menu enables you to change various options on the
displayed plot, and to control printing of the plot via the OS/2 print
system.

:h1 res=104. Options Menu Help
:i2 refid=mopt. Fonts 
:p.The :hp2.Fonts:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu enables you to change the font used in the displayed plot. 
:p.You can also 'drag and drop' a font from a Font Palette onto the Gnushell
window.

:h1 res=101. Options Menu Help
:i2 refid=mopt. Print
:p.The :hp2.Print:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu enables you to print the current window on the default
printer.

:h1 res=115. Options Menu Help
:i2 refid=mopt. Printers
:p.The :hp2.Printers:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu enables you to select the printer to which
output is directed.

:h1 res=207. Options Menu Help
:i2 refid=mopt. Linetype option
:p.Selecting the :hp2.Lines:ehp2. menu item from the :hp2.Options:ehp2.
enables you to choose various options for the lines used for the plots.
:p.Selecting the :hp2.Thick:ehp2. option of the :hp2.Lines:ehp2.
menu item toggles the :hp2.thick line:ehp2. option on and off.
The selection is active if the menu item is checked.
:p.Selecting the :hp2.Solid:ehp2. option of the :hp2.Lines:ehp2.
menu item toggles the :hp2.solid line:ehp2. option on and off.
The selection is active if the menu item is checked.
:p.If the :hp2.Solid:ehp2. option is not active, curves on graphs will be
plotted in various styles of broken lines. 

:h1 res=208. Options Menu Help
:i2 refid=mopt. Thick line option
:p.Selecting the :hp2.Thick:ehp2. option of the :hp2.Lines:ehp2.
menu item toggles the :hp2.thick line:ehp2. option on and off.
The selection is active if the menu item is checked.
:p.The thick line option can give better output on high-resolution
devices like laser printers.

:h1 res=209. Options Menu Help
:i2 refid=mopt. Solid lines option
:p.Selecting the :hp2.Solid:ehp2. option of the :hp2.Lines:ehp2.
menu item toggles the :hp2.solid line:ehp2. option on and off.
The selection is active if the menu item is checked.
:p.If the :hp2.Solid:ehp2. option is not active, curves on graphs will be
plotted in various styles of broken lines. 
:p.This option can be combined with the :hp2.Colours.:ehp2. option. 
When a plot is printed on a printer that does not support colour, the 
:hp2.Solid:ehp2. option is disabled.

:h1 res=206. Options Menu Help
:i2 refid=mopt. Colours
:p.Selecting the :hp2.Colours:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu causes lines used for graphs to be plotted in various
colours. This is the default option for plotting on the screen.
It can be combined with the :hp2.Lines:ehp2. option. 

:h1 res=120. Options Menu Help
:i2 refid=mopt. Pause option
:p.Selecting the :hp2.Pause mode:ehp2. menu item from the :hp2.Options:ehp2.
menu enables you to choose how the Gnuplot 'pause' command is handled.
:p.Selecting the :hp2.Dialog box:ehp2. menu item from the :hp2.Pause options:ehp2.
menu causes the Gnuplot 'pause' command to print a message in a dialog box,
and wait for you to end the dialog before continuing.

:h1 res=121. Options Menu Help
:i2 refid=mopt. Pause with dialog box
:p.Selecting the :hp2.Dialog box:ehp2. menu item from the :hp2.Pause options:ehp2.
menu causes the Gnuplot 'pause' command to print a message in a dialog box,
and wait for you to end the dialog before continuing.
:p.Selecting the :hp2.Menu bar:ehp2. menu item from the :hp2.Pause options:ehp2.
menu causes the Gnuplot 'pause' command to enable the :hp2.Continue:ehp2.
menu item. 
:p.Selecting the :hp2.Gnuplot:ehp2. menu item from the :hp2.Pause options:ehp2.
menu causes the Gnuplot 'pause' command to be handled by the Gnuplot
program.

:h1 res=122. Options Menu Help
:i2 refid=mopt. Pause with menu item
:p.Selecting the :hp2.Menu bar:ehp2. menu item from the :hp2.Pause options:ehp2.
menu causes the Gnuplot 'pause' command to enable the :hp2.Continue:ehp2.
menu item. 
:p.Plotting will be resumed when this item is selected.
:p.Any text message is ignored.

:h1 res=123. Options Menu Help
:i2 refid=mopt. Pause in Gnuplot
:p.Selecting the :hp2.Gnuplot:ehp2. menu item from the :hp2.Pause options:ehp2.
menu causes the Gnuplot 'pause' command to be handled by the Gnuplot
program.
:p.In order to resume plotting, you will have to select the Gnuplot command line
window, and press the enter key.
 

:h1 res=105. Options Menu Help
:i2 refid=mopt. gnushell.ini
:i2 refid=mopt. Save settings
:p.Selecting the :hp2.Save settings:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu causes the current line, colour and font options to be saved.
The positions and sizes of the windows are also saved.
:p.The data is saved in the file :hp2.gnushell.ini:ehp2. file in the 
program&csq.s working directory. You can delete the file if you want to restore
all settings to their default values. (This file is created even if
no settings are saved.) 

:h1 res=210. Options Menu Help
:i2 refid=mopt. Pop to front
:p.Selecting the :hp2.Pop to front:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu causes the plot window to pop to the front of the
window stack each time a graph is plotted. If this item is unchecked,
the user will have to bring the window to the front by manual selection.

:h1 res=211. Options Menu Help
:i2 refid=mopt. Keep aspect ratio
:p.If this menu is checked, then the sides of the plot keep the aspect ratio 
of 1.56 thus filling only partially the plotting window. 
If it is unchecked, then the plot occupies the whole window.

:h1 res=500. Edit Menu Help
:i1 id=medit. Edit menu
:i2 refid=medit. The Edit menu
:p.The :hp2.Edit:ehp2. menu gives you access to commands 
for copying the plot to the clipboard.

:h1 res=501. Edit Menu Help
:i2 refid=medit. Copy to clipboard
:p.Selecting the :hp2.Copy:ehp2. menu item from the :hp2.Edit:ehp2.
pulldown menu causes the current plot to be copied to the system
clipboard. The plot is copied in bitmap and in metafile format.

:h1 res=504. Edit Menu Help
:i2 refid=medit. Clear clipboard
:p.Selecting the :hp2.Clear clipboard:ehp2. menu item from the :hp2.Edit:ehp2.pulldown menu causes the clipboard to be cleared.
:h1 res=300. Gnuplot Menu Help
:i1 id=mgnu. Gnuplot menu
:i2 refid=mgnu. Moving to GNUPLOT window
:p.Selecting the :hp2.Gnuplot:ehp2. menu item causes the GNUPLOT
command window to be brought to the foreground. The same result can be be
obtained by pressing the ESC key when the Gnushell window is active.

:h1 res=600. Mouse Menu Help
:i1 id=mmouse. Mouse menu
:i2 refid=mmouse. The Mouse menu
:p.The :hp2.Mouse:ehp2. menu gives you access to mouse (pointer)-related 
functions.

:h1 res=601. Use Mouse Menu Help
:i2 refid=mmouse. Use mouse
:p.Checking this menu item enables the mouse (pointer device) functionality:
tracing the position over graph, zooming, annotating graph etc. for 2d graphs
and for maps (i.e. `set view` with z-rotation 0,90,180,270 or 360 degrees).
Mousing is not available in multiplot mode. 
Except for the functions available from the menu, 
mouse buttons have the following functions:
:p.:hp2.MB2:ehp2. presents the zoom rectangle. Press :hp2.MB1:ehp2. to force 
zoom, or :hp2.Esc:ehp2. to cancel zooming. Zooming can be canceled if 
pressed <Esc> or at least one of the chosen sizes is smaller than 8 pixels.
You can return to the original values of ranges by choosing the 
:hp2.Unzoom:ehp2. menu option.
:p.:hp2.Double click of MB1:ehp2. writes the current pointer position to
clipbord according to the format chosen in :hp2.sprintf format:ehp2. menu.
:p.:hp2.MB3:ehp2. annotates temporarily the graph.

:h1 res=602. Mouse Coordinates Menu Help
:i2 refid=mmouse. Coordinates
:p.Choose the coordinates which are used for showing the mouse position, 
clipboard copy and annotation. 
:p.:hp2.Real coordinates:ehp2. are coordinates of x and y axes of the current 
graph.
:p.:hp2.Screen coordinates:ehp2. are relative coordinates of the screen, 
i.e. [0,0]..[1,1]. These may be used in gnuplot commands like 
:hp2.set label "ahoj" at screen 0.85,0.85:ehp2.
:p.:hp2.Pixel coordinates:ehp2. are the coordinates of the window depending 
on the screen resolution. They determine the precision of the other 
coordinates.

:h1 res=603. Sprintf Format Menu Help
:i2 refid=mmouse. sprintf format
:p.Choose here the format for writing the cursor position into clipboard
(via double click of MB1).

:h1 res=604. Polar Distance Menu Help
:i2 refid=mmouse. Polar
:p.If this menu item is checked, then the distance between the ruler and 
mouse cursor is printed also in polar coordinates. This is particularly 
useful for dealing with peaks or other objects in maps. Disabled if x or y
axis is logarithmic. 

:h1 res=605. Zoom Menu Help
:i2 refid=mmouse. Zoom
:p.Well, this menu item does nothing just remembers you to use MB2 to 
start zooming. So click (=press and release) MB2, move your mouse 
left and up to select the zooming rectangle. Press MB1 to put the 
replot command into clipboard, or press ESC to cancel the command.

:h1 res=606. Unzoom Menu Help
:i2 refid=mmouse. Unzoom
:p.This choise returns back the x and y ranges of the plot before
the first mouse zoom. 

:h1 res=607. Ruler Menu Help
:i2 refid=mmouse. Ruler
:p.Disables the ruler if it is already on.
:p.If the ruler has been off, then show it at the current pointer position. 
For every mouse movement, print the ruler position aside of the current 
pointer position, and show their distance (for linear scale) or ratio 
(for log scale). 

:h1 res=608. Grid Menu Help
:i2 refid=mmouse. Grid on/off
:p.Switches quickly on or off grid of the graph. Equivalent to the command 
:hp2.set grid; replot:ehp2. or :hp2.set nogrid; replot:ehp2. typed in the 
gnuplot window. (Actually, gnupmdrv can be compiled with this grid command 
or with grid at minor tics.)

:h1 res=609. Lin/Log Y Axis Menu Help
:i2 refid=mmouse. lin/log y axis
:p.Switches quickly between linear and logarithmic y axis. 
Equivalent to the command :hp2.set log y; replot:ehp2. or 
:hp2.set nolog y; replot:ehp2. Beeps for log of negative y range.



:h1 res=400. Continue Menu Help
:i1 id=mcont. Continue menu
:i2 refid=mcont. Continue plotting
:p.Selecting the :hp2.Continue:ehp2. menu item causes plotting
to resume after a pause command is received from Gnuplot.
 
:h1 res=5000. Printer setup dialog box help
:i1 id=qprint. Printing
:i2 refid=qprint. Printer setup
:p.This dialog box enables you to setup the printer.
:p.The printer that output will be sent to is indicated in the 
:hp2.Current printer:ehp2. field. You can select a different
printer by using the :hp2.Printers:ehp2. item of the :hp2.Options:ehp2.
menu.
:p.The setup can be selected by clicking on the :hp2.OK:ehp2. button.
:p.The setup can be cancelled by selecting :hp2.Cancel:ehp2. .
:p.If your printer driver supports printing to a file, the
:hp2.Print to file named:ehp2. field will not be greyed out. In this case,
you may enter a filename here for sending output to a file rather than to 
a printer. Some printer drivers also support this option from the
printer setup dialog box accessible with the :hp2.Set printer:ehp2.
option. You may choose either method. Some printer drivers (e.g. Postscript)
will not overwrite a file if you use the second method.
:p.The area of the page in which the plot will be displayed is
indicated. You can change this by selecting the :hp2.Set Page:ehp2. button.
You can then adjust the area with the mouse, and click on button 1
to select the new area. Another way of doing this is by typing
the appropriate data into the entry windows which give the size of the
plot area, either in centimeters or relative to the page size. 
:p.If you wish to adjust the default behaviour of the printer,
choose the :hp2.Job properties:ehp2. option. This will bring up your
printer setup dialog box. This is part of the printer driver, and the 
features you can adjust will depend on your printer. You can generally
use this to swich between landscape and portrait mode, for instance.
Note that some drivers might not have any options.
 
:h1 res=2000. Printer selection dialog box help
:i2 refid=qprint. Printer selection
:p.This dialog box enables you to select the printer on which
your output will appear.
:p.Choose a printer from the displayed list. The chosen printer
is highlighted.
:p.The printer is selected by clicking on the :hp2.OK:ehp2. button.
:p.The new selection is ignored by selecting :hp2.Cancel:ehp2. .

:h1 res=3000. Pause dialog box help
:i1 id=pausebox. Pause
:p.This dialog box is (optionally) displayed when a 
'pause -1 <text>' command is issued to Gnuplot.
Plotting is paused until you:
:p.Select :hp2.Continue:ehp2. to resume plotting.
:p.Select :hp2.Cancel:ehp2. to cancel plotting and return to the Gnuplot
command line.

:h1 res=6000. Fonts dialog box help
:i1 id=fonts. Font selection
:p.This dialog box enables you to change the font used on the displayed plot.
:p.The font is selected by clicking on the :hp2.OK:ehp2. button.
:p.The new selection is ignored by selecting :hp2.Cancel:ehp2. .
:p.Choose a font from the displayed list. The chosen font
is previwed in the :hp2.Example:ehp2. window.
:euserdoc.
