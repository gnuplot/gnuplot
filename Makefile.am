## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign 1.2h

if BUILD_LISP_SUBDIR
d_lisp = lisp
endif

SUBDIRS = config demo m4 term src docs man tutorial $(d_lisp)

EXTRA_DIST = BUGS FAQ GNUmakefile INSTALL PORTING README CodeStyle Copyright \
INSTALL.gnu Makefile.maint PGPKEYS README.1ST djconfig.sh

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	@for dir in `find $(distdir) -type d -name CVS -print`; do \
	  rm -rf $$dir; done
