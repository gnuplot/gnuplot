#ifndef lint
static char *RCSid() { return RCSid("$Id: variable.c,v 1.12.4.2 2000/07/26 20:35:56 broeker Exp $"); }
#endif

/* GNUPLOT - variable.c */

/*[
 * Copyright 1999   Lars Hecking
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

/* The Death of Global Variables - part one. */

#include "variable.h"

#include "alloc.h"
#include "command.h"
#include "util.h"


#define PATHSEP_TO_NUL(arg) \
{ char *s = arg; \
  while ((s=strchr(s,PATHSEP)) != NULL) \
    *s++ = NUL; }

#define PRINT_LOADPATH(limit) \
  while (p < limit) { \
    fprintf(stderr, "\"%s\" ",p); \
    p += strlen(p) + 1; \
  } \
  fputc('\n',stderr);

/*
 * char *loadpath_handler (int, char *)
 *
 */
char *
loadpath_handler(action, path)
int action;
char *path;
{
    /* loadpath variable
     * the path elements are '\0' separated (!)
     * this way, reading out loadpath is very
     * easy to implement */
    static char *loadpath;
    /* index pointer, end of loadpath,
     * env section of loadpath, current limit, in that order */
    static char *p, *last, *envptr, *limit;
    static int beenhere;

    switch (action) {
    case ACTION_CLEAR:
	/* Clear loadpath, fall through to init */
	FPRINTF((stderr, "Clear loadpath\n"));
	free(loadpath);
	loadpath = p = last = NULL;
	/* HBB 20000726: 'limit' has to be initialized to NULL, too! */
	limit = NULL;
    case ACTION_INIT:
	/* Init loadpath from environment */
	FPRINTF((stderr, "Init loadpath from environment\n"));
	assert(loadpath == NULL);
	if (!loadpath)
	{
	    char *envlib = getenv("GNUPLOT_LIB");
	    if (envlib) {
		int len = strlen(envlib);
		loadpath = gp_strdup(envlib);
		/* point to end of loadpath */
		last = loadpath + len;
		/* convert all PATHSEPs to \0 */
		PATHSEP_TO_NUL(loadpath);
	    }			/* else: NULL = empty */
	}			/* else: already initialised; int_warn (?) */
	/* point to env portion of loadpath */
	envptr = loadpath;
	break;
    case ACTION_SET:
	/* set the loadpath */
	FPRINTF((stderr, "Set loadpath\n"));
	if (path && *path != NUL) {
	    /* length of env portion */
	    size_t elen = last - envptr;
	    size_t plen = strlen(path);
	    if (loadpath && envptr) {
		/* we are prepending a path name; because
		 * realloc() preserves only the contents up
		 * to the minimum of old and new size, we move
		 * the part to be preserved to the beginning
		 * of the string; use memmove() because strings
		 * may overlap */
		memmove(loadpath, envptr, elen + 1);
	    }
	    loadpath = gp_realloc(loadpath, elen + 1 + plen + 1, "expand loadpath");
	    /* now move env part back to the end to make space for
	     * the new path */
	    memmove(loadpath + plen + 1, loadpath, elen + 1);
	    strcpy(loadpath, path);
	    /* separate new path(s) and env path(s) */
	    loadpath[plen] = PATHSEP;
	    /* adjust pointer to env part and last */
	    envptr = &loadpath[plen+1];
	    last = envptr + elen;
	    PATHSEP_TO_NUL(loadpath);
	}			/* else: NULL = empty */
	break;
    case ACTION_SHOW:
	/* print the current, full loadpath */
	FPRINTF((stderr, "Show loadpath\n"));
	if (loadpath) {
	    p = loadpath;
	    fputs("\tloadpath is ", stderr);
	    PRINT_LOADPATH(envptr);
	    if (envptr) {
		/* env part */
		p = envptr;
		fputs("\tsystem loadpath is ", stderr);
		PRINT_LOADPATH(last);
	    }
	} else
	    fputs("\tloadpath is empty\n", stderr);
	break;
    case ACTION_SAVE:
	/* we don't save the load path taken from the
	 * environment, so don't go beyond envptr when
	 * extracting the path elements
	 */
	limit = envptr;
    case ACTION_GET:
	/* subsequent calls to get_loadpath() return all
	 * elements of the loadpath until exhausted
	 */
	FPRINTF((stderr, "Get loadpath\n"));
	if (!loadpath)
	    return NULL;
	if (!beenhere) {
	    /* init section */
	    beenhere = 1;
	    p = loadpath;
	    if (!limit)
		limit = last;
	    return p;
	} else {
	    p += strlen(p);
	    /* skip over '\0' */
	    p++;
	    if (p < limit)
		return p;
	    else {
		beenhere = 0;
		return NULL;
	    }
	}
	break;
    case ACTION_NULL:
	/* just return */
    default:
	break;
    }

    /* should always be ignored - points to the
     * first path in the list */
    return loadpath;

}

/* not set or shown directly, but controlled by 'set locale'
 * defined in national.h
 */

char full_month_names[12][32] =
{ FMON01, FMON02, FMON03, FMON04, FMON05, FMON06, FMON07, FMON08, FMON09, FMON10, FMON11, FMON12 };
char abbrev_month_names[12][8] =
{ AMON01, AMON02, AMON03, AMON04, AMON05, AMON06, AMON07, AMON08, AMON09, AMON10, AMON11, AMON12 };

char full_day_names[7][32] =
{ FDAY0, FDAY1, FDAY2, FDAY3, FDAY4, FDAY5, FDAY6 };
char abbrev_day_names[7][8] =
{ ADAY0, ADAY1, ADAY2, ADAY3, ADAY4, ADAY5, ADAY6 };

char *
locale_handler(action, newlocale)
int action;
char *newlocale;
{
    static char *current_locale = NULL;
    struct tm tm;
    int i;

    switch(action) {
    case ACTION_CLEAR:
    case ACTION_INIT:
	if (current_locale) free(current_locale);
	current_locale = gp_strdup(INITIAL_LOCALE);
	break;
    case ACTION_SET:
#ifndef NO_LOCALE_H
	if (setlocale(LC_TIME, newlocale)) {
	    current_locale = gp_realloc(current_locale, strlen(newlocale) + 1, "locale");
	    strcpy(current_locale, newlocale);
	}
	else
	    int_error(c_token, "Locale not available");

	/* we can do a *lot* better than this ; eg use system functions
	 * where available; create values on first use, etc
	 */
	memset(&tm, 0, sizeof(struct tm));
	for (i = 0; i < 7; ++i) {
	    tm.tm_wday = i;		/* hope this enough */
	    strftime(full_day_names[i], sizeof(full_day_names[i]), "%A", &tm);
	    strftime(abbrev_day_names[i], sizeof(abbrev_day_names[i]), "%a", &tm);
	}
	for (i = 0; i < 12; ++i) {
	    tm.tm_mon = i;		/* hope this enough */
	    strftime(full_month_names[i], sizeof(full_month_names[i]), "%B", &tm);
	    strftime(abbrev_month_names[i], sizeof(abbrev_month_names[i]), "%b", &tm);
	}
#else
	current_locale = gp_realloc(current_locale, strlen(newlocale) + 1, "locale");
	strcpy(current_locale, newlocale);
#endif /* NO_LOCALE_H */
	break;
    case ACTION_SHOW:
	fprintf(stderr, "\tlocale is \"%s\"\n", current_locale);
	break;
    case ACTION_SAVE:
    case ACTION_GET:
    case ACTION_NULL:
    default:
	/* just return */
	break;
    }

    return current_locale;
}

