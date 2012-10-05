## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign

ELS = gnuplot-gui.el gnuplot.el
ELCS = gnuplot.elc gnuplot-gui.elc
EXTRA_DIST = dot.el dotemacs gpelcard.dvi gpelcard.pdf gpelcard.ps gpelcard.tex $(ELS)

lispdir = @lispdir@

CLEANFILES = $(ELCS) gpelcard.pdf gpelcard.ps gpelcard.dvi gpelcard.log gpelcard.aux
DISTCLEANFILES = 

BYTEC = $(EMACS) -batch -q -no-site-file -l $(srcdir)/dot.el -f batch-byte-compile

gnuplot.elc: gnuplot.el
	@if [ "$(top_srcdir)" != "$(top_builddir)" ] ; then \
	  cp $(srcdir)/gnuplot.el . ; \
	fi
	$(BYTEC) gnuplot.el

gnuplot-gui.elc: gnuplot-gui.el
	@if [ "$(top_srcdir)" != "$(top_builddir)" ] ; then \
	  cp $(srcdir)/gnuplot-gui.el . ; \
	fi
	$(BYTEC) gnuplot-gui.el

.el.elc:
	$(BYTEC) $<

.dvi.ps:
	$(DVIPS) -o $@ $<

.tex.dvi:
	$(LATEX) $<

.tex.pdf:
	$(PDFLATEX) $<

all: @LISPFILES@

elcs: $(ELCS) $(ELS)

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
