/*
 * graph3d.h,v 1.1.1.2 1999/08/28 16:52:54 hbb Exp
 *
 */

/* GNUPLOT - graph3d.h */

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

#ifndef GNUPLOT_GRAPH3D_H
# define GNUPLOT_GRAPH3D_H

/* #if... / #include / #define collection: */

#include "plot.h"

/* for convenience while converting to use these arrays */
#define min3d_z min_array[FIRST_Z_AXIS]
#define max3d_z max_array[FIRST_Z_AXIS]
#define x_min3d min_array[FIRST_X_AXIS]
#define x_max3d max_array[FIRST_X_AXIS]
#define y_min3d min_array[FIRST_Y_AXIS]
#define y_max3d max_array[FIRST_Y_AXIS]
#define z_min3d min_array[FIRST_Z_AXIS]
#define z_max3d max_array[FIRST_Z_AXIS]


/* Type definitions */

typedef double transform_matrix[4][4]; /* HBB 990826: added */

/* Variables of graph3d.c needed by other modules: */

extern int hidden_active;
extern int suppressMove;
extern int xmiddle, ymiddle, xscaler, yscaler;
extern double floor_z;
extern int hidden_no_update;
extern transform_matrix trans_mat;
extern unsigned int move_pos_x, move_pos_y;

/* Prototypes from file "graph3d.c" */

void do_3dplot __PROTO((struct surface_points *plots, int pcount));

void clip_move __PROTO((unsigned int x, unsigned int y));
void clip_vector __PROTO((unsigned int x, unsigned int y));


#endif /* GNUPLOT_GRAPH3D_H */
