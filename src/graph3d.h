/*
 * $Id: graph3d.h,v 1.4 2000/05/02 18:01:03 lhecking Exp $
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

/* Function macros to map from user 3D space into normalized -1..1 */
#define map_x3d(x) ((x-min_array[FIRST_X_AXIS])*xscale3d-1.0)
#define map_y3d(y) ((y-min_array[FIRST_Y_AXIS])*yscale3d-1.0)
#define map_z3d(z) ((z-floor_z)*zscale3d-1.0)

/* Type definitions */

typedef double transform_matrix[4][4]; /* HBB 990826: added */

/* Variables of graph3d.c needed by other modules: */

extern int hidden_active;
extern int suppressMove;
extern int xmiddle, ymiddle, xscaler, yscaler;
extern double floor_z;
extern int hidden_no_update;
extern transform_matrix trans_mat;
extern double xscale3d, yscale3d, zscale3d;

#ifdef USE_MOUSE
extern int axis3d_o_x, axis3d_o_y, axis3d_x_dx, axis3d_x_dy, axis3d_y_dx, axis3d_y_dy;
#endif

/* Prototypes from file "graph3d.c" */

void do_openglplot __PROTO((struct surface_points *plots, int pcount));
void do_3dplot __PROTO((struct surface_points *plots, int pcount, int quick));

void clip_move __PROTO((unsigned int x, unsigned int y));
void clip_vector __PROTO((unsigned int x, unsigned int y));
void map3d_position __PROTO((struct position * pos, unsigned int *x,
				  unsigned int *y, const char *what));


#endif /* GNUPLOT_GRAPH3D_H */
