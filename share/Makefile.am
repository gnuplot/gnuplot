## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h
                                                                                
EXTRA_DIST = Gnuplot.app-defaults

SUBDIRS = LaTeX

#
# Someone please rewrite this to work correctly
# DESTDIR is not being recognized; 
# XRESOURCEPATH should come from the syste config
#
XRESOURCEPATH = /usr/lib/X11/app-defaults

install:
	if test -d $(DESTDIR)$(XRESOURCEPATH) && test -w $(DESTDIR)$(XRESOURCEPATH); then \
	  $(INSTALL_DATA) Gnuplot.app-defaults $(DESTDIR)$(XRESOURCEPATH)/Gnuplot \
	; fi

uninstall:
	if test -d $(DESTDIR)$(XRESOURCEPATH) && test -w $(DESTDIR)$(XRESOURCEPATH); then \
	rm -f $(DESTDIR)$(XRESOURCEPATH)/Gnuplot \
	; fi

