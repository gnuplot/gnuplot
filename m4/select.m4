dnl testing new version of FUNC_SELECT
dnl 1.0 Original version by Lars
dnl 1.1 Improved version by Steve
dnl 1.2 Bugfixes by Lars

AC_DEFUN(AC_FUNC_SELECT,
[AC_CHECK_FUNCS(select)
if test "$ac_cv_func_select" = yes; then
  AC_CHECK_HEADERS(unistd.h sys/types.h sys/time.h sys/select.h sys/socket.h)
  AC_MSG_CHECKING([types of arguments for select()])
  AC_CACHE_VAL(ac_cv_type_fd_set_size_t,dnl
   [AC_CACHE_VAL(ac_cv_type_fd_set,dnl
    [AC_CACHE_VAL(ac_cv_type_timeval,dnl
     [for ac_cv_type_fd_set in 'fd_set' 'int'; do
       for ac_cv_type_fd_set_size_t in 'int' 'size_t' 'unsigned long' 'unsigned'; do
        for ac_cv_type_timeval in 'struct timeval' 'const struct timeval'; do
          AC_TRY_COMPILE(dnl
[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif],
[extern select ($ac_cv_type_fd_set_size_t, 
 $ac_cv_type_fd_set *,	$ac_cv_type_fd_set *, $ac_cv_type_fd_set *,
 $ac_cv_type_timeval *);],
[ac_found=yes ; break 3],ac_found=no)
         done
        done
       done
      ])dnl AC_CACHE_VAL
    ])dnl AC_CACHE_VAL
  ])dnl AC_CACHE_VAL
  if test "$ac_found" = no; then
    AC_MSG_ERROR([can't determine argument types])
  fi
  AC_MSG_RESULT([select($ac_cv_type_fd_set_size_t,$ac_cv_type_fd_set *,$ac_cv_type_timeval *)])
  if test "$ac_cv_type_fd_set" = fd_set; then
    AC_DEFINE(SELECT_USES_FD_SET)
  fi
  AC_DEFINE_UNQUOTED(fd_set_size_t, $ac_cv_type_fd_set_size_t)
  AC_DEFINE_UNQUOTED(struct_timeval_t, $ac_cv_type_timeval)
fi
])
