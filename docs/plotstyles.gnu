#
#  Generate a set of figures to illustrate the various plot styles
#  EAM - July 2007
#
#
set term pdfcairo mono font "Times,7" size 3.5,2.0 dashlength 0.2
#
# Line and point type plots  (same data plotted)
# ==============================================
#
set output 'figure_lines.pdf'
set xrange [270:370]
unset xtics
unset ytics
set offset 10,10,4,2
set xzeroaxis
set lmargin screen 0.05
set rmargin screen 0.95
set bmargin screen 0.05
set tmargin screen 0.95

plot '../demo/silver.dat' u 1:($2-10.) title 'with lines' with lines
#
set output 'figure_points.pdf'
plot '../demo/silver.dat' u 1:($2-10.):(1+rand(0)) title 'with points ps variable' \
     with points ps variable pt 6
#
set output 'figure_linespoints.pdf'
f(x) = 8 + 8*sin(x/20)
plot '../demo/silver.dat' u 1:($2-10.) title 'with linespoints' \
     with linespoints pt 6 ps 1, \
     '' u 1:($2) title 'pointinterval -2' with lp pt 4 ps 1 pi -2
#
set output 'figure_fsteps.pdf'
plot '../demo/silver.dat' u 1:($2-10.) title 'with fsteps' with fsteps
#
set output 'figure_steps.pdf'
plot '../demo/silver.dat' u 1:($2-10.) title 'with steps' with steps
#
set output 'figure_histeps.pdf'
plot '../demo/silver.dat' u 1:($2-10.) title 'with histeps' with histeps

#
# Simple bar charts  (same data plotted)
# ======================================
#
set output 'figure_boxes.pdf'
set xzeroaxis
set boxwidth 0.8 relative
plot '../demo/silver.dat' u 1:($2-10.) with boxes title 'with boxes' fs solid 0.5
#
set output 'figure_boxerrorbars.pdf'
set boxwidth 0.8 relative
plot '../demo/silver.dat' u 1:($2-10.):(3*rand(0)) with boxerrorbars title 'with boxerrorbars' fs empty
#
set output 'figure_impulses.pdf'
plot '../demo/silver.dat' u 1:($2-10.) with impulses title 'with impulses'
#
# Error bars and whisker plots
# ============================
#
set xrange [0:11]
set yrange [0:10]
set boxwidth 0.2
unset xzeroaxis
unset offset
#
set output 'figure_candlesticks.pdf'
plot '../demo/candlesticks.dat' using 1:3:2:6:5 title 'with candlesticks' with candlesticks whiskerbar fs empty
#
set output 'figure_financebars.pdf'
set bars 4
plot '../demo/candlesticks.dat' using 1:3:2:6:5 title 'with financebars' with financebars
set bars 1
#
set output 'figure_yerrorbars.pdf'
plot '../demo/candlesticks.dat' using 1:4:3:5 with yerrorbars title 'with yerrorbars'
#
set output 'figure_yerrorlines.pdf'
plot '../demo/candlesticks.dat' using 1:4:3:5 with yerrorlines title 'with yerrorlines'
#
set output 'figure_boxxyerrorbars.pdf'
plot '../demo/candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with boxxyerrorbars title 'with boxxyerrorbars' fs empty
#
set output 'figure_xyerrorbars.pdf'
plot '../demo/candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with xyerrorbars title 'with xyerrorbars'
#
set output 'figure_xyerrorlines.pdf'
plot '../demo/candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with xyerrorlines title 'with xyerrorlines'
#
set output 'figure_xerrorbars.pdf'
plot '../demo/candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.) \
     with xerrorbars title 'with xerrorbars'
#
set output 'figure_xerrorlines.pdf'
plot '../demo/candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.) \
     with xerrorlines title 'with xerrorlines'
#
# Filled curves
# =============
#
set output 'figure_filledcurves.pdf'
set style fill solid 1.0 border -1
set xrange [250:500]
set auto y
set key box title "with filledcurves"
plot '../demo/silver.dat' u 1:2:($3+$1/50.) w filledcurves above title 'above' lc rgb "grey10", \
               '' u 1:2:($3+$1/50.) w filledcurves below title 'below' lc rgb "grey75", \
               '' u 1:2 w lines lt -1 lw 1 title 'curve 1', \
               '' u 1:($3+$1/50.) w lines lt -1 lw 3 title 'curve 2'
#
# Dots
# ====
#
set output 'figure_dots.pdf'
reset
set parametric
set samples 1000
set isosamples 2,2 # Smallest possible
set view map
set lmargin screen 0.05
set rmargin screen 0.95
set tmargin screen 0.95
set bmargin screen 0.05
unset xtics
unset ytics
set xrange [-3:3]
set yrange [-4:4]
splot invnorm(rand(0)),invnorm(rand(0)),invnorm(rand(0)) with dots notitle
#
# Histograms
# ==========
#
reset
set style data histogram
set boxwidth 0.9 rel
set key auto column invert
set yrange [0:*]
set offset 0,0,2,0
unset xtics
set tmargin 1
#
set output 'figure_histclust.pdf'
set style histogram clustered
plot 'histopt.dat' using 1 fs solid 0.5, '' using 2 fs empty
#
set output 'figure_histrows.pdf'
set style histogram rows
set title "Rowstacked" offset 0,-1
plot 'histopt.dat' using 1 fs solid 0.5, '' using 2 fs empty
#
set output 'figure_histcols.pdf'
set style histogram columns
set title "Columnstacked" offset 0,-1
set boxwidth 0.8 rel
set xtics

set style line 1 lt rgb "gray0"
set style line 2 lt rgb "white"
set style line 3 lt rgb "gray40"
set style line 4 lt rgb "gray70"
set style increment user
set style fill solid 1.0 border -1

plot 'histopt.dat' using 1 ti col lt 1, '' using 2 ti col fs solid lt 1
set style increment system
#
# Circles
#
reset
set output 'figure_circles.pdf'
#set title "Circles of Uncertainty"
unset key
set size ratio -1
set xrange [-2.5:1.5]
set yrange [-1:2.5]
set xtics font "Times,5" format "%.1f" scale 0.5
set ytics font "Times,5" format "%.1f" scale 0.5
plot '../demo/optimize.dat' with circles lc rgb "gray" fs transparent solid 0.2 nobo,\
     '../demo/optimize.dat' u 1:2 with linespoints lw 2 pt 7 ps 0.3 lc rgb "black"
#
# 2D heat map from an array of in-line data
#
reset
set output 'figure_heatmap.pdf'
set title "2D Heat map from in-line array of values" offset 0,-1
unset key
set bmargin 1
set tmargin 3
set tics scale 0
unset cbtics
unset xtics
set xrange  [-0.5:4.5]
set x2range [-0.5:4.5]
set yrange  [-0.5:3.5] reverse
set x2tics 0,1
set ytics  0,1
set palette rgbformula -3,-3,-3
plot '-' matrix with image
5 4 3 1 0
2 2 0 0 1
0 0 0 1 0
0 1 2 4 3
e
e

#
# 3D Plot styles
# ==============
#
reset
set view 75, 33, 1.0, 0.82
set bmargin screen 0.18
unset key
set samples 20, 20
set isosamples 21, 21
set xlabel "X axis" 
set ylabel "Y axis" 
set zlabel "Z axis" 
set zlabel  offset 2,0 rotate by -90
unset xtics
unset ytics
unset ztics
set border lw 2.0
set xrange [-3:3]
set yrange [-3:3]
set zrange [-1.5:1]
set hidden3d offset 1

set title "3D surface plot with hidden line removal"  offset 0,1
set output 'figure_surface.pdf'
splot sin(x) * cos(y) with lines lt -1

set contour base
set cntrparam levels auto 9
unset key
set title "3D surface with projected contours" 

set output 'figure_surface+contours.pdf'
splot sin(x) * cos(y) with lines lt -1

unset view
set view map
unset surface
unset grid
set bmargin screen 0.15
set xlabel "X axis" offset 0,2 
set tmargin
set rmargin
set lmargin
set title "projected contours using 'set view map'" offset 0,-1

set output 'figure_mapcontours.pdf'
splot sin(x) * cos(y)

reset
set output "figure_rgb3D.pdf"
set title "RGB image mapped onto a plane in 3D" offset 0,1
set xrange [ -10 : 137 ]
set yrange [ -10 : 137 ]
set zrange [  -1 :   1 ]
set xyplane at -1
set bmargin at screen 0.25
set xtics offset 0,0 font "Times,5"
set ytics offset 0,0 font "Times,5"
set view 45, 25, 1.0, 1.35
set grid
unset key
set format z "%.1f"
splot '../demo/blutux.rgb' binary array=128x128 flip=y format='%uchar%uchar%uchar' with rgbimage

#
#
# Demonstrates how to pull font size from a data file column
#
reset
Scale(size) = 0.25*sqrt(sqrt(column(size)))
CityName(String,Size) = sprintf("{/=%d %s}", Scale(Size), stringcolumn(String))

set termoption enhanced
set output 'figure_labels.pdf'
unset xtics
unset ytics
unset key
set border 0
set size square
set datafile separator "\t"
plot '../demo/cities.dat' using 5:4:($3 < 5000 ? "-" : CityName(1,3)) with labels
reset
