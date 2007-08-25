## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr foreign 1.2h

# in the spirit of automake ...
pkglibexecdir = $(libexecdir)/@PACKAGE@/@VERSION_MAJOR@

# Where to send bug reports and comments
EMAIL = http://sourceforge.net/projects/gnuplot

# Default help file location
HELPFILE=@GIHDIR@/gnuplot.gih

# Default X11 driver location
X11_DRIVER_DIR=$(pkglibexecdir)

# Default Postscript prologues location
GNUPLOT_PS_DIR=$(pkgdatadir)/$(VERSION_MAJOR)/PostScript

# There is probably no need to edit anything below this line

GNUPLOT_X11 = @GNUPLOT_X11@
EXTRA_PROGRAMS = gnuplot_x11
bin_PROGRAMS = gnuplot
pkglibexec_PROGRAMS = $(GNUPLOT_X11)
noinst_PROGRAMS = bf_test

XLIBS = @LIBRARIES_FOR_X@
AM_CPPFLAGS = -I../term -I$(top_srcdir)/term -DBINDIR=\"$(bindir)\" -DX11_DRIVER_DIR=\"$(X11_DRIVER_DIR)\" -DGNUPLOT_PS_DIR=\"$(GNUPLOT_PS_DIR)\" -DCONTACT=\"$(EMAIL)\" -DHELPFILE=\"$(HELPFILE)\"

gnuplot_SOURCES = alloc.c alloc.h ansichek.h axis.c axis.h \
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
template.h term_api.h term.c term.h time.c unset.c util.c util.h \
util3d.c util3d.h variable.c variable.h version.c version.h

if INCLUDE_BINARY_C
gnuplot_SOURCES += binary.c
endif

gnuplot_LDADD = $(TERMLIBS) $(TERMXLIBS) $(WX_LIBS)

gnuplot_x11_SOURCES = gplt_x11.c gplt_x11.h gpexecute.c gpexecute.h mousecmn.h

getcolor_x11.o: getcolor.c
	$(COMPILE) -c -DGPLT_X11_MODE -o getcolor_x11.o $(srcdir)/getcolor.c

gnuplot_x11_LDADD = getcolor_x11.o $(XLIBS)

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
gnuplot_SOURCES += wxterminal/gp_cairo.c wxterminal/wxt_gui.cpp
endif

## we don't need COREOBJS, but CORETERM
## for the term.o dependencies
O=o
T=$(top_srcdir)/term/
# begin makefile.all
include makefile.all
# end makefile.all
term$U.o: $(CORETERM)

