set terminal latex
set output "eg6.tex"
set size 3.5/5, 3/3.
set format y "$%g$"
set format x "$%5.1f\mu$"
set title "This is a title"
set xlabel "This is the $x$ axis"
set ylabel "This is\\a longer\\version\\ of\\the $y$\\ axis"
set label "Data" at -5,-5 right
set arrow from -5,-5 to -3.3,-6.7
set key -4,8
set xtic -10,5,10
plot [-10:10] [-10:10] "eg3.dat" title "Data File"  with linespoints 1 7,\
   3*exp(-x*x)+1  title "$3e^{-x^{2}}+1$" with lines 4
