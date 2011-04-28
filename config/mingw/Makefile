# Hello, emacs: this is a -*- Makefile -*-
# $Id: Makefile,v 1.9 2011/04/17 22:36:34 markisch Exp $
#
# GNUPLOT Makefile for MinGW32 on Win95/98/NT/2k/..
# This assumes usual installation of the MinGW32 toolkit, the free
# GNU gcc. See www.mingw.org for more details.

# To compile gnuplot for Windows:
#
# - compile the package: go to directory 'gnuplot/config/mingw' and issue the command
#	 make
#	 make install
#
# This makefile was tested with MSYS, MingW32 and Microsoft Help
# Workshop 4.03 / Microsoft HTML Help 1.4 SDK.


#
# ************** Begin of Configuration section ************************
#

# Choose target to compile:
# 1. wgnuplot.exe: The usual Windows GUI for input commands.
# 2. wgnuplot_pipes.exe: The usual Windows GUI with support of pipes, so
#    that e.g.  plot '<awk -f preprocess.awk my.dat')  or  x=system('ls -1')
#    can work. Drawback: wgnuplot_pipes.exe keeps attached to a console (DOS)
#    window it was launched from, or it opens a new one.
# 3. gnuplot.exe: Console mode instead of GUI for the text input.
#
# Comment for programs using gnuplot as their plotting engine:
# Both wgnuplot*.exe compile also pgnuplot.exe. This small helper program was
# used when gnuplot.exe was not available on Windows. Nowadays, pgnuplot.exe
# is deprecated -- please use gnuplot.exe instead.

ifndef TARGET
TARGET=wgnuplot.exe
#TARGET=wgnuplot_pipes.exe
#TARGET=gnuplot.exe
endif

# Comment out the definition lines to disable the according features:

# Use more modern HTML help instead of no longer supported Windows help system
# Please update the HHWPATH below if necessary.
HTML_HELP=1

# GIF, PNG, JPEG device drivers
# Requires gd library. There are two possibilities how to configure these
# drivers for use in gnuplot, depending on the gd version. See README.1ST for
# more details.
#
# You should compile gnuplot with GD library v2.0 or newer.
# This library writes PNG, GIF and JPEG images.
# If libgd has been compiled with TrueType font support, then you can use
# scaled TrueType fonts. If not, then uncomment FREETYPE.
# Requires GD, PNG and Z libraries, optionally libfreetype.
# In some cases, libfreetype can depend on additional libraries such as
# fontconfig or iconv; then, uncomment GDAUTOCONFIGLIBS so that all of the
# libraries needed for linking will be taken by running 'gdlib-config'.
# Uncomment BGD if you want to use the binary DLL version of libgd.
#NEWGD=1
#JPEG=1
#FREETYPE=1
#GDAUTOCONFIGLIBS=1
#BGD=1

# PDF device driver
# Requires PNG and Z libraries based on particular PDF library used, and
# optionally also TIFF library according to
# 1. defined PDF_NOTIFF:
#    'Light' pdf library (produces wgnuplot.exe only 200 KB larger!)
#    You can get this pdf library by compiling it without PNG, GD and TIFF
#    support: change  pdflib/p_intern.h  and recompile the pdf library.
#    Gnuplot does not use the tiff routines, and gd/png are elsewhere.
# 2. undefined PDF_NOTIFF:
#    Full pdf library (produces wgnuplot.exe 500 KB larger!)
#PDF=1
#PDF_NOTIFF=1

# DEBUGging support
# creates binaries suitable for debugging. Some bugs may come and go
# as opposed to a production build since we lower the optimization level
#DEBUG=1

# MOUSE support for the windows terminal
MOUSE=1

# Cairo/Pango terminals
# Uncomment the following lines to select the pngcairo and pdfcairo
# terminals
#CAIROTERMS=1
#CAIROLIBS=1

# WXWIDGETS interactive terminal
# Requires wxWidgets, Cairo, Pango, and their respective dependencies.
# Uncomment the following lines to compile it.
#WXT=1
#CAIROLIBS=1

# Uncomment the following line to select the lua/tikz terminal
#LUA=1

# Below you can adapt paths according to your software setup:

# Where to place the helpfile:
ifdef HTML_HELP
  HELPFILE = wgnuplot.chm
else
  HELPFILE = wgnuplot.hlp
endif

# Where to install the PostScript prologue files, relatively to the executable
# wgnuplot.exe
GNUPLOT_PS_DIR = share/PostScript

# Similarly for javascript files for the canvas terminal:
GNUPLOT_JS_DIR = share/js

# Similarly for scripts used by the lua terminal
GNUPLOT_LUA_DIR = share/lua

# Destination directory, used in 'make install':
DESTDIR = /local/opt/gnuplot-4.5

# give here the path to MinGW compiler unless it is already in PATH
#GCCPATH = g:/mingw32/bin/
#GCCPATH = c:/apps/mingw/bin/

# Do you want some special optimization?
# -mpentium means optimise for Pentium processor
# -mpentiumpro means optimize for Pentium II and Pro procesors
#CFLAGS =

ifdef HTML_HELP

# To compile the .chm file you need the Microsoft HTML Help 1.4 SDK
# It can be obtained here:
# http://go.microsoft.com/fwlink/?LinkId=154968
# We need to explicitly set the correct path here since we also need
# adjust the include and linrary paths.
HHWPATH = "$(PROGRAMFILES)/HTML Help Workshop/"
HHC = $(HHWPATH)hhc

else

# To compile the .hlp file you need hcw either out of Microsoft SDK or MS Help
# Workshop. The latter can be obtained from either of
# ftp://ftp.microsoft.com/softlib/mslfiles/hcwsetup.exe
# http://www.microsoft.com/downloads/details.aspx?displaylang=en&FamilyID=34D35502-4DE9-4676-952C-34CC7F64F098
# Put the path to hcw here unless it is already in PATH:
HCWPATH = "$(PROGRAMFILES)/Help\ Workshop/"
HCW = $(HCWPATH)hcw
# Switches are for HCW 4.03:
HCWFLAG =

endif

# Choose which windres/rc do you want to use (GNU windres or MS RC):
GNU_RC = 1
ifndef GNU_RC
  # If uncommented GNU_RC above and rc.exe not in PATH, then set:
  MSSDK = c:/mssdk
endif

# GNU sort
GNUSORT = /bin/sort

#
# *************** End of Configuration section *************************
#


#         There shouldn't be anything to be changed below this line
# **********************************************************************
#

# PIPES: define if you would prefer support of pipes undef Windows (e.g.
# plot '<awk -f preprocess.awk my.dat'). Drawback: wgnuplot_pipes.exe keeps
# attached to a console=DOS window it was launched from, or it opens a new one.
# Note: Maybe it does not work properly with pgnuplot (not tested recently).

# CONSOLE: define if you would prefer the console (non-GUI) version of the
# text command window.

# Set PGNUPLOT, PIPES and CONSOLE according to the target
ifeq ($(TARGET),wgnuplot.exe)
	PGNUPLOT=pgnuplot.exe
	MENUFILE=wgnuplot.mnu
endif
ifeq ($(TARGET),wgnuplot_pipes.exe)
	PGNUPLOT=pgnuplot.exe
	MENUFILE=wgnuplot.mnu
	PIPES=1
endif
ifeq ($(TARGET),gnuplot.exe)
	PIPES=1
	CONSOLE=1
endif


CC = $(GCCPATH)gcc
LD = $(CC)
CXX = $(GCCPATH)g++
LDX = $(CXX)

RM = rm -f
CP = cp -p

ifdef DEBUG
  CFLAGS += -g -Wall
  LDFLAGS += -g
else
  CFLAGS += -O2
  LDFLAGS += -s
endif

LDFLAGS2 = -mwindows
ifdef PIPES
  OPTS += -DPIPES
  LDFLAGS2 = -mconsole
endif

ifdef CONSOLE
  OPTS += -DWGP_CONSOLE
  LDFLAGS2 = -mconsole
endif

TOP = ../..

# macros for makefile.all
O=o
S=$(TOP)/src
W=$(S)/win
T=$(TOP)/term/
D=$(TOP)/docs/
L=docs/latex/
M=$(TOP)/demo/

VPATH = $(S)

TERMFLAGS = -DGNUPLOT_PS_DIR=\"$(GNUPLOT_PS_DIR)\" -DGNUPLOT_JS_DIR=\"$(GNUPLOT_JS_DIR)\"
TERMFLAGS += -I$(TOP)/term

TERMLIBS =

CFLAGS += -I. -I$(S) -D_Windows -DHAVE_CONFIG_H $(OPTS)

ifdef MOUSE
  CFLAGS += -DUSE_MOUSE=1 -DWIN_IPC
endif

ifdef HTML_HELP
  CFLAGS += -DWITH_HTML_HELP -I$(HHWPATH)include
  BUILDHELPFILE = $(D)windows/$(HELPFILE)
else
  BUILDHELPFILE = $(HELPFILE)
endif

ifdef NEWGD
  GD=1
  PNG=1
endif

ifdef GD
  CFLAGS += -DHAVE_LIBGD
  ifndef BGD
    TERMLIBS += -lgd
  else
    TERMLIBS += -lbgd
  endif
endif

ifdef PNG
  CFLAGS += -DHAVE_LIBPNG
  ifndef BGD
  ifndef GDAUTOCONFIGLIBS
    TERMLIBS += -lpng -lz
  endif
  endif
endif

ifdef NEWGD
  CFLAGS += -DHAVE_GD_GIF -DGIF_ANIMATION -DHAVE_GD_PNG
ifdef JPEG
  CFLAGS += -DHAVE_GD_JPEG
  ifndef BGD
  ifndef GDAUTOCONFIGLIBS
    TERMLIBS += -ljpeg
  endif
  endif
endif
ifdef FREETYPE
  CFLAGS += -DHAVE_GD_TTF
  ifndef BGD
  ifndef GDAUTOCONFIGLIBS
    TERMLIBS += -lfreetype
  endif
  endif
endif
ifdef GDAUTOCONFIGLIBS
  TERMLIBS += $(shell gdlib-config --libs)
endif
endif

ifdef PDF
  CFLAGS += -DHAVE_LIBPDF
ifdef PDF_NOTIFF
  TERMLIBS += -lpdf_notiff
else
  TERMLIBS += -lpdf -ltiff
#  TERMLIBS += -lpdf-w -ltiff-w
endif
ifndef PNG
  NEED_PNG_W = 1
else
ifndef NEWGD
  NEED_PNG_W = 1
endif
endif
ifdef NEED_PNG_W
  CFLAGS += -DHAVE_LIBPNG
  TERMLIBS += -lpng-w -lz-w
endif
endif

ifdef CAIROLIBS
	CAIRO_CFLAGS := $(shell pkg-config --cflags cairo | sed s_-Ic:_-I/c_g)
	PANGOCAIRO_CFLAGS := $(shell pkg-config --cflags pangocairo cairo | sed s_-Ic:_-I/c_g)
	PANGOCAIRO_LIBS := $(shell pkg-config --libs pangocairo cairo | sed s_-Lc:_-L/c_g)
	CAIRO_OBJS := gp_cairo.$(O) gp_cairo_helpers.$(O)
	TERMFLAGS += $(PANGOCAIRO_CFLAGS)
endif

ifdef CAIROTERMS
	CFLAGS += -DHAVE_CAIROPDF
endif

ifdef WXT
	CFLAGS += -DWXWIDGETS
	WXT_CXXFLAGS := $(shell wx-config --cxxflags)
	CXXFLAGS += $(WXT_CXXFLAGS)
	WX_LIBS := $(shell wx-config --libs | sed -e "s+-Wl,--subsystem,windows++g" -e "s+-mwindows++g")
	WX_OBJS = wxt_gui.$(O)
endif

ifdef LUA
	CFLAGS += -DHAVE_LUA
	TERMLIBS += -llua
	TERMFLAGS += -DGNUPLOT_LUA_DIR=\"$(GNUPLOT_LUA_DIR)\"
endif

ifdef GNU_RC
  # RC = d:/cygnus/cygwin-b20/H-i586-cygwin32/bin/windres
  RC  = $(GCCPATH)windres
  RCFLAGS = --include-dir=$(W) \
	--define __WIN32__ --define __WIN95__ --define MSRC \
	--define __GNUWIN32__ --use-temp-file
  RCOUT = wgplt_res.$(O)
  RES2COFF = echo wgplt_res.$(O)
else
  RC = $(MSSDK)/bin/rc
  RCFLAGS = -v -i$(MSSDK)/include -i$(W) -dWIN32 -dMSRC
  RCOUT = -fowgnuplot.res
  RES2COFF = res2coff -i wgnuplot.res -o wgplt_res.$(O)
endif

default: $(TARGET) $(HELPFILE) $(MENUFILE) $(M)bf_test.exe $(PGNUPLOT)

all:	default gnuplot.ps gnuplot.pdf


# include the 'core makefile template'
include $(S)/makefile.all

OBJS = $(COREOBJS) version.$(O) gpexecute.$(O) $(WX_OBJS) $(CAIRO_OBJS)

WINOBJS = winmain.$(O) wgnuplib.$(O) wgraph.$(O) wprinter.$(O) wtext.$(O) \
	  wpause.$(O) wmenu.$(O) screenbuf.$(O) wgdiplus.$(O)

WINDOWS = makefile.nt README.win $(W)/wcommon.h \
	$(W)/wgnuplib.c $(W)/wgnuplib.h $(W)/wgnuplib.rc \
	$(W)/wgnuplot.hpj $(W)/wgnuplot.mnu $(W)/wgraph.c \
	$(W)/winmain.c $(W)/wmenu.c $(W)/wpause.c $(W)/wprinter.c \
	$(W)/wresourc.h $(W)/wtext.c $(W)/wtext.h $(W)/geticon.c \
	$(W)/screenbuf.c $(W)/screenbuf.h $(W)/wgdiplus.cpp $(W)/wgdiplus.h \
	$(D)doc2rtf.c $(T)win.trm $(W)/grpicon.ico $(W)/texticon.ico

ifdef HTML_HELP
WINDOWS +=	$(D)windows/doc2html.c
else
WINDOWS +=	$(D)doc2rtf.c
endif

# default rules
.SUFFIXES: .exe .$(O) .c
.c.$(O):
	$(CC) -c $(CFLAGS) $<

LDLIBS = -lkernel32 -lgdi32 -lwinspool -lcomdlg32 -lcomctl32 -ladvapi32 -lshell32 -lmsimg32 -lgdiplus

ifdef HTML_HELP
  LDLIBS += -lhtmlhelp
  LDFLAGS2 += -L$(HHWPATH)lib
endif

$(TARGET): $(OBJS) $(WINOBJS) wgplt_res.$(O) texticon.ico grpicon.ico
	$(LDX) $(LDFLAGS) $(LDFLAGS2) -o $@ $(OBJS) $(WINOBJS) wgplt_res.$(O) $(LDLIBS)\
	$(TERMLIBS) $(WX_LIBS) $(PANGOCAIRO_LIBS)

$(PGNUPLOT): $(W)/pgnuplot.c version.$(O)
	$(CC) -O2 -DHAVE_STDBOOL_H -s -o $@ $^ -I$(W) -I$(S) -luser32

# rules

wgplt_res.$(O) :  $(W)/wgnuplot.rc $(W)/wgnuplib.rc $(W)/wresourc.h texticon.ico grpicon.ico
	$(RC) $(RCFLAGS) $< $(RCOUT)
	$(RES2COFF)

show.$(O): show.c plot.h setshow.h
	$(CC) -c $(CFLAGS) -DHELPFILE=\"$(HELPFILE)\" -DBINDIR=\"$(bindir)\" -DGNUPLOT_PS_DIR=\"$(GNUPLOT_PS_DIR)\" $<

term.$(O): term.c term.h plot.h setshow.h bitmap.h $(CORETERM)
	$(CC) -c $(CFLAGS) $(TERMFLAGS) $<

version.$(O): version.c

WINDEPS = $(W)/wgnuplib.h $(W)/wcommon.h $(W)/wresourc.h $(W)/screenbuf.h $(W)/wgdiplus.h

winmain.$(O): $(W)/winmain.c $(W)/wgnuplib.h $(W)/wtext.h plot.h
	$(CC) -c $(CFLAGS) -DHELPFILE=\"$(HELPFILE)\" $<

wgnuplib.$(O): $(W)/wgnuplib.c $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

wmenu.$(O): $(W)/wmenu.c $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

wtext.$(O): $(W)/wtext.c $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

screenbuf.$(O): $(W)/screenbuf.c $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

wpause.$(O): $(W)/wpause.c $(S)/wxterminal/wxt_term.h $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

wprinter.$(O): $(W)/wprinter.c $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

wgraph.$(O): $(W)/wgraph.c $(WINDEPS)
	$(CC) -c $(CFLAGS) $<

wgdiplus.$(O): $(W)/wgdiplus.cpp $(WINDEPS)
	$(CXX) -c $(CXXFLAGS) $(W)/wgdiplus.cpp

wgnuplot.mnu: $(W)/wgnuplot.mnu
	$(CP) $^ $@

wxt_gui.$(O): $(S)/wxterminal/wxt_gui.cpp $(S)/wxterminal/wxt_gui.h
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CAIRO_CFLAGS) $<

gp_cairo.$(O): $(S)/wxterminal/gp_cairo.c $(S)/wxterminal/gp_cairo.h
	$(CC) -c $(CFLAGS) $(CXXFLAGS) $(PANGOCAIRO_CFLAGS) $<

gp_cairo_helpers.$(O): $(S)/wxterminal/gp_cairo_helpers.c $(S)/wxterminal/gp_cairo_helpers.h
	$(CC) -c $(CFLAGS) $<

# extract icons from wgnuplot.rc
texticon.ico: grpicon.ico

grpicon.ico: geticon.exe $(W)/wgnuplot.rc
	./geticon $(W)/wgnuplot.rc

geticon.exe: $(W)/geticon.c
	$(LD) $(LDFLAGS) -o $@ $<

ifdef HTML_HELP
# convert gnuplot.doc to windows/wgnuplot.html
$(BUILDHELPFILE): doc2html.exe $(D)gnuplot.doc $(D)windows/wgnuplot.hhp $(D)windows/wgnuplot.hhk \
                  $(D)windows/wgnuplot.stp $(D)plotstyles.gnu
	./doc2html $(D)gnuplot.doc $(D)windows/
	-$(TARGET) -e "winhelp=1; cd '$(D)'" plotstyles.gnu
	-$(HHC) $(D)windows/wgnuplot.hhp

$(HELPFILE): $(BUILDHELPFILE)
	$(CP) $^ $@
else
# convert gnuplot.doc to gnuplot.rtf
$(HELPFILE): doc2rtf.exe $(D)gnuplot.doc win/wgnuplot.hpj
	./doc2rtf $(D)gnuplot.doc win/gnuplot.rtf
	$(HCW) -c -e win/wgnuplot.hpj
endif

doc2html.exe: $(D)windows/doc2html.c $(D)termdoc.c $(D)xref.c version.$(O)
	$(LD) $(LDFLAGS) -o $@ -DWINDOWS_NO_GUI $(CFLAGS) -I. -I$(D:/=) -I$(T:/=) $^

doc2rtf.exe: $(D)doc2rtf.c $(D)termdoc.c $(D)xref.c
	$(LD) $(LDFLAGS) -o $@ -DWINDOWS_NO_GUI $(CFLAGS) -I$(S) -I$(D) -I$(T) $^

#make binary demo files
$(M)bf_test.exe : bf_test.c dbinary.$(O) alloc.$(O)
	$(LD) $(LDFLAGS) $(CFLAGS) -U_Windows -o $@ $^
	(cd $(M) ; ./bf_test.exe )
#	cd ..

# _Windows causes wtext.h to define fread() etc
dbinary.$(O): binary.c
	$(CC) -c $(CFLAGS) -U_Windows -o $@ $<

#
# Create config.h
#
$(OBJS) $(WINOBJS) dbinary.$(O): config.h
config.h: $(TOP)/config/config.mgw
	cp -p $< $@


# Create documentation in various formats
#
ALL_TERMINALS_DOC=1
ifdef ALL_TERMINALS_DOC
# Generate TeX documentation with the complete list of all terminals
# (gnuplot.tex should be the same on all platforms):
SORT_TERMINALS=1
ifdef SORT_TERMINALS
# sort alphabetically all terminals (note: req. GNU sort, not from MS)
allterm.h: $(CORETERM)
	@echo "Building allterm.h"
	@for e in `egrep "^[ 	]*START_HELP" $(CORETERM) |\
	     $(GNUSORT) -f -t':' -k2` ; do \
	  f=`echo $$e |cut -d\: -f1` ; s=`echo $$e | cut -d\: -f2` ;\
	  sed -n "/^[ 	]*$$s/,/^[ 	]*END_HELP/p" $$f ; \
	done >$@
else
# sequence of terminals according to "ls term/*.trm":
allterm.h: $(CORETERM)
	@echo "Building allterm.h"
	@cat $(T)*.trm > allterm.c
	$(CPP) $(CFLAGS) -I$(T) -DTERM_DRIVER_H -DTERM_HELP allterm.c | \
	  sed '/^ *$$/d;/^#/d' > allterm.h
	@rm -f allterm.c
endif
doc2tex.exe: $(D)doc2tex.c $(D)termdoc.c allterm.h
	$(LD) $(LDFLAGS) -o $@ -DWINDOWS_NO_GUI -DALL_TERM_DOC $(CFLAGS) \
	  -I. -I$(D) -I$(T) $(D)doc2tex.c $(D)termdoc.c
else
# Old version: generate documentation with only currently used terminals:
doc2tex.exe: $(D)doc2tex.c $(D)termdoc.c
	$(LD) $(LDFLAGS) -o $@ -DWINDOWS_NO_GUI $(CFLAGS) -I. -I$(D) -I$(T) $^
endif

gnuplot.tex: $(D)gnuplot.doc doc2tex.exe
	doc2tex $(D)gnuplot.doc gnuplot.tex

# Call LaTeX three times to get the toc right.
gnuplot.dvi: gnuplot.tex $(D)titlepag.tex
	cp gnuplot.tex $(D)gp_tex2.tex
	cp $(TOP)/VERSION $(D)
	cd $(D) && latex gp_tex2.tex && latex gp_tex2.tex && latex gp_tex2.tex
	mv $(D)gp_tex2.dvi gnuplot.dvi
	rm -f $(D)gp_tex2.*
gnuplot.ps: gnuplot.dvi
	dvips -o gnuplot.ps gnuplot.dvi
gnuplot.pdf: gnuplot.tex $(D)titlepag.tex
	cp gnuplot.tex $(D)gp_tex2.tex
	cp $(TOP)/VERSION $(D)
	cd $(D) && pdflatex gp_tex2.tex && pdflatex gp_tex2.tex && pdflatex gp_tex2.tex
	mv $(D)gp_tex2.pdf gnuplot.pdf
	rm -f $(D)gp_tex2.*


# clean up temporary files
clean:
	$(RM) config.h *.$(O) wgnuplot.map wgnuplot.res $(W)/gnuplot.rtf
	$(RM) doc2*.exe $(W)/wgnuplib.res wgnuplib.map wgnuplot.lib
	$(RM) $(M)bf_test.exe *.ico geticon.exe allterm.h allterm.c
	$(RM) gnuplot.tex gnuplot.dvi gnuplot.ps gnuplot.pdf
	$(RM) $(D)windows/wgnuplot.log $(D)windows/wgnuplot.hhc
	$(RM) $(D)windows/*.png $(D)windows/wgnuplot.html

realclean: veryclean
veryclean: clean
	$(RM) wgnuplot.exe wgnuplot_pipes.exe gnuplot.exe pgnuplot.exe
	$(RM) $(HELPFILE) wgnuplot.chw wgnuplot.gid wgnuplot.mnu
	$(RM) $(M)binary[123] $(M)fit.log $(M)soundfit.par

# now move the whole stuff to its destination
install: default
	mkdir -p $(DESTDIR)
	cp gnuplot.exe $(DESTDIR)/gnuplot.exe
	cp wgnuplot.exe $(DESTDIR)/wgnuplot.exe
	cp wgnuplot_pipes.exe $(DESTDIR)/wgnuplot_pipes.exe
	cp pgnuplot.exe $(DESTDIR)/pgnuplot.exe
	cp $(W)/wgnuplot.mnu $(DESTDIR)/wgnuplot.mnu
ifdef HTML_HELP
	cp $(D)windows/wgnuplot.chm $(DESTDIR)/wgnuplot.chm
else
	cp wgnuplot.hlp $(DESTDIR)/wgnuplot.hlp
endif
	mkdir -p $(DESTDIR)/$(GNUPLOT_PS_DIR)
	cp ../term/PostScript/*.ps $(DESTDIR)/$(GNUPLOT_PS_DIR)
	mkdir -p $(DESTDIR)/$(GNUPLOT_JS_DIR)
	cp ../term/js/* $(DESTDIR)/$(GNUPLOT_JS_DIR)
	mkdir -p $(DESTDIR)/$(GNUPLOT_LUA_DIR)
	cp ../term/lua/* $(DESTDIR)/$(GNUPLOT_LUA_DIR)

