/*
 * $Id: $
 *
 */

/* GNUPLOT - util3d.h */

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

#ifndef GNUPLOT_UTIL3D_H
# define GNUPLOT_UTIL3D_H

/* HBB 980324: hidden_no_update was unused here */
extern int hidden_active;

/* ACCESS THINGS THAT OUGHT TO BE HIDDEN IN hidden3d.c - perhaps we
 * can move the relevant code into hidden3d.c sometime
 */

/* Bitmap of the screen.  The array for each x value is malloc-ed as needed */

extern int suppressMove;

/* for convenience while converting to use these arrays */
#define min3d_z min_array[FIRST_Z_AXIS]
#define max3d_z max_array[FIRST_Z_AXIS]

void draw_clip_line __PROTO((unsigned int, unsigned int, unsigned int, unsigned int));
int clip_line __PROTO((int *, int *, int *, int *));
void edge3d_intersect __PROTO((struct coordinate GPHUGE *, int, double *, double *, double *));
TBOOLEAN two_edge3d_intersect __PROTO((struct coordinate GPHUGE *, int, double *, double *, double *));
void mat_unit __PROTO((double mat[4][4]));
void mat_trans __PROTO((double tx, double ty, double tz, double mat[4][4]));
void mat_scale __PROTO((double sx, double sy, double sz, double mat[4][4]));
void mat_rot_x __PROTO((double teta, double mat[4][4]));
void mat_rot_y __PROTO((double teta, double mat[4][4]));
void mat_rot_z __PROTO((double teta, double mat[4][4]));
void mat_mult __PROTO((double mat_res[4][4], double mat1[4][4], double mat2[4][4]));
int clip_point __PROTO((unsigned int, unsigned int));
void clip_put_text __PROTO((unsigned int, unsigned int, char *));
void clip_put_text_just __PROTO((unsigned int, unsigned int, char *, enum JUSTIFY));

#endif /* GNUPLOT_UTIL3D_H */
