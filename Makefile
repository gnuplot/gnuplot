#------------------------------------------------------------------
# Makefile for gnut2p
#   A translation program from Gnutex to GNUPLOT
#   David Kotz
#   Duke University
#   dfk@cs.duke.edu
#
# derived from gnutex
#

# Define this for production version
CFLAGS=-O -s
# For the debugging version
#CFLAGS=-g

# Your favorite tags program
TAGS=etags
#TAGS=ctags

OBJS = plot.o scanner.o parse.o command.o eval.o \
	standard.o internal.o util.o misc.o 

SRC = plot.h command.c eval.c internal.c misc.c \
	parse.c plot.c scanner.c standard.c util.c

gnut2p: $(OBJS)
	cc $(CFLAGS) $(OBJS) -o gnut2p -lm

$(OBJS): plot.h

TAGS: $(SRC)
	$(TAGS) $(SRC)

clean:
	rm -f *.o *~ core

spotless:
	rm -f *.o *~ core gnut2p TAGS

