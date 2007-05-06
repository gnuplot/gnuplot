/*
 * $Id: graphics.h,v 1.36 2007/01/17 05:34:17 sfeam Exp $
 */

/* GNUPLOT - graphics.h */

/*[
 * Copyright 1999, 2004
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

#include "syscfg.h"
#include "gp_types.h"

#include "gadgets.h"
#include "term_api.h"

/* types defined for 2D plotting */

typedef struct curve_points {
    struct curve_points *next;	/* pointer to next plot in linked list */
    int token;			/* last token used, for second parsing pass */
    enum PLOT_TYPE plot_type;	/* DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? */
    enum PLOT_STYLE plot_style;	/* style set by "with" or by default */
    char *title;		/* plot title, a.k.a. key entry */
    TBOOLEAN title_no_enhanced;	/* don't typeset title in enhanced mode */
    TBOOLEAN title_is_filename;	/* TRUE if title was auto-generated from filename */
    TBOOLEAN title_is_suppressed;/* TRUE if 'notitle' was specified */
    struct lp_style_type lp_properties;
    struct arrow_style_type arrow_properties;
    struct fill_style_type fill_properties;
#ifdef EAM_DATASTRINGS
    struct text_label *labels;	/* Only used if plot_style == LABELPOINTS */
#endif
#ifdef WITH_IMAGE
    struct t_image image_properties;	/* only used if plot_style is IMAGE or RGB_IMAGE */
#endif

    /* 2D and 3D plot structure fields overlay only to this point */

    filledcurves_opts filledcurves_options;
#ifdef EAM_HISTOGRAMS
    struct histogram_style *histogram;	/* Only used if plot_style == HISTOGRAM */
    int histogram_sequence;	/* Ordering of this dataset within the histogram */
#endif
    enum PLOT_SMOOTH plot_smooth; /* which "smooth" method to be used? */
    int p_max;			/* how many points are allocated */
    int p_count;		/* count of points in points */
    int x_axis;			/* FIRST_X_AXIS or SECOND_X_AXIS */
    int y_axis;			/* FIRST_Y_AXIS or SECOND_Y_AXIS */
    int z_axis;			/* same as either x_axis or y_axis, for 5-column plot types */
    struct coordinate GPHUGE *points;
} curve_points;

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

/* externally visible variables of graphics.h */

/* 'set offset' status variables */
extern double loff, roff, toff, boff;

/* 'set bar' status */
extern double bar_size;

/* function prototypes */

void do_plot __PROTO((struct curve_points *, int));
int label_width __PROTO((const char *, int *));
void map_position __PROTO((struct position * pos, unsigned int *x,
				  unsigned int *y, const char *what));
void map_position_r __PROTO((struct position* pos, double* x, double* y,
			     const char* what));
#if defined(sun386) || defined(AMIGA_SC_6_1)
double CheckLog __PROTO((TBOOLEAN, double, double));
#endif
void apply_head_properties __PROTO((struct arrow_style_type *arrow_properties));

#ifdef EAM_HISTOGRAMS
void init_histogram __PROTO((struct histogram_style *hist, char *title));
void free_histlist __PROTO((struct histogram_style *hist));
#endif

#ifdef WITH_IMAGE
void plot_image_or_update_axes __PROTO((void *plot, TBOOLEAN map_points, TBOOLEAN update_axes));
#define PLOT_IMAGE(plot) \
	plot_image_or_update_axes(plot, FALSE, FALSE)
#define UPDATE_AXES_FOR_PLOT_IMAGE(plot) \
	plot_image_or_update_axes(plot, FALSE, TRUE)
#define SPLOT_IMAGE(plot) \
	plot_image_or_update_axes(plot, TRUE, FALSE)
#endif

#ifdef EAM_OBJECTS
void place_rectangles __PROTO((struct object *listhead, int layer, int dimensions, BoundingBox *clip_area));
#else
#define place_rectangles(listhead,layer,dimensions,clip_area) /* void() */
#endif

#endif /* GNUPLOT_GRAPHICS_H */
