/* $Id: fnproto.h,v 1.9 1998/03/22 22:31:32 drd Exp $ */

/* GNUPLOT - fnproto.h */

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


/* prototypes for gnuplot function primitives. These functions are not called
   directly, only via a function table.
*/

/* Prototypes from file "internal.c" */

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

/* Prototypes from file "standard.c" */

void f_real __PROTO((void));
void f_imag __PROTO((void));
void f_int __PROTO((void));
void f_arg __PROTO((void));
void f_conjg __PROTO((void));
void f_sin __PROTO((void));
void f_cos __PROTO((void));
void f_tan __PROTO((void));
void f_asin __PROTO((void));
void f_acos __PROTO((void));
void f_atan __PROTO((void));
void f_atan2 __PROTO((void));
void f_sinh __PROTO((void));
void f_cosh __PROTO((void));
void f_tanh __PROTO((void));
void f_asinh __PROTO((void));
void f_acosh __PROTO((void));
void f_atanh __PROTO((void));
void f_void __PROTO((void));
void f_abs __PROTO((void));
void f_sgn __PROTO((void));
void f_sqrt __PROTO((void));
void f_exp __PROTO((void));
void f_log10 __PROTO((void));
void f_log __PROTO((void));
void f_floor __PROTO((void));
void f_ceil __PROTO((void));
void f_besj0 __PROTO((void));
void f_besj1 __PROTO((void));
void f_besy0 __PROTO((void));
void f_besy1 __PROTO((void));

void f_tmsec __PROTO((void));
void f_tmmin __PROTO((void));
void f_tmhour __PROTO((void));
void f_tmmday __PROTO((void));
void f_tmmon __PROTO((void));
void f_tmyear __PROTO((void));
void f_tmwday __PROTO((void));
void f_tmyday __PROTO((void));

/* Prototypes from file "specfun.c" */

void f_erf __PROTO((void));
void f_erfc __PROTO((void));
void f_ibeta __PROTO((void));
void f_igamma __PROTO((void));
void f_gamma __PROTO((void));
void f_lgamma __PROTO((void));
void f_rand __PROTO((void));
void f_normal __PROTO((void));
void f_inverse_normal __PROTO((void));
void f_inverse_erf __PROTO((void));

/* prototypes from file "datafile.c" */

void f_dollars __PROTO((union argument *x));
void f_column  __PROTO((void));
void f_valid   __PROTO((void));
void f_timecolumn   __PROTO((void));
