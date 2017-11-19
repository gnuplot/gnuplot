#
# Makefile for gnuplot FAQ
#

SHELL = /bin/sh

DVIPS = dvips
L2HT = latex2html
L2HT_OPTS = -split 0 -show_section_numbers -html_version 4.0 -nonavigation
LATEX = latex
LYNX = lynx
LYNX_OPTS = -dump
PS2PDF = ps2pdf

all: dvi html pdf ps txt

html: faq.html
faq.html: faq.tex
	$(L2HT) $(L2HT_OPTS) $<
	-mv faq/$@ .
	-mv faq/faq.css .
	-rm -rf faq

dvi: faq.dvi
faq.dvi: faq.tex
	$(LATEX) $<
	$(LATEX) $<

ps: faq.ps
faq.ps: faq.dvi
	$(DVIPS) -o $@ $<

pdf: faq.pdf
faq.pdf: faq.ps
	$(PS2PDF) $<

txt: faq.txt
faq.txt: faq.html
	$(LYNX) $(LYNX_OPTS) $< >$@

clean:
	-rm -f faq.aux faq.css faq.dvi faq.html faq.log faq.out faq.pdf \
	       faq.ps faq.toc faq.txt

