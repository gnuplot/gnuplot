#
#  Generate a set of figures to illustrate the various plot styles.
#  These figures are imported into the pdf and html versions of the
#  User Manual.
#
#  EAM - July 2007
#

if (strstrt(GPVAL_TERMINALS, " windows ") == 0) {
   fontspec = "Times,12"
} else {
   fontspec = "Tahoma,12"
}
MANUAL_FIGURES = 1

if (!exists("winhelp")) winhelp = 0
if (winhelp > 0) {
#   prefer pngcairo over gd based png
    if (strstrt(GPVAL_TERMINALS, " pngcairo ") > 0) {
        set term pngcairo font fontspec size 448,225 dashlength 0.2 fontscale 0.6
    } else {
        set term png font fontspec size 448,225 dashlength 0.2 fontscale 0.6
    }
    out = "./windows/"
} else if (GNUTERM eq "svg") {
    set term svg font 'Calisto MT,14' size 600,400
    out = "./html/"
} else if (GNUTERM eq "tikz") {
    set term tikz color fontscale 0.75 clip size 3.0in, 1.7in
    out = "./"
} else {
    set term pdfcairo mono font fontspec size 3.5,2.0 dashlength 0.2
# pdfs that need colour have their own terminal setting, check below
    out = "./"
}

set loadpath '../demo'

if (GPVAL_TERM eq "pngcairo" || GPVAL_TERM eq "png") ext=".png"
if (GPVAL_TERM eq "pdfcairo" || GPVAL_TERM eq "pdf") ext=".pdf"
if (GPVAL_TERM eq "svg") ext=".svg"
if (GPVAL_TERM eq "tikz") ext=".tex"

set encoding utf8

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

plot 'silver.dat' u 1:($2-10.) title 'with lines' with lines
#
set output out . 'figure_points' . ext
plot 'silver.dat' u 1:($2-10.):(1+rand(0)) title 'with points ps variable' \
     with points ps variable pt 6
#
set output out . 'figure_linespoints' . ext
set key opaque height 1
f(x) = 8 + 8*sin(x/20)
plot 'silver.dat' u 1:($2-10.) title 'with linespoints' \
     with linespoints pt 6 ps 1, \
     '' u 1:($2) title 'pointinterval -2' with lp pt 4 ps 1 pi -2, \
     '' u 1:($2+10.) with lp pt "α" pi -1 font ",18" title 'with lp pt "α" pi -1'
set key noopaque
#
set output out . 'figure_fsteps' . ext
plot 'silver.dat' u 1:($2-10.) title 'with fsteps' with fsteps, \
               '' u 1:($2-10.) with points pt 7 ps 0.5 lc "black" title 'data points'
#
set output out . 'figure_steps' . ext
set style fill solid 0.25 noborder
plot 'silver.dat' u 1:($2-10.) title 'with fillsteps' with fillsteps, \
               '' u 1:($2-10.) title 'with steps' with steps lw 3 dt solid, \
               '' u 1:($2-10.) with points pt 7 ps 0.5 lc "black" title 'data points'
#
set output out . 'figure_histeps' . ext
plot 'silver.dat' u 1:($2-10.) title 'with histeps' with histeps, \
               '' u 1:($2-10.) with points pt 7 ps 0.5 lc "black" title 'data points'
#
symbol(z) = "•□+⊙♠♣♡♢"[int(z):int(z)]
set output out . 'figure_labels2' . ext
plot 'silver.dat' u 1:($2-10.):(symbol(1+int($0)%8)) \
     with labels font ",18" title "with labels"

#
# Simple bar charts  (same data plotted)
# ======================================
#
# (no reset, keep settings from previous example set)
set output out . 'figure_boxes' . ext
set xzeroaxis
set boxwidth 0.8 relative
plot 'silver.dat' u 1:($2-10.) with boxes title 'with boxes' fs solid 0.5
#
set output out . 'figure_boxerrorbars' . ext
set boxwidth 0.8 relative
plot 'silver.dat' u 1:($2-10.):(3*rand(0)) with boxerrorbars title 'with boxerrorbars' fs solid 0.5 fc "blue"
#
set output out . 'figure_impulses' . ext
set bmargin at screen .2
plot 'silver.dat' u 1:($2-10.) with impulses lw 2 title 'with impulses'
set bmargin at screen .05

#
# Error bars and whisker plots
# ============================
#
# (no reset, keep settings from previous example sets)
set xrange [0:11]
set yrange [0:10]
set boxwidth 0.2
unset xzeroaxis
unset offset
#
set output out . 'figure_candlesticks' . ext
plot 'candlesticks.dat' using 1:3:2:6:5 title 'with candlesticks' with candlesticks whiskerbar fs solid 0.5 fc "cyan"
#
set output out . 'figure_financebars' . ext
set bars 4
plot 'candlesticks.dat' using 1:3:2:6:5 title 'with financebars' with financebars
set bars 1
#
set output out . 'figure_yerrorbars' . ext
plot 'candlesticks.dat' using 1:4:3:5 with yerrorbars title 'with yerrorbars'
#
set output out . 'figure_yerrorlines' . ext
plot 'candlesticks.dat' using 1:4:3:5 with yerrorlines title 'with yerrorlines'
#
set output out . 'figure_boxxyerror' . ext
plot 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with boxxyerror title 'with boxxyerror' fs empty
#
set output out . 'figure_xyerrorbars' . ext
plot 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with xyerrorbars title 'with xyerrorbars'
#
set output out . 'figure_xyerrorlines' . ext
plot 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.):3:5 \
     with xyerrorlines title 'with xyerrorlines'
#
set output out . 'figure_xerrorbars' . ext
plot 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.) \
     with xerrorbars title 'with xerrorbars'
#
set output out . 'figure_xerrorlines' . ext
plot 'candlesticks.dat' using 1:4:($1-sin($1)/2.):($1+sin($1)/2.) \
     with xerrorlines title 'with xerrorlines'

# 
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
plot 'silver.dat' using (1):2:(.25) ps 0.3, \
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
# Circles
# =======
#
reset
set output out . 'figure_circles' . ext
#set title "Circles of Uncertainty"
unset key
set size ratio -1
set xrange [-2.5:1.5]
set yrange [-1:2.5]
set xtics font ",10" format "%.1f" scale 0.5
set ytics font ",10" format "%.1f" scale 0.5
plot 'optimize.dat' with circles lc rgb "gray" fs transparent solid 0.2 nobo,\
     'optimize.dat' u 1:2 with linespoints lw 2 pt 7 ps 0.3 lc rgb "black"
     
#
# Ellipses
# ========
#
reset
set output out . 'figure_ellipses' . ext
unset xtics; unset ytics

plot 'ellipses.dat' u 1:2:3:4:5 with ellipses units xy title "with ellipses",\
     '' u 1:2:3:4:5 with ellipses units xx notitle,\
     '' u 1:2:3:4:5 with ellipses units yy notitle

#
# Following example plots will be set in colour mode for pdf output
#
if (GPVAL_TERM eq "pdfcairo") \
    set term pdfcairo color font fontspec size 3.5,2.0 dashlength 0.2
if (GNUTERM eq "tikz") \
    set term tikz color fontscale 0.75 clip size 3.0in, 1.7in

#
# 2D heat map from an array of in-line data
# =========================================
#
reset
set output out . 'figure_heatmap' . ext
set title "2D Heat map from in-line array of values" offset 0,0
$HEATMAP << EOD
5 4 3 1 0
2 2 0 0 1
0 0 0 1 0
0 1 2 4 3
EOD
unset key
set bmargin 1
set rmargin at screen 0.9
set tmargin 3
set tics scale 0
unset cbtics
unset xtics
unset colorbox
set xrange  [-0.5:4.5]
set x2range [-0.5:4.5]
set yrange  [3.5:-0.5]
set x2tics 0,1
set ytics  0,1
set palette rgbformula -3,-3,-3
plot $HEATMAP matrix with image pixels
#
# 3D Plot styles
# ==============
#
reset
set view 69, 200, 1.18, 0.82
set view 69, 200, 1.18, 1.00
set bmargin at screen 0.3
unset key
set samples 20, 20
set isosamples 21, 21
#set xlabel "X axis" rotate parallel offset 0,-1
#set ylabel "Y axis" rotate parallel offset 0,-1
#set zlabel "Z axis" 
#set zlabel  offset 2,0 rotate by -90
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
set xrange [-3:2]
set yrange [-2:3]
unset surface
unset grid
set xlabel "X axis" offset 0,2 
set ylabel "Y axis" rotate
set rmargin 0
set lmargin at screen .1
set tmargin 0
set bmargin at screen .15
set title "projected contours using 'set view map'" offset 0,0

set output out . 'figure_mapcontours' . ext
set style textbox opaque noborder margins 0.25,0.25
set cntrlabel font ",8"
splot sin(x) * cos(y), sin(x) * cos(y) with labels boxed

reset

set output out . 'figure_3Dboxes' . ext
set title "Full treatment: 3D boxes with pm3d depth sorting and lighting" 
set boxwidth 0.4 absolute
set boxdepth 0.3
set style fill   solid 1.00 border
set grid xtics ytics ztics
set grid vertical layerdefault   lt 0 lw 1,  lt 0 lw 1
unset key
set wall z0  fc  rgb "slategrey"  fillstyle  transparent solid 0.50 border lt -1
set view 59, 24, 1, 1
set bmargin screen 0
set xyplane at 0
set xtics 1; set xtics add ("" 0, "" 11)
set ytics add ("" 0, "" 6)
set xrange [ 0.0 : 11.0 ]
set yrange [ 0.0 : 6.0 ]
set pm3d depthorder base
set pm3d interpolate 1,1 border lw 1.000 dashtype solid
set pm3d lighting primary 0.5 specular 0.2 spec2 0
rgbfudge(x) = x*51*32768 + (11-x)*51*128 + int(abs(5.5-x)*510/9.)
#ti(col) = sprintf("%d",col)
#
splot for [col=1:5] 'candlesticks.dat' using 1:(col):(col*column(col)):(rgbfudge($1))       with boxes fc rgb variable

#
# RGB image mapping
# =================
#
reset
set output out . 'figure_rgb3D' . ext
set title "RGB image mapped onto a plane in 3D" offset 0,1
set xrange [ -10 : 137 ]
set yrange [ -10 : 137 ]
set zrange [  -1 :   1 ]
set xyplane at -1
set bmargin at screen 0.25
set xtics offset 0,0 font ",10"
set ytics offset 0,0 font ",10"
set view 45, 25, 1.0, 1.35
set grid
unset key
set format z "%.1f"
splot 'blutux.rgb' binary array=(128,128) flip=y format='%uchar%uchar%uchar' with rgbimage

#
# Sparse matrix data
#
reset
set output out . 'figure_sparsematrix' . ext
$SPARSEDATA << EOD
1 1 10
1 2 20
1 3 30
1 4 40
2 2 10
2 3 50
2 4 60
3 3 10
3 4 20
4 4 10
EOD
set label 1 center at graph 0.75,0.1 "Intercity Transit"
set tics scale 0
set x2tics ("Atlantis" 1, "Finias" 2, "Ys" 3, "Erewhon" 4)
set ytics  ("Atlantis" 1, "Finias" 2, "Ys" 3, "Erewhon" 4)
set palette defined (0 "#DDDDDD", 0.3 "gold", 0.6 "orange", 1.0 "dark-blue")
set palette maxcolors 6
set cbrange [5:65]
set cbtics format "⋎%g."   add (" Fare " 65)
unset border; unset xtics; unset key
set auto noextend
plot $SPARSEDATA sparse matrix=(4,4) origin=(1,1) with image

#
# Rescale image as plot element
# =============================
#
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

plot 'bldg.png' binary filetype=png origin=(0,0)  dx=0.5 dy=1.5 with rgbimage notitle, \
     'bldg.png' binary filetype=png origin=(60,0) dx=0.5 dy=1 with rgbimage notitle, \
     'bldg.png' binary filetype=png origin=(30,0) dx=0.5 dy=0.7 with rgbimage notitle, \
     'bldg.png' binary filetype=png origin=(110,0) dx=0.5 dy=0.35 with rgbimage notitle

#
# Demonstrates how to pull font size from a data file column
# ==========================================================
#
reset
Scale(size) = 0.33*sqrt(sqrt(column(size)))
CityName(String,Size) = sprintf("{/=%d %s}", Scale(Size), stringcolumn(String))

set termoption enhanced
set output out . 'figure_labels1' . ext
unset xtics
unset ytics
unset key
set border 0
set size square
set datafile separator "\t"
plot 'cities.dat' using 5:4:($3 < 5000 ? "-" : CityName(1,3)) with labels

#
# Use of `keyentry` to construct a key
# ====================================
#
reset
set output out . 'figure_keyentry' . ext
set title "Construct key from custom entries"
set tics scale 0
unset xtics
set xrange  [-0.5:4.5]
set x2range [-0.5:4.5]
set yrange  [3.5:-0.5]
set x2tics 0,1
set ytics  0,1
set palette rgbform -7,2,-7
unset colorbox
set style fill solid border lc "black"
set key outside right center reverse Left samplen 1
set key title "Outcomes" left

plot $HEATMAP matrix with image pixels notitle, \
    keyentry with boxes fc palette cb 0 title "no effect", \
    keyentry with boxes fc palette cb 1 title "threshold", \
    keyentry with boxes fc palette cb 3 title "typical range", \
    keyentry with labels title "as reported in [12]", \
    keyentry with boxes fc palette cb 5 title "strong effect"

#
# Polar plot
# ==========
#
reset
set output out . 'figure_polar' . ext
unset border
set tmargin 2
set bmargin 2
set style fill   solid 0.50 border
set grid polar 0.523599 lt 0 lw 1
set key title "bounding radius 2.5"
set key at screen 0.95, screen 0.95
set key noinvert samplen 0.7
set polar
set size ratio 1 1,1
unset xtics
unset ytics
set ttics ("0" 0, "π/2" 90, "π" 180, "3π/2" 270)
set rrange [ 0.1 : 4.0 ]
butterfly(x)=exp(cos(x))-2*cos(4*x)+sin(x/12)**5
plot 3.+sin(t)*cos(5*t) with filledcurve above r=2.5 notitle, \
     3.+sin(t)*cos(5*t) with line

#
# Vectors
# =======
#
reset
set output out . 'figure_vectors' . ext
set label 1 "Vector field {/:Italic F(x,y) = (ky,-kx)}"
set label 1 at 0.5, 3.0 left
unset key
unset clip one
unset border
set style arrow 1 head filled size .2, 20. lw 2 lc "slateblue1"
set samples 5, 5
set isosamples 5, 5
set size ratio 1 1
set xzeroaxis
set yzeroaxis
set xtics axis add ("" 0)
set ytics axis add ("" 0)
set urange [ -2.0 : 2.0 ]
set vrange [ -2.0 : 2.0 ]

plot '++' using 1:2:($2*0.4):(-$1*0.4) with vectors as 1

     
#
# Missing Datapoints
# ==================
# (This is not an actual demonstration of the effect, just produces a lookalike)
# The pdf version of this is supplied separately to better fit with the LaTeX document.
#
reset

$data1<<EOD
1 10
2 20
2 3
4 40
5 50
EOD
$data2<<EOD
1 10
2 20
4 40
5 50
EOD
$data3<<EOD
1 10
2 20

4 40
5 50
EOD

if (winhelp != 0) {
set output out . 'figure_missing' . ext

set xrange [0.1:5.9]
set yrange [1:55]
set xtics offset 0, graph .09

set multiplot layout 2,4 columnsfirst margins 0.15,.98,0.1,.98 spacing 0.1
set ylabel "Old"
set label 1 at .5,45 "(a)"
plot $data1 w lp pt 7 notitle
set ylabel "New"
plot $data2 w lp pt 7 notitle
unset ylabel
set label 1 at .5,45 "(b)"
plot $data2 w lp pt 7 notitle
plot $data2 w lp pt 7 notitle
set label 1 at .5,45 "(c)"
plot $data2 w lp pt 7 notitle
plot $data3 w lp pt 7 notitle
set label 1 at .5,45 "(d)"
plot $data3 w lp pt 7 notitle
plot $data3 w lp pt 7 notitle

unset multiplot
}

#
# New syntax features
# ===================
#
reset
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
    plot 0.5 + sum [k=1:TERMS] fourier(k,x) notitle lt -1
}
unset multiplot


#
# Filledcurves used to represent error on y
#
reset
set output out . 'figure_yerrorfill' . ext
set logscale y
set ytics  norangelimit logscale autofreq 
set title "Ag 108 decay data" 
set xlabel "Time (sec)" 
set ylabel "Rate" 
Shadecolor = "#80E0A080"
plot 'silver.dat' using 1:($2+$3):($2-$3) with filledcurve fc rgb Shadecolor title "Shaded error region", \
     '' using 1:2 smooth mcspline lw 1.5  title "Monotonic spline through data"     
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
plot 'histopt.dat' using 1 fs solid 0.5, '' using 2 fs empty
#
set output out . 'figure_histerrorbar' . ext
set title "Histogram with error bars" offset 0,-1
set style fill solid border -1
set style histogram errorbars lw 2
set datafile separator tab columnhead
plot 'histerror.dat' using 2:3 fs solid 0.5 ti 'A', '' using 4:5 fs empty ti 'B'
#
set output out . 'figure_histrows' . ext
set style histogram rows
set title "Rowstacked" offset 0,-1
plot 'histopt.dat' using 1 fs solid 0.5, '' using 2 fs empty
#
set output out . 'figure_newhist' . ext
set style histogram cluster
set style data histogram
unset title
set key auto column noinvert
set xtics 1 offset character 0,0.3
plot newhistogram "Set A", \
    'histopt.dat' u 1 t col, '' u 2 t col fs empty, \
    newhistogram "Set B" at 8, \
    'histopt.dat' u 1 t col, '' u 2 t col fs empty
#
set output out . 'figure_histcols' . ext
set style histogram columnstacked
set title "Columnstacked" offset 0,-1
set boxwidth 0.8 rel
set xtics

if (winhelp !=0) {
# greyscale rgb for png
set linetype 11 lc rgb "gray0"
set linetype 12 lc rgb "white"
set linetype 13 lc rgb "gray40"
set linetype 14 lc rgb "gray70"
set style fill solid 1.0 border -1

plot newhistogram lt 11, \
     'histopt.dat' using 1 title column, \
     '' using 2 title column
} else {
# patterned fill for pdf
# set style fill pattern
plot 'histopt.dat' using 1 title column, \
     '' using 2 title column
}


#
# Parallel axis plot
# ==================
#
reset
set output out . 'figure_parallel' . ext
unset border
unset key
set xrange [.5:4.5]
unset ytics
set xtics 1,1,4 format "axis %g" scale 0,0
set for [axis=1:4] paxis axis tics
set paxis 2 range [0:30]
set paxis 4 range [-1:15]
set paxis 4 tics  auto 1 left offset 5

set style data parallelaxes
plot 'silver.dat' using 2:(int($0/25)) lt 1 lc variable, '' using 3, '' using 1, '' using ($3/2)

#
# Filled curves
# =============
#
reset
set output out . 'figure_filledcurves' . ext
set style fill solid 0.75 border -1
set xrange [250:500]
set auto y
set key box title "with filledcurves"
plot 'silver.dat' u 1:2:($3+$1/50.) w filledcurves above title 'above' lc rgb "honeydew", \
               '' u 1:2:($3+$1/50.) w filledcurves below title 'below' lc rgb "dark-violet", \
               '' u 1:2 w lines lt -1 lw 1 title 'curve 1', \
               '' u 1:($3+$1/50.) w lines lt -1 lw 4 title 'curve 2'

#
# Bee swarm plots
#
reset
set output out . 'figure_jitter' . ext
npts = 60
set print $data
do for [i=1:npts] {
    print sprintf("%d %8.5g", (i%5) ? 3 : 4, 25.+10.*invnorm(rand(0)))
}
unset print
set xrange [2.5:4.5]
set xtics ("A" 3, "B" 4)
set border 2
set xtics nomirror scale 0
set ytics nomirror rangelimited scale 0
unset key

set multiplot layout 1, 2 
set jitter over 0.5 spread 1.6 swarm
set title "swarm (default)"
plot $data using 1:2:1 with points pt 6 ps 0.8 lc variable
set jitter over 0.5 spread 1.6 square
set title "square"
replot
unset multiplot

#
# convex hulls
#
reset
set output out . 'figure_convex_hull' . ext
unset key
set border 3
set rmargin 0
set tics nomirror rangelimited
set style fill transparent solid 0.1 border
set title "Convex hull bounding scattered points" offset 0,-1
set style line 1 lc "black" pt 6 ps 0.5
set style line 2 lc "forest-green" pt 7 ps 0.5
set xrange [-30:30]
set yrange [-30:30]

plot for [i=0:1] 'hull.dat' index i with points ls (i+1), \
     for [i=0:1] '' index i convexhull with filledcurve ls (i+1)

# Custom key placement
reset
set output out . 'figure_multiple_keys' . ext
set xtics font ",6"  offset 0,1
set label 1 font ",10"
set key font ",9" spacing 0.5
load 'custom_key.dem'
reset

# zerrorfill demo
set output out . 'figure_zerror' . ext
set view 60, 30, 1., 1.5
set yrange [0.5:5.5]
set zrange [1:*]
set log z
set border 127
set pm3d depth base
set style fill transparent solid 0.5
set xyplane at 1
set key opaque box

splot for [k=5:1:-1] 'silver.dat' using 1:(k):2:3 with zerror lt black fc lt k title "k = ".k

reset

# pm3d plot with solid fill
set output out . 'figure_pm3dsolid' . ext
set border 4095 lw 2
unset key
unset colorbox
set view 65, 34, 1.10, 1.1
set samples 40, 40
set isosamples 40, 40
set xyplane 0
set pm3d depthorder border linewidth 0.100
set pm3d clip z 
set pm3d lighting primary 0.5 specular 0.2 spec2 0.4

set style line 101 lc "gold"
set style line 102 lc "cyan"

set xrange [-1 : 5]
set yrange [-3 : 3]
set zrange [-10 : 10]
set xtics 2 offset 0,-0.5
set ytics 2 offset 0,-0.5
set ztics 5

f(x,y) = x**2 + y**2 * (1 - x)**3

set title "splot with pm3d, solid fillcolor" offset 0,1
splot f(x,y) with pm3d fc ls 101

# add walls to give added depth
set output out . 'figure_walls' . ext

set wall x0
set wall y1
set wall z0
set xyplane 0
set sample 21; set isosample 21
set tmargin 0; set bmargin 0
unset title
set pm3d interp 1,2 border lt -1 lw 0.5
set style fill solid 1.0
splot f(x,y) with pm3d fc "goldenrod"
reset

# Fence plot
set output out . 'figure_fenceplot' . ext
set title "fence plot constructed with zerrorfill" 
unset key
set zrange [-1:1]
set xtics ( "A" -2, "B" -1, "C" 0, "D" 1, "E" 2 ) scale 0 offset -1
set xrange [-3:2]
set xyplane at -1.1
set yrange [-0.5:0.5]
set ytics format "  %.1f" scale 0
set ylabel "Y value"  rotate parallel offset -2
unset ztics
set zlabel "Z value" rotate offset 5
sinc(u,v) = sin(sqrt(u**2+v**2)) / sqrt(u**2+v**2)
set style fill  solid 0.5 noborder
splot for [x=-2:2][y=-50:50:3] '+' using (x):($1/100.):(-1):(-1):(sinc($1/10., 1.+2*x)) with zerrorfill
reset

# Voxel data
set output out. 'figure_isosurface' . ext
set title "isosurface generated from voxel data" offset 0,1
set vgrid $helix size 20
set vxrange [-2:2]; set vyrange [-2:2]; set vzrange [0:11]
set xrange [-2:2]; set yrange [-2:2]; set zrange [0:11]
set xyplane at 0
set style fill solid 0.3
set pm3d depthorder border lc "blue" lw 0.2
set border 0
unset tics
set xzeroaxis; set yzeroaxis; set zzeroaxis
set view 60, 30, 1.6, 1.1

vfill sample [t=0:20] '+'  using (cos($1)):(sin($1)):($1):(0.9):(10.0)
splot $helix with isosurface level 1.0 lt 3 notitle

unset vgrid $helix
reset

# Spider plot
$DATA << EOD
	A       B       C       D       E       F
George	15      75      20      43      90      50
Harriet	40	40	40	60	30	50
EOD
set datafile columnheaders

set output out. 'figure_spiderplot' . ext
set spiderplot
set style spiderplot fs transparent solid 0.2 border
set grid spiderplot
set for [i=1:5] paxis i range [0:100]
set             paxis 1 tics font ',9'
set for [i=2:5] paxis i tics format ""
set for [i=1:5] paxis i label sprintf("Score %d",i) offset 1
set key reverse at screen .9, .9
plot for [i=2:6] $DATA using i:key(1)
reset

# Polygons
set output out. 'figure_polygons' . ext
set xyplane at 0
set view equal xyz
unset border
unset tics
unset key
set view 60,33,1.5
set pm3d depth
set pm3d border lc "black" lw 1.5
splot "icosahedron.dat" with polygons fs transparent solid 0.8 fc bgnd
reset

# Along-path cubic spline smoothing
set output out.'figure_smooth_path' . ext
$CURVE << EOD
1   2
1.5 2
2   2.5
3   5
2.5 5
1.5 4
EOD
$LOOP << EOD
1.5 0.5
2.5 0.5
3   1.0
3.5 1.5
4.0 3.5
3.6 4.5
3.0 2.5
4   1.5
5   1.5
EOD

set xrange [0:6]
set yrange [0:7]
unset tics; unset border; set margins 0,0,0,3
set style fill transparent solid 0.2 border lt 8

plot $CURVE smooth path with filledcurves closed title "smooth path with filledcurves closed", \
     $LOOP  smooth path with lines lt 8 title "smooth path with lines", \
     $CURVE with points pt 7 lc "steelblue" title "original points", \
     $LOOP  with points pt 7 lc "steelblue" notitle
reset

# dgrid3d example
#
set output out.'figure_dgrid3d' . ext
$DATA << EOD
0 0 10
0 1 10
0 2 10
1 0 10
1 0.75 5
1 2 10
2 0 10
2 1.25 1
2 2 10
3 0 10
3 0.5 3
3 1 0
3 2 10
EOD

unset key
set dgrid3d 30,30 splines
set view 55, 76, 1.15, 0.9
set xyplane 0

set hidden3d 
set title "Smooth surface fit to scattered points\nset dgrid3d 30,30 splines" 
set title font ",14"
splot $DATA u 1:2:3 w lines, \
      $DATA u 1:2:3 w points pt 7 ps 0.5 lc "black" nogrid nohidden
reset

# Pie chart
#
set output out.'figure_piechart' . ext

unset tics; unset key; unset border
set datafile nocolumnheaders
set xrange [-15:15]
set style fill transparent solid 0.8 border lt -1
plot '-' using 1:2:3:4:5:6 with circles lc variable
0 0 5   0  30 1
0 0 5  30  70 2
0 0 5  70 120 3
0 0 5 120 230 4
0 0 5 230 360 5
e

reset

# Plotting modulus and phase of complex functions
#
set output out.'figure_E0' . ext
unset tics; unset key
set xtics ("0\nReal(z)" 0) left out nomirror scale 1.5 offset 0,-0.3
set ytics ("0\nImag(z)" 0) left out nomirror scale 1.5 offset 0,-0.3
set zrange [0:50]
if (GNUTERM eq "tikz") {
    set ztics (50) format "$E_0(z)$" offset 6,1 scale 0
} else {
    set label "{/:Italic E_0(z)}" at graph 0,0,1.1
}
set view 60,35
set palette model HSV start 0.3 defined (0 0 1 1, 1 1 1 1)
set cbrange [-pi:pi]
set cbtics ("-π" -pi, "π" pi, "phase" 0) scale 0
set pm3d corners2color c1
E0(z) = exp(-z)/z
I = {0,1}
set urange [-1:2];set vrange [-2:2]
set sample 51; set isosample 51
set xyplane 0

splot '++' using 1:2:(abs(E0(x+I*y))):(arg(E0(x+I*y))) with pm3d
reset

# Convex hull used to mask a pm3d surface
# (this comes out unreasonably large as an svg file)
# 
if (GNUTERM eq "svg") {

    # make a blank dummy file instead
    set output out.'figure_mask' . ext
    set term svg font ',2' size 12,12
    clear
    unset output
    set term svg font 'Calisto MT,14' size 600,400

} else {

set output out.'figure_mask' . ext

set view map
set palette rgb 33,13,10
set xrange [-30:25]
set yrange [-30:25]

set dgrid3d 100,100 gauss 5
set pm3d explicit

unset key
unset tics
unset colorbox
unset border

set table $HULL
plot 'mask_pm3d.dat' using 1:2 convexhull with lines title "Convex hull"
unset table

set multiplot layout 1,2 spacing 0.0 margins 0.05,0.95,0.0,0.85
set title "Cluster of points\n defining the mask region"
splot  'mask_pm3d.dat' using 1:2:3 with pm3d, \
       'mask_pm3d.dat' using 1:2:(0) nogrid with points pt 7 ps .5 lc "black"

set pm3d interp 3,3
set title "pm3d surface masked by\nconvex hull of the cluster"
splot  $HULL using 1:2:(0) with mask, \
       'mask_pm3d.dat' using 1:2:3 mask with pm3d

unset multiplot
reset

} # end (GNUTERM ne "svg")

# illustrate "set palette" options
# First set: default white->red cubehelix viridis
#
set output out.'figure_palette1' . ext
set view map
unset colorbox; unset key; unset tics; set margins 0,0,0,0
set pm3d border retrace lw .1
set xrange [0:1]
set samples 129; set isosamples 2
set title offset 0,-0.6

set multiplot layout 4,1 margins 0, 1, .0, .9 spacing 0, .2
set title "set palette rgbformulae 7,5,15  (this is the default)"
set palette
splot x with pm3d
set title 'set palette defined (0 "white", 1 "dark-red")'
set palette defined (0 "white", 1 "dark-red")
splot x with pm3d
set title "set palette cubehelix"
set palette cubehelix
splot x with pm3d
set title "set palette viridis"
set palette viridis
splot x with pm3d
unset multiplot

# Second set: ocean, rainbow, AFM hot, HSV color cycle
set output out.'figure_palette2' . ext

set multiplot layout 4,1 margins 0.2, 1, .0, .9 spacing 0, .2
set title "ocean"
set palette rgbformulae 23,28,3
splot x with pm3d
set title "rainbow"
set palette rgbformulae 33,13,10
splot x with pm3d
set title "AFM hot"
set palette rgbformulae 34,35,36
splot x with pm3d
set title "set palette model HSV rgbformulae 3,2,2"
set palette model HSV start 0.0 rgbformulae 3,2,2
splot x with pm3d
unset multiplot

reset

#
# watchpoint support might not be present
#
set output out.'figure_watchpoints' . ext

if (!strstrt(GPVAL_COMPILE_OPTIONS, "+WATCHPOINTS")) {
    clear
} else {
    set tics nomirror
    set xrange noextend
    unset ytics
    set y2tics 0.25 format "%.2f"
    set link y2
    unset key
    set lmargin 10
    set grid y2

    set title "Find quartile values on a ROC curve"
    set style watchpoint label offset 1,0.3 point pt 6 ps 1 noboxed textcolor "blue"

    plot 'moli3.dat' using 0:(abs($2)*sin($0/6.)**2) smooth cnormal lt -1 lw 2 \
	 watch y=.25 watch y=.50 watch y=.75 \
	 with lines
}
reset

# Polar grid support might not be present
#
set output out.'figure_polar_grid' . ext

if (!strstrt(GPVAL_COMPILE_OPTIONS, "+POLARGRID")) {
    clear
} else {
      set size square
      set angle degrees
      unset border; unset tics; unset key
      unset colorbox
      set rrange [0:200]
      set rtics 50,50,200
      set grid polar front lt -1 lw 0.2 lc "gray50"
      set palette cubehelix negative gamma 0.8
      set polar grid gauss kdensity scale 35
      set polar grid theta [0:190]
      plot 'silver.dat' with surface, '' with points pt 7 lc "black" ps 0.5
}
reset

#
# Extra width figures for some output formats
#
# Color names
if (GNUTERM eq "svg") {
    set term svg font 'Calisto MT,14' size 1200,600
} else {
    set key font ",7" spacing 0.9
}
set output out . 'figure_colornames'. ext
load 'colornames.dem'
reset

# close last file
unset output

reset
