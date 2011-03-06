# Hello, emacs: this is a -*- Makefile -*-
# $Id: Makefile,v 1.3 2011/03/05 14:40:33 broeker Exp $
#
# GNUPLOT Makefile OpenWatcom 1.3 on Win32

# To compile:
#       Either, from ..\src:
#         wmake -f ..\config\makefile.oww
#       or copy this file to ..\src, edit it to your liking, then
#       from ..\src:
#         wmake -f makefile.oww
# 
# History of this file:
# 2004-02-23: Created by massive editing of makefile.nt (and config.nt)
# 2004-04-25: Removed all remaining MS-isms (by peeking at how OW's
#  MS namesake replacement tools generate their command lines)
#

#TARGET=wgnuplot.exe
#TARGET=wgnuplot_pipes.exe
TARGET=gnuplot.exe

TOP = ..\..

# activate one of these to choose which of the help file formats you want:
#   old-fashioned WinHelp ...
# HELP_EXT=hlp
#   ... or HTML help
HELP_EXT=chm

# Programs: 
# The Microsoft Help compiler:
HCW = "c:\program files\help workshop\hcw.exe"
# ... and the HTML help compiler
HHC_DIR = c:\Program Files\HTML Help Workshop
HHC = "$(HHC_DIR)\hhc.exe"

# OpenWatcom programs:
WCL = wcl386
WCC = wcc386
WLD = wlink
WRC = wrc

# Speed up compilation a bit by running the compiler as a DLL from
# inside wmake:
!ifdef __LOADDLL__
!  loaddll wcc      wccd
!  loaddll wccaxp   wccdaxp
!  loaddll wcc386   wccd386
!  loaddll wpp      wppdi86
!  loaddll wppaxp   wppdaxp
!  loaddll wpp386   wppd386
!  loaddll wlink    wlinkd
!  loaddll wlib     wlibd
!endif

# Where to install the PostScript prologue files, relatively to the executable
# wgnuplot.exe
GNUPLOT_PS_DIR = share/PostScript

# paths for external libs added here... 
LDLIBS = 

# Macros for makefile.all
O=obj
T=$(TOP)\term\
D=$(TOP)\docs\
M=$(TOP)\demo\
S=$(TOP)\src
W=$(S)\win

.c: $(S)

BINARYFILES=$(M)binary1 $(M)binary2 $(M)binary3

# compiler flags: 
#
# Defines

CPPFLAGS =  -DHAVE_CONFIG_H -D_Windows -DHAVE_CONFIG_H -DUSE_MOUSE -DEAM_OBJECTS
# library-dependent ones may have to be added, like:
# -DHAVE_LIBPNG -DHAVE_LIBPDF 

DEFAULT_TARGETS = $(TARGET) $(BINARYFILES)

!ifeqi TARGET wgnuplot.exe
DEFAULT_TARGETS += wgnuplot.mnu pgnuplot.exe
HELPFILE = wgnuplot.$(HELP_EXT)
!endif
!ifeqi TARGET wgnuplot_pipes.exe
DEFAULT_TARGETS += wgnuplot.mnu pgnuplot.exe
HELPFILE = wgnuplot.$(HELP_EXT)
CPPFLAGS += -DPIPES=1
!endif
!ifeqi TARGET gnuplot.exe
HELPFILE = gnuplot.gih
CPPFLAGS += -DPIPES=1 -DWGP_CONSOLE
!endif

!ifeq HELP_EXT chm
CPPFLAGS += -DWITH_HTML_HELP
LDLIBS += LIBRARY htmlhelp.lib
!endif
DEFAULT_TARGETS += $(HELPFILE)

# compiler flags, determined from MS ones by OW's "cl /showwopts"

# -c  : don't link
# -I  : add directory to include search path
# -zq : don't show a version banner message
# -5r : P5-optimized, 386-compatible code, register calling convention
# -zm : put each function in its own section to improve dead code elimination
# -bm : target multi-threaded environment
# -sg : grow the stack if needed
# -zlf: store library usage in .obj files
# -ei : force sizeof(enum) >= sizeof(int)
# -o  : optimization options:   
#    r:   reorder for pipelining
#    i:   inline intrinsics
#    l:   loop optimizations
#    t:   optimize for speed
# -j  : char is signed
# -aa : allow non-const initializers for locals
# -d2 : debug information
WCFLAGS = -zlf -ei -zq -I$(S);$(W) -zm -bm -5r -sg -orilt -j -aa -d2 $(CPPFLAGS)
# removed: -za99

# ...and here.
# see other terminal defines in term.h
TERMFLAGS = -DGNUPLOT_PS_DIR="$(GNUPLOT_PS_DIR)" -I$(T)

# Note: like makefile.wc, this need the special Watcom-style version of
# makefile.all, because line continuation in WMake is different...
!INCLUDE $(S)\makefile.awc

default: $(DEFAULT_TARGETS) .SYMBOLIC

OBJS = $(COREOBJS) version.obj

WINOBJS = winmain.obj wgnuplib.obj wgraph.obj wprinter.obj wtext.obj &
        wpause.obj wmenu.obj gpexecute.obj

WIN_EXTRA_INPUTS = $(WINOBJS) wgnuplot.res texticon.ico grpicon.ico
# default rule
.c.obj: .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

# file-specific rules
wgnuplot.exe: $(OBJS) $(WIN_EXTRA_INPUTS)
        $(WLD) @<<linkopt1.oww $(LDLIBS)
RUNTIME windows=4.0
SYSTEM nt_win
DEBUG ALL
OPTION quiet
FILE {
 $(OBJS)
 $(WINOBJS)
}
LIBRARY kernel32.lib,user32.lib,gdi32.lib,winspool.lib,comdlg32.lib
LIBRARY advapi32.lib,shell32.lib
LIBRARY msimg32.lib
OPTION map
OPTION SYMFILE
NAME $^&
OPTION stack=1M
OPTION resource=wgnuplot.res
OPTION ELIMINATE
REFERENCE _wstart_ 
OPTION START=_wstart_
<<

wgnuplot_pipes.exe: $(OBJS) $(WIN_EXTRA_INPUTS)
        $(WLD) @<<linkopt1.oww $(LDLIBS)
RUNTIME windows=4.0
SYSTEM nt_win
DEBUG ALL
OPTION quiet
FILE {
 $(OBJS)
 $(WINOBJS)
}
LIBRARY kernel32.lib,user32.lib,gdi32.lib,winspool.lib,comdlg32.lib
LIBRARY advapi32.lib,shell32.lib
LIBRARY msimg32.lib
OPTION map
OPTION SYMFILE
NAME $^&
OPTION stack=1M
OPTION resource=wgnuplot.res
OPTION ELIMINATE
REFERENCE _wstart_ 
OPTION START=_wstart_
<<

gnuplot.exe: $(OBJS) $(WIN_EXTRA_INPUTS) 
        $(WLD) @<<linkopt1.oww $(LDLIBS)
RUNTIME console=4.0
SYSTEM nt
DEBUG ALL
OPTION quiet
FILE {
 $(OBJS)
 $(WINOBJS)
}
LIBRARY kernel32.lib,user32.lib,gdi32.lib,winspool.lib,comdlg32.lib
LIBRARY advapi32.lib,shell32.lib
LIBRARY msimg32.lib
OPTION map
OPTION SYMFILE
NAME gnuplot.exe
OPTION stack=1M
OPTION resource=wgnuplot.res
OPTION ELIMINATE
#REFERENCE _wstart_ 
#OPTION START=_wstart_
<<keep

# NOTE: Unknown what /l 0x409 does in MS RC call --- seems to work without, though
wgnuplot.res :  $(W)\wgnuplot.rc $(W)\wgnuplib.rc &
  $(W)\wresourc.h texticon.ico grpicon.ico
        $(WRC) -r -q -bt=nt -fowgnuplot.res -i=$(W) &
	   -d_WIN32 -dNDEBUG -dWIN32 -dMSRC $(W)\wgnuplot.rc

term.obj: $(S)\term.c $(CORETERM) .AUTODEPEND
        $(WCC) $(WCFLAGS) $(TERMFLAGS) $[@

winmain.obj: $(W)\winmain.c .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wgnuplib.obj: $(W)\wgnuplib.c .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wmenu.obj: $(W)\wmenu.c .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wtext.obj: $(W)\wtext.c .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wpause.obj: $(W)\wpause.c .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wprinter.obj: $(W)\wprinter.c .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wgraph.obj: $(W)\wgraph.c .AUTODEPEND .AUTODEPEND
        $(WCC) $(WCFLAGS) $[@

wgnuplot.mnu: $(W)\wgnuplot.mnu
        copy $(W)\wgnuplot.mnu wgnuplot.mnu

pgnuplot.exe: $(W)\pgnuplot.c version.obj .AUTODEPEND
        $(WCL) -5r -zq -d2 -fm $(W)\pgnuplot.c -fe=pgnuplot.exe -I$(S) &
	  version.obj user32.lib


# extract icons from wgnuplot.rc
texticon.ico: geticon.exe $(W)\wgnuplot.rc
        geticon $(W)\wgnuplot.rc

grpicon.ico: geticon.exe $(W)\wgnuplot.rc
        geticon $(W)\wgnuplot.rc

geticon.exe: $(W)\geticon.c
        $(WCL) -bcl=nt -fd -fe=geticon.exe $[@

# convert gnuplot.doc to gnuplot.rtf
wgnuplot.hlp: doc2rtf.exe $(D)gnuplot.doc $(W)\wgnuplot.hpj
        doc2rtf $(D)gnuplot.doc $(W)\gnuplot.rtf
        $(HCW) /c /e $(W)\wgnuplot.hpj
        if exist $(W)\wgnuplot.hlp copy $(W)\wgnuplot.hlp .

doc2rtf.exe: $(D)doc2rtf.c $(D)termdoc.c $(D)xref.c .AUTODEPEND
        $(WCL) $(WCFLAGS) -w1 -DWINDOWS_NO_GUI -I$(D);$(T) -fe$^@ $(D)doc2rtf.c $(D)termdoc.c $(D)xref.c

WD=$(D)windows\
HHP_PROJ=$(WD)wgnuplot.hhp
HHP_CONTENT=$(WD)wgnuplot.hhc
HHP_INPUT=$(WD)wgnuplot.html
HHP_OUTPUT=$(WD)wgnuplot.chm   

wgnuplot.chm: $(HHP_OUTPUT)
        copy $(HHP_OUTPUT) .

$(HHP_OUTPUT): $(HHP_PROJ) $(HHP_INPUT) $(HHP_CONTENT)
        -$(HHC) $(HHP_PROJ)

$(HHP_INPUT) $(HHP_CONTENT): doc2html.exe $(D)gnuplot.doc
        doc2html $(D)gnuplot.doc $(HHP_INPUT) $(HHP_CONTENT)

doc2html.exe: $(WD)doc2html.c $(D)termdoc.c $(D)xref.c version.$(O) .AUTODEPEND
        $(WCL) $(WCFLAGS) -w1 -DWINDOWS_NO_GUI -I$(D);$(T) -fe$^@ $(WD)doc2html.c $(D)termdoc.c $(D)xref.c version.$(O)

gnuplot.gih: doc2gih.exe $(D)gnuplot.doc
        doc2gih $(D)gnuplot.doc $^@

doc2gih.exe: $(D)doc2gih.c $(D)termdoc.c $(D)xref.c .AUTODEPEND
        $(WCL) $(WCFLAGS) -w1 -I$(D);$(T) -fe$^@ $(D)doc2gih.c $(D)termdoc.c $(D)xref.c

#make binary demo files
bf_test.exe : bf_test.c dbinary.obj alloc.obj .AUTODEPEND
        $(WCL) $(WCFLAGS) -w1 -DWINDOWS_NO_GUI -fe$^@ $(S)\bf_test.c dbinary.obj alloc.obj

$(BINARYFILES): bf_test.exe
        .\bf_test
        move binary? $(M).

# _Windows causes wtext.h to define fread() etc
dbinary.obj: $(S)\binary.c
        $(WCC) $(WCFLAGS) /U_Windows -w1 -fodbinary.obj $[@

# clean up temporary files
clean: .SYMBOLIC
        rm -f *.obj *.err *.ico
        rm -f wgnuplot.res $(W)\gnuplot.rtf $(W)\wgnuplot.hlp
        rm -f bf_test.exe linkopt1.oww doc2rtf.exe doc2html.exe geticon.exe
	rm -f doc2gih.exe gnuplot.gih
        rm -f *.map *.sym wgnuplot.gid        

realclean: clean .SYMBOLIC
        rm -f wgnuplot.exe wgnuplot.mnu pgnuplot.exe gnuplot.exe wgnuplot_pipes.exe
        rm -f $(HHP_CONTENT) $(HHP_INPUT) $(HHP_OUTPUT) $(WD)wgnuplot.log
        rm -f wgnuplot.hlp wgnuplot.chm wgnuplot.chw
        rm -f $(D)binary[123] $(D)fit.log $(D)soundfit.par
