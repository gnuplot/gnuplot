set terminal latex
set output "eg2.tex"
set size 5/5., 4/3.
set format xy "$%g$"
set title 'This is a plot of $y=\sin(x)$'
set xlabel 'This is the $x$ axis'
set ylabel 'This is\\the\\$y$ axis'
plot [0:6.28] [0:1] sin(x)
