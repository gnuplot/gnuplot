## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

##m4-files-begin
EXTRA_DIST = README Makefile.am.in apple.m4 beos.m4 msdos.m4 next.m4 \
path-hdr.m4 path-lib.m4 select.m4 
##m4-files-end

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##m4-files-begin/p' $< > $@t
	echo EXTRA_DIST = README Makefile.am.in *.m4 | fmt | \
	  (tr '\012' @; echo ) \
	  |sed 's/@$$/%/;s/@/ \\@/g' |tr @% '\012 ' \
	  >> $@t
	sed -n '/^##m4-files-end/,$$p' $< >> $@t
	chmod a-w $@t
	mv $@t $@

