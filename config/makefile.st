#
# $Id: makefile.st,v 1.6 2002/02/21 12:27:37 lhecking Exp $
#
# GNUPLOT Makefile for GCC-ST 1.x and 2.x
# in a cross development environment (unix) or a native TOS environment
# (see configuration section)
#
# Modified from the BorlandC makefile by Alexander Lehmann
#

# the executable will with both regular AES and MultiAES
# the version for MultiAES is relatively new and is only tested with a beta
# version of MultiAES (AES 4.0) and Mint 0.99, so you'll probably have to 
# expect bugs.

# I was sortof pressed for time when I wrote the window support, so the version
# currently doesn't process redraw messages. All it does is clear the screen.
# I'll try to finish a better version for 3.4

# -------------------- begin of configuration section -----------------------

# CC is name of compiler producing TOS executables
# HOSTCC is the name of the native compiler
# SL is the path separator (/ for unix or UNIXMODE, \ for standard TOS)

# settings for crosscompiling:
CC = gcc
#HOSTCC = gcc
HOSTCC = cc
SL=/

# settings for native compiling:
#CC = gcc
#HOSTCC = $(CC)
#SL=\ 

# -O means optimisation in general
#  o-f-p avoids link-unlink instructions
#  s-r	 optimisation of loops and elimination of iterations variables
#  c-r	 improve register movement
# if you have GCC 2.x (and time), you can choose the -O2 option

# first line for GCC 1.x, second and third for GCC 2.x

#OPTFLAGS = -O -fomit-frame-pointer -fstrength-reduce -fcombine-regs
OPTFLAGS = -O -fomit-frame-pointer -fstrength-reduce
#OPTFLAGS = -O2 -fomit-frame-pointer

# -c means don't link
# -DREADLINE to use the history/line editing capability. If you want this 
#    capability add -DREADLINE to CFLAGS
# -DHAVE_STRNICMP if your library supports strnicmp (GNU- and MiNTLib do)

# NOTE: since readline is the only place where AES events are processed, the
#	new (window) version will no longer work without -DREADLINE

# note that current versions of gcc for Atari may very well have float.h
# if anyone can verify this, please fix it.

CFLAGS = -c -I. -I.. $(OPTFLAGS) -DATARI -DREADLINE -DPIPES -DHAVE_STRNICMP
TERMFLAGS =

# where to place gnuplot.gih helpfile
# if this is an absolute pathname, shel_find will not be used.
HELPFILE=gnuplot.gih

# -------------------- end of configuration section -------------------------

OBJS   = bitmap.o command.o contour.o eval.o graphics.o graph3d.o \
         hidden3d.o util3d.o help.o internal.o misc.o parse.o \
         plot.o readline.o scanner.o set.o show.o specfun.o \
         standard.o stdfn.o term.o util.o version.o binary.o fit.o \
         matrix.o alloc.o datafile.o plot2d.o plot3d.o interpol.o \
         tabulate.o variable.o save.o unset.o time.o dynarray.o history.o

# path prefix for the term subdirectory
TERM=term$(SL)

TERMS = $(TERM)atariaes.trm $(TERM)dumb.trm $(TERM)eepic.trm $(TERM)epson.trm \
	$(TERM)hpgl.trm $(TERM)hpljii.trm $(TERM)hppj.trm $(TERM)latex.trm \
	$(TERM)pbm.trm $(TERM)post.trm $(TERM)texdraw.trm

all: gnuplot.prg $(HELPFILE) demo$(SL)bf_test.tos

gnuplot.prg: $(OBJS)
	$(CC) -o $@ $(OBJS) -lgem -lpml.olb
	fixstk 120k $@
#	strip -k $@		# you might want to add strip here

# to run bivariat demo ca. 100k stack is needed, all others work with ca. 32k

# default rules

%.o: %.c
	$(CC) -o $@ $(CFLAGS) $<

# dependencies

alloc.o:     alloc.c plot.h
bitmap.o:    bitmap.c bitmap.h plot.h
binary.o:    binary.c plot.h
command.o:   command.c plot.h setshow.h help.h
	        $(CC) -o $@ $(CFLAGS) -DHELPFILE=\"$(HELPFILE)\" $<
contour.o:   contour.c plot.h
datafile.o:  datafile.c plot.h fnproto.h binary.h setshow.h
eval.o:      eval.c plot.h
graphics.o:  graphics.c plot.h setshow.h
graph3d.o:   graph3d.c plot.h setshow.h
hidden3d.o:  hidden3d.c plot.h setshow.h
util3d.o:    util3d.c plot.h setshow.h 
fit.o:       fit.c fit.h matrix.h plot.h
interpol.o:  interpol.c plot.h setshow.h
matrix.o:    matrix.c matrix.h fit.h
help.o:      help.c help.h
internal.o:  internal.c plot.h
misc.o:      misc.c plot.h setshow.h help.h
parse.o:     parse.c plot.h
plot.o:      plot.c plot.h fit.h setshow.h fnproto.h
plot2d.o:    plot2d.c plot.h setshow.h fit.h binary.h help.h
plot3d.o:    plot3d.c plot.h setshow.h binary.h help.h
readline.o:  readline.c
save.o:      save.c plot.h setshow.h
scanner.o:   scanner.c plot.h
show.o:      set.c plot.h setshow.h
		$(CC) -o $@ $(CFLAGS) -DHELPFILE=\"$(HELPFILE)\" $<
set.o:       show.c plot.h setshow.h
specfun.o:   plot.h specfun.c
standard.o:  standard.c plot.h
stdfn.o:     stdfn.c sstdfn.h
term.o:      term.c term.h plot.h set.c show.c bitmap.h $(TERMS)
	        $(CC) -o $@ $(CFLAGS) $(TERMFLAGS) -Iterm term.c
unset.o:     unset.c plot.h setshow.h tables.h
util.o:      util.c plot.h
variable.o:  variable.c plot.h variable.h
version.o:   version.c

# convert gnuplot.doc to gnuplot.gih
$(HELPFILE): doc2gih docs$(SL)gnuplot.doc
	doc2gih docs$(SL)gnuplot.doc $(HELPFILE)

doc2gih: docs$(SL)doc2gih.c docs$(SL)termdoc.c
	$(HOSTCC) -I. -I.. -o doc2gih docs$(SL)doc2gih.c docs$(SL)termdoc.c 

demo$(SL)bf_test.tos: bf_test.o binary.o alloc.o
	$(CC) -s -o $@ bf_test.o binary.o alloc.o -lpml

clean:			# remove temporary files, but leave executable intact
	rm -f $(OBJS) bf_test.o doc2gih

realclean: clean	# remove all files generated by make
	rm -f gnuplot.prg gnuplot.gih demo$(SL)binary1 demo$(SL)binary2
	rm -f demo$(SL)binary3 demo$(SL)bf_test.tos

# empty rules for file extensions (some makes might need this)
%.trm:

%.doc:

