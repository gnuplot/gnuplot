#
# generate animations for inclusion with HTML help documents
#
set term webp animate delay 50 size 300,300
set output './html/figure_spinning_d20.webp'

unset border; unset tics; unset key; set view equal xyz
set margins 0,0,0,0
set pm3d border linecolor "black"
do for [ang=1:360:2] {
    set view 60, ang, 1.7
    splot 'icosahedron.dat' with polygons fc "gold"
}
unset output

# This figure is already in the repository as a static illustration
#
# set term pngcairo size 300,300
# set output './html/figure_static_d20.png'
# replot
# unset output

reset

#
# Convex hull used to mask a pm3d surface
# (webp because the svg version is 10x larger)
#
set term webp font "Calisto MT,12" noanimate size 600,400
set output './html/figure_mask.webp'

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

