all: html

distclean: clean
clean:
	rm -f *.png *.gnu *.html *.dat epslatex* fit.log soundfit.par

GNUPLOT_LIB = ..

#
# Auto-generation of html demo set.
#
# Several of the demos need manual intervention:
#	fontfile vector epslatex
#
html: airfoil.html arrowstyle.html binary.html bivariat.html candlesticks.html contours.html controls.html datastrings.html discrete.html electron.html enhancedtext.html epslatex.html fillbetween.html fillcrvs.html fillstyle.html finance.html fit.html hidden.html hidden2.html histograms.html histograms2.html image.html index.html key.html layout.html mgr.html multimsh.html multiplt.html margins.html param.html pm3dcolors.html pm3d.html pm3dgamma.html pointsize.html polar.html poldat.html prob2.html prob.html rainbow.html random.html rectangle.html rgb_variable.html scatter.html simple.html singulr.html spline.html steps.html stringvar.html surface1.html surface2.html textcolor.html textrotate.html tics.html timedat.html using.html vector.html world.html world2.html dashcolor.html animate2.html

index.html: index.save
	cp index.save index.html

image.html: $(GNUPLOT_LIB)/image.dem ./webify.pl
	export DEMOTERM="png truecolor font arial 8 size 420,320"; ./webify.pl image

finance.html: $(GNUPLOT_LIB)/finance.dem ./webify.pl
	export DEMOTERM="png enhanced transparent font arial 8 size 660,320"; ./webify.pl finance

multiplt.html: $(GNUPLOT_LIB)/multiplt.dem ./webify.pl
	export DEMOTERM="png enhanced transparent font arial 8 size 610,480"; ./webify.pl multiplt

# The png terminal does not support dashed lines, so we create an eps file and convert it.
# Do this one last, so that if conversion fails everything else has already been done.
dashcolor.html: $(GNUPLOT_LIB)/dashcolor.dem ./webify.pl
	export DEMOTERM="post eps color dash dashlength 3 font 'Helvetica' 16"; ./webify.pl dashcolor
	mv dashcolor.1.png dashcolor.1.eps
	convert -density 100 dashcolor.1.eps dashcolor.1.png

animate2.html: $(GNUPLOT_LIB)/animate2.dem ./webify.pl
	export DEMOTERM="gif animate transparent opt delay 10 size 200,200 x000000"; ./webify.pl animate2
	mv animate2.html temp.html
	cat temp.html | sed -e 's/1.png/1.gif/g' > animate2.html
	mv animate2.1.png animate2.1.gif
	rm temp.html


%.html: $(GNUPLOT_LIB)/%.dem ./webify.pl
	./webify.pl $*

