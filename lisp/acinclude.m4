## ------------------------
## Emacs LISP file handling
## From Ulrich Drepper
## ------------------------

# serial 1

AC_DEFUN(AM_PATH_LISPDIR,
 [# If set to t, that means we are running in a shell under Emacs.
  # If you have an Emacs named "t", then use the full path.
  test "$EMACS" = t && EMACS=
  AC_PATH_PROGS(EMACS, emacs xemacs, no)
  if test $EMACS != "no"; then
    AC_MSG_CHECKING([where .elc files should go])
    lispdir=`$EMACS -q -batch -eval '(mapcar (lambda (dir) (if (string-match "site-lisp/?$" dir) (progn (princ (replace-match "site-lisp\n" t t dir)) (kill-emacs)))) load-path)'`
    if test ! -d $lispdir; then
      lispdir="\$(datadir)/emacs/site-lisp"
    fi
    AC_MSG_RESULT($lispdir)
  fi
  AC_SUBST(lispdir)])
