/* GNUPLOT - plot2d.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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

#include "gp_types.h"
#include "plot2d.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "datafile.h"
#include "datablock.h"
#include "eval.h"
#include "fit.h"
#include "graphics.h"
#include "interpol.h"
#include "misc.h"
#include "parse.h"
#include "pm3d.h"	/* for is_plot_with_palette */
#include "setshow.h"
#include "tables.h"
#include "tabulate.h"
#include "term_api.h"
#include "util.h"
#include "variable.h" /* For locale handling */

#ifndef _Windows
# include "help.h"
#endif

/* minimum size of points[] in curve_points */
#define MIN_CRV_POINTS 100

/* static prototypes */

static struct curve_points * cp_alloc __PROTO((int num));
static int get_data __PROTO((struct curve_points *));
static void store2d_point __PROTO((struct curve_points *, int i, double x, double y, double xlow, double xhigh, double ylow, double yhigh, double width));
static void eval_plots __PROTO((void));
static void parametric_fixup __PROTO((struct curve_points * start_plot, int *plot_num));
static void box_range_fiddling __PROTO((struct curve_points *plot));
static void boxplot_range_fiddling __PROTO((struct curve_points *plot));
static void histogram_range_fiddling __PROTO((struct curve_points *plot));
static void impulse_range_fiddling __PROTO((struct curve_points *plot));
static int check_or_add_boxplot_factor __PROTO((struct curve_points *plot, char* string, double x));
static void add_tics_boxplot_factors __PROTO((struct curve_points *plot));

/* internal and external variables */

/* the curves/surfaces of the plot */
struct curve_points *first_plot = NULL;
static struct udft_entry plot_func;

/* box width (automatic) */
double   boxwidth              = -1.0;
/* whether box width is absolute (default) or relative */
TBOOLEAN boxwidth_is_absolute  = TRUE;

static double histogram_rightmost = 0.0;    /* Highest x-coord of histogram so far */
static text_label histogram_title;          /* Subtitle for this histogram */
static int stack_count = 0;                 /* counter for stackheight */
static struct coordinate GPHUGE *stackheight = NULL; /* Scratch space for y autoscale */

/* function implementations */

/*
 * cp_alloc() allocates a curve_points structure that can hold 'num'
 * points.   Initialize all fields to NULL.
 */
static struct curve_points *
cp_alloc(int num)
{
    struct curve_points *cp;
    struct lp_style_type default_lp_properties = DEFAULT_LP_STYLE_TYPE;

    cp = (struct curve_points *) gp_alloc(sizeof(struct curve_points), "curve");
    memset(cp,0,sizeof(struct curve_points));

    cp->p_max = (num >= 0 ? num : 0);
    if (num > 0)
	cp->points = (struct coordinate GPHUGE *)
	    gp_alloc(num * sizeof(struct coordinate), "curve points");

    /* Initialize various fields */
    cp->lp_properties = default_lp_properties;
    default_arrow_style(&(cp->arrow_properties));
    cp->fill_properties = default_fillstyle;
    cp->filledcurves_options = filledcurves_opts_data;

    return (cp);
}


/*
 * cp_extend() reallocates a curve_points structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void
cp_extend(struct curve_points *cp, int num)
{
    if (num == cp->p_max)
	return;

    if (num > 0) {
	if (cp->points == NULL) {
	    cp->points = gp_alloc(num * sizeof(cp->points[0]),
				  "curve points");
	} else {
	    cp->points = gp_realloc(cp->points, num * sizeof(cp->points[0]),
				    "expanding curve points");
	    if (cp->varcolor)
		cp->varcolor = gp_realloc(cp->varcolor, num * sizeof(double),
					"expanding curve variable colors");
	    if (cp->z_n) {
		int i;
		for (i = 0; i < cp->n_par_axes; i++)
		    cp->z_n[i] = gp_realloc(cp->z_n[i], num * sizeof(double),
					"expanding curve z_n[i]");
	    }
	}
	cp->p_max = num;
	cp->p_max -= 1;		/* Set trigger point for reallocation ahead of	*/
				/* true end in case two slots are used at once	*/
				/* (e.g. redundant final point of closed curve)	*/
    } else {
	free(cp->points);
	cp->points = NULL;
	cp->p_max = 0;
	free(cp->varcolor);
	cp->varcolor = NULL;
	if (cp->z_n) {
	    int i;
	    for (i = 0; i < cp->n_par_axes; i++)
		free(cp->z_n[i]);
	    free(cp->z_n);
	    cp->n_par_axes = 0;
	    cp->z_n = NULL;
	}
    }
}

/*
 * cp_free() releases any memory which was previously malloc()'d to hold
 *   curve points (and recursively down the linked list).
 */
void
cp_free(struct curve_points *cp)
{
    while (cp) {
	struct curve_points *next = cp->next;

	free(cp->title);
	cp->title = NULL;
	free(cp->title_position);
	cp->title_position = NULL;
	free(cp->points);
	cp->points = NULL;
	free(cp->varcolor);
	cp->varcolor = NULL;
	if (cp->labels)
	    free_labels(cp->labels);
	cp->labels = NULL;
	if (cp->z_n) {
	    int i;
	    for (i = 0; i < cp->n_par_axes; i++)
		free(cp->z_n[i]);
	    free(cp->z_n);
	    cp->n_par_axes = 0;
	    cp->z_n = NULL;
	}

	free(cp);
	cp = next;
    }
}

/*
 * In the parametric case we can say plot [a= -4:4] [-2:2] [-1:1] sin(a),a**2
 * while in the non-parametric case we would say only plot [b= -2:2] [-1:1]
 * sin(b)
 */
void
plotrequest()
{
    int dummy_token = 0;
    AXIS_INDEX axis;

    if (!term)                  /* unknown */
	int_error(c_token, "use 'set term' to set terminal type first");

    is_3d_plot = FALSE;

    /* Deactivate if 'set view map' is still running after the previous 'splot': */
    /* EAM Jan 2012 - this should no longer be necessary, but it doesn't hurt. */
    splot_map_deactivate();

    if (parametric && strcmp(set_dummy_var[0], "u") == 0)
	strcpy(set_dummy_var[0], "t");

    /* initialize the arrays from the 'set' scalars */
    axis_init(&axis_array[FIRST_X_AXIS], FALSE);
    axis_init(&axis_array[FIRST_Y_AXIS], TRUE);
    axis_init(&axis_array[SECOND_X_AXIS], FALSE);
    axis_init(&axis_array[SECOND_Y_AXIS], TRUE);
    axis_init(&axis_array[T_AXIS], FALSE);
    axis_init(&axis_array[U_AXIS], FALSE);
    axis_init(&axis_array[V_AXIS], FALSE);
    axis_init(&axis_array[POLAR_AXIS], TRUE);
    axis_init(&axis_array[COLOR_AXIS], TRUE);

    /* Always be prepared to restore the autoscaled values on "refresh"
     * Dima Kogan April 2018
     * FIXME: Could we fold this into axis_init?
     */
    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	AXIS *this_axis = &axis_array[axis];
	if (this_axis->set_autoscale != AUTOSCALE_NONE)
	    this_axis->range_flags |= RANGE_WRITEBACK;
    }

#ifdef NONLINEAR_AXES
    /* Nonlinear mapping of x or y via linkage to a hidden primary axis. */
    /* The user set autoscale for the visible axis; apply it also to the hidden axis. */
    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	AXIS *secondary = &axis_array[axis];
	if (axis == SAMPLE_AXIS)
	    continue;
	if (secondary->linked_to_primary
	&&  secondary->linked_to_primary->index == -secondary->index) {
	    AXIS *primary = secondary->linked_to_primary;
	    primary->set_autoscale = secondary->set_autoscale;
	    axis_init(primary, 1);
	}
    }
#endif

    /* If we are called from a mouse zoom operation we should ignore	*/
    /* any range limits because otherwise the zoom won't zoom.		*/
    if (inside_zoom) {
	while (equals(c_token,"["))
	    parse_skip_range();
    }

    /* Range limits for the entire plot are optional but must be given	*/
    /* in a fixed order. The keyword 'sample' terminates range parsing.	*/
    if (parametric || polar) {
	dummy_token = parse_range(T_AXIS);
	parse_range(FIRST_X_AXIS);
    } else {
	dummy_token = parse_range(FIRST_X_AXIS);
    }
    parse_range(FIRST_Y_AXIS);
    parse_range(SECOND_X_AXIS);
    parse_range(SECOND_Y_AXIS);
    if (equals(c_token,"sample") && equals(c_token+1,"["))
	c_token++;

    /* Clear out any tick labels read from data files in previous plot */
    for (axis=0; axis<AXIS_ARRAY_SIZE; axis++) {
	struct ticdef *ticdef = &axis_array[axis].ticdef;
	if (ticdef->def.user)
	    ticdef->def.user = prune_dataticks(ticdef->def.user);
	if (!ticdef->def.user && ticdef->type == TIC_USER)
	    ticdef->type = TIC_COMPUTED;
    }

    /* use the default dummy variable unless changed */
    if (dummy_token > 0)
	copy_str(c_dummy_var[0], dummy_token, MAX_ID_LEN);
    else
	strcpy(c_dummy_var[0], set_dummy_var[0]);

    eval_plots();
}


/* Helper function for refresh command.  Reexamine each data point and update the
 * flags for INRANGE/OUTRANGE/UNDEFINED based on the current limits for that axis.
 * Normally the axis limits are already known at this point. But if the user has
 * forced "set autoscale" since the previous plot or refresh, we need to reset the
 * axis limits and try to approximate the full auto-scaling behaviour.
 */
void
refresh_bounds(struct curve_points *first_plot, int nplots)
{
    struct curve_points *this_plot = first_plot;
    int iplot;		/* plot index */

    for (iplot = 0;  iplot < nplots; iplot++, this_plot = this_plot->next) {
	int i;		/* point index */
	struct axis *x_axis = &axis_array[this_plot->x_axis];
	struct axis *y_axis = &axis_array[this_plot->y_axis];

	/* IMAGE clipping is done elsewhere, so we don't need INRANGE/OUTRANGE checks */
	if (this_plot->plot_style == IMAGE || this_plot->plot_style == RGBIMAGE) {
	    if (x_axis->set_autoscale || y_axis->set_autoscale)
		process_image(this_plot, IMG_UPDATE_AXES);
	    continue;
	}

	for (i=0; i<this_plot->p_count; i++) {
	    struct coordinate GPHUGE *point = &this_plot->points[i];

	    if (point->type == UNDEFINED)
		continue;
	    else
		point->type = INRANGE;

	    /* This autoscaling logic is identical to that in
	     * refresh_3dbounds() in plot3d.c
	     */
	    if (!this_plot->noautoscale) {
		autoscale_one_point(x_axis, point->x);
		if (this_plot->plot_style == VECTOR)
		    autoscale_one_point(x_axis, point->xhigh);
	    }
	    if (!inrange(point->x, x_axis->min, x_axis->max)) {
		point->type = OUTRANGE;
		continue;
	    }
	    if (!this_plot->noautoscale) {
		autoscale_one_point(y_axis, point->y);
		if (this_plot->plot_style == VECTOR)
		    autoscale_one_point(y_axis, point->yhigh);
	    }
	    if (!inrange(point->y, y_axis->min, y_axis->max)) {
		point->type = OUTRANGE;
		continue;
	    }
	}
	if (this_plot->plot_style == BOXES || this_plot->plot_style == IMPULSES)
	    impulse_range_fiddling(this_plot);
    }

    this_plot = first_plot;
    for (iplot = 0;  iplot < nplots; iplot++, this_plot = this_plot->next) {

	/* handle 'reverse' ranges */
	axis_revert_range( this_plot->x_axis );
	axis_revert_range( this_plot->y_axis );

	/* Make sure the bounds are reasonable, and tweak them if they aren't */
	axis_checked_extend_empty_range(this_plot->x_axis, NULL);
	axis_checked_extend_empty_range(this_plot->y_axis, NULL);
    }
}


/* A quick note about boxes style. For boxwidth auto, we cannot
 * calculate widths yet, since it may be sorted, etc. But if
 * width is set, we must do it now, before logs of xmin/xmax
 * are taken.
 * We store -1 in point->z as a marker to mean width needs to be
 * calculated, or 0 to mean that xmin/xmax are set correctly
 */
/* current_plot->token is after datafile spec, for error reporting
 * it will later be moved passed title/with/linetype/pointtype
 */
static int
get_data(struct curve_points *current_plot)
{
    int i /* num. points ! */ , j;
    int ngood;
    int max_cols, min_cols;    /* allowed range of column numbers */
    int storetoken = current_plot->token;
    struct coordinate GPHUGE *cp;
    double v[MAXDATACOLS];
    memset(v, 0, sizeof(v));

    if (current_plot->varcolor == NULL) {
	TBOOLEAN variable_color = FALSE;
	if ((current_plot->lp_properties.pm3d_color.type == TC_RGB)
	&&  (current_plot->lp_properties.pm3d_color.value < 0))
	    variable_color = TRUE;
	if (current_plot->lp_properties.pm3d_color.type == TC_Z)
	    variable_color = TRUE;
	if (current_plot->lp_properties.l_type == LT_COLORFROMCOLUMN)
	    variable_color = TRUE;
	if (current_plot->plot_smooth != SMOOTH_NONE) {
	    /* FIXME:  It would be possible to support smooth cspline lc palette */
	    /* but it would require expanding and interpolating plot->varcolor   */
	    /* in parallel with the y values.                                    */
	    variable_color = FALSE;
	}
	if (variable_color)
	    current_plot->varcolor = gp_alloc(current_plot->p_max * sizeof(double),
		"varcolor array");
	if (variable_color && current_plot->plot_style == PARALLELPLOT) {
	    /* Oops, we reserved one column of data too many */
	    free(current_plot->z_n[--(current_plot->n_par_axes)]);
	}
    }

    /* eval_plots has already opened file */

    /* HBB 2000504: For most 2D plot styles the 'z' coordinate is unused.
     * Set it to NO_AXIS to account for that. For styles that use
     * the z coordinate as a real coordinate (i.e. not a width or
     * 'delta' component, change the setting inside the switch: */
    current_plot->z_axis = NO_AXIS;

    /* HBB NEW 20060427: if there's only one, explicit using column,
     * it's y data.  df_axis[] has to reflect that, so df_readline()
     * will expect time/date input. */
    if (df_no_use_specs == 1)
	df_axis[0] = df_axis[1];

    switch (current_plot->plot_style) { /* set maximum columns to scan */
    case XYERRORLINES:
    case XYERRORBARS:
    case BOXXYERROR:
	min_cols = 4;
	max_cols = 7;

	if (df_no_use_specs >= 6) {
	    /* HBB 20060427: signal 3rd and 4th column are absolute x
	     * data --- needed so time/date parsing works */
	    df_axis[2] = df_axis[3] = df_axis[0];
	    /* and 5th and 6th are absolute y data */
	    df_axis[4] = df_axis[5] = df_axis[1];
	}

	break;

    case FINANCEBARS:
	/* HBB 20000504: use 'z' coordinate for y-axis quantity */
	current_plot->z_axis = current_plot->y_axis;
	min_cols = 5;
	max_cols = 6;
	/* HBB 20060427: signal 3rd and 4th column are absolute y data
	 * --- needed so time/date parsing works */
	df_axis[2] = df_axis[3] = df_axis[4] = df_axis[1];
	break;

    case BOXPLOT:
	min_cols = 2;		/* fixed x, lots of y data points */
	max_cols = 4;		/* optional width, optional factor */
	expect_string( 4 );
	break;

    case CANDLESTICKS:
	current_plot->z_axis = current_plot->y_axis;
	min_cols = 5;
	max_cols = 7;
	df_axis[2] = df_axis[3] = df_axis[4] = df_axis[1];
	break;

    case BOXERROR:
	min_cols = 3;
	max_cols = 6;

	/* There are four possible cases: */
	/* 3 cols --> (x,y,dy), auto dx */
	/* 4 cols, boxwidth==-2 --> (x,y,ylow,yhigh), auto dx */
	/* 4 cols, boxwidth!=-2 --> (x,y,dy,dx) */
	/* 5 cols --> (x,y,ylow,yhigh,dx) */
	/* In each case an additional column may hold variable color */
	if ((df_no_use_specs == 4 && boxwidth == -2)
	    || df_no_use_specs >= 5)
	    /* HBB 20060427: signal 3rd and 4th column are absolute y
	     * data --- needed so time/date parsing works */
	    df_axis[2] = df_axis[3] = df_axis[1];
	break;

    case VECTOR:	/* x, y, dx, dy, variable color or arrow style */
	min_cols = 4;
	max_cols = 5;
	break;

    case XERRORLINES:
    case XERRORBARS:
	min_cols = 3;
	max_cols = 5;
	if (df_no_use_specs >= 4)
	    /* HBB 20060427: signal 3rd and 4th column are absolute x
	     * data --- needed so time/date parsing works */
	    df_axis[2] = df_axis[3] = df_axis[0];
	break;

    case YERRORLINES:
    case YERRORBARS:
	min_cols = 2;
	max_cols = 5;
	if (df_no_use_specs >= 4)
	    /* HBB 20060427: signal 3rd and 4th column are absolute y
	     * data --- needed so time/date parsing works */
	    df_axis[2] = df_axis[3] = df_axis[1];
	break;

    case HISTOGRAMS:
	min_cols = 1;
	max_cols = 3;
	break;

    case BOXES:
	min_cols = 1;
	max_cols = 4;

	break;

    case FILLEDCURVES:
	min_cols = 1;
	max_cols = 3;
	df_axis[2] = df_axis[1];	/* Both curves use same y axis */
	break;

    case IMPULSES:	/* 2 + possible variable color */
    case LINES:
    case DOTS:
	min_cols = 1;
	max_cols = 3;
	break;

    case LABELPOINTS:
	/* 3 column data: X Y Label */
	/* extra columns allow variable pointsize and/or rotation */
	min_cols = 3;
	max_cols = 5;
	expect_string( 3 );
	break;

    case IMAGE:
	min_cols = 3;
	max_cols = 3;
	break;

    case RGBIMAGE:
	min_cols = 5;
	max_cols = 6;
	break;

    case RGBA_IMAGE:
	min_cols = 6;
	max_cols = 6;
	break;

#ifdef EAM_OBJECTS
    case CIRCLES:	/* 3 + possible variable color, or 5 + possible variable color */
	min_cols = 2;
	max_cols = 6;
	break;

	case ELLIPSES:
	    min_cols = 2; /* x, y, major axis, minor axis */
	    max_cols = 6; /* + optional angle, possible variable color */
	    break;
#endif

    case POINTSTYLE:
    case LINESPOINTS:
	/* 1 column: y coordinate only */
	/* 2 columns x and y coordinates */
	/* Allow 1 extra column because of 'pointsize variable' */
	/* Allow 1 extra column because of 'pointtype variable' */
	/* Allow 1 extra column because of 'lc rgb variable'    */
	min_cols = 1;
	max_cols = 5;
	break;

    case PARALLELPLOT:
	/* Maximum number of parallel axes is fixed at compile time */
	if (current_plot->n_par_axes > num_parallel_axes)
	    extend_parallel_axis(current_plot->n_par_axes);

	/* First N columns are data; one more is optional varcolor */
	min_cols = current_plot->n_par_axes;
	max_cols = current_plot->n_par_axes + 1;
	/* We have not yet read in any data, so we cannot do complete initialization */
	for (j = 0; j < current_plot->n_par_axes; j++) {
	    struct axis *this_axis = &parallel_axis[j];
	    axis_init(this_axis, 1);
	}
	break;

    case TABLESTYLE:
	min_cols = 1;
	max_cols = MAXDATACOLS;
	break;

    default:
	min_cols = 1;
	max_cols = 2;
	break;
    }

    /* Restictions on plots with "smooth" option */
    switch (current_plot->plot_smooth) {
    case SMOOTH_NONE:
	break;
    case SMOOTH_ACSPLINES:
	max_cols = 3;
	current_plot->z_axis = FIRST_Z_AXIS;
	df_axis[2] = FIRST_Z_AXIS;
	break;
    default:
	if (df_no_use_specs > 2)
	    int_warn(NO_CARET, "extra columns ignored by smoothing option");
	break;
    }

    /* EXPERIMENTAL May 2013 - Treating timedata columns as strings allows */
    /* functions column(N) and column("HEADER") to work on time data.	   */
    /* Sep 2014: But the column count is wrong for HISTOGRAMS */
    if (current_plot->plot_style != HISTOGRAMS) {
	if (axis_array[current_plot->x_axis].datatype == DT_TIMEDATE)
	    expect_string(1);
	if (axis_array[current_plot->y_axis].datatype == DT_TIMEDATE)
	    expect_string(2);
    }

    if (df_no_use_specs > max_cols)
	int_error(NO_CARET, "Too many using specs for this style");

    if (df_no_use_specs > 0 && df_no_use_specs < min_cols)
	int_error(NO_CARET, "Not enough columns for this style");

    i = 0; ngood = 0;

    /* If the user has set an explicit locale for numeric input, apply it */
    /* here so that it affects data fields read from the input file.      */
    set_numeric_locale();

    /* Initial state */
    df_warn_on_missing_columnheader = TRUE;

    while ((j = df_readline(v, max_cols)) != DF_EOF) {

	if (i >= current_plot->p_max) {
	    /* overflow about to occur. Extend size of points[]
	     * array. Double the size, and add 1000 points, to avoid
	     * needlessly small steps. */
	    cp_extend(current_plot, i + i + 1000);
	}

	/* Version 5
	 * We are now trying to pass back all available info even if one of the requested
	 * columns was missing or undefined.  This check replaces the DF_UNDEFINED case in
	 * the main switch statement below.
	 */
	if (j == DF_UNDEFINED) {
	    current_plot->points[i].type = UNDEFINED;
	    if (missing_val && !strcmp(missing_val, "NaN"))
		j = DF_MISSING;
	    else
		j = df_no_use_specs;
	} else {
	    /* Assume range is OK; we will check later */
	    current_plot->points[i].type = INRANGE;
	}

	if (j > 0) {
	    ngood++;
	    /* June 2010 - New mechanism for variable color                  */
	    /* If variable color is requested, take the color value from the */
	    /* final column of input and decrement the column count by one.  */
	    if (current_plot->varcolor) {
		static char *errmsg = "Not enough columns for variable color";
		switch (current_plot->plot_style) {

		case CANDLESTICKS:
		case FINANCEBARS:
				if (j < 6) int_error(NO_CARET,errmsg);
				break;
		case XYERRORLINES:
		case XYERRORBARS:
		case BOXXYERROR:
				if (j != 7 && j != 5) int_error(NO_CARET,errmsg);
				break;
		case VECTOR:
				if (j < 5) int_error(NO_CARET,errmsg);
				break;
		case LABELPOINTS:
		case BOXERROR:
		case XERRORLINES:
		case XERRORBARS:
		case YERRORLINES:
		case YERRORBARS:
				if (j < 4) int_error(NO_CARET,errmsg);
				break;
#ifdef EAM_OBJECTS
		case CIRCLES: 
				if (j == 5 || j < 3) int_error(NO_CARET,errmsg);
				break;
		case ELLIPSES:
#endif
		case BOXES:
		case POINTSTYLE:
		case LINESPOINTS:
		case IMPULSES:
		case LINES:
		case DOTS:
				if (j < 3) int_error(NO_CARET,errmsg);
				break;
		case PARALLELPLOT:
				if (j < 4) int_error(NO_CARET,errmsg);
				break;
		case BOXPLOT:
				/* Only the key sample uses this value */
				v[j++] = current_plot->base_linetype + 1;
				break;
		default:
		    break;
		}

		current_plot->varcolor[i] = v[--j];
	    }

	    if (current_plot->plot_style == TABLESTYLE) {
		/* tabulate_one_line()
		 * returns TRUE if the line was accepted and written out
		 */
		tabulate_one_line(v, df_strings, j);
		continue;
	    }

	}

	/* TODO: It would make more sense to organize the switch below by plot	*/
	/* type rather than by number of columns in use.  The mis-organization 	*/
	/* is particularly evident for parallel axis plots, to the point where	*/
	/* I decided the only reasonable option is to handle it separately.	*/
	if (current_plot->plot_style == PARALLELPLOT && j > 0) {
	    int iaxis;
	    if (j != current_plot->n_par_axes)
		int_error(NO_CARET, "Expecting %d input columns, got %d\n",
			current_plot->n_par_axes, j);
	    /* Primary coordinate structure holds only x range and 1st y value.	*/
	    /* The x range brackets the parallel axes by 0.5 on either side.	*/
	    store2d_point(current_plot, i, 1.0, v[0], 
				0.5, (double)(current_plot->n_par_axes)+0.5,
				v[0], v[0], 0.0);
	    /* The parallel axis data is stored in separate arrays */
	    for (iaxis = 0; iaxis < current_plot->n_par_axes; iaxis++) {
		int dummy_type = INRANGE;
		ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE( current_plot->z_n[iaxis][i],
			v[iaxis], dummy_type,
			&parallel_axis[iaxis],
			current_plot->noautoscale, NOOP, NOOP );
	    }
	    i++;

	} else {
	/* This "else" block currently handles all plot styles other than PARALLEL_AXES */

	switch (j) {
	default:
	    {
		df_close();
		int_error(c_token, "internal error : df_readline returned %d : datafile line %d", j, df_line_number);
	    }

	case DF_MISSING:
	    /* Plot type specific handling of missing points goes here. */
	    if (current_plot->plot_style == HISTOGRAMS) {
		current_plot->points[i].type = UNDEFINED;
		i++;
		continue;
	    }

	    /* Jun 2006 - Return to behavior of 3.7 and current docs:
	     *            do not interrupt plotted line because of missing data
	     */
	    FPRINTF((stderr,"Missing datum %d\n", i));
	    continue;

	case DF_UNDEFINED:
	    /* Version 5:  can't get here because we trapped DF_UNDEFINED above */
#if (0)
	    current_plot->points[i].type = UNDEFINED;
	    FPRINTF((stderr,"undefined point %g %g %g\n", v[0], v[1], v[2]));
	    if (current_plot->plot_style == IMAGE)
		goto images;
	    i++;
#endif
	    continue;

	case DF_FIRST_BLANK:
	    /* The binary input routines generate DF_FIRST_BLANK at the end
	     * of scan lines, so that the data may be used for the isometric
	     * splots.  Rather than turning that off inside the binary
	     * reading routine based upon the plot mode, DF_FIRST_BLANK is
	     * ignored for certain plot types requiring 3D coordinates in
	     * MODE_PLOT.
	     */
	    if (current_plot->plot_style == IMAGE
	    ||  current_plot->plot_style == RGBIMAGE
	    ||  current_plot->plot_style == RGBA_IMAGE)
		continue;

	    /* make type of next point undefined, but recognizable */
	    current_plot->points[i] = blank_data_line;
	    i++;
	    continue;

	case DF_SECOND_BLANK:
	    /* second blank line. We dont do anything
	     * (we did everything when we got FIRST one)
	     */
	    continue;

	case DF_FOUND_KEY_TITLE:
	    df_set_key_title(current_plot);
	    continue;
	case DF_KEY_TITLE_MISSING:
	    fprintf(stderr,"get_data: key title not found in requested column\n");
	    continue;

	case DF_COLUMN_HEADERS:
	    continue;

	case 0:         /* not blank line, but df_readline couldn't parse it */
	    {
		df_close();
		int_error(current_plot->token, "Bad data on line %d of file %s",
			  df_line_number, df_filename ? df_filename : ""); 
	    }

	case 1:
	    /* only one number */
	    if (default_smooth_weight(current_plot->plot_smooth)) { 
		v[1] = 1.0;
	    } else {
		/* x is index, assign number to y */
		v[1] = v[0];
		v[0] = df_datum;
		/* nobreak */
	    }

	case 2:
	    H_ERR_BARS:
	    if (current_plot->plot_style == HISTOGRAMS) {
		if (histogram_opts.type == HT_ERRORBARS) {
		    /* The code is a tangle, but we can get here with j = 1, 2, or 3 */
		    if (j == 1)
			int_error(c_token, "Not enough columns in using specification");
		    else if (j == 2) {
		 	v[3] = v[0] + v[1];
			v[2] = v[0] - v[1];
		    } else {
		 	v[3] = v[2];
			v[2] = v[1];
		    }
		    v[1] = v[0];
		    v[0] = df_datum;
		} else if (j >= 2)
		    int_error(c_token, "Too many columns in using specification");
		else
		    v[2] = v[3] = v[1];

		if (histogram_opts.type == HT_STACKED_IN_TOWERS) {
		    histogram_rightmost = current_plot->histogram_sequence
					+ current_plot->histogram->start;
		    current_plot->histogram->end = histogram_rightmost;
		} else if (v[0] + current_plot->histogram->start > histogram_rightmost) {
		    histogram_rightmost = v[0] + current_plot->histogram->start;
		    current_plot->histogram->end = histogram_rightmost;
		}
		/* Histogram boxwidths are always absolute */
		if (boxwidth > 0)
		    store2d_point(current_plot, i++, v[0], v[1],
				  v[0] - boxwidth / 2, v[0] + boxwidth / 2,
				  v[2], v[3], 0.0);
		else
		    store2d_point(current_plot, i++, v[0], v[1],
				  v[0] - 0.5, v[0] + 0.5,
				  v[2], v[3], 0.0);

		/* x, y */
		/* ylow and yhigh are same as y */

	    } else if ( (current_plot->plot_style == BOXES)
		     && boxwidth > 0 && boxwidth_is_absolute) {
		    /* calculate width now */
		    if (axis_array[current_plot->x_axis].log) {
			double base = axis_array[current_plot->x_axis].base;
			store2d_point(current_plot, i++, v[0], v[1],
				      v[0] * pow(base, -boxwidth/2.), v[0] * pow(base, boxwidth/2.),
				      v[1], v[1], 0.0);
		    } else
			store2d_point(current_plot, i++, v[0], v[1],
				      v[0] - boxwidth / 2, v[0] + boxwidth / 2,
				      v[1], v[1], 0.0);

#ifdef EAM_OBJECTS
	    } else if (current_plot->plot_style == CIRCLES) {
		    /* x, y, default radius, full circle */
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
		    		  0., 360., DEFAULT_RADIUS);
		}  else if (current_plot->plot_style == ELLIPSES) {
			/* x, y, major axis = minor axis = default, default orientation */
		    store2d_point(current_plot, i++, v[0], v[1], 0.0, 0.0,
		    		  0.0, 0.0, DEFAULT_ELLIPSE);

#endif
	    } else if (current_plot->plot_style == YERRORBARS) {
		/* x is index, assign number to y */
		v[2] = v[1];
		v[1] = v[0];
		v[0] = df_datum;
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
			      v[1] - v[2], v[1] + v[2], -1.0);
	    } else if (current_plot->plot_style == BOXPLOT) {
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0], v[1], v[1],
				DEFAULT_BOXPLOT_FACTOR);
	    } else if (current_plot->plot_style == FILLEDCURVES) {
		v[2] = current_plot->filledcurves_options.at;
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[1], v[2], -1.0);
	    } else {
		double w;
		if (current_plot->plot_style == CANDLESTICKS
		    || current_plot->plot_style == FINANCEBARS) {
		    int_warn(storetoken, "This plot style does not work with 1 or 2 cols. Setting to points");
		    current_plot->plot_style = POINTSTYLE;
		}
		if (current_plot->plot_smooth == SMOOTH_ACSPLINES)
		    w = 1.0;	/* Unit weights */
		else
		    w = -1.0;	/* Auto-width boxes in some styles */
		/* Set x/y high/low to exactly [x,y] */
		store2d_point(current_plot, i++, v[0], v[1], 
						 v[0], v[0], v[1], v[1], w);
	    }
	    break;


	case 3:
	    /* x, y, ydelta OR x, y, xdelta OR x, y, width */
	    if (current_plot->plot_smooth == SMOOTH_ACSPLINES)
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0], v[1],
			      v[1], v[2]);
	    else
		switch (current_plot->plot_style) {

		case HISTOGRAMS:
		    if (histogram_opts.type == HT_ERRORBARS)
			goto H_ERR_BARS;
		    else
			/* fall through */
		default:
		    int_warn(storetoken, "This plot style does not work with 3 cols. Setting to yerrorbars");
		    current_plot->plot_style = YERRORBARS;
		    /* fall through */

		case FILLEDCURVES:
		    current_plot->filledcurves_options.closeto = FILLEDCURVES_BETWEEN;
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[1], v[2], -1.0);
		    break;

		case YERRORLINES:
		case YERRORBARS:
		case BOXERROR:  /* x, y, dy */
		    /* auto width if boxes, else ignored */
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[1] - v[2], v[1] + v[2], -1.0);
		    break;

		case XERRORLINES:
		case XERRORBARS:
		    store2d_point(current_plot, i++, v[0], v[1], v[0] - v[2],
				  v[0] + v[2], v[1], v[1], 0.0);
		    break;

		case BOXES:
		    /* calculate xmin and xmax here, so that logs are taken if if necessary */
		    store2d_point(current_plot, i++, v[0], v[1],
				  v[0] - v[2] / 2, v[0] + v[2] / 2,
				  v[1], v[1], 0.0);
		    break;

		case LABELPOINTS:
		    /* Load the coords just as we would have for a point plot */
		    store2d_point(current_plot, i, v[0], v[1], v[0], v[0], v[1],
				  v[1], -1.0);
		    /* Allocate and fill in a text_label structure to match it */
		    if (current_plot->points[i].type != UNDEFINED)
			store_label(current_plot->labels, &(current_plot->points[i]), 
				i, df_tokens[2], 
				current_plot->varcolor ? current_plot->varcolor[i] : 0.0);
		    i++;
		    break;

		case IMAGE:  /* x_center y_center color_value */
		    store2d_point(current_plot, i, v[0], v[1], v[0], v[0], v[1],
				  v[1], v[2]);
		    cp = &(current_plot->points[i]);
		    COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, v[2], cp->type,
			COLOR_AXIS, current_plot->noautoscale, NOOP, NOOP);
		    i++;
		    break;

		case POINTSTYLE: /* x, y, variable point size or type */
		case LINESPOINTS:
		case IMPULSES:
		case LINES:
		case DOTS:
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[2],
				  v[1], v[1], v[2]);
		    break;

		case BOXPLOT:	/* x, y, width expanded to xlow, xhigh */
		    store2d_point(current_plot, i++, v[0], v[1], v[0]-v[2]/2., v[0]+v[2]/2.,
		    		  v[1], v[1], DEFAULT_BOXPLOT_FACTOR);
		    break;

#ifdef EAM_OBJECTS
		case CIRCLES:	/* x, y, radius */
		    /* by default a full circle is drawn */
		    /* negative radius means default radius -> set flag in width */
		    store2d_point(current_plot, i++, v[0], v[1], v[0]-v[2], v[0]+v[2],
		    		  0.0, 360.0, (v[2] >= 0) ? 0.0 : DEFAULT_RADIUS);
		    break;

		case ELLIPSES:	/* x, y, major axis = minor axis, 0 as orientation */
		    store2d_point(current_plot, i++, v[0], v[1], fabs(v[2]), fabs(v[2]),
		    		  0.0, v[2], (v[2] >= 0) ? 0.0 : DEFAULT_RADIUS);
		    break;
#endif
		}               /*inner switch */

	    break;



	case 4:
	    /* x, y, ylow, yhigh OR
	     * x, y, xlow, xhigh OR
	     * x, y, xdelta, ydelta OR
	     * x, y, ydelta, width
	     */

	    switch (current_plot->plot_style) {
	    default:
		int_warn(storetoken, "This plot style does not work with 4 cols. Setting to yerrorbars");
		current_plot->plot_style = YERRORBARS;
		/* fall through */

	    case YERRORLINES:
	    case YERRORBARS:
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0], v[2],
			      v[3], -1.0);
		break;

	    case BOXXYERROR:    /* x, y, dx, dy */
	    case XYERRORLINES:
	    case XYERRORBARS:
		store2d_point(current_plot, i++, v[0], v[1],
			      v[0] - v[2], v[0] + v[2],
			      v[1] - v[3], v[1] + v[3], 0.0);
		break;


	    case BOXES:
		/* x, y, xmin, xmax */
		store2d_point(current_plot, i++, v[0], v[1], v[2], v[3],
			      v[1], v[1], 0.0);
		break;

	    case XERRORLINES:
	    case XERRORBARS:
		/* x, y, xmin, xmax */
		store2d_point(current_plot, i++, v[0], v[1], v[2], v[3],
			      v[1], v[1], 0.0);
		break;

	    case BOXERROR:
		if (boxwidth == -2)
		    /* x,y, ylow, yhigh --- width automatic */
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[2], v[3], -1.0);
		else
		    /* x, y, dy, width */
		    store2d_point(current_plot, i++, v[0], v[1],
				  v[0] - v[3] / 2, v[0] + v[3] / 2,
				  v[1] - v[2], v[1] + v[2], 0.0);
		break;

	    case BOXPLOT:	/* x, y, factor */
		{
		int factor_index = check_or_add_boxplot_factor(current_plot, df_tokens[3], v[0]);
		store2d_point(current_plot, i++, v[0], v[1], v[0]-v[2]/2., v[0]+v[2]/2.,
		    		  v[1], v[1], factor_index);
		}
		break;

	    case VECTOR:
		/* x,y,dx,dy */
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0] + v[2],
			      v[1], v[1] + v[3], 0.);
		break;

	    case LABELPOINTS:
		/* Load the coords just as we would have for a point plot */
		store2d_point(current_plot, i, v[0], v[1],
				v[0], v[0], v[1], v[1], v[3]);
		/* Allocate and fill in a text_label structure to match it */
		if (current_plot->points[i].type != UNDEFINED) {
		    struct text_label *tl;
		    tl = store_label(current_plot->labels, &(current_plot->points[i]), 
			    i, df_tokens[2], 
			    current_plot->varcolor ? current_plot->varcolor[i] : 0.0);
		    if (current_plot->labels->tag == VARIABLE_ROTATE_LABEL_TAG)
			tl->rotate = (int)(v[3]);
		    else
			tl->lp_properties.p_size = v[3];
		}
		i++;
		break;

	    case POINTSTYLE:
	    case LINESPOINTS:
		/* Either there is no using spec and more than 3 columns in the data file */
		/* or this is x:y:variable_size:variable_type */
		store2d_point(current_plot, i++, v[0], v[1], 
				v[0], v[3], v[1], v[1], v[2]);
		break;


#ifdef EAM_OBJECTS
	    case ELLIPSES:	/* x, y, major axis, minor axis, 0 as orientation */
		store2d_point(current_plot, i++, v[0], v[1], fabs(v[2]), fabs(v[3]),
				0.0, v[2], ((v[2] >= 0) && (v[3] >= 0)) ? 0.0 : DEFAULT_RADIUS);
		break;
#endif

	    }                   /*inner switch */

	    break;


	case 5:
	    {   /* x, y, ylow, yhigh, width  or  x open low high close */
		switch (current_plot->plot_style) {
		default:
		    int_warn(storetoken, "Unrecognized 5 column plot style; resetting to boxerrorbars");
		    current_plot->plot_style = BOXERROR;
		    /*fall through */

		case BOXERROR:  /* x, y, ylow, yhigh, width */
		    store2d_point(current_plot, i++, v[0], v[1],
				  v[0] - v[4] / 2, v[0] + v[4] / 2,
				  v[2], v[3], 0.0);
		    break;

		case FINANCEBARS: /* x yopen ylow yhigh yclose */
		case CANDLESTICKS:
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[2], v[3], v[4]);
		    break;

		case VECTOR:
		    /* x,y,dx,dy, variable arrowstyle */
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0] + v[2],
				  v[1], v[1] + v[3], v[4]);
		    break;

#ifdef EAM_OBJECTS
		case CIRCLES:	/* x, y, radius, arc begin, arc end */
		    /* negative radius means default radius -> set flag in width */
		    store2d_point(current_plot, i++, v[0], v[1], v[0]-v[2], v[0]+v[2],
		    		  v[3], v[4], (v[2] >= 0) ? 0.0 : DEFAULT_RADIUS);
		    break;

		case ELLIPSES:	/* x, y, major axis, minor axis, orientation */
		    store2d_point(current_plot, i++, v[0], v[1], fabs(v[2]), fabs(v[3]),
		    		  v[4], v[2], ((v[2] >= 0) && (v[3] >= 0)) ? 0.0 : DEFAULT_RADIUS);
		    break;
#endif

		case RGBIMAGE:  /* x_center y_center r_value g_value b_value (rgb) */
		    goto images;

		case POINTSTYLE:
		case LINESPOINTS:
		    /* If there is no using spec and more than 4 columns in the data file */
		    /* then use only the first 4 columns  x:y:variable_size:variable_type */
		    store2d_point(current_plot, i++, v[0], v[1], 
				    v[0], v[3], v[1], v[1], v[2]);
		    break;

		}               /* inner switch */


		break;
	    }

	case 7:
	    /* same as six columns. Width ignored */
	    /* eh ? - fall through */
	case 6:
	    /* x, y, xlow, xhigh, ylow, yhigh */
	    switch (current_plot->plot_style) {
	    default:
		int_warn(storetoken, "This plot style does not work with 6 cols. Setting to xyerrorbars");
		current_plot->plot_style = XYERRORBARS;
		/*fall through */
	    case XYERRORLINES:
	    case XYERRORBARS:
	    case BOXXYERROR:
		store2d_point(current_plot, i++, v[0], v[1], v[2], v[3], v[4],
			      v[5], 0.0);
		break;

	    case CANDLESTICKS:
		store2d_point(current_plot, i++, v[0], v[1],
				v[5] > 0 ? v[0]-v[5]/2. : v[0], v[0],
				v[2], v[3], v[4]);
		break;

images:
	    case RGBA_IMAGE:  /* x_cent y_cent red green blue alpha */
	    case RGBIMAGE:    /* x_cent y_cent red green blue */
		store2d_point(current_plot, i, v[0], v[1], v[0], v[0], v[1], v[1], v[2]);
		/* We will autoscale the RGB components to  a total range [0:255]
		 * so we don't need to do any fancy scaling here.
		 */
		cp = &(current_plot->points[i]);
		cp->CRD_R = v[2];
		cp->CRD_G = v[3];
		cp->CRD_B = v[4];
		cp->CRD_A = v[5];	/* Alpha channel */
		i++;
		break;
	    }

	}                       /*switch */
	}                       /* "else" case for all plot types */

    }                           /*while */

    /* If the plot style specifically requests a closed curve, we can make */
    /* this easier by duplicating the first point at the end of the curve. */
    if (current_plot->plot_style == FILLEDCURVES
    &&  current_plot->filledcurves_options.closeto == FILLEDCURVES_CLOSED) {
	if (i == current_plot->p_max) { 
	    /* This can happen for a filled curve generated by sampling */
	    cp_extend(current_plot, i+1);
	}
	current_plot->points[i++] = current_plot->points[0];
    }

    /* This removes an extra point caused by blank lines after data. */
    if (i > 0 && current_plot->points[i-1].type == UNDEFINED)
	i--;

    current_plot->p_count = i;
    cp_extend(current_plot, i); /* shrink to fit */

    /* Last chance to substitute input values for placeholders in plot title */
    df_set_key_title(current_plot);

    df_close();

    /* We are finished reading user input; return to C locale for internal use */
    reset_numeric_locale();

    return ngood;                   /* 0 indicates an 'empty' file */
}

/* called by get_data for each point */
static void
store2d_point(
    struct curve_points *current_plot,
    int i,                      /* point number */
    double x, double y,
    double xlow, double xhigh,
    double ylow, double yhigh,
    double width)               /* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */
{
    struct coordinate GPHUGE *cp = &(current_plot->points[i]);
    coord_type dummy_type = INRANGE;   /* sometimes we dont care about outranging */
    TBOOLEAN excluded_range = FALSE;

#ifdef BACKWARDS_COMPATIBLE
    /* jev -- pass data values thru user-defined function */
    /* div -- y is dummy variable 2 - copy value there */
    if (ydata_func.at) {
	struct value val;

	(void) Gcomplex(&ydata_func.dummy_values[0], y, 0.0);
	ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
	evaluate_at(ydata_func.at, &val);
	y = undefined ? 0.0 : real(&val);

	(void) Gcomplex(&ydata_func.dummy_values[0], ylow, 0.0);
	ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
	evaluate_at(ydata_func.at, &val);
	ylow = undefined ? 0 : real(&val);

	(void) Gcomplex(&ydata_func.dummy_values[0], yhigh, 0.0);
	ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
	evaluate_at(ydata_func.at, &val);
	yhigh = undefined ? 0 : real(&val);
    }
#endif

    /* FIXME this destroys any UNDEFINED flag assigned during input */
    cp->type = INRANGE;
    dummy_type = cp->type;

    if (polar) {
	double theta = x;
	AXIS *theta_axis = &axis_array[T_AXIS];

	/* "x" is really the polar angle theta,	so check it against trange. */
	if (theta < theta_axis->data_min)
	    theta_axis->data_min = theta;
	if (theta > theta_axis->data_max)
	    theta_axis->data_max = theta;
	if ( theta < theta_axis->min
	&&  (theta <= theta_axis->max || theta_axis->max == -VERYLARGE)) {
	    if ((theta_axis->autoscale & AUTOSCALE_MAX) == 0)
		excluded_range = TRUE;
	}
	if ( theta > theta_axis->max
	&&  (theta >= theta_axis->min || theta_axis->min == VERYLARGE)) {
	    if ((theta_axis->autoscale & AUTOSCALE_MIN) == 0)
		excluded_range = TRUE;
	}

	/* "y" at this point is really "r", so check it against rrange.	*/
	if (y < R_AXIS.data_min)
	    R_AXIS.data_min = y;
	if (y > R_AXIS.data_max)
	    R_AXIS.data_max = y;

	/* Convert from polar to cartesian coordinates and check ranges */
	if (polar_to_xy(x, y, &x, &y, TRUE) == OUTRANGE)
	    cp->type = OUTRANGE;

	/* Some plot styles use xhigh and yhigh for other quantities, */
	/* which polar mode transforms would break		      */
	if (current_plot->plot_style == CIRCLES) {
	    double radius = (xhigh - xlow)/2.0;
	    xlow = x - radius;
	    xhigh = x + radius;

	} else {
	    /* Jan 2017 - now skipping range check on rhigh, rlow */
	    (void) polar_to_xy(xhigh, yhigh, &xhigh, &yhigh, FALSE);
	    (void) polar_to_xy(xlow, ylow, &xlow, &ylow, FALSE);
	}
    }

    /* Version 5: Allow to store Inf or NaN 
     *  We used to exit immediately in this case rather than storing anything */
    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->x, x, cp->type, current_plot->x_axis,
			current_plot->noautoscale, NOOP, NOOP);
    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->y, y, cp->type, current_plot->y_axis,
			current_plot->noautoscale, NOOP, NOOP);

    switch (current_plot->plot_style) {
    case POINTSTYLE:		/* Only x and y are relevant to axis scaling */
    case LINES:
    case LINESPOINTS:
    case LABELPOINTS:
    case DOTS:
    case IMPULSES:
    case STEPS:
    case FSTEPS:
    case HISTEPS:
	cp->xlow = xlow;
	cp->xhigh = xhigh;
	cp->ylow = ylow;
	cp->yhigh = yhigh;
	break;
    case BOXES:			/* auto-scale to xlow xhigh */
	cp->ylow = ylow;
	cp->yhigh = yhigh;
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, cp->xlow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, current_plot->x_axis,
					current_plot->noautoscale, NOOP, cp->xhigh = -VERYLARGE);
	break;
    case BOXPLOT:			/* auto-scale to xlow xhigh, factor is already in z */
	cp->ylow = ylow;		/* ylow yhigh not really needed but store them anyway */
	cp->yhigh = yhigh;
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, cp->xlow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, current_plot->x_axis,
					current_plot->noautoscale, NOOP, cp->xhigh = -VERYLARGE);
	break;
#ifdef EAM_OBJECTS
    case CIRCLES:
	cp->yhigh = yhigh;
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, cp->xlow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, current_plot->x_axis,
					current_plot->noautoscale, NOOP, cp->xhigh = -VERYLARGE);
	cp->ylow = ylow;	/* arc begin */
	cp->xhigh = yhigh;	/* arc end */
	if (fabs(ylow) > 1000. || fabs(yhigh) > 1000.) /* safety check for insane arc angles */
	    cp->type = UNDEFINED;
	break;
    case ELLIPSES:
	/* We want to pass the parameters to the ellipse drawing routine as they are, 
	 * so we have to calculate the extent of the ellipses for autoscaling here. 
	 * Properly calculating the correct extent of a rotated ellipse, respecting 
	 * axis scales and all would be very hard. 
	 * So we just use the larger of the two axes, multiplied by some empirical factors 
	 * to ensure^Whope that all parts of the ellipses will be in the auto-scaled area. */
	/* xlow = major axis, xhigh = minor axis, ylow = orientation */
#define YRANGE_FACTOR ((current_plot->ellipseaxes_units == ELLIPSEAXES_YY) ? 1.0 : 1.4)
#define XRANGE_FACTOR ((current_plot->ellipseaxes_units == ELLIPSEAXES_XX) ? 1.1 : 1.0)
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow, x-0.5*GPMAX(xlow, xhigh)*XRANGE_FACTOR, 
					dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, 
					cp->xlow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, x+0.5*GPMAX(xlow, xhigh)*XRANGE_FACTOR, 
					dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, 
					cp->xhigh = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->ylow, y-0.5*GPMAX(xlow, xhigh)*YRANGE_FACTOR, 
					dummy_type, current_plot->y_axis, 
					current_plot->noautoscale, NOOP, 
					cp->ylow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->yhigh, y+0.5*GPMAX(xlow, xhigh)*YRANGE_FACTOR, 
					dummy_type, current_plot->y_axis, 
					current_plot->noautoscale, NOOP, 
					cp->yhigh = -VERYLARGE);
	/* So after updating the axes we re-store the parameters */
	cp->xlow = xlow;    /* major axis */
	cp->xhigh = xhigh;  /* minor axis */
	cp->ylow = ylow;    /* orientation */
	break;
#endif

    default:			/* auto-scale to xlow xhigh ylow yhigh */
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, cp->xlow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, current_plot->x_axis,
					current_plot->noautoscale, NOOP, cp->xhigh = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->ylow, ylow, dummy_type, current_plot->y_axis,
					current_plot->noautoscale, NOOP, cp->ylow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->yhigh, yhigh, dummy_type, current_plot->y_axis,
					current_plot->noautoscale, NOOP, cp->yhigh = -VERYLARGE);
	break;
    }

    /* HBB 20010214: if z is not used for some actual value, just
     * store 'width' to that axis and be done with it */
    if ((int)current_plot->z_axis == NO_AXIS)
	cp->z = width;
    else
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->z, width, dummy_type, current_plot->z_axis, 
					current_plot->noautoscale, NOOP, cp->z = -VERYLARGE);

    /* If we have variable color corresponding to a z-axis value, use it to autoscale */
    /* June 2010 - New mechanism for variable color */
    if (current_plot->lp_properties.pm3d_color.type == TC_Z && current_plot->varcolor)
	COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(current_plot->varcolor[i],
		current_plot->varcolor[i], dummy_type, 
		COLOR_AXIS, current_plot->noautoscale, NOOP, NOOP);

    /* July 2014 - Some points are excluded because they fall outside of trange	*/
    /* even though they would be inside the plot if drawn.			*/
    if (excluded_range)
	cp->type = EXCLUDEDRANGE;

}                               /* store2d_point */


/*
 * We abuse the labels structure to store a list of boxplot labels ("factors").
 * Check if <string> is already among the known factors, if not, add it to the list.
 */
static int
check_or_add_boxplot_factor(struct curve_points *plot, char* string, double x)
{
    char * trimmed_string;
    struct text_label *label, *prev_label, *new_label;
    int index = DEFAULT_BOXPLOT_FACTOR;

    /* If there is no factor column (4th using spec) fall back to a single boxplot */
    if (!string)
	return index;

    /* Remove the trailing garbage, quotes etc. from the string */ 
    trimmed_string = df_parse_string_field(string);

    if (strlen(trimmed_string) > 0) {
	TBOOLEAN new = FALSE;
	prev_label = plot->labels;
	if (!prev_label)
	    int_error(NO_CARET, "boxplot labels not initialized");
	for (label = prev_label->next; label; label = label->next, prev_label = prev_label->next) {
	    /* check if string is already stored */
	    if (!strcmp(trimmed_string, label->text))
		break;
	    /* If we are keeping a sorted list, test against current entry */
	    /* (insertion sort).					   */
	    if (boxplot_opts.sort_factors) {
		if (strcmp(trimmed_string, label->text) < 0) {
		    new = TRUE;
		    break;
		}
	    }
	}
	/* not found, so we add it now */
	if (!label || new) {
	    new_label = gp_alloc(sizeof(text_label),"boxplot label");
	    memcpy(new_label,plot->labels,sizeof(text_label));
	    new_label->next = label;
	    new_label->tag = plot->boxplot_factors++;
	    new_label->text = gp_strdup(trimmed_string);
	    new_label->place.x = plot->points[0].x;
	    prev_label->next = new_label;
	    label = new_label;
	}
	index = label->tag;
    }

    free(trimmed_string);
    return index;
}

/* Add tic labels to the boxplots, 
 * showing which level of the factor variable they represent */ 
static void
add_tics_boxplot_factors(struct curve_points *plot)
{
    AXIS_INDEX boxplot_labels_axis;
    text_label *this_label;
    int i = 0;

    boxplot_labels_axis = 
	boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X  ? FIRST_X_AXIS  :
	boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2 ? SECOND_X_AXIS : 
	x_axis;
    for (this_label = plot->labels->next; this_label;
	 this_label = this_label->next) {
	    add_tic_user( &axis_array[boxplot_labels_axis], this_label->text,
		plot->points->x + i * boxplot_opts.separation, -1);
	    i++;
    }
}

/* Autoscaling of box plots cuts off half of the box on each end. */
/* Add a half-boxwidth to the range in this case.  EAM Aug 2007   */
static void
box_range_fiddling(struct curve_points *plot)
{
    double xlow, xhigh;
    int i = plot->p_count - 1;

    if (i <= 0)
	return;
    if (axis_array[plot->x_axis].autoscale & AUTOSCALE_MIN) {
	if (plot->points[0].type != UNDEFINED && plot->points[1].type != UNDEFINED) {
	    if (boxwidth_is_absolute)
		xlow = plot->points[0].x - boxwidth;
	    else
		xlow = plot->points[0].x - (plot->points[1].x - plot->points[0].x) / 2.;
	    xlow = AXIS_DE_LOG_VALUE(plot->x_axis, xlow);
	    if (axis_array[plot->x_axis].min > xlow)
		axis_array[plot->x_axis].min = xlow;
	}
    }
    if (axis_array[plot->x_axis].autoscale & AUTOSCALE_MAX) {
	if (plot->points[i].type != UNDEFINED && plot->points[i-1].type != UNDEFINED) {
	    if (boxwidth_is_absolute)
		xhigh = plot->points[i].x + boxwidth;
	    else
		xhigh = plot->points[i].x + (plot->points[i].x - plot->points[i-1].x) / 2.;
	    xhigh = AXIS_DE_LOG_VALUE(plot->x_axis, xhigh);
	    if (axis_array[plot->x_axis].max < xhigh)
		axis_array[plot->x_axis].max = xhigh;
	}
    }
}

/* Autoscaling of boxplots with no explicit width cuts off the outer edges of the box */
static void
boxplot_range_fiddling(struct curve_points *plot)
{
    double extra_width;
    int N;

    if (plot->p_count <= 0)
	return;

    /* Create a tic label for each boxplot category */
    if (plot->boxplot_factors > 0) {
	if (boxplot_opts.labels != BOXPLOT_FACTOR_LABELS_OFF)
	    add_tics_boxplot_factors(plot);
    }

    /* Sort the points and removed any that are undefined */
    N = filter_boxplot(plot);
    plot->p_count = N;

    if (plot->points[0].type == UNDEFINED)
	int_error(NO_CARET,"boxplot has undefined x coordinate");

    extra_width = plot->points[0].xhigh - plot->points[0].xlow;
    if (extra_width == 0)
	extra_width = (boxwidth > 0 && boxwidth_is_absolute) ? boxwidth : 0.5;
    if (extra_width < 0)
	extra_width = -extra_width;

    if (axis_array[plot->x_axis].autoscale & AUTOSCALE_MIN) {
	if (axis_array[plot->x_axis].min >= plot->points[0].x)
	    axis_array[plot->x_axis].min -= 1.5 * extra_width;
	else if (axis_array[plot->x_axis].min >= plot->points[0].x - extra_width)
	    axis_array[plot->x_axis].min -= 1 * extra_width;
    }
    if (axis_array[plot->x_axis].autoscale & AUTOSCALE_MAX) {
	double nfactors = GPMAX( 0, plot->boxplot_factors - 1 );
	double plot_max = plot->points[0].x + nfactors * boxplot_opts.separation;
	if (axis_array[plot->x_axis].max <= plot_max)
	    axis_array[plot->x_axis].max = plot_max + 1.5 * extra_width;
	else if (axis_array[plot->x_axis].max <= plot_max + extra_width)
	    axis_array[plot->x_axis].max += extra_width;
    }
}

/* Since the stored x values for histogrammed data do not correspond exactly */
/* to the eventual x coordinates, we need to modify the x axis range bounds. */
/* Also the two stacked histogram modes need adjustment of the y axis bounds.*/
static void
histogram_range_fiddling(struct curve_points *plot)
{
    double xlow, xhigh;
    int i;
    /*
     * EAM FIXME - HT_STACKED_IN_TOWERS forcibly resets xmin, which is only
     *   correct if no other plot came first.
     */
    switch (histogram_opts.type) {
	case HT_STACKED_IN_LAYERS:
	    if (axis_array[plot->y_axis].autoscale & AUTOSCALE_MAX) {
		if (plot->histogram_sequence == 0) {
		    if (stackheight)
			free(stackheight);
		    stackheight = gp_alloc( plot->p_count * sizeof(struct coordinate GPHUGE),
					    "stackheight array");
		    for (stack_count=0; stack_count < plot->p_count; stack_count++) {
			stackheight[stack_count].yhigh = 0;
			stackheight[stack_count].ylow = 0;
		    }
		} else if (plot->p_count > stack_count) {
		    stackheight = gp_realloc( stackheight,
					    plot->p_count * sizeof(struct coordinate GPHUGE),
					    "stackheight array");
		    for ( ; stack_count < plot->p_count; stack_count++) {
			stackheight[stack_count].yhigh = 0;
			stackheight[stack_count].ylow = 0;
		    }
		}
		for (i=0; i<stack_count; i++) {
		    if (plot->points[i].type == UNDEFINED)
			continue;
		    if (plot->points[i].y >= 0)
			stackheight[i].yhigh += plot->points[i].y;
		    else
			stackheight[i].ylow += plot->points[i].y;

		    if (axis_array[plot->y_axis].max < stackheight[i].yhigh)
			axis_array[plot->y_axis].max = stackheight[i].yhigh;
		    if (axis_array[plot->y_axis].min > stackheight[i].ylow)
			axis_array[plot->y_axis].min = stackheight[i].ylow;

		}
	    }
		/* fall through to checks on x range */
	case HT_CLUSTERED:
	case HT_ERRORBARS:
		if (!axis_array[FIRST_X_AXIS].autoscale)
		    break;
		if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN) {
		    xlow = plot->histogram->start - 1.0;
		    if (axis_array[FIRST_X_AXIS].min > xlow)
			axis_array[FIRST_X_AXIS].min = xlow;
		}
		if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX) {
		    /* FIXME - why did we increment p_count on UNDEFINED points? */
		    while (plot->points[plot->p_count-1].type == UNDEFINED) {
			plot->p_count--;
			if (!plot->p_count)
			    int_error(NO_CARET,"All points in histogram UNDEFINED");
		    }
		    xhigh = plot->points[plot->p_count-1].x;
		    xhigh += plot->histogram->start + 1.0;
		    if (axis_array[FIRST_X_AXIS].max < xhigh)
			axis_array[FIRST_X_AXIS].max = xhigh;
		}
		break;
	case HT_STACKED_IN_TOWERS:
		/* FIXME: Rather than trying to reproduce the layout along X */
		/* we should just track the actual xmin/xmax as we go.       */
		if (axis_array[FIRST_X_AXIS].set_autoscale) {
		    if ((axis_array[FIRST_X_AXIS].set_autoscale & AUTOSCALE_MIN))
			xlow = -1.0;
		    xhigh = plot->histogram_sequence;
		    xhigh += plot->histogram->start + 1.0;
		    if (axis_array[FIRST_X_AXIS].min > xlow)
			axis_array[FIRST_X_AXIS].min = xlow;
		    if (axis_array[FIRST_X_AXIS].max != xhigh)
			axis_array[FIRST_X_AXIS].max  = xhigh;
		}
		if (axis_array[FIRST_Y_AXIS].set_autoscale) {
		    double ylow, yhigh;
		    for (i=0, yhigh=ylow=0.0; i<plot->p_count; i++)
			if (plot->points[i].type != UNDEFINED) {
			    if (plot->points[i].y >= 0)
				yhigh += plot->points[i].y;
			    else
				ylow += plot->points[i].y;
			}
		    if (axis_array[FIRST_Y_AXIS].set_autoscale & AUTOSCALE_MAX)
			if (axis_array[plot->y_axis].max < yhigh)
			    axis_array[plot->y_axis].max = yhigh;
		    if (axis_array[FIRST_Y_AXIS].set_autoscale & AUTOSCALE_MIN)
			if (axis_array[plot->y_axis].min > ylow)
			    axis_array[plot->y_axis].min = ylow;
		}
		break;
    }
}

/* If the plot is in polar coordinates and the r axis range is autoscaled,
 * we need to apply the maximum radius found to both x and y.
 * Otherwise the autoscaling will be done separately for x and y and the 
 * resulting plot will not be centered at the origin.
 */
void
polar_range_fiddling(struct curve_points *plot)
{
    if (axis_array[POLAR_AXIS].set_autoscale & AUTOSCALE_MAX) {
	double plotmax_x, plotmax_y, plotmax;
	plotmax_x = GPMAX(axis_array[plot->x_axis].max, -axis_array[plot->x_axis].min);
	plotmax_y = GPMAX(axis_array[plot->y_axis].max, -axis_array[plot->y_axis].min);
	plotmax = GPMAX(plotmax_x, plotmax_y);

	if ((axis_array[plot->x_axis].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) {
	    axis_array[plot->x_axis].max = plotmax;
	    axis_array[plot->x_axis].min = -plotmax;
	}
	if ((axis_array[plot->y_axis].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) {
	    axis_array[plot->y_axis].max = plotmax;
	    axis_array[plot->y_axis].min = -plotmax;
	}
    }
}

/* Extend auto-scaling of y-axis to include zero */
static void
impulse_range_fiddling(struct curve_points *plot)
{
    if (axis_array[plot->y_axis].log)
	return;

    if (axis_array[plot->y_axis].autoscale & AUTOSCALE_MIN) {
	if (axis_array[plot->y_axis].min > 0)
	    axis_array[plot->y_axis].min = 0;
    }
    if (axis_array[plot->y_axis].autoscale & AUTOSCALE_MAX) {
	if (axis_array[plot->y_axis].max < 0)
	    axis_array[plot->y_axis].max = 0;
    }
}


/* store_label() is called by get_data for each point */
/* This routine is exported so it can be shared by plot3d */
struct text_label *
store_label(
    struct text_label *listhead,
    struct coordinate *cp,
    int i,                      /* point number */
    char *string,               /* start of label string */
    double colorval)            /* used if text color derived from palette */
{
    static struct text_label *tl = NULL;
    int textlen;

    if (!listhead)
	int_error(NO_CARET,"text_label list was not initialized");

    /* If listhead->next is NULL, the list is currently empty and we will */
    /* insert this label at the head.  Otherwise tl already points to the */
    /* tail (previous insertion) and we will add the new label there.     */
    if (listhead->next == NULL)
	tl = listhead;

    /* Allocate a new label structure and fill it in */
    tl->next = gp_alloc(sizeof(struct text_label),"labelpoint label");
    memcpy(tl->next,tl,sizeof(text_label));
    tl = tl->next;
    tl->next = (text_label *)NULL;
    tl->tag = i;
    tl->place.x = cp->x;
    tl->place.y = cp->y;
    tl->place.z = cp->z;

    /* Check for optional (textcolor palette ...) */
    if (tl->textcolor.type == TC_Z)
	tl->textcolor.value = colorval;
    /* Check for optional (textcolor rgb variable) */
    else if (listhead->textcolor.type == TC_RGB && listhead->textcolor.value < 0)
	tl->textcolor.lt = colorval;
    /* Check for optional (textcolor variable) */
    else if (listhead->textcolor.type == TC_VARIABLE) {
	struct lp_style_type lptmp;
	if (prefer_line_styles)
	    lp_use_properties(&lptmp, (int)colorval);
	else
	    load_linetype(&lptmp, (int)colorval);
	tl->textcolor = lptmp.pm3d_color;
    }

    if ((listhead->lp_properties.flags & LP_SHOW_POINTS)) {
	/* Check for optional (point linecolor palette ...) */
	if (tl->lp_properties.pm3d_color.type == TC_Z)
	    tl->lp_properties.pm3d_color.value = colorval;
	/* Check for optional (point linecolor rgb variable) */
	else if (listhead->lp_properties.pm3d_color.type == TC_RGB 
		&& listhead->lp_properties.pm3d_color.value < 0)
	    tl->lp_properties.pm3d_color.lt = colorval;
	/* Check for optional (point linecolor variable) */
	else if (listhead->lp_properties.l_type == LT_COLORFROMCOLUMN) {
	    struct lp_style_type lptmp;
	    if (prefer_line_styles)
		lp_use_properties(&lptmp, (int)colorval);
	    else
		load_linetype(&lptmp, (int)colorval);
	    tl->lp_properties.pm3d_color = lptmp.pm3d_color;
	}
    }
    

    /* Check for null string (no label) */
    if (!string)
	string = "";

    textlen = 0;
    /* Handle quoted separators and quoted quotes */
    if (df_separators) {
	TBOOLEAN in_quote = FALSE;
	while (string[textlen]) {
	    if (string[textlen] == '"')
		in_quote = !in_quote;
	    else if (strchr(df_separators,string[textlen]) && !in_quote)
		break;
	    textlen++;
	}
	while (textlen > 0 && isspace((unsigned char)string[textlen-1]))
	    textlen--;
    } else {
    /* This is the normal case (no special separator character) */
	if (*string == '"') {
	    for (textlen=1; string[textlen] && string[textlen] != '"'; textlen++);
	}
	while (string[textlen] && !isspace((unsigned char)string[textlen]))
	    textlen++;
    }

    /* Strip double quote from both ends */
    if (string[0] == '"' && string[textlen-1] == '"')
	textlen -= 2, string++;

    tl->text = gp_alloc(textlen+1,"labelpoint text");
    strncpy( tl->text, string, textlen );
    tl->text[textlen] = '\0';
    parse_esc(tl->text);

    FPRINTF((stderr,"LABELPOINT %f %f \"%s\" \n", tl->place.x, tl->place.y, tl->text));

    return tl;
}

/* HBB 20010610: mnemonic names for the bits stored in 'uses_axis' */
typedef enum e_uses_axis {
    USES_AXIS_FOR_DATA = 1,
    USES_AXIS_FOR_FUNC = 2
} t_uses_axis;

/*
 * This parses the plot command after any global range specifications.
 * To support autoscaling on the x axis, we want any data files to define the
 * x range, then to plot any functions using that range. We thus parse the input
 * twice, once to pick up the data files, and again to pick up the functions.
 * Definitions are processed twice, but that won't hurt.
 */
static void
eval_plots()
{
    int i;
    struct curve_points *this_plot = NULL;
    struct curve_points **tp_ptr;
    t_uses_axis uses_axis[AXIS_ARRAY_SIZE];
    TBOOLEAN some_functions = FALSE;
    TBOOLEAN some_tables = FALSE;
    int plot_num, line_num;
    TBOOLEAN was_definition = FALSE;
    int pattern_num;
    char *xtitle = NULL;
    int begin_token = c_token;  /* so we can rewind for second pass */
    int start_token=0, end_token;
    legend_key *key = &keyT;
    char orig_dummy_var[MAX_ID_LEN+1];

    int nbins = 0;
    double binlow = 0, binhigh = 0, binwidth = 0;

    double newhist_start = 0.0;
    int histogram_sequence = -1;
    int newhist_color = 1;
    int newhist_pattern = LT_UNDEFINED;
    histogram_rightmost = 0.0;
    free_histlist(&histogram_opts);
    init_histogram(NULL,NULL);

    uses_axis[FIRST_X_AXIS] =
	uses_axis[FIRST_Y_AXIS] =
	uses_axis[SECOND_X_AXIS] =
	uses_axis[SECOND_Y_AXIS] = 0;

    /* Original Comment follows: */
    /* Reset first_plot. This is usually done at the end of this function.
     * If there is an error within this function, the memory is left allocated,
     * since we cannot call cp_free if the list is incomplete. Making sure that
     * the list structure is always valid requires some rewriting */
    /* EAM Apr 2007 - but we need to keep the previous structures around in 
     * order to be able to refresh/zoom them without re-reading all the data.
     */
    if (first_plot)
	cp_free(first_plot);
    first_plot = NULL;

    tp_ptr = &(first_plot);
    plot_num = 0;
    line_num = 0;               /* default line type */
    pattern_num = default_fillstyle.fillpattern;        /* default fill pattern */
    strcpy(orig_dummy_var, c_dummy_var[0]);
    in_parametric = FALSE;
    xtitle = NULL;

    /* Assume that the input data can be re-read later */
    volatile_data = FALSE;

    /* ** First Pass: Read through data files ***
     * This pass serves to set the xrange and to parse the command, as well
     * as filling in every thing except the function data. That is done after
     * the xrange is defined.
     */
    plot_iterator = check_for_iteration();
    while (TRUE) {

	/* Forgive trailing comma on a multi-element plot command */
	if (END_OF_COMMAND) {
	    if (plot_num == 0)
		int_error(c_token, "function to plot expected");
	    break;
	}

	this_plot = NULL;
	if (!in_parametric && !was_definition)
	    start_token = c_token;

	if (almost_equals(c_token,"newhist$ogram")) {
	    struct lp_style_type lp = DEFAULT_LP_STYLE_TYPE;
	    struct fill_style_type fs;
	    int previous_token;
	    c_token++;
	    histogram_sequence = -1;
	    memset(&histogram_title, 0, sizeof(text_label));

	    if (histogram_rightmost > 0)
		newhist_start = histogram_rightmost + 2;

	    lp.l_type = line_num;
	    newhist_color = lp.l_type + 1;
	    fs.fillpattern = LT_UNDEFINED;

	    do {
		previous_token = c_token;

		if (equals(c_token,"at")) {
		    c_token++;
		    newhist_start = real_expression();
		}

		/* Store title in temporary variable and then copy into the */
		/* new histogram structure when it is allocated.            */
		if (!histogram_title.text && isstringvalue(c_token)) {
		    histogram_title.textcolor = histogram_opts.title.textcolor;
		    histogram_title.boxed = histogram_opts.title.boxed;
		    histogram_title.pos = histogram_opts.title.pos;
		    histogram_title.text = try_to_get_string();
		    histogram_title.font = gp_strdup(histogram_opts.title.font);
		    parse_label_options(&histogram_title, 2);
		}

		/* Allow explicit starting color or pattern for this histogram */
		if (equals(c_token,"lt") || almost_equals(c_token,"linet$ype")) {
		    c_token++;
		    newhist_color = int_expression();
		}
		parse_fillstyle(&fs, FS_SOLID, 100, fs.fillpattern, default_fillstyle.border_color);

		} while (c_token != previous_token);

	    newhist_pattern = fs.fillpattern;
	    if (!equals(c_token,","))
		int_error(c_token,"syntax error");
	} else

	if (is_definition(c_token)) {
	    define();
	    if (equals(c_token,","))
		c_token++;
	    was_definition = TRUE;
	    continue;

	} else {
	    int specs = 0;

	    /* for datafile plot, record datafile spec for title */
	    char* name_str;

	    TBOOLEAN duplication = FALSE;
	    TBOOLEAN set_smooth = FALSE, set_axes = FALSE, set_title = FALSE;
	    TBOOLEAN set_with = FALSE, set_lpstyle = FALSE;
	    TBOOLEAN set_fillstyle = FALSE;
	    TBOOLEAN set_fillcolor = FALSE;
	    TBOOLEAN set_labelstyle = FALSE;
#ifdef EAM_OBJECTS
	    TBOOLEAN set_ellipseaxes_units = FALSE;
#endif
	    t_colorspec fillcolor = DEFAULT_COLORSPEC;

	    /* CHANGE: Aug 2017
	     * Allow sampling both u and v so that it is possible to do
	     * plot sample [u=min:max:inc] [v=min:max:inc] '++' ... with image
	     */
	    t_value original_value_sample_var, original_value_sample_var2;
	    int sample_range_token, v_range_token;

	    plot_num++;

	    /* Check for a sampling range. */
	    init_sample_range(axis_array + FIRST_X_AXIS);
	    sample_range_token = parse_range(SAMPLE_AXIS);
	    v_range_token = 0;
	    if (sample_range_token != 0) {
		axis_array[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;
		/* If the sample was specifically on u we need to check v also */
		if (equals(sample_range_token, "u")) {
		    axis_array[U_AXIS].min = axis_array[SAMPLE_AXIS].min;
		    axis_array[U_AXIS].max = axis_array[SAMPLE_AXIS].max;
		    axis_array[U_AXIS].autoscale = axis_array[SAMPLE_AXIS].autoscale;
		    axis_array[U_AXIS].SAMPLE_INTERVAL = axis_array[SAMPLE_AXIS].SAMPLE_INTERVAL;
		    axis_array[U_AXIS].range_flags = axis_array[SAMPLE_AXIS].range_flags;
		    v_range_token = parse_range(V_AXIS);
		    if (v_range_token != 0)
			axis_array[V_AXIS].range_flags |= RANGE_SAMPLED;
		}
	    }

	    was_definition = FALSE;

	    /* Allow replacement of the dummy variable in a function */
	    if (sample_range_token > 0)
		copy_str(c_dummy_var[0], sample_range_token, MAX_ID_LEN);
	    else if (sample_range_token < 0)
		strcpy(c_dummy_var[0], set_dummy_var[0]);
	    else
		strcpy(c_dummy_var[0], orig_dummy_var);

	    dummy_func = &plot_func;	/* needed by parsing code */
	    name_str = string_or_express(NULL);
	    dummy_func = NULL;

	    if (name_str) { /* data file to plot */
		if (parametric && in_parametric)
		    int_error(c_token, "previous parametric function not fully specified");
		if (sample_range_token !=0 && *name_str != '+')
		    int_warn(sample_range_token, "Ignoring sample range in non-sampled data plot");

		if (*tp_ptr)
		    this_plot = *tp_ptr;
		else {          /* no memory malloc()'d there yet */
		    this_plot = cp_alloc(MIN_CRV_POINTS);
		    *tp_ptr = this_plot;
		}
		this_plot->plot_type = DATA;
		this_plot->plot_style = data_style;
		this_plot->plot_smooth = SMOOTH_NONE;
		this_plot->filledcurves_options.opt_given = 0;

		/* up to MAXDATACOLS cols */
		df_set_plot_mode(MODE_PLOT);    /* Needed for binary datafiles */
		specs = df_open(name_str, MAXDATACOLS, this_plot);

		/* Store a pointer to the named variable used for sampling */
		if (sample_range_token > 0)
		    this_plot->sample_var = add_udv(sample_range_token);
		else
		    this_plot->sample_var = add_udv_by_name(c_dummy_var[0]);
		if (v_range_token > 0)
		    this_plot->sample_var2 = add_udv(v_range_token);
		else
		    this_plot->sample_var2 = add_udv_by_name(c_dummy_var[1]);

		if (this_plot->sample_var->udv_value.type == ARRAY)
		    int_error(NO_CARET, "name conflict: dummy variable cannot be array");

		/* Save prior value of sample variables so we can restore them later */
		original_value_sample_var = this_plot->sample_var->udv_value;
		original_value_sample_var2 = this_plot->sample_var2->udv_value;
		this_plot->sample_var->udv_value.type = NOTDEFINED;
		this_plot->sample_var2->udv_value.type = NOTDEFINED;

		/* Not sure this is necessary */
		Gcomplex(&(this_plot->sample_var->udv_value), 0.0, 0.0);

		/* include modifiers in default title */
		this_plot->token = end_token = c_token - 1;

	    } else if (equals(c_token, "keyentry")) {
		c_token++;
		if (*tp_ptr)
		    this_plot = *tp_ptr;
		else {          /* no memory malloc()'d there yet */
		    this_plot = cp_alloc(MIN_CRV_POINTS);
		    *tp_ptr = this_plot;
		}
		this_plot->plot_type = KEYENTRY;
		this_plot->plot_style = LABELPOINTS;
		this_plot->token = end_token = c_token - 1;

	    } else { /* function to plot */

		some_functions = TRUE;
		if (parametric) /* working on x parametric function */
		    in_parametric = !in_parametric;
		if (*tp_ptr) {
		    this_plot = *tp_ptr;
		    cp_extend(this_plot, samples_1 + 1);
		} else {        /* no memory malloc()'d there yet */
		    this_plot = cp_alloc(samples_1 + 1);
		    *tp_ptr = this_plot;
		}
		this_plot->plot_type = FUNC;
		this_plot->plot_style = func_style;
		this_plot->filledcurves_options.opt_given = 0;
		end_token = c_token - 1;
	    }                   /* end of IS THIS A FILE OR A FUNC block */

	    /* axis defaults */
	    x_axis = FIRST_X_AXIS;
	    y_axis = FIRST_Y_AXIS;

	    /*  Set this before parsing any modifying options */
	    this_plot->base_linetype = line_num;

	    /* pm 25.11.2001 allow any order of options */
	    while (!END_OF_COMMAND) {
		int save_token = c_token;

		/* bin the data if requested */
		if (equals(c_token, "bins")) {
		    if (set_smooth) {
			duplication=TRUE;
			break;
		    }
		    c_token++;
		    this_plot->plot_smooth = SMOOTH_BINS;
		    nbins = samples_1;
		    if (equals(c_token, "=")) {
			c_token++;
			nbins = int_expression();
			if (nbins <= 0)
			    nbins = samples_1;
		    }
		    binlow = binhigh = 0.0;
		    if (equals(c_token, "binrange")) {
			c_token++;
			if (!parse_range(SAMPLE_AXIS))
			    int_error(c_token, "incomplete bin range");
			binlow = axis_array[SAMPLE_AXIS].min;
			binhigh = axis_array[SAMPLE_AXIS].max;
		    }
		    binwidth = -1;
		    if (equals(c_token, "binwidth")) {
			if (!equals(++c_token, "="))
			    int_error(c_token, "expecting binwidth=<width>");
			c_token++;
			binwidth = real_expression();
		    }
		    continue;
		}

		/*  deal with smooth */
		if (almost_equals(c_token, "s$mooth")) {
		    int found_token;

		    if (set_smooth) {
			duplication=TRUE;
			break;
		    }
		    found_token = lookup_table(plot_smooth_tbl, ++c_token);
		    c_token++;

		    switch(found_token) {
		    case SMOOTH_BINS:
			/* catch the "bins" keyword by itself on the next pass */
			c_token--;
			continue;
		    case SMOOTH_UNWRAP:
		    case SMOOTH_FREQUENCY:
		    case SMOOTH_FREQUENCY_NORMALISED:
			this_plot->plot_smooth = found_token;
			break;
		    case SMOOTH_KDENSITY:
			this_plot->smooth_parameter = -1; /* Default */
			if (almost_equals(c_token,"band$width")) {
			    c_token++;
			    this_plot->smooth_parameter = real_expression();
			}
			/* Fall through */
		    case SMOOTH_ACSPLINES:
		    case SMOOTH_BEZIER:
		    case SMOOTH_CSPLINES:
		    case SMOOTH_SBEZIER:
		    case SMOOTH_UNIQUE:
		    case SMOOTH_CUMULATIVE:
		    case SMOOTH_CUMULATIVE_NORMALISED:
		    case SMOOTH_MONOTONE_CSPLINE:
			this_plot->plot_smooth = found_token;
			this_plot->plot_style = LINES;
			break;
		    case SMOOTH_NONE:
		    default:
			int_error(c_token, "unrecognized 'smooth' option");
			break;
		    }
		    set_smooth = TRUE;
		    continue;
		}

		/* look for axes/axis */
		if (almost_equals(c_token, "ax$es")
		    || almost_equals(c_token, "ax$is")) {
		    if (set_axes) {
			duplication=TRUE;
			break;
		    }
		    if (parametric && in_parametric)
			int_error(c_token, "previous parametric function not fully specified");

		    c_token++;
		    switch(lookup_table(&plot_axes_tbl[0],c_token)) {
		    case AXES_X1Y1:
			x_axis = FIRST_X_AXIS;
			y_axis = FIRST_Y_AXIS;
			++c_token;
			break;
		    case AXES_X2Y2:
			x_axis = SECOND_X_AXIS;
			y_axis = SECOND_Y_AXIS;
			++c_token;
			break;
		    case AXES_X1Y2:
			x_axis = FIRST_X_AXIS;
			y_axis = SECOND_Y_AXIS;
			++c_token;
			break;
		    case AXES_X2Y1:
			x_axis = SECOND_X_AXIS;
			y_axis = FIRST_Y_AXIS;
			++c_token;
			break;
		    case AXES_NONE:
		    default:
			int_error(c_token, "axes must be x1y1, x1y2, x2y1 or x2y2");
			break;
		    }
		    set_axes = TRUE;
		    continue;
		}

		/* Allow this plot not to affect autoscaling */
		if (almost_equals(c_token, "noauto$scale")) {
		    c_token++;
		    this_plot->noautoscale = TRUE;
		    continue;
		}

		/* deal with title */
		parse_plot_title(this_plot, xtitle, NULL, &set_title);
		if (save_token != c_token)
		    continue;

		/* deal with style */
		if (almost_equals(c_token, "w$ith")) {
		    if (set_with) {
			duplication=TRUE;
			break;
		    }
		    if (parametric && in_parametric)
			int_error(c_token, "\"with\" allowed only after parametric function fully specified");
		    this_plot->plot_style = get_style();
		    if (this_plot->plot_style == FILLEDCURVES) {
			/* read a possible option for 'with filledcurves' */
			get_filledcurves_style_options(&this_plot->filledcurves_options);
		    }

		    if (this_plot->plot_style == IMAGE
		    ||  this_plot->plot_style == RGBIMAGE
		    ||  this_plot->plot_style == RGBA_IMAGE) {
			if (this_plot->plot_type != DATA)
			    int_error(c_token, "This plot style is only for data files");
			else
			    get_image_options(&this_plot->image_properties);
		    }

		    if ((this_plot->plot_type == FUNC) &&
			((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
			|| (this_plot->plot_style == LABELPOINTS)
			|| (this_plot->plot_style == PARALLELPLOT)
			))
			{
			    int_warn(c_token, "This plot style is only for datafiles, reverting to \"points\"");
			    this_plot->plot_style = POINTSTYLE;
			}

		    set_with = TRUE;
		    continue;
		}

		/* pick up line/point specs and other style-specific keywords
		 * - point spec allowed if style uses points, ie style&2 != 0
		 * - keywords for lt and pt are optional
		 */
		if (this_plot->plot_style == CANDLESTICKS) {
		    if (almost_equals(c_token,"whisker$bars")) {
			this_plot->arrow_properties.head = BOTH_HEADS;
			c_token++;
			if (isanumber(c_token) || type_udv(c_token) == INTGR || type_udv(c_token) == CMPLX)
			    this_plot->arrow_properties.head_length = real_expression();
		    }
		}

		if (this_plot->plot_style == VECTOR) {
		    int stored_token = c_token;

		    if (!set_lpstyle) {
			default_arrow_style(&(this_plot->arrow_properties));
			if (prefer_line_styles)
			    lp_use_properties(&(this_plot->arrow_properties.lp_properties), line_num+1);
			else
			    load_linetype(&(this_plot->arrow_properties.lp_properties), line_num+1);
		    }

		    arrow_parse(&(this_plot->arrow_properties), TRUE);
		    if (stored_token != c_token) {
			if (set_lpstyle) {
			    duplication=TRUE;
			    break;
			} else {
			    set_lpstyle = TRUE;
			    continue;
			}
		    }
		}

#ifdef EAM_OBJECTS
		/* pick up the special 'units' keyword the 'ellipses' style allows */
		if (this_plot->plot_style == ELLIPSES) {
		    int stored_token = c_token;
		    
		    if (!set_ellipseaxes_units)
		        this_plot->ellipseaxes_units = default_ellipse.o.ellipse.type;
		    if (almost_equals(c_token,"unit$s")) {
			c_token++;
		        if (equals(c_token,"xy")) {
		            this_plot->ellipseaxes_units = ELLIPSEAXES_XY;
		        } else if (equals(c_token,"xx")) {
		            this_plot->ellipseaxes_units = ELLIPSEAXES_XX;
		        } else if (equals(c_token,"yy")) {
		            this_plot->ellipseaxes_units = ELLIPSEAXES_YY;
		        } else {
		            int_error(c_token, "expecting 'xy', 'xx' or 'yy'" );
		        }
		        c_token++;
		    }
		    if (stored_token != c_token) {
			if (set_ellipseaxes_units) {
			    duplication=TRUE;
			    break;
			} else {
			    set_ellipseaxes_units = TRUE;
			    continue;
			}
		    }
		}
#endif

		/* Most plot styles accept line and point properties */
		/* but do not want font or text properties           */
		if (this_plot->plot_style != LABELPOINTS) {
		    int stored_token = c_token;
		    struct lp_style_type lp = DEFAULT_LP_STYLE_TYPE;
		    int new_lt = 0;

		    lp.l_type = line_num;
		    lp.p_type = line_num;
		    lp.d_type = line_num;

		    /* user may prefer explicit line styles */
		    if (prefer_line_styles)
			lp_use_properties(&lp, line_num+1);
		    else
			load_linetype(&lp, line_num+1);

		    if (this_plot->plot_style == BOXPLOT) {
			lp.p_type = boxplot_opts.pointtype;
			lp.p_size = PTSZ_DEFAULT;
		    }

		    new_lt = lp_parse(&lp, LP_ADHOC,
				     this_plot->plot_style & PLOT_STYLE_HAS_POINT);

		    if (stored_token != c_token) {
			if (set_lpstyle) {
			    duplication=TRUE;
			    break;
			} else {
			    this_plot->lp_properties = lp;
			    set_lpstyle = TRUE;
			    if (new_lt)
				this_plot->base_linetype = new_lt - 1;
			    if (this_plot->lp_properties.p_type != PT_CHARACTER)
				continue;
			}
		    }
		}

		/* Labels can have font and text property info as plot options */
		/* In any case we must allocate one instance of the text style */
		/* that all labels in the plot will share.                     */
		if ((this_plot->plot_style == LABELPOINTS)
		||  (this_plot->plot_style & PLOT_STYLE_HAS_POINT
			&& this_plot->lp_properties.p_type == PT_CHARACTER)) {
		    int stored_token = c_token;

		    if (this_plot->labels == NULL) {
			this_plot->labels = new_text_label(-1);
			this_plot->labels->pos = CENTRE;
			this_plot->labels->layer = LAYER_PLOTLABELS;
		    }
		    if ((this_plot->plot_style & PLOT_STYLE_HAS_POINT)
                    &&  (this_plot->lp_properties.p_type == PT_CHARACTER)) {
			if (!set_labelstyle)
			    this_plot->labels->textcolor.type = TC_DEFAULT;
		    }
		    parse_label_options(this_plot->labels, 2);
		    if (stored_token != c_token) {
			if (set_labelstyle) {
			    duplication = TRUE;
			    break;
			} else {
			    set_labelstyle = TRUE;
			    continue;
			}
		    } else if (this_plot->lp_properties.p_type == PT_CHARACTER) {
			if (equals(c_token, ","))
			    break;
			else
			    continue;
		    }
		}

		/* Some plots have a fill style as well */
		if (this_plot->plot_style & PLOT_STYLE_HAS_FILL){
		    int stored_token = c_token;
		    if (equals(c_token,"fs") || almost_equals(c_token,"fill$style")) {
			parse_fillstyle(&this_plot->fill_properties,
				default_fillstyle.fillstyle,
				default_fillstyle.filldensity,
				pattern_num,
				default_fillstyle.border_color);
			if (this_plot->plot_style == FILLEDCURVES
			&& this_plot->fill_properties.fillstyle == FS_EMPTY)
			    this_plot->fill_properties.fillstyle = FS_SOLID;
			set_fillstyle = TRUE;
		    }
		    if (equals(c_token,"fc") || almost_equals(c_token,"fillc$olor")) {
			parse_colorspec(&fillcolor, TC_VARIABLE);
			set_fillcolor = TRUE;
		    }
		    if (stored_token != c_token)
			continue;
		}

		break; /* unknown option */

	    } /* while (!END_OF_COMMAND) */

	    if (duplication)
		int_error(c_token, "duplicated or contradicting arguments in plot options");

	    if (this_plot->plot_style == TABLESTYLE) {
		if (!table_mode)
		    int_error(NO_CARET, "'with table' requires a previous 'set table'");
		expect_string( -1 );
		some_tables = TRUE;
	    }

	    /* set default values for title if this has not been specified */
	    this_plot->title_is_filename = FALSE;
	    if (!set_title) {
		this_plot->title_no_enhanced = TRUE; /* filename or function cannot be enhanced */
		if (key->auto_titles == FILENAME_KEYTITLES) {
		    m_capture(&(this_plot->title), start_token, end_token);
		    if (in_parametric)
			xtitle = this_plot->title;
		    this_plot->title_is_filename = TRUE;
		} else if (xtitle != NULL)
		    xtitle[0] = '\0';
	    }

	    /* Vectors will be drawn using linetype from arrow style, so we
	     * copy this to overall plot linetype so that the key sample matches */
	    if (this_plot->plot_style == VECTOR) {
		if (!set_lpstyle) {
		    if (prefer_line_styles)
			lp_use_properties(&(this_plot->arrow_properties.lp_properties), line_num+1);
		    else
			load_linetype(&(this_plot->arrow_properties.lp_properties), line_num+1);
		    arrow_parse(&this_plot->arrow_properties, TRUE);
		}
		this_plot->lp_properties = this_plot->arrow_properties.lp_properties;
		set_lpstyle = TRUE;
	    }
	    /* No line/point style given. As lp_parse also supplies
	     * the defaults for linewidth and pointsize, call it now
	     * to define them. */
	    if (!set_lpstyle) {
		this_plot->lp_properties.l_type = line_num;
		this_plot->lp_properties.l_width = 1.0;
		this_plot->lp_properties.p_type = line_num;
		this_plot->lp_properties.d_type = line_num;
		this_plot->lp_properties.p_size = pointsize;

		/* user may prefer explicit line styles */
		if (prefer_line_styles)
		    lp_use_properties(&this_plot->lp_properties, line_num+1);
		else
		    load_linetype(&this_plot->lp_properties, line_num+1);

		if (this_plot->plot_style == BOXPLOT) {
		    this_plot->lp_properties.p_type = boxplot_opts.pointtype;
		    this_plot->lp_properties.p_size = PTSZ_DEFAULT;
		}

		lp_parse(&this_plot->lp_properties, LP_ADHOC,
			 this_plot->plot_style & PLOT_STYLE_HAS_POINT);
	    }

	    /* If this plot style uses a fillstyle and we saw an explicit */
	    /* fill color, apply it now.				  */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_FILL){
		if (set_fillcolor)
		    this_plot->lp_properties.pm3d_color = fillcolor;
	    }

	    /* Some low-level routines expect to find the pointflag attribute */
	    /* in lp_properties (they don't have access to the full header.   */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		this_plot->lp_properties.flags |= LP_SHOW_POINTS;

	    /* Rule out incompatible line/point/style options */
	    if (this_plot->plot_type == FUNC) {
		if ((this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		&&  (this_plot->lp_properties.p_size == PTSZ_VARIABLE))
		    this_plot->lp_properties.p_size = 1;
	    }
	    if (polar) switch (this_plot->plot_style) {
		case LINES:
		case POINTSTYLE:
		case IMPULSES:
		case LINESPOINTS:
		case DOTS:
		case VECTOR:
		case FILLEDCURVES:
		case LABELPOINTS:
		case CIRCLES:
		case YERRORBARS:
		case YERRORLINES:
				break;
		default:
				int_error(NO_CARET, 
				    "This plot style is not available in polar mode");
	    }

	    /* If we got this far without initializing the fill style, do it now */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_FILL) {
		if (!set_fillstyle)
		    parse_fillstyle(&this_plot->fill_properties,
				default_fillstyle.fillstyle,
				default_fillstyle.filldensity,
				pattern_num,
				default_fillstyle.border_color);
		if ((this_plot->fill_properties.fillstyle == FS_PATTERN)
		  ||(this_plot->fill_properties.fillstyle == FS_TRANSPARENT_PATTERN))
		    pattern_num = this_plot->fill_properties.fillpattern + 1;
		if (this_plot->plot_style == FILLEDCURVES
		&& this_plot->fill_properties.fillstyle == FS_EMPTY)
		    this_plot->fill_properties.fillstyle = FS_SOLID;
	    }

	    this_plot->x_axis = x_axis;
	    this_plot->y_axis = y_axis;

	    /* If we got this far without initializing the character font, do it now */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT
	    &&  this_plot->lp_properties.p_type == PT_CHARACTER) {
		if (this_plot->labels == NULL) {
		    this_plot->labels = new_text_label(-1);
		    this_plot->labels->pos = CENTRE;
		    parse_label_options(this_plot->labels, 2);
		}
	    }

	    /* If we got this far without initializing the label list, do it now */
	    if (this_plot->plot_style == LABELPOINTS) {
		if (this_plot->labels == NULL) {
		    this_plot->labels = new_text_label(-1);
		    this_plot->labels->pos = CENTRE;
		    this_plot->labels->layer = LAYER_PLOTLABELS;
		}
		this_plot->labels->place.scalex =
		    (x_axis == SECOND_X_AXIS) ? second_axes : first_axes;
		this_plot->labels->place.scaley =
		    (y_axis == SECOND_Y_AXIS) ? second_axes : first_axes;

		/* Needed for variable color - June 2010 */
		this_plot->lp_properties.pm3d_color = this_plot->labels->textcolor;
		if (this_plot->labels->textcolor.type == TC_VARIABLE)
		    this_plot->lp_properties.l_type = LT_COLORFROMCOLUMN;

		/* We want to trigger the variable color mechanism even if 
		 * there was no 'textcolor variable/palette/rgb var' , 
		 * but there was a 'point linecolor variable/palette/rgb var'. */
		if ((this_plot->labels->lp_properties.flags & LP_SHOW_POINTS)
		&& this_plot->labels->textcolor.type != TC_Z
		&& this_plot->labels->textcolor.type != TC_VARIABLE
		&& (this_plot->labels->textcolor.type != TC_RGB 
		 || this_plot->labels->textcolor.value >= 0)) {
		    if ((this_plot->labels->lp_properties.pm3d_color.type == TC_RGB)
		    &&  (this_plot->labels->lp_properties.pm3d_color.value < 0)) {
		        this_plot->lp_properties.pm3d_color = this_plot->labels->lp_properties.pm3d_color;
		    }
		    if (this_plot->labels->lp_properties.pm3d_color.type == TC_Z)
		        this_plot->lp_properties.pm3d_color.type = TC_Z;
		    if (this_plot->labels->lp_properties.l_type == LT_COLORFROMCOLUMN)
		        this_plot->lp_properties.l_type = LT_COLORFROMCOLUMN;
		}
		 
	    }

	    /* Initialize the label list in case the BOXPLOT style needs it to store factors */
	    if (this_plot->plot_style == BOXPLOT) {
		if (this_plot->labels == NULL)
		    this_plot->labels = new_text_label(-1);
		/* We only use the list to store strings, so this is all we need here. */
	    }

	    /* Initialize histogram data structure */
	    if (this_plot->plot_style == HISTOGRAMS) {
		if (axis_array[x_axis].log)
		    int_error(c_token, "Log scale on X is incompatible with histogram plots\n");

		if ((histogram_opts.type == HT_STACKED_IN_LAYERS
		||   histogram_opts.type == HT_STACKED_IN_TOWERS)
		&&  axis_array[y_axis].log)
		    int_error(c_token, "Log scale on Y is incompatible with stacked histogram plot\n");
		this_plot->histogram_sequence = ++histogram_sequence;
		/* Current histogram always goes at the front of the list */
		if (this_plot->histogram_sequence == 0) {
		    this_plot->histogram = gp_alloc(sizeof(struct histogram_style), "New histogram");
		    init_histogram(this_plot->histogram, &histogram_title);
		    this_plot->histogram->start = newhist_start;
		    this_plot->histogram->startcolor = newhist_color;
		    this_plot->histogram->startpattern = newhist_pattern;
		} else {
		    this_plot->histogram = histogram_opts.next;
		    this_plot->histogram->clustersize++;
		}

		/* Normally each histogram gets a new set of colors, but in */
		/* 'newhistogram' you can force a starting color instead.   */
		if (!set_lpstyle && this_plot->histogram->startcolor != LT_UNDEFINED)
		    load_linetype(&this_plot->lp_properties, 
			this_plot->histogram_sequence + this_plot->histogram->startcolor);
		if (this_plot->histogram->startpattern != LT_UNDEFINED)
		    this_plot->fill_properties.fillpattern = this_plot->histogram_sequence
						    + this_plot->histogram->startpattern;
	    }

	    /* Styles that use palette */

	    /* we can now do some checks that we deferred earlier */

	    if (this_plot->plot_type == DATA) {
		if (specs < 0) {
		    /* Error check to handle missing or unreadable file */
		    ++line_num;
		    this_plot->plot_type = NODATA;
		    goto SKIPPED_EMPTY_FILE;
		}

		/* Parallel plots require allocating additional storage.		*/
		/* NB: This will be one column more than needed if the final column	*/
		/*     contains variable color information. We will free it later.	*/
		if (this_plot->plot_style == PARALLELPLOT) {
		    int i;
		    if (df_no_use_specs < 2)
			int_error(NO_CARET, "not enough 'using' columns");
		    this_plot->n_par_axes = df_no_use_specs;
		    this_plot->z_n = gp_alloc((df_no_use_specs) * sizeof(double*), "z_n");
		    for (i = 0; i < this_plot->n_par_axes; i++)
			this_plot->z_n[i] = gp_alloc(this_plot->p_max * sizeof(double), "z_n[i]");
		}
		
		/* Reset flags to auto-scale X axis to contents of data set */
		if (!(uses_axis[x_axis] & USES_AXIS_FOR_DATA) && X_AXIS.autoscale) {
		    struct axis *scaling_axis = &axis_array[this_plot->x_axis];
		    if (scaling_axis->autoscale & AUTOSCALE_MIN)
			scaling_axis->min = VERYLARGE;
		    if (scaling_axis->autoscale & AUTOSCALE_MAX)
			scaling_axis->max = -VERYLARGE;
		}
		if (X_AXIS.datatype == DT_TIMEDATE) {
		    if (specs < 2)
			int_error(c_token, "Need full using spec for x time data");
		}
		if (Y_AXIS.datatype == DT_TIMEDATE) {
		    if (specs < 1)
			int_error(c_token, "Need using spec for y time data");
		}
		/* need other cols, but I'm lazy */
		df_axis[0] = x_axis;
		df_axis[1] = y_axis;

		/* separate record of datafile and func */
		uses_axis[x_axis] |= USES_AXIS_FOR_DATA;
		uses_axis[y_axis] |= USES_AXIS_FOR_DATA;
	    } else if (!parametric || !in_parametric) {
		/* for x part of a parametric function, axes are
		 * possibly wrong */
		/* separate record of data and func */
		uses_axis[x_axis] |= USES_AXIS_FOR_FUNC;
		uses_axis[y_axis] |= USES_AXIS_FOR_FUNC;
	    }

	    if (!in_parametric
		&& this_plot->plot_style != IMAGE
		&& this_plot->plot_style != RGBIMAGE
		&& this_plot->plot_style != RGBA_IMAGE
		/* don't increment the default line/point properties if
		 * this_plot is an image */
	    ) {
		++line_num;
	    }
	    if (this_plot->plot_type == DATA) {

		/* get_data() will update the ranges of autoscaled axes, but some */
		/* plot modes, e.g. 'smooth cnorm' and 'boxplot' with nooutliers, */
		/* do not want all the points included in autoscaling.  Save the  */
		/* current autoscaled ranges here so we can restore them later.   */
		save_autoscaled_ranges(&axis_array[this_plot->x_axis], 
					&axis_array[this_plot->y_axis]);

		/* actually get the data now */
		if (get_data(this_plot) == 0) {
		    if (!forever_iteration(plot_iterator))
			int_warn(NO_CARET,"Skipping data file with no valid points");
		    this_plot->plot_type = NODATA;
		    goto SKIPPED_EMPTY_FILE;
		}

		/* Sep 2017 - Check for all points bad or out of range  */
		/* (normally harmless but must not cause infinite loop) */
		if (forever_iteration(plot_iterator)) {
		    int n, ninrange = 0;
		    for (n=0; n<this_plot->p_count; n++)
			if (this_plot->points[n].type == INRANGE)
			    ninrange++;
		    if (ninrange == 0) {
			this_plot->plot_type = NODATA;
			goto SKIPPED_EMPTY_FILE;
		    }
		}

		/* If we are to bin the data, do that first */
		if (this_plot->plot_smooth == SMOOTH_BINS) {
		    make_bins(this_plot, nbins, binlow, binhigh, binwidth);
		}

		/* Restore auto-scaling prior to smoothing operation */
		switch (this_plot->plot_smooth) {
		case SMOOTH_FREQUENCY:
		case SMOOTH_FREQUENCY_NORMALISED:
		case SMOOTH_CUMULATIVE:
		case SMOOTH_CUMULATIVE_NORMALISED:
		    restore_autoscaled_ranges(&axis_array[this_plot->x_axis], &axis_array[this_plot->y_axis]);
		    break;
		default:
		    break;
		}

		/* Fiddle the auto-scaling data for specific plot styles */
		if (this_plot->plot_style == HISTOGRAMS)
		    histogram_range_fiddling(this_plot);
		if (this_plot->plot_style == BOXES)
		    box_range_fiddling(this_plot);
		if (this_plot->plot_style == BOXPLOT)
		    boxplot_range_fiddling(this_plot);
		if (this_plot->plot_style == IMPULSES)
		    impulse_range_fiddling(this_plot);
		if (polar)
		    polar_range_fiddling(this_plot);

		/* sort */
		switch (this_plot->plot_smooth) {
		/* sort and average, if the style requires */
		case SMOOTH_UNIQUE:
		case SMOOTH_FREQUENCY:
		case SMOOTH_FREQUENCY_NORMALISED:
		case SMOOTH_CUMULATIVE:
		case SMOOTH_CUMULATIVE_NORMALISED:
		case SMOOTH_CSPLINES:
		case SMOOTH_ACSPLINES:
		case SMOOTH_SBEZIER:
		case SMOOTH_MONOTONE_CSPLINE:
		    sort_points(this_plot);
		    cp_implode(this_plot);
		case SMOOTH_NONE:
		case SMOOTH_BEZIER:
		case SMOOTH_KDENSITY:
		default:
		    break;
		}
		switch (this_plot->plot_smooth) {
		/* create new data set by evaluation of
		 * interpolation routines */
		case SMOOTH_UNWRAP:
		    gen_interp_unwrap(this_plot);
		    break;
		case SMOOTH_FREQUENCY:
		case SMOOTH_FREQUENCY_NORMALISED:
		case SMOOTH_CUMULATIVE:
		case SMOOTH_CUMULATIVE_NORMALISED:
		    /* These commands all replace the original data  */
		    /* so we must reevaluate min/max for autoscaling */
		    gen_interp_frequency(this_plot);
		    refresh_bounds(this_plot, 1);
		    break;
		case SMOOTH_CSPLINES:
		case SMOOTH_ACSPLINES:
		case SMOOTH_BEZIER:
		case SMOOTH_SBEZIER:
		    gen_interp(this_plot);
		    refresh_bounds(this_plot, 1);
		    break;
		case SMOOTH_KDENSITY:
		    gen_interp(this_plot);
		    fill_gpval_float("GPVAL_KDENSITY_BANDWIDTH", 
			fabs(this_plot->smooth_parameter));
		    break;
		case SMOOTH_MONOTONE_CSPLINE:
		    mcs_interp(this_plot);
		    break;
		case SMOOTH_NONE:
		case SMOOTH_UNIQUE:
		default:
		    break;
		}

		/* Images are defined by a grid representing centers of pixels.
		 * Compensate for extent of the image so `set autoscale fix`
		 * uses outer edges of outer pixels in axes adjustment.
		 */
		if ((this_plot->plot_style == IMAGE
		    || this_plot->plot_style == RGBIMAGE
		    || this_plot->plot_style == RGBA_IMAGE)) {
		    this_plot->image_properties.type = IC_PALETTE;
		    process_image(this_plot, IMG_UPDATE_AXES);
		}

	    }

	    SKIPPED_EMPTY_FILE:
	    /* Note position in command line for second pass */
		this_plot->token = c_token;
		tp_ptr = &(this_plot->next);

	    /* restore original value of sample variables */
	    if (name_str) {
		this_plot->sample_var->udv_value = original_value_sample_var;
		this_plot->sample_var2->udv_value = original_value_sample_var2;
	    }

	} /* !is_defn */

	if (in_parametric) {
	    if (equals(c_token, ",")) {
		c_token++;
		continue;
	    } else
		break;
	}

	/* Iterate-over-plot mechanism */
	if (empty_iteration(plot_iterator) && this_plot)
	    this_plot->plot_type = NODATA;
	if (forever_iteration(plot_iterator) && !this_plot)
	    int_error(NO_CARET,"unbounded iteration in something other than a data plot");
	else if (forever_iteration(plot_iterator) && (this_plot->plot_type == NODATA)) {
	    FPRINTF((stderr,"Ending * iteration at %d\n",plot_iterator->iteration));
	    /* Clearing the plot title ensures that it will not appear in the key */
	    free (this_plot->title);
	    this_plot->title = NULL;
	} else if (forever_iteration(plot_iterator) && (this_plot->plot_type != DATA)) {
	    int_error(NO_CARET,"unbounded iteration in something other than a data plot");
	} else if (next_iteration(plot_iterator)) {
	    c_token = start_token;
	    continue;
	}

	plot_iterator = cleanup_iteration(plot_iterator);
	if (equals(c_token, ",")) {
	    c_token++;
	    plot_iterator = check_for_iteration();
	} else
	    break;
    }

    if (parametric && in_parametric)
	int_error(NO_CARET, "parametric function not fully specified");


/*** Second Pass: Evaluate the functions ***/
    /*
     * Everything is defined now, except the function data. We expect
     * no syntax errors, etc, since the above parsed it all. This
     * makes the code below simpler. If y is autoscaled, the yrange
     * may still change.  we stored last token of each plot, so we
     * dont need to do everything again */

    /* parametric or polar fns can still affect x ranges */
    if (!parametric && !polar) {

	/* If we were expecting to autoscale on X but found no usable
	 * points in the data files, then the axis limits are still sitting
	 * at +/- VERYLARGE.  The default range for bare functions is [-10:10].
	 * Or we could give up and fall through to "x range invalid".
	 */
	if ((some_functions || some_tables) && uses_axis[FIRST_X_AXIS])
	    if (axis_array[FIRST_X_AXIS].max == -VERYLARGE ||
		axis_array[FIRST_X_AXIS].min == VERYLARGE) {
		    axis_array[FIRST_X_AXIS].min = -10;
		    axis_array[FIRST_X_AXIS].max = 10;
	}

	if (uses_axis[FIRST_X_AXIS] & USES_AXIS_FOR_DATA) {
	    /* check that x1min -> x1max is not too small */
	    axis_checked_extend_empty_range(FIRST_X_AXIS, "x range is invalid");
	}

	if (uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA) {
	    /* check that x2min -> x2max is not too small */
	    axis_checked_extend_empty_range(SECOND_X_AXIS, "x2 range is invalid");
	} else if (axis_array[SECOND_X_AXIS].autoscale) {
	    /* copy x1's range */
	    /* FIXME:  merge both cases into update_secondary_axis_range */
	    if (axis_array[SECOND_X_AXIS].linked_to_primary) {
		update_secondary_axis_range(&axis_array[FIRST_X_AXIS]);
	    } else {
		if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MIN)
		    axis_array[SECOND_X_AXIS].min = axis_array[FIRST_X_AXIS].min;
		if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MAX)
		    axis_array[SECOND_X_AXIS].max = axis_array[FIRST_X_AXIS].max;
	    }
	}
    }
    if (some_functions) {

	/* call the controlled variable t, since x_min can also mean
	 * smallest x */
	double t_min = 0., t_max = 0., t_step = 0.;

	if (parametric || polar) {
	    if (! (uses_axis[FIRST_X_AXIS] & USES_AXIS_FOR_DATA)) {
		/* these have not yet been set to full width */
		if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN)
		    axis_array[FIRST_X_AXIS].min = VERYLARGE;
		if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX)
		    axis_array[FIRST_X_AXIS].max = -VERYLARGE;
	    }
	    if (! (uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA)) {
		if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MIN)
		    axis_array[SECOND_X_AXIS].min = VERYLARGE;
		if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MAX)
		    axis_array[SECOND_X_AXIS].max = -VERYLARGE;
	    }
	}

	/* FIXME HBB 20000430: here and elsewhere, the code explicitly
	 * assumes that the dummy variables (t, u, v) cannot possibly
	 * be logscaled in parametric or polar mode. Does this
	 * *really* hold? */
	if (parametric || polar) {
	    t_min = axis_array[T_AXIS].min;
	    t_max = axis_array[T_AXIS].max;
	    t_step = (t_max - t_min) / (samples_1 - 1);
	}
	/* else we'll do it on each plot (see below) */

	tp_ptr = &(first_plot);
	plot_num = 0;
	this_plot = first_plot;
	c_token = begin_token;  /* start over */

	plot_iterator = check_for_iteration();

	/* Read through functions */
	while (TRUE) {
	    if (!in_parametric && !was_definition)
		start_token = c_token;

	    if (is_definition(c_token)) {
		define();
		if (equals(c_token, ","))
		    c_token++;
		was_definition = TRUE;
		continue;

	    } else {
		struct at_type *at_ptr;
		char *name_str;
		int sample_range_token;
		was_definition = FALSE;

		/* Forgive trailing comma on a multi-element plot command */
		if (END_OF_COMMAND || this_plot == NULL) {
		    int_warn(c_token, "ignoring trailing comma in plot command");
		    break;
		}

		/* HBB 20000820: now globals in 'axis.c' */
		x_axis = this_plot->x_axis;
		y_axis = this_plot->y_axis;

		plot_num++;

		/* Check for a sampling range. */
		/* Only relevant to function plots, and only needed in second pass. */
		if (!parametric)
		    init_sample_range(axis_array + x_axis);
		sample_range_token = parse_range(SAMPLE_AXIS);
		dummy_func = &plot_func;

		if (almost_equals(c_token, "newhist$ogram")) {
		    /* Make sure this isn't interpreted as a function */
		    name_str = "";
		} else if (equals(c_token, "keyentry")) {
		    name_str = "";
		} else {
		    /* Allow replacement of the dummy variable in a function */
		    if (sample_range_token > 0)
			copy_str(c_dummy_var[0], sample_range_token, MAX_ID_LEN);
		    else if (sample_range_token < 0)
			strcpy(c_dummy_var[0], set_dummy_var[0]);
		    else
			strcpy(c_dummy_var[0], orig_dummy_var);
		    /* WARNING: do NOT free name_str */
		    name_str = string_or_express(&at_ptr);
		}

		if (!name_str) {            /* function to plot */
		    if (parametric) {   /* toggle parametric axes */
			in_parametric = !in_parametric;
		    }

		    if (this_plot->plot_style == TABLESTYLE)
			int_warn(NO_CARET, "'with table' requires a data source not a pure function");

		    plot_func.at = at_ptr;

		    if (!parametric && !polar) {
			t_min = axis_array[SAMPLE_AXIS].min;
			t_max = axis_array[SAMPLE_AXIS].max;
			if (axis_array[SAMPLE_AXIS].linked_to_primary) {
			    AXIS *primary = axis_array[SAMPLE_AXIS].linked_to_primary;
			    if (primary->log && !(t_min > 0 && t_max > 0))
				int_error(NO_CARET,"logscaled axis must have positive range");
			    t_min = eval_link_function(primary, t_min);
			    t_max = eval_link_function(primary, t_max);
			    FPRINTF((stderr,"sample range on primary axis: %g %g\n", t_min, t_max));
			} else {
			    axis_unlog_interval(&X_AXIS, &t_min, &t_max, 1);
			}

			t_step = (t_max - t_min) / (samples_1 - 1);
		    }
		    for (i = 0; i < samples_1; i++) {
			double x, temp;
			struct value a;
			double t = t_min + i * t_step;

			if (parametric || polar) {
			    /* SAMPLE_AXIS is not relevant in parametric mode */
			} else if (axis_array[SAMPLE_AXIS].linked_to_primary) {
			    AXIS *vis = axis_array[SAMPLE_AXIS].linked_to_primary->linked_to_secondary;
			    t = eval_link_function(vis, t_min + i * t_step);
			} else {
			    /* Zero is often a special point in a function domain. */
			    /* Make sure we don't miss it due to round-off error.  */
			    if ((fabs(t) < 1.e-9) && (fabs(t_step) > 1.e-6))
				t = 0.0;
			}

			/* parametric/polar => NOT a log quantity */
			x = (!parametric && !polar)
			    ? AXIS_DE_LOG_VALUE(x_axis, t) : t;

			(void) Gcomplex(&plot_func.dummy_values[0], x, 0.0);
			evaluate_at(plot_func.at, &a);

			/* Imaginary values are treated as UNDEFINED */
			if (undefined || (fabs(imag(&a)) > zero)) {
			    this_plot->points[i].type = UNDEFINED;
			    continue;
			}

			/* Jan 2010 - initialize all fields! */
			memset(&this_plot->points[i], 0, sizeof(struct coordinate));

			temp = real(&a);

			/* width of box not specified */
			this_plot->points[i].z = -1.0;
			/* for the moment */
			this_plot->points[i].type = INRANGE;

			if (parametric) {
			    /* The syntax is plot x, y XnYnaxes
			     * so we do not know the actual plot axes until
			     * the y plot and cannot do range-checking now.
			     */
			    this_plot->points[i].x = t;
			    this_plot->points[i].y = temp;
			    if (boxwidth >= 0 && boxwidth_is_absolute )
				this_plot->points[i].z = 0;
			} else if (polar) {
			    double y;
			    double theta = x;

			    /* Convert from polar to cartesian coordinates and check ranges */
			    if (polar_to_xy(theta, temp, &x, &y, TRUE) == OUTRANGE)
				this_plot->points[i].type = OUTRANGE;;

			    if ((this_plot->plot_style == FILLEDCURVES) 
			    &&  (this_plot->filledcurves_options.closeto == FILLEDCURVES_ATR)) {
			    	double xhigh, yhigh;
				(void) polar_to_xy(theta, this_plot->filledcurves_options.at,
						    &xhigh, &yhigh, TRUE);
				STORE_WITH_LOG_AND_UPDATE_RANGE(
				    this_plot->points[i].xhigh, xhigh, this_plot->points[i].type, x_axis,
				    this_plot->noautoscale, NOOP, goto come_here_if_undefined);
			 	STORE_WITH_LOG_AND_UPDATE_RANGE(
				    this_plot->points[i].yhigh, yhigh, this_plot->points[i].type, y_axis,
				    this_plot->noautoscale, NOOP, goto come_here_if_undefined);
			    }

			    STORE_WITH_LOG_AND_UPDATE_RANGE(
			    	this_plot->points[i].x, x, this_plot->points[i].type, x_axis,
			    	this_plot->noautoscale, NOOP, goto come_here_if_undefined);
			    STORE_WITH_LOG_AND_UPDATE_RANGE(
			    	this_plot->points[i].y, y, this_plot->points[i].type, y_axis,
			        this_plot->noautoscale, NOOP, goto come_here_if_undefined);
			} else {        /* neither parametric or polar */
			    /* logscale ? log(x) : x */
			    this_plot->points[i].x = t;

			    /* A sampled function can only be OUTRANGE if it has a private range */
			    if (sample_range_token != 0) {
				double xx = AXIS_DE_LOG_VALUE(x_axis, t);
				if (!inrange(xx, axis_array[this_plot->x_axis].min,
						axis_array[this_plot->x_axis].max))
				    this_plot->points[i].type = OUTRANGE;
			    }

			    /* For boxes [only] check use of boxwidth */
			    if ((this_plot->plot_style == BOXES)
			    &&  (boxwidth >= 0 && boxwidth_is_absolute)) {
				double xlow, xhigh;
				coord_type dmy_type = INRANGE;
				this_plot->points[i].z = 0;
				if (axis_array[this_plot->x_axis].log) {
				    double base = axis_array[this_plot->x_axis].base;
				    xlow = x * pow(base, -boxwidth/2.);
				    xhigh = x * pow(base, boxwidth/2.);
				} else {
				    xlow = x - boxwidth/2;
				    xhigh = x + boxwidth/2;
				}
				STORE_WITH_LOG_AND_UPDATE_RANGE( this_plot->points[i].xlow,
					xlow, dmy_type, x_axis,
					this_plot->noautoscale, NOOP, NOOP );
				dmy_type = INRANGE;
				STORE_WITH_LOG_AND_UPDATE_RANGE( this_plot->points[i].xhigh, 
					xhigh, dmy_type, x_axis,
					this_plot->noautoscale, NOOP, NOOP );
			    }

			    if (this_plot->plot_style == FILLEDCURVES) {
				this_plot->points[i].xhigh = this_plot->points[i].x;
				STORE_WITH_LOG_AND_UPDATE_RANGE(
				    this_plot->points[i].yhigh,
				    this_plot->filledcurves_options.at,
				    this_plot->points[i].type, y_axis,
				    TRUE, NOOP, NOOP);
			    }
			    
			    /* Fill in additional fields needed to draw a circle */
#ifdef EAM_OBJECTS
			    if (this_plot->plot_style == CIRCLES) {
				this_plot->points[i].z = DEFAULT_RADIUS;
				this_plot->points[i].ylow = 0;
				this_plot->points[i].xhigh = 360;
			    } else if (this_plot->plot_style == ELLIPSES) {
				this_plot->points[i].z = DEFAULT_RADIUS;
				this_plot->points[i].ylow = default_ellipse.o.ellipse.orientation;
			    }
#endif

			    STORE_WITH_LOG_AND_UPDATE_RANGE(this_plot->points[i].y, temp, 
			    	this_plot->points[i].type, in_parametric ? x_axis : y_axis,
			    	this_plot->noautoscale, NOOP, goto come_here_if_undefined);

			    /* could not use a continue in this case */
			  come_here_if_undefined:
			    ;   /* ansi requires a statement after a label */
			}

		    }   /* loop over samples_1 */
		    this_plot->p_count = i;     /* samples_1 */
		}

		/* skip all modifers func / whole of data plots */
		c_token = this_plot->token;

		/* used below */
		tp_ptr = &(this_plot->next);
		this_plot = this_plot->next;
	    }

	    /* Jan 2014: Earlier 2.6 versions missed this case,   */
	    /*           breaking iteration over parametric plots */
	    if (in_parametric) {
		if (equals(c_token, ",")) {
		    c_token++;
		    continue;
		}
	    }

	    /* Iterate-over-plot mechanism */
	    if (forever_iteration(plot_iterator) && this_plot->plot_type == NODATA) {
		plot_iterator = cleanup_iteration(plot_iterator);
		c_token = start_token;
		continue;
	    }
	    if (next_iteration(plot_iterator)) {
		c_token = start_token;
		continue;
	    }

	    plot_iterator = cleanup_iteration(plot_iterator);
	    if (equals(c_token, ",")) {
		c_token++;
		plot_iterator = check_for_iteration();
	    } else
		break;
	}
	/* when step debugging, set breakpoint here to get through
	 * the 'read function' loop above quickly */
	if (parametric) {
	    /* Now actually fix the plot pairs to be single plots
	     * also fixes up polar&&parametric fn plots */
	    parametric_fixup(first_plot, &plot_num);
	    /* we omitted earlier check for range too small */
	    axis_checked_extend_empty_range(FIRST_X_AXIS, NULL);
	    if (uses_axis[SECOND_X_AXIS]) {
		axis_checked_extend_empty_range(SECOND_X_AXIS, NULL);
	    }
	}

	/* This is the earliest that polar autoscaling can be done for function plots */
	if (polar) {
	    polar_range_fiddling(first_plot);
	}

    }   /* some_functions */

    /* if first_plot is NULL, we have no functions or data at all. This can
     * happen if you type "plot x=5", since x=5 is a variable assignment.
     */

    if (plot_num == 0 || first_plot == NULL) {
	int_error(c_token, "no functions or data to plot");
    }

    if (!uses_axis[FIRST_X_AXIS] && !uses_axis[SECOND_X_AXIS])
	if (first_plot->plot_type == NODATA)
	    int_error(NO_CARET,"No data in plot");

    /* gnuplot version 5.0 always used x1 to track autoscaled range
     * regardless of whether x1 or x2 was used to plot the data. 
     * In version 5.2 we track the x1/x2 axis data limits separately.
     * However if x1 and x2 are linked to each other we must now
     * reconcile their data limits before plotting.
     */
    if (uses_axis[FIRST_X_AXIS] && uses_axis[SECOND_X_AXIS]) {
	AXIS *primary = &axis_array[FIRST_X_AXIS];
	AXIS *secondary = &axis_array[SECOND_X_AXIS];
	reconcile_linked_axes(primary, secondary);
    }
    if (uses_axis[FIRST_Y_AXIS] && uses_axis[SECOND_Y_AXIS]) {
	AXIS *primary = &axis_array[FIRST_Y_AXIS];
	AXIS *secondary = &axis_array[SECOND_Y_AXIS];
	reconcile_linked_axes(primary, secondary);
    }

    if (uses_axis[FIRST_X_AXIS]) {
	if (axis_array[FIRST_X_AXIS].max == -VERYLARGE ||
	    axis_array[FIRST_X_AXIS].min == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	axis_revert_and_unlog_range(FIRST_X_AXIS);
    } else {
	assert(uses_axis[SECOND_X_AXIS]);
    }
    if (uses_axis[SECOND_X_AXIS]) {
	if (axis_array[SECOND_X_AXIS].max == -VERYLARGE ||
	    axis_array[SECOND_X_AXIS].min == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	axis_revert_and_unlog_range(SECOND_X_AXIS);
    } else {
	assert(uses_axis[FIRST_X_AXIS]);
    }

    /* For nonlinear axes, but must also be compatible with "set link x".   */
    /* min/max values were tracked during input for the visible axes.       */
    /* Now we use them to update the corresponding shadow (nonlinear) ones. */
    update_primary_axis_range(&axis_array[FIRST_X_AXIS]);
    update_primary_axis_range(&axis_array[SECOND_X_AXIS]);

    if (this_plot && this_plot->plot_style == TABLESTYLE) {
	/* the y axis range has no meaning in this case */
	;
    } else if (uses_axis[FIRST_Y_AXIS] && nonlinear(&axis_array[FIRST_Y_AXIS])) {
	axis_checked_extend_empty_range(FIRST_Y_AXIS, "all points y value undefined!");
	update_primary_axis_range(&axis_array[FIRST_Y_AXIS]);
    } else if (uses_axis[FIRST_Y_AXIS]) {
	axis_checked_extend_empty_range(FIRST_Y_AXIS, "all points y value undefined!");
	axis_revert_and_unlog_range(FIRST_Y_AXIS);
    }
    if (uses_axis[SECOND_Y_AXIS] && axis_array[SECOND_Y_AXIS].linked_to_primary) {
	axis_checked_extend_empty_range(SECOND_Y_AXIS, "all points y2 value undefined!");
	update_primary_axis_range(&axis_array[SECOND_Y_AXIS]);
    } else if (uses_axis[SECOND_Y_AXIS]) {
	axis_checked_extend_empty_range(SECOND_Y_AXIS, "all points y2 value undefined!");
	axis_revert_and_unlog_range(SECOND_Y_AXIS);
    } else {
	/* FIXME:  probably unneeded our wrong since adding reconcile_linked_axes */
	/* else we want to copy y2 range */
	assert(uses_axis[FIRST_Y_AXIS]);
	if (axis_array[SECOND_Y_AXIS].autoscale & AUTOSCALE_MIN)
	    axis_array[SECOND_Y_AXIS].min = axis_array[FIRST_Y_AXIS].min;
	if (axis_array[SECOND_Y_AXIS].autoscale & AUTOSCALE_MAX)
	    axis_array[SECOND_Y_AXIS].max = axis_array[FIRST_Y_AXIS].max;
	/* Log() fixup is only necessary if the range was *not* copied from
	 * the (already logarithmized) yrange */
	if (! axis_array[SECOND_Y_AXIS].autoscale)
	    axis_revert_and_unlog_range(SECOND_Y_AXIS);
    }

    /* This call was in boundary(), called from do_plot(), but it caused
     * logscaling problems if do_plot() itself was called for refresh
     * rather than for plot/replot.  So we moved it here (June 2014)
     * and added palette checks (March 2017).
     */
    set_plot_with_palette(0, MODE_PLOT);
    if (is_plot_with_palette())
	set_cbminmax();

    /* the following ~5 lines were moved from the end of the
     * function to here, as do_plot calles term->text, which
     * itself might process input events in mouse enhanced
     * terminals. For redrawing to work, line capturing and
     * setting the plot_num must already be done before
     * entering do_plot(). Thu Jan 27 23:56:24 2000 (joze) */
    /* if we get here, all went well, so record this line for replot */
    if (plot_token != -1) {
	/* note that m_capture also frees the old replot_line */
	m_capture(&replot_line, plot_token, c_token - 1);
	plot_token = -1;
	fill_gpval_string("GPVAL_LAST_PLOT", replot_line);
    }

    if (table_mode) {
	print_table(first_plot, plot_num);

    } else {
	do_plot(first_plot, plot_num);

	/* after do_plot(), axis_array[].min and .max
	 * contain the plotting range actually used (rounded
	 * to tic marks, not only the min/max data values)
	 *  --> save them now for writeback if requested
	 */
	save_writeback_all_axes();

	/* Mark these plots as safe for quick refresh */
	SET_REFRESH_OK(E_REFRESH_OK_2D, plot_num);
    }

    /* update GPVAL_ variables available to user */
    update_gpval_variables(1);

}                               /* eval_plots */


/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are garanteed to occur in (x,y) pairs while preserving the
 * non-FUNC type plots intact.  This means we have to work our way through
 * various lists.  Examples (hand checked): start_plot:F1->F2->NULL ==>
 * F2->NULL start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F2->F4->F6->NULL
 * start_plot:F1->F2->D1->D2->F3->F4->D3->NULL ==> F2->D1->D2->F4->D3->NULL
 *
 */
static void
parametric_fixup(struct curve_points *start_plot, int *plot_num)
{
    struct curve_points *xp, *new_list = NULL, *free_list = NULL;
    struct curve_points **last_pointer = &new_list;
    int i, curve;

    /*
     * Ok, go through all the plots and move FUNC types together.  Note: this
     * originally was written to look for a NULL next pointer, but gnuplot
     * wants to be sticky in grabbing memory and the right number of items in
     * the plot list is controlled by the plot_num variable.
     *
     * Since gnuplot wants to do this sticky business, a free_list of
     * curve_points is kept and then tagged onto the end of the plot list as
     * this seems more in the spirit of the original memory behavior than
     * simply freeing the memory.  I'm personally not convinced this sort of
     * concern is worth it since the time spent computing points seems to
     * dominate any garbage collecting that might be saved here...
     */
    new_list = xp = start_plot;
    curve = 0;

    while (++curve <= *plot_num) {
	if (xp->plot_type == FUNC) {
	    /* Here's a FUNC parametric function defined as two parts. */
	    struct curve_points *yp = xp->next;

	    --(*plot_num);

	    assert(xp->p_count == yp->p_count);

	    /* IMPORTANT: because syntax is   plot x(t), y(t) XnYnaxes ..., only
	     * the y function axes are correct
	     */

	    /*
	     * Go through all the points assigning the y's from xp to be
	     * the x's for yp. In polar mode, we need to check max's and
	     * min's as we go.
	     */

	    for (i = 0; i < yp->p_count; ++i) {
		if (polar) {
		    double r = yp->points[i].y;
		    double t = xp->points[i].y;
		    double x, y;

		    /* Convert from polar to cartesian coordinate and check ranges */
		    /* FIXME: The old in-line conversion checked R_AXIS.max agains fabs(r).
		     * That's not what polar_to_xy() is currently doing.
		     */
		    if (polar_to_xy(t, r, &x, &y, TRUE) == OUTRANGE)
			yp->points[i].type = OUTRANGE;

		    if (boxwidth >= 0 && boxwidth_is_absolute) {
			coord_type dmy_type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE( yp->points[i].xlow, x - boxwidth/2, dmy_type, xp->x_axis,
							 xp->noautoscale, NOOP, NOOP );
			dmy_type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE( yp->points[i].xhigh, x + boxwidth/2, dmy_type, xp->x_axis,
							 xp->noautoscale, NOOP, NOOP );
		    }
		    /* we hadn't done logs when we stored earlier */
		    STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].x, x, yp->points[i].type, xp->x_axis,
		    				 xp->noautoscale, NOOP, NOOP);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].y, y, yp->points[i].type, xp->y_axis,
		    				 xp->noautoscale, NOOP, NOOP);
		} else {
		    double x = xp->points[i].y;
		    double y = yp->points[i].y;

		    if (boxwidth >= 0 && boxwidth_is_absolute) {
			coord_type dmy_type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE( yp->points[i].xlow, x - boxwidth/2, dmy_type, yp->x_axis,
							 xp->noautoscale, NOOP, NOOP );
			dmy_type = INRANGE;
			STORE_WITH_LOG_AND_UPDATE_RANGE( yp->points[i].xhigh, x + boxwidth/2, dmy_type, yp->x_axis,
							 xp->noautoscale, NOOP, NOOP );
		    }
		    STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].x, x, yp->points[i].type, yp->x_axis,
		    				 xp->noautoscale, NOOP, NOOP);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].y, y, yp->points[i].type, yp->y_axis,
		    				 xp->noautoscale, NOOP, NOOP);
		}
	    }

	    /* move xp to head of free list */
	    xp->next = free_list;
	    free_list = xp;

	    /* append yp to new_list */
	    *last_pointer = yp;
	    last_pointer = &(yp->next);
	    xp = yp->next;

	} else {                /* data plot */
	    assert(*last_pointer == xp);
	    last_pointer = &(xp->next);
	    xp = xp->next;
	}
    }                           /* loop over plots */

    first_plot = new_list;

    /* Ok, stick the free list at the end of the curve_points plot list. */
    *last_pointer = free_list;
}

/*
 * Shared by plot and splot
 */
void
parse_plot_title(struct curve_points *this_plot, char *xtitle, char *ytitle, TBOOLEAN *set_title)
{
    legend_key *key = &keyT;

    if (almost_equals(c_token, "t$itle") || almost_equals(c_token, "not$itle")) {
	if (*set_title)
	    int_error(c_token, "duplicate title");
	*set_title = TRUE;

	/* title can be enhanced if not explicitly disabled */
	this_plot->title_no_enhanced = !key->enhanced;

	if (almost_equals(c_token++, "not$itle"))
	    this_plot->title_is_suppressed = TRUE;

	if (parametric || this_plot->title_is_suppressed) {
	    if (in_parametric)
		int_error(c_token, "title allowed only after parametric function fully specified");
	    if (xtitle != NULL)
		xtitle[0] = '\0';       /* Remove default title . */
	    if (ytitle != NULL)
		ytitle[0] = '\0';       /* Remove default title . */
	    if (equals(c_token,","))
		return;
	}

	/* This ugliness is because columnheader can be either a keyword */
	/* or a function name.  Yes, the design could have been better. */
	if (almost_equals(c_token,"col$umnheader")
	&& !(equals(c_token,"columnhead") && equals(c_token+1,"(")) ) {
	    df_set_key_title_columnhead(this_plot);
	} else if (equals(c_token,"at")) {
	    *set_title = FALSE;
	} else {
	    char *temp;
	    evaluate_inside_using = TRUE;
	    temp = try_to_get_string();
	    evaluate_inside_using = FALSE;
	    if (!this_plot->title_is_suppressed) {
		if (!temp)
		    int_error(c_token, "expecting \"title\" for plot");
		if (this_plot->plot_style == HISTOGRAMS
		&&  histogram_opts.type == HT_STACKED_IN_TOWERS)
		    df_key_title = temp;
		else
		    this_plot->title = temp;
	    }
	}
	if (equals(c_token,"at")) {
	    int save_token = ++c_token;
	    this_plot->title_position = gp_alloc(sizeof(t_position), NULL);
	    if (equals(c_token,"end")) {
		this_plot->title_position->scalex = character;
		this_plot->title_position->x = 1;
		this_plot->title_position->y = LEFT;
		c_token++;
	    } else if (almost_equals(c_token,"beg$inning")) {
		this_plot->title_position->scalex = character;
		this_plot->title_position->x = -1;
		this_plot->title_position->y = RIGHT;
		c_token++;
	    } else {
		get_position_default(this_plot->title_position, screen, 2);
	    }
	    if (save_token == c_token)
		int_error(c_token, "expecting \"at {beginning|end|<xpos>,<ypos>}\"");
	    if (equals(c_token,"right")) {
		if (this_plot->title_position->scalex == character)
		    this_plot->title_position->y = RIGHT;
		c_token++;
	    }
	    if (equals(c_token,"left")) {
		if (this_plot->title_position->scalex == character)
		    this_plot->title_position->y = LEFT;
		c_token++;
	    }
	}
    }

    if (almost_equals(c_token, "enh$anced")) {
	c_token++;
	this_plot->title_no_enhanced = FALSE;
    } else if (almost_equals(c_token, "noenh$anced")) {
	c_token++;
	this_plot->title_no_enhanced = TRUE;
    }

}

