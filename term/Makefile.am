## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

EXTRA_DIST = README Makefile.am.in compact.c driver.h impcodes.h \
object.h post.h $(CORETERM)

# List of terminal drivers; must be created
# prior to running automake
##trm-files-begin
CORETERM = aed.trm ai.trm amiga.trm apollo.trm atariaes.trm atarivdi.trm \
be.trm cgi.trm cgm.trm corel.trm debug.trm djsvga.trm dumb.trm dxf.trm \
dxy.trm eepic.trm emf.trm emxvga.trm epslatex.trm epson.trm excl.trm \
fg.trm fig.trm ggi.trm gif.trm gnugraph.trm gpic.trm gpr.trm grass.trm \
hp26.trm hp2648.trm hp500c.trm hpgl.trm hpljii.trm hppj.trm imagen.trm \
iris4d.trm kyo.trm latex.trm linux.trm mac.trm metafont.trm metapost.trm \
mgr.trm mif.trm multitos.trm next.trm openstep.trm pbm.trm pc.trm pm.trm \
png.trm post.trm pslatex.trm pstricks.trm qms.trm regis.trm rgip.trm \
sun.trm t410x.trm table.trm tek.trm texdraw.trm tgif.trm tkcanvas.trm \
tpic.trm unixpc.trm unixplot.trm v384.trm vgagl.trm vws.trm win.trm \
x11.trm xlib.trm 
##trm-files-end

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##trm-files-begin/p' Makefile.am.in > $@t
	echo CORETERM = *.trm | fmt | (tr '\012' @; echo) \
	  |sed 's/@$$/%/;s/@/ \\@/g' | tr @% '\012 ' \
	  >> $@t
	sed -n '/^##trm-files-end/,$$p' Makefile.am.in >> $@t
	chmod a-w $@t
	mv $@t $@


