dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.


# serial 2

AC_DEFUN(AM_PATH_LISPDIR,
 [# If set to t, that means we are running in a shell under Emacs.
  # If you have an Emacs named "t", then use the full path.
  test x"$EMACS" = xt && EMACS=
  AC_CHECK_PROGS(EMACS, emacs xemacs, no)
  if test $EMACS != "no"; then
    if test x${lispdir+set} != xset; then
      AC_CACHE_CHECK([where .elc files should go], [am_cv_lispdir], [dnl
	am_cv_lispdir=`$EMACS -q -batch -eval '(while load-path (princ (concat (car load-path) "\n")) (setq load-path (cdr load-path)))' | sed -n -e 's,/$,,' -e '/emacs\/site-lisp$/{p;q;}'`
	if test -z "$am_cv_lispdir"; then
	  am_cv_lispdir='${datadir}/emacs/site-lisp'
	else
	  am_cv_lispdir=`echo "$am_cv_lispdir" | sed -e 's,^.*/lib/,${libdir}/,' -e 's,^.*/share/,${datadir}/,'`
	fi
      ])
      lispdir="$am_cv_lispdir"
    fi
  fi
  AC_SUBST(lispdir)])

