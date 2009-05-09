## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

SUBDIRS = config m4 term src docs $(LISPDIR) man demo tutorial share

EXTRA_DIST = BUGS CodeStyle Copyright FAQ GNUmakefile INSTALL INSTALL.gnu \
Makefile.maint PATCHLEVEL PGPKEYS PORTING README README.1ST \
VERSION configure.vms pm3d

DISTCLEANFILES = stamp-h

LISP_DISTCLEANFILES = gpelcard.dvi gpelcard.pdf gpelcard.ps

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	@for dir in `find $(distdir) -type d -name CVS -print`; do \
	  rm -rf $$dir; done
	@find $(distdir) -type d -name .deps -print | xargs rm -rf
	@find $(distdir) -type f -name .cvsignore -print | xargs rm -f
	@(cd $(distdir)/lisp && rm -f $(LISP_DISTCLEANFILES))

gnuplot:
	cd src; make binonly

