/*
 * $Id: parse.h,v 1.3.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - parse.h */

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

#ifndef PARSE_H
# define PARSE_H

#include "plot.h"

enum operators {
    /* keep this in line with table in eval.c */
    PUSH, PUSHC, PUSHD1, PUSHD2, PUSHD, CALL, CALLN, LNOT, BNOT, UMINUS,
    LOR, LAND, BOR, XOR, BAND, EQ, NE, GT, LT, GE, LE, PLUS, MINUS, MULT,
    DIV, MOD, POWER, FACTORIAL, BOOLE,
    DOLLARS, /* for using extension - div */
    /* only jump operators go between jump and sf_start */
    JUMP, JUMPZ, JUMPNZ, JTERN, SF_START
};

/* action table entry */
struct at_entry {
    enum operators index;	/* index of p-code function */
    union argument arg;
};

struct at_type {
    /* count of entries in .actions[] */
    int a_count;
    /* will usually be less than MAX_AT_LEN is malloc()'d copy */
    struct at_entry actions[MAX_AT_LEN];
};


/* void fpe __PROTO((void)); */
void evaluate_at __PROTO((struct at_type *at_ptr, struct value *val_ptr));
struct value * const_express __PROTO((struct value *valptr));
struct at_type * temp_at __PROTO((void));
struct at_type * perm_at __PROTO((void));

#endif /* PARSE_H */
