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
LIBS="$gp_lib_ldaargs $TERMLIBS $TERMXLIBS -l$1 $3 $LIBS"
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
fi
])

## -------------------------------------------- ##
## Search several directories for LIBRARY.      ##
## SEARCH-DIR may be the full path to LIBRARY,  ##
## in case it lives in a non-standard location. ##
## The resulting linker args are stored in      ##
## gp_LIBRARY_libs for use in configure.in.     ##
## OTHER_LIBRARIES are NOT automatically        ##
## added. This must also be done in             ##
## configure.in!                                ##
## From Lars Hecking                            ##
## -------------------------------------------- ##

# serial 3

dnl GP_PATH_LIB(LIBRARY, FUNCTION, SEARCH-DIR [, OTHER-LIBRARIES])
AC_DEFUN(GP_PATH_LIB,
[ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
gp_lib_var=`echo $1['_libs'] | sed 'y%./+-%__p_%'`
changequote(, )dnl
  gp_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
AC_MSG_CHECKING([for $2 in -l$1])
if test "$3" != yes && test "$3" != no; then
  gp_l_path=`echo "$3" | sed -e 's%/lib$1\.a$%%'`
  gp_l_prfx=`echo $gp_l_path | sed -e 's%/lib$%%' -e 's%/include$%%'`
  gp_l_list="$gp_l_prfx $gp_l_prfx/lib $gp_l_path"
else
  gp_l_list=''
fi
for ac_dir in $gp_l_list '' /usr/local/lib ; do
  test x${ac_dir} != x && gp_lib_ldaargs="-L${ac_dir}"
  GP_CHECK_LIB_QUIET($1,$2,$4)
  if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
    break
  fi
done

if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  eval "gp_$gp_lib_var=\"$gp_lib_ldaargs -l$1\""
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
])

