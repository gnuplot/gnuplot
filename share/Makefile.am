## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h
                                                                                
EXTRA_DIST = Gnuplot.app-defaults

if HAVE_LATEX
SUBDIRS = LaTeX
endif

appdefaultdir = $(pkgdatadir)/$(VERSION_MAJOR)/app-defaults

APPDEFAULTFILES = Gnuplot

if BUILD_GNUPLOT_X11
appdefault_DATA = $(APPDEFAULTFILES)
else
appdefault_DATA =
endif

Gnuplot: $(srcdir)/Gnuplot.app-defaults
	cp -p $(srcdir)/Gnuplot.app-defaults $@

CLEANFILES = $(APPDEFAULTFILES)
