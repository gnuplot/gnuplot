#ifndef lint
static char *RCSid() { return RCSid("$Id: corplot.c,v 1.3.4.1 2000/06/22 12:57:38 broeker Exp $"); }
#endif

/* GNUPLOT - corplot.c */

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


#include <stdio.h>
#include <process.h>
#include <dos.h>
#if (defined(ATARI) || defined(MTOS)) && defined(__PUREC__)
#include "syscfg.h"
#endif

#define BOUNDARY 32768
#define segment(addr) (FP_SEG(m) + ((FP_OFF(m)+15) >> 4));
#define round(value,boundary) (((value) + (boundary) - 1) & ~((boundary) - 1))

char *malloc(), *realloc();

char prog[] = "gnuplot";
char corscreen[] = "CORSCREEN=0";

int
main()
{
    register unsigned int segm, start;
    char *m;
    if (!(m = malloc(BOUNDARY))) {
	printf("malloc() failed\n");
	exit(1);
    }
    segm = segment(m);
    start = round(segm, BOUNDARY / 16);

    if (realloc(m, BOUNDARY + (start - segm) * 16) != m) {
	printf("can't realloc() memory\n");
	exit(2);
    }
    if ((segm = start >> 11) >= 8) {
	printf("not enough room in first 256K\n");
	exit(3);
    }
    corscreen[sizeof(corscreen) - 2] = '0' + segm;
    if (putenv(corscreen))
	perror("putenv");

    if (spawnlp(P_WAIT, prog, prog, NULL))
	perror("spawnlp");
}
