#ifndef lint
static char *RCSid() { return RCSid("$Id: graphics.c,v 1.307 2009/07/05 07:11:50 sfeam Exp $"); }
#endif

/* GNUPLOT - graphics.c */

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

/* Daniel Sebald: added plot_image_or_update_axes() routine for images.
 * (5 November 2003)
 */

#include "graphics.h"

#include "color.h"
#include "pm3d.h"
#include "plot.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "misc.h"
#include "gp_time.h"
#include "gadgets.h"
/* FIXME HBB 20010822: this breaks the plan of disentangling graphics
 * and plot2d, because each #include's the other's header: */
#include "plot2d.h"		/* for boxwidth */
#include "term_api.h"
#include "util.h"


/* Externally visible/modifiable status variables */

/* 'set offset' --- artificial buffer zone between coordinate axes and
 * the area actually covered by the data */
t_position loff = {first_axes, 0.0};
t_position roff = {first_axes, 0.0};
t_position toff = {first_axes, 0.0};
t_position boff = {first_axes, 0.0};

/* set bars */
double bar_size = 1.0;
int    bar_layer = LAYER_FRONT;

/* key placement is calculated in boundary, so we need file-wide variables
 * To simplify adjustments to the key, we set all these once [depends on
 * key->reverse] and use them throughout.
 */

/*{{{  local and global variables */
static int key_sample_width;	/* width of line sample */
static int key_sample_left;	/* offset from x for left of line sample */
static int key_sample_right;	/* offset from x for right of line sample */
static int key_point_offset;	/* offset from x for point sample */
static int key_text_left;	/* offset from x for left-justified text */
static int key_text_right;	/* offset from x for right-justified text */
static int key_size_left;	/* size of left bit of key (text or sample, depends on key->reverse) */
static int key_size_right;	/* size of right part of key (including padding) */
static int max_ptitl_len = 0;	/* max length of plot-titles (keys) */
static double ktitl_lines = 0;	/* no lines in key->title (key header) */
static int ptitl_cnt;		/* count keys with len > 0  */
static int key_rows, key_col_wth, yl_ref;

/* set by tic_callback - how large to draw polar radii */
static double largest_polar_circle;

static int xlablin, x2lablin, ylablin, y2lablin, titlelin, xticlin, x2ticlin;

static int key_entry_height;	/* bigger of t->v_size, pointsize*t->v_tick */
static int p_width, p_height;	/* pointsize * { t->h_tic | t->v_tic } */


/* there are several things on right of plot - key, y2tics and y2label
 * when working out boundary, save posn of y2label for later...
 * Same goes for x2label.
 */
static int ylabel_x, y2label_x, xlabel_y, x2label_y, title_y, time_y, time_x;
static int ylabel_y, y2label_y, xtic_y, x2tic_y, ytic_x, y2tic_x;
/*}}} */

/* Status information for stacked histogram plots */
static struct coordinate GPHUGE *stackheight = NULL;	/* top of previous row */
static int stack_count;					/* points actually used */
static void place_histogram_titles __PROTO((void));

/*{{{  static fns and local macros */
static void recheck_ranges __PROTO((struct curve_points * plot));
static void plot_border __PROTO((void));
static void plot_impulses __PROTO((struct curve_points * plot, int yaxis_x, int xaxis_y));
static void plot_lines __PROTO((struct curve_points * plot));
static void plot_points __PROTO((struct curve_points * plot));
static void plot_dots __PROTO((struct curve_points * plot));
static void plot_bars __PROTO((struct curve_points * plot));
static void plot_boxes __PROTO((struct curve_points * plot, int xaxis_y));
static void plot_filledcurves __PROTO((struct curve_points * plot));
static void finish_filled_curve __PROTO((int, gpiPoint *, struct curve_points *));
static void plot_betweencurves __PROTO((struct curve_points * plot));
static void fill_missing_corners __PROTO((gpiPoint *corners, int *points, int exit, int reentry, int updown, int leftright));
static void fill_between __PROTO((double, double, double, double, double, double, double, double, struct curve_points *));
static TBOOLEAN bound_intersect __PROTO((struct coordinate GPHUGE * points, int i, double *ex, double *ey, filledcurves_opts *filledcurves_options));
static void plot_vectors __PROTO((struct curve_points * plot));
static void plot_f_bars __PROTO((struct curve_points * plot));
static void plot_c_bars __PROTO((struct curve_points * plot));

static void place_labels __PROTO((struct text_label * listhead, int layer, TBOOLEAN clip));
static void place_arrows __PROTO((int layer));
static void place_grid __PROTO((void));

static int edge_intersect __PROTO((struct coordinate GPHUGE * points, int i, double *ex, double *ey));
static TBOOLEAN two_edge_intersect __PROTO((struct coordinate GPHUGE * points, int i, double *lx, double *ly));
static TBOOLEAN two_edge_intersect_steps __PROTO((struct coordinate GPHUGE * points, int i, double *lx, double *ly));

static void plot_steps __PROTO((struct curve_points * plot));	/* JG */
static void plot_fsteps __PROTO((struct curve_points * plot));	/* HOE */
static void plot_histeps __PROTO((struct curve_points * plot));	/* CAC */
static void histeps_horizontal __PROTO((int *xl, int *yl, double x1, double x2, double y));	/* CAC */
static void histeps_vertical __PROTO((int *xl, int *yl, double x, double y1, double y2));	/* CAC */
static void edge_intersect_steps __PROTO((struct coordinate GPHUGE * points, int i, double *ex, double *ey));	/* JG */
static void edge_intersect_fsteps __PROTO((struct coordinate GPHUGE * points, int i, double *ex, double *ey));	/* HOE */
static TBOOLEAN two_edge_intersect_steps __PROTO((struct coordinate GPHUGE * points, int i, double *lx, double *ly));	/* JG */
static TBOOLEAN two_edge_intersect_fsteps __PROTO((struct coordinate GPHUGE * points, int i, double *lx, double *ly));

static void boundary __PROTO((struct curve_points * plots, int count));

/* HBB 20010118: these should be static, but can't --- HP-UX assembler bug */
void ytick2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));
void xtick2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));
int histeps_compare __PROTO((SORTFUNC_ARGS p1, SORTFUNC_ARGS p2));

static void get_arrow __PROTO((struct arrow_def* arrow, int* sx, int* sy, int* ex, int* ey));
static void map_position_double __PROTO((struct position* pos, double* x, double* y, const char* what));

static int find_maxl_keys __PROTO((struct curve_points *plots, int count, int *kcnt));

static void do_key_sample __PROTO((struct curve_points *this_plot, legend_key *key,
				   char *title,  struct termentry *t, int xl, int yl));

static TBOOLEAN check_for_variable_color __PROTO((struct curve_points *plot, struct coordinate *point));

#ifdef EAM_OBJECTS
static void plot_circles __PROTO((struct curve_points *plot));
static void do_rectangle __PROTO((int dimensions, t_object *this_object, int style));
#endif

/* for plotting error bars
 * half the width of error bar tic mark
 */
#define ERRORBARTIC GPMAX((t->h_tic/2),1)

/* For tracking exit and re-entry of bounding curves that extend out of plot */
/* these must match the bit values returned by clip_point(). */
#define LEFT_EDGE	1
#define RIGHT_EDGE	2
#define BOTTOM_EDGE	4
#define TOP_EDGE	8

#define clip_fill	((plot->filledcurves_options.closeto == FILLEDCURVES_CLOSED) || clip_lines2)

/*
 * The Amiga SAS/C 6.2 compiler moans about macro envocations causing
 * multiple calls to functions. I converted these macros to inline
 * functions coping with the problem without losing speed.
 * If your compiler supports __inline, you should add it to the
 * #ifdef directive
 * (MGR, 1993)
 */

#ifdef AMIGA_SC_6_1
GP_INLINE static TBOOLEAN
i_inrange(int z, int min, int max)
{
    return ((min < max)
	    ? ((z >= min) && (z <= max))
	    : ((z >= max) && (z <= min)));
}

GP_INLINE static double
f_max(double a, double b)
{
    return (GPMAX(a, b));
}

GP_INLINE static double
f_min(double a, double b)
{
    return (GPMIN(a, b));
}

#else
#define f_max(a,b) GPMAX((a),(b))
#define f_min(a,b) GPMIN((a),(b))
#define i_inrange(z,a,b) inrange((z),(a),(b))
#endif

/* True if a and b have the same sign or zero (positive or negative) */
#define samesign(a,b) ((a) * (b) >= 0)
/*}}} */

/*{{{  more variables */

/* we make a local copy of the 'key' variable so that if something
 * goes wrong, we can switch it off temporarily
 */

static TBOOLEAN lkey;

/*}}} */

static int
find_maxl_keys(struct curve_points *plots, int count, int *kcnt)
{
    int mlen, len, curve, cnt;
    struct curve_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (curve = 0; curve < count; this_plot = this_plot->next, curve++) {
	if (this_plot->title && !this_plot->title_is_suppressed) {
	    ignore_enhanced(this_plot->title_no_enhanced);
	    len = estimate_strlen(this_plot->title);
	    if (len != 0) {
		cnt++;
		if (len > mlen)
		    mlen = len;
	    }
	    ignore_enhanced(FALSE);
	}

	/* Check for new histogram here and save space for divider */
	if (this_plot->plot_style == HISTOGRAMS
	&&  this_plot->histogram_sequence == 0 && cnt > 1)
	    cnt++;
	/* Check for column-stacked histogram with key entries */
	if (this_plot->plot_style == HISTOGRAMS &&  this_plot->labels) {
	    text_label *key_entry = this_plot->labels->next;
	    for (; key_entry; key_entry=key_entry->next) {
		cnt++;
		len = key_entry->text ? estimate_strlen(key_entry->text) : 0;
		if (len > mlen)
		    mlen = len;
	    }
	}
    }

    if (kcnt != NULL)
	*kcnt = cnt;
    return (mlen);
}


/*{{{  boundary() */
/* borders of plotting area
 * computed once on every call to do_plot
 *
 * The order in which things is done is getting pretty critical:
 *  plot_bounds.ytop depends on title, x2label, ylabels (if no rotated text)
 *  plot_bounds.ybot depends on key, if "under"
 *  once we have these, we can setup the y1 and y2 tics and the
 *  only then can we calculate plot_bounds.xleft and plot_bounds.xright
 *  plot_bounds.xright depends also on key RIGHT
 *  then we can do x and x2 tics
 *
 * For set size ratio ..., everything depends on everything else...
 * not really a lot we can do about that, so we lose if the plot has to
 * be reduced vertically. But the chances are the
 * change will not be very big, so the number of tics will not
 * change dramatically.
 *
 * Margin computation redone by Dick Crawford (rccrawford@lanl.gov) 4/98
 */

static void
boundary(struct curve_points *plots, int count)
{
    int yticlin = 0, y2ticlin = 0, timelin = 0;
    legend_key *key = &keyT;

    struct termentry *t = term;
    int key_h, key_w;
    /* FIXME HBB 20000506: this line is the reason for the 'D0,1;D1,0'
     * bug in the HPGL terminal: we actually carry out the switch of
     * text orientation, just for finding out if the terminal can do
     * that. *But* we're not in graphical mode, yet, so this call
     * yields undesirable results */
    int can_rotate = (*t->text_angle) (TEXT_VERTICAL);

    int xtic_textheight;	/* height of xtic labels */
    int x2tic_textheight;	/* height of x2tic labels */
    int title_textheight;	/* height of title */
    int xlabel_textheight;	/* height of xlabel */
    int x2label_textheight;	/* height of x2label */
    int timetop_textheight;	/* height of timestamp (if at top) */
    int timebot_textheight;	/* height of timestamp (if at bottom) */
    int ylabel_textheight;	/* height of (unrotated) ylabel */
    int y2label_textheight;	/* height of (unrotated) y2label */
    int ylabel_textwidth;	/* width of (rotated) ylabel */
    int y2label_textwidth;	/* width of (rotated) y2label */
    int timelabel_textwidth;	/* width of timestamp */
    int ytic_textwidth;		/* width of ytic labels */
    int y2tic_textwidth;	/* width of y2tic labels */
    int x2tic_height;		/* 0 for tic_in or no x2tics, ticscale*v_tic otherwise */
    int xtic_textwidth=0;	/* amount by which the xtic label protrude to the right */
    int xtic_height;
    int ytic_width;
    int y2tic_width;

    int key_cols = 1;		/* # columns of keys */

    /* figure out which rotatable items are to be rotated
     * (ylabel and y2label are rotated if possible) */
    int vertical_timelabel = can_rotate ? timelabel_rotate : 0;
    int vertical_xtics  = can_rotate ? axis_array[FIRST_X_AXIS].tic_rotate : 0;
    int vertical_x2tics = can_rotate ? axis_array[SECOND_X_AXIS].tic_rotate : 0;
    int vertical_ytics  = can_rotate ? axis_array[FIRST_Y_AXIS].tic_rotate : 0;
    int vertical_y2tics = can_rotate ? axis_array[SECOND_Y_AXIS].tic_rotate : 0;

    TBOOLEAN shift_labels_to_border = FALSE;

    lkey = key->visible;	/* but we may have to disable it later */

    xticlin = ylablin = y2lablin = xlablin = x2lablin = titlelin = 0;

    /*{{{  count lines in labels and tics */
    if (title.text)
	label_width(title.text, &titlelin);
    if (axis_array[FIRST_X_AXIS].label.text)
	label_width(axis_array[FIRST_X_AXIS].label.text, &xlablin);

    /* This should go *inside* label_width(), but it messes up the key title */
    /* Imperfect check for subscripts or superscripts */
    if ((term->flags & TERM_ENHANCED_TEXT) && axis_array[FIRST_X_AXIS].label.text
	&& strpbrk(axis_array[FIRST_X_AXIS].label.text, "_^"))
	    xlablin++;

    if (axis_array[SECOND_X_AXIS].label.text)
	label_width(axis_array[SECOND_X_AXIS].label.text, &x2lablin);
    if (axis_array[FIRST_Y_AXIS].label.text)
	label_width(axis_array[FIRST_Y_AXIS].label.text, &ylablin);
    if (axis_array[SECOND_Y_AXIS].label.text)
	label_width(axis_array[SECOND_Y_AXIS].label.text, &y2lablin);

    if (axis_array[FIRST_X_AXIS].ticmode) {
	label_width(axis_array[FIRST_X_AXIS].formatstring, &xticlin);
	/* Reserve room for user tic labels even if format of autoticks is "" */
	if (xticlin == 0 && axis_array[FIRST_X_AXIS].ticdef.def.user)
	    xticlin = 1;
    }

    if (axis_array[SECOND_X_AXIS].ticmode)
	label_width(axis_array[SECOND_X_AXIS].formatstring, &x2ticlin);
    if (axis_array[FIRST_Y_AXIS].ticmode)
	label_width(axis_array[FIRST_Y_AXIS].formatstring, &yticlin);
    if (axis_array[SECOND_Y_AXIS].ticmode)
	label_width(axis_array[SECOND_Y_AXIS].formatstring, &y2ticlin);
    if (timelabel.text)
	label_width(timelabel.text, &timelin);
    /*}}} */

    /*{{{  preliminary plot_bounds.ytop  calculation */

    /*     first compute heights of things to be written in the margin */

    /* title */
    if (titlelin) {
	double tmpx, tmpy;
	map_position_r(&(title.offset), &tmpx, &tmpy, "boundary");
	title_textheight = (int) ((titlelin + 1) * (t->v_char) + tmpy);
    } else
	title_textheight = 0;

    /* x2label */
    if (x2lablin) {
	double tmpx, tmpy;
	map_position_r(&(axis_array[SECOND_X_AXIS].label.offset),
		       &tmpx, &tmpy, "boundary");
	x2label_textheight = (int) (x2lablin * t->v_char + tmpy);
	if (!axis_array[SECOND_X_AXIS].ticmode)
	    x2label_textheight += 0.5 * t->v_char;
    } else
	x2label_textheight = 0;

    /* tic labels */
    if (axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER) {
	/* ought to consider tics on axes if axis near border */
	if (vertical_x2tics) {
	    /* guess at tic length, since we don't know it yet
	       --- we'll fix it after the tic labels have been created */
	    x2tic_textheight = (int) (5 * t->h_char);
	} else
	    x2tic_textheight = (int) (x2ticlin * t->v_char);
    } else
	x2tic_textheight = 0;

    /* tics */
    if (!axis_array[SECOND_X_AXIS].tic_in
	&& ((axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[FIRST_X_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER))))
	x2tic_height = (int) (t->v_tic * axis_array[SECOND_X_AXIS].ticscale);
    else
	x2tic_height = 0;

    /* timestamp */
    if (timelabel.text && !timelabel_bottom) {
	double tmpx, tmpy;
	map_position_r(&(timelabel.offset), &tmpx, &tmpy, "boundary");
	timetop_textheight = (int) ((timelin + 2) * t->v_char + tmpy);
    } else
	timetop_textheight = 0;

    /* horizontal ylabel */
    if (axis_array[FIRST_Y_AXIS].label.text && !can_rotate) {
	double tmpx, tmpy;
	map_position_r(&(axis_array[FIRST_Y_AXIS].label.offset),
		       &tmpx, &tmpy, "boundary");
	ylabel_textheight = (int) (ylablin * t->v_char + tmpy);
    } else
	ylabel_textheight = 0;

    /* horizontal y2label */
    if (axis_array[SECOND_Y_AXIS].label.text && !can_rotate) {
	double tmpx, tmpy;
	map_position_r(&(axis_array[SECOND_Y_AXIS].label.offset),
		       &tmpx, &tmpy, "boundary");
	y2label_textheight = (int) (y2lablin * t->v_char + tmpy);
    } else
	y2label_textheight = 0;

    /* compute plot_bounds.ytop from the various components
     *     unless tmargin is explicitly specified  */

    plot_bounds.ytop = (int) (0.5 + (ysize + yoffset) * (t->ymax-1));

    if (tmargin.scalex == screen) {
	/* Specified as absolute position on the canvas */
	plot_bounds.ytop = (tmargin.x) * (float)(t->ymax-1);
    } else if (tmargin.x >=0) {
	/* Specified in terms of character height */
	plot_bounds.ytop -= (int)(tmargin.x * (float)t->v_char + 0.5);
    } else {
	/* Auto-calculation of space required */
	int top_margin = x2label_textheight + title_textheight;

	if (timetop_textheight + ylabel_textheight > top_margin)
	    top_margin = timetop_textheight + ylabel_textheight;
	if (y2label_textheight > top_margin)
	    top_margin = y2label_textheight;

	top_margin += x2tic_height + x2tic_textheight;
	/* x2tic_height and x2tic_textheight are computed as only the
	 *     relevant heights, but they nonetheless need a blank
	 *     space above them  */
	if (top_margin > x2tic_height)
	    top_margin += (int) t->v_char;

	plot_bounds.ytop -= top_margin;
	if (plot_bounds.ytop >= (ysize + yoffset) * (t->ymax-1)) {
	    /* make room for the end of rotated ytics or y2tics */
	    plot_bounds.ytop -= (int) (t->h_char * 2);
	}
    }

    /*  end of preliminary plot_bounds.ytop calculation }}} */


    /*{{{  preliminary plot_bounds.xleft, needed for "under" */
    if (lmargin.scalex == screen)
	plot_bounds.xleft = lmargin.x * (float)t->xmax;
    else
	plot_bounds.xleft = xoffset * t->xmax
			  + t->h_char * (lmargin.x >= 0 ? lmargin.x : 1);
    /*}}} */


    /*{{{  tentative plot_bounds.xright, needed for "under" */
    if (rmargin.scalex == screen)
	plot_bounds.xright = rmargin.x * (float)(t->xmax - 1);
    else
	plot_bounds.xright = (xsize + xoffset) * (t->xmax - 1)
			   - t->h_char * (rmargin.x >= 0 ? rmargin.x : 2);
    /*}}} */


    /*{{{  preliminary plot_bounds.ybot calculation
     *     first compute heights of labels and tics */

    /* tic labels */
    shift_labels_to_border = FALSE;
    if (axis_array[FIRST_X_AXIS].ticmode & TICS_ON_AXIS) {
	/* FIXME: This test for how close the axis is to the border does not match */
	/*        the tests in axis_output_tics(), and assumes FIRST_Y_AXIS.       */
	if (!inrange(0.0, axis_array[FIRST_Y_AXIS].min, axis_array[FIRST_Y_AXIS].max))
	    shift_labels_to_border = TRUE;
	if (0.05 > fabs( axis_array[FIRST_Y_AXIS].min
		/ (axis_array[FIRST_Y_AXIS].max - axis_array[FIRST_Y_AXIS].min)))
	    shift_labels_to_border = TRUE;
    }
    if ((axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER)
    ||  shift_labels_to_border) {
	/* ought to consider tics on axes if axis near border */
	if (vertical_xtics) {
	    /* guess at tic length, since we don't know it yet */
	    xtic_textheight = (int) (t->h_char * 5);
	} else
	    xtic_textheight = (int) (t->v_char * (xticlin + 1));
    } else
	xtic_textheight =  0;

    /* tics */
    if (!axis_array[FIRST_X_AXIS].tic_in
	&& ((axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[SECOND_X_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER))))
	xtic_height = (int) (t->v_tic * axis_array[FIRST_X_AXIS].ticscale);
    else
	xtic_height = 0;

    /* xlabel */
    if (xlablin) {
	double tmpx, tmpy;
	map_position_r(&(axis_array[FIRST_X_AXIS].label.offset),
		       &tmpx, &tmpy, "boundary");
	/* offset is subtracted because if > 0, the margin is smaller */
	/* textheight is inflated by 0.2 to allow descenders to clear bottom of canvas */
	xlabel_textheight = (((float)xlablin + 0.2) * t->v_char - tmpy);
	if (!axis_array[FIRST_X_AXIS].ticmode)
	    xlabel_textheight += 0.5 * t->v_char;
    } else
	xlabel_textheight = 0;

    /* timestamp */
    if (timelabel.text && timelabel_bottom) {
	/* && !vertical_timelabel)
	 * DBT 11-18-98 resize plot for vertical timelabels too !
	 */
	double tmpx, tmpy;
	map_position_r(&(timelabel.offset), &tmpx, &tmpy, "boundary");
	/* offset is subtracted because if . 0, the margin is smaller */
	timebot_textheight = (int) (timelin * t->v_char - tmpy);
    } else
	timebot_textheight = 0;

    /* compute plot_bounds.ybot from the various components
     *     unless bmargin is explicitly specified  */

    plot_bounds.ybot = yoffset * (float)t->ymax;

    if (bmargin.scalex == screen) {
	/* Absolute position for bottom of plot */
	plot_bounds.ybot = bmargin.x * (float)t->ymax;
    } else if (bmargin.x >= 0) {
	/* Position based on specified character height */
	plot_bounds.ybot += bmargin.x * (float)t->v_char + 0.5;
    } else {
	plot_bounds.ybot += xtic_height + xtic_textheight;
	if (xlabel_textheight > 0)
	    plot_bounds.ybot += xlabel_textheight;
	if (timebot_textheight > 0)
	    plot_bounds.ybot += timebot_textheight;
	/* HBB 19990616: round to nearest integer, required to escape
	 * floating point inaccuracies */
	if (plot_bounds.ybot == (int) (t->ymax * yoffset)) {
	    /* make room for the end of rotated ytics or y2tics */
	    plot_bounds.ybot += (int) (t->h_char * 2);
	}
    }

    /*  end of preliminary plot_bounds.ybot calculation }}} */

    if (lkey) {
	TBOOLEAN key_panic = FALSE;
	/*{{{  essential key features */

	p_width = pointsize * t->h_tic;
	p_height = pointsize * t->v_tic;

	if (key->swidth >= 0)
	    key_sample_width = key->swidth * t->h_char + p_width;
	else
	    key_sample_width = 0;

	key_entry_height = p_height * 1.25 * key->vert_factor;
	if (key_entry_height < t->v_char)
	    key_entry_height = t->v_char * key->vert_factor;
	/* HBB 20020122: safeguard to prevent division by zero later */
	if (key_entry_height == 0)
	    key_entry_height = 1;

	/* Count max_len key and number keys with len > 0 */
	max_ptitl_len = find_maxl_keys(plots, count, &ptitl_cnt);

	/* Key title length and height */
	if (key->title) {
	    int ytlen, ytheight;
	    ytlen = label_width(key->title, &ytheight);
	    ytlen -= key->swidth + 2;
	    /* EAM FIXME */
	    if ((ytlen > max_ptitl_len) && (key->stack_dir != GPKEY_HORIZONTAL))
		max_ptitl_len = ytlen;
	    ktitl_lines = (int)ytheight;
	}

	if (key->reverse) {
	    key_sample_left = -key_sample_width;
	    key_sample_right = 0;
	    /* if key width is being used, adjust right-justified text */
	    key_text_left = t->h_char;
	    key_text_right = t->h_char * (max_ptitl_len + 1 + key->width_fix);
	    key_size_left = t->h_char - key_sample_left; /* sample left is -ve */
	    key_size_right = key_text_right;
	} else {
	    key_sample_left = 0;
	    key_sample_right = key_sample_width;
	    /* if key width is being used, adjust left-justified text */
	    key_text_left = -(int) (t->h_char
				    * (max_ptitl_len + 1 + key->width_fix));
	    key_text_right = -(int) t->h_char;
	    key_size_left = -key_text_left;
	    key_size_right = key_sample_right + t->h_char;
	}
	key_point_offset = (key_sample_left + key_sample_right) / 2;

	/* advance width for cols */
	key_col_wth = key_size_left + key_size_right;

	key_rows = ptitl_cnt;
	key_cols = 1;

	/* calculate rows and cols for key */

	if (key->stack_dir == GPKEY_HORIZONTAL) {
	    /* maximise no cols, limited by label-length */
	    key_cols = (int) (plot_bounds.xright - plot_bounds.xleft) / key_col_wth;
	    /* EAM Dec 2004 - Rather than turn off the key, try to squeeze */
	    if (key_cols == 0) {
		key_cols = 1;
		key_panic = TRUE;
		key_col_wth = (plot_bounds.xright - plot_bounds.xleft);
	    }
	    key_rows = (int) (ptitl_cnt + key_cols - 1) / key_cols;
	    /* now calculate actual no cols depending on no rows */
	    key_cols = (key_rows == 0) ? 1
		: (int) (ptitl_cnt + key_rows - 1) / key_rows;
	    if (key_cols == 0) {
		key_cols = 1;
		key_panic = TRUE;
	    }
	} else {
	    /* maximise no rows, limited by plot_bounds.ytop-plot_bounds.ybot */
	    int i = (int) (plot_bounds.ytop - plot_bounds.ybot - key->height_fix * t->v_char
			   - (ktitl_lines + 1) * t->v_char)
		/ key_entry_height;

	    if (i == 0) {
		i = 1;
		key_panic = TRUE;
	    }
	    if (ptitl_cnt > i) {
		key_cols = (int) (ptitl_cnt + i - 1) / i;
		/* now calculate actual no rows depending on no cols */
		if (key_cols == 0) {
		    key_cols = 1;
		    key_panic = TRUE;
		}
		key_rows = (int) (ptitl_cnt + key_cols - 1) / key_cols;
	    }
	}

	/* adjust for outside key, leave manually set margins alone */
	if ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC && (key->vpos != JUST_CENTRE || key->hpos != CENTRE))
	    || key->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
	    int more = 0;
	    if (key->margin == GPKEY_BMARGIN && bmargin.x < 0) {
		more = key_entry_height * key_rows + (int) (t->v_char * (ktitl_lines + 1))
			+ (int) (key->height_fix * t->v_char);
		if (plot_bounds.ybot + more > plot_bounds.ytop)
		    key_panic = TRUE;
		else
		    plot_bounds.ybot += more;
	    } else if (key->margin == GPKEY_TMARGIN && tmargin.x < 0) {
		more = key_entry_height * key_rows + (int) (t->v_char * (ktitl_lines + 1))
			- (int) (key->height_fix * t->v_char);
		if (plot_bounds.ytop - more < plot_bounds.ybot)
		    key_panic = TRUE;
		else
		    plot_bounds.ytop -= more;
	    } else if (key->margin == GPKEY_LMARGIN && lmargin.x < 0) {
		more = key_col_wth * key_cols;
		if (plot_bounds.xleft + more > plot_bounds.xright)
		    key_panic = TRUE;
		else
		    plot_bounds.xleft += more;
	    } else if (key->margin == GPKEY_RMARGIN && rmargin.x < 0) {
		more = key_col_wth * key_cols;
		if (plot_bounds.xright - more < plot_bounds.xleft)
		    key_panic = TRUE;
		else
		    plot_bounds.xright -= more;
	    }
	}

	/* warn if we had to punt on key size calculations */
	if (key_panic)
	    int_warn(NO_CARET, "Warning - difficulty fitting plot titles into key");

	/*}}} */
    }

    /*{{{  set up y and y2 tics */
    setup_tics(FIRST_Y_AXIS, 20);
    setup_tics(SECOND_Y_AXIS, 20);
    /*}}} */

    /* Adjust color axis limits if necessary. */
    if (is_plot_with_palette()) {
	set_cbminmax();
	axis_checked_extend_empty_range(COLOR_AXIS, "All points of color axis undefined.");
	setup_tics(COLOR_AXIS, 20);
    }

    /*{{{  recompute plot_bounds.xleft based on widths of ytics, ylabel etc
       unless it has been explicitly set by lmargin */

    /* tic labels */
    shift_labels_to_border = FALSE;
    if (axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_AXIS) {
	/* FIXME: This test for how close the axis is to the border does not match */
	/*        the tests in axis_output_tics(), and assumes FIRST_X_AXIS.       */
	if (!inrange(0.0, axis_array[FIRST_X_AXIS].min, axis_array[FIRST_X_AXIS].max))
	    shift_labels_to_border = TRUE;
	if (0.1 > fabs( axis_array[FIRST_X_AXIS].min
	       /  (axis_array[FIRST_X_AXIS].max - axis_array[FIRST_X_AXIS].min)))
	    shift_labels_to_border = TRUE;
    }
    
    if ((axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER)
    ||  shift_labels_to_border) {
	if (vertical_ytics)
	    /* HBB: we will later add some white space as part of this, so
	     * reserve two more rows (one above, one below the text ...).
	     * Same will be done to similar calc.'s elsewhere */
	    ytic_textwidth = (int) (t->v_char * (yticlin + 2));
	else {
	    widest_tic_strlen = 0;	/* reset the global variable ... */
	    /* get gen_tics to call widest_tic_callback with all labels
	     * the latter sets widest_tic_strlen to the length of the widest
	     * one ought to consider tics on axis if axis near border...
	     */
	    gen_tics(FIRST_Y_AXIS, /* 0, */ widest_tic_callback);

	    ytic_textwidth = (int) (t->h_char * (widest_tic_strlen + 2));
	}
    } else {
	ytic_textwidth = 0;
    }

    /* tics */
    if (!axis_array[FIRST_Y_AXIS].tic_in
	&& ((axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[SECOND_Y_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER))))
	ytic_width = (int) (t->h_tic * axis_array[FIRST_Y_AXIS].ticscale);
    else
	ytic_width = 0;

    /* ylabel */
    if (axis_array[FIRST_Y_AXIS].label.text && can_rotate) {
	double tmpx, tmpy;
	map_position_r(&(axis_array[FIRST_Y_AXIS].label.offset),
		       &tmpx, &tmpy, "boundary");
	ylabel_textwidth = (int) (ylablin * (t->v_char) - tmpx);
	if (!axis_array[FIRST_Y_AXIS].ticmode)
	    ylabel_textwidth += 0.5 * t->v_char;
    } else
	/* this should get large for NEGATIVE ylabel.xoffsets  DBT 11-5-98 */
	ylabel_textwidth = 0;

    /* timestamp */
    if (timelabel.text && vertical_timelabel) {
	double tmpx, tmpy;
	map_position_r(&(timelabel.offset), &tmpx, &tmpy, "boundary");
	timelabel_textwidth = (int) ((timelin + 1.5) * t->v_char - tmpx);
    } else
	timelabel_textwidth = 0;

    if (lmargin.x < 0) {	
	/* Auto-calculation */
	double tmpx, tmpy;

	plot_bounds.xleft += (timelabel_textwidth > ylabel_textwidth
		  ? timelabel_textwidth : ylabel_textwidth)
	    + ytic_width + ytic_textwidth;

	/* make sure plot_bounds.xleft is wide enough for a negatively
	 * x-offset horizontal timestamp
	 */
	map_position_r(&(timelabel.offset), &tmpx, &tmpy, "boundary");
	if (!vertical_timelabel
	    && plot_bounds.xleft - ytic_width - ytic_textwidth < -(int) (tmpx))
	    plot_bounds.xleft = ytic_width + ytic_textwidth - (int) (tmpx);
	if (plot_bounds.xleft == (int) (t->xmax * xoffset)) {
	    /* make room for end of xtic or x2tic label */
	    plot_bounds.xleft += (int) (t->h_char * 2);
	}
	/* DBT 12-3-98  extra margin just in case */
	plot_bounds.xleft += 0.5 * t->h_char;
    }
    /* Note: we took care of explicit 'set lmargin foo' at line 492 */

    /*  end of plot_bounds.xleft calculation }}} */

    /*{{{  recompute plot_bounds.xright based on widest y2tic. y2labels, key "outside"
       unless it has been explicitly set by rmargin */

    /* tic labels */
    if (axis_array[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER) {
	if (vertical_y2tics)
	    y2tic_textwidth = (int) (t->v_char * (y2ticlin + 2));
	else {
	    widest_tic_strlen = 0;	/* reset the global variable ... */
	    /* get gen_tics to call widest_tic_callback with all labels
	     * the latter sets widest_tic_strlen to the length of the widest
	     * one ought to consider tics on axis if axis near border...
	     */
	    gen_tics(SECOND_Y_AXIS, /* 0, */ widest_tic_callback);

	    y2tic_textwidth = (int) (t->h_char * (widest_tic_strlen + 2));
	}
    } else {
	y2tic_textwidth = 0;
    }

    /* EAM May 2009
     * Check to see if any xtic labels are so long that they extend beyond
     * the right boundary of the plot. If so, allow extra room in the margin.
     * If the labels are too long to fit even with a big margin, too bad.
     */
    if (axis_array[FIRST_X_AXIS].ticdef.def.user) {
	struct ticmark *tic = axis_array[FIRST_X_AXIS].ticdef.def.user;
	int maxrightlabel = plot_bounds.xright;
	while (tic) {
	    if (tic->label) {
		double xx;
		int length = estimate_strlen(tic->label)
			   * cos(DEG2RAD * (double)(axis_array[FIRST_X_AXIS].tic_rotate))
			   * term->h_char;

		/* We don't really know the plot layout yet, but try for an estimate */
		AXIS_SETSCALE(FIRST_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
		axis_set_graphical_range(FIRST_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
		xx = axis_log_value_checked(FIRST_X_AXIS, tic->position, "xtic");
	        xx = AXIS_MAP(FIRST_X_AXIS, xx);
		xx += (axis_array[FIRST_X_AXIS].tic_rotate) ? length : length /2;
		if (maxrightlabel < xx)
		    maxrightlabel = xx;
	    }
	    tic = tic->next;
	}
	xtic_textwidth = maxrightlabel - plot_bounds.xright;
	if (xtic_textwidth > term->xmax/2) {
	    xtic_textwidth = term->xmax/2;
	    int_warn(NO_CARET, "difficulty making room for xtic labels");
	}
    }

    /* tics */
    if (!axis_array[SECOND_Y_AXIS].tic_in
	&& ((axis_array[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[FIRST_Y_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER))))
	y2tic_width = (int) (t->h_tic * axis_array[SECOND_Y_AXIS].ticscale);
    else
	y2tic_width = 0;

    /* y2label */
    if (can_rotate && axis_array[SECOND_Y_AXIS].label.text) {
	double tmpx, tmpy;
	map_position_r(&(axis_array[SECOND_Y_AXIS].label.offset),
		       &tmpx, &tmpy, "boundary");
	y2label_textwidth = (int) (y2lablin * t->v_char + tmpx);
	if (!axis_array[SECOND_Y_AXIS].ticmode)
	    y2label_textwidth += 0.5 * t->v_char;
    } else
	y2label_textwidth = 0;

    /* Make room for the color box if needed. */
    if (rmargin.scalex != screen) {
	if (is_plot_with_colorbox()) {
#define COLORBOX_SCALE 0.125
#define WIDEST_COLORBOX_TICTEXT 3
	    if ((color_box.where != SMCOLOR_BOX_NO) && (color_box.where != SMCOLOR_BOX_USER)) {
		plot_bounds.xright -= (int) (plot_bounds.xright-plot_bounds.xleft)*COLORBOX_SCALE;
		plot_bounds.xright -= (int) ((t->h_char) * WIDEST_COLORBOX_TICTEXT);
	    }
	}

	if (rmargin.x < 0) {
	    plot_bounds.xright -= y2tic_width + y2tic_textwidth;
	    if (y2label_textwidth > 0)
		plot_bounds.xright -= y2label_textwidth;

	    if (plot_bounds.xright == (int) (0.5 + (t->xmax - 1) * (xsize + xoffset))) {
		/* make room for end of xtic or x2tic label */
		plot_bounds.xright -= (int) (t->h_char * 2);
	    }
	    /* EAM 2009 - protruding xtic labels */
	    if (term->xmax - plot_bounds.xright < xtic_textwidth)
		plot_bounds.xright = term->xmax - xtic_textwidth;
	    /* DBT 12-3-98  extra margin just in case */
	    plot_bounds.xright -= 0.5 * t->h_char;
	}
	/* Note: we took care of explicit 'set rmargin foo' at line 502 */
    }

    /*  end of plot_bounds.xright calculation }}} */


    /*{{{  set up x and x2 tics */
    /* we should base the guide on the width of the xtics, but we cannot
     * use widest_tics until tics are set up. Bit of a downer - let us
     * assume tics are 5 characters wide
     */
    /* HBB 20001205: moved this block to before aspect_ratio is
     * applied: setup_tics may extend the ranges, which would distort
     * the aspect ratio */

    setup_tics(FIRST_X_AXIS, 20);
    setup_tics(SECOND_X_AXIS, 20);


    /* Modify the bounding box to fit the aspect ratio, if any was
     * given. */
    if (aspect_ratio != 0.0) {
	double current_aspect_ratio;

	if (aspect_ratio < 0
	    && (X_AXIS.max - X_AXIS.min) != 0.0
	    ) {
	    current_aspect_ratio = - aspect_ratio
		* fabs((Y_AXIS.max - Y_AXIS.min) / (X_AXIS.max - X_AXIS.min));
	} else
	    current_aspect_ratio = aspect_ratio;

	/* Set aspect ratio if valid and sensible */
	/* EAM Mar 2008 - fixed borders take precedence over centering */
	if (current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
	    double current = ((double) (plot_bounds.ytop - plot_bounds.ybot)) 
			   / (plot_bounds.xright - plot_bounds.xleft);
	    double required = (current_aspect_ratio * t->v_tic) / t->h_tic;

	    if (current > required) {
		/* too tall */
		int old_height = plot_bounds.ytop - plot_bounds.ybot;
		int new_height = required * (plot_bounds.xright - plot_bounds.xleft);
		if (bmargin.scalex == screen)
		    plot_bounds.ytop = plot_bounds.ybot + new_height;
		else if (tmargin.scalex == screen)
		    plot_bounds.ybot = plot_bounds.ytop - new_height;
		else {
		    plot_bounds.ybot += (old_height - new_height) / 2;
		    plot_bounds.ytop -= (old_height - new_height) / 2;
		}

	    } else {
		int old_width = plot_bounds.xright - plot_bounds.xleft;
		int new_width = (plot_bounds.ytop - plot_bounds.ybot) / required;
		if (lmargin.scalex == screen)
		    plot_bounds.xright = plot_bounds.xleft + new_width;
		else if (rmargin.scalex == screen)
		    plot_bounds.xleft = plot_bounds.xright - new_width;
		else {
		    plot_bounds.xleft += (old_width - new_width) / 2;
		    plot_bounds.xright -= (old_width - new_width) / 2;
		}
	    }
	}
    }

    /*  adjust top and bottom margins for tic label rotation */

    if (tmargin.x < 0
	&& axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER
	&& vertical_x2tics) {
	double projection = sin((double)axis_array[SECOND_X_AXIS].tic_rotate*DEG2RAD);
	widest_tic_strlen = 0;		/* reset the global variable ... */
	gen_tics(SECOND_X_AXIS, /* 0, */ widest_tic_callback);
	plot_bounds.ytop += x2tic_textheight;
	/* Now compute a new one and use that instead: */
	if (projection > 0.0)
	    x2tic_textheight = (int) (t->h_char * (widest_tic_strlen)) * projection;
	else
	    x2tic_textheight = t->v_char;
	plot_bounds.ytop -= x2tic_textheight;
    }
    if (bmargin.x < 0
	&& axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER
	&& vertical_xtics) {
	double projection;
	if (axis_array[FIRST_X_AXIS].tic_rotate == 90)
	    projection = 1.0;
	else
	    projection = -sin((double)axis_array[FIRST_X_AXIS].tic_rotate*DEG2RAD);
	widest_tic_strlen = 0;		/* reset the global variable ... */
	gen_tics(FIRST_X_AXIS, /* 0, */ widest_tic_callback);
	plot_bounds.ybot -= xtic_textheight;
	if (projection > 0.0)
	    xtic_textheight = (int) (t->h_char * widest_tic_strlen) * projection
			    + t->v_char;
	plot_bounds.ybot += xtic_textheight;
    }

    /* EAM - FIXME
     * Notwithstanding all these fancy calculations, plot_bounds.ytop must always be above plot_bounds.ybot
     */
    if (plot_bounds.ytop < plot_bounds.ybot) {
	int i = plot_bounds.ytop;

	plot_bounds.ytop = plot_bounds.ybot;
	plot_bounds.ybot = i;
	FPRINTF((stderr,"boundary: Big problems! plot_bounds.ybot > plot_bounds.ytop\n"));
    }

    /*  compute coordinates for axis labels, title et al
     *     (some of these may not be used) */

    x2label_y = plot_bounds.ytop + x2tic_height + x2tic_textheight + x2label_textheight;
    if (x2tic_textheight && (title_textheight || x2label_textheight))
	x2label_y += t->v_char;

    title_y = x2label_y + title_textheight;

    ylabel_y = plot_bounds.ytop + x2tic_height + x2tic_textheight + ylabel_textheight;

    y2label_y = plot_bounds.ytop + x2tic_height + x2tic_textheight + y2label_textheight;

    /* Shift upward by 0.2 line to allow for descenders in xlabel text */
    xlabel_y = plot_bounds.ybot - xtic_height - xtic_textheight - xlabel_textheight
	+ ((float)xlablin+0.2) * t->v_char;
    ylabel_x = plot_bounds.xleft - ytic_width - ytic_textwidth;
    if (axis_array[FIRST_Y_AXIS].label.text && can_rotate)
	ylabel_x -= ylabel_textwidth;

    y2label_x = plot_bounds.xright + y2tic_width + y2tic_textwidth;
    if (axis_array[SECOND_Y_AXIS].label.text && can_rotate)
	y2label_x += y2label_textwidth - y2lablin * t->v_char;

    if (vertical_timelabel) {
	if (timelabel_bottom)
	    time_y = xlabel_y - timebot_textheight + xlabel_textheight;
	else {
	    time_y = title_y + timetop_textheight - title_textheight
		- x2label_textheight;
	}
    } else {
	if (timelabel_bottom)
	    time_y = plot_bounds.ybot - xtic_height - xtic_textheight - xlabel_textheight
		- timebot_textheight + t->v_char;
	else if (ylabel_textheight > 0)
	    time_y = ylabel_y + timetop_textheight;
	else
	    time_y = plot_bounds.ytop + x2tic_height + x2tic_textheight
		+ timetop_textheight + (int) t->h_char;
    }
    if (vertical_timelabel)
	time_x = plot_bounds.xleft - ytic_width - ytic_textwidth - timelabel_textwidth;
    else {
	double tmpx, tmpy;
	map_position_r(&(timelabel.offset), &tmpx, &tmpy, "boundary");
	time_x = plot_bounds.xleft - ytic_width - ytic_textwidth + (int) (tmpx);
    }

    xtic_y = plot_bounds.ybot - xtic_height
	- (int) (vertical_xtics ? t->h_char : t->v_char);

    x2tic_y = plot_bounds.ytop + x2tic_height
	+ (vertical_x2tics ? (int) t->h_char : x2tic_textheight);

    ytic_x = plot_bounds.xleft - ytic_width
	- (vertical_ytics
	   ? (ytic_textwidth - (int) t->v_char)
	   : (int) t->h_char);

    y2tic_x = plot_bounds.xright + y2tic_width
	+ (int) (vertical_y2tics ? t->v_char : t->h_char);

    /* restore text to horizontal [we tested rotation above] */
    (void) (*t->text_angle) (0);

    /* needed for map_position() below */
    AXIS_SETSCALE(FIRST_Y_AXIS, plot_bounds.ybot, plot_bounds.ytop);
    AXIS_SETSCALE(SECOND_Y_AXIS, plot_bounds.ybot, plot_bounds.ytop);
    AXIS_SETSCALE(FIRST_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
    AXIS_SETSCALE(SECOND_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
    /* HBB 20020122: moved here from do_plot, because map_position
     * needs these, too */
    axis_set_graphical_range(FIRST_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
    axis_set_graphical_range(FIRST_Y_AXIS, plot_bounds.ybot, plot_bounds.ytop);
    axis_set_graphical_range(SECOND_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
    axis_set_graphical_range(SECOND_Y_AXIS, plot_bounds.ybot, plot_bounds.ytop);

    /* Calculate space for keys (do_plot will use these to position key). */
    key_w = key_col_wth * key_cols;
    key_h = (ktitl_lines) * t->v_char + key_rows * key_entry_height;
    key_h += (int) (key->height_fix * t->v_char);
    if (key->region == GPKEY_AUTO_INTERIOR_LRTBC
	|| (key->region == GPKEY_AUTO_EXTERIOR_LRTBC && key->vpos == JUST_CENTRE && key->hpos == CENTRE)) {
	if (key->vpos == JUST_TOP) {
	    key->bounds.ytop = plot_bounds.ytop - t->v_tic;
	    key->bounds.ybot = key->bounds.ytop - key_h;
	} else if (key->vpos == JUST_BOT) {
	    key->bounds.ybot = plot_bounds.ybot + t->v_tic;
	    key->bounds.ytop = key->bounds.ybot + key_h;
	} else /* (key->vpos == JUST_CENTRE) */ {
	    int key_box_half = key_h / 2;
	    key->bounds.ybot = (plot_bounds.ybot + plot_bounds.ytop) / 2 - key_box_half;
	    key->bounds.ytop = (plot_bounds.ybot + plot_bounds.ytop) / 2 + key_box_half;
	}
	if (key->hpos == LEFT) {
	    key->bounds.xleft = plot_bounds.xleft + t->h_char;
	    key->bounds.xright = key->bounds.xleft + key_w;
	} else if (key->hpos == RIGHT) {
	    key->bounds.xright = plot_bounds.xright - t->h_char;
	    key->bounds.xleft = key->bounds.xright - key_w;
	} else /* (key->hpos == CENTER) */ {
	    int key_box_half = key_w / 2;
	    key->bounds.xleft = (plot_bounds.xright + plot_bounds.xleft) / 2 - key_box_half;
	    key->bounds.xright = (plot_bounds.xright + plot_bounds.xleft) / 2 + key_box_half;
	}
    } else if (key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN) {

	/* Vertical alignment */
	if (key->margin == GPKEY_TMARGIN) {
	    /* align top first since tmargin may be manual */
	    key->bounds.ytop = (ysize + yoffset) * t->ymax - t->v_tic;
	    key->bounds.ybot = key->bounds.ytop - key_h;
	} else if (key->margin == GPKEY_BMARGIN) {
	    /* align bottom first since bmargin may be manual */
	    key->bounds.ybot = yoffset * t->ymax + t->v_tic;
	    key->bounds.ytop = key->bounds.ybot + key_h;
	} else {
	    if (key->vpos == JUST_TOP) {
		/* align top first since tmargin may be manual */
		key->bounds.ytop = plot_bounds.ytop;
		key->bounds.ybot = key->bounds.ytop - key_h;
	    } else if (key->vpos == CENTRE) {
		int key_box_half = key_h / 2;
		key->bounds.ybot = (plot_bounds.ybot + plot_bounds.ytop) / 2 - key_box_half;
		key->bounds.ytop = (plot_bounds.ybot + plot_bounds.ytop) / 2 + key_box_half;
	    } else {
		/* align bottom first since bmargin may be manual */
		key->bounds.ybot = plot_bounds.ybot;
		key->bounds.ytop = key->bounds.ybot + key_h;
	    }
	}

	/* Horizontal alignment */
	if (key->margin == GPKEY_LMARGIN) {
	    /* align left first since lmargin may be manual */
	    key->bounds.xleft = xoffset * t->xmax + t->h_char;
	    key->bounds.xright = key->bounds.xleft + key_w;
	} else if (key->margin == GPKEY_RMARGIN) {
	    /* align right first since rmargin may be manual */
	    key->bounds.xright = (xsize + xoffset) * (t->xmax-1) - t->h_char;
	    key->bounds.xleft = key->bounds.xright - key_w;
	} else {
	    if (key->hpos == LEFT) {
		/* align left first since lmargin may be manual */
		key->bounds.xleft = plot_bounds.xleft;
		key->bounds.xright = key->bounds.xleft + key_w;
	    } else if (key->hpos == CENTRE) {
		int key_box_half = key_w / 2;
		key->bounds.xleft = (plot_bounds.xright + plot_bounds.xleft) / 2 - key_box_half;
		key->bounds.xright = (plot_bounds.xright + plot_bounds.xleft) / 2 + key_box_half;
	    } else {
		/* align right first since rmargin may be manual */
		key->bounds.xright = plot_bounds.xright;
		key->bounds.xleft = key->bounds.xright - key_w;
	    }
	}

    } else {
	int x, y;

	map_position(&key->user_pos, &x, &y, "key");
#if 0
/* FIXME!!!
** pm 22.1.2002: if key->user_pos.scalex or scaley == first_axes or second_axes,
** then the graph scaling is not yet known and the box is positioned incorrectly;
** you must do "replot" to avoid the wrong plot ... bad luck if output does not
** go to screen */
#define OK fprintf(stderr,"Line %i of %s is OK\n",__LINE__,__FILE__)
	OK;
	fprintf(stderr,"\tHELE: user pos: x=%i y=%i\n",key->user_pos.x,key->user_pos.y);
	fprintf(stderr,"\tHELE: user pos: x=%i y=%i\n",x,y);
#endif
	/* Here top, bottom, left, right refer to the alignment with respect to point. */
	key->bounds.xleft = x;
	if (key->hpos == CENTRE)
	    key->bounds.xleft -= key_w/2;
	else if (key->hpos == RIGHT)
	    key->bounds.xleft -= key_w;
	key->bounds.xright = key->bounds.xleft + key_w;
	key->bounds.ytop = y;
	if (key->vpos == JUST_CENTRE)
	    key->bounds.ytop += key_h/2;
	else if (key->vpos == JUST_BOT)
	    key->bounds.ytop += key_h;
	key->bounds.ybot = key->bounds.ytop - key_h;
    }
    /*}}} */

    /* Set default clipping to the plot boundary */
    clip_area = &plot_bounds;

}

/*}}} */


static void
get_arrow(
    struct arrow_def *arrow,
    int* sx, int* sy,
    int* ex, int* ey)
{
    double sx_d, sy_d, ex_d, ey_d;
    map_position_double(&arrow->start, &sx_d, &sy_d, "arrow");
    *sx = (int)(sx_d);
    *sy = (int)(sy_d);
    if (arrow->relative) {
	/* different coordinate systems:
	 * add the values in the drivers coordinate system.
	 * For log scale: relative coordinate is factor */
	map_position_r(&arrow->end, &ex_d, &ey_d, "arrow");
	*ex = (int)(ex_d + sx_d);
	*ey = (int)(ey_d + sy_d);
    } else {
	map_position_double(&arrow->end, &ex_d, &ey_d, "arrow");
	*ex = (int)(ex_d);
	*ey = (int)(ey_d);
    }
}

/* FIXME HBB 20020225: this is shared with graph3d.c, so it shouldn't
 * be in this module */
void
apply_head_properties(struct arrow_style_type *arrow_properties)
{
    curr_arrow_headfilled = arrow_properties->head_filled;
    curr_arrow_headlength = 0;
    if (arrow_properties->head_length > 0) {
	/* set head length+angle for term->arrow */
	int itmp, x1, x2;
	struct position headsize = {0,0,0,0.,0.,0.};

	headsize.x = arrow_properties->head_length;
	headsize.scalex = arrow_properties->head_lengthunit;

	headsize.y = 1.0; /* any value, just avoid log y */
	map_position(&headsize, &x2, &itmp, "arrow");

	headsize.x = 0; /* measure length from zero */
	map_position(&headsize, &x1, &itmp, "arrow");

	curr_arrow_headangle = arrow_properties->head_angle;
	curr_arrow_headbackangle = arrow_properties->head_backangle;
	curr_arrow_headlength = x2 - x1;
    }
}

static void
place_grid()
{
    struct termentry *t = term;

    term_apply_lp_properties(&border_lp);	/* border linetype */
    largest_polar_circle = 0;

    /* select first mapping */
    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;

    /* label first y axis tics */
    axis_output_tics(FIRST_Y_AXIS, &ytic_x, FIRST_X_AXIS,
		     /* (GRID_Y | GRID_MY), */ ytick2d_callback);
    /* label first x axis tics */
    axis_output_tics(FIRST_X_AXIS, &xtic_y, FIRST_Y_AXIS,
		     /* (GRID_X | GRID_MX), */ xtick2d_callback);

    /* select second mapping */
    x_axis = SECOND_X_AXIS;
    y_axis = SECOND_Y_AXIS;

    axis_output_tics(SECOND_Y_AXIS, &y2tic_x, SECOND_X_AXIS,
		     /* (GRID_Y2 | GRID_MY2), */ ytick2d_callback);
    axis_output_tics(SECOND_X_AXIS, &x2tic_y, SECOND_Y_AXIS,
		     /* (GRID_X2 | GRID_MX2), */ xtick2d_callback);


    /* select first mapping */
    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;

/* RADIAL LINES FOR POLAR GRID */

    /* note that draw_clip_line takes unsigneds, but (fortunately)
     * clip_line takes signeds
     */
    if (polar_grid_angle) {
	double theta = 0;
	int ox = map_x(0);
	int oy = map_y(0);
	term_apply_lp_properties(&grid_lp);
	for (theta = 0; theta < 6.29; theta += polar_grid_angle) {
	    /* copy ox in case it gets moved (but it shouldn't) */
	    int oox = ox;
	    int ooy = oy;
	    int x = map_x(largest_polar_circle * cos(theta));
	    int y = map_y(largest_polar_circle * sin(theta));
	    if (clip_line(&oox, &ooy, &x, &y)) {
		(*t->move) ((unsigned int) oox, (unsigned int) ooy);
		(*t->vector) ((unsigned int) x, (unsigned int) y);
	    }
	}
	draw_clip_line(ox, oy, map_x(largest_polar_circle * cos(theta)), map_y(largest_polar_circle * sin(theta)));
    }
}

static void
place_arrows(int layer)
{
    struct arrow_def *this_arrow;
    BoundingBox *clip_save = clip_area;

    /* Allow arrows to run off the plot, so long as they are still on the canvas */
    if (term->flags & TERM_CAN_CLIP)
      clip_area = NULL;
    else
      clip_area = &canvas;

    for (this_arrow = first_arrow;
	 this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	int sx, sy, ex, ey;
	
	if (this_arrow->arrow_properties.layer != layer)
	    continue;
	get_arrow(this_arrow, &sx, &sy, &ex, &ey);

	term_apply_lp_properties(&(this_arrow->arrow_properties.lp_properties));
	apply_head_properties(&(this_arrow->arrow_properties));
	draw_clip_arrow(sx, sy, ex, ey, this_arrow->arrow_properties.head);
    }
    term_apply_lp_properties(&border_lp);
    clip_area = clip_save;
}

static void
place_labels(struct text_label *listhead, int layer, TBOOLEAN clip)
{
    struct text_label *this_label;
    int x, y;

    if (term->pointsize)
	(*term->pointsize)(pointsize);

    for (this_label = listhead; this_label != NULL; this_label = this_label->next) {

	if (this_label->layer != layer)
	    continue;

	if (layer == LAYER_PLOTLABELS) {
	    x = map_x(this_label->place.x);
	    y = map_y(this_label->place.y);
	} else
	    map_position(&this_label->place, &x, &y, "label");

	if (clip) {
	    if (this_label->place.scalex == first_axes)
		if (!(inrange(this_label->place.x, axis_array[FIRST_X_AXIS].min, axis_array[FIRST_X_AXIS].max)))
		    continue;
	    if (this_label->place.scalex == second_axes)
		if (!(inrange(this_label->place.x, axis_array[SECOND_X_AXIS].min, axis_array[SECOND_X_AXIS].max)))
		    continue;
	    if (this_label->place.scaley == first_axes)
		if (!(inrange(this_label->place.y, axis_array[FIRST_Y_AXIS].min, axis_array[FIRST_Y_AXIS].max)))
		    continue;
	    if (this_label->place.scaley == second_axes)
		if (!(inrange(this_label->place.y, axis_array[SECOND_Y_AXIS].min, axis_array[SECOND_Y_AXIS].max)))
		    continue;

	}

	write_label(x, y, this_label);
    }
}

#ifdef EAM_OBJECTS
void
place_objects(struct object *listhead, int layer, int dimensions, BoundingBox *clip_area)
{
    t_object *this_object;
    double x1, y1;
    int style;

    for (this_object = listhead; this_object != NULL; this_object = this_object->next) {
	struct lp_style_type lpstyle;
	struct fill_style_type *fillstyle;
    
	if (this_object->layer != layer)
	    continue;

	/* Extract line and fill style, but don't apply it yet */
	if (this_object->lp_properties.l_type == LT_DEFAULT
	    && this_object->object_type == OBJ_RECTANGLE)
	    lpstyle = default_rectangle.lp_properties;
	else
	    lpstyle = this_object->lp_properties;
	
	if (this_object->fillstyle.fillstyle == FS_DEFAULT
	    && this_object->object_type == OBJ_RECTANGLE)
	    fillstyle = &default_rectangle.fillstyle;
	else
	    fillstyle = &this_object->fillstyle;
	style = style_from_fill(fillstyle);

	switch (this_object->object_type) {

	case OBJ_CIRCLE:
	{
	    t_circle *e = &this_object->o.circle;
	    double radius, junk;

	    if (dimensions == 2 || e->center.scalex == screen) {
		map_position_double(&e->center, &x1, &y1, "rect");
		map_position_r(&e->extent, &radius, &junk, "rect");
	    } else if (splot_map) {
		int junkw, junkh;
		map3d_position_double(&e->center, &x1, &y1, "rect");
		map3d_position_r(&e->extent, &junkw, &junkh, "rect");
		radius = junkw;
	    } else
		break;

	    term_apply_lp_properties(&lpstyle);

	    do_arc((int)x1, (int)y1, radius, e->arc_begin, e->arc_end, style);

	    /* Retrace the border if the style requests it */
	    if (need_fill_border(fillstyle))
		do_arc((int)x1, (int)y1, radius, e->arc_begin, e->arc_end, 0);

	    break;
	}

	case OBJ_ELLIPSE:
	{
	    term_apply_lp_properties(&lpstyle);

	    if (dimensions == 2)
		do_ellipse(2, &this_object->o.ellipse, style);
	    else if (splot_map)
		do_ellipse(3, &this_object->o.ellipse, style);
	    else
		break;

	    /* Retrace the border if the style requests it */
	    if (need_fill_border(fillstyle))
		do_ellipse(dimensions, &this_object->o.ellipse, 0);

	    break;
	}

	case OBJ_POLYGON:
	{
	    term_apply_lp_properties(&lpstyle);

	    do_polygon(dimensions, &this_object->o.polygon, style);

	    /* Retrace the border if the style requests it */
	    if (need_fill_border(fillstyle))
		    do_polygon(dimensions, &this_object->o.polygon, 0);

	    break;
	}

	case OBJ_RECTANGLE:
	{
	    do_rectangle(dimensions, this_object, style);
	    break;
	}

	default:
	    break;
	} /* End switch(object_type) */


    }
}
#endif

/*
 * Apply axis range expansions from "set offsets" command
 */
static void
adjust_offsets()
{
    double b = boff.scaley == graph ? fabs(Y_AXIS.max - Y_AXIS.min)*boff.y : boff.y;
    double t = toff.scaley == graph ? fabs(Y_AXIS.max - Y_AXIS.min)*toff.y : toff.y;
    double l = loff.scalex == graph ? fabs(X_AXIS.max - X_AXIS.min)*loff.x : loff.x;
    double r = roff.scalex == graph ? fabs(X_AXIS.max - X_AXIS.min)*roff.x : roff.x;

    if (Y_AXIS.min < Y_AXIS.max) {
	Y_AXIS.min -= b;
	Y_AXIS.max += t;
    } else {
	Y_AXIS.max -= b;
	Y_AXIS.min += t;
    }
    if (X_AXIS.min < X_AXIS.max) {
	X_AXIS.min -= l;
	X_AXIS.max += r;
    } else {
	X_AXIS.max -= l;
	X_AXIS.min += r;
    }

    if (X_AXIS.min == X_AXIS.max)
	int_error(NO_CARET, "x_min should not equal x_max!");
    if (Y_AXIS.min == Y_AXIS.max)
	int_error(NO_CARET, "y_min should not equal y_max!");
}

void
do_plot(struct curve_points *plots, int pcount)
{
    struct termentry *t = term;
    int curve;
    struct curve_points *this_plot = NULL;
    int xl = 0, yl = 0;	/* avoid gcc -Wall warning */
    int key_count = 0;
    legend_key *key = &keyT;

    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;
    adjust_offsets();

    /* EAM June 2003 - Although the comment below implies that font dimensions
     * are known after term_initialise(), this is not true at least for the X11
     * driver.  X11 fonts are not set until an actual display window is
     * opened, and that happens in term->graphics(), which is called from
     * term_start_plot().
     */
    term_initialise();		/* may set xmax/ymax */
    term_start_plot();

    /* Figure out if we need a colorbox for this plot */
    set_plot_with_palette(0, MODE_PLOT); /* EAM FIXME - 1st parameter is a dummy */

    /* compute boundary for plot (plot_bounds.xleft, plot_bounds.xright, plot_bounds.ytop, plot_bounds.ybot)
     * also calculates tics, since xtics depend on plot_bounds.xleft
     * but plot_bounds.xleft depends on ytics. Boundary calculations depend
     * on term->v_char etc, so terminal must be initialised first.
     */
    boundary(plots, pcount);

    /* Make palette */
    if (is_plot_with_palette())
	make_palette();

    /* Give a chance for rectangles to be behind everything else */
    place_objects( first_object, -1, 2, NULL );

    screen_ok = FALSE;

    /* Sync point for epslatex text positioning */
    if (term->layer)
	(term->layer)(TERM_LAYER_BACKTEXT);

    /* DRAW TICS AND GRID */
    if (grid_layer == 0 || grid_layer == -1)
	place_grid();

    /* DRAW AXES */
    /* after grid so that axes linetypes are on top */
    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;	/* chose scaling */

    axis_draw_2d_zeroaxis(FIRST_X_AXIS,FIRST_Y_AXIS);
    axis_draw_2d_zeroaxis(FIRST_Y_AXIS,FIRST_X_AXIS);

    x_axis = SECOND_X_AXIS;
    y_axis = SECOND_Y_AXIS;	/* chose scaling */

    axis_draw_2d_zeroaxis(SECOND_X_AXIS,SECOND_Y_AXIS);
    axis_draw_2d_zeroaxis(SECOND_Y_AXIS,SECOND_X_AXIS);

    /* DRAW PLOT BORDER */
    if (draw_border)
	plot_border();

    /* YLABEL */
    if (axis_array[FIRST_Y_AXIS].label.text) {
	ignore_enhanced(axis_array[FIRST_Y_AXIS].label.noenhanced);
	apply_pm3dcolor(&(axis_array[FIRST_Y_AXIS].label.textcolor),t);
	/* we worked out x-posn in boundary() */
	if ((*t->text_angle) (axis_array[FIRST_Y_AXIS].label.rotate)) {
	    double tmpx, tmpy;
	    unsigned int x, y;
	    map_position_r(&(axis_array[FIRST_Y_AXIS].label.offset),
			   &tmpx, &tmpy, "doplot");

	    x = ylabel_x + (t->v_char / 2);
	    y = (plot_bounds.ytop + plot_bounds.ybot) / 2 + tmpy;

	    write_multiline(x, y, axis_array[FIRST_Y_AXIS].label.text,
			    CENTRE, JUST_TOP, axis_array[FIRST_Y_AXIS].label.rotate,
			    axis_array[FIRST_Y_AXIS].label.font);
	    (*t->text_angle) (0);
	} else {
	    /* really bottom just, but we know number of lines
	       so we need to adjust x-posn by one line */
	    unsigned int x = ylabel_x;
	    unsigned int y = ylabel_y;

	    write_multiline(x, y, axis_array[FIRST_Y_AXIS].label.text,
			    LEFT, JUST_TOP, 0,
			    axis_array[FIRST_Y_AXIS].label.font);
	}
	reset_textcolor(&(axis_array[FIRST_Y_AXIS].label.textcolor),t);
	ignore_enhanced(FALSE);
    }

    /* Y2LABEL */
    if (axis_array[SECOND_Y_AXIS].label.text) {
	ignore_enhanced(axis_array[SECOND_Y_AXIS].label.noenhanced);
	apply_pm3dcolor(&(axis_array[SECOND_Y_AXIS].label.textcolor),t);
	/* we worked out coordinates in boundary() */
	if ((*t->text_angle) (axis_array[SECOND_Y_AXIS].label.rotate)) {
	    double tmpx, tmpy;
	    unsigned int x, y;
	    map_position_r(&(axis_array[SECOND_Y_AXIS].label.offset),
			   &tmpx, &tmpy, "doplot");
	    x = y2label_x + (t->v_char / 2) - 1;
	    y = (plot_bounds.ytop + plot_bounds.ybot) / 2 + tmpy;

	    write_multiline(x, y, axis_array[SECOND_Y_AXIS].label.text,
			    CENTRE, JUST_TOP, 
			    axis_array[SECOND_Y_AXIS].label.rotate,
			    axis_array[SECOND_Y_AXIS].label.font);
	    (*t->text_angle) (0);
	} else {
	    /* really bottom just, but we know number of lines */
	    unsigned int x = y2label_x;
	    unsigned int y = y2label_y;

	    write_multiline(x, y, axis_array[SECOND_Y_AXIS].label.text,
			    RIGHT, JUST_TOP, 0,
			    axis_array[SECOND_Y_AXIS].label.font);
	}
	reset_textcolor(&(axis_array[SECOND_Y_AXIS].label.textcolor),t);
	ignore_enhanced(FALSE);
    }

    /* XLABEL */
    if (axis_array[FIRST_X_AXIS].label.text) {
	double tmpx, tmpy;
	unsigned int x, y;
	map_position_r(&(axis_array[FIRST_X_AXIS].label.offset),
		       &tmpx, &tmpy, "doplot");

	x = (plot_bounds.xright + plot_bounds.xleft) / 2 +  tmpx;
	y = xlabel_y - t->v_char / 2;   /* HBB */

	ignore_enhanced(axis_array[FIRST_X_AXIS].label.noenhanced);
	apply_pm3dcolor(&(axis_array[FIRST_X_AXIS].label.textcolor), t);
	write_multiline(x, y, axis_array[FIRST_X_AXIS].label.text,
			JUST_CENTRE, JUST_TOP, 0,
			axis_array[FIRST_X_AXIS].label.font);
	reset_textcolor(&(axis_array[FIRST_X_AXIS].label.textcolor), t);
	ignore_enhanced(FALSE);
    }

    /* PLACE TITLE */
    if (title.text) {
	double tmpx, tmpy;
	unsigned int x, y;
	map_position_r(&(title.offset), &tmpx, &tmpy, "doplot");
	/* we worked out y-coordinate in boundary() */
	x = (plot_bounds.xleft + plot_bounds.xright) / 2 + tmpx;
	y = title_y - t->v_char / 2;

	ignore_enhanced(title.noenhanced);
	apply_pm3dcolor(&(title.textcolor), t);
	write_multiline(x, y, title.text, CENTRE, JUST_TOP, 0, title.font);
	reset_textcolor(&(title.textcolor), t);
	ignore_enhanced(FALSE);
    }

    /* X2LABEL */
    if (axis_array[SECOND_X_AXIS].label.text) {
	double tmpx, tmpy;
	unsigned int x, y;
	map_position_r(&(axis_array[SECOND_X_AXIS].label.offset),
		       &tmpx, &tmpy, "doplot");
	/* we worked out y-coordinate in boundary() */
	x = (plot_bounds.xright + plot_bounds.xleft) / 2 + tmpx;
	y = x2label_y - t->v_char / 2 - 1;
	ignore_enhanced(axis_array[SECOND_X_AXIS].label.noenhanced);
	apply_pm3dcolor(&(axis_array[SECOND_X_AXIS].label.textcolor),t);
	write_multiline(x, y, axis_array[SECOND_X_AXIS].label.text, CENTRE,
			JUST_TOP, 0, axis_array[SECOND_X_AXIS].label.font);
	reset_textcolor(&(axis_array[SECOND_X_AXIS].label.textcolor),t);
	ignore_enhanced(FALSE);
    }

    /* PLACE TIMEDATE */
    if (timelabel.text) {
	/* we worked out coordinates in boundary() */
	char *str;
	time_t now;
	unsigned int x = time_x;
	unsigned int y = time_y;
	time(&now);
	/* there is probably no way to find out in advance how many
	 * chars strftime() writes */
	str = gp_alloc(MAX_LINE_LEN + 1, "timelabel.text");
	strftime(str, MAX_LINE_LEN, timelabel.text, localtime(&now));

	if (timelabel_rotate && (*t->text_angle) (TEXT_VERTICAL)) {
	    x += t->v_char / 2;	/* HBB */
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_TOP, TEXT_VERTICAL, timelabel.font);
	    else
		write_multiline(x, y, str, RIGHT, JUST_TOP, TEXT_VERTICAL, timelabel.font);
	    (*t->text_angle) (0);
	} else {
	    y -= t->v_char / 2;	/* HBB */
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_BOT, 0, timelabel.font);
	    else
		write_multiline(x, y, str, LEFT, JUST_TOP, 0, timelabel.font);
	}
	free(str);
    }

    /* Add back colorbox if appropriate */
    if (is_plot_with_colorbox() && term->set_color
	&& color_box.layer == LAYER_BACK)
	    draw_color_smooth_box(MODE_PLOT);

    /* And rectangles */
    place_objects( first_object, 0, 2, clip_area );

    /* PLACE LABELS */
    place_labels( first_label, 0, FALSE );

    /* PLACE ARROWS */
    place_arrows( 0 );

    /* Sync point for epslatex text positioning */
    if (term->layer)
	(term->layer)(TERM_LAYER_FRONTTEXT);

    /* WORK OUT KEY SETTINGS AND DO KEY TITLE / BOX */
    if (lkey) {			/* may have been cancelled if something went wrong */
	/* just use key->bounds.xleft etc worked out in boundary() */
	xl = key->bounds.xleft + key_size_left;
	yl = key->bounds.ytop;

	if (*key->title) {
	    int center = (key->bounds.xleft + key->bounds.xright) / 2;
	    double extra_height = 0.0;

	    if (key->textcolor.type == TC_RGB && key->textcolor.value < 0)
		apply_pm3dcolor(&(key->box.pm3d_color), t);
	    else
		apply_pm3dcolor(&(key->textcolor), t);
	    if ((t->flags & TERM_ENHANCED_TEXT) && strchr(key->title,'^'))
		extra_height += 0.51;
	    write_multiline(center, yl - (0.5 + extra_height/2.0) * t->v_char,
			    key->title, CENTRE, JUST_TOP, 0, key->font);
	    if ((t->flags & TERM_ENHANCED_TEXT) && strchr(key->title,'_'))
		extra_height += 0.3;
	    ktitl_lines += extra_height;
	    key->bounds.ybot -= extra_height * t->v_char;
	    yl -= t->v_char * ktitl_lines;
	    (*t->linetype)(LT_BLACK);
	}

	yl -= (int)(0.5 * key->height_fix * t->v_char);
	yl_ref = yl -= key_entry_height / 2;	/* centralise the keys */
	key_count = 0;

	if (key->box.l_type > LT_NODRAW) {
	    BoundingBox *clip_save = clip_area;
	    if (term->flags & TERM_CAN_CLIP)
		clip_area = NULL;
	    else
		clip_area = &canvas;
	    term_apply_lp_properties(&key->box);
	    newpath();
	    draw_clip_line(key->bounds.xleft, key->bounds.ybot, key->bounds.xleft, key->bounds.ytop);
	    draw_clip_line(key->bounds.xleft, key->bounds.ytop, key->bounds.xright, key->bounds.ytop);
	    draw_clip_line(key->bounds.xright, key->bounds.ytop, key->bounds.xright, key->bounds.ybot);
	    draw_clip_line(key->bounds.xright, key->bounds.ybot, key->bounds.xleft, key->bounds.ybot);
	    closepath();
	    /* draw a horizontal line between key title and first entry */
	    draw_clip_line(key->bounds.xleft, key->bounds.ytop - (ktitl_lines) * t->v_char,
			   key->bounds.xright, key->bounds.ytop - (ktitl_lines) * t->v_char);
	    clip_area = clip_save;
	}
    } /* lkey */

    /* DRAW CURVES */
    this_plot = plots;
    for (curve = 0; curve < pcount; this_plot = this_plot->next, curve++) {
	TBOOLEAN localkey = lkey;	/* a local copy */

	/* Sync point for start of new curve (used by svg, post, ...) */
	if (term->layer)
	    (term->layer)(TERM_LAYER_BEFORE_PLOT);

	/* set scaling for this plot's axes */
	x_axis = this_plot->x_axis;
	y_axis = this_plot->y_axis;

	term_apply_lp_properties(&(this_plot->lp_properties));

	/* Why only for histograms? */
	if (this_plot->plot_style == HISTOGRAMS) {
	    if (prefer_line_styles)
		lp_use_properties(&this_plot->lp_properties, this_plot->lp_properties.l_type+1);
	}

	/* Skip a line in the key between histogram clusters */
	if (this_plot->plot_style == HISTOGRAMS
	&&  this_plot->histogram_sequence == 0 && yl != yl_ref) {
	    if (++key_count >= key_rows) {
		yl = yl_ref;
		xl += key_col_wth;
		key_count = 0;
	    } else
		yl = yl - key_entry_height;
	}

	/* Column-stacked histograms store their key titles internally */
	if (this_plot->plot_style == HISTOGRAMS
	&&  histogram_opts.type == HT_STACKED_IN_TOWERS) {
	    text_label *key_entry;
	    localkey = 0;
	    if (this_plot->labels) {
		struct lp_style_type save_lp = this_plot->lp_properties;
		for (key_entry = this_plot->labels->next; key_entry; key_entry = key_entry->next) {
		    key_count++;
		    this_plot->lp_properties.l_type = key_entry->tag;
		    this_plot->fill_properties.fillpattern = key_entry->tag;
		    if (key_entry->text) {
			if (prefer_line_styles)
			    lp_use_properties(&this_plot->lp_properties, key_entry->tag + 1);
			do_key_sample(this_plot, key, key_entry->text, t, xl, yl);
		    }
		    yl = yl - key_entry_height;
		}
		free_labels(this_plot->labels);
		this_plot->labels = NULL;
		this_plot->lp_properties = save_lp;
	    }

	} else if (this_plot->title && !*this_plot->title) {
	    localkey = FALSE;
	} else if (this_plot->plot_type == NODATA) {
	    localkey = FALSE;
	} else {
	    ignore_enhanced(this_plot->title_no_enhanced);
		/* don't write filename or function enhanced */
	    if (localkey && this_plot->title && !this_plot->title_is_suppressed) {
		key_count++;
		if (key->invert)
		    yl = key->bounds.ybot + yl_ref + key_entry_height/2 - yl;
		do_key_sample(this_plot, key, this_plot->title, t, xl, yl);
	    }
	    ignore_enhanced(FALSE);
	}

	/* If any plots have opted out of autoscaling, we need to recheck */
	/* whether their points are INRANGE or not.                       */
	if (this_plot->noautoscale)
	    recheck_ranges(this_plot);

	/* and now the curves, plus any special key requirements */
	/* be sure to draw all lines before drawing any points */
	/* Skip missing/empty curves */
	if (this_plot->plot_type != NODATA) {

	    switch (this_plot->plot_style) {
	    case IMPULSES:
		plot_impulses(this_plot, X_AXIS.term_zero, Y_AXIS.term_zero);
		break;
	    case LINES:
		plot_lines(this_plot);
		break;
	    case STEPS:
		plot_steps(this_plot);
		break;
	    case FSTEPS:
		plot_fsteps(this_plot);
		break;
	    case HISTEPS:
		plot_histeps(this_plot);
		break;
	    case POINTSTYLE:
		plot_points(this_plot);
		break;
	    case LINESPOINTS:
		plot_lines(this_plot);
		plot_points(this_plot);
		break;
	    case DOTS:
		if (localkey && this_plot->title && !this_plot->title_is_suppressed) {
		    if (on_page(xl + key_point_offset, yl))
			(*t->point) (xl + key_point_offset, yl, -1);
		}
		plot_dots(this_plot);
		break;
	    case YERRORLINES:
	    case XERRORLINES:
	    case XYERRORLINES:
		plot_lines(this_plot);
		plot_bars(this_plot);
		plot_points(this_plot);
		break;
	    case YERRORBARS:
	    case XERRORBARS:
	    case XYERRORBARS:
		plot_bars(this_plot);
		plot_points(this_plot);
		break;
	    case BOXXYERROR:
	    case BOXES:
		plot_boxes(this_plot, Y_AXIS.term_zero);
		break;

	    case HISTOGRAMS:
		if (bar_layer == LAYER_FRONT)
		    plot_boxes(this_plot, Y_AXIS.term_zero);
		/* Draw the bars first, so that the box will cover the bottom half */
		if (histogram_opts.type == HT_ERRORBARS) {
		    (term->linewidth)(histogram_opts.bar_lw);
		    if (!need_fill_border(&default_fillstyle))
			(term->linetype)(this_plot->lp_properties.l_type);
		    plot_bars(this_plot);
		    term_apply_lp_properties(&(this_plot->lp_properties));
		}
		if (bar_layer != LAYER_FRONT)
		    plot_boxes(this_plot, Y_AXIS.term_zero);
		break;

	    case BOXERROR:
		if (bar_layer != LAYER_FRONT)
		    plot_bars(this_plot);
		plot_boxes(this_plot, Y_AXIS.term_zero);
		if (bar_layer == LAYER_FRONT)
		    plot_bars(this_plot);
		break;

	    case FILLEDCURVES:
		if (this_plot->filledcurves_options.closeto == FILLEDCURVES_BETWEEN) {
		    plot_betweencurves(this_plot);
		    /* FIXME: would like to call plot_lines() here twice, once for the lower */
		    /* curve and once for the upper curve(), conditional on need_fill_border */
		} else {
		    plot_filledcurves(this_plot);
		    if (need_fill_border(&this_plot->fill_properties))
			plot_lines(this_plot);
		}
		break;

	    case VECTOR:
		plot_vectors(this_plot);
		break;
	    case FINANCEBARS:
		plot_f_bars(this_plot);
		break;
	    case CANDLESTICKS:
		plot_c_bars(this_plot);
		break;

	    case PM3DSURFACE:
		fprintf(stderr, "** warning: can't use pm3d for 2d plots -- please unset pm3d\n");
		break;

	    case LABELPOINTS:
		place_labels( this_plot->labels->next, LAYER_PLOTLABELS, TRUE);
		break;

	    case IMAGE:
		this_plot->image_properties.type = IC_PALETTE;
		plot_image_or_update_axes(this_plot, FALSE);
		break;

	    case RGBIMAGE:
		this_plot->image_properties.type = IC_RGB;
		plot_image_or_update_axes(this_plot, FALSE);
		break;

	    case RGBA_IMAGE:
		this_plot->image_properties.type = IC_RGBA;
		plot_image_or_update_axes(this_plot, FALSE);
		break;

#ifdef EAM_OBJECTS
	    case CIRCLES:
		plot_circles(this_plot);
		break;
#endif
	    }
	}


	if (localkey && this_plot->title && !this_plot->title_is_suppressed) {
	    /* we deferred point sample until now */
	    if (this_plot->plot_style == LINESPOINTS
	    &&  this_plot->lp_properties.p_interval < 0) {
		(*t->linetype)(LT_BACKGROUND);
		(*t->point)(xl + key_point_offset, yl, 6);
		term_apply_lp_properties(&this_plot->lp_properties);
	    }
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT) {
		if (this_plot->lp_properties.p_size == PTSZ_VARIABLE)
		    (*t->pointsize)(pointsize);
		if (on_page(xl + key_point_offset, yl))
		    (*t->point) (xl + key_point_offset, yl, this_plot->lp_properties.p_type);
	    }
	    if (key->invert)
		yl = key->bounds.ybot + yl_ref + key_entry_height/2 - yl;
	    if (key_count >= key_rows) {
		yl = yl_ref;
		xl += key_col_wth;
		key_count = 0;
	    } else
		yl = yl - key_entry_height;
	}

	/* Sync point for end of this curve (used by svg, post, ...) */
	if (term->layer)
	    (term->layer)(TERM_LAYER_AFTER_PLOT);

    }

    /* DRAW TICS AND GRID */
    if (grid_layer == 1)
	place_grid();

    /* REDRAW PLOT BORDER */
    if (draw_border && border_layer == 1)
	plot_border();

    /* Add front colorbox if appropriate */
    if (is_plot_with_colorbox() && term->set_color
	&& color_box.layer == LAYER_FRONT)
	    draw_color_smooth_box(MODE_PLOT);

    /* And rectangles */
    place_objects( first_object, 1, 2, clip_area );

    /* PLACE LABELS */
    place_labels( first_label, 1, FALSE );

    /* PLACE HISTOGRAM TITLES */
    place_histogram_titles();

    /* PLACE ARROWS */
    place_arrows( 1 );

    /* Release the palette if we have used one (PostScript only?) */
    if (is_plot_with_palette() && term->previous_palette)
	term->previous_palette();

    term_end_plot();
}


/*
 * Plots marked "noautoscale" do not yet have INRANGE/OUTRANGE flags set.
 */
static void
recheck_ranges(struct curve_points *plot)
{
    int i;			/* point index */

    for (i = 0; i < plot->p_count; i++) {
	if (plot->noautoscale) {
	    plot->points[i].type = INRANGE;
	    if (!inrange(plot->points[i].x, axis_array[plot->x_axis].min, axis_array[plot->x_axis].max))
		plot->points[i].type = OUTRANGE;
	    if (!inrange(plot->points[i].y, axis_array[plot->y_axis].min, axis_array[plot->y_axis].max))
		plot->points[i].type = OUTRANGE;
	}
    }
}


/* plot_impulses:
 * Plot the curves in IMPULSES style
 */

static void
plot_impulses(struct curve_points *plot, int yaxis_x, int xaxis_y)
{
    int i;
    int x, y;
    struct termentry *t = term;

    for (i = 0; i < plot->p_count; i++) {
	switch (plot->points[i].type) {
	case INRANGE:
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    break;
	case OUTRANGE:
	    if (!inrange(plot->points[i].x, X_AXIS.min, X_AXIS.max))
		continue;
	    {
		double clipped_y = plot->points[i].y;

		x = map_x(plot->points[i].x);
		cliptorange(clipped_y, Y_AXIS.min, Y_AXIS.max);
		y = map_y(clipped_y);

		break;
	    }
	default:		/* just a safety */
	case UNDEFINED:{
		continue;
	    }
	}

	/* variable color read from data column */
	check_for_variable_color(plot, &plot->points[i]);

	if (polar)
	    (*t->move) (yaxis_x, xaxis_y);
	else
	    (*t->move) (x, xaxis_y);
	(*t->vector) (x, y);
    }

}

/* plot_lines:
 * Plot the curves in LINES style
 */
static void
plot_lines(struct curve_points *plot)
{
    int i;			/* point index */
    int x, y;			/* point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double ex, ey;		/* an edge point */
    double lx[2], ly[2];	/* two edge points */

    for (i = 0; i < plot->p_count; i++) {

	/* rgb variable  -  color read from data column */
	check_for_variable_color(plot, &plot->points[i]);

	switch (plot->points[i].type) {
	case INRANGE:{
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);

		if (prev == INRANGE) {
		    (*t->vector) (x, y);
		} else if (prev == OUTRANGE) {
		    /* from outrange to inrange */
		    if (!clip_lines1) {
			(*t->move) (x, y);
		    } else {
			edge_intersect(plot->points, i, &ex, &ey);
			(*t->move) (map_x(ex), map_y(ey));
			(*t->vector) (x, y);
		    }
		} else {	/* prev == UNDEFINED */
		    (*t->move) (x, y);
		    (*t->vector) (x, y);
		}

		break;
	    }
	case OUTRANGE:{
		if (prev == INRANGE) {
		    /* from inrange to outrange */
		    if (clip_lines1) {
			edge_intersect(plot->points, i, &ex, &ey);
			(*t->vector) (map_x(ex), map_y(ey));
		    }
		} else if (prev == OUTRANGE) {
		    /* from outrange to outrange */
		    if (clip_lines2) {
			if (two_edge_intersect(plot->points, i, lx, ly)) {
			    (*t->move) (map_x(lx[0]), map_y(ly[0]));
			    (*t->vector) (map_x(lx[1]), map_y(ly[1]));
			}
		    }
		}
		break;
	    }
	default:		/* just a safety */
	case UNDEFINED:{
		break;
	    }
	}
	prev = plot->points[i].type;
    }
}

/* plot_filledcurves:
 * Plot FILLED curves.
 * pm 8.9.2001 (main routine); pm 5.1.2002 (full support for options)
 */

/* finalize and draw the filled curve */
static void
finish_filled_curve(
    int points,
    gpiPoint *corners,
    struct curve_points *plot)
{
    filledcurves_opts *filledcurves_options = &plot->filledcurves_options;
    long side = 0;
    int i;

    if (points <= 0) return;
    /* add side (closing) points */
    switch (filledcurves_options->closeto) {
	case FILLEDCURVES_CLOSED:
		break;
	case FILLEDCURVES_X1:
		corners[points].x   = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		corners[points].y   =
		corners[points+1].y = axis_array[FIRST_Y_AXIS].term_lower;
		points += 2;
		break;
	case FILLEDCURVES_X2:
		corners[points].x   = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		corners[points].y   =
		corners[points+1].y = axis_array[FIRST_Y_AXIS].term_upper;
		points += 2;
		break;
	case FILLEDCURVES_Y1:
		corners[points].y   = corners[points-1].y;
		corners[points+1].y = corners[0].y;
		corners[points].x   =
		corners[points+1].x = axis_array[FIRST_X_AXIS].term_lower;
		points += 2;
		break;
	case FILLEDCURVES_Y2:
		corners[points].y   = corners[points-1].y;
		corners[points+1].y = corners[0].y;
		corners[points].x   =
		corners[points+1].x = axis_array[FIRST_X_AXIS].term_upper;
		points += 2;
		break;
	case FILLEDCURVES_ATX1:
		corners[points].x   =
		corners[points+1].x = map_x(filledcurves_options->at);
		    /* should be mapping real x1axis/graph/screen => screen */
		corners[points].y   = corners[points-1].y;
		corners[points+1].y = corners[0].y;
		for (i=0; i<points; i++)
		    side += corners[i].x - corners[points].x;
		points += 2;
		break;
	case FILLEDCURVES_ATX2:
		corners[points].x   =
		corners[points+1].x = map_x(filledcurves_options->at);
		    /* should be mapping real x2axis/graph/screen => screen */
		corners[points].y   = corners[points-1].y;
		corners[points+1].y = corners[0].y;
		for (i=0; i<points; i++)
		    side += corners[i].x - corners[points].x;
		points += 2;
		break;
	case FILLEDCURVES_ATY1:
		corners[points].y   =
		corners[points+1].y = map_y(filledcurves_options->at);
		    /* should be mapping real y1axis/graph/screen => screen */
		corners[points].x   = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		for (i=0; i<points; i++)
		    side += corners[i].y - corners[points].y;
		points += 2;
		break;
	case FILLEDCURVES_ATY2:
		corners[points].y   =
		corners[points+1].y = map_y(filledcurves_options->at);
		    /* should be mapping real y2axis/graph/screen => screen */
		corners[points].x   = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		for (i=0; i<points; i++)
		    side += corners[i].y - corners[points].y;
		points += 2;
		break;
	case FILLEDCURVES_ATXY:
		corners[points].x = map_x(filledcurves_options->at);
		    /* should be mapping real x1axis/graph/screen => screen */
		corners[points].y = map_y(filledcurves_options->aty);
		    /* should be mapping real y1axis/graph/screen => screen */
		points++;
		break;
	case FILLEDCURVES_BETWEEN:
		side = (corners[points].x > 0) ? 1 : -1;

		/* Prevent 1-pixel overlap of component rectangles, which */
		/* causes vertical stripe artifacts for transparent fill  */
		if (plot->fill_properties.fillstyle == FS_TRANSPARENT_SOLID) {
		    int direction = (corners[2].x < corners[0].x) ? -1 : 1;
		    if (points >= 4 && corners[2].x == corners[3].x) {
			corners[2].x -= direction, corners[3].x -= direction;
		    } else if (points >= 5 && corners[3].x == corners[4].x) {
			corners[3].x -= direction, corners[4].x -= direction;
		    }
		}
		break;
	default: /* the polygon is closed by default */
		break;
    }

#if 0
    { /* for debugging purposes */
    int i;
    fprintf(stderr, "List of %i corners:\n", points);
    for (i=0; i<points; i++)
	fprintf(stderr, "%2i: %3i,%3i | ", i, corners[i].x, corners[i].y);
    fprintf(stderr, " side = %ld",side);
    fprintf(stderr, "\n");
    }
#endif

    /* Check for request to fill only on one side of a bounding line */
    if (filledcurves_options->oneside > 0 && side < 0)
	return;
    if (filledcurves_options->oneside < 0 && side > 0)
	return;

    /* EAM Mar 2004 - Apply fill style to filled curves */
    corners->style = style_from_fill(&plot->fill_properties);
    term->filled_polygon(points, corners);
}


static void
plot_filledcurves(struct curve_points *plot)
{
    int i;			/* point index */
    int x, y;			/* point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double ex, ey;			/* an edge point */
    double lx[2], ly[2];		/* two edge points */
    int points = 0;			/* how many corners */
    static gpiPoint *corners = 0;	/* array of corners */
    static int corners_allocated = 0;	/* how many allocated */

    /* This set of variables is for tracking closed curve fill areas */
    int exit_edge = 0;		/* Which edge did an OUTRANGE point exit via? */
    int reentry_edge = 0;	/* Where did it reenter? */
    int out_updown = 0;		/* And where has it been in the meantime? */
    int out_leftright = 0;
    int first_entry = 0;	/* If the start point of the curve was OUTRANGE */

    if (!t->filled_polygon) { /* filled polygons are not available */
	plot_lines(plot);
	return;
    }

    if (!plot->filledcurves_options.opt_given) {
	/* no explicitly given filledcurves option for the current plot =>
	   use the default for data or function, respectively
	*/
	if (plot->plot_type == DATA)
	    memcpy(&plot->filledcurves_options, &filledcurves_opts_data, sizeof(filledcurves_opts));
	else
	    memcpy(&plot->filledcurves_options, &filledcurves_opts_func, sizeof(filledcurves_opts));
    }

    /* clip the "at" coordinate to the drawing area */
#define MYNOMIN(x,ax) if (x<axis_array[ax].min) x=axis_array[ax].min;
#define MYNOMAX(x,ax) if (x>axis_array[ax].max) x=axis_array[ax].max;
    /* FIXME HBB 20030127: replace by cliptorange()!? */
    switch (plot->filledcurves_options.closeto) {
	case FILLEDCURVES_ATX1:
	    MYNOMIN(plot->filledcurves_options.at,FIRST_X_AXIS);
	    MYNOMAX(plot->filledcurves_options.at,FIRST_X_AXIS);
	    break;
	case FILLEDCURVES_ATX2:
	    MYNOMIN(plot->filledcurves_options.at,SECOND_X_AXIS);
	    MYNOMAX(plot->filledcurves_options.at,SECOND_X_AXIS);
	    break;
	case FILLEDCURVES_ATY1:
	    MYNOMIN(plot->filledcurves_options.at,FIRST_Y_AXIS);
	    MYNOMAX(plot->filledcurves_options.at,FIRST_Y_AXIS);
	    break;
	case FILLEDCURVES_ATY2:
	    MYNOMIN(plot->filledcurves_options.at,SECOND_Y_AXIS);
	    MYNOMAX(plot->filledcurves_options.at,SECOND_Y_AXIS);
	    break;
	case FILLEDCURVES_ATXY:
	    MYNOMIN(plot->filledcurves_options.at,FIRST_X_AXIS);
	    MYNOMAX(plot->filledcurves_options.at,FIRST_X_AXIS);
	    MYNOMIN(plot->filledcurves_options.aty,FIRST_Y_AXIS);
	    MYNOMAX(plot->filledcurves_options.aty,FIRST_Y_AXIS);
	    break;
    }
#undef MYNOMIN
#undef MYNOMAX

    for (i = 0; i < plot->p_count; i++) {
	if (points+2 >= corners_allocated) { /* there are 2 side points */
	    corners_allocated += 128; /* reallocate more corners */
	    corners = gp_realloc( corners, corners_allocated*sizeof(gpiPoint), "corners for filledcurves");
	}
	switch (plot->points[i].type) {
	case INRANGE:{
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);

		if (prev == INRANGE) {
		    /* Split this segment if it crosses a bounding line */
		    if (bound_intersect(plot->points, i, &ex, &ey,
					&plot->filledcurves_options)) {
			corners[points].x = map_x(ex);
			corners[points++].y = map_y(ey);
			finish_filled_curve(points, corners, plot);
			points = 0;
			corners[points].x = map_x(ex);
			corners[points++].y = map_y(ey);
		    }
		    /* vector(x,y) */
		    corners[points].x = x;
		    corners[points++].y = y;
		} else if (prev == OUTRANGE) {
		    /* from outrange to inrange */
		    if (clip_fill) {  /* EAM concave bounding curves */
			reentry_edge = edge_intersect(plot->points, i, &ex, &ey);

			if (!exit_edge)
			    /* Curve must have started outside the plot area */
			    first_entry = reentry_edge;
			else if (reentry_edge != exit_edge)
			    /* Fill in dummy points at plot corners if the bounding curve */
			    /* went around the corner while out of range */
			    fill_missing_corners(corners, &points, 
				exit_edge, reentry_edge, out_updown, out_leftright);

			/* vector(map_x(ex),map_y(ey)); */
			corners[points].x = map_x(ex);
			corners[points++].y = map_y(ey);
			/* vector(x,y); */
			corners[points].x = x;
			corners[points++].y = y;

		    } else if (!clip_lines1) {
			finish_filled_curve(points, corners, plot);
			points = 0;
			/* move(x,y) */
			corners[points].x = x;
			corners[points++].y = y;

		    } else {
			finish_filled_curve(points, corners, plot);
			points = 0;
			edge_intersect(plot->points, i, &ex, &ey);
			/* move(map_x(ex),map_y(ey)); */
			corners[points].x = map_x(ex);
			corners[points++].y = map_y(ey);
			/* vector(x,y); */
			corners[points].x = x;
			corners[points++].y = y;
		    }
		} else {	/* prev == UNDEFINED */
		    finish_filled_curve(points, corners, plot);
		    points = 0;
		    /* move(x,y) */
		    corners[points].x = x;
		    corners[points++].y = y;
		    /* vector(x,y); */
		    corners[points].x = x;
		    corners[points++].y = y;
		}
		break;
	    }
	case OUTRANGE:{
		if (clip_fill) {
		    int where_was_I = clip_point(map_x(plot->points[i].x), map_y(plot->points[i].y));
		    if (where_was_I & (LEFT_EDGE|RIGHT_EDGE))
			out_leftright = where_was_I & (LEFT_EDGE|RIGHT_EDGE);
		    if (where_was_I & (TOP_EDGE|BOTTOM_EDGE))
			out_updown = where_was_I & (TOP_EDGE|BOTTOM_EDGE);
		}
		if (prev == INRANGE) {
		    /* from inrange to outrange */
		    if (clip_lines1 || clip_fill) {
			exit_edge = edge_intersect(plot->points, i, &ex, &ey);
			/* vector(map_x(ex),map_y(ey)); */
			corners[points].x = map_x(ex);
			corners[points++].y = map_y(ey);
		    }
		} else if (prev == OUTRANGE) {
		    /* from outrange to outrange */
		    if (clip_fill) {
			if (two_edge_intersect(plot->points, i, lx, ly)) {
			    coordinate temp;

			    /* vector(map_x(lx[0]),map_y(ly[0])); */
			    corners[points].x = map_x(lx[0]);
			    corners[points++].y = map_y(ly[0]);

			    /* Figure out which side we entered by */
			    temp.x = plot->points[i].x;
			    temp.y = plot->points[i].y;
			    plot->points[i].x = lx[1];
			    plot->points[i].y = ly[1];
			    reentry_edge = edge_intersect(plot->points, i, &ex, &ey);
			    plot->points[i].x = temp.x;
			    plot->points[i].y = temp.y;

			    if (!exit_edge) {
			    /* Curve must have started outside the plot area */
				first_entry = reentry_edge;
			    } else if (reentry_edge != exit_edge) {
				fill_missing_corners(corners, &points, exit_edge, reentry_edge,
					out_updown, out_leftright);
			    }
			    /* vector(map_x(lx[1]),map_y(ly[1])); */
			    corners[points].x = map_x(lx[1]);
			    corners[points++].y = map_y(ly[1]);

			    /* Figure out which side we left by */
			    temp.x = plot->points[i-1].x;
			    temp.y = plot->points[i-1].y;
			    plot->points[i-1].x = lx[0];
			    plot->points[i-1].y = ly[0];
			    exit_edge = edge_intersect(plot->points, i, &ex, &ey);
			    plot->points[i-1].x = temp.x;
			    plot->points[i-1].y = temp.y;
			}
		    }
		    else if (clip_lines2) {
			if (two_edge_intersect(plot->points, i, lx, ly)) {
			    finish_filled_curve(points, corners, plot);
			    points = 0;
			    /* move(map_x(lx[0]),map_y(ly[0])); */
			    corners[points].x = map_x(lx[0]);
			    corners[points++].y = map_y(ly[0]);
			    /* vector(map_x(lx[1]),map_y(ly[1])); */
			    corners[points].x = map_x(lx[1]);
			    corners[points++].y = map_y(ly[1]);
			}
		    }
		}
		break;
	    }
	case UNDEFINED:{
		/* UNDEFINED flags a blank line in the input file. 
		 * Unfortunately, it can also mean that the point was undefined.
		 * Is there a clean way to detect or handle the latter case?
		 */
		if (prev != UNDEFINED) {
		    if (first_entry && first_entry != exit_edge)
			fill_missing_corners(corners, &points, 
				exit_edge, first_entry, out_updown, out_leftright);
		    finish_filled_curve(points, corners, plot);
		    points = 0;
		    exit_edge = reentry_edge = first_entry = 0;
		}
		break;
	    }
	default:		/* just a safety */
		break;
	}
	prev = plot->points[i].type;
    }

    if (clip_fill) {   /* Did we finish cleanly, or is there an unresolved corner-crossing? */
	if (first_entry && first_entry != exit_edge) {
	    fill_missing_corners(corners, &points, exit_edge, first_entry,
		out_updown, out_leftright);
	}
    }
    
    finish_filled_curve(points, corners, plot);
}

/*
 * When the bounding curve of a filled area passes through the plot box but
 * exits through a different edge than it entered by, in order to properly
 * fill the enclosed area we must add dummy points at the plot corners.
 */
static void
fill_missing_corners(gpiPoint *corners, int *points, int exit, int reentry, int updown, int leftright)
{
    if ((exit | reentry) == (LEFT_EDGE | RIGHT_EDGE)) {
	corners[(*points)].x   = (exit & LEFT_EDGE)
			? map_x(X_AXIS.min) : map_x(X_AXIS.max);
	corners[(*points)++].y = (updown & TOP_EDGE)
			? map_y(Y_AXIS.max) : map_y(Y_AXIS.min);
	corners[(*points)].x   = (reentry & LEFT_EDGE)
			? map_x(X_AXIS.min) : map_x(X_AXIS.max);
	corners[(*points)++].y = (updown & TOP_EDGE)
			? map_y(Y_AXIS.max) : map_y(Y_AXIS.min);
    } else  if ((exit | reentry) == (BOTTOM_EDGE | TOP_EDGE)) {
	corners[(*points)].x   = (leftright & LEFT_EDGE)
			? map_x(X_AXIS.min) : map_x(X_AXIS.max);
	corners[(*points)++].y = (exit & TOP_EDGE)
			? map_y(Y_AXIS.max) : map_y(Y_AXIS.min);
	corners[(*points)].x   = (leftright & LEFT_EDGE)
			? map_x(X_AXIS.min) : map_x(X_AXIS.max);
	corners[(*points)++].y = (reentry & TOP_EDGE)
			? map_y(Y_AXIS.max) : map_y(Y_AXIS.min);
    } else {
	corners[(*points)].x   = (exit | reentry) & LEFT_EDGE
			? map_x(X_AXIS.min) : map_x(X_AXIS.max);
	corners[(*points)++].y = (exit | reentry) & TOP_EDGE
			? map_y(Y_AXIS.max) : map_y(Y_AXIS.min);
    }
}
/*
 * Fill the area between two curves
 */
static void
plot_betweencurves(struct curve_points *plot)
{
    double x1, x2, yl1, yu1, yl2, yu2;
    double xmid, ymid;
    double xu1, xu2;	/* For polar plots */
    int i;

    /* If terminal doesn't support filled polygons, approximate with bars */
    if (!term->filled_polygon) {
	plot_bars(plot);
	return;
    }

    /*
     * Fill the region one quadrilateral at a time.
     * Check each interval to see if the curves cross.
     * If so, split the interval into two parts.
     */
    for (i = 0; i < plot->p_count-1; i++) {

	/* FIXME: This isn't really testing for undefined points, it	*/
	/* is looking for blank lines. We need to distinguish these.	*/
	/* Anyhow, if there's a blank line then start a new fill area.	*/
	if (plot->points[i].type == UNDEFINED
	    || plot->points[i+1].type == UNDEFINED)
	    continue;

	x1  = plot->points[i].x;
	xu1 = plot->points[i].xhigh;
	yl1 = plot->points[i].y;
	yu1 = plot->points[i].yhigh;
	x2  = plot->points[i+1].x;
	xu2 = plot->points[i+1].xhigh;
	yl2 = plot->points[i+1].y;
	yu2 = plot->points[i+1].yhigh;

	/* EAM 19-July-2007  Special case for polar plots. */
	if (polar) {
	    /* Find intersection of the two lines.                   */
	    /* Probably could use this code in the general case too. */
	    double A = (yl2-yl1) / (x2-x1);
	    double C = (yu2-yu1) / (xu2-xu1);
	    double b = yl1 - x1 * A;
	    double d = yu1 - xu1 * C;
	    xmid = (d-b) / (A-C);
	    ymid = A * xmid + b;

	    if ((x1-xmid)*(xmid-x2) > 0) {
		fill_between(x1,xu1,yl1,yu1, xmid,xmid,ymid,ymid,plot);
		fill_between(xmid,xmid,ymid,ymid, x2,xu2,yl2,yu2,plot);
	    } else
		fill_between(x1,xu1,yl1,yu1, x2,xu2,yl2,yu2,plot);

	} else if ((yu1-yl1)*(yu2-yl2) < 0) {
	    /* Cheap test for intersection in the general case */
	    xmid = (x1*(yl2-yu2) + x2*(yu1-yl1))
		 / ((yu1-yl1) + (yl2-yu2));
	    ymid = yu1 + (yu2-yu1)*(xmid-x1)/(x2-x1);
	    fill_between(x1,xu1,yl1,yu1, xmid,xmid,ymid,ymid,plot);
	    fill_between(xmid,xmid,ymid,ymid, x2,xu2,yl2,yu2,plot);

	} else
	    fill_between(x1,xu1,yl1,yu1, x2,xu2,yl2,yu2,plot);

    }
}

static void
fill_between(
double x1, double xu1, double yl1, double yu1, 
double x2, double xu2, double yl2, double yu2,
struct curve_points *plot)
{
    double xmin, xmax, ymin, ymax, dx, dy1, dy2;
    int axis;
    int ic, iy;
    gpiPoint box[8];
    struct { double x,y; } corners[8];

    /* Clip against x-axis range */
    /* It would be nice if we could trust xmin to be less than xmax */
	axis = plot->x_axis;
	xmin = GPMIN(axis_array[axis].min, axis_array[axis].max);
	xmax = GPMAX(axis_array[axis].min, axis_array[axis].max);
	if (!(inrange(x1, xmin, xmax)) && !(inrange(x2, xmin, xmax)))
	    return;
	
    /* Clip end segments. It would be nice to use edge_intersect() here, */
    /* but as currently written it cannot handle the second curve.       */
	dx = x2 - x1;
	if (x1<xmin) {
	    yl1 += (yl2-yl1) * (xmin - x1) / dx;
	    yu1 += (yu2-yu1) * (xmin - x1) / dx;
	    x1 = xmin;
	}
	if (x2>xmax) {
	    yl2 += (yl2-yl1) * (xmax - x2) / dx;
	    yu2 += (yu2-yu1) * (xmax - x2) / dx;
	    x2 = xmax;
	}
	if (!polar) {
	    xu1 = x1;
	    xu2 = x2;
	}

    /* Clip against y-axis range */
	axis = plot->y_axis;
	ymin = GPMIN(axis_array[axis].min, axis_array[axis].max);
	ymax = GPMAX(axis_array[axis].min, axis_array[axis].max);
	if (yl1<ymin && yu1<ymin && yl2<ymin && yu2<ymin)
	    return;
	if (yl1>ymax && yu1>ymax && yl2>ymax && yu2>ymax)
	    return;

	ic = 0;
	corners[ic].x   = map_x(x1);
	corners[ic++].y = map_y(yl1);
	corners[ic].x   = map_x(xu1);
	corners[ic++].y = map_y(yu1);

#define INTERPOLATE(Y1,Y2,YBOUND) do { \
	dy1 = YBOUND - Y1; \
	dy2 = YBOUND - Y2; \
	if (dy1 != dy2 && dy1*dy2 < 0) { \
	    corners[ic].y = map_y(YBOUND); \
	    corners[ic++].x = map_x(x1 + dx * dy1 / (dy1-dy2)); \
	} \
	} while (0)

	INTERPOLATE( yu1, yu2, ymin );
	INTERPOLATE( yu1, yu2, ymax );
	
	corners[ic].x   = map_x(xu2);
	corners[ic++].y = map_y(yu2);
	corners[ic].x   = map_x(x2);
	corners[ic++].y = map_y(yl2);

	INTERPOLATE( yl1, yl2, ymin );
	INTERPOLATE( yl1, yl2, ymax );

#undef INTERPOLATE

    /* Copy the polygon vertices into a gpiPoints structure */
	for (iy=0; iy<ic; iy++) {
	    box[iy].x = corners[iy].x;
	    cliptorange(corners[iy].y, map_y(ymin), map_y(ymax));
	    box[iy].y = corners[iy].y;
	}

    /* finish_filled_curve() will handle   */
    /* current fill style (stored in plot) */
    /* above/below (stored in box[ic].x)   */
	if (polar) {
	    /* "above" or "below" evaluated in terms of radial distance from origin */
	    /* FIXME: Most of this should be offloaded to a separate subroutine */
	    double ox = map_x(0);
	    double oy = map_y(0);
	    double plx = map_x(x1);
	    double ply = map_y(yl1);
	    double pux = map_x(xu1);
	    double puy = map_y(yu1);
	    double drl = (plx-ox)*(plx-ox) + (ply-oy)*(ply-oy);
	    double dru = (pux-ox)*(pux-ox) + (puy-oy)*(puy-oy);
	    double dx1 = dru - drl;

	    double dx2;
	    plx = map_x(x2);
	    ply = map_y(yl2);
	    pux = map_x(xu2);
	    puy = map_y(yu2);
	    drl = (plx-ox)*(plx-ox) + (ply-oy)*(ply-oy);
	    dru = (pux-ox)*(pux-ox) + (puy-oy)*(puy-oy);
	    dx2 = dru - drl;
	    
	    box[ic].x = (dx1+dx2 < 0) ? 1 : 0;
	} else
	    box[ic].x = ((yu1-yl1) + (yu2-yl2) < 0) ? 1 : 0;
	
	finish_filled_curve(ic, box, plot);
}


/* XXX - JG  */
/* plot_steps:
 * Plot the curves in STEPS style
 */
static void
plot_steps(struct curve_points *plot)
{
    int i;			/* point index */
    int x, y;			/* point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double ex, ey;		/* an edge point */
    double lx[2], ly[2];	/* two edge points */
    int yprev = 0;		/* previous point coordinates */

    for (i = 0; i < plot->p_count; i++) {
	switch (plot->points[i].type) {
	case INRANGE:{
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);

		if (prev == INRANGE) {
		    (*t->vector) (x, yprev);
		    (*t->vector) (x, y);
		} else if (prev == OUTRANGE) {
		    /* from outrange to inrange */
		    if (!clip_lines1) {
			(*t->move) (x, y);
		    } else {	/* find edge intersection */
			edge_intersect_steps(plot->points, i, &ex, &ey);
			(*t->move) (map_x(ex), map_y(ey));
			(*t->vector) (x, map_y(ey));
			(*t->vector) (x, y);
		    }
		} else {	/* prev == UNDEFINED */
		    (*t->move) (x, y);
		    (*t->vector) (x, y);
		}
		yprev = y;
		break;
	    }
	case OUTRANGE:{
		if (prev == INRANGE) {
		    /* from inrange to outrange */
		    if (clip_lines1) {
			edge_intersect_steps(plot->points, i, &ex, &ey);
			(*t->vector) (map_x(ex), yprev);
			(*t->vector) (map_x(ex), map_y(ey));
		    }
		} else if (prev == OUTRANGE) {
		    /* from outrange to outrange */
		    if (clip_lines2) {
			if (two_edge_intersect_steps(plot->points, i, lx, ly)) {
			    (*t->move) (map_x(lx[0]), map_y(ly[0]));
			    (*t->vector) (map_x(lx[1]), map_y(ly[0]));
			    (*t->vector) (map_x(lx[1]), map_y(ly[1]));
			}
		    }
		}
		break;
	    }
	default:		/* just a safety */
	case UNDEFINED:{
		break;
	    }
	}
	prev = plot->points[i].type;
    }
}

/* XXX - HOE  */
/* plot_fsteps:
 * Plot the curves in STEPS style by step on forward yvalue
 */
static void
plot_fsteps(struct curve_points *plot)
{
    int i;			/* point index */
    int x, y;			/* point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double ex, ey;		/* an edge point */
    double lx[2], ly[2];	/* two edge points */
    int xprev = 0;		/* previous point coordinates */

    for (i = 0; i < plot->p_count; i++) {
	switch (plot->points[i].type) {
	case INRANGE:{
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);

		if (prev == INRANGE) {
		    (*t->vector) (xprev, y);
		    (*t->vector) (x, y);
		} else if (prev == OUTRANGE) {
		    /* from outrange to inrange */
		    if (!clip_lines1) {
			(*t->move) (x, y);
		    } else {	/* find edge intersection */
			edge_intersect_fsteps(plot->points, i, &ex, &ey);
			(*t->move) (map_x(ex), map_y(ey));
			(*t->vector) (map_x(ex), y);
			(*t->vector) (x, y);
		    }
		} else {	/* prev == UNDEFINED */
		    (*t->move) (x, y);
		    (*t->vector) (x, y);
		}
		xprev = x;
		break;
	    }
	case OUTRANGE:{
		if (prev == INRANGE) {
		    /* from inrange to outrange */
		    if (clip_lines1) {
			edge_intersect_fsteps(plot->points, i, &ex, &ey);
			(*t->vector) (xprev, map_y(ey));
			(*t->vector) (map_x(ex), map_y(ey));
		    }
		} else if (prev == OUTRANGE) {
		    /* from outrange to outrange */
		    if (clip_lines2) {
			if (two_edge_intersect_fsteps(plot->points, i, lx, ly)) {
			    (*t->move) (map_x(lx[0]), map_y(ly[0]));
			    (*t->vector) (map_x(lx[0]), map_y(ly[1]));
			    (*t->vector) (map_x(lx[1]), map_y(ly[1]));
			}
		    }
		}
		break;
	    }
	default:		/* just a safety */
	case UNDEFINED:{
		break;
	    }
	}
	prev = plot->points[i].type;
    }
}

/* HBB 20010625: replaced homegrown bubblesort in plot_histeps() by
 * call of standard routine qsort(). Need to tell the compare function
 * about the plotted dataset via this file scope variable: */
static struct curve_points *histeps_current_plot;

/* NOTE: I'd have made the comp.function 'static', but the HP-sUX gcc
 * bug seems to forbid that :-( */
int
histeps_compare(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2)
{
    double x1=histeps_current_plot->points[*(int *)p1].x;
    double x2=histeps_current_plot->points[*(int *)p2].x;

    if (x1 < x2)
	return -1;
    else
	return (x1 > x2);
}

/* CAC  */
/* plot_histeps:
 * Plot the curves in HISTEPS style
 */
static void
plot_histeps(struct curve_points *plot)
{
    int i;			/* point index */
    int xl, yl;			/* cursor position in terminal coordinates */
    struct termentry *t = term;
    double x, y, xn, yn;	/* point position */
    double y_null;		/* y coordinate of histogram baseline */
    int *gl, goodcount;		/* array to hold list of valid points */

    /* preliminary count of points inside array */
    goodcount = 0;
    for (i = 0; i < plot->p_count; i++)
	if (plot->points[i].type == INRANGE || plot->points[i].type == OUTRANGE)
	    ++goodcount;
    if (goodcount < 2)
	return;			/* cannot plot less than 2 points */

    gl = gp_alloc(goodcount * sizeof(int), "histeps valid point mapping");

    /* fill gl array with indexes of valid (non-undefined) points.  */
    goodcount = 0;
    for (i = 0; i < plot->p_count; i++)
	if (plot->points[i].type == INRANGE || plot->points[i].type == OUTRANGE) {
	    gl[goodcount] = i;
	    ++goodcount;
	}

    /* sort the data --- tell histeps_compare about the plot
     * datastructure to look at, then call qsort() */
    histeps_current_plot = plot;
    qsort(gl, goodcount, sizeof(*gl), histeps_compare);
    /* play it safe: invalidate the static pointer after usage */
    histeps_current_plot = NULL;

    /* HBB 20010625: log y axis must treat 0.0 as -infinity. Define
     * the correct y position for the histogram's baseline once. It'll
     * be used twice (once for each endpoint of the histogram). */
    if (Y_AXIS.log)
	y_null = GPMIN(Y_AXIS.min, Y_AXIS.max);
    else
	y_null = 0.0;

    x = (3.0 * plot->points[gl[0]].x - plot->points[gl[1]].x) / 2.0;
    y = y_null;

    xl = map_x(x);
    yl = map_y(y);

    if (!clip_point(xl,yl))
	(*t->move) (xl, yl);

    for (i = 0; i < goodcount - 1; i++) {	/* loop over all points except last  */
	yn = plot->points[gl[i]].y;
	xn = (plot->points[gl[i]].x + plot->points[gl[i + 1]].x) / 2.0;
	histeps_vertical(&xl, &yl, x, y, yn);
	histeps_horizontal(&xl, &yl, x, xn, yn);

	x = xn;
	y = yn;
    }

    yn = plot->points[gl[i]].y;
    xn = (3.0 * plot->points[gl[i]].x - plot->points[gl[i - 1]].x) / 2.0;
    histeps_vertical(&xl, &yl, x, y, yn);
    histeps_horizontal(&xl, &yl, x, xn, yn);
    histeps_vertical(&xl, &yl, xn, yn, y_null);

    free(gl);
}

/* CAC
 * Draw vertical line for the histeps routine.
 * Performs clipping.
 */
/* HBB 20010214: renamed parameters. xl vs. x1 is just _too_ easy to
 * mis-read */
static void
histeps_vertical(
    int *cur_x, int *cur_y,	/* keeps track of "cursor" position */
    double x,
    double y1, double y2)	/* coordinates of vertical line */
{
    struct termentry *t = term;
    int xm, y1m, y2m;

    /* FIXME HBB 20010215: wouldn't it be simpler to call
     * draw_clip_line() instead? And in histeps_horizontal(), too, of
     * course? */

    /* HBB 20010215: reversed axes need special treatment, here: */
    if (X_AXIS.min <= X_AXIS.max) {
	if ((x < X_AXIS.min) || (x > X_AXIS.max))
	    return;
    } else {
	if ((x < X_AXIS.max) || (x > X_AXIS.min))
	    return;
    }

    if (Y_AXIS.min <= Y_AXIS.max) {
	if ((y1 < Y_AXIS.min && y2 < Y_AXIS.min)
	    || (y1 > Y_AXIS.max && y2 > Y_AXIS.max))
	    return;
	if (y1 < Y_AXIS.min)
	    y1 = Y_AXIS.min;
	if (y1 > Y_AXIS.max)
	    y1 = Y_AXIS.max;
	if (y2 < Y_AXIS.min)
	    y2 = Y_AXIS.min;
	if (y2 > Y_AXIS.max)
	    y2 = Y_AXIS.max;
    } else {
	if ((y1 < Y_AXIS.max && y2 < Y_AXIS.max)
	    || (y1 > Y_AXIS.min && y2 > Y_AXIS.min))
	    return;

	if (y1 < Y_AXIS.max)
	    y1 = Y_AXIS.max;
	if (y1 > Y_AXIS.min)
	    y1 = Y_AXIS.min;
	if (y2 < Y_AXIS.max)
	    y2 = Y_AXIS.max;
	if (y2 > Y_AXIS.min)
	    y2 = Y_AXIS.min;
    }
    xm = map_x(x);
    y1m = map_y(y1);
    y2m = map_y(y2);

    if (y1m != *cur_y || xm != *cur_x)
	(*t->move) (xm, y1m);
    (*t->vector) (xm, y2m);
    *cur_x = xm;
    *cur_y = y2m;

    return;
}

/* CAC
 * Draw horizontal line for the histeps routine.
 * Performs clipping.
 */
static void
histeps_horizontal(
    int *cur_x, int *cur_y,	/* keeps track of "cursor" position */
    double x1, double x2,
    double y)			/* coordinates of vertical line */
{
    struct termentry *t = term;
    int x1m, x2m, ym;

    /* HBB 20010215: reversed axes need special treatment, here: */

    if (Y_AXIS.min <= Y_AXIS.max) {
	if ((y < Y_AXIS.min) || (y > Y_AXIS.max))
	    return;
    } else {
	if ((y < Y_AXIS.max) || (y > Y_AXIS.min))
	    return;
    }

    if (X_AXIS.min <= X_AXIS.max) {
	if ((x1 < X_AXIS.min && x2 < X_AXIS.min)
	    || (x1 > X_AXIS.max && x2 > X_AXIS.max))
	    return;

	if (x1 < X_AXIS.min)
	    x1 = X_AXIS.min;
	if (x1 > X_AXIS.max)
	    x1 = X_AXIS.max;
	if (x2 < X_AXIS.min)
	    x2 = X_AXIS.min;
	if (x2 > X_AXIS.max)
	    x2 = X_AXIS.max;
    } else {
	if ((x1 < X_AXIS.max && x2 < X_AXIS.max)
	    || (x1 > X_AXIS.min && x2 > X_AXIS.min))
	    return;

	if (x1 < X_AXIS.max)
	    x1 = X_AXIS.max;
	if (x1 > X_AXIS.min)
	    x1 = X_AXIS.min;
	if (x2 < X_AXIS.max)
	    x2 = X_AXIS.max;
	if (x2 > X_AXIS.min)
	    x2 = X_AXIS.min;
    }
    ym = map_y(y);
    x1m = map_x(x1);
    x2m = map_x(x2);

    if (x1m != *cur_x || ym != *cur_y)
	(*t->move) (x1m, ym);
    (*t->vector) (x2m, ym);
    *cur_x = x2m;
    *cur_y = ym;

    return;
}


/* plot_bars:
 * Plot the curves in ERRORBARS style
 *  we just plot the bars; the points are plotted in plot_points
 */
static void
plot_bars(struct curve_points *plot)
{
    int i;			/* point index */
    struct termentry *t = term;
    double x, y;		/* position of the bar */
    double ylow, yhigh;		/* the ends of the bars */
    double xlow, xhigh;
    double x1, y1, x2, y2, slope;	/* parameters for polar error bars */
    unsigned int xM, ylowM, yhighM;	/* the mapped version of above */
    unsigned int yM, xlowM, xhighM;
    TBOOLEAN low_inrange, high_inrange;
    int tic = ERRORBARTIC;
    double halfwidth = 0;		/* Used to calculate full box width */

    /* Limitation: no boxes with x errorbars */

    if ((plot->plot_style == YERRORBARS)
	|| (plot->plot_style == XYERRORBARS)
	|| (plot->plot_style == BOXERROR)
	|| (plot->plot_style == YERRORLINES)
	|| (plot->plot_style == XYERRORLINES)
	|| (plot->plot_style == HISTOGRAMS)
	|| (plot->plot_style == FILLEDCURVES) /* Only if term has no filled_polygon! */
	) {
	/* Draw the vertical part of the bar */
	for (i = 0; i < plot->p_count; i++) {
	    /* undefined points don't count */
	    if (plot->points[i].type == UNDEFINED)
		continue;

	    /* check to see if in xrange */
	    x = plot->points[i].x;

	    if (plot->plot_style == HISTOGRAMS) {
		/* Shrink each cluster to fit within one unit along X axis,   */
		/* centered about the integer representing the cluster number */
		/* 'start' is reset to 0 at the top of eval_plots(), and then */
		/* incremented if 'plot new histogram' is encountered.        */
		int clustersize = plot->histogram->clustersize + histogram_opts.gap;
		x  += (i-1) * (clustersize - 1) + plot->histogram_sequence;
		x  += histogram_opts.gap/2;
		x  /= clustersize;
		x  += plot->histogram->start + 0.5;
		/* Calculate width also */
		halfwidth = (plot->points[i].xhigh - plot->points[i].xlow)
			  / (2. * clustersize);
	    }

	    if (!inrange(x, X_AXIS.min, X_AXIS.max))
		continue;
	    xM = map_x(x);

	    /* check to see if in yrange */
	    y = plot->points[i].y;
	    if (!inrange(y, Y_AXIS.min, Y_AXIS.max))
		continue;
	    yM = map_y(y);

	    /* find low and high points of bar, and check yrange */
	    yhigh = plot->points[i].yhigh;
	    ylow = plot->points[i].ylow;

	    high_inrange = inrange(yhigh, Y_AXIS.min, Y_AXIS.max);
	    low_inrange = inrange(ylow, Y_AXIS.min, Y_AXIS.max);

	    /* compute the plot position of yhigh */
	    if (high_inrange)
		yhighM = map_y(yhigh);
	    else if (samesign(yhigh - Y_AXIS.max, Y_AXIS.max - Y_AXIS.min))
		yhighM = map_y(Y_AXIS.max);
	    else
		yhighM = map_y(Y_AXIS.min);

	    /* compute the plot position of ylow */
	    if (low_inrange)
		ylowM = map_y(ylow);
	    else if (samesign(ylow - Y_AXIS.max, Y_AXIS.max - Y_AXIS.min))
		ylowM = map_y(Y_AXIS.max);
	    else
		ylowM = map_y(Y_AXIS.min);

	    if (!high_inrange && !low_inrange && ylowM == yhighM)
		/* both out of range on the same side */
		continue;

	    /* find low and high points of bar, and check xrange */
	    xhigh = plot->points[i].xhigh;
	    xlow = plot->points[i].xlow;

	    if (plot->plot_style == HISTOGRAMS) {
		xlowM = map_x(x-halfwidth);
		xhighM = map_x(x+halfwidth);
	    } else {
		high_inrange = inrange(xhigh, X_AXIS.min, X_AXIS.max);
		low_inrange = inrange(xlow, X_AXIS.min, X_AXIS.max);

		/* compute the plot position of xhigh */
		if (high_inrange)
		    xhighM = map_x(xhigh);
		else if (samesign(xhigh - X_AXIS.max, X_AXIS.max - X_AXIS.min))
		    xhighM = map_x(X_AXIS.max);
		else
		    xhighM = map_x(X_AXIS.min);

		/* compute the plot position of xlow */
		if (low_inrange)
		    xlowM = map_x(xlow);
		else if (samesign(xlow - X_AXIS.max, X_AXIS.max - X_AXIS.min))
		    xlowM = map_x(X_AXIS.max);
		else
		    xlowM = map_x(X_AXIS.min);

		if (!high_inrange && !low_inrange && xlowM == xhighM)
		    /* both out of range on the same side */
		    continue;
	    }

	    /* by here everything has been mapped */
	    if (!polar) {
		/* HBB 981130: use Igor's routine *only* for polar errorbars */
		(*t->move) (xM, ylowM);
		/* draw the main bar */
		(*t->vector) (xM, yhighM);
		if (bar_size < 0.0) {
		    /* draw the bottom tic same width as box */
		    (*t->move) ((unsigned int) (xlowM), ylowM);
		    (*t->vector) ((unsigned int) (xhighM), ylowM);
		    /* draw the top tic same width as box */
		    (*t->move) ((unsigned int) (xlowM), yhighM);
		    (*t->vector) ((unsigned int) (xhighM), yhighM);
		} else if (bar_size > 0.0) {
		    /* draw the bottom tic */
		    (*t->move) ((unsigned int) (xM - bar_size * tic), ylowM);
		    (*t->vector) ((unsigned int) (xM + bar_size * tic), ylowM);
		    /* draw the top tic */
		    (*t->move) ((unsigned int) (xM - bar_size * tic), yhighM);
		    (*t->vector) ((unsigned int) (xM + bar_size * tic), yhighM);
		}
	    } else {
		/* HBB 981130: see above */
		/* The above has been replaced by Igor inorder to get errorbars
		   coming out in polar mode AND to stop the bar from going
		   through the symbol */
		if ((xhighM - xlowM) * (xhighM - xlowM) + (yhighM - ylowM) * (yhighM - ylowM)
		    > pointsize * tic * pointsize * tic * 4.5) {
		    /* Only plot the error bar if it is bigger than the
		     * symbol */
		    /* The factor of 4.5 should strictly be 4.0, but it looks
		     * better to drop the error bar if it is only slightly
		     * bigger than the symbol, Igor. */
		    if (xlowM == xhighM) {
			(*t->move) (xM, ylowM);
			/* draw the main bar to the symbol end */
			(*t->vector) (xM, (unsigned int) (yM - pointsize * tic));
			(*t->move) (xM, (unsigned int) (yM + pointsize * tic));
			/* draw the other part of the main bar */
			(*t->vector) (xM, yhighM);
		    } else {
			(*t->move) (xlowM, ylowM);
			/* draw the main bar in polar mode. Note that here
			 * the bar is drawn through the symbol. I tried to
			 * fix this, but got into trouble with the two bars
			 * (on either side of symbol) not being perfectly
			 * parallel due to mapping considerations. Igor
			 */
			(*t->vector) (xhighM, yhighM);
		    }
		    if (bar_size > 0.0) {
			/* The following attempts to ensure that the tics
			 * are perpendicular to the error bar, Igor. */
			/*perpendicular to the main bar */
			slope = (xlowM * 1.0 - xhighM * 1.0) / (yhighM * 1.0 - ylowM * 1.0 + 1e-10);
			x1 = xlowM + bar_size * tic / sqrt(1.0 + slope * slope);
			x2 = xlowM - bar_size * tic / sqrt(1.0 + slope * slope);
			y1 = slope * (x1 - xlowM) + ylowM;
			y2 = slope * (x2 - xlowM) + ylowM;

			/* draw the bottom tic */
			(*t->move) ((unsigned int) x1, (unsigned int) y1);
			(*t->vector) ((unsigned int) x2, (unsigned int) y2);

			x1 = xhighM + bar_size * tic / sqrt(1.0 + slope * slope);
			x2 = xhighM - bar_size * tic / sqrt(1.0 + slope * slope);
			y1 = slope * (x1 - xhighM) + yhighM;
			y2 = slope * (x2 - xhighM) + yhighM;
			/* draw the top tic */
			(*t->move) ((unsigned int) x1, (unsigned int) y1);
			(*t->vector) ((unsigned int) x2, (unsigned int) y2);
		    }		/* if error bar is bigger than symbol */
		}
	    }			/* HBB 981130: see above */
	}			/* for loop */
    }				/* if yerrorbars OR xyerrorbars OR yerrorlines OR xyerrorlines */
    if ((plot->plot_style == XERRORBARS)
	|| (plot->plot_style == XYERRORBARS)
	|| (plot->plot_style == XERRORLINES)
	|| (plot->plot_style == XYERRORLINES)) {

	/* Draw the horizontal part of the bar */
	for (i = 0; i < plot->p_count; i++) {
	    /* undefined points don't count */
	    if (plot->points[i].type == UNDEFINED)
		continue;

	    /* check to see if in yrange */
	    y = plot->points[i].y;
	    if (!inrange(y, Y_AXIS.min, Y_AXIS.max))
		continue;
	    yM = map_y(y);

	    /* find low and high points of bar, and check xrange */
	    xhigh = plot->points[i].xhigh;
	    xlow = plot->points[i].xlow;

	    high_inrange = inrange(xhigh, X_AXIS.min, X_AXIS.max);
	    low_inrange = inrange(xlow, X_AXIS.min, X_AXIS.max);

	    /* compute the plot position of xhigh */
	    if (high_inrange)
		xhighM = map_x(xhigh);
	    else if (samesign(xhigh - X_AXIS.max, X_AXIS.max - X_AXIS.min))
		xhighM = map_x(X_AXIS.max);
	    else
		xhighM = map_x(X_AXIS.min);

	    /* compute the plot position of xlow */
	    if (low_inrange)
		xlowM = map_x(xlow);
	    else if (samesign(xlow - X_AXIS.max, X_AXIS.max - X_AXIS.min))
		xlowM = map_x(X_AXIS.max);
	    else
		xlowM = map_x(X_AXIS.min);

	    if (!high_inrange && !low_inrange && xlowM == xhighM)
		/* both out of range on the same side */
		continue;

	    /* by here everything has been mapped */
	    (*t->move) (xlowM, yM);
	    (*t->vector) (xhighM, yM);	/* draw the main bar */
	    if (bar_size > 0.0) {
		(*t->move) (xlowM, (unsigned int) (yM - bar_size * tic));	/* draw the left tic */
		(*t->vector) (xlowM, (unsigned int) (yM + bar_size * tic));
		(*t->move) (xhighM, (unsigned int) (yM - bar_size * tic));	/* draw the right tic */
		(*t->vector) (xhighM, (unsigned int) (yM + bar_size * tic));
	    }
	}			/* for loop */
    }				/* if xerrorbars OR xyerrorbars OR xerrorlines OR xyerrorlines */
}

/* plot_boxes:
 * EAM Sep 2002 - Consolidate BOXES and FILLEDBOXES
 */
static void
plot_boxes(struct curve_points *plot, int xaxis_y)
{
    int i;			/* point index */
    int xl, xr, yb, yt;		/* point in terminal coordinates */
    double dxl, dxr, dyt;
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double dyb = 0.0;

    /* The stackheight[] array contains the y coord of the top   */
    /* of the stack so far for each point.                       */
    if (plot->plot_style == HISTOGRAMS) {
	int newsize = plot->p_count;
	if (histogram_opts.type == HT_STACKED_IN_TOWERS)
	    stack_count = 0;
	if (histogram_opts.type == HT_STACKED_IN_LAYERS && plot->histogram_sequence == 0)
	    stack_count = 0;
	if (!stackheight) {
	    stackheight = gp_alloc(
				newsize * sizeof(struct coordinate GPHUGE),
				"stackheight array");
	    for (i = 0; i < newsize; i++) {
		stackheight[i].yhigh = 0;
		stackheight[i].ylow = 0;
	    }
	    stack_count = newsize;
	} else if (stack_count < newsize) {
	    stackheight = gp_realloc( stackheight,
				newsize * sizeof(struct coordinate GPHUGE),
				"stackheight array");
	    for (i = stack_count; i < newsize; i++) {
		stackheight[i].yhigh = 0;
		stackheight[i].ylow = 0;
	    }
	    stack_count = newsize;
	}
    }

    for (i = 0; i < plot->p_count; i++) {

	switch (plot->points[i].type) {
	case OUTRANGE:
	case INRANGE:{
		if (plot->points[i].z < 0.0) {
		    /* need to auto-calc width */
		    if (prev != UNDEFINED)
			if (boxwidth < 0)
			    dxl = (plot->points[i-1].x - plot->points[i].x) / 2.0;
			else if (! boxwidth_is_absolute)
			    dxl = (plot->points[i-1].x - plot->points[i].x) * boxwidth / 2.0;
			else /* Hits here on 3 column BOXERRORBARS */
			    dxl = -boxwidth / 2.0;
		    else
			dxl = 0.0;

		    if (i < plot->p_count - 1) {
			if (plot->points[i + 1].type != UNDEFINED)
			    if (boxwidth < 0)
				dxr = (plot->points[i+1].x - plot->points[i].x) / 2.0;
			    else if (! boxwidth_is_absolute)
				dxr = (plot->points[i+1].x - plot->points[i].x) * boxwidth / 2.0;
			    else /* Hits here on 3 column BOXERRORBARS */
				dxr = boxwidth / 2.0;
			else
			    dxr = -dxl;
		    } else {
			dxr = -dxl;
		    }

		    if (prev == UNDEFINED)
			dxl = -dxr;

		    dxl = plot->points[i].x + dxl;
		    dxr = plot->points[i].x + dxr;
		} else {	/* z >= 0 */
		    dxr = plot->points[i].xhigh;
		    dxl = plot->points[i].xlow;
		}

		/* HBB 20040521: ylow should be clipped to the y range. */
		if (plot->plot_style == BOXXYERROR) {
		    double temp_y = plot->points[i].ylow;

		    cliptorange(temp_y, Y_AXIS.min, Y_AXIS.max);
		    xaxis_y = map_y(temp_y);
		    dyt = plot->points[i].yhigh;
		} else {
		    dyt = plot->points[i].y;
		}

		if (plot->plot_style == HISTOGRAMS) {
		    int ix = i;
		    int histogram_linetype = i;
		    if (plot->histogram->startcolor > 0)
			histogram_linetype += plot->histogram->startcolor;

		    /* Shrink each cluster to fit within one unit along X axis,   */
		    /* centered about the integer representing the cluster number */
		    /* 'start' is reset to 0 at the top of eval_plots(), and then */
		    /* incremented if 'plot new histogram' is encountered.        */
		    if (histogram_opts.type == HT_CLUSTERED
		    ||  histogram_opts.type == HT_ERRORBARS) {
			int clustersize = plot->histogram->clustersize + histogram_opts.gap;
			dxl  += (i-1) * (clustersize - 1) + plot->histogram_sequence;
			dxr  += (i-1) * (clustersize - 1) + plot->histogram_sequence;
			dxl  += histogram_opts.gap/2;
			dxr  += histogram_opts.gap/2;
			dxl  /= clustersize;
			dxr  /= clustersize;
			dxl  += plot->histogram->start + 0.5;
			dxr  += plot->histogram->start + 0.5;
		    } else if (histogram_opts.type == HT_STACKED_IN_TOWERS) {
			dxl  = plot->histogram->start - boxwidth / 2.0;
			dxr  = plot->histogram->start + boxwidth / 2.0;
			dxl += plot->histogram_sequence;
			dxr += plot->histogram_sequence;
		    } else if (histogram_opts.type == HT_STACKED_IN_LAYERS) {
			dxl += plot->histogram->start;
			dxr += plot->histogram->start;
		    }

		    switch (histogram_opts.type) {
		    case HT_STACKED_IN_TOWERS:
			ix = 0;
			/* Line type (color) must match row number */
			if (prefer_line_styles) {
			    struct lp_style_type ls;
			    lp_use_properties(&ls, histogram_linetype+1);
			    apply_pm3dcolor(&ls.pm3d_color, term);
			} else
			    (*t->linetype)(histogram_linetype);
			plot->fill_properties.fillpattern = histogram_linetype;
			/* Fall through */
		    case HT_STACKED_IN_LAYERS:

			if( plot->points[i].y >= 0 ){
			    dyb = stackheight[ix].yhigh;
			    dyt += stackheight[ix].yhigh;
			    stackheight[ix].yhigh += plot->points[i].y;
			} else {
			    dyb = stackheight[ix].ylow;
			    dyt += stackheight[ix].ylow;
			    stackheight[ix].ylow += plot->points[i].y;
			}

			if ((Y_AXIS.min < Y_AXIS.max && dyb < Y_AXIS.min)
			||  (Y_AXIS.max < Y_AXIS.min && dyb > Y_AXIS.min))
			    dyb = Y_AXIS.min;
			if ((Y_AXIS.min < Y_AXIS.max && dyb > Y_AXIS.max)
			||  (Y_AXIS.max < Y_AXIS.min && dyb < Y_AXIS.max))
			    dyb = Y_AXIS.max;
			break;
		    case HT_CLUSTERED:
		    case HT_ERRORBARS:
			break;
		    }
		}

		/* clip to border */
		cliptorange(dyt, Y_AXIS.min, Y_AXIS.max);
		cliptorange(dxr, X_AXIS.min, X_AXIS.max);
		cliptorange(dxl, X_AXIS.min, X_AXIS.max);

		xl = map_x(dxl);
		xr = map_x(dxr);
		yt = map_y(dyt);
		yb = xaxis_y;

		if (plot->plot_style == HISTOGRAMS
		&& (histogram_opts.type == HT_STACKED_IN_LAYERS
		    || histogram_opts.type == HT_STACKED_IN_TOWERS))
			yb = map_y(dyb);

		/* Variable color */
		if (plot->plot_style == BOXES) {
		    check_for_variable_color(plot, &plot->points[i]);
		}

		if ((plot->fill_properties.fillstyle != FS_EMPTY) && t->fillbox) {
		    int x, y, w, h;
		    int style;

		    x = xl;
		    y = yb;
		    w = xr - xl + 1;
		    h = yt - yb + 1;
		    /* avoid negative width/height */
		    if( w <= 0 ) {
			x = xr;
			w = xl - xr + 1;
		    }
		    if( h <= 0 ) {
			y = yt;
			h = yb - yt + 1;
		    }

		    style = style_from_fill(&plot->fill_properties);
		    (*t->fillbox) (style, x, y, w, h);

		    if (!need_fill_border(&plot->fill_properties))
			break;
		}
		newpath();
		(*t->move) (xl, yb);
		(*t->vector) (xl, yt);
		(*t->vector) (xr, yt);
		(*t->vector) (xr, yb);
		(*t->vector) (xl, yb);
		closepath();

		if( t->fillbox && plot->fill_properties.border_color.type != TC_DEFAULT) {
		    (*t->linetype)(plot->lp_properties.l_type);
		    if (plot->lp_properties.use_palette)
			apply_pm3dcolor(&plot->lp_properties.pm3d_color,t);
		}

		break;
	    }			/* case OUTRANGE, INRANGE */

	default:		/* just a safety */
	case UNDEFINED:{
		break;
	    }

	}			/* switch point-type */

	prev = plot->points[i].type;

    }				/*loop */
}



/* plot_points:
 * Plot the curves in POINTSTYLE style
 */
static void
plot_points(struct curve_points *plot)
{
    int i;
    int x, y;
    int interval = plot->lp_properties.p_interval;
    struct termentry *t = term;

    for (i = 0; i < plot->p_count; i++) {
	if ((plot->plot_style == LINESPOINTS) && (interval) && (i % interval)) {
	    continue;
	}
	if (plot->points[i].type == INRANGE) {
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    /* do clipping if necessary */
	    if (!clip_points
		|| (x >= plot_bounds.xleft + p_width
		    && y >= plot_bounds.ybot + p_height
		    && x <= plot_bounds.xright - p_width
		    && y <= plot_bounds.ytop - p_height)) {

		if ((plot->plot_style == POINTSTYLE || plot->plot_style == LINESPOINTS)
		&&  plot->lp_properties.p_size == PTSZ_VARIABLE)
		    (*t->pointsize)(pointsize * plot->points[i].z);

		/* A negative interval indicates we should try to blank out the */
		/* area behind the point symbol. This could be done better by   */
		/* implementing a special point type, but that would require    */
		/* modification to all terminal drivers. It might be worth it.  */
		if (plot->plot_style == LINESPOINTS && interval < 0) {
		    (*t->linetype)(LT_BACKGROUND);
		    (*t->point) (x, y, 6);
		    term_apply_lp_properties(&(plot->lp_properties));
		}

		/* rgb variable  -  color read from data column */
		check_for_variable_color(plot, &plot->points[i]);

		(*t->point) (x, y, plot->lp_properties.p_type);
	    }
	}
    }
}

#ifdef EAM_OBJECTS
/* plot_circles:
 * Plot the curves in CIRCLES style
 */
static void
plot_circles(struct curve_points *plot)
{
    int i;
    int x, y;
    double radius;
    struct fill_style_type *fillstyle = &plot->fill_properties;
    int style = style_from_fill(fillstyle);
    TBOOLEAN withborder = FALSE;

    if (fillstyle->border_color.type != TC_LT
    ||  fillstyle->border_color.lt != LT_NODRAW)
	withborder = TRUE;

    for (i = 0; i < plot->p_count; i++) {
	if (plot->points[i].type == INRANGE) {
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    radius = x - map_x(plot->points[i].xlow);

	    /* rgb variable  -  color read from data column */
	    if (!check_for_variable_color(plot, &plot->points[i]) && withborder)
		term_apply_lp_properties(&plot->lp_properties);
	    do_arc(x,y, radius, 0., 360., style);
	    if (withborder) {
		need_fill_border(&plot->fill_properties);
		do_arc(x,y, radius, 0., 360., 0);
	    }
	}
    }
}
#endif

/* plot_dots:
 * Plot the curves in DOTS style
 */
static void
plot_dots(struct curve_points *plot)
{
    int i;
    int x, y;
    struct termentry *t = term;

    for (i = 0; i < plot->p_count; i++) {
	if (plot->points[i].type == INRANGE) {
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    /* rgb variable  -  color read from data column */
	    check_for_variable_color(plot, &plot->points[i]);
	    /* point type -1 is a dot */
	    (*t->point) (x, y, -1);
	}
    }
}

/* plot_vectors:
 * Plot the curves in VECTORS style
 */
static void
plot_vectors(struct curve_points *plot)
{
    int i;
    int x1, y1, x2, y2;
    struct termentry *t = term;
    struct coordinate points[2];
    double ex, ey;
    double lx[2], ly[2];

    /* Only necessary once because all arrows equal */
    term_apply_lp_properties(&(plot->arrow_properties.lp_properties));
    apply_head_properties(&(plot->arrow_properties));

    for (i = 0; i < plot->p_count; i++) {

	points[0] = plot->points[i];
	if (points[0].type == UNDEFINED)
	    continue;

	points[1].x = plot->points[i].xhigh;
	points[1].y = plot->points[i].yhigh;

	/* variable color read from extra data column. Most styles */
	/* have this stored in yhigh, but VECTOR stuffed it into z */
	points[0].yhigh = points[0].z;
	check_for_variable_color(plot, &points[0]);

	if (inrange(points[1].x, X_AXIS.min, X_AXIS.max)
	    && inrange(points[1].y, Y_AXIS.min, Y_AXIS.max)) {
	    /* to inrange */
	    points[1].type = INRANGE;
	    x2 = map_x(points[1].x);
	    y2 = map_y(points[1].y);
	    if (points[0].type == INRANGE) {
		x1 = map_x(points[0].x);
		y1 = map_y(points[0].y);
		(*t->arrow) (x1, y1, x2, y2, plot->arrow_properties.head);
	    } else if (points[0].type == OUTRANGE) {
		/* from outrange to inrange */
		if (clip_lines1) {
		    edge_intersect(points, 1, &ex, &ey);
		    x1 = map_x(ex);
		    y1 = map_y(ey);
		    if (plot->arrow_properties.head & END_HEAD)
			(*t->arrow) (x1, y1, x2, y2, END_HEAD);
		    else
			(*t->arrow) (x1, y1, x2, y2, NOHEAD);
		}
	    }
	} else {
	    /* to outrange */
	    points[1].type = OUTRANGE;
	    if (points[0].type == INRANGE) {
		/* from inrange to outrange */
		if (clip_lines1) {
		    x1 = map_x(points[0].x);
		    y1 = map_y(points[0].y);
		    edge_intersect(points, 1, &ex, &ey);
		    x2 = map_x(ex);
		    y2 = map_y(ey);
		    if (plot->arrow_properties.head & BACKHEAD)
			(*t->arrow) (x2, y2, x1, y1, BACKHEAD);
		    else
			(*t->arrow) (x1, y1, x2, y2, NOHEAD);
		}
	    } else if (points[0].type == OUTRANGE) {
		/* from outrange to outrange */
		if (clip_lines2) {
		    if (two_edge_intersect(points, 1, lx, ly)) {
			x1 = map_x(lx[0]);
			y1 = map_y(ly[0]);
			x2 = map_x(lx[1]);
			y2 = map_y(ly[1]);
			(*t->arrow) (x1, y1, x2, y2, NOHEAD);
		    }
		}
	    }
	}
    }
}


/* plot_f_bars() - finance bars */
static void
plot_f_bars(struct curve_points *plot)
{
    int i;			/* point index */
    struct termentry *t = term;
    double x;			/* position of the bar */
    double ylow, yhigh, yclose, yopen;	/* the ends of the bars */
    unsigned int xM, ylowM, yhighM;	/* the mapped version of above */
    TBOOLEAN low_inrange, high_inrange;
    int tic = GPMAX(ERRORBARTIC/2,1);

    for (i = 0; i < plot->p_count; i++) {
	/* undefined points don't count */
	if (plot->points[i].type == UNDEFINED)
	    continue;

	/* check to see if in xrange */
	x = plot->points[i].x;
	if (!inrange(x, X_AXIS.min, X_AXIS.max))
	    continue;
	xM = map_x(x);

	/* find low and high points of bar, and check yrange */
	yhigh = plot->points[i].yhigh;
	ylow = plot->points[i].ylow;
	yclose = plot->points[i].z;
	yopen = plot->points[i].y;

	high_inrange = inrange(yhigh, Y_AXIS.min, Y_AXIS.max);
	low_inrange = inrange(ylow, Y_AXIS.min, Y_AXIS.max);

	/* compute the plot position of yhigh */
	if (high_inrange)
	    yhighM = map_y(yhigh);
	else if (samesign(yhigh - Y_AXIS.max, Y_AXIS.max - Y_AXIS.min))
	    yhighM = map_y(Y_AXIS.max);
	else
	    yhighM = map_y(Y_AXIS.min);

	/* compute the plot position of ylow */
	if (low_inrange)
	    ylowM = map_y(ylow);
	else if (samesign(ylow - Y_AXIS.max, Y_AXIS.max - Y_AXIS.min))
	    ylowM = map_y(Y_AXIS.max);
	else
	    ylowM = map_y(Y_AXIS.min);

	if (!high_inrange && !low_inrange && ylowM == yhighM)
	    /* both out of range on the same side */
	    continue;

	/* by here everything has been mapped */
	(*t->move) (xM, ylowM);
	(*t->vector) (xM, yhighM);	/* draw the main bar */
	/* draw the open tic */
	(*t->move) ((unsigned int) (xM - bar_size * tic), map_y(yopen));
	(*t->vector) (xM, map_y(yopen));
	/* draw the close tic */
	(*t->move) ((unsigned int) (xM + bar_size * tic), map_y(yclose));
	(*t->vector) (xM, map_y(yclose));
    }
}


/* plot_c_bars:
 * Plot the curves in CANDLESTICSK style
 * EAM Apr 2008 - switch to using empty/fill rather than empty/striped 
 *		  to distinguish whether (open > close)
 */
static void
plot_c_bars(struct curve_points *plot)
{
    struct termentry *t = term;
    int i;
    double x;						/* position of the bar */
    double dxl, dxr, ylow, yhigh, yclose, yopen;	/* the ends of the bars */
    int xlowM, xhighM, xM, ylowM, yhighM;		/* mapped version of above */
    int ymin, ymax;					/* clipped to plot extent */
    enum coord_type prev = UNDEFINED;			/* type of previous point */
    TBOOLEAN low_inrange, high_inrange;
    TBOOLEAN open_inrange, close_inrange;
    int tic = GPMAX(ERRORBARTIC/2,1);

    for (i = 0; i < plot->p_count; i++) {
	TBOOLEAN skip_box = FALSE;

	/* undefined points don't count */
	if (plot->points[i].type == UNDEFINED)
	    continue;

	/* check to see if in xrange */
	x = plot->points[i].x;
	if (!inrange(x, X_AXIS.min, X_AXIS.max))
	    continue;
	xM = map_x(x);

	/* find low and high points of bar, and check yrange */
	yhigh = plot->points[i].yhigh;
	ylow = plot->points[i].ylow;
	yclose = plot->points[i].z;
	yopen = plot->points[i].y;

	/* HBB 20010928: To make code match the documentation, ensure
	 * yhigh is actually higher than ylow */
	if (yhigh < ylow) {
	    double temp = ylow;
	    ylow = yhigh;
	    yhigh = temp;
	}

	high_inrange = inrange(yhigh, axis_array[y_axis].min, axis_array[y_axis].max);
	low_inrange = inrange(ylow, axis_array[y_axis].min, axis_array[y_axis].max);

	/* compute the plot position of yhigh */
	if (high_inrange)
	    yhighM = map_y(yhigh);
	else if (samesign(yhigh - axis_array[y_axis].max,
			  axis_array[y_axis].max - axis_array[y_axis].min))
	    yhighM = map_y(axis_array[y_axis].max);
	else
	    yhighM = map_y(axis_array[y_axis].min);

	/* compute the plot position of ylow */
	if (low_inrange)
	    ylowM = map_y(ylow);
	else if (samesign(ylow - axis_array[y_axis].max,
			  axis_array[y_axis].max - axis_array[y_axis].min))
	    ylowM = map_y(axis_array[y_axis].max);
	else
	    ylowM = map_y(axis_array[y_axis].min);

	if (!high_inrange && !low_inrange && ylowM == yhighM)
	    /* both out of range on the same side */
	    continue;

	if (boxwidth < 0.0) {
	    xlowM = xM - bar_size * tic;
	    xhighM = xM + bar_size * tic;

	} else {
	    dxl = -boxwidth / 2.0;
	    if (prev != UNDEFINED)
		if (! boxwidth_is_absolute)
		    dxl = (plot->points[i-1].x - plot->points[i].x) * boxwidth / 2.0;

	    dxr = -dxl;
	    if (i < plot->p_count - 1) {
		if (plot->points[i + 1].type != UNDEFINED) {
		    if (! boxwidth_is_absolute)
			dxr = (plot->points[i+1].x - plot->points[i].x) * boxwidth / 2.0;
		    else
			dxr = boxwidth / 2.0;
		}
	    }

	if (prev == UNDEFINED)
	    dxl = -dxr;

	dxl = plot->points[i].x + dxl;
	dxr = plot->points[i].x + dxr;
	cliptorange(dxr, X_AXIS.min, X_AXIS.max);
	cliptorange(dxl, X_AXIS.min, X_AXIS.max);
	xlowM = map_x(dxl);
	xhighM = map_x(dxr);
	}

	/* EAM Feb 2007 Force width to be an odd number of pixels */
	/* so that the center bar can be centered perfectly.	  */
	if (((xhighM-xlowM) & 01) != 0) {
	    xhighM++;
	    if (xM-xlowM > xhighM-xM) xM--;
	    if (xM-xlowM < xhighM-xM) xM++;
	}

	/* EAM Feb 2006 Clip to plot vertical extent */
	open_inrange = inrange(yopen, axis_array[y_axis].min, axis_array[y_axis].max);
	close_inrange = inrange(yclose, axis_array[y_axis].min, axis_array[y_axis].max);
	cliptorange(yopen, Y_AXIS.min, Y_AXIS.max);
	cliptorange(yclose, Y_AXIS.min, Y_AXIS.max);
	if (map_y(yopen) < map_y(yclose)) {
	    ymin = map_y(yopen); ymax = map_y(yclose);
	} else {
	    ymax = map_y(yopen); ymin = map_y(yclose);
	}
	if (!open_inrange && !close_inrange && ymin == ymax)
	    skip_box = TRUE;

	/* Reset to original color, if we changed it for the border */
	if (plot->fill_properties.border_color.type != TC_DEFAULT
	&& !( plot->fill_properties.border_color.type == TC_LT &&
	      plot->fill_properties.border_color.lt == LT_NODRAW)) {
		(*t->linetype)(plot->lp_properties.l_type);
		if (plot->lp_properties.use_palette)
		    apply_pm3dcolor(&plot->lp_properties.pm3d_color,t);
	}
	
	/* Boxes are always filled if an explicit non-empty fillstyle is set. */
	/* If the fillstyle is FS_EMPTY, fill to indicate (open > close).     */
	if (term->fillbox && !skip_box) {
	    int style = style_from_fill(&plot->fill_properties);
	    if ((style != FS_EMPTY) || (yopen > yclose)) {
		unsigned int x = xlowM;
		unsigned int y = ymin;
		unsigned int w = (xhighM-xlowM);
		unsigned int h = (ymax-ymin);

		if (style == FS_EMPTY)
		    style = FS_OPAQUE;
		(*t->fillbox)(style, x, y, w, h);

		if (style_from_fill(&plot->fill_properties) != FS_EMPTY)
		    need_fill_border(&plot->fill_properties);
	    }
	}

	/* Draw whiskers and an open box */
	    (*t->move)   (xM, ylowM);
	    (*t->vector) (xM, ymin);
	    (*t->move)   (xM, ymax);
	    (*t->vector) (xM, yhighM);

	    if (!skip_box) {
		newpath();
		(*t->move)   (xlowM, map_y(yopen));
		(*t->vector) (xhighM, map_y(yopen));
		(*t->vector) (xhighM, map_y(yclose));
		(*t->vector) (xlowM, map_y(yclose));
		(*t->vector) (xlowM, map_y(yopen));
		closepath();
	    }

	/* Some users prefer bars at the end of the whiskers */
	if (plot->arrow_properties.head == BOTH_HEADS) {
	    double frac = plot->arrow_properties.head_length;
	    unsigned int d = (frac <= 0) ? 0 : (xhighM-xlowM)*(1.-frac)/2.;

	    if (high_inrange) {
		(*t->move)   (xlowM+d, yhighM);
		(*t->vector) (xhighM-d, yhighM);
	    }
	    if (low_inrange) {
		(*t->move)   (xlowM+d, ylowM);
		(*t->vector) (xhighM-d, ylowM);
	    }
	}

	/* Through 4.2 gnuplot would indicate (open > close) by drawing     */
	/* three vertical bars.  Now we use solid fill.  But if the current */
	/* terminal does not support filled boxes, fall back to the old way */
	if ((yopen > yclose) && !(term->fillbox)) {
	    (*t->move)   (xM, ymin);
	    (*t->vector) (xM, ymax);
	    (*t->move)   ( (xM + xlowM) / 2, ymin);
	    (*t->vector) ( (xM + xlowM) / 2, ymax);
	    (*t->move)   ( (xM + xhighM) / 2, ymin);
	    (*t->vector) ( (xM + xhighM) / 2, ymax);
	}

	prev = plot->points[i].type;
    }
}


/* FIXME
 * there are LOADS of == style double comparisons in here!
 */
/* single edge intersection algorithm */
/* Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segment defined
 * by the two points.
 */
static int
edge_intersect(
    struct coordinate GPHUGE *points, /* the points array */
    int i,			/* line segment from point i-1 to point i */
    double *ex, double *ey)	/* the point where it crosses an edge */
{
    double ix = points[i - 1].x;
    double iy = points[i - 1].y;
    double ox = points[i].x;
    double oy = points[i].y;
    double x, y;		/* possible intersection point */

    if (points[i].type == INRANGE) {
	/* swap points around so that ix/ix/iz are INRANGE and
	 * ox/oy/oz are OUTRANGE
	 */
	x = ix;
	ix = ox;
	ox = x;
	y = iy;
	iy = oy;
	oy = y;
    }
    /* nasty degenerate cases, effectively drawing to an infinity point (?)
     * cope with them here, so don't process them as a "real" OUTRANGE point
     *
     * If more than one coord is -VERYLARGE, then can't ratio the "infinities"
     * so drop out by returning the INRANGE point.
     *
     * Obviously, only need to test the OUTRANGE point (coordinates) */
    if (ox == -VERYLARGE || oy == -VERYLARGE) {
	*ex = ix;
	*ey = iy;

	if (ox == -VERYLARGE) {
	    /* can't get a direction to draw line, so simply
	     * return INRANGE point */
	    if (oy == -VERYLARGE)
		return LEFT_EDGE|BOTTOM_EDGE;

	    *ex = X_AXIS.min;
	    return LEFT_EDGE;
	}
	/* obviously oy is -VERYLARGE and ox != -VERYLARGE */
	*ey = Y_AXIS.min;
	return BOTTOM_EDGE;
    }
    /*
     * Can't have case (ix == ox && iy == oy) as one point
     * is INRANGE and one point is OUTRANGE.
     */
    if (iy == oy) {
	/* horizontal line */
	/* assume inrange(iy, Y_AXIS.min, Y_AXIS.max) */
	*ey = iy;		/* == oy */

	if (inrange(X_AXIS.max, ix, ox)) {
	    *ex = X_AXIS.max;
	    return RIGHT_EDGE;
	} else if (inrange(X_AXIS.min, ix, ox)) {
	    *ex = X_AXIS.min;
	    return LEFT_EDGE;
	} else {
	    graph_error("error in edge_intersect");
	    return 0;
	}
    } else if (ix == ox) {
	/* vertical line */
	/* assume inrange(ix, X_AXIS.min, X_AXIS.max) */
	*ex = ix;		/* == ox */

	if (inrange(Y_AXIS.max, iy, oy)) {
	    *ey = Y_AXIS.max;
	    return TOP_EDGE;
	} else if (inrange(Y_AXIS.min, iy, oy)) {
	    *ey = Y_AXIS.min;
	    return BOTTOM_EDGE;
	} else {
	    graph_error("error in edge_intersect");
	    return 0;
	}
    }
    /* slanted line of some kind */

    /* does it intersect Y_AXIS.min edge */
    if (inrange(Y_AXIS.min, iy, oy) && Y_AXIS.min != iy && Y_AXIS.min != oy) {
	x = ix + (Y_AXIS.min - iy) * ((ox - ix) / (oy - iy));
	if (inrange(x, X_AXIS.min, X_AXIS.max)) {
	    *ex = x;
	    *ey = Y_AXIS.min;
	    return BOTTOM_EDGE;		/* yes */
	}
    }
    /* does it intersect Y_AXIS.max edge */
    if (inrange(Y_AXIS.max, iy, oy) && Y_AXIS.max != iy && Y_AXIS.max != oy) {
	x = ix + (Y_AXIS.max - iy) * ((ox - ix) / (oy - iy));
	if (inrange(x, X_AXIS.min, X_AXIS.max)) {
	    *ex = x;
	    *ey = Y_AXIS.max;
	    return TOP_EDGE;		/* yes */
	}
    }
    /* does it intersect X_AXIS.min edge */
    if (inrange(X_AXIS.min, ix, ox) && X_AXIS.min != ix && X_AXIS.min != ox) {
	y = iy + (X_AXIS.min - ix) * ((oy - iy) / (ox - ix));
	if (inrange(y, Y_AXIS.min, Y_AXIS.max)) {
	    *ex = X_AXIS.min;
	    *ey = y;
	    return LEFT_EDGE;
	}
    }
    /* does it intersect X_AXIS.max edge */
    if (inrange(X_AXIS.max, ix, ox) && X_AXIS.max != ix && X_AXIS.max != ox) {
	y = iy + (X_AXIS.max - ix) * ((oy - iy) / (ox - ix));
	if (inrange(y, Y_AXIS.min, Y_AXIS.max)) {
	    *ex = X_AXIS.max;
	    *ey = y;
	    return RIGHT_EDGE;
	}
    }
    /* If we reach here, the inrange point is on the edge, and
     * the line segment from the outrange point does not cross any
     * other edges to get there. In this case, we return the inrange
     * point as the 'edge' intersection point. This will basically draw
     * line.
     */
    *ex = ix;
    *ey = iy;
    return 0;
}

/* XXX - JG  */
/* single edge intersection algorithm for "steps" curves */
/*
 * Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segments
 * forming the step between the two points.
 *
 * Recall that if P1 = (x1,y1) and P2 = (x2,y2), the step from
 * P1 to P2 is drawn as two line segments: (x1,y1)->(x2,y1) and
 * (x2,y1)->(x2,y2).
 */
static void
edge_intersect_steps(
    struct coordinate GPHUGE *points, /* the points array */
    int i,			/* line segment from point i-1 to point i */
    double *ex, double *ey)	/* the point where it crosses an edge */
{
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	if (inrange(ay, Y_AXIS.min, Y_AXIS.max)) {
	    *ey = ay;
	    cliptorange(ax, X_AXIS.min, X_AXIS.max);
	    *ex = ax;
	} else {
	    *ex = bx;
	    cliptorange(ay, Y_AXIS.min, Y_AXIS.max);
	    *ey = ay;
	}
    } else {			/* from INRANGE to OUTRANGE */
	if (inrange(bx, X_AXIS.min, X_AXIS.max)) {
	    *ex = bx;
	    cliptorange(by, Y_AXIS.min, Y_AXIS.max);
	    *ey = by;
	} else {
	    *ey = ay;
	    cliptorange(bx, X_AXIS.min, X_AXIS.max);
	    *ex = bx;
	}
    }
    return;
}

/* XXX - HOE  */
/* single edge intersection algorithm for "fsteps" curves */
/* fsteps means step on forward y-value.
 * Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segments
 * forming the step between the two points.
 *
 * Recall that if P1 = (x1,y1) and P2 = (x2,y2), the step from
 * P1 to P2 is drawn as two line segments: (x1,y1)->(x1,y2) and
 * (x1,y2)->(x2,y2).
 */
static void
edge_intersect_fsteps(
    struct coordinate GPHUGE *points, /* the points array */
    int i,			/* line segment from point i-1 to point i */
    double *ex, double *ey)	/* the point where it crosses an edge */
{
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	if (inrange(ax, X_AXIS.min, X_AXIS.max)) {
	    *ex = ax;
	    cliptorange(ay, Y_AXIS.min, Y_AXIS.max);
	    *ey = ay;
	} else {
	    *ey = by;
	    cliptorange(bx, X_AXIS.min, X_AXIS.max);
	    *ex = bx;
	}
    } else {			/* from INRANGE to OUTRANGE */
	if (inrange(by, Y_AXIS.min, Y_AXIS.max)) {
	    *ey = by;
	    cliptorange(bx, X_AXIS.min, X_AXIS.max);
	    *ex = bx;
	} else {
	    *ex = ax;
	    cliptorange(by, Y_AXIS.min, Y_AXIS.max);
	    *ey = by;
	}
    }
    return;
}

/* XXX - JG  */
/* double edge intersection algorithm for "steps" plot */
/* Given two points, both outside the plot, return the points where an
 * edge of the plot intersects the line segments forming a step
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to
 * draw (the one-point case is a degenerate of the two-point case and we do
 * not distinguish it - we draw it anyway).
 *
 * Recall that if P1 = (x1,y1) and P2 = (x2,y2), the step from
 * P1 to P2 is drawn as two line segments: (x1,y1)->(x2,y1) and
 * (x2,y1)->(x2,y2).
 */
static TBOOLEAN			/* any intersection? */
two_edge_intersect_steps(
    struct coordinate GPHUGE *points, /* the points array */
    int i,			/* line segment from point i-1 to point i */
    double *lx, double *ly)	/* lx[2], ly[2]: points where it crosses edges */
{
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (GPMAX(ax, bx) < X_AXIS.min || GPMIN(ax, bx) > X_AXIS.max
	|| GPMAX(ay, by) < Y_AXIS.min || GPMIN(ay, by) > Y_AXIS.max
	|| (!inrange(ay, Y_AXIS.min, Y_AXIS.max)
	    && !inrange(bx, X_AXIS.min, X_AXIS.max))
	) {
	return (FALSE);
    } else if (inrange(ay, Y_AXIS.min, Y_AXIS.max)
	       && inrange(bx, X_AXIS.min, X_AXIS.max)) {
	/* corner of step inside plotspace */
	cliptorange(ax, X_AXIS.min, X_AXIS.max);
	*lx++ = ax;
	*ly++ = ay;

	cliptorange(by, Y_AXIS.min, Y_AXIS.max);
	*lx = bx;
	*ly = by;

	return (TRUE);
    } else if (inrange(ay, Y_AXIS.min, Y_AXIS.max)) {
	/* cross plotspace in x-direction */
	*lx++ = X_AXIS.min;
	*ly++ = ay;
	*lx = X_AXIS.max;
	*ly = ay;
	return (TRUE);
    } else if (inrange(ax, X_AXIS.min, X_AXIS.max)) {
	/* cross plotspace in y-direction */
	*lx++ = bx;
	*ly++ = Y_AXIS.min;
	*lx = bx;
	*ly = Y_AXIS.max;
	return (TRUE);
    } else
	return (FALSE);
}

/* XXX - HOE  */
/* double edge intersection algorithm for "fsteps" plot */
/* Given two points, both outside the plot, return the points where an
 * edge of the plot intersects the line segments forming a step
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to
 * draw (the one-point case is a degenerate of the two-point case and we do
 * not distinguish it - we draw it anyway).
 *
 * Recall that if P1 = (x1,y1) and P2 = (x2,y2), the step from
 * P1 to P2 is drawn as two line segments: (x1,y1)->(x1,y2) and
 * (x1,y2)->(x2,y2).
 */
static TBOOLEAN			/* any intersection? */
two_edge_intersect_fsteps(
    struct coordinate GPHUGE *points, /* the points array */
    int i,			/* line segment from point i-1 to point i */
    double *lx, double *ly)	/* lx[2], ly[2]: points where it crosses edges */
{
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (GPMAX(ax, bx) < X_AXIS.min || GPMIN(ax, bx) > X_AXIS.max
	|| GPMAX(ay, by) < Y_AXIS.min || GPMIN(ay, by) > Y_AXIS.max
	|| (!inrange(by, Y_AXIS.min, Y_AXIS.max)
	    && !inrange(ax, X_AXIS.min, X_AXIS.max))
	) {
	return (FALSE);
    } else if (inrange(by, Y_AXIS.min, Y_AXIS.max)
	       && inrange(ax, X_AXIS.min, X_AXIS.max)) {
	/* corner of step inside plotspace */
	cliptorange(ay, Y_AXIS.min, Y_AXIS.max);
	*lx++ = ax;
	*ly++ = ay;

	cliptorange(bx, X_AXIS.min, X_AXIS.max);
	*lx = bx;
	*ly = by;

	return (TRUE);
    } else if (inrange(by, Y_AXIS.min, Y_AXIS.max)) {
	/* cross plotspace in x-direction */
	*lx++ = X_AXIS.min;
	*ly++ = by;
	*lx = X_AXIS.max;
	*ly = by;
	return (TRUE);
    } else if (inrange(ax, X_AXIS.min, X_AXIS.max)) {
	/* cross plotspace in y-direction */
	*lx++ = ax;
	*ly++ = Y_AXIS.min;
	*lx = ax;
	*ly = Y_AXIS.max;
	return (TRUE);
    } else
	return (FALSE);
}

/* double edge intersection algorithm */
/* Given two points, both outside the plot, return
 * the points where an edge of the plot intersects the line segment defined
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to
 * draw (the one-point case is a degenerate of the two-point case and we do
 * not distinguish it - we draw it anyway).
 */
static TBOOLEAN			/* any intersection? */
two_edge_intersect(
    struct coordinate GPHUGE *points, /* the points array */
    int i,			/* line segment from point i-1 to point i */
    double *lx, double *ly)	/* lx[2], ly[2]: points where it crosses edges */
{
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    int count;
    double ix = points[i - 1].x;
    double iy = points[i - 1].y;
    double ox = points[i].x;
    double oy = points[i].y;
    double t[4];
    double swap;
    double t_min, t_max;

    /* nasty degenerate cases, effectively drawing to an infinity
     * point (?)  cope with them here, so don't process them as a
     * "real" OUTRANGE point

     * If more than one coord is -VERYLARGE, then can't ratio the
     * "infinities" so drop out by returning FALSE */

    count = 0;
    if (ix == -VERYLARGE)
	count++;
    if (ox == -VERYLARGE)
	count++;
    if (iy == -VERYLARGE)
	count++;
    if (oy == -VERYLARGE)
	count++;

    /* either doesn't pass through graph area *or* can't ratio
     * infinities to get a direction to draw line, so simply
     * return(FALSE) */
    if (count > 1) {
	return (FALSE);
    }

    if (ox == -VERYLARGE || ix == -VERYLARGE) {
	/* Horizontal line */
	if (ix == -VERYLARGE) {
	    /* swap points so ix/iy don't have a -VERYLARGE component */
	    swap = ix;
	    ix = ox;
	    ox = swap;
	    swap = iy;
	    iy = oy;
	    oy = swap;
	}
	/* check actually passes through the graph area */
	if (ix > GPMAX(X_AXIS.max, X_AXIS.min)
	    && inrange(iy, Y_AXIS.min, Y_AXIS.max)) {
	    lx[0] = X_AXIS.min;
	    ly[0] = iy;

	    lx[1] = X_AXIS.max;
	    ly[1] = iy;
	    return (TRUE);
	} else {
	    return (FALSE);
	}
    }
    if (oy == -VERYLARGE || iy == -VERYLARGE) {
	/* Vertical line */
	if (iy == -VERYLARGE) {
	    /* swap points so ix/iy don't have a -VERYLARGE component */
	    swap = ix;
	    ix = ox;
	    ox = swap;
	    swap = iy;
	    iy = oy;
	    oy = swap;
	}
	/* check actually passes through the graph area */
	if (iy > GPMAX(Y_AXIS.min, Y_AXIS.max)
	    && inrange(ix, X_AXIS.min, X_AXIS.max)) {
	    lx[0] = ix;
	    ly[0] = Y_AXIS.min;

	    lx[1] = ix;
	    ly[1] = Y_AXIS.max;
	    return (TRUE);
	} else {
	    return (FALSE);
	}
    }
    /*
     * Special horizontal/vertical, etc. cases are checked and remaining
     * slant lines are checked separately.
     *
     * The slant line intersections are solved using the parametric form
     * of the equation for a line, since if we test x/y min/max planes explicitly
     * then e.g. a  line passing through a corner point (X_AXIS.min,Y_AXIS.min)
     * actually intersects 2 planes and hence further tests would be required
     * to anticipate this and similar situations.
     */

    /*
     * Can have case (ix == ox && iy == oy) as both points OUTRANGE
     */
    if (ix == ox && iy == oy) {
	/* but as only define single outrange point, can't intersect graph area */
	return (FALSE);
    }
    if (ix == ox) {
	/* line parallel to y axis */

	/* x coord must be in range, and line must span both Y_AXIS.min and Y_AXIS.max */
	/* note that spanning Y_AXIS.min implies spanning Y_AXIS.max, as both points OUTRANGE */
	if (!inrange(ix, X_AXIS.min, X_AXIS.max)) {
	    return (FALSE);
	}
	if (inrange(Y_AXIS.min, iy, oy)) {
	    lx[0] = ix;
	    ly[0] = Y_AXIS.min;

	    lx[1] = ix;
	    ly[1] = Y_AXIS.max;
	    return (TRUE);
	} else
	    return (FALSE);
    }
    if (iy == oy) {
	/* already checked case (ix == ox && iy == oy) */

	/* line parallel to x axis */
	/* y coord must be in range, and line must span both X_AXIS.min and X_AXIS.max */
	/* note that spanning X_AXIS.min implies spanning X_AXIS.max, as both points OUTRANGE */
	if (!inrange(iy, Y_AXIS.min, Y_AXIS.max)) {
	    return (FALSE);
	}
	if (inrange(X_AXIS.min, ix, ox)) {
	    lx[0] = X_AXIS.min;
	    ly[0] = iy;

	    lx[1] = X_AXIS.max;
	    ly[1] = iy;
	    return (TRUE);
	} else
	    return (FALSE);
    }
    /* nasty 2D slanted line in an xy plane */

    /* From here on, it's essentially the classical Cyrus-Beck, or
     * Liang-Barsky algorithm for line clipping to a rectangle */
    /*
       Solve parametric equation

       (ix, iy) + t (diff_x, diff_y)

       where 0.0 <= t <= 1.0 and

       diff_x = (ox - ix);
       diff_y = (oy - iy);
     */

    t[0] = (X_AXIS.min - ix) / (ox - ix);
    t[1] = (X_AXIS.max - ix) / (ox - ix);
    if (t[0] > t[1]) {
	swap = t[0];
	t[0] = t[1];
	t[1] = swap;
    }

    t[2] = (Y_AXIS.min - iy) / (oy - iy);
    t[3] = (Y_AXIS.max - iy) / (oy - iy);
    if (t[2] > t[3]) {
	swap = t[2];
	t[2] = t[3];
	t[3] = swap;
    }

    t_min = GPMAX(GPMAX(t[0], t[2]), 0.0);
    t_max = GPMIN(GPMIN(t[1], t[3]), 1.0);

    if (t_min > t_max)
	return (FALSE);

    lx[0] = ix + t_min * (ox - ix);
    ly[0] = iy + t_min * (oy - iy);

    lx[1] = ix + t_max * (ox - ix);
    ly[1] = iy + t_max * (oy - iy);

    /*
     * Can only have 0 or 2 intersection points -- only need test one coord
     */
    /* FIXME: this is UGLY. Need an 'almost_inrange()' function */
    if (inrange(lx[0],
		(X_AXIS.min - 1e-5 * (X_AXIS.max - X_AXIS.min)),
		(X_AXIS.max + 1e-5 * (X_AXIS.max - X_AXIS.min)))
	&& inrange(ly[0],
		   (Y_AXIS.min - 1e-5 * (Y_AXIS.max - Y_AXIS.min)),
		   (Y_AXIS.max + 1e-5 * (Y_AXIS.max - Y_AXIS.min))))
    {

	return (TRUE);
    }
    return (FALSE);
}


/* EAM April 2004 - If the line segment crosses a bounding line we will
 * interpolate an extra corner and split the filled polygon into two.
 */
static TBOOLEAN
bound_intersect(
struct coordinate GPHUGE *points,
int i,				/* line segment from point i-1 to point i */
double *ex, double *ey,		/* the point where it crosses a boundary */
filledcurves_opts *filledcurves_options)
{
    double dx1, dx2, dy1, dy2;

    /* If there are no bounding lines in effect, don't bother */
    if (!filledcurves_options->oneside)
	return FALSE;

    switch (filledcurves_options->closeto) {
	case FILLEDCURVES_ATX1:
	case FILLEDCURVES_ATX2:
	    dx1 = filledcurves_options->at - points[i-1].x;
	    dx2 = filledcurves_options->at - points[i].x;
	    dy1 = points[i].y - points[i-1].y;
	    if (dx1*dx2 < 0) {
		*ex = filledcurves_options->at;
		*ey = points[i-1].y + dy1 * dx1 / (dx1-dx2);
		return TRUE;
	    }
	    break;
	case FILLEDCURVES_ATY1:
	case FILLEDCURVES_ATY2:
	    dy1 = filledcurves_options->at - points[i-1].y;
	    dy2 = filledcurves_options->at - points[i].y;
	    dx1 = points[i].x - points[i-1].x;
	    if (dy1*dy2 < 0) {
		*ex = points[i-1].x + dx1 * dy1 / (dy1-dy2);
		*ey = filledcurves_options->at;
		return TRUE;
	    }
	    break;
	case FILLEDCURVES_ATXY:
	default:
	    break;
    }

    return FALSE;
}


/* HBB 20010118: all the *_callback() functions made non-static. This
 * is necessary to work around a bug in HP's assembler shipped with
 * HP-UX 10 and higher, if GCC tries to use it */

/* display a x-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
void
xtick2d_callback(
    AXIS_INDEX axis,
    double place,
    char *text,
    struct lp_style_type grid)	/* grid.l_type == LT_NODRAW means no grid */
{
    struct termentry *t = term;
    /* minitick if text is NULL - beware - h_tic is unsigned */
    int ticsize = tic_direction * (int) t->v_tic * (text ? axis_array[axis].ticscale : axis_array[axis].miniticscale);
    int x = map_x(place);

    (void) axis;		/* avoid "unused parameter" warning */

    if (grid.l_type > LT_NODRAW) {
	if (t->layer)
	    (t->layer)(TERM_LAYER_BEGIN_GRID);
	term_apply_lp_properties(&grid);
	if (polar_grid_angle) {
	    double x = place, y = 0, s = sin(0.1), c = cos(0.1);
	    int i;
	    int ogx = map_x(x);
	    int ogy = map_y(0);
	    int tmpgx, tmpgy, gx, gy;

	    if (place > largest_polar_circle)
		largest_polar_circle = place;
	    else if (-place > largest_polar_circle)
		largest_polar_circle = -place;
	    for (i = 1; i <= 63 /* 2pi/0.1 */ ; ++i) {
		{
		    /* cos(t+dt) = cos(t)cos(dt)-sin(t)cos(dt) */
		    double tx = x * c - y * s;
		    /* sin(t+dt) = sin(t)cos(dt)+cos(t)sin(dt) */
		    y = y * c + x * s;
		    x = tx;
		}
		tmpgx = gx = map_x(x);
		tmpgy = gy = map_y(y);
		if (clip_line(&ogx, &ogy, &tmpgx, &tmpgy)) {
		    (*t->move) ((unsigned int) ogx, (unsigned int) ogy);
		    (*t->vector) ((unsigned int) tmpgx, (unsigned int) tmpgy);
		}
		ogx = gx;
		ogy = gy;
	    }
	} else {
	    legend_key *key = &keyT;
	    if (lkey && x < key->bounds.xright && x > key->bounds.xleft
	    &&  key->bounds.ytop > plot_bounds.ybot && key->bounds.ybot < plot_bounds.ytop) {
		if (key->bounds.ybot > plot_bounds.ybot) {
		    (*t->move) (x, plot_bounds.ybot);
		    (*t->vector) (x, key->bounds.ybot);
		}
		if (key->bounds.ytop < plot_bounds.ytop) {
		    (*t->move) (x, key->bounds.ytop);
		    (*t->vector) (x, plot_bounds.ytop);
		}
	    } else {
		(*t->move) (x, plot_bounds.ybot);
		(*t->vector) (x, plot_bounds.ytop);
	    }
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
	if (t->layer)
	    (t->layer)(TERM_LAYER_END_GRID);
    }	/* End of grid code */


    /* we precomputed tic posn and text posn in global vars */

    (*t->move) (x, tic_start);
    (*t->vector) (x, tic_start + ticsize);

    if (tic_mirror >= 0) {
	(*t->move) (x, tic_mirror);
	(*t->vector) (x, tic_mirror - ticsize);
    }
    if (text) {
	/* get offset */
	double offsetx_d, offsety_d;
	map_position_r(&(axis_array[axis].ticdef.offset),
		       &offsetx_d, &offsety_d, "xtics");
	/* User-specified different color for the tics text */
	if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(axis_array[axis].ticdef.textcolor), t);
	write_multiline(x+(int)offsetx_d, tic_text+(int)offsety_d, text,
			tic_hjust, tic_vjust, rotate_tics,
			axis_array[axis].ticdef.font);
	term_apply_lp_properties(&border_lp);	/* reset to border linetype */
    }
}

/* display a y-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
void
ytick2d_callback(
    AXIS_INDEX axis,
    double place,
    char *text,
    struct lp_style_type grid)	/* grid.l_type == LT_NODRAW means no grid */
{
    struct termentry *t = term;
    /* minitick if text is NULL - v_tic is unsigned */
    int ticsize = tic_direction * (int) t->h_tic * (text ? axis_array[axis].ticscale : axis_array[axis].miniticscale);
    int y = map_y(place);

    (void) axis;		/* avoid "unused parameter" warning */

    if (grid.l_type > LT_NODRAW) {
	if (t->layer)
	    (t->layer)(TERM_LAYER_BEGIN_GRID);
	term_apply_lp_properties(&grid);
	if (polar_grid_angle) {
	    double x = 0, y = place, s = sin(0.1), c = cos(0.1);
	    int i;
	    if (place > largest_polar_circle)
		largest_polar_circle = place;
	    else if (-place > largest_polar_circle)
		largest_polar_circle = -place;
	    clip_move(map_x(x), map_y(y));
	    for (i = 1; i <= 63 /* 2pi/0.1 */ ; ++i) {
		{
		    /* cos(t+dt) = cos(t)cos(dt)-sin(t)cos(dt) */
		    double tx = x * c - y * s;
		    /* sin(t+dt) = sin(t)cos(dt)+cos(t)sin(dt) */
		    y = y * c + x * s;
		    x = tx;
		}
		clip_vector(map_x(x), map_y(y));
	    }
	} else {
	    /* Make the grid avoid the key box */
	    legend_key *key = &keyT;
	    if (lkey && y < key->bounds.ytop && y > key->bounds.ybot
	    &&  key->bounds.xleft < plot_bounds.xright && key->bounds.xright > plot_bounds.xleft) {
		if (key->bounds.xleft > plot_bounds.xleft) {
		    (*t->move) (plot_bounds.xleft, y);
		    (*t->vector) (key->bounds.xleft, y);
		}
		if (key->bounds.xright < plot_bounds.xright) {
		    (*t->move) (key->bounds.xright, y);
		    (*t->vector) (plot_bounds.xright, y);
		}
	    } else {
		(*t->move) (plot_bounds.xleft, y);
		(*t->vector) (plot_bounds.xright, y);
	    }
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
	if (t->layer)
	    (t->layer)(TERM_LAYER_END_GRID);
    }
    /* we precomputed tic posn and text posn */

    (*t->move) (tic_start, y);
    (*t->vector) (tic_start + ticsize, y);

    if (tic_mirror >= 0) {
	(*t->move) (tic_mirror, y);
	(*t->vector) (tic_mirror - ticsize, y);
    }
    if (text) {
	/* get offset */
	double offsetx_d, offsety_d;
	map_position_r(&(axis_array[axis].ticdef.offset),
		       &offsetx_d, &offsety_d, "ytics");
	/* User-specified different color for the tics text */
	if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(axis_array[axis].ticdef.textcolor), t);
	write_multiline(tic_text+(int)offsetx_d, y+(int)offsety_d, text,
			tic_hjust, tic_vjust, rotate_tics,
			axis_array[axis].ticdef.font);
	term_apply_lp_properties(&border_lp);	/* reset to border linetype */
    }
}

/* STR points to a label string, possibly with several lines separated
   by \n.  Return the number of characters in the longest line.  If
   LINES is not NULL, set *LINES to the number of lines in the
   label. */
int
label_width(const char *str, int *lines)
{
    char *lab = NULL, *s, *e;
    int mlen, len, l;

    if (!str || *str == '\0') {
	if (lines)
	    *lines = 0;
	return (0);
    }

    l = mlen = len = 0;
    lab = gp_alloc(strlen(str) + 2, "in label_width");
    strcpy(lab, str);
    strcat(lab, "\n");
    s = lab;
    while ((e = (char *) strchr(s, '\n')) != NULL) {
	*e = '\0';
	len = estimate_strlen(s);	/* = e-s ? */
	if (len > mlen)
	    mlen = len;
	if (len || l || *str == '\n')
	    l++;
	s = ++e;
    }
    /* lines = NULL => not interested - div */
    if (lines)
	*lines = l;

    free(lab);
    return (mlen);
}


/*{{{  map_position, wrapper, which maps double to int */
void
map_position(
    struct position *pos,
    int *x, int *y,
    const char *what)
{
    double xx, yy;
    map_position_double(pos, &xx, &yy, what);
    *x = xx;
    *y = yy;
}

/*}}} */

/*{{{  map_position_double */
static void
map_position_double(
    struct position *pos,
    double *x, double *y,
    const char *what)
{
    switch (pos->scalex) {
    case first_axes:
	{
	    double xx = axis_log_value_checked(FIRST_X_AXIS, pos->x, what);
	    *x = AXIS_MAP(FIRST_X_AXIS, xx);
	    break;
	}
    case second_axes:
	{
	    double xx = axis_log_value_checked(SECOND_X_AXIS, pos->x, what);
	    *x = AXIS_MAP(SECOND_X_AXIS, xx);
	    break;
	}
    case graph:
	{
	    *x = plot_bounds.xleft + pos->x * (plot_bounds.xright - plot_bounds.xleft);
	    break;
	}
    case screen:
	{
	    struct termentry *t = term;
	    *x = pos->x * (t->xmax - 1);
	    break;
	}
    case character:
	{
	    register struct termentry *t = term;
	    *x = pos->x * t->h_char;
	    break;
	}
    }
    switch (pos->scaley) {
    case first_axes:
	{
	    double yy = axis_log_value_checked(FIRST_Y_AXIS, pos->y, what);
	    *y = AXIS_MAP(FIRST_Y_AXIS, yy);
	    break;
	}
    case second_axes:
	{
	    double yy = axis_log_value_checked(SECOND_Y_AXIS, pos->y, what);
	    *y = AXIS_MAP(SECOND_Y_AXIS, yy);
	    break;
	}
    case graph:
	{
	    *y = plot_bounds.ybot + pos->y * (plot_bounds.ytop - plot_bounds.ybot);
	    break;
	}
    case screen:
	{
	    struct termentry *t = term;
	    *y = pos->y * (t->ymax -1);
	    break;
	}
    case character:
	{
	    register struct termentry *t = term;
	    *y = pos->y * t->v_char;
	    break;
	}
    }
    *x += 0.5;
    *y += 0.5;
}

/*}}} */

/*{{{  map_position_r */
void
map_position_r(
    struct position *pos,
    double *x, double *y,
    const char *what)
{
    switch (pos->scalex) {
    case first_axes:
	{
	    double xx = axis_log_value_checked(FIRST_X_AXIS, pos->x, what);
	    *x = xx * axis_array[FIRST_X_AXIS].term_scale;
	    break;
	}
    case second_axes:
	{
	    double xx = axis_log_value_checked(SECOND_X_AXIS, pos->x, what);
	    *x = xx * axis_array[SECOND_X_AXIS].term_scale;
	    break;
	}
    case graph:
	{
	    *x = pos->x * (plot_bounds.xright - plot_bounds.xleft);
	    break;
	}
    case screen:
	{
	    struct termentry *t = term;
	    *x = pos->x * (t->xmax - 1);
	    break;
	}
    case character:
	{
	    register struct termentry *t = term;
	    *x = pos->x * t->h_char;
	    break;
	}
    }
    switch (pos->scaley) {
    case first_axes:
	{
	    double yy = axis_log_value_checked(FIRST_Y_AXIS, pos->y, what);
	    *y = yy * axis_array[FIRST_Y_AXIS].term_scale;
	    return;
	}
    case second_axes:
	{
	    double yy = axis_log_value_checked(SECOND_Y_AXIS, pos->y, what);
	    *y = yy * axis_array[SECOND_Y_AXIS].term_scale;
	    return;
	}
    case graph:
	{
	    *y = pos->y * (plot_bounds.ytop - plot_bounds.ybot);
	    return;
	}
    case screen:
	{
	    struct termentry *t = term;
	    *y = pos->y * (t->ymax -1);
	    return;
	}
    case character:
	{
	    register struct termentry *t = term;
	    *y = pos->y * t->v_char;
	    break;
	}
    }
}
/*}}} */

static void
plot_border()
{
    int min, max;

	term_apply_lp_properties(&border_lp);	/* border linetype */
	if (border_complete)
	    newpath();
	(*term->move) (plot_bounds.xleft, plot_bounds.ytop);

	if (border_west && axis_array[FIRST_Y_AXIS].ticdef.rangelimited) {
		max = AXIS_MAP(FIRST_Y_AXIS,axis_array[FIRST_Y_AXIS].data_max);
		min = AXIS_MAP(FIRST_Y_AXIS,axis_array[FIRST_Y_AXIS].data_min);
		(*term->move) (plot_bounds.xleft, max);
		(*term->vector) (plot_bounds.xleft, min);
		(*term->move) (plot_bounds.xleft, plot_bounds.ybot);
	} else if (border_west) {
	    (*term->vector) (plot_bounds.xleft, plot_bounds.ybot);
	} else {
	    (*term->move) (plot_bounds.xleft, plot_bounds.ybot);
	}

	if (border_south && axis_array[FIRST_X_AXIS].ticdef.rangelimited) {
		max = AXIS_MAP(FIRST_X_AXIS,axis_array[FIRST_X_AXIS].data_max);
		min = AXIS_MAP(FIRST_X_AXIS,axis_array[FIRST_X_AXIS].data_min);
		(*term->move) (min, plot_bounds.ybot);
		(*term->vector) (max, plot_bounds.ybot);
		(*term->move) (plot_bounds.xright, plot_bounds.ybot);
	} else if (border_south) {
	    (*term->vector) (plot_bounds.xright, plot_bounds.ybot);
	} else {
	    (*term->move) (plot_bounds.xright, plot_bounds.ybot);
	}

	if (border_east && axis_array[SECOND_Y_AXIS].ticdef.rangelimited) {
		max = AXIS_MAP(SECOND_Y_AXIS,axis_array[SECOND_Y_AXIS].data_max);
		min = AXIS_MAP(SECOND_Y_AXIS,axis_array[SECOND_Y_AXIS].data_min);
		(*term->move) (plot_bounds.xright, max);
		(*term->vector) (plot_bounds.xright, min);
		(*term->move) (plot_bounds.xright, plot_bounds.ybot);
	} else if (border_east) {
	    (*term->vector) (plot_bounds.xright, plot_bounds.ytop);
	} else {
	    (*term->move) (plot_bounds.xright, plot_bounds.ytop);
	}

	if (border_north && axis_array[SECOND_X_AXIS].ticdef.rangelimited) {
		max = AXIS_MAP(SECOND_X_AXIS,axis_array[SECOND_X_AXIS].data_max);
		min = AXIS_MAP(SECOND_X_AXIS,axis_array[SECOND_X_AXIS].data_min);
		(*term->move) (min, plot_bounds.ytop);
		(*term->vector) (max, plot_bounds.ytop);
		(*term->move) (plot_bounds.xright, plot_bounds.ytop);
	} else if (border_north) {
	    (*term->vector) (plot_bounds.xleft, plot_bounds.ytop);
	} else {
	    (*term->move) (plot_bounds.xleft, plot_bounds.ytop);
	}

	if (border_complete)
	    closepath();
}


void
init_histogram(struct histogram_style *histogram, char *title)
{
    if (stackheight)
	free(stackheight);
    stackheight = NULL;
    if (histogram) {
	memcpy(histogram,&histogram_opts,sizeof(histogram_opts));
	memset(&(histogram->title), 0, sizeof(text_label));
	/* Insert in linked list */
	histogram_opts.next = histogram;
	histogram->title.text = title;
    }
}

void
free_histlist(struct histogram_style *hist)
{
    if (!hist)
	return;
    if (hist->title.text)
	free(hist->title.text);
    if (hist->next) {
	free_histlist(hist->next);
	free(hist->next);
	hist->next = NULL;
    }
}

static void
place_histogram_titles()
{
    histogram_style *hist = &histogram_opts;
    unsigned int x, y;
    while ((hist = hist->next)) {
	if (hist->title.text && *(hist->title.text)) {
	    double xoffset_d, yoffset_d;
	    map_position_r(&(histogram_opts.title.offset), &xoffset_d, &yoffset_d,
			   "histogram");
	    x = map_x((hist->start + hist->end) / 2.);
	    y = xlabel_y;
	    x += (int)xoffset_d;
	    y += (int)yoffset_d + 0.25 * term->v_char;
	    apply_pm3dcolor(&hist->title.textcolor,term);
	    write_multiline(x, y, hist->title.text,
			    CENTRE, JUST_BOT, 0, hist->title.font);
	    reset_textcolor(&hist->title.textcolor,term);
	}
    }
}


/*
 * Make this code a subroutine, rather than in-line, so that it can
 * eventually be shared by other callers. It would be nice to share it
 * with the 3d code also, but as of now the two code sections are not
 * very parallel.  EAM Nov 2003
 */

static void
do_key_sample(
    struct curve_points *this_plot,
    legend_key *key,
    char *title,
    struct termentry *t,
    int xl, int yl)
{
    /* Clip key box against canvas */
    BoundingBox *clip_save = clip_area;
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    if (key->textcolor.type == TC_RGB && key->textcolor.value < 0)
	/* Draw key text in same color as plot */
	;
    else if (key->textcolor.type != TC_DEFAULT)
	/* Draw key text in same color as key title */
	apply_pm3dcolor(&key->textcolor, t);
    else
	/* Draw key text in black */
	(*t->linetype)(LT_BLACK);

    if (key->just == GPKEY_LEFT) {
	write_multiline(xl + key_text_left, yl, title, LEFT, JUST_TOP, 0, key->font);
    } else {
	if ((*t->justify_text) (RIGHT)) {
	    write_multiline(xl + key_text_right, yl, title, RIGHT, JUST_TOP, 0, key->font);
	} else {
	    int x = xl + key_text_right - t->h_char * estimate_strlen(title);
	    if (key->region == GPKEY_AUTO_EXTERIOR_LRTBC ||	/* HBB 990327 */
		key->region == GPKEY_AUTO_EXTERIOR_MARGIN ||
		i_inrange(x, plot_bounds.xleft, plot_bounds.xright))
		write_multiline(x, yl, title, LEFT, JUST_TOP, 0, key->font);
	}
    }

    /* Draw sample in same style and color as the corresponding plot */
    (*t->linetype)(this_plot->lp_properties.l_type);
    if (this_plot->lp_properties.use_palette)
	apply_pm3dcolor(&this_plot->lp_properties.pm3d_color,t);

    /* draw sample depending on bits set in plot_style */
    if (this_plot->plot_style & PLOT_STYLE_HAS_FILL && t->fillbox) {
	struct fill_style_type *fs = &this_plot->fill_properties;
	int style = style_from_fill(fs);
	unsigned int x = xl + key_sample_left;
	unsigned int y = yl - key_entry_height/4;
	unsigned int w = key_sample_right - key_sample_left;
	unsigned int h = key_entry_height/2;

#ifdef EAM_OBJECTS
	if (this_plot->plot_style == CIRCLES && w > 0) {
	    do_arc(xl + key_point_offset, yl, key_entry_height/4, 0., 360., style);
	} else
#endif
	if (w > 0) {    /* All other plot types with fill */
	    if (style != FS_EMPTY)
		(*t->fillbox)(style,x,y,w,h);

	    /* need_fill_border will set the border linetype, but candlesticks don't want it */
	    if ((this_plot->plot_style == CANDLESTICKS && fs->border_color.type == TC_LT
							&& fs->border_color.lt == LT_NODRAW)
	    ||   style == FS_EMPTY
	    ||   need_fill_border(fs)) {
		newpath();
		draw_clip_line( xl + key_sample_left,  yl - key_entry_height/4,
			    xl + key_sample_right, yl - key_entry_height/4);
		draw_clip_line( xl + key_sample_right, yl - key_entry_height/4,
			    xl + key_sample_right, yl + key_entry_height/4);
		draw_clip_line( xl + key_sample_right, yl + key_entry_height/4,
			    xl + key_sample_left,  yl + key_entry_height/4);
		draw_clip_line( xl + key_sample_left,  yl + key_entry_height/4,
			    xl + key_sample_left,  yl - key_entry_height/4);
		closepath();
	    }
	    if (fs->fillstyle != FS_EMPTY && fs->fillstyle != FS_DEFAULT
	    && !(fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW)) {
		(*t->linetype)(this_plot->lp_properties.l_type);
		if (this_plot->lp_properties.use_palette)
		    apply_pm3dcolor(&this_plot->lp_properties.pm3d_color,t);
	    }
	}

    } else if (this_plot->plot_style == VECTOR && t->arrow) {
	    apply_head_properties(&(this_plot->arrow_properties));
	    curr_arrow_headlength = -1;
	    draw_clip_arrow(xl + key_sample_left, yl, xl + key_sample_right, yl,
			this_plot->arrow_properties.head);

    } else if ((this_plot->plot_style & PLOT_STYLE_HAS_LINE)
		   || ((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
		       && this_plot->plot_type == DATA)) {
	    /* errors for data plots only */
	    draw_clip_line(xl + key_sample_left, yl, xl + key_sample_right, yl);
    }

    if ((this_plot->plot_type == DATA)
	&& (this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
	&& (this_plot->plot_style != CANDLESTICKS)
	&& (bar_size > 0.0)) {
	draw_clip_line( xl + key_sample_left, yl + ERRORBARTIC,
			xl + key_sample_left, yl - ERRORBARTIC);
	draw_clip_line( xl + key_sample_right, yl + ERRORBARTIC,
			xl + key_sample_right, yl - ERRORBARTIC);
    }

    /* oops - doing the point sample now would break the postscript
     * terminal for example, which changes current line style
     * when drawing a point, but does not restore it. We must wait
     then draw the point sample at the end of do_plot (line 2058)
     */

    /* Restore previous clipping area */
    clip_area = clip_save;
}

#ifdef EAM_OBJECTS
void
do_rectangle( int dimensions, t_object *this_object, int style )
{
    double x1, y1, x2, y2;
    int x, y;
    unsigned int w, h;
    TBOOLEAN clip_x = FALSE;
    TBOOLEAN clip_y = FALSE;
    struct lp_style_type lpstyle;
    struct fill_style_type *fillstyle;
    t_rectangle *this_rect = &this_object->o.rectangle;

	if (this_rect->type == 1) {
	    double width, height;

	    if (dimensions == 2 || this_rect->center.scalex == screen) {
		map_position_double(&this_rect->center, &x1, &y1, "rect");
		map_position_r(&this_rect->extent, &width, &height, "rect");
	    } else if (splot_map) {
		int junkw, junkh;
		map3d_position_double(&this_rect->center, &x1, &y1, "rect");
		map3d_position_r(&this_rect->extent, &junkw, &junkh, "rect");
		width = junkw;
		height = junkh;
	    } else
		return;

	    x1 -= width/2;
	    y1 -= height/2;
	    x2 = x1 + width;
	    y2 = y1 + height;
	    w = width;
	    h = height;
	    if (this_rect->extent.scalex == first_axes
	    ||  this_rect->extent.scalex == second_axes)
		clip_x = TRUE;
	    if (this_rect->extent.scaley == first_axes
	    ||  this_rect->extent.scaley == second_axes)
		clip_y = TRUE;

	} else {
	    if ((dimensions == 2) 
	    ||  (this_rect->bl.scalex == screen && this_rect->tr.scalex == screen)) {
		map_position_double(&this_rect->bl, &x1, &y1, "rect");
		map_position_double(&this_rect->tr, &x2, &y2, "rect");
	    } else if (splot_map) {
		map3d_position_double(&this_rect->bl, &x1, &y1, "rect");
		map3d_position_double(&this_rect->tr, &x2, &y2, "rect");
	    } else
		return;

	    if (x1 > x2) {double t=x1; x1=x2; x2=t;}
	    if (y1 > y2) {double t=y1; y1=y2; y2=t;}
	    if (this_rect->bl.scalex == first_axes
	    ||  this_rect->bl.scalex == second_axes)
		clip_x = TRUE;
	    if (this_rect->bl.scaley == first_axes
	    ||  this_rect->bl.scaley == second_axes)
		clip_y = TRUE;
	}

	/* FIXME - Should there be a generic clip_rectangle() routine?	*/
	/* Clip to the graph boundaries, but only if the rectangle 	*/
	/* itself was specified in plot coords.				*/
	if (clip_area) {
	    if (clip_x && x1 < clip_area->xleft)
		x1 = clip_area->xleft;
	    if (clip_x && x2 > clip_area->xright)
		x2 = clip_area->xright;
	    if (clip_y && y1 < clip_area->ybot)
		y1 = clip_area->ybot;
	    if (clip_y && y2 > clip_area->ytop)
		y2 = clip_area->ytop;
	    if (x1 > x2 || y1 > y2)
		return;
	}

	w = x2 - x1;
	h = y2 - y1;
	x = x1;
	y = y1;

	if (w == 0 || h == 0)
	    return;

	if (this_object->lp_properties.l_type == LT_DEFAULT)
	    lpstyle = default_rectangle.lp_properties;
	else
	    lpstyle = this_object->lp_properties;
	if (lpstyle.l_width > 0)
	    lpstyle.l_width = this_object->lp_properties.l_width;
	
	if (this_object->fillstyle.fillstyle == FS_DEFAULT)
	    fillstyle = &default_rectangle.fillstyle;
	else
	    fillstyle = &this_object->fillstyle;

	term_apply_lp_properties(&lpstyle);
	style = style_from_fill(fillstyle);

	if (style != FS_EMPTY && term->fillbox)
		(*term->fillbox) (style, x, y, w, h);

	if (need_fill_border(fillstyle)) {
	    (*term->move)   (x, y);
	    (*term->vector) (x, y+h);
	    (*term->vector) (x+w, y+h);
	    (*term->vector) (x+w, y);
	    (*term->vector) (x, y);
	}

    return;
}

void
do_ellipse( int dimensions, t_ellipse *e, int style )
{
    gpiPoint vertex[120];
    int i;
    double angle;
    double cx, cy;
    double xoff, yoff;
    int junkw, junkh;
    double cosO = cos(DEG2RAD * e->orientation);
    double sinO = sin(DEG2RAD * e->orientation);
    double A = e->extent.x / 2.0;	/* Major axis radius */
    double B = e->extent.y / 2.0;	/* Minor axis radius */
    struct position pos = e->extent;	/* working copy with axis info attached */

    /* Choose how many segments to draw for this ellipse */
    int segments = 72;
    double ang_inc  =  M_PI / 36.;

    /* Find the center of the ellipse */
    if (dimensions == 2)
	map_position_double(&e->center, &cx, &cy, "ellipse");
    else
	map3d_position_double(&e->center, &cx, &cy, "ellipse");

    /* Calculate the vertices */
    vertex[0].style = style;
    for (i=0, angle = 0.0; i<=segments; i++, angle += ang_inc) {
	pos.x = A * cosO * cos(angle) - B * sinO * sin(angle);
	pos.y = A * sinO * cos(angle) + B * cosO * sin(angle);
	if (dimensions == 2)
	    map_position_r(&pos, &xoff, &yoff, "ellipse");
	else {
	    map3d_position_r(&pos, &junkw, &junkh, "ellipse");
	    xoff = junkw;
	    yoff = junkh;
	}
	vertex[i].x = cx + xoff;
	vertex[i].y = cy + yoff;
    }

    if (style) {
	/* Fill in the center */
	if (term->filled_polygon)
	    term->filled_polygon(segments, vertex);
    } else {
	/* Draw the arc */
	for (i=0; i<segments; i++)
	    draw_clip_line( vertex[i].x, vertex[i].y,
		vertex[i+1].x, vertex[i+1].y );
    }
}

void
do_polygon( int dimensions, t_polygon *p, int style )
{
    static gpiPoint *corners = NULL;
    static gpiPoint *clpcorn = NULL;
    BoundingBox *clip_save = clip_area;
    TBOOLEAN noclip = TRUE;
    int nv;

    if (!p->vertex)
	return;

    corners = gp_realloc(corners, p->type * sizeof(gpiPoint), "polygon");
    clpcorn = gp_realloc(clpcorn, 2 * p->type * sizeof(gpiPoint), "polygon");
    for (nv = 0; nv < p->type; nv++) {
	if (dimensions == 3)
	    map3d_position(&p->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
	else
	    map_position(&p->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
	
	/* Any vertex not given in screen coords will force clipping */
	if (!noclip || p->vertex[nv].scalex != screen || p->vertex[nv].scaley != screen)
	    noclip = FALSE;
    }

    if (noclip)
	clip_area = &canvas;

    if (term->filled_polygon && style) {
	int i,o,clipped;
	gpiPoint temp;
	for (i=0,o=0; i<nv-1; i++) {
	    clpcorn[o] = corners[i];
	    temp = corners[i+1];
	    clipped = clip_line(&corners[i].x, &corners[i].y, &corners[i+1].x, &corners[i+1].y);
	    if (clipped == 0) continue;	/* both ends out of range */
	    if (clipped  > 0) o++;	/* both ends in range */
	    if (clipped  < 0) {		/* clipped to range */
		clpcorn[o++] = corners[i];
		clpcorn[o++] = corners[i+1];
		corners[i+1] = temp;
	    }
	}
	if (clipped == 1)
	    clpcorn[o++] = corners[i];
	clpcorn[0].style = style;
	term->filled_polygon(o, clpcorn);

    } else { /* Just draw the outline? */
	int i;
 	newpath();
	for (i=0; i<nv-1; i++)
	    draw_clip_line( corners[i].x, corners[i].y,
		corners[i+1].x, corners[i+1].y );
	if (corners[i].x != corners[0].x || corners[i].y != corners[0].y)
	    draw_clip_line( corners[i].x, corners[i].y,
		corners[0].x, corners[0].y );
	closepath();
    }

    clip_area = clip_save;

}
#endif

static TBOOLEAN
check_for_variable_color(struct curve_points *plot, struct coordinate *point)
{
    if ((plot->lp_properties.pm3d_color.value < 0.0)
    &&  (plot->lp_properties.pm3d_color.type == TC_RGB)) {
	set_rgbcolor(point->yhigh);
	return TRUE;
    } else if (plot->lp_properties.pm3d_color.type == TC_Z) {
	set_color( cb2gray(point->yhigh) );
	return TRUE;
    } else if (plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
	lp_style_type lptmp;
	lp_use_properties(&lptmp, (int)(point->yhigh));
	apply_pm3dcolor(&(lptmp.pm3d_color), term);
	return TRUE;
    } else
	return FALSE;
}
	    

/* Similar to HBB's comment above, this routine is shared with
 * graph3d.c, so it shouldn't be in this module (graphics.c).
 * However, I feel that 2d and 3d graphing routines should be
 * made as much in common as possible.  They seem to be
 * bifurcating a bit too much.  (Dan Sebald)
 */
#include "util3d.h"

/* These might work better as fuctions, but defines will do for now. */
#define ERROR_NOTICE(str)         "\nGNUPLOT (plot_image):  " str

/* hyperplane_between_points:
 * Compute the hyperplane representation of a line passing
 *  between two points.
 */
void
hyperplane_between_points(double *p1, double *p2, double *w, double *b)
{
    w[0] = p1[1] - p2[1];
    w[1] = p2[0] - p1[0];
    *b = -(w[0]*p1[0] + w[1]*p1[1]);
}

/* plot_image_or_update_axes:
 * Plot the coordinates similar to the points option except use
 *  pixels.  Check if the data forms a valid image array, i.e.,
 *  one for which points are spaced equidistant along two non-
 *  coincidence vectors.  If the two directions are orthogonal
 *  within some tolerance and they are aligned with the view
 *  box x and y directions, then use the image feature of the
 *  terminal if it has one.  Otherwise, use parallelograms via
 *  the polynomial function.  If it just necessary to update
 *  the axis ranges for `set autoscale`, do so and then return.
 */
void
plot_image_or_update_axes(void *plot, TBOOLEAN update_axes)
{

    struct coordinate GPHUGE *points;
    int p_count;
    int i;
    double w_hyp[2], b_hyp;                    /* Hyperlane vector and constant */
    double p_start_corner[2], p_end_corner[2]; /* Points used for computing hyperplane. */
    int K = 0, L = 0;                          /* Dimensions of image grid. K = <scan line length>, L = <number of scan lines>. */
    double p_mid_corner[2];                    /* Point representing first corner found, i.e. p(K-1) */
    double delta_x_grid[2] = {0, 0};           /* Spacings between points, two non-orthogonal directions. */
    double delta_y_grid[2] = {0, 0};
    int grid_corner[4] = {-1, -1, -1, -1};     /* The corner pixels of the image. */
    double view_port_x[2];                     /* Viewable portion of the image. */
    double view_port_y[2];
    double view_port_z[2] = {0,0};
    t_imagecolor pixel_planes;
    TBOOLEAN project_points = FALSE;		/* True if 3D plot */

    if (((struct surface_points *)plot)->plot_type == DATA3D)
	project_points = TRUE;

    if (project_points) {
	points = ((struct surface_points *)plot)->iso_crvs->points;
	p_count = ((struct surface_points *)plot)->iso_crvs->p_count;
	pixel_planes = ((struct surface_points *)plot)->image_properties.type;
    } else {
	points = ((struct curve_points *)plot)->points;
	p_count = ((struct curve_points *)plot)->p_count;
	pixel_planes = ((struct curve_points *)plot)->image_properties.type;
    }

    if (p_count < 1) {
	fprintf(stderr, ERROR_NOTICE("No points (visible or invisible) to plot.\n\n"));
	return;
    }

    if (p_count < 4) {
	fprintf(stderr, ERROR_NOTICE("Image grid must be at least 4 points (2 x 2).\n\n"));
	return;
    }

    /* Check if the pixel data forms a valid rectangular grid for potential image
     * matrix support.  A general grid orientation is considered.  If the grid
     * points are orthogonal and oriented along the x/y dimensions the terminal
     * function for images will be used.  Otherwise, the terminal function for
     * filled polygons are used to construct parallelograms for the pixel elements.
     */

    /* Compute the hyperplane representation of the cross diagonal from
     * the very first point of the scan to the very last point of the
     * scan.
     */
    if (project_points) {
	map3d_xy_double(points[0].x, points[0].y, points[0].z, &p_start_corner[0], &p_start_corner[1]);
	map3d_xy_double(points[p_count-1].x, points[p_count-1].y, points[p_count-1].z, &p_end_corner[0], &p_end_corner[1]);
    } else {
	p_start_corner[0] = points[0].x;
	p_start_corner[1] = points[0].y;
	p_end_corner[0] = points[p_count-1].x;
	p_end_corner[1] = points[p_count-1].y;
    }

    hyperplane_between_points(p_start_corner, p_end_corner, w_hyp, &b_hyp);

    for (K = p_count, i=1; i < p_count; i++) {
	double p[2];
	if (project_points) {
	    map3d_xy_double(points[i].x, points[i].y, points[i].z, &p[0], &p[1]);
	} else {
	    p[0] = points[i].x;
	    p[1] = points[i].y;
	}
	if (i == 1) {
	    /* Determine what side (sign) of the hyperplane the second point is on.
	     * If the second point is on the negative side of the plane, change
	     * the sign of hyperplane variables.  Then any remaining points on the
	     * first line will test positive in the hyperplane formula.  The first
	     * point on the second line will test negative.
	     */
	    if ((w_hyp[0]*p[0] + w_hyp[1]*p[1] + b_hyp) < 0) {
		w_hyp[0] = -w_hyp[0];
		w_hyp[1] = -w_hyp[1];
		b_hyp = -b_hyp;
	    }
	} else {
	    /* The first point on the opposite side of the hyperplane is the
	     * candidate for the first point of the second scan line.
	     */
	    if ((w_hyp[0]*p[0] + w_hyp[1]*p[1] + b_hyp) < 0) {
		K = i;
		break;
	    }
	}
    }

    if (K == p_count) {
	fprintf(stderr, ERROR_NOTICE("Image grid must be at least 2 x 2.\n\n"));
	return;
    }
    L = p_count/K;
    if (((double)L) != ((double)p_count/K)) {
	fprintf(stderr, ERROR_NOTICE("Number of pixels cannot be factored into integers matching grid. N = %d  K = %d\n\n"), p_count, K);
	return;
    }
    grid_corner[0] = 0;
    grid_corner[1] = K-1;
    grid_corner[3] = p_count - 1;
    grid_corner[2] = p_count - K;
    if (project_points) {
	map3d_xy_double(points[K-1].x, points[K-1].y, points[K-1].z, &p_mid_corner[0], &p_mid_corner[1]);
    } else {
	p_mid_corner[0] = points[K-1].x;
	p_mid_corner[1] = points[K-1].y;
    }
    /* The grid spacing in one direction. */
    delta_x_grid[0] = (p_mid_corner[0] - p_start_corner[0])/(K-1);
    delta_y_grid[0] = (p_mid_corner[1] - p_start_corner[1])/(K-1);
    /* The grid spacing in the second direction. */
    delta_x_grid[1] = (p_end_corner[0] - p_mid_corner[0])/(L-1);
    delta_y_grid[1] = (p_end_corner[1] - p_mid_corner[1])/(L-1);

    if (update_axes) {
	for (i=0; i < 4; i++) {
	    coord_type dummy_type = INRANGE;
	    double x = points[grid_corner[i]].x;
	    double y = points[grid_corner[i]].y;
	    x -= (points[grid_corner[(5-i)%4]].x - points[grid_corner[i]].x)/(2*(K-1));
	    y -= (points[grid_corner[(5-i)%4]].y - points[grid_corner[i]].y)/(2*(K-1));
	    x -= (points[grid_corner[(i+2)%4]].x - points[grid_corner[i]].x)/(2*(L-1));
	    y -= (points[grid_corner[(i+2)%4]].y - points[grid_corner[i]].y)/(2*(L-1));
	    /* Update range and store value back into itself. */
	    STORE_WITH_LOG_AND_UPDATE_RANGE(x, x, dummy_type, ((struct curve_points *)plot)->x_axis,
				((struct curve_points *)plot)->noautoscale, NOOP, x = -VERYLARGE);
	    STORE_WITH_LOG_AND_UPDATE_RANGE(y, y, dummy_type, ((struct curve_points *)plot)->y_axis,
				((struct curve_points *)plot)->noautoscale, NOOP, y = -VERYLARGE);
	}
	return;
    }

    /* Check if the pixel grid is orthogonal and oriented with axes.
     * If so, then can use efficient terminal image routines.
     */
    {
    TBOOLEAN rectangular_image = FALSE;
    TBOOLEAN fallback = FALSE;

#define SHIFT_TOLERANCE 0.01
    if ( ( (fabs(delta_x_grid[0]) < SHIFT_TOLERANCE*fabs(delta_x_grid[1]))
	|| (fabs(delta_x_grid[1]) < SHIFT_TOLERANCE*fabs(delta_x_grid[0])) )
	&& ( (fabs(delta_y_grid[0]) < SHIFT_TOLERANCE*fabs(delta_y_grid[1]))
	|| (fabs(delta_y_grid[1]) < SHIFT_TOLERANCE*fabs(delta_y_grid[0])) ) ) {

	rectangular_image = TRUE;

	/* If the terminal does not have image support then fall back to
	 * using polygons to construct pixels.
	 */
	if (project_points)
	    fallback = !splot_map || ((struct surface_points *)plot)->image_properties.fallback;
	else
	    fallback = ((struct curve_points *)plot)->image_properties.fallback;
    }

    if (pixel_planes == IC_PALETTE && make_palette()) {
	fprintf(stderr, ERROR_NOTICE("This terminal does not support palette-based images.\n\n"));
	return;
    }
    if ((pixel_planes == IC_RGB || pixel_planes == IC_RGBA) && !term->set_color) {
	fprintf(stderr, ERROR_NOTICE("This terminal does not support rgb images.\n\n"));
	return;
    }
    /* Use generic code to handle alpha channel if the terminal can't */
    if (pixel_planes == IC_RGBA && !(term->flags & TERM_ALPHA_CHANNEL))
	fallback = TRUE;

    view_port_x[0] = (X_AXIS.set_autoscale & AUTOSCALE_MIN) ? X_AXIS.min : X_AXIS.set_min;
    view_port_x[1] = (X_AXIS.set_autoscale & AUTOSCALE_MAX) ? X_AXIS.max : X_AXIS.set_max;
    view_port_y[0] = (Y_AXIS.set_autoscale & AUTOSCALE_MIN) ? Y_AXIS.min : Y_AXIS.set_min;
    view_port_y[1] = (Y_AXIS.set_autoscale & AUTOSCALE_MAX) ? Y_AXIS.max : Y_AXIS.set_max;
    if (project_points) {
	view_port_z[0] = (Z_AXIS.set_autoscale & AUTOSCALE_MIN) ? Z_AXIS.min : Z_AXIS.set_min;
	view_port_z[1] = (Z_AXIS.set_autoscale & AUTOSCALE_MAX) ? Z_AXIS.max : Z_AXIS.set_max;
    }

    if (rectangular_image && term->image && !fallback) {

	/* There are eight ways that a valid pixel grid can be entered.  Use table
	 * lookup instead of if() statements.  (Draw the various array combinations
	 * on a sheet of paper, or see the README file.)
	 */
	int line_length, i_delta_pixel, i_delta_line, i_start;
	int pixel_1_1, pixel_M_N;
	coordval *image;
	int array_size;
	float xsts, ysts;

	if (!project_points) {
	    /* Determine axis direction according to the sign of the terminal scale. */
	    xsts = (axis_array[x_axis].term_scale > 0 ? +1 : -1);
	    ysts = (axis_array[y_axis].term_scale > 0 ? +1 : -1);
	} else {
	    /* 3D plots do not use the term_scale mechanism AXIS_SETSCALE(). */
	    xsts = 1;
	    ysts = 1;
	}

	/* Set up parameters for indexing through the image matrix to transfer data.
	 * These formulas were derived for a terminal image routine which uses the
	 * upper left corner as pixel (1,1).
	 */
	if (fabs(delta_x_grid[0]) > fabs(delta_x_grid[1])) {
	    line_length = K;
	    i_start = (delta_y_grid[1]*ysts > 0 ? L : 1) * K - (delta_x_grid[0]*xsts > 0 ? K : 1);
	    i_delta_pixel = (delta_x_grid[0]*xsts > 0 ? +1 : -1);
	    i_delta_line = (delta_x_grid[0]*xsts > 0 ? -K : +K) + (delta_y_grid[1]*ysts > 0 ? -K : +K);
	} else {
	    line_length = L;
	    i_start = (delta_x_grid[1]*xsts > 0 ? 1 : L) * K - (delta_y_grid[0]*ysts > 0 ? 1 : K);
	    i_delta_pixel = (delta_x_grid[1]*xsts > 0 ? +K : -K);
	    i_delta_line = K*L*(delta_x_grid[1]*xsts > 0 ? -1 : +1) + (delta_y_grid[0]*ysts > 0 ? -1 : +1);
	}

	/* Assign enough memory for the maximum image size. */
	array_size = K*L;

	/* If doing color, multiply size by three for RGB triples. */
	if (pixel_planes == IC_RGB)
	    array_size *= 3;
	else if (pixel_planes == IC_RGBA)
	    array_size *= 4;

	image = (coordval *) gp_alloc(array_size*sizeof(image[0]),"image");

	/* Place points into image array based upon the arrangement of point indices and
	 * the visibility of pixels.
	 */
	if (image != NULL) {

	    int j;
	    gpiPoint corners[4];
	    int M = 0, N = 0;  /* M = number of columns, N = number of rows.  (K and L don't
				* have a set direction, but M and N do.)
				*/
	    int i_image, i_sub_image = 0;
	    double d_x_o_2, d_y_o_2, d_z_o_2;
	    int line_pixel_count = 0;

	    d_x_o_2 = ( (points[grid_corner[0]].x - points[grid_corner[1]].x)/(K-1)
			+ (points[grid_corner[0]].x - points[grid_corner[2]].x)/(L-1) ) / 2;
	    d_y_o_2 = ( (points[grid_corner[0]].y - points[grid_corner[1]].y)/(K-1)
			+ (points[grid_corner[0]].y - points[grid_corner[2]].y)/(L-1) ) / 2;
	    d_z_o_2 = ( (points[grid_corner[0]].z - points[grid_corner[1]].z)/(K-1)
			+ (points[grid_corner[0]].z - points[grid_corner[2]].z)/(L-1) ) / 2;

	    pixel_1_1 = -1;
	    pixel_M_N = -1;

	    /* Step through the points placing them in the proper spot in the matrix array. */
	    for (i=0, j=line_length, i_image=i_start; i < p_count; i++) {

		TBOOLEAN visible;
		double x, y, z, x_low, x_high, y_low, y_high, z_low, z_high;

		x = points[i_image].x;
		y = points[i_image].y;
		z = points[i_image].z;
		x_low = x - d_x_o_2;  x_high = x + d_x_o_2;
		y_low = y - d_y_o_2;  y_high = y + d_y_o_2;
		z_low = z - d_z_o_2;  z_high = z + d_z_o_2;

		/* Check if a portion of this pixel will be visible.  Do not use the
		 * points[i].type == INRANGE test because a portion of a pixel can
		 * extend into view and the INRANGE type doesn't account for this.
		 *
		 * This series of tests is designed for speed.  If one of the corners
		 * of the pixel in question falls in the view port range then the pixel
		 * will be visible.  Do this test first because it is the more likely
		 * of situations.  It could also happen that the view port is smaller
		 * than a pixel.  In that case, if one of the view port corners lands
		 * inside the pixel then the pixel in question will be visible.  This
		 * won't be as common, so do those tests last.  Set up the if structure
		 * in such a way that as soon as one of the tests is true, the conditional
		 * tests stop.
		 */
		if ( ( inrange(x_low, view_port_x[0], view_port_x[1]) || inrange(x_high, view_port_x[0], view_port_x[1]) )
		  && ( inrange(y_low, view_port_y[0], view_port_y[1]) || inrange(y_high, view_port_y[0], view_port_y[1]) )
		  && ( !project_points || inrange(z_low, view_port_z[0], view_port_z[1]) || inrange(z_high, view_port_z[0], view_port_z[1]) ) )
		    visible = TRUE;
		else if ( ( inrange(view_port_x[0], x_low, x_high) || inrange(view_port_x[1], x_low, x_high) )
		  && ( inrange(view_port_y[0], y_low, y_high) || inrange(view_port_y[1], y_low, y_high) )
		  && ( !project_points || inrange(view_port_z[0], z_low, z_high) || inrange(view_port_z[1], z_low, z_high) ) )
		    visible = TRUE;
		else
		    visible = FALSE;

		if (visible) {
		    if (pixel_1_1 < 0) {
			/* First visible point. */
			pixel_1_1 = i_image;
			M = 0;
			N = 1;
			line_pixel_count = 1;
		    } else {
			if (line_pixel_count == 0)
			    N += 1;
			line_pixel_count++;
			if ( (N != 1) && (line_pixel_count > M) ) {
			    fprintf(stderr, ERROR_NOTICE("Visible pixel grid has a scan line longer than previous scan lines."));
			    return;
			}
		    }

		    pixel_M_N = i_image;

		    if (pixel_planes == IC_PALETTE) {
			image[i_sub_image++] = cb2gray( points[i_image].CRD_COLOR );
		    } else {
			image[i_sub_image++] = cb2gray( points[i_image].CRD_R );
			image[i_sub_image++] = cb2gray( points[i_image].CRD_G );
			image[i_sub_image++] = cb2gray( points[i_image].CRD_B );
			if (pixel_planes == IC_RGBA)
			    image[i_sub_image++] = points[i_image].CRD_A;
		    }

		}

		i_image += i_delta_pixel;
		j--;
		if (j == 0) {
		    if (M == 0)
			M = line_pixel_count;
		    else if ((line_pixel_count > 0) && (line_pixel_count != M)) {
			fprintf(stderr, ERROR_NOTICE("Visible pixel grid has a scan line shorter than previous scan lines."));
			return;
		    }
		    line_pixel_count = 0;
		    i_image += i_delta_line;
		    j = line_length;
		}
	    }

	    if ( (M > 0) && (N > 0) ) {

		/* The information collected to this point is:
		 *
		 * M = <number of columns>
		 * N = <number of rows>
		 * image[] = M x N array of pixel data.
		 * pixel_1_1 = position in points[] associated with pixel (1,1)
		 * pixel_M_N = position in points[] associated with pixel (M,N)
		 */

		/* One of the delta values in each direction is zero, so add. */
		if (project_points) {
		    double x, y;
		    map3d_xy_double(points[pixel_1_1].x, points[pixel_1_1].y, points[pixel_1_1].z, &x, &y);
		    corners[0].x = x - fabs(delta_x_grid[0]+delta_x_grid[1])/2;
		    corners[0].y = y + fabs(delta_y_grid[0]+delta_y_grid[1])/2;
		    map3d_xy_double(points[pixel_M_N].x, points[pixel_M_N].y, points[pixel_M_N].z, &x, &y);
		    corners[1].x = x + fabs(delta_x_grid[0]+delta_x_grid[1])/2;
		    corners[1].y = y - fabs(delta_y_grid[0]+delta_y_grid[1])/2;
		    map3d_xy_double(view_port_x[0], view_port_y[0], view_port_z[0], &x, &y);
		    corners[2].x = x;
		    corners[2].y = y;
		    map3d_xy_double(view_port_x[1], view_port_y[1], view_port_z[1], &x, &y);
		    corners[3].x = x;
		    corners[3].y = y;
		} else {
		    corners[0].x = map_x(points[pixel_1_1].x - xsts*fabs(d_x_o_2));
		    corners[0].y = map_y(points[pixel_1_1].y + ysts*fabs(d_y_o_2));
		    corners[1].x = map_x(points[pixel_M_N].x + xsts*fabs(d_x_o_2));
		    corners[1].y = map_y(points[pixel_M_N].y - ysts*fabs(d_y_o_2));
		    corners[2].x = map_x(view_port_x[0]);
		    corners[2].y = map_y(view_port_y[1]);
		    corners[3].x = map_x(view_port_x[1]);
		    corners[3].y = map_y(view_port_y[0]);
		}

		if ( (pixel_planes == IC_PALETTE) || (pixel_planes == IC_RGB) || (pixel_planes == IC_RGBA))
		    (*term->image) (M, N, image, corners, pixel_planes);
		else
		    fprintf(stderr, ERROR_NOTICE("Invalid pixel color planes specified.\n\n"));
	    }

	    free ((void *)image);

	} else {
	    fprintf(stderr, ERROR_NOTICE("Could not allocate memory for image."));
	    return;
	}

    } else {	/* no term->image  or "with image failsafe" */

	/* Use sum of vectors to compute the pixel corners with respect to its center. */
	struct {double x; double y; double z;} delta_grid[2], delta_pixel[2];
	int j, i_image;

	if (!term->filled_polygon)
	    int_error(NO_CARET, "This terminal does not support filled polygons");

	/* Grid spacing in 3D space. */
	delta_grid[0].x = (points[grid_corner[1]].x - points[grid_corner[0]].x)/(K-1);
	delta_grid[0].y = (points[grid_corner[1]].y - points[grid_corner[0]].y)/(K-1);
	delta_grid[0].z = (points[grid_corner[1]].z - points[grid_corner[0]].z)/(K-1);
	delta_grid[1].x = (points[grid_corner[2]].x - points[grid_corner[0]].x)/(L-1);
	delta_grid[1].y = (points[grid_corner[2]].y - points[grid_corner[0]].y)/(L-1);
	delta_grid[1].z = (points[grid_corner[2]].z - points[grid_corner[0]].z)/(L-1);

	/* Pixel dimensions in the 3D space. */
	delta_pixel[0].x = (delta_grid[0].x + delta_grid[1].x) / 2;
	delta_pixel[0].y = (delta_grid[0].y + delta_grid[1].y) / 2;
	delta_pixel[0].z = (delta_grid[0].z + delta_grid[1].z) / 2;
	delta_pixel[1].x = (delta_grid[0].x - delta_grid[1].x) / 2;
	delta_pixel[1].y = (delta_grid[0].y - delta_grid[1].y) / 2;
	delta_pixel[1].z = (delta_grid[0].z - delta_grid[1].z) / 2;

	i_image = 0;

	for (j=0; j < L; j++) {

	    double x_line_start, y_line_start, z_line_start;

	    x_line_start = points[grid_corner[0]].x + j * delta_grid[1].x;
	    y_line_start = points[grid_corner[0]].y + j * delta_grid[1].y;
	    z_line_start = points[grid_corner[0]].z + j * delta_grid[1].z;

	    for (i=0; i < K; i++) {

		double x, y, z;
		TBOOLEAN view_in_pixel = FALSE;
		int corners_in_view = 0;
		struct {double x; double y; double z;} p_corners[4]; /* Parallelogram corners. */
		int k;

		/* If terminal can't handle alpha, treat it as all-or-none. */
		if (pixel_planes == IC_RGBA) {
		    if ((points[i_image].CRD_A == 0)
		    ||  (points[i_image].CRD_A < 128 &&  !(term->flags & TERM_ALPHA_CHANNEL))) {
			i_image++;
			continue;
		    }
		}

		x = x_line_start + i * delta_grid[0].x;
		y = y_line_start + i * delta_grid[0].y;
		z = z_line_start + i * delta_grid[0].z;

		p_corners[0].x = x + delta_pixel[0].x;
		p_corners[0].y = y + delta_pixel[0].y;
		p_corners[0].z = z + delta_pixel[0].z;
		p_corners[1].x = x + delta_pixel[1].x;
		p_corners[1].y = y + delta_pixel[1].y;
		p_corners[1].z = z + delta_pixel[1].z;
		p_corners[2].x = x - delta_pixel[0].x;
		p_corners[2].y = y - delta_pixel[0].y;
		p_corners[2].z = z - delta_pixel[0].z;
		p_corners[3].x = x - delta_pixel[1].x;
		p_corners[3].y = y - delta_pixel[1].y;
		p_corners[3].z = z - delta_pixel[1].z;

		/* Check if any of the corners are viewable */
		for (k=0; k < 4; k++) {
		    if ( inrange(p_corners[k].x, view_port_x[0], view_port_x[1])
		    &&   inrange(p_corners[k].y, view_port_y[0], view_port_y[1])
		    &&  (inrange(p_corners[k].z, view_port_z[0], view_port_z[1]) || !project_points || splot_map))
		    	corners_in_view++;
		}

		if (corners_in_view > 0 || view_in_pixel) {

		    int N_corners = 0;    /* Number of corners. */
		    gpiPoint corners[5];  /* At most 5 corners. */

		    corners[0].style = FS_DEFAULT;

		    if (corners_in_view > 0) {
			int i_corners;

			N_corners = 4;

			for (i_corners=0; i_corners < N_corners; i_corners++) {
			    if (project_points) {
				    map3d_xy_double(p_corners[i_corners].x, p_corners[i_corners].y,
						    p_corners[i_corners].z, &x, &y);
				    corners[i_corners].x = x;
				    corners[i_corners].y = y;
			    } else {
				    corners[i_corners].x = map_x(p_corners[i_corners].x);
				    corners[i_corners].y = map_y(p_corners[i_corners].y);
			    }
			    /* Clip rectangle if necessary */
			    if (rectangular_image && term->fillbox && corners_in_view < 4) {
				if (corners[i_corners].x < clip_area->xleft)
				    corners[i_corners].x = clip_area->xleft;
				if (corners[i_corners].x > clip_area->xright)
				    corners[i_corners].x = clip_area->xright;
				if (corners[i_corners].y > clip_area->ytop)
				    corners[i_corners].y = clip_area->ytop;
				if (corners[i_corners].y < clip_area->ybot)
				    corners[i_corners].y = clip_area->ybot;
			    }
			}
		    } else {
			/* DJS FIXME:
			 * Could still be visible if any of the four corners of the view port are
			 * within the parallelogram formed by the pixel.  This is tricky geometry.
			 */
		    }

		    if (N_corners > 0) {
			if (pixel_planes == IC_PALETTE) {
			    if (isnan(points[i_image].CRD_COLOR))
				goto skip_pixel;
			    set_color( cb2gray(points[i_image].CRD_COLOR) );
			} else {
			    int r = cb2gray(points[i_image].CRD_R) * 255. + 0.5;
			    int g = cb2gray(points[i_image].CRD_G) * 255. + 0.5;
			    int b = cb2gray(points[i_image].CRD_B) * 255. + 0.5;
			    int rgblt = (r << 16) + (g << 8) + b;
			    set_rgbcolor(rgblt);
			}
			if (pixel_planes == IC_RGBA) {
			    int alpha = points[i_image].CRD_A * 100./255.;
			    if (alpha == 0)
				goto skip_pixel;
			    if (term->flags & TERM_ALPHA_CHANNEL)
				corners[0].style = FS_TRANSPARENT_SOLID + (alpha<<4);
			}

			if (rectangular_image && term->fillbox) {
			    /* Some terminals (canvas) can do filled rectangles */
			    /* more efficiently than filled polygons. */
			    (*term->fillbox)( corners[0].style,
				GPMIN(corners[0].x, corners[2].x),
				GPMIN(corners[0].y, corners[2].y),
				abs(corners[2].x - corners[0].x),
				abs(corners[2].y - corners[0].y));
			} else {
			    (*term->filled_polygon) (N_corners, corners);
			}
		    }
		}
skip_pixel:
		i_image++;
	    }
	}
    }
    }

}
