## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

##plt-files-begin
PLT_SOURCES = eg1.plt eg2.plt eg3.plt eg4.plt eg5.plt eg6.plt linepoin.plt 
##plt-files-end

EXTRA_DIST = Makefile.am.in $(PLT_SOURCES) \
header.tex makefile.dst tutorial.tex eg3.dat

CLEANFILES = tutorial.aux tutorial.dvi tutorial.log tutorial.ps eg1.tex \
eg2.tex eg3.tex eg4.tex eg5.tex eg6.tex linepoin.tex

DVIPS = dvips
LATEX = latex

SUFFIXES = .dvi .plt .tex .ps

# default target
all: tutorial.dvi

# To touch it up after changes:
remake: tutorial.dvi

tutorial.ps: tutorial.dvi

tutorial.dvi: eg1.tex eg2.tex eg3.tex eg4.tex eg5.tex eg6.tex linepoin.tex \
	tutorial.tex header.tex
	@if [ "$(LATEX)" != no ]; then \
	  echo Building LaTeX tutorial ; \
	  TEXINPUTS=.:$(srcdir): $(LATEX) tutorial ; \
	  TEXINPUTS=.:$(srcdir): $(LATEX) tutorial ; \
	fi

.dvi.ps:
	$(DVIPS) -o $@ $<

.plt.tex:
	@test -f eg3.dat || cp $(srcdir)/*.dat .
	@if test -f $(top_srcdir)/src/gnuplot ; then \
	  $(top_srcdir)/src/gnuplot $< ; \
	else \
	  gnuplot $< ; \
	fi

clean-local:
	@test "$(top_srcdir)" = "$(top_builddir)" || rm -f eg?.dat

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##plt-files-begin/p' $< > $@t
	echo PLT_SOURCES = *.plt | fmt | (tr '\012' @; echo ) \
	  |sed 's/@$$/%/;s/@/ \\@/g;' | tr @% '\012 ' >> $@t
	sed -n '/^##plt-files-end/,$$p' $< >> $@t
	chmod a-w $@t
	mv $@t $@

