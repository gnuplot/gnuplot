.if n \{.nr LL +8m
.na \}
.nr PO +0.3i
.so titlepage
.pn 1
.ds CH GNUPLOT
.ds RH Page %
.bp
.nr PS 12
.nr VS 13
.ta 1.5i 3.0i 4.5i 6.0i 7.5i
\&
.sp 3
.PP
.so intro
.sp 3
.NH 1
clear
.sp 1
.LP
.XS
clear
.XE
This command erases the current screen or output device as specified
by 'set output'.  This usually generates a formfeed on hardcopy
devices.  Use 'set terminal' to set the device type. 

.sp 3
.NH 1
exit
.sp 1
.LP
.XS
exit
.XE
\&'exit', 'quit' and your computer's END-OF-FILE character will exit
GNUPLOT.  All these commands will clear the output device (as the
\&'clear' command does) before exiting. 

.sp 3
.NH 1
expressions
.sp 1
.LP
.XS
expressions
.XE
.sp 2
.RS
.IP
.NH 2
express
.sp 1
.LP
.XS
express
.XE
In general, any mathematical expression accepted by C, FORTRAN,
Pascal, or BASIC is valid.  The precedence of these operators is
determined by the specifications of the C programming language.
White space (spaces and tabs) is ignored inside expressions. 

Complex constants may be expressed as the {real,imag}, where <real>
and <imag> must be numerical constants.  For example {3,2}
represents 3 + 2i; {0,1} represents 'i' itself. 

.sp 2
.NH 2
functions
.sp 1
.LP
.XS
functions
.XE
.sp 2
.RS
.IP
.NH 3
abs
.sp 1
.LP
.XS
abs
.XE
This function returns the absolute value of its argument.  The
returned value is of the same type as the argument. 

For complex arguments, abs(x) is defined as the length of x in the
complex plane [i.e.  sqrt(real(x)**2 + imag(x)**2) ].

.sp 2
.NH 3
acos
.sp 1
.LP
.XS
acos
.XE
This function returns the arc cosine (inverse cosine) of its
argument.  'acos' returns its argument in radians. 

.sp 2
.NH 3
arg
.sp 1
.LP
.XS
arg
.XE
This function returns the phase of a complex number, in radians.  

.sp 2
.NH 3
asin
.sp 1
.LP
.XS
asin
.XE
This function returns the arc sin (inverse sin) of its argument.
\&'asin' returns its argument in radians. 

.sp 2
.NH 3
atan
.sp 1
.LP
.XS
atan
.XE
This function returns the arc tangent (inverse tangent) of its
argument.  'atan' returns its argument in radians. 

.sp 2
.NH 3
besj0
.sp 1
.LP
.XS
besj0
.XE
This function returns the j0th Bessel function of its argument.
\&'besj0' expects its argument to be in radians.

.sp 2
.NH 3
besj1
.sp 1
.LP
.XS
besj1
.XE
This function returns the j1st Bessel function of its argument.
\&'besj1' expects its argument to be in radians.

.sp 2
.NH 3
besy0
.sp 1
.LP
.XS
besy0
.XE
This function returns the y0th Bessel function of its argument.
\&'besy0' expects its argument to be in radians.

.sp 2
.NH 3
besy1
.sp 1
.LP
.XS
besy1
.XE
This function returns the y1st Bessel function of its argument.
\&'besy1' expects its argument to be in radians.

.sp 2
.NH 3
ceil
.sp 1
.LP
.XS
ceil
.XE
This function returns the largest integer not less than its
argument.  For complex numbers, 'ceil' returns the largest integer
not less than the real part of its argument. 

.sp 2
.NH 3
cos
.sp 1
.LP
.XS
cos
.XE
This function returns the cosine of its argument.  'cos' expects its
argument to be in radians. 

.sp 2
.NH 3
cosh
.sp 1
.LP
.XS
cosh
.XE
This function returns the hyperbolic cosine of its argument.
\&'cosh' expects its argument to be in radians. 

.sp 2
.NH 3
exp
.sp 1
.LP
.XS
exp
.XE
This function returns the exponential function of its argument
(e raised to the power of its argument). 

.sp 2
.NH 3
floor
.sp 1
.LP
.XS
floor
.XE
This function returns the smallest integer not greater than its
argument.  For complex numbers, 'floor' returns the smallest
integer not greater than the real part of its argument. 

.sp 2
.NH 3
funct
.sp 1
.LP
.XS
funct
.XE
The functions in GNUPLOT are the same as the corresponding functions
in the UNIX math library, except that all functions accept integer,
real, and complex arguments, unless otherwise noted.  The sgn()
function is also supported, as in BASIC. 

.sp 2
.NH 3
imag
.sp 1
.LP
.XS
imag
.XE
This function returns the imaginary part of its argument as a real number.

.sp 2
.NH 3
int
.sp 1
.LP
.XS
int
.XE
This function returns the integer part of its argument, truncated
toward zero. 

.sp 2
.NH 3
log
.sp 1
.LP
.XS
log
.XE
This function returns the natural logarithm (base e) of its
argument. 

.sp 2
.NH 3
log10
.sp 1
.LP
.XS
log10
.XE
This function returns the logarithm (base 10) of its argument.

.sp 2
.NH 3
real
.sp 1
.LP
.XS
real
.XE
This function returns the real part of its argument.

.sp 2
.NH 3
sgn
.sp 1
.LP
.XS
sgn
.XE
This function returns 1 if its argument is positive, -1 if its
argument is negative, and 0 if its argument is 0.  If the argument
is a complex value, the imaginary component is ignored. 

.sp 2
.NH 3
sin
.sp 1
.LP
.XS
sin
.XE
This function returns the sine of its argument.  'sin' expects its
argument to be in radians. 

.sp 2
.NH 3
sinh
.sp 1
.LP
.XS
sinh
.XE
This function returns the hyperbolic sine of its argument.  'sinh'
expects its argument to be in radians. 

.sp 2
.NH 3
sqrt
.sp 1
.LP
.XS
sqrt
.XE
This function returns the square root of its argument.

.sp 2
.NH 3
tan
.sp 1
.LP
.XS
tan
.XE
This function returns the tangent of its argument.  'tan' expects
its argument to be in radians. 

.sp 2
.NH 3
tanh
.sp 1
.LP
.XS
tanh
.XE
This function returns the hyperbolic tangent of its argument.
\&'tanh' expects its argument to be in radians. 

.sp 2
.RE
.br
.NH 2
operators
.sp 1
.LP
.XS
operators
.XE
.sp 2
.RS
.IP
.NH 3
binary
.sp 1
.LP
.XS
binary
.XE
The following is a list of all the binary operators and their
usages:

.br
 Symbol 	Example 	 Explanation
.br
.br
  **     	 a**b    	  exponentiation
.br
.br
  *      	 a*b     	  multiplication
.br
.br
  /      	 a/b     	  division
.br
.br
  %      	 a%b     	* modulo
.br
.br
  +      	 a+b     	  addition
.br
.br
  -      	 a-b     	  subtraction
.br
.br
  ==     	 a==b    	  equality
.br
.br
  !=     	 a!=b    	  inequality
.br
.br
  &      	 a&b     	* bitwise and
.br
.br
  ^      	 a^b     	* bitwise exclusive or
.br
.br
  |      	 a|b     	* bitwise inclusive or
.br
.br
  &&     	 a&&b    	* logical and
.br
.br
  ||     	 a||b    	* logical or
.br
.br
  ?:     	 a?b:c   	* ternary operation
.br

(*) Starred explanations indicate that the operator requires
integer arguments.

Logical AND (&&) and OR (||) short-circuit the way they do in C.
That is, the second && operand is not evaluated if the first is
false; the second || operand is not evaluated if the first is true.

The ternary operator evaluates its first argument (a).  If it is
true (non-zero) the second argument (b) is evaluated and returned,
otherwise the third argument (c) is evaluated and returned. 

.sp 2
.NH 3
oper
.sp 1
.LP
.XS
oper
.XE
The operators in GNUPLOT are the same as the corresponding operators
in the C programming language, except that all operators accept
integer, real, and complex arguments, unless otherwise noted.
The ** operator (exponentiation) is supported, as in FORTRAN.

Parentheses may be used to change order of evaluation.

.sp 2
.NH 3
unary
.sp 1
.LP
.XS
unary
.XE
The following is a list of all the unary operators and their
usages:

.br
 Symbol 	Example 	 Explanation
.br
.br
  -      	 -a      	  unary minus
.br
.br
  ~      	 ~a      	* one's complement
.br
.br
  !      	 !a      	* logical negation
.br
.br
  !      	 a!      	* factorial
.br

(*) Starred explanations indicate that the operator requires an
integer argument.

The factorial operator returns a real number to allow a greater range.

.sp 3
.RE
.br
.RE
.br
.NH 1
gplt
.sp 1
.LP
.XS
gplt
.XE
GNUPLOT is a command-driven interactive function plotting program.  It
is case sensitive (commands and function names written in lowercase
are not the same as those written in CAPS).  All command names may be
abbreviated, as long as the abbreviation is not ambiguous.  Any number
of commands may appear on a line, separated by semicolons (;). 

.sp 3
.NH 1
help.hlm
.sp 1
.LP
.XS
help.hlm
.XE
GNUPLOT is a command-driven interactive function plotting program.  It
is case sensitive (commands and function names written in lowercase
are not the same as those written in CAPS).  All command names may be
abbreviated, as long as the abbreviation is not ambiguous.  Any number
of commands may appear on a line, separated by semicolons (;). 
.sp 3
.NH 1
help
.sp 1
.LP
.XS
help
.XE
The 'help' command displays on-line help.  To specify information on a
particular topic use the syntax:

.br
    help <topic> 
.br

You may exit the help utility and return to GNUPLOT by either pressing
<return> at the 'Topic?' prompt or pressing your computer's END-OF-FILE
character at any help prompt. 

.sp 3
.NH 1
load
.sp 1
.LP
.XS
load
.XE
This command executes each line of the specified input file as if it
had been typed in interactively.  Files created by the 'save' command
can later be 'load'ed.  Any text file containing valid commands can be
created and then executed by the 'load' command.  Files being 'load'ed
may themselves contain 'load' commands.  Lines beginning with # (or !
if using VMS) will be treated as comments and ignored.

The 'load' command must be the last command on the line. 
.br
        
.br
Syntax:
.br
        load <input-file>
.br

The name of the input file must be enclosed in quotes.

Examples:

.br
        load 'work.gnu'
.br
.br
        load "func.dat"
.br

.sp 3
.NH 1
plot
.sp 1
.LP
.XS
plot
.XE
.sp 2
.RS
.IP
.NH 2
datafile
.sp 1
.LP
.XS
datafile
.XE
Discrete data contained in a file can displayed by specifying the
name of the data file (enclosed in quotes) on the 'plot' command
line. 

Data files should contain one data point per line.  A data point may
be specified either as an X and Y value separated by blank space, or
as just the Y value, in which case the program will use the number
of the coordinate as the X value.  Coordinate numbers starts at 0
and are incremented for each data point read.  Blank lines and lines
beginning with # will be treated as comments and ignored. 


This example compares the data in the file population.dat to a
theoretical curve:

.br
        pop(x) = 103*exp((1965-x)/10)
.br
.br
        plot [1960:1990] 'population.dat', pop(x)
.br

The file population.dat might contain:

.br
 # Gnu population in Antarctica since 1965
.br
.br
 1965   103
.br
.br
 1970   55
.br
.br
 1975   34
.br
.br
 1980   24
.br
.br
 1985   10
.br

.sp 2
.NH 2
plthlp
.sp 1
.LP
.XS
plthlp
.XE
\&'plot' is the primary command of the program.  It plots functions and
data in many, many ways.  The full syntax of this command is:

.br
  plot {ranges}   <function> {style} {, <function> {style}...}
.br

Where <function> is either a mathematical expression or the name of a
data file enclosed in quotes.  User-defined functions and variables
may also be defined here. 

Curly braces {,} denote optional items.

A 'plot' command can be as simple as

.br
 plot sin(x)
.br

or as complex as (!)

.br
 plot [t=1:10] [-pi:pi*2] tan(t),"data.1" with lines,t**2 with points
.br

.sp 2
.NH 2
ranges
.sp 1
.LP
.XS
ranges
.XE
This option specifies the region of the plot which will be displayed. 

Ranges may be provided on the 'plot' command line as synonyms for
the 'set xrange' and 'set yrange' commands.

Syntax:
.br
        [{dummy-var =} {xmin : xmax}] { [{ymin : ymax}] }
.br

Where dummy-var is the independent variable (the default is 'x',
but this may be changed with the 'set dummy' command) and the min
and max terms can be expressions or constants. 

Both the min and max terms are optional.  The ':' is also optional
if neither a min nor a max term is specified.  This allows '[]' to
be used as a null range specification. 

Specifying a Y range turns autoscaling OFF.


Examples:

This uses current ranges:
.br
        plot cos(x)
.br

This sets the xrange only:
.br
        plot [-10:30] sin(pi*x)/(pi*x)
.br

This is the same, but uses t as the dummy-variable:
.br
        plot [t = -10 :30]  sin(pi*t)/(pi*t)
.br

This sets both the x and yranges:
.br
        plot [-pi:pi] [-3:3]  tan(x), 1/x
.br

This sets only the yrange:
.br
        plot [] [-2:sin(5)*-8] sin(x)**besj0(x)
.br

This sets xmax and ymin only:
.br
        plot [:200] [-pi:]  exp(sin(x))
.br

.sp 2
.NH 2
style
.sp 1
.LP
.XS
style
.XE
Plots may be displayed in one of three styles: 'lines', 'points', or
\&'impulses'.  The 'lines' style connects adjacent points with lines. 
The 'points' style displays a small symbol at each point.  The
\&'impulses' style displays a vertical line from the X axis to each
point. 

Default styles are chosen with the 'set function style' and
\&'set data style' commands. 

Syntax:
.br
        with <style>
.br

Where <style> is one of 'lines', 'points', or 'impulses'.  These
keywords may be abbreviated. 


Examples:

This plots sin(x) with impulses:
.br
        plot sin(x) with impulses
.br

This plots sin(x) with points, cos(x) default:
.br
        plot [-9:30]  sin(x) w points, cos(x)
.br

This plots tan(x) with the default function style, "data.1" with lines:
.br
        plot [] [-2:5] tan(x), "data.1" with l
.br

This plots "leastsq.dat" with impulses:
.br
        plot 'leastsq.dat' w i
.br

.sp 3
.RE
.br
.NH 1
print
.sp 1
.LP
.XS
print
.XE
This command prints the value of <expression> to the screen.

Syntax:
.br
        print <expression>
.br

See 'expressions'.

.sp 3
.NH 1
quit
.sp 1
.LP
.XS
quit
.XE
\&'quit' is a synonym for 'exit'.  See 'exit'.

.sp 3
.NH 1
replot
.sp 1
.LP
.XS
replot
.XE
\&'replot' repeats the last 'plot' command.  This can be useful for
viewing a plot with different 'set' options, or when generating the
same plot for several devices.

.sp 3
.NH 1
save
.sp 1
.LP
.XS
save
.XE
This command saves either user-defined functions, variables, or both
to the specified file. 

Syntax:
.br
        save  {option} <filename>
.br

Where <option> is either 'functions' or 'variables'.  If no option is
used GNUPLOT saves both functions and variables. 

\&'save'd files are written in text format and may be read by the 'load'
command.

The filename must be enclosed in quotes.

Examples:

.br
        save "work.gnu"
.br
.br
        save functions 'func.dat'
.br
.br
        save var 'var.dat'
.br

.sp 3
.NH 1
set-show
.sp 1
.LP
.XS
set-show
.XE
.sp 2
.RS
.IP
.NH 2
autoscal
.sp 1
.LP
.XS
autoscal
.XE
If autoscaling is set, the Y axis is automatically scaled to fit the
range of the function or data being plotted.  If autoscaling is not
set, the current Y range is used.  See 'set yrange'. 

Syntax:
.br
        set autoscale
.br
.br
        set noautoscale
.br
.br
        show autoscale
.br

.sp 2
.NH 2
dummy
.sp 1
.LP
.XS
dummy
.XE
By default, GNUPLOT assumes that the independent variable is 'x' on
the 'plot' command line.  'x' is called the dummy variable because
it is just a notation to indicate the independent variable.  The
\&'set dummy' command changes this default dummy variable name.  For
example, you may find it more convenient to call the dummy variable
\&'t' when plotting time functions:

.br
        set dummy t
.br
.br
        plot sin(t), cos(t)
.br

Syntax:
.br
        set dummy <dummy-var>
.br
.br
        show dummy
.br

.sp 2
.NH 2
function
.sp 1
.LP
.XS
function
.XE
The 'show functions' command lists all user-defined functions and
their definitions. 

Syntax:
.br
        show functions
.br

.sp 2
.NH 2
hlp
.sp 1
.LP
.XS
hlp
.XE
The 'set' command sets LOTS of options.

The 'show' command shows their settings.  'show all' shows all the
settings. 

.sp 2
.NH 2
logscale
.sp 1
.LP
.XS
logscale
.XE
Log scaling may be set on the X and/or Y axis.

Syntax:
.br
        set logscale <axes>
.br
.br
        set nologscale
.br
.br
        show logscale
.br

Where <axes> is either 'x', 'y', or 'xy'.

.sp 2
.NH 2
output
.sp 1
.LP
.XS
output
.XE
By default, plots are displayed to the standard output.  The
\&'set output' command redirects the displays to the specified file or
device. 

Syntax:
.br
        set output {filename}
.br
.br
        show output
.br

The filename must be enclosed in quotes.  If the filename is
omitted, output will be sent to the standard output. 

.sp 2
.NH 2
samples
.sp 1
.LP
.XS
samples
.XE
The sampling rate of functions may be changed by the 'set samples'
command.  By default, sampling is set to 160 points.  A higher
sampling rate will produce more accurate plots, but will take
longer.  In generating plots, GNUPLOT will use either the sampling
rate set or the resolution of the current output device, whichever
is lower. 

Syntax:
.br
        set samples <expression>
.br
.br
        show samples
.br

.sp 2
.NH 2
style
.sp 1
.LP
.XS
style
.XE
Plots may be displayed in one of three styles: 'lines', 'points', or
\&'impulses'.  The 'lines' style connects adjacent points with lines. 
The 'points' style displays a small symbol at each point.  The
\&'impulses' style displays a vertical line from the X axis to each
point. 

Default styles are chosen with the 'set function style' and 'set
data style' commands.  See 'plot' for information about how to
override the default plotting style for individual functions. 

Syntax:
.br
        set function style <style>
.br
.br
        set data style <style>
.br
.br
        show function style
.br
.br
        show data style
.br

Where style is either 'lines', 'points', or 'impulses'.

.sp 2
.NH 2
terminal
.sp 1
.LP
.XS
terminal
.XE
GNUPLOT supports many different graphics devices.  Use the 'set
terminal' command to select the type of device for which GNUPLOT
will produce output. 

Syntax:
.br
        set terminal {terminal-type}
.br
.br
        show terminal
.br

If <terminal-type> is omitted, GNUPLOT will list the available
terminal types.  <terminal-type> may be abbreviated. 

Use 'set output' to redirect this output to a file or device.

.sp 2
.NH 2
variable
.sp 1
.LP
.XS
variable
.XE
The 'show variables' command lists all user-defined variables and
their values. 

Syntax:
.br
        show variables
.br

.sp 2
.NH 2
xrange
.sp 1
.LP
.XS
xrange
.XE
The 'set xrange' command sets the horizontal range which will be
displayed. 

This range may also be specified on the 'plot' command line.

Syntax:
.br
        set xrange [{xmin : xmax}]
.br

Where <xmin> and <xmax> terms are expressions or constants.

Both the <xmin> and <xmax> terms are optional.

.sp 2
.NH 2
yrange
.sp 1
.LP
.XS
yrange
.XE
The 'set yrange' command sets the vertical range which will be
displayed.  This command turns autoscaling OFF. 

This range may also be specified on the 'plot' command line.

Syntax:
.br
        set yrange [{ymin : ymax}]
.br

Where <ymin> and <ymax> terms are expressions or constants.

Both the <ymin> and <ymax> terms are optional.

.sp 2
.NH 2
zero
.sp 1
.LP
.XS
zero
.XE
GNUPLOT will not plot a point if its imaginary part is greater in
magnitude than the 'zero' threshold.  The default 'zero' value is
1e-8. 

Syntax:
.br
        set zero <expression>
.br
.br
        show zero
.br

.sp 3
.RE
.br
.NH 1
shell
.sp 1
.LP
.XS
shell
.XE
The 'shell' command spawns an interactive shell.  To return to
GNUPLOT, type 'logout' if using VMS, 'exit' or your END-OF-FILE
character if using Unix, or 'exit' if using MS-DOS. 

A single shell command may be spawned by preceding it with the !
character ($ if using VMS) at the beginning of a command line.
Control will return immediately to GNUPLOT after this command is
executed.  For example,

.br
     ! dir
.br

prints a directory listing and then returns to GNUPLOT.

.sp 3
.NH 1
startxup
.sp 1
.LP
.XS
startxup
.XE
When GNUPLOT is run, it looks for an initialization file to load. 
This file is called '.gnuplot' on Unix systems, and 'GNUPLOT.INI' on
other systems.  If this file is not found in the current directory,
the program will look for it in your home directory (under MS-DOS, the
environment variable GNUPLOT should contain the name of this
directory). 

If this file is found, GNUPLOT executes the commands in this file.
This is most useful for setting your terminal type and defining any
functions or variables which you use often.  The variable 'pi' is
already defined for you. 

.sp 3
.NH 1
subsitut
.sp 1
.LP
.XS
subsitut
.XE
Command-line substitution is specified by a system command enclosed in
backquotes (`).  This command is spawned and the output it produces
replaces the name of the command (and backquotes) on the command line.

Newlines in the output produced by the spawned command are replaced with
blanks.

Command-line substitution can be used anywhere on the GNUPLOT command
line. 


Example:

This will run the program 'leastsq' and substitute `leastsq` on the
command line with its output:

.br
 f(x) = `leastsq`
.br

or, in VMS

.br
 f(x) = `run leastsq`
.br

.sp 3
.NH 1
userdef
.sp 1
.LP
.XS
userdef
.XE
You may define your own functions and variables.  User-defined
functions and variables may be used anywhere. 

User-defined function syntax:
.br
    <function-name> ( <dummy-var> ) =  <expression>
.br

Where <expression> is defined in terms of <dummy-var>.

User-defined variable syntax:
.br
    <variable-name> = <constant-expression>
.br

Examples:
.br
    w = 2
.br
.br
    q = floor(tan(pi/2 - 0.1))
.br
.br
    f(x) = sin(w*x)
.br
.br
    sinc(x) = sin(pi*x)/(pi*x)
.br
.br
    delta(t) = (t == 0)
.br
.br
    ramp(t) = (t > 0) ? t : 0
.br

The variable 'pi' is already defined for you.

See 'show functions' and 'show variables'.

.pn 1
.ds RH %
.af % i
.bp
.PX
