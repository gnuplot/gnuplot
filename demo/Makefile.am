## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

CLEANFILES = binary1 binary2 binary3 defaults.ini fit.log soundfit.par temp.set

BINARY_FILES = binary1 binary2 binary3

all: $(BINARY_FILES)

$(BINARY_FILES): ../src/bf_test
	@echo Creating binary data files
	@../src/bf_test

check-local: check-noninteractive

check-interactive: $(BINARY_FILES)
	@if test -z "$(GNUTERM)" ; then \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUPLOT_LIB=$(srcdir) gnuplot all.dem ); \
	else \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUTERM=$(GNUTERM) GNUPLOT_LIB=$(srcdir) gnuplot all.dem ); \
	fi

check-noninteractive: $(BINARY_FILES)
	@if test -z "$(GNUTERM)" ; then \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUPLOT_LIB=$(srcdir) gnuplot all.dem </dev/null ); \
	else \
	  ( bdir=`pwd` ; PATH=$$bdir/../src GNUTERM=$(GNUTERM) GNUPLOT_LIB=$(srcdir) gnuplot all.dem </dev/null ); \
	fi


##m4-files-begin
EXTRA_DIST = Makefile.am.in reread.bor world.cor 1.dat 2.dat 3.dat \
battery.dat big_peak.dat glass.dat hemisphr.dat klein.dat lcdemo.dat \
moli3.dat scatter2.dat silver.dat soundvel.dat steps.dat timedat.dat \
using.dat whale.dat world.dat airfoil.dem all.dem animate.dem binary.dem \
bivariat.dem borders.dem contours.dem controls.dem discrete.dem \
electron.dem fit.dem hidden.dem mgr.dem multimsh.dem multiplt.dem \
param.dem polar.dem poldat.dem prob.dem prob2.dem random.dem scatter.dem \
simple.dem singulr.dem spline.dem steps.dem surface1.dem surface2.dem \
timedat.dem using.dem vector.dem world.dem density.fnc hexa.fnc line.fnc \
reflect.fnc stat.inc sound.par sound2.par start.par gnuplot.rot 
##m4-files-end

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##m4-files-begin/p' $< > $@t
	echo EXTRA_DIST = Makefile.am.in *.bor *.cor *.dat *.dem *.fnc *.inc \
	  sound.par sound2.par start.par *.rot |fmt | (tr '\012' @; echo) \
	  |sed 's/@$$/%/;s/@/ \\@/g' |tr @% '\012 ' \
	  >> $@t
	sed -n '/^##m4-files-end/,$$p' $< >> $@t
	chmod a-w $@t
	mv $@t $@

