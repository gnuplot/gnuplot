/* 
 * $Id: axis.c,v 1.5 2000/05/01 23:23:29 hbb Exp $
 *
 */

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
#include "gp_time.h"
#include "setshow.h"

/* HBB 20000614: this is the start of my try to centralize everything
 * related to axes, once and for all. It'll probably end up as a
 * global array of OO-style 'axis' objects, when it's done */

/* Variables originally found in graphics.[ch]: */
double min_array[AXIS_ARRAY_SIZE];
double max_array[AXIS_ARRAY_SIZE];
int auto_array[AXIS_ARRAY_SIZE];
TBOOLEAN log_array[AXIS_ARRAY_SIZE];
double base_array[AXIS_ARRAY_SIZE];
double log_base_array[AXIS_ARRAY_SIZE];

/* internal variables in graphics.c: */
/* scale factors for mapping for each axis */
double scale[AXIS_ARRAY_SIZE];

/* low and high end of the axis on output, in terminal coords: */
unsigned int axis_graphical_lower[AXIS_ARRAY_SIZE];
unsigned int axis_graphical_upper[AXIS_ARRAY_SIZE];
/* axis' zero positions in terminal coords */
unsigned int axis_zero[AXIS_ARRAY_SIZE];	

/* either xformat etc or invented time format
 * index with FIRST_X_AXIS etc
 * global because used in gen_tics, which graph3d also uses
 */
static char ticfmt[AXIS_ARRAY_SIZE][MAX_ID_LEN+1];
static int timelevel[AXIS_ARRAY_SIZE];
static double ticstep[AXIS_ARRAY_SIZE];

static const char *axisname_array[AXIS_ARRAY_SIZE] = {
    "z", "y", "x", "t",
    "z2", "y2", "x2", 
    "r", "u", "v"
};
    
/* penalty for doing tics by callback in gen_tics is need for global
 * variables to communicate with the tic routines. Dont need to be
 * arrays for this */
/* HBB 20000614: they may not need to be array[]ed, but it'd sure
 * make coding easier, in some places... */
/* HBB 20000614: for the testing, these are global... */
/* static */ int tic_start, tic_direction, tic_text,
    rotate_tics, tic_hjust, tic_vjust, tic_mirror;

/* if user specifies [10:-10] we use [-10:10] internally, and swap at end */
int reverse_range[AXIS_ARRAY_SIZE];

/* from set.c:*/
/* assert that AXIS_ARRAY_SIZE is still 10, as the next thing assumes: */
#if AXIS_ARRAY_SIZE != 10
# define AXIS_ARRAY_SIZE  AXIS_ARRAY_SIZE_not_defined
#endif

/* do formats look like times - never saved or shown ...  */
int format_is_numeric[AXIS_ARRAY_SIZE] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1 
};

int range_flags[AXIS_ARRAY_SIZE];	/* = {0,0,...} */

/* array of flags telling if an axis is formatted as time/date, on
 * output (--> 'set xdata time') */
int axis_is_timedata[DATATYPE_ARRAY_SIZE];

/* Define the boundary of the plot
 * These are computed at each call to do_plot, and are constant over
 * the period of one do_plot. They actually only change when the term
 * type changes and when the 'set size' factors change.
 * - no longer true, for 'set key out' or 'set key under'. also depend
 * on tic marks and multi-line labels.
 * They are shared with graph3d.c since we want to use its draw_clip_line()
 */
int xleft, xright, ybot, ytop;

/* --------- internal prototypes ------------------------- */
static double dbl_raise __PROTO((double x, int y));
static void   gprintf __PROTO((char *, size_t, char *, double, double));
static double make_ltic __PROTO((int, double));
static double make_tics __PROTO((AXIS_INDEX, int));
static void   mant_exp __PROTO((double log10_base, double x, int scientific, double *m, int *p));
static void   timetic_format __PROTO((AXIS_INDEX, double, double));
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
    if (log_array[axis]) {
	if (checkrange && (*min<= 0.0 || *max <= 0.0))
	    int_error(NO_CARET,
		      "%s range must be greater than 0 for log scale",
		      axisname_array[axis]);
	*min = (*min<=0) ? -VERYLARGE : AXIS_DO_LOG(axis,*min);
	*max = (*max<=0) ? -VERYLARGE : AXIS_DO_LOG(axis,*max);
    }
} 

void
axis_revert_and_unlog_range(axis)
    AXIS_INDEX axis;
{
  if (reverse_range[axis]) {
    double temp = min_array[axis];
    min_array[axis] = max_array[axis];
    max_array[axis] = temp;
  }
  axis_unlog_interval(axis, &min_array[axis], &max_array[axis], 1);
}


/*{{{  axis_log_value_checked() */
double
axis_log_value_checked(axis, coord, what)
     AXIS_INDEX axis;
     double coord;		/* the value */
     const char *what;		/* what is the coord for? */
{
    if (log_array[axis]) {
	if (coord <= 0.0) {
	    graph_error("%s has %s coord of %g; must be above 0 for log scale!",
			what, axisname_array[axis], coord);
	} else
	    return (AXIS_DO_LOG(axis,coord));
    }
    return (coord);
}

/*}}} */

#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))

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

    double dmin = min_array[axis];
    double dmax = max_array[axis];

    /* HBB 20000501: this same action was taken just before most of
     * the invocations of this function, so I moved it into here.
     * Only do this if 'mesg' is non-NULL --> pass NULL if you don't
     * want the test */
    if (mesg
	&& (min_array[axis] == VERYLARGE
	    || max_array[axis] == -VERYLARGE))
	int_error(c_token, mesg);

    if (dmax - dmin == 0.0) {
	/* empty range */
	if (auto_array[axis]) {
	    /* range came from autoscaling ==> widen it */
	    double widen = (dmax == 0.0) ?
		FIXUP_RANGE__WIDEN_ZERO_ABS 
		: FIXUP_RANGE__WIDEN_NONZERO_REL * dmax;
	    fprintf(stderr, "Warning: empty %s range [%g:%g], ",
		    axisname_array[axis], dmin, dmax);
	    min_array[axis] -= widen;
	    max_array[axis] += widen;
	    fprintf(stderr, "adjusting to [%g:%g]\n",
		    min_array[axis], max_array[axis]);
	} else {
	    /* user has explicitly set the range (to something empty)
               ==> we're in trouble */
	    /* FIXME HBB 20000416: is c_token always set properly,
	     * when this is called? We might be better off using
	     * NO_CARET..., here */
	    int_error(c_token, "Can't plot with an empty %s range!",
		      axisname_array[axis]);
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
static void
mant_exp(log10_base, x, scientific, m, p)
double log10_base, x;
int scientific;			/* round to power of 3 */
double *m;
int *p;				/* results */
{
    int sign = 1;
    double l10;
    int power;
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
    if (scientific) {
	power = 3 * floor(power / 3.0);
    }
    if (m)
	*m = sign * pow(10.0, (l10 - power) * log10_base);
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
static void
gprintf(dest, count, format, log10_base, x)
char *dest, *format;
size_t count;
double log10_base, x;
{
    char temp[MAX_LINE_LEN + 1];
    char *t;

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
	while (*++format == '.' || isdigit((int) *format) || *format == '-' || *format == '+' || *format == ' ')
	    *t++ = *format;
	/*}}} */

	/*{{{  convert conversion character */
	switch (*format) {
	    /*{{{  x and o */
	case 'x':
	case 'X':
	case 'o':
	case 'O':
	    t[0] = *format++;
	    t[1] = 0;
	    sprintf(dest, temp, (int) x);
	    dest += strlen(dest);
	    break;
	    /*}}} */
	    /*{{{  e, f and g */
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
	    t[0] = *format++;
	    t[1] = 0;
	    sprintf(dest, temp, x);
	    dest += strlen(dest);
	    break;
	    /*}}} */
	    /*{{{  l */
	case 'l':
	    {
		double mantissa;
		mant_exp(log10_base, x, 0, &mantissa, NULL);
		t[0] = 'f';
		t[1] = 0;
		sprintf(dest, temp, mantissa);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	    /*{{{  t */
	case 't':
	    {
		double mantissa;
		mant_exp(1.0, x, 0, &mantissa, NULL);
		t[0] = 'f';
		t[1] = 0;
		sprintf(dest, temp, mantissa);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	    /*{{{  s */
	case 's':
	    {
		double mantissa;
		mant_exp(1.0, x, 1, &mantissa, NULL);
		t[0] = 'f';
		t[1] = 0;
		sprintf(dest, temp, mantissa);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	    /*{{{  L */
	case 'L':
	    {
		int power;
		mant_exp(log10_base, x, 0, NULL, &power);
		t[0] = 'd';
		t[1] = 0;
		sprintf(dest, temp, power);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	    /*{{{  T */
	case 'T':
	    {
		int power;
		mant_exp(1.0, x, 0, NULL, &power);
		t[0] = 'd';
		t[1] = 0;
		sprintf(dest, temp, power);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	    /*{{{  S */
	case 'S':
	    {
		int power;
		mant_exp(1.0, x, 1, NULL, &power);
		t[0] = 'd';
		t[1] = 0;
		sprintf(dest, temp, power);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	    /*{{{  c */
	case 'c':
	    {
		int power;
		mant_exp(1.0, x, 1, NULL, &power);
		t[0] = 'c';
		t[1] = 0;
		power = power / 3 + 6;	/* -18 -> 0, 0 -> 6, +18 -> 12, ... */
		if (power >= 0 && power <= 12) {
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

		dest += strlen(dest);
		++format;
		break;
	    }
	case 'P':
	    {
		t[0] = 'f';
		t[1] = 0;
		sprintf(dest, temp, x / M_PI);
		dest += strlen(dest);
		++format;
		break;
	    }
	    /*}}} */
	default:
	    int_error(NO_CARET, "Bad format character");
	}
	/*}}} */
    }
}

/*}}} */
#ifdef HAVE_SNPRINTF
# undef sprintf
#endif

/*{{{  timetic_format() */
static void
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
	    if (strchr(timefmt, 'm') < strchr(timefmt, 'd')) {
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
	    if (strchr(timefmt, 'm') < strchr(timefmt, 'd')) {
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

    xr = fabs(min_array[axis] - max_array[axis]);

    l10 = log10(xr);
    tic = set_tic(l10, guide);
    if (log_array[axis] && tic < 1.0)
	tic = 1.0;
    if (axis_is_timedata[axis]) {
	struct tm ftm, etm;
	/* this is not fun */
	ggmtime(&ftm, (double) min_array[axis]);
	ggmtime(&etm, (double) max_array[axis]);

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

void
setup_tics(axis, ticdef, format, max)
     AXIS_INDEX axis;
     struct ticdef *ticdef;
     char *format;
     int max;			/* approx max number of slots available */
{
    double tic = 0;

    int fixmin = (auto_array[axis] & 1) != 0;
    int fixmax = (auto_array[axis] & 2) != 0;

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
	if (min_array[axis] < max_array[axis])
	    min_array[axis] = tic * floor(min_array[axis] / tic);
	else
	    min_array[axis] = tic * ceil(min_array[axis] / tic);
    }
    if (fixmax) {
	if (min_array[axis] < max_array[axis])
	    max_array[axis] = tic * ceil(max_array[axis] / tic);
	else
	    max_array[axis] = tic * floor(max_array[axis] / tic);
    }
    if ((axis_is_timedata[axis]) && format_is_numeric[axis])
	/* invent one for them */
	timetic_format(axis, min_array[axis], max_array[axis]);
    else
	strcpy(ticfmt[axis], format);
}

/*{{{  gen_tics */
/* uses global arrays ticstep[], ticfmt[], min_array[], max_array[],
 * auto_array[], log_array[], log_base_array[]
 * we use any of GRID_X/Y/X2/Y2 and  _MX/_MX2/etc - caller is expected
 * to clear the irrelevent fields from global grid bitmask
 * note this is also called from graph3d, so we need GRID_Z too
 */
void
gen_tics(axis, def, grid, minitics, minifreq, callback)
     AXIS_INDEX axis;		/* FIRST_X_AXIS, etc */
     struct ticdef *def;	/* tic defn */
     int grid;			/* GRID_X | GRID_MX etc */
     int minitics;		/* minitics - off/default/auto/explicit */
     double minifreq;		/* frequency */
     tic_callback callback;	/* fn to call to actually do the work */
{
    /* separate main-tic part of grid */
    struct lp_style_type lgrd, mgrd;

    memcpy(&lgrd, &grid_lp, sizeof(struct lp_style_type));
    memcpy(&mgrd, &mgrid_lp, sizeof(struct lp_style_type));
    lgrd.l_type = (grid & (GRID_X | GRID_Y | GRID_X2 | GRID_Y2 | GRID_Z)) ? grid_lp.l_type : -2;
    mgrd.l_type = (grid & (GRID_MX | GRID_MY | GRID_MX2 | GRID_MY2 | GRID_MZ)) ? mgrid_lp.l_type : -2;

    if (def->type == TIC_USER) {	/* special case */
	/*{{{  do user tics then return */
	struct ticmark *mark = def->def.user;
	double uncertain = (max_array[axis] - min_array[axis]) / 10;
	double ticmin = min_array[axis] - SIGNIF * uncertain;
	double internal_max = max_array[axis] + SIGNIF * uncertain;
	double log10_base = log_array[axis] ? log10(base_array[axis]) : 1.0;

	/* polar labels always +ve, and if rmin has been set, they are
	 * relative to rmin. position is as user specified, but must
	 * be translated. I dont think it will work at all for
	 * log scale, so I shan't worry about it !
	 */
	double polar_shift = (polar && !(autoscale_r & 1)) ? rmin : 0;

	for (mark = def->def.user; mark; mark = mark->next) {
	    char label[64];
	    double internal = AXIS_LOG_VALUE(axis,mark->position);

	    internal -= polar_shift;

	    if (!inrange(internal, ticmin, internal_max))
		continue;

	    if (axis_is_timedata[axis])
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
	double lmin = min_array[axis], lmax = max_array[axis];
	double internal_min, internal_max;	/* to allow for rounding errors */
	double ministart = 0, ministep = 1, miniend = 1;	/* internal or user - depends on step */
	int anyticput = 0;	/* for detection of infinite loop */

	/* gprintf uses log10() of base - log_base_array is log() */
	double log10_base = log_array[axis] ? log10(base_array[axis]) : 1.0;

	if (lmax < lmin) {
	    /* hmm - they have set reversed range for some reason */
	    double temp = lmin;
	    lmin = lmax;
	    lmax = temp;
	}
	/*{{{  choose start, step and end */
	switch (def->type) {
	case TIC_SERIES:
	    if (log_array[axis]) {
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
/* 		else if (log_array[axis]) { */
/* 		    ministart = ministep = step / minifreq * base_array[axis]; */
/* 		    miniend = step * base_array[axis]; */
/* 		} else { */
		ministart = ministep = step / minifreq;
		miniend = step;
/* 		} */
	    } else if (log_array[axis]) {
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
		    miniend = base_array[axis];
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
	    } else if (axis_is_timedata[axis]) {
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

	for (tic = start; tic <= end; tic += step) {
	    if (anyticput == 2)	/* See below... */
		break;
	    if (anyticput && (fabs(tic - start) < DBL_EPSILON)) {
		/* step is too small.. */
		anyticput = 2;	/* Don't try again. */
		tic = end;	/* Put end tic. */
	    } else
		anyticput = 1;

	    /*{{{  calc internal and user co-ords */
	    if (!log_array[axis]) {
		internal = (axis_is_timedata[axis])
		    ? time_tic_just(timelevel[axis], tic)
		    : tic;
		user = CheckZero(internal, step);
	    } else {
		/* log scale => dont need to worry about zero ? */
		internal = tic;
		user = pow(base_array[axis], internal);
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
			if (axis_is_timedata[axis]) {
			    /* If they are doing polar time plot, good luck to them */
			    gstrftime(label, 24, ticfmt[axis], (double) user);
			} else if (polar) {
			    /* if rmin is set, we stored internally with r-rmin */
			    /* HBB 990327: reverted to 'pre-Igor' version... */
#if 1				/* Igor's polar-grid patch */
			    double r = fabs(user) + (autoscale_r & 1 ? 0 : rmin);
#else
			    /* Igor removed fabs to allow -ve labels */
			    double r = user + (autoscale_r & 1 ? 0 : rmin);
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
		    if (axis_is_timedata[axis])
			mtic = time_tic_just(timelevel[axis] - 1, internal + mplace);
		    else
			mtic = internal
			    + (log_array[axis] && step <= 1.5
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
axis_output_tics(axis, want_tics, want_rotated_tics, axis_position,
		 mirror_position, ticlabel_position, zeroaxis_basis,
		 ticdef, grid_choice, want_minitics, minitic_frequency,
		 callback)
     AXIS_INDEX axis;		/* axis number we're dealing with */
     int want_tics;		/* bit pattern */
     int want_rotated_tics;
     int axis_position;		/* 'non-running' coordinate */
     int mirror_position;	/* 'non-running' coordinate, 'other' side */
     int *ticlabel_position;	/* 'non-running' coordinate */
     AXIS_INDEX zeroaxis_basis;	/* axis to base 'non-running' position of
				 * zeroaxis on */
     struct ticdef *ticdef;	/* Tic definition */
     int grid_choice;		/* bitpattern of grid linesets to draw */
     int want_minitics;		/* what type of minitics (or none) wanted */
     double minitic_frequency;	/* user-specified mtics */
     tic_callback callback;	/* tic-drawing callback function */
{
    struct termentry *t = term;
    TBOOLEAN axis_is_vertical = ((axis % SECOND_AXES) == FIRST_Y_AXIS);
    TBOOLEAN axis_is_second = (axis / SECOND_AXES);

    if (want_tics) {
	/* set the globals needed by the _callback() function */

	if (want_rotated_tics && (*t->text_angle) (1)) {
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
		: (axis_is_second ? JUST_TOP : JUST_BOT);
	    rotate_tics = 0;
	}

	if (want_tics & TICS_MIRROR)
	    tic_mirror = mirror_position;
	else
	    tic_mirror = -1;	/* no thank you */

	if ((want_tics & TICS_ON_AXIS)
	    && !log_array[zeroaxis_basis]
	    && inrange(0.0, min_array[zeroaxis_basis],
		       max_array[zeroaxis_basis])
	    ) {
	    tic_start = AXIS_MAP(zeroaxis_basis, 0.0);
	    tic_direction = axis_is_second ? 1 : -1;
	    if (want_tics & TICS_MIRROR)
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
	gen_tics(axis, ticdef, work_grid.l_type & grid_choice,
		 want_minitics, minitic_frequency, callback);
	(*t->text_angle) (0);	/* reset rotation angle */
    }
}
		 
void
axis_set_graphical_range(axis, lower, upper)
    AXIS_INDEX axis;
    unsigned int lower, upper;
{
    axis_graphical_lower[axis] = lower;
    axis_graphical_upper[axis] = upper;
}

TBOOLEAN 
axis_position_zeroaxis(axis)
     AXIS_INDEX axis;
{
    TBOOLEAN is_inside = FALSE;

    if ((min_array[axis] >= 0.0 && max_array[axis] >= 0.0)
	|| log_array[axis]) {
	/* save for impulse plotting */
	axis_zero[axis] = axis_graphical_lower[axis];	
    } else if (min_array[axis] <= 0.0 && max_array[axis] <= 0.0) {
	axis_zero[axis] = axis_graphical_upper[axis];
    } else {
	axis_zero[axis] = AXIS_MAP(axis, 0.0);
	is_inside = TRUE;
    }

    return is_inside;
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

