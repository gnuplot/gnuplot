/*
 * $Id: stdfn.h,v 1.20 1997/07/22 23:20:50 drd Exp $
 *
 */

/* get prototypes or declarations for string and stdlib functions and deal
   with missing functions like strchr. */

/* we will assume the ANSI/Posix/whatever situation as default.
   the header file is called string.h and the index functions are called
   strchr, strrchr. Exceptions have to be listed explicitly */

#ifndef STDFN_H
#define STDFN_H

#include <stdio.h>

#ifdef sequent
#define NO_STRCHR
#endif

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef NO_STRCHR
#ifdef strchr
#undef strchr
#endif
#define strchr index
#ifdef strrchr
#undef strrchr
#endif
#define strrchr rindex
#endif

#ifdef NO_STDLIB_H
char *malloc();
char *realloc();
char *getenv();
int system();
double atof();
int atoi();
long atol();
double strtod();
#else
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#ifdef HAVE_LIBC_H /* NeXT uses libc instead of unistd */
#include <libc.h>
#endif
#endif

#ifndef NO_ERRNO_H
#include <errno.h>
#endif
#ifdef EXTERN_ERRNO
extern int errno;
#endif

#ifndef NO_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef NO_LIMITS_H
#include <limits.h>
#else
#ifdef HAVE_VALUES_H
#include <values.h>
#endif
#endif

#ifdef NO_TIME_H
#ifndef time_t /* should be #defined by config.h, then... */
#define time_t long
#endif
#else /* OK, have time.h: */
#include <time.h> /* ctime etc, should also define time_t and struct tm */
#endif

#if defined(PIPES) && (defined(VMS) || (defined(OSK) && defined(_ANSI_EXT))) || defined(PIPES) && defined(AMIGA_SC_6_1)
FILE *popen __PROTO((char *, char *));
int pclose __PROTO((FILE *));
#endif

#ifndef NO_LOCALE_H
#include <locale.h>
#endif

#ifndef HAVE_STRNICMP
#  ifdef HAVE_STRNCASECMP
#    define strnicmp strncasecmp
#  else
#    define NEED_STRNICMP
int strnicmp __PROTO((char *s1, char *s2, int n));
#  endif
#endif


#ifndef GP_GETCWD
# ifdef OS2
#  define GP_GETCWD(path,len) _getcwd2 (path, len)
# else
#  if defined(HAVE_GETCWD)
#   define GP_GETCWD(path,len) getcwd (path, len)
#  else
#   define GP_GETCWD(path,len) getwd (path)
#  endif
# endif
#endif


#endif /* STDFN_H */
