#ifndef lint
static char *RCSid() { return RCSid("$Id: interpol.c,v 1.32 2007/12/18 19:02:58 sfeam Exp $"); }
#endif

/* GNUPLOT - interpol.c */

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
 * C-Source file identification Header
 *
 * This file belongs to a project which is:
 *
 * done 1993 by MGR-Software, Asgard  (Lars Hanke)
 * written by Lars Hanke
 *
 * Contact me via:
 *
 *  InterNet: mgr@asgard.bo.open.de
 *      FIDO: Lars Hanke @ 2:243/4802.22   (as long as they keep addresses)
 *
 **************************************************************************
 *
 *   Project: gnuplot
 *    Module:
 *      File: interpol.c
 *
 *   Revisor: Lars Hanke
 *   Revised: 26/09/93
 *  Revision: 1.0
 *
 **************************************************************************
 *
 * LEGAL
 *  This module is part of gnuplot and distributed under whatever terms
 *  gnuplot is or will be published, unless exclusive rights are claimed.
 *
 * DESCRIPTION
 *  Supplies 2-D data interpolation and approximation routines
 *
 * IMPORTS
 *  plot.h
 *    - cp_extend()
 *    - structs: curve_points, coordval, coordinate
 *
 *  setshow.h
 *    - samples, axis array[] variables
 *    - plottypes
 *
 *  proto.h
 *    - solve_tri_diag()
 *    - typedef tri_diag
 *
 * EXPORTS
 *  gen_interp()
 *  sort_points()
 *  cp_implode()
 *
 * BUGS and TODO
 *  I would really have liked to use Gershon Elbers contouring code for
 *  all the stuff done here, but I failed. So I used my own code.
 *  If somebody is able to consolidate Gershon's code for this purpose
 *  a lot of gnuplot users would be very happy - due to memory problems.
 *
 **************************************************************************
 *
 * HISTORY
 * Changes:
 *  Nov 24, 1995  Markus Schuh (M.Schuh@meteo.uni-koeln.de):
 *      changed the algorithm for csplines
 *      added algorithm for approximation csplines
 *      copied point storage and range fix from plot2d.c
 *
 *  Dec 12, 1995 David Denholm
 *      oops - at the time this is called, stored co-ords are
 *      internal (ie maybe log of data) but min/max are in
 *      user co-ordinates.
 *      Work with min and max of internal co-ords, and
 *      check at the end whether external min and max need to
 *      be increased. (since samples_1 is typically 100 ; we
 *      dont want to take more logs than necessary)
 *      Also, need to take into account which axes are active
 *
 *  Jun 30, 1996 Jens Emmerich
 *      implemented handling of UNDEFINED points
 */

#include "interpol.h"

#include "alloc.h"
#include "axis.h"
#include "contour.h"
#include "graphics.h"
#include "misc.h"
#include "plot2d.h"
/*  #include "setshow.h" */
#include "util.h"


/* in order to support multiple axes, and to simplify ranging in
 * parametric plots, we use arrays to store some things. For 2d plots,
 * elements are z=0,y1=1,x1=2,z2=4,y2=5,x2=6 these are given symbolic
 * names in plot.h
 */


/*
 * IMHO, code is getting too cluttered with repeated chunks of
 * code. Some macros to simplify, I hope.
 */


/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate Do
 * OUT_ACTION or UNDEF_ACTION as appropriate. Adjust range provided
 * type is INRANGE (ie dont adjust y if x is outrange). VALUE must not
 * be same as STORE */
/* FIXME 20010610: UNDEF_ACTION is completely unused ??? Furthermore,
 * this is so similar to STORE_WITH_LOG_AND_UPDATE_RANGE() from axis.h
 * that the two should probably be merged.  */
#define STORE_AND_FIXUP_RANGE(store, value, type, min, max, auto,	\
			      out_action, undef_action)			\
do {									\
    store=value;							\
    if (type != INRANGE)						\
	break;  /* don't set y range if x is outrange, for example */	\
    if ((value) < (min)) {						\
       if ((auto) & AUTOSCALE_MIN)					\
	   (min) = (value);						\
       else {								\
	   (type) = OUTRANGE;						\
	   out_action;							\
	   break;							\
       }								\
    }									\
    if ((value) > (max)) {						\
       if ((auto) & AUTOSCALE_MAX)					\
	   (max) = (value);						\
       else {								\
	   (type) = OUTRANGE;						\
	   out_action;							\
       }								\
    }									\
} while(0)

#define UPDATE_RANGE(TEST,OLD,NEW,AXIS)		\
do {						\
    if (TEST)					\
	(OLD) = AXIS_DE_LOG_VALUE(AXIS,NEW);	\
} while(0)

#define spline_coeff_size 4
typedef double spline_coeff[spline_coeff_size];
typedef double five_diag[5];

static int next_curve __PROTO((struct curve_points * plot, int *curve_start));
static int num_curves __PROTO((struct curve_points * plot));
static double *cp_binomial __PROTO((int points));
static void eval_bezier __PROTO((struct curve_points * cp, int first_point,
				 int num_points, double sr, coordval * px,
				 coordval *py, double *c));
static void do_bezier __PROTO((struct curve_points * cp, double *bc, int first_point, int num_points, struct coordinate * dest));
static int solve_five_diag __PROTO((five_diag m[], double r[], double x[], int n));
static spline_coeff *cp_approx_spline __PROTO((struct curve_points * plot, int first_point, int num_points));
static spline_coeff *cp_tridiag __PROTO((struct curve_points * plot, int first_point, int num_points));
static void do_cubic __PROTO((struct curve_points * plot, spline_coeff * sc, int first_point, int num_points, struct coordinate * dest));
static void do_freq __PROTO((struct curve_points *plot,	int first_point, int num_points));
int compare_points __PROTO((SORTFUNC_ARGS p1, SORTFUNC_ARGS p2));


/*
 * position curve_start to index the next non-UNDEFINDED point,
 * start search at initial curve_start,
 * return number of non-UNDEFINDED points from there on,
 * if no more valid points are found, curve_start is set
 * to plot->p_count and 0 is returned
 */

static int
next_curve(struct curve_points *plot, int *curve_start)
{
    int curve_length;

    /* Skip undefined points */
    while (*curve_start < plot->p_count
	   && plot->points[*curve_start].type == UNDEFINED) {
	(*curve_start)++;
    };
    curve_length = 0;
    /* curve_length is first used as an offset, then the correct # points */
    while ((*curve_start) + curve_length < plot->p_count
	   && plot->points[(*curve_start) + curve_length].type != UNDEFINED) {
	curve_length++;
    };
    return (curve_length);
}


/*
 * determine the number of curves in plot->points, separated by
 * UNDEFINED points
 */

static int
num_curves(struct curve_points *plot)
{
    int curves;
    int first_point;
    int num_points;

    first_point = 0;
    curves = 0;
    while ((num_points = next_curve(plot, &first_point)) > 0) {
	curves++;
	first_point += num_points;
    }
    return (curves);
}



/*
 * build up a cntr_struct list from curve_points
 * this funtion is only used for the alternate entry point to
 * Gershon's code and thus commented out
 ***deleted***
 */


/* HBB 990205: rewrote the 'bezier' interpolation routine,
 * to prevent numerical overflow and other undesirable things happening
 * for large data files (num_data about 1000 or so), where binomial
 * coefficients would explode, and powers of 'sr' (0 < sr < 1) become
 * extremely small. Method used: compute logarithms of these
 * extremely large and small numbers, and only go back to the
 * real numbers once they've cancelled out each other, leaving
 * a reasonable-sized one. */

/*
 * cp_binomial() computes the binomial coefficients needed for BEZIER stuff
 *   and stores them into an array which is hooked to sdat.
 * (MGR 1992)
 */
static double *
cp_binomial(int points)
{
    double *coeff;
    int n, k;
    int e;

    e = points;			/* well we're going from k=0 to k=p_count-1 */
    coeff = gp_alloc(e * sizeof(double), "bezier coefficients");

    n = points - 1;
    e = n / 2;
    /* HBB 990205: calculate these in 'logarithmic space',
     * as they become _very_ large, with growing n (4^n) */
    coeff[0] = 0.0;

    for (k = 0; k < e; k++) {
	coeff[k + 1] = coeff[k] + log(((double) (n - k)) / ((double) (k + 1)));
    }

    for (k = n; k >= e; k--)
	coeff[k] = coeff[n - k];

    return (coeff);
}


/* This is a subfunction of do_bezier() for BEZIER style computations.
 * It is passed the stepration (STEP/MAXSTEPS) and the addresses of
 * the double values holding the next x and y coordinates.
 * (MGR 1992)
 */

static void
eval_bezier(
    struct curve_points *cp,
    int first_point,		/* where to start in plot->points (to find x-range) */
    int num_points,		/* to determine end in plot->points */
    double sr,			/* position inside curve, range [0:1] */
    coordval *px,		/* OUTPUT: x and y */
    coordval *py,
    double *c)			/* Bezier coefficient array */
{
    unsigned int n = num_points - 1;
    struct coordinate GPHUGE *this_points;

    this_points = (cp->points) + first_point;

    if (sr == 0.0) {
	*px = this_points[0].x;
	*py = this_points[0].y;
    } else if (sr == 1.0) {
	*px = this_points[n].x;
	*py = this_points[n].y;
    } else {
	/* HBB 990205: do calculation in 'logarithmic space',
	 * to avoid over/underflow errors, which would exactly cancel
	 * out each other, anyway, in an exact calculation
	 */
	unsigned int i;
	double lx = 0.0, ly = 0.0;
	double log_dsr_to_the_n = n * log(1 - sr);
	double log_sr_over_dsr = log(sr) - log(1 - sr);

	for (i = 0; i <= n; i++) {
	    double u = exp(c[i] + log_dsr_to_the_n + i * log_sr_over_dsr);

	    lx += this_points[i].x * u;
	    ly += this_points[i].y * u;
	}

	*px = lx;
	*py = ly;
    }
}

/*
 * generate a new set of coordinates representing the bezier curve and
 * set it to the plot
 */

static void
do_bezier(
    struct curve_points *cp,
    double *bc,			/* Bezier coefficient array */
    int first_point,		/* where to start in plot->points */
    int num_points,		/* to determine end in plot->points */
    struct coordinate *dest)	/* where to put the interpolated data */
{
    int i;
    coordval x, y;

    /* min and max in internal (eg logged) co-ordinates. We update
     * these, then update the external extrema in user co-ordinates
     * at the end.
     */

    double ixmin, ixmax, iymin, iymax;
    double sxmin, sxmax, symin, symax;	/* starting values of above */

    x_axis = cp->x_axis;
    y_axis = cp->y_axis;

    ixmin = sxmin = AXIS_LOG_VALUE(x_axis, X_AXIS.min);
    ixmax = sxmax = AXIS_LOG_VALUE(x_axis, X_AXIS.max);
    iymin = symin = AXIS_LOG_VALUE(y_axis, Y_AXIS.min);
    iymax = symax = AXIS_LOG_VALUE(y_axis, Y_AXIS.max);

    for (i = 0; i < samples_1; i++) {
	eval_bezier(cp, first_point, num_points,
		    (double) i / (double) (samples_1 - 1),
		    &x, &y, bc);

	/* now we have to store the points and adjust the ranges */

	dest[i].type = INRANGE;
	STORE_AND_FIXUP_RANGE(dest[i].x, x, dest[i].type, ixmin, ixmax, X_AXIS.autoscale, NOOP, continue);
	STORE_AND_FIXUP_RANGE(dest[i].y, y, dest[i].type, iymin, iymax, Y_AXIS.autoscale, NOOP, NOOP);

	dest[i].xlow = dest[i].xhigh = dest[i].x;
	dest[i].ylow = dest[i].yhigh = dest[i].y;

	dest[i].z = -1;
    }

    UPDATE_RANGE(ixmax > sxmax, X_AXIS.max, ixmax, x_axis);
    UPDATE_RANGE(ixmin < sxmin, X_AXIS.min, ixmin, x_axis);
    UPDATE_RANGE(iymax > symax, Y_AXIS.max, iymax, y_axis);
    UPDATE_RANGE(iymin < symin, Y_AXIS.min, iymin, y_axis);
}

/*
 * call contouring routines -- main entry
 */

/*
 * it should be like this, but it doesn't run. If you find out why,
 * contact me: mgr@asgard.bo.open.de or Lars Hanke 2:243/4802.22@fidonet
 *
 * Well, all this had originally been inside contour.c, so maybe links
 * to functions and of contour.c are broken.
 * ***deleted***
 * end of unused entry point to Gershon's code
 *
 */

/*
 * Solve five diagonal linear system equation. The five diagonal matrix is
 * defined via matrix M, right side is r, and solution X i.e. M * X = R.
 * Size of system given in n. Return TRUE if solution exist.
 *  G. Engeln-Muellges/ F.Reutter:
 *  "Formelsammlung zur Numerischen Mathematik mit Standard-FORTRAN-Programmen"
 *  ISBN 3-411-01677-9
 *
 * /  m02 m03 m04   0   0   0   0    .       .       . \   /  x0  \    / r0  \
 * I  m11 m12 m13 m14   0   0   0    .       .       . I   I  x1  I   I  r1  I
 * I  m20 m21 m22 m23 m24   0   0    .       .       . I * I  x2  I = I  r2  I
 * I    0 m30 m31 m32 m33 m34   0    .       .       . I   I  x3  I   I  r3  I
 *      .   .   .   .   .   .   .    .       .       .        .        .
 * \                           m(n-3)0 m(n-2)1 m(n-1)2 /   \x(n-1)/   \r(n-1)/
 *
 */
static int
solve_five_diag(five_diag m[], double r[], double x[], int n)
{
    int i;
    five_diag *hv;

    hv = gp_alloc((n + 1) * sizeof(five_diag), "five_diag help vars");

    hv[0][0] = m[0][2];
    if (hv[0][0] == 0) {
	free(hv);
	return FALSE;
    }
    hv[0][1] = m[0][3] / hv[0][0];
    hv[0][2] = m[0][4] / hv[0][0];

    hv[1][3] = m[1][1];
    hv[1][0] = m[1][2] - hv[1][3] * hv[0][1];
    if (hv[1][0] == 0) {
	free(hv);
	return FALSE;
    }
    hv[1][1] = (m[1][3] - hv[1][3] * hv[0][2]) / hv[1][0];
    hv[1][2] = m[1][4] / hv[1][0];

    for (i = 2; i < n; i++) {
	hv[i][3] = m[i][1] - m[i][0] * hv[i - 2][1];
	hv[i][0] = m[i][2] - m[i][0] * hv[i - 2][2] - hv[i][3] * hv[i - 1][1];
	if (hv[i][0] == 0) {
	    free(hv);
	    return FALSE;
	}
	hv[i][1] = (m[i][3] - hv[i][3] * hv[i - 1][2]) / hv[i][0];
	hv[i][2] = m[i][4] / hv[i][0];
    }

    hv[0][4] = 0;
    hv[1][4] = r[0] / hv[0][0];
    for (i = 1; i < n; i++) {
	hv[i + 1][4] = (r[i] - m[i][0] * hv[i - 1][4] - hv[i][3] * hv[i][4]) / hv[i][0];
    }

    x[n - 1] = hv[n][4];
    x[n - 2] = hv[n - 1][4] - hv[n - 2][1] * x[n - 1];
    for (i = n - 3; i >= 0; i--)
	x[i] = hv[i + 1][4] - hv[i][1] * x[i + 1] - hv[i][2] * x[i + 2];

    free(hv);
    return TRUE;
}


/*
 * Calculation of approximation cubic splines
 * Input:  x[i], y[i], weights z[i]
 *
 * Returns matrix of spline coefficients
 */
static spline_coeff *
cp_approx_spline(
    struct curve_points *plot,
    int first_point,		/* where to start in plot->points */
    int num_points)		/* to determine end in plot->points */
{
    spline_coeff *sc;
    five_diag *m;
    double *r, *x, *h, *xp, *yp;
    struct coordinate GPHUGE *this_points;
    int i;

    x_axis = plot->x_axis;
    y_axis = plot->y_axis;

    sc = gp_alloc((num_points) * sizeof(spline_coeff),
				   "spline matrix");

    if (num_points < 4)
	int_error(plot->token, "Can't calculate approximation splines, need at least 4 points");

    this_points = (plot->points) + first_point;

    for (i = 0; i < num_points; i++)
	if (this_points[i].z <= 0)
	    int_error(plot->token, "Can't calculate approximation splines, all weights have to be > 0");

    m = gp_alloc((num_points - 2) * sizeof(five_diag), "spline help matrix");

    r = gp_alloc((num_points - 2) * sizeof(double), "spline right side");
    x = gp_alloc((num_points - 2) * sizeof(double), "spline solution vector");
    h = gp_alloc((num_points - 1) * sizeof(double), "spline help vector");

    xp = gp_alloc((num_points) * sizeof(double), "x pos");
    yp = gp_alloc((num_points) * sizeof(double), "y pos");

    /* KB 981107: With logarithmic axis first convert back to linear scale */

    xp[0] = AXIS_DE_LOG_VALUE(x_axis, this_points[0].x);
    yp[0] = AXIS_DE_LOG_VALUE(y_axis, this_points[0].y);
    for (i = 1; i < num_points; i++) {
	xp[i] = AXIS_DE_LOG_VALUE(x_axis, this_points[i].x);
	yp[i] = AXIS_DE_LOG_VALUE(y_axis, this_points[i].y);
	h[i - 1] = xp[i] - xp[i - 1];
    }

    /* set up the matrix and the vector */

    for (i = 0; i <= num_points - 3; i++) {
	r[i] = 3 * ((yp[i + 2] - yp[i + 1]) / h[i + 1]
		    - (yp[i + 1] - yp[i]) / h[i]);

	if (i < 2)
	    m[i][0] = 0;
	else
	    m[i][0] = 6 / this_points[i].z / h[i - 1] / h[i];

	if (i < 1)
	    m[i][1] = 0;
	else
	    m[i][1] = h[i] - 6 / this_points[i].z / h[i] * (1 / h[i - 1] + 1 / h[i])
		- 6 / this_points[i + 1].z / h[i] * (1 / h[i] + 1 / h[i + 1]);

	m[i][2] = 2 * (h[i] + h[i + 1])
	    + 6 / this_points[i].z / h[i] / h[i]
	    + 6 / this_points[i + 1].z * (1 / h[i] + 1 / h[i + 1]) * (1 / h[i] + 1 / h[i + 1])
	    + 6 / this_points[i + 2].z / h[i + 1] / h[i + 1];

	if (i > num_points - 4)
	    m[i][3] = 0;
	else
	    m[i][3] = h[i + 1] - 6 / this_points[i + 1].z / h[i + 1] * (1 / h[i] + 1 / h[i + 1])
		- 6 / this_points[i + 2].z / h[i + 1] * (1 / h[i + 1] + 1 / h[i + 2]);

	if (i > num_points - 5)
	    m[i][4] = 0;
	else
	    m[i][4] = 6 / this_points[i + 2].z / h[i + 1] / h[i + 2];
    }

    /* solve the matrix */
    if (!solve_five_diag(m, r, x, num_points - 2)) {
	free(h);
	free(x);
	free(r);
	free(m);
	free(xp);
	free(yp);
	int_error(plot->token, "Can't calculate approximation splines");
    }
    sc[0][2] = 0;
    for (i = 1; i <= num_points - 2; i++)
	sc[i][2] = x[i - 1];
    sc[num_points - 1][2] = 0;

    sc[0][0] = yp[0] + 2 / this_points[0].z / h[0] * (sc[0][2] - sc[1][2]);
    for (i = 1; i <= num_points - 2; i++)
	sc[i][0] = yp[i] - 2 / this_points[i].z *
	    (sc[i - 1][2] / h[i - 1]
	     - sc[i][2] * (1 / h[i - 1] + 1 / h[i])
	     + sc[i + 1][2] / h[i]);
    sc[num_points - 1][0] = yp[num_points - 1]
	- 2 / this_points[num_points - 1].z / h[num_points - 2]
	* (sc[num_points - 2][2] - sc[num_points - 1][2]);

    for (i = 0; i <= num_points - 2; i++) {
	sc[i][1] = (sc[i + 1][0] - sc[i][0]) / h[i]
	    - h[i] / 3 * (sc[i + 1][2] + 2 * sc[i][2]);
	sc[i][3] = (sc[i + 1][2] - sc[i][2]) / 3 / h[i];
    }

    free(h);
    free(x);
    free(r);
    free(m);
    free(xp);
    free(yp);

    return (sc);
}


/*
 * Calculation of cubic splines
 *
 * This can be treated as a special case of approximation cubic splines, with
 * all weights -> infinity.
 *
 * Returns matrix of spline coefficients
 */
static spline_coeff *
cp_tridiag(struct curve_points *plot, int first_point, int num_points)
{
    spline_coeff *sc;
    tri_diag *m;
    double *r, *x, *h, *xp, *yp;
    struct coordinate GPHUGE *this_points;
    int i;

    x_axis = plot->x_axis;
    y_axis = plot->y_axis;
    if (num_points < 3)
	int_error(plot->token, "Can't calculate splines, need at least 3 points");

    this_points = (plot->points) + first_point;

    sc = gp_alloc((num_points) * sizeof(spline_coeff), "spline matrix");
    m = gp_alloc((num_points - 2) * sizeof(tri_diag), "spline help matrix");

    r = gp_alloc((num_points - 2) * sizeof(double), "spline right side");
    x = gp_alloc((num_points - 2) * sizeof(double), "spline solution vector");
    h = gp_alloc((num_points - 1) * sizeof(double), "spline help vector");

    xp = gp_alloc((num_points) * sizeof(double), "x pos");
    yp = gp_alloc((num_points) * sizeof(double), "y pos");

    /* KB 981107: With logarithmic axis first convert back to linear scale */

    xp[0] = AXIS_DE_LOG_VALUE(x_axis,this_points[0].x);
    yp[0] = AXIS_DE_LOG_VALUE(y_axis,this_points[0].y);
    for (i = 1; i < num_points; i++) {
	xp[i] = AXIS_DE_LOG_VALUE(x_axis,this_points[i].x);
	yp[i] = AXIS_DE_LOG_VALUE(y_axis,this_points[i].y);
	h[i - 1] = xp[i] - xp[i - 1];
    }

    /* set up the matrix and the vector */

    for (i = 0; i <= num_points - 3; i++) {
	r[i] = 3 * ((yp[i + 2] - yp[i + 1]) / h[i + 1]
		    - (yp[i + 1] - yp[i]) / h[i]);

	if (i < 1)
	    m[i][0] = 0;
	else
	    m[i][0] = h[i];

	m[i][1] = 2 * (h[i] + h[i + 1]);

	if (i > num_points - 4)
	    m[i][2] = 0;
	else
	    m[i][2] = h[i + 1];
    }

    /* solve the matrix */
    if (!solve_tri_diag(m, r, x, num_points - 2)) {
	free(h);
	free(x);
	free(r);
	free(m);
	free(xp);
	free(yp);
	int_error(plot->token, "Can't calculate cubic splines");
    }
    sc[0][2] = 0;
    for (i = 1; i <= num_points - 2; i++)
	sc[i][2] = x[i - 1];
    sc[num_points - 1][2] = 0;

    for (i = 0; i <= num_points - 1; i++)
	sc[i][0] = yp[i];

    for (i = 0; i <= num_points - 2; i++) {
	sc[i][1] = (sc[i + 1][0] - sc[i][0]) / h[i]
	    - h[i] / 3 * (sc[i + 1][2] + 2 * sc[i][2]);
	sc[i][3] = (sc[i + 1][2] - sc[i][2]) / 3 / h[i];
    }

    free(h);
    free(x);
    free(r);
    free(m);
    free(xp);
    free(yp);

    return (sc);
}

static void
do_cubic(
    struct curve_points *plot,	/* still containes old plot->points */
    spline_coeff *sc,		/* generated by cp_tridiag */
    int first_point,		/* where to start in plot->points */
    int num_points,		/* to determine end in plot->points */
    struct coordinate *dest)	/* where to put the interpolated data */
{
    double xdiff, temp, x, y;
    double xstart, xend;	/* Endpoints of the sampled x range */
    int i, l;
    struct coordinate GPHUGE *this_points;

    /* min and max in internal (eg logged) co-ordinates. We update
     * these, then update the external extrema in user co-ordinates
     * at the end.
     */
    double ixmin, ixmax, iymin, iymax;
    double sxmin, sxmax, symin, symax;	/* starting values of above */

    x_axis = plot->x_axis;
    y_axis = plot->y_axis;

    ixmin = sxmin = AXIS_LOG_VALUE(x_axis, X_AXIS.min);
    ixmax = sxmax = AXIS_LOG_VALUE(x_axis, X_AXIS.max);
    iymin = symin = AXIS_LOG_VALUE(y_axis, Y_AXIS.min);
    iymax = symax = AXIS_LOG_VALUE(y_axis, Y_AXIS.max);

    this_points = (plot->points) + first_point;

    l = 0;

    /* HBB 20010727: Sample only across the actual x range, not the
     * full range of input data */
#if SAMPLE_CSPLINES_TO_FULL_RANGE
    xstart = this_points[0].x;
    xend = this_points[num_points - 1].x;
#else
    xstart = GPMAX(this_points[0].x, sxmin);
    xend = GPMIN(this_points[num_points - 1].x, sxmax);

    if (xstart >= xend)
	int_error(plot->token,
		  "Cannot smooth: no data within fixed xrange!");
#endif
    xdiff = (xend - xstart) / (samples_1 - 1);

    for (i = 0; i < samples_1; i++) {
	x = xstart + i * xdiff;

	/* Move forward to the spline interval this point is in */
	while ((x >= this_points[l + 1].x) && (l < num_points - 2))
	    l++;

	/* KB 981107: With logarithmic x axis the values were
         * converted back to linear scale before calculating the
         * coefficients. Use exponential for log x values. */
	temp = AXIS_DE_LOG_VALUE(x_axis, x)
	    - AXIS_DE_LOG_VALUE(x_axis, this_points[l].x);

	/* Evaluate cubic spline polynomial */
	y = ((sc[l][3] * temp + sc[l][2]) * temp + sc[l][1]) * temp + sc[l][0];

	/* With logarithmic y axis, we need to convert from linear to
         * log scale now. */
	if (Y_AXIS.log) {
	    if (y > 0.)
		y = AXIS_DO_LOG(y_axis, y);
	    else
		y = symin - (symax - symin);
	}

	dest[i].type = INRANGE;
	STORE_AND_FIXUP_RANGE(dest[i].x, x, dest[i].type, ixmin, ixmax, X_AXIS.autoscale, NOOP, continue);
	STORE_AND_FIXUP_RANGE(dest[i].y, y, dest[i].type, iymin, iymax, Y_AXIS.autoscale, NOOP, NOOP);

	dest[i].xlow = dest[i].xhigh = dest[i].x;
	dest[i].ylow = dest[i].yhigh = dest[i].y;

	dest[i].z = -1;

    }

    UPDATE_RANGE(ixmax > sxmax, X_AXIS.max, ixmax, x_axis);
    UPDATE_RANGE(ixmin < sxmin, X_AXIS.min, ixmin, x_axis);
    UPDATE_RANGE(iymax > symax, Y_AXIS.max, iymax, y_axis);
    UPDATE_RANGE(iymin < symin, Y_AXIS.min, iymin, y_axis);

}


/*
 * do_freq() is like the other smoothers only in that it
 * needs to adjust the plot ranges. We don't have to copy
 * approximated curves or anything like that.
 */

static void
do_freq(
    struct curve_points *plot,	/* still contains old plot->points */
    int first_point,		/* where to start in plot->points */
    int num_points)		/* to determine end in plot->points */
{
    double x, y;
    int i;
    int x_axis = plot->x_axis;
    int y_axis = plot->y_axis;
    struct coordinate GPHUGE *this;

    /* min and max in internal (eg logged) co-ordinates. We update
     * these, then update the external extrema in user co-ordinates
     * at the end.
     */

    double ixmin, ixmax, iymin, iymax;
    double sxmin, sxmax, symin, symax;	/* starting values of above */

    ixmin = sxmin = AXIS_LOG_VALUE(x_axis, X_AXIS.min);
    ixmax = sxmax = AXIS_LOG_VALUE(x_axis, X_AXIS.max);
    iymin = symin = AXIS_LOG_VALUE(y_axis, Y_AXIS.min);
    iymax = symax = AXIS_LOG_VALUE(y_axis, Y_AXIS.max);

    this = (plot->points) + first_point;

    for (i=0; i<num_points; i++) {

	x = this[i].x;
	y = this[i].y;

	this[i].type = INRANGE;

	STORE_AND_FIXUP_RANGE(this[i].x, x, this[i].type, ixmin, ixmax, X_AXIS.autoscale, NOOP, continue);
	STORE_AND_FIXUP_RANGE(this[i].y, y, this[i].type, iymin, iymax, Y_AXIS.autoscale, NOOP, NOOP);

	this[i].xlow = this[i].xhigh = this[i].x;
	this[i].ylow = this[i].yhigh = this[i].y;
	this[i].z = -1;
    }

    UPDATE_RANGE(ixmax > sxmax, X_AXIS.max, ixmax, x_axis);
    UPDATE_RANGE(ixmin < sxmin, X_AXIS.min, ixmin, x_axis);
    UPDATE_RANGE(iymax > symax, Y_AXIS.max, iymax, y_axis);
    UPDATE_RANGE(iymin < symin, Y_AXIS.min, iymin, y_axis);
}


/*
 * Frequency plots have don't need new points allocated; we just need
 * to adjust the plot ranges. Wedging this into gen_interp() would
 * make that code even harder to read.
 */

void
gen_interp_frequency(struct curve_points *plot)
{
    int i, j, curves;
    int first_point, num_points;
    double y;

    curves = num_curves(plot);

    first_point = 0;
    for (i = 0; i < curves; i++) {
        num_points = next_curve(plot, &first_point);

        /* If cumulative, replace the current y-value with the
           sum of all previous y-values. This assumes that the
           data has already been sorted by x-values. */
        if( plot->plot_smooth == SMOOTH_CUMULATIVE ) {
            y = 0;
            for (j = first_point; j < first_point + num_points; j++) {
                if (plot->points[j].type == UNDEFINED) 
                    continue;

                y += plot->points[j].y;
                plot->points[j].y = y;
            }
        }

        do_freq(plot, first_point, num_points);
        first_point += num_points + 1;
    }
    return;
}

/*
 * This is the main entry point used for everything except frequencies.
 * As stated in the header, it is fine, but I'm not too happy with it.
 */

void
gen_interp(struct curve_points *plot)
{

    spline_coeff *sc;
    double *bc;
    struct coordinate *new_points;
    int i, curves;
    int first_point, num_points;

    curves = num_curves(plot);
    new_points = gp_alloc((samples_1 + 1) * curves * sizeof(struct coordinate),
			  "interpolation table");

    first_point = 0;
    for (i = 0; i < curves; i++) {
	num_points = next_curve(plot, &first_point);
	switch (plot->plot_smooth) {
	case SMOOTH_CSPLINES:
	    sc = cp_tridiag(plot, first_point, num_points);
	    do_cubic(plot, sc, first_point, num_points,
		     new_points + i * (samples_1 + 1));
	    free(sc);
	    break;
	case SMOOTH_ACSPLINES:
	    sc = cp_approx_spline(plot, first_point, num_points);
	    do_cubic(plot, sc, first_point, num_points,
		     new_points + i * (samples_1 + 1));
	    free(sc);
	    break;

	case SMOOTH_BEZIER:
	case SMOOTH_SBEZIER:
	    bc = cp_binomial(num_points);
	    do_bezier(plot, bc, first_point, num_points,
		      new_points + i * (samples_1 + 1));
	    free((char *) bc);
	    break;
	default:		/* keep gcc -Wall quiet */
	    ;
	}
	new_points[(i + 1) * (samples_1 + 1) - 1].type = UNDEFINED;
	first_point += num_points;
    }

    free(plot->points);
    plot->points = new_points;
    plot->p_max = curves * (samples_1 + 1);
    plot->p_count = plot->p_max - 1;

    return;
}

/*
 * sort_points
 *
 * sort data succession for further evaluation by plot_splines, etc.
 * This routine is mainly introduced for compilers *NOT* supporting the
 * UNIX qsort() routine. You can then easily replace it by more convenient
 * stuff for your compiler.
 * (MGR 1992)
 */

/* HBB 20010720: To avoid undefined behaviour that would be caused by
 * casting functions pointers around, changed arguments to what
 * qsort() *really* wants */
/* HBB 20010720: removed 'static' to avoid HP-sUX gcc bug */
int
compare_points(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
    struct coordinate const *p1 = arg1;
    struct coordinate const *p2 = arg2;

    if (p1->x > p2->x)
	return (1);
    if (p1->x < p2->x)
	return (-1);
    return (0);
}

void
sort_points(struct curve_points *plot)
{
    int first_point, num_points;

    first_point = 0;
    while ((num_points = next_curve(plot, &first_point)) > 0) {
	/* Sort this set of points, does qsort handle 1 point correctly? */
	/* HBB 20010720: removed casts -- they don't help a thing, but
	 * may hide problems */
	qsort(plot->points + first_point, num_points,
	      sizeof(struct coordinate), compare_points);
	first_point += num_points;
    }
    return;
}


/*
 * cp_implode() if averaging is selected this function computes the new
 *              entries and shortens the whole thing to the necessary
 *              size
 * MGR Addendum
 */

void
cp_implode(struct curve_points *cp)
{
    int first_point, num_points;
    int i, j, k;
    double x = 0., y = 0., sux = 0., slx = 0., suy = 0., sly = 0.;
    /* int x_axis = cp->x_axis; */
    /* int y_axis = cp->y_axis; */
    TBOOLEAN all_inrange = FALSE;

    x_axis = cp->x_axis;
    y_axis = cp->y_axis;
    j = 0;
    first_point = 0;
    while ((num_points = next_curve(cp, &first_point)) > 0) {
	k = 0;
	for (i = first_point; i < first_point + num_points; i++) {
	    /* HBB 20020801: don't try to use undefined datapoints */
	    if (cp->points[i].type == UNDEFINED)
	        continue;
	    if (!k) {
		x = cp->points[i].x;
		y = cp->points[i].y;
		sux = cp->points[i].xhigh;
		slx = cp->points[i].xlow;
		suy = cp->points[i].yhigh;
		sly = cp->points[i].ylow;
		all_inrange = (cp->points[i].type == INRANGE);
		k = 1;
	    } else if (cp->points[i].x == x) {
		y += cp->points[i].y;
		sux += cp->points[i].xhigh;
		slx += cp->points[i].xlow;
		suy += cp->points[i].yhigh;
		sly += cp->points[i].ylow;
		if (cp->points[i].type != INRANGE)
		    all_inrange = FALSE;
		k++;
	    } else {
		cp->points[j].x = x;
 		if ( cp->plot_smooth == SMOOTH_FREQUENCY ||
 		     cp->plot_smooth == SMOOTH_CUMULATIVE )
		    k = 1;
		cp->points[j].y = y /= (double) k;
		cp->points[j].xhigh = sux / (double) k;
		cp->points[j].xlow = slx / (double) k;
		cp->points[j].yhigh = suy / (double) k;
		cp->points[j].ylow = sly / (double) k;
		/* HBB 20000405: I wanted to use STORE_AND_FIXUP_RANGE
		 * here, but won't: it assumes we want to modify the
		 * range, and that the range is given in 'input'
		 * coordinates.  For logarithmic axes, the overhead
		 * would be larger than the possible gain, so write it
		 * out explicitly, instead:
		 * */
		cp->points[j].type = INRANGE;
		if (! all_inrange) {
		    if (X_AXIS.log) {
			if (x <= -VERYLARGE) {
			    cp->points[j].type = OUTRANGE;
			    goto is_outrange;
			}
			x = AXIS_UNDO_LOG(x_axis, x);
		    }
		    if (((x < X_AXIS.min) && !(X_AXIS.autoscale & AUTOSCALE_MIN))
			|| ((x > X_AXIS.max) && !(X_AXIS.autoscale & AUTOSCALE_MAX))) {
			cp->points[j].type = OUTRANGE;
			goto is_outrange;
		    }
		    if (Y_AXIS.log) {
			if (y <= -VERYLARGE) {
			    cp->points[j].type = OUTRANGE;
			    goto is_outrange;
			}
			y = AXIS_UNDO_LOG(y_axis, y);
		    }
		    if (((y < Y_AXIS.min) && !(Y_AXIS.autoscale & AUTOSCALE_MIN))
			|| ((y > Y_AXIS.max) && !(Y_AXIS.autoscale & AUTOSCALE_MAX)))
			cp->points[j].type = OUTRANGE;
		is_outrange:
		    ;
		} /* if(! all inrange) */

		j++;		/* next valid entry */
		k = 0;		/* to read */
		i--;		/* from this (-> last after for(;;)) entry */
	    } /* else (same x position) */
	} /* for(points in curve) */

	if (k) {
	    cp->points[j].x = x;
	    if ( cp->plot_smooth == SMOOTH_FREQUENCY ||
		 cp->plot_smooth == SMOOTH_CUMULATIVE )
		k = 1;
	    cp->points[j].y = y /= (double) k;
	    cp->points[j].xhigh = sux / (double) k;
	    cp->points[j].xlow = slx / (double) k;
	    cp->points[j].yhigh = suy / (double) k;
	    cp->points[j].ylow = sly / (double) k;
	    cp->points[j].type = INRANGE;
	    if (! all_inrange) {
		    if (X_AXIS.log) {
			if (x <= -VERYLARGE) {
			    cp->points[j].type = OUTRANGE;
			    goto is_outrange2;
			}
			x = AXIS_UNDO_LOG(x_axis, x);
		    }
		    if (((x < X_AXIS.min) && !(X_AXIS.autoscale & AUTOSCALE_MIN))
			|| ((x > X_AXIS.max) && !(X_AXIS.autoscale & AUTOSCALE_MAX))) {
			cp->points[j].type = OUTRANGE;
			goto is_outrange2;
		    }
		    if (Y_AXIS.log) {
			if (y <= -VERYLARGE) {
			    cp->points[j].type = OUTRANGE;
			    goto is_outrange2;
			}
			y = AXIS_UNDO_LOG(y_axis, y);
		    }
		    if (((y < Y_AXIS.min) && !(Y_AXIS.autoscale & AUTOSCALE_MIN))
			|| ((y > Y_AXIS.max) && !(Y_AXIS.autoscale & AUTOSCALE_MAX)))
			cp->points[j].type = OUTRANGE;
		is_outrange2:
		    ;
	    }
	    j++;		/* next valid entry */
	}
	/* insert invalid point to separate curves */
	if (j < cp->p_count) {
	    cp->points[j].type = UNDEFINED;
	    j++;
	}
	first_point += num_points;
    }				/* end while */
    cp->p_count = j;
    cp_extend(cp, j);
}
