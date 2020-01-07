## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign

SUBDIRS = config m4 term src docs man demo share

EXTRA_DIST = BUGS Copyright FAQ.pdf INSTALL INSTALL.gnu \
PATCHLEVEL PGPKEYS README RELEASE_NOTES \
configure.vms win

DISTCLEANFILES = stamp-h

ACLOCAL_AMFLAGS = -I m4

distclean-local:
	rm -rf autom4te.cache

dist-hook:
	@for dir in `find $(distdir) -type d -name CVS -print`; do \
	  rm -rf $$dir; done
	@find $(distdir) -type d -name .deps -print | xargs rm -rf
	@find $(distdir) -type f -name .cvsignore -print | xargs rm -f

ReleaseNotes.html: RELEASE_NOTES
	txt2html -pm --style_url ReleaseNotes.css RELEASE_NOTES \
	         --outfile ReleaseNotes.html

gnuplot:
	cd src; make binonly

