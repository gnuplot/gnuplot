/* GNUPLOT - graph3d.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *
 * This software is provided "as is" without express or implied warranty.
 *
 *
 * AUTHORS
 *
 *   Original Software:
 *       Gershon Elber and many others.
 *
 * Send your comments or suggestions to 
 *  pixar!info-gnuplot@sun.com.
 * This is a mailing list; to join it send a note to 
 *  pixar!info-gnuplot-request@sun.com.  
 * Send bug reports to
 *  pixar!bug-gnuplot@sun.com.
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "plot.h"
#include "setshow.h"

extern char *strcpy(),*strncpy(),*strcat(),*ctime(),*tdate;
#ifdef AMIGA_AC_5
extern time_t dated;
#else
#ifdef VMS
extern time_t dated,time();
#else
extern long dated,time();
#endif
#endif

static plot3d_impulses();
static plot3d_lines();
static plot3d_points();
static plot3d_dots();
static cntr3d_impulses();
static cntr3d_lines();
static cntr3d_points();
static cntr3d_dots();
static update_extrema_pts();
static draw_parametric_grid();
static draw_non_param_grid();
static draw_bottom_grid();
static draw_3dxtics();
static draw_3dytics();
static draw_3dztics();
static draw_series_3dxtics();
static draw_series_3dytics();
static draw_series_3dztics();
static draw_set_3dxtics();
static draw_set_3dytics();
static draw_set_3dztics();
static xtick();
static ytick();
static ztick();

#ifndef max		/* Lattice C has max() in math.h, but shouldn't! */
#define max(a,b) ((a > b) ? a : b)
#endif

#ifndef min
#define min(a,b) ((a < b) ? a : b)
#endif

#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

#define apx_eq(x,y) (fabs(x-y) < 0.001)
#define abs(x) ((x) >= 0 ? (x) : -(x))
#define sqr(x) ((x) * (x))

/* Define the boundary of the plot
 * These are computed at each call to do_plot, and are constant over
 * the period of one do_plot. They actually only change when the term
 * type changes and when the 'set size' factors change. 
 */
static int xleft, xright, ybot, ytop, xmiddle, ymiddle, xscaler, yscaler;

/* Boundary and scale factors, in user coordinates */
/* x_min3d, x_max3d, y_min3d, y_max3d, z_min3d, z_max3d are local to this
 * file and are not the same as variables of the same names in other files
 */
static double x_min3d, x_max3d, y_min3d, y_max3d, z_min3d, z_max3d;
static double xscale3d, yscale3d, zscale3d;
static double real_z_min3d, real_z_max3d;
static double min_sy_ox,min_sy_oy; /* obj. coords. for xy tics placement. */
static double min_sx_ox,min_sx_oy; /* obj. coords. for z tics placement. */

typedef double transform_matrix[4][4];
static transform_matrix trans_mat;

/* (DFK) Watch for cancellation error near zero on axes labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
#define NearlyEqual(x,y,tic) (fabs((x)-(y)) < ((tic) * SIGNIF))

/* And the functions to map from user to terminal coordinates */
#define map_x(x) (int)(x+0.5) /* maps floating point x to screen */ 
#define map_y(y) (int)(y+0.5)	/* same for y */

/* And the functions to map from user 3D space into normalized -1..1 */
#define map_x3d(x) ((x-x_min3d)*xscale3d-1.0)
#define map_y3d(y) ((y-y_min3d)*yscale3d-1.0)
#define map_z3d(z) ((z-z_min3d)*zscale3d-1.0)

static mat_unit(mat)
transform_matrix mat;
{
    int i, j;

    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
	if (i == j)
	    mat[i][j] = 1.0;
	else
	    mat[i][j] = 0.0;
}

static mat_trans(tx, ty, tz, mat)
double tx, ty, tz;
transform_matrix mat;
{
     mat_unit(mat);                                 /* Make it unit matrix. */
     mat[3][0] = tx;
     mat[3][1] = ty;
     mat[3][2] = tz;
}

static mat_scale(sx, sy, sz, mat)
double sx, sy, sz;
transform_matrix mat;
{
     mat_unit(mat);                                 /* Make it unit matrix. */
     mat[0][0] = sx;
     mat[1][1] = sy;
     mat[2][2] = sz;
}

static mat_rot_x(teta, mat)
double teta;
transform_matrix mat;
{
    double cos_teta, sin_teta;

    teta *= Pi / 180.0;
    cos_teta = cos(teta);
    sin_teta = sin(teta);

    mat_unit(mat);                                  /* Make it unit matrix. */
    mat[1][1] = cos_teta;
    mat[1][2] = -sin_teta;
    mat[2][1] = sin_teta;
    mat[2][2] = cos_teta;
}

static mat_rot_y(teta, mat)
double teta;
transform_matrix mat;
{
    double cos_teta, sin_teta;

    teta *= Pi / 180.0;
    cos_teta = cos(teta);
    sin_teta = sin(teta);

    mat_unit(mat);                                  /* Make it unit matrix. */
    mat[0][0] = cos_teta;
    mat[0][2] = -sin_teta;
    mat[2][0] = sin_teta;
    mat[2][2] = cos_teta;
}

static mat_rot_z(teta, mat)
double teta;
transform_matrix mat;
{
    double cos_teta, sin_teta;

    teta *= Pi / 180.0;
    cos_teta = cos(teta);
    sin_teta = sin(teta);

    mat_unit(mat);                                  /* Make it unit matrix. */
    mat[0][0] = cos_teta;
    mat[0][1] = -sin_teta;
    mat[1][0] = sin_teta;
    mat[1][1] = cos_teta;
}

/* Multiply two transform_matrix. Result can be one of two operands. */
void mat_mult(mat_res, mat1, mat2)
transform_matrix mat_res, mat1, mat2;
{
    int i, j, k;
    transform_matrix mat_res_temp;

    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) {
        mat_res_temp[i][j] = 0;
        for (k = 0; k < 4; k++) mat_res_temp[i][j] += mat1[i][k] * mat2[k][j];
    }
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
	mat_res[i][j] = mat_res_temp[i][j];
}

/* And the functions to map from user 3D space to terminal coordinates */
static int map3d_xy(x, y, z, xt, yt)
double x, y, z;
int *xt, *yt;
{
    int i,j;
    double v[4], res[4],		     /* Homogeneous coords. vectors. */
	w = trans_mat[3][3];

    v[0] = map_x3d(x); /* Normalize object space to -1..1 */
    v[1] = map_y3d(y);
    v[2] = map_z3d(z);
    v[3] = 1.0;

    for (i = 0; i < 2; i++) {	             /* Dont use the third axes (z). */
        res[i] = trans_mat[3][i];     /* Initiate it with the weight factor. */
        for (j = 0; j < 3; j++) res[i] += v[j] * trans_mat[j][i];
    }

    for (i = 0; i < 3; i++) w += v[i] * trans_mat[i][3];
    if (w == 0) w = 1e-5;

    *xt = ((int) (res[0] * xscaler / w)) + xmiddle;
    *yt = ((int) (res[1] * yscaler / w)) + ymiddle;
}

/* Test a single point to be within the xleft,xright,ybot,ytop bbox.
 * Sets the returned integers 4 l.s.b. as follows:
 * bit 0 if to the left of xleft.
 * bit 1 if to the right of xright.
 * bit 2 if above of ytop.
 * bit 3 if below of ybot.
 * 0 is returned if inside.
 */
static int clip_point(x, y)
int x, y;
{
    int ret_val = 0;

    if (x < xleft) ret_val |= 0x01;
    if (x > xright) ret_val |= 0x02;
    if (y < ybot) ret_val |= 0x04;
    if (y > ytop) ret_val |= 0x08;

    return ret_val;
}

/* Clip the given line to drawing coords defined as xleft,xright,ybot,ytop.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 */
static void draw_clip_line(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    int x, y, dx, dy, x_intr[2], y_intr[2], count, pos1, pos2;
    register struct termentry *t = &term_tbl[term];

    pos1 = clip_point(x1, y1);
    pos2 = clip_point(x2, y2);
    if (pos1 || pos2) {
	if (pos1 & pos2) return;		  /* segment is totally out. */

	/* Here part of the segment MAY be inside. test the intersection
	 * of this segment with the 4 boundaries for hopefully 2 intersections
	 * in. If non found segment is totaly out.
	 */
	count = 0;
	dx = x2 - x1;
	dy = y2 - y1;

	/* Find intersections with the x parallel bbox lines: */
	if (dy != 0) {
	    x = (ybot - y2) * dx / dy + x2;        /* Test for ybot boundary. */
	    if (x >= xleft && x <= xright) {
		x_intr[count] = x;
		y_intr[count++] = ybot;
	    }
	    x = (ytop - y2) * dx / dy + x2;        /* Test for ytop boundary. */
	    if (x >= xleft && x <= xright) {
		x_intr[count] = x;
		y_intr[count++] = ytop;
	    }
	}

	/* Find intersections with the y parallel bbox lines: */
	if (dx != 0) {
	    y = (xleft - x2) * dy / dx + y2;      /* Test for xleft boundary. */
	    if (y >= ybot && y <= ytop) {
		x_intr[count] = xleft;
		y_intr[count++] = y;
	    }
	    y = (xright - x2) * dy / dx + y2;    /* Test for xright boundary. */
	    if (y >= ybot && y <= ytop) {
		x_intr[count] = xright;
		y_intr[count++] = y;
	    }
	}

	if (count == 2) {
	    int x_max, x_min, y_max, y_min;

	    x_min = min(x1, x2);
	    x_max = max(x1, x2);
	    y_min = min(y1, y2);
	    y_max = max(y1, y2);

	    if (pos1 && pos2) {		       /* Both were out - update both */
		x1 = x_intr[0];
		y1 = y_intr[0];
		x2 = x_intr[1];
		y2 = y_intr[1];
	    }
	    else if (pos1) {	       /* Only x1/y1 was out - update only it */
		if (dx * (x2 - x_intr[0]) + dy * (y2 - y_intr[0]) > 0) {
		    x1 = x_intr[0];
		    y1 = y_intr[0];
		}
		else {
		    x1 = x_intr[1];
		    y1 = y_intr[1];
		}
	    }
	    else {	       	       /* Only x2/y2 was out - update only it */
		if (dx * (x_intr[0] - x1) + dy * (y_intr[0] - x1) > 0) {
		    x2 = x_intr[0];
		    y2 = y_intr[0];
		}
		else {
		    x2 = x_intr[1];
		    y2 = y_intr[1];
		}
	    }

	    if (x1 < x_min || x1 > x_max ||
		x2 < x_min || x2 > x_max ||
		y1 < y_min || y1 > y_max ||
		y2 < y_min || y2 > y_max) return;
	}
	else
	    return;
    }

    (*t->move)(x1,y1);
    (*t->vector)(x2,y2);
}

/* Two routine to emulate move/vector sequence using line drawing routine. */
static int move_pos_x, move_pos_y;

static void clip_move(x,y)
int x,y;
{
    move_pos_x = x;
    move_pos_y = y;
}

static void clip_vector(x,y)
int x,y;
{
    draw_clip_line(move_pos_x,move_pos_y, x, y);
    move_pos_x = x;
    move_pos_y = y;
}

/* And text clipping routine. */
static void clip_put_text(x, y, str)
int x,y;
char *str;
{
    register struct termentry *t = &term_tbl[term];

    if (clip_point(x, y)) return;

    (*t->put_text)(x,y,str);
}

/* (DFK) For some reason, the Sun386i compiler screws up with the CheckLog 
 * macro, so I write it as a function on that machine.
 */
#ifndef sun386
/* (DFK) Use 10^x if logscale is in effect, else x */
#define CheckLog(log, x) ((log) ? pow(10., (x)) : (x))
#else
static double
CheckLog(log, x)
     BOOLEAN log;
     double x;
{
  if (log)
    return(pow(10., x));
  else
    return(x);
}
#endif /* sun386 */

static double
LogScale(coord, islog, what, axis)
	double coord;			/* the value */
	BOOLEAN islog;			/* is this axis in logscale? */
	char *what;			/* what is the coord for? */
	char *axis;			/* which axis is this for ("x" or "y")? */
{
    if (islog) {
	   if (coord <= 0.0) {
		  char errbuf[100];		/* place to write error message */
		(void) sprintf(errbuf,"%s has %s coord of %g; must be above 0 for log scale!",
				what, axis, coord);
		  (*term_tbl[term].text)();
		  (void) fflush(outfile);
		  int_error(errbuf, NO_CARET);
	   } else
		return(log10(coord));
    }
    return(coord);
}

/* borders of plotting area */
/* computed once on every call to do_plot */
static boundary3d(scaling)
	BOOLEAN scaling;		/* TRUE if terminal is doing the scaling */
{
    register struct termentry *t = &term_tbl[term];
    xleft = (t->h_char)*2 + (t->h_tic);
    xright = (scaling ? 1 : xsize) * (t->xmax) - (t->h_char)*2 - (t->h_tic);
    ybot = (t->v_char)*5/2 + 1;
    ytop = (scaling ? 1 : ysize) * (t->ymax) - (t->v_char)*5/2 - 1;
    xmiddle = (xright + xleft) / 2;
    ymiddle = (ytop + ybot) / 2;
    xscaler = (xright - xleft) / 2;
    yscaler = (ytop - ybot) / 2;
}

static double dbl_raise(x,y)
double x;
int y;
{
register int i;
double val;

	val = 1.0;
	for (i=0; i < abs(y); i++)
		val *= x;
	if (y < 0 ) return (1.0/val);
	return(val);
}


static double make_3dtics(tmin,tmax,axis,logscale)
double tmin,tmax;
int axis;
BOOLEAN logscale;
{
int len,x1,y1,x2,y2;
register double xr,xnorm,tics,tic,l10;

	xr = fabs(tmin-tmax);

	/* Compute length of axis in screen space coords. */
	switch (axis) {
		case 'x':
			map3d_xy(tmin,0.0,0.0,&x1,&y1);
			map3d_xy(tmax,0.0,0.0,&x2,&y2);
			break;
		case 'y':
			map3d_xy(0.0,tmin,0.0,&x1,&y1);
			map3d_xy(0.0,tmax,0.0,&x2,&y2);
			break;
		case 'z':
			map3d_xy(0.0,0.0,tmin,&x1,&y1);
			map3d_xy(0.0,0.0,tmax,&x2,&y2);
			break;
	}

	if (((long) (x1-x2))*(x1-x2) + ((long) (y1-y2))*(y1-y2) <
	    sqr(3L * term_tbl[term].h_char))
		return -1.0;                  			/* No tics! */

	l10 = log10(xr);
	if (logscale) {
		tic = dbl_raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
		if (tic < 1.0)
			tic = 1.0;
	} else {
		xnorm = pow(10.0,l10-(double)((l10 >= 0.0 ) ? (int)l10 : ((int)l10-1)));
		if (xnorm <= 5)
			tics = 0.5;
		else tics = 1.0;
		tic = tics * dbl_raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
	}
	return(tic);
}

do_3dplot(plots, pcount, min_x, max_x, min_y, max_y, min_z, max_z)
struct surface_points *plots;
int pcount;			/* count of plots in linked list */
double min_x, max_x;
double min_y, max_y;
double min_z, max_z;
{
register struct termentry *t = &term_tbl[term];
register int surface, xaxis_y, yaxis_x;
register struct surface_points *this_plot;
int xl, yl;
			/* only a Pyramid would have this many registers! */
double xtemp, ytemp, ztemp, temp;
struct text_label *this_label;
struct arrow_def *this_arrow;
BOOLEAN scaling;
transform_matrix mat;

/* Initiate transformation matrix using the global view variables. */
    mat_rot_z(surface_rot_z, trans_mat);
    mat_rot_x(surface_rot_x, mat);
    mat_mult(trans_mat, trans_mat, mat);
    mat_scale(surface_scale / 2.0, surface_scale / 2.0, surface_scale / 2.0, mat);
    mat_mult(trans_mat, trans_mat, mat);

/* modify min_z/max_z so it will zscale properly. */
    ztemp = (max_z - min_z) / (2.0 * surface_zscale);
    temp = (max_z + min_z) / 2.0;
    min_z = temp - ztemp;
    max_z = temp + ztemp;

/* store these in variables global to this file */
/* otherwise, we have to pass them around a lot */
    x_min3d = min_x;
    x_max3d = max_x;
    y_min3d = min_y;
    y_max3d = max_y;
    z_min3d = min_z;
    z_max3d = max_z;

    if (polar)
	int_error("Can not splot in polar coordinate system.", NO_CARET);

    if (z_min3d == VERYLARGE || z_max3d == -VERYLARGE ||
	x_min3d == VERYLARGE || x_max3d == -VERYLARGE ||
	y_min3d == VERYLARGE || y_max3d == -VERYLARGE)
        int_error("all points undefined!", NO_CARET);

    /* If we are to draw the bottom grid make sure zmin is updated properly. */
    if (xtics || ytics || grid)
	z_min3d -= (max_z - min_z) * ticslevel;

/*  This used be x_max3d == x_min3d, but that caused an infinite loop once. */
    if (fabs(x_max3d - x_min3d) < zero)
	int_error("x_min3d should not equal x_max3d!",NO_CARET);
    if (fabs(y_max3d - y_min3d) < zero)
	int_error("y_min3d should not equal y_max3d!",NO_CARET);
    if (fabs(z_max3d - z_min3d) < zero)
	int_error("z_min3d should not equal z_max3d!",NO_CARET);

/* INITIALIZE TERMINAL */
    if (!term_init) {
	(*t->init)();
	term_init = TRUE;
    }
    screen_ok = FALSE;
    scaling = (*t->scale)(xsize, ysize);
    (*t->graphics)();

    /* now compute boundary for plot (xleft, xright, ytop, ybot) */
    boundary3d(scaling);

/* SCALE FACTORS */
	zscale3d = 2.0/(z_max3d - z_min3d);
	yscale3d = 2.0/(y_max3d - y_min3d);
	xscale3d = 2.0/(x_max3d - x_min3d);

	(*t->linetype)(-2); /* border linetype */
/* PLACE TITLE */
	if (*title != 0) {
		int x, y;

		x = title_xoffset * t->h_char;
		y = title_yoffset * t->v_char;

		if ((*t->justify_text)(CENTRE)) 
			(*t->put_text)(x+(xleft+xright)/2, 
				       y+ytop+(t->v_char), title);
		else
			(*t->put_text)(x+(xleft+xright)/2 - strlen(title)*(t->h_char)/2,
				       y+ytop+(t->v_char), title);
	}

/* PLACE TIMEDATE */
	if (timedate) {
		int x, y;

		x = time_xoffset * t->h_char;
		y = time_yoffset * t->v_char;
		dated = time( (long *) 0);
		tdate = ctime( &dated);
		tdate[24]='\0';
		if ((*t->text_angle)(1)) {
			if ((*t->justify_text)(CENTRE)) {
				(*t->put_text)(x+(t->v_char),
						 y+ybot+4*(t->v_char), tdate);
			}
			else {
				(*t->put_text)(x+(t->v_char),
						 y+ybot+4*(t->v_char)-(t->h_char)*strlen(ylabel)/2, 
						 tdate);
			}
		}
		else {
			(void)(*t->justify_text)(LEFT);
			(*t->put_text)(x,
						 y+ybot-3*(t->v_char), tdate);
		}
		(void)(*t->text_angle)(0);
	}

/* PLACE LABELS */
    for (this_label = first_label; this_label!=NULL;
			this_label=this_label->next ) {
	    int x,y;

	    xtemp = LogScale(this_label->x, log_x, "label", "x");
	    ytemp = LogScale(this_label->y, log_y, "label", "y");
	    ztemp = LogScale(this_label->z, log_z, "label", "z");
    	    map3d_xy(xtemp,ytemp,ztemp, &x, &y);

		if ((*t->justify_text)(this_label->pos)) {
			(*t->put_text)(x,y,this_label->text);
		}
		else {
			switch(this_label->pos) {
				case  LEFT:
					(*t->put_text)(x,y,this_label->text);
					break;
				case CENTRE:
					(*t->put_text)(x -
						(t->h_char)*strlen(this_label->text)/2,
						y, this_label->text);
					break;
				case RIGHT:
					(*t->put_text)(x -
						(t->h_char)*strlen(this_label->text),
						y, this_label->text);
					break;
			}
		 }
	 }

/* PLACE ARROWS */
    (*t->linetype)(0);	/* arrow line type */
    for (this_arrow = first_arrow; this_arrow!=NULL;
	    this_arrow = this_arrow->next ) {
	int sx,sy,ex,ey;

	xtemp = LogScale(this_arrow->sx, log_x, "arrow", "x");
	ytemp = LogScale(this_arrow->sy, log_y, "arrow", "y");
	ztemp = LogScale(this_arrow->sz, log_y, "arrow", "z");
	map3d_xy(xtemp,ytemp,ztemp, &sx, &sy);

	xtemp = LogScale(this_arrow->ex, log_x, "arrow", "x");
	ytemp = LogScale(this_arrow->ey, log_y, "arrow", "y");
	ztemp = LogScale(this_arrow->ez, log_y, "arrow", "z");
	map3d_xy(xtemp,ytemp,ztemp, &ex, &ey);

	(*t->arrow)(sx, sy, ex, ey, this_arrow->head);
    }


/* DRAW SURFACES AND CONTOURS */
	real_z_min3d = min_z;
	real_z_max3d = max_z;
	if (key == -1) {
	    xl = xright  - (t->h_tic) - (t->h_char)*5;
	    yl = ytop - (t->v_tic) - (t->v_char);
	}
	if (key == 1) {
	    xtemp = LogScale(key_x, log_x, "key", "x");
	    ytemp = LogScale(key_y, log_y, "key", "y");
	    ztemp = LogScale(key_z, log_z, "key", "z");
	    map3d_xy(xtemp,ytemp,ztemp, &xl, &yl);
	}

	this_plot = plots;
	for (surface = 0;
	     surface < pcount;
	     this_plot = this_plot->next_sp, surface++) {
		(*t->linetype)(this_plot->line_type+1);

		if (draw_contour && this_plot->contours != NULL) {
			struct gnuplot_contours *cntrs = this_plot->contours;

			if (key != 0) {
				if ((*t->justify_text)(RIGHT)) {
					clip_put_text(xl,
						yl,this_plot->title);
				}
				else {
				    if (inrange(xl-(t->h_char)*strlen(this_plot->title), 
							 xleft, xright))
					 clip_put_text(xl-(t->h_char)*strlen(this_plot->title),
								 yl,this_plot->title);
				}
				switch(this_plot->plot_style) {
					case IMPULSES:
						clip_move(xl+(t->h_char),yl);
						clip_vector(xl+4*(t->h_char),yl);
						break;
					case LINES:
						clip_move(xl+(int)(t->h_char),yl);
						clip_vector(xl+(int)(4*(t->h_char)),yl);
						break;
					case ERRORBARS: /* ignored; treat like points */
					case POINTS:
						if (!clip_point(xl+2*(t->h_char),yl)) {
						     (*t->point)(xl+2*(t->h_char),yl,
								    this_plot->point_type);
						}
						break;
					case LINESPOINTS:
						clip_move(xl+(int)(t->h_char),yl);
						clip_vector(xl+(int)(4*(t->h_char)),yl);
						break;
					case DOTS:
						if (!clip_point(xl+2*(t->h_char),yl)) {
						     (*t->point)(xl+2*(t->h_char),yl, -1);
						}
						break;
				}
			}

			while (cntrs) {
				switch(this_plot->plot_style) {
					case IMPULSES:
			   			cntr3d_impulses(cntrs, this_plot);
						break;
					case LINES:
						cntr3d_lines(cntrs);
						break;
					case ERRORBARS: /* ignored; treat like points */
					case POINTS:
						cntr3d_points(cntrs, this_plot);
						break;
					case LINESPOINTS:
						cntr3d_lines(cntrs);
						cntr3d_points(cntrs, this_plot);
						break;
					case DOTS:
						cntr3d_dots(cntrs);
						break;
				}
				cntrs = cntrs->next;
			}
			if (key != 0) yl = yl - (t->v_char);
		}

		if ( surface == 0 )
			draw_bottom_grid(this_plot,real_z_min3d,real_z_max3d);

		if (!draw_surface) continue;
		(*t->linetype)(this_plot->line_type);

		if (key != 0) {
			if ((*t->justify_text)(RIGHT)) {
				clip_put_text(xl,
					yl,this_plot->title);
			}
			else {
			    if (inrange(xl-(t->h_char)*strlen(this_plot->title), 
						 xleft, xright))
				 clip_put_text(xl-(t->h_char)*strlen(this_plot->title),
							 yl,this_plot->title);
			}
		}
 
		switch(this_plot->plot_style) {
		    case IMPULSES: {
			   if (key != 0) {
				  clip_move(xl+(t->h_char),yl);
				  clip_vector(xl+4*(t->h_char),yl);
			   }
			   plot3d_impulses(this_plot);
			   break;
		    }
		    case LINES: {
			   if (key != 0) {
				  clip_move(xl+(int)(t->h_char),yl);
				  clip_vector(xl+(int)(4*(t->h_char)),yl);
			   }
			   plot3d_lines(this_plot);
			   break;
		    }
		    case ERRORBARS:	/* ignored; treat like points */
		    case POINTS: {
			   if (key != 0 && !clip_point(xl+2*(t->h_char),yl)) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot3d_points(this_plot);
			   break;
		    }
		    case LINESPOINTS: {
			   /* put lines */
			   if (key != 0) {
				  clip_move(xl+(t->h_char),yl);
				  clip_vector(xl+4*(t->h_char),yl);
			   }
			   plot3d_lines(this_plot);

			   /* put points */
			   if (key != 0 && !clip_point(xl+2*(t->h_char),yl)) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot3d_points(this_plot);
			   break;
		    }
		    case DOTS: {
			   if (key != 0 && !clip_point(xl+2*(t->h_char),yl)) {
				  (*t->point)(xl+2*(t->h_char),yl, -1);
			   }
			   plot3d_dots(this_plot);
			   break;
		    }
		}
		yl = yl - (t->v_char);
	}
	(*t->text)();
	(void) fflush(outfile);
}

/* plot3d_impulses:
 * Plot the surfaces in IMPULSES style
 */
static plot3d_impulses(plot)
	struct surface_points *plot;
{
    int i;				/* point index */
    int x,y,x0,y0;			/* point in terminal coordinates */
    struct termentry *t = &term_tbl[term];
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {
	    if (real_z_max3d<points[i].z)
		real_z_max3d=points[i].z;
	    if (real_z_min3d>points[i].z)
		real_z_min3d=points[i].z;

	    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);
	    map3d_xy(points[i].x, points[i].y, z_min3d, &x0, &y0);

	    clip_move(x0,y0);
	    clip_vector(x,y);
	}

	icrvs = icrvs->next;
    }
}

/* plot3d_lines:
 * Plot the surfaces in LINES style
 */
static plot3d_lines(plot)
	struct surface_points *plot;
{
    int i;
    int x,y;				/* point in terminal coordinates */
    struct termentry *t = &term_tbl[term];
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {
	    if (real_z_max3d<points[i].z)
		real_z_max3d=points[i].z;
	    if (real_z_min3d>points[i].z)
		real_z_min3d=points[i].z;

	    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

	    if (i > 0)
		clip_vector(x,y);
	    else
		clip_move(x,y);
	}

	icrvs = icrvs->next;
    }
}

/* plot3d_points:
 * Plot the surfaces in POINTS style
 */
static plot3d_points(plot)
	struct surface_points *plot;
{
    int i,x,y;
    struct termentry *t = &term_tbl[term];
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i++) {
	    if (real_z_max3d<points[i].z)
		real_z_max3d=points[i].z;
	    if (real_z_min3d>points[i].z)
		real_z_min3d=points[i].z;

	    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

	    if (!clip_point(x,y))
		(*t->point)(x,y, plot->point_type);
	}

	icrvs = icrvs->next;
    }
}

/* plot3d_dots:
 * Plot the surfaces in DOTS style
 */
static plot3d_dots(plot)
	struct surface_points *plot;
{
    int i,x,y;
    struct termentry *t = &term_tbl[term];
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate *points = icrvs->points;

    	for (i = 0; i < icrvs->p_count; i++) {
	    if (real_z_max3d<points[i].z)
		real_z_max3d=points[i].z;
	    if (real_z_min3d>points[i].z)
    		real_z_min3d=points[i].z;

    	    map3d_xy(points[i].x, points[i].y, points[i].z, &x, &y);

    	    if (!clip_point(x,y))
		(*t->point)(x,y, -1);
    	}

	icrvs = icrvs->next;
    }
}

/* cntr3d_impulses:
 * Plot a surface contour in IMPULSES style
 */
static cntr3d_impulses(cntr, plot)
	struct gnuplot_contours *cntr;
	struct surface_points *plot;
{
    int i,j,k;				/* point index */
    int x,y,x0,y0;			/* point in terminal coordinates */
    struct termentry *t = &term_tbl[term];

    if (draw_contour == CONTOUR_SRF || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
		     &x, &y);
	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, z_min3d,
		     &x0, &y0);

	    clip_move(x0,y0);
	    clip_vector(x,y);
	}
    }
    else
	cntr3d_points(cntr, plot);   /* Must be on base grid, so do points. */
}

/* cntr3d_lines:
 * Plot a surface contour in LINES style
 */
static cntr3d_lines(cntr)
	struct gnuplot_contours *cntr;
{
    int i,j,k;				/* point index */
    int x,y;				/* point in terminal coordinates */
    struct termentry *t = &term_tbl[term];

    if (draw_contour == CONTOUR_SRF || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
    		     &x, &y);

    	    if (i > 0)
    		clip_vector(x,y);
	    else
		clip_move(x,y);
    	}
    }

    if (draw_contour == CONTOUR_BASE || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, z_min3d,
    		     &x, &y);

    	    if (i > 0)
    		clip_vector(x,y);
	    else
		clip_move(x,y);
    	}
    }
}

/* cntr3d_points:
 * Plot a surface contour in POINTS style
 */
static cntr3d_points(cntr, plot)
	struct gnuplot_contours *cntr;
	struct surface_points *plot;
{
    int i,j,k;
    int x,y;
    struct termentry *t = &term_tbl[term];

    if (draw_contour == CONTOUR_SRF || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
    		     &x, &y);

	    if (!clip_point(x,y))
		(*t->point)(x,y, plot->point_type);
    	}
    }

    if (draw_contour == CONTOUR_BASE || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, z_min3d,
    		     &x, &y);

	    if (!clip_point(x,y))
		(*t->point)(x,y, plot->point_type);
    	}
    }
}

/* cntr3d_dots:
 * Plot a surface contour in DOTS style
 */
static cntr3d_dots(cntr)
	struct gnuplot_contours *cntr;
{
    int i,j,k;
    int x,y;
    struct termentry *t = &term_tbl[term];

    if (draw_contour == CONTOUR_SRF || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
    		     &x, &y);

	    if (!clip_point(x,y))
		(*t->point)(x,y, -1);
    	}
    }

    if (draw_contour == CONTOUR_BASE || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, z_min3d,
    		     &x, &y);

	    if (!clip_point(x,y))
		(*t->point)(x,y, -1);
    	}
    }
}

static update_extrema_pts(ix, iy, min_sx_x, min_sx_y, min_sy_x, min_sy_y,
			  x, y)
	int ix, iy, *min_sx_x, *min_sx_y, *min_sy_x, *min_sy_y;
	double x, y;
{

    if (*min_sx_x > ix + 2 ||         /* find (bottom) left corner of grid */
	(abs(*min_sx_x - ix) <= 2 && *min_sx_y > iy)) {
	*min_sx_x = ix;
	*min_sx_y = iy;
	min_sx_ox = x;
	min_sx_oy = y;
    }
    if (*min_sy_y > iy + 2 ||         /* find bottom (right) corner of grid */
	(abs(*min_sy_y - iy) <= 2 && *min_sy_x < ix)) {
	*min_sy_x = ix;
	*min_sy_y = iy;
	min_sy_ox = x;
	min_sy_oy = y;
    }
}

/* Draw the bottom grid for the parametric case. */
static draw_parametric_grid(plot)
	struct surface_points *plot;
{
    int i,ix,iy,			/* point in terminal coordinates */
	min_sx_x = 10000,min_sx_y = 10000,min_sy_x = 10000,min_sy_y = 10000,
        grid_iso = plot->plot_type == DATA3D && plot->has_grid_topology ?
					plot->num_iso_read : iso_samples;
    double x,y,dx,dy;
    struct termentry *t = &term_tbl[term];

    if (grid && plot->has_grid_topology) {
    	x = x_min3d;
    	y = y_min3d;

	dx = (x_max3d-x_min3d) / (grid_iso-1);
	dy = (y_max3d-y_min3d) / (grid_iso-1);

	for (i = 0; i < grid_iso; i++) {
	        if (i == 0 || i == grid_iso-1)
		    (*t->linetype)(-2);
		else
		    (*t->linetype)(-1);
		map3d_xy(x_min3d, y, z_min3d, &ix, &iy);
		clip_move(ix,iy);
		update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
				   &min_sy_x,&min_sy_y,x_min3d,y);

		map3d_xy(x_max3d, y, z_min3d, &ix, &iy);
		clip_vector(ix,iy);
		update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
				   &min_sy_x,&min_sy_y,x_max3d,y);

		y += dy;
	}

	for (i = 0; i < grid_iso; i++) {
	        if (i == 0 || i == grid_iso-1)
		    (*t->linetype)(-2);
		else
		    (*t->linetype)(-1);
		map3d_xy(x, y_min3d, z_min3d, &ix, &iy);
		clip_move(ix,iy);
		update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
				   &min_sy_x,&min_sy_y,x,y_min3d);

		map3d_xy(x, y_max3d, z_min3d, &ix, &iy);
		clip_vector(ix,iy);
		update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
				   &min_sy_x,&min_sy_y,x,y_max3d);

		x += dx;
	}
    }
    else {
	(*t->linetype)(-2);

	map3d_xy(x_min3d, y_min3d, z_min3d, &ix, &iy);
	clip_move(ix,iy);
	update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
			   &min_sy_x,&min_sy_y,x_min3d,y_min3d);

	map3d_xy(x_max3d, y_min3d, z_min3d, &ix, &iy);
	clip_vector(ix,iy);
	update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
			   &min_sy_x,&min_sy_y,x_max3d,y_min3d);

	map3d_xy(x_max3d, y_max3d, z_min3d, &ix, &iy);
	clip_vector(ix,iy);
	update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
			   &min_sy_x,&min_sy_y,x_max3d,y_max3d);

	map3d_xy(x_min3d, y_max3d, z_min3d, &ix, &iy);
	clip_vector(ix,iy);
	update_extrema_pts(ix,iy,&min_sx_x,&min_sx_y,
			   &min_sy_x,&min_sy_y,x_min3d,y_max3d);


	map3d_xy(x_min3d, y_min3d, z_min3d, &ix, &iy);
	clip_vector(ix,iy);
    }
}

/* Draw the bottom grid for non parametric case. */
static draw_non_param_grid(plot)
	struct surface_points *plot;
{
    int i,is_boundary=TRUE,crv_count=0,
	x,y,				/* point in terminal coordinates */
	min_sx_x = 10000,min_sx_y = 10000,min_sy_x = 10000,min_sy_y = 10000,
        grid_samples = plot->plot_type == DATA3D && plot->has_grid_topology ?
					plot->iso_crvs->p_count : samples,
        grid_iso = plot->plot_type == DATA3D && plot->has_grid_topology ?
					plot->num_iso_read : iso_samples;
    struct termentry *t = &term_tbl[term];
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate *points = icrvs->points;

	for (i = 0; i < icrvs->p_count; i += icrvs->p_count-1) {
	    map3d_xy(points[i].x, points[i].y, z_min3d, &x, &y);
	    if (is_boundary) {
		(*t->linetype)(-2);
	    }
	    else {
	        (*t->linetype)(-1);
	    }

	    if (i > 0) {
	    	clip_vector(x,y);
	    }
	    else {
	    	clip_move(x,y);
	    }

	    if (draw_surface &&
	        is_boundary &&
	    	(i == 0 || i == icrvs->p_count-1)) {
	    	int x1,y1;		    /* point in terminal coordinates */

		/* Draw a vertical line to surface corner from grid corner. */
	    	map3d_xy(points[i].x, points[i].y, points[i].z, &x1, &y1);
	    	clip_vector(x1,y1);
	    	clip_move(x,y);
		update_extrema_pts(x,y,&min_sx_x,&min_sx_y, &min_sy_x,&min_sy_y,
				   points[i].x,points[i].y);
	    }
	}

	if (grid) {
	    crv_count++;
	    icrvs = icrvs->next;
	    is_boundary = crv_count == grid_iso - 1 ||
			  crv_count == grid_iso ||
			  (icrvs && icrvs->next == NULL);
	}
	else {
	    switch (crv_count++) {
		case 0:
		    for (i = 0; i < grid_iso - 1; i++)
			icrvs = icrvs->next;
		    break;
		case 1:
		    icrvs = icrvs->next;
		    break;
		case 2:
		    while (icrvs->next)
			icrvs = icrvs->next;
		    break;
		case 3:
		    icrvs = NULL;
		    break;
	    }
    	}
    }
}

/* Draw the bottom grid that hold the tic marks for 3d surface. */
static draw_bottom_grid(plot, min_z, max_z)
	struct surface_points *plot;
	double min_z, max_z;
{
    int i,j,k;				/* point index */
    int x,y,min_x = 10000,min_y = 10000;/* point in terminal coordinates */
    double xtic,ytic,ztic;
    struct termentry *t = &term_tbl[term];

    xtic = make_3dtics(x_min3d,x_max3d,'x',log_x);
    ytic = make_3dtics(y_min3d,y_max3d,'y',log_y);
    ztic = make_3dtics(min_z,max_z,'z',log_z);

    if (draw_border)
	if (parametric || !plot->has_grid_topology)
	    draw_parametric_grid(plot);
	else
	    draw_non_param_grid(plot);

    (*t->linetype)(-2); /* border linetype */

/* label x axis tics */
    if (xtics && xtic > 0.0) {
    	switch (xticdef.type) {
    	    case TIC_COMPUTED:
 		if (x_min3d < x_max3d)
		    draw_3dxtics(xtic * floor(x_min3d/xtic),
    				 xtic,
    				 xtic * ceil(x_max3d/xtic),
    				 min_sy_oy);
    	    	else
		    draw_3dxtics(xtic * floor(x_max3d/xtic),
    				 xtic,
    				 xtic * ceil(x_min3d/xtic),
    				 min_sy_oy);
    		break;
	    case TIC_SERIES:
		draw_series_3dxtics(xticdef.def.series.start, 
				    xticdef.def.series.incr, 
				    xticdef.def.series.end,
				    min_sy_oy);
		break;
	    case TIC_USER:
		draw_set_3dxtics(xticdef.def.user,
				 min_sy_oy);
		break;
    	    default:
    		(*t->text)();
    		(void) fflush(outfile);
    		int_error("unknown tic type in xticdef in do_3dplot", NO_CARET);
    		break;		/* NOTREACHED */
    	}
    }
/* label y axis tics */
    if (ytics && ytic > 0.0) {
    	switch (yticdef.type) {
    	    case TIC_COMPUTED:
 		if (y_min3d < y_max3d)
		    draw_3dytics(ytic * floor(y_min3d/ytic),
    				 ytic,
    				 ytic * ceil(y_max3d/ytic),
    				 min_sy_ox);
    	    	else
		    draw_3dytics(ytic * floor(y_max3d/ytic),
    				 ytic,
    				 ytic * ceil(y_min3d/ytic),
    				 min_sy_ox);
    		break;
	    case TIC_SERIES:
		draw_series_3dytics(yticdef.def.series.start, 
				    yticdef.def.series.incr, 
				    yticdef.def.series.end,
				    min_sy_ox);
		break;
	    case TIC_USER:
		draw_set_3dytics(yticdef.def.user,
				 min_sy_ox);
		break;
    	    default:
    		(*t->text)();
    		(void) fflush(outfile);
    		int_error("unknown tic type in yticdef in do_3dplot", NO_CARET);
    		break;		/* NOTREACHED */
    	}
    }
/* label z axis tics */
    if (ztics && ztic > 0.0 && (draw_surface ||
				draw_contour == CONTOUR_SRF ||
				draw_contour == CONTOUR_BOTH)) {
    	switch (zticdef.type) {
    	    case TIC_COMPUTED:
 		if (min_z < max_z)
		    draw_3dztics(ztic * floor(min_z/ztic),
    				 ztic,
    				 ztic * ceil(max_z/ztic),
				 min_sx_ox,
    				 min_sx_oy,
    				 min_z,
				 max_z);
    	    	else
		    draw_3dztics(ztic * floor(max_z/ztic),
    				 ztic,
    				 ztic * ceil(min_z/ztic),
    				 min_sx_ox,
				 min_sx_oy,
    				 max_z,
				 min_z);
    		break;
	    case TIC_SERIES:
		draw_series_3dztics(zticdef.def.series.start, 
				    zticdef.def.series.incr, 
				    zticdef.def.series.end,
				    min_sx_ox,
				    min_sx_oy,
				    min_z,
				    max_z);

		break;
	    case TIC_USER:
		draw_set_3dztics(zticdef.def.user,
				 min_sx_ox,
    				 min_sx_oy,
    				 min_z,
				 max_z);
		break;
    	    default:
    		(*t->text)();
    		(void) fflush(outfile);
    		int_error("unknown tic type in zticdef in do_3dplot", NO_CARET);
    		break;		/* NOTREACHED */
    	}
    }

/* PLACE XLABEL - along the middle grid X axis */
    if (strlen(xlabel) > 0) {
	   int x1,y1;
	   double step = apx_eq( min_sy_oy, y_min3d ) ?	(y_max3d-y_min3d)/4
						      : (y_min3d-y_max3d)/4;
    	   map3d_xy((x_min3d+x_max3d)/2,min_sy_oy-step, z_min3d,&x1,&y1);
	   x1 += xlabel_xoffset * t->h_char;
	   y1 += xlabel_yoffset * t->v_char;
	   if ((*t->justify_text)(CENTRE))
		clip_put_text(x1,y1,xlabel);
	   else
		clip_put_text(x1 - strlen(xlabel)*(t->h_char)/2,y1,xlabel);
    }

/* PLACE YLABEL - along the middle grid Y axis */
    if (strlen(ylabel) > 0) {
	   int x1,y1;
	   double step = apx_eq( min_sy_ox, x_min3d ) ?	(x_max3d-x_min3d)/4
						      : (x_min3d-x_max3d)/4;
    	   map3d_xy(min_sy_ox-step,(y_min3d+y_max3d)/2,z_min3d,&x1,&y1);
	   x1 += ylabel_xoffset * t->h_char;
	   y1 += ylabel_yoffset * t->v_char;
	   if ((*t->justify_text)(CENTRE))
		clip_put_text(x1,y1,ylabel);
	   else
		clip_put_text(x1 - strlen(ylabel)*(t->h_char)/2,y1,ylabel);
    }

/* PLACE ZLABEL - along the middle grid Z axis */
    if (strlen(zlabel) > 0 &&
        (draw_surface ||
	 draw_contour == CONTOUR_SRF ||
	 draw_contour == CONTOUR_BOTH)) {
    	   map3d_xy(min_sx_ox,min_sx_oy,max_z + (max_z-min_z)/4, &x, &y);

	   x += zlabel_xoffset * t->h_char;
	   y += zlabel_yoffset * t->v_char;
	   if ((*t->justify_text)(CENTRE))
		clip_put_text(x,y,zlabel);
	   else
		clip_put_text(x - strlen(zlabel)*(t->h_char)/2,y,zlabel);
    }
}

/* DRAW_3DXTICS: draw a regular tic series, x axis */
static draw_3dxtics(start, incr, end, ypos)
	double start, incr, end, ypos; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace;
	int ltic;		/* for mini log tics */
	double lticplace;	/* for mini log tics */

	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if (ticplace < x_min3d || ticplace > x_max3d) continue;
		xtick(ticplace, xformat, incr, 1.0, ypos);
		if (log_x && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic <= 9; ltic++) {
				lticplace = ticplace+log10((double)ltic);
				xtick(lticplace, "\0", incr, 0.5, ypos);
			}
		}
	}
}

/* DRAW_3DYTICS: draw a regular tic series, y axis */
static draw_3dytics(start, incr, end, xpos)
	double start, incr, end, xpos; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace;
	int ltic;		/* for mini log tics */
	double lticplace;	/* for mini log tics */

	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if (ticplace < y_min3d || ticplace > y_max3d) continue;
		ytick(ticplace, yformat, incr, 1.0, xpos);
		if (log_y && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic <= 9; ltic++) {
				lticplace = ticplace+log10((double)ltic);
				ytick(lticplace, "\0", incr, 0.5, xpos);
			}
		}
	}
}

/* DRAW_3DZTICS: draw a regular tic series, z axis */
static draw_3dztics(start, incr, end, xpos, ypos, z_min, z_max)
	double start, incr, end, xpos, ypos, z_min, z_max;
		/* assume start < end, incr > 0 */
{
	int x, y;
	double ticplace;
	int ltic;		/* for mini log tics */
	double lticplace;	/* for mini log tics */
	register struct termentry *t = &term_tbl[term];

	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if (ticplace < z_min || ticplace > z_max) continue;

		ztick(ticplace, zformat, incr, 1.0, xpos, ypos);
		if (log_z && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic <= 9; ltic++) {
				lticplace = ticplace+log10((double)ltic);
				ztick(lticplace, "\0", incr, 0.5, xpos, ypos);
			}
		}
	}

	/* Make sure the vertical line is fully drawn. */
	(*t->linetype)(-2);	/* axis line type */

	map3d_xy(xpos, ypos, z_min3d, &x, &y);
	clip_move(x,y);
	map3d_xy(xpos, ypos, min(end,z_max)+(log_z ? incr : 0.0), &x, &y);
	clip_vector(x,y);

	(*t->linetype)(-1); /* border linetype */
}

/* DRAW_SERIES_3DXTICS: draw a user tic series, x axis */
static draw_series_3dxtics(start, incr, end, ypos)
		double start, incr, end, ypos; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace, place;
	double ticmin, ticmax;	/* for checking if tic is almost inrange */
	double spacing = log_x ? log10(incr) : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(log_x, x_min3d), CheckLog(log_x, x_max3d));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(log_x, x_min3d), CheckLog(log_x, x_max3d)));

	/* to allow for rounding errors */
	ticmin = min(x_min3d,x_max3d) - SIGNIF*incr;
	ticmax = max(x_min3d,x_max3d) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (log_x ? log10(ticplace) : ticplace);
	    if ( inrange(place,ticmin,ticmax) )
		 xtick(place, xformat, spacing, 1.0, ypos);
	}
}

/* DRAW_SERIES_3DYTICS: draw a user tic series, y axis */
static draw_series_3dytics(start, incr, end, xpos)
		double start, incr, end, xpos; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace, place;
	double ticmin, ticmax;	/* for checking if tic is almost inrange */
	double spacing = log_y ? log10(incr) : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(log_y, y_min3d), CheckLog(log_y, y_max3d));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(log_y, y_min3d), CheckLog(log_y, y_max3d)));

	/* to allow for rounding errors */
	ticmin = min(y_min3d,y_max3d) - SIGNIF*incr;
	ticmax = max(y_min3d,y_max3d) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (log_y ? log10(ticplace) : ticplace);
	    if ( inrange(place,ticmin,ticmax) )
		 ytick(place, xformat, spacing, 1.0, xpos);
	}
}

/* DRAW_SERIES_3DZTICS: draw a user tic series, z axis */
static draw_series_3dztics(start, incr, end, xpos, ypos, z_min, z_max)
		double start, incr, end; /* tic series definition */
		double xpos, ypos, z_min, z_max;
		/* assume start < end, incr > 0 */
{
	int x, y;
	double ticplace, place;
	double ticmin, ticmax;	/* for checking if tic is almost inrange */
	double spacing = log_x ? log10(incr) : incr;
	register struct termentry *t = &term_tbl[term];

	if (end == VERYLARGE)
		end = max(CheckLog(log_z, z_min), CheckLog(log_z, z_max));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(log_z, z_min), CheckLog(log_z, z_max)));

	/* to allow for rounding errors */
	ticmin = min(z_min,z_max) - SIGNIF*incr;
	ticmax = max(z_min,z_max) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (log_z ? log10(ticplace) : ticplace);
	    if ( inrange(place,ticmin,ticmax) )
		 ztick(place, zformat, spacing, 1.0, xpos, ypos);
	}

	/* Make sure the vertical line is fully drawn. */
	(*t->linetype)(-2);	/* axis line type */

	map3d_xy(xpos, ypos, z_min3d, &x, &y);
	clip_move(x,y);
	map3d_xy(xpos, ypos, min(end,z_max)+(log_z ? incr : 0.0), &x, &y);
	clip_vector(x,y);

	(*t->linetype)(-1); /* border linetype */
}

/* DRAW_SET_3DXTICS: draw a user tic set, x axis */
static draw_set_3dxtics(list, ypos)
	struct ticmark *list;	/* list of tic marks */
	double ypos;
{
    double ticplace;
    double incr = (x_max3d - x_min3d) / 10;
    /* global x_min3d, x_max3d, xscale, y_min3d, y_max3d, yscale */

    while (list != NULL) {
	   ticplace = (log_x ? log10(list->position) : list->position);
	   if ( inrange(ticplace, x_min3d, x_max3d) 		/* in range */
		  || NearlyEqual(ticplace, x_min3d, incr)	/* == x_min */
		  || NearlyEqual(ticplace, x_max3d, incr))	/* == x_max */
		xtick(ticplace, list->label, incr, 1.0, ypos);

	   list = list->next;
    }
}

/* DRAW_SET_3DYTICS: draw a user tic set, y axis */
static draw_set_3dytics(list, xpos)
	struct ticmark *list;	/* list of tic marks */
	double xpos;
{
    double ticplace;
    double incr = (y_max3d - y_min3d) / 10;
    /* global x_min3d, x_max3d, xscale, y_min3d, y_max3d, yscale */

    while (list != NULL) {
	   ticplace = (log_y ? log10(list->position) : list->position);
	   if ( inrange(ticplace, y_min3d, y_max3d) 		  /* in range */
		  || NearlyEqual(ticplace, y_min3d, incr)	/* == y_min3d */
		  || NearlyEqual(ticplace, y_max3d, incr))	/* == y_max3d */
		ytick(ticplace, list->label, incr, 1.0, xpos);

	   list = list->next;
    }
}

/* DRAW_SET_3DZTICS: draw a user tic set, z axis */
static draw_set_3dztics(list, xpos, ypos, z_min, z_max)
	struct ticmark *list;	/* list of tic marks */
	double xpos, ypos, z_min, z_max;
{
    int x, y;
    double ticplace;
    double incr = (z_max - z_min) / 10;
    register struct termentry *t = &term_tbl[term];

    while (list != NULL) {
	   ticplace = (log_z ? log10(list->position) : list->position);
	   if ( inrange(ticplace, z_min, z_max) 		/* in range */
		  || NearlyEqual(ticplace, z_min, incr)		/* == z_min */
		  || NearlyEqual(ticplace, z_max, incr))	/* == z_max */
		ztick(ticplace, list->label, incr, 1.0, xpos, ypos);

	   list = list->next;
    }

    /* Make sure the vertical line is fully drawn. */
    (*t->linetype)(-2);	/* axis line type */

    map3d_xy(xpos, ypos, z_min, &x, &y);
    clip_move(x,y);
    map3d_xy(xpos, ypos, z_max+(log_z ? incr : 0.0), &x, &y);
    clip_vector(x,y);

    (*t->linetype)(-1); /* border linetype */
}

/* draw and label a x-axis ticmark */
static xtick(place, text, spacing, ticscale, ypos)
        double place;                   /* where on axis to put it */
        char *text;                     /* optional text label */
        double spacing;         /* something to use with checkzero */
        double ticscale;         /* scale factor for tic mark (0..1] */
	double ypos;
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int x0,y0,x1,y1,x2,y2,x3,y3;
    int ticsize = (int)((t->h_tic) * ticscale);
    double v[2], len;

    place = CheckZero(place,spacing); /* to fix rounding error near zero */

    if (place > x_max3d || place < x_min3d) return;

    map3d_xy(place, ypos, z_min3d, &x0, &y0);
    /* need to figure out which is in. pick the middle point along the */
    /* axis as in.						       */
    map3d_xy(place, (y_max3d + y_min3d) / 2, z_min3d, &x1, &y1);

    /* compute a vector of length 1 into the grid: */
    v[0] = x1 - x0;
    v[1] = y1 - y0;
    len = sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] /= len;
    v[1] /= len;

    if (tic_in) {
	x1 = x0;
	y1 = y0;
	x2 = x1 + ((int) (v[0] * ticsize));
	y2 = y1 + ((int) (v[1] * ticsize));
    	x3 = x0 - ((int) (v[0] * ticsize * 3)); /* compute text position */
    	y3 = y0 - ((int) (v[1] * ticsize * 3));
    } else {
	x1 = x0;
	y1 = y0;
	x2 = x0 - ((int) (v[0] * ticsize));
	y2 = y0 - ((int) (v[1] * ticsize));
    	x3 = x0 - ((int) (v[0] * ticsize * 4)); /* compute text position */
    	y3 = y0 - ((int) (v[1] * ticsize * 4));
    }
    clip_move(x1,y1);
    clip_vector(x2,y2);

    /* label the ticmark */
    if (text == NULL)
	 text = xformat;

    (void) sprintf(ticlabel, text, CheckLog(log_x, place));
    if (apx_eq(v[0], 0.0)) {
    	if ((*t->justify_text)(CENTRE)) {
    	    clip_put_text(x3,y3,ticlabel);
    	} else {
    	    clip_put_text(x3-(t->h_char)*strlen(ticlabel)/2,y3,ticlabel);
    	}
    }
    else if (v[0] > 0) {
    	if ((*t->justify_text)(RIGHT)) {
    	    clip_put_text(x3,y3,ticlabel);
    	} else {
    	    clip_put_text(x3-(t->h_char)*strlen(ticlabel),y3,ticlabel);
    	}
    } else {
    	(*t->justify_text)(LEFT);
	clip_put_text(x3,y3,ticlabel);
    }
}

/* draw and label a y-axis ticmark */
static ytick(place, text, spacing, ticscale, xpos)
        double place;                   /* where on axis to put it */
        char *text;                     /* optional text label */
        double spacing;         /* something to use with checkzero */
        double ticscale;         /* scale factor for tic mark (0..1] */
	double xpos;
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int x0,y0,x1,y1,x2,y2,x3,y3;
    int ticsize = (int)((t->h_tic) * ticscale);
    double v[2], len;

    place = CheckZero(place,spacing); /* to fix rounding error near zero */

    if (place > y_max3d || place < y_min3d) return;

    map3d_xy(xpos, place, z_min3d, &x0, &y0);
    /* need to figure out which is in. pick the middle point along the */
    /* axis as in.						       */
    map3d_xy((x_max3d + x_min3d) / 2, place, z_min3d, &x1, &y1);

    /* compute a vector of length 1 into the grid: */
    v[0] = x1 - x0;
    v[1] = y1 - y0;
    len = sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] /= len;
    v[1] /= len;

    if (tic_in) {
	x1 = x0;
	y1 = y0;
	x2 = x1 + ((int) (v[0] * ticsize));
	y2 = y1 + ((int) (v[1] * ticsize));
    	x3 = x0 - ((int) (v[0] * ticsize * 3)); /* compute text position */
    	y3 = y0 - ((int) (v[1] * ticsize * 3));
    } else {
	x1 = x0;
	y1 = y0;
	x2 = x0 - ((int) (v[0] * ticsize));
	y2 = y0 - ((int) (v[1] * ticsize));
    	x3 = x0 - ((int) (v[0] * ticsize * 4)); /* compute text position */
    	y3 = y0 - ((int) (v[1] * ticsize * 4));
    }
    clip_move(x1,y1);
    clip_vector(x2,y2);

    /* label the ticmark */
    if (text == NULL)
	 text = yformat;

    (void) sprintf(ticlabel, text, CheckLog(log_y, place));
    if (apx_eq(v[0], 0.0)) {
    	if ((*t->justify_text)(CENTRE)) {
    	    clip_put_text(x3,y3,ticlabel);
    	} else {
    	    clip_put_text(x3-(t->h_char)*strlen(ticlabel)/2,y3,ticlabel);
    	}
    }
    else if (v[0] > 0) {
    	if ((*t->justify_text)(RIGHT)) {
    	    clip_put_text(x3,y3,ticlabel);
    	} else {
    	    clip_put_text(x3-(t->h_char)*strlen(ticlabel),y3,ticlabel);
    	}
    } else {
    	(*t->justify_text)(LEFT);
	clip_put_text(x3,y3,ticlabel);
    }
}

/* draw and label a z-axis ticmark */
static ztick(place, text, spacing, ticscale, xpos, ypos)
        double place;                   /* where on axis to put it */
        char *text;                     /* optional text label */
        double spacing;         /* something to use with checkzero */
        double ticscale;         /* scale factor for tic mark (0..1] */
	double xpos, ypos;
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int x0,y0,x1,y1,x2,y2,x3,y3;
    int ticsize = (int)((t->h_tic) * ticscale);

    place = CheckZero(place,spacing); /* to fix rounding error near zero */

    map3d_xy(xpos, ypos, place, &x0, &y0);

    if (tic_in) {
	x1 = x0;
	y1 = y0;
	x2 = x0 + ticsize;
	y2 = y0;
    	x3 = x0 - ticsize;
    	y3 = y0;
    } else {
	x1 = x0;
	y1 = y0;
	x2 = x0 - ticsize;
	y2 = y0;
    	x3 = x0 - ticsize * 2; /* compute text position */
    	y3 = y0;
    }
    clip_move(x1,y1);
    clip_vector(x2,y2);

    /* label the ticmark */
    if (text == NULL)
	 text = zformat;

    (void) sprintf(ticlabel, text, CheckLog(log_z, place));
    if ((*t->justify_text)(RIGHT)) {
        clip_put_text(x3,y3,ticlabel);
    } else {
        clip_put_text(x3-(t->h_char)*(strlen(ticlabel)+1),y3,ticlabel);
    }
}
