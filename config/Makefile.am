## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

# empty
all:

##m4-files-begin
EXTRA_DIST = README Makefile.am.in buildvms.com config.amg config.cyg \
config.mgw config.nt config.os2 djconfig.sh make_vms.com term_pc.h \
makefile.286 makefile.amg makefile.ami makefile.cyg makefile.dj2 \
makefile.emx makefile.g makefile.mgw makefile.msc makefile.msw \
makefile.mto makefile.nt makefile.os2 makefile.os9 makefile.st makefile.tc \
makefile.unx makefile.vms makefile.wc makefile.win makefile.ztc 
##m4-files-end

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##m4-files-begin/p' Makefile.am.in > $@t
	echo EXTRA_DIST = README Makefile.am.in buildvms.com config.* \
	  djconfig.sh make_vms.com term_pc.h makefile.* | fmt | \
	  (tr '\012' @; echo) |sed 's/@$$/%/;s/@/ \\@/g' |tr @% '\012 ' >> $@t
	sed -n '/^##m4-files-end/,$$p' $< >> $@t
	chmod a-w $@t
	mv $@t $@

