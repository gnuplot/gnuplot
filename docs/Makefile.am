# Makefile.am for GNUPLOT documentation directory
#
# Note that the master file for generating documentation is gnuplot.doc.
# See README.
#
# To print manual:
#
#   For LaTeX
#       make dvi
#     or
#       make pdf
#	(requires pdflatex and also graphicx and picins packages)
#     or
#       make tikz 
#	(requires pdflatex and also lua/tikz and wrapfig package)
#     or
#       make pdf_nofig
#       (PDF without figures)
#     or
#       make ps
#
#   For HTML
#	make wxhelp/wgnuplot.html (windows / wxt help system)
#     or
#	make html (HTML for web site)
#
#   For GNU info (texinfo)
#	make info
#

# default is what is needed for interactive gnuplot
gih_DATA = gnuplot.gih
gihdir = @GIHDIR@

noinst_PROGRAMS = checkdoc doc2ipf doc2tex doc2gih doc2hlp \
                  doc2rtf alldoc2gih doc2wxhtml doc2web

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/term \
              -I./ja/term

# Documentation will be prepared for all terminals in CORETERM
# even if the terminal itself is not included in the build.

T = $(top_srcdir)/term/
CORETERM = $(T)ai.trm $(T)aquaterm.trm $(T)be.trm $(T)block.trm $(T)caca.trm \
$(T)cairo.trm $(T)canvas.trm $(T)cgm.trm $(T)context.trm \
$(T)debug.trm $(T)djsvga.trm $(T)dumb.trm $(T)dxf.trm \
$(T)emf.trm $(T)epson.trm \
$(T)estimate.trm $(T)fig.trm $(T)gd.trm \
$(T)gpic.trm $(T)grass.trm $(T)hpgeneric.h $(T)hpgl.trm $(T)imagen.trm \
$(T)linux-vgagl.trm $(T)latex_old.h \
$(T)lua.trm $(T)pbm.trm $(T)pict2e.trm \
$(T)pm.trm $(T)post.trm $(T)pslatex.trm \
$(T)pstricks.trm $(T)qt.trm $(T)regis.trm \
$(T)svg.trm $(T)t410x.trm $(T)tek.trm $(T)texdraw.trm $(T)tgif.trm \
$(T)tkcanvas.trm $(T)webp.trm $(T)win.trm $(T)wxt.trm $(T)x11.trm $(T)xlib.trm 

LEGACYTERM = $(T)latex.trm $(T)emtex.trm $(T)eepic.trm $(T)tpic.trm \
$(T)corel.trm $(T)dxy.trm $(T)excl.trm $(T)kyo.trm $(T)mif.trm \
$(T)metafont.trm $(T)metapost.trm $(T)ggi.trm\
$(T)hp2648.trm $(T)hp26.trm $(T)hp500c.trm $(T)hpljii.trm $(T)hppj.trm \
$(T)pc.trm $(T)pdf.trm $(T)qms.trm $(T)vws.trm

# Same thing for the subset of terminals for which we have
# Japanese documentation
TJ = ja/term/
JATERM = $(TJ)ai.trm $(TJ)aquaterm.trm $(TJ)be.trm $(TJ)block.trm $(TJ)caca.trm \
$(TJ)cairo.trm $(TJ)canvas.trm $(TJ)cgm.trm $(TJ)context.trm \
$(TJ)debug.trm $(TJ)djsvga.trm $(TJ)dumb.trm $(TJ)dxf.trm \
$(TJ)emf.trm $(TJ)epson.trm \
$(TJ)estimate.trm $(TJ)fig.trm $(TJ)gd.trm \
$(TJ)gpic.trm $(TJ)grass.trm $(TJ)hpgeneric.h $(TJ)hpgl.trm $(TJ)imagen.trm \
$(TJ)linux-vgagl.trm $(TJ)latex_old.h \
$(TJ)lua.trm $(TJ)pbm.trm $(TJ)pict2e.trm \
$(TJ)pm.trm $(TJ)post.trm $(TJ)pslatex.trm \
$(TJ)pstricks.trm $(TJ)qt.trm $(TJ)regis.trm \
$(TJ)svg.trm $(TJ)t410x.trm $(TJ)tek.trm $(TJ)texdraw.trm $(TJ)tgif.trm \
$(TJ)tkcanvas.trm $(TJ)webp.trm $(TJ)win.trm $(TJ)wxt.trm $(TJ)x11.trm $(TJ)xlib.trm


# DIST_COMMON = README

SOURCES = checkdoc.c doc2gih.c doc2hlp.c doc2ipf.c \
doc2rtf.c doc2tex.c windows/doc2html.c doc2web.c termdoc.c xref.c

TERM_H = $(top_srcdir)/src/term.h

DOCHEADERS = doc2x.h xref.h

EXTRA_DIST = Makefile.in doc2texi.el gnuplot.doc \
titlepag.ipf titlepag.tex title-ja.tex \
toc_entr.sty psdoc gnuplot-ja.doc README.ja \
term-ja.diff plotstyles.gnu gnuplot.pdf \
windows/wgnuplot.hhk windows/wgnuplot.hhp windows/wgnuplot.stp windows/doc2html.c \
html/gnuplot_docs.css

ELCS = gnuplot-eldoc.elc

MOSTLYCLEANFILES = gnuplot.aux gnuplot.log gnuplot.out gnuplot.toc	\
gnuplot.ind gpinsetfigure.aux gpinsetfigure.tex figurestyle \
no_figures tikz_figures pdf_figures \
wxhelp/doc2html.o

CLEANFILES = gnuplot.dvi gnuplot.tex gnuplot.idx gnuplot.ilg		\
gnuplot.ps gnuplot.gih gnuplot.hlp gnuplot.info* gnuplot.ipf 		\
gnuplot.rtf gnuplot.tmp VERSION gnuplot.4tc gnuplot.4ct			\
gnuplot.idv gnuplot.xref gnuplot.lg gnuplot.pdf				\
ja.doc gnuplot-ja.ind gnuplot-ja.ilg gnuplot-ja.aux gnuplot-ja.toc	\
gnuplot-ja.log gnuplot-ja.idx gnuplot-ja.out gnuplot-ja.tex		\
wxhelp_figures wxhelp/*.html wxhelp/wgnuplot.hhc wxhelp/wgnuplot.hhk	\
wxhelp/wgnuplot.htc wxhelp/doc2html$(EXEEXT) windows/wgnuplot.htk	\
svg_figures html/*.html html/*.svg html/*.hhc html/*.hhk                \
webp_figures html/*.webp                                                \
windows/*.png gnuplot.htb gnuplot.texi gnuplot-eldoc.el $(ELCS)

DISTFILES = $(DIST_COMMON) $(SOURCES) $(DOCHEADERS) $(EXTRA_DIST)

BUILT_SOURCES = allterm.h allterm-ja.h

GNUPLOT_EXE = ../src/gnuplot$(EXEEXT)

$(GNUPLOT_EXE):
	( cd ../src ; $(MAKE) gnuplot$(EXEEXT) )

if BUILD_LUA
LUA_HELP = gnuplot-tikz.help
$(LUA_HELP): $(top_srcdir)/term/lua/gnuplot-tikz.lua
	$(AM_V_GEN) $(LUA) $< termhelp > $@
else
LUA_HELP = 
endif

allterm.h: $(CORETERM) $(LUA_HELP)
	$(AM_V_GEN) for e in `grep -E "^[ 	]*START_HELP" $(CORETERM) |\
	     LC_ALL=C sort -f -t':' -k2` ; do \
	  f=`echo $$e |cut -d\: -f1` ; s=`echo $$e | cut -d\: -f2` ;\
	  sed -n "/^[ 	]*$$s/,/^[ 	]*END_HELP/p" $$f ; \
	done >$@


### PDF documentation can inset figures using either tikz+wrapfig or pdf+picins
pdf_figures: $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	$(AM_V_at)GNUPLOT_LIB=$(top_srcdir)/demo $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	rm -f gpinsetfigure.tex
	printf '%s\n' '\usepackage{graphicx}' \
		'\usepackage{picins}' \
		'\newcommand{\gpinsetfigure}[1]{' \
		'  \parpic[r][rt]{\includegraphics[width=3in,keepaspectratio]{#1}}' \
		'}' > gpinsetfigure.tex
	$(AM_V_GEN)touch $@
	$(AM_V_at)touch figurestyle

tikz_figures: $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	$(AM_V_at)GNUPLOT_LIB=$(top_srcdir)/demo GNUTERM=tikz $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	rm -f gpinsetfigure.tex
	printf '%s\n' '\usepackage{gnuplot-lua-tikz}' \
		'\usepackage{wrapfig}' \
		'\newcommand{\gpinsetfigure}[1]{' \
		'    \begin{wrapfigure}[10]{r}{3.0in}' \
		'    \vspace{-20pt} \input{#1} \vspace{-20pt}' \
		'    \end{wrapfigure}' \
		'}' > gpinsetfigure.tex
	$(AM_V_GEN)touch $@
	$(AM_V_at)touch figurestyle

no_figures:
	rm -f gpinsetfigure.tex
	printf '%s\n' '\newcommand{\gpinsetfigure}[1]{}' > gpinsetfigure.tex
	$(AM_V_GEN)touch $@
	$(AM_V_at)touch figurestyle

gpinsetfigure.tex: no_figures
	$(AM_V_GEN) touch $@

gnuplot.tex: allterm.h doc2tex$(EXEEXT) $(srcdir)/gnuplot.doc
	$(AM_V_GEN)./doc2tex$(EXEEXT) -figures $(srcdir)/gnuplot.doc $@

pdf: pdf_figures
	$(MAKE) gnuplot.pdf
	rm -f tikz_figures no_figures
tikz: tikz_figures gnuplot.pdf figurestyle
	rm -f pdf_figures no_figures
pdf_nofig: no_figures gnuplot.pdf figurestyle
	rm -f tikz_figures pdf_figures

gnuplot.pdf: gnuplot.tex $(srcdir)/titlepag.tex $(srcdir)/toc_entr.sty pdf_figures
	$(AM_V_GEN)if test $(PDFLATEX) != no ; then \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) $< ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) $< ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) $< ; \
	  makeindex gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(PDFLATEX) $< ; \
	  rm -f gnuplot.idx gnuplot.ilg gnuplot.ind ; \
	else \
	  echo pdflatex not found - cannot build pdf file ; \
	fi

### LaTeX documentation
tex: gnuplot.tex

doc2tex_SOURCES = doc2tex.c termdoc.c
doc2tex_CPPFLAGS = -DALL_TERM_DOC $(AM_CPPFLAGS)

dvi: no_figures gnuplot.dvi
gnuplot.dvi: gnuplot.tex $(srcdir)/titlepag.tex $(srcdir)/toc_entr.sty
	$(AM_V_GEN)if test $(LATEX) != no ; then \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) $< ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) $< ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) $< ; \
	  makeindex gnuplot ; \
	  TEXINPUTS=.:$(srcdir):$(top_srcdir):${TEXINPUTS}: $(LATEX) $< ; \
	else \
	  echo "latex not found - cannot build dvi file" ; \
	fi

### Postscript format
# if pslatex has been installed, add "times" to titlepage.tex
ps: gnuplot.ps

gnuplot.ps: gnuplot.dvi
	$(AM_V_GEN)if test $(DVIPS) != no ; then \
	   $(DVIPS) gnuplot.dvi -o gnuplot.ps ; \
	else \
	   echo "dvips not found - cannot build PS file" ; \
	fi

# Prepare PDF copy of User Manual in Japanese
#
# This procedure assumes use of luatex with package luatexja
# Tested with TeX Live 2020
# Known limitations:
# - The title page is not completely translated.
# - Only the original English keywords appear in the index and hyperlinks.
# - gnuplot-tikz.help not included in the resulting *.tex file because
#   translation and conversion to utf8 would happen too late
#   for this build recipe.
allterm-ja.h: $(CORETERM) $(LUA_HELP)
	-mkdir ja
	-mkdir ja/docs;
	cp $(top_srcdir)/docs/doc2rtf.c ja/docs
	cp -r $(top_srcdir)/term $(TJ)
	-( cd ja ; patch -p0 < $(top_srcdir)/docs/term-ja.diff )
	$(AM_V_GEN) for e in `grep -E "^[ 	]*START_HELP" $(JATERM) |\
	     LC_ALL=C sort -f -t':' -k2` ; do \
	  f=`echo $$e |cut -d\: -f1` ; s=`echo $$e | cut -d\: -f2` ;\
	  sed -n "/^[ 	]*$$s/,/^[ 	]*END_HELP/p" $$f ; \
	done >allterm.tmp
	iconv -f EUC-JP -t UTF-8 < allterm.tmp >$@
	rm allterm.tmp
	-chmod -R +w $(TJ)
	rm -rf $(TJ)

gnuplot-ja.pdf: gnuplot-ja.doc title-ja.tex pdf_figures allterm-ja.h doc2tex$(EXEEXT)
	iconv -f EUC-JP -t UTF-8 gnuplot-ja.doc > ja.doc
	./doc2tex$(EXEEXT) -figures -japanese ja.doc | sed 's/titlepag.tex/title-ja.tex/' > gnuplot-ja.tex
	lualatex gnuplot-ja
	lualatex gnuplot-ja
	makeindex gnuplot-ja
	lualatex gnuplot-ja
	rm -f gnuplot-ja.ind gnuplot-ja.ilg gnuplot-ja.aux gnuplot-ja.toc
	rm -f gnuplot-ja.idx gnuplot-ja.out
	rm -f ja.doc gnuplot-ja.tex

# this is how to make gnuplot.hlp
hlp: gnuplot.hlp

gnuplot.hlp: doc2hlp$(EXEEXT) $(srcdir)/gnuplot.doc
	$(AM_V_GEN)./doc2hlp$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.hlp

doc2hlp_SOURCES = doc2hlp.c termdoc.c

### gnuplot interactive help format
gih: gnuplot.gih

gnuplot.gih: doc2gih$(EXEEXT) $(srcdir)/gnuplot.doc
	$(AM_V_GEN)./doc2gih$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.gih

doc2gih_SOURCES = doc2gih.c termdoc.c

# To include all terminals in the .gih file
allgih: alldoc2gih$(EXEEXT) $(srcdir)/gnuplot.doc
	@echo "generate gnuplot.gih with all terminals"
	$(AM_V_at)./alldoc2gih$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.gih

alldoc2gih_SOURCES = doc2gih.c termdoc.c
alldoc2gih_CPPFLAGS = -DALL_TERM_DOC $(AM_CPPFLAGS)

### wxhelp format
htb: gnuplot.htb

gnuplot.htb: wxhelp/wgnuplot.html wxhelp_figures	\
$(srcdir)/windows/wgnuplot.hhp wxhelp/wgnuplot.hhc	\
$(srcdir)/windows/wgnuplot.hhk
	$(AM_V_GEN) zip -j9 gnuplot.htb windows/wgnuplot.hhp		\
wxhelp/wgnuplot.hhc wxhelp/wgnuplot.hhk wxhelp/*.html windows/*.png

wxhelp/wgnuplot.html: doc2wxhtml$(EXEEXT) $(srcdir)/gnuplot.doc 
	$(AM_V_GEN) ./doc2wxhtml$(EXEEXT) $(srcdir)/gnuplot.doc wxhelp/

doc2wxhtml_SOURCES = windows/doc2html.c termdoc.c xref.c allterm.h
doc2wxhtml_CPPFLAGS = -DALL_TERM_DOC -DWXHELP -I../src $(AM_CPPFLAGS)

wxhelp_figures: $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	$(AM_V_at) [ -d windows ] || mkdir -p windows
	$(AM_V_at) GNUPLOT_LIB=$(top_srcdir)/demo $(GNUPLOT_EXE) -e "winhelp=1;" $(srcdir)/plotstyles.gnu
	$(AM_V_GEN)touch $@

### HTML documentation for web site
##  I gave up on doc -> tex -> html 
#
doc2web_SOURCES = doc2web.c termdoc.c xref.c allterm.h
doc2web_CPPFLAGS = -DALL_TERM_DOC -I../src $(AM_CPPFLAGS)

html: allterm.h doc2web$(EXEEXT) $(srcdir)/gnuplot.doc svg_figures webp_figures
	$(AM_V_at) [ -d html ] || mkdir -p html
	$(AM_V_at) rm -f html/Overview.html html/Plotting_Styles.html html/Commands.html html/Terminals.html
	$(AM_V_at) rm -f html/NewFeatures.html
	$(AM_V_GEN)./doc2web$(EXEEXT) $(srcdir)/gnuplot.doc html
	$(AM_V_GEN) sort -k 3 html/index.hhk > html/index.html

svg_figures: $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	$(AM_V_at) [ -d html ] || mkdir -p html
	$(AM_V_at) GNUPLOT_LIB=$(top_srcdir)/demo GNUTERM=svg $(GNUPLOT_EXE) $(srcdir)/plotstyles.gnu
	$(AM_V_GEN)touch $@

webp_figures: $(GNUPLOT_EXE) $(srcdir)/webp_figures.gnu
	$(AM_V_at) [ -d html ] || mkdir -p html
	$(AM_V_at) GNUPLOT_LIB=$(top_srcdir)/demo $(GNUPLOT_EXE) $(srcdir)/webp_figures.gnu
	$(AM_V_GEN)touch $@


### GNU info format
info: gnuplot.info

# Thanks to Bruce Ravel for doc2texi.el!
gnuplot.texi gnuplot-eldoc.el: $(srcdir)/doc2texi.el $(srcdir)/gnuplot.doc
	$(AM_V_GEN) if test "$(EMACS)" != no; then \
	   test "$(top_srcdir)" = "$(top_builddir)" || cp $(srcdir)/gnuplot.doc . ; \
	   $(EMACS) -batch -q --no-site-file -l $< -f d2t-doc-to-texi ; \
	   test "$(top_srcdir)" = "$(top_builddir)" || rm -f ./gnuplot.doc ; \
	else \
	   echo "No emacs found - cannot create texinfo file" ; \
	fi

${ELCS}: gnuplot-eldoc.el
	$(AM_V_GEN) if test "$(EMACS)" != no; then \
	   echo "Compiling gnuplot-eldoc.el" ; \
	   $(EMACS) -batch -q --no-site-file -f batch-byte-compile $* ; \
	else \
	   echo "No emacs found - cannot create gnuplot-eldoc.elc file" ; \
	fi

# Rules processing texinfo could be provided by automake
# automatically, using a line like info_TEXINFOS = gnuplot.texi.  But
# that only appears to work with a non-generated texi file for input.

gnuplot.info: gnuplot.texi
	$(AM_V_GEN) $(MAKEINFO) -I$(srcdir) $< --no-split --output=$@

install-info: gnuplot.info
	$(AM_V_at) $(top_srcdir)/mkinstalldirs $(DESTDIR)$(infodir)
	$(AM_V_at) $(INSTALL_DATA) gnuplot.info $(DESTDIR)$(infodir)/gnuplot.info
	$(AM_V_at) if (install-info --version && \
	     install-info --version | grep -F -i -v debian) >/dev/null 2>&1; then \
	  install-info --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/gnuplot.info || : ;\
	else : ; fi

# this is how to make OS/2 ipfc documentation
ipf: gnuplot.ipf

gnuplot.ipf: doc2ipf$(EXEEXT) $(srcdir)/gnuplot.doc
	$(AM_V_GEN) ./doc2ipf$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.ipf

doc2ipf_SOURCES = doc2ipf.c termdoc.c xref.c

### Rich Text Format
rtf: gnuplot.rtf

gnuplot.rtf: doc2rtf$(EXEEXT) $(srcdir)/gnuplot.doc
	$(AM_V_GEN) ./doc2rtf$(EXEEXT) $(srcdir)/gnuplot.doc gnuplot.rtf

doc2rtf_SOURCES = doc2rtf.c termdoc.c xref.c

# this is how to check the gnuplot.doc file
check-local: checkdoc$(EXEEXT)
	$(AM_V_at)./checkdoc$(EXEEXT) < $(srcdir)/gnuplot.doc; \
	if test $$? -eq 0; then \
	  echo "PASS: gnuplot.doc"; \
	else \
	  :; \
	fi

checkdoc_SOURCES = checkdoc.c termdoc.c gnuplot.doc

clean-local:
	$(AM_V_at)for f in figure*.pdf figure*.tex $(LUA_HELP); do \
	   if [ "$$f" != "figure_missing.pdf" ]; then \
	     rm -f $$f ; \
	   else \
	     :; \
	   fi \
	 done
	$(AM_V_at)rm -f pdf_figures
	$(AM_V_at)rm -rf ja

distclean: clean
	-rm -f $(BUILT_SOURCES)

#the end
