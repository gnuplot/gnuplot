## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr foreign 1.2h

# Where to send email about bugs and comments (locally)
EMAIL = bug-gnuplot@dartmouth.edu

# Default help file location
HELPFILE=$(datadir)/gnuplot.gih

# There is probably no need to edit anything below this line

GNUPLOT_X11 = @GNUPLOT_X11@
EXTRA_PROGRAMS = gnuplot_x11
bin_PROGRAMS = gnuplot $(GNUPLOT_X11)
noinst_PROGRAMS = bf_test

XLIBS = @LIBRARIES_FOR_X@
AM_CPPFLAGS = -I../term -I$(top_srcdir)/term -DBINDIR=\"$(bindir)\" -DCONTACT=\"$(EMAIL)\" -DHELPFILE=\"$(HELPFILE)\"

gnuplot_SOURCES = alloc.c alloc.h ansichek.h binary.c binary.h bitmap.c \
bitmap.h color.c color.h command.c command.h contour.c contour.h datafile.c \
datafile.h dynarray.c dynarray.h eval.c eval.h fit.c fit.h getcolor.c \
gp_hist.h gp_time.h graph3d.c graph3d.h graphics.c graphics.h help.c help.h \
hidden3d.c hidden3d.h history.c internal.c internal.h interpol.c interpol.h \
ipc.h matrix.c matrix.h misc.c misc.h mouse.c mouse.h mousecmn.h national.h \
parse.c parse.h plot.c plot.h plot2d.c plot2d.h plot3d.c plot3d.h pm3d.c \
pm3d.h protos.h readline.c readline.h save.c save.h scanner.c scanner.h \
set.c setshow.h show.c specfun.c specfun.h standard.c standard.h stdfn.c \
stdfn.h syscfg.h tables.c tables.h template.h term_api.h term.c term.h \
time.c unset.c util.c util.h util3d.c util3d.h variable.c variable.h version.c

gnuplot_LDADD = $(TERMLIBS) $(TERMXLIBS)

gnuplot_x11_SOURCES = gplt_x11.c getcolor.c

gnuplot_x11_LDADD = $(XLIBS)

if ALT_X11_DRIVER
$(GNUPLOT_X11): gnuplot_x11
	cp gnuplot_x11 $(GNUPLOT_X11)
endif

bf_test_SOURCES = bf_test.c binary.c alloc.c

if BUILD_SRC_BEOS_SUBDIR
d_beos = beos
endif

SUBDIRS = $(d_beos)

OTHER_SOURCES = amiga.c corplot.c os9.c strftime.c vms.c

EXTRA_DIST = GNUmakefile Makefile.maint NeXT OpenStep corgraph.asm genopt.com \
gnuplot.def gnuplot.opt gnuplot.prj gpexecute.inc header.mac hrcgraph.asm \
intergra.x11 lasergnu lineproc.mac linkopt.vms linkopt.ztc makefile.all os2 \
pcgraph.asm rgipgnu win x11.opt $(OTHER_SOURCES)

## we don't need COREOBJS, but CORETERM
## for the term.o dependencies
O=o
T=$(top_srcdir)/term/
# begin makefile.all
include makefile.all
# end makefile.all
term$U.o: $(CORETERM)
