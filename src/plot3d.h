/*
 * $Id: plot3d.h,v 1.2.2.1 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - plot3d.h */

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

#ifndef GNUPLOT_PLOT3D_H
# define GNUPLOT_PLOT3D_H

#include "plot.h"

/* Variables of plot3d.c needed by other modules: */

extern struct surface_points *first_3dplot;
extern int plot3d_num;

/* prototypes from plot3d.c */

void plot3drequest __PROTO((void));

#endif /* GNUPLOT_PLOT3D_H */
