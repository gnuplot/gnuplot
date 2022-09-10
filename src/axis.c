/* GNUPLOT - axis.c */

/*[
 * Copyright 2000, 2004   Thomas Williams, Colin Kelley
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

#include "axis.h"

#include "stdfn.h"

#include "alloc.h"
#include "command.h"
#include "gadgets.h"
#include "gp_time.h"
#include "term_api.h"
#include "variable.h"

/* HBB 20000725: gather all per-axis variables into a struct, and set
 * up a single large array of such structs. Next step might be to use
 * isolated AXIS structs, instead of an array.
 * EAM 2013: tried that.  The problem is that all the routines and macros
 * that manipulate axis data take an index, not a pointer.  We'd have to
 * rewrite all of them and it just didn't seem worth it.
 * Instead I've added additional non-standard entries on the end, used for
 * parallel axis plots if nothing else.
 * Note: This array is now initialized in reset_command().
 */
AXIS axis_array[AXIS_ARRAY_SIZE];
AXIS *shadow_axis_array;		/* Only if nonlinear axes are in use */

/* Keep defaults varying by axis in their own array, to ease initialization
 * of the main array */
const AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE] = {
    { -10, 10, "z" , TICS_ON_BORDER,               },
    { -10, 10, "y" , TICS_ON_BORDER | TICS_MIRROR, },
    { -10, 10, "x" , TICS_ON_BORDER | TICS_MIRROR, },
    { -10, 10, "cb", TICS_ON_BORDER | TICS_MIRROR, },
    { -10, 10, " z2", NO_TICS,                     },
    { -10, 10, "y2", NO_TICS,                      },
    { -10, 10, "x2", NO_TICS,                      },
    { - 0, 10, "r" , TICS_ON_AXIS,                 },
    { - 5,  5, "t" , NO_TICS,                      },
    { - 5,  5, "u" , NO_TICS,                      },
    { - 5,  5, "v" , NO_TICS,                      }
};

const AXIS default_axis_state = DEFAULT_AXIS_STRUCT;

/* Parallel axis structures are held in an array that is dynamically
 * allocated on demand.
 */
AXIS *parallel_axis_array = NULL;
int num_parallel_axes = 0;

/* Separate axis THETA for tics around perimeter of polar grid
 * Initialize to blank rather than DEFAULT_AXIS_STRUCT because
 * it is too different from "real" axes for the defaults to make sense.
 * The fields we do need are initialized in unset_polar().
 */
AXIS THETA_AXIS = {0};

/* HBB 20000506 new variable: parsing table for use with the table
 * module, to help generalizing set/show/unset/save, where possible */
const struct gen_table axisname_tbl[] =
{
    { "z", FIRST_Z_AXIS},
    { "y", FIRST_Y_AXIS},
    { "x", FIRST_X_AXIS},
    { "cb",COLOR_AXIS},
    { " z2",SECOND_Z_AXIS},
    { "y2",SECOND_Y_AXIS},
    { "x2",SECOND_X_AXIS},
    { "r", POLAR_AXIS},
    { "t", T_AXIS},
    { "u", U_AXIS},
    { "v", V_AXIS},
    { NULL, -1}
};


/* penalty for doing tics by callback in gen_tics is need for global
 * variables to communicate with the tic routines. Dont need to be
 * arrays for this */
/* HBB 20000416: they may not need to be array[]ed, but it'd sure
 * make coding easier, in some places... */
/* HBB 20000416: for the testing, these are global... */
int tic_start, tic_direction, tic_text,
    rotate_tics, tic_hjust, tic_vjust, tic_mirror;

/* These are declare volatile in order to fool the compiler into not */
/* optimizing out intermediate values, thus hiding loss of precision.*/
volatile double vol_this_tic;
volatile double vol_previous_tic;

const struct ticdef default_axis_ticdef = DEFAULT_AXIS_TICDEF;

/* Tic scale for tics with level > 1.  0 means 'inherit minitics scale'  */
double ticscale[MAX_TICLEVEL] = {1,0.5,1,1,1};

/* global default time format */
char *timefmt = NULL;

/* axis labels */
const text_label default_axis_label = EMPTY_LABELSTRUCT;

/* zeroaxis drawing */
const lp_style_type default_axis_zeroaxis = DEFAULT_AXIS_ZEROAXIS;

/* grid drawing */
/* int grid_selection = GRID_OFF; */
#define DEFAULT_GRID_LP {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0, 0.5, 0.0, DEFAULT_P_CHAR, {TC_LT, LT_AXIS, 0.0}, DEFAULT_DASHPATTERN}
const struct lp_style_type default_grid_lp = DEFAULT_GRID_LP;
struct lp_style_type grid_lp   = DEFAULT_GRID_LP;
struct lp_style_type mgrid_lp  = DEFAULT_GRID_LP;
int grid_layer = LAYER_BEHIND;
TBOOLEAN grid_tics_in_front = FALSE;
TBOOLEAN grid_vertical_lines = FALSE;
TBOOLEAN grid_spiderweb = FALSE;
double polar_grid_angle = 0;	/* nonzero means a polar grid */
TBOOLEAN raxis = FALSE;
double theta_origin = 0.0;	/* default origin at right side */
double theta_direction = 1;	/* counterclockwise from origin */

/* Length of the longest tics label, set by widest_tic_callback(): */
int widest_tic_strlen;

/* flag to indicate that in-line axis ranges should be ignored */
TBOOLEAN inside_zoom;

/* axes being used by the current plot */
/* These are mainly convenience variables, replacing separate copies of
 * such variables originally found in the 2D and 3D plotting code */
AXIS_INDEX x_axis = FIRST_X_AXIS;
AXIS_INDEX y_axis = FIRST_Y_AXIS;
AXIS_INDEX z_axis = FIRST_Z_AXIS;

/* Only accessed by save_autoscaled_ranges() and restore_autoscaled_ranges() */
static double save_autoscaled_xmin;
static double save_autoscaled_xmax;
static double save_autoscaled_ymin;
static double save_autoscaled_ymax;

/* --------- internal prototypes ------------------------- */
static double make_auto_time_minitics(t_timelevel, double);
static double make_tics(struct axis *, int);
static double quantize_time_tics(struct axis *, double, double, int);
static double time_tic_just(t_timelevel, double);
static double round_outward(struct axis *, TBOOLEAN, double);
static TBOOLEAN axis_position_zeroaxis(AXIS_INDEX);
static void load_one_range(struct axis *axis, double *a, t_autoscale *autoscale, t_autoscale which );
static double quantize_duodecimal_tics(double, int);
static void get_position_type(enum position_type * type, AXIS_INDEX *axes);

/* ---------------------- routines ----------------------- */

void
check_log_limits(struct axis *axis, double min, double max)
{
    if (axis->log) {
	if (min<= 0.0 || max <= 0.0)
	    int_error(NO_CARET,
		      "%s range must be greater than 0 for log scale",
		      axis_name(axis->index));
    }
}


/* {{{ axis_invert_if_requested() */

void
axis_invert_if_requested(struct axis *axis)
{
    if (((axis->range_flags & RANGE_IS_REVERSED)) &&  (axis->autoscale != 0))
	/* NB: The whole point of this is that we want max < min !!! */
	reorder_if_necessary(axis->max, axis->min);
}


void
axis_init(AXIS *this_axis, TBOOLEAN reset_autoscale)
{
    this_axis->autoscale = this_axis->set_autoscale;
    this_axis->min = (reset_autoscale && (this_axis->set_autoscale & AUTOSCALE_MIN))
	? VERYLARGE : this_axis->set_min;
    this_axis->max = (reset_autoscale && (this_axis->set_autoscale & AUTOSCALE_MAX))
	? -VERYLARGE : this_axis->set_max;
    this_axis->data_min = VERYLARGE;
    this_axis->data_max = -VERYLARGE;
}

void
axis_check_range(AXIS_INDEX axis)
{
    axis_invert_if_requested(&axis_array[axis]);
    check_log_limits(&axis_array[axis], axis_array[axis].min, axis_array[axis].max);
}

/* {{{ axis_log_value_checked() */
double
axis_log_value_checked(AXIS_INDEX axis, double coord, const char *what)
{
    if (axis_array[axis].log && !(coord > 0.0))
	int_error(NO_CARET, "%s has %s coord of %g; must be above 0 for log scale!",
		    what, axis_name(axis), coord);
    return coord;
}

/* }}} */

char *
axis_name(AXIS_INDEX axis)
{
    static char name[] = "primary 00 ";
    if (axis == THETA_index)
	return "t";

    if (axis >= PARALLEL_AXES) {
	sprintf(name, "paxis %d ", (axis-PARALLEL_AXES+1) & 0xff);
	return name;
    }
    if (axis < 0) {
	sprintf(name, "primary %2s", axis_defaults[-axis].name);
	return name;
    }
    return (char *) axis_defaults[axis].name;
}

void
init_sample_range(AXIS *axis, enum PLOT_TYPE plot_type)
{
    axis_array[SAMPLE_AXIS].range_flags = 0;
    axis_array[SAMPLE_AXIS].min = axis->min;
    axis_array[SAMPLE_AXIS].max = axis->max;
    axis_array[SAMPLE_AXIS].set_min = axis->set_min;
    axis_array[SAMPLE_AXIS].set_max = axis->set_max;
    axis_array[SAMPLE_AXIS].datatype = axis->datatype;
    /* Functions are sampled along the x or y plot axis */
    /* Data is drawn from pseudofile '+', assumed to be linear */
    /* NB:  link_udf MUST NEVER BE FREED as it is only a copy */
    if (plot_type == FUNC) {
	axis_array[SAMPLE_AXIS].linked_to_primary = axis->linked_to_primary;
	axis_array[SAMPLE_AXIS].link_udf = axis->link_udf;
    }
}

/*
 * Fill in the starting values for a just-allocated  parallel axis structure
 */
void
init_parallel_axis(AXIS *this_axis, AXIS_INDEX index)
{
    memcpy(this_axis, &default_axis_state, sizeof(AXIS));
    this_axis->formatstring = gp_strdup(DEF_FORMAT);
    this_axis->index = index + PARALLEL_AXES;
    this_axis->ticdef.rangelimited = TRUE;
    this_axis->set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
    axis_init(this_axis, TRUE);
}

/*
 * If we encounter a parallel axis index higher than any used so far,
 * extend parallel_axis[] to hold the corresponding data.
 * Returns pointer to the new axis.
 */
void
extend_parallel_axis(int paxis)
{
    int i;
    if (paxis > num_parallel_axes) {
	parallel_axis_array = gp_realloc(parallel_axis_array, paxis * sizeof(AXIS),
					"extend parallel_axes");
	for (i = num_parallel_axes; i < paxis; i++)
	    init_parallel_axis( &parallel_axis_array[i], i );
	num_parallel_axes = paxis;
    }
}

/*
 * Most of the crashes found during fuzz-testing of version 5.1 were a
 * consequence of an axis range being corrupted, i.e. NaN or Inf.
 * Corruption became easier with the introduction of nonlinear axes,
 * but even apart from that autoscaling bad data could cause a fault.
 * NB: Some platforms may need help with isnan() and isinf().
 */
TBOOLEAN
bad_axis_range(struct axis *axis)
{
    if (isnan(axis->min) || isnan(axis->max))
	return TRUE;
#ifdef isinf
    if (isinf(axis->min) || isinf(axis->max))
	return TRUE;
#endif
    if (axis->max == -VERYLARGE || axis->min == VERYLARGE)
	return TRUE;
    return FALSE;
}

/* {{{ axis_checked_extend_empty_range() */
/*
 * === SYNOPSIS ===
 *
 * This function checks whether the data and/or plot range in a given axis
 * is too small (which would cause divide-by-zero and/or infinite-loop
 * problems later on).  If so,
 * - if autoscaling is in effect for this axis, we widen the range
 * - otherwise, we abort with a call to  int_error()  (which prints out
 *   a suitable error message, then (hopefully) aborts this command and
 *   returns to the command prompt or whatever).
 *
 *
 * === HISTORY AND DESIGN NOTES ===
 *
 * 1998 Oct 4, Jonathan Thornburg <jthorn@galileo.thp.univie.ac.at>
 *
 * This function used to be a (long) macro  FIXUP_RANGE(AXIS, WHICH)
 * which was (identically!) defined in  plot2d.c  and  plot3d.c .  As
 * well as now being a function instead of a macro, the logic is also
 * changed:  The "too small" range test no longer depends on 'set zero'
 * and is now properly scaled relative to the data magnitude.
 *
 * The key question in designing this function is the policy for just how
 * much to widen the data range by, as a function of the data magnitude.
 * This is to some extent a matter of taste.  IMHO the key criterion is
 * that (at least) all of the following should (a) not infinite-loop, and
 * (b) give correct plots, regardless of the 'set zero' setting:
 *      plot 6.02e23            # a huge number >> 1 / FP roundoff level
 *      plot 3                  # a "reasonable-sized" number
 *      plot 1.23e-12           # a small number still > FP roundoff level
 *      plot 1.23e-12 * sin(x)  # a small function still > FP roundoff level
 *      plot 1.23e-45           # a tiny number << FP roundoff level
 *      plot 1.23e-45 * sin(x)  # a tiny function << FP roundoff level
 *      plot 0          # or (more commonly) a data file of all zeros
 * That is, IMHO gnuplot should *never* infinite-loop, and it should *never*
 * producing an incorrect or misleading plot.  In contrast, the old code
 * would infinite-loop on most of these examples with 'set zero 0.0' in
 * effect, or would plot the small-amplitude sine waves as the zero function
 * with 'zero' set larger than the sine waves' amplitude.
 *
 * The current code plots all the above examples correctly and without
 * infinite looping.
 *
 * HBB 2000/05/01: added an additional up-front test, active only if
 *   the new 'mesg' parameter is non-NULL.
 *
 * === USAGE ===
 *
 * Arguments:
 * axis = (in) An integer specifying which axis (x1, x2, y1, y2, z, etc)
 *             we should do our stuff for.  We use this argument as an
 *             index into the global arrays  {min,max,auto}_array .  In
 *             practice this argument will typically be one of the constants
 *              {FIRST,SECOND}_{X,Y,Z}_AXIS  defined in plot.h.
 * mesg = (in) if non-NULL, will check if the axis range is valid (min
 *             not +VERYLARGE, max not -VERYLARGE), and int_error() out
 *             if it isn't.
 *
 * Global Variables:
 * auto_array, min_array, max_array (in out) (defined in axis.[ch]):
 *    variables describing the status of autoscaling and range ends, for
 *    each of the possible axes.
 *
 * c_token = (in) (defined in plot.h) Used in formatting an error message.
 *
 */
void
axis_checked_extend_empty_range(AXIS_INDEX axis, const char *mesg)
{
    struct axis *this_axis = &axis_array[axis];

    /* These two macro definitions set the range-widening policy: */

    /* widen [0:0] by +/- this absolute amount */
#define FIXUP_RANGE__WIDEN_ZERO_ABS	1.0
    /* widen [nonzero:nonzero] by -/+ this relative amount */
#define FIXUP_RANGE__WIDEN_NONZERO_REL	0.01

    double dmin = this_axis->min;
    double dmax = this_axis->max;

    /* pass msg == NULL if for some reason you trust the axis range */
    if (mesg && bad_axis_range(this_axis))
	    int_error(c_token, mesg);

    if (dmax - dmin == 0.0) {
	/* empty range */
	if (this_axis->autoscale) {
	    /* range came from autoscaling ==> widen it */
	    double widen = (dmax == 0.0) ?
		FIXUP_RANGE__WIDEN_ZERO_ABS
		: FIXUP_RANGE__WIDEN_NONZERO_REL * fabs(dmax);
	    if (!(axis == FIRST_Z_AXIS && !mesg)) /* set view map */
		fprintf(stderr, "Warning: empty %s range [%g:%g], ",
		    axis_name(axis), dmin, dmax);
	    /* HBB 20010525: correctly handle single-ended
	     * autoscaling, too: */
	    if (this_axis->autoscale & AUTOSCALE_MIN)
		this_axis->min -= widen;
	    if (this_axis->autoscale & AUTOSCALE_MAX)
		this_axis->max += widen;
	    if (!(axis == FIRST_Z_AXIS && !mesg)) /* set view map */
		fprintf(stderr, "adjusting to [%g:%g]\n",
		    this_axis->min, this_axis->max);
	} else {
	    /* user has explicitly set the range (to something empty) */
	    int_error(NO_CARET, "Can't plot with an empty %s range!",
		      axis_name(axis));
	}
    }
}

/* Simpler alternative routine for nonlinear axes (including log scale) */
void
axis_check_empty_nonlinear(struct axis *axis)
{
    /* Poorly defined via/inv nonlinear mappings can leave NaN in derived range */
    if (bad_axis_range(axis))
	goto undefined_axis_range_error;

    axis = axis->linked_to_primary;
    if (bad_axis_range(axis))
	goto undefined_axis_range_error;

    return;
    undefined_axis_range_error:
	int_error(NO_CARET,"empty or undefined %s axis range", axis_name(axis->index));
    return;
}

/* }}} */

/* {{{ make smalltics for time-axis */
static double
make_auto_time_minitics(t_timelevel tlevel, double incr)
{
    double tinc = 0.0;

    if ((int)tlevel < TIMELEVEL_SECONDS)
	tlevel = TIMELEVEL_SECONDS;
    switch (tlevel) {
    case TIMELEVEL_SECONDS:
    case TIMELEVEL_MINUTES:
	if (incr >= 5)
	    tinc = 1;
	if (incr >= 10)
	    tinc = 5;
	if (incr >= 20)
	    tinc = 10;
	if (incr >= 60)
	    tinc = 20;
	if (incr >= 2 * 60)
	    tinc = 60;
	if (incr >= 6 * 60)
	    tinc = 2 * 60;
	if (incr >= 12 * 60)
	    tinc = 3 * 60;
	if (incr >= 24 * 60)
	    tinc = 6 * 60;
	break;
    case TIMELEVEL_HOURS:
	if (incr >= 20 * 60)
	    tinc = 10 * 60;
	if (incr >= 3600)
	    tinc = 30 * 60;
	if (incr >= 2 * 3600)
	    tinc = 3600;
	if (incr >= 6 * 3600)
	    tinc = 2 * 3600;
	if (incr >= 12 * 3600)
	    tinc = 3 * 3600;
	if (incr >= 24 * 3600)
	    tinc = 6 * 3600;
	break;
    case TIMELEVEL_DAYS:
	if (incr > 2 * 3600)
	    tinc = 3600;
	if (incr > 4 * 3600)
	    tinc = 2 * 3600;
	if (incr > 7 * 3600)
	    tinc = 3 * 3600;
	if (incr > 13 * 3600)
	    tinc = 6 * 3600;
	if (incr > DAY_SEC)
	    tinc = 12 * 3600;
	if (incr > 2 * DAY_SEC)
	    tinc = DAY_SEC;
	break;
    case TIMELEVEL_WEEKS:
	if (incr > 2 * DAY_SEC)
	    tinc = DAY_SEC;
	if (incr > 7 * DAY_SEC)
	    tinc = 7 * DAY_SEC;
	break;
    case TIMELEVEL_MONTHS:
	if (incr > 2 * DAY_SEC)
	    tinc = DAY_SEC;
	if (incr > 15 * DAY_SEC)
	    tinc = 10 * DAY_SEC;
	if (incr > 2 * MON_SEC)
	    tinc = MON_SEC;
	if (incr > 6 * MON_SEC)
	    tinc = 3 * MON_SEC;
	if (incr > 2 * YEAR_SEC)
	    tinc = YEAR_SEC;
	break;
    case TIMELEVEL_YEARS:
	if (incr > 2 * MON_SEC)
	    tinc = MON_SEC;
	if (incr > 6 * MON_SEC)
	    tinc = 3 * MON_SEC;
	if (incr > 2 * YEAR_SEC)
	    tinc = YEAR_SEC;
	if (incr > 10 * YEAR_SEC)
	    tinc = 5 * YEAR_SEC;
	if (incr > 50 * YEAR_SEC)
	    tinc = 10 * YEAR_SEC;
	if (incr > 100 * YEAR_SEC)
	    tinc = 20 * YEAR_SEC;
	if (incr > 200 * YEAR_SEC)
	    tinc = 50 * YEAR_SEC;
	if (incr > 300 * YEAR_SEC)
	    tinc = 100 * YEAR_SEC;
	break;
    }
    return (tinc);
}
/* }}} */

/* {{{ copy_or_invent_formatstring() */
/* Rarely called helper function looks_like_numeric() */
int
looks_like_numeric(char *format)
{
    if (!(format = strchr(format, '%')))
	return 0;

    while (++format && (*format == ' '
			|| *format == '-'
			|| *format == '+'
			|| *format == '#'))
	;			/* do nothing */

    while (isdigit((unsigned char) *format) || *format == '.')
	++format;

    return (*format == 'e' || *format == 'f' || *format == 'g' || *format == 'h');
}


/* Either copies axis->formatstring to axis->ticfmt, or
 * in case that's not applicable because the format hasn't been
 * specified correctly, invents a time/date output format by looking
 * at the range of values.  Considers time/date fields that don't
 * change across the range to be unimportant */
char *
copy_or_invent_formatstring(struct axis *this_axis)
{
    struct tm t_min, t_max;

    char tempfmt[MAX_ID_LEN+1];
    memset(tempfmt, 0, sizeof(tempfmt));

    if (this_axis->tictype != DT_TIMEDATE
    ||  !looks_like_numeric(this_axis->formatstring)) {
	/* The simple case: formatstring is usable, so use it! */
	strncpy(tempfmt, this_axis->formatstring, MAX_ID_LEN);
	/* Ensure enough precision to distinguish tics */
	if (!strcmp(tempfmt, DEF_FORMAT)) {
	    double axmin = this_axis->min;
	    double axmax = this_axis->max;
	    int precision = ceil(-log10(GPMIN(fabs(axmax-axmin),fabs(axmin))));
	    /* FIXME: Does horrible things for large value of precision */
	    /* FIXME: Didn't I have a better patch for this? */
	    if ((axmin*axmax > 0) && 4 < precision && precision < 10)
		sprintf(tempfmt, "%%.%df", precision);
	}

	free(this_axis->ticfmt);
	this_axis->ticfmt = strdup(tempfmt);
	return this_axis->ticfmt;
    }

    /* Else, have to invent an output format string. */

    ggmtime(&t_min, time_tic_just(this_axis->timelevel, this_axis->min));
    ggmtime(&t_max, time_tic_just(this_axis->timelevel, this_axis->max));

    if (t_max.tm_year == t_min.tm_year
	&& t_max.tm_yday == t_min.tm_yday) {
	/* same day, skip date */
	if (t_max.tm_hour != t_min.tm_hour) {
	    strcpy(tempfmt, "%H");
	}
	if (this_axis->timelevel < TIMELEVEL_DAYS) {
	    if (tempfmt[0])
		strcat(tempfmt, ":");
	    strcat(tempfmt, "%M");
	}
	if (this_axis->timelevel < TIMELEVEL_HOURS) {
	    strcat(tempfmt, ":%S");
	}
    } else {
	if (t_max.tm_year != t_min.tm_year) {
	    /* different years, include year in ticlabel */
	    /* check convention, day/month or month/day */
	    if (strchr(timefmt, 'm') < strchr(timefmt, 'd')) {
		strcpy(tempfmt, "%m/%d/%");
	    } else {
		strcpy(tempfmt, "%d/%m/%");
	    }
	    if (((int) (t_max.tm_year / 100)) != ((int) (t_min.tm_year / 100))) {
		strcat(tempfmt, "Y");
	    } else {
		strcat(tempfmt, "y");
	    }

	} else {
	    /* Copy day/month order over from input format */
	    if (strchr(timefmt, 'm') < strchr(timefmt, 'd')) {
		strcpy(tempfmt, "%m/%d");
	    } else {
		strcpy(tempfmt, "%d/%m");
	    }
	}
	if (this_axis->timelevel < TIMELEVEL_WEEKS) {
	    /* Note: seconds can't be useful if there's more than 1
	     * day's worth of data... */
	    strcat(tempfmt, "\n%H:%M");
	}
    }

    free(this_axis->ticfmt);
    this_axis->ticfmt = strdup(tempfmt);
    return this_axis->ticfmt;
}

/* }}} */

/* {{{ quantize_normal_tics() */
/* the guide parameter was intended to allow the number of tics
 * to depend on the relative sizes of the plot and the font.
 * It is the approximate upper limit on number of tics allowed.
 * But it did not go down well with the users.
 * A value of 20 gives the same behaviour as 3.5, so that is
 * hardwired into the calls to here. Maybe we will restore it
 * to the automatic calculation one day
 */

/* HBB 20020220: Changed to use value itself as first argument, not
 * log10(value).  Done to allow changing the calculation method
 * to avoid numerical problems */
double
quantize_normal_tics(double arg, int guide)
{
    /* order of magnitude of argument: */
    double power = pow(10.0, floor(log10(arg)));
    double xnorm = arg / power;	/* approx number of decades */
    /* we expect 1 <= xnorm <= 10 */
    double posns = guide / xnorm; /* approx number of tic posns per decade */
    /* with guide=20, we expect 2 <= posns <= 20 */
    double tics;

    /* HBB 20020220: Looking at these, I would normally expect
     * to see posns*tics to be always about the same size. But we
     * rather suddenly drop from 2.0 to 1.0 at tic step 0.5. Why? */
    /* JRV 20021117: fixed this by changing next to last threshold
       from 1 to 2.  However, with guide=20, this doesn't matter. */
    if (posns > 40)
	tics = 0.05;		/* eg 0, .05, .10, ... */
    else if (posns > 20)
	tics = 0.1;		/* eg 0, .1, .2, ... */
    else if (posns > 10)
	tics = 0.2;		/* eg 0,0.2,0.4,... */
    else if (posns > 4)
	tics = 0.5;		/* 0,0.5,1, */
    else if (posns > 2)
	tics = 1;		/* 0,1,2,.... */
    else if (posns > 0.5)
	tics = 2;		/* 0, 2, 4, 6 */
    else
	/* getting desperate... the ceil is to make sure we
	 * go over rather than under - eg plot [-10:10] x*x
	 * gives a range of about 99.999 - tics=xnorm gives
	 * tics at 0, 99.99 and 109.98  - BAD !
	 * This way, inaccuracy the other way will round
	 * up (eg 0->100.0001 => tics at 0 and 101
	 * I think latter is better than former
	 */
	tics = ceil(xnorm);

    return (tics * power);
}

/* }}} */

/* {{{ make_tics() */
/* Implement TIC_COMPUTED case, i.e. automatically choose a usable
 * ticking interval for the given axis. For the meaning of the guide
 * parameter, see the comment on quantize_normal_tics() */
static double
make_tics(struct axis *this_axis, int guide)
{
    double xr, tic;

    xr = fabs(this_axis->min - this_axis->max);
    if (xr == 0)
	return 1;	/* Anything will do, since we'll never use it */
    if (xr >= VERYLARGE) {
	int_warn(NO_CARET, "%s axis range undefined or overflow, resetting to [0:0]",
	    axis_name(this_axis->index));
	/* FIXME: this used to be int_error but there were false positives
	 * (bad range on unused axis).  However letting +/-VERYLARGE through
	 * can overrun data structures for time conversions. min = max avoids this.
	 */
	this_axis->min = this_axis->max = 0;
    }

    tic = quantize_normal_tics(xr, guide);
    if (this_axis->log && tic < 1.0)
	tic = 1.0;
    if (this_axis->tictype == DT_TIMEDATE)
	tic = quantize_time_tics(this_axis, tic, xr, guide);

    return tic;
}
/* }}} */

/* {{{ quantize_duodecimal_tics */
/* HBB 20020220: New function, to be used to properly tic axes with a
 * duodecimal reference, as used in times (60 seconds, 60 minutes, 24
 * hours, 12 months). Derived from quantize_normal_tics(). The default
 * guide is assumed to be 12, here, not 20 */
static double
quantize_duodecimal_tics(double arg, int guide)
{
    /* order of magnitude of argument: */
    double power = pow(12.0, floor(log(arg)/log(12.0)));
    double xnorm = arg / power;	/* approx number of decades */
    double posns = guide / xnorm; /* approx number of tic posns per decade */

    if (posns > 24)
	return power / 24;	/* half a smaller unit --- shouldn't happen */
    else if (posns > 12)
	return power / 12;	/* one smaller unit */
    else if (posns > 6)
	return power / 6;	/* 2 smaller units = one-6th of a unit */
    else if (posns > 4)
	return power / 4;	/* 3 smaller units = quarter unit */
    else if (posns > 2)
	return power / 2;	/* 6 smaller units = half a unit */
    else if (posns > 1)
	return power;		/* 0, 1, 2, ..., 11 */
    else if (posns > 0.5)
	return power * 2;		/* 0, 2, 4, ..., 10 */
    else if (posns > 1.0/3)
	return power * 3;		/* 0, 3, 6, 9 */
    else
	/* getting desperate... the ceil is to make sure we
	 * go over rather than under - eg plot [-10:10] x*x
	 * gives a range of about 99.999 - tics=xnorm gives
	 * tics at 0, 99.99 and 109.98  - BAD !
	 * This way, inaccuracy the other way will round
	 * up (eg 0->100.0001 => tics at 0 and 101
	 * I think latter is better than former
	 */
	return power * ceil(xnorm);
}
/* }}} */

/* {{{ quantize_time_tics */
/* Look at the tic interval given, and round it to a nice figure
 * suitable for time/data axes, i.e. a small integer number of
 * seconds, minutes, hours, days, weeks or months. As a side effec,
 * this routine also modifies the axis.timelevel to indicate
 * the units these tics are calculated in. */
static double
quantize_time_tics(struct axis *axis, double tic, double xr, int guide)
{
    int guide12 = guide * 3 / 5; /* --> 12 for default of 20 */

    axis->timelevel = TIMELEVEL_SECONDS;
    if (tic > 5) {
	/* turn tic into units of minutes */
	tic = quantize_duodecimal_tics(xr / 60.0, guide12) * 60;
	if (tic >= 60)
	    axis->timelevel = TIMELEVEL_MINUTES;
    }
    if (tic > 5 * 60) {
	/* turn tic into units of hours */
	tic = quantize_duodecimal_tics(xr / 3600.0, guide12) * 3600;
	if (tic >= 3600)
	    axis->timelevel = TIMELEVEL_HOURS;
    }
    if (tic > 3600) {
	/* turn tic into units of days */
	tic = quantize_duodecimal_tics(xr / DAY_SEC, guide12) * DAY_SEC;
	if (tic >= DAY_SEC)
	    axis->timelevel = TIMELEVEL_DAYS;
    }
    if (tic > 2 * DAY_SEC) {
	/* turn tic into units of weeks */
	tic = quantize_normal_tics(xr / WEEK_SEC, guide) * WEEK_SEC;
	if (tic < WEEK_SEC) {	/* force */
	    tic = WEEK_SEC;
	}
	if (tic >= WEEK_SEC)
	    axis->timelevel = TIMELEVEL_WEEKS;
    }
    if (tic > 3 * WEEK_SEC) {
	/* turn tic into units of month */
	tic = quantize_normal_tics(xr / MON_SEC, guide) * MON_SEC;
	if (tic < MON_SEC) {	/* force */
	    tic = MON_SEC;
	}
	if (tic >= MON_SEC)
	    axis->timelevel = TIMELEVEL_MONTHS;
    }
    if (tic > MON_SEC) {
	/* turn tic into units of years */
	tic = quantize_duodecimal_tics(xr / YEAR_SEC, guide12) * YEAR_SEC;
	if (tic >= YEAR_SEC)
	    axis->timelevel = TIMELEVEL_YEARS;
    }
    return (tic);
}

/* }}} */


/* {{{ round_outward */
/* HBB 20011204: new function (repeated code ripped out of setup_tics)
 * that rounds an axis endpoint outward. If the axis is a time/date
 * one, take care to round towards the next whole time unit, not just
 * a multiple of the (averaged) tic size */
static double
round_outward(
    struct axis *this_axis,	/* Axis to work on */
    TBOOLEAN upwards,		/* extend upwards or downwards? */
    double input)		/* the current endpoint */
{
    double tic = this_axis->ticstep;
    double result = tic * (upwards
			   ? ceil(input / tic)
			   : floor(input / tic));

    if (this_axis->tictype == DT_TIMEDATE) {
	double ontime = time_tic_just(this_axis->timelevel, result);

	/* FIXME: how certain is it that we don't want to *always*
	 * return 'ontime'? */
	if ((upwards && (ontime > result))
	||  (!upwards && (ontime < result)))
	    return ontime;
    }

    return result;
}

/* }}} */

/* {{{ setup_tics */
/* setup_tics allows max number of tics to be specified but users don't
 * like it to change with size and font, so we always call with max=20.
 * Note that if format is '', yticlin = 0, so this gives division by zero.
 */

void
setup_tics(struct axis *this, int max)
{
    double tic = 0;
    struct ticdef *ticdef = &(this->ticdef);

    /* Do we or do we not extend the axis range to the	*/
    /* next integer multiple of the ticstep?		*/
    TBOOLEAN autoextend_min = (this->autoscale & AUTOSCALE_MIN)
	&& ! (this->autoscale & AUTOSCALE_FIXMIN);
    TBOOLEAN autoextend_max = (this->autoscale & AUTOSCALE_MAX)
	&& ! (this->autoscale & AUTOSCALE_FIXMAX);
    if (this->linked_to_primary || this->linked_to_secondary)
	autoextend_min = autoextend_max = FALSE;

    /*  Apply constraints on autoscaled axis if requested:
     *  The range is _expanded_ here only.  Limiting the range is done
     *  in the macro STORE_AND_UPDATE_RANGE() of axis.h  */
    if (this->autoscale & AUTOSCALE_MIN) {
      	if (this->min_constraint & CONSTRAINT_UPPER) {
	    if (this->min > this->min_ub)
		this->min = this->min_ub;
	}
    }
    if (this->autoscale & AUTOSCALE_MAX) {
	if (this->max_constraint & CONSTRAINT_LOWER) {
	    if (this->max < this->max_lb)
		this->max = this->max_lb;
	}
    }

    /* HBB 20000506: if no tics required for this axis, do
     * nothing. This used to be done exactly before each call of
     * setup_tics, anyway... */
    if (! this->ticmode)
	return;

    if (ticdef->type == TIC_SERIES) {
	this->ticstep = tic = ticdef->def.series.incr;
	autoextend_min = autoextend_min
			 && (ticdef->def.series.start == -VERYLARGE);
	autoextend_max = autoextend_max
			 && (ticdef->def.series.end == VERYLARGE);
    } else if (ticdef->type == TIC_COMPUTED) {
	this->ticstep = tic = make_tics(this, max);
    } else {
	/* user-defined, day or month */
	autoextend_min = autoextend_max = FALSE;
    }

    /* If an explicit stepsize was set, axis->timelevel wasn't defined,
     * leading to strange misbehaviours of minor tics on time axes.
     * We used to call quantize_time_tics, but that also caused strangeness.
     */
    if (this->tictype == DT_TIMEDATE && ticdef->type == TIC_SERIES) {
	if      (tic >= 365*24*60*60.) this->timelevel = TIMELEVEL_YEARS;
	else if (tic >=  28*24*60*60.) this->timelevel = TIMELEVEL_MONTHS;
	else if (tic >=   7*24*60*60.) this->timelevel = TIMELEVEL_WEEKS;
	else if (tic >=     24*60*60.) this->timelevel = TIMELEVEL_DAYS;
	else if (tic >=        60*60.) this->timelevel = TIMELEVEL_HOURS;
	else if (tic >=           60.) this->timelevel = TIMELEVEL_MINUTES;
	else                           this->timelevel = TIMELEVEL_SECONDS;
    }

    if (autoextend_min) {
	this->min = round_outward(this, ! (this->min < this->max), this->min);
	if (this->min_constraint & CONSTRAINT_LOWER && this->min < this->min_lb)
	    this->min = this->min_lb;
    }

    if (autoextend_max) {
	this->max = round_outward(this, this->min < this->max, this->max);
	if (this->max_constraint & CONSTRAINT_UPPER && this->max > this->max_ub)
	    this->max = this->max_ub;
    }

    /* Set up ticfmt. If necessary (time axis, but not time/date output format),
     * make up a formatstring that suits the range of data */
    copy_or_invent_formatstring(this);
}

/* }}} */

/* {{{  gen_tics */
/*
 * Mar 2015: Modified to take an axis pointer rather than an index into axis_array[].
 */
void
gen_tics(struct axis *this, tic_callback callback)
{
    struct ticdef *def = & this->ticdef;
    t_minitics_status minitics = this->minitics; /* off/default/auto/explicit */

    struct lp_style_type lgrd = grid_lp;
    struct lp_style_type mgrd = mgrid_lp;

    /* gprintf uses log10() of base */
    double log10_base;
    if (this->base == 0.0)
	this->base = 10.0;
    log10_base = log10(this->base);

    if (! this->gridmajor)
	lgrd.l_type = LT_NODRAW;
    if (! this->gridminor)
	mgrd.l_type = LT_NODRAW;

    /* EAM FIXME - This really shouldn't happen, but it triggers for instance */
    /* if x2tics or y2tics are autoscaled but there is no corresponding data. */
    if (this->min >= VERYLARGE || this->max <= -VERYLARGE)
	return;

    /* user-defined tic entries
     * We place them exactly where requested.
     * Note: No minitics in this case
     */
    if (def->def.user) {
	struct ticmark *mark = def->def.user;
	double uncertain = (nonlinear(this)) ? 0 : (this->max - this->min) / 10;
	double internal_min = this->min - SIGNIF * uncertain;
	double internal_max = this->max + SIGNIF * uncertain;

	/* polar labels always +ve, and if rmin has been set, they are
	 * relative to rmin.
	 */
	if (polar && this->index == POLAR_AXIS) {
	    internal_min = X_AXIS.min - SIGNIF * uncertain;
	    internal_max = X_AXIS.max + SIGNIF * uncertain;
	}

	for (mark = def->def.user; mark; mark = mark->next) {
	    char label[MAX_ID_LEN];	/* Scratch space to construct a label */
	    char *ticlabel;		/* Points either to ^^ or to some existing text */
	    double internal;

	    /* This condition is only possible if we are in polar mode */
	    if (this->index == POLAR_AXIS)
		internal = polar_radius(mark->position);
	    else
		internal = mark->position;

	    if (this->index == THETA_index)
		; /* No harm done if the angular placement wraps at 2pi */
	    else if (!inrange(internal, internal_min, internal_max))
		continue;

	    if (mark->level < 0) {
		/* label read from data file */
		ticlabel = mark->label;
	    } else if (mark->label && !strchr(mark->label, '%')) {
		/* string constant that contains no format keys */
		ticlabel = mark->label;
	    } else if (this->index >= PARALLEL_AXES) {
		/* FIXME: needed because axis->ticfmt is not maintained for parallel axes */
		gprintf(label, sizeof(label),
			mark->label ? mark->label : this->formatstring,
			log10_base, mark->position);
		ticlabel = label;
	    } else if (this->tictype == DT_TIMEDATE) {
		gstrftime(label, MAX_ID_LEN-1, mark->label ? mark->label : this->ticfmt, mark->position);
		ticlabel = label;
	    } else if (this->tictype == DT_DMS) {
		gstrdms(label, mark->label ? mark->label : this->ticfmt, mark->position);
		ticlabel = label;
	    } else {
		gprintf(label, sizeof(label), mark->label ? mark->label : this->ticfmt, log10_base, mark->position);
		ticlabel = label;
	    }

	    /* use NULL instead of label for minor tics with level 1,
	     * however, allow labels for minor tics with levels > 1 */
	    (*callback) (this, internal,
	    		(mark->level==1)?NULL:ticlabel,
	    		mark->level,
	    		(mark->level>0)?mgrd:lgrd, NULL);

	    /* Polar axis tics are mirrored across the origin */
	    if (this->index == POLAR_AXIS && (this->ticmode & TICS_MIRROR)) {
		int save_gridline = lgrd.l_type;
		lgrd.l_type = LT_NODRAW;
		(*callback) (this, -internal,
			(mark->level==1)?NULL:ticlabel,
			mark->level,
	    		(mark->level>0)?mgrd:lgrd, NULL);
		lgrd.l_type = save_gridline;
	    }

	}
	if (def->type == TIC_USER)
	    return;
    }

    /* series-tics, either TIC_COMPUTED ("autofreq") or TIC_SERIES (user-specified increment)
     *
     * We need to distinguish internal user coords from user coords.
     * Now that we have nonlinear axes (as of version 5.2)
     *  	internal = primary axis, user = secondary axis
     *		TIC_COMPUTED ("autofreq") tries for equal spacing on primary axis
     *		TIC_SERIES   requests equal spacing on secondary (user) axis
     *		minitics are always evenly spaced in user coords
     */

    {
	double tic;		/* loop counter */
	double internal;	/* in internal co-ords */
	double user;		/* in user co-ords */
	double start, step, end;
	int    nsteps;
	double internal_min, internal_max;	/* to allow for rounding errors */
	double ministart = 0, ministep = 1, miniend = 1;	/* internal or user - depends on step */
	double lmin = this->min, lmax = this->max;

	reorder_if_necessary(lmin, lmax);

	/* {{{  choose start, step and end */
	switch (def->type) {
	case TIC_SERIES:
	    if (this->log && this->index != POLAR_AXIS) {
		/* we can tolerate start <= 0 if step and end > 0 */
		if (def->def.series.end <= 0 || def->def.series.incr <= 0)
		    return;	/* just quietly ignore */
		step = def->def.series.incr;
		if (def->def.series.start <= 0)	/* includes case 'undefined, i.e. -VERYLARGE */
		    start = step * floor(lmin / step);
		else
		    start = def->def.series.start;
		if (def->def.series.end == VERYLARGE)
		    end = step * ceil(lmax / step);
		else
		    end = def->def.series.end;
		if (def->logscaling) {
		    /* This tries to emulate earlier gnuplot versions in handling
		     *     set log y; set ytics 10
		     */
		    if (start <= 0) {
			start = step;
			while (start > this->linked_to_primary->min)
			    start -= step;
		    } else {
			start = eval_link_function(this->linked_to_primary, start);
		    }
		    step  = eval_link_function(this->linked_to_primary, step);
		    end   = eval_link_function(this->linked_to_primary, end);
		    lmin = this->linked_to_primary->min;
		    lmax = this->linked_to_primary->max;
		}
	    } else {
		start = def->def.series.start;
		step = def->def.series.incr;
		end = def->def.series.end;
		if (start == -VERYLARGE)
		    start = step * floor(lmin / step);
		if (end == VERYLARGE)
		    end = step * ceil(lmax / step);
	    }
	    break;
	case TIC_COMPUTED:
	    if (nonlinear(this)) {
		lmin = this->linked_to_primary->min;
		lmax = this->linked_to_primary->max;
		reorder_if_necessary(lmin, lmax);
		this->ticstep = make_tics(this->linked_to_primary, 20);
		/* It may be that we _always_ want ticstep = 1.0 */
		if (this->ticstep < 1.0)
		    this->ticstep = 1.0;
	    }
	    /* round to multiple of step */
	    start = this->ticstep * floor(lmin / this->ticstep);
	    step = this->ticstep;
	    end = this->ticstep * ceil(lmax / this->ticstep);
	    break;
	case TIC_MONTH:
	    start = floor(lmin);
	    end = ceil(lmax);
	    step = floor((end - start) / 12);
	    if (step < 1)
		step = 1;
	    break;
	case TIC_DAY:
	    start = floor(lmin);
	    end = ceil(lmax);
	    step = floor((end - start) / 14);
	    if (step < 1)
		step = 1;
	    break;
	default:
	    int_error(NO_CARET,"Internal error : unknown tic type");
	    return;		/* avoid gcc -Wall warning about start */
	}
	/* }}} */

	reorder_if_necessary(start, end);
	step = fabs(step);

	if ((minitics != MINI_OFF) && (this->miniticscale != 0)) {
	    FPRINTF((stderr,"axis.c: %d  start = %g end = %g step = %g base = %g\n",
			__LINE__, start, end, step, this->base));

	    /* {{{  figure out ministart, ministep, miniend */
	    if (minitics == MINI_USER) {
		/* they have said what they want */
		if (this->mtic_freq <= 0) {
		    minitics = MINI_OFF;
		} else if (nonlinear(this)) {
		    /* NB: In the case of TIC_COMPUTED this is wrong but we'll fix it later */
		    double nsteps = this->mtic_freq;
		    if (this->log && nsteps == this->base)
			nsteps -= 1;
		    ministart = ministep = step / nsteps;
		    miniend = step;
 		} else {
		    ministart = ministep = step / this->mtic_freq;
		    miniend = step;
 		}
	    } else if (nonlinear(this) && this->ticdef.logscaling) {
		/* FIXME: Not sure this works for all values of step */
		ministart = ministep = step / (this->base - 1);
		miniend = step;
	    } else if (this->tictype == DT_TIMEDATE) {
		ministart = ministep =
		    make_auto_time_minitics(this->timelevel, step);
		miniend = step * 0.9;
	    } else if (minitics == MINI_AUTO) {
		int k = fabs(step)/pow(10.,floor(log10(fabs(step))));

		/* so that step == k times some power of 10 */
		ministart = ministep = (k==2 ? 0.5 : 0.2) * step;
		miniend = step;
	    } else
		minitics = MINI_OFF;

	    if (ministep <= 0)
		minitics = MINI_OFF;	/* dont get stuck in infinite loop */
	    /* }}} */
	}

	/* {{{  a few tweaks and checks */
	/* watch rounding errors */
	end += SIGNIF * step;
	/* HBB 20011002: adjusting the endpoints doesn't make sense if
	 * some oversmart user used a ticstep (much) larger than the
	 * yrange itself */
	if (step < (fabs(lmax) + fabs(lmin))) {
	    internal_max = lmax + step * SIGNIF;
	    internal_min = lmin - step * SIGNIF;
	} else {
	    internal_max = lmax;
	    internal_min = lmin;
	}

	if (step == 0)
	    return;		/* just quietly ignore them ! */
	/* }}} */

	/* This protects against user error, not precision errors */
	if ( (internal_max-internal_min)/step > term->xmax) {
	    int_warn(NO_CARET,"Too many axis ticks requested (>%.0g)",
		(internal_max-internal_min)/step);
	    return;
	}

	/* This protects against infinite loops if the separation between 	*/
	/* two ticks is less than the precision of the control variables. 	*/
	/* The for(...) loop here must exactly describe the true loop below. 	*/
	/* Furthermore, compiler optimization can muck up this test, so we	*/
	/* tell the compiler that the control variables are volatile.     	*/
	nsteps = 0;
	vol_previous_tic = start-step;
	for (vol_this_tic = start; vol_this_tic <= end; vol_this_tic += step) {
	    if (fabs(vol_this_tic - vol_previous_tic) < (step/4.)) {
		step = end - start;
		nsteps = 2;
		int_warn(NO_CARET, "tick interval too small for machine precision");
		break;
	    }
	    vol_previous_tic = vol_this_tic;
	    nsteps++;
	}

	/* Special case.  I hate it. */
	if (this->index == THETA_index) {
	    if (start == 0 && end > 360)
		nsteps--;
	}

	for (tic = start; nsteps > 0; tic += step, nsteps--) {

	    /* {{{  calc internal and user co-ords */
	    if (this->index == POLAR_AXIS) {
		/* Defer polar conversion until after limit check */
		internal = tic;
		user = tic;
	    } else if (nonlinear(this)) {
		if (def->type == TIC_SERIES && def->logscaling)
		    user = eval_link_function(this, tic);
		else if (def->type == TIC_COMPUTED)
		    user = eval_link_function(this, tic);
		else
		    user = tic;
		internal = tic;	/* It isn't really, but this makes the range checks work */
	    } else {
		/* Normal case (no log, no link) */
		internal = (this->tictype == DT_TIMEDATE)
		    ? time_tic_just(this->timelevel, tic)
		    : tic;
		user = CheckZero(internal, step);
	    }
	    /* }}} */

	    /* Allows placement of theta tics outside the range [0:360] */
	    if (this->index == THETA_index) {
		if (internal > internal_max)
		    internal -= 360.;
		if (internal < internal_min)
		    internal += 360.;
	    }

	    if (internal > internal_max)
		break;		/* gone too far - end of series = VERYLARGE perhaps */
	    if (internal >= internal_min) {
		/* {{{  draw tick via callback */
		switch (def->type) {
		case TIC_DAY:{
			int d = (long) floor(user + 0.5) % 7;
			if (d < 0)
			    d += 7;
			(*callback) (this, internal, abbrev_day_names[d], 0, lgrd,
					def->def.user);
			break;
		    }
		case TIC_MONTH:{
			int m = (long) floor(user - 1) % 12;
			if (m < 0)
			    m += 12;
			(*callback) (this, internal, abbrev_month_names[m], 0, lgrd,
					def->def.user);
			break;
		    }
		default:{	/* comp or series */
			char label[MAX_ID_LEN]; /* Leave room for enhanced text markup */
			double position = 0;

			if (this->tictype == DT_TIMEDATE) {
			    /* If they are doing polar time plot, good luck to them */
			    gstrftime(label, MAX_ID_LEN-1, this->ticfmt, (double) user);
			} else if (this->tictype == DT_DMS) {
			    gstrdms(label, this->ticfmt, (double)user);
			} else if (this->index == POLAR_AXIS) {
			    user = internal;
			    internal = polar_radius(user);
			    gprintf(label, sizeof(label), this->ticfmt, log10_base, tic);
			} else if (this->index >= PARALLEL_AXES) {
			    /* FIXME: needed because ticfmt is not maintained for parallel axes */
			    gprintf(label, sizeof(label), this->formatstring,
				    log10_base, user);
			} else {
			    gprintf(label, sizeof(label), this->ticfmt, log10_base, user);
			}

			/* This is where we finally decided to put the tic mark */
			if (nonlinear(this) && (def->type == TIC_SERIES && def->logscaling))
			    position = user;
			else if (nonlinear(this) && (def->type == TIC_COMPUTED))
			    position = user;
			else
			    position = internal;

			/* Range-limited tic placement */
			if (def->rangelimited
			&&  !inrange(position, this->data_min, this->data_max))
			    continue;

			/* This writes the tic mark and label */
			(*callback) (this, position, label, 0, lgrd, def->def.user);

	 		/* Polar axis tics are mirrored across the origin */
			if (this->index == POLAR_AXIS && (this->ticmode & TICS_MIRROR)) {
			    int save_gridline = lgrd.l_type;
			    lgrd.l_type = LT_NODRAW;
			    (*callback) (this, -position, label, 0, lgrd, def->def.user);
			    lgrd.l_type = save_gridline;
			}
		    }
		}
		/* }}} */

	    }
	    if ((minitics != MINI_OFF) && (this->miniticscale != 0)) {
		/* {{{  process minitics */
		double mplace, mtic_user, mtic_internal;

		for (mplace = ministart; mplace < miniend; mplace += ministep) {
		    if (this->tictype == DT_TIMEDATE) {
			mtic_user = time_tic_just(this->timelevel - 1, internal + mplace);
			mtic_internal = mtic_user;
		    } else if ((nonlinear(this) && (def->type == TIC_COMPUTED))
			   ||  (nonlinear(this) && (def->type == TIC_SERIES && def->logscaling))) {
			/* Make up for bad calculation of ministart/ministep/miniend */
			double this_major = eval_link_function(this, internal);
			double next_major = eval_link_function(this, internal+step);
			mtic_user = this_major + mplace/miniend * (next_major - this_major);
			mtic_internal = eval_link_function(this->linked_to_primary, mtic_user);
		    } else if (nonlinear(this) && this->log) {
			mtic_user = internal + mplace;
			mtic_internal = eval_link_function(this->linked_to_primary, mtic_user);
		    } else {
			mtic_user = internal + mplace;
			mtic_internal = mtic_user;
		    }
		    if (polar && this->index == POLAR_AXIS) {
			/* FIXME: is this really the only case where	*/
			/* mtic_internal is the correct position?	*/
			mtic_user = user + mplace;
			mtic_internal = polar_radius(mtic_user);
			(*callback) (this, mtic_internal, NULL, 1, mgrd, NULL);
			continue;
		    }

		    /* Range-limited tic placement */
		    if (def->rangelimited
		    &&  !inrange(mtic_user, this->data_min, this->data_max))
			continue;

		    if (inrange(mtic_internal, internal_min, internal_max)
		    &&  inrange(mtic_internal, start - step * SIGNIF, end + step * SIGNIF))
			(*callback) (this, mtic_user, NULL, 1, mgrd, NULL);
		}
		/* }}} */
	    }
	}
    }
}

/* }}} */

/* {{{ time_tic_just() */
/* justify ticplace to a proper date-time value */
static double
time_tic_just(t_timelevel level, double ticplace)
{
    struct tm tm;

    if (level <= TIMELEVEL_SECONDS) {
	return (ticplace);
    }
    ggmtime(&tm, ticplace);
    if (level >= TIMELEVEL_MINUTES) { /* units of minutes */
	if (tm.tm_sec > 55)
	    tm.tm_min++;
	tm.tm_sec = 0;
    }
    if (level >= TIMELEVEL_HOURS) { /* units of hours */
	if (tm.tm_min > 55)
	    tm.tm_hour++;
	tm.tm_min = 0;
    }
    if (level >= TIMELEVEL_DAYS) { /* units of days */
	if (tm.tm_hour > 22) {
	    tm.tm_hour = 0;
	    tm.tm_mday = 0;
	    tm.tm_yday++;
	    ggmtime(&tm, gtimegm(&tm));
	}
    }
    /* skip it, I have not bothered with weekday so far */
    if (level >= TIMELEVEL_MONTHS) {/* units of month */
	if (tm.tm_mday > 25) {
	    tm.tm_mon++;
	    if (tm.tm_mon > 11) {
		tm.tm_year++;
		tm.tm_mon = 0;
	    }
	}
	tm.tm_mday = 1;
    }

    ticplace = gtimegm(&tm);
    return (ticplace);
}
/* }}} */


/* {{{ axis_output_tics() */
/* HBB 20000416: new routine. Code like this appeared 4 times, once
 * per 2D axis, in graphics.c. Always slightly different, of course,
 * but generally, it's always the same. I distinguish two coordinate
 * directions, here. One is the direction of the axis itself (the one
 * it's "running" along). I refer to the one orthogonal to it as
 * "non-running", below. */
void
axis_output_tics(
     AXIS_INDEX axis,		/* axis number we're dealing with */
     int *ticlabel_position,	/* 'non-running' coordinate */
     AXIS_INDEX zeroaxis_basis,	/* axis to base 'non-running' position of
				 * zeroaxis on */
     tic_callback callback)	/* tic-drawing callback function */
{
    struct termentry *t = term;
    struct axis *this_axis = &axis_array[axis];
    TBOOLEAN axis_is_vertical = ((axis == FIRST_Y_AXIS) || (axis == SECOND_Y_AXIS));
    TBOOLEAN axis_is_second = ((axis == SECOND_X_AXIS) || (axis == SECOND_Y_AXIS));
    int axis_position;		/* 'non-running' coordinate */
    int mirror_position;	/* 'non-running' coordinate, 'other' side */
    double axis_coord = 0.0;	/* coordinate of this axis along non-running axis */

    if ((zeroaxis_basis == SECOND_X_AXIS) || (zeroaxis_basis == SECOND_Y_AXIS)) {
	axis_position = axis_array[zeroaxis_basis].term_upper;
	mirror_position = axis_array[zeroaxis_basis].term_lower;
    } else {
	axis_position = axis_array[zeroaxis_basis].term_lower;
	mirror_position = axis_array[zeroaxis_basis].term_upper;
    }

    if (axis >= PARALLEL_AXES)
	axis_coord = axis - PARALLEL_AXES + 1;

    if (this_axis->ticmode) {
	/* set the globals needed by the _callback() function */

	if (this_axis->tic_rotate == TEXT_VERTICAL
	    && (*t->text_angle)(TEXT_VERTICAL)) {
	    tic_hjust = axis_is_vertical
		? CENTRE
		: (axis_is_second ? LEFT : RIGHT);
	    tic_vjust = axis_is_vertical
		? (axis_is_second ? JUST_TOP : JUST_BOT)
		: JUST_CENTRE;
	    rotate_tics = TEXT_VERTICAL;
	    if (axis == FIRST_Y_AXIS)
		(*ticlabel_position) += t->v_char / 2;
	/* EAM - allow rotation by arbitrary angle in degrees      */
	/*       Justification of ytic labels is a problem since   */
	/*	 the position is already [mis]corrected for length */
	} else if (this_axis->tic_rotate
		   && (*t->text_angle)(this_axis->tic_rotate)) {
	    switch (axis) {
	    case FIRST_Y_AXIS:		/* EAM Purely empirical shift - is there a better? */
	    				*ticlabel_position += t->h_char * 2.5;
	    				tic_hjust = RIGHT; break;
	    case SECOND_Y_AXIS:		tic_hjust = LEFT;  break;
	    case FIRST_X_AXIS:		tic_hjust = LEFT;  break;
	    case SECOND_X_AXIS:		tic_hjust = LEFT;  break;
	    default:			tic_hjust = LEFT;  break;
	    }
	    tic_vjust = JUST_CENTRE;
	    rotate_tics = this_axis->tic_rotate;
	} else {
	    tic_hjust = axis_is_vertical
		? (axis_is_second ? LEFT : RIGHT)
		: CENTRE;
	    tic_vjust = axis_is_vertical
		? JUST_CENTRE
		: (axis_is_second ? JUST_BOT : JUST_TOP);
	    rotate_tics = 0;
	}

	if (this_axis->manual_justify)
	    tic_hjust = this_axis->tic_pos;
	else
	    this_axis->tic_pos = tic_hjust;

	if (this_axis->ticmode & TICS_MIRROR)
	    tic_mirror = mirror_position;
	else
	    tic_mirror = -1;	/* no thank you */

	if ((this_axis->ticmode & TICS_ON_AXIS)
	    && !axis_array[zeroaxis_basis].log
	    && inrange(axis_coord, axis_array[zeroaxis_basis].min,
		       axis_array[zeroaxis_basis].max)
	    ) {
	    tic_start = AXIS_MAP(zeroaxis_basis, axis_coord);
	    tic_direction = axis_is_second ? 1 : -1;
	    if (axis_array[axis].ticmode & TICS_MIRROR)
		tic_mirror = tic_start;
	    /* put text at boundary if axis is close to boundary and the
	     * corresponding boundary is switched on */
	    if (axis_is_vertical) {
		if (((axis_is_second ? -1 : 1) * (tic_start - axis_position)
		     > (3 * t->h_char))
		    || (!axis_is_second && (!(draw_border & 2)))
		    || (axis_is_second && (!(draw_border & 8))))
		    tic_text = tic_start;
		else
		    tic_text = axis_position;
		tic_text += (axis_is_second ? 1 : -1) * t->h_char;
	    } else {
		if (((axis_is_second ? -1 : 1) * (tic_start - axis_position)
		     > (2 * t->v_char))
		    || (!axis_is_second && (!(draw_border & 1)))
		    || (axis_is_second && (!(draw_border & 4))))
		    tic_text = tic_start +
			(axis_is_second ? 0 : - this_axis->ticscale * t->v_tic);
		else
		    tic_text = axis_position;
		tic_text -= t->v_char;
	    }
	} else {
	    /* tics not on axis --> on border */
	    tic_start = axis_position;
	    tic_direction = (this_axis->tic_in ? 1 : -1) * (axis_is_second ? -1 : 1);
	    tic_text = (*ticlabel_position);
	}
	/* go for it */
	gen_tics(&axis_array[axis], callback);
	(*t->text_angle) (0);	/* reset rotation angle */
    }
}

/* }}} */

/* {{{ axis_set_scale_and_range() */
void
axis_set_scale_and_range(struct axis *axis, int lower, int upper)
{
    axis->term_scale = (upper - lower) / (axis->max - axis->min);
    axis->term_lower = lower;
    axis->term_upper = upper;
    if (axis->linked_to_primary && axis->linked_to_primary->index <= 0) {
	axis = axis->linked_to_primary;
	axis->term_scale = (upper - lower) / (axis->max - axis->min);
	axis->term_lower = lower;
	axis->term_upper = upper;
    }
}
/* }}} */


/* {{{ axis_position_zeroaxis */
static TBOOLEAN
axis_position_zeroaxis(AXIS_INDEX axis)
{
    TBOOLEAN is_inside = FALSE;
    AXIS *this = axis_array + axis;

    /* NB: This is the only place that axis->term_zero is set. */
    /*     So it is important to reach here before plotting.   */
    if ((this->min > 0.0 && this->max > 0.0)
	|| this->log) {
	this->term_zero = (this->max < this->min)
	    ? this->term_upper : this->term_lower;
    } else if (this->min < 0.0 && this->max < 0.0) {
	this->term_zero = (this->max < this->min)
	    ? this->term_lower : this->term_upper;
    } else {
	this->term_zero = AXIS_MAP(axis, 0.0);
	is_inside = TRUE;
    }

    return is_inside;
}
/* }}} */


/* {{{ axis_draw_2d_zeroaxis() */
void
axis_draw_2d_zeroaxis(AXIS_INDEX axis, AXIS_INDEX crossaxis)
{
    AXIS *this = axis_array + axis;

    if (axis_position_zeroaxis(crossaxis) && this->zeroaxis) {
	term_apply_lp_properties(this->zeroaxis);
	if ((axis == FIRST_X_AXIS) || (axis == SECOND_X_AXIS)) {
	    /* zeroaxis is horizontal, at y == 0 */
	    (*term->move) (this->term_lower, axis_array[crossaxis].term_zero);
	    (*term->vector) (this->term_upper, axis_array[crossaxis].term_zero);
	} else if ((axis == FIRST_Y_AXIS) || (axis == SECOND_Y_AXIS)) {
	    /* zeroaxis is vertical, at x == 0 */
	    (*term->move) (axis_array[crossaxis].term_zero, this->term_lower);
	    (*term->vector) (axis_array[crossaxis].term_zero, this->term_upper);
	}
    }
}
/* }}} */

/* Mouse zoom/scroll operations were constructing a series of "set [xyx2y2]range"
 * commands for interpretation.  This caused loss of precision.
 * This routine replaces the interpreted string with a direct update of the
 * axis min/max.   Called from mouse.c (apply_zoom)
 */
void
set_explicit_range(struct axis *this_axis, double newmin, double newmax)
{
    this_axis->set_min = newmin;
    this_axis->set_autoscale &= ~AUTOSCALE_MIN;
    this_axis->min_constraint = CONSTRAINT_NONE;

    this_axis->set_max = newmax;
    this_axis->set_autoscale &= ~AUTOSCALE_MAX;
    this_axis->max_constraint = CONSTRAINT_NONE;

    /* If this is one end of a linked axis pair, replicate the new range to the	*/
    /* linked axis, possibly via a mapping function. 				*/
    if (this_axis->linked_to_secondary)
	clone_linked_axes(this_axis, this_axis->linked_to_secondary);
    else if (this_axis->linked_to_primary)
	clone_linked_axes(this_axis, this_axis->linked_to_primary);
}

double
get_num_or_time(struct axis *axis)
{
    double value = 0;
    char *ss;

    if ((axis != NULL)
    &&  (axis->datatype == DT_TIMEDATE)
    &&  (ss = try_to_get_string())) {
	struct tm tm;
	double usec;
	if (gstrptime(ss, timefmt, &tm,&usec, &value) == DT_TIMEDATE)
	    value = (double) gtimegm(&tm) + usec;
	free(ss);
    } else {
	value = real_expression();
    }

    return value;
}

static void
load_one_range(struct axis *this_axis, double *a, t_autoscale *autoscale, t_autoscale which )
{
    double number;

    assert( which==AUTOSCALE_MIN || which==AUTOSCALE_MAX );

    if (equals(c_token, "*")) {
	/*  easy:  do autoscaling!  */
	*autoscale |= which;
	if (which==AUTOSCALE_MIN) {
	    this_axis->min_constraint &= ~CONSTRAINT_LOWER;
	    this_axis->min_lb = 0;  /*  dummy entry  */
	} else {
	    this_axis->max_constraint &= ~CONSTRAINT_LOWER;
	    this_axis->max_lb = 0;  /*  dummy entry  */
	}
	c_token++;
    } else {
	/*  this _might_ be autoscaling with constraint or fixed value */
	/*  The syntax of '0 < *...' confuses the parser as he will try to
	    include the '<' as a comparison operator in the expression.
	    Setting scanning_range_in_progress will stop the parser from
	    trying to build an action table if he finds '<' followed by '*'
	    (which would normally trigger a 'invalid expression'),  */
	scanning_range_in_progress = TRUE;
	number = get_num_or_time(this_axis);
	scanning_range_in_progress = FALSE;

	if (END_OF_COMMAND)
	    int_error(c_token, "unfinished range");

	if (equals(c_token, "<")) {
	    /*  this _seems_ to be autoscaling with lower bound  */
	    c_token++;
	    if (END_OF_COMMAND) {
		int_error(c_token, "unfinished range with constraint");
	    } else if (equals(c_token, "*")) {
		/*  okay:  this _is_ autoscaling with lower bound!  */
		*autoscale |= which;
		if (which==AUTOSCALE_MIN) {
		    this_axis->min_constraint |= CONSTRAINT_LOWER;
		    this_axis->min_lb = number;
		} else {
		    this_axis->max_constraint |= CONSTRAINT_LOWER;
		    this_axis->max_lb = number;
		}
		c_token++;
	    } else {
		int_error(c_token, "malformed range with constraint");
	    }
	} else if (equals(c_token, ">")) {
	    int_error(c_token, "malformed range with constraint (use '<' only)");
	} else {
	    /*  no autoscaling-with-lower-bound but simple fixed value only  */
	    *autoscale &= ~which;
	    if (which==AUTOSCALE_MIN) {
		this_axis->min_constraint = CONSTRAINT_NONE;
		this_axis->min_ub = 0;  /*  dummy entry  */
	    } else {
		this_axis->max_constraint = CONSTRAINT_NONE;
		this_axis->max_ub = 0;  /*  dummy entry  */
	    }
	    *a = number;
	}
    }

    if (*autoscale & which) {
	/*  check for upper bound only if autoscaling is on  */
	if (END_OF_COMMAND)  int_error(c_token, "unfinished range");
	if (equals(c_token, "<")) {
	    /*  looks like upper bound up to now...  */

	    c_token++;
	    if (END_OF_COMMAND) int_error(c_token, "unfinished range with constraint");

	    number = get_num_or_time(this_axis);
	    /*  this autoscaling has an upper bound:  */

	    if (which==AUTOSCALE_MIN) {
		this_axis->min_constraint |= CONSTRAINT_UPPER;
		this_axis->min_ub = number;
	    } else {
		this_axis->max_constraint |= CONSTRAINT_UPPER;
		this_axis->max_ub = number;
	    }
	} else if (equals(c_token, ">")) {
	    int_error(c_token, "malformed range with constraint (use '<' only)");
	} else {
	    /*  there is _no_ upper bound on this autoscaling  */
	    if (which==AUTOSCALE_MIN) {
		this_axis->min_constraint &= ~CONSTRAINT_UPPER;
		this_axis->min_ub = 0;  /*  dummy entry  */
	    } else {
		this_axis->max_constraint &= ~CONSTRAINT_UPPER;
		this_axis->max_ub = 0;  /*  dummy entry  */
	    }
	}
    } else if (!END_OF_COMMAND){
	/*  no autoscaling = fixed value --> complain about constraints  */
	if (equals(c_token, "<") || equals(c_token, ">") ) {
	    int_error(c_token, "no upper bound constraint allowed if not autoscaling");
	}
    }

    /*  Consistency check  */
    if (*autoscale & which) {
	if (which==AUTOSCALE_MIN && this_axis->min_constraint==CONSTRAINT_BOTH) {
	    if (this_axis->min_ub < this_axis->min_lb ) {
		int_warn(c_token,"Upper bound of constraint < lower bound:  Turning of constraints.");
		this_axis->min_constraint = CONSTRAINT_NONE;
	    }
	}
	if (which==AUTOSCALE_MAX && this_axis->max_constraint==CONSTRAINT_BOTH) {
	    if (this_axis->max_ub < this_axis->max_lb ) {
		int_warn(c_token,"Upper bound of constraint < lower bound:  Turning of constraints.");
		this_axis->max_constraint = CONSTRAINT_NONE;
	    }
	}
    }
}


/* {{{ load_range() */
/* loads a range specification from the input line into variables 'a'
 * and 'b' */
t_autoscale
load_range(struct axis *this_axis, double *a, double *b, t_autoscale autoscale)
{
    if (equals(c_token, "]")) {
	this_axis->min_constraint = CONSTRAINT_NONE;
	this_axis->max_constraint = CONSTRAINT_NONE;
	return (autoscale);
    }

    if (END_OF_COMMAND) {
	int_error(c_token, "starting range value or ':' or 'to' expected");
    } else if (!equals(c_token, "to") && !equals(c_token, ":")) {
	load_one_range(this_axis, a, &autoscale, AUTOSCALE_MIN );
    }

    if (!equals(c_token, "to") && !equals(c_token, ":"))
	int_error(c_token, "':' or keyword 'to' expected");
    c_token++;

    if (!equals(c_token, "]")) {
	load_one_range(this_axis, b, &autoscale, AUTOSCALE_MAX );
    }

    /* Not all the code can deal nicely with +/- infinity */
    if (*a < -VERYLARGE)
	*a = -VERYLARGE;
    if (*b > VERYLARGE)
	*b = VERYLARGE;

    return (autoscale);
}

/* }}} */


/* we determine length of the widest tick label by getting gen_ticks to
 * call this routine with every label
 */

void
widest_tic_callback(struct axis *this_axis, double place, char *text,
    int ticlevel,
    struct lp_style_type grid,
    struct ticmark *userlabels)
{
    (void) this_axis;		/* avoid "unused parameter" warnings */
    (void) place;
    (void) grid;
    (void) userlabels;

    /* historically, minitics used to have no text,
     * but now they can, except at ticlevel 1
     * (and this restriction is there only for compatibility reasons) */
    if (ticlevel != 1) {
	int len = label_width(text, NULL);
	if (len > widest_tic_strlen)
	    widest_tic_strlen = len;
    }
}


/*
 * get and set routines for range writeback
 * ULIG *
 */

void
save_writeback_all_axes()
{
    AXIS_INDEX axis;

    for (axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
	if (axis_array[axis].range_flags & RANGE_WRITEBACK) {
	    axis_array[axis].writeback_min = axis_array[axis].min;
	    axis_array[axis].writeback_max = axis_array[axis].max;
	}
}

void
check_axis_reversed( AXIS_INDEX axis )
{
    AXIS *this = axis_array + axis;
    if (((this->autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE)
    &&  (this->set_max < this->set_min)) {
	this->min = this->set_min;
	this->max = this->set_max;
    }
}

TBOOLEAN
some_grid_selected()
{
    AXIS_INDEX i;
    for (i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
	if (axis_array[i].gridmajor || axis_array[i].gridminor)
	    return TRUE;
    /* Dec 2016 - CHANGE */
    if (polar_grid_angle > 0)
	return TRUE;
    if (grid_spiderweb)
	return TRUE;
    return FALSE;
}

/*
 * Range checks for the color axis.
 */
void
set_cbminmax()
{
    if (CB_AXIS.set_autoscale & AUTOSCALE_MIN) {
	if (CB_AXIS.min >= VERYLARGE)
	    CB_AXIS.min = Z_AXIS.min;
    }
    CB_AXIS.min = axis_log_value_checked(COLOR_AXIS, CB_AXIS.min, "color axis");

    if (CB_AXIS.set_autoscale & AUTOSCALE_MAX) {
	if (CB_AXIS.max <= -VERYLARGE)
	    CB_AXIS.max = Z_AXIS.max;
    }
    CB_AXIS.max = axis_log_value_checked(COLOR_AXIS, CB_AXIS.max, "color axis");

    if (CB_AXIS.min > CB_AXIS.max) {
	double tmp = CB_AXIS.max;
	CB_AXIS.max = CB_AXIS.min;
	CB_AXIS.min = tmp;
    }
    if (CB_AXIS.linked_to_primary)
	clone_linked_axes(&CB_AXIS, CB_AXIS.linked_to_primary);
}

void
save_autoscaled_ranges(AXIS *x_axis, AXIS *y_axis)
{
    if (x_axis) {
	save_autoscaled_xmin = x_axis->min;
	save_autoscaled_xmax = x_axis->max;
    }
    if (y_axis) {
	save_autoscaled_ymin = y_axis->min;
	save_autoscaled_ymax = y_axis->max;
    }
}

void
restore_autoscaled_ranges(AXIS *x_axis, AXIS *y_axis)
{
    if (x_axis) {
	x_axis->min = save_autoscaled_xmin;
	x_axis->max = save_autoscaled_xmax;
    }
    if (y_axis) {
	y_axis->min = save_autoscaled_ymin;
	y_axis->max = save_autoscaled_ymax;
    }
}

static void
get_position_type(enum position_type *type, AXIS_INDEX *axes)
{
    if (almost_equals(c_token, "fir$st")) {
	++c_token;
	*type = first_axes;
    } else if (almost_equals(c_token, "sec$ond")) {
	++c_token;
	*type = second_axes;
    } else if (almost_equals(c_token, "gr$aph")) {
	++c_token;
	*type = graph;
    } else if (almost_equals(c_token, "sc$reen")) {
	++c_token;
	*type = screen;
    } else if (almost_equals(c_token, "char$acter")) {
	++c_token;
	*type = character;
    } else if (equals(c_token, "polar")) {
	++c_token;
	*type = polar_axes;
    }
    switch (*type) {
    case first_axes:
    case polar_axes:
	*axes = FIRST_AXES;
	return;
    case second_axes:
	*axes = SECOND_AXES;
	return;
    default:
	*axes = NO_AXIS;
	return;
    }
}

/* get_position() - reads a position for label,arrow,key,... */
void
get_position(struct position *pos)
{
    get_position_default(pos, first_axes, 3);
}

/* get_position() - reads a position for label,arrow,key,...
 * with given default coordinate system
 * ndim = 2 only reads x,y
 * otherwise it reads x,y,z
 */
void
get_position_default(struct position *pos, enum position_type default_type, int ndim)
{
    AXIS_INDEX axes;
    enum position_type type = default_type;

    memset(pos, 0, sizeof(struct position));

    get_position_type(&type, &axes);
    pos->scalex = type;
    GET_NUMBER_OR_TIME(pos->x, axes, FIRST_X_AXIS);

    if (equals(c_token, ",")) {
	++c_token;
	get_position_type(&type, &axes);
	pos->scaley = type;
	GET_NUMBER_OR_TIME(pos->y, axes, FIRST_Y_AXIS);
    } else {
	pos->y = 0;
	pos->scaley = type;
    }

    /* Resolves ambiguous syntax when trailing comma ends a plot command */
    if (ndim != 2 && equals(c_token, ",")) {
	++c_token;
	get_position_type(&type, &axes);
	/* HBB 2015-01-28: no secondary Z axis, so patch up if it was selected */
	if (type == second_axes) {
	    type = first_axes;
	    axes = FIRST_AXES;
	}
	pos->scalez = type;
	GET_NUMBER_OR_TIME(pos->z, axes, FIRST_Z_AXIS);
    } else {
	pos->z = 0;
	pos->scalez = type;	/* same as y */
    }
}

/*
 * Add a single tic mark, with label, to the list for this axis.
 * To avoid duplications and overprints, sort the list and allow
 * only one label per position.
 * EAM - called from set.c during `set xtics` (level >= 0)
 *       called from datafile.c during `plot using ::xtic()` (level = -1)
 */
void
add_tic_user(struct axis *this_axis, char *label, double position, int level)
{
    struct ticmark *tic, *newtic;
    struct ticmark listhead;

    if (!label && level < 0)
	return;

    /* Mark this axis as user-generated ticmarks only, unless the */
    /* mix flag indicates that both user- and auto- tics are OK.  */
    if (!this_axis->ticdef.def.mix)
	this_axis->ticdef.type = TIC_USER;

    /* Walk along list to sorted positional order */
    listhead.next = this_axis->ticdef.def.user;
    listhead.position = -DBL_MAX;
    for (tic = &listhead;
	 tic->next && (position > tic->next->position);
	 tic = tic->next) {
    }

    if ((tic->next == NULL) || (position < tic->next->position)) {
	/* Make a new ticmark */
	newtic = (struct ticmark *) gp_alloc(sizeof(struct ticmark), (char *) NULL);
	newtic->position = position;
	/* Insert it in the list */
	newtic->next = tic->next;
	tic->next = newtic;
    } else {
	/* The new tic must duplicate position of tic->next */
	if (position != tic->next->position)
	    int_warn(NO_CARET,"add_tic_user: list sort error");
	newtic = tic->next;
	/* Don't over-write a major tic with a minor tic */
	if (level == 1)
	    return;
	/* User-specified tics are preferred to autogenerated tics. */
	if (level == 0 && newtic->level > 1)
	    return;
	/* FIXME: But are they preferred to data-generated tics?    */
	if (newtic->level < level)
	    return;
	if (newtic->label) {
	    free(newtic->label);
	    newtic->label = NULL;
	}
    }
    newtic->level = level;

    if (label)
	newtic->label = gp_strdup(label);
    else
	newtic->label = NULL;

    /* Make sure the listhead is kept */
    this_axis->ticdef.def.user = listhead.next;
}

/*
 * Degrees/minutes/seconds geographic coordinate format
 * ------------------------------------------------------------
 *  %D 			= integer degrees, truncate toward zero
 *  %<width.precision>d	= floating point degrees
 *  %M 			= integer minutes, truncate toward zero
 *  %<width.precision>m	= floating point minutes
 *  %S 			= integer seconds, truncate toward zero
 *  %<width.precision>s	= floating point seconds
 *  %E                  = E/W instead of +/-
 *  %N                  = N/S instead of +/-
 */
void
gstrdms (char *label, char *format, double value)
{
double Degrees, Minutes, Seconds;
double degrees, minutes, seconds;
int dtype = 0, mtype = 0, stype = 0;
TBOOLEAN EWflag = FALSE;
TBOOLEAN NSflag = FALSE;
char compass = ' ';
char *c, *cfmt;

    /* Limit the range to +/- 180 degrees */
    if (value > 180.)
	value -= 360.;
    if (value < -180.)
	value += 360.;

    degrees = fabs(value);
    Degrees = floor(degrees);
    minutes = (degrees - (double)Degrees) * 60.;
    Minutes = floor(minutes);
    seconds = (degrees - (double)Degrees) * 3600. -  (double)Minutes*60.;
    Seconds = floor(seconds);

    for (c = cfmt = gp_strdup(format); *c; ) {
	if (*c++ == '%') {
	    while (*c && !strchr("DdMmSsEN%",*c)) {
		if (!isdigit((unsigned char)*c) && !isspace((unsigned char)*c) && !ispunct((unsigned char)*c))
			int_error(NO_CARET,"unrecognized format: \"%s\"",format);
		c++;
	    }
	    switch (*c) {
	    case 'D':	*c = 'g'; dtype = 1; degrees = Degrees; break;
	    case 'd':	*c = 'f'; dtype = 2; break;
	    case 'M':	*c = 'g'; mtype = 1; minutes = Minutes; break;
	    case 'm':	*c = 'f'; mtype = 2; break;
	    case 'S':	*c = 'g'; stype = 1; seconds = Seconds; break;
	    case 's':	*c = 'f'; stype = 2; break;
	    case 'E':	*c = 'c'; EWflag = TRUE; break;
	    case 'N':	*c = 'c'; NSflag = TRUE; break;
	    case '%':	int_error(NO_CARET,"unrecognized format: \"%s\"",format);
	    }
	}
    }

    /* By convention the minus sign goes only in front of the degrees */
    /* Watch out for round-off errors! */
    if (value < 0 && !EWflag && !NSflag) {
	if (dtype > 0)  degrees = -fabs(degrees);
	else if (mtype > 0)  minutes = -fabs(minutes);
	else if (stype > 0)  seconds = -fabs(seconds);
    }
    if (EWflag)
	compass = (value == 0) ? ' ' : (value < 0) ? 'W' : 'E';
    if (NSflag)
	compass = (value == 0) ? ' ' : (value < 0) ? 'S' : 'N';

    /* This is tricky because we have to deal with the possibility that
     * the user may not have specified all the possible format components
     */
    if (dtype == 0) {	/* No degrees */
	if (mtype == 0) {
	    if (stype == 0) /* Must be some non-DMS format */
		snprintf(label, MAX_ID_LEN, cfmt, value);
	    else
		snprintf(label, MAX_ID_LEN, cfmt,
		    seconds, compass);
	} else {
	    if (stype == 0)
		snprintf(label, MAX_ID_LEN, cfmt,
		    minutes, compass);
	    else
		snprintf(label, MAX_ID_LEN, cfmt,
		    minutes, seconds, compass);
	}
    } else {	/* Some form of degrees in first field */
	if (mtype == 0) {
	    if (stype == 0)
		snprintf(label, MAX_ID_LEN, cfmt,
		    degrees, compass);
	    else
		snprintf(label, MAX_ID_LEN, cfmt,
		    degrees, seconds, compass);
	} else {
	    if (stype == 0)
		snprintf(label, MAX_ID_LEN, cfmt,
		    degrees, minutes, compass);
	    else
		snprintf(label, MAX_ID_LEN, cfmt,
		    degrees, minutes, seconds, compass);
	}
    }

    free(cfmt);
}

/* Accepts a range of the form [MIN:MAX] or [var=MIN:MAX]
 * Loads new limiting values into axis->min axis->max
 * Returns
 *	 0 = no range spec present
 *	-1 = range spec with no attached variable name
 *	>0 = token indexing the attached variable name
 */
int
parse_range(AXIS_INDEX axis)
{
    struct axis *this_axis = &axis_array[axis];
    int dummy_token = -1;

    /* No range present */
    if (!equals(c_token, "["))
	return 0;

    /* Empty brackets serve as a place holder */
    if (equals(c_token, "[]")) {
	c_token += 2;
	return 0;
    }

    /* If the range starts with "[var=" return the token of the named variable. */
    c_token++;
    if (isletter(c_token) && equals(c_token + 1, "=")) {
	dummy_token = c_token;
	c_token += 2;
    }

    this_axis->autoscale = load_range(this_axis, &this_axis->min, &this_axis->max,
					this_axis->autoscale);

    /* Nonlinear axis - find the linear range equivalent */
    if (this_axis->linked_to_primary) {
	AXIS *primary = this_axis->linked_to_primary;
	clone_linked_axes(this_axis, primary);
    }

    /* This handles (imperfectly) the problem case
     *    set link x2 via f(x) inv g(x)
     *    plot [x=min:max][] something that involves x2
     * Other cases of in-line range changes on a linked axis may fail
     */
    else if (this_axis->linked_to_secondary) {
	AXIS *secondary = this_axis->linked_to_secondary;
	if (secondary->link_udf != NULL && secondary->link_udf->at != NULL)
	    clone_linked_axes(this_axis, secondary);
    }

    if (axis == SAMPLE_AXIS || axis == T_AXIS || axis == U_AXIS || axis == V_AXIS) {
	this_axis->SAMPLE_INTERVAL = 0;
	if (equals(c_token, ":")) {
	    c_token++;
	    this_axis->SAMPLE_INTERVAL = real_expression();
	}
    }

    if (!equals(c_token, "]"))
	int_error(c_token, "']' expected");
    c_token++;

    return dummy_token;
}

/* Called if an in-line range is encountered while inside a zoom command */
void
parse_skip_range()
{
    while (!equals(c_token++,"]"))
	if (END_OF_COMMAND)
	    break;
    return;
}

/*
 * When a secondary axis (axis2) is linked to the corresponding primary
 * axis (axis1), this routine copies the relevant range/scale data
 */
void
clone_linked_axes(AXIS *axis1, AXIS *axis2)
{
    double testmin, testmax, scale;
    TBOOLEAN suspect = FALSE;

    memcpy(axis2, axis1, AXIS_CLONE_SIZE);
    if (axis2->link_udf == NULL || axis2->link_udf->at == NULL)
	return;

    /* Transform the min/max limits of linked secondary axis */
    inverse_function_sanity_check:
	axis2->set_min = eval_link_function(axis2, axis1->set_min);
	axis2->set_max = eval_link_function(axis2, axis1->set_max);
	axis2->min = eval_link_function(axis2, axis1->min);
	axis2->max = eval_link_function(axis2, axis1->max);

	/* Confirm that the inverse mapping actually works, at least at the endpoints.

	We makes sure that inverse_f( f(x) ) = x at the edges of our plot
	bounds, and if not, we throw a warning, and we try to be robust to
	numerical-precision errors causing false-positive warnings. We look at
	the error relative to a scaling:

	   (inverse_f( f(x) ) - x) / scale

	where the scale is the mean of (x(min edge of plot), x(max edge of
	plot)). I.e. we only care about errors that are large on the scale of
	the plot bounds we're looking at.
	*/

	if (isnan(axis2->set_min) || isnan(axis2->set_max))
	    suspect = TRUE;
	testmin = eval_link_function(axis1, axis2->set_min);
	testmax = eval_link_function(axis1, axis2->set_max);
	scale = (fabs(axis1->set_min) + fabs(axis1->set_max))/2.0;

	if (isnan(testmin) || isnan(testmax))
	    suspect = TRUE;
	if (fabs(testmin - axis1->set_min) != 0
	&&  fabs((testmin - axis1->set_min) / scale) > 1.e-6)
	    suspect = TRUE;
	if (fabs(testmax - axis1->set_max) != 0
	&&  fabs((testmax - axis1->set_max) / scale) > 1.e-6)
	    suspect = TRUE;

	if (suspect) {
	    /* Give it one chance to ignore a bad default range [-10:10] */
	    if (((axis1->autoscale & AUTOSCALE_MIN) == AUTOSCALE_MIN)
	    &&  axis1->set_min <= 0 && axis1->set_max > 0.1) {
		axis1->set_min = 0.1;
		suspect = FALSE;
		goto inverse_function_sanity_check;
	    }
	    int_warn(NO_CARET, "could not confirm linked axis inverse mapping function");
	    dump_axis_range(axis1);
	    dump_axis_range(axis2);
	}
}

/* Evaluate the function linking secondary axis to primary axis */
double
eval_link_function(struct axis *axis, double raw_coord)
{
    udft_entry *link_udf = axis->link_udf;
    int dummy_var;
    struct value a;

    /* A test for if (undefined) is allowed only immediately following
     * either evalute_at() or eval_link_function().  Both must clear it
     * on entry so that the value on return reflects what really happened.
     */
    undefined = FALSE;

    /* Special case to speed up evaluation of log scaling */
    /* benchmark timing summary
     * v4.6 (old-style logscale)	42.7 u 42.7 total
     * v5.1 (generic nonlinear) 	57.5 u 66.2 total
     * v5.1 (optimized nonlinear)	42.1 u 42.2 total
     */
    if (axis->log) {
	if (axis->linked_to_secondary) {
	    if (raw_coord <= 0)
		return not_a_number();
	    else
		return log(raw_coord) / axis->log_base;
	} else if (axis->linked_to_primary) {
	    return exp(raw_coord * axis->log_base);
	}
    }

    /* This handles the case "set link x2" with no via/inverse mapping */
    if (link_udf == NULL || link_udf->at == NULL)
	return raw_coord;

    if (abs(axis->index) == FIRST_Y_AXIS || abs(axis->index) == SECOND_Y_AXIS)
	dummy_var = 1;
    else
	dummy_var = 0;
    link_udf->dummy_values[1-dummy_var].type = INVALID_NAME;

    Gcomplex(&link_udf->dummy_values[dummy_var], raw_coord, 0.0);

    evaluate_at(link_udf->at, &a);

    if (undefined || a.type != CMPLX) {
	FPRINTF((stderr, "eval_link_function(%g) returned %s\n",
		raw_coord, undefined ? "undefined" : "unexpected type"));
	a = udv_NaN->udv_value;
    }
    if (isnan(a.v.cmplx_val.real))
	undefined = TRUE;

    return a.v.cmplx_val.real;
}

/*
 * Obtain and initialize a shadow axis.
 * The details are hidden from the rest of the code (dynamic/static allocation, etc).
 */
AXIS *
get_shadow_axis(AXIS *axis)
{
    AXIS *primary = NULL;
    AXIS *secondary = axis;
    int i;

    /* This implementation uses a dynamically allocated array of shadow axis	*/
    /* structures that is allocated on first use and reused after that. 	*/
    if (!shadow_axis_array) {
	shadow_axis_array = gp_alloc( NUMBER_OF_MAIN_VISIBLE_AXES * sizeof(AXIS), NULL);
	for (i=0; i<NUMBER_OF_MAIN_VISIBLE_AXES; i++)
	    memcpy(&shadow_axis_array[i], &default_axis_state, sizeof(AXIS));
    }

    if (axis->index != SAMPLE_AXIS && axis->index < NUMBER_OF_MAIN_VISIBLE_AXES)
	primary = &shadow_axis_array[axis->index];
    else
	int_error(NO_CARET, "invalid shadow axis");

    primary->index = -secondary->index;

    return primary;
}

/*
 * This is necessary if we are to reproduce the old logscaling.
 * Extend the tic range on an independent log-scaled axis to the
 * nearest power of 10.
 * Transfer the new limits over to the user-visible secondary axis.
 */
void
extend_primary_ticrange(AXIS *axis)
{
    AXIS *primary = axis->linked_to_primary;
    TBOOLEAN autoextend_min = (axis->autoscale & AUTOSCALE_MIN)
	&& !(axis->autoscale & AUTOSCALE_FIXMIN);
    TBOOLEAN autoextend_max = (axis->autoscale & AUTOSCALE_MAX)
	&& !(axis->autoscale & AUTOSCALE_FIXMAX);

    if (axis->ticdef.logscaling) {
	/* This can happen on "refresh" if the axis was unused */
	if (primary->min >= VERYLARGE || primary->max <= -VERYLARGE)
	    return;

	/* NB: "zero" is the minimum non-zero value from "set zero" */
	if (autoextend_min
	||  (fabs(primary->min - floor(primary->min)) < zero)) {
	    primary->min = floor(primary->min);
	    axis->min = eval_link_function(axis, primary->min);
	}
	if (autoextend_max
	||  (fabs(primary->max - ceil(primary->max)) < zero)) {
	    primary->max = ceil(primary->max);
	    axis->max = eval_link_function(axis, primary->max);
	}
    }
}

/*
 * As data is read in or functions evaluated, the min/max values are tracked
 * for the secondary (visible) axes but not for the linked primary (linear) axis.
 * This routine fills in the primary min/max from the secondary axis.
 */
void
update_primary_axis_range(struct axis *secondary)
{
    struct axis *primary = secondary->linked_to_primary;

    if (primary != NULL) {
	/* nonlinear axis (secondary is visible; primary is hidden) */
	primary->min = eval_link_function(primary, secondary->min);
	primary->max = eval_link_function(primary, secondary->max);
	primary->data_min = eval_link_function(primary, secondary->data_min);
	primary->data_max = eval_link_function(primary, secondary->data_max);
    }
}

/*
 * Same thing but in the opposite direction.  We read in data on the primary axis
 * and want the autoscaling on a linked secondary axis to match.
 */
void
update_secondary_axis_range(struct axis *primary)
{
    struct axis *secondary = primary->linked_to_secondary;

    if (secondary != NULL) {
       secondary->min = eval_link_function(secondary, primary->min);
       secondary->max = eval_link_function(secondary, primary->max);
       secondary->data_min = eval_link_function(secondary, primary->data_min);
       secondary->data_max = eval_link_function(secondary, primary->data_max);
    }
}

/*
 * range-extend autoscaled log axis
 */
void
extend_autoscaled_log_axis(AXIS *primary)
{
    if (primary->log) {
	extend_primary_ticrange(primary);
	axis_invert_if_requested(primary);
	check_log_limits(primary, primary->min, primary->max);
	update_primary_axis_range(primary);
    }
}

/*
 * gnuplot version 5.0 always maintained autoscaled range on x1
 * specifically, transforming from x2 coordinates if necessary.
 * In version 5.2 we track the x1 and x2 axis data limits separately.
 * However if x1 and x2 are linked to each other we must reconcile
 * their data limits before plotting.
 */
void
reconcile_linked_axes(AXIS *primary, AXIS *secondary)
{
    double dummy;
    coord_type inrange = INRANGE;
    if ((primary->autoscale & AUTOSCALE_BOTH) != AUTOSCALE_NONE
    &&  primary->linked_to_secondary) {
	double min_2_into_1 = eval_link_function(primary, secondary->data_min);
	double max_2_into_1 = eval_link_function(primary, secondary->data_max);

	/* Merge secondary min/max into primary data range */
	store_and_update_range(&dummy, min_2_into_1, &inrange, primary, FALSE);
	store_and_update_range(&dummy, max_2_into_1, &inrange, primary, FALSE);
	(void)dummy;	/* Otherwise the compiler complains about an unused variable */

	/* Take the result back the other way to update secondary */
	secondary->min = eval_link_function(secondary, primary->min);
	secondary->max = eval_link_function(secondary, primary->max);
    }
}


/*
 * Check for linked-axis coordinate transformation given by command
 *     set {x|y}2r link via <expr1> inverse <expr2>
 * If we are plotting on the secondary axis in this case, apply the inverse
 * transform to get back to the primary coordinate system before mapping.
 */

double
map_x_double(double value)
{
    if (axis_array[x_axis].linked_to_primary) {
	AXIS *primary = axis_array[x_axis].linked_to_primary;
	if (primary->link_udf->at) {
	    value = eval_link_function(primary, value);
	    if (undefined)
		return not_a_number();
	    return axis_map_double(primary, value);
	}
    }
    return AXIS_MAP_DOUBLE(x_axis, value);
}

int
map_x(double value)
{
    double x = map_x_double(value);
    if(isnan(x)) return intNaN;

    return axis_map_toint(x);
}

double
map_y_double(double value)
{
    if (axis_array[y_axis].linked_to_primary) {
	AXIS *primary = axis_array[y_axis].linked_to_primary;
	if (primary->link_udf->at) {
	    value = eval_link_function(primary, value);
	    if (undefined)
		return not_a_number();
	    return axis_map_double(primary, value);
	}
    }
    return AXIS_MAP_DOUBLE(y_axis, value);
}

int
map_y(double value)
{
    double y = map_y_double(value);
    if(isnan(y)) return intNaN;

    return axis_map_toint(y);
}

/*
 * Convert polar coordinates [theta;r] to the corresponding [x;y]
 * If update is TRUE then check and update rrange autoscaling
 */
coord_type
polar_to_xy( double theta, double r, double *x, double *y, TBOOLEAN update)
{
    coord_type status = INRANGE;

    /* NB: Range checks from multiple original sites are consolidated here.
     * They were not all identical but I hope this version is close enough.
     * One caller (parametric fixup) did R_AXIS.max range checks
     * against fabs(r) rather than r.  Does that matter?  Did something break?
     */
    if (update) {
	if (inverted_raxis) {
	    if (!inrange(r, R_AXIS.set_min, R_AXIS.set_max))
		status = OUTRANGE;
	} else {
	    if (r < R_AXIS.min) {
		if (R_AXIS.autoscale & AUTOSCALE_MIN)
		    R_AXIS.min = 0;
		else if (R_AXIS.min < 0)
		    status = OUTRANGE;
		else if (r < 0 && -r > R_AXIS.max)
		    status = OUTRANGE;
		else if (r >= 0)
		    status = OUTRANGE;
	    }
	    if (r > R_AXIS.max) {
		if (R_AXIS.autoscale & AUTOSCALE_MAX)	{
		    if ((R_AXIS.max_constraint & CONSTRAINT_UPPER)
		    &&  (R_AXIS.max_ub < r))
			R_AXIS.max = R_AXIS.max_ub;
		    else
			R_AXIS.max = r;
		} else {
		    status = OUTRANGE;
		}
	    }
	}
    }

    if (nonlinear(&R_AXIS)) {
	AXIS *shadow = R_AXIS.linked_to_primary;
	if (R_AXIS.log && r <= 0)
	    r = not_a_number();
	else {
	    r = eval_link_function(shadow, r) - shadow->min;
	    if (update && (R_AXIS.autoscale & AUTOSCALE_MAX) && r > shadow->max)
		shadow->max = r;
	}
    } else if (inverted_raxis) {
	r = R_AXIS.set_min - r;
    } else if ((R_AXIS.autoscale & AUTOSCALE_MIN)) {
	; /* Leave it */
    } else if (r >= R_AXIS.min) {
	/* We store internally as if plotting r(theta) - rmin */
	r = r - R_AXIS.min;
    } else if (r < -R_AXIS.min) {
	/* If (r < R_AXIS.min < 0) we already flagged OUTRANGE above */
	/* That leaves the case (r < 0  &&  R_AXIS.min >= 0) */
	r = r + R_AXIS.min;
    } else {
	*x = not_a_number();
	*y = not_a_number();
	return OUTRANGE;
    }

    /* Correct for theta=0 position and handedness */
    theta = theta * theta_direction * ang2rad + theta_origin * DEG2RAD;

    *x = r * cos(theta);
    *y = r * sin(theta);

    return status;
}

/*
 * converts polar coordinate r into a magnitude on x
 * allowing for R_AXIS.min != 0, axis inversion, nonlinearity, etc.
 */
double
polar_radius(double r)
{
    double px, py;
    polar_to_xy(0.0, r, &px, &py, FALSE);
    return sqrt(px*px + py*py);
}

/*
 * Print current axis range values to terminal.
 * Mostly for debugging.
 */
void
dump_axis_range(struct axis *axis)
{
    if (!axis) return;
    fprintf(stderr, "    %10.10s axis min/max %10g %10g data_min/max %10g %10g\n",
	axis_name(axis->index), axis->min, axis->max,
	axis->data_min, axis->data_max);
    fprintf(stderr, "                set_min/max %10g %10g \t link:\t %s\n",
	axis->set_min, axis->set_max,
	axis->linked_to_primary ? axis_name(axis->linked_to_primary->index) : "none");
}


/*
 * This routine replaces former macro ACTUAL_STORE_AND_UPDATE_RANGE().
 *
 * Version 5: OK to store infinities or NaN
 * Return UNDEFINED so that caller can take action if desired.
 */
coord_type
store_and_update_range(
    double *store,
    double curval,
    coord_type *type,
    struct axis *axis,
    TBOOLEAN noautoscale)
{
    *store = curval;
    if (! (curval > -VERYLARGE && curval < VERYLARGE)) {
	*type = UNDEFINED;
	return UNDEFINED;
    }
    if (axis->log) {
	if (curval < 0.0) {
	    *type = UNDEFINED;
	    return UNDEFINED;
	} else if (curval == 0.0) {
	    *type = OUTRANGE;
	    return OUTRANGE;
	}
    }
    if (noautoscale)
	return 0;  /* this plot is not being used for autoscaling */
    if (*type != INRANGE)
	return 0;  /* don't set y range if x is outrange, for example */
    if ((curval < axis->min)
    &&  ((curval <= axis->max) || (axis->max == -VERYLARGE))) {
	if (axis->autoscale & AUTOSCALE_MIN)	{
	    axis->min = curval;
	    if (axis->min_constraint & CONSTRAINT_LOWER) {
		if (axis->min_lb > curval) {
		    axis->min = axis->min_lb;
		    *type = OUTRANGE;
		    return OUTRANGE;
		}
	    }
	} else if (curval != axis->max) {
	    *type = OUTRANGE;
	    return OUTRANGE;
	}
    }
    if ( curval > axis->max
    &&  (curval >= axis->min || axis->min == VERYLARGE)) {
	if (axis->autoscale & AUTOSCALE_MAX)	{
	    axis->max = curval;
	    if (axis->max_constraint & CONSTRAINT_UPPER) {
		if (axis->max_ub < curval) {
		    axis->max = axis->max_ub;
		    *type = OUTRANGE;
		    return OUTRANGE;
		}
	    }
	} else if (curval != axis->min) {
	    *type = OUTRANGE;
	}
    }
    /* Only update data min/max if the point is INRANGE Jun 2016 */
    if (*type == INRANGE) {
	if (axis->data_min > curval)
	    axis->data_min = curval;
	if (axis->data_max < curval)
	    axis->data_max = curval;
    }
    return 0;
}

/* Simplest form of autoscaling (no check on autoscale constraints).
 * Used by refresh_bounds() and refresh_3dbounds().
 * Used also by autoscale_boxplot.
 * FIXME:  Reversed axes are skipped because not skipping them causes errors
 *         if apply_zoom->refresh_request->refresh_bounds->autoscale_one_point.
 *	   But really autoscaling shouldn't be done at all in that case.
 */
void
autoscale_one_point(struct axis *axis, double x)
{
    if (!(axis->range_flags & RANGE_IS_REVERSED)) {
	if (axis->set_autoscale & AUTOSCALE_MIN && x < axis->min)
	    axis->min = x;
	if (axis->set_autoscale & AUTOSCALE_MAX && x > axis->max)
	    axis->max = x;
    }
}
