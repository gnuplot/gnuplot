#
#  Generate a set of figures to illustrate the various plot styles
#  EAM - July 2007
#
#

if (!exists("winhelp")) winhelp = 0
if (winhelp==0) set term pdfcairo mono font "Times,12" size 3.5,2.0 dashlength 0.2
if (winhelp==1) set term png font "Times,12" size 448,225 dashlength 0.2
demo = "../demo/"
if (winhelp==0) out = "./"
if (winhelp==1) out = "./windows/"

if (GPVAL_TERM eq "pngcairo" || GPVAL_TERM eq "png") ext=".png"
if (GPVAL_TERM eq "pdfcairo" || GPVAL_TERM eq "pdf") ext=".pdf"

#
# Line and point type plots  (same data plotted)
# ==============================================
#
set output out . 'figure_lines' . ext
set xrange [270:370]
unset xtics
unset ytics
set offset 10,10,4,2
set xzeroaxis
set lmargin screen 0.05
set rmargin screen 0.95
set bmargin screen 0.05
set tmargin screen 0.95

plot demo . 'silver.dat' u 1:($2-10.) title 'with lines' with lines
#
set output out . 'figure_points' . ext
plot demo . 'silver.dat' u 1:($2-10.):(1+rand(0)) title 'with points ps variable' \
     with points ps variable pt 6
#
set output out . 'figure_linespoints' . ext
f(x) = 8 + 8*sin(x/20)
plot demo . 'silver.dat' u 1:($2-10.) title 'with linespoints' \
     with linespoints pt 6 ps 1, \
     '' u 1:($2) title 'pointinterval -2' with lp pt 4 ps 1 pi -2
#
set output out . 'figure_fsteps' . ext
plot demo . 'silver.dat' u 1:($2-10.) title 'with fsteps' with fsteps
#
set output out . 'figure_steps' . ext
set style fill solid 0.25 noborder
plot demo . 'silver.dat' u 1:($2-10.) title 'with fillsteps' with fillsteps, \
                      '' u 1:($2-10.) title 'with steps' with steps lw 4
#
set output out . 'figure_histeps' . ext
plot demo . 'silver.dat' u 1:($2-10.) title 'with histeps' with histeps

#
# Simple bar charts  (same data plotted)
# ======================================
#
set output out . 'figure_boxes' . ext
set xzeroaxis
set boxwidth 0.8 relative
plot demo . 'silver.dat' u 1:($2-10.) with boxes title 'with boxes' fs solid 0.5
#
set output out . 'figure_boxerrorbars' . ext
set boxwidth 0.8 relative
plot demo . 'silver.dat' u 1:($2-10.):(3*rand(0)) with boxerrorbars title 'with boxerrorbars' fs empty
#
set output out . 'figure_impulses' . ext
set bmargin at screen .2
plot demo . 'silver.dat' u 1:($2-10.) with impulses title 'with impulses'
set bmargin at screen .05
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
set output out . 'figure_candlesticks' . ext
plot demo . 'candlesticks.dat' using 1:3:2:6:5 title 'with candlesticks' with candlesticks whiskerbar fs empty
#
set output out . 'figure_financebars' . ext
set bars 4
plot demo . 'candlesticks.dat' using 1:3:2:6:5 title 'with financebars' with financebars
set bars 1
#
set output out . 'figure_yerrorbars' . ext
plot demo . 'candlesticks.dat' using 1:4:3:5 with yerrorbars title 'with yerrorbars'
#
set output out . 'figure_yerrorlines' . ext
plot demo . 'candlesticks.dat' using 1:4:3:5 with yerrorlines title 'with yerrorlines'
#
set output out . 'figure_boxxyerrorbars' . ext
plot demo . 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with boxxyerrorbars title 'with boxxyerrorbars' fs empty
#
set output out . 'figure_xyerrorbars' . ext
plot demo . 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with xyerrorbars title 'with xyerrorbars'
#
set output out . 'figure_xyerrorlines' . ext
plot demo . 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with xyerrorlines title 'with xyerrorlines'
#
set output out . 'figure_xerrorbars' . ext
plot demo . 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.) \
     with xerrorbars title 'with xerrorbars'
#
set output out . 'figure_xerrorlines' . ext
plot demo . 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.) \
     with xerrorlines title 'with xerrorlines'
#
# Filled curves
# =============
#
set output out . 'figure_filledcurves' . ext
set style fill solid 1.0 border -1
set xrange [250:500]
set auto y
set key box title "with filledcurves"
plot demo . 'silver.dat' u 1:2:($3+$1/50.) w filledcurves above title 'above' lc rgb "grey10", \
               '' u 1:2:($3+$1/50.) w filledcurves below title 'below' lc rgb "grey75", \
               '' u 1:2 w lines lt -1 lw 1 title 'curve 1', \
               '' u 1:($3+$1/50.) w lines lt -1 lw 3 title 'curve 2'
#
# Boxplot
# =======
#
set output out . 'figure_boxplot' . ext
reset
set style fill solid 0.25 border -1
set yrange [-15:165]
set xrange [0.5:2.0]
set xtics ("A" 1, "B" 1.5) scale 0
set ytics nomirror
set border 2
set lmargin at screen 0.3
unset key
set style data boxplot
plot demo . 'silver.dat' using (1):2:(.25) ps 0.3, \
     '' using (1.5):(5*$3):(.25) ps 0.3
#
# Dots
# ====
#
set output out . 'figure_dots' . ext
reset
set parametric
set samples 500
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
set output out . 'figure_histclust' . ext
set style histogram clustered
plot demo . 'histopt.dat' using 1 fs solid 0.5, '' using 2 fs empty
#
set output out . 'figure_histerrorbar' . ext
set title "Histogram with error bars" offset 0,-1
set style fill solid border -1
set style histogram errorbars lw 2
plot demo . 'histerror.dat' using 2:3 fs solid 0.5 ti 'A', '' using 4:5 fs empty ti 'B'
#
set output out . 'figure_histrows' . ext
set style histogram rows
set title "Rowstacked" offset 0,-1
plot demo . 'histopt.dat' using 1 fs solid 0.5, '' using 2 fs empty
#
set output 'figure_newhist' . ext
set style histogram cluster
set style data histogram
unset title
set key auto column noinvert
set xtics 1 offset character 0,0.3
plot newhistogram "Set A", \
    '../demo/histopt.dat' u 1 t col, '' u 2 t col fs empty, \
    newhistogram "Set B" at 8, \
    '../demo/histopt.dat' u 1 t col, '' u 2 t col fs empty
#
set output out . 'figure_histcols' . ext
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

plot demo . 'histopt.dat' using 1 ti col lt 1, '' using 2 ti col fs solid lt 1
set style increment system
#
# Circles
#
reset
set output out . 'figure_circles' . ext
#set title "Circles of Uncertainty"
unset key
set size ratio -1
set xrange [-2.5:1.5]
set yrange [-1:2.5]
set xtics font "Times,10" format "%.1f" scale 0.5
set ytics font "Times,10" format "%.1f" scale 0.5
plot demo . 'optimize.dat' with circles lc rgb "gray" fs transparent solid 0.2 nobo,\
     demo . 'optimize.dat' u 1:2 with linespoints lw 2 pt 7 ps 0.3 lc rgb "black"
#
# Ellipses
#
reset
set output out . 'figure_ellipses' . ext
unset xtics; unset ytics

plot demo . 'ellipses.dat' u 1:2:3:4:5 with ellipses units xy title "with ellipses",\
     '' u 1:2:3:4:5 with ellipses units xx notitle,\
     '' u 1:2:3:4:5 with ellipses units yy notitle
#
# 2D heat map from an array of in-line data
#
reset
set output out . 'figure_heatmap' . ext
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
set bmargin screen 0.40
unset key
set samples 20, 20
set isosamples 21, 21
set xlabel "X axis" rotate parallel offset 0,-1
set ylabel "Y axis" rotate parallel offset 0,-1
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
set output out . 'figure_surface' . ext
splot sin(x) * cos(y) with lines lt -1

set contour base
set cntrparam levels auto 9
unset key
set title "3D surface with projected contours" 

set output out . 'figure_surface+contours' . ext
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

set output out . 'figure_mapcontours' . ext
splot sin(x) * cos(y)

reset
set output out . 'figure_rgb3D' . ext
set title "RGB image mapped onto a plane in 3D" offset 0,1
set xrange [ -10 : 137 ]
set yrange [ -10 : 137 ]
set zrange [  -1 :   1 ]
set xyplane at -1
set bmargin at screen 0.25
set xtics offset 0,0 font "Times,10"
set ytics offset 0,0 font "Times,10"
set view 45, 25, 1.0, 1.35
set grid
unset key
set format z "%.1f"
splot demo . 'blutux.rgb' binary array=128x128 flip=y format='%uchar%uchar%uchar' with rgbimage

reset
set output out . 'figure_scaled_image' . ext
set title "Rescaled image used as plot element"

set xrange [ -10 : 150 ]
set yrange [   0 : 200 ]
set y2range[   0 : 200 ]

set y2tics
set grid y

set key title "Building Heights\nby Neighborhood"
set key box

set xtics   ("NE" 72.0, "S" 42.0, "Downtown" 12.0, "Suburbs" 122.0)  scale 0.0

plot demo . 'bldg.png' binary filetype=png origin=(0,0)  dx=0.5 dy=1.5 with rgbimage notitle, \
     demo . 'bldg.png' binary filetype=png origin=(60,0) dx=0.5 dy=1 with rgbimage notitle, \
     demo . 'bldg.png' binary filetype=png origin=(30,0) dx=0.5 dy=0.7 with rgbimage notitle, \
     demo . 'bldg.png' binary filetype=png origin=(110,0) dx=0.5 dy=0.35 with rgbimage notitle

#
#
# Demonstrates how to pull font size from a data file column
#
reset
Scale(size) = 0.25*sqrt(sqrt(column(size)))
CityName(String,Size) = sprintf("{/=%d %s}", Scale(Size), stringcolumn(String))

set termoption enhanced
set output out . 'figure_labels' . ext
unset xtics
unset ytics
unset key
set border 0
set size square
set datafile separator "\t"
plot demo . 'cities.dat' using 5:4:($3 < 5000 ? "-" : CityName(1,3)) with labels

#
reset
set output out . 'figure_polar' . ext
unset border
set style fill   solid 0.50 border
set grid polar 0.523599 lt 0 lw 1
set key title "bounding radius 2.5"
set key at screen 0.95, screen 0.95
set key noinvert samplen 0.7
set polar
set size ratio 1 1,1
set noxtics
set noytics
set rrange [ 0.100000 : 4.00000 ]
butterfly(x)=exp(cos(x))-2*cos(4*x)+sin(x/12)**5
GPFUN_butterfly = "butterfly(x)=exp(cos(x))-2*cos(4*x)+sin(x/12)**5"
plot 3.+sin(t)*cos(5*t) with filledcurve above r=2.5 notitle, \
     3.+sin(t)*cos(5*t) with line

reset
#
# New syntax features
#
set output out . 'figure_newsyntax' . ext

unset xtics
unset ytics
unset border
set yrange [-0.3:1.3]
set multiplot layout 2,2
fourier(k, x) = sin(3./2*k)/k * 2./3*cos(k*x)
do for [power = 0:3] {
    TERMS = 10**power
    set xlabel sprintf("%g term Fourier series",TERMS)
    plot 0.5 + sum [k=1:TERMS] fourier(k,x) notitle 
}
unset multiplot


# close last file
unset outp

reset
