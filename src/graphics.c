#ifndef lint
static char *RCSid() { return RCSid("$Id: graphics.c,v 1.29 2000/05/02 18:41:08 lhecking Exp $"); }
#endif

/* GNUPLOT - graphics.c */

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

#include "graphics.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "gp_time.h"
#include "misc.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"

/* key placement is calculated in boundary, so we need file-wide variables
 * To simplify adjustments to the key, we set all these once [depends on
 * key_reverse] and use them throughout.
 */

/*{{{  local and global variables */
static int key_sample_width;	/* width of line sample */
static int key_sample_left;	/* offset from x for left of line sample */
static int key_sample_right;	/* offset from x for right of line sample */
static int key_point_offset;	/* offset from x for point sample */
static int key_text_left;	/* offset from x for left-justified text */
static int key_text_right;	/* offset from x for right-justified text */
static int key_size_left;	/* size of left bit of key (text or sample, depends on key_reverse) */
static int key_size_right;	/* size of right part of key (including padding) */

/* I think the following should also be static ?? */

static int max_ptitl_len = 0;	/* max length of plot-titles (keys) */
static int ktitl_lines = 0;	/* no lines in key_title (key header) */
static int ptitl_cnt;		/* count keys with len > 0  */
static int key_cols;		/* no cols of keys */
static int key_rows, key_col_wth, yl_ref;
static struct clipbox keybox;	/* boundaries for key field */

/* set by tic_callback - how large to draw polar radii */
static double largest_polar_circle;

/* HBB 990829 FIXME: this is never modified at all !?? */
char default_font[MAX_ID_LEN + 1] = "";	/* Entry font added by DJL */

static int xlablin, x2lablin, ylablin, y2lablin, titlelin, xticlin, x2ticlin;

static int key_entry_height;	/* bigger of t->v_size, pointsize*t->v_tick */
static int p_width, p_height;	/* pointsize * { t->h_tic | t->v_tic } */


/* there are several things on right of plot - key, y2tics and y2label
 * when working out boundary, save posn of y2label for later...
 * Same goes for x2label.
 * key posn is also stored in keybox.xl, and tics go at xright
 */
static int ylabel_x, y2label_x, xlabel_y, x2label_y, title_y, time_y, time_x;
static int ylabel_y, y2label_y, xtic_y, x2tic_y, ytic_x, y2tic_x;
/*}}} */

/*{{{  static fns and local macros */
static void plot_impulses __PROTO((struct curve_points * plot, int yaxis_x, int xaxis_y));
static void plot_lines __PROTO((struct curve_points * plot));
static void plot_points __PROTO((struct curve_points * plot));
static void plot_dots __PROTO((struct curve_points * plot));
static void plot_bars __PROTO((struct curve_points * plot));
static void plot_boxes __PROTO((struct curve_points * plot, int xaxis_y));
static void plot_vectors __PROTO((struct curve_points * plot));
static void plot_f_bars __PROTO((struct curve_points * plot));
static void plot_c_bars __PROTO((struct curve_points * plot));

static void edge_intersect __PROTO((struct coordinate GPHUGE * points, int i, double *ex, double *ey));
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

static void boundary __PROTO((TBOOLEAN scaling, struct curve_points * plots, int count));

/* widest2d_callback keeps longest so far in here */
static int widest_tic;

static void widest2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));
static void ytick2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));
static void xtick2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));


/* for plotting error bars
 * half the width of error bar tic mark
 */
#define ERRORBARTIC (t->h_tic/2)

/*
 * The Amiga SAS/C 6.2 compiler moans about macro envocations causing
 * multiple calls to functions. I converted these macros to inline
 * functions coping with the problem without loosing speed.
 * If your compiler supports __inline, you should add it to the
 * #ifdef directive
 * (MGR, 1993)
 */

#ifdef AMIGA_SC_6_1
GP_INLINE static TBOOLEAN
i_inrange(int z, int min, int max)
{
    return ((min < max) ? ((z >= min) && (z <= max)) : ((z >= max) && (z <= min)));
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

static int lkey;

/* First attempt at double axes...
 * put the scale factors into a similar array
 */

/* arrays now in graphics.h */

static int x_axis = FIRST_X_AXIS, y_axis = FIRST_Y_AXIS;	/* current axes */


/* (DFK) Watch for cancellation error near zero on axes labels */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
#define NearlyEqual(x,y,tic) (fabs((x)-(y)) < ((tic) * SIGNIF))
/*}}} */

/*{{{  widest2d_callback() */
/* we determine widest tick label by getting gen_ticks to call this
 * routine with every label
 */

static void
widest2d_callback(axis, place, text, grid)
AXIS_INDEX axis;
double place;
char *text;
struct lp_style_type grid;
{
    int len = label_width(text, NULL);
    if (len > widest_tic)
	widest_tic = len;
}

/*}}} */


/*{{{  boundary() */
/* borders of plotting area
 * computed once on every call to do_plot
 *
 * The order in which things is done is getting pretty critical:
 *  ytop depends on title, x2label, ylabels (if no rotated text)
 *  ybot depends on key, if TUNDER
 *  once we have these, we can setup the y1 and y2 tics and the 
 *  only then can we calculate xleft and xright
 *  xright depends also on key TRIGHT
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
boundary(scaling, plots, count)
TBOOLEAN scaling;		/* TRUE if terminal is doing the scaling */
struct curve_points *plots;
int count;
{
    int ytlen;
    int yticlin = 0, y2ticlin = 0, timelin = 0;

    register struct termentry *t = term;
    int key_h, key_w;
    int can_rotate = (*t->text_angle) (1);

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
    int xtic_height;
    int ytic_width;
    int y2tic_width;

    /* figure out which rotatable items are to be rotated
     * (ylabel and y2label are rotated if possible) */
    int vertical_timelabel = can_rotate && timelabel_rotate;
    int vertical_xtics = can_rotate && rotate_xtics;
    int vertical_x2tics = can_rotate && rotate_x2tics;
    int vertical_ytics = can_rotate && rotate_ytics;
    int vertical_y2tics = can_rotate && rotate_y2tics;

    lkey = key;			/* but we may have to disable it later */

    xticlin = ylablin = y2lablin = xlablin = x2lablin = titlelin = 0;

    /*{{{  count lines in labels and tics */
    if (*title.text)
	label_width(title.text, &titlelin);
    if (*xlabel.text)
	label_width(xlabel.text, &xlablin);
    if (*x2label.text)
	label_width(x2label.text, &x2lablin);
    if (*ylabel.text)
	label_width(ylabel.text, &ylablin);
    if (*y2label.text)
	label_width(y2label.text, &y2lablin);
    if (xtics)
	label_width(xformat, &xticlin);
    if (x2tics)
	label_width(x2format, &x2ticlin);
    if (ytics)
	label_width(yformat, &yticlin);
    if (y2tics)
	label_width(y2format, &y2ticlin);
    if (*timelabel.text)
	label_width(timelabel.text, &timelin);
    /*}}} */

    /*{{{  preliminary ytop  calculation */

    /*     first compute heights of things to be written in the margin */

    /* title */
    if (titlelin)
	title_textheight = (int) ((titlelin + title.yoffset + 1) * (t->v_char));
    else
	title_textheight = 0;

    /* x2label */
    if (x2lablin) {
	x2label_textheight = (int) ((x2lablin + x2label.yoffset) * (t->v_char));
	if (!x2tics)
	    x2label_textheight += 0.5 * t->v_char;
    } else
	x2label_textheight = 0;

    /* tic labels */
    if (x2tics & TICS_ON_BORDER) {
	/* ought to consider tics on axes if axis near border */
	if (vertical_x2tics) {
	    /* guess at tic length, since we don't know it yet
	       --- we'll fix it after the tic labels have been created */
	    x2tic_textheight = (int) (5 * (t->h_char));
	} else
	    x2tic_textheight = (int) ((x2ticlin) * (t->v_char));
    } else
	x2tic_textheight = 0;

    /* tics */
    if (!tic_in && ((x2tics & TICS_ON_BORDER) || ((xtics & TICS_MIRROR) && (xtics & TICS_ON_BORDER))))
	x2tic_height = (int) ((t->v_tic) * ticscale);
    else
	x2tic_height = 0;

    /* timestamp */
    if (*timelabel.text && !timelabel_bottom)
	timetop_textheight = (int) ((timelin + timelabel.yoffset + 2) * (t->v_char));
    else
	timetop_textheight = 0;

    /* horizontal ylabel */
    if (*ylabel.text && !can_rotate)
	ylabel_textheight = (int) ((ylablin + ylabel.yoffset) * (t->v_char));
    else
	ylabel_textheight = 0;

    /* horizontal y2label */
    if (*y2label.text && !can_rotate)
	y2label_textheight = (int) ((y2lablin + y2label.yoffset) * (t->v_char));
    else
	y2label_textheight = 0;

    /* compute ytop from the various components
     *     unless tmargin is explicitly specified  */

    ytop = (int) ((ysize + yoffset) * (t->ymax));

    if (tmargin < 0) {
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
	    top_margin += (int) (t->v_char);

	ytop -= top_margin;
	if (ytop == (int) (0.5 + (ysize + yoffset) * (t->ymax))) {
	    /* make room for the end of rotated ytics or y2tics */
	    ytop -= (int) ((t->h_char) * 2);
	}
    } else
	ytop -= (int) ((t->v_char) * tmargin);

    /*  end of preliminary ytop calculation }}} */


    /*{{{  tentative xleft, needed for TUNDER */
    if (lmargin >= 0)
	xleft = (int) (xoffset * (t->xmax) + (t->h_char) * lmargin);
    else
	xleft = (int) (xoffset * (t->xmax) + (t->h_char) * 2);
    /*}}} */


    /*{{{  tentative xright, needed for TUNDER */
    if (rmargin >= 0)
	xright = (int) ((xsize + xoffset) * (t->xmax) - (t->h_char) * rmargin);
    else
	xright = (int) ((xsize + xoffset) * (t->xmax) - (t->h_char) * 2);
    /*}}} */


    /*{{{  preliminary ybot calculation
     *     first compute heights of labels and tics */

    /* tic labels */
    if (xtics & TICS_ON_BORDER) {
	/* ought to consider tics on axes if axis near border */
	if (vertical_xtics) {
	    /* guess at tic length, since we don't know it yet */
	    xtic_textheight = (int) ((t->h_char) * 5);
	} else
	    xtic_textheight = (int) ((t->v_char) * (xticlin + 1));
    } else
	xtic_textheight = 0;

    /* tics */
    if (!tic_in && ((xtics & TICS_ON_BORDER) || ((x2tics & TICS_MIRROR) && (x2tics & TICS_ON_BORDER))))
	xtic_height = (int) ((t->v_tic) * ticscale);
    else
	xtic_height = 0;

    /* xlabel */
    if (xlablin) {
	/* offset is subtracted because if > 0, the margin is smaller */
	xlabel_textheight = (int) ((xlablin - xlabel.yoffset) * (t->v_char));
	if (!xtics)
	    xlabel_textheight += 0.5 * t->v_char;
    } else
	xlabel_textheight = 0;

    /* timestamp */
    if (*timelabel.text && timelabel_bottom) {
	/* && !vertical_timelabel)
	 * DBT 11-18-98 resize plot for vertical timelabels too !
	 */
	/* offset is subtracted because if . 0, the margin is smaller */
	timebot_textheight = (int) ((timelin - timelabel.yoffset) * (t->v_char));
    } else
	timebot_textheight = 0;

    /* compute ybot from the various components
     *     unless bmargin is explicitly specified  */

    ybot = (int) ((t->ymax) * yoffset);

    if (bmargin < 0) {
	ybot += xtic_height + xtic_textheight;
	if (xlabel_textheight > 0)
	    ybot += xlabel_textheight;
	if (timebot_textheight > 0)
	    ybot += timebot_textheight;
	/* HBB 19990616: round to nearest integer, required to escape
	 * floating point inaccuracies */
	if (ybot == (int) (0.5 + (t->ymax) * yoffset)) {
	    /* make room for the end of rotated ytics or y2tics */
	    ybot += (int) ((t->h_char) * 2);
	}
    } else
	ybot += (int) (bmargin * (t->v_char));

    /*  end of preliminary ybot calculation }}} */


#define KEY_PANIC(x) if (x) { lkey = 0; goto key_escape; }

    if (lkey) {
	/*{{{  essential key features */
	p_width = pointsize * t->h_tic;
	p_height = pointsize * t->v_tic;

	if (key_swidth >= 0)
	    key_sample_width = key_swidth * (t->h_char) + p_width;
	else
	    key_sample_width = 0;

	key_entry_height = p_height * 1.25 * key_vert_factor;
	if (key_entry_height < (t->v_char))
	    key_entry_height = (t->v_char) * key_vert_factor;

	/* count max_len key and number keys with len > 0 */
	max_ptitl_len = find_maxl_keys(plots, count, &ptitl_cnt);
	if ((ytlen = label_width(key_title, &ktitl_lines)) > max_ptitl_len)
	    max_ptitl_len = ytlen;

	if (key_reverse) {
	    key_sample_left = -key_sample_width;
	    key_sample_right = 0;
	    /* if key width is being used, adjust right-justified text */
	    key_text_left = t->h_char;
	    key_text_right = (t->h_char) * (max_ptitl_len + 1 + key_width_fix);
	    key_size_left = t->h_char - key_sample_left;	/* sample left is -ve */
	    key_size_right = key_text_right;
	} else {
	    key_sample_left = 0;
	    key_sample_right = key_sample_width;
	    /* if key width is being used, adjust left-justified text */
	    key_text_left = -(int) ((t->h_char) * (max_ptitl_len + 1 + key_width_fix));
	    key_text_right = -(int) (t->h_char);
	    key_size_left = -key_text_left;
	    key_size_right = key_sample_right + t->h_char;
	}
	key_point_offset = (key_sample_left + key_sample_right) / 2;

	/* advance width for cols */
	key_col_wth = key_size_left + key_size_right;

	key_rows = ptitl_cnt;
	key_cols = 1;

	/* calculate rows and cols for key - if something goes wrong,
	 * the tidiest way out is to  set lkey = 0, and a goto
	 */

	if (lkey == -1) {
	    if (key_vpos == TUNDER) {
		/* maximise no cols, limited by label-length */
		key_cols = (int) (xright - xleft) / key_col_wth;
		KEY_PANIC(key_cols == 0);
		key_rows = (int) (ptitl_cnt + key_cols - 1) / key_cols;
		KEY_PANIC(key_rows == 0);
		/* now calculate actual no cols depending on no rows */
		key_cols = (int) (ptitl_cnt + key_rows - 1) / key_rows;
		KEY_PANIC(key_cols == 0);
		key_col_wth = (int) (xright - xleft) / key_cols;
		/* we divide into columns, then centre in column by considering
		 * ratio of * key_left_size to key_right_size
		 *
		 * key_size_left/(key_size_left+key_size_right) * (xright-xleft)/key_cols
		 * do one integer division to maximise accuracy (hope we
		 * don't overflow !)
		 */
		keybox.xl =
		    xleft - key_size_left + ((xright - xleft) * key_size_left) / (key_cols * (key_size_left + key_size_right));
		keybox.xr = keybox.xl + key_col_wth * (key_cols - 1) + key_size_left + key_size_right;
		keybox.yb = t->ymax * yoffset;
		keybox.yt = keybox.yb + key_rows * key_entry_height + ktitl_lines * t->v_char;
		ybot += key_entry_height * key_rows + (int) ((t->v_char) * (ktitl_lines + 1));
	    } else {
		/* maximise no rows, limited by ytop-ybot */
		int i = (int) (ytop - ybot - (ktitl_lines + 1) * (t->v_char)) / key_entry_height;
		KEY_PANIC(i == 0);
		if (ptitl_cnt > i) {
		    key_cols = (int) (ptitl_cnt + i - 1) / i;
		    /* now calculate actual no rows depending on no cols */
		    KEY_PANIC(key_cols == 0);
		    key_rows = (int) (ptitl_cnt + key_cols - 1) / key_cols;
		}
	    }
	    /* come here if we detect a division by zero in key calculations */
	  key_escape:
	    ;			/* ansi requires this */
	}
	/*}}} */
    }
    /*{{{  set up y and y2 tics */
    {
	/* setup_tics allows max number of tics to be specified
	 * but users dont like it to change with size and font,
	 * so we use value of 20, which is 3.5 behaviour.
	 * Note also that if format is '', yticlin = 0, so this gives
	 * division by zero. 
	 * int guide = (ytop-ybot)/term->v_char;
	 */
	if (ytics)
	    setup_tics(FIRST_Y_AXIS, &yticdef, yformat, 20 /*(int) (guide/yticlin) */ );
	if (y2tics)
	    setup_tics(SECOND_Y_AXIS, &y2ticdef, y2format, 20 /*(int) (guide/y2ticlin) */ );
    }
    /*}}} */


    /*{{{  recompute xleft based on widths of ytics, ylabel etc
       unless it has been explicitly set by lmargin */

    /* tic labels */
    if (ytics & TICS_ON_BORDER) {
	if (vertical_ytics)
	    /* HBB: we will later add some white space as part of this, so
	     * reserve two more rows (one above, one below the text ...).
	     * Same will be done to similar calc.'s elsewhere */
	    ytic_textwidth = (int) ((t->v_char) * (yticlin + 2));
	else {
	    widest_tic = 0;	/* reset the global variable ... */
	    /* get gen_tics to call widest2d_callback with all labels
	     * the latter sets widest_tic to the length of the widest one
	     * ought to consider tics on axis if axis near border...
	     */
	    gen_tics(FIRST_Y_AXIS, &yticdef, 0, 0, 0.0, widest2d_callback);

	    ytic_textwidth = (int) ((t->h_char) * (widest_tic + 2));
	}
    } else {
	ytic_textwidth = 0;
    }

    /* tics */
    if (!tic_in && ((ytics & TICS_ON_BORDER) || ((y2tics & TICS_MIRROR) && (y2tics & TICS_ON_BORDER))))
	ytic_width = (int) ((t->h_tic) * ticscale);
    else
	ytic_width = 0;

    /* ylabel */
    if (*ylabel.text && can_rotate) {
	ylabel_textwidth = (int) ((ylablin - ylabel.xoffset) * (t->v_char));
	if (!ytics)
	    ylabel_textwidth += 0.5 * t->v_char;
    }
    /* this should get large for NEGATIVE ylabel.xoffsets  DBT 11-5-98 */
    else
	ylabel_textwidth = 0;

    /* timestamp */
    if (*timelabel.text && vertical_timelabel)
	timelabel_textwidth = (int) ((timelin - timelabel.xoffset + 1.5) * (t->v_char));
    else
	timelabel_textwidth = 0;

    /* compute xleft from the various components
     *     unless lmargin is explicitly specified  */

    xleft = (int) ((t->xmax) * xoffset);

    if (lmargin < 0) {
	xleft += (timelabel_textwidth > ylabel_textwidth ? timelabel_textwidth : ylabel_textwidth)
	    + ytic_width + ytic_textwidth;

	/* make sure xleft is wide enough for a negatively
	 * x-offset horizontal timestamp
	 */
	if (!vertical_timelabel && xleft - ytic_width - ytic_textwidth < -(int) (timelabel.xoffset * (t->h_char)))
	    xleft = ytic_width + ytic_textwidth - (int) (timelabel.xoffset * (t->h_char));
	if (xleft == (int) (0.5 + (t->xmax) * xoffset)) {
	    /* make room for end of xtic or x2tic label */
	    xleft += (int) ((t->h_char) * 2);
	}
	/* DBT 12-3-98  extra margin just in case */
	xleft += 0.5 * t->v_char;
    } else
	xleft += (int) (lmargin * (t->h_char));

    /*  end of xleft calculation }}} */


    /*{{{  recompute xright based on widest y2tic. y2labels, key TOUT
       unless it has been explicitly set by rmargin */

    /* tic labels */
    if (y2tics & TICS_ON_BORDER) {
	if (vertical_y2tics)
	    y2tic_textwidth = (int) ((t->v_char) * (y2ticlin + 2));
	else {
	    widest_tic = 0;	/* reset the global variable ... */
	    /* get gen_tics to call widest2d_callback with all labels
	     * the latter sets widest_tic to the length of the widest one
	     * ought to consider tics on axis if axis near border...
	     */
	    gen_tics(SECOND_Y_AXIS, &y2ticdef, 0, 0, 0.0, widest2d_callback);

	    y2tic_textwidth = (int) ((t->h_char) * (widest_tic + 2));
	}
    } else {
	y2tic_textwidth = 0;
    }

    /* tics */
    if (!tic_in && ((y2tics & TICS_ON_BORDER) || ((ytics & TICS_MIRROR) && (ytics & TICS_ON_BORDER))))
	y2tic_width = (int) ((t->h_tic) * ticscale);
    else
	y2tic_width = 0;

    /* y2label */
    if (can_rotate && *y2label.text) {
	y2label_textwidth = (int) ((y2lablin + y2label.xoffset) * (t->v_char));
	if (!y2tics)
	    y2label_textwidth += 0.5 * t->v_char;
    } else
	y2label_textwidth = 0;

    /* compute xright from the various components
     *     unless rmargin is explicitly specified  */

    xright = (int) ((t->xmax) * (xsize + xoffset));

    if (rmargin < 0) {
	/* xright -= y2label_textwidth + y2tic_width + y2tic_textwidth; */
	xright -= y2tic_width + y2tic_textwidth;
	if (y2label_textwidth > 0)
	    xright -= y2label_textwidth;

	/* adjust for outside key */
	if (lkey == -1 && key_hpos == TOUT) {
	    xright -= key_col_wth * key_cols;
	    keybox.xl = xright + (int) (t->h_tic);
	}
	if (xright == (int) (0.5 + (t->xmax) * (xsize + xoffset))) {
	    /* make room for end of xtic or x2tic label */
	    xright -= (int) ((t->h_char) * 2);
	}
	xright -= 0.5 * t->v_char;	/* DBT 12-3-98  extra margin just in case */

    } else
	xright -= (int) (rmargin * (t->h_char));

    /*  end of xright calculation }}} */


    if (aspect_ratio != 0.0) {
	double current_aspect_ratio;

	if (aspect_ratio < 0
	    && (max_array[x_axis] - min_array[x_axis]) != 0.0
	    ) {
	    current_aspect_ratio = - aspect_ratio
		* fabs((max_array[y_axis] - min_array[y_axis]) /
		       (max_array[x_axis] - min_array[x_axis]));
	} else
	    current_aspect_ratio = aspect_ratio;

	/*{{{  set aspect ratio if valid and sensible */
	if (current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
	    double current = ((double) (ytop - ybot)) / (xright - xleft);
	    double required = (current_aspect_ratio * t->v_tic) / t->h_tic;

	    if (current > required) {
		/* too tall */
		ytop = ybot + required * (xright - xleft);
	    } else {
		xright = xleft + (ytop - ybot) / required;
	    }
	}
	/*}}} */
    }

    /*{{{  set up x and x2 tics */
    /* we should base the guide on the width of the xtics, but we cannot
     * use widest_tics until tics are set up. Bit of a downer - let us
     * assume tics are 5 characters wide
     */

    {
	/* see equivalent code for ytics above
	 * int guide = (xright - xleft) / (5*t->h_char);
	 */

	if (xtics)
	    setup_tics(FIRST_X_AXIS, &xticdef, xformat, 20 /*guide */ );
	if (x2tics)
	    setup_tics(SECOND_X_AXIS, &x2ticdef, x2format, 20 /*guide */ );
    }
    /*}}} */


    /*  adjust top and bottom margins for tic label rotation */

    if (tmargin < 0 && x2tics & TICS_ON_BORDER && vertical_x2tics) {
	widest_tic = 0;		/* reset the global variable ... */
	gen_tics(SECOND_X_AXIS, &x2ticdef, 0, 0, 0.0, widest2d_callback);
	ytop += x2tic_textheight;
	/* Now compute a new one and use that instead: */
	x2tic_textheight = (int) ((t->h_char) * (widest_tic));
	ytop -= x2tic_textheight;
    }
    if (bmargin < 0 && xtics & TICS_ON_BORDER && vertical_xtics) {
	widest_tic = 0;		/* reset the global variable ... */
	gen_tics(FIRST_X_AXIS, &xticdef, 0, 0, 0.0, widest2d_callback);
	ybot -= xtic_textheight;
	xtic_textheight = (int) ((t->h_char) * widest_tic);
	ybot += xtic_textheight;
    }
    /*  compute coordinates for axis labels, title et al
     *     (some of these may not be used) */

    x2label_y = ytop + x2tic_height + x2tic_textheight + x2label_textheight;
    if (x2tic_textheight && (title_textheight || x2label_textheight))
	x2label_y += t->v_char;

    title_y = x2label_y + title_textheight;

    ylabel_y = ytop + x2tic_height + x2tic_textheight + ylabel_textheight;

    y2label_y = ytop + x2tic_height + x2tic_textheight + y2label_textheight;

    xlabel_y = ybot - xtic_height - xtic_textheight - xlabel_textheight + xlablin * t->v_char;
    ylabel_x = xleft - ytic_width - ytic_textwidth;
    if (*ylabel.text && can_rotate)
	ylabel_x -= ylabel_textwidth;

    y2label_x = xright + y2tic_width + y2tic_textwidth;
    if (*y2label.text && can_rotate)
	y2label_x += y2label_textwidth - y2lablin * t->v_char;

    if (vertical_timelabel) {
	if (timelabel_bottom)
	    time_y = xlabel_y - timebot_textheight + xlabel_textheight;
	else {
	    time_y = title_y + timetop_textheight - title_textheight - x2label_textheight;
	}
    } else {
	if (timelabel_bottom)
	    time_y = ybot - xtic_height - xtic_textheight - xlabel_textheight - timebot_textheight + t->v_char;
	else if (ylabel_textheight > 0)
	    time_y = ylabel_y + timetop_textheight;
	else
	    time_y = ytop + x2tic_height + x2tic_textheight + timetop_textheight + (int) (t->h_char);
    }
    if (vertical_timelabel)
	time_x = xleft - ytic_width - ytic_textwidth - timelabel_textwidth;
    else
	time_x = xleft - ytic_width - ytic_textwidth + (int) (timelabel.xoffset * (t->h_char));

    xtic_y = ybot - xtic_height - (vertical_xtics ? (int) (t->h_char) : (int) (t->v_char));

    x2tic_y = ytop + x2tic_height + (vertical_x2tics ? (int) (t->h_char) : x2tic_textheight);

    ytic_x = xleft - ytic_width - (vertical_ytics ? (ytic_textwidth - (int) t->v_char) : (int) (t->h_char));

    y2tic_x = xright + y2tic_width + (vertical_y2tics ? (int) (t->v_char) : (int) (t->h_char));

    /* restore text to horizontal [we tested rotation above] */
    (void) (*t->text_angle) (0);

    /* needed for map_position() below */
    AXIS_SETSCALE(FIRST_Y_AXIS, ybot, ytop);
    AXIS_SETSCALE(SECOND_Y_AXIS, ybot, ytop);
    AXIS_SETSCALE(FIRST_X_AXIS, xleft, xright);
    AXIS_SETSCALE(SECOND_X_AXIS, xleft, xright);

    /*{{{  calculate the window in the grid for the key */
    if (lkey == 1 || (lkey == -1 && key_vpos != TUNDER)) {
	/* calculate space for keys to prevent grid overwrite the keys */
	/* do it even if there is no grid, as do_plot will use these to position key */
	key_w = key_col_wth * key_cols;
	key_h = (ktitl_lines) * t->v_char + key_rows * key_entry_height;
	if (lkey == -1) {
	    if (key_vpos == TTOP) {
		keybox.yt = (int) ytop - (t->v_tic);
		keybox.yb = keybox.yt - key_h;
	    } else {
		keybox.yb = ybot + (t->v_tic);
		keybox.yt = keybox.yb + key_h;
	    }
	    if (key_hpos == TLEFT) {
		keybox.xl = xleft + (t->h_char);	/* for Left just */
		keybox.xr = keybox.xl + key_w;
	    } else if (key_hpos == TRIGHT) {
		keybox.xr = xright - (t->h_char);	/* for Right just */
		keybox.xl = keybox.xr - key_w;
	    } else {		/* TOUT */
		/* do this here for do_plot() */
		/* align right first since rmargin may be manual */
		keybox.xr = (xsize + xoffset) * (t->xmax) - (t->h_char);
		keybox.xl = keybox.xr - key_w;
	    }
	} else {
	    unsigned int x, y;
	    map_position(&key_user_pos, &x, &y, "key");
	    keybox.xl = x - key_size_left;
	    keybox.xr = keybox.xl + key_w;
	    keybox.yt = y + (ktitl_lines ? t->v_char : key_entry_height) / 2;
	    keybox.yb = keybox.yt - key_h;
	}
    }
    /*}}} */

}

/*}}} */

void
get_offsets(struct text_label *this_label, struct termentry *t, int *htic, int *vtic)
{
    if (-1 != this_label->pointstyle) {
	*htic = (pointsize * t->h_tic * 0.5 * this_label->hoffset);
	*vtic = (pointsize * t->v_tic * 0.5 * this_label->voffset);
    } else {
	*htic = 0;
	*vtic = 0;
    }
}

void
do_plot(plots, pcount)
struct curve_points *plots;
int pcount;			/* count of plots in linked list */
{
    register struct termentry *t = term;
    register int curve;
    register struct curve_points *this_plot = NULL;
    register int xl = 0, yl = 0;	/* avoid gcc -Wall warning */
    register int key_count = 0;
    /* only a Pyramid would have this many registers! */
    struct text_label *this_label;
    struct arrow_def *this_arrow;
    TBOOLEAN scaling;
    char *s, *e;

    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;

/*      Apply the desired viewport offsets. */
    if (min_array[y_axis] < max_array[y_axis]) {
	min_array[y_axis] -= boff;
	max_array[y_axis] += toff;
    } else {
	max_array[y_axis] -= boff;
	min_array[y_axis] += toff;
    }
    if (min_array[x_axis] < max_array[x_axis]) {
	min_array[x_axis] -= loff;
	max_array[x_axis] += roff;
    } else {
	max_array[x_axis] -= loff;
	min_array[x_axis] += roff;
    }

    /*
     * In the beginning, this "empty range" test was for exact
     * equality, eg y_min == y_max , but that caused an infinite loop
     * once.  Then the test was changed to check for being within the
     * 'zero' threshold, fabs(y_max - y_min) < zero) , but that
     * prevented plotting data with ranges below 'zero'.  Now it's an
     * absolute equality test again, since
     * axis_checked_extend_empty_range() should have widened empty
     * ranges before we get here.  */
    if (min_array[x_axis] == max_array[x_axis])
	int_error(NO_CARET, "x_min should not equal x_max!");
    if (min_array[y_axis] == max_array[y_axis])
	int_error(NO_CARET, "y_min should not equal y_max!");

    term_init();		/* may set xmax/ymax */

    /* compute boundary for plot (xleft, xright, ytop, ybot)
     * also calculates tics, since xtics depend on xleft
     * but xleft depends on ytics. Boundary calculations
     * depend on term->v_char etc, so terminal must be
     * initialised.
     */

    scaling = (*t->scale) (xsize, ysize);

    boundary(scaling, plots, pcount);

    /* inform axes about newly found values 'xleft' & Co. */
    axis_set_graphical_range(FIRST_X_AXIS, xleft, xright);
    axis_set_graphical_range(FIRST_Y_AXIS, ybot, ytop);
    axis_set_graphical_range(SECOND_X_AXIS, xleft, xright);
    axis_set_graphical_range(SECOND_Y_AXIS, ybot, ytop);

    screen_ok = FALSE;

    term_start_plot();

/* DRAW TICS AND GRID */
    term_apply_lp_properties(&border_lp);	/* border linetype */
    largest_polar_circle = 0;

    /* select first mapping */
    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;

    /* label first y axis tics */
    axis_output_tics(FIRST_Y_AXIS, ytics, rotate_ytics, xleft, xright,
		     &ytic_x, FIRST_X_AXIS, &yticdef, (GRID_Y | GRID_MY),
		     mytics, mytfreq, ytick2d_callback);
    /* label first x axis tics */
    axis_output_tics(FIRST_X_AXIS, xtics, rotate_xtics, ybot, ytop,
		     &xtic_y, FIRST_Y_AXIS, &xticdef, (GRID_X | GRID_MX),
		     mxtics, mxtfreq, xtick2d_callback);
#if 0
    if (ytics) {
	/* set the globals ytick2d_callback() needs */

	if (rotate_ytics && (*t->text_angle) (1)) {
	    tic_hjust = CENTRE;
	    tic_vjust = JUST_BOT;
	    rotate_tics = 1;	/* HBB 980629 */
	    ytic_x += t->v_char / 2;
	} else {
	    tic_hjust = RIGHT;
	    tic_vjust = JUST_CENTRE;
	    rotate_tics = 0;	/* HBB 980629 */
	}

	if (ytics & TICS_MIRROR)
	    tic_mirror = xright;
	else
	    tic_mirror = -1;	/* no thank you */

	if ((ytics & TICS_ON_AXIS) && !log_array[FIRST_X_AXIS] && inrange(0.0, min_array[x_axis], max_array[x_axis])) {
	    tic_start = map_x(0.0);
	    tic_direction = -1;
	    if (ytics & TICS_MIRROR)
		tic_mirror = tic_start;
	    /* put text at boundary if axis is close to boundary */
	    tic_text = (((tic_start - xleft) > (3 * t->h_char)) ? tic_start : xleft) - t->h_char;
	} else {
	    tic_start = xleft;
	    tic_direction = tic_in ? 1 : -1;
	    tic_text = ytic_x;
	}
	/* go for it */
	gen_tics(FIRST_Y_AXIS, &yticdef, work_grid.l_type & (GRID_Y | GRID_MY), mytics, mytfreq, ytick2d_callback);
	(*t->text_angle) (0);	/* reset rotation angle */

    }
    if (xtics) {
	/* set the globals xtick2d_callback() needs */

	if (rotate_xtics && (*t->text_angle) (1)) {
	    tic_hjust = RIGHT;
	    tic_vjust = JUST_CENTRE;
	    rotate_tics = 1;	/* HBB 980629 */
	} else {
	    tic_hjust = CENTRE;
	    tic_vjust = JUST_TOP;
	    rotate_tics = 0;	/* HBB 980629 */
	}

	if (xtics & TICS_MIRROR)
	    tic_mirror = ytop;
	else
	    tic_mirror = -1;	/* no thank you */
	if ((xtics & TICS_ON_AXIS) && !log_array[FIRST_Y_AXIS] && inrange(0.0, min_array[y_axis], max_array[y_axis])) {
	    tic_start = map_y(0.0);
	    tic_direction = -1;
	    if (xtics & TICS_MIRROR)
		tic_mirror = tic_start;
	    /* put text at boundary if axis is close to boundary */
	    if (tic_start - ybot > 2 * t->v_char)
		tic_text = tic_start - ticscale * t->v_tic - t->v_char;
	    else
		tic_text = ybot - t->v_char;
	} else {
	    tic_start = ybot;
	    tic_direction = tic_in ? 1 : -1;
	    tic_text = xtic_y;
	}
	/* go for it */
	gen_tics(FIRST_X_AXIS, &xticdef, work_grid.l_type & (GRID_X | GRID_MX), mxtics, mxtfreq, xtick2d_callback);
	(*t->text_angle) (0);	/* reset rotation angle */
    }
#endif /* HBB 20000416: test, test */

    /* select second mapping */
    x_axis = SECOND_X_AXIS;
    y_axis = SECOND_Y_AXIS;

    axis_output_tics(SECOND_Y_AXIS, y2tics, rotate_y2tics, xright, xleft,
		     &y2tic_x, SECOND_X_AXIS, &y2ticdef, (GRID_Y2 | GRID_MY2),
		     my2tics, my2tfreq, ytick2d_callback);
    axis_output_tics(SECOND_X_AXIS, x2tics, rotate_x2tics, ytop, ybot,
		     &x2tic_y, SECOND_Y_AXIS, &x2ticdef, (GRID_X2 | GRID_MX2),
		     mx2tics, mx2tfreq, xtick2d_callback);

#if 0 /* HBB 20000501: use generalized routine for these, too: */
    /* label second y axis tics */
    if (y2tics) {
	/* set the globals ytick2d_callback() needs */

	if (rotate_y2tics && (*t->text_angle) (1)) {
	    tic_hjust = CENTRE;
	    tic_vjust = JUST_TOP;
	    rotate_tics = 1;	/* HBB 980629 */
	} else {
	    tic_hjust = LEFT;
	    tic_vjust = JUST_CENTRE;
	    rotate_tics = 0;	/* HBB 980629 */
	}

	if (y2tics & TICS_MIRROR)
	    tic_mirror = xleft;
	else
	    tic_mirror = -1;	/* no thank you */
	if ((y2tics & TICS_ON_AXIS) && !log_array[FIRST_X_AXIS] && inrange(0.0, min_array[x_axis], max_array[x_axis])) {
	    tic_start = map_x(0.0);
	    tic_direction = 1;
	    if (y2tics & TICS_MIRROR)
		tic_mirror = tic_start;
	    /* put text at boundary if axis is close to boundary */
	    tic_text = (((xright - tic_start) > (3 * t->h_char)) ? tic_start : xright) + t->h_char;
	} else {
	    tic_start = xright;
	    tic_direction = tic_in ? -1 : 1;
	    tic_text = y2tic_x;
	}
	/* go for it */
	gen_tics(SECOND_Y_AXIS, &y2ticdef, work_grid.l_type & (GRID_Y2 | GRID_MY2), my2tics, my2tfreq, ytick2d_callback);
	(*t->text_angle) (0);	/* reset rotation angle */
    }
    /* label second x axis tics */
    if (x2tics) {
	/* set the globals xtick2d_callback() needs */

	if (rotate_x2tics && (*t->text_angle) (1)) {
	    tic_hjust = LEFT;
	    tic_vjust = JUST_CENTRE;
	    rotate_tics = 1;	/* HBB 980629 */
	} else {
	    tic_hjust = CENTRE;
	    tic_vjust = JUST_BOT;
	    rotate_tics = 0;	/* HBB 980629 */
	}

	if (x2tics & TICS_MIRROR)
	    tic_mirror = ybot;
	else
	    tic_mirror = -1;	/* no thank you */
	if ((x2tics & TICS_ON_AXIS) && !log_array[SECOND_Y_AXIS] && inrange(0.0, min_array[y_axis], max_array[y_axis])) {
	    tic_start = map_y(0.0);
	    tic_direction = 1;
	    if (x2tics & TICS_MIRROR)
		tic_mirror = tic_start;
	    /* put text at boundary if axis is close to boundary */
	    tic_text = (((ytop - tic_start) > (2 * t->v_char)) ? tic_start : ytop) + t->v_char;
	} else {
	    tic_start = ytop;
	    tic_direction = tic_in ? -1 : 1;
	    tic_text = x2tic_y;
	}
	/* go for it */
	gen_tics(SECOND_X_AXIS, &x2ticdef, work_grid.l_type & (GRID_X2 | GRID_MX2), mx2tics, mx2tfreq, xtick2d_callback);
	(*t->text_angle) (0);	/* reset rotation angle */
    }
#endif /* 1/0 HBB 20000501 */

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

/* DRAW AXES */

    /* after grid so that axes linetypes are on top */

    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;	/* chose scaling */

    /* FIXME HBB 20000430: previous version treated log-scaled
     * y and x axes slightly differently, here: xzeroaxis was set at
     * ybot, but yzeroaxis would still try to map x=0.0... */
    if (axis_position_zeroaxis(FIRST_Y_AXIS)
	&& (xzeroaxis.l_type > -3)) {
	term_apply_lp_properties(&xzeroaxis);
	(*t->move) (xleft, axis_zero[FIRST_Y_AXIS]);
	(*t->vector) (xright, axis_zero[FIRST_Y_AXIS]);
    }
    if (axis_position_zeroaxis(FIRST_X_AXIS)
	&& (yzeroaxis.l_type > -3)) {
	term_apply_lp_properties(&yzeroaxis);
	(*t->move) (axis_zero[FIRST_X_AXIS], ybot);
	(*t->vector) (axis_zero[FIRST_X_AXIS], ytop);
    }

    x_axis = SECOND_X_AXIS;
    y_axis = SECOND_Y_AXIS;	/* chose scaling */

    if (axis_position_zeroaxis(SECOND_Y_AXIS)
	&& (x2zeroaxis.l_type > -3)) {
	term_apply_lp_properties(&x2zeroaxis);
	(*t->move) (xleft, axis_zero[SECOND_Y_AXIS]);
	(*t->vector) (xright, axis_zero[SECOND_Y_AXIS]);
    }
    if (axis_position_zeroaxis(SECOND_X_AXIS)
	&& (y2zeroaxis.l_type > -3)) {
	term_apply_lp_properties(&y2zeroaxis);
	(*t->move) (axis_zero[SECOND_X_AXIS], ybot);
	(*t->vector) (axis_zero[SECOND_X_AXIS], ytop);
    }


    /* DRAW PLOT BORDER */
    if (draw_border) {
	/* HBB 980609: just in case: move over to border linestyle only
	 * if border is to be drawn */
	term_apply_lp_properties(&border_lp);	/* border linetype */
	(*t->move) (xleft, ybot);
	if (border_south) {
	    (*t->vector) (xright, ybot);
	} else {
	    (*t->move) (xright, ybot);
	}
	if (border_east) {
	    (*t->vector) (xright, ytop);
	} else {
	    (*t->move) (xright, ytop);
	}
	if (border_north) {
	    (*t->vector) (xleft, ytop);
	} else {
	    (*t->move) (xleft, ytop);
	}
	if (border_west) {
	    (*t->vector) (xleft, ybot);
	} else {
	    (*t->move) (xleft, ybot);
	}
    }
/* YLABEL */
    if (*ylabel.text) {
	/* we worked out x-posn in boundary() */
	if ((*t->text_angle) (1)) {
	    unsigned int x = ylabel_x + (t->v_char / 2);
	    unsigned int y = (ytop + ybot) / 2 + ylabel.yoffset * (t->h_char);
	    write_multiline(x, y, ylabel.text, CENTRE, JUST_TOP, 1, ylabel.font);
	    (*t->text_angle) (0);
	} else {
	    /* really bottom just, but we know number of lines 
	       so we need to adjust x-posn by one line */
	    unsigned int x = ylabel_x;
	    unsigned int y = ylabel_y;
	    write_multiline(x, y, ylabel.text, LEFT, JUST_TOP, 0, ylabel.font);
	}
    }
/* Y2LABEL */
    if (*y2label.text) {
	/* we worked out coordinates in boundary() */
	if ((*t->text_angle) (1)) {
	    unsigned int x = y2label_x + (t->v_char / 2) - 1;
	    unsigned int y = (ytop + ybot) / 2 + y2label.yoffset * (t->h_char);
	    write_multiline(x, y, y2label.text, CENTRE, JUST_TOP, 1, y2label.font);
	    (*t->text_angle) (0);
	} else {
	    /* really bottom just, but we know number of lines */
	    unsigned int x = y2label_x;
	    unsigned int y = y2label_y;
	    write_multiline(x, y, y2label.text, RIGHT, JUST_TOP, 0, y2label.font);
	}
    }
/* XLABEL */
    if (*xlabel.text) {
	unsigned int x = (xright + xleft) / 2 + xlabel.xoffset * (t->h_char);
	unsigned int y = xlabel_y - t->v_char / 2;	/* HBB */
	write_multiline(x, y, xlabel.text, CENTRE, JUST_TOP, 0, xlabel.font);
    }
/* PLACE TITLE */
    if (*title.text) {
	/* we worked out y-coordinate in boundary() */
	unsigned int x = (xleft + xright) / 2 + title.xoffset * t->h_char;
	unsigned int y = title_y - t->v_char / 2;
	write_multiline(x, y, title.text, CENTRE, JUST_TOP, 0, title.font);
    }
/* X2LABEL */
    if (*x2label.text) {
	/* we worked out y-coordinate in boundary() */
	unsigned int x = (xright + xleft) / 2 + x2label.xoffset * (t->h_char);
	unsigned int y = x2label_y - t->v_char / 2 - 1;
	write_multiline(x, y, x2label.text, CENTRE, JUST_TOP, 0, x2label.font);
    }
/* PLACE TIMEDATE */
    if (*timelabel.text) {
	/* we worked out coordinates in boundary() */
	char *str;
	time_t now;
	unsigned int x = time_x;
	unsigned int y = time_y;
	time(&now);
	/* there is probably now way to find out in advance how many
	 * chars strftime() writes */
	str = gp_alloc(MAX_LINE_LEN + 1, "timelabel.text");
	strftime(str, MAX_LINE_LEN, timelabel.text, localtime(&now));

	if (timelabel_rotate && (*t->text_angle) (1)) {
	    x += t->v_char / 2;	/* HBB */
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_TOP, 1, timelabel.font);
	    else
		write_multiline(x, y, str, RIGHT, JUST_TOP, 1, timelabel.font);
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
/* PLACE LABELS */
    if ((*t->pointsize)) {
	(*t->pointsize) (pointsize);
    }
    for (this_label = first_label; this_label != NULL; this_label = this_label->next) {

	unsigned int x, y;
	int htic;
	int vtic;

	get_offsets(this_label, t, &htic, &vtic);

	if (this_label->layer)
	    continue;
	map_position(&this_label->place, &x, &y, "label");
	if (this_label->rotate && (*t->text_angle) (1)) {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, JUST_TOP, 1, this_label->font);
	    (*t->text_angle) (0);
	} else {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, JUST_TOP, 0, this_label->font);
	}
	if (-1 != this_label->pointstyle) {
	    (*t->point) (x, y, this_label->pointstyle);
	}
    }

/* PLACE ARROWS */
    for (this_arrow = first_arrow; this_arrow != NULL; this_arrow = this_arrow->next) {
	unsigned int sx, sy, ex, ey;

	if (this_arrow->layer)
	    continue;
	map_position(&this_arrow->start, &sx, &sy, "arrow");
	map_position(&this_arrow->end, &ex, &ey, "arrow");

	term_apply_lp_properties(&(this_arrow->lp_properties));
	(*t->arrow) (sx, sy, ex, ey, this_arrow->head);
    }

/* WORK OUT KEY SETTINGS AND DO KEY TITLE / BOX */

    if (lkey) {			/* may have been cancelled if something went wrong */
	/* just use keybox.xl etc worked out in boundary() */
	xl = keybox.xl + key_size_left;
	yl = keybox.yt;

	if (*key_title) {
	    char *ss = gp_alloc(strlen(key_title) + 2, "tmp string ss");
	    strcpy(ss, key_title);
	    strcat(ss, "\n");

	    s = ss;
	    yl -= t->v_char / 2;
	    while ((e = (char *) strchr(s, '\n')) != NULL) {
		*e = '\0';
		if (key_just == JLEFT) {
		    (*t->justify_text) (LEFT);
		    (*t->put_text) (xl + key_text_left, yl, s);
		} else {
		    if ((*t->justify_text) (RIGHT)) {
			(*t->put_text) (xl + key_text_right, yl, s);
		    } else {
			int x = xl + key_text_right - (t->h_char) * strlen(s);
			if (key_hpos == TOUT || key_vpos == TUNDER ||	/* HBB 990327 */
			    inrange(x, xleft, xright))
			    (*t->put_text) (x, yl, s);
		    }
		}
		s = ++e;
		yl -= t->v_char;
	    }
	    yl += t->v_char / 2;
	    free(ss);
	}
	yl_ref = yl -= key_entry_height / 2;	/* centralise the keys */
	key_count = 0;

	if (key_box.l_type > -3) {
	    term_apply_lp_properties(&key_box);
	    (*t->move) (keybox.xl, keybox.yb);
	    (*t->vector) (keybox.xl, keybox.yt);
	    (*t->vector) (keybox.xr, keybox.yt);
	    (*t->vector) (keybox.xr, keybox.yb);
	    (*t->vector) (keybox.xl, keybox.yb);
	    /* draw a horizontal line between key title and first entry */
	    (*t->move) (keybox.xl, keybox.yt - (ktitl_lines) * t->v_char);
	    (*t->vector) (keybox.xr, keybox.yt - (ktitl_lines) * t->v_char);
	}
    } /* lkey */

    /* DRAW CURVES */
    this_plot = plots;
    for (curve = 0; curve < pcount; this_plot = this_plot->next, curve++) {
	int localkey = lkey;	/* a local copy */

	/* set scaling for this plot's axes */
	x_axis = this_plot->x_axis;
	y_axis = this_plot->y_axis;

	term_apply_lp_properties(&(this_plot->lp_properties));

	if (this_plot->title && !*this_plot->title) {
	    localkey = 0;
	} else {
	    if (localkey != 0 && this_plot->title) {
		key_count++;
		if (key_just == JLEFT) {
		    (*t->justify_text) (LEFT);
		    (*t->put_text) (xl + key_text_left, yl, this_plot->title);
		} else {
		    if ((*t->justify_text) (RIGHT)) {
			(*t->put_text) (xl + key_text_right, yl, this_plot->title);
		    } else {
			int x = xl + key_text_right - (t->h_char) * strlen(this_plot->title);
			if (key_hpos == TOUT || key_vpos == TUNDER ||	/* HBB 990327 */
			    i_inrange(x, xleft, xright))
			    (*t->put_text) (x, yl, this_plot->title);
		    }
		}

		/* draw sample depending on bits set in plot_style */
		if ((this_plot->plot_style & 1) || ((this_plot->plot_style & 4) && this_plot->plot_type == DATA)) {
		    /* errors for data plots only */
		    (*t->move) (xl + key_sample_left, yl);
		    (*t->vector) (xl + key_sample_right, yl);
		}
		/* oops - doing the point sample now breaks postscript
		 * terminal for example, which changes current line style
		 * when drawing a point, but does not restore it.
		 * We simply draw the point sample after plotting
		 */

		if (this_plot->plot_type == DATA && (this_plot->plot_style & 4) && bar_size > 0.0) {
		    (*t->move) (xl + key_sample_left, yl + ERRORBARTIC);
		    (*t->vector) (xl + key_sample_left, yl - ERRORBARTIC);
		    (*t->move) (xl + key_sample_right, yl + ERRORBARTIC);
		    (*t->vector) (xl + key_sample_right, yl - ERRORBARTIC);
		}
	    }
	}

	/* and now the curves, plus any special key requirements */
	/* be sure to draw all lines before drawing any points */

	switch (this_plot->plot_style) {
	    /*{{{  IMPULSE */
	case IMPULSES:
	    plot_impulses(this_plot, axis_zero[x_axis], axis_zero[y_axis]);
	    break;
	    /*}}} */
	    /*{{{  LINES */
	case LINES:
	    plot_lines(this_plot);
	    break;
	    /*}}} */
	    /*{{{  STEPS */
	case STEPS:
	    plot_steps(this_plot);
	    break;
	    /*}}} */
	    /*{{{  FSTEPS */
	case FSTEPS:
	    plot_fsteps(this_plot);
	    break;
	    /*}}} */
	    /*{{{  HISTEPS */
	case HISTEPS:
	    plot_histeps(this_plot);
	    break;
	    /*}}} */
	    /*{{{  POINTSTYLE */
	case POINTSTYLE:
	    plot_points(this_plot);
	    break;
	    /*}}} */
	    /*{{{  LINESPOINTS */
	case LINESPOINTS:
	    plot_lines(this_plot);
	    plot_points(this_plot);
	    break;
	    /*}}} */
	    /*{{{  DOTS */
	case DOTS:
	    if (localkey != 0 && this_plot->title) {
		(*t->point) (xl + key_point_offset, yl, -1);
	    }
	    plot_dots(this_plot);
	    break;
	    /*}}} */

	    /*{{{  YERRORLINES */
	case YERRORLINES:{
		plot_lines(this_plot);
		plot_bars(this_plot);
		plot_points(this_plot);
		break;
	    }
	    /*}}} */
	    /*{{{  XERRORLINES */
	case XERRORLINES:{
		plot_lines(this_plot);
		plot_bars(this_plot);
		plot_points(this_plot);
	    }
	    /*}}} */
	    /*{{{  XYERRORLINES */
	case XYERRORLINES:
	    plot_lines(this_plot);
	    plot_bars(this_plot);
	    plot_points(this_plot);
	    break;
	    /*}}} */

	    /*{{{  YERRORBARS */
	case YERRORBARS:{
		plot_bars(this_plot);
		plot_points(this_plot);
		break;
	    }
	    /*}}} */
	    /*{{{  XERRORBARS */
	case XERRORBARS:{
		plot_bars(this_plot);
		plot_points(this_plot);
	    }
	    /*}}} */
	    /*{{{  XYERRORBARS */
	case XYERRORBARS:
	    plot_bars(this_plot);
	    plot_points(this_plot);
	    break;

	    /*}}} */
	    /*{{{  BOXXYERROR */
	case BOXXYERROR:
	    plot_boxes(this_plot, axis_zero[y_axis]);
	    break;
	    /*}}} */
	    /*{{{  BOXERROR (falls through to) */
	case BOXERROR:
	    plot_bars(this_plot);
	    /* no break */

	    /*}}} */
	    /*{{{  BOXES */
	case BOXES:
	    plot_boxes(this_plot, axis_zero[y_axis]);
	    break;
	    /*}}} */
	    /*{{{  VECTOR */
	case VECTOR:
	    plot_vectors(this_plot);
	    break;

	    /*}}} */
	    /*{{{  FINANCEBARS */
	case FINANCEBARS:
	    plot_f_bars(this_plot);
	    break;
	    /*}}} */
	    /*{{{  CANDLESTICKS */
	case CANDLESTICKS:
	    plot_c_bars(this_plot);
	    break;
	    /*}}} */
	}


	if (localkey && this_plot->title) {
	    /* we deferred point sample until now */
	    if (this_plot->plot_style & 2)
		(*t->point) (xl + key_point_offset, yl, this_plot->lp_properties.p_type);

	    if (key_count >= key_rows) {
		yl = yl_ref;
		xl += key_col_wth;
		key_count = 0;
	    } else
		yl = yl - key_entry_height;
	}
    }

/* PLACE LABELS */
    if ((*t->pointsize)) {
	(*t->pointsize) (pointsize);
    }
    for (this_label = first_label; this_label != NULL; this_label = this_label->next) {

	unsigned int x, y;
	int htic;
	int vtic;

	get_offsets(this_label, t, &htic, &vtic);

	if (this_label->layer == 0)
	    continue;
	map_position(&this_label->place, &x, &y, "label");
	if (this_label->rotate && (*t->text_angle) (1)) {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, JUST_TOP, 1, this_label->font);
	    (*t->text_angle) (0);
	} else {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, JUST_TOP, 0, this_label->font);
	}
	if (-1 != this_label->pointstyle) {
	    (*t->point) (x, y, this_label->pointstyle);
	}
    }

/* PLACE ARROWS */
    for (this_arrow = first_arrow; this_arrow != NULL; this_arrow = this_arrow->next) {
	unsigned int sx, sy, ex, ey;

	if (this_arrow->layer == 0)
	    continue;
	map_position(&this_arrow->start, &sx, &sy, "arrow");
	map_position(&this_arrow->end, &ex, &ey, "arrow");

	term_apply_lp_properties(&(this_arrow->lp_properties));
	(*t->arrow) (sx, sy, ex, ey, this_arrow->head);
    }

    term_end_plot();
}


/* plot_impulses:
 * Plot the curves in IMPULSES style
 */

static void
plot_impulses(plot, yaxis_x, xaxis_y)
struct curve_points *plot;
int yaxis_x, xaxis_y;
{
    int i;
    int x, y;
    struct termentry *t = term;

    for (i = 0; i < plot->p_count; i++) {
	switch (plot->points[i].type) {
	case INRANGE:{
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);
		break;
	    }
	case OUTRANGE:{
		if (!inrange(plot->points[i].x, min_array[x_axis], max_array[x_axis]))
		    continue;
		x = map_x(plot->points[i].x);
		if ((min_array[y_axis] < max_array[y_axis] && plot->points[i].y < min_array[y_axis])
		    || (max_array[y_axis] < min_array[y_axis] && plot->points[i].y > min_array[y_axis]))
		    y = map_y(min_array[y_axis]);
		else
		    y = map_y(max_array[y_axis]);

		break;
	    }
	default:		/* just a safety */
	case UNDEFINED:{
		continue;
	    }
	}

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
plot_lines(plot)
struct curve_points *plot;
{
    int i;			/* point index */
    int x, y;			/* point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double ex, ey;		/* an edge point */
    double lx[2], ly[2];	/* two edge points */

    for (i = 0; i < plot->p_count; i++) {
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

/* XXX - JG  */
/* plot_steps:                          
 * Plot the curves in STEPS style
 */
static void
plot_steps(plot)
struct curve_points *plot;
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
plot_fsteps(plot)
struct curve_points *plot;
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

/* CAC  */
/* plot_histeps:                                
 * Plot the curves in HISTEPS style
 */
static void
plot_histeps(plot)
struct curve_points *plot;
{
    int i;			/* point index */
    int hold, bigi;		/* indices for sorting */
    int xl, yl;			/* cursor position in terminal coordinates */
    struct termentry *t = term;
    double x, y, xn, yn;	/* point position */
    int *gl, goodcount;		/* array to hold list of valid points */

    /* preliminary count of points inside array */
    goodcount = 0;
    for (i = 0; i < plot->p_count; i++)
	if (plot->points[i].type == INRANGE || plot->points[i].type == OUTRANGE)
	    ++goodcount;
    if (goodcount < 2)
	return;			/* cannot plot less than 2 points */

    gl = (int *) gp_alloc(goodcount * sizeof(int), "histeps valid point mapping");
    if (gl == NULL)
	return;

/* fill gl array with indexes of valid (non-undefined) points.  */
    goodcount = 0;
    for (i = 0; i < plot->p_count; i++)
	if (plot->points[i].type == INRANGE || plot->points[i].type == OUTRANGE) {
	    gl[goodcount] = i;
	    ++goodcount;
	}
/* sort the data */
    for (bigi = i = 1; i < goodcount;) {
	if (plot->points[gl[i]].x < plot->points[gl[i - 1]].x) {
	    hold = gl[i];
	    gl[i] = gl[i - 1];
	    gl[i - 1] = hold;
	    if (i > 1) {
		i--;
		continue;
	    }
	}
	i = ++bigi;
    }

    x = (3.0 * plot->points[gl[0]].x - plot->points[gl[1]].x) / 2.0;
    y = 0.0;

    xl = map_x(x);
    yl = map_y(y);
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
    histeps_vertical(&xl, &yl, xn, yn, 0.0);

    free(gl);
}

/* CAC 
 * Draw vertical line for the histeps routine.
 * Performs clipping.
 */
static void
histeps_vertical(xl, yl, x, y1, y2)
int *xl, *yl;			/* keeps track of "cursor" position */
double x, y1, y2;		/* coordinates of vertical line */
{
    struct termentry *t = term;
    int xm, y1m, y2m;

    if ((y1 < min_array[y_axis] && y2 < min_array[y_axis]) || (y1 > max_array[y_axis] && y2 > max_array[y_axis]) || x < min_array[x_axis] || x > max_array[x_axis])
	return;

    if (y1 < min_array[y_axis])
	y1 = min_array[y_axis];
    if (y1 > max_array[y_axis])
	y1 = max_array[y_axis];
    if (y2 < min_array[y_axis])
	y2 = min_array[y_axis];
    if (y2 > max_array[y_axis])
	y2 = max_array[y_axis];

    xm = map_x(x);
    y1m = map_y(y1);
    y2m = map_y(y2);

    if (y1m != *yl || xm != *xl)
	(*t->move) (xm, y1m);
    (*t->vector) (xm, y2m);
    *xl = xm;
    *yl = y2m;

    return;
}

/* CAC 
 * Draw horizontal line for the histeps routine.
 * Performs clipping.
 */
static void
histeps_horizontal(xl, yl, x1, x2, y)
int *xl, *yl;			/* keeps track of "cursor" position */
double x1, x2, y;		/* coordinates of vertical line */
{
    struct termentry *t = term;
    int x1m, x2m, ym;

    if ((x1 < min_array[x_axis] && x2 < min_array[x_axis]) ||
	(x1 > max_array[x_axis] && x2 > max_array[x_axis]) ||
	 y < min_array[y_axis] || y > max_array[y_axis])
	return;

    if (x1 < min_array[x_axis])
	x1 = min_array[x_axis];
    if (x1 > max_array[x_axis])
	x1 = max_array[x_axis];
    if (x2 < min_array[x_axis])
	x2 = min_array[x_axis];
    if (x2 > max_array[x_axis])
	x2 = max_array[x_axis];

    ym = map_y(y);
    x1m = map_x(x1);
    x2m = map_x(x2);

    if (x1m != *xl || ym != *yl)
	(*t->move) (x1m, ym);
    (*t->vector) (x2m, ym);
    *xl = x2m;
    *yl = ym;

    return;
}


/* plot_bars:
 * Plot the curves in ERRORBARS style
 *  we just plot the bars; the points are plotted in plot_points
 */
static void
plot_bars(plot)
struct curve_points *plot;
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

/* Limitation: no boxes with x errorbars */

    if ((plot->plot_style == YERRORBARS) ||
	(plot->plot_style == XYERRORBARS) ||
	(plot->plot_style == BOXERROR) || (plot->plot_style == YERRORLINES) || (plot->plot_style == XYERRORLINES)) {
/* Draw the vertical part of the bar */
	for (i = 0; i < plot->p_count; i++) {
	    /* undefined points don't count */
	    if (plot->points[i].type == UNDEFINED)
		continue;

	    /* check to see if in xrange */
	    x = plot->points[i].x;
	    if (!inrange(x, min_array[x_axis], max_array[x_axis]))
		continue;
	    xM = map_x(x);

	    /* check to see if in yrange */
	    y = plot->points[i].y;
	    if (!inrange(y, min_array[y_axis], max_array[y_axis]))
		continue;
	    yM = map_y(y);

	    /* find low and high points of bar, and check yrange */
	    yhigh = plot->points[i].yhigh;
	    ylow = plot->points[i].ylow;

	    high_inrange = inrange(yhigh, min_array[y_axis], max_array[y_axis]);
	    low_inrange = inrange(ylow, min_array[y_axis], max_array[y_axis]);

	    /* compute the plot position of yhigh */
	    if (high_inrange)
		yhighM = map_y(yhigh);
	    else if (samesign(yhigh - max_array[y_axis], max_array[y_axis] - min_array[y_axis]))
		yhighM = map_y(max_array[y_axis]);
	    else
		yhighM = map_y(min_array[y_axis]);

	    /* compute the plot position of ylow */
	    if (low_inrange)
		ylowM = map_y(ylow);
	    else if (samesign(ylow - max_array[y_axis], max_array[y_axis] - min_array[y_axis]))
		ylowM = map_y(max_array[y_axis]);
	    else
		ylowM = map_y(min_array[y_axis]);

	    if (!high_inrange && !low_inrange && ylowM == yhighM)
		/* both out of range on the same side */
		continue;

	    /* find low and high points of bar, and check xrange */
	    xhigh = plot->points[i].xhigh;
	    xlow = plot->points[i].xlow;

	    high_inrange = inrange(xhigh, min_array[x_axis], max_array[x_axis]);
	    low_inrange = inrange(xlow, min_array[x_axis], max_array[x_axis]);

	    /* compute the plot position of xhigh */
	    if (high_inrange)
		xhighM = map_x(xhigh);
	    else if (samesign(xhigh - max_array[x_axis], max_array[x_axis] - min_array[x_axis]))
		xhighM = map_x(max_array[x_axis]);
	    else
		xhighM = map_x(min_array[x_axis]);

	    /* compute the plot position of xlow */
	    if (low_inrange)
		xlowM = map_x(xlow);
	    else if (samesign(xlow - max_array[x_axis], max_array[x_axis] - min_array[x_axis]))
		xlowM = map_x(max_array[x_axis]);
	    else
		xlowM = map_x(min_array[x_axis]);

	    if (!high_inrange && !low_inrange && xlowM == xhighM)
		/* both out of range on the same side */
		continue;

	    /* by here everything has been mapped */
	    if (!polar) {
		/* HBB 981130: use Igor's routine *only* for polar errorbars */
		(*t->move) (xM, ylowM);
		/* draw the main bar */
		(*t->vector) (xM, yhighM);
		if (bar_size > 0.0) {
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
    if ((plot->plot_style == XERRORBARS) ||
	(plot->plot_style == XYERRORBARS) || (plot->plot_style == XERRORLINES) || (plot->plot_style == XYERRORLINES)) {

/* Draw the horizontal part of the bar */
	for (i = 0; i < plot->p_count; i++) {
	    /* undefined points don't count */
	    if (plot->points[i].type == UNDEFINED)
		continue;

	    /* check to see if in yrange */
	    y = plot->points[i].y;
	    if (!inrange(y, min_array[y_axis], max_array[y_axis]))
		continue;
	    yM = map_y(y);

	    /* find low and high points of bar, and check xrange */
	    xhigh = plot->points[i].xhigh;
	    xlow = plot->points[i].xlow;

	    high_inrange = inrange(xhigh, min_array[x_axis], max_array[x_axis]);
	    low_inrange = inrange(xlow, min_array[x_axis], max_array[x_axis]);

	    /* compute the plot position of xhigh */
	    if (high_inrange)
		xhighM = map_x(xhigh);
	    else if (samesign(xhigh - max_array[x_axis], max_array[x_axis] - min_array[x_axis]))
		xhighM = map_x(max_array[x_axis]);
	    else
		xhighM = map_x(min_array[x_axis]);

	    /* compute the plot position of xlow */
	    if (low_inrange)
		xlowM = map_x(xlow);
	    else if (samesign(xlow - max_array[x_axis], max_array[x_axis] - min_array[x_axis]))
		xlowM = map_x(max_array[x_axis]);
	    else
		xlowM = map_x(min_array[x_axis]);

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
 * Plot the curves in BOXES style
 */
static void
plot_boxes(plot, xaxis_y)
struct curve_points *plot;
int xaxis_y;
{
    int i;			/* point index */
    int xl, xr, yt;		/* point in terminal coordinates */
    double dxl, dxr, dyt;
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    TBOOLEAN boxxy = (plot->plot_style == BOXXYERROR);

    for (i = 0; i < plot->p_count; i++) {

	switch (plot->points[i].type) {
	case OUTRANGE:
	case INRANGE:{
		if (plot->points[i].z < 0.0) {
		    /* need to auto-calc width */
		    /* ASSERT(boxwidth <= 0.0); - else graphics.c
		     * provides width */

		    /* calculate width */
		    if (prev != UNDEFINED)
			dxl = (plot->points[i - 1].x - plot->points[i].x) / 2.0;
		    else
			dxl = 0.0;

		    if (i < plot->p_count - 1) {
			if (plot->points[i + 1].type != UNDEFINED)
			    dxr = (plot->points[i + 1].x - plot->points[i].x) / 2.0;
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

		if (boxxy) {
		    dyt = plot->points[i].yhigh;
		    xaxis_y = map_y(plot->points[i].ylow);
		} else {
		    dyt = plot->points[i].y;
		}

		/* clip to border */
		if ((min_array[y_axis] < max_array[y_axis] && dyt < min_array[y_axis])
		    || (max_array[y_axis] < min_array[y_axis] && dyt > min_array[y_axis]))
		    dyt = min_array[y_axis];
		if ((min_array[y_axis] < max_array[y_axis] && dyt > max_array[y_axis])
		    || (max_array[y_axis] < min_array[y_axis] && dyt < max_array[y_axis]))
		    dyt = max_array[y_axis];
		if ((min_array[x_axis] < max_array[x_axis] && dxr < min_array[x_axis])
		    || (max_array[x_axis] < min_array[x_axis] && dxr > min_array[x_axis]))
		    dxr = min_array[x_axis];
		if ((min_array[x_axis] < max_array[x_axis] && dxr > max_array[x_axis])
		    || (max_array[x_axis] < min_array[x_axis] && dxr < max_array[x_axis]))
		    dxr = max_array[x_axis];
		if ((min_array[x_axis] < max_array[x_axis] && dxl < min_array[x_axis])
		    || (max_array[x_axis] < min_array[x_axis] && dxl > min_array[x_axis]))
		    dxl = min_array[x_axis];
		if ((min_array[x_axis] < max_array[x_axis] && dxl > max_array[x_axis])
		    || (max_array[x_axis] < min_array[x_axis] && dxl < max_array[x_axis]))
		    dxl = max_array[x_axis];

		xl = map_x(dxl);
		xr = map_x(dxr);
		yt = map_y(dyt);

		(*t->move) (xl, xaxis_y);
		(*t->vector) (xl, yt);
		(*t->vector) (xr, yt);
		(*t->vector) (xr, xaxis_y);
		(*t->vector) (xl, xaxis_y);
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
plot_points(plot)
struct curve_points *plot;
{
    int i;
    int x, y;
    struct termentry *t = term;

    for (i = 0; i < plot->p_count; i++) {
	if (plot->points[i].type == INRANGE) {
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    /* do clipping if necessary */
	    if (!clip_points || (x >= xleft + p_width && y >= ybot + p_height && x <= xright - p_width && y <= ytop - p_height))
		(*t->point) (x, y, plot->lp_properties.p_type);
	}
    }
}

/* plot_dots:
 * Plot the curves in DOTS style
 */
static void
plot_dots(plot)
struct curve_points *plot;
{
    int i;
    int x, y;
    struct termentry *t = term;

    for (i = 0; i < plot->p_count; i++) {
	if (plot->points[i].type == INRANGE) {
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    /* point type -1 is a dot */
	    (*t->point) (x, y, -1);
	}
    }
}

/* plot_vectors:
 * Plot the curves in VECTORS style
 */
static void
plot_vectors(plot)
struct curve_points *plot;
{
    int i;
    int x1, y1, x2, y2;
    struct termentry *t = term;
    TBOOLEAN head;
    struct coordinate GPHUGE points[2];
    double ex, ey;
    double lx[2], ly[2];

    for (i = 0; i < plot->p_count; i++) {
	points[0] = plot->points[i];
	points[1].x = plot->points[i].xhigh;
	points[1].y = plot->points[i].yhigh;
	if (inrange(points[1].x, min_array[x_axis], max_array[x_axis]) && inrange(points[1].y, min_array[y_axis], max_array[y_axis])) {
	    /* to inrange */
	    points[1].type = INRANGE;
	    x2 = map_x(points[1].x);
	    y2 = map_y(points[1].y);
	    head = TRUE;
	    if (points[0].type == INRANGE) {
		x1 = map_x(points[0].x);
		y1 = map_y(points[0].y);
		(*t->arrow) (x1, y1, x2, y2, head);
	    } else if (points[0].type == OUTRANGE) {
		/* from outrange to inrange */
		if (clip_lines1) {
		    edge_intersect(points, 1, &ex, &ey);
		    x1 = map_x(ex);
		    y1 = map_y(ey);
		    (*t->arrow) (x1, y1, x2, y2, head);
		}
	    }
	} else {
	    /* to outrange */
	    points[1].type = OUTRANGE;
	    head = FALSE;
	    if (points[0].type == INRANGE) {
		/* from inrange to outrange */
		if (clip_lines1) {
		    x1 = map_x(points[0].x);
		    y1 = map_y(points[0].y);
		    edge_intersect(points, 1, &ex, &ey);
		    x2 = map_x(ex);
		    y2 = map_y(ey);
		    (*t->arrow) (x1, y1, x2, y2, head);
		}
	    } else if (points[0].type == OUTRANGE) {
		/* from outrange to outrange */
		if (clip_lines2) {
		    if (two_edge_intersect(points, 1, lx, ly)) {
			x1 = map_x(lx[0]);
			y1 = map_y(ly[0]);
			x2 = map_x(lx[1]);
			y2 = map_y(ly[1]);
			(*t->arrow) (x1, y1, x2, y2, head);
		    }
		}
	    }
	}
    }
}


/* plot_f_bars() - finance bars */
static void
plot_f_bars(plot)
struct curve_points *plot;
{
    int i;			/* point index */
    struct termentry *t = term;
    double x;			/* position of the bar */
    double ylow, yhigh, yclose, yopen;	/* the ends of the bars */
    unsigned int xM, ylowM, yhighM;	/* the mapped version of above */
    TBOOLEAN low_inrange, high_inrange;
    int tic = ERRORBARTIC / 2;

    for (i = 0; i < plot->p_count; i++) {
	/* undefined points don't count */
	if (plot->points[i].type == UNDEFINED)
	    continue;

	/* check to see if in xrange */
	x = plot->points[i].x;
	if (!inrange(x, min_array[x_axis], max_array[x_axis]))
	    continue;
	xM = map_x(x);

	/* find low and high points of bar, and check yrange */
	yhigh = plot->points[i].yhigh;
	ylow = plot->points[i].ylow;
	yclose = plot->points[i].z;
	yopen = plot->points[i].y;

	high_inrange = inrange(yhigh, min_array[y_axis], max_array[y_axis]);
	low_inrange = inrange(ylow, min_array[y_axis], max_array[y_axis]);

	/* compute the plot position of yhigh */
	if (high_inrange)
	    yhighM = map_y(yhigh);
	else if (samesign(yhigh - max_array[y_axis], max_array[y_axis] - min_array[y_axis]))
	    yhighM = map_y(max_array[y_axis]);
	else
	    yhighM = map_y(min_array[y_axis]);

	/* compute the plot position of ylow */
	if (low_inrange)
	    ylowM = map_y(ylow);
	else if (samesign(ylow - max_array[y_axis], max_array[y_axis] - min_array[y_axis]))
	    ylowM = map_y(max_array[y_axis]);
	else
	    ylowM = map_y(min_array[y_axis]);

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
 *  we just plot the bars; the points are not plotted 
 */
static void
plot_c_bars(plot)
struct curve_points *plot;
{
    int i;			/* point index */
    struct termentry *t = term;
    double x;			/* position of the bar */
    double ylow, yhigh, yclose, yopen;	/* the ends of the bars */
    unsigned int xM, ylowM, yhighM;	/* the mapped version of above */
    TBOOLEAN low_inrange, high_inrange;
    int tic = ERRORBARTIC / 2;

    for (i = 0; i < plot->p_count; i++) {
	/* undefined points don't count */
	if (plot->points[i].type == UNDEFINED)
	    continue;

	/* check to see if in xrange */
	x = plot->points[i].x;
	if (!inrange(x, min_array[x_axis], max_array[x_axis]))
	    continue;
	xM = map_x(x);

	/* find low and high points of bar, and check yrange */
	yhigh = plot->points[i].yhigh;
	ylow = plot->points[i].ylow;
	yclose = plot->points[i].z;
	yopen = plot->points[i].y;

	high_inrange = inrange(yhigh, min_array[y_axis], max_array[y_axis]);
	low_inrange = inrange(ylow, min_array[y_axis], max_array[y_axis]);

	/* compute the plot position of yhigh */
	if (high_inrange)
	    yhighM = map_y(yhigh);
	else if (samesign(yhigh - max_array[y_axis], max_array[y_axis] - min_array[y_axis]))
	    yhighM = map_y(max_array[y_axis]);
	else
	    yhighM = map_y(min_array[y_axis]);

	/* compute the plot position of ylow */
	if (low_inrange)
	    ylowM = map_y(ylow);
	else if (samesign(ylow - max_array[y_axis], max_array[y_axis] - min_array[y_axis]))
	    ylowM = map_y(max_array[y_axis]);
	else
	    ylowM = map_y(min_array[y_axis]);

	if (!high_inrange && !low_inrange && ylowM == yhighM)
	    /* both out of range on the same side */
	    continue;

	/* by here everything has been mapped */
	if (yopen <= yclose) {
	    (*t->move) (xM, ylowM);
	    (*t->vector) (xM, map_y(yopen));	/* draw the lower bar */
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM - bar_size * tic), map_y(yopen));
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM + bar_size * tic), map_y(yopen));
	    (*t->vector) ((unsigned int) (xM + bar_size * tic), map_y(yclose));
	    (*t->vector) ((unsigned int) (xM - bar_size * tic), map_y(yclose));
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM - bar_size * tic), map_y(yopen));
	    (*t->move) (xM, map_y(yclose));	/* draw the close tic */
	    (*t->vector) (xM, yhighM);
	} else {
	    (*t->move) (xM, ylowM);
	    (*t->vector) (xM, yhighM);
	    /* draw the open tic */
	    (*t->move) ((unsigned int) (xM - bar_size * tic), map_y(yopen));
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM + bar_size * tic), map_y(yopen));
	    (*t->vector) ((unsigned int) (xM + bar_size * tic), map_y(yclose));
	    (*t->vector) ((unsigned int) (xM - bar_size * tic), map_y(yclose));
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM - bar_size * tic), map_y(yopen));
	    /* draw the close tic */
	    (*t->move) ((unsigned int) (xM - bar_size * tic / 2), map_y(yclose));
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM - bar_size * tic / 2), map_y(yopen));
	    /* draw the close tic */
	    (*t->move) ((unsigned int) (xM + bar_size * tic / 2), map_y(yclose));
	    /* draw the open tic */
	    (*t->vector) ((unsigned int) (xM + bar_size * tic / 2), map_y(yopen));
	}

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
static void
edge_intersect(points, i, ex, ey)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *ex, *ey;		/* the point where it crosses an edge */
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
		return;

	    *ex = min_array[x_axis];
	    return;
	}
	/* obviously oy is -VERYLARGE and ox != -VERYLARGE */
	*ey = min_array[y_axis];
	return;
    }
    /*
     * Can't have case (ix == ox && iy == oy) as one point
     * is INRANGE and one point is OUTRANGE.
     */
    if (iy == oy) {
	/* horizontal line */
	/* assume inrange(iy, min_array[y_axis], max_array[y_axis]) */
	*ey = iy;		/* == oy */

	if (inrange(max_array[x_axis], ix, ox))
	    *ex = max_array[x_axis];
	else if (inrange(min_array[x_axis], ix, ox))
	    *ex = min_array[x_axis];
	else {
	    graph_error("error in edge_intersect");
	}
	return;
    } else if (ix == ox) {
	/* vertical line */
	/* assume inrange(ix, min_array[x_axis], max_array[x_axis]) */
	*ex = ix;		/* == ox */

	if (inrange(max_array[y_axis], iy, oy))
	    *ey = max_array[y_axis];
	else if (inrange(min_array[y_axis], iy, oy))
	    *ey = min_array[y_axis];
	else {
	    graph_error("error in edge_intersect");
	}
	return;
    }
    /* slanted line of some kind */

    /* does it intersect min_array[y_axis] edge */
    if (inrange(min_array[y_axis], iy, oy) && min_array[y_axis] != iy && min_array[y_axis] != oy) {
	x = ix + (min_array[y_axis] - iy) * ((ox - ix) / (oy - iy));
	if (inrange(x, min_array[x_axis], max_array[x_axis])) {
	    *ex = x;
	    *ey = min_array[y_axis];
	    return;		/* yes */
	}
    }
    /* does it intersect max_array[y_axis] edge */
    if (inrange(max_array[y_axis], iy, oy) && max_array[y_axis] != iy && max_array[y_axis] != oy) {
	x = ix + (max_array[y_axis] - iy) * ((ox - ix) / (oy - iy));
	if (inrange(x, min_array[x_axis], max_array[x_axis])) {
	    *ex = x;
	    *ey = max_array[y_axis];
	    return;		/* yes */
	}
    }
    /* does it intersect min_array[x_axis] edge */
    if (inrange(min_array[x_axis], ix, ox) && min_array[x_axis] != ix && min_array[x_axis] != ox) {
	y = iy + (min_array[x_axis] - ix) * ((oy - iy) / (ox - ix));
	if (inrange(y, min_array[y_axis], max_array[y_axis])) {
	    *ex = min_array[x_axis];
	    *ey = y;
	    return;
	}
    }
    /* does it intersect max_array[x_axis] edge */
    if (inrange(max_array[x_axis], ix, ox) && max_array[x_axis] != ix && max_array[x_axis] != ox) {
	y = iy + (max_array[x_axis] - ix) * ((oy - iy) / (ox - ix));
	if (inrange(y, min_array[y_axis], max_array[y_axis])) {
	    *ex = max_array[x_axis];
	    *ey = y;
	    return;
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
    return;
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
edge_intersect_steps(points, i, ex, ey)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *ex, *ey;		/* the point where it crosses an edge */
{
    /* global min_array[x_axis], max_array[x_axis], min_array[y_axis], max_array[x_axis] */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	if (inrange(ay, min_array[y_axis], max_array[y_axis])) {
	    *ey = ay;
	    if (ax > max_array[x_axis])
		*ex = max_array[x_axis];
	    else		/* x < min_array[x_axis] */
		*ex = min_array[x_axis];
	} else {
	    *ex = bx;
	    if (ay > max_array[y_axis])
		*ey = max_array[y_axis];
	    else		/* y < min_array[y_axis] */
		*ey = min_array[y_axis];
	}
    } else {			/* from INRANGE to OUTRANGE */
	if (inrange(bx, min_array[x_axis], max_array[x_axis])) {
	    *ex = bx;
	    if (by > max_array[y_axis])
		*ey = max_array[y_axis];
	    else		/* y < min_array[y_axis] */
		*ey = min_array[y_axis];
	} else {
	    *ey = ay;
	    if (bx > max_array[x_axis])
		*ex = max_array[x_axis];
	    else		/* x < min_array[x_axis] */
		*ex = min_array[x_axis];
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
edge_intersect_fsteps(points, i, ex, ey)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *ex, *ey;		/* the point where it crosses an edge */
{
    /* global min_array[x_axis], max_array[x_axis], min_array[y_axis], max_array[x_axis] */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	if (inrange(ax, min_array[x_axis], max_array[x_axis])) {
	    *ex = ax;
	    if (ay > max_array[y_axis])
		*ey = max_array[y_axis];
	    else		/* y < min_array[y_axis] */
		*ey = min_array[y_axis];
	} else {
	    *ey = by;
	    if (bx > max_array[x_axis])
		*ex = max_array[x_axis];
	    else		/* x < min_array[x_axis] */
		*ex = min_array[x_axis];
	}
    } else {			/* from INRANGE to OUTRANGE */
	if (inrange(by, min_array[y_axis], max_array[y_axis])) {
	    *ey = by;
	    if (bx > max_array[x_axis])
		*ex = max_array[x_axis];
	    else		/* x < min_array[x_axis] */
		*ex = min_array[x_axis];
	} else {
	    *ex = ax;
	    if (by > max_array[y_axis])
		*ey = max_array[y_axis];
	    else		/* y < min_array[y_axis] */
		*ey = min_array[y_axis];
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
two_edge_intersect_steps(points, i, lx, ly)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
{
    /* global min_array[x_axis], max_array[x_axis], min_array[y_axis], max_array[x_axis] */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (GPMAX(ax, bx) < min_array[x_axis] || GPMIN(ax, bx) > max_array[x_axis] ||
	GPMAX(ay, by) < min_array[y_axis] || GPMIN(ay, by) > max_array[y_axis] || ((ay > max_array[y_axis] || ay < min_array[y_axis]) && (bx > max_array[x_axis] || bx < min_array[x_axis]))) {
	return (FALSE);
    } else if (inrange(ay, min_array[y_axis], max_array[y_axis]) && inrange(bx, min_array[x_axis], max_array[x_axis])) {	/* corner of step inside plotspace */
	*ly++ = ay;
	if (ax < min_array[x_axis])
	    *lx++ = min_array[x_axis];
	else
	    *lx++ = max_array[x_axis];

	*lx++ = bx;
	if (by < min_array[y_axis])
	    *ly++ = min_array[y_axis];
	else
	    *ly++ = max_array[y_axis];

	return (TRUE);
    } else if (inrange(ay, min_array[y_axis], max_array[y_axis])) {	/* cross plotspace in x-direction */
	*lx++ = min_array[x_axis];
	*ly++ = ay;
	*lx++ = max_array[x_axis];
	*ly++ = ay;
	return (TRUE);
    } else if (inrange(ax, min_array[x_axis], max_array[x_axis])) {	/* cross plotspace in y-direction */
	*lx++ = bx;
	*ly++ = min_array[y_axis];
	*lx++ = bx;
	*ly++ = max_array[y_axis];
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
two_edge_intersect_fsteps(points, i, lx, ly)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
{
    /* global min_array[x_axis], max_array[x_axis], min_array[y_axis], max_array[x_axis] */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (GPMAX(ax, bx) < min_array[x_axis] || GPMIN(ax, bx) > max_array[x_axis] ||
	GPMAX(ay, by) < min_array[y_axis] || GPMIN(ay, by) > max_array[y_axis] || ((by > max_array[y_axis] || by < min_array[y_axis]) && (ax > max_array[x_axis] || ax < min_array[x_axis]))) {
	return (FALSE);
    } else if (inrange(by, min_array[y_axis], max_array[y_axis]) && inrange(ax, min_array[x_axis], max_array[x_axis])) {	/* corner of step inside plotspace */
	*lx++ = ax;
	if (ay < min_array[y_axis])
	    *ly++ = min_array[y_axis];
	else
	    *ly++ = max_array[y_axis];

	*ly = by;
	if (bx < min_array[x_axis])
	    *lx = min_array[x_axis];
	else
	    *lx = max_array[x_axis];

	return (TRUE);
    } else if (inrange(by, min_array[y_axis], max_array[y_axis])) {	/* cross plotspace in x-direction */
	*lx++ = min_array[x_axis];
	*ly++ = by;
	*lx = max_array[x_axis];
	*ly = by;
	return (TRUE);
    } else if (inrange(ax, min_array[x_axis], max_array[x_axis])) {	/* cross plotspace in y-direction */
	*lx++ = ax;
	*ly++ = min_array[y_axis];
	*lx = ax;
	*ly = max_array[y_axis];
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
two_edge_intersect(points, i, lx, ly)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
{
    /* global min_array[x_axis], max_array[x_axis], min_array[y_axis], max_array[x_axis] */
    int count;
    double ix = points[i - 1].x;
    double iy = points[i - 1].y;
    double ox = points[i].x;
    double oy = points[i].y;
    double t[4];
    double swap;
    double t_min, t_max;
#if 0
    fprintf(stderr, "\ntwo_edge_intersect (%g, %g) and (%g, %g) : ", points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
#endif
    /* nasty degenerate cases, effectively drawing to an infinity point (?)
       cope with them here, so don't process them as a "real" OUTRANGE point 

       If more than one coord is -VERYLARGE, then can't ratio the "infinities"
       so drop out by returning FALSE */

    count = 0;
    if (ix == -VERYLARGE)
	count++;
    if (ox == -VERYLARGE)
	count++;
    if (iy == -VERYLARGE)
	count++;
    if (oy == -VERYLARGE)
	count++;

    /* either doesn't pass through graph area *or* 
       can't ratio infinities to get a direction to draw line, so simply return(FALSE) */
    if (count > 1) {
#if 0
	fprintf(stderr, "\tA\n");
#endif
	return (FALSE);
    }
    if (ox == -VERYLARGE || ix == -VERYLARGE) {
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
	if (ix > max_array[x_axis] && inrange(iy, min_array[y_axis], max_array[y_axis])) {
	    lx[0] = min_array[x_axis];
	    ly[0] = iy;

	    lx[1] = max_array[x_axis];
	    ly[1] = iy;
#if 0
	    fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
	    return (TRUE);
	} else {
#if 0
	    fprintf(stderr, "\tB\n");
#endif
	    return (FALSE);
	}
    }
    if (oy == -VERYLARGE || iy == -VERYLARGE) {
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
	if (iy > max_array[y_axis] && inrange(ix, min_array[x_axis], max_array[x_axis])) {
	    lx[0] = ix;
	    ly[0] = min_array[y_axis];

	    lx[1] = ix;
	    ly[1] = max_array[y_axis];
#if 0
	    fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
	    return (TRUE);
	} else {
#if 0
	    fprintf(stderr, "\tC\n");
#endif
	    return (FALSE);
	}
    }
    /*
     * Special horizontal/vertical, etc. cases are checked and remaining
     * slant lines are checked separately.
     *
     * The slant line intersections are solved using the parametric form
     * of the equation for a line, since if we test x/y min/max planes explicitly
     * then e.g. a  line passing through a corner point (min_array[x_axis],min_array[y_axis]) 
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

	/* x coord must be in range, and line must span both min_array[y_axis] and max_array[y_axis] */
	/* note that spanning min_array[y_axis] implies spanning max_array[y_axis], as both points OUTRANGE */
	if (!inrange(ix, min_array[x_axis], max_array[x_axis])) {
	    return (FALSE);
	}
	if (inrange(min_array[y_axis], iy, oy)) {
	    lx[0] = ix;
	    ly[0] = min_array[y_axis];

	    lx[1] = ix;
	    ly[1] = max_array[y_axis];
#if 0
	    fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
	    return (TRUE);
	} else
	    return (FALSE);
    }
    if (iy == oy) {
	/* already checked case (ix == ox && iy == oy) */

	/* line parallel to x axis */
	/* y coord must be in range, and line must span both min_array[x_axis] and max_array[x_axis] */
	/* note that spanning min_array[x_axis] implies spanning max_array[x_axis], as both points OUTRANGE */
	if (!inrange(iy, min_array[y_axis], max_array[y_axis])) {
	    return (FALSE);
	}
	if (inrange(min_array[x_axis], ix, ox)) {
	    lx[0] = min_array[x_axis];
	    ly[0] = iy;

	    lx[1] = max_array[x_axis];
	    ly[1] = iy;
#if 0
	    fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
	    return (TRUE);
	} else
	    return (FALSE);
    }
    /* nasty 2D slanted line in an xy plane */

    /*
       Solve parametric equation

       (ix, iy) + t (diff_x, diff_y)

       where 0.0 <= t <= 1.0 and

       diff_x = (ox - ix);
       diff_y = (oy - iy);
     */

    t[0] = (min_array[x_axis] - ix) / (ox - ix);
    t[1] = (max_array[x_axis] - ix) / (ox - ix);

    if (t[0] > t[1]) {
	swap = t[0];
	t[0] = t[1];
	t[1] = swap;
    }
    t[2] = (min_array[y_axis] - iy) / (oy - iy);
    t[3] = (max_array[y_axis] - iy) / (oy - iy);

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
		(min_array[x_axis] - 1e-5 * (max_array[x_axis] - min_array[x_axis])),
		(max_array[x_axis] + 1e-5 * (max_array[x_axis] - min_array[x_axis])))
	&& inrange(ly[0],
		   (min_array[y_axis] - 1e-5 * (max_array[y_axis] - min_array[y_axis])),
		   (max_array[y_axis] + 1e-5 * (max_array[y_axis] - min_array[y_axis]))))
    {

#if 0
	fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
	return (TRUE);
    }
    return (FALSE);
}

void
write_multiline(x, y, text, hor, vert, angle, font)
unsigned int x, y;
char *text;
enum JUSTIFY hor;		/* horizontal ... */
int vert;			/* ... and vertical just - text in hor direction despite angle */
int angle;			/* assume term has already been set for this */
const char *font;		/* NULL or "" means use default */
{
    register struct termentry *t = term;
    char *p = text;

    if (!p)
	return;

    if (vert != JUST_TOP) {
	/* count lines and adjust y */
	int lines = 0;		/* number of linefeeds - one fewer than lines */
	while (*p++) {
	    if (*p == '\n')
		++lines;
	}
	if (angle)
	    x -= (vert * lines * t->v_char) / 2;
	else
	    y += (vert * lines * t->v_char) / 2;
    }
    if (font && *font)
	(*t->set_font) (font);


    for (;;) {			/* we will explicitly break out */

	if ((text != NULL) && (p = strchr(text, '\n')) != NULL)
	    *p = 0;		/* terminate the string */

	if ((*t->justify_text) (hor)) {
	    (*t->put_text) (x, y, text);
	} else {
	    int fix = hor * (t->h_char) * strlen(text) / 2;
	    if (angle)
		(*t->put_text) (x, y - fix, text);
	    else
		(*t->put_text) (x - fix, y, text);
	}
	if (angle)
	    x += t->v_char;
	else
	    y -= t->v_char;

	if (!p)
	    break;
	else {
	    /* put it back */
	    *p = '\n';
	}

	text = p + 1;
    }				/* unconditional branch back to the for(;;) - just a goto ! */

    if (font && *font)
	(*t->set_font) (default_font);

}

/* display a x-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
static void
xtick2d_callback(axis, place, text, grid)
AXIS_INDEX axis;
double place;
char *text;
struct lp_style_type grid;	/* linetype or -2 for no grid */
{
    register struct termentry *t = term;
    /* minitick if text is NULL - beware - h_tic is unsigned */
    int ticsize = tic_direction * (int) (t->v_tic) * (text ? ticscale : miniticscale);
    unsigned int x = map_x(place);

    if (grid.l_type > -2) {
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
	    if (lkey && keybox.yt > ybot && x < keybox.xr && x > keybox.xl) {
		if (keybox.yb > ybot) {
		    (*t->move) (x, ybot);
		    (*t->vector) (x, keybox.yb);
		}
		if (keybox.yt < ytop) {
		    (*t->move) (x, keybox.yt);
		    (*t->vector) (x, ytop);
		}
	    } else {
		(*t->move) (x, ybot);
		(*t->vector) (x, ytop);
	    }
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
    }
    /* we precomputed tic posn and text posn in global vars */

    (*t->move) (x, tic_start);
    (*t->vector) (x, tic_start + ticsize);

    if (tic_mirror >= 0) {
	(*t->move) (x, tic_mirror);
	(*t->vector) (x, tic_mirror - ticsize);
    }
    if (text)
	write_multiline(x, tic_text, text, tic_hjust, tic_vjust, rotate_tics, NULL);
}

/* display a y-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
static void
ytick2d_callback(axis, place, text, grid)
AXIS_INDEX axis;
double place;
char *text;
struct lp_style_type grid;	/* linetype or -2 */
{
    register struct termentry *t = term;
    /* minitick if text is NULL - v_tic is unsigned */
    int ticsize = tic_direction * (int) (t->h_tic) * (text ? ticscale : miniticscale);
    unsigned int y = map_y(place);

    if (grid.l_type > -2) {
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
	    if (lkey && y < keybox.yt && y > keybox.yb && keybox.xl < xright /* catch TOUT */ ) {
		if (keybox.xl > xleft) {
		    (*t->move) (xleft, y);
		    (*t->vector) (keybox.xl, y);
		}
		if (keybox.xr < xright) {
		    (*t->move) (keybox.xr, y);
		    (*t->vector) (xright, y);
		}
	    } else {
		(*t->move) (xleft, y);
		(*t->vector) (xright, y);
	    }
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
    }
    /* we precomputed tic posn and text posn */

    (*t->move) (tic_start, y);
    (*t->vector) (tic_start + ticsize, y);

    if (tic_mirror >= 0) {
	(*t->move) (tic_mirror, y);
	(*t->vector) (tic_mirror - ticsize, y);
    }
    if (text)
	write_multiline(tic_text, y, text, tic_hjust, tic_vjust, rotate_tics, NULL);
}

int
label_width(str, lines)
const char *str;
int *lines;
{
    char *lab = NULL, *s, *e;
    int mlen, len, l;

    l = mlen = len = 0;
    lab = gp_alloc(strlen(str) + 2, "in label_width");
    strcpy(lab, str);
    strcat(lab, "\n");
    s = lab;
    while ((e = (char *) strchr(s, '\n')) != NULL) {	/* HBB 980308: quiet BC-3.1 warning */
	*e = '\0';
	len = strlen(s);	/* = e-s ? */
	if (len > mlen)
	    mlen = len;
	if (len || l)
	    l++;
	s = ++e;
    }
    /* lines = NULL => not interested - div */
    if (lines)
	*lines = l;

    free(lab);
    return (mlen);
}


/*{{{  map_position */
void
map_position(pos, x, y, what)
struct position *pos;
unsigned int *x, *y;
const char *what;
{
    switch (pos->scalex) {
    case first_axes:
	{
	    double xx = axis_log_value_checked(FIRST_X_AXIS, pos->x, what);
	    *x = xleft + (xx - min_array[FIRST_X_AXIS]) * scale[FIRST_X_AXIS] + 0.5;
	    break;
	}
    case second_axes:
	{
	    double xx = axis_log_value_checked(SECOND_X_AXIS, pos->x, what);
	    *x = xleft + (xx - min_array[SECOND_X_AXIS]) * scale[SECOND_X_AXIS] + 0.5;
	    break;
	}
    case graph:
	{
	    *x = xleft + pos->x * (xright - xleft) + 0.5;
	    break;
	}
    case screen:
	{
	    register struct termentry *t = term;
	    *x = pos->x * (t->xmax) + 0.5;
	    break;
	}
    }
    switch (pos->scaley) {
    case first_axes:
	{
	    double yy = axis_log_value_checked(FIRST_Y_AXIS, pos->y, what);
	    *y = ybot + (yy - min_array[FIRST_Y_AXIS]) * scale[FIRST_Y_AXIS] + 0.5;
	    return;
	}
    case second_axes:
	{
	    double yy = axis_log_value_checked(SECOND_Y_AXIS, pos->y, what);
	    *y = ybot + (yy - min_array[SECOND_Y_AXIS]) * scale[SECOND_Y_AXIS] + 0.5;
	    return;
	}
    case graph:
	{
	    *y = ybot + pos->y * (ytop - ybot) + 0.5;
	    return;
	}
    case screen:
	{
	    register struct termentry *t = term;
	    *y = pos->y * (t->ymax) + 0.5;
	    return;
	}
    }
}

/*}}} */
