/* GNUPLOT - gadgets.c */

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

#include "gadgets.h"
#include "alloc.h"
#include "command.h"
#include "graph3d.h" /* for map3d_position_r() */
#include "graphics.h"
#include "plot3d.h" /* For is_plot_with_palette() */
#include "axis.h" /* For CB_AXIS */

#include "pm3d.h"

/* This file contains mainly a collection of global variables that
 * describe the status of several parts of the gnuplot plotting engine
 * that are used by both 2D and 3D plots, and thus belong neither to
 * graphics.c nor graph3d.c, alone. This is not a very clean solution,
 * but better than mixing internal status and the user interface as we
 * used to have it, in set.c and setshow.h */

legend_key keyT = DEFAULT_KEY_PROPS;

/* Description of the color box associated with CB_AXIS */
color_box_struct color_box; /* initialized in init_color() */
color_box_struct default_color_box = {SMCOLOR_BOX_DEFAULT, 'v', 1, LT_BLACK, LAYER_FRONT, 0,
					{screen, screen, screen, 0.90, 0.2, 0.0},
					{screen, screen, screen, 0.05, 0.6, 0.0}, FALSE,
					{0,0,0,0} };

/* The graph box (terminal coordinates) calculated by boundary() or boundary3d() */
BoundingBox plot_bounds;

/* The bounding box for 3D plots prior to applying view transformations */
BoundingBox page_bounds;

/* The bounding box for the entire drawable area  of current terminal */
BoundingBox canvas;

/* The bounding box against which clipping is to be done */
BoundingBox *clip_area = &plot_bounds;

/* 'set size', 'set origin' settings */
float xsize = 1.0;		/* scale factor for size */
float ysize = 1.0;		/* scale factor for size */
float zsize = 1.0;		/* scale factor for size */
float xoffset = 0.0;		/* x origin */
float yoffset = 0.0;		/* y origin */
float aspect_ratio = 0.0;	/* don't attempt to force it */
int aspect_ratio_3D = 0;	/* 2 will put x and y on same scale, 3 for z also */

/* EAM Augest 2006 - 
   redefine margin as t_position so that absolute placement is possible */
/* space between left edge and plot_bounds.xleft in chars (-1: computed) */
t_position lmargin = DEFAULT_MARGIN_POSITION;
/* space between bottom and plot_bounds.ybot in chars (-1: computed) */
t_position bmargin = DEFAULT_MARGIN_POSITION;
/* space between right edge and plot_bounds.xright in chars (-1: computed) */
t_position rmargin = DEFAULT_MARGIN_POSITION;
/* space between top edge and plot_bounds.ytop in chars (-1: computed) */
t_position tmargin = DEFAULT_MARGIN_POSITION;

/* Pointer to first 'set dashtype' definition in linked list */
struct custom_dashtype_def *first_custom_dashtype = NULL;

/* Pointer to the start of the linked list of 'set label' definitions */
struct text_label *first_label = NULL;

/* Pointer to first 'set linestyle' definition in linked list */
struct linestyle_def *first_linestyle = NULL;
struct linestyle_def *first_perm_linestyle = NULL;
struct linestyle_def *first_mono_linestyle = NULL;

/* Pointer to first 'set style arrow' definition in linked list */
struct arrowstyle_def *first_arrowstyle = NULL;

/* Listhead for pixmaps */
t_pixmap *pixmap_listhead = NULL;

/* Holds the properties from 'set style parallelaxis' */
struct pa_style parallel_axis_style = DEFAULT_PARALLEL_AXIS_STYLE;

/* Holds properties for 'set style spiderplot' */
struct spider_web spiderplot_style = DEFAULT_SPIDERPLOT_STYLE;

/* set arrow */
struct arrow_def *first_arrow = NULL;

/* Pointer to first object instance in linked list */
struct object *first_object = NULL;
/* Pointer to array of grid walls */
struct object grid_wall[5] = {WALL_Y0, WALL_X0, WALL_Y1, WALL_X1, WALL_Z0};

/* 'set title' status */
text_label title = EMPTY_LABELSTRUCT;

/* 'set timelabel' status */
text_label timelabel = EMPTY_LABELSTRUCT;
int timelabel_bottom = TRUE;

/* flag for polar mode */
TBOOLEAN polar = FALSE;
TBOOLEAN inverted_raxis = FALSE;

/* toggle spiderplot mode on/off */
TBOOLEAN spiderplot = FALSE;

/* zero threshold, may _not_ be 0! */
double zero = ZERO;

/* Status of 'set pointsize' and 'set pointintervalbox' commands */
double pointsize = 1.0;
double pointintervalbox = 1.0;

/* used for filled points */
t_colorspec background_fill = BACKGROUND_COLORSPEC;

/* box width (automatic) for plot style "with boxes" */
double   boxwidth              = -1.0;
/* whether box width is absolute (default) or relative */
TBOOLEAN boxwidth_is_absolute  = TRUE;

/* set border */
int draw_border = 31;	/* The current settings */
int user_border = 31;	/* What the user last set explicitly */
int border_layer = LAYER_FRONT;
# define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }
struct lp_style_type border_lp = DEFAULT_BORDER_LP;
const struct lp_style_type default_border_lp = DEFAULT_BORDER_LP;
const struct lp_style_type background_lp = {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN};

/* set clip */
TBOOLEAN clip_lines1 = TRUE;
TBOOLEAN clip_lines2 = FALSE;
TBOOLEAN clip_points = FALSE;
TBOOLEAN clip_radial = FALSE;

static int clip_line(int *, int *, int *, int *);

/* set samples */
int samples_1 = SAMPLES;
int samples_2 = SAMPLES;

/* set angles */
double ang2rad = 1.0;		/* 1 or pi/180, tracking angles_format */

enum PLOT_STYLE data_style = POINTSTYLE;
enum PLOT_STYLE func_style = LINES;

TBOOLEAN parametric = FALSE;
TBOOLEAN in_parametric = FALSE;

/* If last plot was a 3d one. */
TBOOLEAN is_3d_plot = FALSE;

/* Flag to signal that the existing data is valid for a quick refresh */
TRefresh_Allowed refresh_ok = E_REFRESH_NOT_OK;
/* FIXME: do_plot should be able to figure this out on its own! */
int refresh_nplots = 0;
/* Flag to show that volatile input data is present */
TBOOLEAN volatile_data = FALSE;

fill_style_type default_fillstyle = { FS_EMPTY, 100, 0, DEFAULT_COLORSPEC } ;

/* Default rectangle style - background fill, black border */
struct object default_rectangle = DEFAULT_RECTANGLE_STYLE;
struct object default_circle = DEFAULT_CIRCLE_STYLE;
struct object default_ellipse = DEFAULT_ELLIPSE_STYLE;

/* filledcurves style options */
filledcurves_opts filledcurves_opts_data = EMPTY_FILLEDCURVES_OPTS;
filledcurves_opts filledcurves_opts_func = EMPTY_FILLEDCURVES_OPTS;

/* Prefer line styles over plain line types */
TBOOLEAN prefer_line_styles = FALSE;

/* If current terminal claims to be monochrome, don't try to send it colors */
#define monochrome_terminal ((t->flags & TERM_MONOCHROME) != 0)

histogram_style histogram_opts = DEFAULT_HISTOGRAM_STYLE;

boxplot_style boxplot_opts = DEFAULT_BOXPLOT_STYLE;

/* WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...) */
int current_x11_windowid = 0;

textbox_style textbox_opts[NUM_TEXTBOX_STYLES];

/*****************************************************************/
/* Routines that deal with global objects defined in this module */
/*****************************************************************/

/* Clipping to the bounding box: */

/* Test a single point to be within the BoundingBox.
 * Sets the returned integers 4 l.s.b. as follows:
 * bit 0 if to the left of xleft.
 * bit 1 if to the right of xright.
 * bit 2 if below of ybot.
 * bit 3 if above of ytop.
 * 0 is returned if inside.
 */
int
clip_point(int x, int y)
{
    int ret_val = 0;

    if (!clip_area)
	return 0;
    if ((int)x < clip_area->xleft)
	ret_val |= 0x01;
    if ((int)x > clip_area->xright)
	ret_val |= 0x02;
    if ((int)y < clip_area->ybot)
	ret_val |= 0x04;
    if ((int)y > clip_area->ytop)
	ret_val |= 0x08;

    return ret_val;
}

/* Clip the given line to drawing coords defined by BoundingBox.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 */
int
draw_clip_line(int x1, int y1, int x2, int y2)
{
    struct termentry *t = term;
    int state;

    state = clip_line(&x1, &y1, &x2, &y2);
    if (state != 0) {
	(*t->move) (x1, y1);
	(*t->vector) (x2, y2);
    }

    return state;
}

/* Draw a contiguous line path which may be clipped. Compared to
 * draw_clip_line(), this routine moves to a coordinate only when
 * necessary.
 */
void 
draw_clip_polygon(int points, gpiPoint *p) 
{
    int i;
    int x1, y1, x2, y2;
    int pos1, pos2, clip_ret;
    struct termentry *t = term;
    TBOOLEAN continuous = TRUE;

    if (points <= 1) 
	return;

    if (p[0].x != p[points-1].x || p[0].y != p[points-1].y)
	continuous = FALSE;

    x1 = p[0].x;
    y1 = p[0].y;
    pos1 = clip_point(x1, y1);
    if (!pos1) /* move to first point if it is inside */
	(*t->move)(x1, y1);

    newpath();

    for (i = 1; i < points; i++) {
	x2 = p[i].x;
	y2 = p[i].y;
	pos2 = clip_point(x2, y2);
	clip_ret = clip_line(&x1, &y1, &x2, &y2);

	if (clip_ret) {
	    /* there is a line to draw */
	    if (pos1) /* first vertex was recalculated, move to new start point */
		(*t->move)(x1, y1);
	    (*t->vector)(x2, y2);
	} else {
	    /* Path is not continuous; make sure closepath is not called */
	    continuous = FALSE;
	}

	x1 = p[i].x;
	y1 = p[i].y;
	/* The end point and the line do not necessarily have the same
	 * status. The end point can be 'inside', but the whole line is
	 * 'outside'. Do not update pos1 in this case.  Bug #1268.
	 */
	if (!(clip_ret == 0 && pos2 == 0))
	    pos1 = pos2;
    }

    /* Only call closepath if the polygon is truly closed; otherwise */
    /* a spurious line connecting the start and end is generated.    */
    if (continuous)
	closepath();
}

/*
 * arrow is specified in terminal coordinates
 * but we use double rather than int so that the precision is sufficient
 * to orient and draw the arrow head correctly even for very short vectors.
 */
void
draw_clip_arrow( double dsx, double dsy, double dex, double dey, t_arrow_head head)
{
    struct termentry *t = term;

    int sx = axis_map_toint(dsx);
    int sy = axis_map_toint(dsy);
    int ex = axis_map_toint(dex);
    int ey = axis_map_toint(dey);
    int dx, dy;

    /* Don't draw head if the arrow itself is clipped */
    if (clip_point(sx,sy))
	head &= ~BACKHEAD;
    if (clip_point(ex,ey))
	head &= ~END_HEAD;

    /* clip_line returns 0 if the whole thing is out of range */
    if (!clip_line(&sx, &sy, &ex, &ey))
	return;

    /* Special case code for short vectors */
    /* Most terminals are OK with using this code for long vectors also.	*/
    /* However some terminals (e.g. tikz) look terrible because the shaft of a	*/
    /* long vector overruns the head when the head is drawn with HEADS_ONLY.	*/
    /* FIXME:  this is a very ad hoc definition of "short".			*/
    dx = abs(ex-sx);
    dy = abs(ey-sy);
    if (dx < 25 && dy < 25) {

	/* draw the body of the vector (rounding errors are a problem) */
	if (dx > 1 || dy > 1)
	    if (!((t->flags & TERM_IS_LATEX)))
		(*t->arrow)(sx, sy, ex, ey, SHAFT_ONLY | head);

	/* if we're not supposed to be drawing any heads, we're done */
	if ((head & BOTH_HEADS) == NOHEAD)
	    return;

	/* If this is truly a 0-vector, then we CAN'T draw the head */
	/* because the orientation of the head is indeterminate     */
	if (dsx == dex && dsy == dey)
	    return;

	/* If the head size is fixed we are free to change the length of */
	/* very short vectors so that the orientation is accurate.	 */
	if (curr_arrow_headfixedsize) {
	    /* Direction vector in (dex,dey). I need to convert this to integers
	     * with a scale that's large-enough to give me good angular resolution,
	     * but small-enough to not overflow the data type.
	     */
	    double rescale = 1000. / GPMAX( fabs(dex-dsx), fabs(dey-dsy) );
	    int newlenx = (dex - dsx) * rescale;
	    int newleny = (dey - dsy) * rescale;

	    if (head & END_HEAD)
		(*t->arrow)(ex - newlenx, ey - newleny, ex, ey, END_HEAD|HEADS_ONLY);
	    if (head & BACKHEAD)
		(*t->arrow)(sx, sy, sx + newlenx, sy + newleny, BACKHEAD|HEADS_ONLY);
	} else {
	    (*t->arrow)(sx, sy, ex, ey, head|HEADS_ONLY);
	}
    } else {
	/* The normal case, draw the whole thing at once */
	(*t->arrow)(sx, sy, ex, ey, head);
    }
}

/* Clip the given line to drawing coords defined by BoundingBox.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 * Return 0: entire line segment is outside bounding box
 *        1: entire line segment is inside bounding box
 *       -1: line segment has been clipped to bounding box
 */
int
clip_line(int *x1, int *y1, int *x2, int *y2)
{
    /* Apr 2014: This algorithm apparently assumed infinite precision
     * integer arithmetic. It was failing when passed coordinates that
     * were hugely out of bounds because tests for signedness of the
     * form (dx * dy > 0) would overflow rather than correctly evaluating
     * to (sign(dx) == sign(dy)).  Worse yet, the numerical values are
     * used to determine which end of the segment to update.
     * This is now addressed by making dx and dy (double) rather than (int)
     * but it might be better to hard-code the sign tests.
     */
    double dx, dy, x, y;

    int x_intr[4], y_intr[4], count, pos1, pos2;
    int x_max, x_min, y_max, y_min;
    pos1 = clip_point(*x1, *y1);
    pos2 = clip_point(*x2, *y2);
    if (!pos1 && !pos2)
	return 1;		/* segment is totally in */
    if (pos1 & pos2)
	return 0;		/* segment is totally out. */
    /* Here part of the segment MAY be inside. test the intersection
     * of this segment with the 4 boundaries for hopefully 2 intersections
     * in. If none are found segment is totaly out.
     * Under rare circumstances there may be up to 4 intersections (e.g.
     * when the line passes directly through at least one corner).
     */
    count = 0;
    dx = *x2 - *x1;
    dy = *y2 - *y1;
    /* Find intersections with the x parallel bbox lines: */
    if (dy != 0) {
	x = (clip_area->ybot - *y2) * dx / dy + *x2;	/* Test for clip_area->ybot boundary. */
	if (x >= clip_area->xleft && x <= clip_area->xright) {
	    x_intr[count] = x;
	    y_intr[count++] = clip_area->ybot;
	}
	x = (clip_area->ytop - *y2) * dx / dy + *x2;	/* Test for clip_area->ytop boundary. */
	if (x >= clip_area->xleft && x <= clip_area->xright) {
	    x_intr[count] = x;
	    y_intr[count++] = clip_area->ytop;
	}
    }
    /* Find intersections with the y parallel bbox lines: */
    if (dx != 0) {
	y = (clip_area->xleft - *x2) * dy / dx + *y2;	/* Test for clip_area->xleft boundary. */
	if (y >= clip_area->ybot && y <= clip_area->ytop) {
	    x_intr[count] = clip_area->xleft;
	    y_intr[count++] = y;
	}
	y = (clip_area->xright - *x2) * dy / dx + *y2;	/* Test for clip_area->xright boundary. */
	if (y >= clip_area->ybot && y <= clip_area->ytop) {
	    x_intr[count] = clip_area->xright;
	    y_intr[count++] = y;
	}
    }
    if (count < 2)
	return 0;

    /* check which intersections to use, for more than two intersections the first two may be identical */
    if ((count > 2) && (x_intr[0] == x_intr[1]) && (y_intr[0] == y_intr[1])) {
	x_intr[1] = x_intr[2];
	y_intr[1] = y_intr[2];
    }	

    if (*x1 < *x2) {
	x_min = *x1;
	x_max = *x2;
    } else {
	x_min = *x2;
	x_max = *x1;
    }
    if (*y1 < *y2) {
	y_min = *y1;
	y_max = *y2;
    } else {
	y_min = *y2;
	y_max = *y1;
    }

    if (pos1 && pos2) {		/* Both were out - update both */
	/* EAM Sep 2008 - preserve direction of line segment */
	if ((dx*(x_intr[1]-x_intr[0]) < 0)
	||  (dy*(y_intr[1]-y_intr[0]) < 0)) {
	    *x1 = x_intr[1];
	    *y1 = y_intr[1];
	    *x2 = x_intr[0];
	    *y2 = y_intr[0];
	} else {
	    *x1 = x_intr[0];
	    *y1 = y_intr[0];
	    *x2 = x_intr[1];
	    *y2 = y_intr[1];
	}
    } else if (pos1) {		/* Only x1/y1 was out - update only it */
	if (dx * (*x2 - x_intr[0]) + dy * (*y2 - y_intr[0]) > 0) {
	    *x1 = x_intr[0];
	    *y1 = y_intr[0];
	} else {
	    *x1 = x_intr[1];
	    *y1 = y_intr[1];
	}
    } else {			/* Only x2/y2 was out - update only it */
	/* Same difference here, again */
	if (dx * (x_intr[0] - *x1) + dy * (y_intr[0] - *y1) > 0) {
	    *x2 = x_intr[0];
	    *y2 = y_intr[0];
	} else {
	    *x2 = x_intr[1];
	    *y2 = y_intr[1];
	}
    }

    if (*x1 < x_min || *x1 > x_max || *x2 < x_min || *x2 > x_max || *y1 < y_min || *y1 > y_max || *y2 < y_min || *y2 > y_max)
	return 0;

    return -1;
}

/* test if coordinates of a vertex are inside boundary box. The start
   and end points for the clip_boundary must be in correct order for
   this to work properly (see respective definitions in clip_polygon()). */
TBOOLEAN
vertex_is_inside(gpiPoint test_vertex, gpiPoint *clip_boundary)
{
    if (clip_boundary[1].x > clip_boundary[0].x)              /*bottom edge*/
	if (test_vertex.y >= clip_boundary[0].y) return TRUE;
    if (clip_boundary[1].x < clip_boundary[0].x)              /*top edge*/
	if (test_vertex.y <= clip_boundary[0].y) return TRUE;
    if (clip_boundary[1].y > clip_boundary[0].y)              /*right edge*/
	if (test_vertex.x <= clip_boundary[1].x) return TRUE;
    if (clip_boundary[1].y < clip_boundary[0].y)              /*left edge*/
	if (test_vertex.x >= clip_boundary[1].x) return TRUE;
    return FALSE;
} 

void
intersect_polyedge_with_boundary(gpiPoint first, gpiPoint second, gpiPoint *intersect, gpiPoint *clip_boundary)
{
    /* this routine is called only if one point is outside and the other
       is inside, which implies that clipping is needed at a horizontal
       boundary, that second.y is different from first.y and no division
       by zero occurs. Same for vertical boundary and x coordinates. */
    if (clip_boundary[0].y == clip_boundary[1].y) { /* horizontal */
	(*intersect).y = clip_boundary[0].y;
	(*intersect).x = first.x + (clip_boundary[0].y - first.y) * (second.x - first.x)/(second.y - first.y);
    } else { /* vertical */
	(*intersect).x = clip_boundary[0].x;
	(*intersect).y = first.y + (clip_boundary[0].x - first.x) * (second.y - first.y)/(second.x - first.x);
    }
}

/* Clip the given polygon to a single edge of the bounding box. */
void 
clip_polygon_to_boundary(gpiPoint *in, gpiPoint *out, int in_length, int *out_length, gpiPoint *clip_boundary)
{
    gpiPoint prev, curr; /* start and end point of current polygon edge. */
    int j;

    *out_length = 0;
    if (in_length <= 0)
	return;
    else
	prev = in[in_length - 1]; /* start with the last vertex */

    for (j = 0; j < in_length; j++) {
	curr = in[j];
	if (vertex_is_inside(curr, clip_boundary)) {
	    if (vertex_is_inside(prev, clip_boundary)) {
		/* both are inside, add current vertex */
		out[*out_length] = in[j];
		(*out_length)++;
	    } else {
		/* changed from outside to inside, add intersection point and current point */
		intersect_polyedge_with_boundary(prev, curr, out+(*out_length), clip_boundary);
		out[*out_length+1] = curr;
		*out_length += 2;
	    }
	} else {
	    if (vertex_is_inside(prev, clip_boundary)) {
		/* changed from inside to outside, add intersection point */
		intersect_polyedge_with_boundary(prev, curr, out+(*out_length), clip_boundary);
		(*out_length)++;
	    }
	}
	prev = curr;
    }
}

/* Clip the given polygon to drawing coords defined by BoundingBox.
 * This routine uses the Sutherland-Hodgman algorithm.  When calling
 * this function, you must make sure that you reserved enough
 * memory for the output polygon. out_length can be as big as
 * 2*(in_length - 1)
 */
void
clip_polygon(gpiPoint *in, gpiPoint *out, int in_length, int *out_length)
{
    int i;
    gpiPoint clip_boundary[5];
    static gpiPoint *tmp_corners = NULL;

    if (!clip_area || in_length < 3) {
	memcpy(out, in, in_length * sizeof(gpiPoint));
	*out_length = in_length;
	return;
    }
    tmp_corners = gp_realloc(tmp_corners, 4 * in_length * sizeof(gpiPoint), "clip_polygon");

    /* vertices of the rectangular clipping window starting from
       top-left in counterclockwise direction */
    clip_boundary[0].x = clip_area->xleft;  /* top left */
    clip_boundary[0].y = clip_area->ytop;
    clip_boundary[1].x = clip_area->xleft;  /* bottom left */
    clip_boundary[1].y = clip_area->ybot;
    clip_boundary[2].x = clip_area->xright; /* bottom right */
    clip_boundary[2].y = clip_area->ybot;
    clip_boundary[3].x = clip_area->xright; /* top right */
    clip_boundary[3].y = clip_area->ytop;
    clip_boundary[4] = clip_boundary[0];

    memcpy(tmp_corners, in, in_length * sizeof(gpiPoint));
    for (i = 0; i < 4; i++) {
	clip_polygon_to_boundary(tmp_corners, out, in_length, out_length, clip_boundary+i);
	memcpy(tmp_corners, out, *out_length * sizeof(gpiPoint));
	in_length = *out_length;
    }
}

/* Two routines to emulate move/vector sequence using line drawing routine. */
static int move_pos_x, move_pos_y;

void
clip_move(int x, int y)
{
    move_pos_x = x;
    move_pos_y = y;
}

void
clip_vector(int x, int y)
{
    draw_clip_line(move_pos_x, move_pos_y, x, y);
    move_pos_x = x;
    move_pos_y = y;
}

/*
 * draw_polar_clip_line() assumes that the endpoints have already
 * been categorized as INRANGE/OUTRANGE, and that "set clip radial"
 * is in effect.
 */
void
draw_polar_clip_line( double xbeg, double ybeg, double xend, double yend)
{
    double R;			/* radius of limiting circle */
    double a, b;		/* line expressed as y = a*x + b */
    double x1, y1, x2, y2;	/* Intersections of line and circle */
    double Q, Q2;		/* sqrt term of quadratic equation */
    TBOOLEAN vertical = FALSE;	/* flag for degenerate case */
    TBOOLEAN beg_inrange, end_inrange;

    if (R_AXIS.set_max == -VERYLARGE)
	goto outside;
    R = R_AXIS.set_max - R_AXIS.set_min;

    /* If both endpoints are inside the limiting circle, draw_clip_line suffices */
    beg_inrange = (xbeg*xbeg + ybeg*ybeg) <= R*R;
    end_inrange = (xend*xend + yend*yend) <= R*R;
    if (beg_inrange && end_inrange) {
	draw_clip_line(map_x(xbeg), map_y(ybeg), map_x(xend), map_y(yend));
	return;
    }

    /* FIXME:  logscale and other odd cases are not covered by this equation */
    if (fabs(xbeg - xend) > ZERO) {
	/* Recast line in the form y = a*x + b */
	a = (yend - ybeg) / (xend - xbeg);
	b = ybeg - xbeg * a;
	/* the line may intersect a circle of radius R in two places */
	Q2 = 4*a*a*b*b - 4 * (1 + a*a) * (b*b - R*R);
	if (Q2 < 0)
	    goto outside;
	Q = sqrt(Q2);
	x1 = (-2*a*b + Q) / ( 2*(1+a*a));
	x2 = (-2*a*b - Q) / ( 2*(1+a*a));
	y1 = a * x1 + b;
	y2 = a * x2 + b;
    } else {
	/* degenerate case (vertical line) */
	x1 = x2 = xbeg;
	if (fabs(x1) > R)
	    goto outside;
	vertical = TRUE;
	y1 = sqrt(R*R - x1*x1);
	y2 = -y1;
	if (!inrange(y1,ybeg,yend) && !inrange(y2,ybeg,yend))
	    goto outside;
    }

    /* If one of the original endpoints was INRANGE then use it */
    /* rather than the second intersection point.               */
    if (vertical) {
	y1 = GPMIN(y1, GPMAX(ybeg, yend));
	y2 = GPMAX(y2, GPMIN(ybeg, yend));
    } else if (beg_inrange) {
	if (!inrange(x1, xbeg, xend)) {
	    x1 = xbeg;
	    y1 = ybeg;
	} else {
	    x2 = xbeg;
	    y2 = ybeg;
	}
    } else if (end_inrange) {
	if (!inrange(x1, xbeg, xend)) {
	    x1 = xend;
	    y1 = yend;
	} else {
	    x2 = xend;
	    y2 = yend;
	}
    } else {
	/* Both OUTRANGE. Are they on the same side of the circle? */
	if (!inrange(x1, xbeg, xend))
	    goto outside;
    }

    /* Draw the part of the line inside the bounding circle */
    (term->move)(map_x(x1), map_y(y1));
    (term->vector)(map_x(x2), map_y(y2));
    /* fall through */

outside:
    /* Leave current position at unclipped endpoint */
    (term->move)(map_x(xend), map_y(yend));
    return;
}

/* Common routines for setting text or line color from t_colorspec */

void
apply_pm3dcolor(struct t_colorspec *tc)
{
    struct termentry *t = term;
    double cbval;

    /* V5 - term->linetype(LT_BLACK) would clobber the current	*/
    /* dashtype so instead we use term->set_color(black).	*/
    static t_colorspec black = BLACK_COLORSPEC; 

    /* Replace colorspec with that of the requested line style */
    struct lp_style_type style;
    if (tc->type == TC_LINESTYLE) {
	lp_use_properties(&style, tc->lt);
	tc = &style.pm3d_color;
    }

    if (tc->type == TC_DEFAULT) {
	t->set_color(&black);
	return;
    }
    if (tc->type == TC_LT) {
	t->set_color(tc);
	return;
    }
    if (tc->type == TC_RGB) {
	/* FIXME: several plausible ways for monochrome terminals to handle color request
	 * (1) Allow all color requests despite the label "monochrome"
	 * (2) Choose any color you want so long as it is black
	 * (3) Convert colors to gray scale (NTSC?)
	 */
	/* Monochrome terminals are still allowed to display rgb variable colors */
	if (monochrome_terminal && tc->value >= 0)
	    t->set_color(&black);
	else
	    t->set_color(tc);
	return;
    }
    /* Leave unchanged. (used only by "set errorbars"??) */
    if (tc->type == TC_VARIABLE)
	return;

    switch (tc->type) {
	case TC_Z:
		set_color(cb2gray(tc->value));
		break;
	case TC_CB:
		if (CB_AXIS.log)
		    cbval = (tc->value <= 0) ? CB_AXIS.min : tc->value;
		else
		    cbval = tc->value;
		set_color(cb2gray(cbval));
		break;
	case TC_FRAC:
		set_color(sm_palette.positive == SMPAL_POSITIVE ?  tc->value : 1-tc->value);
		break;
	default:
		break; /* cannot happen */
    }
}

void
reset_textcolor(const struct t_colorspec *tc)
{
    if (tc->type != TC_DEFAULT)
	term->linetype(LT_BLACK);
}


void
default_arrow_style(struct arrow_style_type *arrow)
{
    static const struct lp_style_type tmp_lp_style = 
	{0, LT_DEFAULT, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR,
	DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN};

    arrow->tag = -1;
    arrow->layer = LAYER_BACK;
    arrow->lp_properties = tmp_lp_style;
    arrow->head = 1;
    arrow->head_length = 0.0;
    arrow->head_lengthunit = first_axes;
    arrow->head_angle = 15.0;
    arrow->head_backangle = 90.0;
    arrow->headfill = AS_NOFILL;
    arrow->head_fixedsize = FALSE;
}

void
apply_head_properties(struct arrow_style_type *arrow_properties)
{
    curr_arrow_headfilled = arrow_properties->headfill;
    curr_arrow_headfixedsize = arrow_properties->head_fixedsize;
    curr_arrow_headlength = 0;
    if (arrow_properties->head_length > 0) {
	/* set head length+angle for term->arrow */
	double xtmp, ytmp;
	struct position headsize = {first_axes,graph,graph,0.,0.,0.};

	headsize.x = arrow_properties->head_length;
	headsize.scalex = arrow_properties->head_lengthunit;

	map_position_r(&headsize, &xtmp, &ytmp, "arrow");

	curr_arrow_headangle = arrow_properties->head_angle;
	curr_arrow_headbackangle = arrow_properties->head_backangle;
	curr_arrow_headlength = xtmp;
    }
}

void
free_labels(struct text_label *label)
{
struct text_label *temp;
char *master_font = label->font;

    /* Labels generated by 'plot with labels' all use the same font */
    if (master_font)
    	free(master_font);

    while (label) {
	if (label->text)
	    free(label->text);
	if (label->font && label->font != master_font)
	    free(label->font);
	temp=label->next;
	free(label);
	label = temp;
    }

}

void
get_offsets(
    struct text_label *this_label,
    int *htic, int *vtic)
{
    if ((this_label->lp_properties.flags & LP_SHOW_POINTS)) {
	*htic = (pointsize * term->h_tic * 0.5);
	*vtic = (pointsize * term->v_tic * 0.5);
    } else {
	*htic = 0;
	*vtic = 0;
    }
    if (is_3d_plot) {
	int htic2, vtic2;
	map3d_position_r(&(this_label->offset), &htic2, &vtic2, "get_offsets");
	*htic += htic2;
	*vtic += vtic2;
    } else {
	double htic2, vtic2;
	map_position_r(&(this_label->offset), &htic2, &vtic2, "get_offsets");
	*htic += (int)htic2;
	*vtic += (int)vtic2;
    }
}


/*
 * Write one label, with all the trimmings.
 * This routine is used for both 2D and 3D plots.
 */
void
write_label(int x, int y, struct text_label *this_label)
{
	int htic, vtic;
	int justify = JUST_TOP;	/* This was the 2D default; 3D had CENTRE */
	textbox_style *textbox = NULL;

	apply_pm3dcolor(&(this_label->textcolor));
	ignore_enhanced(this_label->noenhanced);

	/* The text itself */
	if (this_label->hypertext) {
	    if (this_label->text && *(this_label->text)) {
		/* Treat text as hypertext */
		char *font = this_label->font;
		if (font)
		    term->set_font(font);
		if (term->hypertext)
		    term->hypertext(TERM_HYPERTEXT_TOOLTIP, this_label->text);
		if (font)
		    term->set_font("");
	    }
	} else {
	    /* A normal label (always print text) */
	    get_offsets(this_label, &htic, &vtic);
	    if (this_label->boxed < 0)
		textbox = &textbox_opts[0];
	    else if (this_label->boxed > 0)
		textbox = &textbox_opts[this_label->boxed];
	    /* Initialize the bounding box accounting */
	    if (textbox && term->boxed_text && (textbox->opaque || !textbox->noborder))
		(*term->boxed_text)(x + htic, y + vtic, TEXTBOX_INIT);
	    if (this_label->rotate && (*term->text_angle) (this_label->rotate)) {
		write_multiline(x + htic, y + vtic, this_label->text,
				this_label->pos, justify, this_label->rotate,
				this_label->font);
		(*term->text_angle) (0);
	    } else {
		write_multiline(x + htic, y + vtic, this_label->text,
				this_label->pos, justify, 0, this_label->font);
	    }
	}
	if (textbox && term->boxed_text && (textbox->opaque || !textbox->noborder))
	{

	    /* Adjust the bounding box margins */
	    (*term->boxed_text)((int)(textbox->xmargin * 100.),
		(int)(textbox->ymargin * 100.), TEXTBOX_MARGINS);

	    /* Blank out the box and reprint the label */
	    if (textbox->opaque) {
		apply_pm3dcolor(&textbox->fillcolor);
		(*term->boxed_text)(0,0, TEXTBOX_BACKGROUNDFILL);
		apply_pm3dcolor(&(this_label->textcolor));
		/* Init for each of fill and border */
		if (!textbox->noborder)
		    (*term->boxed_text)(x + htic, y + vtic, TEXTBOX_INIT);
		if (this_label->rotate && (*term->text_angle) (this_label->rotate)) {
		    write_multiline(x + htic, y + vtic, this_label->text,
				this_label->pos, justify, this_label->rotate,
				this_label->font);
		    (*term->text_angle) (0);
		} else {
		    write_multiline(x + htic, y + vtic, this_label->text,
				this_label->pos, justify, 0, this_label->font);
		}
	    }

	    /* Draw the bounding box */
	    if (!textbox->noborder) {
		(*term->linewidth)(textbox->linewidth);
		apply_pm3dcolor(&textbox->border_color);
		(*term->boxed_text)(0,0, TEXTBOX_OUTLINE);
	    }

	    (*term->boxed_text)(0,0, TEXTBOX_FINISH);
	}

	/* The associated point, if any */
	/* write_multiline() clips text to on_page; do the same for any point */
	if ((this_label->lp_properties.flags & LP_SHOW_POINTS) && on_page(x,y)) {
	    term_apply_lp_properties(&this_label->lp_properties);
	    (*term->point) (x, y, this_label->lp_properties.p_type);
	    /* the default label color is that of border */
	    term_apply_lp_properties(&border_lp);
	}

	ignore_enhanced(FALSE);
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
	len = estimate_strlen(s, NULL);	/* = e-s ? */
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

/*
 * Here so that it can be shared by the 2D and 3D code
 */
void
do_timelabel(int x, int y)
{
    struct text_label temp = timelabel;
    char str[MAX_LINE_LEN+1];
    time_t now;

    if (timelabel.rotate == 0 && !timelabel_bottom)
	y -= term->v_char;

    time(&now);
    strftime(str, MAX_LINE_LEN, timelabel.text, localtime(&now));
    temp.text = str;

    write_label(x, y, &temp);
}

void
init_gadgets()
{
   int i;
   static t_position y0_wall_corners[5] = WALL_Y0_CORNERS;
   static t_position x0_wall_corners[5] = WALL_X0_CORNERS;
   static t_position y1_wall_corners[5] = WALL_Y1_CORNERS;
   static t_position x1_wall_corners[5] = WALL_X1_CORNERS;
   static t_position z0_wall_corners[5] = WALL_Z0_CORNERS;
   for (i=0; i<4; i++)
       grid_wall[i].next = &grid_wall[i+1];

   grid_wall[WALL_Y0_TAG].o.polygon.vertex = y0_wall_corners;
   grid_wall[WALL_X0_TAG].o.polygon.vertex = x0_wall_corners;
   grid_wall[WALL_Y1_TAG].o.polygon.vertex = y1_wall_corners;
   grid_wall[WALL_X1_TAG].o.polygon.vertex = x1_wall_corners;
   grid_wall[WALL_Z0_TAG].o.polygon.vertex = z0_wall_corners;
   grid_wall[WALL_Y0_TAG].lp_properties.pm3d_color.type = TC_RGB;
   grid_wall[WALL_X0_TAG].lp_properties.pm3d_color.type = TC_RGB;
   grid_wall[WALL_Y1_TAG].lp_properties.pm3d_color.type = TC_RGB;
   grid_wall[WALL_X1_TAG].lp_properties.pm3d_color.type = TC_RGB;
   grid_wall[WALL_Z0_TAG].lp_properties.pm3d_color.type = TC_RGB;
   grid_wall[WALL_Y0_TAG].lp_properties.pm3d_color.lt = WALL_Y_COLOR;
   grid_wall[WALL_X0_TAG].lp_properties.pm3d_color.lt = WALL_X_COLOR;
   grid_wall[WALL_Y1_TAG].lp_properties.pm3d_color.lt = WALL_Y_COLOR;
   grid_wall[WALL_X1_TAG].lp_properties.pm3d_color.lt = WALL_X_COLOR;
   grid_wall[WALL_Z0_TAG].lp_properties.pm3d_color.lt = WALL_Z_COLOR;
}

/*
 * walk through the list of objects to see if any require pm3d processing
 */
TBOOLEAN
pm3d_objects(void)
{
    struct object *obj = first_object;
    while (obj) {
	if (obj->layer == LAYER_DEPTHORDER)
	    return TRUE;
	obj = obj->next;
    }
    return FALSE;
}

/*
 * Place overall title on the canvas (shared by plot and splot).
 */
void
place_title(int title_x, int title_y)
{
    if (title.text) {
	/* NB: write_label applies text color but does not reset it */
	write_label(title_x, title_y, &title);
	reset_textcolor(&(title.textcolor));
    }
}
