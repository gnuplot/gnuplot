## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign

man_MANS = gnuplot.1

EXTRA_DIST = $(man_MANS) lasergnu.1

# Only install the man page if enabled by configure --with-lasergnu
install-data-hook: @LASERGNU@

uninstall-local:
	@$(NORMAL_UNINSTALL)
	rm -f $(DESTDIR)$(man1dir)/lasergnu.1

lasergnu-install:
	$(INSTALL_DATA) $(srcdir)/lasergnu.1 $(DESTDIR)$(man1dir)

lasergnu-noinstall:

