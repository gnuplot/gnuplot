## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign

SUBDIRS = config m4 term src docs man demo tutorial share

EXTRA_DIST = BUGS Copyright FAQ.pdf GNUmakefile INSTALL INSTALL.gnu \
Makefile.maint PATCHLEVEL PGPKEYS README RELEASE_NOTES \
VERSION configure.vms win

DISTCLEANFILES = stamp-h

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	@for dir in `find $(distdir) -type d -name CVS -print`; do \
	  rm -rf $$dir; done
	@find $(distdir) -type d -name .deps -print | xargs rm -rf
	@find $(distdir) -type f -name .cvsignore -print | xargs rm -f

gnuplot:
	cd src; make binonly

