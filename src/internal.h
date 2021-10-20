/* GNUPLOT - internal.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

#include "syscfg.h"
#include "gp_types.h"
#include "eval.h"

/* Prototypes from file "internal.c" */
void eval_reset_after_error(void);

/* the basic operators of our stack machine for function evaluation: */
void f_push(union argument *x);
void f_pushc(union argument *x);
void f_pushd1(union argument *x);
void f_pushd2(union argument *x);
void f_pushd(union argument *x);
void f_pop(union argument *x);
void f_call(union argument *x);
void f_calln(union argument *x);
void f_sum(union argument *x);
void f_lnot(union argument *x);
void f_bnot(union argument *x);
void f_lor(union argument *x);
void f_land(union argument *x);
void f_bor(union argument *x);
void f_xor(union argument *x);
void f_band(union argument *x);
void f_uminus(union argument *x);
void f_nop(union argument *x);
void f_eq(union argument *x);
void f_ne(union argument *x);
void f_gt(union argument *x);
void f_lt(union argument *x);
void f_ge(union argument *x);
void f_le(union argument *x);
void f_leftshift(union argument *x);
void f_rightshift(union argument *x);
void f_plus(union argument *x);
void f_minus(union argument *x);
void f_mult(union argument *x);
void f_div(union argument *x);
void f_mod(union argument *x);
void f_power(union argument *x);
void f_factorial(union argument *x);

void f_concatenate(union argument *x);
void f_eqs(union argument *x);
void f_nes(union argument *x);
void f_gprintf(union argument *x);
void f_range(union argument *x);
void f_index(union argument *x);
void f_cardinality(union argument *x);
void f_sprintf(union argument *x);
void f_strlen(union argument *x);
void f_strstrt(union argument *x);
void f_system(union argument *x);
void f_word(union argument *x);
void f_words(union argument *x);
void f_strftime(union argument *x);
void f_strptime(union argument *x);
void f_time(union argument *x);
void f_assign(union argument *x);
void f_value(union argument *x);
void f_trim(union argument *x);

#endif /* GNUPLOT_INTERNAL_H */
