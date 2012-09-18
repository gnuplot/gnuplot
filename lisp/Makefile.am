## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign

ELS = gnuplot-gui.el gnuplot.el
ELCS = gnuplot.elc gnuplot-gui.elc
EXTRA_DIST = README.1st dot.el dotemacs gnuplot.el.old gpelcard.dvi gpelcard.pdf gpelcard.ps gpelcard.tex $(ELS)

CLEANFILES = $(ELCS) gpelcard.pdf gpelcard.ps gpelcard.dvi gpelcard.log gpelcard.aux
DISTCLEANFILES = 

BYTEC = $(EMACS) -batch -q -no-site-file -l $(srcdir)/dot.el -f batch-byte-compile

.el.elc:
	$(BYTEC) $<

.dvi.ps:
	$(DVIPS) -o $@ $<

.tex.dvi:
	$(LATEX) $<

.tex.pdf:
	$(PDFLATEX) $<

all: @LISPFILES@

elcs: $(ELCS)

noelcs:

pdf: gpelcard.pdf
ps: gpelcard.ps

install-data-hook: @INSTALL_LISP@

install-lisp: install-els install-elcs
install-nolisp:

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

uninstall-local:
	@$(NORMAL_UNINSTALL)
	@for p in $(ELCS) $(ELS) ; do \
	  rm -f $(DESTDIR)$(lispdir)/$$p; \
	done

distclean-local:
	@if test "$(top_srcdir)" != "$(top_builddir)" ; then \
	  for p in $(ELS) gpelcard.tex ; do \
	    rm -f $$p ; \
	  done ; \
	fi

SUFFIXES = .el .elc .pdf .ps .tex
