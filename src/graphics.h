/* $Id: $ */

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
 */

/* Formerly in plot.h */
#define AXIS_ARRAY_SIZE 10

/* Formerly in graphics.c */
/* Define the boundary of the plot
 * These are computed at each call to do_plot, and are constant over
 * the period of one do_plot. They actually only change when the term
 * type changes and when the 'set size' factors change.
 * - no longer true, for 'set key out' or 'set key under'. also depend
 * on tic marks and multi-line labels.
 * They are shared with graph3d.c since we want to use its draw_clip_line()
 */
WHERE int xleft, xright, ybot, ytop;


/* Formerly in graph3d.c */
double xscale3d, yscale3d, zscale3d;


/* Formerly in plot2d.c */
WHERE double min_array[AXIS_ARRAY_SIZE], max_array[AXIS_ARRAY_SIZE];
WHERE int auto_array[AXIS_ARRAY_SIZE];
WHERE TBOOLEAN log_array[AXIS_ARRAY_SIZE];
WHERE double base_array[AXIS_ARRAY_SIZE];
WHERE double log_base_array[AXIS_ARRAY_SIZE];

/* for convenience while converting to use these arrays */
#define x_min3d min_array[FIRST_X_AXIS]
#define x_max3d max_array[FIRST_X_AXIS]
#define y_min3d min_array[FIRST_Y_AXIS]
#define y_max3d max_array[FIRST_Y_AXIS]
#define z_min3d min_array[FIRST_Z_AXIS]
#define z_max3d max_array[FIRST_Z_AXIS]


/* Formerly in set.c */

#ifndef TIMEFMT
# define TIMEFMT "%d/%m/%y\n%H:%M"
#endif

/* array of datatypes (x in 0,y in 1,z in 2,..(rtuv)) */
/* not sure how rtuv come into it ?
 * oh well, make first six compatible with FIRST_X_AXIS, etc
 */
WHERE int datatype[DATATYPE_ARRAY_SIZE];

/* format for date/time for reading time in datafile */
WHERE char timefmt[25] INITVAL (TIMEFMT);

#endif /* GNUPLOT_GRAPHICS_H */
