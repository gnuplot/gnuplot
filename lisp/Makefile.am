## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

ELS = gnuplot-gui.el gnuplot.el info-look.20.2.el info-look.20.3.el
ELCS = gnuplot.elc gnuplot-gui.elc @INFO_LOOK_ELC@
EXTRA_DIST = dot.el dotemacs gnuplot.el.old gnuplot.info gpelcard.dvi gpelcard.pdf gpelcard.ps gpelcard.tex $(ELS)

CLEANFILES = $(ELCS) gnuplot.info gpelcard.pdf gpelcard.ps gpelcard.dvi gpelcard.log gpelcard.aux
DISTCLEANFILES = info-look.el install-sh missing mkinstalldirs

BYTEC = $(EMACS) -batch -q -no-site-file -l $(srcdir)/dot.el -f batch-byte-compile

.el.elc:
	$(BYTEC) $<

.dvi.ps:
	$(DVIPS) -o $@ $<

.tex.dvi:
	$(LATEX) $<

.tex.pdf:
	$(PDFLATEX) $<

all: @LISPFILES@ gnuplot.info

elcs: $(ELCS)

noelcs:

pdf: gpelcard.pdf
ps: gpelcard.ps

gnuplot.info: ../docs/gnuplot.texi
	$(MAKEINFO) -I$(srcdir) ../docs/gnuplot.texi --no-split --output=$@

install: install-els install-elcs

install-els: $(ELS)
	$(mkinstalldirs) $(DESTDIR)$(lispdir)
	@for p in $(ELS) ; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(lispdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(lispdir)/$$p; \
	done

install-elcs: $(ELCS)
	$(mkinstalldirs) $(DESTDIR)$(lispdir)
	@for p in $(ELCS) ; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(lispdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(lispdir)/$$p; \
	done

distclean-local:
	@if test "$(top_srcdir)" != "$(top_builddir)" ; then \
	  rm -f gnuplot.el gnuplot-gui.el gpelcard.tex ; \
	fi

dist-hook:
	@cp install-sh missing mkinstalldirs $(distdir)

SUFFIXES = .el .elc .pdf .ps .tex
