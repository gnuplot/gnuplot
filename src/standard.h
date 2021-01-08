/* GNUPLOT - standard.h */

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

#ifndef GNUPLOT_STANDARD_H
# define GNUPLOT_STANDARD_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "eval.h"

/* Type definitions */

/* Variables of standard.c needed by other modules: */

/* Prototypes of functions exported by standard.c */

/* These are the more 'usual' functions built into the stack machine */
void f_real(union argument *x);
void f_imag(union argument *x);
void f_int(union argument *x);
void f_arg(union argument *x);
void f_conjg(union argument *x);
void f_sin(union argument *x);
void f_cos(union argument *x);
void f_tan(union argument *x);
void f_asin(union argument *x);
void f_acos(union argument *x);
void f_atan(union argument *x);
void f_atan2(union argument *x);
void f_sinh(union argument *x);
void f_cosh(union argument *x);
void f_tanh(union argument *x);
void f_asinh(union argument *x);
void f_acosh(union argument *x);
void f_atanh(union argument *x);
void f_ellip_first(union argument *x);
void f_ellip_second(union argument *x);
void f_ellip_third(union argument *x);
void f_void(union argument *x);
void f_abs(union argument *x);
void f_sgn(union argument *x);
void f_sqrt(union argument *x);
void f_exp(union argument *x);
void f_log10(union argument *x);
void f_log(union argument *x);
void f_floor(union argument *x);
void f_ceil(union argument *x);
void f_besi0(union argument *x);
void f_besi1(union argument *x);
void f_besj0(union argument *x);
void f_besj1(union argument *x);
void f_besjn(union argument *x);
void f_besy0(union argument *x);
void f_besy1(union argument *x);
void f_besyn(union argument *x);
void f_exists(union argument *x);   /* exists("foo") */

void f_tmsec(union argument *x);
void f_tmmin(union argument *x);
void f_tmhour(union argument *x);
void f_tmmday(union argument *x);
void f_tmmon(union argument *x);
void f_tmyear(union argument *x);
void f_tmwday(union argument *x);
void f_tmyday(union argument *x);
void f_tmweek(union argument *x);
void f_weekdate_iso(union argument *x);
void f_weekdate_cdc(union argument *x);

#endif /* GNUPLOT_STANDARD_H */
