## --------------------------------------------- ##
## Search several directories for header file.   ##
## Built around a non-caching and silent version ##
## of AC_CHECK_HEADER.                           ##
## From Lars Hecking                             ##
## --------------------------------------------- ##

# serial 2

dnl GP_PATH_HEADER(HEADER-FILE, SEARCH-DIRS [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
AC_DEFUN(GP_PATH_HEADER,
[ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
changequote(, )dnl
  ac_tr_hdr=HAVE_`echo $1 | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_header_$ac_safe,
[gp_save_CPPFLAGS="$CPPFLAGS"
if test "$2" != yes && test "$2" != no; then
  gp_h_path=`echo "$2" | sed -e 's%/lib$1\.a$%%'`
  gp_h_prfx=`echo "$gp_h_path" | sed -e 's%/lib$%%' -e 's%/include$%%'`
  gp_h_list="$gp_h_prfx $gp_h_prfx/include $gp_h_path"
else
  gp_h_list=''
fi
for ac_dir in '' $gp_h_list /usr/local/include ; do
  test x${ac_dir} != x && CPPFLAGS="$gp_save_CPPFLAGS -I${ac_dir}"
  AC_TRY_CPP([#include <$1>], eval "ac_cv_header_$ac_safe=${ac_dir}",
    eval "ac_cv_header_$ac_safe=no")
  CPPFLAGS="$gp_save_CPPFLAGS"
  if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" != no"; then
    break
  fi
done
])
if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" != no"; then
  if eval "test \"`echo x'$ac_cv_header_'$ac_safe`\" != x" && eval "test \"`echo x'$ac_cv_header_'$ac_safe`\" != xyes"; then
    eval "CPPFLAGS=\"$gp_save_CPPFLAGS -I`echo '$ac_cv_header_'$ac_safe`\""
  fi
  AC_DEFINE_UNQUOTED($ac_tr_hdr)
  AC_MSG_RESULT(yes)
  ifelse([$3], , :, [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi
])

