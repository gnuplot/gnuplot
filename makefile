# Makefile for GNUPLOT documentation
#
# troff a copy of gnuplot -ms if you've got a laser printer
# otherwise, just print gnuplot.nroff on a line printer
#
HELPDIR = /src/local/gnuplot/help
# HELPDIR = /usr/help/gnuplot
HELPFILES = $(HELPDIR)/*.hlp\
            $(HELPDIR)/expressions/*.hlp\
            $(HELPDIR)/expressions/functions/*.hlp\
            $(HELPDIR)/expressions/operators/*.hlp\
            $(HELPDIR)/plot/*.hlp\
            $(HELPDIR)/set-show/*.hlp

gnuplot.nroff: titlepage intro gnuplot.ms
	nroff -ms -Tlpr < gnuplot.ms > gnuplot.nroff

gnuplot.ms: hlp2ms gnuplot.hlp
	./hlp2ms < gnuplot.hlp > gnuplot.ms

gnuplot.hlp: $(HELPFILES)
	./vmshelp.csh $(HELPDIR)/* > gnuplot.hlp

hlp2ms: hlp2ms.c
	cc -o hlp2ms hlp2ms.c

clean:
	rm -f gnuplot.nroff gnuplot.ms gnuplot.hlp hlp2ms
