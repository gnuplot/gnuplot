#ifndef lint
static char *RCSid() { return RCSid("$Id: axis.c,v 1.5 2001/01/16 20:56:08 broeker Exp $"); }
#endif

/* GNUPLOT - axis.c */

/*[
 * Copyright 2000   Thomas Williams, Colin Kelley
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

#include "command.h"
#include "gadgets.h"
#include "gp_time.h"
/*  #include "setshow.h" */
#include "term_api.h"
#include "variable.h"

/* HBB 20000416: this is the start of my try to centralize everything
 * related to axes, once and for all. It'll probably end up as a
 * global array of OO-style 'axis' objects, when it's done */

/* HBB 20000725: gather all per-axis variables into a struct, and set
 * up a single large array of such structs. Next step might be to use
 * isolated AXIS structs, instead of an array. At least for *some* of
 * the axes... */
AXIS axis_array[AXIS_ARRAY_SIZE]
    = AXIS_ARRAY_INITIALIZER(DEFAULT_AXIS_STRUCT);

/* Keep defaults varying by axis in their own array, to ease initialization
 * of the main array */
AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE] = {
    { -10, 10, "z" , TICS_ON_BORDER,               }, 
    { -10, 10, "y" , TICS_ON_BORDER | TICS_MIRROR, }, 
    { -10, 10, "x" , TICS_ON_BORDER | TICS_MIRROR, }, 
    { - 5,  5, "t" , NO_TICS,                      }, 
    { -10, 10, "z2", NO_TICS,                      },
    { -10, 10, "y2", NO_TICS,                      },
    { -10, 10, "x2", NO_TICS,                      },
    { - 0, 10, "r" , NO_TICS,                      }, 
    { - 5,  5, "u" , NO_TICS,                      }, 
    { - 5,  5, "v" , NO_TICS,                      }, 
};


/* either xformat etc or invented time format
 * index with FIRST_X_AXIS etc
 * global because used in gen_tics, which graph3d also uses
 */
static char ticfmt[AXIS_ARRAY_SIZE][MAX_ID_LEN+1];
static int timelevel[AXIS_ARRAY_SIZE];
static double ticstep[AXIS_ARRAY_SIZE];

/* HBB 20000506 new variable: parsing table for use with the table
 * module, to help generalizing set/show/unset/save, where possible */
struct gen_table axisname_tbl[AXIS_ARRAY_SIZE + 1] =
{
    { "z", FIRST_Z_AXIS},
    { "y", FIRST_Y_AXIS},
    { "x", FIRST_X_AXIS},
    { "t", T_AXIS},
    { "z2",SECOND_Z_AXIS},
    { "y2",SECOND_Y_AXIS},
    { "x2",SECOND_X_AXIS},
    { "r", R_AXIS},
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
/* static */ int tic_start, tic_direction, tic_text,
    rotate_tics, tic_hjust, tic_vjust, tic_mirror;

const struct ticdef default_axis_ticdef = DEFAULT_AXIS_TICDEF;

double ticscale = 1.0;		/* scale factor for tic mark */
double miniticscale = 0.5;	/* and for minitics */
TBOOLEAN tic_in = TRUE;		/* tics on inside of coordinate box? */

/* axis labels */
const label_struct default_axis_label = EMPTY_LABELSTRUCT;

/* zeroaxis drawing */
const lp_style_type default_axis_zeroaxis = DEFAULT_AXIS_ZEROAXIS;

/* grid drawing */  
int grid_selection = GRID_OFF;
#ifdef PM3D
# define DEFAULT_GRID_LP { 0, -1, 0, 1.0, 1.0, 0 }
#else
# define DEFAULT_GRID_LP { 0, -1, 0, 1.0, 1.0 }
#endif
const struct lp_style_type default_grid_lp = DEFAULT_GRID_LP;
struct lp_style_type grid_lp   = DEFAULT_GRID_LP;
struct lp_style_type mgrid_lp  = DEFAULT_GRID_LP;
double polar_grid_angle = 0;	/* nonzero means a polar grid */

/* axes being used by the current plot */
/* These are mainly convenience variables, replacing separate copies of
 * such variables originally found in the 2D and 3D plotting code */
AXIS_INDEX x_axis = FIRST_X_AXIS;
AXIS_INDEX y_axis = FIRST_Y_AXIS;
AXIS_INDEX z_axis = FIRST_Z_AXIS;


/* --------- internal prototypes ------------------------- */
static double dbl_raise __PROTO((double x, int y));
static double make_ltic __PROTO((int, double));
static double make_tics __PROTO((AXIS_INDEX, int));
static void   mant_exp __PROTO((double, double, TBOOLEAN, double *, int *, const char *));
static double time_tic_just __PROTO((int, double));

/* ---------------------- routines ----------------------- */

/* check range and take logs of min and max if logscale
 * this also restores min and max for ranges like [10:-10]
 */
#define LOG_MSG(x) x " range must be greater than 0 for scale"

/* this is used in a few places all over the code: undo logscaling of
 * a given range if necessary. If checkrange is TRUE, will int_error() if
 * range is invalid */
void
axis_unlog_interval(axis, min, max, checkrange)
    AXIS_INDEX axis;
    double *min, *max;
    TBOOLEAN checkrange;
{
    if (axis_array[axis].log) {
	if (checkrange && (*min<= 0.0 || *max <= 0.0))
	    int_error(NO_CARET,
		      "%s range must be greater than 0 for log scale",
		      axis_defaults[axis].name);
	*min = (*min<=0) ? -VERYLARGE : AXIS_DO_LOG(axis,*min);
	*max = (*max<=0) ? -VERYLARGE : AXIS_DO_LOG(axis,*max);
    }
} 

void
axis_revert_and_unlog_range(axis)
    AXIS_INDEX axis;
{
  if (axis_array[axis].reverse_range) {
    double temp = axis_array[axis].min;
    axis_array[axis].min = axis_array[axis].max;
    axis_array[axis].max = temp;
  }
  axis_unlog_interval(axis, &axis_array[axis].min, &axis_array[axis].max, 1);
}


/*{{{  axis_log_value_checked() */
double
axis_log_value_checked(axis, coord, what)
     AXIS_INDEX axis;
     double coord;		/* the value */
     const char *what;		/* what is the coord for? */
{
    if (axis_array[axis].log) {
	if (coord <= 0.0) {
	    graph_error("%s has %s coord of %g; must be above 0 for log scale!",
			what, axis_defaults[axis].name, coord);
	} else
	    return (AXIS_DO_LOG(axis,coord));
    }
    return (coord);
}

/*}}} */

/*{{{  axis_checked_extend_empty_range() */
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
axis_checked_extend_empty_range(axis,mesg)
     AXIS_INDEX axis;
     const char *mesg;
{
    /* These two macro definitions set the range-widening policy: */

    /* widen [0:0] by +/- this absolute amount */
#define FIXUP_RANGE__WIDEN_ZERO_ABS	1.0
    /* widen [nonzero:nonzero] by -/+ this relative amount */
#define FIXUP_RANGE__WIDEN_NONZERO_REL	0.01

    double dmin = axis_array[axis].min;
    double dmax = axis_array[axis].max;

    /* HBB 20000501: this same action was taken just before most of
     * the invocations of this function, so I moved it into here.
     * Only do this if 'mesg' is non-NULL --> pass NULL if you don't
     * want the test */
    if (mesg
	&& (axis_array[axis].min == VERYLARGE
	    || axis_array[axis].max == -VERYLARGE))
	int_error(c_token, mesg);

    if (dmax - dmin == 0.0) {
	/* empty range */
	if (axis_array[axis].autoscale) {
	    /* range came from autoscaling ==> widen it */
	    double widen = (dmax == 0.0) ?
		FIXUP_RANGE__WIDEN_ZERO_ABS 
		: FIXUP_RANGE__WIDEN_NONZERO_REL * dmax;
	    fprintf(stderr, "Warning: empty %s range [%g:%g], ",
		    axis_defaults[axis].name, dmin, dmax);
	    axis_array[axis].min -= widen;
	    axis_array[axis].max += widen;
	    fprintf(stderr, "adjusting to [%g:%g]\n",
		    axis_array[axis].min, axis_array[axis].max);
	} else {
	    /* user has explicitly set the range (to something empty)
               ==> we're in trouble */
	    /* FIXME HBB 20000416: is c_token always set properly,
	     * when this is called? We might be better off using
	     * NO_CARET..., here */
	    int_error(c_token, "Can't plot with an empty %s range!",
		      axis_defaults[axis].name);
	}
    }
}

/*}}} */

/* make smalltics for time-axis */
static double
make_ltic(tlevel, incr)
int tlevel;
double incr;
{
    double tinc = 0.0;

    if (tlevel < 0)
	tlevel = 0;
    switch (tlevel) {
    case 0:
    case 1:{
	    if (incr >= 20)
		tinc = 10;
	    if (incr >= 60)
		tinc = 30;
	    if (incr >= 2 * 60)
		tinc = 60;
	    if (incr >= 5 * 60)
		tinc = 2 * 60;
	    if (incr >= 10 * 60)
		tinc = 5 * 60;
	    if (incr >= 20 * 60)
		tinc = 10 * 60;
	    break;
	}
    case 2:{
	    if (incr >= 20 * 60)
		tinc = 10 * 60;
	    if (incr >= 3600)
		tinc = 30 * 60;
	    if (incr >= 2 * 3600)
		tinc = 3600;
	    if (incr >= 5 * 3600)
		tinc = 2 * 3600;
	    if (incr >= 10 * 3600)
		tinc = 5 * 3600;
	    if (incr >= 20 * 3600)
		tinc = 10 * 3600;
	    break;
	}
    case 3:{
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
	}
    case 4:{			/* week: tic per day */
	    if (incr > 2 * DAY_SEC)
		tinc = DAY_SEC;
	    if (incr > 7 * DAY_SEC)
		tinc = 7 * DAY_SEC;
	    break;
	}
    case 5:{			/* month */
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
	}
    case 6:{			/* year */
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
    }
    return (tinc);
}

/*{{{  mant_exp - split into mantissa and/or exponent */
/* HBB 20010121: added code that attempts to fix rounding-induced
 * off-by-one errors in 10^%T and similar output formats */
static void
mant_exp(log10_base, x, scientific, m, p, format)
    double log10_base, x;
    TBOOLEAN scientific;	/* round to power of 3 */
    double *m;			/* results */
    int *p;			
    const char *format;		/* format string for fixup */
{
    int sign = 1;
    double l10;
    int power;
    double mantissa;

    /*{{{  check 0 */
    if (x == 0) {
	if (m)
	    *m = 0;
	if (p)
	    *p = 0;
	return;
    }
    /*}}} */
    /*{{{  check -ve */
    if (x < 0) {
	sign = (-1);
	x = (-x);
    }
    /*}}} */

    l10 = log10(x) / log10_base;
    power = floor(l10);
    mantissa = pow(10.0, (l10 - power) * log10_base);

    /* round power to an integer multiple of 3, to get what's
     * sometimes called 'scientific' or 'engineering' notation. Also
     * useful for handling metric unit prefixes like 'kilo' or 'micro'
     * */
    /* HBB 20010121: avoid recalculation of mantissa via pow() */
    if (scientific) {
	int temp_power  = 3 * floor(power / 3.0);
	switch (power - temp_power) {
	case 2:
	    mantissa *= 100; break;
	case 1:
	    mantissa *= 10; break;
	case 0:
	    break;
	default:
	    int_error (NO_CARET, "Internal error in scientific number formatting");
	}
	power = temp_power;
    }

    /* HBB 20010121: new code for decimal mantissa fixups.  Looks at
     * format string to see how many decimals will be put there.  Iff
     * the number is so close to an exact power of ten that it will be
     * rounded up to 10.0e??? by an sprintf() with that many digits of
     * precision, increase the power by 1 to get a mantissa in the
     * region of 1.0.  If this handling is not wanted, pass NULL as
     * the format string */
    if (format) {
	double upper_border = scientific ? 1000 : 10;
	int precision = 0;

	format = strchr (format, '.');
	if (format != NULL)
	    /* a decimal point was found in the format, so use that 
	     * precision. */
	    precision = strtol(format + 1, NULL, 10);
	
	/* See if mantissa would be right on the border.  All numbers
	 * greater than that will be rounded up to 10, by sprintf(), which
	 * we want to avoid. */
	if (mantissa > (upper_border - pow(10.0, -precision) / 2)
	    ) {
	    mantissa /= upper_border;
	    power += (scientific ? 3 : 1);
	}
    }

    if (m)
	*m = sign * mantissa;
    if (p)
	*p = power;
}

/*}}} */

/*
 * Kludge alert!!
 * Workaround until we have a better solution ...
 * Note: this assumes that all calls to sprintf in gprintf have
 * exactly three args. Lars
 */
#ifdef HAVE_SNPRINTF
# define sprintf(str,fmt,arg) \
    if (snprintf((str),count,(fmt),(arg)) > count) \
      fprintf (stderr,"%s:%d: Warning: too many digits for format\n",__FILE__,__LINE__)
#endif

/*{{{  gprintf */
/* extended s(n)printf */
/* HBB 20010121: added code to maintain consistency between mantissa
 * and exponent across sprintf() calls.  The problem: format string
 * '%t*10^%T' will display 9.99 as '10.0*10^0', but 10.01 as
 * '1.0*10^1'.  This causes problems for people using the %T part,
 * only, with logscaled axes, in combination with the occasional
 * round-off error. */
void
gprintf(dest, count, format, log10_base, x)
    char *dest, *format;
    size_t count;
    double log10_base, x;
{
    char temp[MAX_LINE_LEN + 1];
    char *t;
    TBOOLEAN seen_mantissa = FALSE; /* memorize if mantissa has been
                                       output, already */
    int stored_power = 0;	/* power that matches the mantissa
                                   output earlier */

    for (;;) {
	/*{{{  copy to dest until % */
	while (*format != '%')
	    if (!(*dest++ = *format++))
		return;		/* end of format */
	/*}}} */

	/*{{{  check for %% */
	if (format[1] == '%') {
	    *dest++ = '%';
	    format += 2;
	    continue;
	}
	/*}}} */

	/*{{{  copy format part to temp, excluding conversion character */
	t = temp;
	*t++ = '%';
	/* dont put isdigit first since sideeffect in macro is bad */
	while (*++format == '.' || isdigit((int) *format)
	       || *format == '-' || *format == '+' || *format == ' ')
	    *t++ = *format;
	/*}}} */

	/*{{{  convert conversion character */
	switch (*format) {
	    /*{{{  x and o */
	case 'x':
	case 'X':
	case 'o':
	case 'O':
	    t[0] = *format;
	    t[1] = 0;
	    sprintf(dest, temp, (int) x);
	    break;
	    /*}}} */
	    /*{{{  e, f and g */
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
	    t[0] = *format;
	    t[1] = 0;
	    sprintf(dest, temp, x);
	    break;
	    /*}}} */
	    /*{{{  l --- mantissa to current log base */
	case 'l':
	    {
		double mantissa;

		t[0] = 'f';
		t[1] = 0;
		mant_exp(log10_base, x, FALSE, &mantissa, NULL, NULL);
		sprintf(dest, temp, mantissa);
		break;
	    }
	    /*}}} */
	    /*{{{  t --- base-10 mantissa */
	case 't':
	    {
		double mantissa;

		t[0] = 'f';
		t[1] = 0;
		mant_exp(1.0, x, FALSE, &mantissa, &stored_power, temp);
		seen_mantissa = TRUE;
		sprintf(dest, temp, mantissa);
		break;
	    }
	    /*}}} */
	    /*{{{  s --- base-1000 / 'scientific' mantissa */
	case 's':
	    {
		double mantissa;

		t[0] = 'f';
		t[1] = 0;
		mant_exp(1.0, x, TRUE, &mantissa, &stored_power, temp);
		seen_mantissa = TRUE;
		sprintf(dest, temp, mantissa);
		break;
	    }
	    /*}}} */
	    /*{{{  L --- power to current log base */
	case 'L':
	    {
		int power;

		t[0] = 'd';
		t[1] = 0;
		mant_exp(log10_base, x, FALSE, NULL, &power, NULL);
		sprintf(dest, temp, power);
		break;
	    }
	    /*}}} */
	    /*{{{  T --- power of ten */
	case 'T':
	    {
		int power;

		t[0] = 'd';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else 
		    mant_exp(1.0, x, FALSE, NULL, &power, "%.0f");
		sprintf(dest, temp, power);
		break;
	    }
	    /*}}} */
	    /*{{{  S --- power of 1000 / 'scientific' */
	case 'S':
	    {
		int power;

		t[0] = 'd';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else 
		    mant_exp(1.0, x, TRUE, NULL, &power, "%.0f");
		sprintf(dest, temp, power);
		break;
	    }
	    /*}}} */
	    /*{{{  c --- ISO decimal unit prefix letters */
	case 'c':
	    {
		int power;

		t[0] = 'c';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else 
		    mant_exp(1.0, x, TRUE, NULL, &power, "%.0f");

		if (power >= -18 && power <= 18) {
		    /* -18 -> 0, 0 -> 6, +18 -> 12, ... */
		    /* HBB 20010121: avoid division of -ve ints! */
		    power = (power + 18) / 3;
		    sprintf(dest, temp, "afpnum kMGTPE"[power]);
		} else {
		    /* please extend the range ! */
		    /* name  power   name  power
		       -------------------------
		       atto   -18    Exa    18
		       femto  -15    Peta   15
		       pico   -12    Tera   12
		       nano    -9    Giga    9
		       micro   -6    Mega    6
		       milli   -3    kilo    3   */

		    /* for the moment, print e+21 for example */
		    sprintf(dest, "e%+02d", (power - 6) * 3);
		}

		break;
	    }
	    /*}}} */
	    /*{{{  P --- multiple of pi */
	case 'P':
	    {
		t[0] = 'f';
		t[1] = 0;
		sprintf(dest, temp, x / M_PI);
		break;
	    }
	    /*}}} */
	default:
	    int_error(NO_CARET, "Bad format character");
	} /* switch */
	/*}}} */

	/* this was at the end of every single case, before: */
	dest += strlen(dest);
	++format;
    } /* for ever */
}

/*}}} */
#ifdef HAVE_SNPRINTF
# undef sprintf
#endif

/*{{{  timetic_format() */
void
timetic_format(axis, amin, amax)
     AXIS_INDEX axis;
     double amin, amax;
{
    struct tm t_min, t_max;

    *ticfmt[axis] = 0;		/* make sure we strcat to empty string */

    ggmtime(&t_min, (double) time_tic_just(timelevel[axis], amin));
    ggmtime(&t_max, (double) time_tic_just(timelevel[axis], amax));

    if (t_max.tm_year == t_min.tm_year && t_max.tm_yday == t_min.tm_yday) {
	/* same day, skip date */
	if (t_max.tm_hour != t_min.tm_hour) {
	    strcpy(ticfmt[axis], "%H");
	}
	if (timelevel[axis] < 3) {
	    if (strlen(ticfmt[axis]))
		strcat(ticfmt[axis], ":");
	    strcat(ticfmt[axis], "%M");
	}
	if (timelevel[axis] < 2) {
	    strcat(ticfmt[axis], ":%S");
	}
    } else {
	if (t_max.tm_year != t_min.tm_year) {
	    /* different years, include year in ticlabel */
	    /* check convention, day/month or month/day */
	    if (strchr(axis_array[axis].timefmt, 'm')
		< strchr(axis_array[axis].timefmt, 'd')) {
		strcpy(ticfmt[axis], "%m/%d/%");
	    } else {
		strcpy(ticfmt[axis], "%d/%m/%");
	    }
	    if (((int) (t_max.tm_year / 100)) != ((int) (t_min.tm_year / 100))) {
		strcat(ticfmt[axis], "Y");
	    } else {
		strcat(ticfmt[axis], "y");
	    }

	} else {
	    if (strchr(axis_array[axis].timefmt, 'm')
		< strchr(axis_array[axis].timefmt, 'd')) {
		strcpy(ticfmt[axis], "%m/%d");
	    } else {
		strcpy(ticfmt[axis], "%d/%m");
	    }
	}
	if (timelevel[axis] < 4) {
	    strcat(ticfmt[axis], "\n%H:%M");
	}
    }
}

/*}}} */

/*{{{  dbl_raise() used by set_tics */
/* FIXME HBB 20000426: is this really useful? */
static double
dbl_raise(x, y)
double x;
int y;
{
    register int i = abs(y);
    double val = 1.0;

    while (--i >= 0)
	val *= x;

    if (y < 0)
	return (1.0 / val);
    return (val);
}

/*}}} */

/*{{{  set_tic() */
/* the guide parameter was intended to allow the number of tics
 * to depend on the relative sizes of the plot and the font.
 * It is the approximate upper limit on number of tics allowed.
 * But it did not go down well with the users.
 * A value of 20 gives the same behaviour as 3.5, so that is
 * hardwired into the calls to here. Maybe we will restore it
 * to the automatic calculation one day
 */

double
set_tic(l10, guide)
     double l10;
     int guide;
{
    double xnorm, tics, posns;

    int fl = (int) floor(l10);
    xnorm = pow(10.0, l10 - fl);	/* approx number of decades */

    posns = guide / xnorm;	/* approx number of tic posns per decade */

    if (posns > 40)
	tics = 0.05;		/* eg 0, .05, .10, ... */
    else if (posns > 20)
	tics = 0.1;		/* eg 0, .1, .2, ... */
    else if (posns > 10)
	tics = 0.2;		/* eg 0,0.2,0.4,... */
    else if (posns > 4)
	tics = 0.5;		/* 0,0.5,1, */
    else if (posns > 1)
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

    return (tics * dbl_raise(10.0, fl));
}

/*}}} */

/*{{{  make_tics() */
static double
make_tics(axis, guide)
     AXIS_INDEX axis;
     int guide;
{
    register double xr, tic, l10;

    xr = fabs(axis_array[axis].min - axis_array[axis].max);

    l10 = log10(xr);
    tic = set_tic(l10, guide);
    if (axis_array[axis].log && tic < 1.0)
	tic = 1.0;
    if (axis_array[axis].is_timedata) {
	struct tm ftm, etm;
	/* this is not fun */
	ggmtime(&ftm, (double) axis_array[axis].min);
	ggmtime(&etm, (double) axis_array[axis].max);

	timelevel[axis] = 0;	/* seconds */
	if (tic > 20) {
	    /* turn tic into units of minutes */
	    tic = set_tic(log10(xr / 60.0), guide) * 60;
	    timelevel[axis] = 1;	/* minutes */
	}
	if (tic > 20 * 60) {
	    /* turn tic into units of hours */
	    tic = set_tic(log10(xr / 3600.0), guide) * 3600;
	    timelevel[axis] = 2;	/* hours */
	}
	if (tic > 2 * 3600) {
	    /* need some tickling */
	    tic = set_tic(log10(xr / (3 * 3600.0)), guide) * 3 * 3600;
	}
	if (tic > 6 * 3600) {
	    /* turn tic into units of days */
	    tic = set_tic(log10(xr / DAY_SEC), guide) * DAY_SEC;
	    timelevel[axis] = 3;	/* days */
	}
	if (tic > 3 * DAY_SEC) {
	    /* turn tic into units of weeks */
	    tic = set_tic(log10(xr / WEEK_SEC), guide) * WEEK_SEC;
	    if (tic < WEEK_SEC) {	/* force */
		tic = WEEK_SEC;
	    }
	    timelevel[axis] = 4;	/* weeks */
	}
	if (tic > 3 * WEEK_SEC) {
	    /* turn tic into units of month */
	    tic = set_tic(log10(xr / MON_SEC), guide) * MON_SEC;
	    if (tic < MON_SEC) {	/* force */
		tic = MON_SEC;
	    }
	    timelevel[axis] = 5;	/* month */
	}
	if (tic > 2 * MON_SEC) {
	    /* turn tic into units of month */
	    tic = set_tic(log10(xr / (3 * MON_SEC)), guide) * 3 * MON_SEC;
	}
	if (tic > 6 * MON_SEC) {
	    /* turn tic into units of years */
	    tic = set_tic(log10(xr / YEAR_SEC), guide) * YEAR_SEC;
	    if (tic < (YEAR_SEC / 2)) {
		tic = YEAR_SEC / 2;
	    }
	    timelevel[axis] = 6;	/* year */
	}
    }
    return (tic);
}

/*}}} */

/* setup_tics allows max number of tics to be specified but users dont
 * like it to change with size and font, so we use value of 20, which
 * is 3.5 behaviour.  Note also that if format is '', yticlin = 0, so
 * this gives division by zero.  */

void
setup_tics(axis, max)
     AXIS_INDEX axis;
     int max;			/* approx max number of slots available */
{
    double tic = 0;

    struct ticdef *ticdef = &(axis_array[axis].ticdef);

    int fixmin = (axis_array[axis].autoscale & 1) != 0;
    int fixmax = (axis_array[axis].autoscale & 2) != 0;

    /* HBB 20000506: if no tics required for this axis, do
     * nothing. This used to be done exactly before each call of
     * setup_tics, anyway... */
    if (! axis_array[axis].ticmode)
	return;

    if (ticdef->type == TIC_SERIES) {
	ticstep[axis] = tic = ticdef->def.series.incr;
	fixmin &= (ticdef->def.series.start == -VERYLARGE);
	fixmax &= (ticdef->def.series.end == VERYLARGE);
    } else if (ticdef->type == TIC_COMPUTED) {
	ticstep[axis] = tic = make_tics(axis, max);
    } else {
	fixmin = fixmax = 0;	/* user-defined, day or month */
    }

    if (fixmin) {
	if (axis_array[axis].min < axis_array[axis].max)
	    axis_array[axis].min = tic * floor(axis_array[axis].min / tic);
	else
	    axis_array[axis].min = tic * ceil(axis_array[axis].min / tic);
    }
    if (fixmax) {
	if (axis_array[axis].min < axis_array[axis].max)
	    axis_array[axis].max = tic * ceil(axis_array[axis].max / tic);
	else
	    axis_array[axis].max = tic * floor(axis_array[axis].max / tic);
    }
    if (axis_array[axis].is_timedata && axis_array[axis].format_is_numeric)
	/* invent one for them */
	timetic_format(axis, axis_array[axis].min, axis_array[axis].max);
    else
	strcpy(ticfmt[axis], axis_array[axis].formatstring);
}

/*{{{  gen_tics */
/* uses global arrays ticstep[], ticfmt[], axis_array[], 
 * we use any of GRID_X/Y/X2/Y2 and  _MX/_MX2/etc - caller is expected
 * to clear the irrelevent fields from global grid bitmask
 * note this is also called from graph3d, so we need GRID_Z too
 */
void
gen_tics(axis, grid, callback)
     AXIS_INDEX axis;		/* FIRST_X_AXIS, etc */
     int grid;			/* GRID_X | GRID_MX etc */
     tic_callback callback;	/* fn to call to actually do the work */
{
    /* separate main-tic part of grid */
    struct lp_style_type lgrd, mgrd;
    /* tic defn */
    struct ticdef *def = &axis_array[axis].ticdef; 
    /* minitics - off/default/auto/explicit */
    int minitics = axis_array[axis].minitics;
    /* minitic frequency */
    double minifreq = axis_array[axis].mtic_freq;
     

    memcpy(&lgrd, &grid_lp, sizeof(struct lp_style_type));
    memcpy(&mgrd, &mgrid_lp, sizeof(struct lp_style_type));
    lgrd.l_type = (grid & (GRID_X | GRID_Y | GRID_X2 | GRID_Y2 | GRID_Z)) ? grid_lp.l_type : -2;
    mgrd.l_type = (grid & (GRID_MX | GRID_MY | GRID_MX2 | GRID_MY2 | GRID_MZ)) ? mgrid_lp.l_type : -2;

    if (def->type == TIC_USER) {	/* special case */
	/*{{{  do user tics then return */
	struct ticmark *mark = def->def.user;
	double uncertain = (axis_array[axis].max - axis_array[axis].min) / 10;
	double ticmin = axis_array[axis].min - SIGNIF * uncertain;
	double internal_max = axis_array[axis].max + SIGNIF * uncertain;
	double log10_base = axis_array[axis].log ? log10(axis_array[axis].base) : 1.0;

	/* polar labels always +ve, and if rmin has been set, they are
	 * relative to rmin. position is as user specified, but must
	 * be translated. I dont think it will work at all for
	 * log scale, so I shan't worry about it !
	 */
	double polar_shift = (polar
			      && !(axis_array[R_AXIS].autoscale & 1))
	    ? axis_array[R_AXIS].min : 0;

	for (mark = def->def.user; mark; mark = mark->next) {
	    char label[64];
	    double internal = AXIS_LOG_VALUE(axis,mark->position);

	    internal -= polar_shift;

	    if (!inrange(internal, ticmin, internal_max))
		continue;

	    if (axis_array[axis].is_timedata)
		gstrftime(label, 24, mark->label ? mark->label : ticfmt[axis], mark->position);
	    else
		gprintf(label, sizeof(label), mark->label ? mark->label : ticfmt[axis], log10_base, mark->position);

	    (*callback) (axis, internal, label, lgrd);
	}

	return;			/* NO MINITICS FOR USER-DEF TICS */
	/*}}} */
    }
    /* series-tics
     * need to distinguish user co-ords from internal co-ords.
     * - for logscale, internal = log(user), else internal = user
     *
     * The minitics are a bit of a drag - we need to distinuish
     * the cases step>1 from step == 1.
     * If step = 1, we are looking at 1,10,100,1000 for example, so
     * minitics are 2,5,8, ...  - done in user co-ordinates
     * If step>1, we are looking at 1,1e6,1e12 for example, so
     * minitics are 10,100,1000,... - done in internal co-ords
     */

    {
	double tic;		/* loop counter */
	double internal;	/* in internal co-ords */
	double user;		/* in user co-ords */
	double start, step, end;
	double lmin = axis_array[axis].min, lmax = axis_array[axis].max;
	double internal_min, internal_max;	/* to allow for rounding errors */
	double ministart = 0, ministep = 1, miniend = 1;	/* internal or user - depends on step */
	int anyticput = 0;	/* for detection of infinite loop */

	/* gprintf uses log10() of base - log_base_array is log() */
	double log10_base = axis_array[axis].log ? log10(axis_array[axis].base) : 1.0;

	if (lmax < lmin) {
	    /* hmm - they have set reversed range for some reason */
	    double temp = lmin;
	    lmin = lmax;
	    lmax = temp;
	}
	/*{{{  choose start, step and end */
	switch (def->type) {
	case TIC_SERIES:
	    if (axis_array[axis].log) {
		/* we can tolerate start <= 0 if step and end > 0 */
		if (def->def.series.end <= 0 || def->def.series.incr <= 0)
		    return;	/* just quietly ignore */
		step = AXIS_DO_LOG(axis, def->def.series.incr);
		if (def->def.series.start <= 0)	/* includes case 'undefined, i.e. -VERYLARGE */
		    start = step * floor(lmin / step);
		else
		    start = AXIS_DO_LOG(axis, def->def.series.start);
		if (def->def.series.end == VERYLARGE)
		    end = step * ceil(lmax / step);
		else
		    end = AXIS_DO_LOG(axis, def->def.series.end);
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
	    /* round to multiple of step */
	    start = ticstep[axis] * floor(lmin / ticstep[axis]);
	    step = ticstep[axis];
	    end = ticstep[axis] * ceil(lmax / ticstep[axis]);
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
	    graph_error("Internal error : unknown tic type");
	    return;		/* avoid gcc -Wall warning about start */
	}
	/*}}} */

	/*{{{  ensure ascending order */
	if (end < start) {
	    double temp;
	    temp = end;
	    end = start;
	    start = temp;
	}
	step = fabs(step);
	/*}}} */

	if (minitics) {
	    /*{{{  figure out ministart, ministep, miniend */
	    if (minitics == MINI_USER) {
		/* they have said what they want */
		if (minifreq <= 0)
		    minitics = 0;	/* not much else we can do */
/* 		else if (axis_array[axis].log) { */
/* 		    ministart = ministep = step / minifreq * axis_array[axis].base; */
/* 		    miniend = step * base_array[axis]; */
/* 		} else { */
		ministart = ministep = step / minifreq;
		miniend = step;
/* 		} */
	    } else if (axis_array[axis].log) {
		if (step > 1.5) {	/* beware rounding errors */
		    /*{{{  10,100,1000 case */
		    /* no more than five minitics */
		    ministart = ministep = (int) (0.2 * step);
		    if (ministep < 1)
			ministart = ministep = 1;
		    miniend = step;
		    /*}}} */
		} else {
		    /*{{{  2,5,8 case */
		    miniend = axis_array[axis].base;
		    if (end - start >= 10)
			minitics = 0;	/* none */
		    else if (end - start >= 5) {
			ministart = 2;
			ministep = 3;
		    } else {
			ministart = 2;
			ministep = 1;
		    }
		    /*}}} */
		}
	    } else if (axis_array[axis].is_timedata) {
		ministart = ministep = make_ltic(timelevel[axis], step);
		miniend = step * 0.9;
	    } else if (minitics == MINI_AUTO) {
		ministart = ministep = 0.1 * step;
		miniend = step;
	    } else
		minitics = 0;

	    if (ministep <= 0)
		minitics = 0;	/* dont get stuck in infinite loop */
	    /*}}} */
	}
	/*{{{  a few tweaks and checks */
	/* watch rounding errors */
	end += SIGNIF * step;
	internal_max = lmax + step * SIGNIF;
	internal_min = lmin - step * SIGNIF;

	if (step == 0)
	    return;		/* just quietly ignore them ! */
	/*}}} */

	/* FIXME HBB 20010121: keeping adding 'step' to 'tic' is
	 * begging for rounding errors to strike us. */
	for (tic = start; tic <= end; tic += step) {
	    if (anyticput == 2)	/* See below... */
		break;
	    /* HBB 20010121: Previous code checked absolute value
	     * DBL_EPSILON against tic distance. That's numerical
	     * nonsense. It essentially disallowed series ticmarks for
	     * all axes shorter than DBL_EPSILON in absolute figures.
	     * */
	    if (anyticput && NearlyEqual(tic, start, step)) {
		/* step is too small.. */
		anyticput = 2;	/* Don't try again. */
		tic = end;	/* Put end tic. */
	    } else
		anyticput = 1;

	    /*{{{  calc internal and user co-ords */
	    if (!axis_array[axis].log) {
		internal = (axis_array[axis].is_timedata)
		    ? time_tic_just(timelevel[axis], tic)
		    : tic;
		user = CheckZero(internal, step);
	    } else {
		/* log scale => dont need to worry about zero ? */
		internal = tic;
		user = AXIS_UNDO_LOG(axis, internal);
	    }
	    /*}}} */
	    if (internal > internal_max)
		break;		/* gone too far - end of series = VERYLARGE perhaps */
	    if (internal >= internal_min) {
/* continue; *//* maybe minitics!!!. user series starts below min ? */

		/*{{{  draw tick via callback */
		switch (def->type) {
		case TIC_DAY:{
			int d = (long) floor(user + 0.5) % 7;
			if (d < 0)
			    d += 7;
			(*callback) (axis, internal, abbrev_day_names[d], lgrd);
			break;
		    }
		case TIC_MONTH:{
			int m = (long) floor(user - 1) % 12;
			if (m < 0)
			    m += 12;
			(*callback) (axis, internal, abbrev_month_names[m], lgrd);
			break;
		    }
		default:{	/* comp or series */
			char label[64];
			if (axis_array[axis].is_timedata) {
			    /* If they are doing polar time plot, good luck to them */
			    gstrftime(label, 24, ticfmt[axis], (double) user);
			} else if (polar) {
			    /* if rmin is set, we stored internally with r-rmin */
			    /* HBB 990327: reverted to 'pre-Igor' version... */
#if 1				/* Igor's polar-grid patch */
			    double r = fabs(user) + (axis_array[R_AXIS].autoscale & 1 ? 0 : axis_array[R_AXIS].min);
#else
			    /* Igor removed fabs to allow -ve labels */
			    double r = user + (axis_array[R_AXIS].autoscale & 1 ? 0 : axis_array[R_AXIS].min);
#endif
			    gprintf(label, sizeof(label), ticfmt[axis], log10_base, r);
			} else {
			    gprintf(label, sizeof(label), ticfmt[axis], log10_base, user);
			}
			(*callback) (axis, internal, label, lgrd);
		    }
		}
		/*}}} */

	    }
	    if (minitics) {
		/*{{{  process minitics */
		double mplace, mtic;
		for (mplace = ministart; mplace < miniend; mplace += ministep) {
		    if (axis_array[axis].is_timedata)
			mtic = time_tic_just(timelevel[axis] - 1,
					     internal + mplace);
		    else
			mtic = internal
			    + (axis_array[axis].log && step <= 1.5
			       ? AXIS_DO_LOG(axis,mplace)
			       : mplace);
		    if (inrange(mtic, internal_min, internal_max)
			&& inrange(mtic, start - step * SIGNIF, end + step * SIGNIF))
			(*callback) (axis, mtic, NULL, mgrd);
		}
		/*}}} */
	    }
	}
    }
}

/*}}} */

/* justify ticplace to a proper date-time value */
static double
time_tic_just(level, ticplace)
     int level;
     double ticplace;
{
    struct tm tm;

    if (level <= 0) {
	return (ticplace);
    }
    ggmtime(&tm, (double) ticplace);
    if (level > 0) {		/* units of minutes */
	if (tm.tm_sec > 50)
	    tm.tm_min++;
	tm.tm_sec = 0;
    }
    if (level > 1) {		/* units of hours */
	if (tm.tm_min > 50)
	    tm.tm_hour++;
	tm.tm_min = 0;
    }
    if (level > 2) {		/* units of days */
	if (tm.tm_hour > 14) {
	    tm.tm_hour = 0;
	    tm.tm_mday = 0;
	    tm.tm_yday++;
	    ggmtime(&tm, (double) gtimegm(&tm));
	} else if (tm.tm_hour > 7) {
	    tm.tm_hour = 12;
	} else if (tm.tm_hour > 3) {
	    tm.tm_hour = 6;
	} else {
	    tm.tm_hour = 0;
	}
    }
    /* skip it, I have not bothered with weekday so far */
    if (level > 4) {		/* units of month */
	if (tm.tm_mday > 25) {
	    tm.tm_mon++;
	    if (tm.tm_mon > 11) {
		tm.tm_year++;
		tm.tm_mon = 0;
	    }
	}
	tm.tm_mday = 1;
    }
    if (level > 5) {
	if (tm.tm_mon >= 7)
	    tm.tm_year++;
	tm.tm_mon = 0;
    }
    ticplace = (double) gtimegm(&tm);
    ggmtime(&tm, (double) gtimegm(&tm));
    return (ticplace);
}

/* HBB 20000416: new routine. Code like this appeared 4 times, once
 * per 2D axis, in graphics.c. Always slightly different, of course,
 * but generally, it's always the same. I distinguish two coordinate
 * directions, here. One is the direction of the axis itself (the one
 * it's "running" along). I refer to the one orthogonal to it as
 * "non-running", below. */

void
axis_output_tics(axis, ticlabel_position, zeroaxis_basis, grid_choice,
		 callback)
     AXIS_INDEX axis;		/* axis number we're dealing with */
     int *ticlabel_position;	/* 'non-running' coordinate */
     AXIS_INDEX zeroaxis_basis;	/* axis to base 'non-running' position of
				 * zeroaxis on */
     int grid_choice;		/* bitpattern of grid linesets to draw */
     tic_callback callback;	/* tic-drawing callback function */
{
    struct termentry *t = term;
    TBOOLEAN axis_is_vertical = ((axis % SECOND_AXES) == FIRST_Y_AXIS);
    TBOOLEAN axis_is_second = (axis / SECOND_AXES);
    int axis_position;		/* 'non-running' coordinate */
    int mirror_position;	/* 'non-running' coordinate, 'other' side */

    if (zeroaxis_basis / SECOND_AXES) {
	axis_position = axis_array[zeroaxis_basis].term_upper;
	mirror_position = axis_array[zeroaxis_basis].term_lower;
    } else {
	axis_position = axis_array[zeroaxis_basis].term_lower;
	mirror_position = axis_array[zeroaxis_basis].term_upper;
    }	

    if (axis_array[axis].ticmode) {
	/* set the globals needed by the _callback() function */

	if (axis_array[axis].tic_rotate && (*t->text_angle) (1)) {
	    tic_hjust = axis_is_vertical
		? CENTRE
		: (axis_is_second ? LEFT : RIGHT);
	    tic_vjust = axis_is_vertical
		? (axis_is_second ? JUST_TOP : JUST_BOT)
		: JUST_CENTRE;
	    rotate_tics = 1;
	    /* FIXME HBB 20000501: why would we want this? */
	    if (axis == FIRST_Y_AXIS)
		(*ticlabel_position) += t->v_char / 2;
	} else {
	    tic_hjust = axis_is_vertical
		? (axis_is_second ? LEFT : RIGHT)
		: CENTRE;
	    tic_vjust = axis_is_vertical
		? JUST_CENTRE
		: (axis_is_second ? JUST_BOT : JUST_TOP);
	    rotate_tics = 0;
	}

	if (axis_array[axis].ticmode & TICS_MIRROR)
	    tic_mirror = mirror_position;
	else
	    tic_mirror = -1;	/* no thank you */

	if ((axis_array[axis].ticmode & TICS_ON_AXIS)
	    && !axis_array[zeroaxis_basis].log
	    && inrange(0.0, axis_array[zeroaxis_basis].min,
		       axis_array[zeroaxis_basis].max)
	    ) {
	    tic_start = AXIS_MAP(zeroaxis_basis, 0.0);
	    tic_direction = axis_is_second ? 1 : -1;
	    if (axis_array[axis].ticmode & TICS_MIRROR)
		tic_mirror = tic_start;
	    /* put text at boundary if axis is close to boundary */
	    if (axis_is_vertical) {
		tic_text = (((axis_is_second ? -1 : 1)
			     *(tic_start - axis_position) > (3 * t->h_char))
			    ? tic_start
			    : axis_position
		    ) + (axis_is_second ? 1 : -1) * t->h_char;
	    } else {
		tic_text = (((axis_is_second ? -1 : 1)
			     *(tic_start - axis_position) > (2 * t->v_char))
			    ? tic_start + (axis_is_second
					   ? 0
					   : - ticscale * t->v_tic)
			    : axis_position
		    ) - t->v_char;
	    }
	} else {
	    /* tics not on axis --> on border */
	    tic_start = axis_position;
	    tic_direction = (tic_in ? 1 : -1) * (axis_is_second ? -1 : 1);
	    tic_text = (*ticlabel_position);
	}
	/* go for it */
	gen_tics(axis, grid_selection & grid_choice, callback);
	(*t->text_angle) (0);	/* reset rotation angle */
    }
}
		 
void
axis_set_graphical_range(axis, lower, upper)
    AXIS_INDEX axis;
    unsigned int lower, upper;
{
    axis_array[axis].term_lower = lower;
    axis_array[axis].term_upper = upper;
}

TBOOLEAN 
axis_position_zeroaxis(axis)
     AXIS_INDEX axis;
{
    TBOOLEAN is_inside = FALSE;

    if ((axis_array[axis].min >= 0.0 && axis_array[axis].max >= 0.0)
	|| axis_array[axis].log) {
	/* save for impulse plotting */
	axis_array[axis].term_zero = axis_array[axis].term_lower;	
    } else if (axis_array[axis].min <= 0.0 && axis_array[axis].max <= 0.0) {
	axis_array[axis].term_zero = axis_array[axis].term_upper;
    } else {
	axis_array[axis].term_zero = AXIS_MAP(axis, 0.0);
	is_inside = TRUE;
    }

    return is_inside;
}

void
axis_draw_2d_zeroaxis(axis, crossaxis)
    AXIS_INDEX axis, crossaxis;
{
    if (axis_position_zeroaxis(crossaxis)
	    && (axis_array[axis].zeroaxis.l_type > -3)) {
	term_apply_lp_properties(&axis_array[axis].zeroaxis);
	if ((axis % SECOND_AXES) == FIRST_X_AXIS) {
	    (*term->move) (axis_array[axis].term_lower, axis_array[crossaxis].term_zero);
	    (*term->vector) (axis_array[axis].term_upper, axis_array[crossaxis].term_zero);
	} else {
	    (*term->move) (axis_array[crossaxis].term_zero, axis_array[axis].term_lower);
	    (*term->vector) (axis_array[crossaxis].term_zero, axis_array[axis].term_upper);
	}
    }
}



/* loads a range specification from the input line into variables 'a'
 * and 'b' */
TBOOLEAN
load_range(axis, a, b, autosc)
     AXIS_INDEX axis;
     double *a, *b;
     TBOOLEAN autosc;
{
    if (equals(c_token, "]"))
	return (autosc);
    if (END_OF_COMMAND) {
	int_error(c_token, "starting range value or ':' or 'to' expected");
    } else if (!equals(c_token, "to") && !equals(c_token, ":")) {
	if (equals(c_token, "*")) {
	    autosc |= 1;
	    c_token++;
	} else {
	    GET_NUM_OR_TIME(*a, axis);
	    autosc &= 2;
	}
    }
    if (!equals(c_token, "to") && !equals(c_token, ":"))
	int_error(c_token, "':' or keyword 'to' expected");
    c_token++;
    if (!equals(c_token, "]")) {
	if (equals(c_token, "*")) {
	    autosc |= 2;
	    c_token++;
	} else {
	    GET_NUM_OR_TIME(*b, axis);
	    autosc &= 1;
	}
    }
    return (autosc);
}


/*
 * get and set routines for range writeback
 * ULIG *
 */

double get_writeback_min(axis)
    AXIS_INDEX axis;
{
    /* printf("get min(%d)=%g\n",axis,axis_array[axis].writeback_min); */
    return axis_array[axis].writeback_min;
}

double get_writeback_max(axis)
    AXIS_INDEX axis;
{
    /* printf("get max(%d)=%g\n",axis,axis_array[axis].writeback_min); */
    return axis_array[axis].writeback_max;
}

void set_writeback_min(axis)
    AXIS_INDEX axis;
{
    double val = AXIS_DE_LOG_VALUE(axis,axis_array[axis].min);
    /* printf("set min(%d)=%g\n",axis,val); */
    axis_array[axis].writeback_min = val;
}

void set_writeback_max(axis)
    AXIS_INDEX axis;
{
    double val = AXIS_DE_LOG_VALUE(axis,axis_array[axis].max);
    /* printf("set max(%d)=%g\n",axis,val); */
    axis_array[axis].writeback_max = val;
}

