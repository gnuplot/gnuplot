/*
 * $Id: stdfn.h,v 1.36 2011/01/03 14:30:31 juhaszp Exp $
 */

/* GNUPLOT - stdfn.h */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

/* get prototypes or declarations for string and stdlib functions and deal
   with missing functions like strchr. */

/* we will assume the ANSI/Posix/whatever situation as default.
   the header file is called string.h and the index functions are called
   strchr, strrchr. Exceptions have to be listed explicitly */

#ifndef STDFN_H
#define STDFN_H

#include <ctype.h>
#include <stdio.h>

#include "syscfg.h"

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#ifdef HAVE_BCOPY
# ifndef HAVE_MEMCPY
#  define memcpy(d,s,n) bcopy((s),(d),(n))
# endif
# ifndef HAVE_MEMMOVE
#  define memmove(d,s,n) bcopy((s),(d),(n))
# endif
#else
# ifndef HAVE_MEMCPY
char * memcpy __PROTO((char *, char *, size_t));
# endif
#endif /* HAVE_BCOPY */

#ifndef HAVE_STRCHR
# ifdef strchr
#  undef strchr
# endif
# ifdef HAVE_INDEX
#  define strchr index
# else
char *strchr __PROTO((const char *, int));
# endif
# ifdef strrchr
#  undef strrchr
# endif
# ifdef HAVE_RINDEX
#  define strrchr rindex
# endif
#endif
#ifndef HAVE_STRCSPN
size_t gp_strcspn __PROTO((const char *, const char *));
# define strcspn gp_strcspn
#endif

#ifndef HAVE_STRSTR
char *strstr __PROTO((const char *, const char *));
#endif

#ifndef HAVE_STDLIB_H
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# else
void free();
char *malloc();
char *realloc();
# endif /* HAVE_MALLOC_H */
char *getenv();
int system();
double atof();
int atoi();
long atol();
double strtod();
#else /* HAVE_STDLIB_H */
# include <stdlib.h>
# ifndef VMS
#  ifndef EXIT_FAILURE
#   define EXIT_FAILURE (1)
#  endif
#  ifndef EXIT_SUCCESS
#   define EXIT_SUCCESS (0)
#  endif
# else /* VMS */
#  ifdef VAXC            /* replacement values suppress some messages */
#   ifdef  EXIT_FAILURE
#    undef EXIT_FAILURE
#   endif
#   ifdef  EXIT_SUCCESS
#    undef EXIT_SUCCESS
#   endif
#  endif /* VAXC */
#  ifndef  EXIT_FAILURE
#   define EXIT_FAILURE  0x10000002
#  endif
#  ifndef  EXIT_SUCCESS
#   define EXIT_SUCCESS  1
#  endif
# endif /* VMS */
#endif /* HAVE_STDLIB_H */

/* Deal with varargs functions */
#if defined(HAVE_VFPRINTF) || defined(HAVE_DOPRNT)
# ifdef STDC_HEADERS
#  include <stdarg.h>
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#  define VA_START(args, lastarg) va_start(args)
# endif /* !STDC_HEADERS */
#else /* HAVE_VFPRINTF || HAVE_DOPRNT */
# define va_dcl char *a1, char *a2, char *a3, char *a4, char *a5, char *a6, char *a7, char *a8
#endif /* !(HAVE_VFPRINTF || HAVE_DOPRNT) */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
# ifdef HAVE_LIBC_H /* NeXT uses libc instead of unistd */
#  include <libc.h>
# endif
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
# ifdef EXTERN_ERRNO
extern int errno;
#endif
#ifndef HAVE_STRERROR
char *strerror __PROTO((int));
extern int sys_nerr;
extern char *sys_errlist[];
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>

/* This is all taken from GNU fileutils lib/filemode.h */

#if !S_IRUSR
# if S_IREAD
#  define S_IRUSR S_IREAD
# else
#  define S_IRUSR 00400
# endif
#endif

#if !S_IWUSR
# if S_IWRITE
#  define S_IWUSR S_IWRITE
# else
#  define S_IWUSR 00200
# endif
#endif

#if !S_IXUSR
# if S_IEXEC
#  define S_IXUSR S_IEXEC
# else
#  define S_IXUSR 00100
# endif
#endif

#ifdef STAT_MACROS_BROKEN
# undef S_ISBLK
# undef S_ISCHR
# undef S_ISDIR
# undef S_ISFIFO
# undef S_ISLNK
# undef S_ISMPB
# undef S_ISMPC
# undef S_ISNWK
# undef S_ISREG
# undef S_ISSOCK
#endif /* STAT_MACROS_BROKEN.  */

#if !defined(S_ISBLK) && defined(S_IFBLK)
# define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
# define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
# define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
# define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
# define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
# define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
# define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
#endif
#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
# define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
#endif

#endif /* HAVE_SYS_STAT_H */

#ifdef HAVE_LIMITS_H
# include <limits.h>
#else
# ifdef HAVE_VALUES_H
#  include <values.h>
# endif /* HAVE_VALUES_H */
#endif /* HAVE_LIMITS_H */

/* ctime etc, should also define time_t and struct tm */
#ifdef HAVE_TIME_H
# include <time.h>
#endif

#ifndef HAVE_TIME_T_IN_TIME_H
# define time_t long
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h> /* for gettimeofday() */
#endif

#if defined(PIPES) && defined(VMS)
FILE *popen __PROTO((char *, char *));
int pclose __PROTO((FILE *));
#endif

#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

/* Some older platforms, namely SunOS 4.x, don't define this. */
#ifndef DBL_EPSILON
# define DBL_EPSILON     2.2204460492503131E-16
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef HAVE_MATH_H
# include <math.h>
#endif

/* Normally in <math.h> */
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
# define M_PI_2 1.57079632679489661923
#endif
#ifndef M_LN10
#  define M_LN10    2.3025850929940456840e0
#endif

/* Use definitions from float.h and math.h if we found them */
#if defined(DBL_MIN_10_EXP)
# define E_MINEXP (DBL_MIN_10_EXP * M_LN10)
#endif
#if defined(DBL_MAX_10_EXP)
# define E_MAXEXP (DBL_MAX_10_EXP * M_LN10)
#endif

#ifndef HAVE_STRCASECMP
# ifdef HAVE_STRICMP
#  define strcasecmp stricmp
# else
int gp_stricmp __PROTO((const char *, const char *));
#  define strcasecmp gp_stricmp
# endif
#endif

#ifndef HAVE_STRNCASECMP
# ifdef HAVE_STRNICMP
#  define strncasecmp strnicmp
# else
int gp_strnicmp __PROTO((const char *, const char *, size_t));
#  define strncasecmp gp_strnicmp
# endif
#endif

#ifndef GP_GETCWD
# if defined(HAVE_GETCWD)
#   if defined(__EMX__)
#     define GP_GETCWD(path,len) _getcwd2 (path, len)
#   else
#     define GP_GETCWD(path,len) getcwd (path, len)
#   endif /* __EMX__ */
# else
#  define GP_GETCWD(path,len) getwd (path)
# endif
#endif

#ifdef WIN32
# include <windows.h>
#endif

/* sleep delay time, where delay is a double value */
#if defined(HAVE_USLEEP)
#  define GP_SLEEP(delay) usleep((unsigned int) ((delay)*1e6))
#  ifndef HAVE_SLEEP
#    define HAVE_SLEEP
#  endif
#elif defined(__EMX__)
#  define GP_SLEEP(delay) _sleep2((unsigned int) ((delay)*1e3))
#  ifndef HAVE_SLEEP
#    define HAVE_SLEEP
#  endif
#elif defined(WIN32)
#  define GP_SLEEP(delay) win_sleep((DWORD) 1000*delay)
#  ifndef HAVE_SLEEP
#    define HAVE_SLEEP
#  endif
#endif

#ifndef GP_SLEEP
#  define GP_SLEEP(delay) sleep ((unsigned int) (delay+0.5))
#endif

#ifdef HAVE_ATEXIT
# define GP_ATEXIT(x) atexit((x))
#elif defined(HAVE_ON_EXIT)
# define GP_ATEXIT(x) on_exit((x),0)
#else
# define GP_ATEXIT(x) /* you lose */
#endif

/* Misc. defines */

/* Null character */
#define NUL ('\0')

/* Definitions for debugging */
/* #define NDEBUG */
#include <assert.h>

#ifdef DEBUG
# define DEBUG_WHERE do { fprintf(stderr,"%s:%d ",__FILE__,__LINE__); } while (0)
# define FPRINTF(a) do { DEBUG_WHERE; fprintf a; } while (0)
#else
# define DEBUG_WHERE     /* nought */
# define FPRINTF(a)      /* nought */
#endif /* DEBUG */

#include "syscfg.h"

#define INT_STR_LEN (3*sizeof(int))


/* HBB 20010223: moved this whole block from syscfg.h to here. It
 * needs both "syscfg.h" and <float.h> to have been #include'd before
 * this, since it relies on stuff like DBL_MAX */

/* There is a bug in the NEXT OS. This is a workaround. Lookout for
 * an OS correction to cancel the following dinosaur
 *
 * Hm, at least with my setup (compiler version 3.1, system 3.3p1),
 * DBL_MAX is defined correctly and HUGE and HUGE_VAL are both defined
 * as 1e999. I have no idea to which OS version the bugfix below
 * applies, at least wrt. HUGE, it is inconsistent with the current
 * version. Since we are using DBL_MAX anyway, most of this isn't
 * really needed anymore.
 */

#if defined ( NEXT ) && NX_CURRENT_COMPILER_RELEASE<310
# if defined ( DBL_MAX)
#  undef DBL_MAX
# endif
# define DBL_MAX 1.7976931348623157e+308
# undef HUGE
# define HUGE    DBL_MAX
# undef HUGE_VAL
# define HUGE_VAL DBL_MAX
#endif /* NEXT && NX_CURRENT_COMPILER_RELEASE<310 */

/*
 * Note about VERYLARGE:  This is the upper bound double (or float, if PC)
 * numbers. This flag indicates very large numbers. It doesn't have to
 * be the absolutely biggest number on the machine.
 * If your machine doesn't have HUGE, or float.h,
 * define VERYLARGE here.
 *
 * example:
#define VERYLARGE 1e37
 *
 * To get an appropriate value for VERYLARGE, we can use DBL_MAX (or
 * FLT_MAX on PCs), HUGE or HUGE_VAL. DBL_MAX is usually defined in
 * float.h and is the largest possible double value. HUGE and HUGE_VAL
 * are either DBL_MAX or +Inf (IEEE special number), depending on the
 * compiler. +Inf may cause problems with some buggy fp
 * implementations, so we better avoid that. The following should work
 * better than the previous setup (which used HUGE in preference to
 * DBL_MAX).
 */
/* Now define VERYLARGE. This is usually DBL_MAX/2 - 1. On MS-DOS however
 * we use floats for memory considerations and thus use FLT_MAX.
 */

#ifndef COORDVAL_FLOAT
# ifdef DBL_MAX
#  define VERYLARGE (DBL_MAX/2-1)
# endif
#else /* COORDVAL_FLOAT */
# ifdef FLT_MAX
#  define VERYLARGE (FLT_MAX/2-1)
# endif
#endif /* COORDVAL_FLOAT */


#ifndef VERYLARGE
# ifdef HUGE
#  define VERYLARGE (HUGE/2-1)
# elif defined(HUGE_VAL)
#  define VERYLARGE (HUGE_VAL/2-1)
# else
/* as a last resort */
#  define VERYLARGE (1e37)
/* #  warning "using last resort 1e37 as VERYLARGE define, please check your headers" */
/* Maybe add a note somewhere in the install docs instead */
# endif /* HUGE */
#endif /* VERYLARGE */

/* _POSIX_PATH_MAX is too small for practical purposes */
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifndef PATH_MAX
# ifndef MAXPATHLEN
#  define PATH_MAX 1024
# else
#  define PATH_MAX MAXPATHLEN
# endif
#endif

/* Concatenate a path name and a file name. The file name
 * may or may not end with a "directory separation" character.
 * Path must not be NULL, but can be empty
 */
#define PATH_CONCAT(path,file) \
 { char *p = path; \
   p += strlen(path); \
   if (p!=path) p--; \
   if (*p && (*p != DIRSEP1) && (*p != DIRSEP2)) { \
     if (*p) p++; *p++ = DIRSEP1; *p = NUL; \
   } \
   strcat (path, file); \
 }

#ifndef inrange
# define inrange(z,min,max) \
   (((min)<(max)) ? (((z)>=(min)) && ((z)<=(max))) : \
	            (((z)>=(max)) && ((z)<=(min))))
#endif

/* HBB 20030117: new macro to simplify clipping operations in the
 * presence of possibly reverted axes */
#ifndef cliptorange
# define cliptorange(z,min,max)			\
    do {					\
       if ((min) < (max)) {			\
	   if ((z) > (max))			\
	       (z) = (max);			\
	   else if ((z) < (min))		\
	       (z) = (min);			\
       } else {					\
	   if ((z) > (min))			\
	       (z) = (min);			\
	   else if ((z) < (max))		\
	       (z) = (max);			\
       }					\
    } while (0)
#endif

/* both min/max and MIN/MAX are defined by some compilers.
 * we are now on GPMIN / GPMAX
 */
#define GPMAX(a,b) ( (a) > (b) ? (a) : (b) )
#define GPMIN(a,b) ( (a) < (b) ? (a) : (b) )

/* Prototypes from "stdfn.c" */

char *safe_strncpy __PROTO((char *, const char *, size_t));
#ifndef HAVE_SLEEP
unsigned int sleep __PROTO((unsigned int));
#endif

double gp_strtod __PROTO((const char *str, char **endptr));

double not_a_number __PROTO((void));

#endif /* STDFN_H */
