## ------------------------------ ##
## Like AC_CHECK_LIB, but quiet,  ##
## and no caching.                ##
## From Lars Hecking              ##
## ------------------------------ ##

# serial 1

AC_DEFUN(GP_CHECK_LIB_QUIET,
[ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
ac_save_LIBS="$LIBS"
LIBS="$TERMLIBS $TERMXLIBS -l$1 $5 $LIBS"
AC_TRY_LINK(dnl
ifelse([$2], [main], , dnl Avoid conflicting decl of main.
[/* Override any gcc2 internal prototype to avoid an error.  */
]ifelse(AC_LANG, CPLUSPLUS, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char $2();
]),
            [$2()],
            eval "ac_cv_lib_$ac_lib_var=yes",
            eval "ac_cv_lib_$ac_lib_var=no")
LIBS="$ac_save_LIBS"
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  ifelse([$3], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  LIBS="$LIBS -l$1"
], [$3])
else
  ifelse([$4], , , [$4
])dnl
fi
])

## ------------------------------------------- ##
## Search several directories for library.     ##
## NOTE: OTHER_LIBRARIES are NOT automatically ##
## added to TERMLIBS. This must be done in     ##
## configure.in!                               ##
## From Lars Hecking                           ##
## ------------------------------------------- ##

# serial 1

dnl GP_SEARCH_LIBDIRS(LIBRARY, FUNCTION [, OTHER-LIBRARIES])
AC_DEFUN(GP_SEARCH_LIBDIRS,
[AC_MSG_CHECKING([for $2 in -l$1])
gp_save_TERMLIBS="$TERMLIBS"
changequote(, )dnl
  gp_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
dnl The "no" case is just a safety net
case "$with_$1" in
  yes|no)
    gp_lib_list="";;
  *)
    gp_lib_path=`echo $with_$1 | sed -e 's%/lib$1\.a$%%'`
    gp_lib_prefix=`echo $gp_lib_path | sed 's%/lib$%%'`
    gp_lib_list="$gp_lib_prefix $gp_lib_prefix/lib $gp_lib_path"
esac
for ac_dir in '' /usr/local/lib $gp_lib_list ; do
  test x${ac_dir} != x && TERMLIBS="-L${ac_dir} $gp_save_TERMLIBS"
  GP_CHECK_LIB_QUIET($1,$2,dnl
    TERMLIBS="$TERMLIBS -l$1"; break, dnl ACTION-IF-FOUND
    TERMLIBS="$gp_save_TERMLIBS",     dnl ACTION-IF-NOT-FOUND
    $3)                               dnl OTHER-LIBRARIES
done
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
])

