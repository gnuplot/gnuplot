# demo for pm3d splotting
#
# This demo can be directly used if your default terminal supports pm3d, 
# like OS/2 Presentation Manager, X11 and Linux VGA.
# Otherwise use the other demos. They use this one by an appropriate 
# awk preprocessing.

# Prepared by Petr Mikulik
# History: 
#	- 15. 6. 1999: update for `set pm3d`+`set palette`
# 	- 29. 4. 1999: 1st version
# 	- 03. 3. 2000: updated to show new pm3d features (Johannes Zellner)


print "Working..."

#BEGIN

set xlabel "x"
set ylabel "y"
set key top
set border 4095
set xrange [-15:15]
set yrange [-15:15]
set zrange [-0.25:1]
set samples 25
set isosamples 20

set title "pm3d demo. Radial sinc function. Default options."
set pm3d; set palette
#show pm3d
#show palette
splot sin(sqrt(x**2+y**2))/sqrt(x**2+y**2)
pause -1 "Press Enter."

set title "pm3d at s (surface) / ticslevel 0"
set ticslevel 0
set pm3d at s
replot
pause -1 "Press Enter."

set title "set pm3d solid; the surface hides part of border, tics and lables"
set pm3d solid
replot
pause -1 "Press Enter."
set pm3d transparent

set title "pm3d at b (bottom)"
set pm3d at b
replot
pause -1 "Press Enter."

set title "unset surface; set pm3d at st (surface and top)"
unset surface
set pm3d at st solid
replot
pause -1 "Press Enter."

set title "set pm3d at bstbst (funny combination, only for screen or postscript)"
set view 50,50
set pm3d at bstbst
replot
pause -1 "Press Enter."
set pm3d transparent

set title "gray map"
set pm3d at b
set palette gray
set view 180,0,1.2; set yrange [*:*] reverse
set samples 100; set isosamples 100
replot
pause -1 "Press Enter."

set title "gray map, negative"
set pm3d at b
set palette gray negative
set view 180,0,1.2; set yrange [*:*] reverse
replot
pause -1 "Press Enter."

set title "colour map, using default rgbformulae 7,5,15 ... traditional pm3d (black-blue-red-yellow)"
set palette color positive
set view 180,0,1.2; set yrange [*:*] reverse
set samples 50; set isosamples 50
replot
pause -1 "Press Enter."

set title "colour, rgbformulae 3,11,6 ... green-red-violet"
set palette rgbformulae 3,11,6
replot
pause -1 "Press Enter."

set title "colour, rgbformulae 23,28,3  ... ocean (green-blue-white); OK are also all other permutations"
set palette rgbformulae 23,28,3
replot
pause -1 "Press Enter."

set title "colour, rgbformulae 30,31,32 ... color printable on gray (black-blue-violet-yellow-white)"
set palette rgbformulae 30,31,32
replot
pause -1 "Press Enter."

set title "rgbformulae 31,-11,32: negative formula number=inverted color"
set palette rgbformulae 31,-11,32
replot
pause -1 "Press Enter."
set yrange [*:*] noreverse


reset
set title "surface at view 130,10 (viewed from below)"
set pm3d
set palette
set view 130,10
set samples 50; set isosamples 50
set border 4095
unset surface
set pm3d at s scansforward
splot sin(sqrt(x**2+y**2))/sqrt(x**2+y**2)
pause -1 "Press Enter."

set title '"set pm3d scansbackward" makes this as viewed from above'
set xlabel
set border 4095
set pm3d scansbackward
replot
pause -1 "Press Enter."


set title "set hidden3d"
set samples 30; set isosamples 30
set hidden3d
set pm3d
set surface
set view 50,220
set xrange [-2:2]
set yrange [-2:2]
splot log(x*x*y*y)
pause -1 "Press Enter."
unset hidden3d

# draw the surface using pm3d's hidden3d with line type 100
unset hidden
unset surface

set title "set pm3d hidden3d <linetype>: pm3d's much faster hidden3d variant"
set samples 30; set isosamples 30
set pm3d
set style line 100 lt 5 lw 0.5
set pm3d solid hidden3d 100
set view 50,220
set xrange [-2:2]
set yrange [-2:2]
splot log(x*x*y*y)
pause -1 "Press Enter."
set pm3d nohidden3d

set title "bad: surface and top are too close together"
set xrange [-1:1]
set yrange [-1:1]
unset hidd
set zrange [-15:4]
set ticslevel 0
set pm3d at st
splot log(x*x*y*y)
pause -1 "Press Enter."

set title "solution: use independent 'set zrange' and 'set pm3d zrange'"
unset surf
set pm3d zrange [-15:4]
set zrange [-15:60]
splot log(x*x*y*y)
pause -1 "Press Enter."

set title "color box is on by default at a certain position"
set samples 20; set isosamples 20
set autoscale
set key
set pm3d
set pm3d at s
set view 60,30
splot y
pause -1 "Press Enter."

set title "color box is on again, now with horizontal gradient"
set palette cbhorizontal
replot 
pause -1 "Press Enter."

set title "color box is switched off"
set palette nocb
replot 
pause -1 "Press Enter."

set title 'using again "set pm3d solid"'
set palette nocb
set pm3d solid
replot 
pause -1 "Press Enter."

set title "Any position of the color box has not been implemented. Volunteer?"
unset pm3d
replot
pause -1 "Press Enter."


set xlabel "X LABEL"
set ylabel "Y LABEL" 7

set sample 11; set isosamples 11
unset surface
set pm3d
set palette
set lmargin 0
set view 180,0,1.3; set yrange [*:*] reverse
#set term table; set out 'demo.dat'
#splot x+y<0 ? 1/0 : y

set pm3d flush begin
set title "Datafile with different nb of points in scans; pm3d flush begin"
splot 'triangle.dat'
#show pm3d
pause -1 "Press Enter."



set title "Datafile with different nb of points in scans; pm3d flush center"
set pm3d flush center scansforward
replot
pause -1 "Press Enter."

set title "Datafile with different nb of points in scans; pm3d flush end"
set pm3d flush end scansforward
replot
pause -1 "Press Enter."


reset
set title "only for enhanced terminals: 'set format z ...'"
set xlabel "X"
set ylabel "Y"
set sample 31; set isosamples 31
set xrange [-185:185]
set yrange [-185:185]
set format z "%.01t*10^{%T}"
unset surface
set border 4095
set ticslevel 0
set pm3d at s solid; set palette gray
splot abs(x)**3+abs(y)**3
pause -1 "Press Enter."


reset
set title "Demo for clipping of 2 rectangles will come. Now xrange is [0:2]"
set pm3d; set palette
set pm3d map

set xrange [0:2]
splot 'demoTwo.dat'
pause -1 "Press Enter."

set xrange [0:1.5]

set title "...now xrange is [0:1.5] and 'set pm3d clip1in'"
set pm3d clip1in
replot
pause -1 "Press Enter."

set title "...now xrange is [0:1.5] and 'set pm3d clip4in'"
set pm3d clip4in
replot
pause -1 "Press Enter."

print "End of pm3d demo."
