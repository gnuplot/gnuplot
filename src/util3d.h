/*
 * $Id: util3d.h,v 1.3.2.1 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - util3d.h */

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

#ifndef GNUPLOT_UTIL3D_H
# define GNUPLOT_UTIL3D_H

/* HBB 990828: moved all those variable decl's and #defines to new
 * file "graph3d.h", as the definitions are in graph3d.c, not in
 * util3d.c. Include that file from here, to ensure everything is
 * known */

#include "graph3d.h"

/* Prototypes of functions exported by "util3d.c" */

void draw_clip_line __PROTO((int, int, int, int));
int clip_line __PROTO((int *, int *, int *, int *));
void edge3d_intersect __PROTO((struct coordinate GPHUGE *, int, double *, double *, double *));
TBOOLEAN two_edge3d_intersect __PROTO((struct coordinate GPHUGE *, int, double *, double *, double *));
void mat_scale __PROTO((double sx, double sy, double sz, double mat[4][4]));
void mat_rot_x __PROTO((double teta, double mat[4][4]));
void mat_rot_z __PROTO((double teta, double mat[4][4]));
void mat_mult __PROTO((double mat_res[4][4], double mat1[4][4], double mat2[4][4]));
int clip_point __PROTO((unsigned int, unsigned int));
void clip_put_text __PROTO((unsigned int, unsigned int, char *));
void clip_put_text_just __PROTO((unsigned int, unsigned int, char *, enum JUSTIFY));

#endif /* GNUPLOT_UTIL3D_H */
