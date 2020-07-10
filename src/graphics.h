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

#ifndef AXIS_INDEX
#include "axis.h"
#endif

/* types defined for 2D plotting */

typedef struct curve_points {
    struct curve_points *next;	/* pointer to next plot in linked list */
    int token;			/* last token used, for second parsing pass */
    enum PLOT_TYPE plot_type;	/* DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? */
    enum PLOT_STYLE plot_style;	/* style set by "with" or by default */
    char *title;		/* plot title, a.k.a. key entry */
    t_position *title_position;	/* title at {beginning|end|<xpos>,<ypos>} */
    TBOOLEAN title_no_enhanced;	/* don't typeset title in enhanced mode */
    TBOOLEAN title_is_automated;/* TRUE if title was auto-generated */
    TBOOLEAN title_is_suppressed;/* TRUE if 'notitle' was specified */
    TBOOLEAN noautoscale;	/* ignore data from this plot during autoscaling */
    struct lp_style_type lp_properties;
    struct arrow_style_type arrow_properties;
    struct fill_style_type fill_properties;
    struct text_label *labels;	/* Only used if plot_style == LABELPOINTS */
    struct t_image image_properties;	/* only used if plot_style is IMAGE or RGB_IMAGE */
    struct udvt_entry *sample_var;	/* used by '+' if plot has private sampling range */
    struct udvt_entry *sample_var2;	/* used by '++'if plot has private sampling range */

    /* 2D and 3D plot structure fields overlay only to this point */
    filledcurves_opts filledcurves_options;
    int base_linetype;		/* before any calls to load_linetype(), lc variable */
				/* analogous to hidden3d_top_linetype in graph3d.h  */
    int ellipseaxes_units;              /* Only used if plot_style == ELLIPSES */    
    struct histogram_style *histogram;	/* Only used if plot_style == HISTOGRAM */
    int histogram_sequence;	/* Ordering of this dataset within the histogram */
    enum PLOT_SMOOTH plot_smooth; /* which "smooth" method to be used? */
    double smooth_parameter;	/* e.g. optional bandwidth for smooth kdensity */
    double smooth_period;	/* e.g. 2pi for a circular function */
    int boxplot_factors;	/* Only used if plot_style == BOXPLOT */
    int p_max;			/* how many points are allocated */
    int p_count;		/* count of points in points */
    AXIS_INDEX x_axis;		/* FIRST_X_AXIS or SECOND_X_AXIS */
    AXIS_INDEX y_axis;		/* FIRST_Y_AXIS or SECOND_Y_AXIS */
    AXIS_INDEX z_axis;		/* same as either x_axis or y_axis, for 5-column plot types */
    int current_plotno;		/* Only used by "pn" option of linespoints */
    AXIS_INDEX p_axis;		/* Only used for parallel axis plots */
    double *varcolor;		/* Only used if plot has variable color */
    struct coordinate *points;
} curve_points;

/* externally visible variables of graphics.h */

/* 'set offset' status variables */
extern t_position loff, roff, toff, boff;
extern TBOOLEAN retain_offsets;

/* 'set bar' status */
extern double bar_size;
extern int bar_layer;
extern struct lp_style_type bar_lp;

/* 'set rgbmax {0|255}' */
extern double rgbmax;

/* function prototypes */

void do_plot(struct curve_points *, int);
void map_position(struct position * pos, int *x, int *y, const char *what);
void map_position_r(struct position* pos, double* x, double* y, const char* what);

void init_histogram(struct histogram_style *hist, text_label *title);
void free_histlist(struct histogram_style *hist);

typedef enum en_procimg_action {
    IMG_PLOT,
    IMG_UPDATE_AXES,
    IMG_UPDATE_CORNERS
} t_procimg_action;

void process_image(void *plot, t_procimg_action action);
TBOOLEAN check_for_variable_color(struct curve_points *plot, double *colorvalue);
void autoscale_boxplot(struct curve_points *plot);

void place_objects(struct object *listhead, int layer, int dimensions);
void do_ellipse(int dimensions, t_ellipse *e, int style, TBOOLEAN do_own_mapping );

void place_pixmaps(int layer, int dimensions);

int filter_boxplot(struct curve_points *);
void attach_title_to_plot(struct curve_points *this_plot, legend_key *key);

#endif /* GNUPLOT_GRAPHICS_H */
