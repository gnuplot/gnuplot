#ifndef lint
static char *RCSid() { return RCSid("$Id: plot2d.c,v 1.193.2.9 2010/05/11 04:25:59 sfeam Exp $"); }
#endif

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
#include "binary.h"
#include "command.h"
#include "datafile.h"
#include "eval.h"
#include "fit.h"
#include "graphics.h"
#include "interpol.h"
#include "misc.h"
#include "parse.h"
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
static void histogram_range_fiddling __PROTO((struct curve_points *plot));

/* internal and external variables */

/* the curves/surfaces of the plot */
struct curve_points *first_plot = NULL;
static struct udft_entry plot_func;

/* box width (automatic) */
double   boxwidth              = -1.0;
/* whether box width is absolute (default) or relative */
TBOOLEAN boxwidth_is_absolute  = TRUE;

static double histogram_rightmost = 0.0;        /* Highest x-coord of histogram so far */
static char *histogram_title = NULL;            /* Subtitle for this histogram */
static struct coordinate GPHUGE *stackheight = NULL; /* Scratch space for y autoscale */
static int stack_count = 0;                     /* counter for stackheight */

/* function implementations */

/* HBB 20000508: moved cp_alloc() &friends to the main module using them, and
 * made cp_alloc 'static'.
 */
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

    return (cp);
}


/*
 * cp_extend() reallocates a curve_points structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void
cp_extend(struct curve_points *cp, int num)
{

#if defined(DOS16) || defined(WIN16)
    /* Make sure we do not allocate more than 64k points in msdos since
     * indexing is done with 16-bit int
     * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
	int_error(NO_CARET, "Array index must be less than 32k in msdos");
#endif /* MSDOS */

    if (num == cp->p_max)
	return;

    if (num > 0) {
	if (cp->points == NULL) {
	    cp->points = gp_alloc(num * sizeof(cp->points[0]),
				  "curve points");
	} else {
	    cp->points = gp_realloc(cp->points, num * sizeof(cp->points[0]),
				    "expanding curve points");
	}
	cp->p_max = num;
    } else {
	if (cp->points != NULL)
	    free(cp->points);
	cp->points = NULL;
	cp->p_max = 0;
    }
}

/*
 * cp_free() releases any memory which was previously malloc()'d to hold
 *   curve points (and recursively down the linked list).
 */
/* HBB 20000506: instead of risking stack havoc by recursion, operate
 * iteratively */
void
cp_free(struct curve_points *cp)
{
    while (cp) {
	struct curve_points *next = cp->next;

	if (cp->title)
	    free(cp->title);
	if (cp->points)
	    free(cp->points);
	if (cp->labels) {
	    free_labels(cp->labels);
	    cp->labels = (struct text_label *)NULL;
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
    int dummy_token = -1;
    int t_axis;

    if (!term)                  /* unknown */
	int_error(c_token, "use 'set term' to set terminal type first");

    is_3d_plot = FALSE;

    /* Deactivate if 'set view map' is still running after the previous 'splot': */
    splot_map_deactivate();

    if (parametric && strcmp(set_dummy_var[0], "u") == 0)
	strcpy(set_dummy_var[0], "t");

    /* initialise the arrays from the 'set' scalars */

    AXIS_INIT2D(FIRST_X_AXIS, 0);
    AXIS_INIT2D(FIRST_Y_AXIS, 1);
    AXIS_INIT2D(SECOND_X_AXIS, 0);
    AXIS_INIT2D(SECOND_Y_AXIS, 1);
    AXIS_INIT2D(T_AXIS, 0);
    AXIS_INIT2D(R_AXIS, 1);
    AXIS_INIT2D(COLOR_AXIS, 1);

    t_axis = (parametric || polar) ? T_AXIS : FIRST_X_AXIS;

    PARSE_NAMED_RANGE(t_axis, dummy_token);
    if (parametric || polar)    /* set optional x ranges */
	PARSE_RANGE(FIRST_X_AXIS);

    /* possible reversal of x range *does* matter, even in parametric
     * or polar mode */
    CHECK_REVERSE(FIRST_X_AXIS);

    PARSE_RANGE(FIRST_Y_AXIS);
    CHECK_REVERSE(FIRST_Y_AXIS);
    PARSE_RANGE(SECOND_X_AXIS);
    CHECK_REVERSE(SECOND_X_AXIS);
    PARSE_RANGE(SECOND_Y_AXIS);
    CHECK_REVERSE(SECOND_Y_AXIS);

    /* Clear out any tick labels read from data files in previous plot */
    for (t_axis=0; t_axis<AXIS_ARRAY_SIZE; t_axis++) {
	struct ticdef *ticdef = &axis_array[t_axis].ticdef;
	if (ticdef->def.user)
	    ticdef->def.user = prune_dataticks(ticdef->def.user);
	if (!ticdef->def.user && ticdef->type == TIC_USER)
	    ticdef->type = TIC_COMPUTED;
    }

    /* use the default dummy variable unless changed */
    if (dummy_token >= 0)
	copy_str(c_dummy_var[0], dummy_token, MAX_ID_LEN);
    else
	(void) strcpy(c_dummy_var[0], set_dummy_var[0]);

    eval_plots();
}


#ifdef VOLATILE_REFRESH
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

	/* IMAGE clipping is done elsewhere, so we don't need INRANGE/OUTRANGE
	 * checks.  
	 */
	if (this_plot->plot_style == IMAGE || this_plot->plot_style == RGBIMAGE) {
	    if (x_axis->set_autoscale)
		plot_image_or_update_axes(this_plot,TRUE);
	    continue;
	}

	for (i=0; i<this_plot->p_count; i++) {
	    struct coordinate GPHUGE *point = &this_plot->points[i];

	    if (point->type == UNDEFINED)
		continue;
	    else
		point->type = INRANGE;

	    /* If the state has been set to autoscale since the last plot,
	     * mark everything INRANGE and re-evaluate the axis limits now.
	     * Otherwise test INRANGE/OUTRANGE against previous axis limits.
	     */
	    if (!this_plot->noautoscale
	    &&  (x_axis->set_autoscale & (AUTOSCALE_MIN|AUTOSCALE_MAX))) {
		if (point->x > x_axis->max) x_axis->max = point->x;
		if (point->x < x_axis->min) x_axis->min = point->x;
	    } else if (!inrange(point->x, x_axis->min, x_axis->max)) {
		point->type = OUTRANGE;
		continue;
	    }

	    if (!this_plot->noautoscale
	    &&  (y_axis->set_autoscale & (AUTOSCALE_MIN|AUTOSCALE_MAX))) {
		if (point->y > y_axis->max) y_axis->max = point->y;
		if (point->y < y_axis->min) y_axis->min = point->y;
	    } else if (!inrange(point->y, y_axis->min, y_axis->max)) {
		point->type = OUTRANGE;
		continue;
	    }
	}

    }
}
#endif


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
    int max_cols, min_cols;    /* allowed range of column numbers */
    double v[MAXDATACOLS];
    int storetoken = current_plot->token;
    struct coordinate GPHUGE *cp;

    TBOOLEAN variable_color = FALSE;
    double   variable_color_value;
    if ((current_plot->lp_properties.pm3d_color.type == TC_RGB)
    &&  (current_plot->lp_properties.pm3d_color.value < 0))
	variable_color = TRUE;
    if (current_plot->lp_properties.pm3d_color.type == TC_Z)
	variable_color = TRUE;
    if (current_plot->lp_properties.l_type == LT_COLORFROMCOLUMN)
	variable_color = TRUE;

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
	max_cols = 7;		/* HBB FIXME 20060427: what's 7th? */

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
	min_cols = max_cols = 5;
	/* HBB 20060427: signal 3rd and 4th column are absolute y data
	 * --- needed so time/date parsing works */
	df_axis[2] = df_axis[3] = df_axis[4] = df_axis[1];
	break;

    case CANDLESTICKS:
	current_plot->z_axis = current_plot->y_axis;
	min_cols = 5;
	max_cols = 6;
	df_axis[2] = df_axis[3] = df_axis[4] = df_axis[1];
	break;

    case BOXERROR:
	min_cols = 3;           /* HBB 20040520: fixed, was 4 */
	max_cols = 5;

	/* There are four(!) possible cases: */
	/* 3 cols --> (x,y,dy), auto dx */
	/* 4 cols, boxwidth==-2 --> (x,y,ylow,yhigh), auto dx */
	/* 4 cols, boxwidth!=-2 --> (x,y,dy,dx) */
	/* 5 cols --> (x,y,ylow,yhigh,dx) */
	if ((df_no_use_specs == 4 && boxwidth == -2)
	    || df_no_use_specs == 5)
	    /* HBB 20060427: signal 3rd and 4th column are absolute y
	     * data --- needed so time/date parsing works */
	    df_axis[2] = df_axis[3] = df_axis[1];
	break;

    case VECTOR:	/* x, y, dx, dy, variable_color */
	min_cols = 4;
	max_cols = 5;
	break;

    case XERRORLINES:
    case XERRORBARS:
	min_cols = 3;
	max_cols = 4;
	if (df_no_use_specs == 4)
	    /* HBB 20060427: signal 3rd and 4th column are absolute x
	     * data --- needed so time/date parsing works */
	    df_axis[2] = df_axis[3] = df_axis[0];
	break;

    case YERRORLINES:
    case YERRORBARS:
	min_cols = 3;
	max_cols = 4;
	if (df_no_use_specs == 4)
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
	/* 4th column allows rgb variable */
	min_cols = 3;
	max_cols = 4;
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
    case CIRCLES:	/* 3 + possible variable color */
	min_cols = 3;
	max_cols = 4;
	break;
#endif

    case POINTSTYLE:
    case LINESPOINTS:
	/* Allow 3rd column because of 'pointsize variable' */
	/* Allow 4th column because of 'lc rgb variable' */
	min_cols = 1;
	max_cols = 4;
	break;

    default:
	min_cols = 1;
	max_cols = 2;
	break;
    }

    if (current_plot->plot_smooth == SMOOTH_ACSPLINES) {
	max_cols = 3;
	current_plot->z_axis = FIRST_Z_AXIS;
	df_axis[2] = FIRST_Z_AXIS;
    }

    if (df_no_use_specs > max_cols)
	int_error(NO_CARET, "Too many using specs for this style");

    if (df_no_use_specs > 0 && df_no_use_specs < min_cols)
	int_error(NO_CARET, "Not enough columns for this style");

    i = 0;

    /* If the user has set an explicit locale for numeric input, apply it */
    /* here so that it affects data fields read from the input file.      */
    set_numeric_locale();

    while ((j = df_readline(v, max_cols)) != DF_EOF) {
	/* j <= max_cols */

	if (i >= current_plot->p_max) {
	    /* overflow about to occur. Extend size of points[]
	     * array. Double the size, and add 1000 points, to avoid
	     * needlessly small steps. */
	    cp_extend(current_plot, i + i + 1000);
	}

        if (j > 0) {
            /* Allow for optional columns.  Currently only used for a few styles, */
            /* but could be extended to a more general mechanism.                 */
            variable_color_value = 0;
            if (variable_color) {
                static char *errmsg = "Not enough columns for variable color";
                switch (current_plot->plot_style) {
                case VECTOR:	if (j < 5) int_error(NO_CARET,errmsg);
                case CIRCLES: 	if (j < 4) int_error(NO_CARET,errmsg);
                case BOXES:	if (j < 3) int_error(NO_CARET,errmsg);
                    variable_color_value = v[--j];
                default:	break;
                }
            }
        }
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
	    /* NaN or bad result from extended using expression */
	    current_plot->points[i].type = UNDEFINED;
	    i++;
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

	    /* break in data, make next point undefined */
	    /* FIXME: We really should distinguish between a blank	*/
	    /*        line and an undefined value on a non-blank line.	*/
	    current_plot->points[i].type = UNDEFINED;
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

	case 0:         /* not blank line, but df_readline couldn't parse it */
	    {
		df_close();
		int_error(current_plot->token,
			  "Bad data on line %d", df_line_number);
	    }

	case 1:
	    {                   /* only one number */
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
				      v[1], variable_color_value, 0.0);
		    } else
			store2d_point(current_plot, i++, v[0], v[1],
				      v[0] - boxwidth / 2, v[0] + boxwidth / 2,
				      v[1], variable_color_value, 0.0);

	    } else {
		    if (current_plot->plot_style == CANDLESTICKS
			|| current_plot->plot_style == FINANCEBARS) {
			int_warn(storetoken, "This plot style does not work with 1 or 2 cols. Setting to points");
			current_plot->plot_style = POINTSTYLE;
		    }
		    /* xlow and xhigh are same as x */
		    /* auto width if boxes, else ignored */
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0], v[1],
				  variable_color_value, -1.0);
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
				  v[1], variable_color_value, 0.0);
		    break;

		case LABELPOINTS:
		    /* Load the coords just as we would have for a point plot */
		    store2d_point(current_plot, i, v[0], v[1], v[0], v[0], v[1],
				  v[1], -1.0);
		    /* Allocate and fill in a text_label structure to match it */
		    store_label(current_plot->labels,
				&(current_plot->points[i]), i, df_tokens[2], 0.0);
		    i++;
		    break;

		case IMAGE:  /* x_center y_center color_value */
		    store2d_point(current_plot, i, v[0], v[1], v[0], v[0], v[1],
				  v[1], v[2]);
		    cp = &(current_plot->points[i]);
		    COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, v[2], cp->type,
			COLOR_AXIS, current_plot->noautoscale, NOOP, cp->CRD_COLOR=v[2]);
		    i++;
		    break;

		case POINTSTYLE: /* x, y, variable point size or variable color */
		case LINESPOINTS:
		case IMPULSES:
		case LINES:
		case DOTS:
		    store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[1], v[2], v[2]);
		    break;

#ifdef EAM_OBJECTS
		case CIRCLES:	/* x, y, radius */
		    store2d_point(current_plot, i++, v[0], v[1], v[0]-v[2], v[0]+v[2],
		    		  v[1], variable_color_value, v[2]);
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
			      v[1], variable_color_value, 0.0);
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

	    case VECTOR:
		/* x,y,dx,dy */
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0] + v[2],
			      v[1], v[1] + v[3], variable_color_value);
		break;

	    case POINTSTYLE: /* x, y, variable point size and variable color */
	    case LINESPOINTS:
		store2d_point(current_plot, i++, v[0], v[1], v[0], v[0],
				  v[1], v[3], v[2]);
		break;

	    case LABELPOINTS:
		/* Load the coords just as we would have for a point plot */
		store2d_point(current_plot, i, v[0], v[1], v[0], v[0], v[1],
			      v[1], -1.0);
		/* Allocate and fill in a text_label structure to match it */
		store_label(current_plot->labels,
			    &(current_plot->points[i]), i, df_tokens[2], v[3]);
		i++;
		break;

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

		case RGBIMAGE:  /* x_center y_center r_value g_value b_value (rgb) */
		    goto images;

		}
		break;
	    }

	case 7:
	    /* same as six columns. Width ignored */
	    /* eh ? - fall through */
	case 6:
	    /* x, y, xlow, xhigh, ylow, yhigh */
	    switch (current_plot->plot_style) {
	    default:
		int_warn(storetoken, "This plot style not work with 6 cols. Setting to xyerrorbars");
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

    }                           /*while */

    current_plot->p_count = i;
    cp_extend(current_plot, i); /* shrink to fit */

    df_close();

    /* We are finished reading user input; return to C locale for internal use */
    reset_numeric_locale();

    return i;                   /* i==0 indicates an 'empty' file */
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

    /* jev -- pass data values thru user-defined function */
    /* div -- y is dummy variable 2 - copy value there */
    if (ydata_func.at) {
	struct value val;

	(void) Gcomplex(&ydata_func.dummy_values[0], y, 0.0);
	ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
	evaluate_at(ydata_func.at, &val);
	y = undefined ? 0.0 : real(&val);

	/* EAM FIXME - filtering ylow and yhigh is nonsense for many plot styles */

	(void) Gcomplex(&ydata_func.dummy_values[0], ylow, 0.0);
	ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
	evaluate_at(ydata_func.at, &val);
	ylow = undefined ? 0 : real(&val);

	(void) Gcomplex(&ydata_func.dummy_values[0], yhigh, 0.0);
	ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
	evaluate_at(ydata_func.at, &val);
	yhigh = undefined ? 0 : real(&val);
    }
    dummy_type = cp->type = INRANGE;

    if (polar) {
	double newx, newy;
	if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MAX) && y > axis_array[R_AXIS].max) {
	    cp->type = OUTRANGE;
	}
	if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MIN)) {
	    /* we store internally as if plotting r(t)-rmin */
	    y -= axis_array[R_AXIS].min;
	}
	newx = y * cos(x * ang2rad);
	newy = y * sin(x * ang2rad);
	y = newy;
	x = newx;

	if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MAX) && yhigh > axis_array[R_AXIS].max) {
	    cp->type = OUTRANGE;
	}
	if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MIN)) {
	    /* we store internally as if plotting r(t)-rmin */
	    yhigh -= axis_array[R_AXIS].min;
	}
	newx = yhigh * cos(xhigh * ang2rad);
	newy = yhigh * sin(xhigh * ang2rad);
	yhigh = newy;
	xhigh = newx;

	if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MAX) && ylow > axis_array[R_AXIS].max) {
	    cp->type = OUTRANGE;
	}
	if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MIN)) {
	    /* we store internally as if plotting r(t)-rmin */
	    ylow -= axis_array[R_AXIS].min;
	}
	newx = ylow * cos(xlow * ang2rad);
	newy = ylow * sin(xlow * ang2rad);
	ylow = newy;
	xlow = newx;
    }
    /* return immediately if x or y are undefined
     * we dont care if outrange for high/low.
     * BUT if high/low undefined (ie log( < 0 ), no number is stored,
     * but graphics.c doesn't know.
     * explicitly store -VERYLARGE;
     */
    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->x, x, cp->type, current_plot->x_axis,
    			current_plot->noautoscale, NOOP, return);
    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->y, y, cp->type, current_plot->y_axis,
    			current_plot->noautoscale, NOOP, return);

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
    case CIRCLES:
	cp->ylow = y;
	cp->yhigh = yhigh;	/* really variable_color_data */
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, current_plot->x_axis, 
					current_plot->noautoscale, NOOP, cp->xlow = -VERYLARGE);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, current_plot->x_axis,
					current_plot->noautoscale, NOOP, cp->xhigh = -VERYLARGE);
	break;
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
    if ((int)current_plot->z_axis != NO_AXIS)
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->z, width, dummy_type, current_plot->z_axis, 
					current_plot->noautoscale, NOOP, cp->z = -VERYLARGE);
    else
	cp->z = width;

    /* If we have variable color corresponding to a z-axis value, use it to autoscale */
    if (current_plot->lp_properties.pm3d_color.type == TC_Z)
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->z, cp->z, dummy_type, COLOR_AXIS, 
					current_plot->noautoscale, NOOP, NOOP);

}                               /* store2d_point */


/* Autoscaling of box plots cuts off half of the box on each end. */
/* Add a half-boxwidth to the range in this case.  EAM Aug 2007   */
static void
box_range_fiddling(struct curve_points *plot)
{
    double xlow, xhigh;

    if (axis_array[plot->x_axis].autoscale & AUTOSCALE_MIN) {
	if (plot->points[0].type != UNDEFINED && plot->points[1].type != UNDEFINED) {
	    xlow = plot->points[0].x - (plot->points[1].x - plot->points[0].x) / 2.;
	    if (axis_array[plot->x_axis].min > xlow)
		axis_array[plot->x_axis].min = xlow;
	}
    }
    if (axis_array[plot->x_axis].autoscale & AUTOSCALE_MAX) {
	int i = plot->p_count -1;
	if (plot->points[i].type != UNDEFINED && plot->points[i-1].type != UNDEFINED) {
	    xhigh = plot->points[i].x + (plot->points[i].x - plot->points[i-1].x) / 2.;
	    if (axis_array[plot->x_axis].max < xhigh)
		axis_array[plot->x_axis].max = xhigh;
	}
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

/* store_label() is called by get_data for each point */
/* This routine is exported so it can be shared by plot3d */
void
store_label(
    struct text_label *listhead,
    struct coordinate *cp,
    int i,                      /* point number */
    char *string,               /* start of label string */
    double colorval)            /* used if text color derived from palette */
{
    struct text_label *tl = listhead;
    int textlen;

    /* Walk through list to get to the end. Yes I know this is inefficient */
    /* but is anyone really going to plot so many labels that it matters?  */
    if (!tl) int_error(NO_CARET,"text_label list was not initialized");
    while (tl->next) tl = tl->next;

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
    else if (tl->textcolor.type == TC_RGB && tl->textcolor.value < 0)
	tl->textcolor.lt = colorval;

    /* Check for null string (no label) */
    if (!string)
	string = "";

    textlen = 0;
    /* FIXME EAM - this code is ugly but seems to work */
    /* We need to handle quoted separators and quoted quotes */
    if (df_separator) {
	TBOOLEAN in_quote = FALSE;
	while (string[textlen]) {
	    if (string[textlen] == '"')
		in_quote = !in_quote;
	    else if (string[textlen] == df_separator && !in_quote)
		break;
	    textlen++;
	}
	while (textlen > 0 && isspace(string[textlen-1]))
	    textlen--;
    } else {
    /* This is the normal case (no special separator character) */
	if (*string == '"') {
	    for (textlen=1; string[textlen] && string[textlen] != '"'; textlen++);
	}
	while (string[textlen] && !isspace(string[textlen]))
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
}


/*
 * print_points: a debugging routine to print out the points of a curve, and
 * the curve structure. If curve<0, then we print the list of curves.
 */

#if 0                           /* not used */
static char *plot_type_names[] =
{
    "Function", "Data", "3D Function", "3d data"
};
static char *plot_style_names[] =
{
    "Lines", "Points", "Impulses", "LinesPoints", "Dots", "XErrorbars",
    "YErrorbars", "XYErrorbars", "BoxXYError", "Boxes", "Boxerror", "Steps",
    "FSteps", "Vector",
    "XErrorlines", "YErrorlines", "XYErrorlines"
};
static char *plot_smooth_names[] =
{
    "None", "Unique", "Frequency", "CSplines", "ACSplines", "Bezier", "SBezier"
};

static void
print_points(int curve)
{
    struct curve_points *this_plot;
    int i;

    if (curve < 0) {
	for (this_plot = first_plot, i = 0;
	     this_plot != NULL;
	     i++, this_plot = this_plot->next) {
	    printf("Curve %d:\n", i);
	    if ((int) this_plot->plot_type >= 0 && (int) (this_plot->plot_type) < 4)
		printf("Plot type %d: %s\n", (int) (this_plot->plot_type),
			plot_type_names[(int) (this_plot->plot_type)]);
	    else
		printf("Plot type %d: BAD\n", (int) (this_plot->plot_type));
	    if ((int) this_plot->plot_style >= 0 && (int) (this_plot->plot_style) < 14)
		printf("Plot style %d: %s\n", (int) (this_plot->plot_style),
			plot_style_names[(int) (this_plot->plot_style)]);
	    else
		printf("Plot style %d: BAD\n", (int) (this_plot->plot_style));
	    if ((int) this_plot->plot_smooth >= 0 && (int) (this_plot->plot_smooth) < 6)
		printf("Plot smooth style %d: %s\n", (int) (this_plot->plot_style),
			plot_smooth_names[(int) (this_plot->plot_smooth)]);
	    else
		printf("Plot smooth style %d: BAD\n", (int) (this_plot->plot_smooth));
	    printf("\
Plot title: '%s'\n\
Line type %d\n\
Point type %d\n\
max points %d\n\
current points %d\n\n",
		   this_plot->title,
		   this_plot->line_type,
		   this_plot->point_type,
		   this_plot->p_max,
		   this_plot->p_count);
	}
    } else {
	for (this_plot = first_plot, i = 0;
	     i < curve && this_plot != NULL;
	     i++, this_plot = this_plot->next);
	if (this_plot == NULL)
	    printf("Curve %d does not exist; list has %d curves\n", curve, i);
	else {
	    printf("Curve %d, %d points\n", curve, this_plot->p_count);
	    for (i = 0; i < this_plot->p_count; i++) {
		printf("%c x=%g y=%g z=%g xlow=%g xhigh=%g ylow=%g yhigh=%g\n",
			this_plot->points[i].type == INRANGE ? 'i'
			: this_plot->points[i].type == OUTRANGE ? 'o'
			: 'u',
			this_plot->points[i].x,
			this_plot->points[i].y,
			this_plot->points[i].z,
			this_plot->points[i].xlow,
			this_plot->points[i].xhigh,
			this_plot->points[i].ylow,
			this_plot->points[i].yhigh);
	    }
	    printf("\n");
	}
    }
}
#endif /* not used */

/* HBB 20010610: mnemonic names for the bits stored in 'uses_axis' */
typedef enum e_uses_axis {
    USES_AXIS_FOR_DATA = 1,
    USES_AXIS_FOR_FUNC = 2
} t_uses_axis;

/*
 * This parses the plot command after any range specifications. To support
 * autoscaling on the x axis, we want any data files to define the x range,
 * then to plot any functions using that range. We thus parse the input
 * twice, once to pick up the data files, and again to pick up the functions.
 * Definitions are processed twice, but that won't hurt.
 * div - okay, it doesn't hurt, but every time an option as added for
 * datafiles, code to parse it has to be added here. Change so that
 * we store starting-token in the plot structure.
 */
static void
eval_plots()
{
    int i;
    struct curve_points *this_plot, **tp_ptr;
    t_uses_axis uses_axis[AXIS_ARRAY_SIZE];
    int some_functions = 0;
    int plot_num, line_num, point_num;
    TBOOLEAN in_parametric = FALSE;
    TBOOLEAN was_definition = FALSE;
    int pattern_num;
    char *xtitle = NULL;
    int begin_token = c_token;  /* so we can rewind for second pass */
    int start_token=0, end_token;
    legend_key *key = &keyT;

    double newhist_start = 0.0;
    int histogram_sequence = -1;
    int newhist_color = 0;
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
    point_num = 0;              /* default point type */
    pattern_num = default_fillstyle.fillpattern;        /* default fill pattern */

    xtitle = NULL;

    /* Assume that the input data can be re-read later */
    volatile_data = FALSE;

    /* ** First Pass: Read through data files ***
     * This pass serves to set the xrange and to parse the command, as well
     * as filling in every thing except the function data. That is done after
     * the xrange is defined.
     */
    check_for_iteration();
    while (TRUE) {
	if (END_OF_COMMAND)
	    int_error(c_token, "function to plot expected");

	this_plot = NULL;
	if (!in_parametric && !was_definition)
	    start_token = c_token;

	if (almost_equals(c_token,"newhist$ogram")) {
	    struct lp_style_type lp = DEFAULT_LP_STYLE_TYPE;
	    struct fill_style_type fs;
	    int previous_token;
	    c_token++;
	    histogram_sequence = -1;
	    free(histogram_title);
	    histogram_title = NULL;

	    if (histogram_rightmost > 0)
		newhist_start = histogram_rightmost + 2;

	    lp.l_type = line_num;
	    fs.fillpattern = LT_UNDEFINED;

	    do {
		previous_token = c_token;

		if (equals(c_token,"at")) {
		    c_token++;
		    newhist_start = real_expression();
		}

		/* Store title in temporary variable and then copy into the */
		/* new histogram structure when it is allocated.            */
		if (!histogram_title && isstringvalue(c_token))
		    histogram_title = try_to_get_string();

		/* Allow explicit starting color or pattern for this histogram */
		lp_parse(&lp, FALSE, FALSE);
		parse_fillstyle(&fs, FS_SOLID, 100, fs.fillpattern, default_fillstyle.border_color);

		} while (c_token != previous_token);

	    newhist_color = lp.l_type;
	    newhist_pattern = fs.fillpattern;
	} else

	if (is_definition(c_token)) {
	    define();
	    if (!equals(c_token,",")) {
		was_definition = TRUE;
		continue;
	    }

	} else {
	    int specs = 0;

	    /* for datafile plot, record datafile spec for title */
	    char* name_str;

	    TBOOLEAN duplication = FALSE;
	    TBOOLEAN set_smooth = FALSE, set_axes = FALSE, set_title = FALSE;
	    TBOOLEAN set_with = FALSE, set_lpstyle = FALSE;
	    TBOOLEAN set_fillstyle = FALSE;
	    TBOOLEAN set_labelstyle = FALSE;

	    plot_num++;

	    was_definition = FALSE;
	    dummy_func = &plot_func;
	    /* should this be saved in "this_plot"? */
	    name_str = string_or_express(NULL);
	    dummy_func = NULL;

	    if (name_str) { /* data file to plot */
		if (parametric && in_parametric)
		    int_error(c_token, "previous parametric function not fully specified");

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
		/* default no palette */
		this_plot->lp_properties.use_palette = 0;

		/* up to MAXDATACOLS cols */
		df_set_plot_mode(MODE_PLOT);    /* Needed for binary datafiles */
		specs = df_open(name_str, MAXDATACOLS, this_plot);

		/* include modifiers in default title */
		this_plot->token = end_token = c_token - 1;

	    } else {

		/* function to plot */

		some_functions = 1;
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
		/* default no palette */
		this_plot->lp_properties.use_palette = 0;
		end_token = c_token - 1;
	    }                   /* end of IS THIS A FILE OR A FUNC block */

	    /* axis defaults */
	    x_axis = FIRST_X_AXIS;
	    y_axis = FIRST_Y_AXIS;

	    /* pm 25.11.2001 allow any order of options */
	    while (!END_OF_COMMAND) {

		/*  deal with smooth */
		if (almost_equals(c_token, "s$mooth")) {
		    int found_token;

		    if (set_smooth) {
			duplication=TRUE;
			break;
		    }
		    found_token = lookup_table(plot_smooth_tbl, ++c_token);

		    switch(found_token) {
		    case SMOOTH_ACSPLINES:
		    case SMOOTH_BEZIER:
		    case SMOOTH_CSPLINES:
		    case SMOOTH_SBEZIER:
		    case SMOOTH_UNIQUE:
		    case SMOOTH_FREQUENCY:
		    case SMOOTH_CUMULATIVE:
		    case SMOOTH_KDENSITY:
			this_plot->plot_smooth = found_token;
			break;
		    case SMOOTH_NONE:
		    default:
			int_error(c_token, "expecting 'unique', 'frequency', 'cumulative', 'kdensity', 'acsplines', 'csplines', 'bezier' or 'sbezier'");
			break;
		    }
		    this_plot->plot_style = LINES;
		    c_token++;      /* skip format */
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

		/* deal with title */
		if (almost_equals(c_token, "t$itle")) {
		    if (set_title) {
			duplication=TRUE;
			break;
		    }
		    this_plot->title_no_enhanced = !key->enhanced;
			/* title can be enhanced if not explicitly disabled */
		    if (parametric) {
			if (in_parametric)
			    int_error(c_token, "\"title\" allowed only after parametric function fully specified");
			else if (xtitle != NULL)
			    xtitle[0] = '\0';       /* Remove default title . */
		    }
		    c_token++;

		    if (almost_equals(c_token,"col$umnheader")) {
			df_set_key_title_columnhead(this_plot->plot_type);
		    } else 

#ifdef BACKWARDS_COMPATIBLE
		    /* Annoying backwards-compatibility hack - deprecate! */
		    if (isanumber(c_token)) {
			c_token--;
			df_set_key_title_columnhead(this_plot->plot_type);
		    } else
#endif

		    if (!(this_plot->title = try_to_get_string()))
			int_error(c_token, "expecting \"title\" for plot");
		    set_title = TRUE;
		    continue;
		}

		if (almost_equals(c_token, "not$itle")) {
		    if (set_title) {
			duplication=TRUE;
			break;
		    }
		    c_token++;
		    if (isstringvalue(c_token))
			try_to_get_string(); /* ignore optionally given title string */
		    this_plot->title_is_suppressed = TRUE;
		    if (xtitle != NULL)
			xtitle[0] = '\0';
		    set_title = TRUE;
		    continue;
		}

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
		    ||  this_plot->plot_style == RGBA_IMAGE)
			get_image_options(&this_plot->image_properties);

		    if ((this_plot->plot_type == FUNC) &&
			((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
			|| (this_plot->plot_style == LABELPOINTS)
			))
			{
			    int_warn(c_token, "This plot style is only for datafiles, reverting to \"points\"");
			    this_plot->plot_style = POINTSTYLE;
			}
		    set_with = TRUE;
		    continue;
		}

		/* Labels can have font and text property info as plot options */
		/* In any case we must allocate one instance of the text style */
		/* that all labels in the plot will share.                     */
		if (this_plot->plot_style == LABELPOINTS) {
		    int stored_token = c_token;

		    if (this_plot->labels == NULL) {
			this_plot->labels = new_text_label(-1);
			this_plot->labels->pos = JUST_CENTRE;
			this_plot->labels->layer = LAYER_PLOTLABELS;
		    }
		    parse_label_options(this_plot->labels);
		    if (stored_token != c_token) {
			if (set_labelstyle) {
			    duplication = TRUE;
			    break;
			} else {
			    set_labelstyle = TRUE;
			    continue;
			}
		    }
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
			    this_plot->arrow_properties.lp_properties.l_type = line_num;
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
		} else {
		    int stored_token = c_token;
		    struct lp_style_type lp = DEFAULT_LP_STYLE_TYPE;

		    lp.l_type = line_num;
		    lp.p_type = point_num;

		    /* user may prefer explicit line styles */
		    if (prefer_line_styles)
			lp_use_properties(&lp, line_num+1);

		    lp_parse(&lp, TRUE,
			     this_plot->plot_style & PLOT_STYLE_HAS_POINT);
		    if (stored_token != c_token) {
			if (set_lpstyle) {
			    duplication=TRUE;
			    break;
			} else {
			    this_plot->lp_properties = lp;
			    set_lpstyle = TRUE;
			    continue;
			}
		    }
		}

		/* Some plots have a fill style as well */
		if (this_plot->plot_style & PLOT_STYLE_HAS_FILL){
		    if (equals(c_token,"fs") || almost_equals(c_token,"fill$style")) {
			int stored_token = c_token;
			parse_fillstyle(&this_plot->fill_properties,
				default_fillstyle.fillstyle,
				default_fillstyle.filldensity,
				pattern_num,
				default_fillstyle.border_color);
			if (this_plot->plot_style == FILLEDCURVES
			&& this_plot->fill_properties.fillstyle == FS_EMPTY)
			    this_plot->fill_properties.fillstyle = FS_SOLID;
			set_fillstyle = TRUE;
			if (stored_token != c_token)
			    continue;
		    }
		}

		break; /* unknown option */

	    } /* while (!END_OF_COMMAND) */

	    if (duplication)
		int_error(c_token, "duplicated or contradicting arguments in plot options");

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
			this_plot->arrow_properties.lp_properties.l_type = line_num;
		    arrow_parse(&this_plot->arrow_properties, TRUE);
		}
		this_plot->lp_properties = this_plot->arrow_properties.lp_properties;
		set_lpstyle = TRUE;
	    }
	    /* No line/point style given. As lp_parse also supplies
	     * the defaults for linewidth and pointsize, call it now
	     * to define them. */
	    if (! set_lpstyle) {
		this_plot->lp_properties.l_type = line_num;
		this_plot->lp_properties.l_width = 1.0;
		this_plot->lp_properties.p_type = point_num;
		this_plot->lp_properties.p_size = pointsize;
		this_plot->lp_properties.use_palette = 0;

		/* user may prefer explicit line styles */
		if (prefer_line_styles)
		    lp_use_properties(&this_plot->lp_properties, line_num+1);

		lp_parse(&this_plot->lp_properties, TRUE,
			 this_plot->plot_style & PLOT_STYLE_HAS_POINT);

#ifdef BACKWARDS_COMPATIBLE
		/* allow old-style syntax - ignore case lt 3 4 for example */
		if (!END_OF_COMMAND && isanumber(c_token)) {
		    this_plot->lp_properties.l_type =
			this_plot->lp_properties.p_type = int_expression() - 1;
		    if (isanumber(c_token))
			this_plot->lp_properties.p_type = int_expression() - 1;
		}
#endif /* BACKWARDS_COMPATIBLE */

	    }

	    /* Some low-level routines expect to find the pointflag attribute */
	    /* in lp_properties (they don't have access to the full header.   */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		this_plot->lp_properties.pointflag = TRUE;

	    /* Rule out incompatible line/point/style options */
	    if (this_plot->plot_type == FUNC) {
		if ((this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		&&  (this_plot->lp_properties.p_size == PTSZ_VARIABLE))
		    this_plot->lp_properties.p_size = 1;
	    }

#if (0)	    /* FIXME:  restore this after fixing initialization problems with df_matrix */
	    if (df_matrix) {
		if (!(this_plot->plot_style == IMAGE || this_plot->plot_style == RGBIMAGE
	              || this_plot->plot_style == RGBA_IMAGE || this_plot->plot_style == LINES
		      || this_plot->plot_style == POINTSTYLE))
		int_error(NO_CARET, "this 2D plot style cannot handle matrix data");
	    }
#endif

	    /* Similar argument for check that all fill styles were set */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_FILL) {
		if (! set_fillstyle)
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

	    /* If we got this far without initializing the label list, do it now */
	    if (this_plot->plot_style == LABELPOINTS) {
		if (this_plot->labels == NULL) {
		    this_plot->labels = new_text_label(-1);
		    this_plot->labels->pos = JUST_CENTRE;
		    this_plot->labels->layer = LAYER_PLOTLABELS;
		}
		this_plot->labels->place.scalex =
		    (x_axis == SECOND_X_AXIS) ? second_axes : first_axes;
		this_plot->labels->place.scaley =
		    (y_axis == SECOND_Y_AXIS) ? second_axes : first_axes;
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
		    init_histogram(this_plot->histogram,histogram_title);
		    histogram_title = NULL;
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
		    this_plot->lp_properties.l_type = this_plot->histogram_sequence
						    + this_plot->histogram->startcolor;
		if (this_plot->histogram->startpattern != LT_UNDEFINED)
		    this_plot->fill_properties.fillpattern = this_plot->histogram_sequence
						    + this_plot->histogram->startpattern;
	    }

	    /* Styles that use palette */
	    if (this_plot->plot_style == IMAGE)
		this_plot->lp_properties.use_palette = 1;

	    /* we can now do some checks that we deferred earlier */

	    if (this_plot->plot_type == DATA) {
		if (specs < 0) {
		    /* Error check to handle missing or unreadable file */
		    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
			++point_num;
		    ++line_num;
		    this_plot->plot_type = NODATA;
		    goto SKIPPED_EMPTY_FILE;
		}
		if (! (uses_axis[x_axis] & USES_AXIS_FOR_DATA)
		    && X_AXIS.autoscale) {
		    if (X_AXIS.autoscale & AUTOSCALE_MIN)
			X_AXIS.min = VERYLARGE;
		    if (X_AXIS.autoscale & AUTOSCALE_MAX)
			X_AXIS.max = -VERYLARGE;
		}
		if (X_AXIS.is_timedata) {
		    if (specs < 2)
			int_error(c_token, "Need full using spec for x time data");
		}
		if (Y_AXIS.is_timedata) {
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
		if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		    ++point_num;
		++line_num;
	    }
	    if (this_plot->plot_type == DATA) {
		/* actually get the data now */
		if (get_data(this_plot) == 0) {
		    /* EAM 2005 - warn, but keep going */
		    int_warn(c_token-1,"Skipping data file with no valid points");
		    this_plot->plot_type = NODATA;
		    goto SKIPPED_EMPTY_FILE;
		}

		/* Fiddle the auto-scaling data for specific plot styles */
		if (this_plot->plot_style == HISTOGRAMS)
		    histogram_range_fiddling(this_plot);
		if (this_plot->plot_style == BOXES)
		    box_range_fiddling(this_plot);
		if (this_plot->plot_style == RGBIMAGE || this_plot->plot_style == RGBA_IMAGE) {
		    if (CB_AXIS.autoscale & AUTOSCALE_MIN)
			CB_AXIS.min = 0;
		    if (CB_AXIS.autoscale & AUTOSCALE_MAX)
			CB_AXIS.max = 255;
		}

		/* sort */
		switch (this_plot->plot_smooth) {
		/* sort and average, if the style requires */
		case SMOOTH_UNIQUE:
		case SMOOTH_FREQUENCY:
		case SMOOTH_CUMULATIVE:
		case SMOOTH_CSPLINES:
		case SMOOTH_ACSPLINES:
		case SMOOTH_SBEZIER:
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
		case SMOOTH_FREQUENCY:
		case SMOOTH_CUMULATIVE:
		    gen_interp_frequency(this_plot);
		    break;
		case SMOOTH_CSPLINES:
		case SMOOTH_ACSPLINES:
		case SMOOTH_BEZIER:
		case SMOOTH_SBEZIER:
		case SMOOTH_KDENSITY:
		    gen_interp(this_plot);
		case SMOOTH_NONE:
		case SMOOTH_UNIQUE:
		default:
		    break;
		}

		/* now that we know the plot style, adjust the x- and yrange */
		/* adjust_range(this_plot); no longer needed */

		/* Images are defined by a grid representing centers of pixels.
		 * Compensate for extent of the image so `set autoscale fix`
		 * uses outer edges of outer pixels in axes adjustment.
		 */
                if ((this_plot->plot_style == IMAGE
		    || this_plot->plot_style == RGBIMAGE
		    || this_plot->plot_style == RGBA_IMAGE)) {
		    this_plot->image_properties.type = IC_PALETTE;
		    plot_image_or_update_axes(this_plot, TRUE);
		}

	    }

	    SKIPPED_EMPTY_FILE:
	    /* Note position in command line for second pass */
		this_plot->token = c_token;
		tp_ptr = &(this_plot->next);

	} /* !is_defn */

	if (in_parametric) {
	    if (equals(c_token, ",")) {
		c_token++;
		continue;
	    } else
		break;
	}

	/* Iterate-over-plot mechanism */
	if (empty_iteration() && this_plot) {
	    this_plot->plot_type = NODATA;
	} else if (next_iteration()) {
	    c_token = start_token;
	    continue;
	}

	if (equals(c_token, ",")) {
	    c_token++;
	    check_for_iteration();
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
	if (some_functions && uses_axis[FIRST_X_AXIS])
	    if (axis_array[FIRST_X_AXIS].max == -VERYLARGE ||
		axis_array[FIRST_X_AXIS].min == VERYLARGE) {
		    axis_array[FIRST_X_AXIS].min = -10;
		    axis_array[FIRST_X_AXIS].max = 10;
	}

	/* check that xmin -> xmax is not too small */
	axis_checked_extend_empty_range(FIRST_X_AXIS, "x range is invalid");

	if (uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA) {
	    /* check that x2min -> x2max is not too small */
	    axis_checked_extend_empty_range(SECOND_X_AXIS, "x2 range is invalid");
	} else if (axis_array[SECOND_X_AXIS].autoscale) {
	    /* copy x1's range */
	    if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MIN)
		axis_array[SECOND_X_AXIS].min = axis_array[FIRST_X_AXIS].min;
	    if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MAX)
		axis_array[SECOND_X_AXIS].max = axis_array[FIRST_X_AXIS].max;
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

	check_for_iteration();

	/* Read through functions */
	while (TRUE) {
	    if (!in_parametric && !was_definition)
		start_token = c_token;

	    if (is_definition(c_token)) {
		define();
		if (!equals(c_token,",")) {
		    was_definition = TRUE;
		    continue;
		}

	    } else {
		struct at_type *at_ptr;
		char *name_str;
		was_definition = FALSE;

		/* HBB 20000820: now globals in 'axis.c' */
		x_axis = this_plot->x_axis;
		y_axis = this_plot->y_axis;

		plot_num++;

		dummy_func = &plot_func;
		/* WARNING: do NOT free name_str */
		/* FIXME: should this be saved in "this_plot"? */
		name_str = string_or_express(&at_ptr);

		if (!name_str) {            /* function to plot */
		    if (parametric) {   /* toggle parametric axes */
			in_parametric = !in_parametric;
		    }
		    plot_func.at = at_ptr;

		    if (!parametric && !polar) {
			t_min = X_AXIS.min;
			t_max = X_AXIS.max;
			axis_unlog_interval(x_axis, &t_min, &t_max, 1);
			t_step = (t_max - t_min) / (samples_1 - 1);
		    }
		    for (i = 0; i < samples_1; i++) {
			double temp;
			struct value a;
			double t = t_min + i * t_step;
			/* parametric/polar => NOT a log quantity */
			double x = (!parametric && !polar)
			    ? AXIS_DE_LOG_VALUE(x_axis, t) : t;

			(void) Gcomplex(&plot_func.dummy_values[0], x, 0.0);
			evaluate_at(plot_func.at, &a);

			if (undefined || (fabs(imag(&a)) > zero)) {
			    this_plot->points[i].type = UNDEFINED;
			    continue;
			}
			temp = real(&a);

			/* width of box not specified */
			this_plot->points[i].z = -1.0;
			/* for the moment */
			this_plot->points[i].type = INRANGE;

			if (parametric) {
			    /* we cannot do range-checking now, since for
			     * the x function we did not know which axes
			     * we were using
			     * DO NOT TAKE LOGS YET - do it in parametric_fixup
			     */
			    /* ignored, actually... */
			    this_plot->points[i].x = t;
			    this_plot->points[i].y = temp;
			    if (boxwidth >= 0 && boxwidth_is_absolute )
				this_plot->points[i].z = 0;
			} else if (polar) {
			    double y;
			    if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MAX) && temp > axis_array[R_AXIS].max)
				this_plot->points[i].type = OUTRANGE;
			    if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MIN))
				temp -= axis_array[R_AXIS].min;
			    y = temp * sin(x * ang2rad);
			    x = temp * cos(x * ang2rad);
			    if (boxwidth >= 0 &&  boxwidth_is_absolute) {
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
				STORE_WITH_LOG_AND_UPDATE_RANGE( this_plot->points[i].xlow, xlow, dmy_type, x_axis, 
								 this_plot->noautoscale, NOOP, NOOP );
				dmy_type = INRANGE;
				STORE_WITH_LOG_AND_UPDATE_RANGE( this_plot->points[i].xhigh, xhigh, dmy_type, x_axis,
								 this_plot->noautoscale, NOOP, NOOP );
			    }
			    temp = y;
			    STORE_WITH_LOG_AND_UPDATE_RANGE(this_plot->points[i].x, x, this_plot->points[i].type, x_axis,
			    				    this_plot->noautoscale, NOOP, goto come_here_if_undefined);
			    STORE_WITH_LOG_AND_UPDATE_RANGE(this_plot->points[i].y, y, this_plot->points[i].type, y_axis,
			    				    this_plot->noautoscale, NOOP, goto come_here_if_undefined);
			} else {        /* neither parametric or polar */
			    /* If non-para, it must be INRANGE */
			    /* logscale ? log(x) : x */
			    this_plot->points[i].x = t;

			    /* For boxes [only] check use of boxwidth */
			    if ((this_plot->plot_style == BOXES)
			    &&  (boxwidth >= 0 && boxwidth_is_absolute)) {
				double xlow, xhigh;
				int dmy_type = INRANGE;
				this_plot->points[i].z = 0;
				if (axis_array[this_plot->x_axis].log) {
				    double base = axis_array[this_plot->x_axis].base;
				    xlow = x * pow(base, -boxwidth/2.);
				    xhigh = x * pow(base, boxwidth/2.);
				} else {
				    xlow = x - boxwidth/2;
				    xhigh = x + boxwidth/2;
				}
				STORE_WITH_LOG_AND_UPDATE_RANGE( this_plot->points[i].xlow, xlow, dmy_type, x_axis,
								    this_plot->noautoscale, NOOP, NOOP );
				dmy_type = INRANGE;
				STORE_WITH_LOG_AND_UPDATE_RANGE( this_plot->points[i].xhigh, xhigh, dmy_type, x_axis,
								    this_plot->noautoscale, NOOP, NOOP );
			    }
			    /* Fill in additional fields needed to draw a circle */
			    if (this_plot->plot_style == CIRCLES) {
				this_plot->points[i].xlow = t;
				this_plot->points[i].ylow = 0;
				this_plot->points[i].xhigh = 360;
			    }

			    STORE_WITH_LOG_AND_UPDATE_RANGE(this_plot->points[i].y, temp, this_plot->points[i].type, in_parametric ? x_axis : y_axis,
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

	    /* Iterate-over-plot mechanism */
	    if (!in_parametric && next_iteration()) {
		c_token = start_token;
		continue;
	    }

	    if (equals(c_token, ",")) {
		c_token++;
		if (!in_parametric)
		    check_for_iteration();
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
    }   /* some_functions */

    /* if first_plot is NULL, we have no functions or data at all. This can
     * happen, if you type "plot x=5", since x=5 is a variable assignment */

    if (plot_num == 0 || first_plot == NULL) {
	int_error(c_token, "no functions or data to plot");
    }

    if (!uses_axis[FIRST_X_AXIS] && !uses_axis[SECOND_X_AXIS])
	if (first_plot->plot_type == NODATA)
	    int_error(NO_CARET,"No data in plot");

    if (uses_axis[FIRST_X_AXIS]) {
	if (axis_array[FIRST_X_AXIS].max == -VERYLARGE ||
	    axis_array[FIRST_X_AXIS].min == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	axis_revert_and_unlog_range(FIRST_X_AXIS);
    }
    if (uses_axis[SECOND_X_AXIS]) {
	if (axis_array[SECOND_X_AXIS].max == -VERYLARGE ||
	    axis_array[SECOND_X_AXIS].min == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	axis_revert_and_unlog_range(SECOND_X_AXIS);
    } else {
	assert(uses_axis[FIRST_X_AXIS]);
	if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MIN)
	    axis_array[SECOND_X_AXIS].min = axis_array[FIRST_X_AXIS].min;
	if (axis_array[SECOND_X_AXIS].autoscale & AUTOSCALE_MAX)
	    axis_array[SECOND_X_AXIS].max = axis_array[FIRST_X_AXIS].max;
	if (! axis_array[SECOND_X_AXIS].autoscale)
	    axis_revert_and_unlog_range(SECOND_X_AXIS);
    }
    if (! uses_axis[FIRST_X_AXIS]) {
	assert(uses_axis[SECOND_X_AXIS]);
	if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN)
	    axis_array[FIRST_X_AXIS].min = axis_array[SECOND_X_AXIS].min;
	if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX)
	    axis_array[FIRST_X_AXIS].max = axis_array[SECOND_X_AXIS].max;
    }


    if (uses_axis[FIRST_Y_AXIS]) {
	axis_checked_extend_empty_range(FIRST_Y_AXIS, "all points y value undefined!");
	axis_revert_and_unlog_range(FIRST_Y_AXIS);
    }
    if (uses_axis[SECOND_Y_AXIS]) {
	axis_checked_extend_empty_range(SECOND_Y_AXIS, "all points y2 value undefined!");
	axis_revert_and_unlog_range(SECOND_Y_AXIS);
    } else {
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
    if (! uses_axis[FIRST_Y_AXIS]) {
	assert(uses_axis[SECOND_Y_AXIS]);
	if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MIN)
	    axis_array[FIRST_Y_AXIS].min = axis_array[SECOND_Y_AXIS].min;
	if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MAX)
	    axis_array[FIRST_Y_AXIS].max = axis_array[SECOND_Y_AXIS].max;
    }

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

    if (table_mode)
	print_table(first_plot, plot_num);
    else {
	START_LEAK_CHECK();     /* check for memory leaks in this routine */

	/* do_plot now uses axis_array[] */
	do_plot(first_plot, plot_num);

	END_LEAK_CHECK();

	/* after do_plot(), axis_array[].min and .max
	 * contain the plotting range actually used (rounded
	 * to tic marks, not only the min/max data values)
	 *  --> save them now for writeback if requested
	 */
	SAVE_WRITEBACK_ALL_AXES;
	/* update GPVAL_ variables available to user */
	update_gpval_variables(1);

#ifdef VOLATILE_REFRESH
	/* Mark these plots as safe for quick refresh */
	refresh_nplots = plot_num;
	refresh_ok = 2;
#endif
    }

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
    size_t tlen;
    int i, curve;
    char *new_title;

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

	    /* because syntax is   plot x(t), y(t) axes ..., only
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
		    double t = xp->points[i].y * ang2rad;
		    double x, y;
		    if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MAX) && r > axis_array[R_AXIS].max)
			yp->points[i].type = OUTRANGE;
		    if (!(axis_array[R_AXIS].autoscale & AUTOSCALE_MIN)) {
			/* store internally as if plotting r(t)-rmin */
			r -= axis_array[R_AXIS].min;
		    }
		    x = r * cos(t);
		    y = r * sin(t);
		    if (boxwidth >= 0 && boxwidth_is_absolute) {
			int dmy_type = INRANGE;
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
			int dmy_type = INRANGE;
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
