all: html

clean:
	rm -f *.png *.gnu *.html

GNUPLOT_LIB = ..

#
# Auto-generation of html demo set.
# If you want to use a stylesheet (gnuplot.css) you must copy it to this directory
# before running webify.pl or make.
#
# Several of the demos need manual intervention:
#	fontfile vector multiplt epslatex
#
html: airfoil.html arrowstyle.html binary.html bivariat.html candlesticks.html contours.html controls.html datastrings.html discrete.html electron.html enhancedtext.html epslatex.html fillbetween.html fillcrvs.html fillstyle.html finance.html fit.html hidden.html histograms.html histograms2.html image.html index.html key.html layout.html mgr.html multimsh.html multiplt.html param.html pm3dcolors.html pm3d.html pm3dgamma.html pointsize.html polar.html poldat.html prob2.html prob.html rainbow.html random.html scatter.html simple.html singulr.html spline.html steps.html surface1.html surface2.html textcolor.html textrotate.html timedat.html using.html vector.html world.html world2.html

index.html: index.save
	cp index.save index.html

image.html: $(GNUPLOT_LIB)/image.dem ./webify.pl
	export DEMOTERM="png truecolor font arial 8 size 420,320"; ./webify.pl image

finance.html: $(GNUPLOT_LIB)/finance.dem ./webify.pl
	export DEMOTERM="png enhanced transparent font arial 8 size 660,320"; ./webify.pl finance

multiplt.html: $(GNUPLOT_LIB)/multiplt.dem ./webify.pl
	export DEMOTERM="png enhanced transparent font arial 8 size 610,480"; ./webify.pl multiplt

%.html: $(GNUPLOT_LIB)/%.dem ./webify.pl
	./webify.pl $*

