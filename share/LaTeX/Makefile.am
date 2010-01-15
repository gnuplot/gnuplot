## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

EXTRA_DIST = README gnuplot.cfg gnuplot-lua-tikz.sty

tex_DATA = gnuplot.cfg gnuplot-lua-tikz.sty
texdir = @TEXDIR@

install-data-hook:
	test -n "$(DESTDIR)" || $(TEXHASH)

uninstall-hook: 
	test -n "$(DESTDIR)" || $(TEXHASH)

