#
# Create ps_symbols.ps
#

top_srcdir = ../..

all: ps_symbols.ps

ps_symbols.ps: ps_symbols.gpi
	@if test -f $(top_srcdir)/gnuplot ; then \
		$(top_srcdir)/gnuplot ps_symbols.gpi ; \
	else \
		gnuplot ps_symbols.gpi ; \
	fi

clean:
	-rm -f ps_symbols.ps

distclean: clean

