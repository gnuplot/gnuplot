#ifndef lint
static char *RCSid() { return RCSid("gadgets.c,v 1.1.3.1 2000/05/03 21:47:15 hbb Exp"); }
#endif

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
#include "command.h"
#include "graph3d.h" /* for map3d_position_r() */
#include "graphics.h"
#include "plot3d.h" /* For is_plot_with_palette() */

#include "pm3d.h"

/* This file contains mainly a collection of global variables that
 * used to be in 'set.c', where they didn't really belong. They
 * describe the status of several parts of the gnuplot plotting engine
 * that are used by both 2D and 3D plots, and thus belong neither to
 * graphics.c nor graph3d.c, alone. This is not a very clean solution,
 * but better than mixing internal status and the user interface as we
 * used to have it, in set.c and setshow.h */

legend_key keyT = DEFAULT_KEY_PROPS;

/* The graph box, in terminal coordinates, as calculated by boundary()
 * or boundary3d(): */
/* FIXME HBB 20000521: should probably be unsigned... */
int xleft, xright, ybot, ytop;

/* 'set size', 'set origin' setttings */
float xsize = 1.0;		/* scale factor for size */
float ysize = 1.0;		/* scale factor for size */
float zsize = 1.0;		/* scale factor for size */
float xoffset = 0.0;		/* x origin */
float yoffset = 0.0;		/* y origin */
float aspect_ratio = 0.0;	/* don't attempt to force it */

/* space between left edge and xleft in chars (-1: computed) */
float lmargin = -1;
/* space between bottom and ybot in chars (-1: computed) */
float bmargin = -1;
/* space between right egde and xright in chars (-1: computed) */
float rmargin = -1;
/* space between top egde and ytop in chars (-1: computed) */
float tmargin = -1;

/* File descriptor for output during 'set table' mode */
FILE *table_outfile = NULL;
TBOOLEAN table_mode = FALSE;

/* Pointer to the start of the linked list of 'set label' definitions */
struct text_label *first_label = NULL;

/* Pointer to first 'set linestyle' definition in linked list */
struct linestyle_def *first_linestyle = NULL;

/* Pointer to first 'set style arrow' definition in linked list */
struct arrowstyle_def *first_arrowstyle = NULL;

/* set arrow */
struct arrow_def *first_arrow = NULL;

/* 'set title' status */
text_label title = EMPTY_LABELSTRUCT;

/* 'set timelabel' status */
text_label timelabel = EMPTY_LABELSTRUCT;
int timelabel_rotate = FALSE;
int timelabel_bottom = TRUE;

/* flag for polar mode */
TBOOLEAN polar = FALSE;

/* zero threshold, may _not_ be 0! */
double zero = ZERO;

/* Status of 'set pointsize' command */
double pointsize = 1.0;

/* set border */
int draw_border = 31;
int border_layer = 1;
# define DEFAULT_BORDER_LP { 0, -2, 0, 1.0, 1.0, 0 }
struct lp_style_type border_lp = DEFAULT_BORDER_LP;
const struct lp_style_type default_border_lp = DEFAULT_BORDER_LP;

/* set clip */
TBOOLEAN clip_lines1 = TRUE;
TBOOLEAN clip_lines2 = FALSE;
/* FIXME HBB 20000521: clip_points is only used by 2D plots. This may
 * well constitute a yet unknown bug... */
TBOOLEAN clip_points = FALSE;

/* set samples */
int samples_1 = SAMPLES;
int samples_2 = SAMPLES;

/* set angles */
double ang2rad = 1.0;		/* 1 or pi/180, tracking angles_format */

enum PLOT_STYLE data_style = POINTSTYLE;
enum PLOT_STYLE func_style = LINES;

TBOOLEAN parametric = FALSE;

#if 0 /* UNUSED */
TBOOLEAN suppressMove = FALSE;	/* to prevent moveto while drawing contours */
#endif

fill_style_type default_fillstyle = { FS_EMPTY, 100, 0, LT_UNDEFINED } ;

/* filledcurves style options */
filledcurves_opts filledcurves_opts_data = EMPTY_FILLEDCURVES_OPTS;
filledcurves_opts filledcurves_opts_func = EMPTY_FILLEDCURVES_OPTS;

#ifdef EAM_HISTOGRAMS
histogram_style histogram_opts = DEFAULT_HISTOGRAM_STYLE;
#endif

/*****************************************************************/
/* Routines that deal with global objects defined in this module */
/*****************************************************************/

/* Clipping to the bounding box: */

/* Test a single point to be within the xleft,xright,ybot,ytop bbox.
 * Sets the returned integers 4 l.s.b. as follows:
 * bit 0 if to the left of xleft.
 * bit 1 if to the right of xright.
 * bit 2 if above of ytop.
 * bit 3 if below of ybot.
 * 0 is returned if inside.
 */
int
clip_point(unsigned int x, unsigned int y)
{
    int ret_val = 0;

    if ((int)x < xleft)
	ret_val |= 0x01;
    if ((int)x > xright)
	ret_val |= 0x02;
    if ((int)y < ybot)
	ret_val |= 0x04;
    if ((int)y > ytop)
	ret_val |= 0x08;

    return ret_val;
}

/* Clip the given line to drawing coords defined as xleft,xright,ybot,ytop.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 */
/* FIXME HBB 20000522: The parameter type has to become unsigned int */
void
draw_clip_line(int x1, int y1, int x2, int y2)
{
    struct termentry *t = term;

#if defined(ATARI) || defined(MTOS)
    /* HBB 20000522: why would this test be particular to ATARIs? And
     * what was the bug this is supposed to fix? */
    if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0)
	return;			/* temp bug fix */
#endif

    /* HBB 20000522: I've made this routine use the clippling in
     * clip_line(), in a movement to reduce code duplication. There
     * was one very small difference between these two routines. See
     * clip_line() for a comment about it, at the relevant place. */
    if (!clip_line(&x1, &y1, &x2, &y2))
	/* clip_line() returns zero --> segment completely outside
	 * bounding box */
	return;

    /* HBB 20000617: there was a design error, here. By nature, this
     * is a 2D routine. It should never have to check for hidden3d
     * related variables, or call the hidden3d routine
     * draw_line_hidden, like this. I've thrown this out. */

    /* FIXME HBB 20000522: I strongly doubt this can work as
     * advertised.  It's supposed to allow for continuous contour
     * curves, but as soon as the contour curve moves outside the
     * boundary, you get overpainting instead, as the contour curve
     * walks along the border. Or worse artefacts.  */
#if 0 /* UNUSED */
    if (!suppressMove)
#endif
	(*t->move) (x1, y1);
    (*t->vector) (x2, y2);
}



/* And text clipping routine. */
void
clip_put_text(unsigned int x, unsigned y, char *str)
{
    struct termentry *t = term;

    if (clip_point(x, y))
	return;

    (*t->put_text) (x, y, str);
}


/* Clip the given line to drawing coords defined as xleft,xright,ybot,ytop.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 */
/* FIXME HBB 20000521: the parameters of this routine should become
 * unsigned ints. */
int
clip_line(int *x1, int *y1, int *x2, int *y2)
{
    int x, y, dx, dy, x_intr[4], y_intr[4], count, pos1, pos2;
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
     * when the line passes directly through at least one corner). In
     * this case it is sufficient to take any 2 intersections (e.g. the
     * first two found).
     */
    count = 0;
    dx = *x2 - *x1;
    dy = *y2 - *y1;
    /* Find intersections with the x parallel bbox lines: */
    if (dy != 0) {
	x = (ybot - *y2) * dx / dy + *x2;	/* Test for ybot boundary. */
	if (x >= xleft && x <= xright) {
	    x_intr[count] = x;
	    y_intr[count++] = ybot;
	}
	x = (ytop - *y2) * dx / dy + *x2;	/* Test for ytop boundary. */
	if (x >= xleft && x <= xright) {
	    x_intr[count] = x;
	    y_intr[count++] = ytop;
	}
    }
    /* Find intersections with the y parallel bbox lines: */
    if (dx != 0) {
	y = (xleft - *x2) * dy / dx + *y2;	/* Test for xleft boundary. */
	if (y >= ybot && y <= ytop) {
	    x_intr[count] = xleft;
	    y_intr[count++] = y;
	}
	y = (xright - *x2) * dy / dx + *y2;	/* Test for xright boundary. */
	if (y >= ybot && y <= ytop) {
	    x_intr[count] = xright;
	    y_intr[count++] = y;
	}
    }
    if (count < 2)
	return 0;

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
	*x1 = x_intr[0];
	*y1 = y_intr[0];
	*x2 = x_intr[1];
	*y2 = y_intr[1];
    } else if (pos1) {		/* Only x1/y1 was out - update only it */
	/* This is about the only real difference between this and
	 * draw_clip_line(): it compares for '>0', here */
	if (dx * (*x2 - x_intr[0]) + dy * (*y2 - y_intr[0]) >= 0) {
	    *x1 = x_intr[0];
	    *y1 = y_intr[0];
	} else {
	    *x1 = x_intr[1];
	    *y1 = y_intr[1];
	}
    } else {			/* Only x2/y2 was out - update only it */
	/* Same difference here, again */
	if (dx * (x_intr[0] - *x1) + dy * (y_intr[0] - *y1) >= 0) {
	    *x2 = x_intr[0];
	    *y2 = y_intr[0];
	} else {
	    *x2 = x_intr[1];
	    *y2 = y_intr[1];
	}
    }

    if (*x1 < x_min || *x1 > x_max || *x2 < x_min || *x2 > x_max || *y1 < y_min || *y1 > y_max || *y2 < y_min || *y2 > y_max)
	return 0;

    return 1;
}


/* Two routines to emulate move/vector sequence using line drawing routine. */
static unsigned int move_pos_x, move_pos_y;

void
clip_move(unsigned int x, unsigned int y)
{
    move_pos_x = x;
    move_pos_y = y;
}

void
clip_vector(unsigned int x, unsigned int y)
{
    draw_clip_line(move_pos_x, move_pos_y, x, y);
    move_pos_x = x;
    move_pos_y = y;
}

/* Common routines for setting text or line color from t_colorspec */

void
apply_pm3dcolor(struct t_colorspec *tc, const struct termentry *t)
{

    /* Replace colorspec with that of the requested line style */
    struct lp_style_type style;
    if (tc->type == TC_LINESTYLE) {
	lp_use_properties(&style, tc->lt, 0);
	(*t->linetype)(style.l_type);
	tc = &style.pm3d_color;
    }

    if (tc->type == TC_DEFAULT) {
	(*t->linetype)(LT_BLACK);
	return;
    }
    if (tc->type == TC_LT) {
	if (t->set_color)
	    t->set_color(tc);
	else
	    (*t->linetype)(tc->lt);
	return;
    }
    if (tc->type == TC_RGB && t->set_color) {
	t->set_color(tc);
	return;
    }
    if (!is_plot_with_palette() || !t->set_color) {
	(*t->linetype)(LT_BLACK);
	return;
    }
    switch (tc->type) {
	case TC_Z:    set_color(cb2gray(z2cb(tc->value))); break;
	case TC_CB:   set_color(cb2gray(tc->value));       break;
	case TC_FRAC: set_color(sm_palette.positive == SMPAL_POSITIVE ?
				tc->value : 1-tc->value);
		      break;
    }
    if (tc->type == TC_LT) {
	(*t->linetype)(tc->lt);
	return;
    }
}

void
reset_textcolor(const struct t_colorspec *tc, const struct termentry *t)
{
    if (tc->type != TC_DEFAULT)
	(*t->linetype)(LT_BLACK);
}


void
default_arrow_style(struct arrow_style_type *arrow)
{
    struct lp_style_type tmp_lp_style = DEFAULT_LP_STYLE_TYPE;
    arrow->layer = 0;
    arrow->lp_properties = tmp_lp_style;
    arrow->head = 1;
    arrow->head_length = 0.0;
    arrow->head_lengthunit = first_axes;
    arrow->head_angle = 15.0;
    arrow->head_backangle = 90.0;
    arrow->head_filled = 0;
}

#ifdef EAM_DATASTRINGS
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
#endif

void
get_offsets(
    struct text_label *this_label,
    struct termentry *t,
    int *htic, int *vtic)
{
    if (this_label->lp_properties.pointflag) {
	*htic = (pointsize * t->h_tic * 0.5);
	*vtic = (pointsize * t->v_tic * 0.5);
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
write_label(unsigned int x, unsigned int y, struct text_label *this_label)
{
	int htic, vtic;
	int justify = JUST_TOP;	/* This was the 2D default; 3D had CENTRE */

	apply_pm3dcolor(&(this_label->textcolor),term);
	ignore_enhanced(this_label->noenhanced);

	get_offsets(this_label, term, &htic, &vtic);
	if (this_label->rotate && (*term->text_angle) (this_label->rotate)) {
	    write_multiline(x + htic, y + vtic, this_label->text,
			    this_label->pos, justify, this_label->rotate,
			    this_label->font);
	    (*term->text_angle) (0);
	} else {
	    write_multiline(x + htic, y + vtic, this_label->text,
			    this_label->pos, justify, 0, this_label->font);
	}
	if (this_label->lp_properties.pointflag) {
	    term_apply_lp_properties(&this_label->lp_properties);
	    (*term->point) (x, y, this_label->lp_properties.p_type);
	    /* the default label color is that of border */
	    term_apply_lp_properties(&border_lp);
	}

	ignore_enhanced(FALSE);
}
