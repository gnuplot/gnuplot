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
[#ifdef __DJGPP__ && __DJGPP__ == 2
  yes
#endif
], LDFLAGS="$LDFLAGS -lpc"
   AC_DEFINE(MSDOS)
   AC_DEFINE(DOS32)
   AC_MSG_RESULT(yes)
   with_linux_vga=no
   AC_CHECK_LIB(grx20,GrLine,
      LDFLAGS="$LDFLAGS -lgrx20"
      TERMFLAGS="$TERMFLAGS -DDJSVGA -fno-inline-functions"
      AC_CHECK_LIB(grx20,GrCustomLine,TERMFLAGS="$TERMFLAGS -DGRX21"))
 , AC_MSG_RESULT(no)
 )dnl 
])

## ------------------------------- ##
## Check for NeXT.                 ##
## From Lars Hecking               ##
## ------------------------------- ##

# serial 1

AC_DEFUN(gp_NEXT,
[AC_MSG_CHECKING(for NeXT)
NEXTOBJS=
AC_SUBST(NEXTOBJS)
AC_EGREP_CPP(yes,
[#ifdef __NeXT__
  yes
#endif
], LIBS="$LIBS -lsys_s -lNeXT_s"
   NEXTOBJS=epsviewe.o
   TERMFLAGS=-ObjC
   AC_MSG_RESULT(yes),AC_MSG_RESULT(no))
])

# The next two macros are silent versions
# of the resp. AC_ macros. They are needed
# for the new gp_CHECK_LIB_PATH and
# gp_CHECK_HEADER macros, which print their
# own messages. -lh

## --------------------------------- ##
## Like AC_CHECK_HEADER, but quiet.  ##
## From Lars Hecking                 ##
## --------------------------------- ##

# serial 1

dnl gp_CHECK_HEADER_QUIET(HEADER-FILE, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(gp_CHECK_HEADER_QUIET,
[dnl Do the transliteration at runtime so arg 1 can be a shell variable.
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_header_$ac_safe,
[AC_TRY_CPP([#include <$1>], eval "ac_cv_header_$ac_safe=yes",
  eval "ac_cv_header_$ac_safe=no")])dnl
if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
  ifelse([$2], , :, [$2])
else
ifelse([$3], , , [$3
])dnl
fi
])

## ------------------------------ ##
## Like AC_CHECK_LIB, but quiet.  ##
## From Lars Hecking              ##
## ------------------------------ ##

# serial 1

AC_DEFUN(gp_CHECK_LIB_QUIET,
[dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $1 defining function $2, not
dnl just for library $1.  Separate tests with the same $1 and different $2s
dnl may have different results.
ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"
LIBS="$TERMLIBS -l$1 $5 $LIBS"
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
])dnl
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

## --------------------------------------- ##
## Check several directories for library.  ##
## From Lars Hecking                       ##
## --------------------------------------- ##

# serial 1

dnl gp_CHECK_LIB_PATH(LIBRARY, FUNCTION [, OTHER-LIBRARIES])
dnl
AC_DEFUN(gp_CHECK_LIB_PATH,
[
if test "$with_$1" != no; then

  AC_MSG_CHECKING([for $2 in -l$1])

  gp_save_TERMLIBS="$TERMLIBS"
  gp_tr_lib="HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`"

  case "$with_$1" in
    yes)
      gp_lib_list="";;

    *)
      with_$1=`echo $with_$1 | sed 's%/lib$1\.a$%%'`
      gp_lib_prefix=`echo $with_$1 | sed 's%/lib$%%'`
      gp_lib_list="$gp_lib_prefix $gp_lib_prefix/lib $with_$1"
  esac

  for ac_dir in '' $libdir $gp_lib_list ; do
    TERMLIBS="`test x${ac_dir} != x && echo -L${ac_dir}` $gp_save_TERMLIBS"
      gp_CHECK_LIB_QUIET($1,$2,
        dnl ACTION-IF-FOUND
        TERMLIBS="$TERMLIBS -l$1 $3"; break,
        dnl ACTION-IF-NOT-FOUND
        TERMLIBS="$gp_save_TERMLIBS"
        unset ac_cv_lib_$ac_lib_var,
        dnl OTHER-LIBRARIES
        $3)
    done

    if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi

fi dnl with_$1 != no

])dnl macro end


## -------------------------------------------- ##
## Check several directories for include file.  ##
## From Lars Hecking                            ##
## -------------------------------------------- ##

# serial 1

dnl gp_CHECK_HEADER(HEADER-FILE, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(gp_CHECK_HEADER,
[
gp_save_CPPFLAGS="$CPPFLAGS"

AC_MSG_CHECKING([for $1])

for ac_dir in '' $includedir $gp_lib_prefix $gp_lib_prefix/include ; do
  CPPFLAGS="$gp_save_CPPFLAGS `test x${ac_dir} != x && echo -I${ac_dir}`"
  gp_CHECK_HEADER_QUIET($1,
    break,
    CPPFLAGS="$ac_save_CPPFLAGS"
    unset ac_cv_header_$ac_safe
    )dnl gp_CHECK_HEADER_QUIET
done

if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi

])dnl macro end
