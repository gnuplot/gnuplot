## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

CLEANFILES = binary1 binary2 binary3 defaults.ini fit.log soundfit.par \
temp.set fontfile.ps fontfile_latex.ps

BINARY_FILES = binary1 binary2 binary3

DEMO = all.dem

GNUPLOT = gnuplot

all: $(BINARY_FILES)

$(BINARY_FILES): ../src/bf_test
	@echo Creating binary data files
	@../src/bf_test

check-local: check-noninteractive

check-interactive: $(BINARY_FILES)
	@if test -z "$(GNUTERM)" ; then \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUPLOT_DRIVER_DIR=$$bdir/../src \
	    GNUPLOT_LIB=$(srcdir) $(GNUPLOT) $(DEMO) ); \
	else \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUPLOT_DRIVER_DIR=$$bdir/../src \
	    GNUTERM=$(GNUTERM) GNUPLOT_LIB=$(srcdir) $(GNUPLOT) $(DEMO) ); \
	fi

check-noninteractive: $(BINARY_FILES)
	@if test -z "$(GNUTERM)" ; then \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUPLOT_DRIVER_DIR=$$bdir/../src \
	    GNUPLOT_LIB=$(srcdir) $(GNUPLOT) $(DEMO) </dev/null ); \
	else \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUPLOT_DRIVER_DIR=$$bdir/../src \
	    GNUTERM=$(GNUTERM) GNUPLOT_LIB=$(srcdir) $(GNUPLOT) $(DEMO) </dev/null ); \
	fi


##m4-files-begin
EXTRA_DIST = Makefile.am.in reread.bor world.cor 1.dat 2.dat 3.dat \
battery.dat big_peak.dat clip14in.dat glass.dat hemisphr.dat klein.dat \
lcdemo.dat moli3.dat scatter2.dat silver.dat soundvel.dat steps.dat \
timedat.dat triangle.dat using.dat whale.dat world.dat airfoil.dem \
all.dem animate.dem binary.dem bivariat.dem borders.dem contours.dem \
controls.dem discrete.dem electron.dem fillbox.dem fillcrvs.dem \
fit.dem hidden.dem mgr.dem multimsh.dem multiplt.dem param.dem pm3d.dem \
polar.dem poldat.dem prob.dem prob2.dem random.dem scatter.dem simple.dem \
singulr.dem spline.dem steps.dem surface1.dem surface2.dem timedat.dem \
using.dem vector.dem world.dem density.fnc hexa.fnc line.fnc reflect.fnc \
stat.inc sound.par sound2.par start.par gnuplot.rot fontfile.dem \
fontfile_latex.dem
##m4-files-end

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##m4-files-begin/p' Makefile.am.in > $@t
	echo EXTRA_DIST = Makefile.am.in *.bor *.cor *.dat *.dem *.fnc *.inc \
	  sound.par sound2.par start.par *.rot |fmt | (tr '\012' @; echo) \
	  |sed 's/@$$/%/;s/@/ \\@/g' |tr @% '\012 ' \
	  >> $@t
	sed -n '/^##m4-files-end/,$$p' $< >> $@t
	chmod a-w $@t
	mv $@t $@

