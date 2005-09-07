## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

EXTRA_DIST = README gnuplot.cfg
INSTALLDIR = `kpsexpand '$$TEXMFLOCAL'`/tex/latex/gnuplot

install-data-hook: install-cfg

install-cfg:
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(INSTALLDIR)
	@if [ ! -f $(DESTDIR)$(INSTALLDIR)/gnuplot.cfg ]; then \
	  $(INSTALL_DATA) $(srcdir)/gnuplot.cfg $(DESTDIR)$(INSTALLDIR)/gnuplot.cfg && \
	  texhash; \
	fi
