dnl $Id: acinclude.m4,v 1.2 1998/04/14 00:14:45 drd Exp $

# a note to the uninitiated : the program aclocal (part of
# GNU automake) generates the file aclocal.m4 from
# the file acinclude.m4
# Furthermore, configure.in invokes various AM_ macros
# which I think are supplied as part of GNU automake.
# aclocal also extracts these macros and adds them
# to aclocal.m4


## ------------------------------- ##
## Check for function prototypes.  ##
## From Franc,ois Pinard           ##
## ------------------------------- ##

# serial 1

AC_DEFUN(AM_C_PROTOTYPES,
[AC_REQUIRE([AM_PROG_CC_STDC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_MSG_CHECKING([for function prototypes])
if test "$am_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES)
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=./ansi2knr
  # Ensure some checks needed by ansi2knr itself.
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h)
fi
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])

# serial 1
 
# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it defines @code{__STDC__} to 1 and
# handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac
 
AC_DEFUN(AM_PROG_CC_STDC,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX                   -qlanglvl=ansi
# Ultrix and OSF/1      -std1
# HP-UX                 -Aa -D_HPUX_SOURCE
# SVR4                  -Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#if !defined(__STDC__) || __STDC__ != 1
choke me
#endif
/* DYNIX/ptx V4.1.3 can't compile sys/stat.h with -Xc -D__EXTENSIONS__. */
#ifdef _SEQUENT_
# include <sys/types.h>
# include <sys/stat.h>
#endif
], [
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
AC_MSG_RESULT($am_cv_prog_cc_stdc)
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])

## ------------------------------------------------- ##
## See if the system preprocessor understands        ##
## the ANSI C preprocessor stringification operator. ##
## Snarfed from the current egcs snapshot. -lh       ##
## ------------------------------------------------- ##

AC_DEFUN(gp_PROG_CPP_STRINGIFY,
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING(whether cpp understands the stringify operator)
AC_CACHE_VAL(gcc_cv_c_have_stringify,
[AC_TRY_COMPILE(,
[#define S(x)   #x
char *test = S(foo);],
gcc_cv_c_have_stringify=yes, gcc_cv_c_have_stringify=no)])
AC_MSG_RESULT($gcc_cv_c_have_stringify)
if test $gcc_cv_c_have_stringify = yes; then
  AC_DEFINE(HAVE_CPP_STRINGIFY)
fi
])

## ------------------------------- ##
## Check for MS-DOS/djgpp.         ##
## From Lars Hecking               ##
## ------------------------------- ##
## Changes from Hans-Bernhard Broeker:            ##
## - Only use GRX20 if really present.            ##
## - Check if GRX is actually the new 21a release ##
## - disable looking for -lvga from linux         ##

# serial 1

AC_DEFUN(gp_MSDOS,
[AC_MSG_CHECKING(for MS-DOS/djgpp/libGRX)
AC_EGREP_CPP(yes,
[#if __DJGPP__ && __DJGPP__ == 2
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="-lpc $LIBS"
   AC_DEFINE(MSDOS)
   AC_DEFINE(DOS32)
   with_linux_vga=no
   AC_CHECK_LIB(grx20,GrLine,dnl
      LIBS="-lgrx20 $LIBS"
      CFLAGS="$CFLAGS -fno-inline-functions"
      AC_DEFINE(DJSVGA)
      AC_CHECK_LIB(grx20,GrCustomLine,AC_DEFINE(GRX21))),dnl
   AC_MSG_RESULT(no)
   )dnl 
])

## ------------------------------- ##
## Check for NeXT.                 ##
## From Lars Hecking               ##
## ------------------------------- ##

# serial 1

AC_DEFUN(gp_NEXT,
[AC_MSG_CHECKING(for NeXT)
AC_EGREP_CPP(yes,
[#if __NeXT__
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="$LIBS -lsys_s -lNeXT_s"
   NEXTOBJS=epsviewe.o
   CFLAGS="$CFLAGS -ObjC",dnl
   NEXTOBJS=
   AC_MSG_RESULT(no))
])

## --------------------------------- ##
## Like AC_CHECK_HEADER, but quiet,  ##
## and no caching.                   ##
## From Lars Hecking                 ##
## --------------------------------- ##

# serial 1

dnl gp_CHECK_HEADER_QUIET(HEADER-FILE, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(gp_CHECK_HEADER_QUIET,
[dnl Do the transliteration at runtime so arg 1 can be a shell variable.
dnl No checking of cache values.
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
AC_TRY_CPP([#include <$1>], eval "ac_cv_header_$ac_safe=yes",
  eval "ac_cv_header_$ac_safe=no")
if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , , [$3
])dnl
fi
])

## ------------------------------ ##
## Like AC_CHECK_LIB, but quiet,  ##
## and no caching.                ##
## From Lars Hecking              ##
## ------------------------------ ##

# serial 1

AC_DEFUN(gp_CHECK_LIB_QUIET,
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
## Check several directories for library.      ##
## NOTE: OTHER_LIBRARIES are NOT automatically ##
## added to TERMLIBS. This must be done in     ##
## configure.in!                               ##
## From Lars Hecking                           ##
## ------------------------------------------- ##

# serial 1

dnl gp_CHECK_LIB_PATH(LIBRARY, FUNCTION [, OTHER-LIBRARIES])
dnl
AC_DEFUN(gp_CHECK_LIB_PATH,
[AC_MSG_CHECKING([for $2 in -l$1])
gp_save_TERMLIBS="$TERMLIBS"
changequote(, )dnl
  gp_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
dnl The case for "no" is really just a safety net
case "$with_$1" in
  yes|no)
    gp_lib_list="";;
  *)
    gp_lib_path=`echo $with_$1 | sed -e 's%/lib$1\.a$%%'`
    gp_lib_prefix=`echo $gp_lib_path | sed 's%/lib$%%'`
    gp_lib_list="$gp_lib_prefix $gp_lib_prefix/lib $gp_lib_path"
esac
for ac_dir in '' /usr/local/lib $gp_lib_list ; do
  TERMLIBS="`test x${ac_dir} != x && echo -L${ac_dir}` $gp_save_TERMLIBS"
  gp_CHECK_LIB_QUIET($1,$2,dnl
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

## ------------------------------------------- ##
## Check several directories for header file.  ##
## From Lars Hecking                           ##
## ------------------------------------------- ##

# serial 1

dnl gp_CHECK_HEADER(HEADER-FILE, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(gp_CHECK_HEADER,
[AC_MSG_CHECKING([for $1])
gp_save_CPPFLAGS="$CPPFLAGS"
for ac_dir in '' /usr/local/include $gp_lib_prefix $gp_lib_prefix/include ; do
  CPPFLAGS="$gp_save_CPPFLAGS `test x${ac_dir} != x && echo -I${ac_dir}`"
  gp_CHECK_HEADER_QUIET($1,break,CPPFLAGS="$ac_save_CPPFLAGS")
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
