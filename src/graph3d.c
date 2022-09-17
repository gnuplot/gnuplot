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
#include "command.h"
#include "contour.h"
#include "gadgets.h"
#include "hidden3d.h"
#include "misc.h"
#include "term_api.h"
#include "util3d.h"
#include "util.h"
#include "vplot.h"

#include "pm3d.h"
#include "plot3d.h"
#include "color.h"

#include "plot.h"

static int key_entry_height;	/* bigger of t->v_char, pointsize*t->v_tick */
static int key_title_height;
static int key_title_extra;	/* allow room for subscript/superscript */
static int key_title_width;

/* is contouring wanted ? */
t_contour_placement draw_contour = CONTOUR_NONE;
TBOOLEAN clabel_onecolor = FALSE;	/* use same linetype for all contours */
int clabel_interval = 20;		/* label every 20th contour segment */
int clabel_start = 5;			/*       starting with the 5th */
char *clabel_font = NULL;		/* default to current font */

/* Draw the surface at all? (FALSE if only contours are wanted) */
TBOOLEAN draw_surface = TRUE;
/* Always create a gridded surface when lines are read from a data file */
TBOOLEAN implicit_surface = TRUE;

/* Was hidden3d display selected by user? */
TBOOLEAN hidden3d = FALSE;
int hidden3d_layer = LAYER_BACK;

/* Rotation and scale of the 3d view, as controlled by 'set view': */
float surface_rot_z = 30.0;
float surface_rot_x = 60.0;
float surface_scale = 1.0;
float surface_zscale = 1.0;
float surface_lscale = 0.0;
float mapview_scale = 1.0;
float azimuth = 0.0;

/* These indicate projection onto the xy, xz or yz plane
 * as requested by 'set view map' or 'set view projection'
 */
TBOOLEAN splot_map = FALSE;
TBOOLEAN xz_projection = FALSE;
TBOOLEAN yz_projection = FALSE;

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

static void do_3dkey_layout(legend_key *key, int *xinkey, int *yinkey);
static void plot3d_impulses(struct surface_points * plot);
static void plot3d_lines(struct surface_points * plot);
static void plot3d_points(struct surface_points * plot);
static void plot3d_polygons(struct surface_points * plot);
static void plot3d_zerrorfill(struct surface_points * plot);
static void plot3d_boxes(struct surface_points * plot);
static void plot3d_vectors(struct surface_points * plot);
static void plot3d_lines_pm3d(struct surface_points * plot);
static void get_surface_cbminmax(struct surface_points *plot, double *cbmin, double *cbmax);
static void cntr3d_impulses(struct gnuplot_contours * cntr, struct lp_style_type * lp);
static void cntr3d_lines(struct gnuplot_contours * cntr, struct lp_style_type * lp);
static void cntr3d_points(struct gnuplot_contours * cntr, struct lp_style_type * lp);
static void cntr3d_labels(struct gnuplot_contours * cntr, char * leveltext,
				   struct text_label * label);
static void check_corner_height(struct coordinate * point,
					 double height[2][2], double depth[2][2]);
static void setup_3d_box_corners(void);
static void draw_3d_graphbox(struct surface_points * plot,
				      int plot_count,
				      WHICHGRID whichgrid, int current_layer);

static void xtick_callback(struct axis *, double place, char *text, int ticlevel,
			   struct lp_style_type grid, struct ticmark *userlabels);
static void ytick_callback(struct axis *, double place, char *text, int ticlevel,
			   struct lp_style_type grid, struct ticmark *userlabels);
static void ztick_callback(struct axis *, double place, char *text, int ticlevel,
			   struct lp_style_type grid, struct ticmark *userlabels);

static int find_maxl_cntr(struct gnuplot_contours * contours, int *count);
static int find_maxl_keys3d(struct surface_points *plots, int count, int *kcnt);
static void boundary3d(struct surface_points * plots, int count);

/* put entries in the key */
static void key_sample_line(int xl, int yl);
static void key_sample_point(struct surface_points *this_plot, int xl, int yl, int pointtype);
static void key_sample_line_pm3d(struct surface_points *plot, int xl, int yl);
static void key_sample_point_pm3d(struct surface_points *plot, int xl, int yl, int pointtype);
static void key_sample_fill(int xl, int yl, struct surface_points *this_plot);
static TBOOLEAN can_pm3d = FALSE;
static void key_text(int xl, int yl, char *text);
static void check3d_for_variable_color(struct surface_points *plot, struct coordinate *point);

static TBOOLEAN get_arrow3d(struct arrow_def*, double*, double*, double*, double*);
static void place_arrows3d(int);
static void place_labels3d(struct text_label * listhead, int layer);
static int map3d_getposition(struct position* pos, const char* what, double* xpos, double* ypos, double* zpos);

static void flip_projection_axis(struct axis *axis);
static void splot_map_activate(void);
static void splot_map_deactivate(void);

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
double xyscaler;
double radius_scaler;
static int ptitl_cnt;
static int max_ptitl_len;
static int titlelin;
static int key_sample_width, key_rows, key_cols, key_col_wth, yl_ref;
static double ktitle_lines = 0;


/* Boundary and scale factors, in user coordinates */

/* These positions assume a single linear scale encompassing the
 * zrange plus extra space below for the baseplane.  This was messy but
 * correct before the introduction of nonlinear axes. Now - not so much.
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

/* To handle a non-linear z axis we need to calculate these values on
 * the other end of the linked linear:nonlinear axis pair.
 */
double floor_z1;	/* Used also by map_z3d() */
static double ceiling_z1, base_z1;

transform_matrix trans_mat;

/* x and y input range endpoints where the three axes are to be
 * displayed (left, front-left, and front-right edges of the cube) */
static double xaxis_y, yaxis_x, zaxis_x, zaxis_y;

/* ... and the same for the back, right, and front corners */
static double back_x, back_y;
static double right_x, right_y;
static double front_x, front_y;

/* The global flags splot_map, xz_projection, and yz_projection are specific views.
 * These flag the more general case of looking down the x or y axis
 */
static TBOOLEAN xz_plane, yz_plane;

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
	 * drawing either surface, or unlabeled contours
	 */
	if (this_plot->title && *this_plot->title
	&& !this_plot->title_is_suppressed && !this_plot->title_position) {
	    ++cnt;
	    len = estimate_strlen(this_plot->title, NULL);
	    if (len > mlen)
		mlen = len;
	}
	if (draw_contour && !clabel_onecolor && this_plot->contours != NULL
	&&  !this_plot->title_is_suppressed
	&&  this_plot->plot_style != LABELPOINTS) {
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
	if (cntrs->isNewLevel) {
	    len = estimate_strlen(cntrs->label, NULL)
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
    int i;

    titlelin = 0;

    if (key->swidth >= 0)
	key_sample_width = key->swidth * t->h_char + t->h_tic;
    else
	key_sample_width = 0;
    key_entry_height = t->v_tic * 1.25 * key->vert_factor;
    if (key_entry_height < t->v_char) {
	/* is this reasonable ? */
	key_entry_height = t->v_char * key->vert_factor;
    }

    /* Approximate width of titles is used to determine number of rows, cols
     * The actual widths will be recalculated later
     */
    max_ptitl_len = find_maxl_keys3d(plots, count, &ptitl_cnt);
    key_title_width = label_width(key->title.text, &i) * t->h_char;
    ktitle_lines = i;
    key_col_wth = (max_ptitl_len + 4) * t->h_char + key_sample_width;

    if (lmargin.scalex == screen)
	plot_bounds.xleft = lmargin.x * (double)t->xmax + 0.5;
    else if (lmargin.x >= 0)
	plot_bounds.xleft = lmargin.x * (double)t->h_char + 0.5;
    else
	plot_bounds.xleft = t->h_char * 2 + t->h_tic;

    if (rmargin.scalex == screen)
	plot_bounds.xright = rmargin.x * (double)t->xmax + 0.5;
    else if (splot_map && rmargin.x >= 0)
	plot_bounds.xright = xsize * t->xmax - (rmargin.x * t->h_char + 0.5);
    else
	plot_bounds.xright = xsize * t->xmax - t->h_char * 2 - t->h_tic;

    key_rows = ptitl_cnt;
    key_cols = 1;
    if (key_rows > key->maxrows && key->maxrows > 0) {
	key_rows = key->maxrows;
	key_cols = (ptitl_cnt - 1)/key_rows + 1;
    }

    if (key->visible)
    if ((key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
	&& key->margin == GPKEY_BMARGIN) {
	if (ptitl_cnt > 0) {
	    /* calculate max no cols, limited by label-length */
	    key_cols = (int) (plot_bounds.xright - plot_bounds.xleft)
		     / ((max_ptitl_len + 4) * t->h_char + key_sample_width);
	    if (key_cols == 0)
		key_cols = 1;
	    key_rows = (int) ((ptitl_cnt - 1)/ key_cols) + 1;
	    /* Limit the number of rows if requested by user */
	    if (key_rows > key->maxrows && key->maxrows > 0)
		key_rows = key->maxrows;
	    /* now calculate actual no cols depending on no rows */
	    key_cols = (int) ((ptitl_cnt - 1)/ key_rows) + 1;
	    key_col_wth = (int) (plot_bounds.xright - plot_bounds.xleft) / key_cols;
	} else {
	    key_rows = key_cols = key_col_wth = 0;
	}
    }

    /* Sanity check top and bottom margins, in case the user got confused */
    if (bmargin.scalex == screen && tmargin.scalex == screen)
	if (bmargin.x > tmargin.x) {
	    double tmp = bmargin.x;
	    bmargin.x = tmargin.x;
	    tmargin.x = tmp;
	}

    /* this should also consider the view and number of lines in
     * xformat || yformat || xlabel || ylabel */

    if (bmargin.scalex == screen)
	plot_bounds.ybot = bmargin.x * (double)t->ymax + 0.5;
    else if (splot_map && bmargin.x >= 0)
	plot_bounds.ybot = (double)t->v_char * bmargin.x;
    else
	plot_bounds.ybot = t->v_char * 2.5 + 1;

    if (key->visible)
    if (key_rows && (key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
	&& key->margin == GPKEY_BMARGIN)
	plot_bounds.ybot += key_rows * key_entry_height + key_title_height;

    if (title.text) {
	titlelin++;
	for (i = 0; i < strlen(title.text); i++) {
	    if (title.text[i] == '\\')
		titlelin++;
	}
    }

    if (tmargin.scalex == screen)
	plot_bounds.ytop = tmargin.x * (double)t->ymax + 0.5;
    else if (splot_map && tmargin.x >= 0)
	plot_bounds.ytop = ysize * t->ymax - t->v_char * (titlelin + tmargin.x) - 1;
    else
	plot_bounds.ytop = ysize * t->ymax - t->v_char * (titlelin + 1.5) - 1;

    if (key->visible)
    if (key->region == GPKEY_AUTO_INTERIOR_LRTBC
	|| ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN)
	    && key->margin == GPKEY_RMARGIN)) {
	/* calculate max no rows, limited by plot_bounds.ytop-plot_bounds.ybot */
	i = (int) (plot_bounds.ytop - plot_bounds.ybot) / t->v_char - 1 - ktitle_lines;
	if (i > key->maxrows && key->maxrows > 0)
	    i = key->maxrows;
	if (i <= 0)
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
	int key_width = key_col_wth * key_cols - 2 * t->h_char;
	if (rmargin.scalex != screen)
	    plot_bounds.xright -= key_width;
    }

    if (key->visible)
    if ((key->region == GPKEY_AUTO_EXTERIOR_LRTBC || key->region == GPKEY_AUTO_EXTERIOR_MARGIN)
	&& key->margin == GPKEY_LMARGIN) {
	int key_width = key_col_wth * key_cols - 2 * t->h_char;
	if (lmargin.scalex != screen)
	    plot_bounds.xleft += key_width;
    }

    /* Make room for the colorbar to the right of the plot */
    if (splot_map && is_plot_with_colorbox() && rmargin.scalex != screen) {
	if ((color_box.where != SMCOLOR_BOX_NO) && (color_box.where != SMCOLOR_BOX_USER))
	    plot_bounds.xright -= 0.1 * (plot_bounds.xright-plot_bounds.xleft);
	color_box.xoffset = 0;
    }

    /* Leave room for optional x2label and y2label in "set view map" mode.
     * These estimates are crude compared to the effort we make in 2D plots
     * but the user can adjust it with "offset" and "set rmargin".
     * Later we will adjust the colorbox position by the same amount.
     */
    if (splot_map && axis_array[SECOND_X_AXIS].label.text)
	plot_bounds.ytop -= 1.5 * t->v_char;
    if (splot_map && axis_array[SECOND_Y_AXIS].label.text)
	plot_bounds.xright -= 2.5 * t->h_char;

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

    /* Allow explicit control via set {}margin screen */
    if (tmargin.scalex == screen && bmargin.scalex == screen)
	yscaler = (plot_bounds.ytop - plot_bounds.ybot) / surface_scale;
    if (rmargin.scalex == screen && lmargin.scalex == screen)
	xscaler = (plot_bounds.xright - plot_bounds.xleft) / surface_scale;

    /* prevent infinite loop or divide-by-zero if scaling is bad */
    if (yscaler == 0) yscaler = 1;
    if (xscaler == 0) xscaler = 1;

    /* 'set size {square|ratio}' for splots */
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

    /* For anything that really wants to be the same on x and y */
    xyscaler = sqrt((double)xscaler * (double)yscaler);

    /* This one is used to scale circles in 3D plots */
    radius_scaler = xscaler * surface_scale / (X_AXIS.max - X_AXIS.min);

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
    double* dsx, double* dsy,
    double* dex, double* dey)
{
    map3d_position_double(&(arrow->start), dsx, dsy, "arrow");

    if (arrow->type == arrow_end_relative) {
	map3d_position_r_double(&(arrow->end), dex, dey, "arrow");
	*dex += *dsx;
	*dey += *dsy;
    } else if (arrow->type == arrow_end_oriented) {
	double aspect = (double)term->v_tic / (double)term->h_tic;
	double radius;
	double junkw, junkh;

#ifdef _WIN32
	if (strcmp(term->name, "windows") == 0)
	    aspect = 1.;
#endif
	if (arrow->end.scalex != screen && arrow->end.scalex != character && !splot_map)
	    return FALSE;
	map3d_position_r_double(&arrow->end, &junkw, &junkh, "arrow");
	radius = junkw;
	*dex = *dsx + cos(DEG2RAD * arrow->angle) * radius;
	*dey = *dsy + sin(DEG2RAD * arrow->angle) * radius * aspect;
    } else {
	map3d_position_double(&(arrow->end), dex, dey, "arrow");
    }

    return TRUE;
}

static void
place_labels3d(struct text_label *listhead, int layer)
{
    struct text_label *this_label;
    int x, y;

    term->pointsize(pointsize);

    /* Hypertext labels? */
    /* NB: currently svg is the only terminal that needs this extra step */
    if (layer == LAYER_PLOTLABELS && listhead && listhead->hypertext
    &&  term->hypertext) {
	term->hypertext(TERM_HYPERTEXT_FONT, listhead->font);
    }

    for (this_label = listhead;
	 this_label != NULL;
	 this_label = this_label->next) {

	if (this_label->layer != layer)
	    continue;

	if (layer == LAYER_PLOTLABELS) {
	    double xx, yy;
	    map3d_xy_double(this_label->place.x, this_label->place.y,
		     this_label->place.z, &xx, &yy);
	    x = xx;
	    y = yy;
	    /* Only clip in 2D */
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
	double dsx, dsy, dex, dey;

	if (this_arrow->arrow_properties.layer != layer)
	    continue;
	if (get_arrow3d(this_arrow, &dsx, &dsy, &dex, &dey)) {
	    term_apply_lp_properties(&(this_arrow->arrow_properties.lp_properties));
	    apply_head_properties(&(this_arrow->arrow_properties));
	    draw_clip_arrow(dsx, dsy, dex, dey, this_arrow->arrow_properties.head);
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
    REPLOT_TYPE replot_mode) 	/* replot/refresh/axes-only/quick-refresh */
{
    struct termentry *t = term;
    int surface;
    struct surface_points *this_plot = NULL;
    int xl = 0, yl = 0;
    int xl_save, yl_save;
    int xl_prev = 0, yl_prev = 0;
    int title_x = 0, title_y = 0;
    transform_matrix mat;
    int key_count;
    TBOOLEAN key_pass = FALSE;
    legend_key *key = &keyT;
    TBOOLEAN pm3d_order_depth = FALSE;
    AXIS *primary_z;

    /* Initiate transformation matrix using the global view variables. */
    if (splot_map) {
	splot_map_activate();
    } else if (xz_projection) {
	surface_rot_x = 270.;
	surface_rot_z = 0.;
	surface_scale = 1.425 * mapview_scale;
    } else if (yz_projection) {
	surface_rot_x = 90.;
	surface_rot_z = 90.;
	surface_scale = 1.425 * mapview_scale;
	flip_projection_axis(&axis_array[FIRST_Z_AXIS]);
    }
    mat_rot_z(surface_rot_z, trans_mat);
    mat_rot_x(surface_rot_x, mat);
    mat_mult(trans_mat, trans_mat, mat);
    mat_scale(surface_scale / 2.0, surface_scale / 2.0, surface_scale / 2.0, mat);
    mat_mult(trans_mat, trans_mat, mat);

    /* The azimuth is applied as a rotation about the line of sight */
    if (azimuth !=0 && !splot_map) {
	mat_rot_z(azimuth, mat);
	mat_mult(trans_mat, trans_mat, mat);
    }

    if (polar)
	int_error(NO_CARET,"Cannot splot in polar coordinate system.");

    /* In the case of a nonlinear z axis this points to the linear version */
    /* that shadows it.  Otherwise it just points to FIRST_Z_AXIS.         */
    primary_z = (nonlinear(&Z_AXIS)) ? Z_AXIS.linked_to_primary : &Z_AXIS;

    /* absolute or relative placement of xyplane along z */
    if (nonlinear(&Z_AXIS)) {
	if (xyplane.absolute) {
	    if (primary_z->log && xyplane.z <= 0) {
		base_z1 = eval_link_function(primary_z, Z_AXIS.min);
	    } else {
		base_z1 = eval_link_function(primary_z, xyplane.z);
	    }
	} else {
	    base_z1 = primary_z->min - (primary_z->max - primary_z->min) * xyplane.z;
	}
	base_z = eval_link_function(&Z_AXIS, base_z1);
    } else {
	if (xyplane.absolute)
	    base_z1 = xyplane.z;
	else
	    base_z1 = primary_z->min - (primary_z->max - primary_z->min) * xyplane.z;
	base_z = base_z1;
    }

    /* If we are to draw some portion of the xyplane make sure zmin is updated properly. */
    if (X_AXIS.ticmode || Y_AXIS.ticmode || draw_border & 0x00F) {
	if (primary_z->min > primary_z->max) {
	    floor_z1 = GPMAX(primary_z->min, base_z1);
	    ceiling_z1 = GPMIN(primary_z->max, base_z1);
	} else {
	    floor_z1 = GPMIN(primary_z->min, base_z1);
	    ceiling_z1 = GPMAX(primary_z->max, base_z1);
	}
    } else {
	floor_z1 = primary_z->min;
	ceiling_z1 = primary_z->max;
    }

    if (nonlinear(&Z_AXIS)) {
	floor_z = eval_link_function(&Z_AXIS, floor_z1);
	ceiling_z = eval_link_function(&Z_AXIS, ceiling_z1);
    } else {
	floor_z = floor_z1;
	ceiling_z = ceiling_z1;
    }

    if (X_AXIS.min == X_AXIS.max)
	int_error(NO_CARET,"x_min3d should not equal x_max3d!");
    if (Y_AXIS.min == Y_AXIS.max)
	int_error(NO_CARET,"y_min3d should not equal y_max3d!");
    if (Z_AXIS.min == Z_AXIS.max)
	int_error(NO_CARET,"z_min3d should not equal z_max3d!");

    /* Special case projections of the xz or yz plane */
    /* Place x or y axis to the left of the plot */
    xz_plane = yz_plane = FALSE;
    if (!splot_map && (surface_rot_x == 90 || surface_rot_x == 270)) {
	if (surface_rot_z ==  0 || surface_rot_z == 180) {
	    xz_plane = TRUE;
	    base_z = floor_z;
	}
	if (surface_rot_z == 90 || surface_rot_z == 270) {
	    yz_plane = TRUE;
	    if (surface_rot_x == 270 || yz_projection)
		base_z = ceiling_z;
	}
    }

    term_start_plot();
    (term->layer)(TERM_LAYER_3DPLOT);

    screen_ok = FALSE;

    /* Sync point for epslatex text positioning */
    (term->layer)(TERM_LAYER_BACKTEXT);

    /* now compute boundary for plot */
    boundary3d(plots, pcount);

    axis_set_scale_and_range(&axis_array[FIRST_X_AXIS], plot_bounds.xleft, plot_bounds.xright);
    axis_set_scale_and_range(&axis_array[FIRST_Y_AXIS], plot_bounds.ybot, plot_bounds.ytop);
    axis_set_scale_and_range(&axis_array[FIRST_Z_AXIS], floor_z, ceiling_z);

    /* SCALE FACTORS */
    zscale3d = 2.0 / (ceiling_z - floor_z) * surface_zscale;
    yscale3d = 2.0 / (Y_AXIS.max - Y_AXIS.min);
    xscale3d = 2.0 / (X_AXIS.max - X_AXIS.min);
    if (nonlinear(&X_AXIS))
	xscale3d = 2.0 / (X_AXIS.linked_to_primary->max - X_AXIS.linked_to_primary->min);
    if (nonlinear(&Y_AXIS))
	yscale3d = 2.0 / (Y_AXIS.linked_to_primary->max - Y_AXIS.linked_to_primary->min);
    if (nonlinear(&Z_AXIS))
	zscale3d = 2.0 / (ceiling_z1 - floor_z1) * surface_zscale;

    /* Allow 'set view equal xy' to adjust rendered length of the X and/or Y axes.  */
    /* NB: only works correctly for terminals whose coordinate system is isotropic. */
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

    /* FIXME: I do not understand why this is correct */
    if (nonlinear(&Z_AXIS))
	zcenter3d = 0.0;
    /* Without this the rotation center would be located at */
    /* the bottom of the plot. This places it in the middle.*/
    else
	zcenter3d =  -(ceiling_z - floor_z) / 2.0 * zscale3d + 1;

    /* Needed for mousing by outboard terminal drivers */
    if (splot_map) {
	AXIS *X = &axis_array[FIRST_X_AXIS];
	AXIS *Y = &axis_array[FIRST_Y_AXIS];
	AXIS *Z = &axis_array[FIRST_Z_AXIS];
	int xl, xr, yb, yt;
	map3d_xy(X->min, Y->min, Z->min, &xl, &yb);
	map3d_xy(X->max, Y->max, Z->min, &xr, &yt);
	axis_set_scale_and_range(X, xl, xr);
	axis_set_scale_and_range(Y, yb, yt);
    }

    /* Initialize palette */
    if (replot_mode != AXIS_ONLY_ROTATE) {
	can_pm3d = is_plot_with_palette() && !make_palette()
		   && ((term->flags & TERM_NULL_SET_COLOR) == 0);
    }

    /* Give a chance for rectangles to be behind everything else */
    place_objects( first_object, LAYER_BEHIND, 3);
    if (replot_mode != AXIS_ONLY_ROTATE)
	place_pixmaps(LAYER_BEHIND, 3);

    term_apply_lp_properties(&border_lp);	/* border linetype */

    /* must come before using draw_3d_graphbox() the first time */
    setup_3d_box_corners();

    /* DRAW GRID AND BORDER */
    /* Original behaviour: draw entire grid in back, if 'set grid back': */
    /* HBB 20040331: but not if in hidden3d mode */
    if (splot_map && border_layer != LAYER_FRONT)
	draw_3d_graphbox(plots, pcount, BORDERONLY, LAYER_BACK);

    else if (!hidden3d && (grid_layer == LAYER_BACK))
	draw_3d_graphbox(plots, pcount, ALLGRID, LAYER_BACK);

    else if (!hidden3d && (grid_layer == LAYER_BEHIND))
	/* Default layering mode.  Draw the back part now, but not if
	 * hidden3d is in use, because that relies on all isolated
	 * lines being output after all surfaces have been defined. */
	draw_3d_graphbox(plots, pcount, BACKGRID, LAYER_BACK);

    else if (hidden3d && border_layer == LAYER_BEHIND)
	draw_3d_graphbox(plots, pcount, ALLGRID, LAYER_BACK);

    /* Save state of plot_bounds before applying rotations, etc */
    memcpy(&page_bounds, &plot_bounds, sizeof(page_bounds));

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

    /* Define the clipping area in 3D to lie between the left-most and
     * right-most graph box edges.  This is introduced for the benefit of
     * zooming in the canvas terminal.  It may or may not make any practical
     * difference for other terminals.  If it causes problems, then we will need
     * a separate BoundingBox structure to track the actual 3D graph box.
     */
    else if (azimuth == 0) {
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
	int x, y;
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
	    x = ((map_x1 + map_x2) / 2);
	    y = (map_y1 + tics_len + (titlelin + 0.5) * (t->v_char));
	    if (splot_map && axis_array[SECOND_X_AXIS].ticmode)
		y += 0.5 * t->v_char;
	    if (splot_map && axis_array[SECOND_X_AXIS].label.text)
		y += 1.0 * t->v_char;
	} else { /* usual 3d set view ... */
	    x = (plot_bounds.xleft + plot_bounds.xright) / 2;
	    y = (plot_bounds.ytop + titlelin * (t->h_char));
	}

	/* Save title position for later */
	title_x = x;
	title_y = y;
    }

    /* PLACE TIMELABEL */
    if (timelabel.text) {
	int x, y;
	x = t->v_char;
	y = timelabel_bottom
	  ? yoffset * Y_AXIS.max + t->v_char
	  : plot_bounds.ytop - t->v_char;
	do_timelabel(x,y);
    }

    /* Add 'back' color box */
    if ((replot_mode != AXIS_ONLY_ROTATE)
    && can_pm3d && is_plot_with_colorbox() && color_box.layer == LAYER_BACK)
	draw_color_smooth_box(MODE_SPLOT);

    /* Grid walls */
    place_objects(grid_wall, LAYER_BACK, 3);

    /* pixmaps before objects so that a rectangle can be used as a border */
    place_pixmaps(LAYER_BACK, 3);

    /* Add 'back' rectangles */
    place_objects(first_object, LAYER_BACK, 3);

    /* PLACE LABELS */
    place_labels3d(first_label, LAYER_BACK);

    /* PLACE ARROWS */
    place_arrows3d(LAYER_BACK);

    /* Sync point for epslatex text positioning */
    (term->layer)(TERM_LAYER_FRONTTEXT);

    if (hidden3d && draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
	init_hidden_line_removal();
	reset_hidden_line_removal();
    }

    /* WORK OUT KEY POSITION AND SIZE */
    do_3dkey_layout(key, &xl, &yl);

    /* "set key opaque" requires two passes, with the key drawn in the second pass */
    xl_save = xl; yl_save = yl;
    SECOND_KEY_PASS:

    /* This tells the canvas, qt, and svg terminals to restart the plot   */
    /* count so that key titles are in sync with the plots they describe. */
    (term->layer)(TERM_LAYER_RESET_PLOTNO);

    /* Key box */
    if (key->visible) {
	(term->layer)(TERM_LAYER_KEYBOX);

	/* In two-pass mode, we blank out the key area after the graph	*/
	/* is drawn and then redo the key in the blank area.		*/
	if (key_pass && t->fillbox && !(t->flags & TERM_NULL_SET_COLOR)) {
	    (*t->set_color)(&key->fillcolor);
	    (*t->fillbox)(FS_OPAQUE, key->bounds.xleft, key->bounds.ybot,
		    key->bounds.xright - key->bounds.xleft,
		    key->bounds.ytop - key->bounds.ybot);
	}

	if (key->box.l_type > LT_NODRAW
	&&  key->bounds.ytop != key->bounds.ybot) {
	    term_apply_lp_properties(&key->box);
	    newpath();
	    clip_move(key->bounds.xleft, key->bounds.ybot);
	    clip_vector(key->bounds.xleft, key->bounds.ytop);
	    clip_vector(key->bounds.xright, key->bounds.ytop);
	    clip_vector(key->bounds.xright, key->bounds.ybot);
	    clip_vector(key->bounds.xleft, key->bounds.ybot);
	    closepath();

	    /* draw a horizontal line between key title and first entry  JFi */
	    clip_move(key->bounds.xleft,
			key->bounds.ytop - key_title_height - key_title_extra);
	    clip_vector(key->bounds.xright,
			key->bounds.ytop - key_title_height - key_title_extra);
	}

	if (key->title.text) {
	    int center = (key->bounds.xright + key->bounds.xleft) / 2;
	    int titley = key->bounds.ytop - key_title_height/2;
	    /* FIXME: empirical tweak. I don't know why this is needed */
	    titley += (ktitle_lines-1) * t->v_char/2;
	    write_label(center, titley, &key->title);
	    (*t->linetype)(LT_BLACK);
	}
    }


    /* DRAW SURFACES AND CONTOURS */

    if (!key_pass)
    if (hidden3d && (hidden3d_layer == LAYER_BACK) && draw_surface
    && (replot_mode != AXIS_ONLY_ROTATE)) {
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
#define INVERT_KEY()					\
    do {						\
	yl = key->bounds.ybot + (yl_ref - yl) + (key_entry_height/2); \
    } while (0)

    key_count = 0;
    yl_ref = yl -= key_entry_height / 2;	/* centralise the keys */


    /* PM January 2005: The mistake of missing blank lines in the data file is
     * so frequently made (see questions at comp.graphics.apps.gnuplot) that it
     * really deserves this warning. But don't show it too often --- only if it
     * is a single surface in the plot.
     */
    if (plots->plot_style != BOXES)
    if (pcount == 1 && plots->num_iso_read == 1 && can_pm3d &&
	(plots->plot_style == PM3DSURFACE || PM3D_IMPLICIT == pm3d.implicit))
	    fprintf(stderr, "  Warning: Single isoline (scan) is not enough for a pm3d plot.\n\t   Hint: Missing blank lines in the data file? See 'help pm3d' and FAQ.\n");


    pm3d_order_depth = (can_pm3d && !draw_contour && pm3d.direction == PM3D_DEPTH);

    /* TODO:
     *   During "refresh" from rotation it would be better to re-use previously
     *   built quadrangle list rather than clearing and rebuilding it.
     */
    if (pm3d_order_depth || track_pm3d_quadrangles) {
	pm3d_depth_queue_clear();
	place_objects(first_object, LAYER_DEPTHORDER, 3);
    }

    this_plot = plots;
    if (replot_mode != AXIS_ONLY_ROTATE)
	for (surface = 0;
	     surface < pcount;
	     this_plot = this_plot->next_sp, surface++) {
	    /* just an abbreviation */
	    TBOOLEAN lkey, draw_this_surface;

	    /* Skip over abortive data structures */
	    if (this_plot->plot_type == NODATA)
		continue;

	    /* Sync point for start of new curve (used by svg, post, ...) */
	    (term->layer)(TERM_LAYER_BEFORE_PLOT);

	    if (!key_pass && this_plot->plot_type != KEYENTRY)
	    if (can_pm3d && PM3D_IMPLICIT == pm3d.implicit)
		pm3d_draw_one(this_plot);

	    lkey = (key->visible && this_plot->title && this_plot->title[0]
				 && !this_plot->title_is_suppressed);
	    draw_this_surface = (draw_surface && !this_plot->opt_out_of_surface);
	    if (this_plot->plot_type == KEYENTRY)
		draw_this_surface = TRUE;

	    /* User-specified key locations can use the 2D code */
	    if (this_plot->title_position) {
		xl_prev = xl;
		yl_prev = yl;
		if (this_plot->title_position->scalex != character) {
		    map3d_position(this_plot->title_position, &xl, &yl, "key sample");
		    xl -=  (key->just == GPKEY_LEFT) ? key_text_left : key_text_right;
		} else {
		    /* Option to label the end of the curve on the plot itself */
		    attach_title_to_plot((struct curve_points *)this_plot, key);
		}
	    }

	    if (lkey
	    &&  (!this_plot->title_position || this_plot->title_position->scalex != character)) {
		char *title = this_plot->title;

		if (this_plot->title_is_automated && (term->flags & TERM_IS_LATEX))
		    title = texify_title(title, this_plot->plot_type);

		if (key->textcolor.type != TC_DEFAULT)
		    /* Draw key text in same color as key title */
		    apply_pm3dcolor(&key->textcolor);
		else
		    /* Draw key text in black */
		    (*t->linetype)(LT_BLACK);
		ignore_enhanced(this_plot->title_no_enhanced);
		key_text(xl, yl, title);
		ignore_enhanced(FALSE);
	    }
	    term_apply_lp_properties(&(this_plot->lp_properties));

	    /* Voxel data is a special case.
	     * what about hidden3d mode? pm3d?
	     */
	    if (!key_pass && this_plot->plot_type == VOXELDATA) {
		switch (this_plot->plot_style) {
		default:
		    /* style should default to DOTS */
		    this_plot->plot_style = DOTS;
		case DOTS:
		case POINTSTYLE:
		    vplot_points(this_plot, this_plot->iso_level);
		    break;
		case ISOSURFACE:
		    if (replot_mode == QUICK_REFRESH)
			vplot_isosurface(this_plot, 4);
		    else
			vplot_isosurface(this_plot, 1);
		    break;
		}
	    }

	    /* First draw the graph plot itself */
	    if (!key_pass && this_plot->plot_type != KEYENTRY && this_plot->plot_type != VOXELDATA)
	    switch (this_plot->plot_style) {
	    case FILLEDCURVES:	/* same, but maybe we could dummy up ZERRORFILL? */
	    case IMPULSES:
		if (!hidden3d)
		    plot3d_impulses(this_plot);
		break;
	    case STEPS:	/* HBB: I think these should be here */
	    case FILLSTEPS:
	    case FSTEPS:
	    case HISTEPS:
	    case SURFACEGRID:
	    case LINES:
		if (draw_this_surface) {
		    if (!hidden3d || this_plot->opt_out_of_hidden3d)
			plot3d_lines_pm3d(this_plot);
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
	    case ARROWS:
	    case CIRCLES:
	    case ELLIPSES:
	    case POINTSTYLE:
	    case DOTS:
		if (draw_this_surface) {
		    if (!hidden3d || this_plot->opt_out_of_hidden3d)
			plot3d_points(this_plot);
		}
		break;

	    case LINESPOINTS:
		if (draw_this_surface) {
		    if (!hidden3d || this_plot->opt_out_of_hidden3d) {
			plot3d_lines_pm3d(this_plot);
			plot3d_points(this_plot);
		    }
		}
		break;

	    case VECTOR:
		if (!hidden3d || this_plot->opt_out_of_hidden3d)
		    plot3d_vectors(this_plot);
		break;

	    case ZERRORFILL:
		/* Always draw filled areas even if we _also_ do hidden3d processing */
		if (term->filled_polygon)
		    plot3d_zerrorfill(this_plot);
		term_apply_lp_properties(&(this_plot->lp_properties));
		plot3d_lines(this_plot);
		break;

	    case BOXES:
		if (term->filled_polygon)
		    plot3d_boxes(this_plot);
		else
		    plot3d_impulses(this_plot);
		break;

	    case PM3DSURFACE:
		if (draw_this_surface) {
		    if (can_pm3d && PM3D_IMPLICIT != pm3d.implicit) {
			pm3d_draw_one(this_plot);
			if (!pm3d_order_depth)
			    pm3d_depth_queue_flush(); /* draw plot immediately */
		    }
		}
		break;

	    case POLYGONS:
		if (term->filled_polygon)
		    plot3d_polygons(this_plot);
		else
		    plot3d_lines(this_plot);
		break;

	    case LABELPOINTS:
		if (draw_this_surface) {
		    if (hidden3d && !(this_plot->opt_out_of_hidden3d))
			break;
		    if (draw_contour && !(this_plot->opt_out_of_contours))
			break;
		    if (pm3d_order_depth || track_pm3d_quadrangles)
			break;
		    place_labels3d(this_plot->labels->next, LAYER_PLOTLABELS);
		}
		break;

	    case IMAGE:
		/* Plot image using projection of 3D plot coordinates to 2D viewing coordinates. */
		this_plot->image_properties.type = IC_PALETTE;
		process_image(this_plot, IMG_PLOT);
		break;

	    case RGBIMAGE:
		/* Plot image using projection of 3D plot coordinates to 2D viewing coordinates. */
		this_plot->image_properties.type = IC_RGB;
		process_image(this_plot, IMG_PLOT);
		break;

	    case RGBA_IMAGE:
		this_plot->image_properties.type = IC_RGBA;
		process_image(this_plot, IMG_PLOT);
		break;

	    case PARALLELPLOT:
	    case SPIDERPLOT:
	    case HISTOGRAMS:
	    case CANDLESTICKS:
	    case BOXPLOT:
	    case FINANCEBARS:
		/* These should have been caught in plot3d */
		int_error(NO_CARET, "plot style not supported in 3D");
		break;

	    case ISOSURFACE:
	    case PLOT_STYLE_NONE:
	    case TABLESTYLE:
		/* cannot happen */
		break;
	    }			/* switch(plot-style) plot proper */

	    /* Next draw the key sample */
	    if (lkey
	    &&  (!this_plot->title_position || this_plot->title_position->scalex != character))
	    switch (this_plot->plot_style) {
	    case FILLEDCURVES:
	    case IMPULSES:
		if (!(hidden3d && draw_this_surface))
		    key_sample_line(xl, yl);
		break;
	    case STEPS:	/* HBB: I think these should be here */
	    case FILLSTEPS:
	    case FSTEPS:
	    case HISTEPS:
	    case SURFACEGRID:
	    case LINES:
		/* Normal case (surface) */
		if (draw_this_surface)
		    key_sample_line_pm3d(this_plot, xl, yl);
		/* Contour plot with no surface, all contours use the same linetype */
		else if (this_plot->contours != NULL && clabel_onecolor) {
		    key_sample_line(xl, yl);
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
	    case ELLIPSES:
	    case POINTSTYLE:
		if (this_plot->plot_type == VOXELDATA) {
		    if (this_plot->lp_properties.pm3d_color.type == TC_Z)
			set_color(0.5);
		    key_sample_point(this_plot, xl, yl, this_plot->lp_properties.p_type);
		} else

		if (draw_this_surface)
		    key_sample_point_pm3d(this_plot, xl, yl, this_plot->lp_properties.p_type);
		break;

	    case LABELPOINTS:
		if ((this_plot->labels->lp_properties.flags & LP_SHOW_POINTS)) {
		    term_apply_lp_properties(&this_plot->labels->lp_properties);
		    key_sample_point(this_plot, xl, yl, this_plot->labels->lp_properties.p_type);
		}
		break;

	    case LINESPOINTS:
		if (draw_this_surface) {
		    if (this_plot->lp_properties.l_type != LT_NODRAW)
			key_sample_line_pm3d(this_plot, xl, yl);
		    key_sample_point_pm3d(this_plot, xl, yl, this_plot->lp_properties.p_type);
		}
		break;

	    case DOTS:
		if (draw_this_surface)
		    key_sample_point_pm3d(this_plot, xl, yl, -1);
		break;

	    case VECTOR:
		key_sample_line_pm3d(this_plot, xl, yl);
		break;

	    case ZERRORFILL:
		apply_pm3dcolor(&this_plot->fill_properties.border_color);
		key_sample_fill(xl, yl, this_plot);
		term_apply_lp_properties(&this_plot->lp_properties);
		key_sample_line(xl, yl);
		break;

	    case BOXES:
	    case CIRCLES:
		apply_pm3dcolor(&this_plot->lp_properties.pm3d_color);
		if (this_plot->iso_crvs)
		    check3d_for_variable_color(this_plot, this_plot->iso_crvs->points);
		key_sample_fill(xl, yl, this_plot);
		break;

	    case ISOSURFACE:
		apply_pm3dcolor(&this_plot->fill_properties.border_color);
		key_sample_fill(xl, yl, this_plot);
		break;

	    case PLOT_STYLE_NONE:
		/* cannot happen */
	    default:
		break;

	    }			/* switch(plot-style) key sample */

	    /* If the title went somewhere other than the key,
	     * restore the previous key position.
	     * Else move down one line in the key.
	     */
	    if (this_plot->title_position) {
		xl = xl_prev;
		yl = yl_prev;
	    } else if (lkey)
		NEXT_KEY_LINE();

	    /* Draw contours for previous surface */
	    if (draw_contour && this_plot->contours != NULL) {
		struct gnuplot_contours *cntrs = this_plot->contours;
		struct lp_style_type thiscontour_lp_properties;
		static char *thiscontour_label = NULL;
		TBOOLEAN save_hidden3d;
		int ic = 1;	/* ic will index the contour linetypes */

		thiscontour_lp_properties = this_plot->lp_properties;
		term_apply_lp_properties(&(thiscontour_lp_properties));

		while (cntrs) {
		    if (!clabel_onecolor && cntrs->isNewLevel) {
			if (key->visible && !this_plot->title_is_suppressed
			&&  this_plot->plot_style != LABELPOINTS) {
			    (*t->linetype)(LT_BLACK);
			    key_text(xl, yl, cntrs->label);
			}
			if (thiscontour_lp_properties.pm3d_color.type == TC_Z)
			    set_color( cb2gray(cntrs->z) );
			else {
			    struct lp_style_type ls = thiscontour_lp_properties;
			    int contour_linetype;
			    ic++;	/* Increment linetype used for contour */

			    /* First contour line type defaults to surface linetype + 1  */
			    /* but can be changed using 'set cntrparams firstlinetype N' */
			    if (contour_firstlinetype > 0)
				contour_linetype = contour_firstlinetype + ic - 2;
			    else
				contour_linetype = this_plot->hidden3d_top_linetype + ic;

			    /* hidden3d processing looks directly at l_type */
			    /* for other purposes the line color is set by load_linetype */
			    if (hidden3d)
				thiscontour_lp_properties.l_type = contour_linetype - 1;
			    load_linetype(&ls, contour_linetype);

			    thiscontour_lp_properties.pm3d_color = ls.pm3d_color;
			    thiscontour_lp_properties.l_width = ls.l_width
							      * this_plot->lp_properties.l_width;
			    thiscontour_lp_properties.d_type = ls.d_type;
			    thiscontour_lp_properties.custom_dash_pattern = ls.custom_dash_pattern;
			    term_apply_lp_properties(&thiscontour_lp_properties);
			}

			if (key->visible && !this_plot->title_is_suppressed
			&& !(this_plot->plot_style == LABELPOINTS)) {

			    switch (this_plot->plot_style) {
			    case IMPULSES:
			    case LINES:
			    case LINESPOINTS:
			    case FILLEDCURVES:
			    case VECTOR:
			    case STEPS:
			    case FSTEPS:
			    case HISTEPS:
			    case PM3DSURFACE:
				key_sample_line(xl, yl);
				break;
			    case POINTSTYLE:
				key_sample_point(this_plot, xl, yl, this_plot->lp_properties.p_type);
				break;
			    case DOTS:
				key_sample_point(this_plot, xl, yl, -1);
				break;
			    default:
				break;
			    }	/* switch */

			    NEXT_KEY_LINE();

			} /* key */
		    } /* clabel_onecolor */

		    /* now draw the contour */
		    if (!key_pass)
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
			save_hidden3d = hidden3d;
			if (this_plot->opt_out_of_hidden3d)
			    hidden3d = FALSE;
			cntr3d_lines(cntrs, &thiscontour_lp_properties);
			hidden3d = save_hidden3d;
			break;

		    case LINESPOINTS:
			cntr3d_lines(cntrs, &thiscontour_lp_properties);
			/* Fall through to draw the points */
		    case DOTS:
		    case POINTSTYLE:
			cntr3d_points(cntrs, &thiscontour_lp_properties);
			break;

		    case LABELPOINTS:
			if (cntrs->isNewLevel) {
			    char *c = &cntrs->label[strspn(cntrs->label," ")];
			    free(thiscontour_label);
			    thiscontour_label = gp_strdup(c);
			}
			cntr3d_labels(cntrs, thiscontour_label, this_plot->labels);
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

    if (!key_pass)
    if (pm3d_order_depth || track_pm3d_quadrangles) {
	pm3d_depth_queue_flush(); /* draw pending plots */
    }

    if (!key_pass)
    if (hidden3d && (hidden3d_layer == LAYER_FRONT) && draw_surface
    &&  (replot_mode != AXIS_ONLY_ROTATE)) {
	(term->layer)(TERM_LAYER_BEFORE_PLOT);
	plot3d_hidden(plots, pcount);
	(term->layer)(TERM_LAYER_AFTER_PLOT);
    }

    /* Add labels that were defered until after depth-sorted pm3d surfaces */
    if (!key_pass)
    if ((pm3d_order_depth || track_pm3d_quadrangles) && !hidden3d) {
	this_plot = plots;
	for (surface = 0; surface < pcount; this_plot = this_plot->next_sp, surface++) {
	    if (this_plot->plot_style == LABELPOINTS)
		if (!draw_contour || this_plot->opt_out_of_contours)
		    place_labels3d(this_plot->labels->next, LAYER_PLOTLABELS);
	}
    }

    /* Draw grid and border.
     * The 1st case allows "set border behind" to override hidden3d processing.
     * The 2nd case either leaves everything to hidden3d or forces it to the front.
     * The 3rd case is the non-hidden3d default - draw back pieces (done earlier),
     * then the graph, and now the front pieces.
     */
    if (hidden3d && border_layer == LAYER_BEHIND)
	/* the important thing is _not_ to draw the back grid */
	/* draw_3d_graphbox(plots, pcount, FRONTGRID, LAYER_FRONT) */
	;

    else if (hidden3d || grid_layer == LAYER_FRONT)
	draw_3d_graphbox(plots, pcount, ALLGRID, LAYER_FRONT);

    else if (grid_layer == LAYER_BEHIND)
	draw_3d_graphbox(plots, pcount, FRONTGRID, LAYER_FRONT);

    /* Go back and draw the legend in a separate pass if "key opaque" */
    if (key->visible && key->front && !key_pass) {
	key_pass = TRUE;
	xl = xl_save; yl = yl_save;
	goto SECOND_KEY_PASS;
    }

    /* Add 'front' color box */
    if ((replot_mode != AXIS_ONLY_ROTATE)
    &&  can_pm3d && is_plot_with_colorbox() && color_box.layer == LAYER_FRONT)
	draw_color_smooth_box(MODE_SPLOT);

    /* Add 'front' rectangles */
    place_pixmaps(LAYER_FRONT, 3);
    place_objects(first_object, LAYER_FRONT, 3);

    /* Grid walls */
    place_objects(grid_wall, LAYER_FRONT, 3);

    /* Add 'front' labels */
    place_labels3d(first_label, LAYER_FRONT);

    /* PLACE ARROWS */
    place_arrows3d(LAYER_FRONT);

    /* PLACE TITLE LAST */
    if (title.text != 0)
	place_title(title_x, title_y);

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

    if (splot_map)
	splot_map_deactivate();
    else if (xz_projection || yz_projection)
	surface_scale = 1.0;
    else if (yz_projection)
	flip_projection_axis(&axis_array[FIRST_Z_AXIS]);
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

    if (colortype == TC_RGB)
	set_rgbcolor_const(plot->lp_properties.pm3d_color.lt);

    while (icrvs) {
	struct coordinate *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {

	    check3d_for_variable_color(plot, &points[i]);

	    switch (points[i].type) {
	    case INRANGE:
		{
		    double z = 0.0;
		    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);
		    cliptorange(z, Z_AXIS.min, Z_AXIS.max);
		    map3d_xy(points[i].x, points[i].y, z, &xx0, &yy0);

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
    struct coordinate *points;
    TBOOLEAN rgb_from_column;

    /* These are handled elsewhere.  */
    if (plot->has_grid_topology && hidden3d)
	return;

    /* These don't need to be drawn at all */
    if (plot->lp_properties.l_type == LT_NODRAW)
	return;

    rgb_from_column = plot->pm3d_color_from_column
			&& plot->lp_properties.pm3d_color.type == TC_RGB
			&& plot->lp_properties.pm3d_color.value < 0.0;

    while (icrvs) {
	enum coord_type prev = UNDEFINED;	/* type of previous plot */

	for (i = 0, points = icrvs->points; i < icrvs->p_count; i++) {

	    if (rgb_from_column)
		set_rgbcolor_var((unsigned int)points[i].CRD_COLOR);
	    else if (plot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
		plot->lp_properties.pm3d_color.lt = (int)(points[i].CRD_COLOR);
		apply_pm3dcolor(&(plot->lp_properties.pm3d_color));
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
				edge3d_intersect(&points[i-1], &points[i], &clip_x, &clip_y, &clip_z);

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
			    edge3d_intersect(&points[i-1], &points[i], &clip_x, &clip_y, &clip_z);

			    map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

			    clip_vector(xx0, yy0);
			}
		    } else if (prev == OUTRANGE) {
			/* from outrange to outrange */
			if (clip_lines2) {
			    double lx[2], ly[2], lz[2];	/* two edge points */
			    /*
			     * Calculate the two 3D intersection points if present
			     */
			    if (two_edge3d_intersect(&points[i-1], &points[i], lx, ly, lz)) {
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
		    int_warn(NO_CARET,"Unknown point type in plot3d_lines");
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
    int invert[2] = {0,0};
    int n[2] = {0,0};

    int i, set, scan;
    int x, y, xx0, yy0;	/* point in terminal coordinates */
    double clip_x, clip_y, clip_z;
    struct coordinate *points;
    enum coord_type prev = UNDEFINED;
    double z;

    /* just a shortcut */
    TBOOLEAN color_from_column = plot->pm3d_color_from_column;

    /* If plot really uses RGB rather than pm3d colors, let plot3d_lines take over */
    if (plot->lp_properties.pm3d_color.type == TC_RGB) {
	apply_pm3dcolor(&(plot->lp_properties.pm3d_color));
	plot3d_lines(plot);
	return;
    } else if (plot->lp_properties.pm3d_color.type == TC_LT) {
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
	icrvs_pair, &n[0], &invert[0],
	icrvs_pair + 1, &n[1], &invert[1]);

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
				z =  (points[i - step].z + points[i].z) * 0.5;
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
				    edge3d_intersect(&points[i-step], &points[i], &clip_x, &clip_y, &clip_z);

				    map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

				    clip_move(xx0, yy0);
				    if (color_from_column)
					z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
				    else
					z =  (points[i - step].z + points[i].z) * 0.5;
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

				edge3d_intersect(&points[i-step], &points[i], &clip_x, &clip_y, &clip_z);

				map3d_xy(clip_x, clip_y, clip_z, &xx0, &yy0);

				if (color_from_column)
				    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
				else
				    z =  (points[i - step].z + points[i].z) * 0.5;
				set_color( cb2gray(z));
				clip_vector(xx0, yy0);
			    }
			} else if (prev == OUTRANGE) {
			    /* from outrange to outrange */
			    if (clip_lines2) {
				/*
				 * Calculate the two 3D intersection points if present
				 */
				double lx[2], ly[2], lz[2];
				if (two_edge3d_intersect(&points[i-step], &points[i], lx, ly, lz)) {
				    map3d_xy(lx[0], ly[0], lz[0], &x, &y);
				    map3d_xy(lx[1], ly[1], lz[1], &xx0, &yy0);

				    clip_move(x, y);
				    if (color_from_column)
					z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
				    else
					z =  (points[i - step].z + points[i].z) * 0.5;
				    set_color( cb2gray(z) );
				    clip_vector(xx0, yy0);
				}
			    }
			}
			break;
		    case UNDEFINED:
			break;
		    default:
			int_warn(NO_CARET,"Unknown point type in plot3d_lines");
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
plot3d_points(struct surface_points *plot)
{
    int i;
    int x, y;
    struct termentry *t = term;
    struct iso_curve *icrvs = plot->iso_crvs;
    int interval = plot->lp_properties.p_interval;

    /* Set whatever we can that applies to every point in the loop */
    if (plot->lp_properties.p_type == PT_CHARACTER) {
	ignore_enhanced(TRUE);
	if (plot->labels->font && plot->labels->font[0])
	    (*t->set_font) (plot->labels->font);
	(*t->justify_text) (CENTRE);
    }

    while (icrvs) {
	struct coordinate *point;
	int colortype = plot->lp_properties.pm3d_color.type;
	const char *ptchar;

	/* Apply constant color outside of the loop */
	if (plot->plot_style == CIRCLES)
	    set_rgbcolor_const( plot->fill_properties.border_color.lt );
	else if (colortype == TC_RGB)
	    set_rgbcolor_const( plot->lp_properties.pm3d_color.lt );

	for (i = 0; i < icrvs->p_count; i++) {
	
	    /* Only print 1 point per interval */
	    if ((plot->plot_style == LINESPOINTS) && (interval) && (i % interval))
		continue;

	    point = &(icrvs->points[i]);
	    if (point->type == INRANGE) {
		map3d_xy(point->x, point->y, point->z, &x, &y);

		if (!clip_point(x, y)) {

		    /* A negative interval indicates we should blank */
		    /* out the area behind the point symbol          */
		    if (plot->plot_style == LINESPOINTS && interval < 0) {
			(*t->set_color)(&background_fill);
			(*t->pointsize)(pointsize * pointintervalbox);
			(*t->point) (x, y, 6);
			term_apply_lp_properties(&(plot->lp_properties));
		    }

		    check3d_for_variable_color(plot, point);

		    if ((plot->plot_style == POINTSTYLE || plot->plot_style == LINESPOINTS)
		    &&  plot->lp_properties.p_size == PTSZ_VARIABLE)
			(*t->pointsize)(pointsize * point->CRD_PTSIZE);

		    /* We could dummy up circles as a point of type 7, but this way */
		    /* the radius can use x-axis coordinates rather than pointsize. */
		    /* FIXME: track per-plot fillstyle */
		    if (plot->plot_style == CIRCLES) {
			double radius = point->CRD_PTSIZE * radius_scaler;
			do_arc(x, y, radius, 0., 360.,
				style_from_fill(&default_fillstyle), FALSE);
			/* Retrace the border if the style requests it */
			if (need_fill_border(&default_fillstyle))
			    do_arc(x, y, radius, 0., 360., 0, FALSE);
			continue;
		    }

		    /* This code is also used for "splot ... with dots" */
		    if (plot->plot_style == DOTS) {
			(*t->point) (x, y, -1);
			continue;
		    }

		    /* variable point type */
		    if ((plot->lp_properties.p_type == PT_VARIABLE)
			 &&  !(isnan(point->CRD_PTTYPE))) {
			(*t->point) (x, y, (int)(point->CRD_PTTYPE) - 1);
		    }

		    /* Print special character rather than drawn symbol */
		    if (plot->lp_properties.p_type == PT_CHARACTER)
			ptchar = plot->lp_properties.p_char;
		    else if (plot->lp_properties.p_type == PT_VARIABLE
			 &&  isnan(point->CRD_PTTYPE))
			ptchar = (char *)(&point->CRD_PTCHAR);
		    else
			ptchar = NULL;
		    if (ptchar) {
			if (plot->labels)
			    apply_pm3dcolor(&(plot->labels->textcolor));
			(*t->put_text)(x, y, ptchar);
		    }

		    /* The normal case */
		    else if (plot->lp_properties.p_type >= -1)
			(*t->point) (x, y, plot->lp_properties.p_type);

		}
	    }
	}

	icrvs = icrvs->next;
    }

    /* Return to initial state */
    if (plot->lp_properties.p_type == PT_CHARACTER) {
	if (plot->labels->font && plot->labels->font[0])
	    (*t->set_font) ("");
	ignore_enhanced(FALSE);
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

/* cntr3d_lines:
 * Plot a surface contour in LINES style
 */
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

/* cntr3d_labels:
 * Place contour labels on a contour line at the base.
 * These are the same labels that would be used in the key.
 * The label density is controlled by the point interval property
 *     splot FOO with labels point pi 20 nosurface
 */
static void
cntr3d_labels(struct gnuplot_contours *cntr, char *level_text, struct text_label *label)
{
    int i;
    int interval;
    int x, y;
    vertex v;
    struct lp_style_type *lp = &(label->lp_properties);

    /* Drawing a label at every point would be too crowded */
    interval = lp->p_interval;
    if (interval <= 0) interval = 999;	/* Place label only at start point */

    if (draw_contour & CONTOUR_BASE) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if ((i-clabel_start) % interval)	/* Offset to avoid sitting on the border */
		continue;
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, base_z, &x, &y);
	    label->text = level_text;
	    label->font = clabel_font;
	    if (hidden3d) {
		map3d_xyz(cntr->coords[i].x, cntr->coords[i].y, base_z, &v);
		v.real_z = cntr->coords[i].z;
		v.label = label;
		draw_label_hidden(&v, lp, x, y);
	    } else {
		write_label(x, y, label);
	    }
	    label->text = NULL;		/* Otherwise someone will try to free it */
	    label->font = NULL;
	}
    }
}

/* map xmin | xmax to 0 | 1 and same for y
 * 0.1 avoids any rounding errors
 */
#define MAP_HEIGHT_X(x) ( ((x)-X_AXIS.min)/(X_AXIS.max-X_AXIS.min) > 0.9 ? 1 : 0 )
#define MAP_HEIGHT_Y(y) ( ((y)-Y_AXIS.min)/(Y_AXIS.max-Y_AXIS.min) > 0.9 ? 1 : 0 )

/* if point is at corner, update height[][] and depth[][]
 * we are still assuming that extremes of surfaces are at corners,
 * but we are not assuming order of corners
 */
static void
check_corner_height(
    struct coordinate *p,
    double height[2][2], double depth[2][2])
{
    if (p->type != INRANGE)
	return;
    /* FIXME HBB 20010121: don't compare 'zero' to data values in
     * absolute terms. */
    if ((fabs(p->x - X_AXIS.min) < zero || fabs(p->x - X_AXIS.max) < zero) &&
	(fabs(p->y - Y_AXIS.min) < zero || fabs(p->y - Y_AXIS.max) < zero)) {
	int x = MAP_HEIGHT_X(p->x);
	int y = MAP_HEIGHT_Y(p->y);
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

    quadrant = surface_rot_x / 90;
    if ((quadrant & 2) && !splot_map) {
	double temp;
	temp = front_y;
	front_y = back_y;
	back_y = temp;
	temp = front_x;
	front_x = back_x;
	back_x = temp;
    }

    if ((quadrant + 1) & 2) {
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

    FPRINTF((stderr,
	"draw_3d_graphbox: whichgrid = %d current_layer = %d border_layer = %d\n",
	whichgrid,current_layer,border_layer));

    clip_area = &canvas;
    if (draw_border && splot_map) {
	if (border_layer == current_layer) {
	    term_apply_lp_properties(&border_lp);
	    if ((draw_border & 15) == 15)
		newpath();
	    map3d_xy(zaxis_x, zaxis_y, base_z, &x, &y);
	    clip_move(x, y);
	    map3d_xy(back_x , back_y , base_z, &x, &y);
	    if (draw_border & 2)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(right_x, right_y, base_z, &x, &y);
	    if (draw_border & 8)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(front_x, front_y, base_z, &x, &y);
	    if (draw_border & 4)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(zaxis_x, zaxis_y, base_z, &x, &y);
	    if (draw_border & 1)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    if ((draw_border & 15) == 15)
		closepath();
	}

    } else if (draw_border && yz_projection) {
	if (border_layer == current_layer) {
	    struct axis *yaxis = &axis_array[FIRST_Y_AXIS];
	    struct axis *zaxis = &axis_array[FIRST_Z_AXIS];
	    term_apply_lp_properties(&border_lp);
	    if ((draw_border & 15) == 15)
		newpath();
	    map3d_xy(0.0, yaxis->min, zaxis->min, &x, &y);
	    clip_move(x, y);
	    map3d_xy(0.0, yaxis->max, zaxis->min, &x, &y);
	    if (draw_border & 8)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(0.0, yaxis->max, zaxis->max, &x, &y);
	    if (draw_border & 4)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(0.0, yaxis->min, zaxis->max, &x, &y);
	    if (draw_border & 2)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(0.0, yaxis->min, zaxis->min, &x, &y);
	    if (draw_border & 1)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    if ((draw_border & 15) == 15)
		closepath();
	}

    } else if (draw_border && xz_projection) {
	if (border_layer == current_layer) {
	    struct axis *xaxis = &axis_array[FIRST_X_AXIS];
	    struct axis *zaxis = &axis_array[FIRST_Z_AXIS];
	    term_apply_lp_properties(&border_lp);
	    if ((draw_border & 15) == 15)
		newpath();
	    map3d_xy(xaxis->min, 0.0, zaxis->min, &x, &y);
	    clip_move(x, y);
	    map3d_xy(xaxis->max, 0.0, zaxis->min, &x, &y);
	    if (draw_border & 2)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(xaxis->max, 0.0, zaxis->max, &x, &y);
	    if (draw_border & 4)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(xaxis->min, 0.0, zaxis->max, &x, &y);
	    if (draw_border & 8)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    map3d_xy(xaxis->min, 0.0, zaxis->min, &x, &y);
	    if (draw_border & 1)
		clip_vector(x, y);
	    else
		clip_move(x, y);
	    if ((draw_border & 15) == 15)
		closepath();
	}

    } else if (draw_border) {
	/* the four corners of the base plane, in normalized view
	 * coordinates (-1..1) on all three axes. */
	vertex bl, bb, br, bf;

	/* map to normalized view coordinates the corners of the
	 * baseplane: left, back, right and front, in that order: */
	map3d_xyz(zaxis_x, zaxis_y, base_z, &bl);
	map3d_xyz(back_x , back_y , base_z, &bb);
	map3d_xyz(right_x, right_y, base_z, &br);
	map3d_xyz(front_x, front_y, base_z, &bf);

	if (BACKGRID != whichgrid) {
	    /* Draw front part of base grid, right to front corner: */
	    if (draw_border & 4)
		draw3d_line(&br, &bf, &border_lp);
	    /* ... and left to front: */
	    if (draw_border & 1)
		draw3d_line(&bl, &bf, &border_lp);
	}
	if (FRONTGRID != whichgrid) {
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
		if (FRONTGRID != whichgrid) {
		    /* Draw the back verticals floor-to-ceiling, left: */
		    draw3d_line(&fl, &tl, &border_lp);
		    /* ... back: */
		    draw3d_line(&fb, &tb, &border_lp);
		    /* ... and right */
		    draw3d_line(&fr, &tr, &border_lp);
		}
		if (BACKGRID != whichgrid) {
		    /* Draw the front vertical: floor-to-ceiling, front: */
		    draw3d_line(&ff, &tf, &border_lp);
		}
	    } else {
		/* find heights of surfaces at the corners of the xy
		 * rectangle */
		double height[2][2];
		double depth[2][2];
		int zaxis_i = MAP_HEIGHT_X(zaxis_x);
		int zaxis_j = MAP_HEIGHT_Y(zaxis_y);
		int back_i = MAP_HEIGHT_X(back_x);
		int back_j = MAP_HEIGHT_Y(back_y);

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

		    if (plot->plot_type == NODATA || plot->plot_type == KEYENTRY)
			continue;
		    if (plot->plot_type == VOXELDATA)
			continue;
		    if (plot->plot_type == DATA3D) {
			if (!plot->has_grid_topology)
			    continue;
			iso = plot->num_iso_read;
		    } else
			iso = iso_samples_2;

		    count = curve->p_count;
		    if (count == 0)
			continue;

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
		} else if (height[i][j] != depth[i][j] &&	\
			   (X_AXIS.ticmode || Y_AXIS.ticmode ||	\
			    draw_border & 0x00F)) {		\
		    vertex a, b;				\
		    map3d_xyz(x,y,depth[i][j],&a);		\
		    map3d_xyz(x,y,height[i][j],&b);		\
		    draw3d_line(&a, &b, &border_lp);		\
		}

		if (FRONTGRID != whichgrid) {
		    /* Draw back verticals: floor-to-ceiling left: */
		    VERTICAL(0x10, zaxis_x, zaxis_y, zaxis_i, zaxis_j, &fl, &tl);
		    /* ... back: */
		    VERTICAL(0x20, back_x, back_y, back_i, back_j, &fb, &tb);
		    /* ... and right: */
		    VERTICAL(0x40, right_x, right_y, 1 - zaxis_i, 1 - zaxis_j,
			     &fr, &tr);
		}
		if (BACKGRID != whichgrid) {
		    /* Draw front verticals: floor-to-ceiling front */
		    VERTICAL(0x80, front_x, front_y, 1 - back_i, 1 - back_j,
			     &ff, &tf);
		}
#undef VERTICAL
	    } /* else (all 4 verticals drawn?) */

	    /* now border lines on top */
	    if (FRONTGRID != whichgrid) {
		/* Draw back part of top of box: top left to back corner: */
		if (draw_border & 0x100)
		    draw3d_line(&tl, &tb, &border_lp);
		/* ... and top right to back: */
		if (draw_border & 0x200)
		    draw3d_line(&tr, &tb, &border_lp);
	    }
	    if (BACKGRID != whichgrid) {
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
    if (splot_map && current_layer != abs(grid_layer)) {
	clip_area = clip_save;
	return;
    }
    if (whichgrid == BORDERONLY) {
	clip_area = clip_save;
	return;
    }

    /* Draw ticlabels and axis labels */

    /* x axis */
    if ((X_AXIS.ticmode || X_AXIS.label.text) && !yz_plane) {
	vertex v0, v1;
	double other_end = Y_AXIS.min + Y_AXIS.max - xaxis_y;
	double mid_x;

	if (nonlinear(&X_AXIS)) {
	    AXIS *primary = X_AXIS.linked_to_primary;
	    mid_x = (primary->max + primary->min) / 2.;
	    mid_x = eval_link_function(&X_AXIS, mid_x);
	} else {
	    mid_x = (X_AXIS.max + X_AXIS.min) / 2.;
	}

	map3d_xyz(mid_x, xaxis_y, base_z, &v0);
	map3d_xyz(mid_x, other_end, base_z, &v1);

	/* Unusual case: 2D projection of the xz plane */
	if (!splot_map && xz_plane)
	    map3d_xyz(mid_x, xaxis_y, Z_AXIS.max+Z_AXIS.min-base_z, &v1);

	tic_unitx = (v1.x - v0.x) / xyscaler;
	tic_unity = (v1.y - v0.y) / xyscaler;
	tic_unitz = (v1.z - v0.z) / xyscaler;

	/* Don't output tics and grids if this is the front part of a
	 * two-part grid drawing process: */
	if ((surface_rot_x <= 90 && FRONTGRID != whichgrid) ||
	    (surface_rot_x > 90 && BACKGRID != whichgrid))

	if (X_AXIS.ticmode)
	    gen_tics(&axis_array[FIRST_X_AXIS], xtick_callback);

	if (X_AXIS.label.text) {

	    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
		(surface_rot_x > 90 && FRONTGRID != whichgrid) ||
		splot_map) {

	    int x1, y1;

	    if (splot_map) { /* case 'set view map' */
		/* copied from xtick_callback(): baseline of tics labels */
		vertex v1, v2;
		map3d_xyz(mid_x, xaxis_y, base_z, &v1);
		v2.x = v1.x;
		v2.y = v1.y - tic_unity * t->v_char;
		if (!X_AXIS.tic_in)
		    v2.y -= tic_unity * t->v_tic * X_AXIS.ticscale;
		TERMCOORD(&v2, x1, y1);
		/* Default displacement with respect to baseline of tics labels */
		y1 -= (1.5 * t->v_char);
	    } else { /* usual 3d set view ... */
		if (X_AXIS.label.tag == ROTATE_IN_3D_LABEL_TAG) {
		    double ang, angx0, angx1, angy0, angy1;
		    map3d_xy_double(X_AXIS.min, xaxis_y, base_z, &angx0, &angy0);
		    map3d_xy_double(X_AXIS.max, xaxis_y, base_z, &angx1, &angy1);
		    ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
		    if (ang < -90) ang += 180;
		    if (ang > 90) ang -= 180;
		    X_AXIS.label.rotate = (ang > 0) ? floor(ang + 0.5) : floor(ang - 0.5);
		}

		if (X_AXIS.ticmode & TICS_ON_AXIS) {
		    map3d_xyz(mid_x, 0.0, base_z, &v1);
		} else {
		    map3d_xyz(mid_x, xaxis_y, base_z, &v1);
		}

		if (xz_projection) {
		    v1.x -= 3. * t->h_tic * tic_unitx;
		    v1.y -= 3. * t->h_tic * tic_unity;
		} else if (X_AXIS.ticmode & TICS_ON_AXIS) {
		    v1.x += 2. * t->h_tic * ((X_AXIS.tic_in) ? 1.0 : -1.0) * tic_unitx;
		    v1.y += 2. * t->h_tic * ((X_AXIS.tic_in) ? 1.0 : -1.0) * tic_unity;
		} else {
		    v1.x -= 10. * t->h_tic * tic_unitx;
		    v1.y -= 10. * t->h_tic * tic_unity;
		}

		if (!X_AXIS.tic_in) {
		    v1.x -= tic_unitx * X_AXIS.ticscale * t->h_tic;
		    v1.y -= tic_unity * X_AXIS.ticscale * t->h_tic;
		}
		TERMCOORD(&v1, x1, y1);
	    }

	    write_label(x1, y1, &X_AXIS.label);
	    }
	}

	/* In "set view map" 2D projection there may also be x2 tics and label */
	if (splot_map && axis_array[SECOND_X_AXIS].ticmode)
	    gen_tics(&axis_array[SECOND_X_AXIS], xtick_callback);
	if (splot_map && axis_array[SECOND_X_AXIS].label.text) {
	    int x2 = (plot_bounds.xright + plot_bounds.xleft) / 2;
	    int y2 = plot_bounds.ytop + 1.0 * t->v_char;
	    if (axis_array[SECOND_X_AXIS].ticmode)
		y2 += 2.5 * t->h_char;
	    write_label(x2, y2, &(axis_array[SECOND_X_AXIS].label));
	}
    }

    /* y axis */
    if ((Y_AXIS.ticmode || Y_AXIS.label.text) && !xz_plane) {
	vertex v0, v1;
	double other_end = X_AXIS.min + X_AXIS.max - yaxis_x;
	double mid_y;

	if (nonlinear(&Y_AXIS)) {
	    AXIS *primary = Y_AXIS.linked_to_primary;
	    mid_y = (primary->max + primary->min) / 2.;
	    mid_y = eval_link_function(&Y_AXIS, mid_y);
	} else {
	    mid_y = (Y_AXIS.max + Y_AXIS.min) / 2.;
	}

	map3d_xyz(yaxis_x, mid_y, base_z, &v0);
	map3d_xyz(other_end, mid_y, base_z, &v1);

	/* Unusual case: 2D projection of the yz plane */
	if (!splot_map && yz_plane)
	    map3d_xyz(yaxis_x, mid_y, Z_AXIS.max+Z_AXIS.min-base_z, &v1);

	tic_unitx = (v1.x - v0.x) / xyscaler;
	tic_unity = (v1.y - v0.y) / xyscaler;
	tic_unitz = (v1.z - v0.z) / xyscaler;

	/* Don't output tics and grids if this is the front part of a
	 * two-part grid drawing process: */
	if ((surface_rot_x <= 90 && FRONTGRID != whichgrid) ||
	    (surface_rot_x > 90 && BACKGRID != whichgrid))

	if (Y_AXIS.ticmode)
	    gen_tics(&axis_array[FIRST_Y_AXIS], ytick_callback);

	if (Y_AXIS.label.text) {
	    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
		(surface_rot_x > 90 && FRONTGRID != whichgrid) ||
		splot_map) {
		int x1, y1;
		int save_rotate = Y_AXIS.label.rotate;

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
			gen_tics(&axis_array[FIRST_Y_AXIS], widest_tic_callback);
		    }
		    /* Default displacement with respect to baseline of tics labels */
		    x1 -= (0.5 + widest_tic_strlen) * t->h_char;
		} else { /* usual 3d set view ... */
		    if (Y_AXIS.label.tag == ROTATE_IN_3D_LABEL_TAG) {
			double ang, angx0, angx1, angy0, angy1;
			map3d_xy_double(yaxis_x, Y_AXIS.min, base_z, &angx0, &angy0);
			map3d_xy_double(yaxis_x, Y_AXIS.max, base_z, &angx1, &angy1);
			ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
			if (ang < -90) ang += 180;
			if (ang > 90) ang -= 180;
			Y_AXIS.label.rotate = (ang > 0) ? floor(ang + 0.5) : floor(ang - 0.5);
		    } else if (!yz_projection) {
			/* The 2D default state (ylabel rotate) is not wanted in 3D */
			Y_AXIS.label.rotate = 0;
		    }

		    if (Y_AXIS.ticmode & TICS_ON_AXIS) {
			map3d_xyz(0.0, mid_y, base_z, &v1);
		    } else {
			map3d_xyz(yaxis_x, mid_y, base_z, &v1);
		    }

		    if (yz_projection) {
			v1.x -= 3. * t->h_tic * tic_unitx;
			v1.y -= 3. * t->h_tic * tic_unity;
		    } else if (Y_AXIS.ticmode & TICS_ON_AXIS) {
			v1.x += 2. * t->h_tic * ((Y_AXIS.tic_in) ? 1.0 : -1.0) * tic_unitx;
			v1.y += 2. * t->h_tic * ((Y_AXIS.tic_in) ? 1.0 : -1.0) * tic_unity;
		    } else {
			v1.x -= 10. * t->h_tic * tic_unitx;
			v1.y -= 10. * t->h_tic * tic_unity;
		    }
		
		    if (!Y_AXIS.tic_in) {
			v1.x -= tic_unitx * Y_AXIS.ticscale * t->v_tic;
			v1.y -= tic_unity * Y_AXIS.ticscale * t->v_tic;
		    }
		    TERMCOORD(&v1, x1, y1);
		}

		write_label(x1, y1, &Y_AXIS.label);
		Y_AXIS.label.rotate = save_rotate;
	    }
	}

	/* In "set view map" 2D projection there may also be y2 tics and label */
	if (splot_map) {
	    int y2tic_textwidth = 0;      /* width of y2 tic labels */
	    int y2label_width = 0;
	    if (axis_array[SECOND_Y_AXIS].ticmode) {
		gen_tics(&axis_array[SECOND_Y_AXIS], ytick_callback);
		widest_tic_strlen = 0;
		gen_tics(&axis_array[SECOND_Y_AXIS], widest_tic_callback);
		y2tic_textwidth = (1.5 + widest_tic_strlen) * t->h_char;
	    }
	    if (axis_array[SECOND_Y_AXIS].label.text) {
		int y2 = (plot_bounds.ytop + plot_bounds.ybot) / 2;
		int x2 = plot_bounds.xright;
		y2label_width = 2.5 * t->h_char;
		x2 += y2tic_textwidth;
		x2 += y2label_width;
		write_label(x2, y2, &(axis_array[SECOND_Y_AXIS].label));
	    }
	    /* Jan 2022: this is at least one character width to the right of previous layout */
	    color_box.xoffset = t->h_char + y2tic_textwidth + y2label_width;
	}
    }

    /* do z tics */
    if (Z_AXIS.ticmode
	/* Don't output tics and grids if this is the front part of a
	 * two-part grid drawing process: */
	&& (FRONTGRID != whichgrid)
	&& (splot_map == FALSE)
	&& (surface_rot_x != 0)
	&& (draw_surface
	    || (draw_contour & CONTOUR_SRF)
	    || strchr(pm3d.where,'s') != NULL
	    )
	) {
	gen_tics(&axis_array[FIRST_Z_AXIS], ztick_callback);
    }
    if ((Y_AXIS.zeroaxis)
	&& !X_AXIS.log
	&& inrange(0, X_AXIS.min, X_AXIS.max)
	) {
	vertex v1, v2;

	/* line through x=0 */
	map3d_xyz(0.0, Y_AXIS.min, base_z, &v1);
	map3d_xyz(0.0, Y_AXIS.max, base_z, &v2);
	draw3d_line(&v1, &v2, Y_AXIS.zeroaxis);
    }
    if ((Z_AXIS.zeroaxis)
	&& !X_AXIS.log
	&& inrange(0, X_AXIS.min, X_AXIS.max)
	) {
	vertex v1, v2;

	/* line through x=0 y=0 */
	map3d_xyz(0.0, 0.0, Z_AXIS.min, &v1);
	map3d_xyz(0.0, 0.0, Z_AXIS.max, &v2);
	draw3d_line(&v1, &v2, Z_AXIS.zeroaxis);
    }
    if ((X_AXIS.zeroaxis)
	&& !Y_AXIS.log
	&& inrange(0, Y_AXIS.min, Y_AXIS.max)
	) {
	vertex v1, v2;

	term_apply_lp_properties(X_AXIS.zeroaxis);
	/* line through y=0 */
	map3d_xyz(X_AXIS.min, 0.0, base_z, &v1);
	map3d_xyz(X_AXIS.max, 0.0, base_z, &v2);
	draw3d_line(&v1, &v2, X_AXIS.zeroaxis);
    }

    /* PLACE ZLABEL - along the middle grid Z axis - eh ? */
    if (Z_AXIS.label.text
	&& (splot_map == FALSE)
	&& (current_layer == LAYER_FRONT || whichgrid == ALLGRID)
	&& (draw_surface
	    || (draw_contour & CONTOUR_SRF)
	    || strpbrk(pm3d.where,"st") != NULL
	    )
	) {
	vertex v1;
	double mid_z;

	if (nonlinear(&Z_AXIS)) {
	    mid_z = (Z_AXIS.linked_to_primary->max + Z_AXIS.linked_to_primary->min) / 2.;
	    mid_z = eval_link_function(&Z_AXIS, mid_z);
	} else
	    mid_z = (Z_AXIS.max + Z_AXIS.min) / 2.;

	if (Z_AXIS.ticmode & TICS_ON_AXIS) {
	    map3d_xyz(0, 0, mid_z, &v1);
	    TERMCOORD(&v1, x, y);
	    x -= 5 * t->h_char;
	} else {
	    map3d_xyz(zaxis_x, zaxis_y, mid_z, &v1);
	    TERMCOORD(&v1, x, y);
	    if (fabs(azimuth) > 80)
		y += 2 * sgn(azimuth) * t->v_char;
	    else
		x -= 7 * t->h_char;
	}

	if (Z_AXIS.label.tag == ROTATE_IN_3D_LABEL_TAG) {
	    double ang, angx0, angx1, angy0, angy1;
	    map3d_xy_double(zaxis_x, zaxis_y, Z_AXIS.min, &angx0, &angy0);
	    map3d_xy_double(zaxis_x, zaxis_y, Z_AXIS.max, &angx1, &angy1);
	    ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
	    if (ang < -90) ang += 180;
	    if (ang > 90) ang -= 180;
	    Z_AXIS.label.rotate = (ang > 0) ? floor(ang + 0.5) : floor(ang - 0.5);
	}

	write_label(x, y, &Z_AXIS.label);
    }

    clip_area = clip_save;
}

static void
xtick_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid,		/* linetype or -2 for none */
    struct ticmark *userlabels)
{
    double scale = tic_scale(ticlevel, this_axis) * (this_axis->tic_in ? 1 : -1);
    double other_end = Y_AXIS.min + Y_AXIS.max - xaxis_y;
    struct termentry *t = term;
    vertex v1, v2, v3, v4;

    /* Draw full-length grid line */
    map3d_xyz(place, xaxis_y, base_z, &v1);
    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	/* to save mapping twice, map non-axis y */
	map3d_xyz(place, other_end, base_z, &v3);
	draw3d_line(&v1, &v3, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    /* Vertical grid lines (in yz plane) */
    if (grid_vertical_lines && grid.l_type > LT_NODRAW) {
	vertex v4, v5;
	double which_face = (surface_rot_x > 90 && surface_rot_x < 270) ? xaxis_y : other_end;
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	map3d_xyz(place, which_face, Z_AXIS.min, &v4);
	map3d_xyz(place, which_face, ceiling_z, &v5);
	draw3d_line(&v4, &v5, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    if ((X_AXIS.ticmode & TICS_ON_AXIS)
	&& !Y_AXIS.log
	&& inrange (0.0, Y_AXIS.min, Y_AXIS.max)
	) {
	map3d_xyz(place, 0.0, base_z, &v1);
    }

    /* NB: secondary axis must be linked to primary */
    if (this_axis->index == SECOND_X_AXIS
    &&  this_axis->linked_to_primary
    &&  this_axis->link_udf->at != NULL) {
	place = eval_link_function(&axis_array[FIRST_X_AXIS], place);
    }

    /* Draw bottom tic mark */
    if ((this_axis->index == FIRST_X_AXIS)
    ||  (this_axis->index == SECOND_X_AXIS && (this_axis->ticmode & TICS_MIRROR))) {
	v2.x = v1.x + tic_unitx * scale * t->v_tic ;
	v2.y = v1.y + tic_unity * scale * t->v_tic ;
	v2.z = v1.z + tic_unitz * scale * t->v_tic ;
	v2.real_z = v1.real_z;
	draw3d_line(&v1, &v2, &border_lp);
    }

    /* Draw top tic mark */
    if ((this_axis->index == SECOND_X_AXIS)
    ||  (this_axis->index == FIRST_X_AXIS && (this_axis->ticmode & TICS_MIRROR))) {
	if (xz_projection)
	    map3d_xyz(place, other_end, Z_AXIS.max, &v3);
	else
	    map3d_xyz(place, other_end, base_z, &v3);
	v4.x = v3.x - tic_unitx * scale * t->v_tic;
	v4.y = v3.y - tic_unity * scale * t->v_tic;
	v4.z = v3.z - tic_unitz * scale * t->v_tic;
	v4.real_z = v3.real_z;
	draw3d_line(&v3, &v4, &border_lp);
    }

    /* Draw tic label */
    if (text) {
	int just;
	int x2, y2;
	int angle;
	int offsetx, offsety;

	/* Skip label if we've already written a user-specified one here */
#	define MINIMUM_SEPARATION 0.001
	while (userlabels) {
	    if (fabs((place - userlabels->position) / (X_AXIS.max - X_AXIS.min))
		<= MINIMUM_SEPARATION) {
		text = NULL;
		break;
	    }
	    userlabels = userlabels->next;
	}
#	undef MINIMUM_SEPARATION

	/* get offset */
	map3d_position_r(&(this_axis->ticdef.offset), &offsetx, &offsety, "xtics");

	/* allow manual justification of tick labels, but only for projections */
	if ((splot_map || xz_projection) && this_axis->manual_justify)
	    just = this_axis->tic_pos;
	else if (tic_unitx * xscaler < -0.9)
	    just = LEFT;
	else if (tic_unitx * xscaler < 0.9)
	    just = CENTRE;
	else
	    just = RIGHT;

	if (this_axis->index == SECOND_X_AXIS) {
	    v4.x = v3.x + tic_unitx * t->h_char * 1;
	    v4.y = v3.y + tic_unity * t->v_char * 1;
	    if (!this_axis->tic_in) {
		v4.x += tic_unitx * t->v_tic * this_axis->ticscale;
		v4.y += tic_unity * t->v_tic * this_axis->ticscale;
	    }
	    TERMCOORD(&v4, x2, y2);
	} else {
	    v2.x = v1.x - tic_unitx * t->h_char * 1;
	    v2.y = v1.y - tic_unity * t->v_char * 1;
	    if (!this_axis->tic_in) {
		v2.x -= tic_unitx * t->v_tic * this_axis->ticscale;
		v2.y -= tic_unity * t->v_tic * this_axis->ticscale;
	    }
	    TERMCOORD(&v2, x2, y2);
	}

	/* User-specified different color for the tics text */
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	angle = this_axis->tic_rotate;
	if (!(splot_map && angle && term->text_angle(angle)))
	    angle = 0;
	ignore_enhanced(!this_axis->ticdef.enhanced);
	write_multiline(x2+offsetx, y2+offsety, text, just, JUST_TOP,
			    angle, this_axis->ticdef.font);
	ignore_enhanced(FALSE);
	term->text_angle(0);
	term_apply_lp_properties(&border_lp);
    }
}

static void
ytick_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid,
    struct ticmark *userlabels)
{
    double scale = tic_scale(ticlevel, this_axis) * (this_axis->tic_in ? 1 : -1);
    double other_end = X_AXIS.min + X_AXIS.max - yaxis_x;
    struct termentry *t = term;
    vertex v1, v2, v3, v4;

    /* Draw full-length grid line */
    map3d_xyz(yaxis_x, place, base_z, &v1);
    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	map3d_xyz(other_end, place, base_z, &v3);
	draw3d_line(&v1, &v3, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    /* Vertical grid lines (in xz plane) */
    if (grid_vertical_lines && grid.l_type > LT_NODRAW) {
	vertex v4, v5;
	double which_face = (surface_rot_x > 90 && surface_rot_x < 270) ? yaxis_x : other_end;
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	map3d_xyz(which_face, place, Z_AXIS.min, &v4);
	map3d_xyz(which_face, place, ceiling_z, &v5);
	draw3d_line(&v4, &v5, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    if (Y_AXIS.ticmode & TICS_ON_AXIS
	&& !X_AXIS.log
	&& inrange (0.0, X_AXIS.min, X_AXIS.max)
	) {
	map3d_xyz(0.0, place, base_z, &v1);
    }

    /* NB: secondary axis must be linked to primary */
    if (this_axis->index == SECOND_Y_AXIS
    &&  this_axis->linked_to_primary
    &&  this_axis->link_udf->at != NULL) {
	place = eval_link_function(&axis_array[FIRST_Y_AXIS], place);
    }

    /* Draw left tic mark */
    if ((this_axis->index == FIRST_Y_AXIS)
    ||  (this_axis->index == SECOND_Y_AXIS && (this_axis->ticmode & TICS_MIRROR))) {
	v2.x = v1.x + tic_unitx * scale * t->h_tic;
	v2.y = v1.y + tic_unity * scale * t->h_tic;
	v2.z = v1.z + tic_unitz * scale * t->h_tic;
	v2.real_z = v1.real_z;
	draw3d_line(&v1, &v2, &border_lp);
    }

    /* Draw right tic mark */
    if ((this_axis->index == SECOND_Y_AXIS)
    ||  (this_axis->index == FIRST_Y_AXIS && (this_axis->ticmode & TICS_MIRROR))) {
	if (yz_projection)
	    map3d_xyz(other_end, place, Z_AXIS.min, &v3);
	else
	    map3d_xyz(other_end, place, base_z, &v3);
	v4.x = v3.x - tic_unitx * scale * t->h_tic;
	v4.y = v3.y - tic_unity * scale * t->h_tic;
	v4.z = v3.z - tic_unitz * scale * t->h_tic;
	v4.real_z = v3.real_z;
	draw3d_line(&v3, &v4, &border_lp);
    }

    /* Draw tic label */
    if (text) {
	int just;
	int x2, y2;
	int angle;
	int offsetx, offsety;

	/* Skip label if we've already written a user-specified one here */
#	define MINIMUM_SEPARATION 0.001
	while (userlabels) {
	    if (fabs((place - userlabels->position) / (Y_AXIS.max - Y_AXIS.min))
		<= MINIMUM_SEPARATION) {
		text = NULL;
		break;
	    }
	    userlabels = userlabels->next;
	}
#	undef MINIMUM_SEPARATION

	/* get offset */
	map3d_position_r(&(this_axis->ticdef.offset), &offsetx, &offsety, "ytics");

	/* allow manual justification of tick labels, but only for projections */
	if ((splot_map || yz_projection) && this_axis->manual_justify)
	    just = this_axis->tic_pos;
	else if (tic_unitx * xscaler < -0.9)
	    just = (this_axis->index == FIRST_Y_AXIS) ? LEFT : RIGHT;
	else if (tic_unitx * xscaler < 0.9)
	    just = CENTRE;
	else
	    just = (this_axis->index == FIRST_Y_AXIS) ? RIGHT : LEFT;

	if (this_axis->index == SECOND_Y_AXIS) {
	    v4.x = v3.x + tic_unitx * t->h_char * 1;
	    v4.y = v3.y + tic_unity * t->v_char * 1;
	    if (!this_axis->tic_in) {
		v4.x += tic_unitx * t->h_tic * this_axis->ticscale;
		v4.y += tic_unity * t->v_tic * this_axis->ticscale;
	    }
	    TERMCOORD(&v4, x2, y2);
	} else {
	    v2.x = v1.x - tic_unitx * t->h_char * 1;
	    v2.y = v1.y - tic_unity * t->v_char * 1;
	    if (!this_axis->tic_in) {
		v2.x -= tic_unitx * t->h_tic * this_axis->ticscale;
		v2.y -= tic_unity * t->v_tic * this_axis->ticscale;
	    }
	    TERMCOORD(&v2, x2, y2);
	}

	/* User-specified different color for the tics text */
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	angle = this_axis->tic_rotate;
	if (!(splot_map && angle && term->text_angle(angle)))
	    angle = 0;
	ignore_enhanced(!this_axis->ticdef.enhanced);
	write_multiline(x2+offsetx, y2+offsety, text, just, JUST_TOP,
			angle, this_axis->ticdef.font);
	ignore_enhanced(FALSE);
	term->text_angle(0);
	term_apply_lp_properties(&border_lp);
    }
}

static void
ztick_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid,
    struct ticmark *userlabels)
{
    struct termentry *t = term;
    int len = tic_scale(ticlevel, this_axis)
	* (this_axis->tic_in ? 1 : -1) * (term->h_tic);
    vertex v1, v2, v3;

    if (this_axis->ticmode & TICS_ON_AXIS)
	map3d_xyz(0., 0., place, &v1);
    else
	map3d_xyz(zaxis_x, zaxis_y, place, &v1);

    /* Needed both for grid and for azimuth ztics */
    map3d_xyz(right_x, right_y, place, &v3);

    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	map3d_xyz(back_x, back_y, place, &v2);
	draw3d_line(&v1, &v2, &grid);
	draw3d_line(&v2, &v3, &grid);
	(t->layer)(TERM_LAYER_END_GRID);
    }
    if (azimuth != 0) {
	v2.x = v1.x + (v3.x - v1.x) * len / xyscaler;
	v2.y = v1.y + (v3.y - v1.y) * len / xyscaler;
	v2.z = v1.z + (v3.z - v1.z) * len / xyscaler;
    } else {
	v2.x = v1.x + len / (double)xscaler;
	v2.y = v1.y;
	v2.z = v1.z;
    }
    v2.real_z = v1.real_z;
    draw3d_line(&v1, &v2, &border_lp);

    if (text) {
	int x1, y1;
	int just;
	int offsetx, offsety;

	/* Skip label if we've already written a user-specified one here */
#	define MINIMUM_SEPARATION 0.001
	while (userlabels) {
	    if (fabs((place - userlabels->position) / (Z_AXIS.max - Z_AXIS.min))
		<= MINIMUM_SEPARATION) {
		text = NULL;
		break;
	    }
	    userlabels = userlabels->next;
	}
#	undef MINIMUM_SEPARATION

	/* get offset */
	map3d_position_r(&(this_axis->ticdef.offset), &offsetx, &offsety, "ztics");
	TERMCOORD(&v1, x1, y1);

	if (fabs(azimuth) > 80) {
	    /* Z axis is (nearly) horizontal */
	    y1 += sgn(azimuth) * (term->v_tic) * 2;
	} else {
	    /* the normal case */
	    x1 -= (term->h_tic) * 2;
	    if (!this_axis->tic_in)
		x1 -= (term->h_tic) * this_axis->ticscale;
	}

	/* allow manual justification of tick labels, but only for projections */
	if ((xz_projection || yz_projection) && this_axis->manual_justify)
	    just = this_axis->tic_pos;
	else
	    just = RIGHT;

	/* User-specified different color for the tics text */
	if (this_axis->ticdef.textcolor.type == TC_Z)
	    this_axis->ticdef.textcolor.value = place;
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	ignore_enhanced(!this_axis->ticdef.enhanced);
	write_multiline(x1+offsetx, y1+offsety, text, just, JUST_CENTRE,
			0, this_axis->ticdef.font);
	ignore_enhanced(FALSE);
	term_apply_lp_properties(&border_lp);
    }

    if (Z_AXIS.ticmode & TICS_MIRROR) {
	if (azimuth != 0) {
	    v2.x = v3.x + (v1.x - v3.x) * len / xyscaler;
	    v2.y = v3.y + (v1.y - v3.y) * len / xyscaler;
	    v2.z = v3.z + (v1.z - v3.z) * len / xyscaler;
	    draw3d_line(&v3, &v2, &border_lp);
	} else {
	    map3d_xyz(right_x, right_y, place, &v1);
	    v2.x = v1.x - len / (double)xscaler;
	    v2.y = v1.y;
	    v2.z = v1.z;
	    v2.real_z = v1.real_z;
	    draw3d_line(&v1, &v2, &border_lp);
	}
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
    double xx, yy;

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
    case polar_axes:
	(void) polar_to_xy(*xpos, *ypos, &xx, &yy, FALSE);
	*xpos = axis_log_value_checked(FIRST_X_AXIS, xx, what);
	*ypos = axis_log_value_checked(FIRST_Y_AXIS, yy, what);
	plot_coords = TRUE;
	pos->scaley = polar_axes;	/* Just to make sure */
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
    case polar_axes:
	break;
    }

    switch (pos->scalez) {
    case first_axes:
    case second_axes:
    case polar_axes:
	if (splot_map)
	    *zpos = 1;	/* Avoid failure if z=0 with logscale z */
	else
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
	int_error(NO_CARET,"Cannot mix screen or character coords with plot coords");

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
    double xx, yy;
    map3d_position_r_double(pos, &xx, &yy, what);
    *x = xx;
    *y = yy;
}

void
map3d_position_r_double(
    struct position *pos,
    double *xx, double *yy,
    const char *what)
{
    double xpos = pos->x;
    double ypos = pos->y;
    double zpos = (splot_map) ? Z_AXIS.min : pos->z;

    /* startpoint in graph coordinates */
    if (map3d_getposition(pos, what, &xpos, &ypos, &zpos) == 0) {
	int xoriginlocal, yoriginlocal;
	map3d_xy_double(xpos, ypos, zpos, xx, yy);
	if (pos->scalex == graph)
	    xpos = X_AXIS.min;
	else
	    xpos = 0;
	if (pos->scaley == graph)
	    ypos = (splot_map) ? Y_AXIS.max : Y_AXIS.min;
	else
	    ypos = 0;
	if (pos->scalez == graph)
	    zpos = Z_AXIS.min;
	else if (splot_map)
	    zpos = Z_AXIS.min;
	else
	    zpos = 0;
	map3d_xy(xpos, ypos, zpos, &xoriginlocal, &yoriginlocal);
	*xx -= xoriginlocal;
	*yy -= yoriginlocal;
    } else {
    /* endpoint `screen' or 'character' coordinates */
	*xx = xpos;
	*yy = ypos;
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

    if (key->invert)
	INVERT_KEY();

    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
    if (key->just == GPKEY_LEFT) {
	write_multiline(xl + key_text_left, yl, text, LEFT, JUST_TOP, 0, key->font);
    } else {
	if ((*term->justify_text) (RIGHT)) {
	    write_multiline(xl + key_text_right, yl, text, RIGHT, JUST_TOP, 0, key->font);
	} else {
	    int x = xl + key_text_right - (term->h_char) * estimate_strlen(text, NULL);
	    write_multiline(x, yl, text, LEFT, JUST_TOP, 0, key->font);
	}
    }
    (term->layer)(TERM_LAYER_END_KEYSAMPLE);
}

static void
key_sample_line(int xl, int yl)
{
    legend_key *key = &keyT;
    BoundingBox *clip_save = clip_area;

    /* Clip against canvas */
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    if (key->invert)
	INVERT_KEY();

    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
    draw_clip_line(xl + key_sample_left, yl, xl + key_sample_right, yl);
    (term->layer)(TERM_LAYER_END_KEYSAMPLE);

    clip_area = clip_save;
}

static void
key_sample_point(struct surface_points *this_plot, int xl, int yl, int pointtype)
{
    legend_key *key = &keyT;
    BoundingBox *clip_save = clip_area;

    /* Clip against canvas */
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;


    if (key->invert)
	INVERT_KEY();

    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
    if (!clip_point(xl + key_point_offset, yl)) {
	if (pointtype == PT_CHARACTER && this_plot) {
	    apply_pm3dcolor(&(this_plot->labels->textcolor));
	    (*term->put_text) (xl + key_point_offset, yl,
			       this_plot->lp_properties.p_char);
	    apply_pm3dcolor(&(this_plot->lp_properties.pm3d_color));
	} else {
	    (*term->point) (xl + key_point_offset, yl, pointtype);
	}
    }
    (term->layer)(TERM_LAYER_END_KEYSAMPLE);

    clip_area = clip_save;
}

static void
key_sample_fill(int xl, int yl, struct surface_points *this_plot)
{
    legend_key *key = &keyT;
    struct fill_style_type *fs = &this_plot->fill_properties;
    int style = style_from_fill(fs);
    int x = xl + key_sample_left;
    int y = yl - key_entry_height/4;
    int w = key_sample_right - key_sample_left;
    int h = key_entry_height/2;

    if (key->invert) {
	INVERT_KEY();
	y = yl - key_entry_height/4;
    }

    if (!(term->fillbox))
	return;
    (term->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);

    if (this_plot->plot_style == CIRCLES) {
	do_arc(x+w/2, yl, key_entry_height/4, 0., 360., style, FALSE);
	/* Retrace the border if the style requests it */
	if (need_fill_border(fs))
	    do_arc(x+w/2, yl, key_entry_height/4, 0., 360., 0, FALSE);

    } else if (w > 0) {
	(term->fillbox)(style,x,y,w,h);

	/* FIXME:  what other plot styles want a border on the key sample? */
	if ((this_plot->plot_style & PLOT_STYLE_HAS_PM3DBORDER)) {
	    if (pm3d.border.l_type != LT_NODRAW && pm3d.border.l_type != LT_DEFAULT)
		term_apply_lp_properties(&pm3d.border);
	    newpath();
	    draw_clip_line( x, y, x+w, y);
	    draw_clip_line( x+w, y, x+w, y+h);
	    draw_clip_line( x+w, y+h, x, y+h);
	    draw_clip_line( x, y+h, x, y);
	    closepath();
	}
    }

    (term->layer)(TERM_LAYER_END_KEYSAMPLE);
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
    struct coordinate *points;

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
    || (colortype == TC_LINESTYLE)) {
	lp_style_type lptmp = plot->lp_properties;
	if (plot->lp_properties.l_type == LT_COLORFROMCOLUMN)
		lp_use_properties(&lptmp, (int)(plot->iso_crvs->points[0].CRD_COLOR));
	apply_pm3dcolor(&lptmp.pm3d_color);
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

    if (key->invert)
	INVERT_KEY();

    clip_move(x1, yl);
    x2 = x1;
    while (i <= steps) {
	/* if (i>1) set_color( i==steps ? 1 : (i-0.5)/steps ); ... range [0:1] */
	gray = (i==steps) ? gray_to : gray_from+i*gray_step;
	set_color(gray);
	clip_move(x2, yl);
	x2 = (i==steps) ? x_to : x1 + (int)(i*step+0.5);
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
    legend_key *key = &keyT;
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
    || (colortype == TC_LINESTYLE)) {
	lp_style_type lptmp = plot->lp_properties;
	if (plot->lp_properties.l_type == LT_COLORFROMCOLUMN)
		lp_use_properties(&lptmp, (int)(plot->iso_crvs->points[0].CRD_COLOR));
	apply_pm3dcolor(&lptmp.pm3d_color);
	key_sample_point(plot, xl, yl, pointtype);
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
    /* Clip to canvas */
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    if (key->invert)
	INVERT_KEY();

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
    struct coordinate *tails = plot->iso_crvs->points;
    struct coordinate *heads = plot->iso_crvs->next->points;

    /* Only necessary once, unless variable arrow style */
    ap = plot->arrow_properties;
    term_apply_lp_properties(&ap.lp_properties);
    apply_head_properties(&ap);

    for (i = 0; i < plot->iso_crvs->p_count; i++) {

	if (heads[i].type == UNDEFINED || tails[i].type == UNDEFINED)
	    continue;

	/* variable arrow style read from extra data column */
	if (plot->arrow_properties.tag == AS_VARIABLE) {
	    int as = heads[i].CRD_ASTYLE;
	    arrow_use_properties(&ap, as);
	    term_apply_lp_properties(&ap.lp_properties);
	    apply_head_properties(&ap);
	    /* copy to plot lp_properties so check for variable color can see it */
	    plot->lp_properties = ap.lp_properties;
	}

	/* variable color read from extra data column */
	check3d_for_variable_color(plot, &heads[i]);

	/* The normal case: both ends in range */
	if (heads[i].type == INRANGE && tails[i].type == INRANGE) {
	    map3d_xy_double(tails[i].x, tails[i].y, tails[i].z, &x1, &y1);
	    map3d_xy_double(heads[i].x, heads[i].y, heads[i].z, &x2, &y2);
	    draw_clip_arrow(x1, y1, x2, y2, ap.head);

	/* "set clip two" - both ends out of range */
	} else if (heads[i].type != INRANGE && tails[i].type != INRANGE) {
	    double lx[2], ly[2], lz[2];
	    if (!clip_lines2)
		continue;
	    two_edge3d_intersect(&tails[i], &heads[i], lx, ly, lz);
	    map3d_xy_double(lx[0], ly[0], lz[0], &x1, &y1);
	    map3d_xy_double(lx[1], ly[1], lz[1], &x2, &y2);
	    draw_clip_arrow(x1, y1, x2, y2, ap.head);

	/* "set clip one" - one end out of range */
	} else if (clip_lines1) {
	    double clip_x, clip_y, clip_z;
	    edge3d_intersect(&heads[i], &tails[i], &clip_x, &clip_y, &clip_z);
	    if (tails[i].type == INRANGE) {
		map3d_xy_double(tails[i].x, tails[i].y, tails[i].z, &x1, &y1);
		map3d_xy_double(clip_x, clip_y, clip_z, &x2, &y2);
	    } else {
		map3d_xy_double(clip_x, clip_y, clip_z, &x1, &y1);
		map3d_xy_double(heads[i].x, heads[i].y, heads[i].z, &x2, &y2);
	    }
	    draw_clip_arrow(x1, y1, x2, y2, ap.head);
	}
    }
}

/*
 * splot with zerrorfill
 * This 3D style is similar to a 2D filledcurves plot between two lines.
 * Put together a list of the component quadrangles using the data structures
 * normally used by pm3d routines pm3d_plot(), pm3d_depth_queue_flush().
 * The component quadrangles from all plots are sorted and flushed together.
 */
static void
plot3d_zerrorfill(struct surface_points *plot)
{
    struct iso_curve *curve = plot->iso_crvs;
    int i1, i2;		/* index leading and trailing coord of current quadrangle */
    int count = 0;
    gpdPoint corner[4];

    /* Find leading edge of first quadrangle */
    for (i1=0; i1 < curve->p_count; i1++) {
	if (curve->points[i1].type == INRANGE)
	    break;
    }

    for (i2=i1+1; i2 < curve->p_count; i2++) {
	if (curve->points[i2].type != INRANGE)
	    continue;
	count++;	/* Found one */
	corner[0].x = corner[1].x = curve->points[i1].x;
	corner[0].y = corner[1].y = curve->points[i1].y;
	corner[0].z = curve->points[i1].CRD_ZLOW;
	corner[1].z = curve->points[i1].CRD_ZHIGH;
	corner[2].x = corner[3].x = curve->points[i2].x;
	corner[2].y = corner[3].y = curve->points[i2].y;
	corner[3].z = curve->points[i2].CRD_ZLOW;
	corner[2].z = curve->points[i2].CRD_ZHIGH;
	pm3d_add_quadrangle(plot, corner);
	i1 = i2;
    }

    if (count == 0)
	int_error(NO_CARET, "all points out of range");

    /* Default is to write out each zerror plot as we come to it     */
    /* (most recent plot occludes all previous plots). To get proper */
    /* sorting, use "set pm3d depthorder".                           */
    if (pm3d.direction != PM3D_DEPTH)
	pm3d_depth_queue_flush();

}

/*
 * 3D version of plot with boxes.
 * By default only a flat rectangle is drawn.  "set boxdepth <depth>"
 * changes this to draw real boxes (4 sides + top).
 * The boxes are drawn as pm3d rectangles. This means that depth-cueing
 * must be done with "set pm3d depth base" rather than with "set hidden3d".
 */
static void
plot3d_boxes(struct surface_points *plot)
{
    int i;			/* point index */
    double dxl, dxh;		/* rectangle extent along X axis */
    double dyl, dyh;		/* rectangle extent along Y axis */
    double zbase, dz;		/* box base and height */
    fill_style_type save_fillstyle;

    struct iso_curve *icrvs = plot->iso_crvs;
    gpdPoint corner[4];

    /* This initialization is normally done via pm3d_plot()
     * but 3D boxes are drawn in a parallel code path.
     */
    if (pm3d_shade.strength > 0)
	pm3d_init_lighting_model();

    /* FIXME: fillstyle and border color always come from "set style fill" */
    pm3d.border = plot->lp_properties;
    pm3d.border.pm3d_color = default_fillstyle.border_color;

    while (icrvs) {
	struct coordinate *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {

	    if (points[i].type == UNDEFINED)
		continue;

	    dxh = points[i].xhigh;
	    dxl = points[i].xlow;
	    dyl = points[i].y;
	    dyh = points[i].y;
	    dz = points[i].z;

	    /* Box is out of range on y */
	    if ((dyl > Y_AXIS.min && dyl > Y_AXIS.max)
	    ||  (dyl < Y_AXIS.min && dyl < Y_AXIS.max))
		continue;

	    if (boxdepth != 0) {
		double depth = (boxdepth > 0) ? boxdepth : boxwidth * yscaler/xscaler;
		if (Y_AXIS.log) {
		    dyl *= pow(Y_AXIS.base, -depth/2.);
		    dyh *= pow(Y_AXIS.base, depth/2.);
		} else {
		    dyl -= depth / 2.;
		    dyh += depth / 2.;
		}
		cliptorange(dyl, Y_AXIS.min, Y_AXIS.max);
		cliptorange(dyh, Y_AXIS.min, Y_AXIS.max);
	    }

	    /* clip to border */
	    cliptorange(dxl, X_AXIS.min, X_AXIS.max);
	    cliptorange(dxh, X_AXIS.min, X_AXIS.max);

	    /* Entire box is out of range on x */
	    if (dxl == dxh && (dxl == X_AXIS.min || dxl == X_AXIS.max))
		continue;

	    zbase = 0;
	    cliptorange(zbase, Z_AXIS.min, Z_AXIS.max);

	    /* Copy variable color value into plot header for pm3d_add_quadrangle */
	    if (plot->pm3d_color_from_column)
		plot->lp_properties.pm3d_color.lt =  points[i].CRD_COLOR;

	    /* Construct and store single pm3d rectangle (front of box) */
	    /* Z	corner1	corner2	*/
	    /* 0	corner0 corner3 */
	    corner[0].x = corner[1].x = dxl;
	    corner[2].x = corner[3].x = dxh;
	    corner[0].y = corner[1].y = corner[2].y = corner[3].y = dyl;
	    corner[0].z = corner[3].z = zbase;
	    corner[1].z = corner[2].z = dz;
	    pm3d_add_quadrangle(plot, corner);

	    /* The normal case is to draw the front only (boxdepth = 0) */
	    if (boxdepth == 0)
		continue;

	    /* Back side of the box */
	    corner[0].y = corner[1].y = corner[2].y = corner[3].y = dyh;
	    pm3d_add_quadrangle(plot, corner);

	    /* Left side of box */
	    corner[2].x = corner[3].x = dxl;
	    corner[0].y = corner[1].y = dyl;
	    corner[0].z = corner[3].z = zbase;
	    corner[1].z = corner[2].z = dz;
	    pm3d_add_quadrangle(plot, corner);

	    /* Right side of box */
	    corner[0].x = corner[1].x = corner[2].x = corner[3].x = dxh;
	    pm3d_add_quadrangle(plot, corner);

	    /* Top of box */
	    corner[0].x = corner[1].x = dxl;
	    corner[0].y = corner[3].y = dyl;
	    corner[1].y = corner[2].y = dyh;
	    corner[0].z = corner[3].z = dz;
	    pm3d_add_quadrangle(plot, corner);

	}	/* loop over points */

	icrvs = icrvs->next;
    }

    /* FIXME The only way to get the pm3d flush code to see our fill */
    /* style is to temporarily copy it to the global fillstyle.      */
    save_fillstyle = default_fillstyle;
    default_fillstyle = plot->fill_properties;

    /* By default we write out each set of boxes as it is seen.  */
    /* The other option is to let them accummulate and then sort */
    /* them together with all other pm3d elements to draw later. */
    if (pm3d.direction != PM3D_DEPTH) {
	pm3d.base_sort = TRUE;
	pm3d_depth_queue_flush();
	pm3d.base_sort = FALSE;
    }

    /* Restore global fillstyle */
    default_fillstyle = save_fillstyle;
}

/*
 * Plot the data as a set of polygons.
 * Successive lines of input data provide vertex coordinates.
 * A blank line separates polygons.
 * E.g. two triangles:
 *	x1 y1 z1
 *	x2 y2 z2
 *	x3 y3 z3
 *
 *	x1 y1 z1
 *	x2 y2 z2
 *	x3 y3 z3
 */
static void
plot3d_polygons(struct surface_points *plot)
{
    int nv;
    struct iso_curve *icrvs;
    struct coordinate *points;
    int style;

    static gpdPoint *quad = NULL;
    static int quadmax = 0;

    /* These don't need to be drawn at all */
    if (plot->lp_properties.l_type == LT_NODRAW)
	return;

    /* This initialization is normally done in pm3d_plot()
     * but polygons do not necessarily use that code path.
     */
    if (pm3d_shade.strength > 0)
	pm3d_init_lighting_model();
    style = style_from_fill(&plot->fill_properties);

    /* Most polygons are small */
    quadmax = 8;
    quad = gp_realloc(quad, quadmax * sizeof(gpdPoint), NULL);

    for (icrvs = plot->iso_crvs; icrvs; icrvs = icrvs->next) {

	/* Allow for very large polygons (e.g. cartographic outlines) */
	int npoints = icrvs->p_count;
	if (npoints > quadmax) {
	    quadmax = npoints;
	    quad = gp_realloc(quad, quadmax * sizeof(gpdPoint), NULL);
	}

	/* Copy the vertex coordinates into a pm3d quadrangle */
	for (nv = 0, points = icrvs->points; nv < npoints; nv++) {
	    quad[nv].x = points[nv].x;
	    quad[nv].y = points[nv].y;
	    quad[nv].z = points[nv].z;
	}
	/* Treat triangle as a degenerate quadrangle */
	if (nv == 3) {
	    quad[3].x = points[0].x;
	    quad[3].y = points[0].y;
	    quad[3].z = points[0].z;
	}
	/* Ignore lines and points */
	if (nv < 3)
	    continue;

	/* Coloring piggybacks on options for isosurface */
	quad[0].c = plot->fill_properties.border_color.lt;
	quad[1].c = style;
	pm3d_add_polygon( plot, quad, nv );
    }

    /* Default is to write out each polygon as we come to it. */
    /* To get proper sorting, use "set pm3d depthorder".      */
    if (pm3d.direction != PM3D_DEPTH)
	pm3d_depth_queue_flush();

    /* Clean up */
    free(quad);
    quadmax = 0;
    quad = NULL;
}


static void
check3d_for_variable_color(struct surface_points *plot, struct coordinate *point)
{
    int colortype = plot->lp_properties.pm3d_color.type;

    switch( colortype ) {
    case TC_RGB:
	if (plot->pm3d_color_from_column && plot->lp_properties.pm3d_color.value < 0.0)
	    set_rgbcolor_var( (unsigned int)point->CRD_COLOR );
	break;
    case TC_Z:
    case TC_DEFAULT:   /* pm3d mode assumes this is default */
	if (plot->pm3d_color_from_column)
	    set_color( cb2gray(point->CRD_COLOR) );
	else
	    set_color( cb2gray(point->z) );
	break;
    case TC_LINESTYLE:	/* color from linestyle in data column */
	plot->lp_properties.pm3d_color.lt = (int)(point->CRD_COLOR);
	apply_pm3dcolor(&(plot->lp_properties.pm3d_color));
	break;
    default:
	/* The other cases were taken care of already */
	break;
    }
}

void
do_3dkey_layout(legend_key *key, int *xinkey, int *yinkey)
{
    struct termentry *t = term;
    int key_height, key_width;
    int dx, dy;

    /* NOTE: All of these had better not change after being calculated here! */
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

    /* Key title width and height, adjusted for font size and markup */
    key_title_extra = 0;
    key_title_height = 0;
    if (key->title.text) {
	double est_height;
	if (key->title.font)
	    t->set_font(key->title.font);
	estimate_strlen(key->title.text, &est_height);
	key_title_height = est_height * t->v_char;
	if (key->title.font)
	    t->set_font("");
	/* Allow a little extra clearance for markup */
	if ((t->flags & TERM_ENHANCED_TEXT)
	&&  (strchr(key->title.text,'^') || strchr(key->title.text,'_')))
	    key_title_extra = t->v_char/2;
    }

    key_width = key_col_wth * (key_cols - 1) + key_size_right + key_size_left;
    key_height = key_title_height + key_title_extra
		+ key_entry_height * key_rows + key->height_fix * t->v_char;

    /* Make room for extra long title */
    if (key_width < key_title_width)
	key_width = key_title_width;

    /* Now that we know the size of the key, we can position it as requested */
    if (key->region == GPKEY_USER_PLACEMENT) {
	int corner_x, corner_y;
	t_position keypos = key->user_pos;

	/* Translate request for graph coordinates from x/y
	 * to whatever the equivalent is for an xz or yz projection
	 */
	if (yz_projection && key->user_pos.scalex == graph) {
	    keypos.scalez = graph;
	    keypos.z = 1.0 - key->user_pos.x;
	    keypos.x = 0;
	}
	if (xz_projection && key->user_pos.scalex == graph) {
	    keypos.scalez = graph;
	    keypos.z = key->user_pos.x;
	    keypos.x = 0;
	}
	if (xz_projection && key->user_pos.scaley == graph) {
	    keypos.scalex = graph;
	    keypos.x = key->user_pos.y;
	    keypos.y = 0;
	}
	
	map3d_position(&keypos, &corner_x, &corner_y, "key");

	if (key->hpos == CENTRE)
	    key->bounds.xleft = corner_x - key_width / 2;
	else if (key->hpos == RIGHT)
	    key->bounds.xleft = corner_x - key_width;
	else
	    key->bounds.xleft = corner_x;
	key->bounds.xright = key->bounds.xleft + key_width;

	key->bounds.ytop = corner_y;
	key->bounds.ybot = corner_y - key_height;

	*xinkey = key->bounds.xleft + key_size_left;
	*yinkey = key->bounds.ytop - key_title_height - key_title_extra;

    } else {
	BoundingBox *bounds;
	if (key->fixed && !splot_map)
	    bounds = &page_bounds;
	else
	    bounds = &plot_bounds;

	if (key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_BMARGIN) {
	    if (ptitl_cnt > 0) {
		/* we divide into columns, then centre in column by considering
		 * ratio of key_left_size to key_right_size
		 * key_size_left / (key_size_left+key_size_right)
		 *               * (bounds->xright-bounds->xleft)/key_cols
		 * do one integer division to maximise accuracy (hope we dont overflow!)
		 */
		*xinkey = bounds->xleft
		   + ((bounds->xright - bounds->xleft) * key_size_left)
		   / (key_cols * (key_size_left + key_size_right));
		key->bounds.xleft = *xinkey - key_size_left;
		key->bounds.xright = key->bounds.xleft + key_width;

		key->bounds.ytop = bounds->ybot;
		key->bounds.ybot = bounds->ybot - key_height;
		*yinkey = key->bounds.ytop - key_title_height - key_title_extra;
	    }

	} else {
	    if (key->vpos == JUST_TOP) {
		key->bounds.ytop = bounds->ytop - t->v_tic;
		key->bounds.ybot = key->bounds.ytop - key_height;
		*yinkey = key->bounds.ytop - key_title_height - key_title_extra;
	    } else {
		key->bounds.ybot = bounds->ybot + t->v_tic;
		key->bounds.ytop = key->bounds.ybot + key_height;
		*yinkey = key->bounds.ytop - key_title_height - key_title_extra;
	    }
	    if (key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_RMARGIN) {
		/* keys outside plot border (right) */
		key->bounds.xleft = bounds->xright + t->h_tic;
		key->bounds.xright = key->bounds.xleft + key_width;
		*xinkey = key->bounds.xleft + key_size_left;
	    } else if (key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_LMARGIN) {
		/* keys outside plot border (left) */
		key->bounds.xright = bounds->xleft - t->h_tic;
		key->bounds.xleft = key->bounds.xright - key_width;
		*xinkey = key->bounds.xleft + key_size_left;
	    } else if (key->hpos == LEFT) {
		key->bounds.xleft = bounds->xleft + t->h_tic;
		key->bounds.xright = key->bounds.xleft + key_width;
		*xinkey = key->bounds.xleft + key_size_left;
	    } else {
		key->bounds.xright = bounds->xright - t->h_tic;
		key->bounds.xleft = key->bounds.xright - key_width;
		*xinkey = key->bounds.xleft + key_size_left;
	    }
	}
	yl_ref = *yinkey - key_title_height - key_title_extra;

    }

    /* Center the key entries vertically, allowing for requested extra space */
    *yinkey -= (key->height_fix * t->v_char) / 2;

    /* Regardless of how the key was nominally positioned,
     * the result can be manually tweaked by "set key offset dx, dy"
     */
    map3d_position_r(&key->offset, &dx, &dy, "key");
    key->bounds.ytop += dy;
    key->bounds.ybot += dy;
    key->bounds.xleft += dx;
    key->bounds.xright += dx;
    *xinkey += dx;
    *yinkey += dy;
}


/*
 * Support routines for "set view map"
 */
static int splot_map_active = 0;
static float splot_map_surface_rot_x;
static float splot_map_surface_rot_z;
static float splot_map_surface_scale;

static void
flip_projection_axis(struct axis *axis)
{
    double temp = axis->min;
    axis->min = axis->max;
    axis->max = temp;
    if (axis->linked_to_primary) {
	axis = axis->linked_to_primary;
	temp = axis->min;
	axis->min = axis->max;
	axis->max = temp;
    }
}

void
splot_map_activate()
{
    if (splot_map_active)
	return;
    splot_map_active = 1;
    /* save current values */
    splot_map_surface_rot_x = surface_rot_x;
    splot_map_surface_rot_z = surface_rot_z ;
    splot_map_surface_scale = surface_scale;
    /* set new values */
    surface_rot_x = 180;
    surface_rot_z = 0;
    /* version 4 had constant value surface_scale = 1.3 */
    surface_scale = 1.425 * mapview_scale;
    /* The Y axis runs backwards from a normal 2D plot */
    flip_projection_axis(&axis_array[FIRST_Y_AXIS]);
}

void
splot_map_deactivate()
{
    if (!splot_map_active)
	return;
    splot_map_active = 0;
    /* restore the original values */
    surface_rot_x = splot_map_surface_rot_x;
    surface_rot_z = splot_map_surface_rot_z;
    surface_scale = splot_map_surface_scale;
    /* The Y axis runs backwards from a normal 2D plot */
    flip_projection_axis(&axis_array[FIRST_Y_AXIS]);
}

