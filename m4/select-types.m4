## -------------------------------------- ##
## Check the type of arguments accepted   ##
## by select. Based on the code in perl's ##
## Configure script by Andy Dougherty.    ##
## From Lars Hecking                      ##
## -------------------------------------- ##

# serial 1

AC_DEFUN(gp_FIND_SELECT_ARGTYPES,
[AC_MSG_CHECKING([types of arguments accepted by select])
 AC_CACHE_VAL(ac_cv_type_select_arg234,dnl
 [AC_CACHE_VAL(ac_cv_type_select_arg1,dnl
  [AC_CACHE_VAL(ac_cv_type_select_arg5,dnl
   [for ac_cv_type_select_arg234 in 'fd_set *' 'int *' 'void *'; do
     for ac_cv_type_select_arg1 in 'int' 'size_t' 'unsigned long' 'unsigned'; do
      for ac_cv_type_select_arg5 in 'struct timeval *' 'const struct timeval *'; do
       AC_TRY_COMPILE(dnl
[#ifndef NO_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifndef NO_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
extern select ($ac_cv_type_select_arg1,$ac_cv_type_select_arg234,$ac_cv_type_select_arg234,$ac_cv_type_select_arg234,$ac_cv_type_select_arg5);],,dnl
        [ac_not_found=no ; break 3],ac_not_found=yes)
      done
     done
    done
   ])dnl AC_CACHE_VAL
  ])dnl AC_CACHE_VAL
 ])dnl AC_CACHE_VAL
 if test "$ac_not_found" = yes; then
  ac_cv_type_select_arg1=int 
  ac_cv_type_select_arg234='int *' 
  ac_cv_type_select_arg5='struct timeval *'
 fi
 AC_MSG_RESULT([$ac_cv_type_select_arg1,$ac_cv_type_select_arg234,$ac_cv_type_select_arg5])
 AC_DEFINE_UNQUOTED(SELECT_ARGTYPE_1,$ac_cv_type_select_arg1)
 AC_DEFINE_UNQUOTED(SELECT_ARGTYPE_234,($ac_cv_type_select_arg234))
 AC_DEFINE_UNQUOTED(SELECT_ARGTYPE_5,($ac_cv_type_select_arg5))
])

