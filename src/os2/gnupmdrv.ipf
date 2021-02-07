:userdoc.
:docprof toc=12.

:h1 res=5. This program
:p.This program is a display interface, a "terminal" external driver for
gnuplot.

:h1 res=100. Options menu help
:i1 id=mopt. Options menu
:i2 refid=mopt. The options menu
:p.The :hp2.Options:ehp2. menu enables you to change various options on the
displayed plot, and to control printing of the plot via the OS/2 print
system.

:h2 res=101. Print
:i2 refid=mopt. Print
:p.The :hp2.Print:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu enables you to print the graph window using the default
printer. You can change the printer to output to using the :hp2.Printers:ehp2.
option.

:h2 res=115. Printers
:i2 refid=mopt. Printers
:p.The :hp2.Printers:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu enables you to select the printer to which
output is directed.

:h2 res=104. Fonts
:i2 refid=mopt. Fonts
:p.The :hp2.Fonts:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu enables you to change the font used in the displayed plot.
:p.You can also 'drag and drop' a font from a :hp2.Font Palette:ehp2. onto
the plot window.

:h2 res=206. Colours
:i2 refid=mopt. Colours
:p.Selecting the :hp2.Colours:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu causes lines used for graphs to be plotted in various
colours.

:h2 res=208. Thick line option
:i2 refid=mopt. Thick line option
:p.Selecting the :hp2.Thick:ehp2. option of the :hp2.Options:ehp2.
menu item toggles the :hp2.thick line:ehp2. option on and off.
The selection is active if the menu item is checked.
:p.The thick line option can give better output on high-resolution
devices like laser printers.

:h2 res=120. Pause mode option
:i2 refid=mopt. Pause mode option
:p.Selecting the :hp2.Pause options:ehp2. menu item from the :hp2.Options:ehp2.
menu enables you to choose how the gnuplot 'pause' command is handled.
:p.Selecting the :link reftype=hd res=121.Dialog box:elink. menu item
enu causes the gnuplot 'pause' command to print a message in a dialog box,
and wait for you to end the dialog before continuing or cancelling.
:p.Selecting the :link reftype=hd res=122.Menu bar:elink. menu item
causes the gnuplot 'pause' command to enable the :hp2.Continue:ehp2.
menu item.
:p.Selecting the :link reftype=hd res=123.Gnuplot:elink. menu item
menu causes the gnuplot 'pause' command to be handled by the gnuplot
program.

:h2 res=121. Pause with dialog box
:i2 refid=mopt. Pause with dialog box
:p.Selecting the :hp2.Dialog box:ehp2. menu item from the
:link reftype=hd res=120.Pause options:elink.
menu causes the gnuplot 'pause' command to print a message in a dialog box,
and wait for you to end the dialog before continuing.

:h2 res=122. Pause with menu item
:i2 refid=mopt. Pause with menu item
:p.Selecting the :hp2.Menu bar:ehp2. menu item from the
:link reftype=hd res=120.Pause options:elink.
menu causes the gnuplot 'pause' command to enable the :hp2.Continue:ehp2.
menu item. 
:p.Plotting will be resumed when this item is selected.
:p.Any text message passed to the 'pause' command is ignored.
:note.This mode does not offer a possibility to cancel.

:h2 res=123. Pause in gnuplot
:i2 refid=mopt. Pause in gnuplot
:p.Selecting the :hp2.Gnuplot:ehp2. menu item from the
:link reftype=hd res=120.Pause options:elink.
menu causes the gnuplot 'pause' command to be handled by the gnuplot
program.
:p.In order to resume plotting, you will have to select the gnuplot command
line window, and press the enter key.
 
:h2 res=210. Pop to front
:i2 refid=mopt. Pop to front
:p.Selecting the :hp2.Pop to front:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu causes the plot window to pop to the front of the
window stack each time a graph is plotted. If this item is unchecked,
the user will have to bring the window to the front by manual selection.

:h2 res=211. Keep aspect ratio
:i2 refid=mopt. Keep aspect ratio
:p.If this menu is checked, then the sides of the plot keep the aspect ratio 
of 1.56 thus filling only partially the plotting window. 
If it is unchecked, then the plot occupies the whole window.

:h2 res=105. Save settings
:i2 refid=mopt. gnupmdrv.ini
:i2 refid=mopt. Save settings
:p.Selecting the :hp2.Save settings:ehp2. menu item from the :hp2.Options:ehp2.
pulldown menu causes the current line, colour and font options to be saved.
The positions and sizes of the windows are also saved.
:p.The data is saved in the file :hp2.gnupmdrv.ini:ehp2. file in the 
program&csq.s working directory. You can delete the file if you want to restore
all settings to their default values. (This file is created even if no settings
are saved.) 


:h1 res=500. Edit menu help
:i1 id=medit. Edit menu
:i2 refid=medit. The Edit menu
:p.The :hp2.Edit:ehp2. menu gives you access to commands 
for copying the plot to the clipboard.

:h2 res=501. Copy to clipboard
:i2 refid=medit. Copy to clipboard
:p.Selecting the :hp2.Copy:ehp2. menu item from the :hp2.Edit:ehp2.
pulldown menu causes the current plot to be copied to the system
clipboard. The plot is copied in bitmap and in metafile format.

:h2 res=504. Clear clipboard
:i2 refid=medit. Clear clipboard
:p.Selecting the :hp2.Clear clipboard:ehp2. menu item from 
the :hp2.Edit:ehp2. pulldown menu causes the clipboard to be cleared.


:h1 res=600. Mouse menu help
:i1 id=mmouse. Mouse menu
:i2 refid=mmouse. The Mouse menu
:p.The :hp2.Mouse:ehp2. menu gives you access to mouse (pointer)-related 
functions. Hotkeys and other actions are configurable using gnuplot commands;
see 'help mouse' and 'help bind'.
:p.Help on the current mouse configuration is available via the hotkey 'h'.
The behaviour of the submenu items may be changed according to gnuplot
or user settings. Menu items show two different hotkeys: the first one
is user-configurable (and hence may not correspond to the actual settings),
while the other is fixed and has precedence over user settings.

:h2 res=601. Use mouse
:i2 refid=mmouse. Use mouse
:p.Checking this menu item enables the mouse (pointer device) functionality:
tracing the position over graph, zooming, annotating graph etc. for 2d graphs
and for maps (`set view map`).
Mousing is not available in multiplot mode. 

:p.By default, mouse buttons (MB) have the following functions, where MB1
typically is the left, MB2 the right, and MB3 the middle mouse button:
:ul.
:li.:hp2.MB2:ehp2. starts marking the zoom region. Press :hp2.MB1:ehp2. to
zoom, or :hp2.Esc:ehp2. to cancel zooming.
.br
Note that you can browse through the history of zoomed ranges via
:hp2.Unzoom all:ehp2., :hp2.Unzoom back:ehp2. and :hp2.Zoom next:ehp2..

:li.:hp2.Double click of MB1:ehp2. writes the current pointer position to the
clipbord according to the format chosen e.g. via the :hp2.format:ehp2. menu.

:li.:hp2.MB3:ehp2. annotates the graph.

:eul.
:p. Press 'h' to see the complete list.

:h2 res=602. Coordinates format
:i2 refid=mmouse. Coordinates
:p.Choose the format of coordinates used for showing the mouse position,
copy to clipboard and annotation. It is equivalent to the
:xmp.
set mouse mouseformat <n>|<format string>
:exmp.
command. The following options are available via the menu:
:dl compact tsize=13.
:dt.:hp2.x,y:ehp2. :dd.(mouseformat 1)
:dt.:hp2.[x,y]:ehp2. :dd.(mouseformat "[%g, %g]")
:dt.:hp2.x y:ehp2.:dd.(mouseformat "%g %g")
:dt.:hp2.timefmt:ehp2. :dd.(mouseformat 3)
:dt.:hp2.date:ehp2. :dd.(mouseformat 4)
:dt.:hp2.time:ehp2. :dd.(mouseformat 5)
:dt.:hp2.date / time:ehp2. :dd.coordinate is useful when the coordinate on
the x-axis is time or date. (mouseformat 6)
:edl.
:note. You can cycle through the predefined mouses format by pressing '1' and
'2' (previous/next).

:h2 res=605. By mouse...
:i2 refid=mmouse. By mouse: zoom
:p.Well, subitems of this do nothing just remember you which mouse button 
combinations to use for putting the current position to the clipboard, for 
zooming or temporarily annotating the graph. 

:h2 res=610. Unzoom and zoom history
:i2 refid=mmouse. Unzoom and zoom
:p.These menu item let you browse through the history of zoom choices.

:h2 res=613. Ruler
:i2 refid=mmouse. Ruler
:p.Toggles the ruler on or off.
:p.The ruler is drawn at the pointer position when activated. The display of
the mouse coordinates is extended by the ruler position and the distance of
the current pointer position to it, and optionally the
:link reftype=hd res=615.polar distance:elink..

:h2 res=615. Polar distance
:i2 refid=mmouse. Polar distance
:p.If this menu item is checked, then the distance between the ruler and 
mouse cursor is printed also in polar coordinates. This is particularly 
useful for dealing with peaks or other objects in maps.


:h1 res=650. Utilities menu help
:i1 id=mutils. Utilities menu
:i2 refid=mutils. The Utilities menu
:p.The :hp2.Utilities:ehp2. menu gives you access to miscellaneous functions, 
mostly those which communicate with gnuplot.

:h2 res=651. Break drawing
:i2 refid=mutils. Break drawing
:p.This menu item, or the hotkey Ctrl-C, lets you abort long drawing operations
immediately.

:h2 res=652. Grid on/off
:i2 refid=mutils. Grid on/off
:p.Toggles the grid of the graph on or off. Equivalent to the gnuplot commands
:xmp.set grid; replot:exmp.
.br
or
:xmp.unset grid; replot:exmp.
.br
respectively.

:h2 res=653. Lin/log y axis
:i2 refid=mutils. Lin/log y axis
:p.Switches quickly between linear and logarithmic y axis. 
Equivalent to commands
:xmp.set log y; replot:exmp.
.br
or
:xmp.unset log y; replot:exmp.
.br
respectively.

:h2 res=700. Set
:i2 refid=mutils. Set
:p.Switches miscellaneous :hp2.set:ehp2. properties. For example:
:xmp.set style data dots; replot:exmp.

:h2 res=655. Autoscale
:i2 refid=mutils. Autoscale
:p.Switches autoscale of x and y axes. Equivalent to
:xmp.set autoscale; replot:exmp.

:h2 res=656. Replot
:i2 refid=mutils. Replot
:p.Replots the graph by sending the :hp2.replot:ehp2. command.

:h2 res=657. Reload
:i2 refid=mutils. Reload
:p.Reloads a file, i.e it issues :xmp.history !load:exmp.

:h2 res=658. Command
:i2 refid=mutils. Command
:p.This enables you to send any command to gnuplot from within this 
display driver. This is particularly useful when the gnuplot command line 
is not available (an application sends commands and data to gnuplot via 
pipe, for instance).


:h1 res=300. Gnuplot menu help
:i1 id=mgnu. Gnuplot menu
:i2 refid=mgnu. Switching to gnuplot window
:p.Selecting the :hp2.Gnuplot:ehp2. menu item causes the gnuplot
command window to be brought to the foreground.


:h1 res=400. Continue menu help
:i1 id=mcont. Continue menu
:i2 refid=mcont. Continue plotting
:p.Selecting the :hp2.Continue:ehp2. menu item causes plotting
to resume after a pause command is received from gnuplot.
:note.This menu item will only be active if you selected the
:link reftype=hd res=122.Menu bar:elink. pause mode.

:h1 res=1. Help Menu Help
:i1 id=mabout. Help menu
:p.gnupmdrv help or gnuplot documentation can be accessed from this menu.

:h2 res=10004. Help index
:p.Use :hp2.Help index:ehp2. to browse through the help for this terminal
driver.

:h2 res=10005. gnuplot docs
:p.Use :hp2.gnuplot docs:ehp2. to read the gnuplot documentation.

:h2 res=10006. About
:i2 refid=mabout. About
:p.The :hp2.About:ehp2. menu item displays the :hp2.About message box:ehp2.,
which just identifies the program.


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
use this to switch between landscape and portrait mode, for instance.
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
'pause -1 <text>' command is issued to gnuplot.
Plotting is paused until you:
:p.Select :hp2.Continue:ehp2. to resume plotting.
:p.Select :hp2.Cancel:ehp2. to cancel plotting and return to the gnuplot
command line.

:h1 res=6000. Fonts dialog box help
:i1 id=fonts. Font selection
:p.This dialog box enables you to change the font used on the displayed plot.
:p.The font is selected by clicking on the :hp2.OK:ehp2. button.
:p.The new selection is ignored by selecting :hp2.Cancel:ehp2. .
:p.Choose a font from the displayed list. The chosen font
is previwed in the :hp2.Example:ehp2. window.
:euserdoc.
