# Makefile.am for GNUPLOT documentation directory
# based on Makefile.in as of 20-may-2012
#
# Note that the master file for generating documentation is gnuplot.doc.
# See README.
#
# To print manual:
#
#   For LaTeX
#       make gnuplot.dvi
#     or
#       make pdf
#	(requires pdflatex and also graphicx and picins packages) 
#     or
#	make nofigures pdf
#     or
#       make ps
#
#   For groff (GNU troff}
#       make grotxt
#       (print or view gnuplot.txt)
#     or
#       make grodvi
#       (print or view gnuplot.dvi)
#     or
#       make grops
#       (print or view gnuplot.ps)
#
#   For nroff
#     make gnuplot.nroff
#     (print or view gnuplot.nroff)
#
#   For troff
#     make "TROFF=itroff" troff    (use your troff here)
#
#   For GNU info (texinfo)
#     make info
#

# default is what is needed for interactive gnuplot
gih_DATA = gnuplot.gih
gihdir = @GIHDIR@

noinst_PROGRAMS = checkdoc doc2ipf doc2tex doc2gih doc2rnh doc2hlp \
                  doc2rtf doc2ms alldoc2gih 
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/term

T = $(top_srcdir)/term/
CORETERM = $(T)ai.trm $(T)aquaterm.trm $(T)be.trm \
$(T)cairo.trm $(T)canvas.trm $(T)cgm.trm $(T)context.trm \
$(T)corel.trm $(T)debug.trm $(T)djsvga.trm $(T)dumb.trm $(T)dxf.trm \
$(T)dxy.trm $(T)eepic.trm $(T)emf.trm $(T)emxvga.trm $(T)epson.trm \
$(T)estimate.trm $(T)excl.trm $(T)fig.trm $(T)gd.trm $(T)ggi.trm \
$(T)gpic.trm $(T)grass.trm $(T)hp2648.trm $(T)hp26.trm $(T)hp500c.trm \
$(T)hpgl.trm $(T)hpljii.trm $(T)hppj.trm $(T)imagen.trm $(T)kyo.trm \
$(T)latex.trm $(T)linux.trm $(T)lua.trm $(T)metafont.trm \
$(T)metapost.trm $(T)mif.trm $(T)next.trm $(T)openstep.trm $(T)pbm.trm \
$(T)pdf.trm $(T)pm.trm $(T)post.trm $(T)pslatex.trm \
$(T)pstricks.trm $(T)qms.trm $(T)qt.trm $(T)regis.trm $(T)sun.trm \
$(T)svg.trm $(T)t410x.trm $(T)tek.trm $(T)texdraw.trm $(T)tgif.trm \
$(T)tkcanvas.trm $(T)tpic.trm $(T)vgagl.trm \
$(T)vws.trm $(T)win.trm $(T)wxt.trm $(T)x11.trm $(T)xlib.trm 

# DIST_COMMON = README

SOURCES = checkdoc.c doc2gih.c doc2hlp.c doc2ipf.c doc2ms.c doc2rnh.c	\
doc2rtf.c doc2tex.c windows/doc2html.c termdoc.c xref.c

TERM_H = $(top_srcdir)/src/term.h

DOCHEADERS = doc2x.h xref.h

EXTRA_DIST = Makefile.in doc2texi.el figure_missing.pdf gnuplot.doc \
gpcard.tex makefile.dst titlepag.ipf titlepag.ms titlepag.tex	\
toc_entr.sty psdoc faq-ja.tex gnuplot-ja.doc README.ja	\
term-ja.diff plotstyles.gnu gnuplot.pdf windows


ELCS = gnuplot-eldoc.elc

CLEANFILES = allterm.h gnuplot.tex gnuplot.dvi gnuplot.aux gnuplot.log	\
gnuplot.toc gnuplot.ps gpcard.dvi gpcard.log gpcard.ps alldoc2gih	\
doc2gih gnuplot.gih doc2hlp gnuplot.hlp gnuplot.idx gnuplot.ilg		\
gnuplot.ind gnuplot.info* gnuplot.out doc2ipf gnuplot.ipf doc2ms	\
gnuplot.ms doc2rnh gnuplot.rnh doc2rtf gnuplot.rtf gnuplot.nroff	\
checkdoc gnuplot.txt gnuplot.tmp VERSION gnuplot.4tc gnuplot.4ct	\
gnuplot.idv gnuplot.xref gnuplot.lg figures pdffigures.*		\
wxhelp/*.html wxhelp/wgnuplot.htc windows/wgnuplot.htk gnuplot.pdf	\
gnuplot.htb gnuplot.texi gnuplot-eldoc.el $(ELCS)

DISTFILES = $(DIST_COMMON) $(SOURCES) $(DOCHEADERS) $(EXTRA_DIST)

BUILT_SOURCES = allterm.h

allterm.h: $(CORETERM)
	@echo "Building allterm.h"
	@for e in `egrep "^[ 	]*START_HELP" $(CORETERM) |\
	     LC_ALL=C sort -f -t':' -k2` ; do \
	  f=`echo $$e |cut -d\: -f1` ; s=`echo $$e | cut -d\: -f2` ;\
	  sed -n "/^[ 	]*$$s/,/^[ 	]*END_HELP/p" $$f ; \
	done >$@

# for debugging
alldoc: check dvi gih hlp html info ipf ms nroff pdf ps rnh rtf htb

### [tn]roff documentation
troff: gnuplot.ms titlepag.ms
	@if [ "$(srcdir)" = "$(top_builddir)" ] ; then \
	    tbl gnuplot.ms | eqn | $(TROFF) -ms ; \
	else \
	    < gnuplot.ms sed "s,titlepag\.ms,$(srcdir)/titlepag\.ms," |\
	      tbl | eqn | $(TROFF) -ms ; \
	fi

nroff: gnuplot.nroff

gnuplot.nroff: gnuplot.ms titlepag.ms
	@if [ "$(srcdir)" = "$(top_builddir)" ] ; then \
	    tbl gnuplot.ms | neqn | nroff -ms | col > gnuplot.nroff ; \
	else \
	    < gnuplot.ms sed "s,titlepag\.ms,$(srcdir)/titlepag\.ms," |\
	      tbl | neqn | nroff -ms | col > gnuplot.nroff ; \
	fi

### groff documentation

groff: grotxt grodvi grops

grotxt: gnuplot.ms titlepag.ms
	@if [ "$(srcdir)" = "$(top_builddir)" ] ; then \
	    groff -s -p -t -e -ms -Tlatin1 gnuplot.ms > gnuplot.txt ; \
	else \
	    sed "s,titlepag\.ms,$(srcdir)/titlepag\.ms," gnuplot.ms |\
	      groff -s -p -t -e -ms -Tlatin1 gnuplot.ms > gnuplot.txt ; \
	fi

grodvi: gnuplot.ms titlepag.ms
	@if [ "$(srcdir)" = "$(top_builddir)" ] ; then \
	    groff -s -p -t -e -ms -Tdvi gnuplot.ms > gnuplot.dvi ; \
	else \
	    sed "s,titlepag\.ms,$(srcdir)/titlepag\.ms," gnuplot.ms |\
	      groff -s -p -t -e -ms -Tdvi gnuplot.ms > gnuplot.dvi ; \
	fi

grops: gnuplot.ms titlepag.ms
	@if [ "$(srcdir)" = "$(top_builddir)" ] ; then \
	    groff -s -p -t -e -ms -Tps gnuplot.ms > gnuplot.ps ; \
	else \
	    sed "s,titlepag\.ms,$(srcdir)/titlepag\.ms," gnuplot.ms |\
	      groff -s -p -t -e -ms -Tps gnuplot.ms > gnuplot.ps ; \
	fi

### doc2xxx dependencies
ms: gnuplot.ms

gnuplot.ms: doc2ms$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2ms$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.ms

doc2ms_SOURCES = doc2ms.c termdoc.c
doc2ms_CPPFLAGS = -DALL_TERM_DOC $(AM_CPPFLAGS)
	$(COMPILE) -DALL_TERM_DOC -c $(srcdir)/doc2ms.c

### html
html: htmldocs/gnuplot.html

htmldocs/gnuplot.html: gnuplot.tex
	mkdir -p htmldocs
	latex2html -dir htmldocs -local_icons gnuplot

### PDF documentation
pdffigures: pdffigures.tex $(top_srcdir)/src/gnuplot $(top_srcdir)/docs/plotstyles.gnu
	rm -f figures

pdffigures.tex:	figures
	echo "\usepackage{graphicx}" > pdffigures.tex ; \
	echo "\usepackage{picins}" >> pdffigures.tex

figures: allterm.h doc2tex$(EXEEXT) $(srcdir)/gnuplot.doc
	@if [ -a $(top_srcdir)/src/gnuplot ] ; then \
	  GNUPLOT_LIB=$(top_srcdir)/demo ../src/gnuplot $(srcdir)/plotstyles.gnu ; \
	  ./doc2tex$(EXEEXT) -figures $(srcdir)/gnuplot.doc gnuplot.tex ; \
	else \
	  echo gnuplot executable not found - cannot regenerate manual figures ; \
	fi

nofigures:
	echo "" > pdffigures.tex
	touch figures
	@if [ -a $(top_srcdir)/src/gnuplot ] ; then \
	  GNUPLOT_LIB=$(top_srcdir)/demo ../src/gnuplot $(srcdir)/plotstyles.gnu ; \
	  ./doc2tex$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.tex ; \
	else \
	  echo gnuplot executable not found - cannot regenerate manual figures ; \
	fi

pdf: pdffigures.tex gnuplot.pdf

gnuplot.pdf: gnuplot.tex
	@if test $(PDFLATEX) != no ; then \
	  rm -f gnuplot.idx ; \
	  touch gnuplot.idx ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) gnuplot ; \
	  makeindex gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) gnuplot ; \
	else \
	  echo pdflatex not found - cannot build pdf file ; \
	fi

### LaTeX documentation
tex: gnuplot.tex

gnuplot.tex: allterm.h doc2tex$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2tex$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.tex

doc2tex_SOURCES = doc2tex.c termdoc.c
doc2tex_CPPFLAGS = -DALL_TERM_DOC $(AM_CPPFLAGS)

# this is how to make DVI files
dvi: gnuplot.dvi gpcard.dvi

gnuplot.dvi: gnuplot.tex titlepag.tex toc_entr.sty
	@if test $(LATEX) != no ; then \
	  rm -f gnuplot.idx ; \
	  touch gnuplot.idx ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) gnuplot ; \
	  makeindex gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) gnuplot ; \
	else \
	  echo "latex not found - cannot build dvi file" ; \
	fi

gpcard.dvi: gpcard.tex
	@if test $(PLAINTEX) != no ; then \
              TEXINPUTS=.:$(srcdir):$(top_srcdir): tex gpcard ; \
	else \
	  echo "plaintex not found - cannot build dvi file" ; \
	fi

### Postscript format
# if pslatex has been installed, add "times" to titlepage.tex
ps: gnuplot.ps gpcard.ps

gnuplot.ps: gnuplot.dvi
	@if test $(DVIPS) != no ; then \
	   $(DVIPS) gnuplot.dvi -o gnuplot.ps ; \
	else \
	   echo "dvips not found - cannot build PS file" ; \
	fi

gpcard.ps: gpcard.dvi
	@if test $(DVIPS) != no ; then \
            $(DVIPS) gpcard.dvi -o gpcard.ps ; \
	else \
	   echo "dvips not found - cannot build PS file" ; \
	fi


# this is how to make gnuplot.hlp
hlp: gnuplot.hlp

gnuplot.hlp: doc2hlp$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2hlp$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.hlp

doc2hlp_SOURCES = doc2hlp.c termdoc.c

### gnuplot interactive help format
gih: gnuplot.gih

gnuplot.gih: doc2gih$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2gih$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.gih

doc2gih_SOURCES = doc2gih.c termdoc.c

# To include all terminals in the .gih file
allgih: alldoc2gih$(EXEEXT) $(srcdir)/gnuplot.doc
	./alldoc2gih$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.gih

alldoc2gih_SOURCES = doc2gih.c termdoc.c
alldoc2gih_CPPFLAGS = -DALL_TERM_DOC $(AM_CPPFLAGS)

### wxhelp format
htb: gnuplot.htb

gnuplot.htb: wxhelp/wgnuplot.html
	GNUPLOT_LIB=$(srcdir)/../demo ../src/gnuplot -e "winhelp=1;" $(srcdir)/plotstyles.gnu
	zip -j9 gnuplot.htb windows/wgnuplot.hhp wxhelp/wgnuplot.hhc wxhelp/wgnuplot.hhk wxhelp/*.html windows/*.png

wxhelp/wgnuplot.html: wxhelp/doc2html$(EXEEXT) $(srcdir)/gnuplot.doc 
	./wxhelp/doc2html$(EXEEXT) $(srcdir)/gnuplot.doc wxhelp/

wxhelp/doc2html.o: $(srcdir)/windows/doc2html.c $(BUILT_SOURCES)
	[ -d wxhelp ] || mkdir wxhelp
	$(COMPILE) -c -o $@ -I.. -DALL_TERM_DOC -DWXHELP $<

wxhelp/doc2html: wxhelp/doc2html.o termdoc.o xref.o ../src/version.o
	$(LINK) $^

### GNU info format
info: gnuplot.info

# Thanks to Bruce Ravel for doc2texi.el!
gnuplot.texi gnuplot-eldoc.el: $(srcdir)/doc2texi.el $(srcdir)/gnuplot.doc
	@echo "Creating texinfo and eldoc strings file"
	@if test "$(EMACS)" != no; then \
	   test "$(top_srcdir)" = "$(top_builddir)" || cp $(srcdir)/gnuplot.doc . ; \
	   $(EMACS) -batch -q --no-site-file -l $(srcdir)/doc2texi.el -f d2t-doc-to-texi ; \
	   echo "Compiling gnuplot-eldoc.el" ; \
	   $(EMACS) -batch -q --no-site-file -f batch-byte-compile gnuplot-eldoc.el ; \
	   test "$(top_srcdir)" = "$(top_builddir)" || rm -f ./gnuplot.doc ; \
	else \
	   echo "No emacs found - cannot create texinfo file" ; \
	fi

${ELCS}: gnuplot-eldoc.el
	@if test "$(EMACS)" != no; then \
	   echo "Compiling gnuplot-eldoc.el" ; \
	   $(EMACS) -batch -q --no-site-file -f batch-byte-compile gnuplot-eldoc.el ; \
	else \
	   echo "No emacs found - cannot create gnuplot-eldoc.elc file" ; \
	fi

gnuplot.info: gnuplot.texi
	$(MAKEINFO) -I$(srcdir) $(srcdir)/gnuplot.texi --no-split --output=$@

install-info: gnuplot.info
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(infodir)
	$(INSTALL_DATA) gnuplot.info $(DESTDIR)$(infodir)/gnuplot.info
	@if (install-info --version && \
	     install-info --version | fgrep -i -v debian) >/dev/null 2>&1; then \
	  echo "install-info --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/gnuplot.info";\
	  install-info --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/gnuplot.info || :;\
	else : ; fi

# this is how to make OS/2 ipfc documentation
ipf: gnuplot.ipf

gnuplot.ipf: doc2ipf$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2ipf$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.ipf

doc2ipf_SOURCES = doc2ipf.c termdoc.c xref.c

### Rich Text Format
rtf: gnuplot.rtf

gnuplot.rtf: doc2rtf$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2rtf$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.rtf

doc2rtf_SOURCES = doc2rtf.c termdoc.c xref.c

# this is how to make rnh documentation (VMS help format)
rnh: gnuplot.rnh

gnuplot.rnh: doc2rnh$(EXEEXT) $(srcdir)/gnuplot.doc
	./doc2rnh$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.rnh

doc2rnh_SOURCES = doc2rnh.c termdoc.c

# this is how to check the gnuplot.doc file
check-local: checkdoc$(EXEEXT)
	@./checkdoc$(EXEEXT) < $(srcdir)/gnuplot.doc; \
	if test $$? -eq 0; then \
	  echo "PASS: gnuplot.doc"; \
	else \
	  :; \
	fi

clean-local:
	@for f in figure*.pdf ; do \
	   if [ "$$f" != "figure_missing.pdf" ]; then \
	     rm -f $$f ; \
	   else \
	     :; \
	   fi \
	 done

checkdoc_SOURCES = checkdoc.c termdoc.c gnuplot.doc

# for VMS only
install-vms: gnuplot.hlp
	$(INSTALL_DATA) gnuplot.hlp $(datadir)/gnuplot.hlp

#the end
