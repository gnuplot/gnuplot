/*
 * $Id: util.h,v 1.1 1999/06/14 19:21:31 lhecking Exp $
 *
 */

/* GNUPLOT - util.h */

/*[
 * Copyright 1986 - 1993, 1998, 1999   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_UTIL_H
# define GNUPLOT_UTIL_H

/* TRUE if command just typed; becomes FALSE whenever we
 * send some other output to screen.  If FALSE, the command line
 * will be echoed to the screen before the ^ error message.
 */
extern TBOOLEAN screen_ok;

extern int chr_in_str __PROTO((int, int));
extern int equals __PROTO((int, const char *));
extern int almost_equals __PROTO((int, const char *));
extern int isstring __PROTO((int));
extern int isanumber __PROTO((int));
extern int isletter __PROTO((int));
extern int is_definition __PROTO((int));
extern void copy_str __PROTO((char *, int, int));
extern int token_len __PROTO((int));
extern void quote_str __PROTO((char *, int, int));
extern void capture __PROTO((char *, int, int, int));
extern void m_capture __PROTO((char **, int, int));
extern void m_quote_capture __PROTO((char **, int, int));
extern char *gp_strdup __PROTO((const char *));
extern void convert __PROTO((struct value *, int));
extern void disp_value __PROTO((FILE *, struct value *));
extern double real __PROTO((struct value *));
extern double imag __PROTO((struct value *));
extern double magnitude __PROTO((struct value *));
extern double angle __PROTO((struct value *));
extern struct value * Gcomplex __PROTO((struct value *, double, double));
extern struct value * Ginteger __PROTO((struct value *, int));
#if defined(VA_START) && defined(ANSI_C)
extern void os_error __PROTO((int, const char *, ...));
extern void int_error __PROTO((int, const char *, ...));
extern void int_warn __PROTO((int, const char *, ...));
#else
extern void os_error __PROTO(());
extern void int_error __PROTO(());
extern void int_warn __PROTO(());
#endif
extern void lower_case __PROTO((char *));
extern void squash_spaces __PROTO((char *));

#endif /* GNUPLOT_UTIL_H */
