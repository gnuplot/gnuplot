# aclocal.m4 generated automatically by aclocal 1.4c

# Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 4

# We require 2.13 because we rely on SHELL being computed by configure.
AC_PREREQ([2.13])

# AC_PROVIDE_IFELSE(MACRO-NAME, IF-PROVIDED, IF-NOT-PROVIDED)
# -----------------------------------------------------------
# If MACRO-NAME is provided do IF-PROVIDED, else IF-NOT-PROVIDED.
# The purpose of this macro is to provide the user with a means to
# check macros which are provided without letting her know how the
# information is coded.
# If this macro is not defined by Autoconf, define it here.
ifdef([AC_PROVIDE_IFELSE],
      [],
      [define([AC_PROVIDE_IFELSE],
              [ifdef([AC_PROVIDE_$1],
                     [$2], [$3])])])


# AM_INIT_AUTOMAKE(PACKAGE,VERSION, [NO-DEFINE])
# ----------------------------------------------
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AC_PROG_INSTALL])dnl
# test to see if srcdir already configured
if test "`CDPATH=:; cd $srcdir && pwd`" != "`pwd`" &&
   test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi

# Define the identity of the package.
PACKAGE=$1
AC_SUBST(PACKAGE)dnl
VERSION=$2
AC_SUBST(VERSION)dnl
ifelse([$3],,
[AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])])

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG(ACLOCAL, aclocal)
AM_MISSING_PROG(AUTOCONF, autoconf)
AM_MISSING_PROG(AUTOMAKE, automake)
AM_MISSING_PROG(AUTOHEADER, autoheader)
AM_MISSING_PROG(MAKEINFO, makeinfo)
AM_MISSING_PROG(AMTAR, tar)
AM_MISSING_INSTALL_SH
# We need awk for the "check" target.  The system "awk" is bad on
# some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl
AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_PROVIDE_IFELSE([AC_PROG_CC],
                  [AM_DEPENDENCIES(CC)],
                  [define([AC_PROG_CC],
                          defn([AC_PROG_CC])[AM_DEPENDENCIES(CC)])])dnl
AC_PROVIDE_IFELSE([AC_PROG_CXX],
                  [AM_DEPENDENCIES(CXX)],
                  [define([AC_PROG_CXX],
                          defn([AC_PROG_CXX])[AM_DEPENDENCIES(CXX)])])dnl
])

#
# Check to make sure that the build environment is sane.
#

# serial 2

AC_DEFUN([AM_SANITY_CHECK],
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
   if test "$[*]" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "$[*]" != "X $srcdir/configure conftestfile" \
      && test "$[*]" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "$[2]" = conftestfile
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


# serial 2

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])


# AM_MISSING_INSTALL_SH
# ---------------------
# Like AM_MISSING_PROG, but only looks for install-sh.
AC_DEFUN([AM_MISSING_INSTALL_SH],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
if test -z "$install_sh"; then
   for install_sh in "$ac_aux_dir/install-sh" \
                     "$ac_aux_dir/install.sh" \
                     "${am_missing_run}${ac_auxdir}/install-sh";
   do
     test -f "$install_sh" && break
   done
   # FIXME: an evil hack: we remove the SHELL invocation from
   # install_sh because automake adds it back in.  Sigh.
   install_sh=`echo $install_sh | sed -e 's/\${SHELL}//'`
fi
AC_SUBST(install_sh)])


# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it supports --run.
# If it does, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[test x"${MISSING+set}" = xset ||
  MISSING="\${SHELL} `CDPATH=:; cd $ac_aux_dir && pwd`/missing"
# Use eval to expand $SHELL
if eval "$MISSING --run :"; then
  am_missing_run="$MISSING --run "
else
  am_missing_run=
  am_backtick='`'
  AC_MSG_WARN([${am_backtick}missing' script is too old or missing])
fi
])

# serial 2

# AM_DEPENDENCIES(NAME)
# ---------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX" or "OBJC".
# We try a few techniques and use that to set a single cache variable.
AC_DEFUN([AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])
ifelse([$1], CC,
       [AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CPP])
depcc="$CC"
depcpp="$CPP"],
       [$1], CXX, [AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_PROG_CXXCPP])
depcc="$CXX"
depcpp="$CXXCPP"],
       [$1], OBJC, [am_cv_OBJC_dependencies_compiler_type=gcc],
       [AC_REQUIRE([AC_PROG_$1])
depcc="$$1"
depcpp=""])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP"; then
  echo '#include "conftest.h"' > conftest.c
  echo 'int i;' > conftest.h

  am_cv_$1_dependencies_compiler_type=none
  for depmode in `sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < "$am_depcomp"`; do
    case "$depmode" in
    nosideeffect)
      # after this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    none) break ;;
    esac
    # We check with `-c' and `-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle `-M -o', and we need to detect this.
    if depmode="$depmode" \
       source=conftest.c object=conftest.o \
       depfile=conftest.Po tmpdepfile=conftest.TPo \
       $SHELL $am_depcomp $depcc -c conftest.c -o conftest.o >/dev/null 2>&1 &&
       grep conftest.h conftest.Po > /dev/null 2>&1; then
      am_cv_$1_dependencies_compiler_type="$depmode"
      break
    fi
  done

  rm -f conftest.*
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
$1DEPMODE="depmode=$am_cv_$1_dependencies_compiler_type"
AC_SUBST([$1DEPMODE])
])

# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in AM_DEPENDENCIES

AC_DEFUN([AM_SET_DEPDIR],[
if test -d .deps || mkdir .deps 2> /dev/null || test -d .deps; then
  DEPDIR=.deps
  # We redirect because .deps might already exist and be populated.
  # In this situation we don't want to see an error.
  rmdir .deps > /dev/null 2>&1
else
  DEPDIR=_deps
fi
AC_SUBST(DEPDIR)
])

AC_DEFUN([AM_DEP_TRACK],[
AC_ARG_ENABLE(dependency-tracking,
[  --disable-dependency-tracking Speeds up one-time builds
  --enable-dependency-tracking  Do not reject slow dependency extractors])
if test "x$enable_dependency_tracking" = xno; then
  AMDEP="#"
else
  am_depcomp="$ac_aux_dir/depcomp"
  if test ! -f "$am_depcomp"; then
    AMDEP="#"
  else
    AMDEP=
  fi
fi
AC_SUBST(AMDEP)
if test -z "$AMDEP"; then
  AMDEPBACKSLASH='\'
else
  AMDEPBACKSLASH=
fi
pushdef([subst], defn([AC_SUBST]))
subst(AMDEPBACKSLASH)
popdef([subst])
])

# Generate code to set up dependency tracking.
# This macro should only be invoked once -- use via AC_REQUIRE.
# Usage:
# AM_OUTPUT_DEPENDENCY_COMMANDS

#
# This code is only required when automatic dependency tracking
# is enabled.  FIXME.  This creates each `.P' file that we will
# need in order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],[
AC_OUTPUT_COMMANDS([
test x"$AMDEP" != x"" ||
for mf in $CONFIG_FILES; do
  case "$mf" in
  Makefile) dirpart=.;;
  */Makefile) dirpart=`echo "$mf" | sed -e 's|/[^/]*$||'`;;
  *) continue;;
  esac
  grep '^DEP_FILES *= *[^ #]' < "$mf" > /dev/null || continue
  # Extract the definition of DEP_FILES from the Makefile without
  # running `make'.
  DEPDIR=`sed -n -e '/^DEPDIR = / s///p' < "$mf"`
  test -z "$DEPDIR" && continue
  # When using ansi2knr, U may be empty or an underscore; expand it
  U=`sed -n -e '/^U = / s///p' < "$mf"`
  test -d "$dirpart/$DEPDIR" || mkdir "$dirpart/$DEPDIR"
  # We invoke sed twice because it is the simplest approach to
  # changing $(DEPDIR) to its actual value in the expansion.
  for file in `sed -n -e '
    /^DEP_FILES = .*\\\\$/ {
      s/^DEP_FILES = //
      :loop
	s/\\\\$//
	p
	n
	/\\\\$/ b loop
      p
    }
    /^DEP_FILES = / s/^DEP_FILES = //p' < "$mf" | \
       sed -e 's/\$(DEPDIR)/'"$DEPDIR"'/g' -e 's/\$U/'"$U"'/g'`; do
    # Make sure the directory exists.
    test -f "$dirpart/$file" && continue
    fdir=`echo "$file" | sed -e 's|/[^/]*$||'`
    $ac_aux_dir/mkinstalldirs "$dirpart/$fdir" > /dev/null 2>&1
    # echo "creating $dirpart/$file"
    echo '# dummy' > "$dirpart/$file"
  done
done
], [AMDEP="$AMDEP"
ac_aux_dir="$ac_aux_dir"])])


# serial 1

AC_DEFUN([AM_C_PROTOTYPES],
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
fi
# Ensure some checks needed by ansi2knr itself.
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h)
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

AC_DEFUN([AM_PROG_CC_STDC],
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING([for ${CC-cc} option to accept ANSI C])
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX 10.20 and later	-Ae
# HP-UX older versions	-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 -Ae "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
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
  AC_MSG_RESULT([$am_cv_prog_cc_stdc])
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])


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


