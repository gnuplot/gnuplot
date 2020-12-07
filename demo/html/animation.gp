# Generate webp gif and fallback png files for animation demo

set term webp size 300,300 animate delay 100
set output 'animation.webp'
load '../animation.dem'

set term gif size 300,300 animate delay 100
set output 'animation.gif'
load '../animation.dem'

set term pngcairo size 300,300 font "Times"
set output 'animation_fail.png'
set view 60,50,1.5
set title "Your browser does not support\nthis animation format" offset 0,1
replot


