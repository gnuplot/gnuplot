#
# Makefile for gnuplot FAQ
#

SHELL = /bin/sh

L2HT = htlatex
L2HT_OPTS = "htlatex.cfg"
LATEX = latex
LYNX = lynx
LYNX_OPTS = -dump
PDFLATEX = pdflatex

all: html pdf

html: faq.html
faq.html: faq.tex
	$(L2HT) $< $(L2HT_OPTS)

pdf: faq.pdf
faq.pdf: faq.tex
	$(PDFLATEX) $<
	$(PDFLATEX) $<

txt: faq.txt
faq.txt: faq.html
	$(LYNX) $(LYNX_OPTS) $< >$@

clean:
	-rm -f faq.aux faq.css faq.dvi faq.html faq.log faq.out faq.pdf \
	       faq.tmp faq.ps faq.toc faq.txt faq.4tc faq.4ct faq.lg faq.idv faq.xref \
	       faq*.png *.log *.aux

