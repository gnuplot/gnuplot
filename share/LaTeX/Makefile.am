## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

EXTRA_DIST = README gnuplot.cfg

install-data-hook: install-cfg
uninstall-hook: uninstall-cfg

install-cfg:
	@installdir=`kpsexpand '$$TEXMFLOCAL'/tex/latex/gnuplot |sed "s%^.*share/%$(prefix)/share/%"` ; \
	  $(top_srcdir)/mkinstalldirs $(DESTDIR)$$installdir ; \
	  if [ ! -f $(DESTDIR)$$installdir/gnuplot.cfg ]; then \
	    $(INSTALL_DATA) $(srcdir)/gnuplot.cfg $(DESTDIR)$$installdir/gnuplot.cfg && \
	    texhash; \
	  fi

uninstall-cfg:
	@$(NORMAL_UNINSTALL)
	@installdir=`kpsexpand '$$TEXMFLOCAL'/tex/latex/gnuplot |sed "s%^.*share/%$(prefix)/share/%"` ; \
	  rm -f "$(DESTDIR)$$installdir/gnuplot.cfg"
