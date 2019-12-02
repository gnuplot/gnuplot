/* GNUPLOT - alloc.c */

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

/*
 * AUTHORS
 *
 * Alexander Lehmann (collected functions from misc.c and binary.c)
 *
 */

#include "alloc.h"
#include "util.h"	/* for int_error() */

/* gp_alloc:
 * allocate memory
 * This is a protected version of malloc. It causes an int_error
 * if there is not enough memory. If message is NULL, we allow NULL return.
 * Otherwise, we handle the error, using the message to create the int_error string.
 * Note cp/sp_extend uses realloc, so it depends on this using malloc().
 */

generic *
gp_alloc(size_t size, const char *message)
{
    char *p;			/* the new allocation */

	p = malloc(size);	/* try again */
	if (p == NULL) {
	    /* really out of memory */
	    if (message != NULL) {
		int_error(NO_CARET, "out of memory for %s", message);
		/* NOTREACHED */
	    }
	    /* else we return NULL */
	}

    return (p);
}

/*
 * note gp_realloc assumes that failed realloc calls leave the original mem
 * block allocated. If this is not the case with any C compiler, a substitute
 * realloc function has to be used.
 */

generic *
gp_realloc(generic *p, size_t size, const char *message)
{
    char *res;			/* the new allocation */

    /* realloc(NULL,x) is meant to do malloc(x), but doesn't always */
    if (!p)
	return gp_alloc(size, message);

    res = realloc(p, size);
    if (res == (char *) NULL) {
	if (message != NULL) {
	    int_error(NO_CARET, "out of memory for %s", message);
	    /* NOTREACHED */
	}
	/* else we return NULL */
    }

    return (res);
}

