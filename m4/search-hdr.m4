## --------------------------------------------- ##
## Search several directories for header file.   ##
## Built around a non-caching and silent version ##
## of AC_CHECK_HEADER.                           ##
## From Lars Hecking                             ##
## --------------------------------------------- ##

# serial 1

dnl gp_SEARCH_HEADERDIRS(HEADER-FILE [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
AC_DEFUN(gp_SEARCH_HEADERDIRS,
[AC_REQUIRE([gp_SEARCH_LIBDIRS])
AC_MSG_CHECKING([for $1])
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
gp_save_CPPFLAGS="$CPPFLAGS"
for ac_dir in '' /usr/local/include $gp_lib_prefix $gp_lib_prefix/include ; do
  CPPFLAGS="$gp_save_CPPFLAGS `test x${ac_dir} != x && echo -I${ac_dir}`"
  AC_TRY_CPP([#include <$1>], eval "ac_cv_header_$ac_safe=yes",
    eval "ac_cv_header_$ac_safe=no")
  if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
    break
  else
    CPPFLAGS="$ac_save_CPPFLAGS"
  fi
done
if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi
])

