# Hello, emacs: this is a -*- Makefile -*-
#
# GNUPLOT Makefile for DJGPP version 2 using a cross compiler and a Unix-like shell
#
# Tested on Windows using MSYS2.
#
# Usage:
#   Change the configuration section in 'config/djgpp/Makfile' to
#     match your setup.
#   Start e.g. the MSYS/MSYS2 prompt and change directory to 'config/djgpp'.
#   Options can also be changed on the command line, e.g.:
#	 make GD=1 LUA=1 ICONV=1 CERF=1 MGRX=1 all
#	 make install
#
# AUTHORS : several ... including Edzer Pebesma <pebesma@metoor.frw.uva.nl>
#           and         Hans-Bernhard Broeker <broeker@physik.rwth-aachen.de>
#           and         Bastian Maerkisch
# 
# "install" target: copies the executable etc.to a given
# directory; to customize change FINALDEST below
#

# External libraries
#  Use the MGRX fork instead of GRX
#MGRX=1
#  lua, libiconv and readline are available as binaries
LUA=1
ICONV=1
#READLINE=1
#  libgd has to be compiled from sources, it requires
#  libjpeg, libpng16 and libz (binaries available) and
#  freetype (sources)
#GD=1
#GD_FONTCONFIG=1
#  cerf has to be compiled from sources
#CERF=1

# Options
DJSVGA=1
MOUSE=1
#DEBUG=1

# Choose cross-platform compilation (CROSS not defined is untested)
CROSS=1

#
# Destination of the install target
# (no trailing \ for directory names)
FINALDEST=c:/dos

# here customize the name of the executable
# exe file will be  gnuplot.exe:
EXENAME = gnuplot

# where to place gnuplot.gih helpfile upon MAKE
# note it will be built inside gnuplot\$(HELPDEST)
# and eventually copied to FINALDEST during install
HELPDEST =.

# name of the helpfile
HELPFILE =gnuplot.gih

# gnuplot will look for gnuplotrc here:
GNUPLOT_SHARE_DIR = share

# Where to install the PostScript prologue files, relatively to the executable
# gnuplot.exe
GNUPLOT_PS_DIR = share/PostScript

# Similarly for javascript files for the canvas terminal:
GNUPLOT_JS_DIR = share/js

# Similarly for lua scripts used by the lua terminal
GNUPLOT_LUA_DIR = share/lua

# Install will put TeX files here:
GNUPLOT_TEX_DIR = share/texmf/tex


# DJGPP (cross-)compiler
ifdef CROSS
DJDIR = c:/DJGPP/i586-pc-msdosdjgpp
DJBIN = c:/DJGPP/bin
CC = $(DJBIN)/i586-pc-msdosdjgpp-gcc
OBJCOPY = $(DJBIN)/i586-pc-msdosdjgpp-objcopy
STUBIFY = $(DJDIR)/bin/stubify
DXE3GEN = $(DJDIR)/bin/dxe3gen
UNIXTOOLS=1
else
CC = gcc
OBJCOPY = objcopy
STUBIFY = stubify
DXE3GEN = dxe3gen
endif

ifdef UNIXTOOLS
COPY = cp -p
DEL = rm
MKDIR = mkdir -p
XCOPY = cp -r
WD = ./
SS = \"
else
COPY = copy
DEL = del
MKDIR = md
XCOP = xcopy
WD =
SS = "
endif

# Native compiler
NCC = gcc

# include directories
INCLUDE = -I. -I$(S) -Ic:/djgpp/include
NINCLUDE = -I$(TOP)/config/mingw -I$(S)

# libraries
LIB = -Lc:/djgpp/lib

TERMFLAGS = \
	-DGNUPLOT_PS_DIR=$(SS)$(GNUPLOT_PS_DIR)$(SS) \
	-DGNUPLOT_JS_DIR=$(SS)$(GNUPLOT_JS_DIR)$(SS)
TERMLIBS =

ifdef LUA
	OPTS += -DHAVE_LUA
	TERMFLAGS += -DGNUPLOT_LUA_DIR=$(SS)$(GNUPLOT_LUA_DIR)$(SS)
	TERMLIBS += -llua
	LUATARGETS += gnuplot-lua-tikz.sty
	LUA_HELP = gnuplot-tikz.help
endif

# for svga term driver
ifdef DJSVGA
ifndef MGRX
	TERMFLAGS += -DDJSVGA=1 -DHAVE_GRX -Ic:/djgpp/contrib/grx249/include
	TERMLIBS  += -Lc:/djgpp/contrib/grx249/lib/dj2 -lgrx20
else
	TERMFLAGS += -DDJSVGA=1 -DHAVE_GRX -DHAVE_MGRX -Ic:/djgpp/contrib/mgrx110/include
	TERMLIBS  += -Lc:/djgpp/contrib/mgrx110/lib/dj2 -lmgrx
endif
else
	TERMFLAGS += -DDJSVGA=0
endif

ifdef MOUSE
#ifdef MGRX
	OPTS += -DUSE_MOUSE
#endif
endif

ifdef ICONV
	OPTS += -DHAVE_ICONV
	LIB += -liconv 
endif

ifdef GD
	OPTS += -Ic:/djgpp/include/gd \
		-DHAVE_LIBGD -DHAVE_GD_H -DHAVE_GD_GIF -DGIF_ANIMATION -DHAVE_GD_PNG -DHAVE_GD_JPEG \
		-DHAVE_GD_TTF
	TERMLIBS += -lgd -ljpeg -lpng16 -lz \
		-lfreetype
ifdef GD_FONTCONFIG
	TERMLIBS += -lfontconfig -lexpat -luuid -lfreetype
endif
endif

ifdef READLINE
	OPTS += -DHAVE_LIBREADLINE -D HAVE_READLINE_READLINE_H -DHAVE_READLINE_HISTORY_H -DHAVE_READLINE_RESET \
		-DHAVE_READLINE_SIGNAL_HANDLER -DHAVE_READLINE_PENDING_SIGNAL
	LIB += -lreadline
endif

ifdef CERF
	OPTS += -DHAVE_LIBCERF -DNEED_CEXP
	LIB += -lcerf
endif

ifdef DEBUG
	OPTS += -g
endif

# HBB : added $(OPTS) so user can
#   change CFLAGS easily, without having to change makefile
CFLAGS = -DHAVE_CONFIG_H \
	-DGNUPLOT_SHARE_DIR=$(SS)$(GNUPLOT_SHARE_DIR)$(SS) \
	-O2 $(OPTS) -Wall -Wno-misleading-indentation

LINKOPT = $(LDFLAGS) $(LIB) $(TERMLIBS) -lm -lpc
ifdef DEBUG
	LINKOPT += -g
endif

# abbreviations for relative directory names
TOP=../..
S=$(TOP)/src/
T=$(TOP)/term/
D=$(TOP)/docs/
M=$(TOP)/demo/
O=o

VPATH = $(S)

# use makefile.all:
include $(S)/makefile.all

OBJS = $(COREOBJS) version.$(O)

.SUFFIXES: .exe .o .c

.c.o:  
	$(CC) -c $(CFLAGS) $(INCLUDE) -MMD -MT '$*.o' -MF $*.d -o $@ $<

all: $(EXENAME).exe  $(HELPDEST)/$(HELPFILE)  $(M)binary?  demo_plugin.dxe  $(LUATARGETS)

# -s performs the coff2exe step implicitly (?)
# HBB: that's plain wrong: -s means 'strip executable',
#   and stubify (formerly called stubify) is now always done
#   automatically in the link step. I changed this so you
#   get a debuggable gnuplot, but a stripped gnuplot.exe
$(EXENAME).exe : $(OBJS)
	$(CC) -o $(EXENAME) $(OBJS) $(LINKOPT)
	$(OBJCOPY) --strip-all $(EXENAME) $(EXENAME).exe
	$(STUBIFY) $(EXENAME).exe

command.o: command.c plot.h setshow.h help.h
	$(CC) -c $(CFLAGS) $(INCLUDE) -DHELPFILE=$(SS)$(HELPFILE)$(SS) -MMD -MT '$*.o' -MF $*.d -o $@ $<

show.o: show.c plot.h setshow.h
	$(CC) -c $(CFLAGS) $(INCLUDE) -DHELPFILE=$(SS)$(HELPFILE)$(SS) -MMD -MT '$*.o' -MF $*.d -o $@ $<

term.o: term.c term.h plot.h setshow.h bitmap.h $(CORETERM)
	$(CC) -c $(CFLAGS) $(INCLUDE) $(TERMFLAGS) -I$(T) -MMD -MT '$*.o' -MF $*.d -o $@ $<

EMAIL=gnuplot-info@lists.sourceforge.net
version.o: version.c
	$(CC) -c -DCONTACT=$(SS)$(EMAIL)$(SS) $(CFLAGS) $(INCLUDE) -MMD -MT '$*.o' -MF $*.d -o $@ $<

# convert gnuplot.doc to gnuplot.gih
$(HELPDEST)/$(HELPFILE): doc2gih.exe $(D)gnuplot.doc
	$(WD)doc2gih.exe  $(D)gnuplot.doc > $@

doc2gih.exe: $(D)doc2gih.c $(D)termdoc.c $(CORETERM) $(LUA_HELP)
	$(NCC) $(CFLAGS) -DWINDOWS_NO_GUI $(NINCLUDE) $(TERMFLAGS) -I$(T) -o $@ $(D)doc2gih.c $(D)termdoc.c

demo_plugin.o: $(M)plugin/demo_plugin.c
	$(CC) $(CFLAGS) $(INCLUDE) -I$(M)plugin -c -o $@ $<

demo_plugin.dxe: demo_plugin.o
ifdef CROSS
	PATH=$(subst /,\\,$(DJBIN)) DJDIR=$(DJDIR) $(DXE3GEN) -o $@ -V -E _sinc -E _nsinc -E _divisors $< -lm -lc
else
	$(DXE3GEN) -o $@ -V -E _sinc -E _nsinc -E _divisors $< -lm -lc
endif

gnuplot-lua-tikz.sty: $(T)/lua/gnuplot-tikz.lua
	lua.exe $< style

$(LUA_HELP): $(T)/lua/gnuplot-tikz.lua
	lua.exe $< termhelp > $@

# now for the demo bin
$(M)binary?: bf_test.exe
	$(WD)bf_test.exe
	$(COPY) binary? $(M)
	$(DEL) binary?

bf_test.o: bf_test.c
	$(NCC) $(NINCLUDE) $(CFLAGS) -c $<

bf_test.exe: bf_test.o
	$(NCC) $(NINCLUDE) $(CFLAGS) -DWINDOWS_NO_GUI bf_test.o -lm -o $@

# now install things and create a .bat file to make things easier
install: all
	-$(MKDIR) $(FINALDEST)\bin
	$(COPY) $(EXENAME).exe $(FINALDEST)\bin
ifdef DEBUG
	$(COPY) $(EXENAME) $(FINALDEST)\bin
endif
	$(COPY) $(HELPDEST)\\$(HELPFILE) $(FINALDEST)\bin
# configuration files
	-$(MKDIR) $(FINALDEST)/$(GNUPLOT_SHARE_DIR)
	$(COPY) $(TOP)/share/gnuplotrc $(FINALDEST)/$(GNUPLOT_SHARE_DIR)
	$(COPY) $(TOP)/share/*.gp $(FINALDEST)/$(GNUPLOT_SHARE_DIR)
# terminal support files
	-$(MKDIR) $(FINALDEST)/$(GNUPLOT_PS_DIR)
	$(COPY) $(TOP)/term/PostScript/*.ps $(FINALDEST)/$(GNUPLOT_PS_DIR)/
	$(COPY) $(TOP)/term/PostScript/*.txt $(FINALDEST)/$(GNUPLOT_PS_DIR)/
	-$(MKDIR) $(FINALDEST)/$(GNUPLOT_JS_DIR)
	$(COPY) $(TOP)/term/js/*.* $(FINALDEST)/$(GNUPLOT_JS_DIR)/
	$(COPY) $(TOP)/term/js/README $(FINALDEST)/$(GNUPLOT_JS_DIR)/
	$(MKDIR) $(FINALDEST)/$(GNUPLOT_TEX_DIR)/generic/gnuplot
	$(MKDIR) $(FINALDEST)/$(GNUPLOT_TEX_DIR)/plain/gnuplot
	$(MKDIR) $(FINALDEST)/$(GNUPLOT_TEX_DIR)/latex/gnuplot
	$(MKDIR) $(FINALDEST)/$(GNUPLOT_TEX_DIR)/context/gnuplot
	$(COPY) $(TOP)/share/LaTeX/README $(FINALDEST)/$(GNUPLOT_TEX_DIR)/latex/gnuplot/
	$(COPY) $(TOP)/share/LaTeX/gnuplot.cfg $(FINALDEST)/$(GNUPLOT_TEX_DIR)/latex/gnuplot/
ifdef LUA
	$(MKDIR) $(FINALDEST)/$(GNUPLOT_LUA_DIR)
	$(COPY) $(TOP)/term/lua/README $(FINALDEST)/$(GNUPLOT_LUA_DIR)/
	$(COPY) $(TOP)/term/lua/NEWS $(FINALDEST)/$(GNUPLOT_LUA_DIR)/
	$(COPY) $(TOP)/term/lua/TODO $(FINALDEST)/$(GNUPLOT_LUA_DIR)/
	$(COPY) $(TOP)/term/lua/gnuplot-tikz.lua $(FINALDEST)/$(GNUPLOT_LUA_DIR)/
	-$(COPY) gnuplot-lua-tikz.sty $(FINALDEST)/$(GNUPLOT_TEX_DIR)/latex/gnuplot/
	-$(COPY) gnuplot-lua-tikz.tex $(FINALDEST)/$(GNUPLOT_TEX_DIR)/plain/gnuplot/
	-$(COPY) t-gnuplot-lua-tikz.tex $(FINALDEST)/$(GNUPLOT_TEX_DIR)/context/gnuplot/
	-$(COPY) gnuplot-lua-tikz-common.tex $(FINALDEST)/$(GNUPLOT_TEX_DIR)/generic/gnuplot/
endif
# README etc
	-$(COPY) $(TOP)/readme.dj2 $(FINALDEST)/
	$(COPY) $(TOP)/Copyright $(FINALDEST)/
	$(COPY) $(TOP)/README $(FINALDEST)/
	$(COPY) $(TOP)/NEWS $(FINALDEST)/
	$(COPY) $(TOP)/BUGS $(FINALDEST)/
	$(COPY) $(TOP)/RELEASE_NOTES $(FINALDEST)/
	$(COPY) $(TOP)/INSTALL $(FINALDEST)/
	#-unix2dos $(FINALDEST)/*.txt $(FINALDEST)/README
	#-unix2dos $(FINALDEST)/Copyright $(FINALDEST)/NEWS $(FINALDEST)/BUGS $(FINALDEST)/RELEASE_NOTES
# demo and contrib
	$(MKDIR) $(FINALDEST)/demo
	-$(COPY) $(M)* $(FINALDEST)/demo/
	$(MKDIR) $(FINALDEST)/demo/games
	-$(COPY) $(M)games/* $(FINALDEST)/demo/games/
	$(MKDIR) $(FINALDEST)/demo/plugin
	-$(COPY) $(M)plugin/* $(FINALDEST)/demo/plugin/
	-$(COPY) demo_plugin.dxe $(FINALDEST)/demo/plugin/
#HBB: if at all, this batch-file should only be generated by a
#separate target, like 'install-bat'. Besides, it doesn't work,
#as is, because DOS always executes foo.exe before even looking for
#a foo.bat
#	$(DEL)  $(EXENAME).bat
#	echo @echo off > $(EXENAME).bat
#	echo SET GNUHELP= $(FINALDEST)\\$(HELPFILE) >> $(EXENAME).bat
#	echo SET GNUPLOT= $(FINALDEST) >> $(EXENAME).bat
#	echo $(FINALDEST)\\$(EXENAME).exe %1 %2 %3 %4 %5 %6 >> $(EXENAME).bat
#	echo SET GNUHELP= >> $(EXENAME).bat
#	echo SET GNUPLOT= >> $(EXENAME).bat
#	$(COPY) $(EXENAME).bat $(FINALDEST)


# clean target - remove all temp files, but leave executable intact
clean:
	-$(DEL) *.o
	-$(DEL) $(EXENAME)
	-$(DEL) doc2gih
	-$(DEL) doc2gih.exe
	-$(DEL) bf_test
	-$(DEL) bf_test.exe

# realclean target - remove all files created by the makefile
realclean: clean
	-$(DEL) $(EXENAME).exe
	-$(DEL) $(HELPDEST)/$(HELPFILE)
	-$(DEL) $(TOP)/demo/binary1
	-$(DEL) $(TOP)/demo/binary2
	-$(DEL) $(TOP)/demo/binary3
	-$(DEL) $(TOP)/demo/fit.log
	-$(DEL) $(TOP)/demo/soundfit.par
	-$(DEL) demo_plugin.dxe

ifndef NODEPS
-include $(OBJS:.$(O)=.d)
endif

#####
##     END OF GNUPLOT MAKEFILE FOR DJGPP V2
#####
