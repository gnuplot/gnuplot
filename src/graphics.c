#ifndef lint
static char *RCSid() { return RCSid("$Id: graphics.c,v 1.81 2003/01/27 20:39:32 broeker Exp $"); }
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
#include "gadgets.h"
/* FIXME HBB 20010822: this breaks the plan of disentangling graphics
 * and plot2d, because each #include's the other's header: */
#include "plot2d.h"		/* for boxwidth */ 
#include "term_api.h"
#include "util.h"


/* Externally visible/modifiable status variables */

/* 'set offset' --- artificial buffer zone between coordinate axes and
 * the area actually covered by the data */
double loff = 0.0;
double roff = 0.0;
double toff = 0.0;
double boff = 0.0;

/* set bars */
double bar_size = 1.0;

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

/* I think the following should also be static ?? */

static int max_ptitl_len = 0;	/* max length of plot-titles (keys) */
static int ktitl_lines = 0;	/* no lines in key->title (key header) */
static int ptitl_cnt;		/* count keys with len > 0  */
static int key_cols;		/* no cols of keys */
static int key_rows, key_col_wth, yl_ref;
static struct clipbox keybox;	/* boundaries for key field */

/* set by tic_callback - how large to draw polar radii */
static double largest_polar_circle;

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
#ifdef PM3D
static void plot_filledcurves __PROTO((struct curve_points * plot));
static void finish_filled_curve __PROTO((int, gpiPoint *, filledcurves_opts *));
#endif
static void plot_vectors __PROTO((struct curve_points * plot));
static void plot_f_bars __PROTO((struct curve_points * plot));
static void plot_c_bars __PROTO((struct curve_points * plot));

static void place_labels __PROTO((struct text_label * listhead, int layer));
static void place_arrows __PROTO((int layer));
static void place_grid __PROTO((void));

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

static void boundary __PROTO((struct curve_points * plots, int count));

/* HBB 20010118: these should be static, but can't --- HP-UX assembler bug */
void ytick2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));
void xtick2d_callback __PROTO((AXIS_INDEX, double place, char *text, struct lp_style_type grid));
int histeps_compare __PROTO((SORTFUNC_ARGS p1, SORTFUNC_ARGS p2));

static void get_arrow __PROTO((struct arrow_def* arrow, unsigned int* sx, unsigned int* sy, unsigned int* ex, unsigned int* ey));
static void map_position_double __PROTO((struct position* pos, double* x, double* y, const char* what));
static void map_position_r __PROTO((struct position* pos, double* x, double* y, const char* what));

static int find_maxl_keys __PROTO((struct curve_points *plots, int count, int *kcnt));

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

static t_key_flag lkey;

/*}}} */

static int
find_maxl_keys(plots, count, kcnt)
struct curve_points *plots;
int count, *kcnt;
{
    int mlen, len, curve, cnt;
    register struct curve_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (curve = 0; curve < count; this_plot = this_plot->next, curve++)
	if (this_plot->title
	    && ((len = /*assign */ strlen(this_plot->title)) != 0)	/* HBB 980308: quiet BCC warning */
	    ) {
	    cnt++;
	    if (len > mlen)
		mlen = strlen(this_plot->title);
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
boundary(plots, count)
    struct curve_points *plots;
    int count;
{
    int ytlen;
    int yticlin = 0, y2ticlin = 0, timelin = 0;
    legend_key *key = &keyT;

    register struct termentry *t = term;
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
    int xtic_height;
    int ytic_width;
    int y2tic_width;

    /* figure out which rotatable items are to be rotated
     * (ylabel and y2label are rotated if possible) */
    int vertical_timelabel = can_rotate ? timelabel_rotate : 0;
    int vertical_xtics  = can_rotate ? axis_array[FIRST_X_AXIS].tic_rotate : 0;
    int vertical_x2tics = can_rotate ? axis_array[SECOND_X_AXIS].tic_rotate : 0;
    int vertical_ytics  = can_rotate ? axis_array[FIRST_Y_AXIS].tic_rotate : 0;
    int vertical_y2tics = can_rotate ? axis_array[SECOND_Y_AXIS].tic_rotate : 0;

    lkey = key->flag;		/* but we may have to disable it later */

    xticlin = ylablin = y2lablin = xlablin = x2lablin = titlelin = 0;

    /*{{{  count lines in labels and tics */
    if (*title.text)
	label_width(title.text, &titlelin);
    if (*axis_array[FIRST_X_AXIS].label.text)
	label_width(axis_array[FIRST_X_AXIS].label.text, &xlablin);
    if (*axis_array[SECOND_X_AXIS].label.text)
	label_width(axis_array[SECOND_X_AXIS].label.text, &x2lablin);
    if (*axis_array[FIRST_Y_AXIS].label.text)
	label_width(axis_array[FIRST_Y_AXIS].label.text, &ylablin);
    if (*axis_array[SECOND_Y_AXIS].label.text)
	label_width(axis_array[SECOND_Y_AXIS].label.text, &y2lablin);
    if (axis_array[FIRST_X_AXIS].ticmode)
	label_width(axis_array[FIRST_X_AXIS].formatstring, &xticlin);
    if (axis_array[SECOND_X_AXIS].ticmode)
	label_width(axis_array[SECOND_X_AXIS].formatstring, &x2ticlin);
    if (axis_array[FIRST_Y_AXIS].ticmode)
	label_width(axis_array[FIRST_Y_AXIS].formatstring, &yticlin);
    if (axis_array[SECOND_Y_AXIS].ticmode)
	label_width(axis_array[SECOND_Y_AXIS].formatstring, &y2ticlin);
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
	x2label_textheight = (int) ((x2lablin + axis_array[SECOND_X_AXIS].label.yoffset) * (t->v_char));
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
	    x2tic_textheight = (int) (5 * (t->h_char));
	} else
	    x2tic_textheight = (int) ((x2ticlin) * (t->v_char));
    } else
	x2tic_textheight = 0;

    /* tics */
    if (!tic_in
	&& ((axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[FIRST_X_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER))))
	x2tic_height = (int) ((t->v_tic) * ticscale);
    else
	x2tic_height = 0;

    /* timestamp */
    if (*timelabel.text && !timelabel_bottom)
	timetop_textheight = (int) ((timelin + timelabel.yoffset + 2) * (t->v_char));
    else
	timetop_textheight = 0;

    /* horizontal ylabel */
    if (*axis_array[FIRST_Y_AXIS].label.text && !can_rotate)
	ylabel_textheight = (int) ((ylablin + axis_array[FIRST_Y_AXIS].label.yoffset) * (t->v_char));
    else
	ylabel_textheight = 0;

    /* horizontal y2label */
    if (*axis_array[SECOND_Y_AXIS].label.text && !can_rotate)
	y2label_textheight = (int) ((y2lablin + axis_array[SECOND_Y_AXIS].label.yoffset) * (t->v_char));
    else
	y2label_textheight = 0;

    /* compute ytop from the various components
     *     unless tmargin is explicitly specified  */

    /* HBB 20010118: fix round-off bug */
    ytop = (int) (0.5 + (ysize + yoffset) * (t->ymax));

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
    if (axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER) {
	/* ought to consider tics on axes if axis near border */
	if (vertical_xtics) {
	    /* guess at tic length, since we don't know it yet */
	    xtic_textheight = (int) ((t->h_char) * 5);
	} else
	    xtic_textheight = (int) ((t->v_char) * (xticlin + 1));
    } else
	xtic_textheight = 0;

    /* tics */
    if (!tic_in
	&& ((axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[SECOND_X_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER))))
	xtic_height = (int) ((t->v_tic) * ticscale);
    else
	xtic_height = 0;

    /* xlabel */
    if (xlablin) {
	/* offset is subtracted because if > 0, the margin is smaller */
	xlabel_textheight = ((xlablin - axis_array[FIRST_X_AXIS].label.yoffset)
			     * t->v_char);
	if (!axis_array[FIRST_X_AXIS].ticmode)
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

    ybot = (int) (0.5 + (t->ymax) * yoffset);

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


#define KEY_PANIC(x) if (x) { lkey = KEY_NONE; goto key_escape; }

    if (lkey) {
	/*{{{  essential key features */
	p_width = pointsize * t->h_tic;
	p_height = pointsize * t->v_tic;

	if (key->swidth >= 0)
	    key_sample_width = key->swidth * (t->h_char) + p_width;
	else
	    key_sample_width = 0;

	key_entry_height = p_height * 1.25 * key->vert_factor;
	if (key_entry_height < (t->v_char))
	    key_entry_height = (t->v_char) * key->vert_factor;
	/* HBB 20020122: safeguard to prevent division by zero later */
	if (key_entry_height == 0)
	    key_entry_height = 1;

	/* count max_len key and number keys with len > 0 */
	max_ptitl_len = find_maxl_keys(plots, count, &ptitl_cnt);
	if ((ytlen = label_width(key->title, &ktitl_lines)) > max_ptitl_len)
	    max_ptitl_len = ytlen;

	if (key->reverse) {
	    key_sample_left = -key_sample_width;
	    key_sample_right = 0;
	    /* if key width is being used, adjust right-justified text */
	    key_text_left = t->h_char;
	    key_text_right = (t->h_char) * (max_ptitl_len + 1 + key->width_fix);
	    key_size_left = t->h_char - key_sample_left;	/* sample left is -ve */
	    key_size_right = key_text_right;
	} else {
	    key_sample_left = 0;
	    key_sample_right = key_sample_width;
	    /* if key width is being used, adjust left-justified text */
	    key_text_left = -(int) ((t->h_char) * (max_ptitl_len + 1 + key->width_fix));
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

	if (lkey == KEY_AUTO_PLACEMENT) {
	    if (key->vpos == TUNDER) {
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
		keybox.yt += (int)(key->height_fix * (t->v_char));
		ybot += key_entry_height * key_rows + (int) ((t->v_char) * (ktitl_lines + 1));
		ybot += (int)(key->height_fix * (t->v_char));
	    } else {
		/* maximise no rows, limited by ytop-ybot */
		int i = (int) (ytop - ybot - key->height_fix * (t->v_char)
			       - (ktitl_lines + 1) * (t->v_char))
		    / key_entry_height;

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
    setup_tics(FIRST_Y_AXIS, 20);
    setup_tics(SECOND_Y_AXIS, 20);
    /*}}} */


    /*{{{  recompute xleft based on widths of ytics, ylabel etc
       unless it has been explicitly set by lmargin */

    /* tic labels */
    if (axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER) {
	if (vertical_ytics)
	    /* HBB: we will later add some white space as part of this, so
	     * reserve two more rows (one above, one below the text ...).
	     * Same will be done to similar calc.'s elsewhere */
	    ytic_textwidth = (int) ((t->v_char) * (yticlin + 2));
	else {
	    widest_tic_strlen = 0;	/* reset the global variable ... */
	    /* get gen_tics to call widest_tic_callback with all labels
	     * the latter sets widest_tic_strlen to the length of the widest
	     * one ought to consider tics on axis if axis near border...
	     */
	    gen_tics(FIRST_Y_AXIS, /* 0, */ widest_tic_callback);

	    ytic_textwidth = (int) ((t->h_char) * (widest_tic_strlen + 2));
	}
    } else {
	ytic_textwidth = 0;
    }

    /* tics */
    if (!tic_in
	&& ((axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[SECOND_Y_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER))))
	ytic_width = (int) ((t->h_tic) * ticscale);
    else
	ytic_width = 0;

    /* ylabel */
    if (*axis_array[FIRST_Y_AXIS].label.text && can_rotate) {
	ylabel_textwidth = (int) ((ylablin - axis_array[FIRST_Y_AXIS].label.xoffset) * (t->v_char));
	if (!axis_array[FIRST_Y_AXIS].ticmode)
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

    xleft = (int) (0.5 + (t->xmax) * xoffset);

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
    if (axis_array[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER) {
	if (vertical_y2tics)
	    y2tic_textwidth = (int) ((t->v_char) * (y2ticlin + 2));
	else {
	    widest_tic_strlen = 0;	/* reset the global variable ... */
	    /* get gen_tics to call widest_tic_callback with all labels
	     * the latter sets widest_tic_strlen to the length of the widest
	     * one ought to consider tics on axis if axis near border...
	     */
	    gen_tics(SECOND_Y_AXIS, /* 0, */ widest_tic_callback);

	    y2tic_textwidth = (int) ((t->h_char) * (widest_tic_strlen + 2));
	}
    } else {
	y2tic_textwidth = 0;
    }

    /* tics */
    if (!tic_in
	&& ((axis_array[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER)
	    || ((axis_array[FIRST_Y_AXIS].ticmode & TICS_MIRROR)
		&& (axis_array[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER))))
	y2tic_width = (int) ((t->h_tic) * ticscale);
    else
	y2tic_width = 0;

    /* y2label */
    if (can_rotate && *axis_array[SECOND_Y_AXIS].label.text) {
	y2label_textwidth = (int) ((y2lablin + axis_array[SECOND_Y_AXIS].label.xoffset) * (t->v_char));
	if (!axis_array[SECOND_Y_AXIS].ticmode)
	    y2label_textwidth += 0.5 * t->v_char;
    } else
	y2label_textwidth = 0;

    /* compute xright from the various components
     *     unless rmargin is explicitly specified  */

    xright = (int) (0.5 + (t->xmax) * (xsize + xoffset));

    if (rmargin < 0) {
	/* xright -= y2label_textwidth + y2tic_width + y2tic_textwidth; */
	xright -= y2tic_width + y2tic_textwidth;
	if (y2label_textwidth > 0)
	    xright -= y2label_textwidth;

	/* adjust for outside key */
	if (lkey == KEY_AUTO_PLACEMENT && key->hpos == TOUT) {
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
		* fabs((Y_AXIS.max - Y_AXIS.min) /
		       (X_AXIS.max - X_AXIS.min));
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

    /*  adjust top and bottom margins for tic label rotation */

    if (tmargin < 0
	&& axis_array[SECOND_X_AXIS].ticmode & TICS_ON_BORDER
	&& vertical_x2tics) {
	widest_tic_strlen = 0;		/* reset the global variable ... */
	gen_tics(SECOND_X_AXIS, /* 0, */ widest_tic_callback);
	ytop += x2tic_textheight;
	/* Now compute a new one and use that instead: */
	x2tic_textheight = (int) ((t->h_char) * (widest_tic_strlen));
	ytop -= x2tic_textheight;
    }
    if (bmargin < 0
	&& axis_array[FIRST_X_AXIS].ticmode & TICS_ON_BORDER
	&& vertical_xtics) {
	widest_tic_strlen = 0;		/* reset the global variable ... */
	gen_tics(FIRST_X_AXIS, /* 0, */ widest_tic_callback);
	ybot -= xtic_textheight;
	xtic_textheight = (int) ((t->h_char) * widest_tic_strlen);
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
    if (*axis_array[FIRST_Y_AXIS].label.text && can_rotate)
	ylabel_x -= ylabel_textwidth;

    y2label_x = xright + y2tic_width + y2tic_textwidth;
    if (*axis_array[SECOND_Y_AXIS].label.text && can_rotate)
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
    /* HBB 20020122: moved here from do_plot, because map_position
     * needs these, too */
    axis_set_graphical_range(FIRST_X_AXIS, xleft, xright);
    axis_set_graphical_range(FIRST_Y_AXIS, ybot, ytop);
    axis_set_graphical_range(SECOND_X_AXIS, xleft, xright);
    axis_set_graphical_range(SECOND_Y_AXIS, ybot, ytop);

    /*{{{  calculate the window in the grid for the key */
    if (lkey == KEY_USER_PLACEMENT || (lkey == KEY_AUTO_PLACEMENT && key->vpos != TUNDER)) {
	/* calculate space for keys to prevent grid overwrite the keys */
	/* do it even if there is no grid, as do_plot will use these to position key */
	key_w = key_col_wth * key_cols;
	key_h = (ktitl_lines) * t->v_char + key_rows * key_entry_height;
	key_h += (int)(key->height_fix * (t->v_char));
	if (lkey == KEY_AUTO_PLACEMENT) {
	    if (key->vpos == TTOP) {
		keybox.yt = (int) ytop - (t->v_tic);
		keybox.yb = keybox.yt - key_h;
	    } else {
		keybox.yb = ybot + (t->v_tic);
		keybox.yt = keybox.yb + key_h;
	    }
	    if (key->hpos == TLEFT) {
		keybox.xl = xleft + (t->h_char);	/* for Left just */
		keybox.xr = keybox.xl + key_w;
	    } else if (key->hpos == TRIGHT) {
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
	    map_position(&key->user_pos, &x, &y, "key");
#if 0
// FIXME!!!
// pm 22.1.2002: if key->user_pos.scalex or scaley == first_axes or second_axes,
// then the graph scaling is not yet known and the box is positioned incorrectly;
// you must do "replot" to avoid the wrong plot ... bad luck if output does not
// go to screen
#define OK fprintf(stderr,"Line %i of %s is OK\n",__LINE__,__FILE__);
OK
fprintf(stderr,"\tHELE: user pos: x=%i y=%i\n",key->user_pos.x,key->user_pos.y);
fprintf(stderr,"\tHELE: user pos: x=%i y=%i\n",x,y);
#endif
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
    if (this_label->lp_properties.pointflag) {
	*htic = (pointsize * t->h_tic * 0.5 * this_label->hoffset);
	*vtic = (pointsize * t->v_tic * 0.5 * this_label->voffset);
    } else {
	*htic = 0;
	*vtic = 0;
    }
}


static void
get_arrow(arrow, sx, sy, ex, ey)
struct arrow_def* arrow;
unsigned int* sx;
unsigned int* sy;
unsigned int* ex;
unsigned int* ey;
{
    if (arrow->relative) {
	if (arrow->start.scalex == arrow->end.scalex &&
	    arrow->start.scaley == arrow->end.scaley) {
	    /* coordinate systems are equal. The advantage of
	     * handling this special case is that it works also
	     * for logscale (which might not work otherwise, if
	     * the relative arrows point downwards for example) */
	    struct position delta_pos;
	    delta_pos = arrow->start;
	    delta_pos.x += arrow->end.x;
	    delta_pos.y += arrow->end.y;
	    map_position(&arrow->start, sx, sy, "arrow");
	    map_position(&delta_pos, ex, ey, "arrow");
	} else {
	    /* different coordinate systems:
	     * add the values in the drivers
	     * coordinate system */
	    double sx_d, sy_d, ex_d, ey_d;
	    map_position_double(&arrow->start, &sx_d, &sy_d, "arrow");
	    map_position_r(&arrow->end, &ex_d, &ey_d, "arrow");
	    *sx = (unsigned int)sx_d;
	    *sy = (unsigned int)sy_d;
	    *ex = (unsigned int)(ex_d + sx_d);
	    *ey = (unsigned int)(ey_d + sy_d);
	}
    } else {
	map_position(&arrow->start, sx, sy, "arrow");
	map_position(&arrow->end, ex, ey, "arrow");
    }
}

/* FIXME HBB 20020225: this is shared with graph3d.c, so it shouldn't
 * be in this module */
void
apply_head_properties(struct arrow_style_type *arrow_properties)
{
    curr_arrow_headfilled = arrow_properties->head_filled;
    curr_arrow_headlength = 0;
    if (arrow_properties->head_length > 0) { /* set head length+angle for term->arrow */
	unsigned int itmp, x1, x2;
	struct position headsize;
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
    register struct termentry *t = term;
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
place_arrows(layer)
    int layer;
{
    struct arrow_def *this_arrow;
    register struct termentry *t = term;
    for (this_arrow = first_arrow; this_arrow != NULL; this_arrow = this_arrow->next) {
	unsigned int sx, sy, ex, ey;

	if (this_arrow->arrow_properties.layer != layer)
	    continue;
	get_arrow(this_arrow, &sx, &sy, &ex, &ey);

	term_apply_lp_properties(&(this_arrow->arrow_properties.lp_properties));
	apply_head_properties(&(this_arrow->arrow_properties));
	(*t->arrow) (sx, sy, ex, ey, this_arrow->arrow_properties.head);
    }
}

static void
place_labels(listhead, layer)
    struct text_label *listhead;
    int layer;
{
    struct text_label *this_label;
    register struct termentry *t = term;
    if ((t->pointsize)) {
	(*t->pointsize) (pointsize);
    }
    for (this_label = listhead; this_label != NULL; this_label = this_label->next) {

	unsigned int x, y;
	int htic;
	int vtic;

	get_offsets(this_label, t, &htic, &vtic);

	if (this_label->layer != layer)
	    continue;
	map_position(&this_label->place, &x, &y, "label");

	/* EAM - textcolor support in progress */
	apply_textcolor(&(this_label->textcolor),t);

	/* EAM - Allow arbitrary rotation of label text */
	if (this_label->rotate && (*t->text_angle) (this_label->rotate)) {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, JUST_TOP, 
	    		    TEXT_VERTICAL, this_label->font);
	    (*t->text_angle) (0);
	} else {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, JUST_TOP, 
	    		    0, this_label->font);
	}
	if (this_label->lp_properties.pointflag) {
	    term_apply_lp_properties(&this_label->lp_properties);
	    (*t->point) (x, y, this_label->lp_properties.p_type);
	    /* the default label colour is that of border */
	    term_apply_lp_properties(&border_lp);
	}
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
    legend_key *key = &keyT;
    char *s, *e;

    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;

/*      Apply the desired viewport offsets. */
    if (Y_AXIS.min < Y_AXIS.max) {
	Y_AXIS.min -= boff;
	Y_AXIS.max += toff;
    } else {
	Y_AXIS.max -= boff;
	Y_AXIS.min += toff;
    }
    if (X_AXIS.min < X_AXIS.max) {
	X_AXIS.min -= loff;
	X_AXIS.max += roff;
    } else {
	X_AXIS.max -= loff;
	X_AXIS.min += roff;
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
    if (X_AXIS.min == X_AXIS.max)
	int_error(NO_CARET, "x_min should not equal x_max!");
    if (Y_AXIS.min == Y_AXIS.max)
	int_error(NO_CARET, "y_min should not equal y_max!");

    term_init();		/* may set xmax/ymax */

    /* compute boundary for plot (xleft, xright, ytop, ybot)
     * also calculates tics, since xtics depend on xleft
     * but xleft depends on ytics. Boundary calculations
     * depend on term->v_char etc, so terminal must be
     * initialised.
     */
    boundary(plots, pcount);

    screen_ok = FALSE;

    term_start_plot();

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
    if (*axis_array[FIRST_Y_AXIS].label.text) {
	apply_textcolor(&(axis_array[FIRST_Y_AXIS].label.textcolor),t);
	/* we worked out x-posn in boundary() */
	if ((*t->text_angle) (TEXT_VERTICAL)) {
	    unsigned int x = ylabel_x + (t->v_char / 2);
	    unsigned int y = (ytop + ybot) / 2 + axis_array[FIRST_Y_AXIS].label.yoffset * (t->h_char);
	    write_multiline(x, y, axis_array[FIRST_Y_AXIS].label.text, CENTRE, JUST_TOP, 
	    		    TEXT_VERTICAL, axis_array[FIRST_Y_AXIS].label.font);
	    (*t->text_angle) (0);
	} else {
	    /* really bottom just, but we know number of lines 
	       so we need to adjust x-posn by one line */
	    unsigned int x = ylabel_x;
	    unsigned int y = ylabel_y;
	    write_multiline(x, y, axis_array[FIRST_Y_AXIS].label.text, LEFT, JUST_TOP, 0, axis_array[FIRST_Y_AXIS].label.font);
	}
	reset_textcolor(&(axis_array[FIRST_Y_AXIS].label.textcolor),t);
    }
/* Y2LABEL */
    if (*axis_array[SECOND_Y_AXIS].label.text) {
	apply_textcolor(&(axis_array[SECOND_Y_AXIS].label.textcolor),t);
	/* we worked out coordinates in boundary() */
	if ((*t->text_angle) (TEXT_VERTICAL)) {
	    unsigned int x = y2label_x + (t->v_char / 2) - 1;
	    unsigned int y = (ytop + ybot) / 2 + axis_array[SECOND_Y_AXIS].label.yoffset * (t->h_char);
	    write_multiline(x, y, axis_array[SECOND_Y_AXIS].label.text, CENTRE, JUST_TOP, 
	    		    TEXT_VERTICAL, axis_array[SECOND_Y_AXIS].label.font);
	    (*t->text_angle) (0);
	} else {
	    /* really bottom just, but we know number of lines */
	    unsigned int x = y2label_x;
	    unsigned int y = y2label_y;
	    write_multiline(x, y, axis_array[SECOND_Y_AXIS].label.text, RIGHT, JUST_TOP, 0, axis_array[SECOND_Y_AXIS].label.font);
	}
	reset_textcolor(&(axis_array[SECOND_Y_AXIS].label.textcolor),t);
    }
/* XLABEL */
    if (*axis_array[FIRST_X_AXIS].label.text) {
	unsigned int x = (xright + xleft) / 2 + axis_array[FIRST_X_AXIS].label.xoffset * (t->h_char);
	unsigned int y = xlabel_y - t->v_char / 2;	/* HBB */
	apply_textcolor(&(axis_array[FIRST_X_AXIS].label.textcolor),t);
	write_multiline(x, y, axis_array[FIRST_X_AXIS].label.text, CENTRE, JUST_TOP, 0, axis_array[FIRST_X_AXIS].label.font);
	reset_textcolor(&(axis_array[FIRST_X_AXIS].label.textcolor),t);
    }
/* PLACE TITLE */
    if (*title.text) {
	/* we worked out y-coordinate in boundary() */
	unsigned int x = (xleft + xright) / 2 + title.xoffset * t->h_char;
	unsigned int y = title_y - t->v_char / 2;
	apply_textcolor(&(title.textcolor),t);
	write_multiline(x, y, title.text, CENTRE, JUST_TOP, 0, title.font);
	reset_textcolor(&(title.textcolor),t);
    }
/* X2LABEL */
    if (*axis_array[SECOND_X_AXIS].label.text) {
	/* we worked out y-coordinate in boundary() */
	unsigned int x = (xright + xleft) / 2 + axis_array[SECOND_X_AXIS].label.xoffset * (t->h_char);
	unsigned int y = x2label_y - t->v_char / 2 - 1;
	apply_textcolor(&(axis_array[SECOND_X_AXIS].label.textcolor),t);
	write_multiline(x, y, axis_array[SECOND_X_AXIS].label.text, CENTRE, JUST_TOP, 0, axis_array[SECOND_X_AXIS].label.font);
	reset_textcolor(&(axis_array[SECOND_X_AXIS].label.textcolor),t);
    }
/* PLACE TIMEDATE */
    if (*timelabel.text) {
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

/* PLACE LABELS */
    place_labels( first_label, 0 );

/* PLACE ARROWS */
    place_arrows( 0 );

/* WORK OUT KEY SETTINGS AND DO KEY TITLE / BOX */

    if (lkey) {			/* may have been cancelled if something went wrong */
	/* just use keybox.xl etc worked out in boundary() */
	xl = keybox.xl + key_size_left;
	yl = keybox.yt;
	yl -= (int)(0.5 * key->height_fix * (t->v_char));

	if (*key->title) {
	    char *ss = gp_alloc(strlen(key->title) + 2, "tmp string ss");
	    strcpy(ss, key->title);
	    strcat(ss, "\n");

	    s = ss;
	    yl -= t->v_char / 2;
	    while ((e = (char *) strchr(s, '\n')) != NULL) {
		*e = '\0';
		if (key->just == JLEFT) {
		    (*t->justify_text) (LEFT);
		    (*t->put_text) (xl + key_text_left, yl, s);
		} else {
		    if ((*t->justify_text) (RIGHT)) {
			(*t->put_text) (xl + key_text_right, yl, s);
		    } else {
			int x = xl + key_text_right - (t->h_char) * strlen(s);
			if (key->hpos == TOUT || key->vpos == TUNDER ||	/* HBB 990327 */
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

	if (key->box.l_type > L_TYPE_NODRAW) {
	    term_apply_lp_properties(&key->box);
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
	    ignore_enhanced_text = this_plot->title_no_enhanced == 1;
		/* don't write filename or function enhanced */
	    if (localkey != 0 && this_plot->title) {
		key_count++;

		/* EAM - force key text to black, then restore */ 
		    (*t->linetype)(LT_BLACK);

		if (key->just == JLEFT) {
		    (*t->justify_text) (LEFT);
		    (*t->put_text) (xl + key_text_left, yl, this_plot->title);
		} else {
		    if ((*t->justify_text) (RIGHT)) {
			(*t->put_text) (xl + key_text_right, yl, this_plot->title);
		    } else {
			int x = xl + key_text_right - (t->h_char) * strlen(this_plot->title);
			if (key->hpos == TOUT || key->vpos == TUNDER ||	/* HBB 990327 */
			    i_inrange(x, xleft, xright))
			    (*t->put_text) (x, yl, this_plot->title);
		    }
		}
		/* EAM - restore plot line type */
		    (*t->linetype)(this_plot->lp_properties.l_type); 

		/* draw sample depending on bits set in plot_style */
#if USE_ULIG_FILLEDBOXES
		if (this_plot->plot_style == BOXES && *t->fillbox) {
		    int style;
		    struct fill_style_type *fs = &this_plot->fill_properties;
		    switch(fs->fillstyle) {
			case FS_SOLID:
			    style = (fs->filldensity << 4) + FS_SOLID;
			    break;
			case FS_PATTERN:
			    style = (fs->fillpattern << 4) + FS_PATTERN;
			    break;
			default: style = FS_EMPTY;
		    }
		    (*t->fillbox)( style,
				  xl + key_sample_left, yl - key_entry_height/4, 
				  key_sample_right - key_sample_left,
				  key_entry_height/2);
		    if (fs->fillstyle != FS_EMPTY && fs->border_linetype != LT_UNDEFINED)
			(*t->linetype)(fs->border_linetype);
		    if (fs->border_linetype != LT_NODRAW) {
			(*t->move)  (xl + key_sample_left,  yl - key_entry_height/4);
			(*t->vector)(xl + key_sample_right, yl - key_entry_height/4);
			(*t->vector)(xl + key_sample_right, yl + key_entry_height/4);
			(*t->vector)(xl + key_sample_left,  yl + key_entry_height/4);
			(*t->vector)(xl + key_sample_left,  yl - key_entry_height/4);
		    }
		    if (fs->fillstyle != FS_EMPTY && fs->border_linetype != LT_UNDEFINED)
			(*t->linetype)(this_plot->lp_properties.l_type);
		} else
#endif
		if ((this_plot->plot_style & PLOT_STYLE_HAS_LINE)
		    || ((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
			&& this_plot->plot_type == DATA)) {
		    /* errors for data plots only */
		    (*t->move) (xl + key_sample_left, yl);
		    (*t->vector) (xl + key_sample_right, yl);
		} 
		/* oops - doing the point sample now breaks postscript
		 * terminal for example, which changes current line style
		 * when drawing a point, but does not restore it.
		 * We simply draw the point sample after plotting
		 */

		if ((this_plot->plot_type == DATA)
		    && (this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
		    && (bar_size > 0.0)) {
		    (*t->move) (xl + key_sample_left, yl + ERRORBARTIC);
		    (*t->vector) (xl + key_sample_left, yl - ERRORBARTIC);
		    (*t->move) (xl + key_sample_right, yl + ERRORBARTIC);
		    (*t->vector) (xl + key_sample_right, yl - ERRORBARTIC);
		}
	    }
	ignore_enhanced_text = 0;
	}

	/* and now the curves, plus any special key requirements */
	/* be sure to draw all lines before drawing any points */

	switch (this_plot->plot_style) {
	    /*{{{  IMPULSE */
	case IMPULSES:
	    plot_impulses(this_plot, X_AXIS.term_zero, Y_AXIS.term_zero);
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
	  plot_boxes(this_plot, Y_AXIS.term_zero);
	    break;
	    /*}}} */
	    /*{{{  BOXERROR (falls through to) */
	case BOXERROR:
	    plot_bars(this_plot);
	    /* no break */

	    /*}}} */
	    /*{{{  BOXES */
	case BOXES:
          plot_boxes(this_plot, Y_AXIS.term_zero);
	    break;
	    /*}}} */
#ifdef PM3D
	    /*{{{  FILLEDCURVES */
	case FILLEDCURVES:
	    plot_filledcurves(this_plot);
	    break;
	    /*}}} */
#endif
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
#ifdef PM3D
	case PM3DSURFACE:
	    fprintf(stderr, "** warning: can't use pm3d for 2d plots\n");
	    break;
#endif
	}


	if (localkey && this_plot->title) {
	    /* we deferred point sample until now */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		(*t->point) (xl + key_point_offset, yl, this_plot->lp_properties.p_type);

	    if (key_count >= key_rows) {
		yl = yl_ref;
		xl += key_col_wth;
		key_count = 0;
	    } else
		yl = yl - key_entry_height;
	}
    }

/* DRAW TICS AND GRID */
    if (grid_layer == 1)
	place_grid();

/* PLACE LABELS */
    place_labels( first_label, 1 );

/* PLACE ARROWS */
    place_arrows( 1 );

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
		if (!inrange(plot->points[i].x, X_AXIS.min, X_AXIS.max))
		    continue;
		x = map_x(plot->points[i].x);
		y = plot->points[i].y;
		cliptorange(y, Y_AXIS.min, Y_AXIS.max);
		y = map_y(y);

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

/* plot_filledcurves:
 * Plot FILLED curves.
 * Only compiled in for PM3D because term->filled_polygon() is required.
 * pm 8.9.2001 (main routine); pm 5.1.2002 (full support for options)
 */
#ifdef PM3D

/* finalize and draw the filled curve */
static void
finish_filled_curve(points, corners, filledcurves_options)
    int points;
    gpiPoint *corners;
    filledcurves_opts *filledcurves_options;
{
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
		points += 2;
		break;
	case FILLEDCURVES_ATX2:
		corners[points].x   = 
		corners[points+1].x = map_x(filledcurves_options->at);
		    /* should be mapping real x2axis/graph/screen => screen */
		corners[points].y   = corners[points-1].y;
		corners[points+1].y = corners[0].y;
		points += 2;
		break;
	case FILLEDCURVES_ATY1:
		corners[points].y   = 
		corners[points+1].y = map_y(filledcurves_options->at);
		    /* should be mapping real y1axis/graph/screen => screen */
		corners[points].x   = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		points += 2;
		break;
	case FILLEDCURVES_ATY2:
		corners[points].y   = 
		corners[points+1].y = map_y(filledcurves_options->at);
		    /* should be mapping real y2axis/graph/screen => screen */
		corners[points].x   = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		points += 2;
		break;
	case FILLEDCURVES_ATXY:
		corners[points].x = map_x(filledcurves_options->at);
		    /* should be mapping real x1axis/graph/screen => screen */
		corners[points].y = map_y(filledcurves_options->aty);
		    /* should be mapping real y1axis/graph/screen => screen */
		points++;
		break;
	default: /* the polygon is closed by default */
		break;
    }
    /* now plot the filled polygon */
    term->filled_polygon(points, corners);
#if 0
    { /* for debugging purposes */
    int i;
    fprintf(stderr, "List of %i corners:\n", points);
    for (i=0; i<points; i++)
	fprintf(stderr, "%2i: %3i,%3i | ", i, corners[i].x, corners[i].y);
    fprintf(stderr, "\n");
    }
#endif
}


static void
plot_filledcurves(plot)
struct curve_points *plot;
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
		    /* vector(x,y) */
		    corners[points].x = x;
		    corners[points++].y = y;
		} else if (prev == OUTRANGE) {
		    /* from outrange to inrange */
		    if (!clip_lines1) {
			finish_filled_curve(points, corners, &plot->filledcurves_options);
			points = 0;
			/* move(x,y) */
			corners[points].x = x;
			corners[points++].y = y;
		    } else {
			finish_filled_curve(points, corners, &plot->filledcurves_options);
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
		    finish_filled_curve(points, corners, &plot->filledcurves_options);
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
		if (prev == INRANGE) {
		    /* from inrange to outrange */
		    if (clip_lines1) {
			edge_intersect(plot->points, i, &ex, &ey);
			/* vector(map_x(ex),map_y(ey)); */
			corners[points].x = map_x(ex);
			corners[points++].y = map_y(ey);
		    }
		} else if (prev == OUTRANGE) {
		    /* from outrange to outrange */
		    if (clip_lines2) {
			if (two_edge_intersect(plot->points, i, lx, ly)) {
			    finish_filled_curve(points, corners, &plot->filledcurves_options);
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
	default:		/* just a safety */
	case UNDEFINED:{
		break;
	    }
	}
	prev = plot->points[i].type;
    }
    finish_filled_curve(points, corners, &plot->filledcurves_options);
}
#endif /* plot_filledcurves() only ifdef PM3D */

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

/* HBB 20010625: replaced homegrown bubblesort in plot_histeps() by
 * call of standard routine qsort(). Need to tell the compare function
 * about the plotted dataset via this file scope variable: */
static struct curve_points *histeps_current_plot;

/* NOTE: I'd have made the comp.function 'static', but the HP-sUX gcc
 * bug seems to forbid that :-( */
int
histeps_compare(p1, p2)
    SORTFUNC_ARGS p1;
    SORTFUNC_ARGS p2;
{
    double x1=histeps_current_plot->points[*(int *)p1].x;
    double x2=histeps_current_plot->points[*(int *)p2].x;
    
    if (x1 < x2)
	return -1;
    else
	return (x2 > x1);
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
    if (gl == NULL)
	return;

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
histeps_vertical(cur_x, cur_y, x, y1, y2)
    int *cur_x, *cur_y;		/* keeps track of "cursor" position */
    double x, y1, y2;		/* coordinates of vertical line */
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
histeps_horizontal(cur_x, cur_y, x1, x2, y)
    int *cur_x, *cur_y;		/* keeps track of "cursor" position */
    double x1, x2, y;		/* coordinates of vertical line */
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
#if USE_ULIG_RELATIVE_BOXWIDTH
                        if (! boxwidth_is_absolute) /* consider relative boxwidth (ULIG) */
			    dxl = (plot->points[i-1].x - plot->points[i].x) * boxwidth / 2.0;
                        else
#endif /* USE_RELATIVE_BOXWIDTH */
			    dxl = (plot->points[i-1].x - plot->points[i].x) / 2.0;
		    else
			dxl = 0.0;

		    if (i < plot->p_count - 1) {
			if (plot->points[i + 1].type != UNDEFINED)
#if USE_ULIG_RELATIVE_BOXWIDTH
                            if (! boxwidth_is_absolute) /* consider relative boxwidth (ULIG) */
			        dxr = (plot->points[i+1].x - plot->points[i].x) * boxwidth / 2.0;
                            else
#endif /* USE_RELATIVE_BOXWIDTH */
			        dxr = (plot->points[i+1].x - plot->points[i].x) / 2.0;
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
		cliptorange(dyt, Y_AXIS.min, Y_AXIS.max);
		cliptorange(dxr, X_AXIS.min, X_AXIS.max);
		cliptorange(dxl, X_AXIS.min, X_AXIS.max);

		xl = map_x(dxl);
		xr = map_x(dxr);
		yt = map_y(dyt);

#if USE_ULIG_FILLEDBOXES
		if ((plot->fill_properties.fillstyle != FS_EMPTY) && t->fillbox) {
                    int x, y, w, h;
                    int fillpar, style;
		    
                    x = xl;
                    y = xaxis_y;
                    w = xr - xl + 1;
                    h = yt - xaxis_y + 1;
                    /* avoid negative width/height */
                    if( w <= 0 ) { 
			x = xr; 
			w = xl - xr + 1;
		    }
                    if( h <= 0 ) {
			y = yt;
			h = xaxis_y - yt + 1;
		    }

                    /* squeeze all fill information into the old style
                     * parameter.  The terminal driver knows how to
                     * extract the information.  We assume that the
                     * style (int) has only 16 bit, therefore we take
                     * 4 bits for the style and allow 12 bits for the
                     * corresponding fill parameter.  This limits the
                     * number of styles to 16 and the fill parameter's
                     * values to the range 0...4095, which seems
                     * acceptable. */
                    switch( plot->fill_properties.fillstyle ) {
                    case FS_SOLID:
			fillpar = plot->fill_properties.filldensity;
			style = ((fillpar & 0xfff) << 4) + FS_SOLID;
                        break;
                    case FS_PATTERN:
			fillpar = plot->fill_properties.fillpattern;
			style = ((fillpar & 0xfff) << 4) + FS_PATTERN;
                        break;
                    default:
			/* style == 0 or unknown --> solid fill with
                         * background color */
			fillpar = 0;
			style = FS_EMPTY;
                    }

                    (*t->fillbox) (style, x, y, w, h);

		    /* FIXME EAM - Is this still correct??? */
		    if (strcmp(t->name, "fig") == 0) break;

		    if (plot->fill_properties.border_linetype == LT_NODRAW)
			break;
		    if (plot->fill_properties.border_linetype != LT_UNDEFINED)
			(*t->linetype)(plot->fill_properties.border_linetype);
		}
#endif /* USE_ULIG_FILLEDBOXES */

		(*t->move) (xl, xaxis_y);
		(*t->vector) (xl, yt);
		(*t->vector) (xr, yt);
		(*t->vector) (xr, xaxis_y);
		(*t->vector) (xl, xaxis_y);
#if USE_ULIG_FILLEDBOXES
                if( t->fillbox && 
		    plot->fill_properties.border_linetype != LT_UNDEFINED)
		    (*t->linetype)(plot->lp_properties.l_type);
#endif
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
    struct coordinate points[2];
    double ex, ey;
    double lx[2], ly[2];

    /* Only necessary once because all arrows equal */
    term_apply_lp_properties(&(plot->arrow_properties.lp_properties));
    apply_head_properties(&(plot->arrow_properties));

    for (i = 0; i < plot->p_count; i++) {
	points[0] = plot->points[i];
	points[1].x = plot->points[i].xhigh;
	points[1].y = plot->points[i].yhigh;
	if (inrange(points[1].x, X_AXIS.min, X_AXIS.max) && inrange(points[1].y, Y_AXIS.min, Y_AXIS.max)) {
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
		    if (plot->arrow_properties.head == 2)
			(*t->arrow) (x1, y1, x2, y2, 1);
		    else
			(*t->arrow) (x1, y1, x2, y2, plot->arrow_properties.head);
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
		    if (plot->arrow_properties.head == 2)
			(*t->arrow) (x2, y2, x1, y1, 1);
		    else
			(*t->arrow) (x2, y2, x1, y1, plot->arrow_properties.head);
		}
	    } else if (points[0].type == OUTRANGE) {
		/* from outrange to outrange */
		if (clip_lines2) {
		    if (two_edge_intersect(points, 1, lx, ly)) {
			x1 = map_x(lx[0]);
			y1 = map_y(ly[0]);
			x2 = map_x(lx[1]);
			y2 = map_y(ly[1]);
			(*t->arrow) (x1, y1, x2, y2, FALSE);
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
	if (!inrange(x, X_AXIS.min, X_AXIS.max))
	    continue;
	xM = map_x(x);

	/* find low and high points of bar, and check yrange */
	yhigh = plot->points[i].yhigh;
	ylow = plot->points[i].ylow;
	yclose = plot->points[i].z;
	yopen = plot->points[i].y;

	high_inrange = inrange(yhigh, axis_array[y_axis].min, axis_array[y_axis].max);
	low_inrange = inrange(ylow, axis_array[y_axis].min, axis_array[y_axis].max);

	/* HBB 20010928: To make code match the documentation, ensure
	 * yhigh is actually higher than ylow */
	if (yhigh < ylow) {
	    double temp = ylow;
	    ylow = yhigh;
	    yhigh = temp;
	}

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

#ifdef USE_ULIG_FILLEDBOXES
	/* EAM Sep 2001 use term->fillbox() code if present */
	if ((plot->fill_properties.fillstyle != FS_EMPTY) && (term->fillbox)) {
	    int ymin, ymax, fillpar, style;
	    /* This code duplicated in plot_boxes() */
                    switch( plot->fill_properties.fillstyle ) {
                    case FS_SOLID:
                        fillpar = plot->fill_properties.filldensity;
                        break;
                    case FS_PATTERN:
                        fillpar = plot->fill_properties.fillpattern;
                        break;
		    case FS_EMPTY:
                    default:
                        fillpar = 0;
                    }
	    /* Set style parameter (horrible bit-packing hack) */
                    style = ((fillpar & 0xfff) << 4) 
			  + (plot->fill_properties.fillstyle & 0xf);
	    if (map_y(yopen) < map_y(yclose)) {
		ymin = map_y(yopen); ymax = map_y(yclose);
	    } else {
		ymax = map_y(yopen); ymin = map_y(yclose);
	    }
	    term->fillbox( style,
		    (unsigned int)(xM - bar_size * tic),
		    (unsigned int)(ymin),
		    (unsigned int)(2 * bar_size * tic),
		    (unsigned int)(ymax-ymin)  );
	}
#endif

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

	    *ex = X_AXIS.min;
	    return;
	}
	/* obviously oy is -VERYLARGE and ox != -VERYLARGE */
	*ey = Y_AXIS.min;
	return;
    }
    /*
     * Can't have case (ix == ox && iy == oy) as one point
     * is INRANGE and one point is OUTRANGE.
     */
    if (iy == oy) {
	/* horizontal line */
	/* assume inrange(iy, Y_AXIS.min, Y_AXIS.max) */
	*ey = iy;		/* == oy */

	if (inrange(X_AXIS.max, ix, ox))
	    *ex = X_AXIS.max;
	else if (inrange(X_AXIS.min, ix, ox))
	    *ex = X_AXIS.min;
	else {
	    graph_error("error in edge_intersect");
	}
	return;
    } else if (ix == ox) {
	/* vertical line */
	/* assume inrange(ix, X_AXIS.min, X_AXIS.max) */
	*ex = ix;		/* == ox */

	if (inrange(Y_AXIS.max, iy, oy))
	    *ey = Y_AXIS.max;
	else if (inrange(Y_AXIS.min, iy, oy))
	    *ey = Y_AXIS.min;
	else {
	    graph_error("error in edge_intersect");
	}
	return;
    }
    /* slanted line of some kind */

    /* does it intersect Y_AXIS.min edge */
    if (inrange(Y_AXIS.min, iy, oy) && Y_AXIS.min != iy && Y_AXIS.min != oy) {
	x = ix + (Y_AXIS.min - iy) * ((ox - ix) / (oy - iy));
	if (inrange(x, X_AXIS.min, X_AXIS.max)) {
	    *ex = x;
	    *ey = Y_AXIS.min;
	    return;		/* yes */
	}
    }
    /* does it intersect Y_AXIS.max edge */
    if (inrange(Y_AXIS.max, iy, oy) && Y_AXIS.max != iy && Y_AXIS.max != oy) {
	x = ix + (Y_AXIS.max - iy) * ((ox - ix) / (oy - iy));
	if (inrange(x, X_AXIS.min, X_AXIS.max)) {
	    *ex = x;
	    *ey = Y_AXIS.max;
	    return;		/* yes */
	}
    }
    /* does it intersect X_AXIS.min edge */
    if (inrange(X_AXIS.min, ix, ox) && X_AXIS.min != ix && X_AXIS.min != ox) {
	y = iy + (X_AXIS.min - ix) * ((oy - iy) / (ox - ix));
	if (inrange(y, Y_AXIS.min, Y_AXIS.max)) {
	    *ex = X_AXIS.min;
	    *ey = y;
	    return;
	}
    }
    /* does it intersect X_AXIS.max edge */
    if (inrange(X_AXIS.max, ix, ox) && X_AXIS.max != ix && X_AXIS.max != ox) {
	y = iy + (X_AXIS.max - ix) * ((oy - iy) / (ox - ix));
	if (inrange(y, Y_AXIS.min, Y_AXIS.max)) {
	    *ex = X_AXIS.max;
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
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	if (inrange(ay, Y_AXIS.min, Y_AXIS.max)) {
	    *ey = ay;
	    if (ax > X_AXIS.max)
		*ex = X_AXIS.max;
	    else		/* x < X_AXIS.min */
		*ex = X_AXIS.min;
	} else {
	    *ex = bx;
	    if (ay > Y_AXIS.max)
		*ey = Y_AXIS.max;
	    else		/* y < Y_AXIS.min */
		*ey = Y_AXIS.min;
	}
    } else {			/* from INRANGE to OUTRANGE */
	if (inrange(bx, X_AXIS.min, X_AXIS.max)) {
	    *ex = bx;
	    if (by > Y_AXIS.max)
		*ey = Y_AXIS.max;
	    else		/* y < Y_AXIS.min */
		*ey = Y_AXIS.min;
	} else {
	    *ey = ay;
	    if (bx > X_AXIS.max)
		*ex = X_AXIS.max;
	    else		/* x < X_AXIS.min */
		*ex = X_AXIS.min;
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
    /* global X_AXIS.min, X_AXIS.max, Y_AXIS.min, X_AXIS.max */
    double ax = points[i - 1].x;
    double ay = points[i - 1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	if (inrange(ax, X_AXIS.min, X_AXIS.max)) {
	    *ex = ax;
	    if (ay > Y_AXIS.max)
		*ey = Y_AXIS.max;
	    else		/* y < Y_AXIS.min */
		*ey = Y_AXIS.min;
	} else {
	    *ey = by;
	    if (bx > X_AXIS.max)
		*ex = X_AXIS.max;
	    else		/* x < X_AXIS.min */
		*ex = X_AXIS.min;
	}
    } else {			/* from INRANGE to OUTRANGE */
	if (inrange(by, Y_AXIS.min, Y_AXIS.max)) {
	    *ey = by;
	    if (bx > X_AXIS.max)
		*ex = X_AXIS.max;
	    else		/* x < X_AXIS.min */
		*ex = X_AXIS.min;
	} else {
	    *ex = ax;
	    if (by > Y_AXIS.max)
		*ey = Y_AXIS.max;
	    else		/* y < Y_AXIS.min */
		*ey = Y_AXIS.min;
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
    struct coordinate GPHUGE *points; /* the points array */
    int i;			/* line segment from point i-1 to point i */
    double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
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
two_edge_intersect_fsteps(points, i, lx, ly)
    struct coordinate GPHUGE *points; /* the points array */
    int i;			/* line segment from point i-1 to point i */
    double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
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
two_edge_intersect(points, i, lx, ly)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
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
	if (ix > X_AXIS.max && inrange(iy, Y_AXIS.min, Y_AXIS.max)) {
	    lx[0] = X_AXIS.min;
	    ly[0] = iy;

	    lx[1] = X_AXIS.max;
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
	if (iy > Y_AXIS.max && inrange(ix, X_AXIS.min, X_AXIS.max)) {
	    lx[0] = ix;
	    ly[0] = Y_AXIS.min;

	    lx[1] = ix;
	    ly[1] = Y_AXIS.max;
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
#if 0
	    fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
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

#if 0
	fprintf(stderr, "(%g %g) -> (%g %g)", lx[0], ly[0], lx[1], ly[1]);
#endif
	return (TRUE);
    }
    return (FALSE);
}

/* HBB 20010118: all the *_callback() functions made non-static. This
 * is necessary to work around a bug in HP's assembler shipped with
 * HP-UX 10 and higher, if GCC tries to use it */

/* display a x-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
void
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

    (void) axis;		/* avoid "unused parameter" warning */

    if (grid.l_type > L_TYPE_NODRAW) {
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
    if (text) {
        /* User-specified different color for the tics text */
        if (axis_array[axis].ticdef.textcolor.lt != TC_DEFAULT)
            apply_textcolor(&(axis_array[axis].ticdef.textcolor), t);
	write_multiline(x, tic_text, text, tic_hjust, tic_vjust, rotate_tics, 
			axis_array[axis].ticdef.font);
	term_apply_lp_properties(&border_lp);	/* reset to border linetype */
    }
}

/* display a y-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
void
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

    (void) axis;		/* avoid "unused parameter" warning */

    if (grid.l_type > L_TYPE_NODRAW) {
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
    if (text) {
        /* User-specified different color for the tics text */
        if (axis_array[axis].ticdef.textcolor.lt != TC_DEFAULT)
            apply_textcolor(&(axis_array[axis].ticdef.textcolor), t);
	write_multiline(tic_text, y, text, tic_hjust, tic_vjust, rotate_tics, 
			axis_array[axis].ticdef.font);
	term_apply_lp_properties(&border_lp);	/* reset to border linetype */
    }
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


/*{{{  map_position, wrapper, which maps double to int */
void
map_position(pos, x, y, what)
struct position *pos;
unsigned int *x, *y;
const char *what;
{
    double xx, yy;
    map_position_double(pos, &xx, &yy, what);
    *x = xx;
    *y = yy;
}

/*}}} */

/*{{{  map_position_double */
static void
map_position_double(pos, x, y, what)
    struct position *pos;
    double *x, *y;
    const char *what;
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
	    *x = xleft + pos->x * (xright - xleft);
	    break;
	}
    case screen:
	{
	    register struct termentry *t = term;
	    /* HBB 20000914: Off-by-one bug. Max. allowable result is
	     * t->xmax - 1, not t->xmax ! */
	    *x = pos->x * (t->xmax - 1);
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
	    *y = ybot + pos->y * (ytop - ybot);
	    break;
	}
    case screen:
	{
	    register struct termentry *t = term;
	    /* HBB 20000914: Off-by-one bug. Max. allowable result is
	     * t->ymax - 1, not t->ymax ! */
	    *y = pos->y * (t->ymax -1);
	    break;
	}
    }
    *x += 0.5;
    *y += 0.5;
}

/*}}} */

/*{{{  map_position_r */
static void
map_position_r(pos, x, y, what)
struct position *pos;
double *x, *y;
const char *what;
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
	    *x = pos->x * (xright - xleft);
	    break;
	}
    case screen:
	{
	    register struct termentry *t = term;
	    *x = pos->x * (t->xmax - 1);
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
	    *y = pos->y * (ytop - ybot);
	    return;
	}
    case screen:
	{
	    register struct termentry *t = term;
	    /* HBB 20000914: Off-by-one bug. Max. allowable result is
	     * t->ymax - 1, not t->ymax ! */
	    *y = pos->y * (t->ymax -1);
	    return;
	}
    }
}
/*}}} */






