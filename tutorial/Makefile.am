# $Header$

AUTOMAKE_OPTIONS = foreign 1.2h

PLT_files = eg1.plt eg2.plt eg3.plt eg4.plt eg5.plt eg6.plt linepoin.plt 

EXTRA_DIST =  $(PLT_files) \
              header.tex tutorial.tex eg3.dat

CLEANFILES = tutorial.aux tutorial.dvi tutorial.log tutorial.pdf tutorial.ps \
             eg1.tex eg2.tex eg3.tex eg4.tex eg5.tex eg6.tex linepoin.tex

SUFFIXES = .dvi .pdf .plt .ps .tex

# default target
all: @TUTORIAL@

tutorial: tutorial.dvi

notutorial:

# To touch it up after changes:
remake: tutorial.dvi

tutorial.ps: tutorial.dvi

tutorial.dvi: eg1.tex eg2.tex eg3.tex eg4.tex eg5.tex eg6.tex linepoin.tex \
	tutorial.tex header.tex
	@echo Building LaTeX tutorial
	TEXINPUTS=.:$(srcdir):${TEXINPUTS}: $(LATEX) tutorial
	TEXINPUTS=.:$(srcdir):${TEXINPUTS}: $(LATEX) tutorial

tutorial.pdf: eg1.tex eg2.tex eg3.tex eg4.tex eg5.tex eg6.tex linepoin.tex \
	tutorial.tex header.tex
	@echo Building LaTeX tutorial
	TEXINPUTS=.:$(srcdir):${TEXINPUTS}: $(PDFLATEX) tutorial
	TEXINPUTS=.:$(srcdir):${TEXINPUTS}: $(PDFLATEX) tutorial

.dvi.ps:
	$(DVIPS) -o $@ $<

.plt.tex:
	@test -f eg3.dat || cp $(srcdir)/*.dat .
	@if test -x ../src/gnuplot ; then \
	  ../src/gnuplot $< ; \
	else \
	  gnuplot $< ; \
	fi
