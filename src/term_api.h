/*
 * $Id: term_api.h,v 1.2.2.2 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - term_api.h */

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

#ifndef GNUPLOT_TERM_API_H
# define GNUPLOT_TERM_API_H

/* #if... / #include / #define collection: */

#include "plot.h"

/* Type definitions */

/* Variables of term.c needed by other modules: */

extern FILE *gpoutfile;

/* Prototypes of functions exported by term.c */

void term_set_output __PROTO((char *));
void term_init __PROTO((void));
void term_start_plot __PROTO((void));
void term_end_plot __PROTO((void));
void term_start_multiplot __PROTO((void));
void term_end_multiplot __PROTO((void));
/* void term_suspend __PROTO((void)); */
void term_reset __PROTO((void));
void term_apply_lp_properties __PROTO((struct lp_style_type *lp));
void term_check_multiplot_okay __PROTO((TBOOLEAN));

GP_INLINE int term_count __PROTO((void));
void list_terms __PROTO((void));
struct termentry *set_term __PROTO((int));
void init_terminal __PROTO((void));
void test_term __PROTO((void));
#ifdef LINUXVGA
void LINUX_setup __PROTO((void));
#endif
#ifdef VMS
void vms_reset();
#endif

/* in set.c (used in pm3d.c) */
void lp_use_properties __PROTO((struct lp_style_type *lp, int tag, int pointflag));

#endif /* GNUPLOT_TERM_API_H */
