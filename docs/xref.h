/* GNUPLOT - xref.h */

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

#ifndef DOCS_XREF_H
# define DOCS_XREF_H

/*
 * this file is included from xref.c
 *
 */

struct LIST {
    int level;
    int line;
    char *string;
    struct LIST *next;
    struct LIST *prev;
};

#ifdef DOCS_XREF_MAIN
# define EXTERN /* nought */
#else
# define EXTERN extern
#endif

EXTERN void parse(FILE * a);
EXTERN struct LIST *lookup(char *);
EXTERN struct LIST *lkup_by_number(int line);
EXTERN void list_free(void);
EXTERN void refs(int l, FILE * f, char *start, char *end, char *format);

void *xmalloc(size_t size);

#endif /* DOCS_XREF_H */
