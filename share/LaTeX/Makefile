
INSTALLDIR= `kpsexpand '$$TEXMFLOCAL'`/tex/latex/gnuplot

all: mkdir $(INSTALLDIR)/gnuplot.cfg

mkdir:
	@if [ ! -d $(INSTALLDIR) ]; then mkdir -v $(INSTALLDIR); fi

$(INSTALLDIR)/gnuplot.cfg: gnuplot.cfg
	@if [ ! -f $(INSTALLDIR)/gnuplot.cfg ]; then \
		install -v -m 644 gnuplot.cfg $(INSTALLDIR)/gnuplot.cfg && \
		texhash; \
	fi
