#
# $Id: Makefile,v 1.2 2011/10/30 18:31:01 markisch Exp $
#
# GNUPLOT Makefile Microsoft Visual C++ 2005 or newer

# John Bollinger bbands@yahoo.com, 4 March 2002:
#   Added png support with zlib and libpng as subs of .\src
#     libpng and zlib can be downloaded from ftp://ftp.uu.net/graphics/png
#   Added pdf support with pdflib as a sub of .\src
#     pdflib can be downloaded from http://www.pdflib.com
#   Added MSC multi-thread support for compatibility with pdflib
# Bastian Maerkisch bmaerkisch@web.de:
#   Added support for binary version of the GD library (bgd.dll)
#     gdlib can be downloaded from http://www.libgd.org/
#     Development of libgd seems to have stalled:
#     Latest binary build can be found at http://www.libgd.org/releases/gd-latest-win32.zip
#     As of March 2011 this is version 2.0.34.
#   Removed zlib and libpng again as they're already included in bgd.dll
# Hans-Bernhard Broeker:
#   updated to current version of MSVC
#   undid silly name change for pgnuplot.exe, call console version gnuplot.exe instead
#   removed remainder of wgnuplot_pipes.exe support
#   isolated more configuration-dependent parts (gd, pdf, wx) to lines of their own,
#     so they can be turned on/off more easily.
# Bastian Maerkisch bmaerkisch@web.de:
#   Out of tree version. Moved to config\msvc
#

#
# To compile with Visual C:
#	Optionally download gdlib to config\msvc\gdwin32
#	Optionally download pdflib to config\msvc\pdflib and compile it if you choose the source
#
#	Start the Microsoft Visual C++ command shell (e.g. via link installed setup)
#	change to the gnuplot\config\msvc directory
#   Edit the Makefile to match your setup. (If you don't have the optional libraries,
#   you will probably have to disable some parts.)
#	Now run:
#	  nmake
#

# Compilation options, disable by setting to zero
# Is libgd available? (required by terminal png/jpeg/gif)?
GDLIB = 0
# Is pdflib available? (required by terminal pdf)
PDFLIB = 0
# Build wxt terminal? (required GTK, wxWidgets, cairo and pango libraries)
# See http://wiki.wxwidgets.org/Microsoft_Visual_C%2B%2B_Guide
#     http://wiki.wxwidgets.org/Visual_Studio_2008_%289.0%29
WXT = 0
# Build the lua/tikz terminal?
LUA = 0
# Choose help format:
# for HTML help (.chm) this should be 1, for (.hlp) it should be 0
HTML_HELP = 1

# Location of additional libraries
GDDIR = gdwin32
LUADIR = lua
PDFDIR = pdflib

# installation directory
DESTDIR = "$(PROGRAMFILES)\gnuplot"

TOP = ..\\..

# directory for PostScript prolog and associated files
GNUPLOT_PS_DIR = share\\PostScript

# Similarly for javascript files for the canvas terminal:
GNUPLOT_JS_DIR = share\\js

# Similarly for scripts used by the lua terminal
GNUPLOT_LUA_DIR = share\\lua

#CC = cl
LD = link

# compiler flags
# /I  means extra include directory
# /Fm means produce link map
# /Od means suppress optimizations (esp. for debug)
# /O2 mans optimize for speed
# /Zi means prepare for codeview
# /MD means include multi-thread support (DLL)
CBASEFLAGS = /EHsc /GR /MD /O2 /nologo
CFLAGS = $(CBASEFLAGS) /I. /I$(TOP) /I$(S) /D_Windows /DWIN32 /DHAVE_CONFIG_H /D__MSC__ /DHELPFILE=\"$(HELPFILE)\" $(OPTIONS)
CONSOLE_CFLAGS = /DWGP_CONSOLE /DCONSOLE_SWITCH_CP /DPIPES
OPTIONS = /DUSE_MOUSE
OPTIONS = $(OPTIONS) /DGNUPLOT_HISTORY

# flags for terminals
# see other terminal defines in term.h
TERMFLAGS = /I$(T) /DGNUPLOT_PS_DIR=\"$(GNUPLOT_PS_DIR)\" /DGNUPLOT_JS_DIR=\"$(GNUPLOT_JS_DIR)\"

# linker flags, append paths for external libs later
LDFLAGS = /nologo /MACHINE:$(MACHINE) /MANIFEST
LDFLAGS = $(LDFLAGS) /MANIFESTDEPENDENCY:"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'"
COMMONLIBS = kernel32.lib user32.lib gdi32.lib winspool.lib \
             comdlg32.lib comctl32.lib advapi32.lib shell32.lib \
             msimg32.lib gdiplus.lib
CONSOLELIBS = ole32.lib oleaut32.lib comctl32.lib

!IF "$(GDLIB)" == "1"
OPTIONS = $(OPTIONS) /DHAVE_LIBGD /DHAVE_GD_H /DHAVE_GD_GIF /DGIF_ANIMATION /DHAVE_GD_PNG /DHAVE_GD_JPEG /DHAVE_GD_TTF
OPTIONS = $(OPTIONS) /I$(GDDIR)\include
LDFLAGS = $(LDFLAGS) /libpath:$(GDDIR)\lib
!ENDIF

!IF "$(PDFLIB)" == "1"
OPTIONS = $(OPTIONS) /DHAVE_LIBPDF
LDFLAGS = $(LDFLAGS) /libpath:$(PDFDIR)\pdflib
TERMFLAGS = $(TERMFLAGS) /I$(PDFDIR)\pdflib
!ENDIF

!IF "$(WXT)" == "1"
WXTLIBS = glib-2.0.lib gobject-2.0.lib gmodule-2.0.lib \
	cairo.lib pango-1.0.lib pangocairo-1.0.lib \
	wxmsw28.lib wxtiff.lib
!ENDIF

!IF "$(LUA)" == "1"
OPTIONS = $(OPTIONS) /DHAVE_LUA
OPTIONS = $(OPTIONS) /I$(LUADIR)/include
LDFLAGS = $(LDFLAGS) /libpath:$(LUADIR)
TERMFLAGS = $(TERMFLAGS) /DGNUPLOT_LUA_DIR=\"$(GNUPLOT_LUA_DIR)\"
LUALIBS = lua51.lib
!ENDIF

!IF "$(HTML_HELP)" == "1"
HELPFILE = wgnuplot.chm
OPTIONS = $(OPTIONS) /DWITH_HTML_HELP
HHWPATH=$(PROGRAMFILES)\HTML Help Workshop\\
!ELSE
HELPFILE = wgnuplot.hlp
HCWPATH=$(PROGRAMFILES)\help workshop\\
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
MACHINE = $(PROCESSOR_ARCHITECTURE)
!ELSE
MACHINE = IX86
!ENDIF

# macros for makefile.all

O=obj
S=$(TOP)\\src
W=$(S)\\win\\
T=$(TOP)\\term\\
D=$(TOP)\\docs\\
M=$(TOP)\\demo\\

default: wgnuplot.exe $(HELPFILE) wgnuplot.mnu $(M)bf_test.exe pgnuplot.exe gnuplot.exe

!INCLUDE $(S)\\makefile.all

OBJS = $(COREOBJS) version.obj

WINOBJS = winmain.obj wgnuplib.obj wgraph.obj wprinter.obj wtext.obj \
	screenbuf.obj wpause.obj wmenu.obj gpexecute.obj wgdiplus.obj

ALL_CONSOLE_OBJS = $(OBJS:.obj=.cobj) $(WINOBJS:.obj=.cobj)


# default rules
{$(S)}.c.obj:
	$(CC) /c $(CFLAGS) $(S)\\$*.c

{$(S)}.c.cobj:
	$(CC) /c $(CFLAGS) $(CONSOLE_CFLAGS) $(S)\\$*.c /Fo$@

{$(W)}.c.obj:
	$(CC) /c $(CFLAGS) $(W)$*.c /Fo$@

{$(W)}.c.cobj:
	$(CC) /c $(CFLAGS) $(CONSOLE_CFLAGS) $(W)$*.c /Fo$@

{$(W)}.cpp.obj:
	$(CC) /c $(CFLAGS) $(W)$*.cpp /Fo$@

{$(W)}.cpp.cobj:
	$(CC) /c $(CFLAGS) $(CONSOLE_CFLAGS) $(W)$*.cpp /Fo$@

{$(S)}.c{$(M)}.exe:
	$(CC) /c $(CFLAGS) $(W)$*.c /Fo$@


$(OBJS): config.h

config.h: $(TOP)\config\config.nt
	copy $(TOP)\config\config.nt config.h

wgnuplot.exe: $(OBJS) $(WINOBJS) wgnuplot.res linkopt-w.msw texticon.ico grpicon.ico
	$(LD) /subsystem:windows $(LDFLAGS) /map:wgnuplot.map /out:$@ @linkopt-w.msw
!IF "$(GDLIB)" == "1"
	copy $(GDDIR)\bin\*.dll .
!ENDIF
!IF "$(LUA)" == "1"
	copy $(LUADIR)\*.dll .
!ENDIF

gnuplot.exe: $(ALL_CONSOLE_OBJS) wgnuplot.res linkopt-c.msw texticon.ico grpicon.ico
	$(LD) /subsystem:console $(LDFLAGS) /map:wgnuplot.map /out:$@ @linkopt-c.msw
!IF "$(GDLIB)" == "1"
	copy $(GDDIR)\bin\*.dll .
!ENDIF
!IF "$(LUA)" == "1"
	copy $(LUADIR)\*.dll .
!ENDIF

pgnuplot.exe: $(W)pgnuplot.c
	$(CC) $(CBASEFLAGS) /I. /I$(TOP) /I$(S) $(W)pgnuplot.c /Fe$@ /link version.obj user32.lib

linkopt-w.msw: Makefile
	echo $(OBJS) > linkopt-w.msw
	echo $(WINOBJS) >> linkopt-w.msw
	echo $(COMMONLIBS) >> linkopt-w.msw
	echo wgnuplot.res >> linkopt-w.msw
!IF "$(HTML_HELP)" == "1"
	echo htmlhelp.lib >> linkopt-w.msw
!ENDIF
!IF "$(GDLIB)" == "1"
	echo bgd.lib >> linkopt-w.msw
!ENDIF
!IF "$(PDFLIB)" == "1"
	echo pdflib.lib >> linkopt-w.msw
!ENDIF
!IF "$(WXT)" == "1"
	echo $(WXTLIBS) >> linkopt-w.msw
!ENDIF
!IF "$(LUA)" == "1"
	echo $(LUALIBS) >> linkopt-w.msw
!ENDIF

linkopt-c.msw: Makefile
	echo $(ALL_CONSOLE_OBJS) > linkopt-c.msw
	echo $(COMMONLIBS) >> linkopt-c.msw
	echo $(CONSOLELIBS) >> linkopt-c.msw
	echo wgnuplot.res >> linkopt-c.msw
!IF "$(HTML_HELP)" == "1"
	echo htmlhelp.lib >> linkopt-c.msw
!ENDIF
!IF "$(GDLIB)" == "1"
	echo bgd.lib >> linkopt-c.msw
!ENDIF
!IF "$(PDFLIB)" == "1"
	echo pdflib.lib >> linkopt-c.msw
!ENDIF
!IF "$(WXT)" == "1"
	echo $(WXTLIBS) >> linkopt-c.msw
!ENDIF
!IF "$(LUA)" == "1"
	echo $(LUALIBS) >> linkopt-c.msw
!ENDIF

# rules

term.obj: $(S)\term.c $(S)\term.h $(S)\plot.h $(S)\setshow.h $(S)\bitmap.h $(CORETERM)
	$(CC) /c $(CFLAGS) $(TERMFLAGS) $(S)\$(*B).c /Fo$@

term.cobj: $(S)\term.c $(S)\term.h $(S)\plot.h $(S)\setshow.h $(S)\bitmap.h $(CORETERM)
	$(CC) /c $(CFLAGS) $(CONSOLE_CFLAGS) $(TERMFLAGS) $(S)\$(*B).c /Fo$@

winmain.obj: $(W)winmain.c $(W)wgnuplib.h $(W)wtext.h $(S)\plot.h
	$(CC) /c $(CFLAGS) $(W)$(*B).c /Fo$@

winmain.cobj: $(W)winmain.c $(W)wgnuplib.h $(W)wtext.h $(S)\plot.h
	$(CC) /c $(CFLAGS) $(CONSOLE_CFLAGS) $(W)winmain.c /Fo$@


# resources
wgnuplot.mnu: $(W)wgnuplot.mnu
	copy $(W)wgnuplot.mnu wgnuplot.mnu

wgnuplot.res :  $(W)wgnuplot.rc $(W)wgnuplib.rc $(W)wresourc.h texticon.ico grpicon.ico
	$(RC) /l 0x409 /fowgnuplot.res /I $(W) /d "NDEBUG" /d WIN32 /d MSRC /d NO_MANIFEST $(W)wgnuplot.rc

# extract icons from wgnuplot.rc
texticon.ico: grpicon.ico

grpicon.ico: geticon.exe $(W)wgnuplot.rc
	geticon $(W)wgnuplot.rc

geticon.exe: $(W)geticon.c
	$(CC) /DMSDOS /F 5000 /W1 $(W)geticon.c


# build windows help file
!IF "$(HTML_HELP)" == "1"
$(HELPFILE): doc2html.exe $(D)gnuplot.doc gnuplot.exe
	doc2html $(D)gnuplot.doc $(D)windows\\
	cd $(D)
	-"..\\config\\msvc\\gnuplot" -e "winhelp=1" plotstyles.gnu
	-"$(HHWPATH)hhc" windows\wgnuplot.hhp
	cd ..\\config\\msvc
	if exist $(D)windows\wgnuplot.chm copy $(D)windows\wgnuplot.chm .
!ELSE
# convert gnuplot.doc to gnuplot.rtf
$(HELPFILE): doc2rtf.exe $(D)gnuplot.doc $(W)wgnuplot.hpj
	doc2rtf $(D)gnuplot.doc $(W)gnuplot.rtf
	"$(HCWPATH)hcw" /c /e $(W)wgnuplot.hpj
	if exist $(W)wgnuplot.hlp copy $(W)wgnuplot.hlp .
!ENDIF

doc2html.exe: $(D)windows\doc2html.c $(D)termdoc.c $(D)xref.c version.obj
	$(CC) $(CFLAGS) /F 5000 /W1 /I. /DWINDOWS_NO_GUI /I$(D) /I$(T) -Fedoc2html.exe $(D)windows\doc2html.c $(D)termdoc.c $(D)xref.c version.obj

doc2rtf.exe: $(D)doc2rtf.c $(D)termdoc.c $(D)xref.c
	$(CC) $(CFLAGS) /F 5000 /W1 /I. /DWINDOWS_NO_GUI /I$(D) /I$(T) -Fedoc2rtf.exe $(D)doc2rtf.c $(D)termdoc.c $(D)xref.c


# make binary demo files
$(M)bf_test.exe : $(S)\\bf_test.c dbinary.obj alloc.obj
	$(CC) $(CFLAGS) /F 5000 /W1 /DWINDOWS_NO_GUI /Fe$(M)bf_test.exe $(S)\\bf_test.c dbinary.obj alloc.obj
	cd $(M)
	bf_test
	cd ..\\config\\msvc

# _Windows causes wtext.h to define fread() etc
dbinary.obj: $(S)\\binary.c
	$(CC) /c $(CFLAGS) /U_Windows /F 5000 /W1 /Fodbinary.obj $(S)\\binary.c

# clean up temporary files
clean:
	if exist *.obj del *.obj
	if exist *.cobj del *.cobj
	if exist *.ico del *.ico
	if exist wgnuplot.res del wgnuplot.res
	if exist gnuplot.rtf del gnuplot.rtf
	if exist $(M)bf_test.exe del $(M)bf_test.exe
	if exist linkopt-w.msw del linkopt-w.msw
	if exist linkopt-c.msw del linkopt-c.msw
	if exist doc2html.exe del doc2html.exe
	if exist doc2rtf.exe del doc2rtf.exe
	if exist geticon.exe del geticon.exe

veryclean: clean
	if exist wgnuplot.exe del wgnuplot.exe
	if exist wgnuplot.chm del wgnuplot.chm
	if exist wgnuplot.hlp del wgnuplot.hlp
	if exist wgnuplot.mnu del wgnuplot.mnu
	if exist pgnuplot.exe del pgnuplot.exe
	if exist gnuplot.exe del gnuplot.exe
	if exist $(M)binary1 del $(M)binary1
	if exist $(M)binary2 del $(M)binary2
	if exist $(M)binary3 del $(M)binary3
	if exist $(M)fit.log del $(M)fit.log
	if exist $(M)soundfit.par del $(M)soundfit.par
	if exist config.h del config.h

install:
	if not exist $(DESTDIR) mkdir $(DESTDIR)
	if not exist $(DESTDIR)\bin mkdir $(DESTDIR)\bin
	copy /Y wgnuplot.exe $(DESTDIR)\bin
	copy /Y pgnuplot.exe $(DESTDIR)\bin
	copy /Y gnuplot.exe $(DESTDIR)\bin
	copy /Y *gnuplot.exe.manifest $(DESTDIR)\bin
	copy /Y $(W)wgnuplot.mnu $(DESTDIR)\bin
	copy /Y $(W)wgnuplot-ja.mnu $(DESTDIR)\bin
	if exist wgnuplot.chm copy /Y wgnuplot.chm $(DESTDIR)\bin
	if exist wgnuplot-ja.chm copy /Y wgnuplot-ja.chm $(DESTDIR)\bin
	if exist wgnuplot.hlp copy /Y wgnuplot.hlp $(DESTDIR)\bin
	copy /Y *.dll $(DESTDIR)\bin
	if not exist $(DESTDIR)\$(GNUPLOT_PS_DIR) mkdir $(DESTDIR)\$(GNUPLOT_PS_DIR)
	xcopy /Y $(T)PostScript\*.ps $(DESTDIR)\$(GNUPLOT_PS_DIR)
	if not exist $(DESTDIR)\$(GNUPLOT_JS_DIR) mkdir $(DESTDIR)\$(GNUPLOT_JS_DIR)
	xcopy /Y $(T)js\*.* $(DESTDIR)\$(GNUPLOT_JS_DIR)
	if not exist $(DESTDIR)\$(GNUPLOT_LUA_DIR) mkdir $(DESTDIR)\$(GNUPLOT_LUA_DIR)
	xcopy /Y $(T)lua\*.* $(DESTDIR)\$(GNUPLOT_LUA_DIR)
	if not exist $(DESTDIR)\share\LaTeX mkdir $(DESTDIR)\share\LaTeX
	copy /Y $(TOP)\share\LaTeX\README $(DESTDIR)\share\LaTeX
	copy /Y $(TOP)\share\LaTeX\gnuplot.cfg $(DESTDIR)\share\LaTeX
	for %f in (BUGS README Copyright NEWS ChangeLog) do copy $(TOP)\%f $(DESTDIR)\%f
	if exist $(TOP)\win\README.Windows copy /Y $(TOP)\win\README.Windows $(DESTDIR)
	copy /Y $(TOP)\src\win\README.win-ja $(DESTDIR)
	if not exist $(DESTDIR)\demo mkdir $(DESTDIR)\demo
	xcopy /Y $(TOP)\demo $(DESTDIR)\demo
	if not exist $(DESTDIR)\contrib\pm3d mkdir $(DESTDIR)\contrib\pm3d
	xcopy /Y $(TOP)\pm3d\contrib\*.* $(DESTDIR)\contrib\pm3d
