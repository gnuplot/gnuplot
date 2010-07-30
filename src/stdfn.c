#ifndef lint
static char *RCSid() { return RCSid("$Id: stdfn.c,v 1.19 2010/03/14 18:01:46 sfeam Exp $"); }
#endif

/* GNUPLOT - stdfn.c */

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


/* This module collects various functions, which were previously scattered
 * all over the place. In a future implementation of gnuplot, each of
 * these functions will probably reside in their own file in a subdirectory.
 * - Lars Hecking
 */

#include "stdfn.h"

#ifdef WIN32
/* the WIN32 API has a Sleep function that does not consume CPU cycles */
#include <windows.h>
#endif


/*
 * ANSI C functions
 */

/* memcpy() */

#ifndef HAVE_MEMCPY
# ifndef HAVE_BCOPY
/*
 * cheap and slow version of memcpy() in case you don't have one
 */

char *
memcpy(char *dest, char *src, size_t len)
{
    while (len--)
	*dest++ = *src++;

    return dest;
}
# endif				/* !HAVE_BCOPY */
#endif /* HAVE_MEMCPY */

/* strchr()
 * Simple and portable version, conforming to Plauger.
 * Would this be more efficient as a macro?
 */
#ifndef HAVE_STRCHR
# ifndef HAVE_INDEX

char *
strchr(const char *s, int c)
{
    do {
	if (*s == (char) c)
	    return s;
    } while (*s++ != (char) 0);

    return NULL;
}
# endif				/* !HAVE_INDEX */
#endif /* HAVE_STRCHR */


/* memset ()
 *
 * Since we want to use memset, we have to map a possibly nonzero fill byte
 * to the bzero function. The following defined might seem a bit odd, but I
 * think this is the only possible way.
 */

#ifndef HAVE_MEMSET
# ifdef HAVE_BZERO
#  define memset(s, b, l) \
do {                      \
  assert((b)==0);         \
  bzero((s), (l));        \
} while(0)
#  else
#  define memset NO_MEMSET_OR_BZERO
# endif /* HAVE_BZERO */
#endif /* HAVE_MEMSET */


/* strerror() */
#ifndef HAVE_STRERROR

char *
strerror(int no)
{
    static char res_str[30];

    if (no > sys_nerr) {
	sprintf(res_str, "unknown errno %d", no);
	return res_str;
    } else {
	return sys_errlist[no];
    }
}
#endif /* HAVE_STRERROR */


/* strstr() */
#ifndef HAVE_STRSTR

char *
strstr(const char *cs, const char *ct)
{
    size_t len;

    if (!cs || !ct)
	return NULL;

    if (!*ct)
	return (char *) cs;

    len = strlen(ct);
    while (*cs) {
	if (strncmp(cs, ct, len) == 0)
	    return (char *) cs;
	cs++;
    }

    return NULL;
}
#endif /* HAVE_STRSTR */


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

int
purec_sscanf(const char *string, const char *format,...)
{
    va_list args;
    int cnt = 0;
    char onefmt[256];
    char buffer[256];
    const char *f = format;
    const char *s = string;
    char *f2;
    char ch;
    int ignore;
    void *p;
    int *ip;
    int pos;

    va_start(args, format);
    while (*f && *s) {
	ch = *f++;
	if (ch != '%') {
	    if (isspace((unsigned char) ch)) {
		/* match any number of whitespace */
		while (isspace((unsigned char) *s))
		    s++;
	    } else {
		/* match exactly the character ch */
		if (*s != ch)
		    goto finish;
		s++;
	    }
	} else {
	    /* we have got a '%' */
	    ch = *f++;
	    if (ch == '%') {
		/* match exactly % */
		if (*s != ch)
		    goto finish;
		s++;
	    } else {
		f2 = onefmt;
		*f2++ = '%';
		*f2++ = ch;
		ignore = 0;
		if (ch == '*') {
		    ignore = 1;
		    ch = f2[-1] = *f++;
		}
		while (isdigit((unsigned char) ch)) {
		    ch = *f2++ = *f++;
		}
		if (ch == 'l' || ch == 'L' || ch == 'h') {
		    ch = *f2++ = *f++;
		}
		switch (ch) {
		case '[':
		    while (ch && ch != ']') {
			ch = *f2++ = *f++;
		    }
		    if (!ch)
			goto error;
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
		case 'n':	/* special case handled below */
		    break;
		default:
		    goto error;
		}
		if (ch != 'n') {
		    strcpy(f2, "%n");
		    if (ignore) {
			p = buffer;
		    } else {
			p = va_arg(args, void *);
		    }
		    switch (sscanf(s, onefmt, p, &pos)) {
		    case EOF:
			goto error;
		    case 0:
			goto finish;
		    }
		    if (!ignore)
			cnt++;
		    s += pos;
		} else {
		    if (!ignore) {
			ip = va_arg(args, int *);
			*ip = (int) (s - string);
		    }
		}
	    }
	}
    }

    if (!*f)
	goto finish;

  error:
    cnt = EOF;
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
 * to what is required by POSIX.1, but I suppose
 * it doesn't really matter on these systems. lh
 */


unsigned int
sleep(unsigned int delay)
{
#if defined(MSDOS) || defined(_Windows)
# if !(defined(__TURBOC__) || defined(__EMX__) || defined(DJGPP)) || defined(_Windows)	/* Turbo C already has sleep() */
    /* kludge to provide sleep() for msc 5.1 */
    unsigned long time_is_up;

    time_is_up = time(NULL) + (unsigned long) delay;
    while (time(NULL) < time_is_up)
	/* wait */ ;
# endif /* !__TURBOC__ ... */
#endif /* MSDOS ... */

#ifdef WIN32
    Sleep((DWORD) delay * 1000);
#endif

    return 0;
}

#endif /* HAVE_SLEEP */


/*
 * Other common functions
 */

/*****************************************************************
    portable implementation of strnicmp (hopefully)
*****************************************************************/
#ifndef HAVE_STRCASECMP
# ifndef HAVE_STRICMP

/* return (see MSVC documentation and strcasecmp()):
 *  -1  if str1 < str2
 *   0  if str1 == str2
 *   1  if str1 > str2
 */
int
gp_stricmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;

    do {
	c1 = *s1++;
	if (islower(c1))
	    c1 = toupper(c1);
	c2 = *s2++;
	if (islower(c2))
	    c2 = toupper(c2);
    } while (c1 == c2 && c1 && c2);

    if (c1 == c2)
	return 0;
    if (c1 == '\0' || c1 > c2)
	return 1;
    return -1;
}
# endif				/* !HAVE_STRCASECMP */
#endif /* !HAVE_STRNICMP */

/*****************************************************************
    portable implementation of strnicmp (hopefully)
*****************************************************************/

#ifndef HAVE_STRNCASECMP
# ifndef HAVE_STRNICMP

int
gp_strnicmp(const char *s1, const char *s2, size_t n)
{
    unsigned char c1, c2;

    if (n == 0)
	return 0;

    do {
	c1 = *s1++;
	if (islower(c1))
	    c1 = toupper(c1);
	c2 = *s2++;
	if (islower(c2))
	    c2 = toupper(c2);
    } while (c1 == c2 && c1 && c2 && --n > 0);

    if (n == 0 || c1 == c2)
	return 0;
    if (c1 == '\0' || c1 > c2)
	return 1;
    return -1;
}
# endif				/* !HAVE_STRNCASECMP */
#endif /* !HAVE_STRNICMP */


/* Safe, '\0'-terminated version of strncpy()
 * safe_strncpy(dest, src, n), where n = sizeof(dest)
 * This is basically the old fit.c(copy_max) function
 */
char *
safe_strncpy(char *d, const char *s, size_t n)
{
    char *ret;

    ret = strncpy(d, s, n);
    if (strlen(s) >= n)
	d[n > 0 ? n - 1 : 0] = NUL;

    return ret;
}

#ifndef HAVE_STRCSPN
/*
 * our own substitute for strcspn()
 * return the length of the inital segment of str1
 * consisting of characters not in str2
 * returns strlen(str1) if none of the characters
 * from str2 are in str1
 * based in misc.c(instring) */
size_t
gp_strcspn(const char *str1, const char *str2)
{
    char *s;
    size_t pos;

    if (!str1 || !str2)
	return 0;
    pos = strlen(str1);
    while (*str2++)
	if (s = strchr(str1, *str2))
	    if ((s - str1) < pos)
		pos = s - str1;
    return (pos);
}
#endif /* !HAVE_STRCSPN */

double
gp_strtod(const char *str, char **endptr)
{
#if (0)  /* replace with test for platforms with broken strtod() */
    int used;
    double d;
    int n = sscanf(str, "%lf%n", &d, &used);
    if (n < 1)
	*endptr = (char *)str;
    else
	*endptr = (char *)(str + used);
    return d;
#else
    return strtod(str,endptr);
#endif
}

/* Implement portable generation of a NaN value. */

double
not_a_number(void)
{
#ifdef __MSC__
	unsigned long lnan[2]={0xffffffff, 0x7fffffff};
    return *( double* )lnan;
#else
	return atof("NaN");
#endif
}
