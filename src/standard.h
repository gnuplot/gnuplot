/*
 * $Id: standard.h,v 1.2.2.1 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - standard.h */

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

#ifndef GNUPLOT_STANDARD_H
# define GNUPLOT_STANDARD_H

/* #if... / #include / #define collection: */

#include "plot.h"

/* Type definitions */

/* Variables of standard.c needed by other modules: */

/* Prototypes of functions exported by standard.c */

/* These are the more 'usual' functions built into the stack machine */
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

#endif /* GNUPLOT_STANDARD_H */
