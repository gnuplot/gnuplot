#ifndef lint
static char *RCSid = "$Id: graph3d.c%v 3.50.1.9 1993/08/05 05:38:59 woo Exp $";
#endif


/* GNUPLOT - graph3d.c */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
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
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#if !defined(u3b2)
#include <time.h>
#endif
#if !defined(sequent) && !defined(apollo) && !defined(alliant)
#include <limits.h>
#endif
#include "plot.h"
#include "setshow.h"

#if defined(DJGPP)||defined(sun386)
#define time_t unsigned long
#endif

#if defined(apollo) || defined(sequent) || defined(u3b2) || defined(alliant) || defined(sun386)
#include <sys/types.h> /* typedef long time_t; */
#endif

int suppressMove = 0;  /* for preventing moveto while drawing contours */
#ifndef AMIGA_SC_6_1
extern char *strcpy(),*strncpy(),*strcat(),*ctime(),*tdate;
#else /* AMIGA_SC_6_1 */
extern char *tdate;
#endif /* AMIGA_SC_6_1 */
#ifdef AMIGA_AC_5
extern time_t dated;
#else
extern time_t dated; /* ,time(); */
#include <time.h>
#endif

#ifdef __TURBOC__
#include <stdlib.h>		/* for qsort */
#endif

/*
 * hidden_line_type_above, hidden_line_type_below - controls type of lines
 *   for above and below parts of the surface.
 * hidden_no_update - if TRUE lines will be hidden line removed but they
 *   are not assumed to be part of the surface (i.e. grid) and therefore
 *   do not influence the hidings.
 * hidden_active - TRUE if hidden lines are to be removed.
 */
static int hidden_active = FALSE;

/* LITE defines a restricted memory version for MS-DOS */

#ifndef LITE

static int hidden_line_type_above, hidden_line_type_below, hidden_no_update;

/* We divvy up the figure into the component boxes that make it up, and then
   sort them by the z-value (which is really just an average value).  */
struct pnts{
  int x,y,z;
  int flag;
  long int style_used;	/* acw test */
  int nplot;
};
static int * boxlist;
static struct pnts * nodes;
/* These variables are used to keep track of the range of x values used in the
line drawing routine.  */
static long int xmin_hl,xmax_hl;
/* These arrays are used to keep track of the minimum and maximum y values used
   for each X value.  These are only used for drawing the individual boxes that
   make up the 3d figure.  After each box is drawn, the information is copied
   to the bitmap. */
static short int *ymin_hl, *ymax_hl;
/*
 * These numbers are chosen as dividers into the bitmap.
 */
static short int xfact, yfact;
#define XREDUCE(X) ((X)/xfact)
#define YREDUCE(Y) ((Y)/yfact)
/* Bitmap of the screen.  The array for each x value is malloc-ed as needed */
static short int **pnt;
#define IFSET(X,Y) (pnt[X] == 0 ? 0 : (((pnt[X])[(Y)>>4] >> ((Y) & 0xf)) & 0x01))
static plot3d_hidden();

#endif /* LITE */


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
static setlinestyle();
#ifdef __PUREC__
/* a little problem with the 16bit int size of PureC. this completely broke
   the hidded3d feature. doesn't really fix it, but I'm working at it.  (AL) */
static int clip_point(int x, int y);
static void clip_put_text(int x, int y, char *str);
#endif

#ifndef max		/* Lattice C has max() in math.h, but shouldn't! */
#define max(a,b) ((a > b) ? a : b)
#endif

#ifndef min
#define min(a,b) ((a < b) ? a : b)
#endif

#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

#define apx_eq(x,y) (fabs(x-y) < 0.001)
#ifndef abs
#define abs(x) ((x) >= 0 ? (x) : -(x))
#endif
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

/* And the functions to map from user 3D space to terminal z coordinate */
static int map3d_z(x, y, z)
double x, y, z;
{
    int i, zt;
    double v[4], res,			     /* Homogeneous coords. vectors. */
	w = trans_mat[3][3];

    v[0] = map_x3d(x); /* Normalize object space to -1..1 */
    v[1] = map_y3d(y);
    v[2] = map_z3d(z);
    v[3] = 1.0;

    res = trans_mat[3][2];     	      /* Initiate it with the weight factor. */
    for (i = 0; i < 3; i++) res += v[i] * trans_mat[i][2];
    if(w==0) w= 1e-5;
    for (i = 0; i < 3; i++) w += v[i] * trans_mat[i][3];
    zt = ((int) (res * 16384 / w));
    return  zt;
}

/* Initialize the line style using the current device and set hidden styles  */
/* to it as well if hidden line removal is enabled.			     */
static setlinestyle(style)
int style;
{
    register struct termentry *t = &term_tbl[term];

    (*t->linetype)(style);

#ifndef LITE
    if (hidden3d) {
	hidden_line_type_above = style;
	hidden_line_type_below = style;
    }
#endif  /* LITE */
}

#ifndef LITE
/* Initialize the necessary steps for hidden line removal. */
static void init_hidden_line_removal()
{
  int i;
  /*  We want to keep the bitmap size less than 2048x2048, so we choose
   *  integer dividers for the x and y coordinates to keep the x and y
   *  ranges less than 2048.  In practice, the x and y sizes for the bitmap
   *  will be somewhere between 1024 and 2048, except in cases where the
   *  coordinates ranges for the device are already less than 1024.
   *  We do this mainly to control the size of the bitmap, but it also
   *  speeds up the computation.  We maintain separate dividers for
   *  x and y.
   */
  xfact = (xright-xleft)/1024;
  yfact = (ytop-ybot)/1024;
  if(xfact == 0) xfact=1;
  if(yfact == 0) yfact=1;
  if(pnt == 0){
    i = sizeof(short int*)*(XREDUCE(xright) - XREDUCE(xleft) + 1);
    pnt = (short int **) alloc((unsigned long)i, "hidden");
    bzero(pnt,i);
  };
  ymin_hl = (short int *) alloc((unsigned long)sizeof(short int)*
				(XREDUCE(xright) - XREDUCE(xleft) + 1), "hidden");
  ymax_hl = (short int *) alloc((unsigned long)sizeof(short int)*
				(XREDUCE(xright) - XREDUCE(xleft) + 1), "hidden");
}

/* Reset the hidden line data to a fresh start.				     */
static void reset_hidden_line_removal()
{
    int i;
    if(pnt){
      for(i=0;i<=XREDUCE(xright)-XREDUCE(xleft);i++) {
	if(pnt[i])
	  { free(pnt[i]); pnt[i] = 0;};
      };
    };
}

/* Terminates the hidden line removal process. Free any memory allocated by  */
/* init_hidden_line_removal above.					     */
static void term_hidden_line_removal()
{
     if(pnt){
       int j;
       for(j=0;j<=XREDUCE(xright)-XREDUCE(xleft);j++) {
 	if(pnt[j])
 	  { free(pnt[j]); pnt[j] = 0;};
       };
       free(pnt);
       pnt = 0;
     };
   free(ymin_hl);
   free(ymax_hl);
}
#endif /* not LITE */

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

#ifndef LITE
    if(hidden3d && draw_surface)
      {
	char flag;
	register int xv, yv, errx, erry, err;
	register int xvr, yvr;
	int xve, yve;
	register int dy, nstep, dyr;
	int i;
	if (x1 > x2){
	  xvr = x2;
	  yvr = y2;
	  xve = x1;
	  yve = y1;
	} else {
	  xvr = x1;
	  yvr = y1;
	  xve = x2;
	  yve = y2;
	};
	errx = XREDUCE(xve) - XREDUCE(xvr);
	erry = YREDUCE(yve) - YREDUCE(yvr);
	dy = (erry > 0 ? 1 : -1);
	dyr = dy*yfact;
	switch (dy){
	case 1:
	  nstep = errx + erry;
	  errx = -errx;
	  break;
	case -1:
	  nstep = errx - erry;
	  errx = -errx;
	  erry = -erry;
	  break;
	};
	err = errx + erry;
	errx <<= 1;
	erry <<= 1;
	xv = XREDUCE(xvr) - XREDUCE(xleft);
	yv = YREDUCE(yvr) - YREDUCE(ybot);
	(*t->move)(xvr,yvr);
	if( !IFSET(xv,yv) ) flag = 0;
	else flag = 1;
	if(!hidden_no_update){ /* Check first point */
	  if (xv < xmin_hl) xmin_hl = xv;
	  if (xv > xmax_hl) xmax_hl = xv;
	  if (yv > ymax_hl[xv]) ymax_hl[xv] = yv;
	  if (yv < ymin_hl[xv]) ymin_hl[xv] = yv;
	};
	for (i=0;i<nstep;i++){
	  if (err < 0){
	    xv ++;
	    xvr += xfact;
	    err += erry;
	  } else {
	    yv += dy;
	    yvr += dyr;
	    err += errx;
	  };
	  if( !IFSET(xv,yv)){
	    if(flag != 0) {(*t->move)(xvr,yvr); flag = 0;};
	  } else {
	    if(flag == 0) {(*t->vector)(xvr,yvr); flag = 1;};
	  };
	  if(!hidden_no_update){
	    if (xv < xmin_hl) xmin_hl = xv;
	    if (xv > xmax_hl) xmax_hl = xv;
	    if (yv > ymax_hl[xv]) ymax_hl[xv] = yv;
	    if (yv < ymin_hl[xv]) ymin_hl[xv] = yv;
	  };
	};
	if (flag == 0) (*t->vector)(xve, yve);
	return;
      };
#endif /* not LITE */
    if(!suppressMove) (*t->move)(x1,y1);
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
#define CheckLog(is_log, base_log, x) ((is_log) ? pow(base_log, (x)) : (x))
#else
static double
CheckLog(is_log, base_log, x)
     TBOOLEAN is_log;
     double base_log;
     double x;
{
  if (is_log)
    return(pow(base_log, x));
  else
    return(x);
}
#endif /* sun386 */

static double
LogScale(coord, is_log, log_base_log, what, axis)
	double coord;			/* the value */
	TBOOLEAN is_log;			/* is this axis in logscale? */
	double log_base_log;		/* if so, the log of its base */
	char *what;			/* what is the coord for? */
	char *axis;			/* which axis is this for ("x" or "y")? */
{
    if (is_log) {
	   if (coord <= 0.0) {
		  char errbuf[100];		/* place to write error message */
		(void) sprintf(errbuf,"%s has %s coord of %g; must be above 0 for log scale!",
				what, axis, coord);
		  (*term_tbl[term].text)();
		  (void) fflush(outfile);
		  int_error(errbuf, NO_CARET);
	   } else
		return(log(coord)/log_base_log);
    }
    return(coord);
}

/* borders of plotting area */
/* computed once on every call to do_plot */
static boundary3d(scaling)
	TBOOLEAN scaling;		/* TRUE if terminal is doing the scaling */
{
    register struct termentry *t = &term_tbl[term];
    /* luecken@udel.edu modifications
       sizes the plot to take up more of available resolution */
    xleft = (t->h_char)*2 + (t->h_tic);
    xright = (scaling ? 1 : xsize) * (t->xmax) - (t->h_char)*2 - (t->h_tic);
    ybot = (t->v_char)*5/2 + 1;
    ytop = (scaling ? 1 : ysize) * (t->ymax) - (t->v_char)*5/2 - 1;
    xmiddle = (xright + xleft) / 2;
    ymiddle = (ytop + ybot) / 2;
    xscaler = (xright - xleft) * 4 / 7;
    yscaler = (ytop - ybot) * 4 / 7;
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


static double make_3dtics(tmin,tmax,axis,logscale, base_log)
double tmin,tmax;
int axis;
TBOOLEAN logscale;
double base_log;
{
int x1,y1,x2,y2;
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
		tic = dbl_raise(base_log,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
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
register int surface;
register struct surface_points *this_plot;
int xl, yl, linetypeOffset = 0;
			/* only a Pyramid would have this many registers! */
double xtemp, ytemp, ztemp, temp;
struct text_label *this_label;
struct arrow_def *this_arrow;
TBOOLEAN scaling;
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

	/* The extrema need to be set even when a surface is not being
	 * drawn.   Without this, gnuplot used to assume that the X and
	 * Y axis started at zero.   -RKC
	 */

	/* find (bottom) left corner of grid */
	min_sx_ox = min_x;
	min_sx_oy = min_y;
	/* find bottom (right) corner of grid */
	min_sy_ox = max_x;
	min_sy_oy = min_y;


    if (polar)
	int_error("Cannot splot in polar coordinate system.", NO_CARET);

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

#ifndef LITE
    if (hidden3d) {
	struct surface_points *plot;
  
        /* Verify data is hidden line removable - grid based. */
      	for (plot = plots; plot != NULL; plot = plot->next_sp) {
 	    if (plot->plot_type == DATA3D && !plot->has_grid_topology){
	      fprintf(stderr,"Notice: Cannot remove hidden lines from non grid data\n");
              return(0);
            }
	
        }
    }
#endif /* not LITE */

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
		dated = time( (time_t *) 0);
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
						 y+ybot-1*(t->v_char), tdate);
		}
		(void)(*t->text_angle)(0);
	}

/* PLACE LABELS */
    for (this_label = first_label; this_label!=NULL;
			this_label=this_label->next ) {
	    int x,y;

	    xtemp = LogScale(this_label->x, is_log_x, log_base_log_x, "label", "x");
	    ytemp = LogScale(this_label->y, is_log_y, log_base_log_y, "label", "y");
	    ztemp = LogScale(this_label->z, is_log_z, log_base_log_z, "label", "z");
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

	xtemp = LogScale(this_arrow->sx, is_log_x, log_base_log_x, "arrow", "x");
	ytemp = LogScale(this_arrow->sy, is_log_y, log_base_log_y, "arrow", "y");
	ztemp = LogScale(this_arrow->sz, is_log_z, log_base_log_z, "arrow", "z");
	map3d_xy(xtemp,ytemp,ztemp, &sx, &sy);

	xtemp = LogScale(this_arrow->ex, is_log_x, log_base_log_x, "arrow", "x");
	ytemp = LogScale(this_arrow->ey, is_log_y, log_base_log_y, "arrow", "y");
	ztemp = LogScale(this_arrow->ez, is_log_z, log_base_log_z, "arrow", "z");
	map3d_xy(xtemp,ytemp,ztemp, &ex, &ey);

	(*t->arrow)(sx, sy, ex, ey, this_arrow->head);
    }

#ifndef LITE
    if (hidden3d && draw_surface) {
	init_hidden_line_removal();
	reset_hidden_line_removal();
	hidden_active = TRUE;
    }
#endif /* not LITE */

/* DRAW SURFACES AND CONTOURS */
	real_z_min3d = min_z;
	real_z_max3d = max_z;
	if (key == -1) {
	    xl = xright  - (t->h_tic) - (t->h_char)*5;
	    yl = ytop - (t->v_tic) - (t->v_char);
	}
	if (key == 1) {
	    xtemp = LogScale(key_x, is_log_x, log_base_log_x, "key", "x");
	    ytemp = LogScale(key_y, is_log_y, log_base_log_y, "key", "y");
	    ztemp = LogScale(key_z, is_log_z, log_base_log_z, "key", "z");
	    map3d_xy(xtemp,ytemp,ztemp, &xl, &yl);
	}

#ifndef LITE
	if (hidden3d && draw_surface) plot3d_hidden(plots,pcount);
#endif /* not LITE */
	this_plot = plots;
	for (surface = 0;
	     surface < pcount;
	     this_plot = this_plot->next_sp, surface++) {
#ifndef LITE
		if ( hidden3d )
		    hidden_no_update = FALSE;
#endif /* not LITE */

		if (draw_surface) {
		    (*t->linetype)(this_plot->line_type);
#ifndef LITE
		    if (hidden3d) {
			hidden_line_type_above = this_plot->line_type;
			hidden_line_type_below = this_plot->line_type + 1;
		    }
#endif /* not LITE */		    
		    if (key != 0 && this_plot->title) {
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
				case BOXES: /* can't do boxes in 3d yet so use impulses */
		        case IMPULSES: {
			    if (key != 0 && this_plot->title) {
				clip_move(xl+(t->h_char),yl);
				clip_vector(xl+4*(t->h_char),yl);
			    }
			    if (!(hidden3d && draw_surface))
			      plot3d_impulses(this_plot);
			    break;
			}
			case LINES: {
			    if (key != 0 && this_plot->title) {
				clip_move(xl+(int)(t->h_char),yl);
				clip_vector(xl+(int)(4*(t->h_char)),yl);
			    }
			    if (!(hidden3d && draw_surface))
			      plot3d_lines(this_plot);
			    break;
			}
			case ERRORBARS:	/* ignored; treat like points */
			case POINTSTYLE: {
			    if (key != 0 && this_plot->title 
				&& !clip_point(xl+2*(t->h_char),yl)) {
				(*t->point)(xl+2*(t->h_char),yl,
					    this_plot->point_type);
			    }
			    if (!(hidden3d && draw_surface))
			      plot3d_points(this_plot);
			    break;
			}
			case LINESPOINTS: {
			    /* put lines */
			    if (key != 0 && this_plot->title) {
				clip_move(xl+(t->h_char),yl);
				clip_vector(xl+4*(t->h_char),yl);
			    }
 			    if (!(hidden3d && draw_surface))
 			      plot3d_lines(this_plot);
			
			    /* put points */
			    if (key != 0 && this_plot->title 
				&& !clip_point(xl+2*(t->h_char),yl)) {
				(*t->point)(xl+2*(t->h_char),yl,
					    this_plot->point_type);
			    }
			    if (!(hidden3d && draw_surface))
			      plot3d_points(this_plot);
			    break;
			}
			case DOTS: {
			    if (key != 0 && this_plot->title
				&& !clip_point(xl+2*(t->h_char),yl)) {
				(*t->point)(xl+2*(t->h_char),yl, -1);
			    }
			    if (!(hidden3d && draw_surface))
			      plot3d_dots(this_plot);
			    break;
			}
		    }
		    if (key != 0 && this_plot->title)
		        yl = yl - (t->v_char);
		}

#ifndef LITE
		if ( hidden3d ) {
		    hidden_no_update = TRUE;
		    hidden_line_type_above = this_plot->line_type + (hidden3d ? 2 : 1);
		    hidden_line_type_below = this_plot->line_type + (hidden3d ? 2 : 1);
		}
#endif /* not LITE */

		if (draw_contour && this_plot->contours != NULL) {
			struct gnuplot_contours *cntrs = this_plot->contours;

			(*t->linetype)(this_plot->line_type + (hidden3d ? 2 : 1));

			if (key != 0 && this_plot->title 
			          && !(draw_surface && label_contours) ) {
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
					case POINTSTYLE:
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
				yl = yl - (t->v_char);
			}
			yl = yl + (t->v_char);

 			linetypeOffset = this_plot->line_type + (hidden3d ? 2 : 1);
			while (cntrs) {
 				if(label_contours && cntrs->isNewLevel) {
 					(*t->linetype)(linetypeOffset++);
#ifndef LITE
 					if(hidden3d) hidden_line_type_below = hidden_line_type_above = linetypeOffset-1;
#endif /* not LITE */
 					yl -= (t->v_char);
 					if ((*t->justify_text)(RIGHT)) {
 					   clip_put_text(xl,
 						   yl,cntrs->label);
 				   }
 				   else {
 					   if (inrange(xl-(t->h_char)*strlen(cntrs->label),
 								xleft, xright))
 						clip_put_text(xl-(t->h_char)*strlen(cntrs->label),
 									yl,cntrs->label);
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
 					   case POINTSTYLE:
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
				switch(this_plot->plot_style) {
					case IMPULSES:
			   			cntr3d_impulses(cntrs, this_plot);
						break;
					case LINES:
						cntr3d_lines(cntrs);
						break;
					case ERRORBARS: /* ignored; treat like points */
					case POINTSTYLE:
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
			if (key != 0 && this_plot->title)
			  yl = yl - (t->v_char);
		}

		if (surface == 0)
		    draw_bottom_grid(this_plot,real_z_min3d,real_z_max3d);
	}
	(*t->text)();
	(void) fflush(outfile);

#ifndef LITE
	if (hidden3d) {
	    term_hidden_line_removal();
	    hidden_active = FALSE;
	}
#endif /* not LITE */
}

/* plot3d_impulses:
 * Plot the surfaces in IMPULSES style
 */
static plot3d_impulses(plot)
	struct surface_points *plot;
{
    int i;				/* point index */
    int x,y,x0,y0;			/* point in terminal coordinates */
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate GPHUGE *points = icrvs->points;

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
/* We want to always draw the lines in the same direction, otherwise when
   we draw an adjacent box we might get the line drawn a little differently
   and we get splotches.  */

#ifndef LITE

static int zsort( r1, r2)
int * r1;
int * r2;
{
  int z1, z2;
  z1 = nodes[*r1].z;
  z2 = nodes[*r2].z;
  if (z1 < z2) return 1;
  if (z1 == z2) return 0;
  return -1;
}
#define TESTBOX(X,Y)					\
  if(X<xmin_box) xmin_box = X;				\
  if(X>xmax_box) xmax_box = X;				\
  if(Y<ymin_box) ymin_box = Y;				\
  if(Y>ymax_box) ymax_box = Y;
/* Usefull macro to help us figure out which side of the surface we are on */
#define XPRD(I,J,K) 					\
  ((nodes[I].x-nodes[J].x)*(nodes[J].y-nodes[K].y) -	\
  (nodes[I].y-nodes[J].y)*(nodes[J].x-nodes[K].x))
#define MAYBE_LINEPOINT(J)				        \
    if((nodes[J].flag & 0x20) != 0) {				\
      x = nodes[J].x;						\
      y = nodes[J].y;						\
      nodes[J].flag -= 0x20;					\
      if (!clip_point(x,y) && 					\
	  !IFSET(XREDUCE(x)-XREDUCE(xleft),YREDUCE(y)-YREDUCE(ybot))) \
	(*t->point)(x,y, plot_info[nplot].point_type);		\
    };

struct surface_plots{
  int above_color;
  int below_color;
  int row_offset;
  int point_type;
};
/* All of the plots coming into this routine are assumed to have grid
   topology.  */

static plot3d_hidden(plots, pcount)
     struct surface_points *plots;
     int pcount;
{
  struct surface_points *this_plot;
  long int i, j;
  int nplot;
  long int x,y,z ,nseg, ncrv, ncrv1;        /* point in terminal coordinates */
#ifdef AMIGA_SC_6_1
  unsigned short int * cpnt;
#else /* !AMIGA_SC_6_1 */
  short int * cpnt;
#endif /* !AMIGA_SC_6_1 */
  short int  mask1, mask2;
  long int indx1, indx2, k, m;
  short int xmin_box, xmax_box, ymin_box, ymax_box;
  struct surface_plots * plot_info;
  int row_offset, nnode;
  short int y_malloc;  /* Amount of space we need for one vertical row of
                          bitmap, and byte offset of first used element */
  struct termentry *t = &term_tbl[term];
  struct iso_curve *icrvs;
  int current_style = 0x7fff;  /* Current line style */
  int surface;
  nnode = 0;
  nseg = 0;
  nplot = 0;
  this_plot = plots;

  for (surface = 0;
       surface < pcount;
       this_plot = this_plot->next_sp, surface++) {
    nplot++;
    icrvs = plots->iso_crvs;
    icrvs = plots->iso_crvs;
    if(this_plot->plot_type == FUNC3D) {
        for(icrvs = this_plot->iso_crvs,ncrv=0;icrvs;icrvs=icrvs->next,ncrv++) { };
 /*      if(this_plot->has_grid_topology) ncrv >>= 1; */
    };
    if(this_plot->plot_type == DATA3D)
       ncrv = this_plot->num_iso_read;
    nnode += ncrv * (this_plot->iso_crvs->p_count);
/*    for(icrvs = this_plot->iso_crvs,ncrv=0;icrvs;icrvs=icrvs->next,ncrv++) { };
    nnode += ncrv * (this_plot->iso_crvs->p_count); */
    switch(this_plot->plot_style) {
    case ERRORBARS:
    case DOTS:
    case POINTSTYLE:
    case LINESPOINTS:
      nseg += (ncrv) * (this_plot->iso_crvs->p_count);
      break;
    case LINES:
      nseg += (ncrv-1) * (this_plot->iso_crvs->p_count-1);
      break;
    case IMPULSES:
      /* There will be two nodes for each segment */
      nnode += ncrv * (this_plot->iso_crvs->p_count);
      nseg += (ncrv) * (this_plot->iso_crvs->p_count);
      break;
    }
  };
  boxlist = (int *) alloc((unsigned long)sizeof(int)*nseg, "hidden");
  nodes = (struct pnts *) alloc((unsigned long)sizeof(struct pnts)*nnode, "hidden");
  plot_info = (struct surface_plots *) alloc((unsigned long)sizeof(struct surface_plots)*nplot,"hidden");
  nnode = 0;
  nseg = 0;
  nplot = 0;
  this_plot = plots;
  hidden_no_update = FALSE;

  if ( hidden3d && draw_surface)
    for (surface = 0;
	 surface < pcount;
	 this_plot = this_plot->next_sp, surface++) {
      (*t->linetype)(this_plot->line_type);
      hidden_line_type_above = this_plot->line_type;
		hidden_line_type_below = this_plot->line_type + 1;
    if(this_plot->plot_type == FUNC3D) {
      for(icrvs = this_plot->iso_crvs,ncrv=0;icrvs;icrvs=icrvs->next,ncrv++) { };
/*      if(this_plot->has_grid_topology) ncrv >>= 1; */
    };
    if(this_plot->plot_type == DATA3D)
      ncrv = this_plot->num_iso_read;
      icrvs = this_plot->iso_crvs;
      ncrv1 = ncrv;
      ncrv = 0;
      while ( icrvs) {
	struct coordinate GPHUGE *points = icrvs->points;
	for (i = 0; i < icrvs->p_count; i++) {
	  map3d_xy(points[i].x, points[i].y, points[i].z,&nodes[nnode].x,&nodes[nnode].y);
	  nodes[nnode].z = map3d_z(points[i].x, points[i].y, points[i].z);
	  nodes[nnode].flag = (i==0 ? 1 : 0) + (ncrv == 0 ? 2 : 0) +
	    (i == icrvs->p_count-1 ? 4 : 0) + (ncrv == ncrv1-1 ? 8 : 0);
	  nodes[nnode].nplot = nplot;
	  nodes[nnode].style_used = -1000; /* indicates no style */
	  switch(this_plot->plot_style) {
	  case LINESPOINTS:
	    if(i < icrvs->p_count-1 && ncrv < ncrv1-1)
	      nodes[nnode].flag |= 0x30;
	    else
	      nodes[nnode].flag |= 0x20;
	    boxlist[nseg++] = nnode++;
	    break;
	  case LINES:
	    if(i < icrvs->p_count-1 && ncrv < ncrv1-1)
	      {
		nodes[nnode].flag |= 0x10;
		boxlist[nseg++] = nnode++;
	      }
	    else
	      nnode++;
	    break;
	  case ERRORBARS:
	  case POINTSTYLE:
	  case DOTS:
	    nodes[nnode].flag |= 0x40;
	    boxlist[nseg++] = nnode++;
	    break;
	  case IMPULSES:
	    nodes[nnode].flag |= 0x80;
	    boxlist[nseg++] = nnode++;
	    map3d_xy(points[i].x, points[i].y, z_min3d, &nodes[nnode].x,&nodes[nnode].y);
	    nodes[nnode].z = map3d_z(points[i].x, points[i].y, z_min3d);
	    nnode++;
	    break;
	    break;
	  }
	}
	icrvs = icrvs->next;
	ncrv++;
	if(ncrv == ncrv1) break;
      }
      /* Next we go through all of the boxes, and substitute the average z value
	 for the box for the z value of the corner node */
      plot_info[nplot].above_color = this_plot->line_type;
      plot_info[nplot].below_color = this_plot->line_type+1;
      plot_info[nplot].point_type =
	((this_plot->plot_style == DOTS) ? -1 : this_plot->point_type);
      plot_info[nplot++].row_offset = this_plot->iso_crvs->p_count;
    }
      for(i=0; i<nseg; i++){
	j = boxlist[i];
	if ((nodes[j].flag & 0x80) != 0) {
	  nodes[j].z = (nodes[j].z < nodes[j+1].z ? nodes[j].z : nodes[j+1].z);
	  continue;
	};
	if ((nodes[j].flag & 0x10) == 0) continue;
	row_offset = plot_info[nodes[j].nplot].row_offset;
	z = nodes[j].z;
	if (z < nodes[j+1].z) z = nodes[j+1].z;
	if (z < nodes[j+row_offset].z) z = nodes[j+row_offset].z;
	if (z < nodes[j+row_offset+1].z) z = nodes[j+row_offset+1].z;
      };
  qsort (boxlist, nseg, sizeof(int), zsort);
  y_malloc = (2+ (YREDUCE(ytop)>>4) - (YREDUCE(ybot)>>4))*sizeof(short int);
  for(i=0;i<=(XREDUCE(xright)-XREDUCE(xleft));i++) {
    ymin_hl[i] = 0x7fff; 
    ymax_hl[i] = 0;
  };
  for(i=0;i<nseg;i++) {
    j = boxlist[i];
    nplot = nodes[j].nplot;
    row_offset = plot_info[nplot].row_offset;
    if((nodes[j].flag & 0x40) != 0) {
      x = nodes[j].x;
      y = nodes[j].y;
      if (!clip_point(x,y) &&
	  !IFSET(XREDUCE(x)-XREDUCE(xleft),YREDUCE(y)-YREDUCE(ybot)))
	(*t->point)(x,y, plot_info[nplot].point_type);
    };
    if((nodes[j].flag & 0x80) != 0) { /* impulses */
      clip_move(nodes[j].x,nodes[j].y);
      clip_vector(nodes[j+1].x,nodes[j+1].y);
    };
    if((nodes[j].flag & 0x10) != 0) {
/* It is possible, and often profitable, to take a quick look and see
   if the current box is entirely obscured.  If this is the case we will
   not even bother testing this box any further.  */
      xmin_box = 0x7fff; 
      xmax_box = 0;
      ymin_box = 0x7fff; 
      ymax_box = 0;
      TESTBOX(nodes[j].x-xleft,nodes[j].y-ybot);
      TESTBOX(nodes[j+1].x-xleft,nodes[j+1].y-ybot);
      TESTBOX(nodes[j+row_offset].x-xleft,nodes[j+row_offset].y-ybot);
      TESTBOX(nodes[j+row_offset+1].x-xleft,nodes[j+row_offset+1].y-ybot);
      z=0;
      if(xmin_box < 0) xmin_box = 0;
      if(ymin_box < 0) ymin_box = 0;
      if(xmax_box > xright-xleft) xmax_box = xright-xleft;
      if(ymax_box > ytop-ybot) ymax_box = ytop-ybot;
      /* Now check bitmap.  These coordinates have not been reduced */
      if(xmin_box <= xmax_box && ymin_box <= ymax_box){
	ymin_box = YREDUCE(ymin_box);
	ymax_box = YREDUCE(ymax_box);
	xmin_box = XREDUCE(xmin_box);
	xmax_box = XREDUCE(xmax_box);
	indx1 = ymin_box >> 4;
	indx2 = ymax_box >> 4;
	mask1 = 0xffff << (ymin_box & 0x0f);
	mask2 = 0xffff >> (0x0f-(ymax_box & 0x0f));
	for(m=xmin_box;m<=xmax_box;m++) {
	  if(pnt[m] == 0) {z++; break;};
	  cpnt = pnt[m] + indx1;
	  if(indx1 == indx2){
	    if((*cpnt & mask1 & mask2) != (mask1 & mask2)) {z++; break;}
	  } else {
	    if((*cpnt++ & mask1) != mask1) {z++; break;}
	    k = indx1+1;
	    while (k != indx2) {
	      if((unsigned short)*cpnt++ != 0xffff) {z++; break;}
	      k++;
	    };
	    if((*cpnt++ & mask2) != mask2) {z++; break;}
	  };
	};
      };
      /* z is 0 if all of the pixels used by the current box are already covered.
	 No point in proceeding, so we just skip all further processing of this
	 box. */
      if(!z) continue;
      /* Now we need to figure out whether we are looking at the top or the
	 bottom of the square.  A simple cross product will tell us this.
	 If the square is really distorted then this will not be accurate,
	 but in such cases we would actually be seeing both sides at the same
	 time.  We choose the vertex with the largest z component to
	 take the cross product at.  */
      {
	int z1, z2 ,z3, z4;
	z1 = XPRD(j+row_offset,j,j+1);
	z2 = XPRD(j,j+1,j+1+row_offset);
	z3 = XPRD(j+1,j+row_offset+1,j+row_offset);
	z4 = XPRD(j+row_offset+1,j+row_offset,j);
	z=0;
	z += (z1 > 0 ? 1 : -1);
	z += (z2 > 0 ? 1 : -1);
	z += (z3 > 0 ? 1 : -1);
	z += (z4 > 0 ? 1 : -1);
	/* See if the box is uniformly one side or another. */
	if(z != 4 && z != -4) {
/* It isn't.  Now find the corner of the box with the largest z value that
   has already been plotted, and use the same style used for that node.  */
	  k = -1000;
	  x = -32768;
	  if (nodes[j].z > x && nodes[j].style_used !=-1000) {
	    k = nodes[j].style_used;
	    x = nodes[j].z;
	  };
	  if (nodes[j+1].z > x && nodes[j+1].style_used !=-1000) {
	    k = nodes[j+1].style_used;
	    x = nodes[j+1].z;
	  };
	  if (nodes[j+row_offset+1].z > x && nodes[j+row_offset+1].style_used !=-1000) {
	    k = nodes[j+row_offset+1].style_used;
	    x = nodes[j+row_offset+1].z;
	  };
	  if (nodes[j+row_offset].z > x && nodes[j+row_offset].style_used !=-1000) {
	    k = nodes[j+row_offset].style_used;
	    x = nodes[j+row_offset].z;
	  };
	  if( k != -1000){
	    z = 0; /* To defeat the logic to come.  */
	    current_style = k;
	    (*t->linetype)(current_style);
	  };
	};
	/* If k == -1000 then no corner found.  I guess it does not matter.  */
      };
      if(z > 0 && current_style != plot_info[nplot].above_color) {
	current_style = plot_info[nplot].above_color;
	(*t->linetype)(current_style);
      };
      if(z < 0 && current_style != plot_info[nplot].below_color) {
	current_style = plot_info[nplot].below_color;
	(*t->linetype)(current_style);
      };
      xmin_hl = (sizeof(xleft) == 4 ? 0x7fffffff : 0x7fff ); 
      xmax_hl = 0;
      clip_move(nodes[j].x,nodes[j].y);
      clip_vector(nodes[j+1].x,nodes[j+1].y);
      clip_vector(nodes[j+row_offset+1].x,nodes[j+row_offset+1].y);
      clip_vector(nodes[j+row_offset].x,nodes[j+row_offset].y);
      clip_vector(nodes[j].x,nodes[j].y);
      nodes[j].style_used = current_style;
      nodes[j+1].style_used = current_style;
      nodes[j+row_offset+1].style_used = current_style;
      nodes[j+row_offset].style_used = current_style;
      MAYBE_LINEPOINT(j);
      MAYBE_LINEPOINT(j+1);
      MAYBE_LINEPOINT(j+row_offset+1);
      MAYBE_LINEPOINT(j+row_offset);
      if( xmin_hl < 0 || xmax_hl > XREDUCE(xright)-XREDUCE(xleft))
	int_error("Logic error #3 in hidden line",NO_CARET);
      /* now mark the area as being filled in the bitmap.  These coordinates
         have already been reduced. */
      if (xmin_hl < xmax_hl)
	for(j=xmin_hl;j<=xmax_hl;j++) {
	  if (ymin_hl[j] == 0x7fff) 
	    int_error("Logic error #2 in hidden line",NO_CARET);
	  if(pnt[j] == 0) {
	    pnt[j] = (short int *) alloc((unsigned long)y_malloc,"hidden");
	    bzero(pnt[j],y_malloc);
	  };
	  if(ymin_hl[j] < 0 || ymax_hl[j] > YREDUCE(ytop)-YREDUCE(ybot))
	    int_error("Logic error #1 in hidden line",NO_CARET);
/* this shift is wordsize dependent */
	  indx1 = ymin_hl[j] >> 4;
	  indx2 = ymax_hl[j] >> 4;
	  mask1 = 0xffff << (ymin_hl[j] & 0xf);
	  mask2 = 0xffff >> (0xf-(ymax_hl[j] & 0xf));
	  cpnt = pnt[j] + indx1;
	  if(indx1 == indx2){
	    *cpnt |= (mask1 & mask2);
	  } else {
	    *cpnt++ |= mask1;
	    k = indx1+1;
	    while (k != indx2) {
	      *cpnt++ = 0xffff; 
	      k++;
	    };
	    *cpnt |= mask2;
	  };
	  ymin_hl[j]=0x7fff; 
	  ymax_hl[j]=0;
	};
    };
  };
  free(nodes);
  free(boxlist);
  free(plot_info);
}

#endif /* not LITE */

static plot3d_lines(plot)
	struct surface_points *plot;
{
    int i;
    int x,y;				/* point in terminal coordinates */
    struct iso_curve *icrvs = plot->iso_crvs;
    struct coordinate GPHUGE *points;

#ifndef LITE
/* These are handled elsewhere.  */
    if (plot->has_grid_topology && hidden3d)
	return(0);
#endif /* not LITE */

    while (icrvs) {

	for (i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
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
 * Plot the surfaces in POINTSTYLE style
 */
static plot3d_points(plot)
	struct surface_points *plot;
{
    int i,x,y;
    struct termentry *t = &term_tbl[term];
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate GPHUGE *points = icrvs->points;

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
	struct coordinate GPHUGE *points = icrvs->points;

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
    int i;				/* point index */
    int x,y,x0,y0;			/* point in terminal coordinates */

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
    int i;				/* point index */
    int x,y;				/* point in terminal coordinates */

    if (draw_contour == CONTOUR_SRF || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z,
    		     &x, &y);

 			if (i > 0) {
 				clip_vector(x,y);
 				if(i == 1) suppressMove = TRUE;
 			} else {
 				clip_move(x,y);
 			}
    	}
    }
 	suppressMove = FALSE;  /* beginning a new contour level, so moveto() required */

    if (draw_contour == CONTOUR_BASE || draw_contour == CONTOUR_BOTH) {
	for (i = 0; i < cntr->num_pts; i++) {
	    if (real_z_max3d<cntr->coords[i].z)
		real_z_max3d=cntr->coords[i].z;
	    if (real_z_min3d>cntr->coords[i].z)
		real_z_min3d=cntr->coords[i].z;

    	    map3d_xy(cntr->coords[i].x, cntr->coords[i].y, z_min3d,
    		     &x, &y);

 			if (i > 0) {
 				clip_vector(x,y);
 				if(i == 1) suppressMove = TRUE;
 			} else {
 				clip_move(x,y);
 			}
 		}
 	}
 	suppressMove = FALSE;  /* beginning a new contour level, so moveto() required */
}

/* cntr3d_points:
 * Plot a surface contour in POINTSTYLE style
 */
static cntr3d_points(cntr, plot)
	struct gnuplot_contours *cntr;
	struct surface_points *plot;
{
    int i;
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
    int i;
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
        grid_iso_1 = plot->plot_type == DATA3D && plot->has_grid_topology ?
					plot->iso_crvs->p_count : iso_samples_1,
        grid_iso_2 = plot->plot_type == DATA3D && plot->has_grid_topology ?
					plot->num_iso_read : iso_samples_2;
    double x,y,dx,dy;

    if (grid && plot->has_grid_topology) {

	/* fix grid lines to tic marks, D. Taber, 02-01-93 */
	if(xtics && xticdef.type == TIC_SERIES) {
		dx = xticdef.def.series.incr;
		x = xticdef.def.series.start;
		grid_iso_1 = 1 + (xticdef.def.series.end - x) / dx;
	} else {
		x = x_min3d;
	dx = (x_max3d-x_min3d) / (grid_iso_1-1);
	}

	if(ytics && yticdef.type == TIC_SERIES) {
		dy = yticdef.def.series.incr;
		y = yticdef.def.series.start;
		grid_iso_2 = 1 + (yticdef.def.series.end - y) / dy;
	} else {
		y = y_min3d;
	dy = (y_max3d-y_min3d) / (grid_iso_2-1);
	}

	for (i = 0; i < grid_iso_2; i++) {
	        if (i == 0 || i == grid_iso_2-1)	        
		    setlinestyle(-2);
		else
		    setlinestyle(-1);
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

	for (i = 0; i < grid_iso_1; i++) {
	        if (i == 0 || i == grid_iso_1-1)
		    setlinestyle(-2);
		else
		    setlinestyle(-1);
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
	setlinestyle(-2);

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
        grid_iso = plot->plot_type == DATA3D && plot->has_grid_topology ?
					plot->num_iso_read : iso_samples_2;
    struct iso_curve *icrvs = plot->iso_crvs;

    while ( icrvs ) {
	struct coordinate GPHUGE *points = icrvs->points;
	int saved_hidden_active = hidden_active;
	int z1 = map3d_z(points[0].x, points[0].y, 0.0),
	       z2 = map3d_z(points[icrvs->p_count-1].x,
                            points[icrvs->p_count-1].y, 0.0);

	for (i = 0; i < icrvs->p_count; i += icrvs->p_count-1) {
	    map3d_xy(points[i].x, points[i].y, z_min3d, &x, &y);
	    if (is_boundary) {
		setlinestyle(-2);
	    }
	    else {
	        setlinestyle(-1);
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
#ifndef LITE
	    	if (hidden3d) {
		    if ((i == 0 && z1 > z2) ||
		        (i == icrvs->p_count-1 && z2 > z1)) {
		        hidden_active = FALSE; /* This one is always visible. */
		    }	    		
	    	}
#endif /* not LITE */
	    	clip_vector(x1,y1);
	    	clip_move(x,y);
		hidden_active = saved_hidden_active;
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
    if(hidden3d){
      struct iso_curve *lcrvs = plot->iso_crvs;
      struct coordinate GPHUGE *points, GPHUGE *lpoints;
      icrvs = lcrvs;
      while(lcrvs->next) lcrvs = lcrvs->next;
      points = icrvs->points;
      lpoints = lcrvs->points;
      is_boundary = TRUE;
      for (i = 0; i < icrvs->p_count; i += (grid ? 1 : icrvs->p_count - 1)) {
	if ((i == 0) || (i == icrvs->p_count - 1)) {
	  setlinestyle(-2);
	}
	else {
	  setlinestyle(-1);
	}
	map3d_xy(points[i].x, points[i].y, z_min3d, &x, &y);
	clip_move(x, y);
	map3d_xy(lpoints[i].x, lpoints[i].y, z_min3d, &x, &y);
	clip_vector(x, y);
      };
    };
}

/* Draw the bottom grid that hold the tic marks for 3d surface. */
static draw_bottom_grid(plot, min_z, max_z)
	struct surface_points *plot;
	double min_z, max_z;
{
    int x,y;	/* point in terminal coordinates */
    double xtic,ytic,ztic;
    struct termentry *t = &term_tbl[term];

    xtic = make_3dtics(x_min3d,x_max3d,'x',is_log_x,base_log_x);
    ytic = make_3dtics(y_min3d,y_max3d,'y',is_log_y,base_log_y);
    ztic = make_3dtics(min_z,max_z,'z',is_log_z,base_log_z);

    if (draw_border)
	if (parametric || !plot->has_grid_topology)
	    draw_parametric_grid(plot);
	else
	    draw_non_param_grid(plot);

    setlinestyle(-2); /* border linetype */

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
	    case TIC_MONTH:
		draw_month_3dxtics(min_sy_oy);
		break;
	    case TIC_DAY:
		draw_day_3dxtics(min_sy_oy);
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
    if (ytics && (ytic > 0.0)) {
    	switch (yticdef.type) {
    	    case TIC_COMPUTED:
 		if (y_min3d < y_max3d) {
		    draw_3dytics(ytic * floor(y_min3d/ytic),
    				 ytic,
    				 ytic * ceil(y_max3d/ytic),
    				 min_sy_ox);
		 }else{
		    draw_3dytics(ytic * floor(y_max3d/ytic),
    				 ytic,
    				 ytic * ceil(y_min3d/ytic),
    				 min_sy_ox);
		}
    		break;
	    case TIC_MONTH:
		draw_month_3dytics(min_sy_ox);
		break;
	    case TIC_DAY:
		draw_day_3dytics(min_sy_ox);
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
	    case TIC_MONTH:
		draw_month_3dztics(min_sx_ox,min_sx_oy,min_z,max_z);
		break;
	    case TIC_DAY:
		draw_day_3dztics(min_sx_ox,min_sx_oy,min_z,max_z);
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
		if (ticplace < start || ticplace > end) continue;
		xtick(ticplace, xformat, incr, 1.0, ypos);
		if (is_log_x && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic < (int)base_log_x; ltic++) {
				lticplace = ticplace+log((double)ltic)/log_base_log_x;
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
		if (ticplace < start || ticplace > end) continue;
		ytick(ticplace, yformat, incr, 1.0, xpos);
		if (is_log_y && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic < (int)base_log_y; ltic++) {
				lticplace = ticplace+log((double)ltic)/log_base_log_y;
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

	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if (ticplace < start || ticplace > end) continue;

		ztick(ticplace, zformat, incr, 1.0, xpos, ypos);
		if (is_log_z && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic < (int)base_log_z; ltic++) {
				lticplace = ticplace+log((double)ltic)/log_base_log_z;
				ztick(lticplace, "\0", incr, 0.5, xpos, ypos);
			}
		}
	}

	/* Make sure the vertical line is fully drawn. */
	setlinestyle(-2);	/* axis line type */

	map3d_xy(xpos, ypos, z_min3d, &x, &y);
	clip_move(x,y);
	map3d_xy(xpos, ypos, min(end,z_max)+(is_log_z ? incr : 0.0), &x, &y);
	clip_vector(x,y);

	setlinestyle(-1); /* border linetype */
}

/* DRAW_SERIES_3DXTICS: draw a user tic series, x axis */
static draw_series_3dxtics(start, incr, end, ypos)
		double start, incr, end, ypos; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace, place;
	double ticmin, ticmax;	/* for checking if tic is almost inrange */
	double spacing = is_log_x ? log(incr)/log_base_log_x : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(is_log_x, base_log_x, x_min3d),
			  CheckLog(is_log_x, base_log_x, x_max3d));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(is_log_x, base_log_x, x_min3d),
			     CheckLog(is_log_x, base_log_x, x_max3d)));

	/* to allow for rounding errors */
	ticmin = min(x_min3d,x_max3d) - SIGNIF*incr;
	ticmax = max(x_min3d,x_max3d) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (is_log_x ? log(ticplace)/log_base_log_x : ticplace);
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
	double spacing = is_log_y ? log(incr)/log_base_log_y : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(is_log_y, base_log_y, y_min3d),
			  CheckLog(is_log_y, base_log_y, y_max3d));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(is_log_y, base_log_y, y_min3d),
			     CheckLog(is_log_y, base_log_y, y_max3d)));

	/* to allow for rounding errors */
	ticmin = min(y_min3d,y_max3d) - SIGNIF*incr;
	ticmax = max(y_min3d,y_max3d) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (is_log_y ? log(ticplace)/log_base_log_y : ticplace);
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
	double spacing = is_log_x ? log(incr)/log_base_log_x : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(is_log_z, base_log_z, z_min),
			  CheckLog(is_log_z, base_log_z, z_max));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(is_log_z, base_log_z, z_min),
			     CheckLog(is_log_z, base_log_z, z_max)));

	/* to allow for rounding errors */
	ticmin = min(z_min,z_max) - SIGNIF*incr;
	ticmax = max(z_min,z_max) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (is_log_z ? log(ticplace)/log_base_log_z : ticplace);
	    if ( inrange(place,ticmin,ticmax) )
		 ztick(place, zformat, spacing, 1.0, xpos, ypos);
	}

	/* Make sure the vertical line is fully drawn. */
	setlinestyle(-2);	/* axis line type */

	map3d_xy(xpos, ypos, z_min3d, &x, &y);
	clip_move(x,y);
	map3d_xy(xpos, ypos, min(end,z_max)+(is_log_z ? incr : 0.0), &x, &y);
	clip_vector(x,y);

	setlinestyle(-1); /* border linetype */
}
extern char *month[];
extern char *day[];
draw_month_3dxtics(ypos)
double ypos;
{
    long l_ticplace,l_incr,l_end,m_calc;

    l_ticplace = (long)x_min3d;
    if((double)l_ticplace<x_min3d)l_ticplace++;
    l_end=(long)x_max3d;
    l_incr=(l_end-l_ticplace)/12;
    if(l_incr<1)l_incr=1;
    while(l_ticplace<=l_end)
    {	m_calc=(l_ticplace-1)%12;
	if(m_calc<0)m_calc += 12;
	xtick((double)l_ticplace,month[m_calc],(double)l_incr,1.0,ypos);
	l_ticplace += l_incr;
    }
}
draw_month_3dytics(xpos)
double xpos;
{
    long l_ticplace,l_incr,l_end,m_calc;

    l_ticplace = (long)y_min3d;
    if((double)l_ticplace<y_min3d)l_ticplace++;
    l_end=(long)y_max3d;
    l_incr=(l_end-l_ticplace)/12;
    if(l_incr<1)l_incr=1;
    while(l_ticplace<=l_end)
    {	m_calc=(l_ticplace-1)%12;
	if(m_calc<0)m_calc += 12;
	ytick((double)l_ticplace,month[m_calc],(double)l_incr,1.0,xpos);
	l_ticplace += l_incr;
    }
}
draw_month_3dztics(xpos,ypos,z_min3d,z_max3d)
double xpos,ypos,z_min3d,z_max3d;
{
    long l_ticplace,l_incr,l_end,m_calc;

    l_ticplace = (long)z_min3d;
    if((double)l_ticplace<z_min3d)l_ticplace++;
    l_end=(long)z_max3d;
    l_incr=(l_end-l_ticplace)/12;
    if(l_incr<1)l_incr=1;
    while(l_ticplace<=l_end)
    {	m_calc=(l_ticplace-1)%12;
	if(m_calc<0)m_calc += 12;
	ztick((double)l_ticplace,month[m_calc],(double)l_incr,1.0,xpos,ypos);
	l_ticplace += l_incr;
    }
}
draw_day_3dxtics(ypos)
double ypos;
{
    long l_ticplace,l_incr,l_end,m_calc;

    l_ticplace = (long)x_min3d;
    if((double)l_ticplace<x_min3d)l_ticplace++;
    l_end=(long)x_max3d;
    l_incr=(l_end-l_ticplace)/14;
    if(l_incr<1)l_incr=1;
    while(l_ticplace<=l_end)
    {	m_calc=l_ticplace%7;
	if(m_calc<0)m_calc += 7;
	xtick((double)l_ticplace,day[m_calc],(double)l_incr,1.0,ypos);
	l_ticplace += l_incr;
    }
}
draw_day_3dytics(xpos)
double xpos;
{
    long l_ticplace,l_incr,l_end,m_calc;

    l_ticplace = (long)y_min3d;
    if((double)l_ticplace<y_min3d)l_ticplace++;
    l_end=(long)y_max3d;
    l_incr=(l_end-l_ticplace)/14;
    if(l_incr<1)l_incr=1;
    while(l_ticplace<=l_end)
    {	m_calc=l_ticplace%7;
	if(m_calc<0)m_calc += 7;
	ytick((double)l_ticplace,day[m_calc],(double)l_incr,1.0,xpos);
	l_ticplace += l_incr;
    }
}
draw_day_3dztics(xpos,ypos,z_min3d,z_max3d)
double xpos,ypos,z_min3d,z_max3d;
{
    long l_ticplace,l_incr,l_end,m_calc;

    l_ticplace = (long)z_min3d;
    if((double)l_ticplace<z_min3d)l_ticplace++;
    l_end=(long)z_max3d;
    l_incr=(l_end-l_ticplace)/14;
    if(l_incr<1)l_incr=1;
    while(l_ticplace<=l_end)
    {	m_calc=l_ticplace%7;
	if(m_calc<0)m_calc += 7;
	ztick((double)l_ticplace,day[m_calc],(double)l_incr,1.0,xpos,ypos);
	l_ticplace += l_incr;
    }
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
	   ticplace = (is_log_x ? log(list->position)/log_base_log_x
				: list->position);
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
	   ticplace = (is_log_y ? log(list->position)/log_base_log_y
				: list->position);
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

    while (list != NULL) {
	   ticplace = (is_log_z ? log(list->position)/log_base_log_z
				: list->position);
	   if ( inrange(ticplace, z_min, z_max) 		/* in range */
		  || NearlyEqual(ticplace, z_min, incr)		/* == z_min */
		  || NearlyEqual(ticplace, z_max, incr))	/* == z_max */
		ztick(ticplace, list->label, incr, 1.0, xpos, ypos);

	   list = list->next;
    }

    /* Make sure the vertical line is fully drawn. */
    setlinestyle(-2);	/* axis line type */

    map3d_xy(xpos, ypos, z_min, &x, &y);
    clip_move(x,y);
    map3d_xy(xpos, ypos, z_max+(is_log_z ? incr : 0.0), &x, &y);
    clip_vector(x,y);

    setlinestyle(-1); /* border linetype */
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


    if(x_max3d> x_min3d){
    	if (place > x_max3d || place < x_min3d) return(0);
    }else{
    	if (place > x_min3d || place < x_max3d) return(0);
    }

    map3d_xy(place, ypos, z_min3d, &x0, &y0);
    /* need to figure out which is in. pick the middle point along the */
    /* axis as in.						       */
    map3d_xy(place, (y_max3d + y_min3d) / 2, z_min3d, &x1, &y1);

    /* compute a vector of length 1 into the grid: */
    v[0] = x1 - x0;
    v[1] = y1 - y0;
    len = sqrt(v[0] * v[0] + v[1] * v[1]);
    if (len == 0.0) return;
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

    (void) sprintf(ticlabel, text, CheckLog(is_log_x, base_log_x, place));
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

    if(y_max3d> y_min3d){
    	if (place > y_max3d || place < y_min3d) return(0);
    }else{
    	if (place > y_min3d || place < y_max3d) return(0);
    }

    map3d_xy(xpos, place, z_min3d, &x0, &y0);
    /* need to figure out which is in. pick the middle point along the */
    /* axis as in.						       */
    map3d_xy((x_max3d + x_min3d) / 2, place, z_min3d, &x1, &y1);

    /* compute a vector of length 1 into the grid: */
    v[0] = x1 - x0;
    v[1] = y1 - y0;
    len = sqrt(v[0] * v[0] + v[1] * v[1]);
    if (len == 0.0) return(0);
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

    (void) sprintf(ticlabel, text, CheckLog(is_log_y, base_log_y, place));
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

    (void) sprintf(ticlabel, text, CheckLog(is_log_z, base_log_z, place));
    if ((*t->justify_text)(RIGHT)) {
        clip_put_text(x3,y3,ticlabel);
    } else {
        clip_put_text(x3-(t->h_char)*(strlen(ticlabel)+1),y3,ticlabel);
    }
}
