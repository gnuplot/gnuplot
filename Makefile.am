## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

if BUILD_LISP_SUBDIR
d_lisp = lisp
endif

SUBDIRS = config m4 term src docs man demo tutorial $(d_lisp)

EXTRA_DIST = BUGS CodeStyle Copyright FAQ GNUmakefile INSTALL INSTALL.gnu \
Makefile.maint PATCHLEVEL PGPKEYS PORTING README README.1ST README.exp VERSION

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	@for dir in `find $(distdir) -type d -name CVS -print`; do \
	  rm -rf $$dir; done
