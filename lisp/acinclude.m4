## ------------------------
## Emacs LISP file handling
## From Ulrich Drepper
## Modified to get default
## prefix from EMACS by
## Lars Hecking
## ------------------------

# serial 1

AC_DEFUN(AM_PATH_LISPDIR,
 [# If set to t, that means we are running in a shell under Emacs.
  # If you have an Emacs named "t", then use the full path.
  test "$EMACS" = t && EMACS=
  AC_PATH_PROGS(EMACS, emacs xemacs, no)
  if test $EMACS != "no"; then
    AC_MSG_CHECKING([where .elc files should go])
    dnl Set default value
    lispdir="\$(datadir)/emacs/site-lisp"
    emacs_flavor=`echo "$EMACS" | sed -e 's,^.*/,,'`
    emacs_prefix=`echo "$EMACS" | sed -e 's,/bin/.*$,,'`
    if test -d $emacs_prefix/share/$emacs_flavor/site-lisp; then
      lispdir="$emacs_prefix/share/$emacs_flavor/site-lisp"
    else
      if test -d $emacs_prefix/lib/$emacs_flavor/site-lisp; then
	lispdir="$emacs_prefix/lib/$emacs_flavor/site-lisp"
      else
	if test "x$prefix" = "xNONE"; then
	  if test -d $ac_default_prefix/share/$emacs_flavor/site-lisp; then
	    lispdir="\$(prefix)/share/$emacs_flavor/site-lisp"
	  elif test -d $ac_default_prefix/lib/$emacs_flavor/site-lisp; then
	    lispdir="\$(prefix)/lib/$emacs_flavor/site-lisp"
	  fi
	else
	  if test -d $prefix/share/$emacs_flavor/site-lisp; then
	    lispdir="\$(prefix)/share/$emacs_flavor/site-lisp"
	  elif test -d $prefix/lib/$emacs_flavor/site-lisp; then
	    lispdir="\$(prefix)/lib/$emacs_flavor/site-lisp"
	  fi
	fi
      fi
    fi
    AC_MSG_RESULT($lispdir)
  fi
  AC_SUBST(lispdir)])
