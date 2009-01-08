## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr foreign 1.2h

# in the spirit of automake ...
pkglibexecdir = $(libexecdir)/@PACKAGE@/@VERSION_MAJOR@

# Where to send email about bugs and comments (locally)
EMAIL = gnuplot-bugs@lists.sourceforge.net

# Default help file location
HELPFILE=@GIHDIR@/gnuplot.gih

# Default X11 driver location
X11_DRIVER_DIR=$(pkglibexecdir)

# Default lua driver location
LUA_DRIVER_DIR=$(pkglibexecdir)

# Default javascript location
GNUPLOT_JS_DIR=$(pkgdatadir)/$(VERSION_MAJOR)/js

# Default Postscript prologues location
GNUPLOT_PS_DIR=$(pkgdatadir)/$(VERSION_MAJOR)/PostScript

# There is probably no need to edit anything below this line

bin_PROGRAMS = gnuplot
noinst_PROGRAMS = bf_test

transform = @program_transform_name@
GNUPLOT_X11 = `echo gnuplot_x11 | sed '$(transform)'`$(EXEEXT)

AM_CPPFLAGS = -I../term -I$(top_srcdir)/term -DBINDIR=\"$(bindir)\" -DX11_DRIVER_DIR=\"$(X11_DRIVER_DIR)\" -DGNUPLOT_PS_DIR=\"$(GNUPLOT_PS_DIR)\" -DGNUPLOT_JS_DIR=\"$(GNUPLOT_JS_DIR)\"  -DCONTACT=\"$(EMAIL)\" -DHELPFILE=\"$(HELPFILE)\" -DGNUPLOT_X11=\"$(GNUPLOT_X11)\"

gnuplot_SOURCES = alloc.c alloc.h ansichek.h axis.c axis.h binary.c \
breaders.c breaders.h bitmap.c bitmap.h color.c color.h command.c \
command.h contour.c contour.h datafile.c datafile.h dynarray.c dynarray.h \
eval.c eval.h fit.c fit.h gadgets.c gadgets.h getcolor.c getcolor.h gp_hist.h \
gp_time.h gp_types.h gplt_x11.h graph3d.c graph3d.h graphics.c graphics.h \
help.c help.h hidden3d.c hidden3d.h history.c internal.c internal.h \
interpol.c interpol.h matrix.c matrix.h misc.c misc.h mouse.c mouse.h \
mousecmn.h national.h parse.c parse.h plot.c plot.h plot2d.c plot2d.h \
plot3d.c plot3d.h pm3d.c pm3d.h readline.c readline.h save.c \
save.h scanner.c scanner.h set.c setshow.h show.c specfun.c specfun.h \
standard.c standard.h stdfn.c stdfn.h syscfg.h tables.c tables.h \
tabulate.c tabulate.h \
template.h term_api.h term.c term.h time.c unset.c util.c util.h \
util3d.c util3d.h variable.c variable.h version.c version.h

gnuplot_LDADD = $(TERMLIBS) $(TERMXLIBS) $(WX_LIBS)

pkglibexec_PROGRAMS = 

if BUILD_GNUPLOT_X11
pkglibexec_PROGRAMS += gnuplot_x11
gnuplot_x11_SOURCES = gplt_x11.c gplt_x11.h gpexecute.c gpexecute.h mousecmn.h
XLIBS = @LIBRARIES_FOR_X@
gnuplot_x11_LDADD = getcolor_x11.o $(XLIBS)
endif

if BUILD_LUA
pkglibexec_PROGRAMS += gnuplot.lua
gnuplot_lua_SOURCES =
gnuplot.lua: $(top_srcdir)/term/lua/gnuplot.lua
	cp $(top_srcdir)/term/lua/gnuplot.lua .
endif

getcolor_x11.o: getcolor.c
	$(COMPILE) -c -DGPLT_X11_MODE -o getcolor_x11.o $(srcdir)/getcolor.c

bf_test_SOURCES = bf_test.c binary.c binary.h alloc.c

if BUILD_SRC_BEOS_SUBDIR
d_beos = beos
endif

SUBDIRS = $(d_beos) wxterminal

EXTRA_gnuplot_SOURCES = amiga.c corplot.c os9.c strftime.c vms.c

EXTRA_DIST = GNUmakefile Makefile.maint NeXT OpenStep README \
corgraph.asm genopt.com gnuplot.def gnuplot.opt gnuplot.prj header.mac \
hrcgraph.asm intergra.x11 lasergnu lineproc.mac linkopt.vms linkopt.ztc \
makefile.all makefile.awc os2 pcgraph.asm rgipgnu win x11.opt \
$(EXTRA_gnuplot_SOURCES)

if BUILD_WXWIDGETS
gnuplot_SOURCES += wxterminal/wxt_gui.cpp
endif

if BUILD_GPCAIRO
gnuplot_SOURCES += wxterminal/gp_cairo.c
endif

## we don't need COREOBJS, but CORETERM
## for the term.o dependencies
O=o
T=$(top_srcdir)/term/
# begin makefile.all
include makefile.all
# end makefile.all
term$U.o: $(CORETERM)

