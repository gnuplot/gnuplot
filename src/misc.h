/*
 * $Id: misc.h,v 1.2.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - misc.h */

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

#ifndef GNUPLOT_MISC_H
# define GNUPLOT_MISC_H

#include "plot.h"

/* Variables of misc.c needed by other modules: */

extern char *infile_name;

/* Prototypes from file "misc.c" */

struct curve_points * cp_alloc __PROTO((int num));
void cp_extend __PROTO((struct curve_points *cp, int num));
void cp_free __PROTO((struct curve_points *cp));
struct iso_curve * iso_alloc __PROTO((int num));
void iso_extend __PROTO((struct iso_curve *ip, int num));
void iso_free __PROTO((struct iso_curve *ip));
struct surface_points * sp_alloc __PROTO((int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2));
void sp_replace __PROTO((struct surface_points *sp, int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2));
void sp_free __PROTO((struct surface_points *sp));
void load_file __PROTO((FILE *fp, char *name, TBOOLEAN subst_args));
FILE *lf_top __PROTO((void));
void load_file_error __PROTO((void));
size_t gp_strcspn __PROTO((const char *, const char *));
int find_maxl_keys __PROTO((struct curve_points *plots, int count, int *kcnt));
int find_maxl_keys3d __PROTO((struct surface_points *plots, int count, int *kcnt));
FILE *loadpath_fopen __PROTO((const char *, const char *));

#endif /* GNUPLOT_MISC_H */
