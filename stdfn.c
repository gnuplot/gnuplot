#ifndef lint
static char *RCSid = "$Id: $";
#endif


/* GNUPLOT - stdfn.c */

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


/* This module collects various functions, which were previously scattered
 * all over the place. In a future implementation of gnuplot, each of
 * these functions will probably reside in their own file in a subdirectory.
 * - Lars Hecking
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ansichek.h"
#include "stdfn.h"

/*
 * ANSI C functions
 */

/* memcpy() */

#ifdef NO_MEMCPY
# ifdef HAVE_BCOPY
#  define memcpy(dest,src,len) bcopy((src),(dest),(len))
# else
/*
 * cheap and slow version of memcpy() in case you don't have one 
 */
int memcpy __PROTO((char *, char *, size_t));

int
memcpy (dest, src, len)
     char *dest, *src;
     size_t len;
{
  while (len--)
    *dest++ = *src++;
}
# endif /* HAVE_BCOPY */
#endif /* NO_MEMCPY */


/* strchr()
 * Simple and portable version, conforming to Plauger.
 * Would this be more efficient as a macro?
 */
#ifdef NO_STRCHR
# ifndef HAVE_INDEX
char *strchr __PROTO((const char *, int));

char *
strchr (s, c)
     const char *s;
     int c;
{
  do {
    if (*s == (char)c)
      return s;
  } while (*s++ != (char)0);

  return NULL;
}
# endif /* !HAVE_INDEX */
#endif /* NO_STRCHR */


/* memset () 
 *
 * Since we want to use memset, we have to map a possibly nonzero fill byte
 * to the bzero function. The following defined might seem a bit odd, but I
 * think this is the only possible way.
 */

#ifdef NO_MEMSET
# ifdef HAVE_BZERO
#  define memset(s, b, l) \
do {                      \
  assert((b)==0);         \
  bzero((s), (l));        \
} while(0)
#  else
#  error You must have either memset or bzero
# endif /* HAVE_BZERO */
#endif /* NO_MEMSET */


/* strerror() */
#ifdef NO_STRERROR

extern int sys_nerr;
extern char *sys_errlist[];

char *strerror __PROTO((int));

char *strerror(no)
int no;
{
  static char res_str[30];

  if(no>sys_nerr) {
    sprintf(res_str, "unknown errno %d", no);
    return res_str;
  } else {
    return sys_errlist[no];
  }
}
#endif /* NO_STRERROR */


/* strstr() */
#ifdef NO_STRSTR
char *strstr __PROTO((const char *, const char *));

char *strstr (cs, ct)
const char *cs, *ct;
{
  size_t len;

  if (!cs || !ct)
    return NULL;

  if (!*ct)
    return cs;
  
  len = strlen(ct);
  while (*cs)
    {
      if (strncmp(cs, ct, len)==0)
	return cs;
      cs++;
    }

  return NULL;
}
#endif /* NO_STRSTR */


#ifdef __PUREC__
/*
 * a substitute for PureC's buggy sscanf.
 * this uses the normal sscanf and fixes the following bugs:
 * - whitespace in format matches whitespace in string, but doesn't
 *   require any. ( "%f , %f" scans "1,2" correctly )
 * - the ignore value feature works (*). this created an address error
 *   in PureC.
 */

#include <stdarg.h>

int purec_sscanf( const char *string, const char *format, ... )
{
  va_list args;
  int cnt=0;
  char onefmt[256];
  char buffer[256];
  const char *f=format;
  const char *s=string;
  char *f2;
  char ch;
  int ignore;
  void *p;
  int *ip;
  int pos;

  va_start(args,format);
  while( *f && *s ) {
    ch=*f++;
    if( ch!='%' ) {
      if(isspace(ch)) {
        /* match any number of whitespace */
        while(isspace(*s)) s++;
      } else {
        /* match exactly the character ch */
        if( *s!=ch ) goto finish;
        s++;
      }
    } else {
      /* we have got a '%' */
      ch=*f++;
      if( ch=='%' ) {
        /* match exactly % */
        if( *s!=ch ) goto finish;
        s++;
      } else {
        f2=onefmt;
        *f2++='%';
        *f2++=ch;
        ignore=0;
        if( ch=='*' ) {
          ignore=1;
          ch=f2[-1]=*f++;
        }
        while( isdigit(ch) ) {
          ch=*f2++=*f++;
        }
        if( ch=='l' || ch=='L' || ch=='h' ) {
          ch=*f2++=*f++;
        }
        switch(ch) {
          case '[':
            while( ch && ch!=']' ) {
              ch=*f2++=*f++;
            }
            if( !ch ) goto error;
            break;
          case 'e':
          case 'f':
          case 'g':
          case 'd':
          case 'o':
          case 'i':
          case 'u':
          case 'x':
          case 'c':
          case 's':
          case 'p':
          case 'n': /* special case handled below */
            break;
          default:
            goto error;
        }
        if( ch!='n' ) {
          strcpy(f2,"%n");
          if( ignore ) {
            p=buffer;
          } else {
            p=va_arg(args,void *);
          }
          switch( sscanf( s, onefmt, p, &pos ) ) {
            case EOF: goto error;
            case  0 : goto finish;
          }
          if( !ignore ) cnt++;
          s+=pos;
        } else {
          if( !ignore ) {
            ip=va_arg(args,int *);
            *ip=(int)(s-string);
          }
        }
      }
    }
  }

  if( !*f ) goto finish;

error:
  cnt=EOF;
finish:
  va_end(args);
  return cnt;
}

/* use the substitute now. I know this is dirty trick, but it works. */
#define sscanf purec_sscanf

#endif /* __PUREC__ */


/*
 * POSIX functions
 */

#ifndef HAVE_SLEEP
/* The implementation below does not even come close
   to what is required by POSIX.1, but I suppose
   it doesn't really matter on these systems. lh
 */

unsigned int sleep __PROTO((unsigned int));

#ifdef AMIGA_SC_6_1
#include <proto/dos.h>
#endif

#ifdef WIN32
/* the WIN32 API has a Sleep function that does not consume CPU cycles */
#include <windows.h>
#endif

unsigned int
sleep(delay)
    unsigned int delay;
{
#if defined(MSDOS) || defined(_Windows) || defined(DOS386) || defined(AMIGA_AC_5)
#if !(defined(__TURBOC__) || defined(__EMX__) || defined(DJGPP)) || defined(_Windows) /* Turbo C already has sleep() */
/* kludge to provide sleep() for msc 5.1 */
    unsigned long   time_is_up;

    time_is_up = time(NULL) + (unsigned long) delay;
    while (time(NULL) < time_is_up)
	 /* wait */ ;
#endif /* !__TURBOC__ ... */
#endif /* MSDOS ... */

#ifdef AMIGA_SC_6_1
    Delay(50 * delay);
#endif

#ifdef WIN32
    Sleep( (DWORD) delay*1000 );
#endif

    return (unsigned int)0;
}
#endif                          /* HAVE_SLEEP */


/*
 * Other common functions
 */

/*****************************************************************
    portable implementation of strnicmp (hopefully)
*****************************************************************/

#ifndef HAVE_STRNICMP
# ifndef HAVE_STRNCASECMP
int strnicmp __PROTO((char *, char *, int));

int strnicmp (s1, s2, n)
char *s1;
char *s2;
int n;
{
    char c1,c2;

    if(n==0) return 0;

    do {
	c1 = *s1++; if(islower(c1)) c1=toupper(c1);
	c2 = *s2++; if(islower(c2)) c2=toupper(c2);
    } while(c1==c2 && c1 && c2 && --n>0);

    if(n==0 || c1==c2) return 0;
    if(c1=='\0' || c1<c2) return 1;
    return -1;
}
# endif /* !HAVE_STRNCASECMP */
#endif /* !HAVE_STRNICMP */
