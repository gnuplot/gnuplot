dnl testing new version of FUNC_SELECT

AC_DEFUN(AC_FUNC_SELECT,
[AC_CHECK_FUNCS(select)
if test "$ac_cv_func_select" = yes; then
  AC_CHECK_HEADERS(unistd.h sys/types.h sys/time.h sys/select.h sys/socket.h)
  AC_MSG_CHECKING([argument types of select()])
  AC_CACHE_VAL(ac_cv_type_fd_set_size_t,dnl
    [AC_CACHE_VAL(ac_cv_type_fd_set,dnl
      [for ac_cv_type_fd_set in 'fd_set' 'int' 'void'; do
        for ac_cv_type_fd_set_size_t in 'int' 'size_t' 'unsigned long' 'unsigned'; do
	  for ac_type_timeval in 'struct timeval' 'const struct timeval'; do
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
 $ac_type_timeval *);],
[ac_found=yes ; break 3],ac_found=no)
          done
        done
      done
    ])dnl AC_CACHE_VAL
  ])dnl AC_CACHE_VAL
  if test "$ac_found" = no; then
    AC_MSG_ERROR([can't determine argument types])
  fi

  AC_MSG_RESULT([select($ac_cv_type_fd_set_size_t,$ac_cv_type_fd_set *,...)])
  AC_DEFINE_UNQUOTED(fd_set_size_t, $ac_cv_type_fd_set_size_t,
                     [ First arg for select(). ])
  ac_cast=
  if test "$ac_cv_type_fd_set" != fd_set; then
    # Arguments 2-4 are not fd_set.  Some weirdo systems use fd_set type for
    # FD_SET macros, but insist that you cast the argument to select.  I don't
    # understand why that might be, but it means we cannot define fd_set.
    AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])fd_set[^a-zA-Z_0-9]>>dnl
changequote([,]),dnl
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
#endif],dnl
    # We found fd_set type in a header, need special cast
    ac_cast="($ac_cv_type_fd_set *)",dnl
    # No fd_set type; it is safe to define it
    AC_DEFINE_UNQUOTED(fd_set,$ac_cv_type_fd_set,
                       [ Define if the type in arguments 2-4 to select() is fd_set. ]))
  fi
  AC_DEFINE_UNQUOTED(SELECT_FD_SET_CAST,$ac_cast,
                     [ Define if the type in arguments 2-4 to select() is fd_set. ])
fi
])

