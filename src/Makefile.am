## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr foreign 1.2h

# Where to send email about bugs and comments (locally)
EMAIL = bug-gnuplot@dartmouth.edu

# Help file location
HELPFILE=$(datadir)/gnuplot.gih

EXTRA_PROGRAMS = gnuplot_x11
bin_PROGRAMS = gnuplot @GNUPLOT_X11@ bf_test gpltconv

# There is probably no need to edit anything below this line
XLIBS = @LIBRARIES_FOR_X@
AM_CPPFLAGS = -I../term -I$(top_srcdir)/term -DBINDIR=\"$(bindir)\" -DCONTACT=\"$(EMAIL)\" -DHELPFILE=\"$(HELPFILE)\"

gnuplot_SOURCES = alloc.c alloc.h ansichek.h binary.c binary.h bitmap.c \
bitmap.h command.c command.h contour.c datafile.c datafile.h eval.c fit.c \
fit.h graph3d.c graphics.c graphics.h help.c help.h hidden3d.c internal.c \
interpol.c matrix.c matrix.h misc.c national.h parse.c parse.h plot.c plot.h \
plot2d.c plot3d.c protos.h readline.c save.c scanner.c set.c setshow.h show.c \
specfun.c standard.c stdfn.c stdfn.h syscfg.h tables.c tables.h term.c term.h \
time.c unset.c util.c util.h util3d.c util3d.h variable.c variable.h version.c

EXTRA_gnuplot_SOURCES = @NEXTOBJS@

gnuplot_LDADD = $(TERMLIBS) $(TERMXLIBS)

gnuplot_x11_SOURCES = gplt_x11.c

gnuplot_x11_LDADD = $(XLIBS)

bf_test_SOURCES = bf_test.c binary.c alloc.c

gpltconv_SOURCES = gpltconv.c

if BUILD_SRC_BEOS_SUBDIR
d_beos = beos
endif

SUBDIRS = $(d_beos)

OTHER_SOURCES = amiga.c corplot.c os9.c strftime.c vms.c

EXTRA_DIST = GNUmakefile Makefile.maint NeXT OpenStep corgraph.asm genopt.com \
gnuplot.def gnuplot.opt gnuplot.prj header.mac hrcgraph.asm intergra.x11 \
lasergnu lineproc.mac linkopt.vms linkopt.ztc makefile.all os2 pcgraph.asm \
rgipgnu win x11.opt $(OTHER_SOURCES)

## we don't need COREOBJS, but CORETERM
## for the term.o dependencies
O=o
T=$(top_srcdir)/term/
# begin makefile.all
include makefile.all
# end makefile.all

