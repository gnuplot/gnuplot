/* GNUPLOT - interpol.h */

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

#ifndef GNUPLOT_INTERPOL_H
# define GNUPLOT_INTERPOL_H

#include "syscfg.h"
#include "graphics.h"
#include "graph3d.h"

/* Variables of interpol.c needed by other modules: */

/* Prototypes of functions exported by interpol.c */
void gen_interp(struct curve_points *plot);
void gen_interp_unwrap(struct curve_points *plot);
void gen_interp_frequency(struct curve_points *plot);
void sort_points(struct curve_points *plot);
void zsort_points(struct curve_points *plot);
void zrange_points(struct curve_points *plot);
void cp_implode(struct curve_points *cp);
int next_curve(struct curve_points * plot, int *curve_start);
int num_curves(struct curve_points * plot);

#define spline_coeff_size 4
typedef double spline_coeff[spline_coeff_size];
spline_coeff *cp_approx_spline(struct coordinate *first_point, int num_points,
				int path_dim, int spline_dim, int w_dim);
spline_coeff *cp_tridiag(struct coordinate *first_point, int num_points,
			int path_dim, int spline_dim);

int compare_x(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
int compare_z(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
int compare_xyz(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);

#endif /* GNUPLOT_INTERPOL_H */
