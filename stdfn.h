/*
 * $Id: stdfn.h,v 1.23 1998/06/18 14:55:18 ddenholm Exp $
 *
 */

/* GNUPLOT - stdfn.h */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
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

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef NO_STRCHR
#ifdef strchr
#undef strchr
#endif
#ifdef HAVE_INDEX
#define strchr index
#endif
#ifdef strrchr
#undef strrchr
#endif
#ifdef HAVE_RINDEX
#define strrchr rindex
#endif
#endif

#ifdef NO_STDLIB_H
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
#else /* !NO_STDLIB_H */
# include <stdlib.h>
/* need to find out about VMS */
# ifndef VMS
#  ifndef EXIT_FAILURE
#   define EXIT_FAILURE (1)
#  endif
#  ifndef EXIT_SUCCESS
#   define EXIT_SUCCESS (0)
#  endif
# else /* VMS */
#  ifndef EXIT_FAILURE
#   define  EXIT_FAILURE  0x10000002
# endif
#  ifndef EXIT_SUCCESS
#   define  EXIT_SUCCESS  0
#  endif
# endif /* VMS */
#endif /* !NO_STDLIB_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
# ifdef HAVE_LIBC_H /* NeXT uses libc instead of unistd */
#  include <libc.h>
# endif
# ifdef VMS
/* prototype for sleep() */
#  include <signal.h>
# endif
#endif /* HAVE_UNISTD_H */

#if VMS
# ifndef HAVE_SLEEP
#  define HAVE_SLEEP
# endif
#endif

#ifndef NO_ERRNO_H
# include <errno.h>
#endif
# ifdef EXTERN_ERRNO
extern int errno;
#endif

#ifndef NO_SYS_TYPES_H
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

#ifndef NO_LIMITS_H
# include <limits.h>
#else
# ifdef HAVE_VALUES_H
#  include <values.h>
# endif /* HAVE_VALUES_H */
#endif /* !NO_LIMITS_H */

#ifdef NO_TIME_H
# ifndef time_t /* should be #defined by config.h, then... */
#  define time_t long
# endif
#else
# include <time.h> /* ctime etc, should also define time_t and struct tm */
#endif

#if defined(PIPES) && (defined(VMS) || (defined(OSK) && defined(_ANSI_EXT))) || defined(PIPES) && defined(AMIGA_SC_6_1)
FILE *popen __PROTO((char *, char *));
int pclose __PROTO((FILE *));
#endif

#ifndef NO_FLOAT_H
# include <float.h>
#endif

#ifndef NO_LOCALE_H
# include <locale.h>
#endif

#ifndef NO_MATH_H
# include <math.h>
#endif

#ifndef HAVE_STRNICMP
# ifdef HAVE_STRNCASECMP
#  define strnicmp strncasecmp
# else
int strnicmp __PROTO((char *, char *, int));
# endif
#endif

/* Argument types for select() */
#ifdef SELECT_ARGTYPE_1
# define gp_nfds_t SELECT_ARGTYPE_1
#else
# define gp_nfds_t int
#endif /* 1 */
#ifdef SELECT_ARGTYPE_234
# define gp_fd_set_p SELECT_ARGTYPE_234
#else
# ifndef __EMX__
#  define gp_fd_set_p (int *)
# else
#  define gp_fd_set_p (fd_set *)
# endif
#endif /* 234 */
#ifdef SELECT_ARGTYPE_5
# define gp_timeval_p SELECT_ARGTYPE_5
#else
# define gp_timeval_p (struct timeval *)
#endif /* 5 */

#ifndef GP_GETCWD
# if defined(HAVE_GETCWD)
#  define GP_GETCWD(path,len) getcwd (path, len)
# else
#  define GP_GETCWD(path,len) getwd (path)
# endif
#endif

#ifndef GP_SLEEP
# ifdef __ZTC__
#  define GP_SLEEP(delay) usleep ((unsigned long) (delay))
# else
#  define GP_SLEEP(delay) sleep ((unsigned int) (delay))
# endif
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

#endif /* STDFN_H */
