/* $Id: graphics.h,v 1.1 1999/06/11 18:55:51 lhecking Exp $ */

/* GNUPLOT - graphics.h */

/*[
 * Copyright 1999
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

#ifndef GNUPLOT_GRAPHICS_H
# define GNUPLOT_GRAPHICS_H

/* Collect all global vars in one file.
 * The comments may go at a later date,
 * but are needed for reference now
 *
 * Maybe this should be split into separate files
 * for 2d/3d/parser/other?
 *
 * The ultimate target is of course to eliminate global vars.
 * If possible. Over time. Maybe.
 */

/* Formerly in plot.h */
#define AXIS_ARRAY_SIZE 10

extern int xleft, xright, ybot, ytop;

extern double xscale3d, yscale3d, zscale3d;


/* Formerly in plot2d.c; where they don't belong */
extern double min_array[AXIS_ARRAY_SIZE], max_array[AXIS_ARRAY_SIZE];
extern int auto_array[AXIS_ARRAY_SIZE];
extern TBOOLEAN log_array[AXIS_ARRAY_SIZE];
extern double base_array[AXIS_ARRAY_SIZE];
extern double log_base_array[AXIS_ARRAY_SIZE];

/* for convenience while converting to use these arrays */
#define x_min3d min_array[FIRST_X_AXIS]
#define x_max3d max_array[FIRST_X_AXIS]
#define y_min3d min_array[FIRST_Y_AXIS]
#define y_max3d max_array[FIRST_Y_AXIS]
#define z_min3d min_array[FIRST_Z_AXIS]
#define z_max3d max_array[FIRST_Z_AXIS]

/* format for date/time for reading time in datafile */
extern char timefmt[];

extern int datatype[];

#endif /* GNUPLOT_GRAPHICS_H */
