#ifndef lint
static char *RCSid = "$Id: plot2d.c,v 1.53 1998/06/18 14:55:14 ddenholm Exp $";
#endif

/* GNUPLOT - plot2d.c */

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

#include "plot.h"
#include "setshow.h"
#include "fit.h"
#include "binary.h"

#ifndef _Windows
# include "help.h"
#endif

#ifndef STDOUT
#define STDOUT 1
#endif

/* static prototypes */

void plotrequest __PROTO((void));
void plot3drequest __PROTO((void));
void define __PROTO((void));
static int get_data __PROTO((struct curve_points * this_plot));
static void store2d_point __PROTO((struct curve_points * this_plot, int i, double x, double y, double xlow, double xhigh, double ylow, double yhigh, double width));
static void print_table __PROTO((struct curve_points * first_plot, int plot_num));
static void eval_plots __PROTO((void));
static void parametric_fixup __PROTO((struct curve_points * start_plot, int *plot_num));


/* the curves/surfaces of the plot */
struct curve_points *first_plot = NULL;
static struct udft_entry plot_func;
extern struct udft_entry *dummy_func;

/* jev -- for passing data thru user-defined function */
/* Needed by datafile.c */
struct udft_entry ydata_func;

extern int datatype[];
extern char timefmt[];

extern TBOOLEAN is_3d_plot;
extern int plot_token;

/* in order to support multiple axes, and to
 * simplify ranging in parametric plots, we use
 * arrays to store some things.
 * Elements are z = 0, y1 = 1, x1 = 2, [z2 =4 ], y2 = 5, x2 = 6
 * these are given symbolic names in plot.h
 */

/* Were declared in command.c */
double min_array[AXIS_ARRAY_SIZE], max_array[AXIS_ARRAY_SIZE];
int auto_array[AXIS_ARRAY_SIZE];
TBOOLEAN log_array[AXIS_ARRAY_SIZE];
double base_array[AXIS_ARRAY_SIZE];
double log_base_array[AXIS_ARRAY_SIZE];

/* Deleted from setshow.h and renamed */
extern FILE *gpoutfile;

/* if user specifies [10:-10] we use [-10:10] internally, and swap at end */
int reverse_range[AXIS_ARRAY_SIZE];

/* info from datafile module */
extern int df_datum;
extern int df_line_number;
extern int df_no_use_specs;
extern int df_eof;
extern int df_timecol[];
extern TBOOLEAN df_binary;

#define Inc_c_token if (++c_token >= num_tokens) \
  int_error (c_token, "Syntax error");


/*
 * IMHO, code is getting too cluttered with repeated chunks of
 * code. Some macros to simplify, I hope.
 *
 * do { } while(0) is comp.lang.c recommendation for complex macros
 * also means that break can be specified as an action, and it will
 * 
 */

/*  copy scalar data to arrays
 * optimiser should optimise infinite away
 * dont know we have to support ranges [10:-10] - lets reverse
 * it for now, then fix it at the end.
 */
#define INIT_ARRAYS(axis, min, max, auto, is_log, base, log_base, infinite) \
do{auto_array[axis] = auto; \
   min_array[axis] = (infinite && (auto&1)) ? VERYLARGE : min; \
   max_array[axis] = (infinite && (auto&2)) ? -VERYLARGE : max; \
   log_array[axis] = is_log; base_array[axis] = base; log_base_array[axis] = log_base;\
}while(0)
/* handle reversed ranges */
#define CHECK_REVERSE(axis) \
do{\
 if (auto_array[axis] == 0 && max_array[axis] < min_array[axis]) {\
  double temp = min_array[axis]; min_array[axis] = max_array[axis]; max_array[axis] = temp;\
  reverse_range[axis] = 1; \
 } else reverse_range[axis] = (range_flags[axis]&RANGE_REVERSE); \
}while(0)


/* get optional [min:max] */
#define LOAD_RANGE(axis) \
do {\
 if (equals(c_token, "[")) { \
  c_token++; \
  auto_array[axis] = load_range(axis,&min_array[axis], &max_array[axis], auto_array[axis]);\
  if (!equals(c_token, "]"))\
   int_error(c_token, "']' expected");\
  c_token++;\
 }\
} while (0)


/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate
 * Do OUT_ACTION or UNDEF_ACTION as appropriate
 * adjust range provided type is INRANGE (ie dont adjust y if x is outrange
 * VALUE must not be same as STORE
 */

#define STORE_WITH_LOG_AND_FIXUP_RANGE(STORE, VALUE, TYPE, AXIS, OUT_ACTION, UNDEF_ACTION)\
do { if (log_array[AXIS]) { if (VALUE<0.0) {TYPE = UNDEFINED; UNDEF_ACTION; break;} \
              else if (VALUE == 0.0){STORE = -VERYLARGE; TYPE = OUTRANGE; OUT_ACTION; break;} \
              else { STORE = log(VALUE)/log_base_array[AXIS]; } \
     } else STORE = VALUE; \
     if (TYPE != INRANGE) break;  /* dont set y range if x is outrange, for example */ \
     if ( VALUE<min_array[AXIS] ) { \
      if (auto_array[AXIS] & 1) min_array[AXIS] = VALUE; else { TYPE = OUTRANGE; OUT_ACTION; break; }  \
     } \
     if ( VALUE>max_array[AXIS] ) { \
      if (auto_array[AXIS] & 2) max_array[AXIS] = VALUE; else { TYPE = OUTRANGE; OUT_ACTION; }   \
     } \
} while(0)

/* use this instead empty macro arguments to work around NeXT cpp bug */
/* if this fails on any system, we might use ((void)0) */
#define NOOP			/* */

/* check range and take logs of min and max if logscale
 * this also restores min and max for ranges like [10:-10]
 */
#ifdef HAVE_STRINGIZE
# define LOG_MSG(x) #x " range must be greater than 0 for log scale!"
#else
# define LOG_MSG(x) "x range must be greater than 0 for log scale!"
#endif

#define FIXUP_RANGE_FOR_LOG(AXIS, WHICH) \
do { if (reverse_range[AXIS]) { \
      double temp = min_array[AXIS]; \
      min_array[AXIS] = max_array[AXIS]; \
      max_array[AXIS] = temp; \
     }\
     if (log_array[AXIS]) { \
      if (min_array[AXIS] <= 0.0 || max_array[AXIS] <= 0.0) \
       int_error(NO_CARET, LOG_MSG(WHICH)); \
      min_array[AXIS] = log(min_array[AXIS])/log_base_array[AXIS]; \
      max_array[AXIS] = log(max_array[AXIS])/log_base_array[AXIS];  \
   } \
} while(0)



void plotrequest()
/*
 * In the parametric case we can say plot [a= -4:4] [-2:2] [-1:1] sin(a),a**2
 * while in the non-parametric case we would say only plot [b= -2:2] [-1:1]
 * sin(b)
 */
{
    int dummy_token = -1;

    if (!term)			/* unknown */
	int_error(c_token, "use 'set term' to set terminal type first");

    is_3d_plot = FALSE;

    if (parametric && strcmp(dummy_var[0], "u") == 0)
	strcpy(dummy_var[0], "t");

    /* initialise the arrays from the 'set' scalars */

    INIT_ARRAYS(FIRST_X_AXIS, xmin, xmax, autoscale_x, is_log_x, base_log_x, log_base_log_x, 0);
    INIT_ARRAYS(FIRST_Y_AXIS, ymin, ymax, autoscale_y, is_log_y, base_log_y, log_base_log_y, 1);
    INIT_ARRAYS(SECOND_X_AXIS, x2min, x2max, autoscale_x2, is_log_x2, base_log_x2, log_base_log_x2, 0);
    INIT_ARRAYS(SECOND_Y_AXIS, y2min, y2max, autoscale_y2, is_log_y2, base_log_y2, log_base_log_y2, 1);

    min_array[T_AXIS] = tmin;
    max_array[T_AXIS] = tmax;

    if (equals(c_token, "[")) {
	c_token++;
	if (isletter(c_token)) {
	    if (equals(c_token + 1, "=")) {
		dummy_token = c_token;
		c_token += 2;
	    } else {
		/* oops; probably an expression with a variable. */
		/* Parse it as an xmin expression. */
		/* used to be: int_error("'=' expected",c_token); */
	    }
	} {
	    int axis = (parametric || polar) ? T_AXIS : FIRST_X_AXIS;


	    auto_array[axis] = load_range(axis, &min_array[axis], &max_array[axis], auto_array[axis]);
	    if (!equals(c_token, "]"))
		int_error(c_token, "']' expected");
	    c_token++;
	}			/* end of scope of 'axis' */
    }				/* first '[' */
    if (parametric || polar)	/* set optional x ranges */
	LOAD_RANGE(FIRST_X_AXIS);
    else {
	/* order of t doesn't matter, but x does */
	CHECK_REVERSE(FIRST_X_AXIS);
    }

    LOAD_RANGE(FIRST_Y_AXIS);
    CHECK_REVERSE(FIRST_Y_AXIS);
    LOAD_RANGE(SECOND_X_AXIS);
    CHECK_REVERSE(SECOND_X_AXIS);
    LOAD_RANGE(SECOND_Y_AXIS);
    CHECK_REVERSE(SECOND_Y_AXIS);

    /* use the default dummy variable unless changed */
    if (dummy_token >= 0)
	copy_str(c_dummy_var[0], dummy_token, MAX_ID_LEN);
    else
	(void) strcpy(c_dummy_var[0], dummy_var[0]);

    eval_plots();
}

/* Use up to 7 columns in data file at once -- originally it was 5 */
#define NCOL 7


/* A quick note about boxes style. For boxwidth auto, we cannot
 * calculate widths yet, since it may be sorted, etc. But if
 * width is set, we must do it now, before logs of xmin/xmax
 * are taken.
 * We store -1 in point->z as a marker to mean width needs to be
 * calculated, or 0 to mean that xmin/xmax are set correctly
 */


static int get_data(this_plot)
struct curve_points *this_plot;
/* this_plot->token is after datafile spec, for error reporting
 * it will later be moved passed title/with/linetype/pointtype
 */
{
    register int i /* num. points ! */ , j, col;
    double v[NCOL];
    int storetoken = this_plot->token;

    /* eval_plots has already opened file */

    switch (this_plot->plot_style) {	/* set maximum columns to scan */
    case XYERRORLINES:
    case XYERRORBARS:
    case BOXXYERROR:
	col = 7;
	break;

    case BOXERROR:
    case FINANCEBARS:
    case CANDLESTICKS:
	col = 5;
	break;

    case XERRORLINES:
    case YERRORLINES:
    case XERRORBARS:
    case YERRORBARS:
    case VECTOR:
	col = 4;
	break;

    case BOXES:
	col = 4;
	break;

    default:
	col = 2;
    }

    if (this_plot->plot_smooth == ACSPLINES)
	col = 3;

    if (df_no_use_specs > col)
	fputs("warning : too many using specs for this style\n", stderr);

    i = 0;
    while ((j = df_readline(v, col)) != DF_EOF) {
	/* j <= col */

	if (i >= this_plot->p_max) {
	    /*
	     * overflow about to occur. Extend size of points[] array. We
	     * either double the size, or add 1000 points, whichever is a
	     * smaller increment. Note i = p_max.
	     */
	    cp_extend(this_plot, i + (i < 1000 ? i : 1000));
	}
	/* Limitation: No xerrorbars with boxes */
	switch (j) {
	default:
	    {
		df_close();
		int_error(c_token, "internal error : df_readline returned %d : datafile line %d", j, df_line_number);
	    }
	case DF_UNDEFINED:
	    /* bad result from extended using expression */
	    this_plot->points[i].type = UNDEFINED;
	    i++;
	    continue;

	case DF_FIRST_BLANK:
	    /* break in data, make next point undefined */
	    this_plot->points[i].type = UNDEFINED;
	    i++;
	    continue;

	case DF_SECOND_BLANK:
	    /* second blank line. We dont do anything
	     * (we did everything when we got FIRST one)
	     */
	    continue;

	case 0:		/* not blank line, but df_readline couldn't parse it */
	    {
		df_close();
		int_error(this_plot->token,
			  "Bad data on line %d", df_line_number);
	    }

	case 1:
	    {			/* only one number */
		/* x is index, assign number to y */
		v[1] = v[0];
		v[0] = df_datum;
		/* nobreak */
	    }

	case 2:
	    /* x, y */
	    /* ylow and yhigh are same as y */

	    if (this_plot->plot_style == BOXES && boxwidth > 0) {
		/* calc width now */
		store2d_point(this_plot, i++, v[0], v[1], v[0] - boxwidth / 2, v[0] + boxwidth / 2, v[1], v[1], 0.0);
	    } else {
		/* xlow and xhigh are same as x */
		store2d_point(this_plot, i++, v[0], v[1], v[0], v[0], v[1], v[1],
			      -1.0);	/* auto width if boxes, else ignored */
	    }
	    break;


	case 3:
	    /* x, y, ydelta OR x, y, xdelta OR x, y, width */
	    if (this_plot->plot_smooth == ACSPLINES)
		store2d_point(this_plot, i++, v[0], v[1], v[0], v[0], v[1], v[1], v[2]);
	    else
		switch (this_plot->plot_style) {
		default:
		    int_warn(storetoken, "This plot style not work with 3 cols. Setting to yerrorbars");
		    this_plot->plot_style = YERRORBARS;
		    /* fall through */

		case YERRORLINES:
		case YERRORBARS:
		case BOXERROR:	/* x, y, dy */
		    store2d_point(this_plot, i++, v[0], v[1], v[0], v[0], v[1] - v[2], v[1] + v[2],
				  -1.0);	/* auto width if boxes, else ignored */
		    break;

		case XERRORLINES:
		case XERRORBARS:
		    store2d_point(this_plot, i++, v[0], v[1], v[0] - v[2], v[0] + v[2], v[1], v[1], 0.0);
		    break;

		case BOXES:
		    /* calculate xmin and xmax here, so that logs are taken if
		     * if necessary
		     */
		    store2d_point(this_plot, i++, v[0], v[1], v[0] - v[2] / 2, v[0] + v[2] / 2, v[1], v[1], 0.0);
		    break;

		}		/*inner switch */

	    break;



	case 4:
	    /* x, y, ylow, yhigh OR
	     * x, y, xlow, xhigh OR
	     * x, y, xdelta, ydelta OR
	     * x, y, ydelta, width
	     */

	    switch (this_plot->plot_style) {
	    default:
		int_warn(storetoken, "This plot style does not work with 4 cols. Setting to yerrorbars");
		this_plot->plot_style = YERRORBARS;
		/* fall through */

	    case YERRORLINES:
	    case YERRORBARS:
		store2d_point(this_plot, i++, v[0], v[1], v[0], v[0], v[2], v[3], -1.0);
		break;

	    case BOXXYERROR:	/* x, y, dx, dy */
	    case XYERRORLINES:
	    case XYERRORBARS:
		store2d_point(this_plot, i++, v[0], v[1], v[0] - v[2], v[0] + v[2], v[1] - v[3], v[1] + v[3], 0.0);
		break;


	    case BOXES:	/* x, y, xmin, xmax */
		store2d_point(this_plot, i++, v[0], v[1], v[2], v[3], v[1], v[1], 0.0);
		break;

	    case XERRORLINES:
	    case XERRORBARS:
		store2d_point(this_plot, i++, v[0], v[1], v[2], v[3], v[1], v[1], 0.0);
		break;

	    case BOXERROR:
		/* x,y, xleft, xright */
		store2d_point(this_plot, i++, v[0], v[1], v[0], v[0], v[1] - v[2], v[1] + v[2], 0.0);
		break;

	    case VECTOR:
		/* x,y,dx,dy */
		store2d_point(this_plot, i++, v[0], v[1], v[0], v[0] + v[2], v[1], v[1] + v[3], -1.0);
		break;
	    }			/*inner switch */

	    break;


	case 5:
	    {			/* x, y, ylow, yhigh, width  or  x open low high close */
		switch (this_plot->plot_style) {
		default:
		    int_warn(storetoken, "Five col. plot style must be boxerrorbars, financebars or candlesticks. Setting to boxerrorbars");
		    this_plot->plot_style = BOXERROR;
		    /*fall through */

		case BOXERROR:	/* x, y, ylow, yhigh, width */
		    store2d_point(this_plot, i++, v[0], v[1], v[0] - v[4] / 2, v[0] + v[4] / 2, v[2], v[3], 0.0);
		    break;

		case FINANCEBARS:
		case CANDLESTICKS:
		    store2d_point(this_plot, i++, v[0], v[1], v[0], v[0], v[2], v[3], v[4]);
		    break;
		}
		break;
	    }

	case 7:
	    /* same as six columns. Width ignored */
	    /* eh ? - fall through */
	case 6:
	    /* x, y, xlow, xhigh, ylow, yhigh */
	    switch (this_plot->plot_style) {
	    default:
		int_warn(storetoken, "This plot style not work with 6 cols. Setting to xyerrorbars");
		this_plot->plot_style = XYERRORBARS;
		/*fall through */
	    case XYERRORLINES:
	    case XYERRORBARS:
	    case BOXXYERROR:
		store2d_point(this_plot, i++, v[0], v[1], v[2], v[3], v[4], v[5], 0.0);
		break;
	    }

	}			/*switch */

    }				/*while */

    this_plot->p_count = i;
    cp_extend(this_plot, i);	/* shrink to fit */

    df_close();
    
    return i; /* i==0 indicates an 'empty' file */
}

/* called by get_data for each point */
static void store2d_point(this_plot, i, x, y, xlow, xhigh, ylow, yhigh, width)
struct curve_points *this_plot;
int i;				/* point number */
double x, y;
double ylow, yhigh;
double xlow, xhigh;
double width;			/* -1 means autocalc, 0 means use xmin/xmax */
{
    struct coordinate GPHUGE *cp = &(this_plot->points[i]);
    int dummy_type = INRANGE;	/* sometimes we dont care about outranging */


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
    dummy_type = cp->type = INRANGE;

    if (polar) {
	double newx, newy;
	if (!(autoscale_r & 2) && y > rmax) {
	    cp->type = OUTRANGE;
	}
	if (!(autoscale_r & 1)) {
	    /* we store internally as if plotting r(t)-rmin */
	    y -= rmin;
	}
	newx = y * cos(x * ang2rad);
	newy = y * sin(x * ang2rad);
#if 0 /* HBB 981118: added polar errorbars */
	/* only lines and points supported with polar */
	y = ylow = yhigh = newy;
	x = xlow = xhigh = newx;
#else 
	y = newy;
	x = newx;

	if (!(autoscale_r & 2) && yhigh > rmax) {
	    cp->type = OUTRANGE;
	}
	if (!(autoscale_r & 1)) {
	    /* we store internally as if plotting r(t)-rmin */
	    yhigh -= rmin;
	}
	newx = yhigh * cos(xhigh * ang2rad);
	newy = yhigh * sin(xhigh * ang2rad);
	yhigh = newy;
	xhigh = newx;

	if (!(autoscale_r & 2) && ylow > rmax) {
	    cp->type = OUTRANGE;
	}
	if (!(autoscale_r & 1)) {
	    /* we store internally as if plotting r(t)-rmin */
	    ylow -= rmin;
	}
	newx = ylow * cos(xlow * ang2rad);
	newy = ylow * sin(xlow * ang2rad);
	ylow = newy;
	xlow = newx;
#endif
    }
    /* return immediately if x or y are undefined
     * we dont care if outrange for high/low.
     * BUT if high/low undefined (ie log( < 0 ), no number is stored,
     * but graphics.c doesn't know.
     * explicitly store -VERYLARGE;
     */
    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->x, x, cp->type, this_plot->x_axis, NOOP, return);
    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->xlow, xlow, dummy_type, this_plot->x_axis, NOOP, cp->xlow = -VERYLARGE);
    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->xhigh, xhigh, dummy_type, this_plot->x_axis, NOOP, cp->xhigh = -VERYLARGE);
    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->y, y, cp->type, this_plot->y_axis, NOOP, return);
    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->ylow, ylow, dummy_type, this_plot->y_axis, NOOP, cp->ylow = -VERYLARGE);
    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->yhigh, yhigh, dummy_type, this_plot->y_axis, NOOP, cp->yhigh = -VERYLARGE);
    cp->z = width;
}				/* store2d_point */



/*
 * print_points: a debugging routine to print out the points of a curve, and
 * the curve structure. If curve<0, then we print the list of curves.
 */

#if 0				/* not used */
static char *plot_type_names[4] =
{
    "Function", "Data", "3D Function", "3d data"
};
static char *plot_style_names[14] =
{
    "Lines", "Points", "Impulses", "LinesPoints", "Dots", "XErrorbars",
 "YErrorbars", "XYErrorbars", "BoxXYError", "Boxes", "Boxerror", "Steps",
    "FSteps", "Vector",
 "XErrorlines", "YErrorlines", "XYErrorlines"
};
static char *plot_smooth_names[5] =
{
    "None", "Unique", "CSplines", "ACSplines", "Bezier", "SBezier"
};

static void print_points(curve)
int curve;			/* which curve to print */
{
    register struct curve_points *this_plot;
    int i;

    if (curve < 0) {
	for (this_plot = first_plot, i = 0;
	     this_plot != NULL;
	     i++, this_plot = this_plot->next_cp) {
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
	    printf("Plot title: '%s'\n", this_plot->title);
	    printf("Line type %d\n", this_plot->line_type);
	    printf("Point type %d\n", this_plot->point_type);
	    printf("max points %d\n", this_plot->p_max);
	    printf("current points %d\n", this_plot->p_count);
	    printf("\n");
	}
    } else {
	for (this_plot = first_plot, i = 0;
	     i < curve && this_plot != NULL;
	     i++, this_plot = this_plot->next_cp);
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

static void print_table(this_plot, plot_num)
struct curve_points *this_plot;
int plot_num;
{
    int i, curve;

    for (curve = 0; curve < plot_num;
	 curve++, this_plot = this_plot->next_cp) {
	fprintf(gpoutfile, "#Curve %d, %d points\n#x y type\n", curve, this_plot->p_count);
	for (i = 0; i < this_plot->p_count; i++) {
	    fprintf(gpoutfile, "%g %g %c\n",
		    this_plot->points[i].x,
		    this_plot->points[i].y,
		    this_plot->points[i].type == INRANGE ? 'i'
		    : this_plot->points[i].type == OUTRANGE ? 'o'
		    : 'u');
	}
	fputc('\n', gpoutfile);
    }
/* two blank lines between plots in table output */
    fputc('\n', gpoutfile);
    fflush(gpoutfile);
}

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
static void eval_plots()
{
    register int i;
    register struct curve_points *this_plot, **tp_ptr;

    int some_functions = 0;
    int plot_num, line_num, point_num, xparam = 0;
    char *xtitle;
    int begin_token = c_token;	/* so we can rewind for second pass */

    int uses_axis[AXIS_ARRAY_SIZE];

    uses_axis[FIRST_X_AXIS] =
	uses_axis[FIRST_Y_AXIS] =
	uses_axis[SECOND_X_AXIS] =
	uses_axis[SECOND_Y_AXIS] = 0;

    /* Reset first_plot. This is usually done at the end of this function.
       If there is an error within this function, the memory is left allocated,
       since we cannot call cp_free if the list is incomplete. Making sure that
       the list structure is always vaild requires some rewriting */
    first_plot = NULL;

    tp_ptr = &(first_plot);
    plot_num = 0;
    line_num = 0;		/* default line type */
    point_num = 0;		/* default point type */

    xtitle = NULL;

    /*** First Pass: Read through data files ***
     * This pass serves to set the xrange and to parse the command, as well
     * as filling in every thing except the function data. That is done after
     * the xrange is defined.
     */
    while (TRUE) {
	if (END_OF_COMMAND)
	    int_error(c_token, "function to plot expected");

	if (is_definition(c_token)) {
	    define();
	} else {
	    int x_axis = 0, y_axis = 0;
	    int specs;

	    /* for datafile plot, record datafile spec for title */
	    int start_token = c_token, end_token;

	    plot_num++;

	    if (isstring(c_token)) {	/* data file to plot */

		if (parametric && xparam)
		    int_error(c_token, "previous parametric function not fully specified");

		if (*tp_ptr)
		    this_plot = *tp_ptr;
		else {		/* no memory malloc()'d there yet */
		    this_plot = cp_alloc(MIN_CRV_POINTS);
		    *tp_ptr = this_plot;
		}
		this_plot->plot_type = DATA;
		this_plot->plot_style = data_style;
		this_plot->plot_smooth = NONE;

		specs = df_open(NCOL);	/* up to NCOL cols */
		/* this parses data-file-specific modifiers only */
		/* we'll sort points when we know style, if necessary */
		if (df_binary)
		    int_error(c_token, "2d binary files not yet supported");

		this_plot->token = end_token = c_token - 1;	/* include modifiers in default title */

	    } else {

		/* function to plot */

		some_functions = 1;
		if (parametric)	/* working on x parametric function */
		    xparam = 1 - xparam;
		if (*tp_ptr) {
		    this_plot = *tp_ptr;
		    cp_extend(this_plot, samples + 1);
		} else {	/* no memory malloc()'d there yet */
		    this_plot = cp_alloc(samples + 1);
		    *tp_ptr = this_plot;
		}
		this_plot->plot_type = FUNC;
		this_plot->plot_style = func_style;
		dummy_func = &plot_func;
		plot_func.at = temp_at();
		dummy_func = NULL;
		/* ignore it for now */
		end_token = c_token - 1;
	    }			/* end of IS THIS A FILE OR A FUNC block */


	    /*  deal with smooth */
	    if (almost_equals(c_token, "s$mooth")) {

		if (END_OF_COMMAND)
		    int_error(c_token, "expecting smooth parameter");
		else {
		    c_token++;
		    if (almost_equals(c_token, "u$nique"))
			this_plot->plot_smooth = UNIQUE;
		    else if (almost_equals(c_token, "a$csplines"))
			this_plot->plot_smooth = ACSPLINES;
		    else if (almost_equals(c_token, "c$splines"))
			this_plot->plot_smooth = CSPLINES;
		    else if (almost_equals(c_token, "b$ezier"))
			this_plot->plot_smooth = BEZIER;
		    else if (almost_equals(c_token, "s$bezier"))
			this_plot->plot_smooth = SBEZIER;
		    else
			int_error(c_token, "expecting 'unique', 'acsplines', 'csplines', 'bezier' or 'sbezier'");
		}
		this_plot->plot_style = LINES;
		c_token++;	/* skip format */
	    }
	    /* look for axes/axis */

	    if (almost_equals(c_token, "ax$es") || almost_equals(c_token, "ax$is")) {
		if (parametric && xparam)
		    int_error(c_token, "previous parametric function not fully specified");

		if (equals(++c_token, "x1y1")) {
		    x_axis = FIRST_X_AXIS;
		    y_axis = FIRST_Y_AXIS;
		    ++c_token;
		} else if (equals(c_token, "x2y2")) {
		    x_axis = SECOND_X_AXIS;
		    y_axis = SECOND_Y_AXIS;
		    ++c_token;
		} else if (equals(c_token, "x1y2")) {
		    x_axis = FIRST_X_AXIS;
		    y_axis = SECOND_Y_AXIS;
		    ++c_token;
		} else if (equals(c_token, "x2y1")) {
		    x_axis = SECOND_X_AXIS;
		    y_axis = FIRST_Y_AXIS;
		    ++c_token;
		} else
		    int_error(c_token, "axes must be x1y1, x1y2, x2y1 or x2y2");
	    } else {
		x_axis = FIRST_X_AXIS;
		y_axis = FIRST_Y_AXIS;
	    }


	    this_plot->x_axis = x_axis;
	    this_plot->y_axis = y_axis;

	    /* we can now do some checks that we deferred earlier */

	    if (this_plot->plot_type == DATA) {
		if (!(uses_axis[x_axis] & 1) && autoscale_lx) {
		    if (auto_array[x_axis] & 1)
			min_array[x_axis] = VERYLARGE;
		    if (auto_array[x_axis] & 2)
			max_array[x_axis] = -VERYLARGE;
		}
		if (datatype[x_axis] == TIME) {
		    if (specs < 2)
			int_error(c_token, "Need full using spec for x time data");
		    df_timecol[0] = 1;
		}
		if (datatype[y_axis] == TIME) {
		    if (specs < 1)
			int_error(c_token, "Need using spec for y time data");
		    df_timecol[y_axis] = 1;	/* need other cols, but I'm lazy */
		}
		uses_axis[x_axis] |= 1;		/* separate record of datafile and func */
		uses_axis[y_axis] |= 1;
	    } else if (!parametric || !xparam) {
		/* for x part of a parametric function, axes are possibly wrong */
		uses_axis[x_axis] |= 2;		/* separate record of data and func */
		uses_axis[y_axis] |= 2;
	    }
	    if (almost_equals(c_token, "t$itle")) {
		if (parametric) {
		    if (xparam)
			int_error(c_token, "\"title\" allowed only after parametric function fully specified");
		    else if (xtitle != NULL)
			xtitle[0] = '\0';	/* Remove default title . */
		}
		c_token++;
		if (isstring(c_token)) {
		    m_quote_capture(&(this_plot->title), c_token, c_token);
		} else {
		    int_error(c_token, "expecting \"title\" for plot");
		}
		c_token++;
	    } else if (almost_equals(c_token, "not$itle")) {
		if (xtitle != NULL)
		    xtitle[0] = '\0';
		c_token++;
	    } else {
		m_capture(&(this_plot->title), start_token, end_token);
		if (xparam)
		    xtitle = this_plot->title;
	    }


	    if (almost_equals(c_token, "w$ith")) {
		if (parametric && xparam)
		    int_error(c_token, "\"with\" allowed only after parametric function fully specified");
		this_plot->plot_style = get_style();
	    }
	    /* pick up line/point specs
	     * - point spec allowed if style uses points, ie style&2 != 0
	     * - keywords for lt and pt are optional
	     */
	    LP_PARSE(this_plot->lp_properties, 1, this_plot->plot_style & 2,
		     line_num, point_num);

	    /* allow old-style syntax too - ignore case lt 3 4 for example */
	    if (!equals(c_token, ",") && !END_OF_COMMAND) {
		struct value t;
		this_plot->lp_properties.l_type =
		    this_plot->lp_properties.p_type = (int) real(const_express(&t)) - 1;

		if (!equals(c_token, ",") && !END_OF_COMMAND)
		    this_plot->lp_properties.p_type = (int) real(const_express(&t)) - 1;
	    }
	    if (!xparam) {
		if (this_plot->plot_style & 2)	/* style includes points */
		    ++point_num;
		++line_num;
	    }
	    if (this_plot->plot_type == DATA) {
		/* actually get the data now */
		if ( get_data(this_plot) == 0) {
			/* am: not a single line of data (point to be more precise)
			   has been found. So don't issue a misleading warning like
			   "x range is invalid" but stop here! */
			int_error(c_token, "no data point found in specified file");
		}

		/* sort */
		switch (this_plot->plot_smooth) {	/* sort and average, if */
		case UNIQUE:	/* the style requires   */
		case CSPLINES:
		case ACSPLINES:
		case SBEZIER:
		    sort_points(this_plot);
		    cp_implode(this_plot);
		    break;
		default:
		    ;		/* keep gcc -Wall happy */
		}
		switch (this_plot->plot_smooth) {	/* create new data set     */
		case SBEZIER:	/* by evaluation of        */
		case BEZIER:	/* interpolation routines  */
		case ACSPLINES:
		case CSPLINES:
		    gen_interp(this_plot);
		    break;
		default:
		    ;		/* keep gcc -Wall happy */
		}

		/* now that we know the plot style, adjust the x- and yrange */
		/* adjust_range(this_plot); no longer needed */
	    }
	    this_plot->token = c_token;		/* save end of plot for second pass */
	    tp_ptr = &(this_plot->next_cp);

	}			/* !is_defn */

	if (equals(c_token, ","))
	    c_token++;
	else
	    break;
    }

    if (parametric && xparam)
	int_error(NO_CARET, "parametric function not fully specified");


/*** Second Pass: Evaluate the functions ***/
    /*
     * Everything is defined now, except the function data. We expect no
     * syntax errors, etc, since the above parsed it all. This makes the code
     * below simpler. If autoscale_ly, the yrange may still change.
     * we stored last token of each plot, so we dont need to do everything again
     */

    /* give error if xrange badly set from missing datafile error
     * parametric or polar fns can still affect x ranges
     */

    if (!parametric && !polar) {
	if (min_array[FIRST_X_AXIS] == VERYLARGE ||
	    max_array[FIRST_X_AXIS] == -VERYLARGE)
	    int_error(c_token, "x range is invalid");
	/* check that xmin -> xmax is not too small */
	fixup_range(FIRST_X_AXIS, "x");

	if (uses_axis[SECOND_X_AXIS] & 1) {
	    /* some data plots with x2 */
	    if (min_array[SECOND_X_AXIS] == VERYLARGE ||
		max_array[SECOND_X_AXIS] == -VERYLARGE)
		int_error(c_token, "x2 range is invalid");
	    /* check that x2min -> x2max is not too small */
	    fixup_range(SECOND_X_AXIS, "x2");
	} else if (auto_array[SECOND_X_AXIS]) {
	    /* copy x1's range */
	    if (auto_array[SECOND_X_AXIS] & 1)
		min_array[SECOND_X_AXIS] = min_array[FIRST_X_AXIS];
	    if (auto_array[SECOND_X_AXIS] & 2)
		max_array[SECOND_X_AXIS] = max_array[FIRST_X_AXIS];
	}
    }
    if (some_functions) {

	/* call the controlled variable t, since x_min can also mean smallest x */
	double t_min, t_max, t_step;

	if (parametric || polar) {
	    if (!(uses_axis[FIRST_X_AXIS] & 1)) {
		/* these have not yet been set to full width */
		if (auto_array[FIRST_X_AXIS] & 1)
		    min_array[FIRST_X_AXIS] = VERYLARGE;
		if (auto_array[FIRST_X_AXIS] & 2)
		    max_array[FIRST_X_AXIS] = -VERYLARGE;
	    }
	    if (!(uses_axis[SECOND_X_AXIS] & 1)) {
		if (auto_array[SECOND_X_AXIS] & 1)
		    min_array[SECOND_X_AXIS] = VERYLARGE;
		if (auto_array[SECOND_X_AXIS] & 2)
		    max_array[SECOND_X_AXIS] = -VERYLARGE;
	    }
	}
#define SET_DUMMY_RANGE(AXIS) \
do{ assert(!polar && !parametric); \
 if (log_array[AXIS]) {\
  if (min_array[AXIS] <= 0.0 || max_array[AXIS] <= 0.0)\
   int_error(NO_CARET, "x/x2 range must be greater than 0 for log scale!");\
  t_min = log(min_array[AXIS])/log_base_array[AXIS]; t_max = log(max_array[AXIS])/log_base_array[AXIS];\
 } else {\
  t_min = min_array[AXIS]; t_max = max_array[AXIS];\
 }\
 t_step = (t_max - t_min) / (samples - 1); \
}while(0)

	if (parametric || polar) {
	    t_min = min_array[T_AXIS];
	    t_max = max_array[T_AXIS];
	    t_step = (t_max - t_min) / (samples - 1);
	}
	/* else we'll do it on each plot */

	tp_ptr = &(first_plot);
	plot_num = 0;
	this_plot = first_plot;
	c_token = begin_token;	/* start over */

	/* Read through functions */
	while (TRUE) {
	    if (is_definition(c_token)) {
		define();
	    } else {
		int x_axis = this_plot->x_axis;
		int y_axis = this_plot->y_axis;

		plot_num++;
		if (!isstring(c_token)) {	/* function to plot */
		    if (parametric) {	/* toggle parametric axes */
			xparam = 1 - xparam;
		    }
		    dummy_func = &plot_func;
		    plot_func.at = temp_at();	/* reparse function */

		    if (!parametric && !polar) {
			SET_DUMMY_RANGE(x_axis);
		    }
		    for (i = 0; i < samples; i++) {
			double temp;
			struct value a;
			double t = t_min + i * t_step;
			/* parametric/polar => NOT a log quantity */
			double x = (!parametric && !polar &&
				    log_array[x_axis]) ? pow(base_array[x_axis], t) : t;

			(void) Gcomplex(&plot_func.dummy_values[0], x, 0.0);
			evaluate_at(plot_func.at, &a);

			if (undefined || (fabs(imag(&a)) > zero)) {
			    this_plot->points[i].type = UNDEFINED;
			    continue;
			}
			temp = real(&a);

			this_plot->points[i].z = -1.0;	/* width of box not specified */
			this_plot->points[i].type = INRANGE;	/* for the moment */

			if (parametric) {
			    /* we cannot do range-checking now, since for
			     * the x function we did not know which axes
			     * we were using
			     * DO NOT TAKE LOGS YET - do it in parametric_fixup
			     */
			    this_plot->points[i].x = t;		/* ignored, actually... */
			    this_plot->points[i].y = temp;
			} else if (polar) {
			    double y;
			    if (!(autoscale_r & 2) && temp > rmax)
				this_plot->points[i].type = OUTRANGE;
			    if (!(autoscale_r & 1))
				temp -= rmin;
			    y = temp * sin(x * ang2rad);
			    x = temp * cos(x * ang2rad);
			    temp = y;
			    STORE_WITH_LOG_AND_FIXUP_RANGE(this_plot->points[i].x, x, this_plot->points[i].type,
			      x_axis, NOOP, goto come_here_if_undefined);
			    STORE_WITH_LOG_AND_FIXUP_RANGE(this_plot->points[i].y, y, this_plot->points[i].type,
			      y_axis, NOOP, goto come_here_if_undefined);
			} else {	/* neither parametric or polar */
			    /* If non-para, it must be INRANGE */
			    this_plot->points[i].x = t;		/* logscale ? log(x) : x */

			    STORE_WITH_LOG_AND_FIXUP_RANGE(this_plot->points[i].y, temp, this_plot->points[i].type,
							   y_axis + (x_axis - y_axis) * xparam, NOOP, goto come_here_if_undefined);

			  come_here_if_undefined:	/* could not use a continue in this case */
			    ;	/* ansi requires a statement after a label */
			}

		    }		/* loop over samples */
		    this_plot->p_count = i;	/* samples */
		}
		c_token = this_plot->token;	/* skip all modifers func / whole of data plots */

		tp_ptr = &(this_plot->next_cp);		/* used below */
		this_plot = this_plot->next_cp;
	    }

	    if (equals(c_token, ","))
		c_token++;
	    else
		break;
	}
	/* when step debugging, set breakpoint here to get through
	 * the 'read function' loop above quickly */
	if (parametric) {
	    /* Now actually fix the plot pairs to be single plots */
	    /* also fixes up polar&&parametric fn plots */
	    parametric_fixup(first_plot, &plot_num);
	    /* we omitted earlier check for range too small */
	    fixup_range(FIRST_X_AXIS, "x");
	    if (uses_axis[SECOND_X_AXIS]) {
		fixup_range(SECOND_X_AXIS, "x2");
	    }
	}
    }				/* some_functions */
    /* throw out all curve_points at end of list, that we don't need  */
    cp_free(*tp_ptr);
    *tp_ptr = NULL;


    /* if first_plot is NULL, we have no functions or data at all. This can
       happen, if you type "plot x=5", since x=5 is a variable assignment */

    if (plot_num == 0 || first_plot == NULL) {
	int_error(c_token, "no functions or data to plot");
    }
    if (uses_axis[FIRST_X_AXIS]) {
	if (max_array[FIRST_X_AXIS] == -VERYLARGE ||
	    min_array[FIRST_X_AXIS] == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	FIXUP_RANGE_FOR_LOG(FIRST_X_AXIS, x);
    }
    if (uses_axis[SECOND_X_AXIS]) {
	if (max_array[SECOND_X_AXIS] == -VERYLARGE ||
	    min_array[SECOND_X_AXIS] == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	FIXUP_RANGE_FOR_LOG(SECOND_X_AXIS, x2);
    } else {
	assert(uses_axis[FIRST_X_AXIS]);
	if (auto_array[SECOND_X_AXIS] & 1)
	    min_array[SECOND_X_AXIS] = min_array[FIRST_X_AXIS];
	if (auto_array[SECOND_X_AXIS] & 2)
	    max_array[SECOND_X_AXIS] = max_array[FIRST_X_AXIS];
    }

    if (!uses_axis[FIRST_X_AXIS]) {
	assert(uses_axis[SECOND_X_AXIS]);
	if (auto_array[FIRST_X_AXIS] & 1)
	    min_array[FIRST_X_AXIS] = min_array[SECOND_X_AXIS];
	if (auto_array[FIRST_X_AXIS] & 2)
	    max_array[FIRST_X_AXIS] = max_array[SECOND_X_AXIS];
    }
    if (uses_axis[FIRST_Y_AXIS]) {
	if (max_array[FIRST_Y_AXIS] == -VERYLARGE ||
	    min_array[FIRST_Y_AXIS] == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	fixup_range(FIRST_Y_AXIS, "y");
	FIXUP_RANGE_FOR_LOG(FIRST_Y_AXIS, y);
    }				/* else we want to copy y2 range, but need to fix it up first */
    if (uses_axis[SECOND_Y_AXIS]) {
	if (max_array[SECOND_Y_AXIS] == -VERYLARGE ||
	    min_array[SECOND_Y_AXIS] == VERYLARGE)
	    int_error(NO_CARET, "all points undefined!");
	fixup_range(SECOND_Y_AXIS, "y2");
	FIXUP_RANGE_FOR_LOG(SECOND_Y_AXIS, y2);
    } else {
	assert(uses_axis[FIRST_Y_AXIS]);
	if (auto_array[SECOND_Y_AXIS] & 1)
	    min_array[SECOND_Y_AXIS] = min_array[FIRST_Y_AXIS];
	if (auto_array[SECOND_Y_AXIS] & 2)
	    max_array[SECOND_Y_AXIS] = max_array[FIRST_Y_AXIS];
	FIXUP_RANGE_FOR_LOG(SECOND_Y_AXIS, y2);
    }

    if (!uses_axis[FIRST_Y_AXIS]) {
	assert(uses_axis[SECOND_Y_AXIS]);
	if (auto_array[FIRST_Y_AXIS] & 1)
	    min_array[FIRST_Y_AXIS] = min_array[SECOND_Y_AXIS];
	if (auto_array[FIRST_Y_AXIS] & 2)
	    max_array[FIRST_Y_AXIS] = max_array[SECOND_Y_AXIS];
    }
#define WRITEBACK(axis,min,max) \
if(range_flags[axis]&RANGE_WRITEBACK) \
  {if (auto_array[axis]&1) min = min_array[axis]; \
   if (auto_array[axis]&2) max = max_array[axis]; \
  }

    WRITEBACK(FIRST_X_AXIS, xmin, xmax)
    WRITEBACK(FIRST_Y_AXIS, ymin, ymax)
    WRITEBACK(SECOND_X_AXIS, x2min, x2max)
    WRITEBACK(SECOND_Y_AXIS, y2min, y2max)
    if (strcmp(term->name, "table") == 0)
	print_table(first_plot, plot_num);
    else {
	START_LEAK_CHECK();	/* check for memory leaks in this routine */

	/* do_plot now uses max_array[], etc */
	do_plot(first_plot, plot_num);

	END_LEAK_CHECK();
    }

    /* if we get here, all went well, so record this line for replot */

    if (plot_token != -1) {
	/* note that m_capture also frees the old replot_line */
	m_capture(&replot_line, plot_token, c_token - 1);
	plot_token = -1;
    }
    cp_free(first_plot);
    first_plot = NULL;
}				/* eval_plots */




static void parametric_fixup(start_plot, plot_num)
struct curve_points *start_plot;
int *plot_num;
/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are garanteed to occur in (x,y) pairs while preserving the
 * non-FUNC type plots intact.  This means we have to work our way through
 * various lists.  Examples (hand checked): start_plot:F1->F2->NULL ==>
 * F2->NULL start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F2->F4->F6->NULL
 * start_plot:F1->F2->D1->D2->F3->F4->D3->NULL ==> F2->D1->D2->F4->D3->NULL
 * 
 */
{
    struct curve_points *xp, *new_list = NULL, *free_list = NULL;
    struct curve_points **last_pointer = &new_list;
    int i, tlen, curve;
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
	    struct curve_points *yp = xp->next_cp;

	    --(*plot_num);

	    assert(xp->p_count == yp->p_count);

	    /* because syntax is   plot x(t), y(t) axes ..., only
	     * the y function axes are correct
	     */


	    /*
	     * Go through all the points assigning the y's from xp to be the x's
	     * for yp. In polar mode, we need to check max's and min's as we go.
	     */

	    for (i = 0; i < yp->p_count; ++i) {
		if (polar) {
		    double r = yp->points[i].y;
		    double t = xp->points[i].y * ang2rad;
		    double x, y;
		    if (!(autoscale_r & 2) && r > rmax)
			yp->points[i].type = OUTRANGE;
		    if (!(autoscale_r & 1))
			r -= rmin;	/* store internally as if plotting r(t)-rmin */
		    x = r * cos(t);
		    y = r * sin(t);
		    /* we hadn't done logs when we stored earlier */
		    STORE_WITH_LOG_AND_FIXUP_RANGE(yp->points[i].x, x, yp->points[i].type,
						 xp->x_axis, NOOP, NOOP);
		    STORE_WITH_LOG_AND_FIXUP_RANGE(yp->points[i].y, y, yp->points[i].type,
						 xp->y_axis, NOOP, NOOP);
		} else {
		    double x = xp->points[i].y;
		    double y = yp->points[i].y;
		    STORE_WITH_LOG_AND_FIXUP_RANGE(yp->points[i].x, x,
			     yp->points[i].type, yp->x_axis, NOOP, NOOP);
		    STORE_WITH_LOG_AND_FIXUP_RANGE(yp->points[i].y, y,
			     yp->points[i].type, yp->y_axis, NOOP, NOOP);
		}
	    }

	    /* Ok, fix up the title to include both the xp and yp plots. */
	    if (xp->title && xp->title[0] != '\0' && yp->title) {
		tlen = strlen(yp->title) + strlen(xp->title) + 3;
		new_title = gp_alloc((unsigned long) tlen, "string");
		strcpy(new_title, xp->title);
		strcat(new_title, ", ");	/* + 2 */
		strcat(new_title, yp->title);	/* + 1 = + 3 */
		free(yp->title);
		yp->title = new_title;
	    }
	    /* move xp to head of free list */
	    xp->next_cp = free_list;
	    free_list = xp;

	    /* append yp to new_list */
	    *last_pointer = yp;
	    last_pointer = &(yp->next_cp);
	    xp = yp->next_cp;

	} else {		/* data plot */
	    assert(*last_pointer == xp);
	    last_pointer = &(xp->next_cp);
	    xp = xp->next_cp;
	}
    }				/* loop over plots */

    first_plot = new_list;

    /* Ok, stick the free list at the end of the curve_points plot list. */
    *last_pointer = free_list;
}
