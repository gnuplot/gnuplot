/*
 * $Id: graphics.h,v 1.11.2.2 2000/09/20 01:25:58 joze Exp $
 */

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

#include "plot.h"

/* Collect all global vars in one file.
 * HBB 990829: *Don't!* 
 * The comment below holds ... Lars
 *
 * The ultimate target is of course to eliminate global vars.
 * If possible. Over time. Maybe.
 */

extern int xleft, xright, ybot, ytop;
extern double xscale3d, yscale3d, zscale3d;

/* Formerly in plot2d.c; where they don't belong */
extern double min_array[AXIS_ARRAY_SIZE], max_array[AXIS_ARRAY_SIZE];
extern int auto_array[AXIS_ARRAY_SIZE];
extern TBOOLEAN log_array[AXIS_ARRAY_SIZE];
extern double base_array[AXIS_ARRAY_SIZE];
extern double log_base_array[AXIS_ARRAY_SIZE];
extern char   default_font[];	/* Entry font added by DJL */

/* From ESR's "Actual code patch" :) */
/* An exclusion box.  
 * Right now, the only exclusion region is the key box, but that will
 * change when we support boxed labels.
 */
struct clipbox {
    int xl;
    int xr;
    int yt;
    int yb;
};

/* function prototypes */

extern void graph_error __PROTO((const char *));
extern void fixup_range __PROTO((int, const char *));
int nearest_label_tag(int x, int y, struct termentry* t,
    void (*map_func)(struct position * pos, unsigned int *x,
	unsigned int *y, const char *what));
void get_offsets __PROTO((struct text_label* this_label,
	struct termentry* t, int* htic, int* vtic));
extern void do_plot __PROTO((struct curve_points *, int));
extern int label_width __PROTO((const char *, int *));
extern double set_tic __PROTO((double, int));
extern void setup_tics __PROTO((int, struct ticdef *, char *, int));
void gprintf __PROTO((char *, size_t, char *, double, double));
/* is this valid use of __P ? */
typedef void (*tic_callback) __PROTO((int, double, char *, struct lp_style_type ));
extern void gen_tics __PROTO((int, struct ticdef *, int, int, double, tic_callback));
extern void write_multiline __PROTO((unsigned int, unsigned int, char *, enum JUSTIFY, int, int, const char *));
extern double LogScale __PROTO((double, TBOOLEAN, double, const char *, const char *));
void map_position __PROTO((struct position * pos, unsigned int *x,
				  unsigned int *y, const char *what));
#if defined(sun386) || defined(AMIGA_SC_6_1)
extern double CheckLog __PROTO((TBOOLEAN, double, double));
#endif

void apply_head_properties __PROTO((struct position* headsize));

#endif /* GNUPLOT_GRAPHICS_H */
