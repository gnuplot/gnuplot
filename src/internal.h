/*
 * $Id: internal.h,v 1.2.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - internal.h */

/*[
 * Copyright 1999   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_INTERNAL_H
# define GNUPLOT_INTERNAL_H

/* #if... / #include / #define collection: */

#include "plot.h"

#define STACK_DEPTH 100		/* maximum size of the execution stack */

/* Variables of internal.c needed by other modules: */

/* Prototypes from file "internal.c" */

#ifdef MINEXP
double gp_exp __PROTO((double x));
#else
#define gp_exp(x) exp(x)
#endif

void reset_stack __PROTO((void));
void check_stack __PROTO((void));
struct value *pop __PROTO((struct value *x));
void push __PROTO((struct value *x));

/* the basic operators of our stack machine for function evaluation: */
void f_push __PROTO((union argument *x));
void f_pushc __PROTO((union argument *x));
void f_pushd1 __PROTO((union argument *x));
void f_pushd2 __PROTO((union argument *x));
void f_pushd __PROTO((union argument *x));
void f_call __PROTO((union argument *x));
void f_calln __PROTO((union argument *x));
void f_lnot __PROTO((void));
void f_bnot __PROTO((void));
void f_bool __PROTO((void));
void f_lor __PROTO((void));
void f_land __PROTO((void));
void f_bor __PROTO((void));
void f_xor __PROTO((void));
void f_band __PROTO((void));
void f_uminus __PROTO((void));
void f_eq __PROTO((void));
void f_ne __PROTO((void));
void f_gt __PROTO((void));
void f_lt __PROTO((void));
void f_ge __PROTO((void));
void f_le __PROTO((void));
void f_plus __PROTO((void));
void f_minus __PROTO((void));
void f_mult __PROTO((void));
void f_div __PROTO((void));
void f_mod __PROTO((void));
void f_power __PROTO((void));
void f_factorial __PROTO((void));
int f_jump __PROTO((union argument *x));
int f_jumpz __PROTO((union argument *x));
int f_jumpnz __PROTO((union argument *x));
int f_jtern __PROTO((union argument *x));

#endif /* GNUPLOT_INTERNAL_H */
