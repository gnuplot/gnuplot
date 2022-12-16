/* GNUPLOT - loadpath.c */

/*[
 * Copyright 1999, 2004   Lars Hecking
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

#include <string.h>

#include "loadpath.h"
#include "alloc.h"
#include "util.h"

#define PATHSEP_TO_NUL(arg)			\
do {						\
    char *s = arg;				\
    while ((s = strchr(s, PATHSEP)) != NULL)	\
	*s++ = NUL;				\
} while (0)

#define PRINT_PATHLIST(start, limit)		\
do {						\
    char *s = start;				\
						\
    while (s < limit) {				\
	fprintf(stderr, "\"%s\" ", s);		\
	s += strlen(s) + 1;			\
    }						\
    fputc('\n',stderr);				\
} while (0)

/* The path elements are '\0' separated (!)
 * This way, reading out loadpath is very easy to implement
 */
static char *loadpath = NULL;

/* index pointer, end of loadpath,
 * env section of loadpath, current limit,
 * in that order */
static char *p, *last, *envptr, *limit;

void
init_loadpath()
{
    /* Init loadpath from environment */
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
	}
	/* else: NULL = empty */
    }
    /* point to env portion of loadpath */
    envptr = loadpath;
}

void
clear_loadpath()
{
    free(loadpath);
    loadpath = p = last = NULL;
    limit = NULL;
    init_loadpath();
}

void
set_var_loadpath( char *path )
{
    /* length of env portion */
    size_t elen = last - envptr;
    size_t plen = strlen(path);

    if (!path || !(*path))
	return;

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
    /* now move env part back to the end to make space for the new path */
    memmove(loadpath + plen + 1, loadpath, elen + 1);
    strcpy(loadpath, path);
    /* separate new path(s) and env path(s) */
    loadpath[plen] = PATHSEP;
    /* adjust pointer to env part and last */
    envptr = &loadpath[plen+1];
    last = envptr + elen;
    PATHSEP_TO_NUL(loadpath);
}

void
dump_loadpath()
{
    /* print the current, full loadpath */
    if (loadpath) {
	fputs("\tloadpath is ", stderr);
	PRINT_PATHLIST(loadpath, envptr);
	if (envptr) {
	    /* env part */
	    fputs("\tloadpath from GNUPLOT_LIB is ", stderr);
	    PRINT_PATHLIST(envptr, last);
	}
    } else
	fputs("\tloadpath is empty\n", stderr);
#ifdef GNUPLOT_SHARE_DIR
    fprintf(stderr, "\tgnuplotrc is read from %s\n", GNUPLOT_SHARE_DIR);
#endif
}

char *
save_loadpath()
{
    /* we don't save the load path taken from the environment,
     * so don't go beyond envptr when extracting the path elements
     */
    limit = envptr;
    return get_loadpath();
}

char *
get_loadpath()
{
    /* successive calls to get_loadpath() return all
     * elements of the loadpath until exhausted
     */
    if (!loadpath)
	return NULL;
    if (!p) {
	/* init section */
	p = loadpath;
	if (!limit)
	    limit = last;
    } else {
	/* skip over '\0' */
	p += strlen(p) + 1;
    }
    if (p >= limit)
	limit = p = NULL;

    return p;
}

