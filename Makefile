# Makefile for GNUPLOT documentation
#
# troff a copy of gnuplot -ms if you've got a laser printer
# otherwise, just print gnuplot.nroff on a line printer
#
HELPDIR = /usr/local/help/gnuplot

gnuplot.ms: hlp2ms gnuplot.hlp
	./hlp2ms < gnuplot.hlp > gnuplot.ms

helptree: helptree.c
	cc -o helptree helptree.c

hlp2ms: hlp2ms.c
	cc -o hlp2ms hlp2ms.c

clean:
	rm -f gnuplot.ms gnuplot.hold hlp2ms helptree

# Dependencies are hard (for me) so just rebuild everthing out of help tree
# (This assumes help tree is more recent than gnuplot.hlp)

hlp:
	- mv gnuplot.hlp gnuplot.hold
	./helptree -f $(HELPDIR) > gnuplot.hlp
