/*
 * $Id: datafile.h,v 1.3.4.2 2000/06/22 12:57:38 broeker Exp $
 */

/* GNUPLOT - datafile.h */

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

#ifndef GNUPLOT_DATAFILE_H
# define GNUPLOT_DATAFILE_H

/* #if... / #include / #define collection: */

#include "axis.h"
#include "graph3d.h"
#include "graphics.h"

/* returns from DF_READLINE in datafile.c */
/* +ve is number of columns read */
#define DF_EOF          (-1)
#define DF_UNDEFINED    (-2)
#define DF_FIRST_BLANK  (-3)
#define DF_SECOND_BLANK (-4)

#ifndef MAXINT			/* should there be one already defined ? */
# ifdef INT_MAX			/* in limits.h ? */
#  define MAXINT INT_MAX
# else
#  define MAXINT ((~0)>>1)
# endif
#endif

/* Variables of datafile.c needed by other modules: */

/* how many using columns were specified */
extern int df_no_use_specs;

/* suggested x value if none given */
extern int df_datum;

/* is this a matrix splot? */
extern TBOOLEAN df_matrix;

/* is this a binary file? */
extern TBOOLEAN df_binary;

extern int df_eof;
extern int df_line_number;
extern AXIS_INDEX df_axis[];
extern struct udft_entry ydata_func; /* HBB 990829: moved from command.h */

/* string representing missing values, ascii datafiles */
extern char *missing_val;

/* flag if any 'inline' data are in use, for the current plot */
extern TBOOLEAN plotted_data_from_stdin;


/* Prototypes of functions exported by datafile.c */

int df_open __PROTO((int));
int df_readline __PROTO((double [], int));
void df_close __PROTO((void));
int df_2dbinary __PROTO((struct curve_points *));
int df_3dmatrix __PROTO((struct surface_points *));

void f_dollars __PROTO((union argument *x));
void f_column  __PROTO((void));
void f_valid   __PROTO((void));
void f_timecolumn   __PROTO((void));

#endif /* GNUPLOT_DATAFILE_H */
