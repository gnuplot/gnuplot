#ifndef lint
static char *RCSid() { return RCSid("$Id: graph3d.c,v 1.245 2011/04/01 22:35:45 sfeam Exp $"); }
#endif

/* GNUPLOT - graph3d.c */

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


/*
 * AUTHORS
 *
 *   Original Software:
 *       Gershon Elber and many others.
 *
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * 3.6 - split graph3d.c into graph3d.c (graph),
 *                            util3d.c (intersections, etc)
 *                            hidden3d.c (hidden-line removal code)
 *
 */

#include "graph3d.h"

#include "alloc.h"
#include "axis.h"
#include "gadgets.h"
#include "hidden3d.h"
#include "misc.h"
#include "term_api.h"
#include "util3d.h"
#include "util.h"

#include "pm3d.h"
#include "plot3d.h"
#include "color.h"

#include "plot.h"

/* HBB NEW 20040311: PM3D did already split up grid drawing into two
 * parts, one before, the other after drawing the main surfaces, as a
 * poor-man's depth-sorting algorithm.  Make this independent of
 * PM3D. Turn the new option on by default. */
#define USE_GRID_LAYERS 1


static int p_height;
static int p_width;		/* pointsize * t->h_tic */
static int key_entry_height;	/* bigger of t->v_size, pointsize*t->v_tick */

/* is contouring wanted ? */
t_contour_placement draw_contour = CONTOUR_NONE;
/* different linestyles are used for contours when set */
TBOOLEAN label_contours = TRUE;

/* Want to draw surfaces? FALSE mainly useful in contouring mode */
TBOOLEAN draw_surface = TRUE;

/* Was hidden3d display selected by user? */
TBOOLEAN hidden3d = FALSE;
int hidden3d_layer = LAYER_BACK;

/* Rotation and scale of the 3d view, as controlled by 'set view': */
float surface_rot_z = 30.0;
float surface_rot_x = 60.0;
float surface_scale = 1.0;
float surface_zscale = 1.0;
float surface_lscale = 0.0;

/* Set by 'set view map': */
int splot_map = FALSE;

/* position of the base plane, as given by 'set ticslevel' or 'set xyplane' */
t_xyplane xyplane = { 0.5, FALSE };

/* 'set isosamples' settings */
int iso_samples_1 = ISO_SAMPLES;
int iso_samples_2 = ISO_SAMPLES;

double xscale3d, yscale3d, zscale3d;
double xcenter3d = 0.0;
double ycenter3d = 0.0;
double zcenter3d = 0.0;

typedef enum { ALLGRID, FRONTGRID, BACKGRID, BORDERONLY } WHICHGRID;

static void plot3d_impulses __PROTO((struct surface_points * plot));
static void plot3d_lines __PROTO((struct surface_points * plot));
static void plot3d_points __PROTO((struct surface_points * plot, /* FIXME PM3D: */ int p_type));
static void plot3d_vectors __PROTO((struct surface_points * plot));
/* no pm3d for impulses */
static void plot3d_lines_pm3d __PROTO((struct surface_points * plot));
static void get_surface_cbminmax __PROTO((struct surface_points *plot, double *cbmin, double *cbmax));
static void cntr3d_impulses __PROTO((struct gnuplot_contours * cntr,
				     struct lp_style_type * lp));
static void cntr3d_lines __PROTO((struct gnuplot_contours * cntr,
				  struct lp_style_type * lp));
/* HBB UNUSED 20031219 */
/* static void cntr3d_linespoints __PROTO((struct gnuplot_contours * cntr, */
/* 					struct lp_style_type * lp)); */
static void cntr3d_points __PROTO((struct gnuplot_contours * cntr,
				   struct lp_style_type * lp));
static void check_corner_height __PROTO((struct coordinate GPHUGE * point,
					 double height[2][2], double depth[2][2]));
static void setup_3d_box_corners __PROTO((void));
static void draw_3d_graphbox __PROTO((struct surface_points * plot,
				      int plot_count,
				      WHICHGRID whichgrid, int current_layer));
/* HBB 20010118: these should be static, but can't --- HP-UX assembler bug */
void xtick_callback __PROTO((AXIS_INDEX, double place, char *text,
			     struct lp_style_type grid, struct ticmark *userlabels));
void ytick_callback __PROTO((AXIS_INDEX, double place, char *text,
			     struct lp_style_type grid, struct ticmark *userlabels));
void ztick_callback __PROTO((AXIS_INDEX, double place, char *text,
			     struct lp_style_type grid, struct ticmark *userlabels));

static int find_maxl_cntr __PROTO((struct gnuplot_contours * contours, int *count));
static int find_maxl_keys3d __PROTO((struct surface_points *plots, int count, int *kcnt));
static void boundary3d __PROTO((struct surface_points * plots, int count));

/* put entries in the key */
static void key_sample_line __PROTO((int xl, int yl));
static void key_sample_point __PROTO((int xl, int yl, int pointtype));
static void key_sample_line_pm3d __PROTO((struct surface_points *plot, int xl, int yl));
static void key_sample_point_pm3d __PROTO((struct surface_points *plot, int xl, int yl, int pointtype));
static TBOOLEAN can_pm3d = FALSE;
static void key_text __PROTO((int xl, int yl, char *text));
static void check_for_variable_color __PROTO((struct surface_points *plot, struct coordinate *point));

static TBOOLEAN get_arrow3d __PROTO((struct arrow_def*, int*, int*, int*, int*));
static void place_arrows3d __PROTO((int));
static void place_labels3d __PROTO((struct text_label * listhead, int layer));
static int map3d_getposition __PROTO((struct position* pos, const char* what, double* xpos, double* ypos, double* zpos));

# define f_max(a,b) GPMAX((a),(b))
# define f_min(a,b) GPMIN((a),(b))
# define i_inrange(z,a,b) inrange((z),(a),(b))

#define apx_eq(x,y) (fabs(x-y) < 0.001)
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define SQR(x) ((x) * (x))

/* Define the boundary of the plot
 * These are computed at each call to do_plot, and are constant over
 * the period of one do_plot. They actually only change when the term
 * type changes and when the 'set size' factors change.
 */

int xmiddle, ymiddle, xscaler, yscaler;
static int ptitl_cnt;
static int max_ptitl_len;
static int titlelin;
static int key_sample_width, key_rows, key_cols, key_col_wth, yl_ref;
static double ktitle_lines = 0;


/* Boundary and scale factors, in user coordinates */

/* There are several z's to take into account - I hope I get these
 * right !
 *
 * ceiling_z is the highest z in use
 * floor_z   is the lowest z in use
 * base_z is the z of the base
 * min3d_z is the lowest z of the graph area
 * max3d_z is the highest z of the graph area
 *
 * ceiling_z is either max3d_z or base_z, and similarly for floor_z
 * There should be no part of graph drawn outside
 * min3d_z:max3d_z  - apart from arrows, perhaps
 */

double floor_z;
double ceiling_z, base_z;	/* made exportable for PM3D */

transform_matrix trans_mat;

/* x and y input range endpoints where the three axes are to be
 * displayed (left, front-left, and front-right edges of the cube) */
static double xaxis_y, yaxis_x, zaxis_x, zaxis_y;

/* ... and the same for the back, right, and front corners */
static double back_x, back_y;
static double right_x, right_y;
static double front_x, front_y;

#ifdef USE_MOUSE
int axis3d_o_x, axis3d_o_y, axis3d_x_dx, axis3d_x_dy, axis3d_y_dx, axis3d_y_dy;
#endif

/* the penalty for convenience of using tic_gen to make callbacks
 * to tick routines is that we cannot pass parameters very easily.
 * We communicate with the tick_callbacks using static variables
 */

/* unit vector (terminal coords) */
static double tic_unitx, tic_unity, tic_unitz;

/* calculate the number and max-width of the keys for an splot.
 * Note that a blank line is issued after each set of contours
 */
static int
find_maxl_keys3d(struct surface_points *plots, int count, int *kcnt)
{
    int mlen, len, surf, cnt;
    struct surface_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (surf = 0; surf < count; this_plot = this_plot->next_sp, surf++) {

	/* we draw a main entry if there is one, and we are
	 * drawing either surface, or unlabelled contours
	 */
	if (this_plot->title && *this_plot->title && !this_plot->title_is_suppressed) {
	    ++cnt;
	    len = estimate_strlen(this_plot->title);
	    if (len > mlen)
		mlen = len;
	}
	if (draw_contour && label_contours && this_plot->contours != NULL) {
	    len = find_maxl_cntr(this_plot->contours, &cnt);
	    if (len > mlen)
		mlen = len;
	}
    }

    if (kcnt != NULL)
	*kcnt = cnt;
    return (mlen);
}

static int
find_maxl_cntr(struct gnuplot_contours *contours, int *count)
{
    int cnt;
    int mlen, len;
    struct gnuplot_contours *cntrs = contours;

    mlen = cnt = 0;
    while (cntrs) {
	if (label_contours && cntrs->isNewLevel) {
	    len = estimate_strlen(cntrs->label)
		- strspn(cntrs->label," ");
	    if (len)
		cnt++;
	    if (len > mlen)
		mlen = len;
	}
	cntrs = cntrs->next;
    }
    *count += cnt;
    return (mlen);
}


/* borders of plotting area */
/* computed once on every call to do_plot */
static void
boundary3d(struct surface_points *plots, int count)
{
    legend_key *key = &keyT;
    struct termentry *t = term;
    int ytlen, i;

    titlelin = 0;

    p_height = pointsize * t->v_tic;
    p_width = pointsize * t->h_tic;
    if (key->swidth >= 0)
	key_sample_width = key->swidth * t->h_char + pointsize * t->h_tic;
    else
	key_sample_width = 0;
    key_entry_height = pointsize * t->v_tic * 1.25 * key->vert_factor;
    if (key_entry_height < t->v_char) {
	/* is this reasonable ? */
	key_entry_height = t->v_char * key->vert_factor;
    }
    /* count max_len key and number keys (plot-titles and contour labels) with len > 0 */
    max_ptitl_len = find_maxl_keys3d(plots, count, &ptitl_cnt);
    ytlen = label_width(key->title, &i) - (key->swidth + 2);
    ktitle_lines = i;
    if (ytlen > max_ptitl_len)
	max_ptitl_len = ytlen;
    key_col_wth = (max_ptitl_len + 4) * t->h_char + key_sample_width;

    if (lmargin.scalex == screen)
	plot_bounds.xleft = lmargin.x * (float)t->xmax + 0.5;
    else if (lmargin.x >= 0)
	plot_bounds.xleft = lmargin.x * (float)t->h_char + 0.5;
    else
	plot_bounds.xleft = t->h_char * 2 + t->h_tic;

    if (rmargin.scalex == screen)
	plot_bounds.xright = rmargin.x * (float)t->xmax + 0.5;
    else /* No tic label on the right side, so ignore rmargin */
	plot_bounds.xright = xsize * t->xmax - t->h_char * 2 - t->h_tic;

    key_rows = ptitl_cnt;
    key_cols = 1;
    if (key->visible)
    if ((key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
	&& key->margin == GPKEY_BMARGIN) {
	if (ptitl_cnt > 0) {
	    /* calculate max no cols, limited by label-length */
	    key_cols = (int) (plot_bounds.xright - plot_bounds.xleft) 
		     / ((max_ptitl_len + 4) * t->h_char + key_sample_width);
	    /* HBB 991019: fix division by zero problem */
	    if (key_cols == 0)
		key_cols = 1;
	    key_rows = (int) ((ptitl_cnt - 1)/ key_cols) + 1;
	    /* now calculate actual no cols depending on no rows */
	    key_cols = (int) ((ptitl_cnt - 1)/ key_rows) + 1;
	    key_col_wth = (int) (plot_bounds.xright - plot_bounds.xleft) / key_cols;
	} else {
	    key_rows = key_cols = key_col_wth = 0;
	}
    }
    /* this should also consider the view and number of lines in
     * xformat || yformat || xlabel || ylabel */

    if (bmargin.scalex == screen)
	plot_bounds.ybot = bmargin.x * (float)t->ymax + 0.5;
    else if (splot_map && bmargin.x >= 0)
	plot_bounds.ybot = (float)t->v_char * bmargin.x;
    else
	plot_bounds.ybot = t->v_char * 2.5 + 1;

    if (key->visible)
    if (key_rows && (key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
	&& key->margin == GPKEY_BMARGIN)
	plot_bounds.ybot += key_rows * key_entry_height + ktitle_lines * t->v_char;

    if (title.text) {
	titlelin++;
	for (i = 0; i < strlen(title.text); i++) {
	    if (title.text[i] == '\\')
		titlelin++;
	}
    }

    if (tmargin.scalex == screen)
	plot_bounds.ytop = tmargin.x * (float)t->ymax + 0.5;
    else /* FIXME: Why no provision for tmargin in terms of character height? */
	plot_bounds.ytop = ysize * t->ymax - t->v_char * (titlelin + 1.5) - 1;

    if (key->visible)
    if (key->region == GPKEY_AUTO_INTERIOR_LRTBC
	|| ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN)
	    && key->margin == GPKEY_RMARGIN)) {
	/* calculate max no rows, limited by plot_bounds.ytop-plot_bounds.ybot */
	i = (int) (plot_bounds.ytop - plot_bounds.ybot) / t->v_char - 1 - ktitle_lines;
	/* HBB 20030321: div by 0 fix like above */
	if (i == 0)
	    i = 1;
	if (ptitl_cnt > i) {
	    key_cols = (int) ((ptitl_cnt - 1)/ i) + 1;
	    /* now calculate actual no rows depending on no cols */
	    key_rows = (int) ((ptitl_cnt - 1) / key_cols) + 1;
	}
    }
    if (key->visible)
    if ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN)
	&& key->margin == GPKEY_RMARGIN) {
	int key_width = key_col_wth * (key_cols - 1) + key_col_wth - 2 * t->h_char;
	if (rmargin.scalex != screen)
	    plot_bounds.xright -= key_width;
    }

    if (key->visible)
    if ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN)
	&& key->margin == GPKEY_LMARGIN) {
	int key_width = key_col_wth * (key_cols - 1) + key_col_wth - 2 * t->h_char;
	if (lmargin.scalex != screen)
	    plot_bounds.xleft += key_width;
    }

    if (!splot_map && aspect_ratio_3D > 0) {
	int height = (plot_bounds.ytop - plot_bounds.ybot);
	int width  = (plot_bounds.xright - plot_bounds.xleft);
	if (height > width) {
	    plot_bounds.ybot += (height-width)/2;
	    plot_bounds.ytop -= (height-width)/2;
	} else {
	    plot_bounds.xleft += (width-height)/2;
	    plot_bounds.xright -= (width-height)/2;
	}
    }

    if (lmargin.scalex != screen)
	plot_bounds.xleft += t->xmax * xoffset;
    if (rmargin.scalex != screen)
	plot_bounds.xright += t->xmax * xoffset;
    if (tmargin.scalex != screen)
	plot_bounds.ytop += t->ymax * yoffset;
    if (bmargin.scalex != screen)
	plot_bounds.ybot += t->ymax * yoffset;
    xmiddle = (plot_bounds.xright + plot_bounds.xleft) / 2;
    ymiddle = (plot_bounds.ytop + plot_bounds.ybot) / 2;

    
    /* HBB: Magic number alert! */
    xscaler = ((plot_bounds.xright - plot_bounds.xleft) * 4L) / 7L;
    yscaler = ((plot_bounds.ytop - plot_bounds.ybot) * 4L) / 7L;

    /* EAM Aug 2006 - Allow explicit control via set {}margin screen */
    if (tmargin.scalex == screen || bmargin.scalex == screen)
	yscaler = (plot_bounds.ytop - plot_bounds.ybot) / surface_scale;
    if (rmargin.scalex == screen || lmargin.scalex == screen)
	xscaler = (plot_bounds.xright - plot_bounds.xleft) / surface_scale;

    /* EAM Jul 2010 - prevent infinite loop or divide-by-zero if scaling is bad */
    if (yscaler == 0) yscaler = 1;
    if (xscaler == 0) xscaler = 1;

    /* HBB 20011011: 'set size {square|ratio}' for splots */
    if (splot_map && aspect_ratio != 0.0) {
	double current_aspect_ratio;

	if (aspect_ratio < 0 && (X_AXIS.max - X_AXIS.min) != 0.0) {
	    current_aspect_ratio = - aspect_ratio
		* fabs((Y_AXIS.max - Y_AXIS.min) /
		       (X_AXIS.max - X_AXIS.min));
	} else
	    current_aspect_ratio = aspect_ratio;

	/*{{{  set aspect ratio if valid and sensible */
	if (current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
	    double current = (double)yscaler / xscaler ;
	    double required = current_aspect_ratio * t->v_tic / t->h_tic;

	    if (current > required)
		/* too tall */
		yscaler = xscaler * required;
	    else
		/* too wide */
		xscaler = yscaler / required;
	}
    }

    /* Set default clipping */
    if (splot_map)
	clip_area = &plot_bounds;
    else if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;
}

static TBOOLEAN
get_arrow3d(
    struct arrow_def* arrow,
    int* sx, int* sy,
    int* ex, int* ey)
{
    map3d_position(&(arrow->start), sx, sy, "arrow");

    if (arrow->relative) {
	map3d_position_r(&(arrow->end), ex, ey, "arrow");
	*ex += *sx;
	*ey += *sy;
    } else {
	map3d_position(&(arrow->end), ex, ey, "arrow");
    }

    return TRUE;
}

static void
place_labels3d(struct text_label *listhead, int layer)
{
    struct text_label *this_label;
    int x, y;

    if (term->pointsize)
	(*term->pointsize)(pointsize);

    for (this_label = listhead;
	 this_label != NULL;
	 this_label = this_label->next) {

	if (this_label->layer != layer)
	    continue;

	/* HBB FIXME 20050428: conflicting types for &x,&y in these
	 * two routines.  One takes pointers to unsigned, the other to
	 * signed ints. */
	if (layer == LAYER_PLOTLABELS) {
	    double xx, yy;
	    map3d_xy_double(this_label->place.x, this_label->place.y,
		     this_label->place.z, &xx, &yy);
	    x = xx;
	    y = yy;
	    /* Only clip in 2D.   EAM - why? */
	    if (splot_map && clip_point(x, y))
		continue;
	} else
	    map3d_position(&this_label->place, &x, &y, "label");

	write_label(x, y, this_label);
    }
}

static void
place_arrows3d(int layer)
{
    struct arrow_def *this_arrow;
    BoundingBox *clip_save = clip_area;

    /* Allow arrows to run off the plot, so long as they are still on the canvas */
    if (term->flags & TERM_CAN_CLIP)
      clip_area = NULL;
    else
      clip_area = &canvas;

    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	int sx, sy, ex, ey;

	if (this_arrow->arrow_properties.layer != layer)
	    continue;
	if (get_arrow3d(this_arrow, &sx, &sy, &ex, &ey)) {
	    term_apply_lp_properties(&(this_arrow->arrow_properties.lp_properties));
	    apply_3dhead_properties(&(this_arrow->arrow_properties));
	    draw_clip_arrow(sx, sy, ex, ey, this_arrow->arrow_properties.head);
	} else {
	    FPRINTF((stderr,"place_arrows3d: skipping out-of-bounds arrow\n"));
	}
    }
    clip_area = clip_save;
}

/* we precalculate features of the key, to save lots of nested
 * ifs in code - x,y = user supplied or computed position of key
 * taken to be inner edge of a line sample
 */
static int key_sample_left;	/* offset from x for left of line sample */
static int key_sample_right;	/* offset from x for right of line sample */
static int key_point_offset;	/* offset from x for point sample */
static int key_text_left;	/* offset from x for left-justified text */
static int key_text_right;	/* offset from x for right-justified text */
static int key_size_left;	/* distance from x to left edge of box */
static int key_size_right;	/* distance from x to right edge of box */

void
do_3dplot(
    struct surface_points *plots,
    int pcount,			/* count of plots in linked list */
    int quick)		 	/* !=0 means plot only axes etc., for quick rotation */
{
    struct termentry *t = term;
    int surface;
    struct surface_points *this_plot = NULL;
    int xl, yl;
    transform_matrix mat;
    int key_count;
    legend_key *key = &keyT;
    TBOOLEAN pm3d_order_depth = 0;

    /* Initiate transformation matrix using the global view variables. */
    mat_rot_z(surface_rot_z, trans_mat);
    mat_rot_x(surface_rot_x, mat);
    mat_mult(trans_mat, trans_mat, mat);
    mat_scale(surface_scale / 2.0, surface_scale / 2.0, surface_scale / 2.0, mat);
    mat_mult(trans_mat, trans_mat, mat);

    /* The extrema need to be set even when a surface is not being
     * drawn.   Without this, gnuplot used to assume that the X and
     * Y axis started at zero.   -RKC
     */

    if (polar)
	graph_error("Cannot splot in polar coordinate system.");

    /* done in plot3d.c
     *    if (z_min3d == VERYLARGE || z_max3d == -VERYLARGE ||
     *      x_min3d == VERYLARGE || x_max3d == -VERYLARGE ||
     *      y_min3d == VERYLARGE || y_max3d == -VERYLARGE)
     *      graph_error("all points undefined!");
     */

    /* absolute or relative placement of xyplane along z */
    if (xyplane.absolute)
	base_z = AXIS_LOG_VALUE(0, xyplane.z);
    else
	base_z = Z_AXIS.min - (Z_AXIS.max - Z_AXIS.min) * xyplane.z;

    /* If we are to draw the bottom grid make sure zmin is updated properly. */
    if (X_AXIS.ticmode || Y_AXIS.ticmode || some_grid_selected()) {
	if (Z_AXIS.range_flags & RANGE_REVERSE) {
	    floor_z = GPMAX(Z_AXIS.min, base_z);
	    ceiling_z = GPMIN(Z_AXIS.max, base_z);
	} else {
	    floor_z = GPMIN(Z_AXIS.min, base_z);
	    ceiling_z = GPMAX(Z_AXIS.max, base_z);
	}
    } else {
	floor_z = Z_AXIS.min;
	ceiling_z = Z_AXIS.max;
    }

    /*  see comment accompanying similar tests of x_min/x_max and y_min/y_max
     *  in graphics.c:do_plot(), for history/rationale of these tests */
    if (X_AXIS.min == X_AXIS.max)
	graph_error("x_min3d should not equal x_max3d!");
    if (Y_AXIS.min == Y_AXIS.max)
	graph_error("y_min3d should not equal y_max3d!");
    if (Z_AXIS.min == Z_AXIS.max)
	graph_error("z_min3d should not equal z_max3d!");

    term_start_plot();

    screen_ok = FALSE;

    /* Sync point for epslatex text positioning */
    (term->layer)(TERM_LAYER_BACKTEXT);

    /* now compute boundary for plot */
    boundary3d(plots, pcount);

    axis_set_graphical_range(FIRST_X_AXIS, plot_bounds.xleft, plot_bounds.xright);
    axis_set_graphical_range(FIRST_Y_AXIS, plot_bounds.ybot, plot_bounds.ytop);
    axis_set_graphical_range(FIRST_Z_AXIS, floor_z, ceiling_z);

    /* SCALE FACTORS */
    zscale3d = 2.0 / (ceiling_z - floor_z) * surface_zscale;
    yscale3d = 2.0 / (Y_AXIS.max - Y_AXIS.min);
    xscale3d = 2.0 / (X_AXIS.max - X_AXIS.min);

    /* Allow 'set view equal xy' to adjust rendered length of the X and/or Y axes. */
    /* FIXME EAM - This only works correctly if the coordinate system of the       */
    /* terminal itself is isotropic.  E.g. x11 does not work because the x and y   */
    /* coordinates always run from 0-4095 regardless of the shape of the window.   */
    xcenter3d = ycenter3d = zcenter3d = 0.0;
    if (aspect_ratio_3D >= 2) {
	if (yscale3d > xscale3d) {
	    ycenter3d = 1.0 - xscale3d/yscale3d;
	    yscale3d = xscale3d;
	} else if (xscale3d > yscale3d) {
	    xcenter3d = 1.0 - yscale3d/xscale3d;
	    xscale3d = yscale3d;
	}
	if (aspect_ratio_3D >= 3)
	    zscale3d = xscale3d;
    }

    /* Initialize palette */
    if (!quick) {
	can_pm3d = is_plot_with_palette() && !make_palette() && term->set_color;
    }

    /* Give a chance for rectangles to be behind everything else */
    place_objects( first_object, -1, 3, NULL );

    term_apply_lp_properties(&border_lp);	/* border linetype */

    /* must come before using draw_3d_graphbox() the first time */
    setup_3d_box_corners();

    /* DRAW GRID AND BORDER */
    /* Original behaviour: draw entire grid in back, if 'set grid back': */
    /* HBB 20040331: but not if in hidden3d mode */
    if (!hidden3d && grid_layer == 0)
	draw_3d_graphbox(plots, pcount, ALLGRID, LAYER_BACK);
    else if (splot_map && border_layer == 0)
	draw_3d_graphbox(plots, pcount, BORDERONLY, LAYER_BACK);

#ifdef USE_GRID_LAYERS
    if (!hidden3d && (grid_layer == -1))
	/* Default layering mode.  Draw the back part now, but not if
	 * hidden3d is in use, because that relies on all isolated
	 * lines being output after all surfaces have been defined. */
	draw_3d_graphbox(plots, pcount, BACKGRID, LAYER_BACK);
#endif /* USE_GRID_LAYERS */

    /* Clipping in 'set view map' mode should be like 2D clipping */
    if (splot_map) {
	int map_x1, map_y1, map_x2, map_y2;
	map3d_xy(X_AXIS.min, Y_AXIS.min, base_z, &map_x1, &map_y1);
	map3d_xy(X_AXIS.max, Y_AXIS.max, base_z, &map_x2, &map_y2);
	plot_bounds.xleft = map_x1;
	plot_bounds.xright = map_x2;
	plot_bounds.ybot = map_y2;
	plot_bounds.ytop = map_y1;
    }

    /* Mar 2009 - This is a change!
     * Define the clipping area in 3D to lie between the left-most and
     * right-most graph box edges.  This is introduced for the benefit of
     * zooming in the canvas terminal.  It may or may not make any practical
     * difference for other terminals.  If it causes problems, then we will need
     * a separate BoundingBox structure to track the actual 3D graph box.
     */
    else {
	int xl, xb, xr, xf, yl, yb, yr, yf;

	map3d_xy(zaxis_x, zaxis_y, base_z, &xl, &yl);
	map3d_xy(back_x , back_y , base_z, &xb, &yb);
	map3d_xy(right_x, right_y, base_z, &xr, &yr);
	map3d_xy(front_x, front_y, base_z, &xf, &yf);
	plot_bounds.xleft = GPMIN(xl, xb);	/* Always xl? */
	plot_bounds.xright = GPMAX(xb, xr);	/* Always xr? */
    }

    /* PLACE TITLE */
    if (title.text != 0) {
	unsigned int x, y;
	int tmpx, tmpy;
	if (splot_map) { /* case 'set view map' */
	    int map_x1, map_y1, map_x2, map_y2;
	    int tics_len = 0;
	    if (X_AXIS.ticmode & TICS_MIRROR) {
		tics_len = (int)(X_AXIS.ticscale * (X_AXIS.tic_in ? -1 : 1) * (term->v_tic));
		if (tics_len < 0) tics_len = 0; /* take care only about upward tics */
	    }
	    map3d_xy(X_AXIS.min, Y_AXIS.min, base_z, &map_x1, &map_y1);
	    map3d_xy(X_AXIS.max, Y_AXIS.max, base_z, &map_x2, &map_y2);
	    /* Distance between the title base line and graph top line or the upper part of
	       tics is as given by character height: */
	    map3d_position_r(&(title.offset), &tmpx, &tmpy, "3dplot");
#define DEFAULT_Y_DISTANCE 1.0
	    x = (unsigned int) ((map_x1 + map_x2) / 2 + tmpx);
	    y = (unsigned int) (map_y1 + tics_len + tmpy + (DEFAULT_Y_DISTANCE + titlelin - 0.5) * (t->v_char));
#undef DEFAULT_Y_DISTANCE
	} else { /* usual 3d set view ... */
	    map3d_position_r(&(title.offset), &tmpx, &tmpy, "3dplot");
	    x = (unsigned int) ((plot_bounds.xleft + plot_bounds.xright) / 2 + tmpx);
	    y = (unsigned int) (plot_bounds.ytop + tmpy + titlelin * (t->h_char));
	}
	ignore_enhanced(title.noenhanced);
	apply_pm3dcolor(&(title.textcolor),t);
	/* PM: why there is JUST_TOP and not JUST_BOT? We should draw above baseline!
	 * But which terminal understands that? It seems vertical justification does
	 * not work... */
	write_multiline(x, y, title.text, CENTRE, JUST_TOP, 0, title.font);
	reset_textcolor(&(title.textcolor),t);
	ignore_enhanced(FALSE);
    }

    /* PLACE TIMEDATE */
    if (timelabel.text) {
	char str[MAX_LINE_LEN+1];
	time_t now;
	int tmpx, tmpy;
	unsigned int x, y;

	map3d_position_r(&(timelabel.offset), &tmpx, &tmpy, "3dplot");
	x = t->v_char + tmpx;
	y = timelabel_bottom
	    ? yoffset * Y_AXIS.max + tmpy + t->v_char
	    : plot_bounds.ytop + tmpy - t->v_char;

	time(&now);
	strftime(str, MAX_LINE_LEN, timelabel.text, localtime(&now));

	if (timelabel_rotate && (*t->text_angle) (TEXT_VERTICAL)) {
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_TOP, TEXT_VERTICAL, timelabel.font);
	    else
		write_multiline(x, y, str, RIGHT, JUST_TOP, TEXT_VERTICAL, timelabel.font);

	    (*t->text_angle) (0);
	} else {
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_BOT, 0, timelabel.font);
	    else
		write_multiline(x, y, str, LEFT, JUST_TOP, 0, timelabel.font);
	}
    }

    /* Add 'back' color box */
    if (!quick && can_pm3d && is_plot_with_colorbox() && color_box.layer == LAYER_BACK)
	draw_color_smooth_box(MODE_SPLOT);

    /* Add 'back' rectangles */
    place_objects(first_object, 0, 3, &plot_bounds);

    /* PLACE LABELS */
    place_labels3d(first_label, 0);

    /* PLACE ARROWS */
    place_arrows3d(0);

    /* Sync point for epslatex text positioning */
    (term->layer)(TERM_LAYER_FRONTTEXT);

    if (hidden3d && draw_surface && !quick) {
	init_hidden_line_removal();
	reset_hidden_line_removal();
    }

    /* WORK OUT KEY SETTINGS AND DO KEY TITLE / BOX */

    if (key->reverse) {
	key_sample_left = -key_sample_width;
	key_sample_right = 0;
	key_text_left = t->h_char;
	key_text_right = t->h_char * (max_ptitl_len + 1);
	key_size_right = t->h_char * (max_ptitl_len + 2 + key->width_fix);
	key_size_left = t->h_char + key_sample_width;
    } else {
	key_sample_left = 0;
	key_sample_right = key_sample_width;
	key_text_left = -(int) (t->h_char * (max_ptitl_len + 1));
	key_text_right = -(int) t->h_char;
	key_size_left = t->h_char * (max_ptitl_len + 2 + key->width_fix);
	key_size_right = t->h_char + key_sample_width;
    }
    key_point_offset = (key_sample_left + key_sample_right) / 2;

    if (key->visible)
    if (key->region != GPKEY_USER_PLACEMENT) {
	if (key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_BMARGIN) {
	    /* HBB 19990608: why calculate these again? boundary3d has already
	     * done it... */
	    if (ptitl_cnt > 0) {
		/* maximise no cols, limited by label-length */
		key_cols = (int) (plot_bounds.xright - plot_bounds.xleft) / key_col_wth;
		if (key_cols < 1) key_cols = 1;
		key_rows = (int) (ptitl_cnt + key_cols - 1) / key_cols;
		/* now calculate actual no cols depending on no rows */
		key_cols = (int) (ptitl_cnt + key_rows - 1) / key_rows;
		key_col_wth = (int) (plot_bounds.xright - plot_bounds.xleft) / key_cols;
		/* we divide into columns, then centre in column by considering
		 * ratio of key_left_size to key_right_size
		 * key_size_left/(key_size_left+key_size_right) 
		 *  * (plot_bounds.xright-plot_bounds.xleft)/key_cols
		 * do one integer division to maximise accuracy (hope we dont
		 * overflow !)
		 */
		xl = plot_bounds.xleft
		   + ((plot_bounds.xright - plot_bounds.xleft) * key_size_left) 
		   / (key_cols * (key_size_left + key_size_right));
		yl = yoffset * t->ymax + (key_rows) * key_entry_height 
		   + (ktitle_lines + 2) * t->v_char;
	    }

	} else {
	    if (key->vpos == JUST_TOP) {
		yl = plot_bounds.ytop - t->v_tic - t->v_char;
	    } else {
		yl = plot_bounds.ybot + t->v_tic + key_entry_height * key_rows + ktitle_lines * t->v_char;
	    }
	    if (key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_RMARGIN) {
		/* keys outside plot border (right) */
		xl = plot_bounds.xright + t->h_tic + key_size_left;
	    } else if (key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_LMARGIN) {
		/* keys outside plot border (left) */
		xl = key_size_left + 2 * t->h_char;
	    } else if (key->hpos == LEFT) {
		xl = plot_bounds.xleft + t->h_tic + key_size_left;
	    } else if (rmargin.scalex == screen 
		   && (key->region == GPKEY_AUTO_EXTERIOR_LRTBC 
			|| key->region == GPKEY_AUTO_EXTERIOR_MARGIN)) {
		xl = plot_bounds.xright - key_size_right + key_col_wth - 2 * t->h_char;
	    } else {
		xl = plot_bounds.xright - key_size_right - key_col_wth * (key_cols - 1);
	    }
	}
	yl_ref = yl - ktitle_lines * t->v_char;
    }
    if (key->region == GPKEY_USER_PLACEMENT) {
	map3d_position(&key->user_pos, &xl, &yl, "key");
    }
   
    /* Key bounds */
	key->bounds.xright = xl + key_col_wth * (key_cols - 1) + key_size_right;
	key->bounds.xleft = xl - key_size_left;
	key->bounds.ytop = yl + t->v_char * ktitle_lines;
	key->bounds.ybot = yl - key_entry_height * key_rows;

    /* Key title */
    if (key->visible && (*key->title)) {
	int center = (key->bounds.xright + key->bounds.xleft) / 2;
	double extra_height = 0.0;

	if (key->textcolor.type == TC_RGB && key->textcolor.value < 0)
	    apply_pm3dcolor(&(key->box.pm3d_color), t);
	else
	    apply_pm3dcolor(&(key->textcolor), t);
	if ((t->flags & TERM_ENHANCED_TEXT) && strchr(key->title,'^'))
	    extra_height += 0.51;
	write_multiline(center, key->bounds.ytop - (0.5 + extra_height/2.0) * t->v_char,
			key->title, CENTRE, JUST_TOP, 0, key->font);
	if ((t->flags & TERM_ENHANCED_TEXT) && strchr(key->title,'_'))
	    extra_height += 0.3;
	ktitle_lines += extra_height;
	yl -= t->v_char * extra_height;
	key->bounds.ybot -= t->v_char * extra_height;
	(*t->linetype)(LT_BLACK);
    }

    /* Key box */
    if (key->visible && key->box.l_type > LT_NODRAW) {
	int tmp = (int)(0.5 * key->height_fix * t->v_char);
	key->bounds.ybot -= 2*tmp;
	yl -= tmp;

	term_apply_lp_properties(&key->box);
	newpath();
	(*t->move) (key->bounds.xleft, key->bounds.ybot);
	(*t->vector) (key->bounds.xleft, key->bounds.ytop);
	(*t->vector) (key->bounds.xright, key->bounds.ytop);
	(*t->vector) (key->bounds.xright, key->bounds.ybot);
	(*t->vector) (key->bounds.xleft, key->bounds.ybot);
	closepath();

	/* draw a horizontal line between key title and first entry  JFi */
	(*t->move) (key->bounds.xleft, key->bounds.ytop - (ktitle_lines) * t->v_char);
	(*t->vector) (key->bounds.xright, key->bounds.ytop - (ktitle_lines) * t->v_char);
    }


    /* DRAW SURFACES AND CONTOURS */

    if (hidden3d && (hidden3d_layer == LAYER_BACK) && draw_surface && !quick) {
	(term->layer)(TERM_LAYER_BEFORE_PLOT);
	plot3d_hidden(plots, pcount);
	(term->layer)(TERM_LAYER_AFTER_PLOT);
    }

    /* Set up bookkeeping for the individual key titles */
#define NEXT_KEY_LINE()					\
    do {                                                \
    if ( ++key_count >= key_rows ) {			\
	yl = yl_ref; xl += key_col_wth; key_count = 0;	\
    } else						\
	yl -= key_entry_height;                         \
    } while (0)
    key_count = 0;
    yl_ref = yl -= key_entry_height / 2;	/* centralise the keys */


    /* PM January 2005: The mistake of missing blank lines in the data file is
     * so frequently made (see questions at comp.graphics.apps.gnuplot) that it
     * really deserves this warning. But don't show it too often --- only if it
     * is a single surface in the plot.
     */
    if (pcount == 1 && plots->num_iso_read == 1 && can_pm3d &&
	(plots->plot_style == PM3DSURFACE || PM3D_IMPLICIT == pm3d.implicit))
	    fprintf(stderr, "  Warning: Single isoline (scan) is not enough for a pm3d plot.\n\t   Hint: Missing blank lines in the data file? See 'help pm3d' and FAQ.\n");


    pm3d_order_depth = (can_pm3d && !draw_contour && pm3d.direction == PM3D_DEPTH);

    if (pm3d_order_depth) {
	pm3d_depth_queue_clear();
    }

    this_plot = plots;
    if (!quick)
	for (surface = 0;
	     surface < pcount;
	     this_plot = this_plot->next_sp, surface++) {
	    /* just an abbreviation */
	    TBOOLEAN use_palette = can_pm3d && this_plot->lp_properties.use_palette;
	    TBOOLEAN lkey;

	    /* Skip over abortive data structures */
	    if (this_plot->plot_type == NODATA)
		continue;

	    /* Sync point for start of new curve (used by svg, post, ...) */
	    (term->layer)(TERM_LAYER_BEFORE_PLOT);

	    if (can_pm3d && PM3D_IMPLICIT == pm3d.implicit)
		pm3d_draw_one(this_plot);

	    lkey = (key->visible && this_plot->title && this_plot->title[0]
				 && !this_plot->title_is_suppressed);

	    if (lkey) {
		if (key->textcolor.type != TC_DEFAULT)
		    /* Draw key text in same color as key title */
		    apply_pm3dcolor(&key->textcolor, t);
		else
		    /* Draw key text in black */
		    (*t->linetype)(LT_BLACK);
		ignore_enhanced(this_plot->title_no_enhanced);
		key_text(xl, yl, this_plot->title);
		ignore_enhanced(FALSE);
	    }
	    term_apply_lp_properties(&(this_plot->lp_properties));

	    switch (this_plot->plot_style) {
	    case BOXES:	/* can't do boxes in 3d yet so use impulses */
	    case FILLEDCURVES:
	    case IMPULSES:
		{
		    if (lkey) {
			key_sample_line(xl, yl);
		    }
		    if (!(hidden3d && draw_surface && !this_plot->opt_out_of_surface))
			plot3d_impulses(this_plot);
		    break;
		}
	    case STEPS:	/* HBB: I think these should be here */
	    case FSTEPS:
	    case HISTEPS:
	    case LINES:
		if (draw_surface && !this_plot->opt_out_of_surface) {
		    if (lkey) {
			if (this_plot->lp_properties.use_palette)
			    key_sample_line_pm3d(this_plot, xl, yl);
			else
			    key_sample_line(xl, yl);
		    }
		    if (!hidden3d || this_plot->opt_out_of_hidden3d) {
			if (use_palette)
			    plot3d_lines_pm3d(this_plot);
			else
			    plot3d_lines(this_plot);
		    }
		}
		break;
	    case YERRORLINES:	/* ignored; treat like points */
	    case XERRORLINES:	/* ignored; treat like points */
	    case XYERRORLINES:	/* ignored; treat like points */
	    case YERRORBARS:	/* ignored; treat like points */
	    case XERRORBARS:	/* ignored; treat like points */
	    case XYERRORBARS:	/* ignored; treat like points */
	    case BOXXYERROR:	/* HBB: ignore these as well */
	    case BOXERROR:
	    case CANDLESTICKS:	/* HBB: ditto */
	    case BOXPLOT:
	    case FINANCEBARS:
#ifdef EAM_OBJECTS
	    case CIRCLES:
	    case ELLIPSES:
#endif
	    case POINTSTYLE:
		if (draw_surface && !this_plot->opt_out_of_surface) {
		    if (lkey) {
			if (this_plot->lp_properties.use_palette)
			    key_sample_point_pm3d(this_plot, xl, yl, this_plot->lp_properties.p_type);
			else
			    key_sample_point(xl, yl, this_plot->lp_properties.p_type);
		    }
		    if (!hidden3d || this_plot->opt_out_of_hidden3d)
			plot3d_points(this_plot, this_plot->lp_properties.p_type);
		}
		break;

	    case LINESPOINTS:
		if (draw_surface && !this_plot->opt_out_of_surface) {

		    /* put lines */
		    if (lkey) {
			if (this_plot->lp_properties.use_palette)
			    key_sample_line_pm3d(this_plot, xl, yl);
			else
			    key_sample_line(xl, yl);
		    }

		    if (!hidden3d || this_plot->opt_out_of_hidden3d) {
			if (use_palette)
			    plot3d_lines_pm3d(this_plot);
			else
			    plot3d_lines(this_plot);
		    }

		    /* put points */
		    if (lkey) {
			if (this_plot->lp_properties.use_palette)
			    key_sample_point_pm3d(this_plot, xl, yl, this_plot->lp_properties.p_type);
			else
			    key_sample_point(xl, yl, this_plot->lp_properties.p_type);
		     }

		    if (!hidden3d || this_plot->opt_out_of_hidden3d)
			plot3d_points(this_plot, this_plot->lp_properties.p_type);

		}
		break;

	    case DOTS:
		if (draw_surface && !this_plot->opt_out_of_surface) {
		    this_plot->lp_properties.p_type = -1;
		    this_plot->lp_properties.pointflag = TRUE;
		    if (lkey) {
			if (this_plot->lp_properties.use_palette)
			    key_sample_point_pm3d(this_plot, xl, yl, -1);
			else
			    key_sample_point(xl, yl, -1);
		    }
		    if (!hidden3d || this_plot->opt_out_of_hidden3d)
			plot3d_points(this_plot, -1);
		}
		break;

	    case VECTOR:
		if (lkey) {
		    if (this_plot->lp_properties.use_palette)
			key_sample_line_pm3d(this_plot, xl, yl);
		    else
			key_sample_line(xl, yl);
		}
		if (!hidden3d || this_plot->opt_out_of_hidden3d)
		    plot3d_vectors(this_plot);
		break;

	    case PM3DSURFACE:
		if (draw_surface && !this_plot->opt_out_of_surface) {
		    if (can_pm3d && PM3D_IMPLICIT != pm3d.implicit) {
			pm3d_draw_one(this_plot);
			if (!pm3d_order_depth)
			    pm3d_depth_queue_flush(); /* draw plot immediately */
		    }
		}
		break;

	    case LABELPOINTS:
		if (!hidden3d || this_plot->opt_out_of_hidden3d)
		    place_labels3d(this_plot->labels->next, LAYER_PLOTLABELS);
		break;

	    case HISTOGRAMS: /* Cannot happen */
		break;

	    case IMAGE:
		/* Plot image using projection of 3D plot coordinates to 2D viewing coordinates. */
		this_plot->image_properties.type = IC_PALETTE;
		plot_image_or_update_axes(this_plot, FALSE);
		break;

	    case RGBIMAGE:
		/* Plot image using projection of 3D plot coordinates to 2D viewing coordinates. */
		this_plot->image_properties.type = IC_RGB;
		plot_image_or_update_axes(this_plot, FALSE);
		break;

	    case RGBA_IMAGE:
		this_plot->image_properties.type = IC_RGBA;
		plot_image_or_update_axes(this_plot, FALSE);
		break;

	    }			/* switch(plot-style) */

	    /* move key on a line */
	    if (lkey)
		NEXT_KEY_LINE();

	    if (draw_contour && this_plot->contours != NULL) {
		struct gnuplot_contours *cntrs = this_plot->contours;
		struct lp_style_type thiscontour_lp_properties =
		    this_plot->lp_properties;

		thiscontour_lp_properties.l_type += (hidden3d ? 1 : 0);

		term_apply_lp_properties(&(thiscontour_lp_properties));

		if (key->visible && this_plot->title && this_plot->title[0]
		    && !this_plot->title_is_suppressed
		    && !draw_surface && !label_contours) {
		    /* unlabelled contours but no surface : put key entry in now */
		    /* EAM - force key text to black, then restore */
		    (*t->linetype)(LT_BLACK);
		    key_text(xl, yl, this_plot->title);
		    term_apply_lp_properties(&(thiscontour_lp_properties));

		    switch (this_plot->plot_style) {
		    case IMPULSES:
		    case LINES:
		    case BOXES:	/* HBB: I think these should be here... */
		    case FILLEDCURVES:
		    case VECTOR:
		    case STEPS:
		    case FSTEPS:
		    case HISTEPS:
			key_sample_line(xl, yl);
			break;
		    case YERRORLINES:	/* ignored; treat like points */
		    case XERRORLINES:	/* ignored; treat like points */
		    case XYERRORLINES:	/* ignored; treat like points */
		    case YERRORBARS:	/* ignored; treat like points */
		    case XERRORBARS:	/* ignored; treat like points */
		    case XYERRORBARS:	/* ignored; treat like points */
		    case BOXERROR:	/* HBB: ignore these as well */
		    case BOXXYERROR:
		    case CANDLESTICKS:	/* HBB: ditto */
		    case BOXPLOT:
		    case FINANCEBARS:
#ifdef EAM_OBJECTS
		    case CIRCLES:
		    case ELLIPSES:
#endif
		    case POINTSTYLE:
			key_sample_point(xl, yl, this_plot->lp_properties.p_type);
			break;
		    case LINESPOINTS:
			key_sample_line(xl, yl);
			break;
		    case DOTS:
			key_sample_point(xl, yl, -1);
			break;

		    default:
			break;
		    }
		    NEXT_KEY_LINE();
		}

		while (cntrs) {
		    if (label_contours && cntrs->isNewLevel) {
			if (key->visible) {
			    (*t->linetype)(LT_BLACK);
			    key_text(xl, yl, cntrs->label);
			}
			if (use_palette && thiscontour_lp_properties.pm3d_color.type == TC_Z)
			    set_color( cb2gray( z2cb(cntrs->z) ) );
			else {
			    struct lp_style_type ls = thiscontour_lp_properties;
			    if (thiscontour_lp_properties.l_type == LT_COLORFROMCOLUMN) {
		    		thiscontour_lp_properties.l_type = 0;
				thiscontour_lp_properties.use_palette = TRUE;
			    }
			    if (prefer_line_styles && label_contours)
				lp_use_properties(&ls, ++thiscontour_lp_properties.l_type+1);
			    else
				load_linetype(&ls, ++thiscontour_lp_properties.l_type+1);
			    thiscontour_lp_properties.pm3d_color = ls.pm3d_color;
			    term_apply_lp_properties(&thiscontour_lp_properties);
			}

			if (key->visible) {

			    switch (this_plot->plot_style) {
			    case IMPULSES:
			    case LINES:
			    case LINESPOINTS:
			    case BOXES:	/* HBB: these should be treated as well... */
			    case FILLEDCURVES:
			    case VECTOR:
			    case STEPS:
			    case FSTEPS:
			    case HISTEPS:
			    case PM3DSURFACE:
				key_sample_line(xl, yl);
				break;
			    case YERRORLINES:	/* ignored; treat like points */
			    case XERRORLINES:	/* ignored; treat like points */
			    case XYERRORLINES:	/* ignored; treat like points */
			    case YERRORBARS:	/* ignored; treat like points */
			    case XERRORBARS:	/* ignored; treat like points */
			    case XYERRORBARS:	/* ignored; treat like points */
			    case BOXERROR:		/* HBB: treat these likewise */
			    case BOXXYERROR:
			    case CANDLESTICKS:	/* HBB: ditto */
			    case BOXPLOT:
			    case FINANCEBARS:
#ifdef EAM_OBJECTS
			    case CIRCLES:
			    case ELLIPSES:
#endif
			    case POINTSTYLE:
				    key_sample_point(xl, yl, this_plot->lp_properties.p_type);
				break;
			    case DOTS:
				    key_sample_point(xl, yl, -1);
				break;

			    default:
				break;
			    }	/* switch */

			    NEXT_KEY_LINE();

			} /* key */
		    } /* label_contours */

		    /* now draw the contour */
		    switch (this_plot->plot_style) {
			/* treat boxes like impulses: */
		    case BOXES:
		    case FILLEDCURVES:
		    case VECTOR:
		    case IMPULSES:
			cntr3d_impulses(cntrs, &thiscontour_lp_properties);
			break;

		    case STEPS:
		    case FSTEPS:
		    case HISTEPS:
			/* treat all the above like 'lines' */
		    case LINES:
		    case PM3DSURFACE:
			cntr3d_lines(cntrs, &thiscontour_lp_properties);
			break;

		    case LINESPOINTS:
			cntr3d_lines(cntrs, &thiscontour_lp_properties);
			/* FALLTHROUGH to draw the points */
		    case YERRORLINES:
		    case XERRORLINES:
		    case XYERRORLINES:
		    case YERRORBARS:
		    case XERRORBARS:
		    case XYERRORBARS:
		    case BOXERROR:
		    case BOXXYERROR:
		    case CANDLESTICKS:
		    case FINANCEBARS:
#ifdef EAM_OBJECTS
		    case CIRCLES:
		    case ELLIPSES:		    
#endif
			/* treat all the above like points */
		    case DOTS:
		    case POINTSTYLE:
			cntr3d_points(cntrs, &thiscontour_lp_properties);
			break;

		    default:
			break;
		    } /*switch */

		    cntrs = cntrs->next;
		} /* loop over contours */
	    } /* draw contours */
	    
	    /* Sync point for end of this curve (used by svg, post, ...) */
	    (term->layer)(TERM_LAYER_AFTER_PLOT);

	} /* loop over surfaces */

    if (pm3d_order_depth) {
	pm3d_depth_queue_flush(); /* draw pending plots */
    }

    if (hidden3d && (hidden3d_layer == LAYER_FRONT) && draw_surface && !quick) {
	(term->layer)(TERM_LAYER_BEFORE_PLOT);
	plot3d_hidden(plots, pcount);
	(term->layer)(TERM_LAYER_AFTER_PLOT);
    }

    /* DRAW GRID AND BORDER */
#ifndef USE_GRID_LAYERS
    /* Old behaviour: draw entire grid now, unless it was requested to
     * be in the back. */
    if (grid_layer != 0)
	draw_3d_graphbox(plots, pcount, ALLGRID, LAYER_FRONT);
#else
    /* HBB NEW 20040311: do front part now, after surfaces have been
     * output. If "set grid front", or hidden3d is active, must output
     * the whole shebang now, otherwise only the front part. */
    if (hidden3d || grid_layer == 1)
	draw_3d_graphbox(plots, pcount, ALLGRID, LAYER_FRONT);
    else if (grid_layer == -1)
	draw_3d_graphbox(plots, pcount, FRONTGRID, LAYER_FRONT);
    if (splot_map && (border_layer == 1))
	draw_3d_graphbox(plots, pcount, BORDERONLY, LAYER_FRONT);

#endif /* USE_GRID_LAYERS */

    /* Add 'front' color box */
    if (!quick && can_pm3d && is_plot_with_colorbox() && color_box.layer == LAYER_FRONT)
	draw_color_smooth_box(MODE_SPLOT);

    /* Add 'front' rectangles */
    place_objects(first_object, 1, 3, &plot_bounds);

    /* PLACE LABELS */
    place_labels3d(first_label, 1);

    /* PLACE ARROWS */
    place_arrows3d(1);

#ifdef USE_MOUSE
    /* finally, store the 2d projection of the x and y axis, to enable zooming by mouse */
    {
	int x,y;
	map3d_xy(X_AXIS.min, Y_AXIS.min, base_z, &axis3d_o_x, &axis3d_o_y);
	map3d_xy(X_AXIS.max, Y_AXIS.min, base_z, &x, &y);
	axis3d_x_dx = x - axis3d_o_x;
	axis3d_x_dy = y - axis3d_o_y;
	map3d_xy(X_AXIS.min, Y_AXIS.max, base_z, &x, &y);
	axis3d_y_dx = x - axis3d_o_x;
	axis3d_y_dy = y - axis3d_o_y;
    }
#endif

    /* Release the palette if we have used one (PostScript only?) */
    if (is_plot_with_palette() && term->previous_palette)
	term->previous_palette();

    term_end_plot();

    if (hidden3d && draw_surface) {
	term_hidden_line_removal();
    }

}


/* plot3d_impulses:
 * Plot the surfaces in IMPULSES style
 */
static void
plot3d_impulses(struct surface_points *plot)
{
    int i;				/* point index */
    int x, y, xx0, yy0;			/* point in terminal coordinates */
    struct iso_curve *icrvs = plot->iso_crvs;
    int colortype = plot->lp_properties.pm3d_color.type;

    TBOOLEAN rgb_from_column = can_pm3d && plot->pm3d_color_from_column
			&& plot->lp_properties.pm3d_color.value < 0.0;

    if (colortype == TC_RGB && !rgb_from_column)
	set_rgbcolor(plot->lp_properties.pm3d_color.lt);

    while (icrvs) {
	struct coordinate GPHUGE *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {

	    check_for_variable_color(plot, &points[i]);

	    switch (points[i].type) {
	    case INRANGE:
		{
		    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

		    if (inrange(0.0, Z_AXIS.min, Z_AXIS.max)) {
			map3d_xy(points[i].x, points[i].y, 0.0, &xx0, &yy0);
		    } else if (inrange(Z_AXIS.min, 0.0, points[i].z)) {
			map3d_xy(points[i].x, points[i].y, Z_AXIS.min, &xx0, &yy0);
		    } else {
			map3d_xy(points[i].x, points[i].y, Z_AXIS.max, &xx0, &yy0);
		    }

		    clip_move(xx0, yy0);
		    clip_vector(x, y);

		    break;
		}
	    case OUTRANGE:
		{
		    if (!inrange(points[i].x, X_AXIS.min, X_AXIS.max) ||
			!inrange(points[i].y, Y_AXIS.min, Y_AXIS.max))
			break;

		    if (inrange(0.0, Z_AXIS.min, Z_AXIS.max)) {
			/* zero point is INRANGE */
			map3d_xy(points[i].x, points[i].y, 0.0, &xx0, &yy0);

			/* must cross z = Z_AXIS.min or Z_AXIS.max limits */
			if (inrange(Z_AXIS.min, 0.0, points[i].z) &&
			    Z_AXIS.min != 0.0 && Z_AXIS.min != points[i].z) {
			    map3d_xy(points[i].x, points[i].y, Z_AXIS.min, &x, &y);
			} else {
			    map3d_xy(points[i].x, points[i].y, Z_AXIS.max, &x, &y);
			}
		    } else {
			/* zero point is also OUTRANGE */
			if (inrange(Z_AXIS.min, 0.0, points[i].z) &&
			    inrange(Z_AXIS.max, 0.0, points[i].z)) {
			    /* crosses z = Z_AXIS.min or Z_AXIS.max limits */
			    map3d_xy(points[i].x, points[i].y, Z_AXIS.max, &x, &y);
			    map3d_xy(points[i].x, points[i].y, Z_AXIS.min, &xx0, &yy0);
			} else {
			    /* doesn't cross z = Z_AXIS.min or Z_AXIS.max limits */
			    break;
			}
		    }

		    clip_move(xx0, yy0);
		    clip_vector(x, y);

		    break;
		}
	    default:		/* just a safety */
	    case UNDEFINED:{
		    break;
		}
	    }
	}

	icrvs = icrvs->next;
    }
}


/* plot3d_lines:
 * Plot the surfaces in LINES style
 */
/* We want to always draw the lines in the same direction, otherwise when
   we draw an adjacent box we might get the line drawn a little differently
   and we get splotches.  */

static void
plot3d_lines(struct surface_points *plot)
{
    int i;
    int x, y, xx0, yy0;	/* point in terminal coordinates */
    double clip_x, clip_y, clip_z;
    struct iso_curve *icrvs = plot->iso_crvs;
    struct coordinate GPHUGE *points;
    double lx[2], ly[2], lz[2];	/* two edge points */
    TBOOLEAN rgb_from_column;

/* These are handled elsewhere.  */
    if (plot->has_grid_topology && hidden3d)
	return;

    rgb_from_column = plot->pm3d_color_from_column
			&& plot->lp_properties.pm3d_color.type == TC_RGB
			&& plot->lp_properties.pm3d_color.value < 0.0;

    while (icrvs) {
	enum coord_type prev = UNDEFINED;	/* type of previous plot */

	for (i = 0, points = icrvs->points; i < icrvs->p_count; i++) {

	    if (rgb_from_column)
		set_rgbcolor((int)points[i].CRD_COLOR);
	    else if (plot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
		plot->lp_properties.pm3d_color.lt = (int)(points[i].CRD_COLOR);
		apply_pm3dcolor(&(plot->lp_properties.pm3d_color), term);
	    }
	
	    switch (points[i].type) {
	    case INRANGE:{
		    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

		    if (prev == INRANGE) {
			clip_vector(x, y);
		    } else {
			if (prev == OUTRANGE) {
			    /* from outrange to inrange */
			    if (!clip_lines1) {
				clip_move(x, y);
			    } else {
				/*
				 * Calculate intersection point and draw
				 * vector from there
				 */
				edge3d_intersect(points, i, &clip_x, &clip_y, &clip_z);

				map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

				clip_move(xx0, yy0);
				clip_vector(x, y);
			    }
			} else {
			    clip_move(x, y);
			}
		    }

		    break;
		}
	    case OUTRANGE:{
		    if (prev == INRANGE) {
			/* from inrange to outrange */
			if (clip_lines1) {
			    /*
			     * Calculate intersection point and draw
			     * vector to it
			     */

			    edge3d_intersect(points, i, &clip_x, &clip_y, &clip_z);

			    map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

			    clip_vector(xx0, yy0);
			}
		    } else if (prev == OUTRANGE) {
			/* from outrange to outrange */
			if (clip_lines2) {
			    /*
			     * Calculate the two 3D intersection points
			     * if present
			     */
			    if (two_edge3d_intersect(points, i, lx, ly, lz)) {

				map3d_xy(lx[0], ly[0], lz[0], &x, &y);

				map3d_xy(lx[1], ly[1], lz[1], &xx0, &yy0);

				clip_move(x, y);
				clip_vector(xx0, yy0);
			    }
			}
		    }
		    break;
		}
	    case UNDEFINED:{
		    break;
		}
	    default:
		    graph_error("Unknown point type in plot3d_lines");
	    }

	    prev = points[i].type;
	}

	icrvs = icrvs->next;
    }
}

/* this is basically the same function as above, but:
 *  - it splits the bunch of scans in two sets corresponding to
 *    the two scan directions.
 *  - reorders the two sets -- from behind to front
 *  - checks if inside on scan of a set the order should be inverted
 */
static void
plot3d_lines_pm3d(struct surface_points *plot)
{
    struct iso_curve** icrvs_pair[2];
    int invert[2];
    int n[2];

    int i, set, scan;
    int x, y, xx0, yy0;	/* point in terminal coordinates */
    double clip_x, clip_y, clip_z;
    struct coordinate GPHUGE *points;
    enum coord_type prev = UNDEFINED;
    double lx[2], ly[2], lz[2];	/* two edge points */
    double z;

    /* just a shortcut */
    TBOOLEAN color_from_column = plot->pm3d_color_from_column;

    /* If plot really uses RGB rather than pm3d colors, let plot3d_lines take over */
    if (plot->lp_properties.pm3d_color.type == TC_RGB) {
	apply_pm3dcolor(&(plot->lp_properties.pm3d_color), term);
	plot3d_lines(plot);
	return;
    } else if (plot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
	plot3d_lines(plot);
	return;
    }

    /* These are handled elsewhere.  */
    if (plot->has_grid_topology && hidden3d)
	return;

    /* split the bunch of scans in two sets in
     * which the scans are already depth ordered */
    pm3d_rearrange_scan_array(plot,
	icrvs_pair, n, invert,
	icrvs_pair + 1, n + 1, invert + 1);

    for (set = 0; set < 2; set++) {

	int begin = 0;
	int step;

	if (invert[set]) {
	    /* begin is set below to the length of the scan - 1 */
	    step = -1;
	} else {
	    step = 1;
	}

	for (scan = 0; scan < n[set] && icrvs_pair[set]; scan++) {

	    int cnt;
	    struct iso_curve *icrvs = icrvs_pair[set][scan];

	    if (invert[set]) {
		begin = icrvs->p_count - 1;
	    }

	    prev = UNDEFINED;	/* type of previous plot */

	    for (cnt = 0, i = begin, points = icrvs->points; cnt < icrvs->p_count; cnt++, i += step) {
		switch (points[i].type) {
		    case INRANGE:
			map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

			if (prev == INRANGE) {
			    if (color_from_column)
				z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
			    else
				z =  (z2cb(points[i - step].z) + z2cb(points[i].z)) * 0.5;
			    set_color( cb2gray(z) );
			    clip_vector(x, y);
			} else {
			    if (prev == OUTRANGE) {
				/* from outrange to inrange */
				if (!clip_lines1) {
				    clip_move(x, y);
				} else {
				    /*
				     * Calculate intersection point and draw
				     * vector from there
				     */
				    edge3d_intersect(points, i, &clip_x, &clip_y, &clip_z);

				    map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

				    clip_move(xx0, yy0);
				    if (color_from_column)
					z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
				    else
					z =  (z2cb(points[i - step].z) + z2cb(points[i].z)) * 0.5;
				    set_color( cb2gray(z) );
				    clip_vector(x, y);
				}
			    } else {
				clip_move(x, y);
			    }
			}

			break;
		    case OUTRANGE:
			if (prev == INRANGE) {
			    /* from inrange to outrange */
			    if (clip_lines1) {
				/*
				 * Calculate intersection point and draw
				 * vector to it
				 */

				edge3d_intersect(points, i, &clip_x, &clip_y, &clip_z);

				map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

				if (color_from_column)
				    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
				else
				    z =  (z2cb(points[i - step].z) + z2cb(points[i].z)) * 0.5;
				set_color( cb2gray(z));
				clip_vector(xx0, yy0);
			    }
			} else if (prev == OUTRANGE) {
			    /* from outrange to outrange */
			    if (clip_lines2) {
				/*
				 * Calculate the two 3D intersection points
				 * if present
				 */
				if (two_edge3d_intersect(points, i, lx, ly, lz)) {

				    map3d_xy(lx[0], ly[0], lz[0], &x, &y);

				    map3d_xy(lx[1], ly[1], lz[1], &xx0, &yy0);

				    clip_move(x, y);
				    if (color_from_column)
					z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
				    else
					z =  (z2cb(points[i - step].z) + z2cb(points[i].z)) * 0.5;
				    set_color( cb2gray(z) );
				    clip_vector(xx0, yy0);
				}
			    }
			}
			break;
		    case UNDEFINED:
			break;
		    default:
			graph_error("Unknown point type in plot3d_lines");
		}

		prev = points[i].type;

	    } /* one scan */

	} /* while (icrvs)  */

    } /* for (scan = 0; scan < 2; scan++) */

    if (icrvs_pair[0])
	free(icrvs_pair[0]);
    if (icrvs_pair[1])
	free(icrvs_pair[1]);
}

/* plot3d_points:
 * Plot the surfaces in POINTSTYLE style
 */
static void
plot3d_points(struct surface_points *plot, int p_type)
{
    int i;
    int x, y;
    struct termentry *t = term;
    struct iso_curve *icrvs = plot->iso_crvs;

    while (icrvs) {
	struct coordinate GPHUGE *point;
	int colortype = plot->lp_properties.pm3d_color.type;
	TBOOLEAN rgb_from_column = plot->pm3d_color_from_column
			&& colortype == TC_RGB
			&& plot->lp_properties.pm3d_color.value < 0.0;

	/* Apply constant color outside of the loop */
	if (colortype == TC_RGB && !rgb_from_column)
	    set_rgbcolor( plot->lp_properties.pm3d_color.lt );

	for (i = 0; i < icrvs->p_count; i++) {
	    point = &(icrvs->points[i]);
	    if (point->type == INRANGE) {
		map3d_xy(point->x, point->y, point->z, &x, &y);

		if (!clip_point(x, y)) {
		    check_for_variable_color(plot, point);

		    if ((plot->plot_style == POINTSTYLE || plot->plot_style == LINESPOINTS)
		    &&  plot->lp_properties.p_size == PTSZ_VARIABLE)
			(*t->pointsize)(pointsize * point->CRD_PTSIZE);
		    (*t->point) (x, y, p_type);
		}
	    }
	}

	icrvs = icrvs->next;
    }
}

/* cntr3d_impulses:
 * Plot a surface contour in IMPULSES style
 */
static void
cntr3d_impulses(struct gnuplot_contours *cntr, struct lp_style_type *lp)
{
    int i;				/* point index */
    vertex vertex_on_surface, vertex_on_base;

    if (draw_contour & CONTOUR_SRF) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
		      &vertex_on_surface);
	    map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, 0.0,
		      &vertex_on_base);
	    /* HBB 20010822: Provide correct color-coding for
	     * "linetype palette" PM3D mode */
	    vertex_on_base.real_z = cntr->coords[i].z;
	    draw3d_line(&vertex_on_surface, &vertex_on_base, lp);
	}
    } else {
	/* Must be on base grid, so do points. */
	cntr3d_points(cntr, lp);
    }
}

/* cntr3d_lines: Plot a surface contour in LINES style */
/* HBB NEW 20031218: changed to use move/vector() style polyline
 * drawing. Gets rid of variable "previous_vertex" */
static void
cntr3d_lines(struct gnuplot_contours *cntr, struct lp_style_type *lp)
{
    int i;			/* point index */
    vertex this_vertex;

    /* In the case of "set view map" (only) clip the contour lines to the graph */
    BoundingBox *clip_save = clip_area;
    if (splot_map)
	clip_area = &plot_bounds;

    if (draw_contour & CONTOUR_SRF) {
	map3d_xyz(cntr->coords[0].x, cntr->coords[0].y, cntr->coords[0].z,
		  &this_vertex);
	/* move slightly frontward, to make sure the contours are
	 * visible in front of the the triangles they're in, if this
	 * is a hidden3d plot */
	if (hidden3d && !VERTEX_IS_UNDEFINED(this_vertex))
	    this_vertex.z += 1e-2;

	polyline3d_start(&this_vertex);

	for (i = 1; i < cntr->num_pts; i++) {
	    map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
		      &this_vertex);
	    /* move slightly frontward, to make sure the contours are
	     * visible in front of the the triangles they're in, if this
	     * is a hidden3d plot */
	    if (hidden3d && !VERTEX_IS_UNDEFINED(this_vertex))
		this_vertex.z += 1e-2;
	    polyline3d_next(&this_vertex, lp);
	}
    }

    if (draw_contour & CONTOUR_BASE) {
	map3d_xyz(cntr->coords[0].x, cntr->coords[0].y, base_z,
		  &this_vertex);
	this_vertex.real_z = cntr->coords[0].z;
	polyline3d_start(&this_vertex);

	for (i = 1; i < cntr->num_pts; i++) {
	    map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, base_z,
		      &this_vertex);
	    this_vertex.real_z = cntr->coords[i].z;
	    polyline3d_next(&this_vertex, lp);
	}
    }

    if (splot_map)
	clip_area = clip_save;
}

/* cntr3d_points:
 * Plot a surface contour in POINTSTYLE style
 */
static void
cntr3d_points(struct gnuplot_contours *cntr, struct lp_style_type *lp)
{
    int i;
    vertex v;

    if (draw_contour & CONTOUR_SRF) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
		      &v);
	    /* move slightly frontward, to make sure the contours and
	     * points are visible in front of the triangles they're
	     * in, if this is a hidden3d plot */
	    if (hidden3d && !VERTEX_IS_UNDEFINED(v))
		v.z += 1e-2;
	    draw3d_point(&v, lp);
	}
    }
    if (draw_contour & CONTOUR_BASE) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, base_z,
		     &v);
	    /* HBB 20010822: see above */
	    v.real_z = cntr->coords[i].z;
	    draw3d_point(&v, lp);
	}
    }
}

/* map xmin | xmax to 0 | 1 and same for y
 * 0.1 avoids any rounding errors
 */
#define MAP_HEIGHT_X(x) ( (int) (((x)-X_AXIS.min)/(X_AXIS.max-X_AXIS.min)+0.1) )
#define MAP_HEIGHT_Y(y) ( (int) (((y)-Y_AXIS.min)/(Y_AXIS.max-Y_AXIS.min)+0.1) )

/* if point is at corner, update height[][] and depth[][]
 * we are still assuming that extremes of surfaces are at corners,
 * but we are not assuming order of corners
 */
static void
check_corner_height(
    struct coordinate GPHUGE *p,
    double height[2][2], double depth[2][2])
{
    if (p->type != INRANGE)
	return;
    /* FIXME HBB 20010121: don't compare 'zero' to data values in
     * absolute terms. */
    if ((fabs(p->x - X_AXIS.min) < zero || fabs(p->x - X_AXIS.max) < zero) &&
	(fabs(p->y - Y_AXIS.min) < zero || fabs(p->y - Y_AXIS.max) < zero)) {
	unsigned int x = MAP_HEIGHT_X(p->x);
	unsigned int y = MAP_HEIGHT_Y(p->y);
	if (height[x][y] < p->z)
	    height[x][y] = p->z;
	if (depth[x][y] > p->z)
	    depth[x][y] = p->z;
    }
}

/* work out where the axes and tics are drawn */
static void
setup_3d_box_corners()
{
    int quadrant = surface_rot_z / 90;
    if ((quadrant + 1) & 2) {
	zaxis_x = X_AXIS.max;
	right_x = X_AXIS.min;
	back_y  = Y_AXIS.min;
	front_y  = Y_AXIS.max;
    } else {
	zaxis_x = X_AXIS.min;
	right_x = X_AXIS.max;
	back_y  = Y_AXIS.max;
	front_y  = Y_AXIS.min;
    }

    if (quadrant & 2) {
	zaxis_y = Y_AXIS.max;
	right_y = Y_AXIS.min;
	back_x  = X_AXIS.max;
	front_x  = X_AXIS.min;
    } else {
	zaxis_y = Y_AXIS.min;
	right_y = Y_AXIS.max;
	back_x  = X_AXIS.min;
	front_x  = X_AXIS.max;
    }

    if (surface_rot_x > 90) {
	/* labels on the back axes */
	yaxis_x = back_x;
	xaxis_y = back_y;
    } else {
	yaxis_x = front_x;
	xaxis_y = front_y;
    }
}

/* Draw all elements of the 3d graph box, including borders, zeroaxes,
 * tics, gridlines, ticmarks, axis labels and the base plane. */
static void
draw_3d_graphbox(struct surface_points *plot, int plot_num, WHICHGRID whichgrid, int current_layer)
{
    int x, y;		/* point in terminal coordinates */
    struct termentry *t = term;
    BoundingBox *clip_save = clip_area;

    if (draw_border && splot_map) {
	if (border_layer == current_layer) {
	    term_apply_lp_properties(&border_lp);
	    if ((draw_border & 15) == 15)
		newpath();
	    map3d_xy(zaxis_x, zaxis_y, base_z, &x, &y);
	    term->move(x, y);
	    map3d_xy(back_x , back_y , base_z, &x, &y);
	    if (draw_border & 2)
		term->vector(x, y);
	    else
		term->move(x, y);
	    map3d_xy(right_x, right_y, base_z, &x, &y);
	    if (draw_border & 8)
		term->vector(x, y);
	    else
		term->move(x, y);
	    map3d_xy(front_x, front_y, base_z, &x, &y);
	    if (draw_border & 4)
		term->vector(x, y);
	    else
		term->move(x, y);
	    map3d_xy(zaxis_x, zaxis_y, base_z, &x, &y);
	    if (draw_border & 1)
		term->vector(x, y);
	    else
		term->move(x, y);
	    if ((draw_border & 15) == 15)
		closepath();
	}
    } else

    if (draw_border) {
	/* the four corners of the base plane, in normalized view
	 * coordinates (-1..1) on all three axes. */
	vertex bl, bb, br, bf;

	/* map to normalized view coordinates the corners of the
	 * baseplane: left, back, right and front, in that order: */
	map3d_xyz(zaxis_x, zaxis_y, base_z, &bl);
	map3d_xyz(back_x , back_y , base_z, &bb);
	map3d_xyz(right_x, right_y, base_z, &br);
	map3d_xyz(front_x, front_y, base_z, &bf);

#ifdef USE_GRID_LAYERS
	if (BACKGRID != whichgrid)
#endif
	{
	    /* Draw front part of base grid, right to front corner: */
	    if (draw_border & 4)
		draw3d_line(&br, &bf, &border_lp);
	    /* ... and left to front: */
	    if (draw_border & 1)
		draw3d_line(&bl, &bf, &border_lp);
	}
#ifdef USE_GRID_LAYERS
	if (FRONTGRID != whichgrid)
#endif
	{
	    /* Draw back part of base grid: left to back corner: */
	    if (draw_border & 2)
		draw3d_line(&bl, &bb, &border_lp);
	    /* ... and right to back: */
	    if (draw_border & 8)
		draw3d_line(&br, &bb, &border_lp);
	}

	/* if surface is drawn, draw the rest of the graph box, too: */
	if (draw_surface || (draw_contour & CONTOUR_SRF)
	    || (pm3d.implicit == PM3D_IMPLICIT && strpbrk(pm3d.where,"st") != NULL)
	   ) {
	    vertex fl, fb, fr, ff; /* floor left/back/right/front corners */
	    vertex tl, tb, tr, tf; /* top left/back/right/front corners */

	    map3d_xyz(zaxis_x, zaxis_y, floor_z, &fl);
	    map3d_xyz(back_x , back_y , floor_z, &fb);
	    map3d_xyz(right_x, right_y, floor_z, &fr);
	    map3d_xyz(front_x, front_y, floor_z, &ff);

	    map3d_xyz(zaxis_x, zaxis_y, ceiling_z, &tl);
	    map3d_xyz(back_x , back_y , ceiling_z, &tb);
	    map3d_xyz(right_x, right_y, ceiling_z, &tr);
	    map3d_xyz(front_x, front_y, ceiling_z, &tf);

	    if ((draw_border & 0xf0) == 0xf0) {
		/* all four verticals are drawn - save some time by
		 * drawing them to the full height, regardless of
		 * where the surface lies */
#ifdef USE_GRID_LAYERS
		if (FRONTGRID != whichgrid) {
#endif
		    /* Draw the back verticals floor-to-ceiling, left: */
		    draw3d_line(&fl, &tl, &border_lp);
		    /* ... back: */
		    draw3d_line(&fb, &tb, &border_lp);
		    /* ... and right */
		    draw3d_line(&fr, &tr, &border_lp);
#ifdef USE_GRID_LAYERS
		}
		if (BACKGRID != whichgrid)
#endif
		    /* Draw the front vertical: floor-to-ceiling, front: */
		    draw3d_line(&ff, &tf, &border_lp);
	    } else {
		/* find heights of surfaces at the corners of the xy
		 * rectangle */
		double height[2][2];
		double depth[2][2];
		unsigned int zaxis_i = MAP_HEIGHT_X(zaxis_x);
		unsigned int zaxis_j = MAP_HEIGHT_Y(zaxis_y);
		unsigned int back_i = MAP_HEIGHT_X(back_x);
		unsigned int back_j = MAP_HEIGHT_Y(back_y);

		height[0][0] = height[0][1]
		    = height[1][0] = height[1][1] = base_z;
		depth[0][0] = depth[0][1]
		    = depth[1][0] = depth[1][1] = base_z;

		/* FIXME HBB 20000617: this method contains the
		 * assumption that the topological corners of the
		 * surface mesh(es) are also the geometrical ones of
		 * their xy projections. This is only true for
		 * 'explicit' surface datasets, i.e. z(x,y) */
		for (; --plot_num >= 0; plot = plot->next_sp) {
		    struct iso_curve *curve = plot->iso_crvs;
		    int count;
		    int iso;

		    if (plot->plot_type == NODATA)
			continue;
		    if (plot->plot_type == DATA3D) {
			if (!plot->has_grid_topology)
			    continue;
			iso = plot->num_iso_read;
		    } else
			iso = iso_samples_2;

		    count = curve->p_count;
		    check_corner_height(curve->points, height, depth);
		    check_corner_height(curve->points + count - 1, height, depth);
		    while (--iso)
			curve = curve->next;
		    check_corner_height(curve->points, height, depth);
		    check_corner_height(curve->points + count - 1, height, depth);
		}

#define VERTICAL(mask,x,y,i,j,bottom,top)			\
		if (draw_border&mask) {				\
		    draw3d_line(bottom,top, &border_lp);	\
		} else if (height[i][j] != depth[i][j]) {	\
		    vertex a, b;				\
		    map3d_xyz(x,y,depth[i][j],&a);		\
		    map3d_xyz(x,y,height[i][j],&b);		\
		    draw3d_line(&a, &b, &border_lp);		\
		}

#ifdef USE_GRID_LAYERS
		if (FRONTGRID != whichgrid) {
#endif
		    /* Draw back verticals: floor-to-ceiling left: */
		    VERTICAL(0x10, zaxis_x, zaxis_y, zaxis_i, zaxis_j, &fl, &tl);
		    /* ... back: */
		    VERTICAL(0x20, back_x, back_y, back_i, back_j, &fb, &tb);
		    /* ... and right: */
		    VERTICAL(0x40, right_x, right_y, 1 - zaxis_i, 1 - zaxis_j,
			     &fr, &tr);
#ifdef USE_GRID_LAYERS
		}
		if (BACKGRID != whichgrid) {
#endif
		    /* Draw front verticals: floor-to-ceiling front */
		    VERTICAL(0x80, front_x, front_y, 1 - back_i, 1 - back_j,
			     &ff, &tf);
#ifdef USE_GRID_LAYERS
		}
#endif
#undef VERTICAL
	    } /* else (all 4 verticals drawn?) */

	    /* now border lines on top */
#ifdef USE_GRID_LAYERS
	    if (FRONTGRID != whichgrid)
#endif
	    {
		/* Draw back part of top of box: top left to back corner: */
		if (draw_border & 0x100)
		    draw3d_line(&tl, &tb, &border_lp);
		/* ... and top right to back: */
		if (draw_border & 0x200)
		    draw3d_line(&tr, &tb, &border_lp);
	    }
#ifdef USE_GRID_LAYERS
	    if (BACKGRID != whichgrid)
#endif
	    {
		/* Draw front part of top of box: top left to front corner: */
		if (draw_border & 0x400)
		    draw3d_line(&tl, &tf, &border_lp);
		/* ... and top right to front: */
		if (draw_border & 0x800)
		    draw3d_line(&tr, &tf, &border_lp);
	    }
	} /* else (surface is drawn) */
    } /* if (draw_border) */

    /* In 'set view map' mode, treat grid as in 2D plots */
    if (splot_map && current_layer != abs(grid_layer))
	return;
    if (whichgrid == BORDERONLY)
	return;

    if (splot_map)
	clip_area = NULL;

    /* Draw ticlabels and axis labels. x axis, first:*/
    if (X_AXIS.ticmode || X_AXIS.label.text) {
	vertex v0, v1;
	double other_end =
	    Y_AXIS.min + Y_AXIS.max - xaxis_y;
	double mid_x =
	    (X_AXIS.max + X_AXIS.min) / 2;

	map3d_xyz(mid_x, xaxis_y, base_z, &v0);
	map3d_xyz(mid_x, other_end, base_z, &v1);

	tic_unitx = (v1.x - v0.x) / (double)yscaler;
	tic_unity = (v1.y - v0.y) / (double)yscaler;
	tic_unitz = (v1.z - v0.z) / (double)yscaler;

#ifdef USE_GRID_LAYERS
	/* Don't output tics and grids if this is the front part of a
	 * two-part grid drawing process: */
	if ((surface_rot_x <= 90 && FRONTGRID != whichgrid) ||
	    (surface_rot_x > 90 && BACKGRID != whichgrid))
#endif
	if (X_AXIS.ticmode) {
	    gen_tics(FIRST_X_AXIS, xtick_callback);
	}

	if (X_AXIS.label.text) {
	    /* label at xaxis_y + 1/4 of (xaxis_y-other_y) */
#ifdef USE_GRID_LAYERS /* FIXME: still needed??? what for? */
	    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
		(surface_rot_x > 90 && FRONTGRID != whichgrid) ||
		splot_map) {
#endif
	    unsigned int x1, y1;
	    int tmpx, tmpy;

	    if (splot_map) { /* case 'set view map' */
		/* copied from xtick_callback(): baseline of tics labels */
		vertex v1, v2;
		map3d_xyz(mid_x, xaxis_y, base_z, &v1);
		v2.x = v1.x;
		v2.y = v1.y - tic_unity * t->v_char * 1;
		if (!X_AXIS.tic_in) {
		    /* FIXME
		     * This code and its source in xtick_callback() is wrong --- tics
		     * can be "in" but ticscale <0 ! To be corrected in both places!
		     */
		    v2.y -= tic_unity * t->v_tic * X_AXIS.ticscale;
		}
#if 0
		/* PM: correct implementation for map should probably be like this: */
		if (X_AXIS.ticmode) {
		    int tics_len = (int)(X_AXIS.ticscale * (X_AXIS.tic_in ? -1 : 1) * (term->v_tic));
		    if (tics_len < 0) tics_len = 0; /* take care only about upward tics */
		    v2.y += tics_len;
		}
#endif
		TERMCOORD(&v2, x1, y1);
		/* DEFAULT_Y_DISTANCE is with respect to baseline of tics labels */
#define DEFAULT_Y_DISTANCE 0.5
		y1 -= (unsigned int) ((1 + DEFAULT_Y_DISTANCE) * t->v_char);
#undef DEFAULT_Y_DISTANCE
	    } else { /* usual 3d set view ... */
		double step = (xaxis_y - other_end) / 4;

		if (X_AXIS.ticmode & TICS_ON_AXIS) {
		    map3d_xyz(mid_x, (X_AXIS.tic_in ? step : -step)/2., base_z, &v1);
		} else {
		    map3d_xyz(mid_x, xaxis_y + step, base_z, &v1);
		}
		if (!X_AXIS.tic_in) {
		    v1.x -= tic_unitx * X_AXIS.ticscale * t->v_tic;
		    v1.y -= tic_unity * X_AXIS.ticscale * t->v_tic;
		}
		TERMCOORD(&v1, x1, y1);
	    }

	    map3d_position_r(&(X_AXIS.label.offset), &tmpx, &tmpy, "graphbox");
	    x1 += tmpx; /* user-defined label offset */
	    y1 += tmpy;
	    ignore_enhanced(X_AXIS.label.noenhanced);
	    apply_pm3dcolor(&(X_AXIS.label.textcolor),t);
	    if (X_AXIS.label.rotate != 0 && splot_map && (term->text_angle)(X_AXIS.label.rotate)) {
		write_multiline(x1, y1, X_AXIS.label.text, CENTRE, JUST_TOP,
			    X_AXIS.label.rotate, X_AXIS.label.font);
		(term->text_angle)(0);
	    } else {
		write_multiline(x1, y1, X_AXIS.label.text,
			    CENTRE, JUST_TOP, 0,
			    X_AXIS.label.font);
	    }
	    reset_textcolor(&(X_AXIS.label.textcolor),t);
	    ignore_enhanced(FALSE);
#ifdef USE_GRID_LAYERS
	    }
#endif
	}
    }

    /* y axis: */
    if (Y_AXIS.ticmode || Y_AXIS.label.text) {
	vertex v0, v1;
	double other_end = X_AXIS.min + X_AXIS.max - yaxis_x;
	double mid_y = (Y_AXIS.max + Y_AXIS.min) / 2;

	map3d_xyz(yaxis_x, mid_y, base_z, &v0);
	map3d_xyz(other_end, mid_y, base_z, &v1);

	tic_unitx = (v1.x - v0.x) / (double)xscaler;
	tic_unity = (v1.y - v0.y) / (double)xscaler;
	tic_unitz = (v1.z - v0.z) / (double)xscaler;

#ifdef USE_GRID_LAYERS
	/* Don't output tics and grids if this is the front part of a
	 * two-part grid drawing process: */
	if ((surface_rot_x <= 90 && FRONTGRID != whichgrid) ||
	    (surface_rot_x > 90 && BACKGRID != whichgrid))
#endif
	if (Y_AXIS.ticmode) {
	    gen_tics(FIRST_Y_AXIS, ytick_callback);
	}
	if (Y_AXIS.label.text) {
#ifdef USE_GRID_LAYERS
	    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
		(surface_rot_x > 90 && FRONTGRID != whichgrid) ||
		splot_map) {
#endif
		unsigned int x1, y1;
		int tmpx, tmpy;
		int h_just, v_just;
		int angle;

		if (splot_map) { /* case 'set view map' */
		    /* copied from ytick_callback(): baseline of tics labels */
		    vertex v1, v2;
		    map3d_xyz(yaxis_x, mid_y, base_z, &v1);
		    if (Y_AXIS.ticmode & TICS_ON_AXIS
			    && !X_AXIS.log
			    && inrange (0.0, X_AXIS.min, X_AXIS.max)
		       ) {
			map3d_xyz(0.0, yaxis_x, base_z, &v1);
		    }
		    v2.x = v1.x - tic_unitx * t->h_char * 1;
		    v2.y = v1.y;
		    if (!X_AXIS.tic_in)
			v2.x -= tic_unitx * t->h_tic * X_AXIS.ticscale;
		    TERMCOORD(&v2, x1, y1);
		    /* calculate max length of y-tics labels */
		    widest_tic_strlen = 0;
		    if (Y_AXIS.ticmode & TICS_ON_BORDER) {
			widest_tic_strlen = 0; /* reset the global variable */
			gen_tics(FIRST_Y_AXIS, widest_tic_callback);
		    }
		    /* DEFAULT_Y_DISTANCE is with respect to baseline of tics labels */
#define DEFAULT_X_DISTANCE 0.
		    x1 -= (unsigned int) ((DEFAULT_X_DISTANCE + 0.5 + widest_tic_strlen) * t->h_char);
#undef DEFAULT_X_DISTANCE
#if 0
		    /* another method ... but not compatible */
		    unsigned int map_y1, map_x2, map_y2;
		    int tics_len = 0;
		    if (Y_AXIS.ticmode) {
			tics_len = (int)(X_AXIS.ticscale * (X_AXIS.tic_in ? 1 : -1) * (term->v_tic));
			if (tics_len > 0) tics_len = 0; /* take care only about left tics */
		    }
		    map3d_xy(X_AXIS.min, Y_AXIS.min, base_z, &x1, &map_y1);
		    map3d_xy(X_AXIS.max, Y_AXIS.max, base_z, &map_x2, &map_y2);
		    y1 = (unsigned int)((map_y1 + map_y2) * 0.5);
		    /* Distance between the title base line and graph top line or the upper part of
		       tics is as given by character height: */
#define DEFAULT_X_DISTANCE 0
		    x1 += (unsigned int) (tics_len + (-0.5 + Y_AXIS.label.xoffset) * t->h_char);
		    y1 += (unsigned int) ((DEFAULT_X_DISTANCE + Y_AXIS.label.yoffset) * t->v_char);
#undef DEFAULT_X_DISTANCE
#endif
		    h_just = CENTRE; /* vertical justification for rotated text */
		    v_just = JUST_BOT; /* horizontal -- does not work for rotated text? */
		    angle = Y_AXIS.label.rotate;
		} else { /* usual 3d set view ... */
		    double step = (other_end - yaxis_x) / 4;
		    if (Y_AXIS.ticmode & TICS_ON_AXIS) {
			map3d_xyz((X_AXIS.tic_in ? -step : step)/2., mid_y, base_z, &v1);
		    } else {
			map3d_xyz(yaxis_x - step, mid_y, base_z, &v1);
		    }
		    if (!X_AXIS.tic_in) {
			v1.x -= tic_unitx * X_AXIS.ticscale * t->h_tic;
			v1.y -= tic_unity * X_AXIS.ticscale * t->h_tic;
		    }
		    TERMCOORD(&v1, x1, y1);
		    h_just = CENTRE;
		    v_just = JUST_TOP;
		    /* No Y-label rotation in 3D plot mode */
		    angle = 0;
		}

		map3d_position_r(&(Y_AXIS.label.offset), &tmpx, &tmpy, "graphbox");
		x1 += tmpx; /* user-defined label offset */
		y1 += tmpy;

		/* write_multiline mods it */
		ignore_enhanced(Y_AXIS.label.noenhanced);
		apply_pm3dcolor(&(Y_AXIS.label.textcolor),t);

		if (angle != 0 && splot_map && (term->text_angle)(angle)) {
		    write_multiline(x1, y1, Y_AXIS.label.text, h_just, v_just,
				    angle, Y_AXIS.label.font);
		    (term->text_angle)(0);
		} else {
		    write_multiline(x1, y1, Y_AXIS.label.text, h_just, v_just,
				    0, Y_AXIS.label.font);
		}
		
		reset_textcolor(&(Y_AXIS.label.textcolor),t);
		ignore_enhanced(FALSE);
#ifdef USE_GRID_LAYERS
	    }
#endif
	}
    }

    /* do z tics */
    if (Z_AXIS.ticmode
#ifdef USE_GRID_LAYERS
	/* Don't output tics and grids if this is the front part of a
	 * two-part grid drawing process: */
	&& (FRONTGRID != whichgrid)
#endif
	&& (splot_map == FALSE)
	&& (draw_surface
	    || (draw_contour & CONTOUR_SRF)
	    || strchr(pm3d.where,'s') != NULL
	    )
	) {
	gen_tics(FIRST_Z_AXIS, ztick_callback);
    }
    if ((Y_AXIS.zeroaxis.l_type > LT_NODRAW)
	&& !X_AXIS.log
	&& inrange(0, X_AXIS.min, X_AXIS.max)
	) {
	vertex v1, v2;

	/* line through x=0 */
	map3d_xyz(0.0, Y_AXIS.min, base_z, &v1);
	map3d_xyz(0.0, Y_AXIS.max, base_z, &v2);
	draw3d_line(&v1, &v2, &Y_AXIS.zeroaxis);
    }
    if ((Z_AXIS.zeroaxis.l_type > LT_NODRAW)
	&& !X_AXIS.log
	&& inrange(0, X_AXIS.min, X_AXIS.max)
	) {
	vertex v1, v2;

	/* line through x=0 y=0 */
	map3d_xyz(0.0, 0.0, Z_AXIS.min, &v1);
	map3d_xyz(0.0, 0.0, Z_AXIS.max, &v2);
	draw3d_line(&v1, &v2, &Z_AXIS.zeroaxis);
    }
    if ((X_AXIS.zeroaxis.l_type > LT_NODRAW)
	&& !Y_AXIS.log
	&& inrange(0, Y_AXIS.min, Y_AXIS.max)
	) {
	vertex v1, v2;

	term_apply_lp_properties(&X_AXIS.zeroaxis);
	/* line through y=0 */
	map3d_xyz(X_AXIS.min, 0.0, base_z, &v1);
	map3d_xyz(X_AXIS.max, 0.0, base_z, &v2);
	draw3d_line(&v1, &v2, &X_AXIS.zeroaxis);
    }
    /* PLACE ZLABEL - along the middle grid Z axis - eh ? */
    if (Z_AXIS.label.text
	&& (splot_map == FALSE)
	&& (draw_surface
	    || (draw_contour & CONTOUR_SRF)
	    || strpbrk(pm3d.where,"st") != NULL
	    )
	) {
	int tmpx, tmpy;
	vertex v1;
	int h_just = CENTRE;
	int v_just = JUST_TOP;
	double other_end = X_AXIS.min + X_AXIS.max - zaxis_x;
	double mid_z = (Z_AXIS.max + Z_AXIS.min) / 2.;
	double step = (other_end - zaxis_x) / 4.;

	if (Z_AXIS.ticmode & TICS_ON_AXIS) {
	    map3d_xyz(0, 0, mid_z, &v1);
	    TERMCOORD(&v1, x, y);
	    x -= 5 * t->h_char;
	    h_just = RIGHT;
	} else {
	    map3d_xyz(zaxis_x - step, zaxis_y, mid_z, &v1);
	    TERMCOORD(&v1, x, y);
	    h_just = CENTRE;
	}

	map3d_position_r(&(Z_AXIS.label.offset), &tmpx, &tmpy, "graphbox");
	x += tmpx;
	y += tmpy;

	ignore_enhanced(Z_AXIS.label.noenhanced);
	apply_pm3dcolor(&(Z_AXIS.label.textcolor),t);
	if (Z_AXIS.label.rotate != 0 && (term->text_angle)(Z_AXIS.label.rotate)) {
	    write_multiline(x, y, Z_AXIS.label.text,
			    h_just, v_just, Z_AXIS.label.rotate, Z_AXIS.label.font);
	    (term->text_angle)(0);
	} else {
	    write_multiline(x, y, Z_AXIS.label.text,
			    h_just, v_just, 0, Z_AXIS.label.font);
	}
	reset_textcolor(&(Z_AXIS.label.textcolor),t);
	ignore_enhanced(FALSE);
    }

    if (splot_map)
	clip_area = clip_save;
}

/* HBB 20010118: all the *_callback() functions made non-static. This
 * is necessary to work around a bug in HP's assembler shipped with
 * HP-UX 10 and higher, if GCC tries to use it */

void
xtick_callback(
    AXIS_INDEX axis,
    double place,
    char *text,
    struct lp_style_type grid,		/* linetype or -2 for none */
    struct ticmark *userlabels)	/* currently ignored in 3D plots */
{
    vertex v1, v2;
    double scale = (text ? axis_array[axis].ticscale : axis_array[axis].miniticscale) * (axis_array[axis].tic_in ? 1 : -1);
    double other_end =
	Y_AXIS.min + Y_AXIS.max - xaxis_y;
    struct termentry *t = term;

    (void) axis;		/* avoid -Wunused warning */

    map3d_xyz(place, xaxis_y, base_z, &v1);
    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	/* to save mapping twice, map non-axis y */
	map3d_xyz(place, other_end, base_z, &v2);
	draw3d_line(&v1, &v2, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    if ((X_AXIS.ticmode & TICS_ON_AXIS)
	&& !Y_AXIS.log
	&& inrange (0.0, Y_AXIS.min, Y_AXIS.max)
	) {
	map3d_xyz(place, 0.0, base_z, &v1);
    }
    v2.x = v1.x + tic_unitx * scale * t->v_tic ;
    v2.y = v1.y + tic_unity * scale * t->v_tic ;
    v2.z = v1.z + tic_unitz * scale * t->v_tic ;
    v2.real_z = v1.real_z;
    draw3d_line(&v1, &v2, &border_lp);
    term_apply_lp_properties(&border_lp);

    if (text) {
	int just;
	unsigned int x2, y2;
	int angle;

	/* get offset */
	int offsetx, offsety;
	map3d_position_r(&(axis_array[axis].ticdef.offset),
		       &offsetx, &offsety, "xtics");
	if (tic_unitx * xscaler < -0.9)
	    just = LEFT;
	else if (tic_unitx * xscaler < 0.9)
	    just = CENTRE;
	else
	    just = RIGHT;
	v2.x = v1.x - tic_unitx * t->h_char * 1;
	v2.y = v1.y - tic_unity * t->v_char * 1;
	if (!axis_array[axis].tic_in) {
	    v2.x -= tic_unitx * t->v_tic * axis_array[axis].ticscale;
	    v2.y -= tic_unity * t->v_tic * axis_array[axis].ticscale;
	}
	TERMCOORD(&v2, x2, y2);
	/* User-specified different color for the tics text */
	if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(axis_array[axis].ticdef.textcolor), t);
	angle = axis_array[axis].tic_rotate;
	if (!(splot_map && angle && term->text_angle(angle)))
	    angle = 0;
	write_multiline(x2+offsetx, y2+offsety, text, just, JUST_TOP,
			    angle, axis_array[axis].ticdef.font);
	term->text_angle(0);
	term_apply_lp_properties(&border_lp);
    }

    if (X_AXIS.ticmode & TICS_MIRROR) {
	map3d_xyz(place, other_end, base_z, &v1);
	v2.x = v1.x - tic_unitx * scale * t->v_tic;
	v2.y = v1.y - tic_unity * scale * t->v_tic;
	v2.z = v1.z - tic_unitz * scale * t->v_tic;
	v2.real_z = v1.real_z;
	draw3d_line(&v1, &v2, &border_lp);
    }
}

void
ytick_callback(
    AXIS_INDEX axis,
    double place,
    char *text,
    struct lp_style_type grid,
    struct ticmark *userlabels)	/* currently ignored in 3D plots */
{
    vertex v1, v2;
    double scale = (text ? axis_array[axis].ticscale : axis_array[axis].miniticscale) * (axis_array[axis].tic_in ? 1 : -1);
    double other_end =
	X_AXIS.min + X_AXIS.max - yaxis_x;
    struct termentry *t = term;

    (void) axis;		/* avoid -Wunused warning */

    map3d_xyz(yaxis_x, place, base_z, &v1);
    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	map3d_xyz(other_end, place, base_z, &v2);
	draw3d_line(&v1, &v2, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    if (Y_AXIS.ticmode & TICS_ON_AXIS
	&& !X_AXIS.log
	&& inrange (0.0, X_AXIS.min, X_AXIS.max)
	) {
	map3d_xyz(0.0, place, base_z, &v1);
    }

    v2.x = v1.x + tic_unitx * scale * t->h_tic;
    v2.y = v1.y + tic_unity * scale * t->h_tic;
    v2.z = v1.z + tic_unitz * scale * t->h_tic;
    v2.real_z = v1.real_z;
    draw3d_line(&v1, &v2, &border_lp);

    if (text) {
	int just;
	unsigned int x2, y2;
	int angle;

	/* get offset */
	int offsetx, offsety;
	map3d_position_r(&(axis_array[axis].ticdef.offset),
		       &offsetx, &offsety, "ytics");

	if (tic_unitx * xscaler < -0.9)
	    just = LEFT;
	else if (tic_unitx * xscaler < 0.9)
	    just = CENTRE;
	else
	    just = RIGHT;
	v2.x = v1.x - tic_unitx * t->h_char * 1;
	v2.y = v1.y - tic_unity * t->v_char * 1;
	if (!axis_array[axis].tic_in) {
	    v2.x -= tic_unitx * t->h_tic * axis_array[axis].ticscale;
	    v2.y -= tic_unity * t->v_tic * axis_array[axis].ticscale;
	}
	/* User-specified different color for the tics text */
	if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(axis_array[axis].ticdef.textcolor), t);
	TERMCOORD(&v2, x2, y2);
	angle = axis_array[axis].tic_rotate;
	if (!(splot_map && angle && term->text_angle(angle)))
	    angle = 0;
	write_multiline(x2+offsetx, y2+offsety, text, just, JUST_TOP,
			angle, axis_array[axis].ticdef.font);
	term->text_angle(0);
	term_apply_lp_properties(&border_lp);
    }

    if (Y_AXIS.ticmode & TICS_MIRROR) {
	map3d_xyz(other_end, place, base_z, &v1);
	v2.x = v1.x - tic_unitx * scale * t->h_tic;
	v2.y = v1.y - tic_unity * scale * t->h_tic;
	v2.z = v1.z - tic_unitz * scale * t->h_tic;
	v2.real_z = v1.real_z;
	draw3d_line(&v1, &v2, &border_lp);
    }
}

void
ztick_callback(
    AXIS_INDEX axis,
    double place,
    char *text,
    struct lp_style_type grid,
    struct ticmark *userlabels)	/* currently ignored in 3D plots */
{
    /* HBB: inserted some ()'s to shut up gcc -Wall, here and below */
    int len = (text ? axis_array[axis].ticscale : axis_array[axis].miniticscale)
	* (axis_array[axis].tic_in ? 1 : -1) * (term->h_tic);
    vertex v1, v2, v3;
    struct termentry *t = term;

    (void) axis;		/* avoid -Wunused warning */

    if (axis_array[axis].ticmode & TICS_ON_AXIS)
	map3d_xyz(0., 0., place, &v1);
    else
	map3d_xyz(zaxis_x, zaxis_y, place, &v1);
    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	map3d_xyz(back_x, back_y, place, &v2);
	map3d_xyz(right_x, right_y, place, &v3);
	draw3d_line(&v1, &v2, &grid);
	draw3d_line(&v2, &v3, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    v2.x = v1.x + len / (double)xscaler;
    v2.y = v1.y;
    v2.z = v1.z;
    v2.real_z = v1.real_z;
    draw3d_line(&v1, &v2, &border_lp);

    if (text) {
	unsigned int x1, y1;
	/* get offset */
	int offsetx, offsety;
	map3d_position_r(&(axis_array[axis].ticdef.offset),
		       &offsetx, &offsety, "ztics");

	TERMCOORD(&v1, x1, y1);
	x1 -= (term->h_tic) * 2;
	if (!axis_array[axis].tic_in)
	    x1 -= (term->h_tic) * axis_array[axis].ticscale;
	/* User-specified different color for the tics text */
	if (axis_array[axis].ticdef.textcolor.type == TC_Z)
	    axis_array[axis].ticdef.textcolor.value = place;
	if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(axis_array[axis].ticdef.textcolor), term);
	write_multiline(x1+offsetx, y1+offsety, text, RIGHT, JUST_CENTRE,
			0, axis_array[axis].ticdef.font);
	term_apply_lp_properties(&border_lp);
    }

    if (Z_AXIS.ticmode & TICS_MIRROR) {
	map3d_xyz(right_x, right_y, place, &v1);
	v2.x = v1.x - len / (double)xscaler;
	v2.y = v1.y;
	v2.z = v1.z;
	v2.real_z = v1.real_z;
	draw3d_line(&v1, &v2, &border_lp);
    }
}

static int
map3d_getposition(
    struct position *pos,
    const char *what,
    double *xpos, double *ypos, double *zpos)
{
    TBOOLEAN screen_coords = FALSE;
    TBOOLEAN char_coords = FALSE;
    TBOOLEAN plot_coords = FALSE;

    switch (pos->scalex) {
    case first_axes:
    case second_axes:
	*xpos = axis_log_value_checked(FIRST_X_AXIS, *xpos, what);
	plot_coords = TRUE;
	break;
    case graph:
	*xpos = X_AXIS.min + *xpos * (X_AXIS.max - X_AXIS.min);
	plot_coords = TRUE;
	break;
    case screen:
	*xpos = *xpos * (term->xmax -1) + 0.5;
	screen_coords = TRUE;
	break;
    case character:
	*xpos = *xpos * term->h_char + 0.5;
	char_coords = TRUE;
	break;
    }

    switch (pos->scaley) {
    case first_axes:
    case second_axes:
	*ypos = axis_log_value_checked(FIRST_Y_AXIS, *ypos, what);
	plot_coords = TRUE;
	break;
    case graph:
	if (splot_map)
	    *ypos = Y_AXIS.max - *ypos * (Y_AXIS.max - Y_AXIS.min);
	else
	    *ypos = Y_AXIS.min + *ypos * (Y_AXIS.max - Y_AXIS.min);
	plot_coords = TRUE;
	break;
    case screen:
	*ypos = *ypos * (term->ymax -1) + 0.5;
	screen_coords = TRUE;
	break;
    case character:
	*ypos = *ypos * term->v_char + 0.5;
	char_coords = TRUE;
	break;
    }

    switch (pos->scalez) {
    case first_axes:
    case second_axes:
	*zpos = axis_log_value_checked(FIRST_Z_AXIS, *zpos, what);
	plot_coords = TRUE;
	break;
    case graph:
	*zpos = Z_AXIS.min + *zpos * (Z_AXIS.max - Z_AXIS.min);
	plot_coords = TRUE;
	break;
    case screen:
	screen_coords = TRUE;
	break;
    case character:
	char_coords = TRUE;
	break;
    }

    if (plot_coords && (screen_coords || char_coords))
	graph_error("Cannot mix screen or character coords with plot coords");

    return (screen_coords || char_coords);
}

/*
 * map3d_position()  wrapper for map3d_position_double
 */
void
map3d_position(
    struct position *pos,
    int *x, int *y,
    const char *what)
{
    double xx, yy;

    map3d_position_double(pos, &xx, &yy, what);
    *x = xx;
    *y = yy;
}

void
map3d_position_double(
    struct position *pos,
    double *x, double *y,
    const char *what)
{
    double xpos = pos->x;
    double ypos = pos->y;
    double zpos = pos->z;

    if (map3d_getposition(pos, what, &xpos, &ypos, &zpos) == 0) {
	map3d_xy_double(xpos, ypos, zpos, x, y);
    } else {
	/* Screen or character coordinates */
	*x = xpos;
	*y = ypos;
    }
}

void
map3d_position_r(
    struct position *pos,
    int *x, int *y,
    const char *what)
{
    double xpos = pos->x;
    double ypos = pos->y;
    double zpos = pos->z;

    /* startpoint in graph coordinates */
    if (map3d_getposition(pos, what, &xpos, &ypos, &zpos) == 0) {
	int xoriginlocal, yoriginlocal;
	double xx, yy;
	map3d_xy_double(xpos, ypos, zpos, &xx, &yy);
	*x = xx;
	*y = yy;
	if (pos->scalex == graph)
	    xpos = X_AXIS.min;
	else
	    xpos = 0;
	if (pos->scaley == graph)
	    ypos = (splot_map) ? Y_AXIS.max : Y_AXIS.min;
	else
	    ypos = (splot_map) ? Y_AXIS.max : 0;
	if (pos->scalez == graph)
	    zpos = Z_AXIS.min;
	else
	    zpos = 0;
	map3d_xy(xpos, ypos, zpos, &xoriginlocal, &yoriginlocal);
	*x -= xoriginlocal;
	*y -= yoriginlocal;
    } else {
    /* endpoint `screen' or 'character' coordinates */
	*x = xpos;
	*y = ypos;
    }
    return;
}

/*
 * these code blocks were moved to functions, to make the code simpler
 */

static void
key_text(int xl, int yl, char *text)
{
    legend_key *key = &keyT;

    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
    if (key->just == GPKEY_LEFT && key->region != GPKEY_USER_PLACEMENT) {
	(*term->justify_text) (LEFT);
	(*term->put_text) (xl + key_text_left, yl, text);
    } else {
	if ((*term->justify_text) (RIGHT)) {
	    (*term->put_text) (xl + key_text_right, yl, text);
	} else {
	    int x = xl + key_text_right - (term->h_char) * estimate_strlen(text);
	    (*term->put_text) (x, yl, text);
	}
    }
    (term->layer)(TERM_LAYER_END_KEYSAMPLE);
}

static void
key_sample_line(int xl, int yl)
{
    BoundingBox *clip_save = clip_area;

    /* Clip against canvas */
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
    draw_clip_line(xl + key_sample_left, yl, xl + key_sample_right, yl);
    (term->layer)(TERM_LAYER_END_KEYSAMPLE);

    clip_area = clip_save;
}

static void
key_sample_point(int xl, int yl, int pointtype)
{
    BoundingBox *clip_save = clip_area;

    /* Clip against canvas */
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
    if (!clip_point(xl + key_point_offset, yl))
	(*term->point) (xl + key_point_offset, yl, pointtype);
    (term->layer)(TERM_LAYER_END_KEYSAMPLE);

    clip_area = clip_save;
}


/*
 * returns minimal and maximal values of the cb-range (or z-range if taking the
 * color from the z value) of the given surface
 */
static void
get_surface_cbminmax(struct surface_points *plot, double *cbmin, double *cbmax)
{
    int i, curve = 0;
    TBOOLEAN color_from_column = plot->pm3d_color_from_column; /* just a shortcut */
    coordval cb;
    struct iso_curve *icrvs = plot->iso_crvs;
    struct coordinate GPHUGE *points;

    *cbmin = VERYLARGE;
    *cbmax = -VERYLARGE;

    while (icrvs && curve < plot->num_iso_read) {
	/* fprintf(stderr,"**** NEW ISOCURVE - nb of pts: %i ****\n", icrvs->p_count); */
	for (i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
		/* fprintf(stderr,"  point i=%i => x=%4g y=%4g z=%4lg cb=%4lg\n",i, points[i].x,points[i].y,points[i].z,points[i].CRD_COLOR); */
		if (points[i].type == INRANGE) {
		    /* ?? if (!clip_point(x, y)) ... */
		    cb = color_from_column ? points[i].CRD_COLOR : points[i].z;
		    if (cb < *cbmin) *cbmin = cb;
		    if (cb > *cbmax) *cbmax = cb;
		}
	} /* points on one scan */
	icrvs = icrvs->next;
	curve++;
    } /* surface */
}


/*
 * Draw a gradient color line for a key (legend).
 */
static void
key_sample_line_pm3d(struct surface_points *plot, int xl, int yl)
{
    legend_key *key = &keyT;
    int steps = GPMIN(24, abs(key_sample_right - key_sample_left));
    /* don't multiply by key->swidth --- could be >> palette.maxcolors */
    int x_to = xl + key_sample_right;
    double step = ((double)(key_sample_right - key_sample_left)) / steps;
    int i = 1, x1 = xl + key_sample_left, x2;
    double cbmin, cbmax;
    double gray, gray_from, gray_to, gray_step;
    int colortype = plot->lp_properties.pm3d_color.type;

    /* If plot uses a constant color, set it here and then let simpler routine take over */
    if ((colortype == TC_RGB && plot->lp_properties.pm3d_color.value >= 0.0)
    || (colortype == TC_LT)
    || (colortype == TC_LINESTYLE && plot->lp_properties.l_type != LT_COLORFROMCOLUMN)) {
	apply_pm3dcolor(&(plot->lp_properties.pm3d_color), term);
	key_sample_line(xl,yl);
	return;
    }

    /* color gradient only over the cb-values of the surface, if smaller than the
     * cb-axis range (the latter are gray values [0:1]) */
    get_surface_cbminmax(plot, &cbmin, &cbmax);
    if (cbmin > cbmax) return; /* splot 1/0, for example */
    cbmin = GPMAX(cbmin, CB_AXIS.min);
    cbmax = GPMIN(cbmax, CB_AXIS.max);
    gray_from = cb2gray(cbmin);
    gray_to = cb2gray(cbmax);
    gray_step = (gray_to - gray_from)/steps;

    if (key->region != GPKEY_USER_PLACEMENT)
	(*term->move) (x1, yl);
    else
	clip_move(x1, yl);
    x2 = x1;
    while (i <= steps) {
	/* if (i>1) set_color( i==steps ? 1 : (i-0.5)/steps ); ... range [0:1] */
	gray = (i==steps) ? gray_to : gray_from+i*gray_step;
	set_color(gray);
	(*term->move) (x2, yl);
	x2 = (i==steps) ? x_to : x1 + (int)(i*step+0.5);
	if (key->region != GPKEY_USER_PLACEMENT)
	    (*term->vector) (x2, yl);
	else
	    clip_vector(x2, yl);
	i++;
    }
}


/*
 * Draw a sequence of points with gradient color a key (legend).
 */
static void
key_sample_point_pm3d(
    struct surface_points *plot,
    int xl, int yl,
    int pointtype)
{
    BoundingBox *clip_save = clip_area;
    int x_to = xl + key_sample_right;
    int i = 0, x1 = xl + key_sample_left, x2;
    double cbmin, cbmax;
    double gray, gray_from, gray_to, gray_step;
    int colortype = plot->lp_properties.pm3d_color.type;
    /* rule for number of steps: 3*char_width*pointsize or char_width for dots,
     * but at least 3 points */
    double step = term->h_char * (pointtype == -1 ? 1 : 3*(1+(pointsize-1)/2));
    int steps = (int)(((double)(key_sample_right - key_sample_left)) / step + 0.5);

    if (steps < 2) steps = 2;
    step = ((double)(key_sample_right - key_sample_left)) / steps;

    /* If plot uses a constant color, set it here and then let simpler routine take over */
    if ((colortype == TC_RGB && plot->lp_properties.pm3d_color.value >= 0.0)
    || (colortype == TC_LT)
    || (colortype == TC_LINESTYLE && plot->lp_properties.l_type != LT_COLORFROMCOLUMN)) {
	apply_pm3dcolor(&(plot->lp_properties.pm3d_color), term);
	key_sample_point(xl,yl,pointtype);
	return;
    }

    /* color gradient only over the cb-values of the surface, if smaller than the
     * cb-axis range (the latter are gray values [0:1]) */
    get_surface_cbminmax(plot, &cbmin, &cbmax);
    if (cbmin > cbmax) return; /* splot 1/0, for example */
    cbmin = GPMAX(cbmin, CB_AXIS.min);
    cbmax = GPMIN(cbmax, CB_AXIS.max);
    gray_from = cb2gray(cbmin);
    gray_to = cb2gray(cbmax);
    gray_step = (gray_to - gray_from)/steps;
#if 0
    fprintf(stderr,"POINT_pm3D: cbmin=%g  cbmax=%g\n",cbmin, cbmax);
    fprintf(stderr,"steps=%i gray_from=%g gray_to=%g gray_step=%g\n",steps,gray_from,gray_to,gray_step);
#endif
    /* Clip to canvas */
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    while (i <= steps) {
	/* if (i>0) set_color( i==steps ? gray_to : (i-0.5)/steps ); ... range [0:1] */
	gray = (i==steps) ? gray_to : gray_from+i*gray_step;
	set_color(gray);
	x2 = i==0 ? x1 : (i==steps ? x_to : x1 + (int)(i*step+0.5));
	/* x2 += key_point_offset; ... that's if there is only 1 point */
	if (!clip_point(x2, yl))
	    (*term->point) (x2, yl, pointtype);
	i++;
    }

    clip_area = clip_save;
}


/* plot_vectors:
 * Plot the curves in VECTORS style
 */
static void
plot3d_vectors(struct surface_points *plot)
{
    int i;
    double x1, y1, x2, y2;
    arrow_style_type ap;
    struct coordinate GPHUGE *heads = plot->iso_crvs->points;
    struct coordinate GPHUGE *tails = plot->iso_crvs->next->points;

    /* Only necessary once, unless variable arrow style */
    ap = plot->arrow_properties;
    term_apply_lp_properties(&ap.lp_properties);
    apply_3dhead_properties(&ap);

    for (i = 0; i < plot->iso_crvs->p_count; i++) {
	
	if (heads[i].type == UNDEFINED || tails[i].type == UNDEFINED)
	    continue;

	/* variable arrow style read from extra data column */
	if (plot->arrow_properties.tag == AS_VARIABLE) {
	    int as= heads[i].CRD_COLOR;
	    arrow_use_properties(&ap, as);
	    term_apply_lp_properties(&ap.lp_properties);
	    apply_head_properties(&ap);
	} else {
	    check_for_variable_color(plot, &heads[i]);
	}

	if (heads[i].type == INRANGE && tails[i].type == INRANGE) {
	    map3d_xy_double(heads[i].x, heads[i].y, heads[i].z, &x1, &y1);
	    map3d_xy_double(tails[i].x, tails[i].y, tails[i].z, &x2, &y2);
	    draw_clip_arrow((int)x1, (int)y1, (int)x2, (int)y2, ap.head);
	}
    }
}

static void
check_for_variable_color(struct surface_points *plot, struct coordinate *point)
{
    int colortype = plot->lp_properties.pm3d_color.type;

    switch( colortype ) {
    case TC_RGB:
	if (plot->pm3d_color_from_column)
	    set_rgbcolor( (int)point->CRD_COLOR );
	break;
    case TC_Z:
    case TC_DEFAULT:   /* pm3d mode assumes this is default */
	if (can_pm3d && plot->lp_properties.use_palette) {
	    if (plot->pm3d_color_from_column)
		set_color( cb2gray(point->CRD_COLOR) );
	    else
		set_color( cb2gray( z2cb(point->z) ) );
	}
	break;
    case TC_LINESTYLE:	/* color from linestyle in data column */
	plot->lp_properties.pm3d_color.lt = (int)(point->CRD_COLOR);
	apply_pm3dcolor(&(plot->lp_properties.pm3d_color), term);
	break;
    default:
	/* The other cases were taken care of already */
	break;
    }
}
