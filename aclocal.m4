dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])


# serial 1

AC_DEFUN(AM_C_PROTOTYPES,
[AC_REQUIRE([AM_PROG_CC_STDC])
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for function prototypes])
if test "$am_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES,1,[Define if compiler has function prototypes])
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
# compiler to be in ANSI C mode if it handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN(AM_PROG_CC_STDC,
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX			-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;
], [
return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];
],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
if test -z "$am_cv_prog_cc_stdc"; then
  AC_MSG_RESULT([none needed])
else
  AC_MSG_RESULT($am_cv_prog_cc_stdc)
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])

# Define a conditional.

AC_DEFUN(AM_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])


# serial 1

AC_DEFUN(GP_MSDOS,
[AC_MSG_CHECKING(for MS-DOS/djgpp/libGRX)
AC_EGREP_CPP(yes,
[#if __DJGPP__ && __DJGPP__ == 2
  yes
#endif
],AC_MSG_RESULT(yes)
  LIBS="-lpc $LIBS"
  AC_DEFINE(MSDOS, 1,
            [ Define if this is an MSDOS system. ])
  AC_DEFINE(DOS32, 1,
            [ Define if this system uses a 32-bit DOS extender (djgpp/emx). ])
  with_linux_vga=no
  AC_CHECK_LIB(grx20,GrLine,
    LIBS="-lgrx20 $LIBS"
    CFLAGS="$CFLAGS -fno-inline-functions"
    AC_DEFINE(DJSVGA, 1,
              [ Define if you want to use libgrx20 with MSDOS/djgpp. ])
    AC_CHECK_LIB(grx20,GrCustomLine,
      AC_DEFINE(GRX21, 1,
                [ Define if you want to use a newer version of libgrx under MSDOS/djgpp. ])dnl
    )dnl
  ),
  AC_MSG_RESULT(no)
  )dnl 
])



# serial 1

AC_DEFUN(GP_NEXT,
[AC_MSG_CHECKING(for NeXT)
AC_EGREP_CPP(yes,
[#if __NeXT__
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="$LIBS -lsys_s -lNeXT_s"
   CFLAGS="$CFLAGS -ObjC",
   AC_MSG_RESULT(no))
])



# serial 1

AC_DEFUN(GP_APPLE,
[AC_MSG_CHECKING(for Apple MacOS X)
AC_EGREP_CPP(yes,
[#if defined(__APPLE__) && defined(__MACH__)
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="$LIBS -framework Foundation -framework AppKit"
   CFLAGS="$CFLAGS -ObjC",
   AC_MSG_RESULT(no))
])



# serial 1

AC_DEFUN(GP_BEOS,
[AC_MSG_CHECKING(for BeOS)
AC_EGREP_CPP(yes,
[#if __BEOS__
  yes
#endif
], AC_MSG_RESULT(yes)
   build_src_beos_subdir=yes,
   build_src_beos_subdir=no
   AC_MSG_RESULT(no))
])


dnl testing new version of FUNC_SELECT

AC_DEFUN(AC_FUNC_SELECT,
[AC_CHECK_FUNCS(select)
if test "$ac_cv_func_select" = yes; then
  AC_CHECK_HEADERS(unistd.h sys/types.h sys/time.h sys/select.h sys/socket.h)
  AC_MSG_CHECKING([argument types of select()])
  AC_CACHE_VAL(ac_cv_type_fd_set_size_t,
    [AC_CACHE_VAL(ac_cv_type_fd_set,
      [for ac_cv_type_fd_set in 'fd_set' 'int' 'void'; do
        for ac_cv_type_fd_set_size_t in 'int' 'size_t' 'unsigned long' 'unsigned'; do
	  for ac_type_timeval in 'struct timeval' 'const struct timeval'; do
            AC_TRY_COMPILE(
[#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
],
[#ifdef __STDC__
extern int select ($ac_cv_type_fd_set_size_t, 
 $ac_cv_type_fd_set *,	$ac_cv_type_fd_set *, $ac_cv_type_fd_set *,
 $ac_type_timeval *);
#else
extern int select ();
  $ac_cv_type_fd_set_size_t s;
  $ac_cv_type_fd_set *p;
  $ac_type_timeval *t;
#endif
],
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
                     [ First arg for select. ])
  ac_cast=
  if test "$ac_cv_type_fd_set" != fd_set; then
    # Arguments 2-4 are not fd_set.  Some weirdo systems use fd_set type for
    # FD_SET macros, but insist that you cast the argument to select.  I don't
    # understand why that might be, but it means we cannot define fd_set.
    AC_EGREP_CPP(
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])fd_set[^a-zA-Z_0-9]>>dnl
changequote([,]),dnl
[#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
],
    # We found fd_set type in a header, need special cast
    ac_cast="($ac_cv_type_fd_set *)",dnl
    # No fd_set type; it is safe to define it
    AC_DEFINE_UNQUOTED(fd_set,$ac_cv_type_fd_set,
                       [ Define if the type in arguments 2-4 to select is fd_set. ]))
  fi
  AC_DEFINE_UNQUOTED(SELECT_FD_SET_CAST,$ac_cast,
                     [ Define if the type in arguments 2-4 to select is fd_set. ])
fi
])



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



# serial 2

dnl GP_PATH_HEADER(HEADER-FILE, SEARCH-DIRS [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
AC_DEFUN(GP_PATH_HEADER,
[ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
changequote(, )dnl
  ac_tr_hdr=HAVE_`echo $1 | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl
AC_MSG_CHECKING([for $1])
gp_save_CPPFLAGS="$CPPFLAGS"
if test "$2" != yes && test "$2" != no; then
  gp_h_path=`echo "$2" | sed -e 's%/lib$1\.a$%%'`
  gp_h_prfx=`echo "$gp_h_path" | sed -e 's%/lib$%%' -e 's%/include$%%'`
  gp_h_list="$gp_h_prfx $gp_h_prfx/include $gp_h_path"
else
  gp_h_list=''
fi
for ac_dir in $gp_h_list '' /usr/local/include ; do
  test x${ac_dir} != x && CPPFLAGS="$gp_save_CPPFLAGS -I${ac_dir}"
  AC_TRY_CPP([#include <$1>], eval "ac_cv_header_$ac_safe=yes",
    eval "ac_cv_header_$ac_safe=no")
  if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
    break
  fi
done

if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
  AC_DEFINE_UNQUOTED($ac_tr_hdr)
  AC_MSG_RESULT(yes)
  ifelse([$3], , :, [$3])
else
  AC_MSG_RESULT(no)
  CPPFLAGS="$gp_save_CPPFLAGS"
  ifelse([$4], , , [$4
])dnl
fi
])


