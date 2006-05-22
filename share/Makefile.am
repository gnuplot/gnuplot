## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h
                                                                                
EXTRA_DIST = Gnuplot.app-defaults

SUBDIRS = LaTeX

#
# Someone please generalize this path definition. 
# I have no idea how find it via autoconf!
#
XRESOURCEPATH = /usr/lib/X11/app-defaults

install:
	if test -d $(XRESOURCEPATH); then \
	  $(INSTALL_DATA) Gnuplot.app-defaults $(XRESOURCEPATH)/Gnuplot \
	; fi

uninstall:
	rm -f $(XRESOURCEPATH)/Gnuplot

