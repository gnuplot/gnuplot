#ifndef lint
static char *RCSid() { return RCSid("$Id: graph3d.c,v 1.23.2.8 2000/10/23 18:57:54 joze Exp $"); }
#endif

/* GNUPLOT - graph3d.c */

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
#include "graphics.h"
#include "hidden3d.h"
#include "misc.h"
#include "setshow.h"
#include "term_api.h"
#include "util3d.h"
#include "util.h"

#ifdef PM3D
#   include "pm3d.h"
#   include "color.h"
#endif

static int p_height;
static int p_width;		/* pointsize * t->h_tic */
static int key_entry_height;	/* bigger of t->v_size, pointsize*t->v_tick */

int suppressMove = 0;		/* to prevent moveto while drawing contours */

/*
 * hidden_no_update - if TRUE lines will be hidden line removed but they
 *   are not assumed to be part of the surface (i.e. grid) and therefore
 *   do not influence the hidings.
 * hidden_active - TRUE if hidden lines are to be removed.
 */
int hidden_active = FALSE;
int hidden_no_update;		

/* LITE defines a restricted memory version for MS-DOS, which doesn't
 * use the routines in hidden3d.c
 */

/* HBB 990826: hidden_line_type\(above|below\) were unused --> purged */

double xscale3d, yscale3d, zscale3d;

#ifdef PM3D
typedef enum { ALLGRID, FRONTGRID, BACKGRID } WHICHGRID;
WHICHGRID whichgrid = ALLGRID;
#endif

#ifndef PM3D
/* *not* static, we use it in color.c (joze) */
/* HBB 20000813: provide either static prototype here, or non-static in graph3d.h */
static void map3d_xy __PROTO((double x, double y, double z, unsigned int *xt, unsigned int *yt));
#endif

static void plot3d_impulses __PROTO((struct surface_points * plot));
static void plot3d_points __PROTO((struct surface_points * plot, int p_type));
static void plot3d_lines __PROTO((struct surface_points * plot));
#ifdef PM3D
/* no pm3d for impulses */
static void plot3d_lines_pm3d __PROTO((struct surface_points * plot));
static void plot3d_points_pm3d __PROTO((struct surface_points * plot, int p_type));
#endif
static void cntr3d_impulses __PROTO((struct gnuplot_contours * cntr,
				     struct surface_points * plot));
static void cntr3d_lines __PROTO((struct gnuplot_contours * cntr));
static void cntr3d_points __PROTO((struct gnuplot_contours * cntr,
				   int p_type));
static void check_corner_height __PROTO((struct coordinate GPHUGE * point,
					 double height[2][2], double depth[2][2]));
static void draw_bottom_grid __PROTO((struct surface_points * plot,
				      int plot_count));
static void xtick_callback __PROTO((int axis, double place, char *text,
				    struct lp_style_type grid));
static void ytick_callback __PROTO((int axis, double place, char *text,
				    struct lp_style_type grid));
static void ztick_callback __PROTO((int axis, double place, char *text,
				    struct lp_style_type grid));
static void setlinestyle __PROTO((struct lp_style_type style));

static void boundary3d __PROTO((TBOOLEAN scaling, struct surface_points * plots,
				int count));
#if 0				/* not used */
static double dbl_raise __PROTO((double x, int y));
#endif

/* put entries in the key */
static void key_sample_line __PROTO((int xl, int yl));
static void key_sample_point __PROTO((int xl, int yl, int pointtype));
static void key_text __PROTO((int xl, int yl, char *text));

static void get_arrow3d(struct arrow_def* arrow, unsigned int* sx, unsigned int* sy, unsigned int* ex, unsigned int* ey);
static int map3d_getposition __PROTO((struct position* pos, const char* what, double* xpos, double* ypos, double* zpos));
static void map3d_position_r __PROTO((struct position* pos1, struct position* pos2, unsigned int* x, unsigned int* y, const char* what));

/*
 * The Amiga SAS/C 6.2 compiler moans about macro envocations causing
 * multiple calls to functions. I converted these macros to inline
 * functions coping with the problem without loosing speed.
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
    return (max(a, b));
}

GP_INLINE static double
f_min(double a, double b)
{
    return (min(a, b));
}

#else
# define f_max(a,b) GPMAX((a),(b))
# define f_min(a,b) GPMIN((a),(b))
# define i_inrange(z,a,b) inrange((z),(a),(b))
#endif

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
static int ktitle_lines = 0;


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
#ifndef PM3D /* used in color.c */
static
#endif
double ceiling_z, base_z;

/* HBB 990826: {min|max}3d_z #defines, and transform_matrix typedef
 * moved to grahp3d.h */

transform_matrix trans_mat;

static double xaxis_y, yaxis_x, zaxis_x, zaxis_y;

/* the co-ordinates of the back corner */
static double back_x, back_y;

#ifdef USE_MOUSE
int axis3d_o_x, axis3d_o_y, axis3d_x_dx, axis3d_x_dy, axis3d_y_dx, axis3d_y_dy;
#endif

/* the penalty for convenience of using tic_gen to make callbacks
 * to tick routines is that we cant pass parameters very easily.
 * We communicate with the tick_callbacks using static variables
 */

/* unit vector (terminal coords) */
static double tic_unitx, tic_unity;

/* (DFK) Watch for cancellation error near zero on axes labels */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
#define NearlyEqual(x,y,tic) (fabs((x)-(y)) < ((tic) * SIGNIF))

/* And the functions to map from user to terminal coordinates */
#define map_x(x) (int)(x+0.5)	/* maps floating point x to screen */
#define map_y(y) (int)(y+0.5)	/* same for y */

/* And the functions to map from user 3D space into normalized -1..1 */
#define map_x3d(x) ((x-x_min3d)*xscale3d-1.0)
#define map_y3d(y) ((y-y_min3d)*yscale3d-1.0)
#define map_z3d(z) ((z-floor_z)*zscale3d-1.0)



/* Initialize the line style using the current device and set hidden styles
 * to it as well if hidden line removal is enabled */
static void
setlinestyle(style)
struct lp_style_type style;
{
    term_apply_lp_properties(&style);
}

/* And the functions to map from user 3D space to terminal coordinates */
#ifndef PM3D
static
#endif
void
map3d_xy(x, y, z, xt, yt)
double x, y, z;
unsigned int *xt, *yt;
{
    int i, j;
    double v[4], res[4],	/* Homogeneous coords. vectors. */
     w = trans_mat[3][3];

    v[0] = map_x3d(x);		/* Normalize object space to -1..1 */
    v[1] = map_y3d(y);
    v[2] = map_z3d(z);
    v[3] = 1.0;

    for (i = 0; i < 2; i++) {	/* Dont use the third axes (z). */
	res[i] = trans_mat[3][i];	/* Initiate it with the weight factor */
	for (j = 0; j < 3; j++)
	    res[i] += v[j] * trans_mat[j][i];
    }

    for (i = 0; i < 3; i++)
	w += v[i] * trans_mat[i][3];
    if (w == 0)
	w = 1e-5;

    *xt = (unsigned int) ((res[0] * xscaler / w) + xmiddle);
    *yt = (unsigned int) ((res[1] * yscaler / w) + ymiddle);
}

/* HBB 991021: moved to here, from hidden3d.c */
/* Two routines to emulate move/vector sequence using line drawing routine. */
unsigned int move_pos_x, move_pos_y;

void
clip_move(x, y)
unsigned int x, y;
{
    move_pos_x = x;
    move_pos_y = y;
}

void
clip_vector(x, y)
unsigned int x, y;
{
    draw_clip_line(move_pos_x, move_pos_y, x, y);
    move_pos_x = x;
    move_pos_y = y;
}

#if 0 /* HBB 990829: unused! --> commented out */
/* And the functions to map from user 3D space to terminal z coordinate */
int
map3d_z(x, y, z)
double x, y, z;
{
    int i, zt;
    double v[4], res,		/* Homogeneous coords. vectors. */
     w = trans_mat[3][3];

    v[0] = map_x3d(x);		/* Normalize object space to -1..1 */
    v[1] = map_y3d(y);
    v[2] = map_z3d(z);
    v[3] = 1.0;

    res = trans_mat[3][2];	/* Initiate it with the weight factor. */
    for (i = 0; i < 3; i++)
	res += v[i] * trans_mat[i][2];
    if (w == 0)
	w = 1e-5;
    for (i = 0; i < 3; i++)
	w += v[i] * trans_mat[i][3];
    zt = ((int) (res * 16384 / w));
    return zt;
}
#endif /* commented out */

/* borders of plotting area */
/* computed once on every call to do_plot */
static void
boundary3d(scaling, plots, count)
TBOOLEAN scaling;		/* TRUE if terminal is doing the scaling */
struct surface_points *plots;
int count;
{
    register struct termentry *t = term;
    int ytlen, i;

    titlelin = 0;

    p_height = pointsize * t->v_tic;
    p_width = pointsize * t->h_tic;
    if (key_swidth >= 0)
	key_sample_width = key_swidth * (t->h_char) + pointsize * (t->h_tic);
    else
	key_sample_width = 0;
    key_entry_height = pointsize * (t->v_tic) * 1.25 * key_vert_factor;
    if (key_entry_height < (t->v_char)) {
	/* is this reasonable ? */
	key_entry_height = (t->v_char) * key_vert_factor;
    }
    /* count max_len key and number keys (plot-titles and contour labels) with len > 0 */
    max_ptitl_len = find_maxl_keys3d(plots, count, &ptitl_cnt);
    if ((ytlen = label_width(key_title, &ktitle_lines)) > max_ptitl_len)
	max_ptitl_len = ytlen;
    key_col_wth = (max_ptitl_len + 4) * (t->h_char) + key_sample_width;

    /* luecken@udel.edu modifications
       sizes the plot to take up more of available resolution */
    if (lmargin >= 0)
	xleft = (t->h_char) * lmargin;
    else
	xleft = (t->h_char) * 2 + (t->h_tic);
    xright = (scaling ? 1 : xsize) * (t->xmax) - (t->h_char) * 2 - (t->h_tic);
    key_rows = ptitl_cnt;
    key_cols = 1;
    if (key == -1 && key_vpos == TUNDER) {
	if (ptitl_cnt > 0) {
	    /* calculate max no cols, limited by label-length */
	    key_cols = (int) (xright - xleft) / ((max_ptitl_len + 4) * (t->h_char) + key_sample_width);
	    /* HBB 991019: fix division by zero problem */
	    if (key_cols == 0)
		key_cols = 1;
	    key_rows = (int) (ptitl_cnt / key_cols) + ((ptitl_cnt % key_cols) > 0);
	    /* now calculate actual no cols depending on no rows */
	    key_cols = (int) (ptitl_cnt / key_rows) + ((ptitl_cnt % key_rows) > 0);
	    key_col_wth = (int) (xright - xleft) / key_cols;
	    /* key_rows += ktitle_lines; - messes up key - div */
	} else {
	    key_rows = key_cols = key_col_wth = 0;
	}
    }
    /* this should also consider the view and number of lines in
     * xformat || yformat || xlabel || ylabel */

    /* an absolute 1, with no terminal-dependent scaling ? */
    ybot = (t->v_char) * 2.5 + 1;
    if (key_rows && key == -1 && key_vpos == TUNDER)
	ybot += key_rows * key_entry_height + ktitle_lines * t->v_char;

    if (strlen(title.text)) {
	titlelin++;
	for (i = 0; i < strlen(title.text); i++) {
	    if (title.text[i] == '\\')
		titlelin++;
	}
    }
    ytop = (scaling ? 1 : ysize) * (t->ymax) - (t->v_char) * (titlelin + 1.5) - 1;
    if (key == -1 && key_vpos != TUNDER) {
	/* calculate max no rows, limited be ytop-ybot */
	i = (int) (ytop - ybot) / (t->v_char) - 1 - ktitle_lines;
	if (ptitl_cnt > i) {
	    key_cols = (int) (ptitl_cnt / i) + ((ptitl_cnt % i) > 0);
	    /* now calculate actual no rows depending on no cols */
	    key_rows = (int) (ptitl_cnt / key_cols) + ((ptitl_cnt % key_cols) > 0);
	}
	key_rows += ktitle_lines;
    }
    if (key == -1 && key_hpos == TOUT) {
	xright -= key_col_wth * (key_cols - 1) + key_col_wth - 2 * (t->h_char);
    }
    xleft += t->xmax * xoffset;
    xright += t->xmax * xoffset;
    ytop += t->ymax * yoffset;
    ybot += t->ymax * yoffset;
    xmiddle = (xright + xleft) / 2;
    ymiddle = (ytop + ybot) / 2;
    /* HBB 980308: sigh... another 16bit glitch: on term's with more than
     * 8000 pixels in either direction, these calculations produce garbage
     * results if done in (16bit) ints */
    xscaler = ((xright - xleft) * 4L) / 7L;	/* HBB: Magic number alert! */
    yscaler = ((ytop - ybot) * 4L) / 7L;
}

static void
get_arrow3d(arrow, sx, sy, ex, ey)
struct arrow_def* arrow;
unsigned int* sx;
unsigned int* sy;
unsigned int* ex;
unsigned int* ey;
{
    map3d_position(&(arrow->start), sx, sy, "arrow");
    if (arrow->relative) {
	if (arrow->start.scalex == arrow->end.scalex &&
	    arrow->start.scaley == arrow->end.scaley &&
	    arrow->start.scalez == arrow->end.scalez) {
	    /* coordinate systems are equal. The advantage of
	     * handling this special case is that it works also
	     * for logscale (which might not work otherwise, if
	     * the relative arrows point downwards for example) */
	    struct position delta_pos;
	    delta_pos = arrow->start;
	    delta_pos.x += arrow->end.x;
	    delta_pos.y += arrow->end.y;
	    delta_pos.z += arrow->end.z;
	    map_position(&delta_pos, ex, ey, "arrow");
	} else {
	    map3d_position_r(&(arrow->start), &(arrow->end), ex, ey, "arrow");
	}
    } else {
	map3d_position(&(arrow->end), ex, ey, "arrow");
    }
}

#if 0
/* not used ; anyway, should be done using bitshifts and squares,
 * rather than iteratively
 */
static double
dbl_raise(x, y)
double x;
int y;
{
    register int i = ABS(y);
    double val = 1.0;
    while (--i >= 0)
	val *= x;
    if (y < 0)
	return (1.0 / val);
    return (val);
}
#endif

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
do_3dplot(plots, pcount, quick)
struct surface_points *plots;
int pcount;			/* count of plots in linked list */
int quick;			/* !=0 means plot only axes etc., for quick rotation */
{
    struct termentry *t = term;
    int surface;
    struct surface_points *this_plot = NULL;
    unsigned int xl, yl;
    int linetypeOffset = 0;
    /* double ztemp, temp; unused */
    struct text_label *this_label;
    struct arrow_def *this_arrow;
    TBOOLEAN scaling;
    transform_matrix mat;
    int key_count;
    char *s, *e;
#ifdef PM3D
    TBOOLEAN can_pm3d = 0;
#endif

    /* Initiate transformation matrix using the global view variables. */
    mat_rot_z(surface_rot_z, trans_mat);
    mat_rot_x(surface_rot_x, mat);
    mat_mult(trans_mat, trans_mat, mat);
    mat_scale(surface_scale / 2.0, surface_scale / 2.0, surface_scale / 2.0, mat);
    mat_mult(trans_mat, trans_mat, mat);

#if 0
    /* HBB 19990609: this is *not* the way to implement 'set view' <z_scale> */
    /* modify min_z/max_z so it will zscale properly. */
    ztemp = (z_max3d - z_min3d) / (2.0 * surface_zscale);
    temp = (z_max3d + z_min3d) / 2.0;
    z_min3d = temp - ztemp;
    z_max3d = temp + ztemp;
#endif /* 0 */

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

    /* If we are to draw the bottom grid make sure zmin is updated properly. */
    if (xtics || ytics || work_grid.l_type) {
	base_z = z_min3d - (z_max3d - z_min3d) * ticslevel;
	if (ticslevel >= 0)
	    floor_z = base_z;
	else
	    floor_z = z_min3d;

	if (ticslevel < -1)
	    ceiling_z = base_z;
	else
	    ceiling_z = z_max3d;
    } else {
	floor_z = base_z = z_min3d;
	ceiling_z = z_max3d;
    }

    /*  see comment accompanying similar tests of x_min/x_max and y_min/y_max
     *  in graphics.c:do_plot(), for history/rationale of these tests */
    if (x_min3d == x_max3d)
	graph_error("x_min3d should not equal x_max3d!");
    if (y_min3d == y_max3d)
	graph_error("y_min3d should not equal y_max3d!");
    if (z_min3d == z_max3d)
	graph_error("z_min3d should not equal z_max3d!");

#ifndef LITE
    if (hidden3d) {
	struct surface_points *plot;
	int p = 0;
	/* Verify data is hidden line removable - grid based. */
	for (plot = plots; ++p <= pcount; plot = plot->next_sp) {
	    if (plot->plot_type == DATA3D && !plot->has_grid_topology) {
		fprintf(stderr, "Notice: Cannot remove hidden lines from non grid data\n");
		return;
	    }
	}
    }
#endif /* not LITE */

    term_start_plot();

    screen_ok = FALSE;
    scaling = (*t->scale) (xsize, ysize);

    /* now compute boundary for plot (xleft, xright, ytop, ybot) */
    boundary3d(scaling, plots, pcount);

    /* SCALE FACTORS */
    zscale3d = 2.0 / (ceiling_z - floor_z) * surface_zscale;
    yscale3d = 2.0 / (y_max3d - y_min3d);
    xscale3d = 2.0 / (x_max3d - x_min3d);

    term_apply_lp_properties(&border_lp);	/* border linetype */

#ifdef PM3D
    /* DRAW PM3D ALL COLOUR SURFACES */
    can_pm3d = 0;
    if (!quick) {
	int palette = 0;
	if (!pm3d.where[0]) {
	    for (this_plot = plots, surface = 0; surface < pcount;
		this_plot = this_plot->next_sp, surface++) {
		if (this_plot->lp_properties.use_palette) {
		    palette = 1;
		    break;
		}
	    }
	} else {
	    palette = 1;
	}
	if (palette)
	    can_pm3d = set_pm3d_zminmax() && !make_palette() && term->set_color;
	if (pm3d.where[0] && can_pm3d) {
	    if (pm3d.solid) {
		whichgrid = BACKGRID;
		draw_bottom_grid(plots, pcount);
	    }
	    pm3d_draw_all(plots, pcount);
	}

	if (can_pm3d) {
	    /* draw colour box */
	    draw_color_smooth_box();
	}
    }
#endif

    /* PLACE TITLE */
    if (*title.text != 0) {
	write_multiline((unsigned int) ((xleft + xright) / 2 + title.xoffset * t->h_char),
			(unsigned int) (ytop + (titlelin + title.yoffset) * (t->h_char)),
			title.text, CENTRE, JUST_TOP, 0, title.font);
    }
    /* PLACE TIMEDATE */
    if (*timelabel.text) {
	char str[MAX_LINE_LEN+1];
	time_t now;
	unsigned int x = t->v_char + timelabel.xoffset * t->h_char;
	unsigned int y = timelabel_bottom ?
	yoffset * ymax + (timelabel.yoffset + 1) * t->v_char :
	ytop + (timelabel.yoffset - 1) * t->v_char;

	time(&now);
	strftime(str, MAX_LINE_LEN, timelabel.text, localtime(&now));

	if (timelabel_rotate && (*t->text_angle) (1)) {
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_TOP, 1, timelabel.font);
	    else
		write_multiline(x, y, str, RIGHT, JUST_TOP, 1, timelabel.font);

	    (*t->text_angle) (0);
	} else {
	    if (timelabel_bottom)
		write_multiline(x, y, str, LEFT, JUST_BOT, 0, timelabel.font);
	    else
		write_multiline(x, y, str, LEFT, JUST_TOP, 0, timelabel.font);
	}
    }
    /* PLACE LABELS */
    if (t->pointsize) {
	(*t->pointsize)(pointsize);
    }
    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {

	unsigned int x, y;
	int htic;
	int vtic;

	get_offsets(this_label, t, &htic, &vtic);

	if (this_label->layer)
	    continue;
	map3d_position(&this_label->place, &x, &y, "label");
	if (this_label->rotate && (*t->text_angle) (1)) {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, CENTRE, 1, this_label->font);
	    (*t->text_angle) (0);
	} else {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, CENTRE, 0, this_label->font);
	}
	if (-1 != this_label->pointstyle) {
	    (*t->point)(x, y, this_label->pointstyle);
	}
    }

    /* PLACE ARROWS */
    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	unsigned int sx, sy, ex, ey;

	if (this_arrow->layer)
	    continue;
	get_arrow3d(this_arrow, &sx, &sy, &ex, &ey);
	term_apply_lp_properties(&(this_arrow->lp_properties));
	apply_head_properties(&(this_arrow->headsize));
	(*t->arrow) (sx, sy, ex, ey, this_arrow->head);
    }

#ifndef LITE
    if (hidden3d && draw_surface && !quick) {
	init_hidden_line_removal();
	reset_hidden_line_removal();
	hidden_active = TRUE;
    }
#endif /* not LITE */

    /* WORK OUT KEY SETTINGS AND DO KEY TITLE / BOX */

    if (key_reverse) {
	key_sample_left = -key_sample_width;
	key_sample_right = 0;
	key_text_left = t->h_char;
	key_text_right = (t->h_char) * (max_ptitl_len + 1);
	key_size_right = (t->h_char) * (max_ptitl_len + 2 + key_width_fix);
	key_size_left = (t->h_char) + key_sample_width;
    } else {
	key_sample_left = 0;
	key_sample_right = key_sample_width;
	key_text_left = -(int) ((t->h_char) * (max_ptitl_len + 1));
	key_text_right = -(int) (t->h_char);
	key_size_left = (t->h_char) * (max_ptitl_len + 2 + key_width_fix);
	key_size_right = (t->h_char) + key_sample_width;
    }
    key_point_offset = (key_sample_left + key_sample_right) / 2;

    if (key == -1) {
	if (key_vpos == TUNDER) {
#if 0
	    yl = yoffset * t->ymax + (key_rows) * key_entry_height + (ktitle_lines + 2) * t->v_char;
	    xl = max_ptitl_len * 1000 / (key_sample_width / (t->h_char) + max_ptitl_len + 2);
	    xl *= (xright - xleft) / key_cols;
	    xl /= 1000;
	    xl += xleft;
#else
	    /* HBB 19990608: why calculate these again? boundary3d has already 
	     * done it... */
	    if (ptitl_cnt > 0) {
		/* maximise no cols, limited by label-length */
		key_cols = (int) (xright - xleft) / key_col_wth;
		key_rows = (int) (ptitl_cnt + key_cols - 1) / key_cols;
		/* now calculate actual no cols depending on no rows */
		key_cols = (int) (ptitl_cnt + key_rows - 1) / key_rows;
		key_col_wth = (int) (xright - xleft) / key_cols;
		/* we divide into columns, then centre in column by considering
		 * ratio of key_left_size to key_right_size
		 * key_size_left/(key_size_left+key_size_right) * (xright-xleft)/key_cols
		 * do one integer division to maximise accuracy (hope we dont
		 * overflow !)
		 */
		xl = xleft + ((xright - xleft) * key_size_left) / (key_cols * (key_size_left + key_size_right));
		yl = yoffset * t->ymax + (key_rows) * key_entry_height + (ktitle_lines + 2) * t->v_char;
	    }
#endif
	} else {
	    if (key_vpos == TTOP) {
		yl = ytop - (t->v_tic) - t->v_char;
	    } else {
		yl = ybot + (t->v_tic) + key_entry_height * key_rows + ktitle_lines * t->v_char;
	    }
	    if (key_hpos == TOUT) {
		/* keys outside plot border (right) */
		xl = xright + (t->h_tic) + key_size_left;
	    } else if (key_hpos == TLEFT) {
		xl = xleft + (t->h_tic) + key_size_left;
	    } else {
		xl = xright - key_size_right - key_col_wth * (key_cols - 1);
	    }
	}
	yl_ref = yl - ktitle_lines * (t->v_char);
    }
    if (key == 1) {
	map3d_position(&key_user_pos, &xl, &yl, "key");
    }
    if (key && key_box.l_type > -3) {
	int yt = yl;
	int yb = yl - key_entry_height * (key_rows - ktitle_lines) - ktitle_lines * t->v_char;
	int key_xr = xl + key_col_wth * (key_cols - 1) + key_size_right;
	/* key_rows seems to contain title at this point ??? */
	term_apply_lp_properties(&key_box);
	(*t->move) (xl - key_size_left, yb);
	(*t->vector) (xl - key_size_left, yt);
	(*t->vector) (key_xr, yt);
	(*t->vector) (key_xr, yb);
	(*t->vector) (xl - key_size_left, yb);

	/* draw a horizontal line between key title and first entry  JFi */
	(*t->move) (xl - key_size_left, yt - (ktitle_lines) * t->v_char);
	(*t->vector) (xl + key_size_right, yt - (ktitle_lines) * t->v_char);
    }
    /* DRAW SURFACES AND CONTOURS */

#ifndef LITE
    if (hidden3d && draw_surface && !quick)
	plot3d_hidden(plots, pcount);
#endif /* not LITE */

    /* KEY TITLE */
    if (key != 0 && strlen(key_title)) {
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
		    (*t->put_text) (xl + key_text_right,
				    yl, s);
		} else {
		    int x = xl + key_text_right - (t->h_char) * strlen(s);
		    if (inrange(x, xleft, xright))
			(*t->put_text) (x, yl, s);
		}
	    }
	    s = ++e;
	    yl -= t->v_char;
	}
	yl += t->v_char / 2;
	free(ss);
    }
    key_count = 0;
    yl_ref = yl -= key_entry_height / 2;	/* centralise the keys */

#define NEXT_KEY_LINE() \
 if ( ++key_count >= key_rows ) { \
   yl = yl_ref; xl += key_col_wth; key_count = 0; \
 } else \
   yl -= key_entry_height

    this_plot = plots;
  if (!quick)
    for (surface = 0;
	 surface < pcount;
	 this_plot = this_plot->next_sp, surface++) {

#ifdef PM3D
	/* just an abbreviation */
	TBOOLEAN use_palette = can_pm3d && this_plot->lp_properties.use_palette;
#endif

#ifndef LITE
	if (hidden3d)
	    hidden_no_update = FALSE;
#endif /* not LITE */

	if (draw_surface) {
	    int lkey = (key != 0 && this_plot->title && this_plot->title[0]);
	    term_apply_lp_properties(&(this_plot->lp_properties));


	    if (lkey) {
		ignore_enhanced_text = this_plot->title_no_enhanced == 1;
		key_text(xl, yl, this_plot->title);
		ignore_enhanced_text = 0;
	    }
	    switch (this_plot->plot_style) {
	    case BOXES:	/* can't do boxes in 3d yet so use impulses */
	    case IMPULSES:{
		    if (lkey) {
			key_sample_line(xl, yl);
		    }
		    if (!(hidden3d && draw_surface))
			plot3d_impulses(this_plot);
		    break;
		}
	    case STEPS:	/* HBB: I think these should be here */
	    case FSTEPS:
	    case HISTEPS:
	    case LINES:{
		    if (lkey) {
			key_sample_line(xl, yl);
		    }
		    if (!(hidden3d && draw_surface)) {
#ifdef PM3D
			if (use_palette)
			    plot3d_lines_pm3d(this_plot);
			else
#endif
			    plot3d_lines(this_plot);
		    }
		    break;
		}
	    case YERRORLINES:	/* ignored; treat like points */
	    case XERRORLINES:	/* ignored; treat like points */
	    case XYERRORLINES:	/* ignored; treat like points */
	    case YERRORBARS:	/* ignored; treat like points */
	    case XERRORBARS:	/* ignored; treat like points */
	    case XYERRORBARS:	/* ignored; treat like points */
	    case BOXXYERROR:	/* HBB: ignore these as well */
	    case BOXERROR:
	    case CANDLESTICKS:	/* HBB: dito */
	    case FINANCEBARS:
	    case VECTOR:
	    case POINTSTYLE:
		if (lkey) {
		    key_sample_point(xl, yl, this_plot->lp_properties.p_type);
		}
		if (!(hidden3d && draw_surface)) {
#ifdef PM3D
		    if (use_palette)
			plot3d_points_pm3d(this_plot, this_plot->lp_properties.p_type);
		    else
#endif
			plot3d_points(this_plot, this_plot->lp_properties.p_type);
		}
		break;

	    case LINESPOINTS:
		/* put lines */
		if (lkey)
		    key_sample_line(xl, yl);

		if (!(hidden3d && draw_surface)) {
#ifdef PM3D
		    if (use_palette)
			plot3d_lines_pm3d(this_plot);
		    else
#endif
			plot3d_lines(this_plot);
		}

		/* put points */
		if (lkey)
		    key_sample_point(xl, yl, this_plot->lp_properties.p_type);

		if (!(hidden3d && draw_surface)) {
#ifdef PM3D
		    if (use_palette)
			plot3d_points_pm3d(this_plot, this_plot->lp_properties.p_type);
		    else
#endif
			plot3d_points(this_plot, this_plot->lp_properties.p_type);
		}

		break;

	    case DOTS:
		if (lkey) {
		    if (key == 1) {
			if (!clip_point(xl + key_point_offset, yl))
			    (*t->point) (xl + key_point_offset, yl, -1);
		    } else {
			(*t->point) (xl + key_point_offset, yl, -1);
			/* (*t->point)(xl+2*(t->h_char),yl, -1); */
		    }
		}
		if (!(hidden3d && draw_surface)) {
#ifdef PM3D
		    if (use_palette)
			plot3d_points_pm3d(this_plot, -1);
		    else
#endif
			plot3d_points(this_plot, -1);
		}

		break;


	    }			/* switch(plot-style) */

	    /* move key on a line */
	    if (lkey) {
		NEXT_KEY_LINE();
	    }
	}			/* draw_surface */
#ifndef LITE
	if (hidden3d) {
	    hidden_no_update = TRUE;
	}
#endif /* not LITE */

	if (draw_contour && this_plot->contours != NULL) {
	    struct gnuplot_contours *cntrs = this_plot->contours;

	    term_apply_lp_properties(&(this_plot->lp_properties));
	    (*t->linetype) (this_plot->lp_properties.l_type + (hidden3d ? 2 : 1));

	    if (key != 0 && this_plot->title && this_plot->title[0]
		&& !draw_surface && !label_contours) {
		/* unlabelled contours but no surface : put key entry in now */
		ignore_enhanced_text = this_plot->title_no_enhanced == 1;
		key_text(xl, yl, this_plot->title);
		ignore_enhanced_text = 0;

		switch (this_plot->plot_style) {
		case IMPULSES:
		case LINES:
		case BOXES:	/* HBB: I think these should be here... */
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
		case CANDLESTICKS:	/* HBB: dito */
		case FINANCEBARS:
		case VECTOR:
		case POINTSTYLE:
		    key_sample_point(xl, yl, this_plot->lp_properties.p_type);
		    break;
		case LINESPOINTS:
		    key_sample_line(xl, yl);
		    break;
		case DOTS:
		    key_sample_point(xl, yl, -1);
		    break;
		}
		NEXT_KEY_LINE();
	    }
	    linetypeOffset = this_plot->lp_properties.l_type + (hidden3d ? 2 : 1);
	    while (cntrs) {
		if (label_contours && cntrs->isNewLevel) {
#ifdef PM3D
		    if (use_palette)
			set_color(z2gray(cntrs->z));
		    else
#endif
			(*t->linetype) (linetypeOffset++);
		    if (key) {

			key_text(xl, yl, cntrs->label);

			switch (this_plot->plot_style) {
			case IMPULSES:
			case LINES:
			case LINESPOINTS:
			case BOXES:	/* HBB: these should be treated as well... */
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
			case BOXERROR:		/* HBB: treat these likewise */
			case BOXXYERROR:
			case CANDLESTICKS:	/* HBB: dito */
			case FINANCEBARS:
			case VECTOR:
			case POINTSTYLE:
			    key_sample_point(xl, yl, this_plot->lp_properties.p_type);
			    break;
			case DOTS:
			    key_sample_point(xl, yl, -1);
			    break;
			}	/* switch */

			NEXT_KEY_LINE();

		    }		/* key */
		}		/* label_contours */
		/* now draw the contour */
		switch (this_plot->plot_style) {
		case IMPULSES:
		case BOXES:	/* HBB: this should also be treated somehow */
		    cntr3d_impulses(cntrs, this_plot);
		    break;
		case LINES:
		case STEPS:	/* HBB: these should also be handled, I think */
		case FSTEPS:
		case HISTEPS:
		    cntr3d_lines(cntrs);
		    break;
		case YERRORLINES:	/* ignored; treat like points */
		case XERRORLINES:	/* ignored; treat like points */
		case XYERRORLINES:	/* ignored; treat like points */
		case YERRORBARS:	/* ignored; treat like points */
		case XERRORBARS:	/* ignored; treat like points */
		case XYERRORBARS:	/* ignored; treat like points */
		case BOXERROR:	/* HBB: ignore these too... */
		case BOXXYERROR:
		case CANDLESTICKS:	/* HBB: dito */
		case FINANCEBARS:
		case VECTOR:
		case POINTSTYLE:
		    cntr3d_points(cntrs, this_plot->lp_properties.p_type);
		    break;
		case LINESPOINTS:
		    cntr3d_lines(cntrs);
		    cntr3d_points(cntrs, this_plot->lp_properties.p_type);
		    break;
		case DOTS:
		    cntr3d_points(cntrs, -1);
		    break;
		}		/*switch */

		cntrs = cntrs->next;
	    }			/* loop over contours */
	}			/* draw contours */
    }				/* loop over surfaces */

#ifdef PM3D
    /* draw grid and coordinate system */
    if (pm3d.where[0] && pm3d.solid && !quick) {
	whichgrid = FRONTGRID;
    } else {
	whichgrid = ALLGRID;
    }
#endif
    draw_bottom_grid(plots, pcount);

    /* PLACE LABELS */
    if (t->pointsize) {
	(*t->pointsize)(pointsize);
    }
    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {

	unsigned int x, y;
	int htic;
	int vtic;

	get_offsets(this_label, t, &htic, &vtic);

	if (this_label->layer == 0)
	    continue;
	map3d_position(&this_label->place, &x, &y, "label");
	if (this_label->rotate && (*t->text_angle) (1)) {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, CENTRE, 1, this_label->font);
	    (*t->text_angle) (0);
	} else {
	    write_multiline(x + htic, y + vtic, this_label->text, this_label->pos, CENTRE, 0, this_label->font);
	}
	if (-1 != this_label->pointstyle) {
	    (*t->point)(x, y, this_label->pointstyle);
	}
    }

    /* PLACE ARROWS */
    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	unsigned int sx, sy, ex, ey;

	if (this_arrow->layer == 0)
	    continue;
	get_arrow3d(this_arrow, &sx, &sy, &ex, &ey);
	term_apply_lp_properties(&(this_arrow->lp_properties));
	apply_head_properties(&(this_arrow->headsize));
	(*t->arrow) (sx, sy, ex, ey, this_arrow->head);
    }

#ifdef USE_MOUSE
    /* finally, store the 2d projection of the x and y axis, to enable zooming by mouse */
    {
	unsigned int o_x, o_y, x, y;
	map3d_xy(x_min3d, y_min3d, base_z, &o_x, &o_y);
	axis3d_o_x = (int)o_x;
	axis3d_o_y = (int)o_y;
	map3d_xy(x_max3d, y_min3d, base_z, &x, &y);
	axis3d_x_dx = (int)x - axis3d_o_x;
	axis3d_x_dy = (int)y - axis3d_o_y;
	map3d_xy(x_min3d, y_max3d, base_z, &x, &y);
	axis3d_y_dx = (int)x - axis3d_o_x;
	axis3d_y_dy = (int)y - axis3d_o_y;
    }
#endif

    term_end_plot();

#ifndef LITE
    if (hidden3d && draw_surface) {
	term_hidden_line_removal();
	hidden_active = FALSE;
    }
#endif /* not LITE */

}

/* plot3d_impulses:
 * Plot the surfaces in IMPULSES style
 */
static void
plot3d_impulses(plot)
struct surface_points *plot;
{
    int i;				/* point index */
    unsigned int x, y, xx0, yy0;	/* point in terminal coordinates */
    struct iso_curve *icrvs = plot->iso_crvs;

    while (icrvs) {
	struct coordinate GPHUGE *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {
	    switch (points[i].type) {
	    case INRANGE:
		{
		    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

		    if (inrange(0.0, min3d_z, max3d_z)) {
			map3d_xy(points[i].x, points[i].y, 0.0, &xx0, &yy0);
		    } else if (inrange(min3d_z, 0.0, points[i].z)) {
			map3d_xy(points[i].x, points[i].y, min3d_z, &xx0, &yy0);
		    } else {
			map3d_xy(points[i].x, points[i].y, max3d_z, &xx0, &yy0);
		    }

		    clip_move(xx0, yy0);
		    clip_vector(x, y);

		    break;
		}
	    case OUTRANGE:
		{
		    if (!inrange(points[i].x, x_min3d, x_max3d) ||
			!inrange(points[i].y, y_min3d, y_max3d))
			break;

		    if (inrange(0.0, min3d_z, max3d_z)) {
			/* zero point is INRANGE */
			map3d_xy(points[i].x, points[i].y, 0.0, &xx0, &yy0);

			/* must cross z = min3d_z or max3d_z limits */
			if (inrange(min3d_z, 0.0, points[i].z) &&
			    min3d_z != 0.0 && min3d_z != points[i].z) {
			    map3d_xy(points[i].x, points[i].y, min3d_z, &x, &y);
			} else {
			    map3d_xy(points[i].x, points[i].y, max3d_z, &x, &y);
			}
		    } else {
			/* zero point is also OUTRANGE */
			if (inrange(min3d_z, 0.0, points[i].z) &&
			    inrange(max3d_z, 0.0, points[i].z)) {
			    /* crosses z = min3d_z or max3d_z limits */
			    map3d_xy(points[i].x, points[i].y, max3d_z, &x, &y);
			    map3d_xy(points[i].x, points[i].y, min3d_z, &xx0, &yy0);
			} else {
			    /* doesn't cross z = min3d_z or max3d_z limits */
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
plot3d_lines(plot)
struct surface_points *plot;
{
    int i;
    unsigned int x, y, xx0, yy0;	/* point in terminal coordinates */
    double clip_x, clip_y, clip_z;
    struct iso_curve *icrvs = plot->iso_crvs;
    struct coordinate GPHUGE *points;
    enum coord_type prev = UNDEFINED;
    double lx[2], ly[2], lz[2];	/* two edge points */

#ifndef LITE
/* These are handled elsewhere.  */
    if (plot->has_grid_topology && hidden3d)
	return;
#endif /* not LITE */

    while (icrvs) {
	prev = UNDEFINED;	/* type of previous plot */

	for (i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
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
	    default:
		    graph_error("Unknown point type in plot3d_lines");
		}
	    }

	    prev = points[i].type;
	}

	icrvs = icrvs->next;
    }
}

#ifdef PM3D
/* this is basically the same function as above, but:
 *  - it splits the bunch of scans in two sets corresponding to
 *    the two scan directions.
 *  - reorders the two sets -- from behind to front
 *  - checks if inside on scan of a set the order should be inverted
 */
static void
plot3d_lines_pm3d(plot)
    struct surface_points *plot;
{
    struct iso_curve** icrvs_pair[2];
    int invert[2];
    int n[2];

    int i, set, scan;
    unsigned int x, y, xx0, yy0;	/* point in terminal coordinates */
    double clip_x, clip_y, clip_z;
    struct coordinate GPHUGE *points;
    enum coord_type prev = UNDEFINED;
    double lx[2], ly[2], lz[2];	/* two edge points */
    double z;


#ifndef LITE
/* These are handled elsewhere.  */
    if (plot->has_grid_topology && hidden3d)
	return;
#endif /* not LITE */

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

	for (scan = 0; scan < n[set]; scan++) {

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
			    z =  (points[i - step].z + points[i].z) * .5;
			    set_color(z2gray(z));
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
				    z =  (points[i - step].z + points[i].z) * .5;
				    set_color(z2gray(z));
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

				z =  (points[i - step].z + points[i].z) * .5;
				set_color(z2gray(z));
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
				    z =  (points[i - step].z + points[i].z) * .5;
				    set_color(z2gray(z));
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
#endif

/* plot3d_points:
 * Plot the surfaces in POINTSTYLE style
 */
static void
plot3d_points(plot, p_type)
    struct surface_points *plot;
    int p_type;
{
    int i;
    unsigned int x, y;
    struct termentry *t = term;
    struct iso_curve *icrvs = plot->iso_crvs;

    while (icrvs) {
	struct coordinate GPHUGE *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {
	    if (points[i].type == INRANGE) {
		map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

		if (!clip_point(x, y))
		    (*t->point) (x, y, p_type);
	    }
	}

	icrvs = icrvs->next;
    }
}

#ifdef PM3D
static void
plot3d_points_pm3d(plot, p_type)
    struct surface_points *plot;
    int p_type;
{
    struct iso_curve** icrvs_pair[2];
    int invert[2];
    int n[2];

    int i, set, scan;
    unsigned int x, y;
    struct termentry *t = term;

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

	for (scan = 0; scan < n[set]; scan++) {

	    int cnt;
	    struct iso_curve *icrvs = icrvs_pair[set][scan];
	    struct coordinate GPHUGE *points;

	    if (invert[set]) {
		begin = icrvs->p_count - 1;
	    }

	    for (cnt = 0, i = begin, points = icrvs->points; cnt < icrvs->p_count; cnt++, i += step) {

		if (points[i].type == INRANGE) {
		    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

		    if (!clip_point(x, y)) {
			set_color(z2gray(points[i].z));
			(*t->point) (x, y, p_type);
		    }
		}
	    }
	} /* scan */
    } /* set */
}
#endif


/* cntr3d_impulses:
 * Plot a surface contour in IMPULSES style
 */
static void
cntr3d_impulses(cntr, plot)
struct gnuplot_contours *cntr;
struct surface_points *plot;
{
    int i;				/* point index */
    unsigned int x, y, xx0, yy0;	/* point in terminal coordinates */

    if (draw_contour & CONTOUR_SRF) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
		     &x, &y);
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, base_z,
		     &xx0, &yy0);

	    clip_move(xx0, yy0);
	    clip_vector(x, y);
	}
    } else {
	/* Must be on base grid, so do points. */
	cntr3d_points(cntr, plot->lp_properties.p_type);
    }
}

/* cntr3d_lines:
 * Plot a surface contour in LINES style
 */
static void
cntr3d_lines(cntr)
struct gnuplot_contours *cntr;
{
    int i;			/* point index */
    unsigned int x, y;		/* point in terminal coordinates */

    if (draw_contour & CONTOUR_SRF) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
		     &x, &y);

	    if (i > 0) {
		clip_vector(x, y);
		if (i == 1)
		    suppressMove = TRUE;
	    } else {
		clip_move(x, y);
	    }
	}
    }
    /* beginning a new contour level, so moveto() required */
    suppressMove = FALSE;

    if (draw_contour & CONTOUR_BASE) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, base_z,
		     &x, &y);

	    if (i > 0) {
		clip_vector(x, y);
		if (i == 1)
		    suppressMove = TRUE;
	    } else {
		clip_move(x, y);
	    }
	}
    }
    /* beginning a new contour level, so moveto() required */
    suppressMove = FALSE;
}

/* cntr3d_points:
 * Plot a surface contour in POINTSTYLE style
 */
static void
cntr3d_points(cntr, p_type)
struct gnuplot_contours *cntr;
int p_type;
{
    int i;
    unsigned int x, y;
    struct termentry *t = term;

    if (draw_contour & CONTOUR_SRF) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &x, &y);

	    if (!clip_point(x, y))
		(*t->point) (x, y, p_type);
	}
    }
    if (draw_contour & CONTOUR_BASE) {
	for (i = 0; i < cntr->num_pts; i++) {
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, base_z,
		     &x, &y);

	    if (!clip_point(x, y))
		(*t->point) (x, y, p_type);
	}
    }
}


/* map xmin | xmax to 0 | 1 and same for y
 * 0.1 avoids any rounding errors
 */
#define MAP_HEIGHT_X(x) ( (int) (((x)-x_min3d)/(x_max3d-x_min3d)+0.1) )
#define MAP_HEIGHT_Y(y) ( (int) (((y)-y_min3d)/(y_max3d-y_min3d)+0.1) )

/* if point is at corner, update height[][] and depth[][]
 * we are still assuming that extremes of surfaces are at corners,
 * but we are not assuming order of corners
 */
static void
check_corner_height(p, height, depth)
struct coordinate GPHUGE *p;
double height[2][2];
double depth[2][2];
{
    if (p->type != INRANGE)
	return;
    if ((fabs(p->x - x_min3d) < zero || fabs(p->x - x_max3d) < zero) &&
	(fabs(p->y - y_min3d) < zero || fabs(p->y - y_max3d) < zero)) {
	unsigned int x = MAP_HEIGHT_X(p->x);
	unsigned int y = MAP_HEIGHT_Y(p->y);
	if (height[x][y] < p->z)
	    height[x][y] = p->z;
	if (depth[x][y] > p->z)
	    depth[x][y] = p->z;
    }
}


/* Draw the bottom grid that hold the tic marks for 3d surface. */
static void
draw_bottom_grid(plot, plot_num)
struct surface_points *plot;
int plot_num;
{
    unsigned int x, y;		/* point in terminal coordinates */
    struct termentry *t = term;

    /* work out where the axes and tics are drawn */

    {
	int quadrant = surface_rot_z / 90;
	if ((quadrant + 1) & 2) {
	    zaxis_x = x_max3d;
	    xaxis_y = y_max3d;
	} else {
	    zaxis_x = x_min3d;
	    xaxis_y = y_min3d;
	}

	if (quadrant & 2) {
	    zaxis_y = y_max3d;
	    yaxis_x = x_min3d;
	} else {
	    zaxis_y = y_min3d;
	    yaxis_x = x_max3d;
	}

	if (surface_rot_x > 90) {
	    /* labels on the back axes */
	    back_x = yaxis_x = x_min3d + x_max3d - yaxis_x;
	    back_y = xaxis_y = y_min3d + y_max3d - xaxis_y;
	} else {
	    back_x = x_min3d + x_max3d - yaxis_x;
	    back_y = y_min3d + y_max3d - xaxis_y;
	}
    }

    if (draw_border) {
	unsigned int bl_x, bl_y;	/* bottom left */
	unsigned int bb_x, bb_y;	/* bottom back */
	unsigned int br_x, br_y;	/* bottom right */
	unsigned int bf_x, bf_y;	/* bottom front */

#ifndef LITE
	int save_update = hidden_no_update;
	hidden_no_update = TRUE;
#endif /* LITE */

	/* Here is the one and only call to this function. */
	setlinestyle(border_lp);

	map3d_xy(zaxis_x, zaxis_y, base_z, &bl_x, &bl_y);
	map3d_xy(back_x, back_y, base_z, &bb_x, &bb_y);
	map3d_xy(x_min3d + x_max3d - zaxis_x, y_min3d + y_max3d - zaxis_y, base_z, &br_x, &br_y);
	map3d_xy(x_min3d + x_max3d - back_x, y_min3d + y_max3d - back_y, base_z, &bf_x, &bf_y);

	/* border around base */
#ifdef PM3D
	if (BACKGRID != whichgrid)
#endif
	{
	    int save = hidden_active;
	    hidden_active = FALSE;	/* this is in front */
	    if (draw_border & 4)
		draw_clip_line(br_x, br_y, bf_x, bf_y);
	    if (draw_border & 1)
		draw_clip_line(bl_x, bl_y, bf_x, bf_y);
	    hidden_active = save;
	}
#ifdef PM3D
	if (FRONTGRID != whichgrid)
#endif
	{
	    if (draw_border & 2)
		draw_clip_line(bl_x, bl_y, bb_x, bb_y);
	    if (draw_border & 8)
		draw_clip_line(br_x, br_y, bb_x, bb_y);
	}

	if (draw_surface || (draw_contour & CONTOUR_SRF)
#ifdef PM3D
	    || strpbrk(pm3d.where,"st") != NULL
#endif
	   ) {
	    int save = hidden_active;
	    /* map the 8 corners to screen */
	    unsigned int fl_x, fl_y;	/* floor left */
	    unsigned int fb_x, fb_y;	/* floor back */
	    unsigned int fr_x, fr_y;	/* floor right */
	    unsigned int ff_x, ff_y;	/* floor front */

	    unsigned int tl_x, tl_y;	/* top left */
	    unsigned int tb_x, tb_y;	/* top back */
	    unsigned int tr_x, tr_y;	/* top right */
	    unsigned int tf_x, tf_y;	/* top front */

	    map3d_xy(zaxis_x, zaxis_y, floor_z, &fl_x, &fl_y);
	    map3d_xy(back_x, back_y, floor_z, &fb_x, &fb_y);
	    map3d_xy(x_min3d + x_max3d - zaxis_x, y_min3d + y_max3d - zaxis_y, floor_z, &fr_x, &fr_y);
	    map3d_xy(x_min3d + x_max3d - back_x, y_min3d + y_max3d - back_y, floor_z, &ff_x, &ff_y);

	    map3d_xy(zaxis_x, zaxis_y, ceiling_z, &tl_x, &tl_y);
	    map3d_xy(back_x, back_y, ceiling_z, &tb_x, &tb_y);
	    map3d_xy(x_min3d + x_max3d - zaxis_x, y_min3d + y_max3d - zaxis_y, ceiling_z, &tr_x, &tr_y);
	    map3d_xy(x_min3d + x_max3d - back_x, y_min3d + y_max3d - back_y, ceiling_z, &tf_x, &tf_y);

	    /* vertical lines, to surface or to very top */
	    if ((draw_border & 0xf0) == 0xf0) {
#ifdef PM3D
		if (FRONTGRID != whichgrid) {
#endif
		    draw_clip_line(fl_x, fl_y, tl_x, tl_y);
		    draw_clip_line(fb_x, fb_y, tb_x, tb_y);
		    draw_clip_line(fr_x, fr_y, tr_x, tr_y);
#ifdef PM3D
		}
		if (BACKGRID != whichgrid) {
#endif
		    hidden_active = FALSE;	/* this is in front */
		    draw_clip_line(ff_x, ff_y, tf_x, tf_y);
		    hidden_active = save;
#ifdef PM3D
		}
#endif
	    } else {
		/* search surfaces for heights at corners */
		double height[2][2];
		double depth[2][2];
		unsigned int zaxis_i = MAP_HEIGHT_X(zaxis_x);
		unsigned int zaxis_j = MAP_HEIGHT_Y(zaxis_y);
		unsigned int back_i = MAP_HEIGHT_X(back_x);
		/* HBB: why isn't back_j unsigned ??? */
		int back_j = MAP_HEIGHT_Y(back_y);

		height[0][0] = height[0][1] = height[1][0] = height[1][1] = base_z;
		depth[0][0] = depth[0][1] = depth[1][0] = depth[1][1] = base_z;

		for (; --plot_num >= 0; plot = plot->next_sp) {
		    struct iso_curve *curve = plot->iso_crvs;
		    int count = curve->p_count;
		    int iso;
		    if (plot->plot_type == DATA3D) {
			if (!plot->has_grid_topology)
			    continue;
			iso = plot->num_iso_read;
		    } else
			iso = iso_samples_2;

		    check_corner_height(curve->points, height, depth);
		    check_corner_height(curve->points + count - 1, height, depth);
		    while (--iso)
			curve = curve->next;
		    check_corner_height(curve->points, height, depth);
		    check_corner_height(curve->points + count - 1, height, depth);
		}

#define VERTICAL(mask,x,y,i,j,bx,by,tx,ty) \
 if (draw_border&mask) \
  draw_clip_line(bx,by,tx,ty);\
else if (height[i][j] != depth[i][j]) \
{ unsigned int a0,b0, a1, b1; \
  map3d_xy(x,y,depth[i][j],&a0,&b0); \
  map3d_xy(x,y,height[i][j],&a1,&b1); \
  draw_clip_line(a0,b0,a1,b1); \
}

#ifdef PM3D
		if (FRONTGRID != whichgrid) {
#endif
		VERTICAL(16, zaxis_x, zaxis_y, zaxis_i, zaxis_j, fl_x, fl_y, tl_x, tl_y);
		VERTICAL(32, back_x, back_y, back_i, back_j, fb_x, fb_y, tb_x, tb_y);
		VERTICAL(64, x_min3d + x_max3d - zaxis_x, y_min3d + y_max3d - zaxis_y, 1 -
			 zaxis_i, 1 - zaxis_j, fr_x, fr_y, tr_x, tr_y);
#ifdef PM3D
		}
		if (BACKGRID != whichgrid) {
#endif
		hidden_active = FALSE;
		VERTICAL(128, x_min3d + x_max3d - back_x, y_min3d + y_max3d - back_y, 1 - back_i,
			 1 - back_j, ff_x, ff_y, tf_x, tf_y);
		hidden_active = save;
#ifdef PM3D
		}
#endif
	    }

	    /* now border lines on top */
#ifdef PM3D
	    if (FRONTGRID != whichgrid)
#endif
	    {
		if (draw_border & 256)
		    draw_clip_line(tl_x, tl_y, tb_x, tb_y);
		if (draw_border & 512)
		    draw_clip_line(tr_x, tr_y, tb_x, tb_y);
	    }
	    /* these lines are in front of surface (?) */
#ifdef PM3D
	    if (BACKGRID != whichgrid)
#endif
	    {
		hidden_active = FALSE;
		if (draw_border & 1024)
		    draw_clip_line(tl_x, tl_y, tf_x, tf_y);
		if (draw_border & 2048)
		    draw_clip_line(tr_x, tr_y, tf_x, tf_y);
		hidden_active = save;
	    }
	}
#ifndef LITE
	hidden_no_update = save_update;
#endif /* LITE */

    }
    if (xtics || *xlabel.text) {
	unsigned int x0, y0, x1, y1;
	double mid_x = (x_max3d + x_min3d) / 2;
	double len;
	map3d_xy(mid_x, xaxis_y, base_z, &x0, &y0);
	map3d_xy(mid_x, y_min3d + y_max3d - xaxis_y, base_z, &x1, &y1);
	{			/* take care over unsigned quantities */
	    int dx = x1 - x0;
	    int dy = y1 - y0;
	    /* HBB 980309: 16bit strikes back: */
	    len = sqrt(((double) dx) * dx + ((double) dy) * dy);
	    if (len != 0) {
		tic_unitx = dx / len;
		tic_unity = dy / len;
	    } else {
		tic_unitx = tic_unity = 0;
	    }
	}

	if (xtics) {
	    gen_tics(FIRST_X_AXIS, &xticdef,
		     work_grid.l_type & (GRID_X | GRID_MX),
		     mxtics, mxtfreq, xtick_callback);
	}
	if (*xlabel.text) {
	    /* label at xaxis_y + 1/4 of (xaxis_y-other_y) */
#ifdef PM3D
	    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
		(surface_rot_x > 90 && FRONTGRID != whichgrid)) {
#endif
	    double step = (2 * xaxis_y - y_max3d - y_min3d) / 4;
	    map3d_xy(mid_x, xaxis_y + step, base_z, &x1, &y1);
	    x1 += xlabel.xoffset * t->h_char;
	    y1 += xlabel.yoffset * t->v_char;
	    if (!tic_in) {
		x1 -= tic_unitx * ticscale * (t->h_tic);
		y1 -= tic_unity * ticscale * (t->v_tic);
	    }
	    /* write_multiline mods it */
	    write_multiline(x1, y1, xlabel.text, CENTRE, JUST_TOP, 0, xlabel.font);
#ifdef PM3D
	    }
#endif
	}
    }
    if (ytics || *ylabel.text) {
	unsigned int x0, y0, x1, y1;
	double mid_y = (y_max3d + y_min3d) / 2;
	double len;
	map3d_xy(yaxis_x, mid_y, base_z, &x0, &y0);
	map3d_xy(x_min3d + x_max3d - yaxis_x, mid_y, base_z, &x1, &y1);
	{			/* take care over unsigned quantities */
	    int dx = x1 - x0;
	    int dy = y1 - y0;
	    /* HBB 980309: 16 Bits strike again: */
	    len = sqrt(((double) dx) * dx + ((double) dy) * dy);
	    if (len != 0) {
		tic_unitx = dx / len;
		tic_unity = dy / len;
	    } else {
		tic_unitx = tic_unity = 0;
	    }
	}
	if (ytics) {
	    gen_tics(FIRST_Y_AXIS, &yticdef,
		     work_grid.l_type & (GRID_Y | GRID_MY),
		     mytics, mytfreq, ytick_callback);
	}
	if (*ylabel.text) {
#ifdef PM3D
	    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
		(surface_rot_x > 90 && FRONTGRID != whichgrid)) {
#endif
	    double step = (x_max3d + x_min3d - 2 * yaxis_x) / 4;
	    map3d_xy(yaxis_x - step, mid_y, base_z, &x1, &y1);
	    x1 += ylabel.xoffset * t->h_char;
	    y1 += ylabel.yoffset * t->v_char;
	    if (!tic_in) {
		x1 -= tic_unitx * ticscale * (t->h_tic);
		y1 -= tic_unity * ticscale * (t->v_tic);
	    }
	    /* write_multiline mods it */
#ifdef PM3D
	    if (pm3d_map_rotate_ylabel)
		(*t->text_angle)(1);
#endif
	    write_multiline(x1, y1, ylabel.text, CENTRE, JUST_TOP, 0, ylabel.font);
#ifdef PM3D
	    if (pm3d_map_rotate_ylabel)
		(*t->text_angle)(0);
#endif
#ifdef PM3D
	    }
#endif
	}
    }
    /* do z tics */

#ifndef PM3D
    if (ztics && (draw_surface || (draw_contour & CONTOUR_SRF))) {
#else
    if (ztics && (draw_surface || (draw_contour & CONTOUR_SRF) ||
		  strchr(pm3d.where,'s') != NULL)) {
#endif
	gen_tics(FIRST_Z_AXIS, &zticdef, work_grid.l_type & (GRID_Z | GRID_MZ),
		 mztics, mztfreq, ztick_callback);
    }
    if ((yzeroaxis.l_type >= -2) && !is_log_x && inrange(0, x_min3d, x_max3d)) {
	unsigned int x, y, x1, y1;
	term_apply_lp_properties(&yzeroaxis);
	map3d_xy(0.0, y_min3d, base_z, &x, &y);		/* line through x=0 */
	map3d_xy(0.0, y_max3d, base_z, &x1, &y1);
	draw_clip_line(x, y, x1, y1);
    }
    if ((xzeroaxis.l_type >= -2) && !is_log_y && inrange(0, y_min3d, y_max3d)) {
	unsigned int x, y, x1, y1;
	term_apply_lp_properties(&xzeroaxis);
	map3d_xy(x_min3d, 0.0, base_z, &x, &y);		/* line through y=0 */
	map3d_xy(x_max3d, 0.0, base_z, &x1, &y1);
	draw_clip_line(x, y, x1, y1);
    }
    /* PLACE ZLABEL - along the middle grid Z axis - eh ? */
#ifndef PM3D
    if (*zlabel.text && (draw_surface || (draw_contour & CONTOUR_SRF))) {
#else
    if (*zlabel.text && (draw_surface || (draw_contour & CONTOUR_SRF) ||
			 strpbrk(pm3d.where,"st") != NULL)) {
#endif
	map3d_xy(zaxis_x, zaxis_y, z_max3d + (z_max3d - base_z) / 4, &x, &y);

	x += zlabel.xoffset * t->h_char;
	y += zlabel.yoffset * t->v_char;

	write_multiline(x, y, zlabel.text, CENTRE, CENTRE, 0, zlabel.font);

    }
}


static void
xtick_callback(axis, place, text, grid)
int axis;
double place;
char *text;
struct lp_style_type grid;	/* linetype or -2 for none */
{
    unsigned int x, y, x1, y1;
    double scale = (text ? ticscale : miniticscale);
    int dirn = tic_in ? 1 : -1;
    register struct termentry *t = term;

#ifdef PM3D
    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
	(surface_rot_x > 90 && FRONTGRID != whichgrid)) {
#endif

    map3d_xy(place, xaxis_y, base_z, &x, &y);
    if (grid.l_type > -2) {
	term_apply_lp_properties(&grid);
	/* to save mapping twice, map non-axis y */
	map3d_xy(place, y_min3d + y_max3d - xaxis_y, base_z, &x1, &y1);
	draw_clip_line(x, y, x1, y1);
	term_apply_lp_properties(&border_lp);
    }
    if (xtics & TICS_ON_AXIS) {
	map3d_xy(place, (y_min3d + y_max3d) / 2, base_z, &x, &y);
    }
    x1 = x + tic_unitx * scale * (t->h_tic) * dirn;
    y1 = y + tic_unity * scale * (t->v_tic) * dirn;
    draw_clip_line(x, y, x1, y1);
    if (text) {
	int just;
	if (tic_unitx < -0.9)
	    just = LEFT;
	else if (tic_unitx < 0.9)
	    just = CENTRE;
	else
	    just = RIGHT;
#if 1
/* HBB 970729: let's see if the 'tic labels collide with axes' problem
 * may be fixed this way: */
	x1 = x - tic_unitx * (t->h_char) * 1;
	y1 = y - tic_unity * (t->v_char) * 1;
#else
	x1 = x - tic_unitx * (t->h_tic) * 2;
	y1 = y - tic_unity * (t->v_tic) * 2;
#endif
	if (!tic_in) {
	    x1 -= tic_unitx * (t->h_tic) * ticscale;
	    y1 -= tic_unity * (t->v_tic) * ticscale;
	}
	clip_put_text_just(x1, y1, text, just);
    }
#ifdef PM3D
    }
    if ((surface_rot_x <= 90 && FRONTGRID != whichgrid) ||
	(surface_rot_x > 90 && BACKGRID != whichgrid)) {
#endif
    if (xtics & TICS_MIRROR) {
	map3d_xy(place, y_min3d + y_max3d - xaxis_y, base_z, &x, &y);
	x1 = x - tic_unitx * scale * (t->h_tic) * dirn;
	y1 = y - tic_unity * scale * (t->v_tic) * dirn;
	draw_clip_line(x, y, x1, y1);
    }
#ifdef PM3D
    }
#endif
}

static void
ytick_callback(axis, place, text, grid)
int axis;
double place;
char *text;
struct lp_style_type grid;
{
    unsigned int x, y, x1, y1;
    double scale = (text ? ticscale : miniticscale);
    int dirn = tic_in ? 1 : -1;
    register struct termentry *t = term;

#ifdef PM3D
    if ((surface_rot_x <= 90 && BACKGRID != whichgrid) ||
	(surface_rot_x > 90 && FRONTGRID != whichgrid)) {
#endif

    map3d_xy(yaxis_x, place, base_z, &x, &y);
    if (grid.l_type > -2) {
	term_apply_lp_properties(&grid);
	map3d_xy(x_min3d + x_max3d - yaxis_x, place, base_z, &x1, &y1);
	draw_clip_line(x, y, x1, y1);
	term_apply_lp_properties(&border_lp);
    }
    if (ytics & TICS_ON_AXIS) {
	map3d_xy((x_min3d + x_max3d) / 2, place, base_z, &x, &y);
    }
    x1 = x + tic_unitx * scale * dirn * (t->h_tic);
    y1 = y + tic_unity * scale * dirn * (t->v_tic);
    draw_clip_line(x, y, x1, y1);
    if (text) {
	int just;
	if (tic_unitx < -0.9)
	    just = LEFT;
	else if (tic_unitx < 0.9)
	    just = CENTRE;
	else
	    just = RIGHT;
#if 1
	/* HBB 970729: same as above in xtics_callback */
	x1 = x - tic_unitx * (t->h_char) * 1;
	y1 = y - tic_unity * (t->v_char) * 1;
#else
	x1 = x - tic_unitx * (t->h_tic) * 2;
	y1 = y - tic_unity * (t->v_tic) * 2;
#endif
	if (!tic_in) {
	    x1 -= tic_unitx * (t->h_tic) * ticscale;
	    y1 -= tic_unity * (t->v_tic) * ticscale;
	}
	clip_put_text_just(x1, y1, text, just);
    }
#ifdef PM3D
    }
    if ((surface_rot_x <= 90 && FRONTGRID != whichgrid) ||
	(surface_rot_x > 90 && BACKGRID != whichgrid)) {
#endif
    if (ytics & TICS_MIRROR) {
	map3d_xy(x_min3d + x_max3d - yaxis_x, place, base_z, &x, &y);
	x1 = x - tic_unitx * scale * (t->h_tic) * dirn;
	y1 = y - tic_unity * scale * (t->v_tic) * dirn;
	draw_clip_line(x, y, x1, y1);
    }
#ifdef PM3D
    }
#endif
}

static void
ztick_callback(axis, place, text, grid)
int axis;
double place;
char *text;
struct lp_style_type grid;
{
/* HBB: inserted some ()'s to shut up gcc -Wall, here and below */
    int len = (text ? ticscale : miniticscale) * (tic_in ? 1 : -1) * (term->h_tic);
    unsigned int x, y;

    if (grid.l_type > -2) {
	unsigned int x1, y1, x2, y2, x3, y3;
	double other_x = x_min3d + x_max3d - zaxis_x;
	double other_y = y_min3d + y_max3d - zaxis_y;
	term_apply_lp_properties(&grid);
	map3d_xy(zaxis_x, zaxis_y, place, &x1, &y1);
	map3d_xy(back_x, back_y, place, &x2, &y2);
	map3d_xy(other_x, other_y, place, &x3, &y3);
	draw_clip_line(x1, y1, x2, y2);
	draw_clip_line(x2, y2, x3, y3);
	term_apply_lp_properties(&border_lp);
    }
    map3d_xy(zaxis_x, zaxis_y, place, &x, &y);
    draw_clip_line(x, y, x + len, y);
    if (text) {
	int x1 = x - (term->h_tic) * 2;
	if (!tic_in)
	    x1 -= (term->h_tic) * ticscale;
	clip_put_text_just(x1, y, text, RIGHT);
    }
    if (ztics & TICS_MIRROR) {
	double other_x = x_min3d + x_max3d - zaxis_x;
	double other_y = y_min3d + y_max3d - zaxis_y;
	map3d_xy(other_x, other_y, place, &x, &y);
	draw_clip_line(x, y, x - len, y);
    }
}

static int
map3d_getposition(pos, what, xpos, ypos, zpos)
struct position *pos;
const char *what;
double* xpos;
double* ypos;
double* zpos;
{
    int screens = 0;		/* need either 0 or 3 screen co-ordinates */

    switch (pos->scalex) {
    case first_axes:
    case second_axes:
	*xpos = LogScale(*xpos, is_log_x, log_base_log_x, what, "x");
	break;
    case graph:
	*xpos = min_array[FIRST_X_AXIS] +
	    *xpos * (max_array[FIRST_X_AXIS] - min_array[FIRST_X_AXIS]);
	break;
    case screen:
	++screens;
    }

    switch (pos->scaley) {
    case first_axes:
    case second_axes:
	*ypos = LogScale(*ypos, is_log_y, log_base_log_y, what, "y");
	break;
    case graph:
	*ypos = min_array[FIRST_Y_AXIS] +
	    *ypos * (max_array[FIRST_Y_AXIS] - min_array[FIRST_Y_AXIS]);
	break;
    case screen:
	++screens;
    }

    switch (pos->scalez) {
    case first_axes:
    case second_axes:
	*zpos = LogScale(*zpos, is_log_z, log_base_log_z, what, "z");
	break;
    case graph:
	*zpos = min_array[FIRST_Z_AXIS] +
	    *zpos * (max_array[FIRST_Z_AXIS] - min_array[FIRST_Z_AXIS]);
	break;
    case screen:
	++screens;
    }

    return screens;
}

void
map3d_position(pos, x, y, what)
struct position *pos;
unsigned int *x, *y;
const char *what;
{
    double xpos = pos->x;
    double ypos = pos->y;
    double zpos = pos->z;
    /* need either 0 or 3 screen co-ordinates */
    int screens = map3d_getposition(pos, what, &xpos, &ypos, &zpos);

    if (screens == 0) {
	map3d_xy(xpos, ypos, zpos, x, y);
	return;
    }
    if (screens != 3) {
	graph_error("Cannot mix screen co-ordinates with other types");
    } {
	register struct termentry *t = term;
	/* HBB 20000914: off-by-one bug. Maximum allowed output is
         * t->?max - 1, not t->?max itself! */
	*x = pos->x * (t->xmax -1) + 0.5;
	*y = pos->y * (t->ymax -1) + 0.5;
    }

    return;
}

static void
map3d_position_r(pos_s, pos_e, x, y, what)
struct position *pos_s;
struct position *pos_e;
unsigned int *x, *y;
const char *what;
{
    double xpos_s = pos_s->x;
    double ypos_s = pos_s->y;
    double zpos_s = pos_s->z;
    int screens1 = map3d_getposition(pos_s, what, &xpos_s, &ypos_s, &zpos_s);

    double xpos_e = pos_e->x;
    double ypos_e = pos_e->y;
    double zpos_e = pos_e->z;
    int screens2 = map3d_getposition(pos_e, what, &xpos_e, &ypos_e, &zpos_e);

    if (screens1 == 0 && screens2 == 0) {
	map3d_xy(xpos_s + xpos_e, ypos_s + ypos_e, zpos_s + zpos_e, x, y);
	return;
    }
    if (screens1 != 3 || screens2 != 3) {
	graph_error("Cannot mix screen co-ordinates with other types");
    } {
	register struct termentry *t = term;
	*x = pos_s->x + pos_e->x * (t->xmax -1) + 0.5;
	*y = pos_s->y + pos_e->y * (t->ymax -1) + 0.5;
    }
    /* TODO:
     *   would it make any sense in 3d to mix screen
     *   with non-screen coordinates ? (joze)
     */

    return;
}

/*
 * these code blocks were moved to functions, to make the code simpler
 */

static void
key_text(xl, yl, text)
int xl, yl;
char *text;
{
    if (key_just == JLEFT && key == -1) {
	(*term->justify_text) (LEFT);
	(*term->put_text) (xl + key_text_left, yl, text);
    } else {
	if ((*term->justify_text) (RIGHT)) {
	    if (key == 1)
		clip_put_text(xl + key_text_right, yl, text);
	    else
		(*term->put_text) (xl + key_text_right, yl, text);
	} else {
	    int x = xl + key_text_right - (term->h_char) * strlen(text);
	    if (key == 1) {
		if (i_inrange(x, xleft, xright))
		    clip_put_text(x, yl, text);
	    } else {
		(*term->put_text) (x, yl, text);
	    }
	}
    }
}

static void
key_sample_line(xl, yl)
int xl, yl;
{
    if (key == -1) {
	(*term->move) (xl + key_sample_left, yl);
	(*term->vector) (xl + key_sample_right, yl);
    } else {
	/* HBB 981118: avoid crash if hidden3d sees a manually placed key:
	 * simply turn off hidden-lining while we're drawing the key line: */
	int save_hidden_active = hidden_active;
	hidden_active = FALSE;

	clip_move(xl + key_sample_left, yl);
	clip_vector(xl + key_sample_right, yl);
	hidden_active = save_hidden_active;
    }
}

static void
key_sample_point(xl, yl, pointtype)
int xl, yl;
int pointtype;
{
    /* HBB 20000412: fixed incorrect clipping: the point sample was
     * clipped against the graph box, even if in 'below' or 'outside'
     * position. But the result of that clipping was utterly ignored,
     * because the 'else' part did exactly the same thing as the
     * 'then' one. Some callers of this routine thus did their own
     * clipping, which I removed, along with this change.
     *
     * Now, all 'automatically' placed cases will never be clipped,
     * only user-specified ones. */
    if ((key == -1)            /* ==-1 means auto-placed key */
	|| !clip_point(xl + key_point_offset, yl)) {
	(*term->point) (xl + key_point_offset, yl, pointtype);
    }
}
