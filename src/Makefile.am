## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

EXTRA_PROGRAMS = gnuplot_x11

bin_PROGRAMS = gnuplot @GNUPLOT_X11@ bf_test

TERMLIBS = @TERMLIBS@
TERMXLIBS = @TERMXLIBS@
XLIBS = @LIBRARIES_FOR_X@
HELPFILE=$(datadir)/gnuplot.gih
# Where to send email about bugs and comments (locally)
EMAIL = bug-gnuplot@dartmouth.edu

gnuplot_SOURCES = alloc.c alloc.h ansichek.h binary.c binary.h bitmap.c \
bitmap.h command.c contour.c datafile.c eval.c fit.c fit.h fnproto.h \
graph3d.c graphics.c help.c help.h hidden3d.c internal.c interpol.c matrix.c \
matrix.h misc.c national.h parse.c plot.c plot.h plot2d.c plot3d.c protos.h \
readline.c scanner.c set.c setshow.h show.c specfun.c standard.c stdfn.c \
stdfn.h syscfg.h term.c term.h time.c util.c util3d.c variable.c variable.h \
version.c

EXTRA_gnuplot_SOURCES = @NEXTOBJS@

gnuplot_LDADD = $(TERMLIBS) $(TERMXLIBS)

gnuplot_x11_SOURCES = gplt_x11.c stdfn.c

gnuplot_x11_LDADD = $(XLIBS)

bf_test_SOURCES = bf_test.c binary.c alloc.c

if BUILD_SRC_BEOS_SUBDIR
d_beos = beos
endif

SUBDIRS = $(d_beos)

OTHER_SOURCES = amiga.c corplot.c ctrl87.c ctrl87.h os9.c strftime.c vms.c

EXTRA_DIST = GNUmakefile Makefile.maint NeXT corgraph.asm genopt.com \
gnuplot.def gnuplot.opt gnuplot.prj header.mac hrcgraph.asm intergra.x11 \
lasergnu lineproc.mac linkopt.vms linkopt.ztc makefile.all os2 pcgraph.asm \
rgipgnu win $(OTHER_SOURCES)

# explicit targets
command.o: command.c plot.h ansichek.h stdfn.h syscfg.h protos.h alloc.h \
	   setshow.h variable.h fit.h binary.h help.h
	$(COMPILE) -DHELPFILE=\"$(HELPFILE)\" -c $<

show.o: show.c plot.h ansichek.h stdfn.h syscfg.h protos.h alloc.h setshow.h \
	variable.h
	$(COMPILE) -DHELPFILE=\"$(HELPFILE)\" -DBINDIR=\"$(bindir)\" -c $<

term.o: term.c plot.h ansichek.h stdfn.h syscfg.h protos.h alloc.h bitmap.h \
	setshow.h variable.h ../term/driver.h term.h $(CORETERM)
	$(COMPILE) -I../term -I$(top_srcdir)/term -c $<

version.o: version.c plot.h ansichek.h stdfn.h syscfg.h protos.h alloc.h
	$(COMPILE) -DCONTACT=\"$(EMAIL)\" -c $<

