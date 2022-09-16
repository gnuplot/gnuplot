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
#include "boundary.h"

#include "color.h"
#include "pm3d.h"
#include "plot.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "misc.h"
#include "gp_time.h"
#include "gadgets.h"
#include "jitter.h"
#include "plot2d.h"		/* for boxwidth */
#include "term_api.h"
#include "util.h"
#include "util3d.h"


/* Externally visible/modifiable status variables */

/* 'set offset' --- artificial buffer zone between coordinate axes and
 * the area actually covered by the data.
 * The retain_offsets flag is an interlock to prevent repeated application
 * of the offsets when a plot is refreshed or scrolled.
 */
t_position loff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
t_position roff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
t_position toff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
t_position boff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
TBOOLEAN retain_offsets = FALSE;

/* set bars */
double bar_size = 1.0;
int    bar_layer = LAYER_FRONT;
struct lp_style_type bar_lp;

/* 'set rgbmax {0|255}' */
double rgbmax = 255;

/* radius used to draw ttics and radial grid lines. */
/* NB: x-axis coordinates, not polar. updated by xtick2d_callback. */
static double largest_polar_circle;

/* End points and tickmark offsets for radial axes in spiderplots */
static double spoke_x0, spoke_y0, spoke_x1, spoke_y1;
static double spoke_dx, spoke_dy;

/* Used to prevent tic labels from being drawn more than once */
static int current_layer = 0;

/*}}} */

/* Status information for stacked histogram plots */
static struct coordinate *stackheight = NULL;	/* top of previous row */
static int stack_count;				/* points actually used */
static void place_histogram_titles(void);

/*{{{  static fns and local macros */
static void adjust_offsets(void);
static void adjust_nonlinear_offset(struct axis *axis);
static void recheck_ranges(struct curve_points * plot);
static void plot_border(void);
static void plot_impulses(struct curve_points * plot, int yaxis_x, int xaxis_y);
static void plot_lines(struct curve_points * plot);
static void plot_points(struct curve_points * plot);
static void plot_dots(struct curve_points * plot);
static void plot_bars(struct curve_points * plot);
static void plot_boxes(struct curve_points * plot, int xaxis_y);
static void plot_filledcurves(struct curve_points * plot);
static void finish_filled_curve(int, gpiPoint *, struct curve_points *);
static void plot_betweencurves(struct curve_points * plot);
static void plot_vectors(struct curve_points * plot);
static void plot_f_bars(struct curve_points * plot);
static void plot_c_bars(struct curve_points * plot);
static int compare_ypoints(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2);
static void plot_boxplot(struct curve_points * plot, TBOOLEAN only_autoscale);

static void place_labels(struct text_label * listhead, int layer, TBOOLEAN clip);
static void place_arrows(int layer);
static void place_grid(int layer);
static void place_raxis(void);
static void place_parallel_axes(struct curve_points *plots, int layer);
static void place_spiderplot_axes(struct curve_points *plots, int layer);

static void plot_steps(struct curve_points * plot);	/* JG */
static void plot_fsteps(struct curve_points * plot);	/* HOE */
static void plot_histeps(struct curve_points * plot);	/* CAC */

static void ytick2d_callback(struct axis *, double place, char *text, int ticlevel, struct lp_style_type grid, struct ticmark *userlabels);
static void xtick2d_callback(struct axis *, double place, char *text, int ticlevel, struct lp_style_type grid, struct ticmark *userlabels);
static void ttick_callback(struct axis *, double place, char *text, int ticlevel, struct lp_style_type grid, struct ticmark *userlabels);

static void spidertick_callback(struct axis *, double place, char *text, int ticlevel, struct lp_style_type grid, struct ticmark *userlabels);

static int histeps_compare(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);

static void get_arrow(struct arrow_def* arrow, double* sx, double* sy, double* ex, double* ey);
static void map_position_double(struct position* pos, double* x, double* y, const char* what);

static void plot_circles(struct curve_points *plot);
static void plot_ellipses(struct curve_points *plot);
static void do_rectangle(int dimensions, t_object *this_object, fill_style_type *fillstyle);
static void do_polygon(int dimensions, t_object *this_object, int style, int facing );

static double rgbscale(double rawvalue);

static void draw_polar_circle(double place);

static void plot_parallel(struct curve_points *plot);
static void plot_spiderplot(struct curve_points *plot);

/* for plotting error bars
 * half the width of error bar tic mark
 */
#define ERRORBARTIC GPMAX((t->h_tic/2),1)

/* used by compare_ypoints via q_sort from filter_boxplot */
static TBOOLEAN boxplot_factor_sort_required;

/* For tracking exit and re-entry of bounding curves that extend out of plot */
/* these must match the bit values returned by clip_point(). */
#define LEFT_EDGE	1
#define RIGHT_EDGE	2
#define BOTTOM_EDGE	4
#define TOP_EDGE	8

#define f_max(a,b) GPMAX((a),(b))
#define f_min(a,b) GPMIN((a),(b))

/* True if a and b have the same sign or zero (positive or negative) */
#define samesign(a,b) ((sgn(a) * sgn(b)) >= 0)
/*}}} */

static void
get_arrow(
    struct arrow_def *arrow,
    double* sx, double* sy,
    double* ex, double* ey)
{
    map_position_double(&arrow->start, sx, sy, "arrow");
    if (arrow->type == arrow_end_relative) {
	/* different coordinate systems:
	 * add the values in the drivers coordinate system.
	 * For log scale: relative coordinate is factor */
	map_position_r(&arrow->end, ex, ey, "arrow");
	*ex += *sx;
	*ey += *sy;
    } else if (arrow->type == arrow_end_oriented) {
	double aspect = (double)term->v_tic / (double)term->h_tic;
	double radius;

#ifdef _WIN32
	if (strcmp(term->name, "windows") == 0)
	    aspect = 1.;
#endif
	map_position_r(&arrow->end, &radius, NULL, "arrow");
	*ex = *sx + cos(DEG2RAD * arrow->angle) * radius;
	*ey = *sy + sin(DEG2RAD * arrow->angle) * radius * aspect;
    } else {
	map_position_double(&arrow->end, ex, ey, "arrow");
    }
}

static void
place_grid(int layer)
{
    struct termentry *t = term;
    int save_lgrid = grid_lp.l_type;
    int save_mgrid = mgrid_lp.l_type;
    BoundingBox *clip_save = clip_area;

    term_apply_lp_properties(&border_lp);	/* border linetype */
    largest_polar_circle = 0;

    /* This suppresses redrawing the grid lines */
    if (layer == LAYER_FOREGROUND)
	grid_lp.l_type = mgrid_lp.l_type = LT_NODRAW;

    if (TRUE) {
	/* select first mapping */
	x_axis = FIRST_X_AXIS;
	y_axis = FIRST_Y_AXIS;

	/* label first y axis tics */
	axis_output_tics(FIRST_Y_AXIS, &ytic_x, FIRST_X_AXIS, ytick2d_callback);
	/* label first x axis tics */
	axis_output_tics(FIRST_X_AXIS, &xtic_y, FIRST_Y_AXIS, xtick2d_callback);

	/* select second mapping */
	x_axis = SECOND_X_AXIS;
	y_axis = SECOND_Y_AXIS;

	axis_output_tics(SECOND_Y_AXIS, &y2tic_x, SECOND_X_AXIS, ytick2d_callback);
	axis_output_tics(SECOND_X_AXIS, &x2tic_y, SECOND_Y_AXIS, xtick2d_callback);
    }

    /* select first mapping */
    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;

    /* Sep 2018: polar grid is clipped to x/y range limits */
    clip_area = &plot_bounds;

    /* POLAR GRID circles */
    if (R_AXIS.ticmode && (raxis || polar)) {
	/* Piggyback on the xtick2d_callback.  Avoid a call to the full    */
	/* axis_output_tics(), which wasn't really designed for this axis. */
	tic_start = map_y(0);   /* Always equivalent to tics on theta=0 axis */
	tic_mirror = tic_start; /* tic extends on both sides of theta=0 */
	tic_text = tic_start - t->v_char;
	rotate_tics = R_AXIS.tic_rotate;
	if (rotate_tics == 0)
	    tic_hjust = CENTRE;
	else if ((*t->text_angle)(rotate_tics))
	    tic_hjust = (rotate_tics == TEXT_VERTICAL) ? RIGHT : LEFT;
	if (R_AXIS.manual_justify)
	    tic_hjust = R_AXIS.tic_pos;
	tic_direction = 1;
	gen_tics(&axis_array[POLAR_AXIS], xtick2d_callback);
	(*t->text_angle) (0);
    }

    /* POLAR GRID radial lines */
    if (polar_grid_angle > 0) {
	double theta = 0;
	int ox = map_x(0);
	int oy = map_y(0);
	term->layer(TERM_LAYER_BEGIN_GRID);
	term_apply_lp_properties(&grid_lp);
	if (largest_polar_circle <= 0)
	    largest_polar_circle = polar_radius(R_AXIS.max);
	for (theta = 0; theta < 6.29; theta += polar_grid_angle) {
	    int x = map_x(largest_polar_circle * cos(theta));
	    int y = map_y(largest_polar_circle * sin(theta));
	    draw_clip_line(ox, oy, x, y);
	}
	term->layer(TERM_LAYER_END_GRID);
    }

    /* POLAR GRID tickmarks along the perimeter of the outer circle */
    if (THETA_AXIS.ticmode) {
	term_apply_lp_properties(&border_lp);
	if (draw_border & 0x1000)
	    largest_polar_circle = polar_radius(R_AXIS.max);
	copy_or_invent_formatstring(&THETA_AXIS);
	gen_tics(&THETA_AXIS, ttick_callback);
	term->text_angle(0);
    }

    /* Restore the grid line types if we had turned them off to draw labels only */
    grid_lp.l_type = save_lgrid;
    mgrid_lp.l_type = save_mgrid;
    clip_area = clip_save;
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
	double dsx=0, dsy=0, dex=0, dey=0;

	if (this_arrow->arrow_properties.layer != layer)
	    continue;
	if (this_arrow->type == arrow_end_undefined)
	    continue;
	get_arrow(this_arrow, &dsx, &dsy, &dex, &dey);

	term_apply_lp_properties(&(this_arrow->arrow_properties.lp_properties));
	apply_head_properties(&(this_arrow->arrow_properties));
	draw_clip_arrow(dsx, dsy, dex, dey, this_arrow->arrow_properties.head);
    }
    term_apply_lp_properties(&border_lp);
    clip_area = clip_save;
}

/*
 * place_pixmaps() handles both 2D and 3D pixmaps
 * NOTE: implemented via term->image(), not individual pixels
 */
void
place_pixmaps(int layer, int dimensions)
{
    t_pixmap *pixmap;
    gpiPoint corner[4];
    int x, y, dx, dy;

    if (!term->image)
	return;

    for (pixmap = pixmap_listhead; pixmap; pixmap = pixmap->next) {
	if (layer != pixmap->layer)
	    continue;

	/* ignore zero-size pixmap from read failure */
	if (!pixmap->nrows || !pixmap->ncols)
	    continue;

	/* Allow a single backing pixmap behind multiple multiplot panels */
	if (layer == LAYER_BEHIND && multiplot_count > 1)
	    continue;

	if (dimensions == 3)
	    map3d_position(&pixmap->pin, &x, &y, "pixmap");
	else
	    map_position(&pixmap->pin, &x, &y, "pixmap");

	/* dx = dy = 0 means 1-to-1 representation of pixels */
	if (pixmap->extent.x == 0 && pixmap->extent.y == 0) {
	    dx = pixmap->ncols * term->tscale;
	    dy = pixmap->ncols * term->tscale;
	} else if (dimensions == 3) {
	    map3d_position_r(&pixmap->extent, &dx, &dy, "pixmap");
	    if (pixmap->extent.scalex == first_axes)
		dx = pixmap->extent.x * radius_scaler;
	    if (pixmap->extent.scaley == first_axes)
		dy = pixmap->extent.y * radius_scaler;
	} else {
	    double Dx, Dy;
	    map_position_r(&pixmap->extent, &Dx, &Dy, "pixmap");
	    dx = fabs(Dx);
	    dy = fabs(Dy);
	}

	/* default is to keep original aspect ratio */
	if (pixmap->extent.y == 0)
	    dy = dx * (double)(pixmap->nrows) / (double)(pixmap->ncols);
	if (pixmap->extent.x == 0)
	    dx = dy * (double)(pixmap->ncols) / (double)(pixmap->nrows);

	if (pixmap->center) {
	    x -= dx/2;
	    y -= dy/2;
	}

	corner[0].x = x;
	corner[0].y = y + dy;
	corner[1].x = x + dx;
	corner[1].y = y;
	corner[2].x = 0;		/* no clipping */
	corner[2].y = term->ymax;
	corner[3].x = term->xmax;
	corner[3].y = 0;

	term->image(pixmap->ncols, pixmap->nrows, pixmap->image_data, corner, IC_RGBA);
    }
}

/*
 * place_labels() handles both individual labels and 2D plot with labels
 */
static void
place_labels(struct text_label *listhead, int layer, TBOOLEAN clip)
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

    for (this_label = listhead; this_label != NULL; this_label = this_label->next) {

	if (this_label->layer != layer)
	    continue;

	if (layer == LAYER_PLOTLABELS) {
	    x = map_x(this_label->place.x);
	    y = map_y(this_label->place.y);
	} else
	    map_position(&this_label->place, &x, &y, "label");

	/* Trap undefined values from e.g. nonlinear axis mapping */
	if (invalid_coordinate(x,y))
	    continue;

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

void
place_objects(struct object *listhead, int layer, int dimensions)
{
    t_object *this_object;
    double x1, y1;
    int style;

    for (this_object = listhead; this_object != NULL; this_object = this_object->next) {
	struct lp_style_type lpstyle;
	struct fill_style_type *fillstyle;

	if (this_object->layer != layer && this_object->layer != LAYER_FRONTBACK)
	    continue;

	/* Extract line and fill style, but don't apply it yet */
	    lpstyle = this_object->lp_properties;

	if (this_object->fillstyle.fillstyle == FS_DEFAULT
	    && this_object->object_type == OBJ_RECTANGLE)
	    fillstyle = &default_rectangle.fillstyle;
	else
	    fillstyle = &this_object->fillstyle;
	style = style_from_fill(fillstyle);

	term_apply_lp_properties(&lpstyle);

	switch (this_object->object_type) {

	case OBJ_CIRCLE:
	{
	    t_circle *e = &this_object->o.circle;
	    double radius;
	    BoundingBox *clip_save = clip_area;

	    if (dimensions == 2) {
		map_position_double(&e->center, &x1, &y1, "object");
		map_position_r(&e->extent, &radius, NULL, "object");
	    } else if (splot_map) {
		int junkw, junkh;
		map3d_position_double(&e->center, &x1, &y1, "object");
		map3d_position_r(&e->extent, &junkw, &junkh, "object");
		radius = junkw;
	    } else /* General 3D splot */ {
		if (e->center.scalex == screen)
		    map_position_double(&e->center, &x1, &y1, "object");
		else if (e->center.scalex == first_axes || e->center.scalex == polar_axes)
		    map3d_position_double(&e->center, &x1, &y1, "object");
		else
		    break;
		/* radius must not change with rotation */
		if (e->extent.scalex == first_axes) {
		    radius = e->extent.x * radius_scaler;
		} else {
		    map_position_r(&e->extent, &radius, NULL, "object");
		}
	    }

	    if ((e->center.scalex == screen || e->center.scaley == screen)
	    ||  (this_object->clip == OBJ_NOCLIP))
	    	clip_area = &canvas;

	    do_arc((int)x1, (int)y1, radius, e->arc_begin, e->arc_end, style, FALSE);

	    /* Retrace the border if the style requests it */
	    if (need_fill_border(fillstyle))
		do_arc((int)x1, (int)y1, radius, e->arc_begin, e->arc_end, 0, e->wedge);

	    clip_area = clip_save;
	    break;
	}

	case OBJ_ELLIPSE:
	{
	    t_ellipse *e = &this_object->o.ellipse;
	    BoundingBox *clip_save = clip_area;

	    if ((e->center.scalex == screen || e->center.scaley == screen)
	    ||  (this_object->clip == OBJ_NOCLIP))
	    	clip_area = &canvas;

	    if (dimensions == 2)
		do_ellipse(2, e, style, TRUE);
	    else if (splot_map)
		do_ellipse(3, e, style, TRUE);
	    else
		break;

	    /* Retrace the border if the style requests it */
	    if (need_fill_border(fillstyle))
		do_ellipse(dimensions, e, 0, TRUE);

	    clip_area = clip_save;
	    break;
	}

	case OBJ_POLYGON:
	{
	    /* Polygons have an extra option LAYER_FRONTBACK that matches
	     * FRONT or BACK depending on which way the polygon faces
	     */
	    int facing = LAYER_BEHIND;	/* This will be ignored */
	    if (this_object->layer == LAYER_FRONTBACK) {
		if ((layer == LAYER_FRONT) || (layer == LAYER_BACK))
		    facing = layer;
		else
		    break;
	    }

	    do_polygon(dimensions, this_object, style, facing);

	    /* Retrace the border if the style requests it */
	    if (this_object->layer != LAYER_DEPTHORDER)
		if (need_fill_border(fillstyle))
		    do_polygon(dimensions, this_object, 0, facing);

	    break;
	}

	case OBJ_RECTANGLE:
	{
	    do_rectangle(dimensions, this_object, fillstyle);
	    break;
	}

	default:
	    break;
	} /* End switch(object_type) */


    }
}

/*
 * Apply axis range expansions from "set offsets" command
 */
static void
adjust_offsets(void)
{
    double b = boff.scaley == graph ? fabs(Y_AXIS.max - Y_AXIS.min)*boff.y : boff.y;
    double t = toff.scaley == graph ? fabs(Y_AXIS.max - Y_AXIS.min)*toff.y : toff.y;
    double l = loff.scalex == graph ? fabs(X_AXIS.max - X_AXIS.min)*loff.x : loff.x;
    double r = roff.scalex == graph ? fabs(X_AXIS.max - X_AXIS.min)*roff.x : roff.x;

    if (retain_offsets) {
	retain_offsets = FALSE;
	return;
    }

    if ((Y_AXIS.autoscale & AUTOSCALE_BOTH) != AUTOSCALE_NONE) {
	if (nonlinear(&Y_AXIS)) {
	    adjust_nonlinear_offset(&Y_AXIS);
	} else {
	    if (Y_AXIS.min < Y_AXIS.max) {
		Y_AXIS.min -= b;
		Y_AXIS.max += t;
	    } else {
		Y_AXIS.max -= b;
		Y_AXIS.min += t;
	    }
	}
    }

    if ((X_AXIS.autoscale & AUTOSCALE_BOTH) != AUTOSCALE_NONE) {
	if (nonlinear(&X_AXIS)) {
	    adjust_nonlinear_offset(&X_AXIS);
	} else {
	    if (X_AXIS.min < X_AXIS.max) {
		X_AXIS.min -= l;
		X_AXIS.max += r;
	    } else {
		X_AXIS.max -= l;
		X_AXIS.min += r;
	    }
	}
    }

    if (X_AXIS.min == X_AXIS.max)
	int_error(NO_CARET, "x_min should not equal x_max!");
    if (Y_AXIS.min == Y_AXIS.max)
	int_error(NO_CARET, "y_min should not equal y_max!");

    if (axis_array[FIRST_X_AXIS].linked_to_secondary)
	clone_linked_axes(&axis_array[FIRST_X_AXIS], &axis_array[SECOND_X_AXIS]);
    if (axis_array[FIRST_Y_AXIS].linked_to_secondary)
	clone_linked_axes(&axis_array[FIRST_Y_AXIS], &axis_array[SECOND_Y_AXIS]);
}

/*
 * This routine is called only if we know the axis passed in is either
 * nonlinear X or nonlinear Y.  We apply the offsets to the primary (linear)
 * end of the linkage and then transform back to the axis itself (seconary).
 */
static void
adjust_nonlinear_offset( struct axis *secondary)
{
    struct axis *primary = secondary->linked_to_primary;
    double range = fabs(primary->max - primary->min);
    double offset1, offset2;

    if (secondary->index == FIRST_X_AXIS) {
	if ((loff.scalex != graph && loff.x != 0)
	||  (roff.scalex != graph && roff.x != 0))
	    int_error(NO_CARET, "nonlinear axis offsets must be in graph units");
	offset1 = loff.x;
	offset2 = roff.x;
    } else {
	if ((boff.scaley != graph && boff.y != 0)
	||  (toff.scaley != graph && toff.y != 0))
	    int_error(NO_CARET, "nonlinear axis offsets must be in graph units");
	offset1 = boff.y;
	offset2 = toff.y;
    }

    primary->min -= range * offset1;
    primary->max += range * offset2;
    secondary->min = eval_link_function(secondary, primary->min);
    secondary->max = eval_link_function(secondary, primary->max); 
}

void
do_plot(struct curve_points *plots, int pcount)
{
    struct termentry *t = term;
    int curve;
    struct curve_points *this_plot = NULL;
    TBOOLEAN key_pass = FALSE;
    legend_key *key = &keyT;
    int previous_plot_style;

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

    /* Give a chance for background items to be behind everything else */
    current_layer = LAYER_BEHIND;
    place_pixmaps(LAYER_BEHIND, 2);
    place_objects( first_object, LAYER_BEHIND, 2);

    screen_ok = FALSE;

    /* Sync point for epslatex text positioning */
    (term->layer)(TERM_LAYER_BACKTEXT);

    /* DRAW TICS AND GRID */
    current_layer = LAYER_BACK;
    if (grid_layer == LAYER_BACK || grid_layer == LAYER_BEHIND)
	place_grid(grid_layer);

    /* DRAW ZERO AXES and update axis->term_zero */
    axis_draw_2d_zeroaxis(FIRST_X_AXIS,FIRST_Y_AXIS);
    axis_draw_2d_zeroaxis(FIRST_Y_AXIS,FIRST_X_AXIS);
    axis_draw_2d_zeroaxis(SECOND_X_AXIS,SECOND_Y_AXIS);
    axis_draw_2d_zeroaxis(SECOND_Y_AXIS,SECOND_X_AXIS);

    /* DRAW VERTICAL AXES OF PARALLEL AXIS PLOTS */
    place_parallel_axes(plots, LAYER_BACK);

    /* DRAW RADIAL AXES OF SPIDERPLOTS */
    place_spiderplot_axes(plots, LAYER_BACK);

    /* DRAW PLOT BORDER */
    if (draw_border)
	plot_border();

    /* Add back colorbox if appropriate */
    if (is_plot_with_colorbox() && color_box.layer == LAYER_BACK)
	    draw_color_smooth_box(MODE_PLOT);

    /* Pixmaps before objects */
    place_pixmaps(LAYER_BACK, 2);

    /* Fixed objects */
    place_objects( first_object, LAYER_BACK, 2);

    /* PLACE LABELS */
    place_labels( first_label, LAYER_BACK, FALSE );

    /* PLACE ARROWS */
    place_arrows( LAYER_BACK );

    /* Sync point for epslatex text positioning */
    (term->layer)(TERM_LAYER_FRONTTEXT);

    /* Draw axis labels and timestamps */
    /* Note: As of Dec 2012 these are drawn as "front" text. */
    draw_titles();

    /* Draw the key, or at least reserve space for it (pass 1) */
    if (key->visible)
	draw_key( key, key_pass);
    SECOND_KEY_PASS:
	/* This tells the canvas, qt, and svg terminals to restart the plot   */
	/* count so that key titles are in sync with the plots they describe. */
	(*t->layer)(TERM_LAYER_RESET_PLOTNO);

    /* DRAW CURVES */
    this_plot = plots;
    previous_plot_style = 0;
    for (curve = 0; curve < pcount; this_plot = this_plot->next, curve++) {

	TBOOLEAN localkey = key->visible;	/* a local copy */
	this_plot->current_plotno = curve;

	/* Sync point for start of new curve (used by svg, post, ...) */
	if (term->hypertext) {
	    char *plaintext;
	    if (this_plot->title_no_enhanced)
		plaintext = this_plot->title;
	    else
		plaintext = estimate_plaintext(this_plot->title);
	    (term->hypertext)(TERM_HYPERTEXT_TITLE, plaintext);
	}
	(term->layer)(TERM_LAYER_BEFORE_PLOT);

	/* set scaling for this plot's axes */
	x_axis = this_plot->x_axis;
	y_axis = this_plot->y_axis;

	/* Crazy corner case handling Bug #3499425 */
	if (this_plot->plot_style == HISTOGRAMS)
	    if ((!key_pass && key->front) &&  (prefer_line_styles)) {
		struct lp_style_type ls;
		lp_use_properties(&ls, this_plot->lp_properties.l_type+1);
		this_plot->lp_properties.pm3d_color = ls.pm3d_color;
	    }

	term_apply_lp_properties(&(this_plot->lp_properties));

	/* Skip a line in the key between histogram clusters */
	if (this_plot->plot_style == HISTOGRAMS
	&&  previous_plot_style == HISTOGRAMS
	&&  this_plot->histogram_sequence == 0
	&&  this_plot->histogram->keyentry
	&& !at_left_of_key()) {
	    key_count++;
	    advance_key(TRUE);	/* correct for inverted key */
	    advance_key(0);
	}

	/* Column-stacked histograms store their key titles internally */
	if (this_plot->plot_style == HISTOGRAMS
	&&  histogram_opts.type == HT_STACKED_IN_TOWERS) {
	    text_label *key_entry;
	    localkey = 0;
	    if (this_plot->labels && (key_pass || !key->front)) {
		struct lp_style_type save_lp = this_plot->lp_properties;
		for (key_entry = this_plot->labels->next; key_entry;
		     key_entry = key_entry->next) {
		    int histogram_linetype = key_entry->tag + this_plot->histogram->startcolor;
		    this_plot->lp_properties.l_type = histogram_linetype;
		    this_plot->fill_properties.fillpattern = histogram_linetype;
		    if (key_entry->text) {
			if (prefer_line_styles)
			    lp_use_properties(&this_plot->lp_properties, histogram_linetype);
			else
			    load_linetype(&this_plot->lp_properties, histogram_linetype);
			do_key_sample(this_plot, key, key_entry->text, 0.0);
		    }
		    key_count++;
		    advance_key(0);
		}
		free_labels(this_plot->labels);
		this_plot->labels = NULL;
		this_plot->lp_properties = save_lp;
	    }

	/* Parallel plot titles are placed as xtic labels */
	} else if (this_plot->plot_style == PARALLELPLOT) {
	    localkey = FALSE;

	/* Spiderplot key samples are handled in plot_spiderplot */
	} else if (this_plot->plot_style == SPIDERPLOT && !(this_plot->plot_type == KEYENTRY)) {
	    localkey = FALSE;
	} else if (this_plot->title && !*this_plot->title) {
	    localkey = FALSE;
	} else if (this_plot->plot_type == NODATA) {
	    localkey = FALSE;
	} else if (key_pass || !key->front) {
	    ignore_enhanced(this_plot->title_no_enhanced);
		/* don't write filename or function enhanced */
	    if (localkey && this_plot->title && !this_plot->title_is_suppressed) {
		/* If title is "at {end|beg}" do not draw it in the key */
		if (!this_plot->title_position
		||  this_plot->title_position->scalex != character) {
		    coordval var_color;
		    key_count++;
		    advance_key(TRUE);	/* invert only */
		    var_color = (this_plot->varcolor) ? this_plot->varcolor[0] : 0.0;
		    do_key_sample(this_plot, key, this_plot->title, var_color);
		}
	    }
	    ignore_enhanced(FALSE);
	}

	/* If any plots have opted out of autoscaling, we need to recheck */
	/* whether their points are INRANGE or not.                       */
	if (this_plot->noautoscale  &&  !key_pass)
	    recheck_ranges(this_plot);

	/* and now the curves, plus any special key requirements */
	/* be sure to draw all lines before drawing any points */
	/* Skip missing/empty curves */
	if (this_plot->plot_type != NODATA  &&  !key_pass) {

	    switch (this_plot->plot_style) {
	    case IMPULSES:
		plot_impulses(this_plot, X_AXIS.term_zero, Y_AXIS.term_zero);
		break;
	    case LINES:
		plot_lines(this_plot);
		break;
	    case STEPS:
	    case FILLSTEPS:
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
		    /* Note that the bar linewidth may not match the border or plot linewidth */
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
	    case POLYGONS:
		if (this_plot->filledcurves_options.closeto == FILLEDCURVES_DEFAULT) {
		    if (this_plot->plot_type == DATA)
			memcpy(&this_plot->filledcurves_options,
				 &filledcurves_opts_data, sizeof(filledcurves_opts));
		    else
			memcpy(&this_plot->filledcurves_options,
				&filledcurves_opts_func, sizeof(filledcurves_opts));
		}
		if (this_plot->filledcurves_options.closeto == FILLEDCURVES_BETWEEN
		||  this_plot->filledcurves_options.closeto == FILLEDCURVES_ABOVE
		||  this_plot->filledcurves_options.closeto == FILLEDCURVES_BELOW) {
		    plot_betweencurves(this_plot);
		} else if (!this_plot->plot_smooth &&
		   (this_plot->filledcurves_options.closeto == FILLEDCURVES_ATY1
		||  this_plot->filledcurves_options.closeto == FILLEDCURVES_ATY2
		||  this_plot->filledcurves_options.closeto == FILLEDCURVES_ATR)) {
		    /* Smoothing may have trashed the original contents	*/
		    /* of the 2nd y data column, so piggybacking on the	*/
		    /* code for FILLEDCURVES_BETWEEN will not work.	*/
		    /* FIXME: Maybe piggybacking is always a bad idea?		*/
		    /* IIRC the original rationale was to get better clipping	*/
		    /* but the general polygon clipping code should now work.	*/
		    plot_betweencurves(this_plot);
		} else {
		    plot_filledcurves(this_plot);
		}
		break;

	    case VECTOR:
	    case ARROWS:
		plot_vectors(this_plot);
		break;
	    case FINANCEBARS:
		plot_f_bars(this_plot);
		break;
	    case CANDLESTICKS:
		plot_c_bars(this_plot);
		break;

	    case BOXPLOT:
		plot_boxplot(this_plot, FALSE);
		break;

	    case PM3DSURFACE:
	    case SURFACEGRID:
		int_warn(NO_CARET, "Can't use pm3d or surface for 2d plots");
		break;

	    case LABELPOINTS:
		place_labels( this_plot->labels->next, LAYER_PLOTLABELS, TRUE);
		break;

	    case IMAGE:
		this_plot->image_properties.type = IC_PALETTE;
		process_image(this_plot, IMG_PLOT);
		break;

	    case RGBIMAGE:
		this_plot->image_properties.type = IC_RGB;
		process_image(this_plot, IMG_PLOT);
		break;

	    case RGBA_IMAGE:
		this_plot->image_properties.type = IC_RGBA;
		process_image(this_plot, IMG_PLOT);
		break;

	    case CIRCLES:
		plot_circles(this_plot);
		break;

	    case ELLIPSES:
		plot_ellipses(this_plot);
		break;

	    case PARALLELPLOT:
		plot_parallel(this_plot);
		break;

	    case SPIDERPLOT:
		plot_spiderplot(this_plot);
		break;

	    default:
		int_error(NO_CARET, "unknown plot style");
	    }
	}


	/* If there are two passes, defer key sample till the second */
	/* KEY SAMPLES */
	if (key->front && !key_pass)
	    ;
	else if (localkey && this_plot->title && !this_plot->title_is_suppressed) {
	    /* we deferred point sample until now */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		do_key_sample_point(this_plot, key);

	    if (this_plot->plot_style == LABELPOINTS)
		do_key_sample_point(this_plot, key);

	    if (this_plot->plot_style == DOTS)
		do_key_sample_point(this_plot, key);

	    if (!this_plot->title_position)
		advance_key(0);
	}

	/* Option to label the end of the curve on the plot itself */
	if (this_plot->title_position && this_plot->title_position->scalex == character)
	    attach_title_to_plot(this_plot, key);

	/* Sync point for end of this curve (used by svg, post, ...) */
	(term->layer)(TERM_LAYER_AFTER_PLOT);
	previous_plot_style = this_plot->plot_style;

    }

    /* Go back and draw the legend in a separate pass if necessary */
    if (key->visible && key->front && !key_pass) {
	key_pass = TRUE;
	draw_key( key, key_pass);
	goto SECOND_KEY_PASS;
    }

    /* DRAW TICS AND GRID */
    current_layer = LAYER_FRONT;
    if (grid_layer == LAYER_FRONT)
	place_grid(LAYER_FRONT);

    if (raxis)
	place_raxis();

    /* Redraw the axis tic labels and tic marks if "set tics front" */
    if (grid_tics_in_front) {
	current_layer = LAYER_FOREGROUND;
	place_grid(LAYER_FOREGROUND);
    }

    /* DRAW ZERO AXES */
    /* redraw after grid so that axes linetypes are on top */
    if (grid_layer == LAYER_FRONT) {
	axis_draw_2d_zeroaxis(FIRST_X_AXIS,FIRST_Y_AXIS);
	axis_draw_2d_zeroaxis(FIRST_Y_AXIS,FIRST_X_AXIS);
	axis_draw_2d_zeroaxis(SECOND_X_AXIS,SECOND_Y_AXIS);
	axis_draw_2d_zeroaxis(SECOND_Y_AXIS,SECOND_X_AXIS);
    }

    /* DRAW VERTICAL AXES OF PARALLEL AXIS PLOTS */
    if (parallel_axis_style.layer == LAYER_FRONT)
	place_parallel_axes(plots, LAYER_FRONT);

    /* DRAW RADIAL AXES OF SPIDERPLOTS */
    if (parallel_axis_style.layer == LAYER_FRONT)
	place_spiderplot_axes(plots, LAYER_FRONT);

    /* REDRAW PLOT BORDER */
    if (draw_border && border_layer == LAYER_FRONT)
	plot_border();

    /* Add front colorbox if appropriate */
    if (is_plot_with_colorbox() && color_box.layer == LAYER_FRONT)
	    draw_color_smooth_box(MODE_PLOT);

    /* pixmaps in behind rectangles to enable rectangle as border */
    place_pixmaps( LAYER_FRONT, 2);

    /* And rectangles */
    place_objects( first_object, LAYER_FRONT, 2);

    /* PLACE LABELS */
    place_labels( first_label, LAYER_FRONT, FALSE );

    /* PLACE HISTOGRAM TITLES */
    place_histogram_titles();

    /* PLACE ARROWS */
    place_arrows( LAYER_FRONT );

    /* PLACE TITLE LAST */
    place_title(title_x, title_y);

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
	if (plot->noautoscale && plot->points[i].type != UNDEFINED) {
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
 * Mar 2017 - Apply "set jitter" to x coordinate of impulses
 */

static void
plot_impulses(struct curve_points *plot, int yaxis_x, int xaxis_y)
{
    int i;
    int x, y;

    /* Displace overlapping impulses if "set jitter" is in effect.
     * This operation loads jitter offsets into xhigh and yhigh.
     */
    if (jitter.spread > 0)
	jitter_points(plot);

    for (i = 0; i < plot->p_count; i++) {

	if (plot->points[i].type == UNDEFINED)
	    continue;

	if (!polar && !inrange(plot->points[i].x, X_AXIS.min, X_AXIS.max))
	    continue;

	/* This catches points that are outside trange[theta_min:theta_max] */
	if (polar && (plot->points[i].type == EXCLUDEDRANGE))
	    continue;

	x = map_x(plot->points[i].x);
	y = map_y(plot->points[i].y);

	/* The jitter x offset is a scaled multiple of character width. */
	if (!polar && jitter.spread > 0)
	    x += plot->points[i].CRD_XJITTER * 0.3 * term->h_char;

	if (invalid_coordinate(x,y))
	    continue;

	check_for_variable_color(plot, &plot->varcolor[i]);

	if (polar)
	    draw_clip_line(yaxis_x, xaxis_y, x, y);
	else
	    draw_clip_line(x, xaxis_y, x, y);

    }
}

/* plot_lines:
 * Plot the curves in LINES style
 */
static void
plot_lines(struct curve_points *plot)
{
    int i;			/* point index */
    int x=0, y=0;		/* current point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double xprev = 0.0;
    double yprev = 0.0;
    double xnow, ynow;

    /* If all the lines are invisible, don't bother to draw them */
    if (plot->lp_properties.l_type == LT_NODRAW)
	return;

    for (i = 0; i < plot->p_count; i++) {
	xnow = plot->points[i].x;
	ynow = plot->points[i].y;

	/* rgb variable  -  color read from data column */
	check_for_variable_color(plot, &plot->varcolor[i]);

	/* Only map and plot the point if it is well-behaved (not UNDEFINED).
	 * Note that map_x or map_y can hit NaN during eval_link_function(),
	 * in which case the coordinate value is garbage so we set UNDEFINED.
	 */
	if (plot->points[i].type != UNDEFINED) {
	    x = map_x(xnow);
	    y = map_y(ynow);
	    if (invalid_coordinate(x,y))
		plot->points[i].type = UNDEFINED;
	}

	switch (plot->points[i].type) {
	case INRANGE:
		if (prev == INRANGE) {
		    (*t->vector) (x, y);
		} else if (prev == OUTRANGE) {
		    /* from outrange to inrange */
		    if (!clip_lines1) {
			(*t->move) (x, y);
		    } else if (polar && clip_radial) {
			draw_polar_clip_line(xprev, yprev, xnow, ynow );
		    } else {
			if (!draw_clip_line( map_x(xprev), map_y(yprev), x, y))
			    /* This is needed if clip_line() doesn't agree that	*/
			    /* the current point is INRANGE, i.e. it is on the	*/
			    /*  border or just outside depending on rounding.	*/
			    (*t->move)(x, y);
		    }
		} else {	/* prev == UNDEFINED */
		    (*t->move) (x, y);
		    (*t->vector) (x, y);
		}
		break;

	case OUTRANGE:
		if (prev == INRANGE) {
		    /* from inrange to outrange */
		    if (clip_lines1) {
			if (polar && clip_radial)
			    draw_polar_clip_line(xprev, yprev, xnow, ynow );
			else
			    draw_clip_line( map_x(xprev), map_y(yprev), x, y);
		    }
		} else if (prev == OUTRANGE) {
		    /* from outrange to outrange */
		    if (clip_lines2) {
			if (polar && clip_radial)
			    draw_polar_clip_line(xprev, yprev, xnow, ynow );
			else
			    draw_clip_line( map_x(xprev), map_y(yprev), x, y);
		    }
		}
		break;

	case UNDEFINED:
	default:		/* just a safety */
		break;
	}
	prev = plot->points[i].type;
	xprev = xnow;
	yprev = ynow;
    }
}

/* plot_filledcurves:
 *        {closed | {above | below} {x1 | x2 | y1 | y2 | r}[=<a>] | xy=<x>,<y>}
 */

/* finalize and draw the filled curve */
static void
finish_filled_curve(
    int points,
    gpiPoint *corners,
    struct curve_points *plot)
{
    static gpiPoint *clipcorners = NULL;
    int clippoints;
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
	case FILLEDCURVES_ATX2:
		corners[points].x   =
		corners[points+1].x = map_x(filledcurves_options->at);
		    /* should be mapping real x1/x2axis/graph/screen => screen */
		corners[points].y   = corners[points-1].y;
		corners[points+1].y = corners[0].y;
		for (i=0; i<points; i++)
		    side += corners[i].x - corners[points].x;
		points += 2;
		break;
	case FILLEDCURVES_ATXY:
		corners[points].x = map_x(filledcurves_options->at);
		    /* should be mapping real x1axis/graph/screen => screen */
		corners[points].y = map_y(filledcurves_options->aty);
		    /* should be mapping real y1axis/graph/screen => screen */
		points++;
		break;
	case FILLEDCURVES_ATY1:
	case FILLEDCURVES_ATY2:
		corners[points].y = map_y(filledcurves_options->at);
		corners[points+1].y = corners[points].y;
		corners[points].x = corners[points-1].x;
		corners[points+1].x = corners[0].x;
		points += 2;
		/* Fall through */
	case FILLEDCURVES_BETWEEN:
		/* fill_between() allocated an extra point for the above/below flag */
		if (filledcurves_options->closeto == FILLEDCURVES_BETWEEN)
		    side = (corners[points].x > 0) ? 1 : -1;
		/* Fall through */
	case FILLEDCURVES_ATR:
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

    /* Check for request to fill only on one side of a bounding line */
    if (filledcurves_options->oneside > 0 && side < 0)
	return;
    if (filledcurves_options->oneside < 0 && side > 0)
	return;

    /* The polygon clipping code does not deal well with 1- or 2- vertex "polygons" */
    if (points < 3)
	return;
    clipcorners = gp_realloc(clipcorners, 2*points*sizeof(gpiPoint), "filledcurve vertices");
    clip_polygon(corners, clipcorners, points, &clippoints);
    clipcorners->style = style_from_fill(&plot->fill_properties);
    if (clippoints > 0)
	term->filled_polygon(clippoints, clipcorners);
}


static void
plot_filledcurves(struct curve_points *plot)
{
    int i;			/* point index */
    int x, y;			/* point in terminal coordinates */
    struct termentry *t = term;
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    int points = 0;			/* how many corners */
    static gpiPoint *corners = 0;	/* array of corners */
    static int corners_allocated = 0;	/* how many allocated */

    if (!t->filled_polygon) { /* filled polygons are not available */
	plot_lines(plot);
	return;
    }

    /* clip the "at" coordinate to the drawing area */
    switch (plot->filledcurves_options.closeto) {
	case FILLEDCURVES_ATX1:
	    cliptorange(plot->filledcurves_options.at,
			axis_array[FIRST_X_AXIS].min, axis_array[FIRST_X_AXIS].max);
	    break;
	case FILLEDCURVES_ATX2:
	    cliptorange(plot->filledcurves_options.at,
			axis_array[SECOND_X_AXIS].min, axis_array[SECOND_X_AXIS].max);
	    break;
	case FILLEDCURVES_ATY1:
	case FILLEDCURVES_ATY2:
	    cliptorange(plot->filledcurves_options.at,
			axis_array[plot->y_axis].min, axis_array[plot->y_axis].max);
	    break;
	case FILLEDCURVES_ATXY:
	    cliptorange(plot->filledcurves_options.at,
			axis_array[FIRST_X_AXIS].min, axis_array[FIRST_X_AXIS].max);
	    cliptorange(plot->filledcurves_options.aty,
			axis_array[FIRST_Y_AXIS].min, axis_array[FIRST_Y_AXIS].max);
	    break;
	default:
	    break;
    }

    for (i = 0; i < plot->p_count; i++) {
	if (points+2 >= corners_allocated) { /* there are 2 side points */
	    corners_allocated += 128; /* reallocate more corners */
	    corners = gp_realloc( corners, corners_allocated*sizeof(gpiPoint), "filledcurve vertices");
	}
	switch (plot->points[i].type) {
	case INRANGE:
	case OUTRANGE:
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);
		corners[points].x = x;
		corners[points].y = y;
		if (points == 0)
		    check_for_variable_color(plot, &plot->varcolor[i]);
		points++;
		break;
	case UNDEFINED:
		/* UNDEFINED flags a blank line in the input file.
		 * Unfortunately, it can also mean that the point was undefined.
		 * Is there a clean way to detect or handle the latter case?
		 */
		if (prev != UNDEFINED) {
		    finish_filled_curve(points, corners, plot);
		    points = 0;
		}
		break;
	default:		/* just a safety */
		break;
	}
	prev = plot->points[i].type;
    }

    finish_filled_curve(points, corners, plot);

    /* If the fill style has a border and this is a closed curve then	*/
    /* retrace the boundary.  Otherwise ignore "border" property.	*/
    if (plot->filledcurves_options.closeto == FILLEDCURVES_CLOSED
    &&  need_fill_border(&plot->fill_properties)) {
	plot_lines(plot);
    }
}

/*
 * Fill the area between two curves
 */
static void
plot_betweencurves(struct curve_points *plot)
{
    double x1, x2, yl1, yu1, yl2, yu2, dy;
    double xmid=0, ymid=0;
    double xu1, xu2;	/* For polar plots */
    int i, j, istart=0, finish=0, points=0, max_corners_needed;
    static gpiPoint *corners = 0;
    static int corners_allocated = 0;

    /* If terminal doesn't support filled polygons, approximate with bars */
    if (!term->filled_polygon) {
	plot_bars(plot);
	return;
    }

    /* Jan 2015: We are now using the plot_between code to also handle option
     * y=atval, but the style option in the plot header does not reflect this.
     * Change it here so that finish_filled_curve() doesn't get confused.
     */
    plot->filledcurves_options.closeto = FILLEDCURVES_BETWEEN;

    /* there are possibly 2 side points plus one extra to specify above/below */
    max_corners_needed = plot->p_count * 2 + 3;
    if (max_corners_needed > corners_allocated) {
	corners_allocated = max_corners_needed;
	corners = gp_realloc(corners, corners_allocated*sizeof(gpiPoint), "betweencurves vertices");
    }
    /*
     * Form a polygon, first forward along the lower points
     *    and then backward along the upper ones.
     * Check each interval to see if the curves cross.
     * If so, split the polygon into multiple parts.
     */
    for (i = 0; i < plot->p_count; i++) {

	/* This isn't really testing for undefined points, it is looking */
	/* for blank lines. If there is one then start a new fill area.  */
	if (plot->points[i].type == UNDEFINED)
	    continue;

	if (points == 0) {
	    istart=i;
	    dy=0.0;
	}

	if (finish == 2) { /* start the polygon at the previously-found crossing */
 	    corners[points].x = map_x(xmid);
	    corners[points].y = map_y(ymid);
	    points++;
	}

	x1  = plot->points[i].x;
	xu1 = plot->points[i].xhigh;
	yl1 = plot->points[i].y;
	yu1 = plot->points[i].yhigh;
	if (i+1 >= plot->p_count || plot->points[i+1].type == UNDEFINED)
	    finish=1;
	else {
	    finish=0;
	    x2  = plot->points[i+1].x;
	    xu2 = plot->points[i+1].xhigh;
	    yl2 = plot->points[i+1].y;
	    yu2 = plot->points[i+1].yhigh;
	}

	corners[points].x = map_x(x1);
	corners[points].y = map_y(yl1);
	points++;

	if (polar) {
	    double ox = map_x(0);
	    double oy = map_y(0);
	    double plx = map_x(plot->points[istart].x);
	    double ply = map_y(plot->points[istart].y);
	    double pux = map_x(plot->points[istart].xhigh);
	    double puy = map_y(plot->points[istart].yhigh);
	    double drl = (plx-ox)*(plx-ox) + (ply-oy)*(ply-oy);
	    double dru = (pux-ox)*(pux-ox) + (puy-oy)*(puy-oy);

	    dy += dru-drl;
	} else {
	    dy += yu1-yl1;
	}

	if (!finish) {
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

		if ((x1-xmid)*(xmid-x2) > 0)
		    finish=2;
	    } else if ((yu1-yl1) == 0 && (yu2-yl2) == 0) {
		/* nothing */
	    } else if ((yu1-yl1)*(yu2-yl2) <= 0) {
		/* Cheap test for intersection in the general case */
		xmid = (x1*(yl2-yu2) + x2*(yu1-yl1))
		     / ((yu1-yl1) + (yl2-yu2));
		ymid = yu1 + (yu2-yu1)*(xmid-x1)/(x2-x1);

		finish=2;
	    }
	}

	if (finish == 2) { /* curves cross */
	    corners[points].x = map_x(xmid);
	    corners[points].y = map_y(ymid);
	    points++;
	}

	if (finish) {
	    for (j = i; j >= istart; j--) {
		corners[points].x = map_x(plot->points[j].xhigh);
		corners[points].y = map_y(plot->points[j].yhigh);
		points++;
	    }

	    corners[points].x = (dy < 0) ? 1 : 0;

	    finish_filled_curve(points, corners, plot);
	    points=0;
	}

    }
}


/* plot_steps:
 * Plot the curves in STEPS or FILLSTEPS style
 * Each new value is reached by tracing horizontally to the new x value
 * and then up/down to the new y value.
 */
static void
plot_steps(struct curve_points *plot)
{
    struct termentry *t = term;
    int i;				/* point index */
    int x=0, y=0;			/* point in terminal coordinates */
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    int xprev, yprev;			/* previous point coordinates */
    int xleft, xright, ytop, ybot;	/* plot limits in terminal coords */
    int y0=0;				/* baseline */
    int style = 0;
    int oneside = plot->filledcurves_options.oneside;	/* above/below */

    /* EAM April 2011:  Default to lines only, but allow filled boxes */
    if ((plot->plot_style & PLOT_STYLE_HAS_FILL) && t->fillbox) {
	double ey = 0;
	style = style_from_fill(&plot->fill_properties);
	if (plot->filledcurves_options.closeto == FILLEDCURVES_ATY1)
	    ey = plot->filledcurves_options.at;
	else if (Y_AXIS.log)
	    ey = Y_AXIS.min;
	else
	    cliptorange(ey, Y_AXIS.min, Y_AXIS.max);
	y0 = map_y(ey);
    }

    xleft = map_x(X_AXIS.min);
    xright = map_x(X_AXIS.max);
    ybot = map_y(Y_AXIS.min);
    ytop = map_y(Y_AXIS.max);

    for (i = 0; i < plot->p_count; i++) {
	xprev = x; yprev = y;

	switch (plot->points[i].type) {
	case INRANGE:
	case OUTRANGE:
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);

		if (prev == UNDEFINED || invalid_coordinate(x,y))
		    break;
		if (style) {
		    /* We don't yet have a generalized draw_clip_rectangle routine */
		    int xl = xprev;
		    int xr = x;

		    cliptorange(xr, xleft, xright);
		    cliptorange(xl, xleft, xright);
		    cliptorange(y, ybot, ytop);
		    cliptorange(yprev, ybot, ytop);

		    /* Entire box is out of range on x */
		    if (xr == xl && (xr == xleft || xr == xright))
			break;

		    /* Some terminals fail to completely color the join between boxes */
		    if (style == FS_OPAQUE && oneside == 0)
			draw_clip_line(xl, yprev, xl, y0);

		    if ((yprev - y0 < 0) && (oneside <= 0))
			(*t->fillbox)(style, xl, yprev, (xr-xl), y0-yprev);
		    if ((yprev - y0 >= 0) && (oneside >= 0))
			(*t->fillbox)(style, xl, y0, (xr-xl), yprev-y0);
		} else {
		    draw_clip_line(xprev, yprev, x, yprev);
		    draw_clip_line(x, yprev, x, y);
		}

		break;

	default:		/* just a safety */
	case UNDEFINED:
		break;
	}
	prev = plot->points[i].type;
    }
}

/* plot_fsteps:
 * Each new value is reached by tracing up/down to the new y value
 * and then horizontally to the new x value.
 */
static void
plot_fsteps(struct curve_points *plot)
{
    int i;			/* point index */
    int x=0, y=0;		/* point in terminal coordinates */
    int xprev, yprev;		/* previous point coordinates */
    enum coord_type prev = UNDEFINED;	/* type of previous point */

    for (i = 0; i < plot->p_count; i++) {
	xprev = x; yprev = y;

	switch (plot->points[i].type) {
	case INRANGE:
	case OUTRANGE:
		x = map_x(plot->points[i].x);
		y = map_y(plot->points[i].y);

		if (prev == UNDEFINED || invalid_coordinate(x,y))
		    break;
		if (prev == INRANGE) {
		    draw_clip_line(xprev, yprev, xprev, y);
		    draw_clip_line(xprev, y, x, y);
		} else if (prev == OUTRANGE) {
		    draw_clip_line(xprev, yprev, xprev, y);
		    draw_clip_line(xprev, y, x, y);
		}

		break;

	default:		/* just a safety */
	case UNDEFINED:
		break;
	}
	prev = plot->points[i].type;
    }
}

/* HBB 20010625: replaced homegrown bubblesort in plot_histeps() by
 * call of standard routine qsort(). Need to tell the compare function
 * about the plotted dataset via this file scope variable: */
static struct curve_points *histeps_current_plot;

static int
histeps_compare(SORTFUNC_ARGS p1, SORTFUNC_ARGS p2)
{
    double x1 = histeps_current_plot->points[*(int *)p1].x;
    double x2 = histeps_current_plot->points[*(int *)p2].x;

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
    int x1m, y1m, x2m, y2m;	/* mapped coordinates */
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

    /* HBB 20010625: log y axis must treat 0.0 as -infinity.
     * Define the correct y position for the histogram's baseline.
     */
    if (Y_AXIS.log)
	y_null = GPMIN(Y_AXIS.min, Y_AXIS.max);
    else
	y_null = 0.0;

    x = (3.0 * plot->points[gl[0]].x - plot->points[gl[1]].x) / 2.0;
    y = y_null;

    for (i = 0; i < goodcount - 1; i++) {	/* loop over all points except last  */
	yn = plot->points[gl[i]].y;
	if ((Y_AXIS.log) && yn < y_null)
	    yn = y_null;
	xn = (plot->points[gl[i]].x + plot->points[gl[i + 1]].x) / 2.0;

	x1m = map_x(x);
	x2m = map_x(xn);
	y1m = map_y(y);
	y2m = map_y(yn);
	draw_clip_line(x1m, y1m, x1m, y2m);
	draw_clip_line(x1m, y2m, x2m, y2m);

	x = xn;
	y = yn;
    }

    yn = plot->points[gl[i]].y;
    xn = (3.0 * plot->points[gl[i]].x - plot->points[gl[i - 1]].x) / 2.0;

    x1m = map_x(x);
    x2m = map_x(xn);
    y1m = map_y(y);
    y2m = map_y(yn);
    draw_clip_line(x1m, y1m, x1m, y2m);
    draw_clip_line(x1m, y2m, x2m, y2m);
    draw_clip_line(x2m, y2m, x2m, map_y(y_null));

    free(gl);
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
    int xM, ylowM, yhighM;	/* the mapped version of above */
    int yM, xlowM, xhighM;
    int tic = ERRORBARTIC;
    double halfwidth = 0;	/* Used to calculate full box width */

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
		x  += (histogram_opts.gap - 1) / 2.;
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
	    yhighM = map_y(yhigh);
	    ylowM = map_y(ylow);
	    /* This can happen if the y errorbar on a log-scaled Y goes negative */
	    if (plot->points[i].ylow == -VERYLARGE)
		ylowM = map_y(GPMIN(Y_AXIS.min, Y_AXIS.max));

	    /* find low and high points of bar, and check xrange */
	    xhigh = plot->points[i].xhigh;
	    xlow = plot->points[i].xlow;

	    if (plot->plot_style == HISTOGRAMS) {
		xlowM = map_x(x-halfwidth);
		xhighM = map_x(x+halfwidth);
	    } else {
		xhighM = map_x(xhigh);
		xlowM = map_x(xlow);
	    }

	    /* Check for variable color - June 2010 */
	    if ((plot->plot_style != HISTOGRAMS)
		&& (plot->plot_style != FILLEDCURVES)
		) {
		check_for_variable_color(plot, &plot->varcolor[i]);
	    }

	    /* Error bars can now have a separate line style */
	    if ((bar_lp.flags & LP_ERRORBAR_SET) != 0)
		term_apply_lp_properties(&bar_lp);
	    /* Error bars should be drawn in the border color for filled boxes
	     * but only if there *is* a border color. */
	    else if ((plot->plot_style == BOXERROR) && t->fillbox)
		need_fill_border(&plot->fill_properties);

	    /* By here everything has been mapped */
	    /* First draw the main part of the error bar */
	    if (polar) /* only relevant to polar mode "with yerrorbars" */
		draw_clip_line(xlowM, ylowM, xhighM, yhighM);
	    else
		draw_clip_line(xM, ylowM, xM, yhighM);

	    /* Even if error bars are dotted, the end lines are always solid */
	    if ((bar_lp.flags & LP_ERRORBAR_SET) != 0)
		term->dashtype(DASHTYPE_SOLID,NULL);

	    if (!polar) {
		if (bar_size < 0.0) {
		    /* draw the bottom tic same width as box */
		    draw_clip_line(xlowM, ylowM, xhighM, ylowM);
		    /* draw the top tic same width as box */
		    draw_clip_line(xlowM, yhighM, xhighM, yhighM);
		} else if (bar_size > 0.0) {
		    /* draw the bottom tic */
		    draw_clip_line((int)(xM - bar_size * tic), ylowM,
				   (int)(xM + bar_size * tic), ylowM);
		    /* draw the top tic */
		    draw_clip_line((int)(xM - bar_size * tic), yhighM,
				   (int)(xM + bar_size * tic), yhighM);
		}
	    } else { /* Polar error bars */
	    /* Draw the whiskers perpendicular to the main bar */
		if (bar_size > 0.0) {
		    int x1, y1, x2, y2;
		    double slope;

		    slope = atan2((double)(yhighM - ylowM), (double)(xhighM - xlowM));
		    x1 = xlowM - (bar_size * tic * sin(slope));
		    x2 = xlowM + (bar_size * tic * sin(slope));
		    y1 = ylowM + (bar_size * tic * cos(slope));
		    y2 = ylowM - (bar_size * tic * cos(slope));

		    /* draw the bottom tic */
		    if (!clip_point(xlowM,ylowM)) {
			(*t->move)(x1, y1);
			(*t->vector)(x2, y2);
		    }

		    x1 += xhighM - xlowM;
		    x2 += xhighM - xlowM;
		    y1 += yhighM - ylowM;
		    y2 += yhighM - ylowM;

		    /* draw the top tic */
		    if (!clip_point(xhighM,yhighM)) {
			(*t->move)(x1, y1);
			(*t->vector)(x2, y2);
		    }
		}
	    }
	}	/* for loop */
    }		/* if yerrorbars OR xyerrorbars OR yerrorlines OR xyerrorlines */

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
	    xhighM = map_x(xhigh);
	    xlowM = map_x(xlow);
	    /* This can happen if the x errorbar on a log-scaled X goes negative */
	    if (plot->points[i].xlow == -VERYLARGE)
		xlowM = map_x(GPMIN(X_AXIS.min, X_AXIS.max));

	    /* Check for variable color - June 2010 */
	    check_for_variable_color(plot, &plot->varcolor[i]);

	    /* Error bars can now have their own line style */
	    if ((bar_lp.flags & LP_ERRORBAR_SET) != 0)
		term_apply_lp_properties(&bar_lp);

	    /* by here everything has been mapped */
	    draw_clip_line(xlowM, yM, xhighM, yM);

	    /* Even if error bars are dotted, the end lines are always solid */
	    if ((bar_lp.flags & LP_ERRORBAR_SET) != 0)
		term->dashtype(DASHTYPE_SOLID,NULL);

	    if (bar_size > 0.0) {
		draw_clip_line( xlowM, (int)(yM - bar_size * tic),
				xlowM, (int)(yM + bar_size * tic));
		draw_clip_line( xhighM, (int)(yM - bar_size * tic),
				xhighM, (int)(yM + bar_size * tic));
	    }
	}	/* for loop */
    }		/* if xerrorbars OR xyerrorbars OR xerrorlines OR xyerrorlines */

    /* Restore original line properties */
    term_apply_lp_properties(&(plot->lp_properties));
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
    int lastdef = 0;			/* most recent point that was not UNDEFINED */
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
				newsize * sizeof(struct coordinate),
				"stackheight array");
	    for (i = 0; i < newsize; i++) {
		stackheight[i].yhigh = 0;
		stackheight[i].ylow = 0;
	    }
	    stack_count = newsize;
	} else if (stack_count < newsize) {
	    stackheight = gp_realloc( stackheight,
				newsize * sizeof(struct coordinate),
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
		    if (boxwidth < 0)
			dxl = (plot->points[lastdef].x - plot->points[i].x) / 2.0;
		    else if (!boxwidth_is_absolute)
			dxl = (plot->points[lastdef].x - plot->points[i].x) * boxwidth / 2.0;
		    else
			dxl = -boxwidth / 2.0;

		    if (i < plot->p_count - 1) {
			int nextdef;
			for (nextdef = i+1; nextdef < plot->p_count; nextdef++)
			    if (plot->points[nextdef].type != UNDEFINED)
				break;
			if (nextdef == plot->p_count)	/* i is the last non-UNDEFINED point */
			    nextdef = i;
			if (boxwidth < 0)
			    dxr = (plot->points[nextdef].x - plot->points[i].x) / 2.0;
			else if (!boxwidth_is_absolute)
			    dxr = (plot->points[nextdef].x - plot->points[i].x) * boxwidth / 2.0;
			else /* Hits here on 3 column BOXERRORBARS */
			    dxr = boxwidth / 2.0;

			if (plot->points[nextdef].type == UNDEFINED)
			    dxr = -dxl;

		    } else {
			dxr = -dxl;
		    }

		    if (prev == UNDEFINED && lastdef == 0)
			dxl = -dxr;

		    dxl = plot->points[i].x + dxl;
		    dxr = plot->points[i].x + dxr;
		} else {	/* z >= 0 */
		    dxr = plot->points[i].xhigh;
		    dxl = plot->points[i].xlow;
		}

		if (plot->plot_style == BOXXYERROR) {
		    dyb = plot->points[i].ylow;
		    cliptorange(dyb, Y_AXIS.min, Y_AXIS.max);
		    xaxis_y = map_y(dyb);
		    dyt = plot->points[i].yhigh;
		} else {
		    dyt = plot->points[i].y;
		}

		if (plot->plot_style == HISTOGRAMS) {
		    int ix = plot->points[i].x;
		    int histogram_linetype = i;
		    struct lp_style_type ls;
		    int stack = i;
		    if (plot->histogram->startcolor > 0)
			histogram_linetype += plot->histogram->startcolor;

		    /* Shrink each cluster to fit within one unit along X axis,   */
		    /* centered about the integer representing the cluster number */
		    /* 'start' is reset to 0 at the top of eval_plots(), and then */
		    /* incremented if 'plot new histogram' is encountered.        */
		    if (histogram_opts.type == HT_CLUSTERED
		    ||  histogram_opts.type == HT_ERRORBARS) {
			int clustersize = plot->histogram->clustersize + histogram_opts.gap;
			dxl  += (ix-1) * (clustersize - 1) + plot->histogram_sequence;
			dxr  += (ix-1) * (clustersize - 1) + plot->histogram_sequence;
			dxl  += (histogram_opts.gap - 1)/2.;
			dxr  += (histogram_opts.gap - 1)/2.;
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
		    case HT_STACKED_IN_TOWERS: /* columnstacked */
			stack = 0;
			/* Line type (color) must match row number */
			if (prefer_line_styles)
			    lp_use_properties(&ls, histogram_linetype);
			else
			    load_linetype(&ls, histogram_linetype);
			apply_pm3dcolor(&ls.pm3d_color);
			plot->fill_properties.fillpattern = histogram_linetype;
			/* Fall through */
		    case HT_STACKED_IN_LAYERS: /* rowstacked */
			if (plot->points[i].y >= 0){
			    dyb = stackheight[stack].yhigh;
			    dyt += stackheight[stack].yhigh;
			    stackheight[stack].yhigh += plot->points[i].y;
			} else {
			    dyb = stackheight[stack].ylow;
			    dyt += stackheight[stack].ylow;
			    stackheight[stack].ylow += plot->points[i].y;
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
		    case HT_NONE:
			break;
		    }
		}

		/* clip to border */
		cliptorange(dyt, Y_AXIS.min, Y_AXIS.max);
		cliptorange(dxr, X_AXIS.min, X_AXIS.max);
		cliptorange(dxl, X_AXIS.min, X_AXIS.max);

		/* Entire box is out of range on x */
		if (dxr == dxl && (dxr == X_AXIS.min || dxr == X_AXIS.max))
		    break;

		xl = map_x(dxl);
		xr = map_x(dxr);
		yt = map_y(dyt);
		yb = xaxis_y;

		/* Entire box is out of range on y */
		if (yb == yt && (dyt == Y_AXIS.min || dyt == Y_AXIS.max))
		    break;

		if (plot->plot_style == HISTOGRAMS
		&& (histogram_opts.type == HT_STACKED_IN_LAYERS
		    || histogram_opts.type == HT_STACKED_IN_TOWERS))
			yb = map_y(dyb);

		/* Variable color */
		if (plot->plot_style == BOXES || plot->plot_style == BOXXYERROR
		    || plot->plot_style == HISTOGRAMS
		    || plot->plot_style == BOXERROR) {
		    check_for_variable_color(plot, &plot->varcolor[i]);
		}

		if ((plot->fill_properties.fillstyle != FS_EMPTY) && t->fillbox) {
		    int x, y, w, h;
		    int style;

		    x = xl;
		    y = yb;
		    w = xr - xl + 1;
		    h = yt - yb + 1;
		    /* avoid negative width/height */
		    if (w <= 0) {
			x = xr;
			w = xl - xr + 1;
		    }
		    if (h <= 0) {
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

		if (t->fillbox && plot->fill_properties.border_color.type != TC_DEFAULT) {
		    term_apply_lp_properties(&plot->lp_properties);
		}

		break;
	    }			/* case OUTRANGE, INRANGE */

	default:		/* just a safety */
	case UNDEFINED:{
		break;
	    }

	}			/* switch point-type */

	prev = plot->points[i].type;
	if (prev != UNDEFINED)
	    lastdef = i;
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
    int p_width, p_height;
    int pointtype;
    struct termentry *t = term;
    int interval = plot->lp_properties.p_interval;
    int number = abs(plot->lp_properties.p_number);
    int offset = 0;
    const char *ptchar;

    /* The "pointnumber" property limits the total number of points drawn for this curve */
    if (number) {
	int pcountin = 0;
	for (i = 0; i < plot->p_count; i++) {
	    if (plot->points[i].type == INRANGE) pcountin++;
	}
	if (pcountin > number) {
	    if (number > 1)
		interval = (double)(pcountin-1)/(double)(number-1);
	    else
		interval = pcountin;
	    /* offset the first point drawn so that successive plots are more distinct */
	    offset = plot->current_plotno * ceil(interval/6.0);
	    if (plot->lp_properties.p_number < 0)
		interval = -interval;
	}
    }

    /* Set whatever we can that applies to every point in the loop */
    if (plot->lp_properties.p_type == PT_CHARACTER) {
	ignore_enhanced(TRUE);
	if (plot->labels->font && plot->labels->font[0])
	    (*t->set_font) (plot->labels->font);
	(*t->justify_text) (CENTRE);
    }

    p_width = t->h_tic * plot->lp_properties.p_size;
    p_height = t->v_tic * plot->lp_properties.p_size;

    /* Displace overlapping points if "set jitter" is in effect	*/
    /* This operation leaves x and y untouched, but loads the	*/
    /* jitter offsets into xhigh and yhigh.			*/
    if (jitter.spread > 0)
	jitter_points(plot);

    for (i = 0; i < plot->p_count; i++) {

	/* Only print 1 point per interval */
	if ((plot->plot_style == LINESPOINTS) && (interval) && ((i-offset) % interval))
	    continue;

	if (plot->points[i].type == INRANGE) {

	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);

	    /* map_x or map_y can hit NaN during eval_link_function(), in which */
	    /* case the coordinate value is garbage and undefined is TRUE.      */
	    if (invalid_coordinate(x,y))
		plot->points[i].type = UNDEFINED;
	    if (plot->points[i].type == UNDEFINED)
		continue;

	    /* Apply jitter offsets.
	     * Swarm jitter x offset is a multiple of character width.
	     * Swarm jitter y offset is in the original coordinate system.
	     * vertical jitter y offset is a multiple of character heights.
	     */
	    if (jitter.spread > 0) {
		x += plot->points[i].CRD_XJITTER * 0.7 * t->h_char;
		switch (jitter.style) {
		    case JITTER_ON_Y:
			y += plot->points[i].CRD_YJITTER * 0.7 * t->v_char;
			break;
		    case JITTER_SWARM:
		    case JITTER_SQUARE:
		    default:
			y = map_y(plot->points[i].y + plot->points[i].CRD_YJITTER);
			break;
		}
	    }

	    /* do clipping if necessary */
	    if (!clip_points
		|| (x >= plot_bounds.xleft + p_width
		    && y >= plot_bounds.ybot + p_height
		    && x <= plot_bounds.xright - p_width
		    && y <= plot_bounds.ytop - p_height)) {

		if ((plot->lp_properties.p_size == PTSZ_VARIABLE)
		&&  (plot->plot_style == POINTSTYLE || plot->plot_style == LINESPOINTS
		     || plot->plot_style == YERRORBARS))
		    (*t->pointsize)(pointsize * plot->points[i].CRD_PTSIZE);

		/* Feb 2016: variable point type */
		if ((plot->plot_style == POINTSTYLE || plot->plot_style == LINESPOINTS
		     || plot->plot_style == YERRORBARS)
		&&  (plot->lp_properties.p_type == PT_VARIABLE)
		&&  !(isnan(plot->points[i].CRD_PTTYPE))) {
		    pointtype = plot->points[i].CRD_PTTYPE - 1;
		} else {
		    pointtype = plot->lp_properties.p_type;
		}

		/* A negative interval indicates we should try to blank out the */
		/* area behind the point symbol. This could be done better by   */
		/* implementing a special point type, but that would require    */
		/* modification to all terminal drivers. It might be worth it.  */
		/* term_apply_lp_properties will restore the point type and size*/
		if ((plot->plot_style == LINESPOINTS && interval < 0)
		||  (plot->plot_style == YERRORBARS)) {
		    if (pointintervalbox != 0) {
			(*t->set_color)(&background_fill);
			(*t->pointsize)(pointsize * pointintervalbox);
			(*t->point) (x, y, 6);
			term_apply_lp_properties(&(plot->lp_properties));
		    }
		}

		/* rgb variable  -  color read from data column */
		check_for_variable_color(plot, &plot->varcolor[i]);

		/* There are two conditions where we will print a character rather
		 * than a point symbol. Otherwise ptchar = NULL;
		 * (1) plot->lp_properties.p_type == PT_CHARACTER
		 * (2) plot->lp_properties.p_type == PT_VARIABLE and the data file
		 *     contained a string rather than a number
		 */
		if (plot->lp_properties.p_type == PT_CHARACTER)
		    ptchar = plot->lp_properties.p_char;
		else if (pointtype == PT_VARIABLE && isnan(plot->points[i].CRD_PTTYPE))
		    ptchar = (char *)(&plot->points[i].CRD_PTCHAR);
		else
		    ptchar = NULL;

		/* Print special character rather than drawn symbol */
		if (ptchar) {
		    if (plot->labels && (plot->labels->textcolor.type != TC_DEFAULT))
			apply_pm3dcolor(&(plot->labels->textcolor));
		    (*t->put_text)(x, y, ptchar);
		}

		/* The normal case */
		else if (pointtype >= -1)
		    (*t->point) (x, y, pointtype);
	    }
	}
    }

    /* Return to initial state */
    if (plot->lp_properties.p_type == PT_CHARACTER) {
	if (plot->labels->font && plot->labels->font[0])
	    (*t->set_font) ("");
	ignore_enhanced(FALSE);
    }
}

/* plot_circles:
 * Plot the curves in CIRCLES style
 */
static void
plot_circles(struct curve_points *plot)
{
    int i;
    int x, y;
    double radius, arc_begin, arc_end;
    struct fill_style_type *fillstyle = &plot->fill_properties;
    int style = style_from_fill(fillstyle);
    TBOOLEAN withborder = FALSE;
    BoundingBox *clip_save = clip_area;

    if (default_circle.clip == OBJ_NOCLIP)
	clip_area = &canvas;

    if (fillstyle->border_color.type != TC_LT
    ||  fillstyle->border_color.lt != LT_NODRAW)
	withborder = TRUE;

    for (i = 0; i < plot->p_count; i++) {
	if (plot->points[i].type == INRANGE) {
	    x = map_x(plot->points[i].x);
	    y = map_y(plot->points[i].y);
	    if (invalid_coordinate(x,y))
		continue;
	    radius = x - map_x(plot->points[i].xlow);
	    if (plot->points[i].z == DEFAULT_RADIUS)
		map_position_r( &default_circle.o.circle.extent, &radius, NULL, "radius");

	    arc_begin = plot->points[i].ylow;
	    arc_end = plot->points[i].xhigh;

	    /* rgb variable  -  color read from data column */
	    if (!check_for_variable_color(plot, &plot->varcolor[i]) && withborder)
		term_apply_lp_properties(&plot->lp_properties);
	    do_arc(x,y, radius, arc_begin, arc_end, style, FALSE);
	    if (withborder) {
		need_fill_border(&plot->fill_properties);
		do_arc(x,y, radius, arc_begin, arc_end, 0, default_circle.o.circle.wedge);
	    }
	}
    }

    clip_area = clip_save;
}

/* plot_ellipses:
 * Plot the curves in ELLIPSES style
 */
static void
plot_ellipses(struct curve_points *plot)
{
    int i;
    t_ellipse *e = (t_ellipse *) gp_alloc(sizeof(t_ellipse), "ellipse plot");
    double tempx, tempy, tempfoo;
    struct fill_style_type *fillstyle = &plot->fill_properties;
    int style = style_from_fill(fillstyle);
    TBOOLEAN withborder = FALSE;
    BoundingBox *clip_save = clip_area;

    if (default_ellipse.clip == OBJ_NOCLIP)
	clip_area = &canvas;

    if (fillstyle->border_color.type != TC_LT
    ||  fillstyle->border_color.lt != LT_NODRAW)
	withborder = TRUE;

    e->extent.scalex = (plot->x_axis == SECOND_X_AXIS) ? second_axes : first_axes;
    e->extent.scaley = (plot->y_axis == SECOND_Y_AXIS) ? second_axes : first_axes;
    e->type = plot->ellipseaxes_units;

    for (i = 0; i < plot->p_count; i++) {
	if (plot->points[i].type == INRANGE) {
	    e->center.x = map_x(plot->points[i].x);
	    e->center.y = map_y(plot->points[i].y);
	    if (invalid_coordinate(e->center.x, e->center.y))
		continue;

	    e->extent.x = plot->points[i].xlow; /* major axis */
	    e->extent.y = plot->points[i].xhigh; /* minor axis */
	    /* the mapping can be set by the
	     * "set ellipseaxes" setting
	     * both x units, mixed, both y units */
	    /* clumsy solution */
	    switch (e->type) {
	    case ELLIPSEAXES_XY:
		map_position_r(&e->extent, &tempx, &tempy, "ellipse");
		e->extent.x = tempx;
		e->extent.y = tempy;
		break;
	    case ELLIPSEAXES_XX:
		map_position_r(&e->extent, &tempx, &tempy, "ellipse");
		tempfoo = tempx;
		e->extent.x = e->extent.y;
		map_position_r(&e->extent, &tempy, &tempx, "ellipse");
		e->extent.x = tempfoo;
		e->extent.y = tempy;
		break;
	    case ELLIPSEAXES_YY:
		map_position_r(&e->extent, &tempx, &tempy, "ellipse");
		tempfoo = tempy;
		e->extent.y = e->extent.x;
		map_position_r(&e->extent, &tempy, &tempx, "ellipse");
		e->extent.x = tempx;
		e->extent.y = tempfoo;
		break;
	    }

	    if (plot->points[i].z <= DEFAULT_RADIUS) {
		map_position_r(&default_ellipse.o.ellipse.extent,
				&e->extent.x, &e->extent.y, "ellipse");
	    }

	    /* May 2018 - default orientation used to be signalled by
	     * DEFAULT_ELLIPSE rather than passed in explicitly
	     */
	    e->orientation = plot->points[i].ylow;

	    /* rgb variable  -  color read from data column */
	    if (!check_for_variable_color(plot, &plot->varcolor[i]) && withborder)
		term_apply_lp_properties(&plot->lp_properties);
	    do_ellipse(2, e, style, FALSE);
	    if (withborder) {
		need_fill_border(&plot->fill_properties);
		do_ellipse(2, e, 0, FALSE);
	    }
	}
    }
    free(e);
    clip_area = clip_save;
}

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
	    if (invalid_coordinate(x,y))
		continue;
	    /* rgb variable  -  color read from data column */
	    check_for_variable_color(plot, &plot->varcolor[i]);
	    /* point type -1 is a dot */
	    (*t->point) (x, y, -1);
	}
    }

}

/* plot_vectors:
 * Plot the curves in VECTORS style (used also for ARROWS)
 */
static void
plot_vectors(struct curve_points *plot)
{
    int i;
    arrow_style_type ap;
    BoundingBox *clip_save = clip_area;

    /* Normally this is only necessary once because all arrows equal */
    ap = plot->arrow_properties;
    term_apply_lp_properties(&ap.lp_properties);
    apply_head_properties(&ap);

    /* Clip to plot */
    clip_area = &plot_bounds;

    for (i = 0; i < plot->p_count; i++) {

	double x0, y0, x1, y1;
	struct coordinate *tail = &(plot->points[i]);

	if (tail->type == UNDEFINED)
	    continue;

	/* The only difference between "with vectors" and "with arrows"
	 * is that vectors already have the head coordinates in xhigh, yhigh
	 * while arrows need to generate them from length + angle.
	 */
	x0 = map_x_double(tail->x);
	y0 = map_y_double(tail->y);
	if (plot->plot_style == VECTOR) {
	    x1 = map_x_double(tail->xhigh);
	    y1 = map_y_double(tail->yhigh);
	} else {  /* ARROWS */
	    double length;
	    double angle = DEG2RAD * tail->yhigh;
	    double aspect = (double)term->v_tic / (double)term->h_tic;
	    if (strcmp(term->name, "windows") == 0)
		aspect = 1.0;
	    if (tail->xhigh > 0)
		/* length > 0 is in x-axis coords */
		length = map_x_double(tail->x + tail->xhigh) - x0;
	    else {
		/* -1 < length < 0 indicates graph coordinates */
		length = tail->xhigh * (plot_bounds.xright - plot_bounds.xleft);
		length = fabs(length);
	    }
	    x1 = x0 + cos(angle) * length;
	    y1 = y0 + sin(angle) * length * aspect;
	}

	/* variable arrow style read from extra data column */
	if (plot->arrow_properties.tag == AS_VARIABLE) {
	    int as = tail->z;
	    arrow_use_properties(&ap, as);
	    term_apply_lp_properties(&ap.lp_properties);
	    apply_head_properties(&ap);
	    /* Over-write plot lp_properties so the check for variable color works */
	    plot->lp_properties = ap.lp_properties;
	}

	/* variable color read from extra data column. */
	check_for_variable_color(plot, &plot->varcolor[i]);

	/* draw_clip_arrow does the hard work for us */
	draw_clip_arrow(x0, y0, x1, y1, ap.head);
    }

    clip_area = clip_save;
}


/* plot_f_bars:
 * Plot the curves in FINANCEBARS style
 * EAM Feg 2010	- This routine is also used for BOXPLOT, which
 *		  loads a median value into xhigh
 */
static void
plot_f_bars(struct curve_points *plot)
{
    int i;			/* point index */
    struct termentry *t = term;
    double x;			/* position of the bar */
    double ylow, yhigh, yclose, yopen;	/* the ends of the bars */
    double ymedian;
    int xM, ylowM, yhighM;	/* the mapped version of above */
    int yopenM, ycloseM, ymedianM;
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
	ymedian = plot->points[i].xhigh;

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

	/* variable color read from extra data column. June 2010 */
	check_for_variable_color(plot, &plot->varcolor[i]);

	yopenM = map_y(yopen);
	ycloseM = map_y(yclose);
	ymedianM = map_y(ymedian);

	/* draw the main bar, open tic, close tic */
	draw_clip_line(xM, ylowM, xM, yhighM);
	draw_clip_line(xM - bar_size * tic, yopenM, xM, yopenM);
	draw_clip_line(xM + bar_size * tic, ycloseM, xM, ycloseM);

	/* Draw a bar at the median */
	if (plot->plot_style == BOXPLOT)
	    draw_clip_line(xM - bar_size * tic, ymedianM, xM + bar_size * tic, ymedianM);
    }
}


/* plot_c_bars:
 * Plot the curves in CANDLESTICKS style
 * EAM Apr 2008 - switch to using empty/fill rather than empty/striped
 *		  to distinguish whether (open > close)
 * EAM Dec 2009	- allow an optional 6th column to specify width
 *		  This routine is also used for BOXPLOT, which
 *		  loads a median value into xhigh
 */
static void
plot_c_bars(struct curve_points *plot)
{
    struct termentry *t = term;
    int i;
    double x;						/* position of the bar */
    double dxl, dxr, ylow, yhigh, yclose, yopen, ymed;	/* the ends of the bars */
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
	ymed = plot->points[i].xhigh;

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

	if (plot->points[i].xlow != plot->points[i].x) {
	    dxl = plot->points[i].xlow;
	    dxr = 2 * x - dxl;
	    cliptorange(dxr, X_AXIS.min, X_AXIS.max);
	    cliptorange(dxl, X_AXIS.min, X_AXIS.max);
	    xlowM = map_x(dxl);
	    xhighM = map_x(dxr);

	} else if (plot->plot_style == BOXPLOT) {
	    dxr = (boxwidth_is_absolute && boxwidth > 0) ? boxwidth/2. : 0.25;
	    xlowM = map_x(x-dxr);
	    xhighM = map_x(x+dxr);

	} else if (boxwidth < 0.0) {
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

	    dxl = x + dxl;
	    dxr = x + dxr;
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
		term_apply_lp_properties(&plot->lp_properties);
	}
	/* Reset also if we changed it for the errorbars */
	else if ((bar_lp.flags & LP_ERRORBAR_SET) != 0) {
		term_apply_lp_properties(&plot->lp_properties);
	}

	/* variable color read from extra data column. June 2010 */
	check_for_variable_color(plot, &plot->varcolor[i]);

	/* Boxes are always filled if an explicit non-empty fillstyle is set. */
	/* If the fillstyle is FS_EMPTY, fill to indicate (open > close).     */
	if (term->fillbox && !skip_box) {
	    int style = style_from_fill(&plot->fill_properties);
	    if ((style != FS_EMPTY) || (yopen > yclose)) {
		int x = xlowM;
		int y = ymin;
		int w = (xhighM-xlowM);
		int h = (ymax-ymin);

		if (style == FS_EMPTY && plot->plot_style != BOXPLOT)
		    style = FS_OPAQUE;
		(*t->fillbox)(style, x, y, w, h);

		if (style_from_fill(&plot->fill_properties) != FS_EMPTY)
		    need_fill_border(&plot->fill_properties);
	    }
	}

	/* Draw open box */
	if (!skip_box) {
	    newpath();
	    (*t->move)   (xlowM, map_y(yopen));
	    (*t->vector) (xhighM, map_y(yopen));
	    (*t->vector) (xhighM, map_y(yclose));
	    (*t->vector) (xlowM, map_y(yclose));
	    (*t->vector) (xlowM, map_y(yopen));
	    closepath();
	}

	/* BOXPLOT wants a median line also, which is stored in xhigh. */
	/* If no special style has been assigned for the median line   */
	/* draw it now, otherwise wait until later.                    */
	if (plot->plot_style == BOXPLOT && boxplot_opts.median_linewidth < 0) {
	    int ymedianM = map_y(ymed);
	    draw_clip_line(xlowM,  ymedianM, xhighM, ymedianM);
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

	/* Error bars can now have their own line style */
	if ((bar_lp.flags & LP_ERRORBAR_SET) != 0) {
	    term_apply_lp_properties(&bar_lp);
	}

	/* Draw whiskers */
	draw_clip_line(xM, ylowM, xM, ymin);
	draw_clip_line(xM, ymax, xM, yhighM);

	/* Some users prefer bars at the end of the whiskers */
	if (plot->plot_style == BOXPLOT
	||  plot->arrow_properties.head == BOTH_HEADS) {
	    int d;
	    if (plot->plot_style == BOXPLOT) {
		if (bar_size < 0)
		    d = 0;
		else
		    d = (xhighM-xlowM)/2. - (bar_size * term->h_tic);
	    } else {
		double frac = plot->arrow_properties.head_length;
		d = (frac <= 0) ? 0 : (xhighM-xlowM)*(1.-frac)/2.;
	    }

	    draw_clip_line(xlowM+d, yhighM, xhighM-d, yhighM);
	    draw_clip_line(xlowM+d, ylowM, xhighM-d, ylowM);
	}

	/* BOXPLOT wants a median line also, which is stored in xhigh. */
	/* If a special linewidth has been assigned draw it now.       */
	if (plot->plot_style == BOXPLOT && boxplot_opts.median_linewidth > 0) {
	    int ymedianM = map_y(ymed);
	    (*t->linewidth) (boxplot_opts.median_linewidth);
	    draw_clip_line(xlowM,  ymedianM, xhighM, ymedianM);
	    (*t->linewidth) (plot->lp_properties.l_width);
	}

	prev = plot->points[i].type;
    }
}

static void
plot_parallel(struct curve_points *plot)
{
    int i;
    int x0, y0, x1, y1;
    struct curve_points *thisplot;

    /* The parallel axis data is stored in successive plot structures. */
    /* We will draw it all at once when we see the first one and ignore the rest. */
    if (plot->p_axis != 1)
	return;

    for (i = 0; i < plot->p_count; i++) {
	struct axis *this_axis = &parallel_axis_array[plot->p_axis-1];
	TBOOLEAN prev_NaN = FALSE;

	/* rgb variable  -  color read from data column */
	check_for_variable_color(plot, &plot->varcolor[i]);

	x0 = map_x(plot->points[i].x);
	y0 = axis_map(this_axis, plot->points[i].y);
	prev_NaN = isnan(plot->points[i].y);

	thisplot = plot;
	while ((thisplot = thisplot->next)) {

	    if (thisplot->plot_style != PARALLELPLOT)
		continue;

	    if (thisplot->points == NULL) {
		prev_NaN = TRUE;
		continue;
	    }

	    this_axis = &parallel_axis_array[thisplot->p_axis-1];
	    x1 = map_x(thisplot->points[i].x);
	    y1 = axis_map(this_axis, thisplot->points[i].y);
	    if (prev_NaN)
		prev_NaN = isnan(thisplot->points[i].y);
	    else if (!(prev_NaN = isnan(thisplot->points[i].y)))
		draw_clip_line(x0, y0, x1, y1);
	    x0 = x1;
	    y0 = y1;
	}

    }
}

/*
 * Spiderplots, also known as radar charts, are a form of parallel-axis plot
 * in which the axes are arranged radially.  Each sequential clause in a
 * "plot ... with spiderplot" command provides values along a single one of
 * these axes.  Line, point, and fill properties are taken from the first
 * clause of the plot command.  Line properties have already been applied
 * prior to calling this routine.
 */
static void
plot_spiderplot(struct curve_points *plot)
{
    int i, j;
    struct curve_points *thisplot;
    static gpiPoint *corners = NULL;
    static gpiPoint *clpcorn = NULL;
    BoundingBox *clip_save = clip_area;
    int n_spokes = 0;

    /* The parallel axis data is stored in successive plot structures.
     * We will draw it all at once when we see the first one and ignore the rest.
     */
    if (plot->p_axis != 1)
	return;

    /* This loop counts the number of radial axes */
    for (thisplot = plot; thisplot; thisplot = thisplot->next) {
	if (thisplot->plot_type == KEYENTRY)
	    continue;
	if (thisplot->plot_style != SPIDERPLOT) {
	    int_warn(NO_CARET, "plot %d is not a spiderplot component", n_spokes);
	    continue;
	}

	/* Triggers when there is more than one spiderplot in the 'plot' command */
	if (thisplot->p_axis < n_spokes-1)
	    break;
	n_spokes++;

	/* Use plot title to label the corresponding radial axis */
	if (thisplot->title) {
	    free (parallel_axis_array[thisplot->p_axis-1].label.text);
	    parallel_axis_array[thisplot->p_axis-1].label.text = strdup(thisplot->title);
	}
    }

    if (n_spokes < 3)
	int_error(NO_CARET, "at least 3 axes are needed for a spiderplot");

    /* Allocate data structures for one polygon */
    corners = gp_realloc(corners, (n_spokes+1) * sizeof(gpiPoint), "polygon");
    clpcorn = gp_realloc(clpcorn, (2*n_spokes+1) * sizeof(gpiPoint), "polygon");
    clip_area = &canvas;

    /*
     * Each row of data (NB: *not* each column) describes a vertex of a polygon.
     * There is one vertex for each comma-separated clause within the overall 2D
     * "plot" command.  Thus p_count rows of data produce p_count polygons.
     * If any row contains NaN or a missing value, no polygon is produced.
     */
    for (i = 0; i < plot->p_count; i++) {
	TBOOLEAN bad_data = FALSE;
	struct axis *this_axis;
	double r, theta;
	double x, y;
	int out_length;
	int p_type;
	TBOOLEAN already_did_one = FALSE;

	for (thisplot = plot; thisplot != NULL; thisplot = thisplot->next) {

	    /* Ignore other stuff, e.g. KEYENTRY */
	    if (thisplot->plot_style != SPIDERPLOT || thisplot->plot_type != DATA)
		continue;

	    /* If any point is missing or NaN, skip the whole polygon */
	    if ((thisplot->points == NULL) || (thisplot->p_count <= i)
	    ||  (thisplot->points[i].type == UNDEFINED)
	    ||  isnan(thisplot->points[i].x) || isnan(thisplot->points[i].y)) {
		/* FIXME EAM: how to exit cleanly? */
		bad_data = TRUE;
		break;
	    }

	    /* Ran off end of previous spiderplot */
	    if (thisplot->p_axis == 1 && already_did_one)
		break;
	    else
		already_did_one = TRUE;

	    /* stored values are axis number, unscaled R */
	    this_axis = &parallel_axis_array[thisplot->p_axis-1];
	    theta = M_PI_2 - (thisplot->points[i].x - 1) * 2*M_PI / n_spokes;
	    r = (thisplot->points[i].y - this_axis->min) / (this_axis->max - this_axis->min);
	    polar_to_xy(theta, r, &x, &y, FALSE);
	    corners[thisplot->p_axis-1].x = map_x(x);
	    corners[thisplot->p_axis-1].y = map_y(y);
	}

	/* Spider plots are unusual in that each row starts a new plot
	 * In order to associate the key entry with the correct plot we
	 * must do it inside the loop over rows
	 */
	if (i > 0) {
	    term->layer(TERM_LAYER_AFTER_PLOT);
	    term->layer(TERM_LAYER_BEFORE_PLOT);
	}
	if (plot->labels) {
	    TBOOLEAN default_color;
	    text_label *key_entry;
	    for (key_entry = plot->labels->next; key_entry; key_entry = key_entry->next) {
		if (key_entry->tag != i)
		    continue;
		default_color = (plot->lp_properties.pm3d_color.type == TC_DEFAULT);
		if (default_color)
		    load_linetype(&plot->lp_properties, key_entry->tag + 1);
		advance_key(TRUE);
		do_key_sample(plot, &keyT, key_entry->text, plot->points[i].CRD_COLOR);
		if (default_color)
		    plot->lp_properties.pm3d_color.type = TC_DEFAULT;
		key_count++;
		advance_key(0);
	    }
	}

	/* Do not draw anything if one or more of the values was bad */
	if (bad_data) {
	    int_warn(NO_CARET, "Skipping spiderplot with bad data");
	    continue;
	}

	corners[n_spokes].x = corners[0].x;
	corners[n_spokes].y = corners[0].y;
	clip_polygon(corners, clpcorn, n_spokes, &out_length);
	clpcorn[0].style = style_from_fill(&plot->fill_properties);

	/* rgb variable  -  color read from data column */
	if (!check_for_variable_color(plot, &plot->varcolor[i])
	&&   plot->lp_properties.pm3d_color.type == TC_DEFAULT) {
	    lp_style_type lptmp;
	    load_linetype(&lptmp, i+1);
	    apply_pm3dcolor(&(lptmp.pm3d_color));
	}

	/* variable point type */
	p_type = plot->points[i].CRD_PTTYPE - 1;

	/* Draw filled area */
	if (out_length > 1 && plot->fill_properties.fillstyle != FS_EMPTY) {
	    if (term->filled_polygon)
		term->filled_polygon(out_length, clpcorn);
	}

	/* Draw perimeter */
	if (need_fill_border(&plot->fill_properties)) {
	    for (j = 0; j < n_spokes; j++)
		draw_clip_line( corners[j].x, corners[j].y,
				corners[j+1].x, corners[j+1].y );
	}

	/* Points */
	if (p_type) {
	    for (j = 0; j < n_spokes; j++)
		term->point(corners[j].x, corners[j].y, p_type);
	}

    } /* End of loop over rows, each a separate polygon */

    clip_area = clip_save;
}

/*
 * Plot the curves in BOXPLOT style
 * helper functions: compare_ypoints, filter_boxplot
 */

static int
compare_ypoints(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
    struct coordinate const *p1 = arg1;
    struct coordinate const *p2 = arg2;

    if (boxplot_factor_sort_required) {
	/* Primary sort key is the "factor" */
	if (p1->z > p2->z)
	    return (1);
	if (p1->z < p2->z)
	    return (-1);
    }

    if (p1->y > p2->y)
	return (1);
    if (p1->y < p2->y)
	return (-1);
    return (0);
}

int
filter_boxplot(struct curve_points *plot)
{
    int N = plot->p_count;
    int i;

    /* Force any undefined points to the end of the list by y value */
    for (i=0; i<N; i++)
	if (plot->points[i].type == UNDEFINED)
	    plot->points[i].y = plot->points[i].z = VERYLARGE;

    /* Sort the points to find median and quartiles */
    if (plot->boxplot_factors > 1)
	boxplot_factor_sort_required = TRUE;
    qsort(plot->points, N, sizeof(struct coordinate), compare_ypoints);

    /* Return a count of well-defined points with this index */
    while (plot->points[N-1].type == UNDEFINED)
	N--;

    return N;
}

/*
 * wrapper called by do_plot after reading in data but before plotting
 */
void
autoscale_boxplot(struct curve_points *plot)
{
    plot_boxplot(plot, TRUE);
}

static void
plot_boxplot(struct curve_points *plot, TBOOLEAN only_autoscale)
{
    int N;
    struct coordinate *save_points = plot->points;
    int saved_p_count = plot->p_count;

    struct coordinate *subset_points;
    int subset_count, true_count;
    struct text_label *subset_label = plot->labels;

    struct coordinate candle;
    double median, quartile1, quartile3;
    double whisker_top=0, whisker_bot=0;

    int level;
    int levels = plot->boxplot_factors;
    if (levels == 0)
	levels = 1;

    if (!save_points || saved_p_count == 0)
	return;

    /* The entire collection of points was already sorted in filter_boxplot()
     * called from boxplot_range_fiddling().  That sort used the category
     * (a.k.a. "factor" a.k.a. "level") as a primary key and the y value as
     * a secondary key.  That is sufficient for describing all points in a
     * single boxplot, but if we want a separate boxplot for each category
     * then additional bookkeeping is required.
     */
    for (level=0; level<levels; level++) {
	if (levels == 1) {
	    subset_points = save_points;
	    subset_count = saved_p_count;
	} else {
	    subset_label = subset_label->next;
	    true_count = 0;
	    /* advance to first point in subset */
	    for (subset_points = save_points;
		 subset_points->z != subset_label->tag;
		 subset_points++, true_count++) {
		    /* No points found for this boxplot factor */
		    if (true_count >= saved_p_count)
			break;
	    }

	    /* count well-defined points in this subset */
	    for (subset_count=0;
		 true_count < saved_p_count
		    && subset_points[subset_count].z == subset_label->tag;
		 subset_count++, true_count++) {
			if (subset_points[subset_count].type == UNDEFINED)
			    break;
		}
	}

	/* Not enough points left to make a boxplot */
	N = subset_count;
	if (N < 4) {
	    if (only_autoscale)
		continue;
	    candle.x = subset_points->x + boxplot_opts.separation * level;
	    candle.yhigh = -VERYLARGE;
	    candle.ylow = VERYLARGE;
	    goto outliers;
	}

	if ((N & 0x1) == 0)
	    median = 0.5 * (subset_points[N/2 - 1].y + subset_points[N/2].y);
	else
	    median = subset_points[(N-1)/2].y;
	if ((N & 0x3) == 0)
	    quartile1 = 0.5 * (subset_points[N/4 - 1].y + subset_points[N/4].y);
	else
	    quartile1 = subset_points[(N+3)/4 - 1].y;
	if ((N & 0x3) == 0)
	    quartile3 = 0.5 * (subset_points[N - N/4].y + subset_points[N - N/4 - 1].y);
	else
	    quartile3 = subset_points[N - (N+3)/4].y;

	FPRINTF((stderr,"Boxplot: quartile boundaries for %d points: %g %g %g\n",
			N, quartile1, median, quartile3));

	/* Set the whisker limits based on the user-defined style */
	if (boxplot_opts.limit_type == 0) {
	    /* Fraction of interquartile range */
	    double whisker_len = boxplot_opts.limit_value * (quartile3 - quartile1);
	    int i;
	    whisker_bot = quartile1 - whisker_len;
	    for (i=0; i<N; i++)
		if (subset_points[i].y >= whisker_bot) {
		    whisker_bot = subset_points[i].y;
		    break;
		}
	    whisker_top = quartile3 + whisker_len;
	    for (i=N-1; i>= 0; i--)
		if (subset_points[i].y <= whisker_top) {
		    whisker_top = subset_points[i].y;
		    break;
		}

	} else {
	    /* Set limits to include some fraction of the total number of points. */
	    /* The limits are symmetric about the median, but are truncated to    */
	    /* lie on a point in the data set.                                    */
	    int top = N-1;
	    int bot = 0;
	    while ((double)(top-bot+1)/(double)(N) >= boxplot_opts.limit_value) {
		/* This point is outside of the fractional limit. Remember where it is,
		 * step over all points with the same value, then trim back one point.
		 */
		whisker_top = subset_points[top].y;
		whisker_bot = subset_points[bot].y;
		if (whisker_top - median >= median - whisker_bot) {
		    while ((top > 0) && (subset_points[top].y == subset_points[top-1].y))
			top--;
		    top--;
		}
		if (whisker_top - median <= median - whisker_bot) {
		    while ((bot < top) && (subset_points[bot].y == subset_points[bot+1].y))
			bot++;
		    bot++;
		}
	    }
	}

	/* X coordinate needed both for autoscaling and to draw the candlestick */
	if (plot->plot_type == FUNC)
	    candle.x = (subset_points[0].x + subset_points[N-1].x) / 2.;
	else
	    candle.x = subset_points->x + boxplot_opts.separation * level;

	/* We're only here for autoscaling */
	if (only_autoscale) {
	    autoscale_one_point((&X_AXIS), candle.x);
	    autoscale_one_point((&Y_AXIS), whisker_bot);
	    autoscale_one_point((&Y_AXIS), whisker_top);
	    continue;
	}

	/* Dummy up a single-point candlesticks plot using these limiting values */
	candle.type = INRANGE;
	candle.y = quartile1;
	candle.z = quartile3;
	candle.ylow  = whisker_bot;
	candle.yhigh = whisker_top;
	candle.xlow  = subset_points->xlow + boxplot_opts.separation * level;
	candle.xhigh = median;	/* Crazy order of candlestick parameters! */
	plot->points = &candle;
	plot->p_count = 1;

	/* for boxplots "lc variable" means color by factor index */
	if (plot->varcolor)
	    plot->varcolor[0] = plot->base_linetype + level + 1;

	if (boxplot_opts.plotstyle == FINANCEBARS)
	    plot_f_bars( plot );
	else
	    plot_c_bars( plot );

	/* Now draw individual points for the outliers */
	outliers:
	if (boxplot_opts.outliers) {
	    int i,j,x,y;
	    int p_width = term->h_tic * plot->lp_properties.p_size;
	    int p_height = term->v_tic * plot->lp_properties.p_size;

	    for (i = 0; i < subset_count; i++) {

		if (subset_points[i].y >= candle.ylow
		&&  subset_points[i].y <= candle.yhigh)
		    continue;

		if (subset_points[i].type == UNDEFINED)
		    continue;

		x = map_x(candle.x);
		y = map_y(subset_points[i].y);

		/* previous INRANGE/OUTRANGE no longer valid */
		if (x < plot_bounds.xleft + p_width
		||  y < plot_bounds.ybot + p_height
		||  x > plot_bounds.xright - p_width
		||  y > plot_bounds.ytop - p_height)
			continue;

		/* Separate any duplicate outliers */
		for (j=1; (i >= j) && (subset_points[i].y == subset_points[i-j].y); j++)
		    x += p_width * ((j & 1) == 0 ? -j : j);;

		(term->point) (x, y, plot->lp_properties.p_type);
	    }
	}

    /* Restore original dataset points and size */
    plot->points = save_points;
    plot->p_count = saved_p_count;
    }
}


/* display a x-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
static void
xtick2d_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid,	/* grid.l_type == LT_NODRAW means no grid */
    struct ticmark *userlabels)	/* User-specified tic labels */
{
    struct termentry *t = term;
    /* minitick if text is NULL - beware - h_tic is unsigned */
    int ticsize = tic_direction * (int) t->v_tic * tic_scale(ticlevel, this_axis);
    int x = map_x(place);

    /* Skip label if we've already written a user-specified one here */
#   define MINIMUM_SEPARATION 2
    while (userlabels) {
	int here = map_x(userlabels->position);
	if (abs(here-x) <= MINIMUM_SEPARATION) {
	    text = NULL;
	    break;
	}
	userlabels = userlabels->next;
    }
#   undef MINIMUM_SEPARATION

    if (grid.l_type > LT_NODRAW) {
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	term_apply_lp_properties(&grid);
	if (this_axis->index == POLAR_AXIS) {
	    if (fabs(place) > largest_polar_circle)
		largest_polar_circle = fabs(place);
	    draw_polar_circle(place);
	} else {
	    legend_key *key = &keyT;
	    if (key->visible && x < key->bounds.xright && x > key->bounds.xleft
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
	(t->layer)(TERM_LAYER_END_GRID);
    }	/* End of grid code */


    /* we precomputed tic posn and text posn in global vars */
    if (x < clip_area->xleft || x > clip_area->xright)
	return;

    (*t->move) (x, tic_start);
    (*t->vector) (x, tic_start + ticsize);

    if (tic_mirror >= 0) {
	(*t->move) (x, tic_mirror);
	(*t->vector) (x, tic_mirror - ticsize);
    }

    /* If grid_tics_in_front, defer tic labels until LAYER_FOREGROUND */
    if (grid_tics_in_front && current_layer != LAYER_FOREGROUND)
	return;

    if (text) {
	/* get offset */
	double offsetx_d, offsety_d;
	map_position_r(&(this_axis->ticdef.offset),
		       &offsetx_d, &offsety_d, "xtics");
	/* User-specified different color for the tics text */
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	ignore_enhanced(!this_axis->ticdef.enhanced);
	write_multiline(x+(int)offsetx_d, tic_text+(int)offsety_d, text,
			tic_hjust, tic_vjust, rotate_tics,
			this_axis->ticdef.font);
	ignore_enhanced(FALSE);
	term_apply_lp_properties(&border_lp);	/* reset to border linetype */
    }
}

/* display a y-axis ticmark - called by gen_ticks */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
static void
ytick2d_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid,	/* grid.l_type == LT_NODRAW means no grid */
    struct ticmark *userlabels)	/* User-specified tic labels */
{
    struct termentry *t = term;
    /* minitick if text is NULL - v_tic is unsigned */
    int ticsize = tic_direction * (int) t->h_tic * tic_scale(ticlevel, this_axis);
    int y;

    if (this_axis->index >= PARALLEL_AXES)
	y = axis_map(this_axis, place);
    else
	y = map_y(place);

    /* Skip label if we've already written a user-specified one here */
#   define MINIMUM_SEPARATION 2
    while (userlabels) {
	int here = map_y(userlabels->position);
	if (abs(here-y) <= MINIMUM_SEPARATION) {
	    text = NULL;
	    break;
	}
	userlabels = userlabels->next;
    }
#   undef MINIMUM_SEPARATION

    if (grid.l_type > LT_NODRAW) {
	legend_key *key = &keyT;
	(t->layer)(TERM_LAYER_BEGIN_GRID);
	term_apply_lp_properties(&grid);
	/* Make the grid avoid the key box */
	if (key->visible && y < key->bounds.ytop && y > key->bounds.ybot
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
	term_apply_lp_properties(&border_lp);	/* border linetype */
	(t->layer)(TERM_LAYER_END_GRID);
    }

    /* we precomputed tic posn and text posn */
    (*t->move) (tic_start, y);
    (*t->vector) (tic_start + ticsize, y);

    if (tic_mirror >= 0) {
	(*t->move) (tic_mirror, y);
	(*t->vector) (tic_mirror - ticsize, y);
    }

    /* If grid_tics_in_front, defer tic labels until LAYER_FOREGROUND */
    if (grid_tics_in_front && current_layer != LAYER_FOREGROUND)
	return;

    if (text) {
	/* get offset */
	double offsetx_d, offsety_d;
	map_position_r(&(this_axis->ticdef.offset),
		       &offsetx_d, &offsety_d, "ytics");
	/* User-specified different color for the tics text */
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	ignore_enhanced(!this_axis->ticdef.enhanced);
	write_multiline(tic_text+(int)offsetx_d, y+(int)offsety_d, text,
			tic_hjust, tic_vjust, rotate_tics,
			this_axis->ticdef.font);
	ignore_enhanced(FALSE);
	term_apply_lp_properties(&border_lp);	/* reset to border linetype */
    }
}

/* called by gen_ticks to place ticmarks on perimeter of polar grid circle */
/* also uses global tic_start, tic_direction, tic_text and tic_just */
static void
ttick_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid,	/* grid.l_type == LT_NODRAW means no grid */
    struct ticmark *userlabels)	/* User-specified tic labels */
{
    int xl, yl; /* Inner limit of ticmark */
    int xu, yu; /* Outer limit of ticmark */
    int text_x, text_y;
    double delta = 0.05 * tic_scale(ticlevel, this_axis) * (this_axis->tic_in ? -1 : 1);
    double theta = (place * theta_direction + theta_origin) * DEG2RAD;
    double cos_t = largest_polar_circle * cos(theta);
    double sin_t = largest_polar_circle * sin(theta);

    /* Skip label if we've already written a user-specified one here */
    while (userlabels) {
	double here = userlabels->position;
	if (fabs(here - place) <= 0.02) {
	    text = NULL;
	    break;
	}
	userlabels = userlabels->next;
    }

    xl = map_x(0.95 * cos_t);
    yl = map_y(0.95 * sin_t);
    xu = map_x(cos_t);
    yu = map_y(sin_t);

    /* The normal meaning of "offset" as x/y displacement doesn't work well */
    /* for theta tic labels. Use it as a radial offset instead */
    text_x = xu + (xu-xl) * (2. + this_axis->ticdef.offset.x);
    text_y = yu + (yu-yl) * (2. + this_axis->ticdef.offset.x);

    xl = map_x( (1.+delta) * cos_t);
    yl = map_y( (1.+delta) * sin_t);
    if (this_axis->ticmode & TICS_MIRROR) {
	xu = map_x( (1.-delta) * cos_t);
	yu = map_y( (1.-delta) * sin_t);
    }

    draw_clip_line(xl, yl, xu, yu);

    /* If grid_tics_in_front, defer tic labels until LAYER_FOREGROUND */
    if (grid_tics_in_front && current_layer != LAYER_FOREGROUND)
	return;

    if (text && !clip_point(xu, yu)) {
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	/* The only rotation angle that makes sense is the angle being labeled */
	if (this_axis->tic_rotate != 0.0)
	    term->text_angle(place * theta_direction + theta_origin - 90.0);
	write_multiline(text_x, text_y, text,
		tic_hjust, tic_vjust, 0.0, /* FIXME: these are not correct */
		this_axis->ticdef.font);
	term_apply_lp_properties(&border_lp);
    }
}

/*{{{  map_position, wrapper, which maps double to int */
void
map_position(
    struct position *pos,
    int *x, int *y,
    const char *what)
{
    double xx=0, yy=0;
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
    case second_axes:
    default:
	{
	    AXIS_INDEX index = (pos->scalex == first_axes) ? FIRST_X_AXIS : SECOND_X_AXIS;
	    AXIS *this_axis = &axis_array[index];
	    AXIS *primary = this_axis->linked_to_primary;
	    if (primary && primary->link_udf->at) {
		double xx = eval_link_function(primary, pos->x);
		*x = axis_map(primary, xx);
	    } else {
		*x = axis_map(this_axis, pos->x);
	    }
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
    case polar_axes:
	{
	double xx, yy;
	    (void) polar_to_xy(pos->x, pos->y, &xx, &yy, FALSE);
	    *x = AXIS_MAP(FIRST_X_AXIS, xx);
	    *y = AXIS_MAP(FIRST_Y_AXIS, yy);
	    pos->scaley = polar_axes;	/* Just to make sure */
	    break;
	}
    }
    switch (pos->scaley) {
    case first_axes:
    case second_axes:
    default:
	{
	    AXIS_INDEX index = (pos->scaley == first_axes) ? FIRST_Y_AXIS : SECOND_Y_AXIS;
	    AXIS *this_axis = &axis_array[index];
	    AXIS *primary = this_axis->linked_to_primary;
	    if (primary && primary->link_udf->at) {
		double yy = eval_link_function(primary, pos->y);
		*y = axis_map(primary, yy);
	    } else {
		*y = axis_map(this_axis, pos->y);
	    }
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
    case polar_axes:
	    break;
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
    /* Catches the case of "first" or "second" coords on a log-scaled axis */
    if (pos->x == 0)
	*x = 0;
    else

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
    case polar_axes:
	    *x = 0;
	    break;
    }

    /* Maybe they only want one coordinate translated? */
    if (y == NULL)
	return;

    /* Catches the case of "first" or "second" coords on a log-scaled axis */
    if (pos->y == 0)
	*y = 0;
    else

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
    case polar_axes:
	    *y = 0;
	    break;
    }
}
/*}}} */

static void
plot_border()
{
    int min, max;

	TBOOLEAN border_complete = ((draw_border & 15) == 15);

	(*term->layer) (TERM_LAYER_BEGIN_BORDER);
	term_apply_lp_properties(&border_lp);	/* border linetype */
	if (border_complete)
	    newpath();

	/* Trace border anticlockwise from upper left */
	(*term->move) (plot_bounds.xleft, plot_bounds.ytop);

	if (border_west && axis_array[FIRST_Y_AXIS].ticdef.rangelimited) {
		y_axis = FIRST_Y_AXIS;
		max = map_y(axis_array[FIRST_Y_AXIS].data_max);
		min = map_y(axis_array[FIRST_Y_AXIS].data_min);
		(*term->move) (plot_bounds.xleft, max);
		(*term->vector) (plot_bounds.xleft, min);
		(*term->move) (plot_bounds.xleft, plot_bounds.ybot);
	} else if (border_west) {
	    (*term->vector) (plot_bounds.xleft, plot_bounds.ybot);
	} else {
	    (*term->move) (plot_bounds.xleft, plot_bounds.ybot);
	}

	if (border_south && axis_array[FIRST_X_AXIS].ticdef.rangelimited) {
		x_axis = FIRST_X_AXIS;
		max = map_x(axis_array[FIRST_X_AXIS].data_max);
		min = map_x(axis_array[FIRST_X_AXIS].data_min);
		(*term->move) (min, plot_bounds.ybot);
		(*term->vector) (max, plot_bounds.ybot);
		(*term->move) (plot_bounds.xright, plot_bounds.ybot);
	} else if (border_south) {
	    (*term->vector) (plot_bounds.xright, plot_bounds.ybot);
	} else {
	    (*term->move) (plot_bounds.xright, plot_bounds.ybot);
	}

	if (border_east && axis_array[SECOND_Y_AXIS].ticdef.rangelimited) {
		y_axis = SECOND_Y_AXIS;
		max = map_y(axis_array[SECOND_Y_AXIS].data_max);
		min = map_y(axis_array[SECOND_Y_AXIS].data_min);
		(*term->move) (plot_bounds.xright, min);
		(*term->vector) (plot_bounds.xright, max);
		(*term->move) (plot_bounds.xright, plot_bounds.ytop);
	} else if (border_east) {
	    (*term->vector) (plot_bounds.xright, plot_bounds.ytop);
	} else {
	    (*term->move) (plot_bounds.xright, plot_bounds.ytop);
	}

	if (border_north && axis_array[SECOND_X_AXIS].ticdef.rangelimited) {
		x_axis = SECOND_X_AXIS;
		max = map_x(axis_array[SECOND_X_AXIS].data_max);
		min = map_x(axis_array[SECOND_X_AXIS].data_min);
		(*term->move) (max, plot_bounds.ytop);
		(*term->vector) (min, plot_bounds.ytop);
		(*term->move) (plot_bounds.xright, plot_bounds.ytop);
	} else if (border_north) {
	    (*term->vector) (plot_bounds.xleft, plot_bounds.ytop);
	} else {
	    (*term->move) (plot_bounds.xleft, plot_bounds.ytop);
	}

	if (border_complete)
	    closepath();

	/* Polar border.  FIXME: Should this be limited to known R_AXIS.max? */
	if ((draw_border & 0x1000) != 0) {
	    lp_style_type polar_border = border_lp;
	    BoundingBox *clip_save = clip_area;
	    clip_area = &plot_bounds;

	    /* Full-width circular border is visually too heavy compared to the edges */
	    polar_border.l_width = polar_border.l_width / 2.;
	    term_apply_lp_properties(&polar_border);
	    draw_polar_circle(polar_radius(R_AXIS.max));
	    clip_area = clip_save;
	}

	(*term->layer) (TERM_LAYER_END_BORDER);
}


void
init_histogram(struct histogram_style *histogram, text_label *title)
{
    if (stackheight)
	free(stackheight);
    stackheight = NULL;
    if (histogram) {
	memcpy(histogram, &histogram_opts, sizeof(histogram_opts));
	memcpy(&histogram->title, title, sizeof(text_label));
	memset(title, 0, sizeof(text_label));
	/* Insert in linked list */
	histogram_opts.next = histogram;
    }
}

void
free_histlist(struct histogram_style *hist)
{
    if (!hist)
	return;
    if (hist != &histogram_opts) {
	free(hist->title.text);
	free(hist->title.font);
    }
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
    int x, y;

    while ((hist = hist->next)) {
	if (hist->title.text && *(hist->title.text)) {
	    double xoffset_d, yoffset_d;
	    map_position_r(&(histogram_opts.title.offset), &xoffset_d, &yoffset_d,
			   "histogram");
	    x = map_x((hist->start + hist->end) / 2.);
	    y = xlabel_y;
	    /* NB: offset in "newhistogram" is additive with that in "set style hist" */
	    x += (int)xoffset_d;
	    y += (int)yoffset_d + 0.25 * term->v_char;

	    write_label(x, y, &(hist->title));
	    reset_textcolor(&hist->title.textcolor);
	}
    }
}

/*
 * Draw a solid line for the polar axis.
 * If the center of the polar plot is not at zero (rmin != 0)
 * indicate this by drawing an open circle.
 */
static void
place_raxis()
{
    t_object raxis_circle = {
	NULL, 1, 1, OBJ_CIRCLE,	OBJ_CLIP, /* link, tag, layer (front), object_type, clip */
	{FS_SOLID, 100, 0, BLACK_COLORSPEC},
	{0, LT_BACKGROUND, 0, DASHTYPE_AXIS, 0, 0, 0.2, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN},
	{.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360. }}
    };
    int x0,y0, xend,yend;

    if (inverted_raxis) {
	xend = map_x(polar_radius(R_AXIS.set_min));
	x0   = map_x(polar_radius(R_AXIS.set_max));
    } else {
	double rightend = (R_AXIS.autoscale & AUTOSCALE_MAX) ? R_AXIS.max : R_AXIS.set_max;
	xend = map_x(rightend - R_AXIS.set_min);
	x0 = map_x(0);
    }
    yend = y0 = map_y(0);
    term_apply_lp_properties(&border_lp);
    draw_clip_line(x0,y0,xend,yend);

    if (!inverted_raxis)
    if (!(R_AXIS.autoscale & AUTOSCALE_MIN) && R_AXIS.set_min != 0)
	place_objects( &raxis_circle, LAYER_FRONT, 2);

}

static void
place_parallel_axes(struct curve_points *first_plot, int layer)
{
    struct curve_points *plot = first_plot;
    struct axis *this_axis;
    int j, axes_in_use = 0;

    /* Walk through the plots and prepare parallel axes as needed */
    for (plot = first_plot; plot; plot = plot->next) {
	if (plot->plot_style == PARALLELPLOT && plot->p_count > 0) {
	    axes_in_use = plot->p_axis;
	    this_axis = &parallel_axis_array[plot->p_axis - 1];
	    axis_invert_if_requested(this_axis);
	    this_axis->term_lower = plot_bounds.ybot;
	    this_axis->term_scale = (plot_bounds.ytop - plot_bounds.ybot)
				  / (this_axis->max - this_axis->min);
	    setup_tics(this_axis, 20);
	}
    }

    if (parallel_axis_style.layer == LAYER_FRONT && layer == LAYER_BACK)
	return;

    /* Draw the axis lines */
    term_apply_lp_properties(&parallel_axis_style.lp_properties);
    for (j = 1; j <= axes_in_use; j++) {
	struct axis *this_axis = &parallel_axis_array[j-1];
	int max = axis_map(this_axis, this_axis->data_max);
	int min = axis_map(this_axis, this_axis->data_min);
	int axis_x = map_x(this_axis->paxis_x);
	draw_clip_line( axis_x, min, axis_x, max );
    }

    /* Draw the axis tickmarks and labels.  Piggyback on ytick2d_callback */
    /* but avoid a call to the full axis_output_tics(). 		  */
    for (j = 1; j <= axes_in_use; j++) {
	struct axis *this_axis = &parallel_axis_array[j-1];
	double axis_coord = this_axis->paxis_x;

	if ((this_axis->ticmode & TICS_MASK) == NO_TICS)
	    continue;

	if (this_axis->tic_rotate && term->text_angle(this_axis->tic_rotate)) {
	    tic_hjust = LEFT;
	    tic_vjust = CENTRE;
	} else {
	    tic_hjust = CENTRE;
	    tic_vjust = JUST_TOP;
	}
	if (this_axis->manual_justify)
	    tic_hjust = this_axis->tic_pos;

	tic_start = axis_map(&axis_array[FIRST_X_AXIS], axis_coord);
	tic_mirror = tic_start; /* tic extends on both sides of axis */
	tic_direction = -1;
	tic_text = tic_start - this_axis->ticscale * term->v_tic;
	tic_text -= term->v_char;

	gen_tics(this_axis, ytick2d_callback);
	term->text_angle(0);
    }
}

/*
 * Label the curve by placing its title at one end of the curve.
 * This option is independent of the plot key, but uses the same
 * color/font/text options controlled by "set key".
 * This routine is shared by 2D and 3D plots.
 */
void
attach_title_to_plot(struct curve_points *this_plot, legend_key *key)
{
    struct coordinate *points;
    char *title;
    int npoints;
    int index, x, y;
    TBOOLEAN is_3D;

    if (this_plot->plot_type == NODATA || this_plot->plot_type == KEYENTRY)
	return;

    /* This routine handles both 2D and 3D plots */
    if (this_plot->plot_type == DATA3D || this_plot->plot_type == FUNC3D) {
	points = ((struct surface_points *)this_plot)->iso_crvs->points;
	npoints = ((struct surface_points *)this_plot)->iso_crvs->p_count;
	is_3D = TRUE;
    } else {
	points = this_plot->points;
	npoints = this_plot->p_count;
	is_3D = FALSE;
    }

    /* beginning or end of plot trace */
    if (this_plot->title_position->x > 0) {
	for (index = npoints-1; index > 0; index--)
	    if (points[index].type == INRANGE)
		break;
    } else {
	for (index=0; index < npoints-1; index++)
	    if (points[index].type == INRANGE)
		break;
    }
    if (points[index].type != INRANGE)
	return;

    if (is_3D) {
	map3d_xy(points[index].x, points[index].y, points[index].z, &x, &y);
    } else {
	x = map_x(points[index].x);
	y = map_y(points[index].y);
    }

    if (key->textcolor.type == TC_VARIABLE)
	/* Draw key text in same color as plot */
	;
    else if (key->textcolor.type != TC_DEFAULT)
	/* Draw key text in same color as key title */
	apply_pm3dcolor(&key->textcolor);
    else
	/* Draw key text in black */
	(*term->linetype)(LT_BLACK);

    title = this_plot->title;
    if (this_plot->title_is_automated && (term->flags & TERM_IS_LATEX))
	title = texify_title(title, this_plot->plot_type);

    write_multiline(x, y, title,
    	(JUSTIFY)this_plot->title_position->y,
	JUST_TOP, 0, key->font);
}

void
do_rectangle( int dimensions, t_object *this_object, fill_style_type *fillstyle )
{
    double x1, y1, x2, y2;
    int x, y;
    int style;
    unsigned int w, h;
    TBOOLEAN clip_x = FALSE;
    TBOOLEAN clip_y = FALSE;
    t_rectangle *this_rect = &this_object->o.rectangle;

	if (this_rect->type == 1) {	/* specified as center + size */
	    double width, height;

	    if (dimensions == 2 || this_rect->center.scalex == screen) {
		map_position_double(&this_rect->center, &x1, &y1, "rect");
		map_position_r(&this_rect->extent, &width, &height, "rect");
	    } else if (splot_map) {
		int junkw, junkh;
		map3d_position_double(&this_rect->center, &x1, &y1, "rect");
		map3d_position_r(&this_rect->extent, &junkw, &junkh, "rect");
		width = abs(junkw);
		height = abs(junkh);
	    } else
		return;

	    x1 -= width/2;
	    y1 -= height/2;
	    x2 = x1 + width;
	    y2 = y1 + height;
	    w = width;
	    h = height;
	    if (this_object->clip == OBJ_CLIP) {
		if (this_rect->extent.scalex == first_axes
		    ||  this_rect->extent.scalex == second_axes)
		    clip_x = TRUE;
		if (this_rect->extent.scaley == first_axes
		    ||  this_rect->extent.scaley == second_axes)
		    clip_y = TRUE;
	    }

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
	    if (this_object->clip == OBJ_CLIP) {
		if (this_rect->bl.scalex != screen && this_rect->tr.scalex != screen)
		    clip_x = TRUE;
		if (this_rect->bl.scaley != screen && this_rect->tr.scaley != screen)
		    clip_y = TRUE;
	    }
	}

	/* FIXME - Should there be a generic clip_rectangle() routine?	*/
	/* Clip to the graph boundaries, but only if the rectangle 	*/
	/* itself was specified in plot coords.				*/
	if (clip_area) {
	    BoundingBox *clip_save = clip_area;
	    clip_area = &plot_bounds;
	    if (clip_x) {
		cliptorange(x1, clip_area->xleft, clip_area->xright);
		cliptorange(x2, clip_area->xleft, clip_area->xright);
	    }
	    if (clip_y) {
		cliptorange(y1, clip_area->ybot, clip_area->ytop);
		cliptorange(y2, clip_area->ybot, clip_area->ytop);
	    }
	    clip_area = clip_save;
	}

	w = x2 - x1;
	h = y2 - y1;
	x = x1;
	y = y1;

	if (w == 0 || h == 0)
	    return;

	style = style_from_fill(fillstyle);
	if (style != FS_EMPTY && term->fillbox)
		(*term->fillbox) (style, x, y, w, h);

	/* Now the border */
	if (need_fill_border(fillstyle)) {
	    newpath();
	    (*term->move)   (x, y);
	    (*term->vector) (x, y+h);
	    (*term->vector) (x+w, y+h);
	    (*term->vector) (x+w, y);
	    (*term->vector) (x, y);
	    closepath();
	}

    return;
}

void
do_ellipse( int dimensions, t_ellipse *e, int style, TBOOLEAN do_own_mapping )
{
    gpiPoint vertex[120];
    int i, in;
    double angle;
    double cx, cy;
    double xoff, yoff;
    double junkfoo;
    int junkw, junkh;
    double cosO = cos(DEG2RAD * e->orientation);
    double sinO = sin(DEG2RAD * e->orientation);
    double A = e->extent.x / 2.0;	/* Major axis radius */
    double B = e->extent.y / 2.0;	/* Minor axis radius */
    struct position pos = e->extent;	/* working copy with axis info attached */
    double aspect = (double)term->v_tic / (double)term->h_tic;

    /* Choose how many segments to draw for this ellipse */
    int segments = 72;
    double ang_inc  =  M_PI / 36.;

#ifdef _WIN32
    if (strcmp(term->name, "windows") == 0)
	aspect = 1.;
#endif

    /* Find the center of the ellipse */
    /* If this ellipse is part of a plot - as opposed to an object -
     * then the caller plot_ellipses function already did the mapping for us.
     * Else we do it here. The 'ellipses' plot style is 2D only, but objects
     * can apparently be placed on splot maps too, so we do 3D mapping if needed. */
	if (!do_own_mapping) {
	    cx = e->center.x;
	    cy = e->center.y;
	}
	else if (dimensions == 2)
	    map_position_double(&e->center, &cx, &cy, "ellipse");
	else
	    map3d_position_double(&e->center, &cx, &cy, "ellipse");

    /* Calculate the vertices */
    for (i=0, angle = 0.0; i<=segments; i++, angle += ang_inc) {
	/* Given that the (co)sines of same sequence of angles
	 * are calculated every time - shouldn't they be precomputed
	 * and put into a table? */
	    pos.x = A * cosO * cos(angle) - B * sinO * sin(angle);
	    pos.y = A * sinO * cos(angle) + B * cosO * sin(angle);
	    if (!do_own_mapping) {
		xoff = pos.x;
		yoff = pos.y;

	    } else if (dimensions == 2) {
		switch (e->type) {
		case ELLIPSEAXES_XY:
		    map_position_r(&pos, &xoff, &yoff, "ellipse");
		    break;
		case ELLIPSEAXES_XX:
		    map_position_r(&pos, &xoff, NULL, "ellipse");
		    pos.x = pos.y;
		    map_position_r(&pos, &yoff, NULL, "ellipse");
		    break;
		case ELLIPSEAXES_YY:
		    map_position_r(&pos, &junkfoo, &yoff, "ellipse");
		    pos.y = pos.x;
		    map_position_r(&pos, &junkfoo, &xoff, "ellipse");
		    break;
		}

	    } else {
		switch (e->type) {
		case ELLIPSEAXES_XY:
		    map3d_position_r(&pos, &junkw, &junkh, "ellipse");
		    xoff = junkw;
		    yoff = junkh;
		    break;
		case ELLIPSEAXES_XX:
		    map3d_position_r(&pos, &junkw, &junkh, "ellipse");
		    xoff = junkw;
		    pos.x = pos.y;
		    map3d_position_r(&pos, &junkh, &junkw, "ellipse");
		    yoff = junkh;
		    break;
		case ELLIPSEAXES_YY:
		    map3d_position_r(&pos, &junkw, &junkh, "ellipse");
		    yoff = junkh;
		    pos.y = pos.x;
		    map3d_position_r(&pos, &junkh, &junkw, "ellipse");
		    xoff = junkw;
		    break;
		}
	    }

	    vertex[i].x = cx + xoff;
	    if (!do_own_mapping)
		vertex[i].y = cy + yoff * aspect;
	    else
		vertex[i].y = cy + yoff;
    }

    if (style) {
	/* Fill in the center */
	gpiPoint fillarea[120];
	clip_polygon(vertex, fillarea, segments, &in);
	fillarea[0].style = style;
	if ((in > 1) && term->filled_polygon)
	    term->filled_polygon(in, fillarea);
    } else {
	/* Draw the arc */
	draw_clip_polygon(segments+1, vertex);
    }
}

void
do_polygon( int dimensions, t_object *this_object, int style, int facing )
{
    t_polygon *p = &this_object->o.polygon;
    t_clip_object clip = this_object->clip;
    static gpiPoint *corners = NULL;
    static gpiPoint *clpcorn = NULL;
    BoundingBox *clip_save = clip_area;
    int vertices = p->type;
    int nv;

    if (!p->vertex || vertices < 2)
	return;

    corners = gp_realloc(corners, vertices * sizeof(gpiPoint), "polygon");
    clpcorn = gp_realloc(clpcorn, 2 * vertices * sizeof(gpiPoint), "polygon");

    for (nv = 0; nv < vertices; nv++) {
	if (dimensions == 3)
	    map3d_position(&p->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");
	else
	    map_position(&p->vertex[nv], &corners[nv].x, &corners[nv].y, "pvert");

	/* Any vertex given in screen coords will disable clipping */
	if (p->vertex[nv].scalex == screen || p->vertex[nv].scaley == screen)
	    clip = OBJ_NOCLIP;
    }

    /* Do we require this polygon to face front or back? */
    if (dimensions == 3 && facing >= 0) {
	double v1[2], v2[2], cross_product;
	v1[0] = corners[1].x - corners[0].x;
	v1[1] = corners[1].y - corners[0].y;
	v2[0] = corners[vertices-2].x - corners[0].x;
	v2[1] = corners[vertices-2].y - corners[0].y;
	cross_product = v1[0]*v2[1] - v1[1]*v2[0];
	if (facing == LAYER_FRONT && cross_product > 0)
	    return;
	if (facing == LAYER_BACK && cross_product < 0)
	    return;
    }

    if (clip == OBJ_NOCLIP)
	clip_area = &canvas;

    if (term->filled_polygon && style) {
	int out_length;
	clip_polygon(corners, clpcorn, nv, &out_length);
	clpcorn[0].style = style;

	if ((this_object->layer == LAYER_DEPTHORDER) && (vertices < 12)) {
	    /* FIXME - size arbitrary limit */
	    gpdPoint quad[12];
	    for (nv=0; nv < vertices; nv++) {
		quad[nv].x = p->vertex[nv].x;
		quad[nv].y = p->vertex[nv].y;
		quad[nv].z = p->vertex[nv].z;
	    }
	    /* Allow 2-sided coloring */
	    /* FIXME: this assumes pm3d_color.type == TC_RGB
	     *        if type == TC_LT instead this will come out off-white
	     */
	    quad[0].c = this_object->lp_properties.pm3d_color.lt;
	    if (this_object->lp_properties.pm3d_color.type == TC_LINESTYLE) {
		int base_color = this_object->lp_properties.pm3d_color.lt;
		struct coordinate triangle[3];
		struct lp_style_type face;
		int side;
		int t;
		for (t=0; t<3; t++) {
		    triangle[t].x = quad[t].x;
		    triangle[t].y = quad[t].y;
		    triangle[t].z = quad[t].z;
		}
		/* NB: This is sensitive to the order of the vertices */
		side = pm3d_side( &(triangle[0]), &(triangle[1]), &(triangle[2]) );
		lp_use_properties(&face, side < 0 ? base_color+1 : base_color);
		quad[0].c = face.pm3d_color.lt;
	    }

	    /* FIXME: could we pass through a per-quadrangle border style also? */
	    quad[1].c = style;
	    pm3d_add_polygon( NULL, quad, vertices );

	} else { /* Not depth-sorted; draw it now */
	    if (out_length > 1)
		term->filled_polygon(out_length, clpcorn);
	}

    } else { /* Just draw the outline? */
 	newpath();
	draw_clip_polygon(nv, corners);
	closepath();
    }

    clip_area = clip_save;
}

TBOOLEAN
check_for_variable_color(struct curve_points *plot, double *colorvalue)
{
    if (!plot->varcolor)
	return FALSE;

    if ((plot->lp_properties.pm3d_color.value < 0.0)
    &&  (plot->lp_properties.pm3d_color.type == TC_RGB)) {
	set_rgbcolor_var(*colorvalue);
	return TRUE;
    } else if (plot->lp_properties.pm3d_color.type == TC_Z) {
	set_color( cb2gray(*colorvalue) );
	return TRUE;
    } else if (plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
	lp_style_type lptmp;
	/* lc variable will only pick up line _style_ as opposed to _type_ */
	/* in the case of "set style increment user".  THIS IS A CHANGE.   */
	if (prefer_line_styles)
	    lp_use_properties(&lptmp, (int)(*colorvalue));
	else
	    load_linetype(&lptmp, (int)(*colorvalue));
	apply_pm3dcolor(&(lptmp.pm3d_color));
	return TRUE;
    } else
	return FALSE;
}

/* rgbscale
 * RGB image color components are normally in the range [0:255] but some
 * data conventions may use [0:1] instead.  This does the conversion.
 */
static double
rgbscale( double component )
{
    if (rgbmax != 255.)
	component = 255. * component/rgbmax;
    return component > 255 ? 255 : component < 0 ? 0 : component;
}

/* process_image:
 *
 * IMG_PLOT - Plot the coordinates similar to the points option except
 *  use pixels.  Check if the data forms a valid image array, i.e., one
 *  for which points are spaced equidistant along two non-coincidence
 *  vectors.  If the two directions are orthogonal within some tolerance
 *  and they are aligned with the view box x and y directions, then use
 *  the image feature of the terminal if it has one.  Otherwise, use
 *  parallelograms via the polynomial function to represent pixels.
 *
 * IMG_UPDATE_AXES - Update the axis ranges for `set autoscale` and then
 *  return.
 *
 * IMG_UPDATE_CORNERS - Update the corners of the outlining phantom
 *  parallelogram for `set hidden3d` and then return.
 */
void
process_image(void *plot, t_procimg_action action)
{

    struct coordinate *points;
    int p_count;
    int i;
    double p_start_corner[2], p_end_corner[2]; /* Points used for computing hyperplane. */
    int K = 0, L = 0;                          /* Dimensions of image grid. K = <scan line length>, L = <number of scan lines>. */
    unsigned int ncols, nrows;		        /* EAM DEBUG - intended to replace K and L above */
    double p_mid_corner[2];                    /* Point representing first corner found, i.e. p(K-1) */
    double delta_x_grid[2] = {0, 0};           /* Spacings between points, two non-orthogonal directions. */
    double delta_y_grid[2] = {0, 0};
    int grid_corner[4] = {-1, -1, -1, -1};     /* The corner pixels of the image. */
    double view_port_x[2];                     /* Viewable portion of the image. */
    double view_port_y[2];
    double view_port_z[2] = {0,0};
    t_imagecolor pixel_planes;

    /* Detours necessary to handle 3D plots */
    TBOOLEAN project_points = FALSE;		/* True if 3D plot */
    int image_x_axis, image_y_axis;

    if (((struct surface_points *)plot)->plot_type == NODATA) {
	int_warn(NO_CARET, "no image data");
	return;
    }

    if ((((struct surface_points *)plot)->plot_type == DATA3D)
    ||  (((struct surface_points *)plot)->plot_type == FUNC3D))
	project_points = TRUE;

    if (project_points) {
	points = ((struct surface_points *)plot)->iso_crvs->points;
	p_count = ((struct surface_points *)plot)->iso_crvs->p_count;
	pixel_planes = ((struct surface_points *)plot)->image_properties.type;
	ncols = ((struct surface_points *)plot)->image_properties.ncols;
	nrows = ((struct surface_points *)plot)->image_properties.nrows;
	image_x_axis = FIRST_X_AXIS;
	image_y_axis = FIRST_Y_AXIS;
    } else {
	points = ((struct curve_points *)plot)->points;
	p_count = ((struct curve_points *)plot)->p_count;
	pixel_planes = ((struct curve_points *)plot)->image_properties.type;
	ncols = ((struct curve_points *)plot)->image_properties.ncols;
	nrows = ((struct curve_points *)plot)->image_properties.nrows;
	image_x_axis = ((struct curve_points *)plot)->x_axis;
	image_y_axis = ((struct curve_points *)plot)->y_axis;
    }

    if (p_count < 1) {
	int_warn(NO_CARET, "No points (visible or invisible) to plot.\n\n");
	return;
    }

    if (p_count < 4) {
	int_warn(NO_CARET, "Image grid must be at least 4 points (2 x 2).\n\n");
	return;
    }

    if (project_points && (X_AXIS.log || Y_AXIS.log || Z_AXIS.log)) {
	int_warn(NO_CARET, "Log scaling of 3D image plots is not supported");
	return;
    }


    /* Check if the pixel data forms a valid rectangular grid for potential image
     * matrix support.  A general grid orientation is considered.  If the grid
     * points are orthogonal and oriented along the x/y dimensions the terminal
     * function for images will be used.  Otherwise, the terminal function for
     * filled polygons are used to construct parallelograms for the pixel elements.
     */
    /* Catch pathological cases */
    if (points[0].type == UNDEFINED || points[p_count-1].type == UNDEFINED)
	int_error(NO_CARET, "image coordinates undefined");

    if (project_points) {
	map3d_xy_double(points[0].x, points[0].y, points[0].z,
			&p_start_corner[0], &p_start_corner[1]);
	map3d_xy_double(points[p_count-1].x, points[p_count-1].y, points[p_count-1].z,
			&p_end_corner[0], &p_end_corner[1]);
    } else {
	p_start_corner[0] = points[0].x;
	p_start_corner[1] = points[0].y;
	p_end_corner[0] = points[p_count-1].x;
	p_end_corner[1] = points[p_count-1].y;
    }

    /* Catch pathological cases */
    if (isnan(p_start_corner[0]) || isnan(p_end_corner[0])
    ||  isnan(p_start_corner[1]) || isnan(p_end_corner[1]))
	int_error(NO_CARET, "image coordinates undefined");

    /* This is a vestige of older code that calculated K and L on the fly	*/
    /* rather than keeping track of matrix/array/image dimensions on input	*/
    K = ncols;
    L = nrows;

    /* FIXME: We don't track the dimensions of image data provided as x/y/value	*/
    /* with individual coords rather than via array, matrix, or image format.	*/
    /* This might better be done when the data is entered rather than here.	*/
    if (L == 0 || K == 0) {
	if (points[0].x == points[1].x) {
	    /* y coord varies fastest */
	    for (K = 0; points[K].x == points[0].x; K++)
		    if (K >= p_count)
			    break;
	    L = p_count / K;
	} else {
	    /* x coord varies fastest */
	    for (K = 0; points[K].y == points[0].y; K++)
		    if (K >= p_count)
			    break;
	    L = p_count / K;
	}
	FPRINTF((stderr, "No dimension information for %d pixels total. Trying %d x %d\n",
		p_count, L, K));
    }

    grid_corner[0] = 0;
    grid_corner[1] = K-1;
    grid_corner[3] = p_count - 1;
    grid_corner[2] = p_count - K;

    if (action == IMG_UPDATE_AXES) {
	for (i=0; i < 4; i++) {
	    coord_type dummy_type;
	    double x,y;

	    x = points[grid_corner[i]].x;
	    y = points[grid_corner[i]].y;
	    x -= (points[grid_corner[(5-i)%4]].x - points[grid_corner[i]].x)/(2*(K-1));
	    y -= (points[grid_corner[(5-i)%4]].y - points[grid_corner[i]].y)/(2*(K-1));
	    x -= (points[grid_corner[(i+2)%4]].x - points[grid_corner[i]].x)/(2*(L-1));
	    y -= (points[grid_corner[(i+2)%4]].y - points[grid_corner[i]].y)/(2*(L-1));

	    /* Update range and store value back into itself. */
	    dummy_type = INRANGE;
	    STORE_AND_UPDATE_RANGE(x, x, dummy_type, image_x_axis,
				((struct curve_points *)plot)->noautoscale, x = -VERYLARGE);
	    dummy_type = INRANGE;
	    STORE_AND_UPDATE_RANGE(y, y, dummy_type, image_y_axis,
				((struct curve_points *)plot)->noautoscale, y = -VERYLARGE);
	}
	return;
    }

    if (action == IMG_UPDATE_CORNERS) {

	/* Shortcut pointer to phantom parallelogram. */
	struct iso_curve *iso_crvs = ((struct surface_points *)plot)->next_sp->iso_crvs;

	/* Set the phantom parallelogram as an outline of the image.  Use
	 * corner point 0 as a reference point.  Imagine vectors along the
	 * generally non-orthogonal directions of the two nearby corners. */

	double delta_x_1 = (points[grid_corner[1]].x - points[grid_corner[0]].x)/(2*(K-1));
	double delta_y_1 = (points[grid_corner[1]].y - points[grid_corner[0]].y)/(2*(K-1));
	double delta_z_1 = (points[grid_corner[1]].z - points[grid_corner[0]].z)/(2*(K-1));
	double delta_x_2 = (points[grid_corner[2]].x - points[grid_corner[0]].x)/(2*(L-1));
	double delta_y_2 = (points[grid_corner[2]].y - points[grid_corner[0]].y)/(2*(L-1));
	double delta_z_2 = (points[grid_corner[2]].z - points[grid_corner[0]].z)/(2*(L-1));

	iso_crvs->points[0].x = points[grid_corner[0]].x - delta_x_1 - delta_x_2;
	iso_crvs->points[0].y = points[grid_corner[0]].y - delta_y_1 - delta_y_2;
	iso_crvs->points[0].z = points[grid_corner[0]].z - delta_z_1 - delta_z_2;
	iso_crvs->next->points[0].x = points[grid_corner[2]].x - delta_x_1 + delta_x_2;
	iso_crvs->next->points[0].y = points[grid_corner[2]].y - delta_y_1 + delta_y_2;
	iso_crvs->next->points[0].z = points[grid_corner[2]].z - delta_z_1 + delta_z_2;
	iso_crvs->points[1].x = points[grid_corner[1]].x + delta_x_1 - delta_x_2;
	iso_crvs->points[1].y = points[grid_corner[1]].y + delta_y_1 - delta_y_2;
	iso_crvs->points[1].z = points[grid_corner[1]].z + delta_z_1 - delta_z_2;
	iso_crvs->next->points[1].x = points[grid_corner[3]].x + delta_x_1 + delta_x_2;
	iso_crvs->next->points[1].y = points[grid_corner[3]].y + delta_y_1 + delta_y_2;
	iso_crvs->next->points[1].z = points[grid_corner[3]].z + delta_z_1 + delta_z_2;

	return;
    }

    if (project_points) {
	map3d_xy_double(points[K-1].x, points[K-1].y, points[K-1].z,
			&p_mid_corner[0], &p_mid_corner[1]);
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
	/* int_warn(NO_CARET, "This terminal does not support palette-based images.\n\n"); */
	return;
    }
    if ((pixel_planes == IC_RGB || pixel_planes == IC_RGBA)
    &&  ((term->flags & TERM_NULL_SET_COLOR))) {
	/* int_warn(NO_CARET, "This terminal does not support rgb images.\n\n"); */
	return;
    }
    /* Use generic code to handle alpha channel if the terminal can't */
    if (pixel_planes == IC_RGBA && !(term->flags & TERM_ALPHA_CHANNEL))
	fallback = TRUE;

    /* Also use generic code if the pixels are of unequal size, e.g. log scale */
    if (X_AXIS.log || Y_AXIS.log)
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
	    /* 3D plots do not use the term_scale mechanism */
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

		/* This of course should not happen, but if an improperly constructed
		 * input file presents more data than expected, the extra data can
		 * cause this overflow.
		 */
		if (i_image >= p_count || i_image < 0)
		    int_error(NO_CARET, "Unexpected line of data in matrix encountered");

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
			    int_warn(NO_CARET, "Visible pixel grid has a scan line longer than previous scan lines.");
			    return;
			}
		    }

		    /* This can happen if the data supplied for a matrix does not */
		    /* match the matrix dimensions found when the file was opened */
		    if (i_sub_image >= array_size) {
			int_warn(NO_CARET,"image data corruption");
			break;
		    }

		    pixel_M_N = i_image;

		    if (pixel_planes == IC_PALETTE) {
			image[i_sub_image++] = cb2gray( points[i_image].CRD_COLOR );
		    } else {
			image[i_sub_image++] = rgbscale(points[i_image].CRD_R) / 255.0;
			image[i_sub_image++] = rgbscale(points[i_image].CRD_G) / 255.0;
			image[i_sub_image++] = rgbscale(points[i_image].CRD_B) / 255.0;
			if (pixel_planes == IC_RGBA)
			    image[i_sub_image++] = rgbscale(points[i_image].CRD_A);
		    }

		}

		i_image += i_delta_pixel;
		j--;
		if (j == 0) {
		    if (M == 0)
			M = line_pixel_count;
		    else if ((line_pixel_count > 0) && (line_pixel_count != M)) {
			int_warn(NO_CARET, "Visible pixel grid has a scan line shorter than previous scan lines.");
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

		(*term->image) (M, N, image, corners, pixel_planes);
	    }

	    free ((void *)image);

	} else {
	    int_warn(NO_CARET, "Could not allocate memory for image.");
	    return;
	}

    } else {	/* no term->image  or "with image pixels" */

	/* Use sum of vectors to compute the pixel corners with respect to its center. */
	struct {double x; double y; double z;} delta_grid[2], delta_pixel[2];
	int j, i_image;

	/* If the pixels are rectangular we will call term->fillbox   */
	/* non-rectangular pixels must be treated as general polygons */
	if (!term->filled_polygon && !rectangular_image)
	    int_error(NO_CARET, "This terminal does not support filled polygons");

	(term->layer)(TERM_LAYER_BEGIN_IMAGE);

	/* Grid spacing in 3D space. */
	delta_grid[0].x = (points[grid_corner[1]].x - points[grid_corner[0]].x) / (K-1);
	delta_grid[0].y = (points[grid_corner[1]].y - points[grid_corner[0]].y) / (K-1);
	delta_grid[0].z = (points[grid_corner[1]].z - points[grid_corner[0]].z) / (K-1);
	delta_grid[1].x = (points[grid_corner[2]].x - points[grid_corner[0]].x) / (L-1);
	delta_grid[1].y = (points[grid_corner[2]].y - points[grid_corner[0]].y) / (L-1);
	delta_grid[1].z = (points[grid_corner[2]].z - points[grid_corner[0]].z) / (L-1);

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
		    gpiPoint corners[8];  /* At most 5 corners. */
		    gpiPoint clipped[8];  /* used during clipping */

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
			    if (rectangular_image && term->fillbox
			    &&  (corners_in_view < 4) &&  clip_area) {
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
			    if ((points[i_image].type == UNDEFINED)
			    ||  (isnan(points[i_image].CRD_COLOR))) {
				/* EAM April 2012 Distinguish +/-Inf from NaN */
			    	FPRINTF((stderr,"undefined pixel value %g\n",
					points[i_image].CRD_COLOR));
				if (isnan(points[i_image].CRD_COLOR))
					goto skip_pixel;
			    }
			    set_color( cb2gray(points[i_image].CRD_COLOR) );
			} else {
			    int r = rgbscale(points[i_image].CRD_R);
			    int g = rgbscale(points[i_image].CRD_G);
			    int b = rgbscale(points[i_image].CRD_B);
			    int rgblt = (r << 16) + (g << 8) + b;
			    set_rgbcolor_var(rgblt);
			}
			if (pixel_planes == IC_RGBA) {
			    int alpha = rgbscale(points[i_image].CRD_A) * 100./255.;
			    if (alpha <= 0)
				goto skip_pixel;
			    if (alpha > 100)
				alpha = 100;
			    if (term->flags & TERM_ALPHA_CHANNEL)
				corners[0].style = FS_TRANSPARENT_SOLID + (alpha<<4);
			}

			/* Clip to x/y in 2D projection */
			if (!project_points || splot_map) {
			    for (k=0; k<N_corners; k++)
				clipped[k] = corners[k];
			    clip_polygon(clipped, corners, N_corners, &N_corners);
			}

			if (rectangular_image && term->fillbox
			&&  !(term->flags & TERM_POLYGON_PIXELS)) {
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

	(term->layer)(TERM_LAYER_END_IMAGE);
    }
    }

}


/*
 * Draw one circle of the polar grid or border
 * NB: place is in x-axis coordinates
 */
static void
draw_polar_circle(double place)
{
    double x, y, angle;
    double step = 2.5;
    int ogx, ogy, gx, gy;

    x = place;
    y = 0.0;
    ogx = map_x(x);
    ogy = map_y(y);
    for ( angle = step; angle <= 360.; angle += step) {
	x = place * cos(angle*DEG2RAD);
	y = place * sin(angle*DEG2RAD);
	gx = map_x(x);
	gy = map_y(y);
	draw_clip_line(ogx, ogy, gx, gy);
	ogx = gx;
	ogy = gy;
    }
}

static void
place_spiderplot_axes(struct curve_points *first_plot, int layer)
{
    struct curve_points *plot = first_plot;
    struct axis *this_axis;
    int j, n_spokes = 0;

    if (!spiderplot)
	return;

    /* Walk through the plots and adjust axis min/max as needed */
    for (plot = first_plot; plot; plot = plot->next) {
	if (plot->plot_style != SPIDERPLOT)
	    continue;
	if (plot->p_count == 0)
	    continue;
	n_spokes = plot->p_axis;
	if (n_spokes > num_parallel_axes)
	    int_error(NO_CARET, "attempt to draw undefined radial axis");
	this_axis = &parallel_axis_array[plot->p_axis - 1];
	setup_tics(this_axis, 20);
	/* Use plot title to label the corresponding radial axis */
	if (plot->title) {
	    free(this_axis->label.text);
	    this_axis->label.text = strdup(plot->title);
	}
    }

    /* This should never happen if there really are spiderplots,
     * but we have left open the possibility that other plot types
     * could be mapped onto the radial axes.
     * That case does not yet have support in place.
     */
    if (n_spokes == 0 || parallel_axis_array == NULL)
	return;

    /* Place the grid lines */
    if (grid_spiderweb && layer == LAYER_BACK) {
	this_axis = &parallel_axis_array[0];
	this_axis->gridmajor = TRUE;
	term_apply_lp_properties(&grid_lp);
	/* copy n_spokes somewhere that spidertick_callback can see it */
	this_axis->term_zero = n_spokes;
	this_axis->ticdef.rangelimited = FALSE;
	gen_tics(this_axis, spidertick_callback);
	this_axis->gridmajor = FALSE;
    }

    if (parallel_axis_style.layer == LAYER_FRONT && layer == LAYER_BACK)
	return;

    /* Draw the axis lines */
    for (j = 1; j <= n_spokes; j++) {
	coordval theta = M_PI_2 - (j - 1) * 2*M_PI / n_spokes;

	/* axis linestyle can be customized */
	this_axis = &parallel_axis_array[j - 1];
	if (this_axis->zeroaxis)
	    term_apply_lp_properties(this_axis->zeroaxis);
	else
	    term_apply_lp_properties(&parallel_axis_style.lp_properties);

	polar_to_xy(theta, 0.0, &spoke_x0, &spoke_y0, FALSE);
	polar_to_xy(theta, 1.0, &spoke_x1, &spoke_y1, FALSE);
	draw_clip_line( map_x(spoke_x0), map_y(spoke_y0), map_x(spoke_x1), map_y(spoke_y1) );

	/* Draw the tickmarks and labels */
	if (this_axis->ticmode) {
	    spoke_dx = (spoke_y0 - spoke_y1) * .02;
	    spoke_dy = (spoke_x1 - spoke_x0) * .02;
	    /* FIXME: separate control of tic linewidth? */
	    term_apply_lp_properties(&border_lp);
	    this_axis->ticdef.rangelimited = FALSE;
	    gen_tics(this_axis, spidertick_callback);
	}

	/* Draw the axis label */
	/* Interpret any requested offset as a radial offset */
	if (this_axis->label.text) {
	    double radial_offset = this_axis->label.offset.x;
	    this_axis->label.offset.x = 0.0;
	    write_label( map_x(spoke_x1 + (1. + radial_offset) * 0.12 * (spoke_x1-spoke_x0)),
			 map_y(spoke_y1 + (1. + radial_offset) * 0.12 * (spoke_y1-spoke_y0)),
			 &this_axis->label );
	    this_axis->label.offset.x = radial_offset;
	}
    }
}

static void
spidertick_callback(struct axis *axis, double place, char *text, int ticlevel,
                    struct lp_style_type grid, struct ticmark *userlabels)
{
    double fraction = (place - axis->min) / (axis->max - axis->min);
    double tic_x = fraction * (spoke_x1 - spoke_x0);
    double tic_y = fraction * (spoke_y1 - spoke_y0);
    double ticsize = tic_scale(ticlevel, axis);

    if (fraction <= 0)
	return;

    /* This is an awkward place to draw the grid, but due to the general
     * mechanism of calculating tick positions via callback it is the only
     * place we know the desired radial position of the grid lines.
     */
    if (grid_spiderweb && axis->gridmajor && (grid_lp.l_type != LT_NODRAW)) {
	int n_spokes = axis->term_zero;
	gpiPoint *corners = gp_alloc((n_spokes+1) * sizeof(gpiPoint), "polygon");
	double x, y;
	int i;

	for (i = 0; i < n_spokes; i++) {
	    double theta = M_PI_2 - 2*M_PI * (double)i / (double)n_spokes;
	    polar_to_xy( theta, fraction, &x, &y, FALSE);
	    corners[i].x = map_x(x);
	    corners[i].y = map_y(y);
	}
	corners[n_spokes].x = corners[0].x;
	corners[n_spokes].y = corners[0].y;
	for (i = 0; i < n_spokes; i++)
	    draw_clip_line( corners[i].x, corners[i].y, corners[i+1].x, corners[i+1].y );

	free(corners);
	return;
    }

    /* Draw tick mark itself */
    draw_clip_line( map_x(tic_x - ticsize*spoke_dx), map_y(tic_y - ticsize*spoke_dy),
		    map_x(tic_x + ticsize*spoke_dx), map_y(tic_y + ticsize*spoke_dy));

    /* Draw tick label */
    if (text) {
	double offsetx_d, offsety_d;
	int tic_label_x = map_x(tic_x - (4.+ticsize)*spoke_dx);
	int tic_label_y = map_y(tic_y - (4.+ticsize)*spoke_dy);

	map_position_r(&(axis->ticdef.offset), &offsetx_d, &offsety_d, "");
	if (axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(axis->ticdef.textcolor));
	ignore_enhanced(!axis->ticdef.enhanced);
	write_multiline(tic_label_x + (int)offsetx_d, tic_label_y + (int)offsety_d,
			text,
			CENTRE, JUST_CENTRE, axis->tic_rotate,
			axis->ticdef.font);
	ignore_enhanced(FALSE);

	/* FIXME:  the plan is to have a separate lp for spiderplot tics */
	if (axis->ticdef.textcolor.type != TC_DEFAULT)
	    term_apply_lp_properties(&border_lp);
    }
}
