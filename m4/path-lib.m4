## ------------------------------ ##
## Like AC_CHECK_LIB, but quiet,  ##
## and no caching.                ##
## From Lars Hecking              ##
## ------------------------------ ##

# serial 2

dnl AC_CHECK_LIB(LIBRARY, FUNCTION [, OTHER-LIBRARIES])
AC_DEFUN(GP_CHECK_LIB_QUIET,
[ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
ac_save_LIBS="$LIBS"
LIBS="$TERMLIBS $TERMXLIBS -l$1 $3 $LIBS"
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
changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
dnl  LIBS="$LIBS -l$1"
fi
])

## ------------------------------------------- ##
## Search several directories for library.     ##
## NOTE: OTHER_LIBRARIES are NOT automatically ##
## added to TERMLIBS. This must be done in     ##
## configure.in!                               ##
## From Lars Hecking                           ##
## ------------------------------------------- ##

# serial 2

dnl GP_PATH_LIB(LIBRARY, FUNCTION, SEARCH-DIRS [, OTHER-LIBRARIES])
AC_DEFUN(GP_PATH_LIB,
[ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
changequote(, )dnl
  gp_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
AC_MSG_CHECKING([for $2 in -l$1])
gp_save_TERMLIBS="$TERMLIBS"
if test "$3" != yes && test "$3" != no; then
  gp_l_path=`echo "$3" | sed -e 's%/lib$1\.a$%%'`
  gp_l_prfx=`echo $gp_l_path | sed -e 's%/lib$%%' -e 's%/include$%%'`
  gp_l_list="$gp_l_prfx $gp_l_prfx/lib $gp_l_path"
else
  gp_l_list=''
fi
for ac_dir in $gp_l_list '' /usr/local/lib ; do
  test x${ac_dir} != x && TERMLIBS="-L${ac_dir} $gp_save_TERMLIBS"
  GP_CHECK_LIB_QUIET($1,$2,$4)
  if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
    break
  fi
done

if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  TERMLIBS="$TERMLIBS -l$1"
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  TERMLIBS="$gp_save_TERMLIBS"
fi
])

