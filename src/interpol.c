#ifndef lint
static char *RCSid() { return RCSid("$Id: interpol.c,v 1.13.2.1 2000/05/03 21:26:11 joze Exp $"); }
#endif

/* GNUPLOT - interpol.c */

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
 *    - samples, is_log_x, base_log_x, xmin, xmax, autoscale_lx
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
 *      be increased. (since samples is typically 100 ; we
 *      dont want to take more logs than necessary)
 *      Also, need to take into account which axes are active
 *
 *  Jun 30, 1996 Jens Emmerich
 *      implemented handling of UNDEFINED points
 */

#include "interpol.h"

#include "alloc.h"
#include "contour.h"
#include "graphics.h"
#include "misc.h"
#include "setshow.h"
#include "util.h"


/* in order to support multiple axes, and to simplify ranging in
 * parametric plots, we use arrays to store some things. For 2d plots,
 * elements are z=0,y1=1,x1=2,z2=4,y2=5,x2=6 these are given symbolic
 * names in plot.h
 */


/*
 * IMHO, code is getting too cluttered with repeated chunks of
 * code. Some macros to simplify, I hope.
 *
 * do { } while(0) is comp.lang.c recommendation for complex macros
 * also means that break can be specified as an action, and it will
 * 
 */


/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate
 * Do OUT_ACTION or UNDEF_ACTION as appropriate
 * adjust range provided type is INRANGE (ie dont adjust y if x is outrange
 * VALUE must not be same as STORE
 */

#define STORE_AND_FIXUP_RANGE(STORE, VALUE, TYPE, MIN, MAX, AUTO, OUT_ACTION, UNDEF_ACTION)\
do { STORE=VALUE; \
    if (TYPE != INRANGE) break;  /* dont set y range if x is outrange, for example */ \
    if ( VALUE<MIN ) { \
       if (AUTO & 1) MIN=VALUE; else { TYPE=OUTRANGE; OUT_ACTION; break; }  \
    } \
    if ( VALUE>MAX ) {\
       if (AUTO & 2) MAX=VALUE; else { TYPE=OUTRANGE; OUT_ACTION; }   \
    } \
} while(0)

#define UPDATE_RANGE(TEST,OLD,NEW,AXIS) \
do { if (TEST) { \
     if (log_array[AXIS]) OLD = pow(base_array[AXIS], NEW); else OLD = NEW; \
     } \
} while(0)

/* use this instead empty macro arguments to work around NeXT cpp bug */
/* if this fails on any system, we might use ((void)0) */

#define NOOP			/* */

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
static int compare_points __PROTO((struct coordinate * p1, struct coordinate * p2));


/*
 * position curve_start to index the next non-UNDEFINDED point,
 * start search at initial curve_start,
 * return number of non-UNDEFINDED points from there on,
 * if no more valid points are found, curve_start is set
 * to plot->p_count and 0 is returned
 */

static int
next_curve(plot, curve_start)
struct curve_points *plot;
int *curve_start;
{
    int curve_length;

    /* Skip undefined points */
    while (*curve_start < plot->p_count
	   && plot->points[*curve_start].type == UNDEFINED) {
	(*curve_start)++;
    };
    curve_length = 0;
    /* curve_length is first used as an offset, then the correkt # points */
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
num_curves(plot)
struct curve_points *plot;
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
cp_binomial(points)
int points;
{
    register double *coeff;
    register int n, k;
    int e;

    e = points;			/* well we're going from k=0 to k=p_count-1 */
    coeff = (double *) gp_alloc(e * sizeof(double), "bezier coefficients");

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
eval_bezier(cp, first_point, num_points, sr, px, py, c)
struct curve_points *cp;
int first_point;		/* where to start in plot->points (to find x-range) */
int num_points;			/* to determine end in plot->points */
double sr;
coordval *px;
coordval *py;
double *c;
{
    unsigned int n = num_points - 1;
    /* HBB 980308: added 'GPHUGE' tag for DOS */
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
do_bezier(cp, bc, first_point, num_points, dest)
struct curve_points *cp;
double *bc;
int first_point;		/* where to start in plot->points */
int num_points;			/* to determine end in plot->points */
struct coordinate *dest;	/* where to put the interpolated data */
{
    int i;
    coordval x, y;
    int xaxis = cp->x_axis;
    int yaxis = cp->y_axis;

    /* min and max in internal (eg logged) co-ordinates. We update
     * these, then update the external extrema in user co-ordinates
     * at the end.
     */

    double ixmin, ixmax, iymin, iymax;
    double sxmin, sxmax, symin, symax;	/* starting values of above */

    if (log_array[xaxis]) {
	ixmin = sxmin = log(min_array[xaxis]) / log_base_array[xaxis];
	ixmax = sxmax = log(max_array[xaxis]) / log_base_array[xaxis];
    } else {
	ixmin = sxmin = min_array[xaxis];
	ixmax = sxmax = max_array[xaxis];
    }

    if (log_array[yaxis]) {
	iymin = symin = log(min_array[yaxis]) / log_base_array[yaxis];
	iymax = symax = log(max_array[yaxis]) / log_base_array[yaxis];
    } else {
	iymin = symin = min_array[yaxis];
	iymax = symax = max_array[yaxis];
    }

    for (i = 0; i < samples; i++) {
	eval_bezier(cp, first_point, num_points, (double) i / (double) (samples - 1), &x, &y, bc);

	/* now we have to store the points and adjust the ranges */

	dest[i].type = INRANGE;
	STORE_AND_FIXUP_RANGE(dest[i].x, x, dest[i].type, ixmin, ixmax, auto_array[xaxis], NOOP, continue);
	STORE_AND_FIXUP_RANGE(dest[i].y, y, dest[i].type, iymin, iymax, auto_array[yaxis], NOOP, NOOP);

	dest[i].xlow = dest[i].xhigh = dest[i].x;
	dest[i].ylow = dest[i].yhigh = dest[i].y;

	dest[i].z = -1;
    }

    UPDATE_RANGE(ixmax > sxmax, max_array[xaxis], ixmax, xaxis);
    UPDATE_RANGE(ixmin < sxmin, min_array[xaxis], ixmin, xaxis);
    UPDATE_RANGE(iymax > symax, max_array[yaxis], iymax, yaxis);
    UPDATE_RANGE(iymin < symin, min_array[yaxis], iymin, yaxis);
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
solve_five_diag(m, r, x, n)
five_diag m[];
double r[], x[];
int n;
{
    int i;
    five_diag *hv;

    hv = (five_diag *) gp_alloc((n + 1) * sizeof(five_diag), "five_diag help vars");

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

    for (i = 2; i <= n - 1; i++) {
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
    for (i = 1; i <= n - 1; i++) {
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
cp_approx_spline(plot, first_point, num_points)
struct curve_points *plot;
int first_point;		/* where to start in plot->points */
int num_points;			/* to determine end in plot->points */
{
    spline_coeff *sc;
    five_diag *m;
    int xaxis = plot->x_axis;
    int yaxis = plot->y_axis;
    double *r, *x, *h, *xp, *yp;
    /* HBB 980308: added 'GPHUGE' tag */
    struct coordinate GPHUGE *this_points;
    int i;

    sc = (spline_coeff *) gp_alloc((num_points) * sizeof(spline_coeff),
				   "spline matrix");

    if (num_points < 4)
	int_error(NO_CARET, "Can't calculate approximation splines, need at least 4 points");

    this_points = (plot->points) + first_point;

    for (i = 0; i <= num_points - 1; i++)
	if (this_points[i].z <= 0)
	    int_error(NO_CARET, "Can't calculate approximation splines, all weights have to be > 0");

    m = (five_diag *) gp_alloc((num_points - 2) * sizeof(five_diag), "spline help matrix");

    r = (double *) gp_alloc((num_points - 2) * sizeof(double), "spline right side");
    x = (double *) gp_alloc((num_points - 2) * sizeof(double), "spline solution vector");
    h = (double *) gp_alloc((num_points - 1) * sizeof(double), "spline help vector");

    xp = (double *) gp_alloc((num_points) * sizeof(double), "x pos");
    yp = (double *) gp_alloc((num_points) * sizeof(double), "y pos");

    /* KB 981107: With logarithmic axis first convert back to linear scale */

    if (log_array[xaxis]) {
	for (i = 0; i <= num_points - 1; i++)
	    xp[i] = exp(this_points[i].x * log_base_array[xaxis]);
    } else {
	for (i = 0; i <= num_points - 1; i++)
	    xp[i] = this_points[i].x;
    }
    if (log_array[yaxis]) {
	for (i = 0; i <= num_points - 1; i++)
	    yp[i] = exp(this_points[i].y * log_base_array[yaxis]);
    } else {
	for (i = 0; i <= num_points - 1; i++)
	    yp[i] = this_points[i].y;
    }

    for (i = 0; i <= num_points - 2; i++)
	h[i] = xp[i + 1] - xp[i];

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
	int_error(NO_CARET, "Can't calculate approximation splines");
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
cp_tridiag(plot, first_point, num_points)
struct curve_points *plot;
int first_point, num_points;
{
    spline_coeff *sc;
    tri_diag *m;
    int xaxis = plot->x_axis;
    int yaxis = plot->y_axis;
    double *r, *x, *h, *xp, *yp;
    /* HBB 980308: added 'GPHUGE' tag */
    struct coordinate GPHUGE *this_points;
    int i;

    if (num_points < 3)
	int_error(NO_CARET, "Can't calculate splines, need at least 3 points");

    this_points = (plot->points) + first_point;

    sc = (spline_coeff *) gp_alloc((num_points) * sizeof(spline_coeff), "spline matrix");
    m = (tri_diag *) gp_alloc((num_points - 2) * sizeof(tri_diag), "spline help matrix");

    r = (double *) gp_alloc((num_points - 2) * sizeof(double), "spline right side");
    x = (double *) gp_alloc((num_points - 2) * sizeof(double), "spline solution vector");
    h = (double *) gp_alloc((num_points - 1) * sizeof(double), "spline help vector");

    xp = (double *) gp_alloc((num_points) * sizeof(double), "x pos");
    yp = (double *) gp_alloc((num_points) * sizeof(double), "y pos");

    /* KB 981107: With logarithmic axis first convert back to linear scale */

    if (log_array[xaxis]) {
	for (i = 0; i <= num_points - 1; i++)
	    xp[i] = exp(this_points[i].x * log_base_array[xaxis]);
    } else {
	for (i = 0; i <= num_points - 1; i++)
	    xp[i] = this_points[i].x;
    }
    if (log_array[yaxis]) {
	for (i = 0; i <= num_points - 1; i++)
	    yp[i] = exp(this_points[i].y * log_base_array[yaxis]);
    } else {
	for (i = 0; i <= num_points - 1; i++)
	    yp[i] = this_points[i].y;
    }

    for (i = 0; i <= num_points - 2; i++)
	h[i] = xp[i + 1] - xp[i];

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
	int_error(NO_CARET, "Can't calculate cubic splines");
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
do_cubic(plot, sc, first_point, num_points, dest)
struct curve_points *plot;	/* still containes old plot->points */
spline_coeff *sc;		/* generated by cp_tridiag */
int first_point;		/* where to start in plot->points */
int num_points;			/* to determine end in plot->points */
struct coordinate *dest;	/* where to put the interpolated data */
{
    double xdiff, temp, x, y;
    int i, l;
    int xaxis = plot->x_axis;
    int yaxis = plot->y_axis;
    /* HBB 980308: added 'GPHUGE' tag */
    struct coordinate GPHUGE *this_points;

    /* min and max in internal (eg logged) co-ordinates. We update
     * these, then update the external extrema in user co-ordinates
     * at the end.
     */

    double ixmin, ixmax, iymin, iymax;
    double sxmin, sxmax, symin, symax;	/* starting values of above */

    if (log_array[xaxis]) {
	ixmin = sxmin = log(min_array[xaxis]) / log_base_array[xaxis];
	ixmax = sxmax = log(max_array[xaxis]) / log_base_array[xaxis];
    } else {
	ixmin = sxmin = min_array[xaxis];
	ixmax = sxmax = max_array[xaxis];
    }

    if (log_array[yaxis]) {
	iymin = symin = log(min_array[yaxis]) / log_base_array[yaxis];
	iymax = symax = log(max_array[yaxis]) / log_base_array[yaxis];
    } else {
	iymin = symin = min_array[yaxis];
	iymax = symax = max_array[yaxis];
    }


    this_points = (plot->points) + first_point;

    l = 0;

    xdiff = (this_points[num_points - 1].x - this_points[0].x) / (samples - 1);

    for (i = 0; i < samples; i++) {
	x = this_points[0].x + i * xdiff;

	while ((x >= this_points[l + 1].x) && (l < num_points - 2))
	    l++;

	/* KB 981107: With logarithmic x axis the values were converted back to linear  */
	/* scale before calculating the coefficients. Use exponential for log x values. */

	if (log_array[xaxis]) {
	    temp = exp(x * log_base_array[xaxis]) - exp(this_points[l].x * log_base_array[xaxis]);
	    y = ((sc[l][3] * temp + sc[l][2]) * temp + sc[l][1]) * temp + sc[l][0];
	} else {
	    temp = x - this_points[l].x;
	    y = ((sc[l][3] * temp + sc[l][2]) * temp + sc[l][1]) * temp + sc[l][0];
	}
	/* With logarithmic y axis, we need to convert from linear to log scale now. */
	if (log_array[yaxis]) {
	    if (y > 0.)
		y = log(y) / log_base_array[yaxis];
	    else
		y = symin - (symax - symin);
	}
	dest[i].type = INRANGE;
	STORE_AND_FIXUP_RANGE(dest[i].x, x, dest[i].type, ixmin, ixmax, auto_array[xaxis], NOOP, continue);
	STORE_AND_FIXUP_RANGE(dest[i].y, y, dest[i].type, iymin, iymax, auto_array[yaxis], NOOP, NOOP);

	dest[i].xlow = dest[i].xhigh = dest[i].x;
	dest[i].ylow = dest[i].yhigh = dest[i].y;

	dest[i].z = -1;

    }

    UPDATE_RANGE(ixmax > sxmax, max_array[xaxis], ixmax, xaxis);
    UPDATE_RANGE(ixmin < sxmin, min_array[xaxis], ixmin, xaxis);
    UPDATE_RANGE(iymax > symax, max_array[yaxis], iymax, yaxis);
    UPDATE_RANGE(iymin < symin, min_array[yaxis], iymin, yaxis);

}

/*
 * This is the main entry point used. As stated in the header, it is fine,
 * but I'm not too happy with it.
 */

void
gen_interp(plot)
struct curve_points *plot;
{

    spline_coeff *sc;
    double *bc;
    struct coordinate *new_points;
    int i, curves;
    int first_point, num_points;

    curves = num_curves(plot);
    new_points = (struct coordinate *) gp_alloc((samples + 1) * curves * sizeof(struct coordinate), "interpolation table");

    first_point = 0;
    for (i = 0; i < curves; i++) {
	num_points = next_curve(plot, &first_point);
	switch (plot->plot_smooth) {
	case SMOOTH_CSPLINES:
	    sc = cp_tridiag(plot, first_point, num_points);
	    do_cubic(plot, sc, first_point, num_points,
		     new_points + i * (samples + 1));
	    free(sc);
	    break;
	case SMOOTH_ACSPLINES:
	    sc = cp_approx_spline(plot, first_point, num_points);
	    do_cubic(plot, sc, first_point, num_points,
		     new_points + i * (samples + 1));
	    free(sc);
	    break;

	case SMOOTH_BEZIER:
	case SMOOTH_SBEZIER:
	    bc = cp_binomial(num_points);
	    do_bezier(plot, bc, first_point, num_points,
		      new_points + i * (samples + 1));
	    free((char *) bc);
	    break;
	default:		/* keep gcc -Wall quiet */
	    ;
	}
	new_points[(i + 1) * (samples + 1) - 1].type = UNDEFINED;
	first_point += num_points;
    }

    free(plot->points);
    plot->points = new_points;
    plot->p_max = curves * (samples + 1);
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

static int
compare_points(p1, p2)
struct coordinate *p1;
struct coordinate *p2;
{
    if (p1->x > p2->x)
	return (1);
    if (p1->x < p2->x)
	return (-1);
    return (0);
}

void
sort_points(plot)
struct curve_points *plot;
{
    int first_point, num_points;

    first_point = 0;
    while ((num_points = next_curve(plot, &first_point)) > 0) {
	/* Sort this set of points, does qsort handle 1 point correctly? */
	qsort((char *) (plot->points + first_point), (size_t)num_points,
	      sizeof(struct coordinate), (sortfunc) compare_points);
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
cp_implode(cp)
struct curve_points *cp;
{
    int first_point, num_points;
    int i, j, k;
    double x = 0., y = 0., sux = 0., slx = 0., suy = 0., sly = 0.;
    int xaxis = cp->x_axis;                                                   
    int yaxis = cp->y_axis;                                                   
    TBOOLEAN all_inrange; /* HBB 20000401: use the right type for this flag */

    j = 0;
    first_point = 0;
    while ((num_points = next_curve(cp, &first_point)) > 0) {
	k = 0;
	for (i = first_point; i < first_point + num_points; i++) {
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
		    if (log_array[xaxis])
		    	x = exp(x * log_base_array[xaxis]);
		    if (((x < min_array[xaxis]) && !(auto_array[xaxis] & 1))
			|| ((x > max_array[xaxis]) && !(auto_array[xaxis] & 2)))
			cp->points[j].type = OUTRANGE;
		    else {
			if (log_array[yaxis])
		    	    y = exp(y * log_base_array[yaxis]);
			if (((y < min_array[yaxis]) && !(auto_array[yaxis] & 1))
			    || ((y > max_array[yaxis]) && !(auto_array[yaxis] & 2)))
			    cp->points[j].type = OUTRANGE;
		    }
		}
		j++;		/* next valid entry */
		k = 0;		/* to read */
		i--;		/* from this (-> last after for(;;)) entry */
	    }
	}
	if (k) {
	    cp->points[j].x = x;
	    cp->points[j].y = y /= (double) k;
	    cp->points[j].xhigh = sux / (double) k;
	    cp->points[j].xlow = slx / (double) k;
	    cp->points[j].yhigh = suy / (double) k;
	    cp->points[j].ylow = sly / (double) k;
	    cp->points[j].type = INRANGE;
	    if (! all_inrange) {
		if (log_array[xaxis])
		    x = exp(x * log_base_array[xaxis]);
		if (((x < min_array[xaxis]) && !(auto_array[xaxis] & 1))
		    || ((x > max_array[xaxis]) && !(auto_array[xaxis] & 2)))
		    cp->points[j].type = OUTRANGE;
		else {
		    if (log_array[yaxis])
			y = exp(y * log_base_array[yaxis]);
		    if (((y < min_array[yaxis]) && !(auto_array[yaxis] & 1))
			|| ((y > max_array[yaxis]) && !(auto_array[yaxis] & 2)))
			cp->points[j].type = OUTRANGE;
		}
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
