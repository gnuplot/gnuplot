Here's a brief description of what each term.c procedure does:

_init()  Called once, when the device is first selected.  This procedure
should set up things that only need to be set once, like handshaking and
character sets etc...

_graphics()  Called just before a plot is going to be displayed.  This
procedure should set the device into graphics mode.  Devices which can't
be used as terminals (like plotters) will probably be in graphics mode always
and therefore won't need this.

_text()  Called after a plot is displayed.  This procedure should set the
device back into text mode if it is also a terminal, so that commands can
be seen as they're typed.  Again, this will probably do nothing if the
device can't be used as a terminal.

_linetype(lt)  Called to set the line type before text is displayed or line(s)
plotted.  This procedure should select a pen color or line style if the
device has these capabilities.  lt is an integer from -2 to 8.  An lt of
-2 is used for the border of the plot.  An lt of -1 is used for the X and Y
axes.  lt 0 through 8 are used for plots 0 through 8.

_move(x,y)  Called at the start of a line.  The cursor should move to the
(x,y) position without drawing.

_vector(x,y)  Called when a line is to be drawn.  This should display a line
from the last (x,y) position given by _move() or _vector() to this new (x,y)
position.

_ulput_text(row,str)  Called to display text in the upper-left corner of
the screen/page.  row is an integer from 0 through 8.  The row starts
at the upper-left with 0 and proceed down.  str is the string to be displayed.

_lrput_text(row,str)  Called to display text in the lower-right corner of
the screen/page.  This is the same as ulput_text(), except that the string
should be displayed right-justified in the lower right corner of the screen.
The row starts at the lower-right with 0 and proceeds up.

_reset()  Called when Gnuplot is exited.  This procedure should reset the
device, possibly flushing a buffer somewhere or generating a form feed.
