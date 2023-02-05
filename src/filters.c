/* GNUPLOT - filters.c */

/*[
 * Copyright Ethan A Merritt 2013 - 2023
 * All code in this file is dual-licensed.
 *
 * Gnuplot license:
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
 *
 * Alternative license:
 *
 * As an alternative to distributing code in this file under the gnuplot license,
 * you may instead comply with the terms below. In this case, redistribution and
 * use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
]*/

#include "filters.h"
#include "interpol.h"
#include "alloc.h"
#include "datafile.h"	/* for blank_data_line */
#include "plot2d.h"	/* for cp_extend() */
#include "watch.h"	/* for bisect_target() */

/*
 * local prototypes
 */

static int do_curve_cleanup(struct coordinate *point, int npoints);
static void winnow_interior_points (struct curve_points *plot, t_cluster *cluster);
static double fpp_SG5(struct coordinate *p);
static void cluster_stats(struct coordinate *points, t_cluster *cluster);

/* Variables related to clustering
 * These really belong somewhere else, but the only routines so
 * far that use cluster properties are the hull routines,
 * so keep them together for now.
 *	cluster_outlier_threshold sets criterion for finding outliers
 *	    not currently settable
 *	chi_shape_default_length sets the default choice of chi_length to
 *			chi_shape_default_length * max(edgelengths)
 *	    currently controlled by "set chi_shape fraction <value>"
 */
double cluster_outlier_threshold = 0.0;
double chi_shape_default_length = 0.6;

/*
 * EAM December 2013
 * monotonic cubic spline using the Fritsch-Carlson algorithm
 * FN Fritsch & RE Carlson (1980). "Monotone Piecewise Cubic Interpolation".
 * SIAM Journal on Numerical Analysis (SIAM) 17 (2): 238–246. doi:10.1137/0717021.
 */

void
mcs_interp(struct curve_points *plot)
{
    /* These track the original (pre-sorted) data points */
    int N = plot->p_count;
    struct coordinate *p = gp_realloc(plot->points, (N+1) * sizeof(coordinate), "mcs");
    int i;

    /* These will track the resulting smoothed curve (>= 3X original count) */
    /* Larger number of samples gives smoother curve (no surprise!) */
    int Nsamp = (samples_1 > 2*N) ? samples_1 : 2*N;
    int Ntot = N + Nsamp;
    struct coordinate *new_points = gp_alloc((Ntot) * sizeof(coordinate), "mcs");
    double xstart = GPMAX(p[0].x, X_AXIS.min);
    double xend = GPMIN(p[N-1].x, X_AXIS.max);
    double xstep = (xend - xstart) / (Nsamp - 1);

    /* Load output x coords for sampling */
    for (i=0; i<N; i++)
	new_points[i].x = p[i].x;
    for ( ; i<Ntot; i++)
	new_points[i].x = xstart + (i-N)*xstep;
    /* Sort output x coords */
    qsort(new_points, Ntot, sizeof(struct coordinate), compare_x);
    /* Displace any collisions */
    for (i=1; i<Ntot-1; i++) {
	double delta = new_points[i].x - new_points[i-1].x;
	if (new_points[i+1].x - new_points[i].x < delta/1000.)
	    new_points[i].x -= delta/2.;
    }

    /* Calculate spline coefficients */
#define DX	xlow
#define SLOPE	xhigh
#define C1	ylow
#define C2	yhigh
#define C3	z

    for (i = 0; i < N-1; i++) {
	p[i].DX = p[i+1].x - p[i].x;
	p[i].SLOPE = (p[i+1].y - p[i].y) / p[i].DX;
    }

    /* The SIAM paper only mentions setting the final slope to zero if the
     * calculation is otherwise ill-behaved (how would one detect that?).
     * Retaining the data-derived slope makes the handling at the two ends
     * of the data range consistent. See Bug #2055
     */
    /* p[N-1].SLOPE = 0; */
    p[N-1].SLOPE = p[N-2].SLOPE;

    p[0].C1 = p[0].SLOPE;
    for (i = 0; i < N-1; i++) {
	if (p[i].SLOPE * p[i+1].SLOPE <= 0) {
	    p[i+1].C1 = 0;
	} else {
	    double sum = p[i].DX + p[i+1].DX;
	    p[i+1].C1 = (3. * sum)
		    / ((sum + p[i+1].DX) /  p[i].SLOPE + (sum + p[i].DX) /  p[i+1].SLOPE);
	}
    }
    p[N].C1 = p[N-1].SLOPE;

    for (i = 0; i < N; i++) {
	double temp = p[i].C1 + p[i+1].C1 - 2*p[i].SLOPE;
	p[i].C2 = (p[i].SLOPE - p[i].C1 -temp) / p[i].DX;
	p[i].C3 = temp / (p[i].DX * p[i].DX);
    }

    /* Use the coefficients C1, C2, C3 to interpolate over the requested range */
    for (i = 0; i < Ntot; i++) {
	double x = new_points[i].x;
	double y;
	TBOOLEAN exact = FALSE;

	if (x == p[N-1].x) {	/* Exact value for right-most point of original data */
	    y = p[N-1].y;
	    exact = TRUE;
	} else {
	    int low = 0;
	    int mid;
	    int high = N-1;
	    while (low <= high) {
		mid = floor((low + high) / 2);
		if (p[mid].x < x)
		    low = mid + 1;
		else if (p[mid].x > x)
		    high = mid - 1;
		else {		/* Exact value for some point in original data */
		    y = p[mid].y;
		    exact = TRUE;
		    break;
		}
	    }
	    if (!exact) {
		int j = GPMAX(0, high);
		double diff = x - p[j].x;
		y = p[j].y + p[j].C1 * diff + p[j].C2 * diff * diff + p[j].C3 * diff * diff * diff;
	    }
	}

	xstart = X_AXIS.min;
	xend = X_AXIS.max;
	if (inrange(x, xstart, xend))
	    new_points[i].type = INRANGE;
	else
	    new_points[i].type = OUTRANGE;
	y_axis = plot->y_axis;
	store_and_update_range(&new_points[i].y, y, &new_points[i].type,
		&Y_AXIS, plot->noautoscale);
    }

    /* Replace original data with the interpolated curve */
    free(p);
    plot->points = new_points;
    plot->p_count = Ntot;
    plot->p_max = Ntot + 1;

#undef DX
#undef SLOPE
#undef C1
#undef C2
#undef C3
}

/*
 * Binned histogram of input values.
 *
 *   plot FOO using N:(1) bins{=<nbins>} {binrange=[binlow:binhigh]}
 *                        {binwidth=<width>} with boxes
 *
 * If no binrange is given, binlow and binhigh are taken from the x range of the data.
 * In either of these cases binlow is the midpoint x-coordinate of the first bin
 * and binhigh is the midpoint x-coordinate of the last bin.
 * Points that lie exactly on a bin boundary are assigned to the upper bin.
 * Bin assignments are not affected by "set xrange".
 * Notes:
 *    binwidth = (binhigh-binlow) / (nbins-1)
 *        xmin = binlow - binwidth/2
 *        xmax = binhigh + binwidth/2
 *    first bin holds points with (xmin =< x < xmin + binwidth)
 *    last bin holds points with (xmax-binwidth =< x < binhigh + binwidth)
 *
 *    binopt = 0 (default) return sum of y values for points in each bin
 *    binopt = 1 return mean of y values for points in each bin
 *
 * Ethan A Merritt 2015
 */
void
make_bins(struct curve_points *plot, int nbins,
          double binlow, double binhigh, double binwidth,
	  int binopt)
{
    int i, binno;
    double *bin;
    double bottom, top, range;
    int *members;
    struct axis *xaxis = &axis_array[plot->x_axis];
    struct axis *yaxis = &axis_array[plot->y_axis];
    double ymax = 0;
    int N = plot->p_count;

    /* Find the range of points to be binned */
    if (binlow != binhigh) {
	/* Explicit binrange [min:max] in the plot command */
	bottom = binlow;
	top = binhigh;
    } else {
	/* Take binrange from the data itself */
	bottom = VERYLARGE; top = -VERYLARGE;
	for (i=0; i<N; i++) {
	    if (bottom > plot->points[i].x)
		bottom = plot->points[i].x;
	    if (top < plot->points[i].x)
		top = plot->points[i].x;
	}
	if (top <= bottom)
	    int_warn(NO_CARET, "invalid bin range [%g:%g]", bottom, top);
    }

    /* If a fixed binwidth was provided, find total number of bins */
    if (binwidth > 0) {
	double temp;
	nbins = 1 + (top - bottom) / binwidth;
	temp = nbins * binwidth - (top - bottom);
	bottom -= temp/2.;
	top += temp/2.;
    }
    /* otherwise we use (N-1) intervals between midpoints of bin 1 and bin N */
    else {
	binwidth = (top - bottom) / (nbins - 1);
	bottom -= binwidth/2.;
	top += binwidth/2.;
    }
    range = top - bottom;

    bin = gp_alloc(nbins*sizeof(double), "bins");
    members = gp_alloc(nbins*sizeof(int), "bins");
    for (i=0; i<nbins; i++) {
 	bin[i] = 0;
	members[i] = 0;
    }
    for (i=0; i<N; i++) {
	if (plot->points[i].type == UNDEFINED)
	    continue;
	binno = floor(nbins * (plot->points[i].x - bottom) / range);
	if (0 <= binno && binno < nbins) {
	    bin[binno] += plot->points[i].y;
	    members[binno]++;
	}
    }

    if (xaxis->autoscale & AUTOSCALE_MIN) {
	if (xaxis->min > bottom)
	    xaxis->min = bottom;
    }
    if (xaxis->autoscale & AUTOSCALE_MAX) {
	if (xaxis->max < top)
	    xaxis->max = top;
    }

    /* Replace the original data with one entry per bin.
     * new x = midpoint of bin
     * new y = sum of individual y values over all points in bin
     * new z = number of points in the bin
     */
    plot->p_count = nbins;
    cp_extend(plot, nbins);
    for (i=0; i<nbins; i++) {
	double bincent = bottom + (0.5 + (double)i) * binwidth;
	double ybin = bin[i];
	if ((binopt == 1) && (members[i] > 1))
	    ybin = bin[i]/members[i];

	plot->points[i].type = INRANGE;
	plot->points[i].x     = bincent;
	plot->points[i].xlow  = bincent - binwidth/2.;
	plot->points[i].xhigh = bincent + binwidth/2.;
	plot->points[i].y     = ybin;
	plot->points[i].ylow  = ybin;
	plot->points[i].yhigh = ybin;
	plot->points[i].z     = members[i];

	if (inrange(bincent, xaxis->min, xaxis->max)) {
	    if (ymax < ybin)
		ymax = ybin;
	} else {
	    plot->points[i].type = OUTRANGE;
	}
    }

    if (yaxis->autoscale & AUTOSCALE_MIN) {
	if (yaxis->min > 0)
	    yaxis->min = 0;
    }
    if (yaxis->autoscale & AUTOSCALE_MAX) {
	if (yaxis->max < ymax)
	    yaxis->max = ymax;
    }

    /* Recheck range on y */
    for (i=0; i<nbins; i++)
	if (!inrange(plot->points[i].y, yaxis->min, yaxis->max))
	    plot->points[i].type = OUTRANGE;

    /* Clean up */
    free(bin);
    free(members);
}


/*
 * spline approximation of 3D lines
 *     do_3d_cubic gen_2d_path_splines gen_3d_splines
 * Ethan A Merritt 2019
 */

/*
 * Replace one isocurve with a 3D natural cubic spline interpolation.
 * If there are multiple isocurves, or multiple curves with isocurves,
 * the caller must sort that out and call here separately for each one.
 * TODO:
 *	number of spline samples should be independent of "set samples"
 */
static void
do_3d_cubic(struct iso_curve *curve, enum PLOT_SMOOTH smooth_option)
{
    int i, l;

    int nseg = samples_1;
    struct coordinate *old_points, *new_points;
    double xrange, yrange, zrange;
    double dx, dy, dz;
    double maxdx, maxdy, maxdz;
    double t, tsum, tstep;

    spline_coeff *sc_x = NULL;
    spline_coeff *sc_y = NULL;
    spline_coeff *sc_z = NULL;

    old_points = curve->points;

    /*
     * Sanity check axis ranges.
     * This catches curves that lie in a plane of constant x or y.
     * The fixup prints a warning to the user but we don't see it here.
     */
    axis_checked_extend_empty_range(FIRST_X_AXIS, "at time of spline generation");
    axis_checked_extend_empty_range(FIRST_Y_AXIS, "at time of spline generation");

    /* prevent gross mismatch of x/y/z units */
    xrange = fabs(axis_array[FIRST_X_AXIS].max - axis_array[FIRST_X_AXIS].min);
    yrange = fabs(axis_array[FIRST_Y_AXIS].max - axis_array[FIRST_Y_AXIS].min);
    zrange = fabs(axis_array[FIRST_Z_AXIS].max - axis_array[FIRST_Z_AXIS].min);

    /* Construct path-length vector; store it in unused slot of old_points */
    t = tsum = 0.0;
    maxdx = maxdy = maxdz = 0.0;
    old_points[0].CRD_PATH = 0;
    for (i = 1; i < curve->p_count; i++) {
	dx = (old_points[i].x - old_points[i-1].x) / xrange;
	dy = (old_points[i].y - old_points[i-1].y) / yrange;
	dz = (old_points[i].z - old_points[i-1].z) / zrange;
	tsum += sqrt( dx*dx + dy*dy + dz*dz );
	old_points[i].CRD_PATH = tsum;

	/* Track planarity */
	if (fabs(dx) > maxdx)
	    maxdx = fabs(dx);
	if (fabs(dy) > maxdy)
	    maxdy = fabs(dy);
	if (fabs(dz) > maxdz)
	    maxdz = fabs(dz);
    }

    /* Normalize so that the path always runs from 0 to 1 */
    for (i = 1; i < curve->p_count; i++)
	old_points[i].CRD_PATH /= tsum;
    tstep = old_points[curve->p_count-1].CRD_PATH / (double)(nseg - 1);

    /* Create new list to hold interpolated points */
    new_points = gp_alloc((nseg+1) * sizeof(struct coordinate), "3D spline");
    memset( new_points, 0, (nseg+1) * sizeof(struct coordinate));

    /*
     * If the curve being fitted lies entirely in one plane,
     * we can do better by fitting a 2D spline rather than a 3D spline.
     * This benefits the relatively common case of drawing a stack of
     * 2D plots (e.g. fence plots).
     * First check for a curve lying in the yz plane (x = constant).
     */
    if (maxdx < FLT_EPSILON) {
	tstep = (old_points[curve->p_count-1].y - old_points[0].y) / (double)(nseg - 1);

	if (smooth_option == SMOOTH_ACSPLINES)
	    sc_z = cp_approx_spline(curve->points, curve->p_count, 1, 2, 3);
	else
	    sc_z = cp_tridiag(curve->points, curve->p_count, 1, 2);

	for (i = 0, l = 0; i < nseg; i++) {
	    double temp;
	    t = old_points[0].y + i * tstep;
	    /* Move forward to the spline interval this point is in */
	    while ((t >= old_points[l + 1].y) && (l < curve->p_count- 2))
		l++;
	    temp = t - old_points[l].y;
	    new_points[i].x = old_points[l].x;	/* All the same */
	    new_points[i].y = t;
	    new_points[i].z = ((sc_z[l][3] * temp + sc_z[l][2]) * temp + sc_z[l][1])
			    * temp + sc_z[l][0];
	}
    }

    /*
     * Check for a curve lying in the xz plane (y = constant).
     */
    else if (maxdy < FLT_EPSILON) {
	tstep = (old_points[curve->p_count-1].x - old_points[0].x) / (double)(nseg - 1);
	if (smooth_option == SMOOTH_ACSPLINES)
	    sc_z = cp_approx_spline(curve->points, curve->p_count, 0, 2, 3);
	else
	    sc_z = cp_tridiag(curve->points, curve->p_count, 0, 2);

	for (i = 0, l = 0; i < nseg; i++) {
	    double temp;
	    t = old_points[0].x + i * tstep;
	    /* Move forward to the spline interval this point is in */
	    while ((t >= old_points[l + 1].x) && (l < curve->p_count- 2))
		l++;
	    temp = t - old_points[l].x;
	    new_points[i].x = t;
	    new_points[i].y = old_points[l].y;	/* All the same */
	    new_points[i].z = ((sc_z[l][3] * temp + sc_z[l][2]) * temp + sc_z[l][1])
			    * temp + sc_z[l][0];
	}
    }

    /*
     * Check for a curve lying in the xy plane (z = constant).
     */
    else if (maxdz < FLT_EPSILON) {
	tstep = (old_points[curve->p_count-1].x - old_points[0].x) / (double)(nseg - 1);
	if (smooth_option == SMOOTH_ACSPLINES)
	    sc_y = cp_approx_spline(curve->points, curve->p_count, 0, 1, 3);
	else
	    sc_y = cp_tridiag(curve->points, curve->p_count, 0, 1);

	for (i = 0, l = 0; i < nseg; i++) {
	    double temp;
	    t = old_points[0].x + i * tstep;
	    /* Move forward to the spline interval this point is in */
	    while ((t >= old_points[l + 1].x) && (l < curve->p_count- 2))
		l++;
	    temp = t - old_points[l].x;
	    new_points[i].x = t;
	    new_points[i].y = ((sc_y[l][3] * temp + sc_y[l][2]) * temp + sc_y[l][1])
			    * temp + sc_y[l][0];
	    new_points[i].z = old_points[l].z;	/* All the same */
	}
    }

    /*
     * This is the general case.
     * Calculate spline coefficients for each dimension x, y, z
     */
    else {
	if (smooth_option == SMOOTH_ACSPLINES) {
	    sc_x = cp_approx_spline(curve->points, curve->p_count, PATHCOORD, 0, 3);
	    sc_y = cp_approx_spline(curve->points, curve->p_count, PATHCOORD, 1, 3);
	    sc_z = cp_approx_spline(curve->points, curve->p_count, PATHCOORD, 2, 3);
	} else {
	    sc_x = cp_tridiag( curve->points, curve->p_count, PATHCOORD, 0);
	    sc_y = cp_tridiag( curve->points, curve->p_count, PATHCOORD, 1);
	    sc_z = cp_tridiag( curve->points, curve->p_count, PATHCOORD, 2);
	}

	for (i = 0, l=0; i < nseg; i++) {
	    double temp;
	    t = i * tstep;
	    /* Move forward to the spline interval this point is in */
	    while ((t >= old_points[l + 1].CRD_PATH) && (l < curve->p_count- 2))
		l++;
	    temp = t - old_points[l].CRD_PATH;

	    new_points[i].x = ((sc_x[l][3] * temp + sc_x[l][2]) * temp + sc_x[l][1])
			    * temp + sc_x[l][0];
	    new_points[i].y = ((sc_y[l][3] * temp + sc_y[l][2]) * temp + sc_y[l][1])
			    * temp + sc_y[l][0];
	    new_points[i].z = ((sc_z[l][3] * temp + sc_z[l][2]) * temp + sc_z[l][1])
			    * temp + sc_z[l][0];
	}
    }

    /* We're done with the spline coefficients */
    free(sc_x);
    free(sc_y);
    free(sc_z);

    /* Replace original data with spline approximation */
    free(curve->points);
    curve->points = new_points;
    curve->p_count = nseg;
    curve->p_max = nseg+1;	/* not sure why we asked for 1 extra */

}

/*
 * Generate 2D splines along a path for each set of points in the plot,
 * smoothing option SMOOTH_PATH.
 * TODO:
 * - number of spline samples should be controlled by something other
 *   than "set samples"
 * - spline weights from an additional column
 */
void
gen_2d_path_splines( struct curve_points *plot )
{
    int i;
    int ic, first_point;	/* indexes for original data */
    int is = 0;			/* index for new (splined) data */
    struct coordinate *old_points = NULL;
    struct coordinate *splined_points;
    spline_coeff *sc_x = NULL;
    spline_coeff *sc_y = NULL;

    double xrange = fabs(axis_array[plot->x_axis].max - axis_array[plot->x_axis].min);
    double yrange = fabs(axis_array[plot->y_axis].max - axis_array[plot->y_axis].min);
    int curves = num_curves(plot);

    /* Allocate space to hold the interpolated points */
    splined_points = gp_alloc( (plot->p_count + samples_1 * curves) * sizeof(struct coordinate), NULL );
    memset( splined_points, 0, (plot->p_count + samples_1 * curves) * sizeof(struct coordinate));

    first_point = 0;
    for (ic = 0; ic < curves; ic++) {
	double t, tstep, tsum;
	double dx, dy;
	int l;
	int nold;
	int num_points = next_curve(plot, &first_point);
	TBOOLEAN closed = FALSE;

	/* Make a copy of the original points so that we don't corrupt the
	 * list by adding up to three new ones.
	 */
	old_points = gp_realloc( old_points, (num_points + 3) * sizeof(struct coordinate),
				"spline points");
	memcpy( &old_points[1], &plot->points[first_point], num_points * sizeof(struct coordinate));

	/* Remove any unusable points (NaN, missing, duplicates) before fitting a spline.
	 * If that leaves fewer than 3 points, skip it.
	 */
	nold = do_curve_cleanup(&old_points[1], num_points);
	if (nold < 3) {
	    first_point += num_points;
	    continue;
	}

	/* We expect one of two cases. Either this really is a closed
	 * curve (end point matches start point) or it is an open-ended
	 * path that may not be monotonic on x.
	 * For plot style "with filledcurves closed" we add an extra
	 * point at the end if it is not already there.
	 */
	if (old_points[1].x == old_points[nold].x
	&&  old_points[1].y == old_points[nold].y)
	    closed = TRUE;
	if ((plot->plot_style == FILLEDCURVES) && !closed) {
	    old_points[++nold] = old_points[1];
	    closed = TRUE;
	}

	if (closed) {
	    /* Wrap around to one point before and one point after the path closure */
	    nold += 2;
	    old_points[0] = old_points[nold-3];
	    old_points[nold-1] = old_points[2];
	} else {
	    /* Dummy up an extension at either end */
	    nold += 2;
	    old_points[0].x = old_points[1].x + old_points[1].x - old_points[2].x;
	    old_points[nold-1].x = old_points[nold-2].x + old_points[nold-2].x - old_points[nold-3].x;
	    old_points[0].y = old_points[1].y + old_points[1].y - old_points[2].y;
	    old_points[nold-1].y = old_points[nold-2].y + old_points[nold-2].y - old_points[nold-3].y;
	}

	/* Construct path-length vector; store it in an unused slot of old_points */
	t = tsum = 0.0;
	old_points[0].CRD_PATH = 0;
	if (xrange == 0)
	    xrange = 1.;
	if (yrange == 0)
	    yrange = 1.;
	for (i = 1; i < nold; i++) {
	    dx = (old_points[i].x - old_points[i-1].x) / xrange;
	    dy = (old_points[i].y - old_points[i-1].y) / yrange;
	    tsum += sqrt( dx*dx + dy*dy );
	    old_points[i].CRD_PATH = tsum;
	}

	/* Normalize so that the path fraction always runs from 0 to 1 */
	for (i = 1; i < nold; i++)
	    old_points[i].CRD_PATH /= tsum;
	tstep = 1.0 / (double)(samples_1 - 1);

	/* Calculate spline coefficients for x and for y as a function of path */
	sc_x = cp_tridiag( old_points, nold, PATHCOORD, 0);
	sc_y = cp_tridiag( old_points, nold, PATHCOORD, 1);

	/* First output point is the same as the original first point */
	splined_points[is++] = old_points[1];

	/* Skip the points in the overlap region */
	for (i = 0; i * tstep < old_points[1].CRD_PATH; i++)
	    ;

	/* Use spline coefficients to generate a new point at each sample interval. */
	for (l=0; i < samples_1; i++) {
	    double temp;
	    t = i * tstep;

	    /* Stop before wrapping around. Copy the original end point. */
	    if (t > old_points[nold-2].CRD_PATH) {
		splined_points[is++] = old_points[nold-2];
		break;
	    }

	    /* Move forward to the spline interval this point is in */
	    while ((t >= old_points[l + 1].CRD_PATH) && (l < nold- 2))
		l++;
	    temp = t - old_points[l].CRD_PATH;

	    splined_points[is].x = ((sc_x[l][3] * temp + sc_x[l][2]) * temp + sc_x[l][1])
				 * temp + sc_x[l][0];
	    splined_points[is].y = ((sc_y[l][3] * temp + sc_y[l][2]) * temp + sc_y[l][1])
				 * temp + sc_y[l][0];
	    is++;
	}

	/* Done with spline coefficients */
	free(sc_x);
	free(sc_y);

	/* Add a seperator point after this set of splined points */
	splined_points[is++].type = UNDEFINED;

	first_point += num_points;
    }

    /* Replace original data with splined approximation */
    free(old_points);
    free(plot->points);
    plot->points = splined_points;
    plot->p_max = curves * samples_1;
    plot->p_count = is;

    return;
}

/*
 * Externally callable interface to 3D spline routines
 */
void
gen_3d_splines( struct surface_points *plot )
{
    struct iso_curve *curve = plot->iso_crvs;

    while (curve) {
	/* Remove any unusable points before fitting a spline */
	curve->p_count = do_curve_cleanup(curve->points, curve->p_count);
	if (curve->p_count > 3)
	    do_3d_cubic(curve, plot->plot_smooth);
	curve = curve->next;
    }
}

static int
do_curve_cleanup( struct coordinate *point, int npoints )
{
    int i, keep;

    /* Step through points in curve keeping only the usable ones.
     * Discard duplicates
     */
    keep = 0;
    for (i = 0; i < npoints; i++) {
	if (point[i].type == UNDEFINED)
	    continue;
	if (isnan(point[i].x) || isnan(point[i].y) || isnan(point[i].z))
	    continue;
	if (i != keep)
	    point[keep] = point[i];
	/* FIXME: should probably check fabs(this-prev) < EPS */
	if ((keep > 0)	&& (point[keep].x == point[keep-1].x)
			&& (point[keep].y == point[keep-1].y)
			&& (point[keep].z == point[keep-1].z))
	    continue;
	keep++;
    }

    return keep;
}

/*
 * convex_hull() replaces the original set of points with a subset that
 * delimits the convex hull of the original points.
 * The hull is found using Graham's algorithm
 *    RL Graham (1972), Information Processing Letters 1: 132–133.
 * winnow_interior_points() is a helper routine that can greatly reduce
 * processing time for large data sets but is otherwise not necessary.
 * expand_hull() pushes the hull segments away from the interior to
 * produce a bounding curve exterior to all points.
 * - Ethan A Merritt 2021
 */
#define CROSS(p1,p2,p3) \
  ( ((p2)->x - (p1)->x) * ((p3)->y - (p2)->y) \
  - ((p2)->y - (p1)->y) * ((p3)->x - (p2)->x) )

void
convex_hull(struct curve_points *plot)
{
    int i;
    struct coordinate *points = plot->points;
    struct coordinate **stack = NULL;
    int np = plot->p_count;
    int ntop;

    /* cluster is used to hold x/y ranges and centroid */
    t_cluster cluster;
    cluster.npoints = plot->p_count;

    /* Special cases */
    if (np < 3)
	return;
    if (np == 3) {
	cp_extend(plot, 4);
	plot->points[3] = plot->points[0];
	plot->p_count = 4;
	return;
    }

    /* Find x and y limits of points in the cluster.  */
    cluster_stats(plot->points, &cluster);

    /* This is not strictly necessary, but greatly reduces the number
     * of points to be sorted and tested for the hull boundary.
     */
    winnow_interior_points(plot, &cluster);

    /* Sort the remaining points (probably only need to sort on x?) */
    qsort(plot->points, plot->p_count,
	  sizeof(struct coordinate), compare_xyz);

    /* Find hull points using a variant of Graham's algorithm.
     * The path through the points is accumulated on a stack.
     */
    stack = gp_alloc( (np+1)*sizeof(void *), "Hull" );
    /* Initialize stack with known start of top arc and first candidate point */
    stack[0] = &points[0];
    stack[1] = &points[1];
    np = 2;
    for (i=2; i<plot->p_count; i++) {
	while ((np >= 2) && CROSS( stack[np-2], stack[np-1], &points[i] ) >= 0)
		np--;
	stack[np++] = &points[i];
    }
    /* push onto stack the first candidate point for lower arc */
    i -= 2;
    stack[np++] = &points[i];
    ntop = np;
    for (i--; i>=0; i--) {
	while ( (np >= ntop) && CROSS( stack[np-2], stack[np-1], &points[i] ) >= 0)
		np--;
	stack[np++] = &points[i];
    }

    /* Replace the original list of points with the ordered path */
    points = gp_alloc( np * sizeof(struct coordinate), "Hull" );
    for (i=0; i<np; i++)
	points[i] = *(stack[i]);
    cp_extend(plot, 0);
    free(stack);
    plot->points = points;
    plot->p_count = np;
    plot->p_max = np;
}

/*
 * winnow_interior_points() is an optional helper routine for convex_hull.
 * It reduces the number of points to be sorted and processed by removing
 * points in a quadrilateral bounded by the four points with max/min x/y.
 */
static void
winnow_interior_points (struct curve_points *plot, t_cluster *cluster)
{
#define TOLERANCE -1.e-10

    struct coordinate *p, *pp1, *pp2, *pp3, *pp4;
    struct coordinate *points = plot->points;
    double area;
    int i, np;

    /* Find points defining maximal extent on x and y */
    pp1 = pp2 = pp3 = pp4 = plot->points;
    for (p = plot->points; p < &(plot->points[plot->p_count]); p++) {
	if (p->x == cluster->xmin) pp1 = p;
	if (p->x == cluster->xmax) pp2 = p;
	if (p->y == cluster->ymin) pp3 = p;
	if (p->y == cluster->ymax) pp4 = p;
    }

    /* Ignore any points that lie inside the clockwise triangle bounded by pp1 pp2 pp3 */
    area = fabs(-pp2->y*pp3->x + pp1->y*(-pp2->x + pp3->x)
		+ pp1->x*(pp2->y - pp3->y) + pp2->x*pp3->y);
    area += TOLERANCE;
    for (i=0; i<plot->p_count; i++) {
	double px = points[i].x;
	double py = points[i].y;
	double s = (pp1->y*pp3->x - pp1->x*pp3->y + (pp3->y - pp1->y)*px + (pp1->x - pp3->x)*py);
	double t = (pp1->x*pp2->y - pp1->y*pp2->x + (pp1->y - pp2->y)*px + (pp2->x - pp1->x)*py);
	if ( (s < TOLERANCE) && (t < TOLERANCE) && (fabs(s+t) < area) )
	    points[i].type = EXCLUDEDRANGE;
    }

    /* Also ignore points in the clockwise triangle bounded by pp3 pp4 pp1 */
    area = fabs(-pp4->y*pp1->x + pp3->y*(-pp4->x + pp1->x)
		+ pp3->x*(pp4->y - pp1->y) + pp4->x*pp1->y);
    area += TOLERANCE;
    for (i=0; i<plot->p_count; i++) {
	double px = points[i].x;
	double py = points[i].y;
	double s = (pp3->y*pp1->x - pp3->x*pp1->y + (pp1->y - pp3->y)*px + (pp3->x - pp1->x)*py);
	double t = (pp3->x*pp4->y - pp3->y*pp4->x + (pp3->y - pp4->y)*px + (pp4->x - pp3->x)*py);
	if ( (s < TOLERANCE) && (t < TOLERANCE) && (fabs(s+t) < area) )
	    points[i].type = EXCLUDEDRANGE;
    }

    /* Discard the interior points and outliers */
    np = 0;
    p = points;
    for  (i=0; i<plot->p_count; i++) {
	if (points[i].type == UNDEFINED)
	    continue;
	if (points[i].type != EXCLUDEDRANGE)
	    p[np++] = points[i];
    }
    plot->p_count = np;

#undef TOLERANCE
}

/*
 * expand_hull() "inflates" a convex or concave hull by displacing
 * each edge away from the interior along its normal vector
 * by requested distance d.
 */
void
expand_hull(struct curve_points *plot)
{
    struct coordinate *newpoints = NULL;
    struct coordinate *points = plot->points;
    double scale = plot->smooth_parameter;
    double d = fabs(scale);
    int N = plot->p_count;

    struct coordinate *v1, *v2, *v3;
    t_cluster cluster;
    double winding;	/* +1 for clockwise -1 for anticlockwise */
    int ni;

    /* Determine whether the hull points are ordered clockwise or
     * anticlockwise.  This allows easy determination of the interior
     * side of each edge and the concave/convex status of each vertex.
     */
    cluster.npoints = N;
    cluster_stats(points, &cluster);
    v1 = &points[ (cluster.pin == 0) ? N-2 : cluster.pin-1 ];
    v2 = &points[cluster.pin];
    v3 = &points[cluster.pin + 1];

    if (CROSS(v2,v1,v3) > 0)
	winding = 1.0;
    else
	winding = -1.0;

    /* Each edge of the hull is displaced outward by a constant amount d.
     * At each convex vertex this replaces the original point with two
     * points that are the vertices of a beveled join.
     * At each concave vertex this displaces the original point toward
     * the mouth of the concavity.
     */
    newpoints = gp_alloc(2*N * sizeof(struct coordinate), "expand hull");
    ni = 0;
    for (int i = 0; i < N; i++) {
	double m1, m2, d2norm;
	double dx1, dy1, dx2, dy2;

	v1 = &points[ (i == 0) ? N-2 : i-1 ];
	v2 = &points[i];
	v3 = &points[ (i == N-1) ? 1 : i+1 ];

	m1 = (v2->y - v1->y) / (v2->x - v1->x);	/* slope of v1v2 */
	d2norm = d*d / (1/(m1*m1) + 1);
	dx1 = copysign( sqrt(d2norm), -winding * (v2->y - v1->y) );
	dy1 = copysign( sqrt(d*d - d2norm), winding * (v2->x - v1->x) );
	m2 = (v3->y - v2->y) / (v3->x - v2->x);	/* slope of v2v3 */
	d2norm = d*d / (1/(m2*m2) + 1);
	dx2 = copysign( sqrt(d2norm), -winding * (v3->y - v2->y) );
	dy2 = copysign( sqrt(d*d - d2norm), winding * (v3->x - v2->x) );

	/* convex vertex */
	if (winding * CROSS(v1,v2,v3) < 0) {
	    newpoints[ni] = points[i];
	    newpoints[ni].x = v2->x + dx1;
	    newpoints[ni].y = v2->y + dy1;
	    ni++;
	    newpoints[ni] = points[i];
	    newpoints[ni].x = v2->x + dx2;
	    newpoints[ni].y = v2->y + dy2;
	    ni++;

	/* concave vertex (over-emphasizes steep holes) */
	} else {
	    double dnorm = d / sqrt((dx1+dx2)*(dx1+dx2) + (dy1+dy2)*(dy1+dy2));
	    newpoints[ni] = points[i];
	    newpoints[ni].x = v2->x + dnorm * (dx1 + dx2);
	    newpoints[ni].y = v2->y + dnorm * (dy1 + dy2);
	    ni++;
	}
    }

    /* Replace original point list with the new one */
    cp_extend(plot, 0);
    plot->points = newpoints;
    plot->p_count = ni;
    plot->p_max = 2*N;
}

/*
 * Find min, max, center of mass points in cluster held in plot->points.
 * All points not marked EXCLUDEDRANGE are assumed to be in a single cluster.
 */
static void
cluster_stats( struct coordinate *points, t_cluster *cluster )
{
    double xsum = 0;
    double ysum = 0;
    int excluded = 0;

    cluster->xmin = cluster->ymin = VERYLARGE;
    cluster->xmax = cluster->ymax = -VERYLARGE;
    cluster->pin = -1;

    for (int i = 0; i < cluster->npoints; i++) {
	if (points[i].type == EXCLUDEDRANGE || points[i].type == UNDEFINED) {
	    excluded++;
	    continue;
	}
	xsum += points[i].x;
	ysum += points[i].y;
	if ((cluster->xmin == points[i].x)
	&&  (points[i].y > points[cluster->pin].y))
	    cluster->pin = i;
	if (cluster->xmin > points[i].x) {
	    cluster->xmin = points[i].x;
	    cluster->pin = i;
	    }
	if (cluster->ymin > points[i].y)
	    cluster->ymin = points[i].y;
	if (cluster->xmax < points[i].x)
	    cluster->xmax = points[i].x;
	if (cluster->ymax < points[i].y)
	    cluster->ymax = points[i].y;
    }
    cluster->cx = xsum / (cluster->npoints - excluded);
    cluster->cy = ysum / (cluster->npoints - excluded);
}

/*
 * The "sharpen" filter looks for truncated extrema in the function being plotted.
 * The true local extremum is found by bisection and added to the set of
 * points being plotted.
 *
 * Motivation:
 * If the function being plotted has a sharp extremum that lies between
 * two of the sampled points, the resulting plot truncates the extremum.
 * Increasing the number of samples would help, of course, but they are
 * only really needed close to the extremum.  And no matter how many
 * samples there are, the true peak may lie between two of them.
 * Example:   plot lgamma(x)
 *
 * Method:
 * - Approximate the second derivative of the function at each point using
 *   a 5-term Savitsky-Golay filter.
 * - Inspect a moving window centered on point i
 *   Truncation is suspected if either
 *	f(i) is a minimum and f''(i-2) > 0 and/or f''(i+2) > 0
 *   or
 *	f(i) is a maximum and f''(i-2) < 0 and/or f''(i+2) < 0
 * - Use bisection to find the true local extremum
 * - Add the new point next to the original point i
 *
 * Ethan A Merritt Dec 2022
 *
 * TODO:
 * - Diagnose and warn that finer sampling might help?
 */
void
sharpen(struct curve_points *plot)
{
    struct axis *y_axis = &axis_array[plot->y_axis];
    int newcount = plot->p_count;
    struct coordinate *p;

    /* Restrictions */
    if (parametric || polar)
	return;

    /* Make more than enough room for new points */
    cp_extend(plot, 1.5 * plot->p_count);
    p = plot->points;

    /* We expect that sharpened peaks may go to +/- Infinity
     * and in fact the plot may already contain such points.
     * Because these were tagged UNDEFINED in store_and_update_range()
     * they will be lost when the points are sorted.
     * As a work-around we look for these and flag them OUTRANGE instead.
     * If there are other UNDEFINED points we set y to 0 so that they
     * do not cause the f'' approximation to blow up.
     */
    for (int i = 0; i < plot->p_count; i++) {
	if (p[i].type == UNDEFINED) {
	    if (p[i].y >= VERYLARGE) {
		p[i].type = OUTRANGE;
		p[i].y = VERYLARGE;
	    } else if (p[i].y <= -VERYLARGE) {
		p[i].type = OUTRANGE;
		p[i].y = -VERYLARGE;
	    } else
		p[i].y = 0;
	}
    }

    /* Look for minima in a sliding window */
    /* This is a very simple test; it may not be sufficient
     *   f(i) is lower than its neighbors
     *   f''(x) is negative on at least one side
     */
    for (int i = 4; i < plot->p_count-4; i++) {
	TBOOLEAN criterion = FALSE;
	criterion = (fpp_SG5(&p[i-2]) < 0 || fpp_SG5(&p[i+2]) < 0);

	if (p[i].y <= p[i-1].y && p[i].y <= p[i+1].y
	&&  p[i].y <= p[i-2].y && p[i].y <= p[i+2].y
	&&  criterion)
	{
	    double hit_x = p[i].x;
	    double hit_y = p[i].y;
	    bisect_hit( plot, BISECT_MINIMIZE, &hit_x, &hit_y, p[i-1].x, p[i+1].x );
	    p[newcount] = p[i];	/* copy any other properties */
	    p[newcount].x = hit_x;
	    p[newcount].y = hit_y;
	    /* Use sharpened peak for autoscaling only if it approaches zero,
	     * not if it's heading towards +/- Infinity!
	     */
	    if (fabs(hit_y) < 1.e-7 && !y_axis->log)
		autoscale_one_point(y_axis, hit_y);
	    if (!inrange(hit_y, y_axis->min, y_axis->max))
		p[newcount].type = OUTRANGE;
	    else
		p[newcount].type = INRANGE;
	    if (debug)
		FPRINTF((stderr, "\tbisect min [%4g %g]\tto [%4g %4g]\n",
			p[i].x, p[i].y, hit_x, hit_y));
	    newcount++;
	}
    }

    /* Equivalent search for maxima */
    for (int i = 4; i < plot->p_count-4; i++) {
	TBOOLEAN criterion = FALSE;
	if (p[i].type == UNDEFINED || p[i-1].type == UNDEFINED || p[i+1].type == UNDEFINED)
	    continue;
	criterion = (fpp_SG5(&p[i-2]) > 0 || fpp_SG5(&p[i+2]) > 0);

	if (p[i].y >= p[i-1].y && p[i].y >= p[i+1].y
	&&  p[i].y >= p[i-2].y && p[i].y >= p[i+2].y
	&&  criterion)
	{
	    double hit_x = p[i].x;
	    double hit_y = p[i].y;
	    bisect_hit( plot, BISECT_MAXIMIZE, &hit_x, &hit_y, p[i-1].x, p[i+1].x );
	    p[newcount] = p[i];	/* copy any other properties */
	    p[newcount].x = hit_x;
	    p[newcount].y = hit_y;
	    if (fabs(hit_y) < 1.e-7 && !y_axis->log)
		autoscale_one_point(y_axis, hit_y);
	    if (!inrange(hit_y, y_axis->min, y_axis->max))
		p[newcount].type = OUTRANGE;
	    else
		p[newcount].type = INRANGE;
	    if (debug)
		FPRINTF((stderr, "\tbisect max [%4g %g]\tto [%4g %4g]\n",
			p[i].x, p[i].y, hit_x, hit_y));
	    newcount++;
	}
    }

    /* Move new points into proper order */
    if (newcount > plot->p_count) {
	plot->p_count = newcount;
	qsort(plot->points, newcount, sizeof(struct coordinate), compare_x);
    }
}

/* Approximate second derivative using 5-term Savitsky-Golay filter
 *         [ +2 -1 -2 -1 +2 ] . p[i-2 : i+2]
 * (helper function for sharpen).
 *
 * Normalization by 1. / delta_x**2 is not included.
 */
static double
fpp_SG5( struct coordinate *p )
{
    double fpp;

    if (p->type == UNDEFINED)
	return 0;
    fpp = 2.0 * (p-2)->y - (p-1)->y - 2.0 * p->y - (p+1)->y + 2.0 * (p+2)->y;
    fpp /= 7.0;

    return fpp;
}


#ifdef WITH_CHI_SHAPES

/*
 * The routines in this section support the generation of concave hulls.
 * Copyright Ethan A Merritt 2022
 *
 * Delaunay triangulation uses the Bowyer-Watson algorithm
 *	A Bowyer (1981) Computer Journal 24:162.
 *	DF Watson (1981) Computer Journal 24:167.
 *
 * Generation of χ-shapes follows the description in
 *	M Duckham, L Kulik, M Worboys, and A Galton (2008)
 *	Pattern Recognition 41: 3224_3236.
 *
 * Corresponding routines called as filter operations from plot2d.c
 *	delaunay_triangulation( plot )
 *	concave_hull( plot )
 */

typedef struct triangle {
    int v1, v2, v3;	/* indices of vertices in array points[] */
    double cx, cy;	/* center of circumcircle */
    double r;		/* radius of circumcircle */
    struct triangle *next;
} triangle;

typedef struct t_edge {
    int v1, v2;		/* indices of the endpoints in array points[] */
    double length;	/* used for χ-shapes */
} t_edge;

/*
 * local prototypes for helper routines
 */
static triangle *insert_new_triangle( struct coordinate *points, int v1, int v2, int v3 );
static void find_circumcircle( struct coordinate *points, triangle *t);
static TBOOLEAN in_circumcircle( triangle *t, double x, double y );
static void invalidate_triangle( triangle *prev, triangle *this );
static void freeze_triangle( triangle *prev, triangle *this );
static void delete_triangles( triangle *list_head );
static int compare_edges(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2);
static double edge_length( t_edge *edge, struct coordinate *p );
static void chi_reduction( struct curve_points *plot, double l_chi );
static TBOOLEAN dig_one_hole( struct curve_points *plot, double l_chi );

#define HULL_POINT 1

/* List heads
 * The lists these point to are freed and reinitialized on each call to
 * delaunay_triangulation().  The constituent vertices of each triangle
 * are stored as indices for the points array of the current plot.
 * Therefore the lists are useful only in the context of the current plot,
 * and only so long as the points are not overwritten, as indeed they
 * are when replaced by a hull in concave_hull().
 */
static triangle good_triangles =   { .next = NULL };
static triangle bad_triangles =    { .next = NULL };
static triangle frozen_triangles = { .next = NULL };

/* Similarly for an array containing line segments making up the
 * perimeter of the triangulation.
 */
static t_edge *bounding_edges = NULL;
static int n_bounding_edges = 0;
static int max_bounding_edges = 0;

/*
 * Free local storage (called also from global reset command)
 */
void
reset_hulls()
{
    delete_triangles(&good_triangles);
    delete_triangles(&bad_triangles);
    delete_triangles(&frozen_triangles);
    free(bounding_edges);
    bounding_edges = NULL;
    n_bounding_edges = 0;
    max_bounding_edges = 0;
}

/*
 * Delaunay triangulation of a set of N points.
 * The points are taken from the current plot structure.
 * This is a filter operation; i.e. it is called from a gnuplot "plot"
 * command after reading in data but before smoothing or plot generation.
 * The resulting triangles are stored in a list with head good_triangles.
 * The edges making up the convex hull are stored in array bounding_edges[].
 * All points that lie on the hull are flagged by setting
 *     point->extra = HULL_POINT
 */
void
delaunay_triangulation( struct curve_points *plot )
{
    struct coordinate *points;
    int N = plot->p_count;

    t_edge *edges = NULL;
    int nedges;
    coordinate *p;
    triangle *t, *prev;
    triangle *frozen_tail = NULL;
    int i;

    /* Clear out any previous work */
    reset_hulls();

    /* Pull color out of the varcolor array and copy it into point.CRD_COLOR
     * so that it isn't lost during sort.
     */
    if (plot->varcolor)
	for (i = 0; i < plot->p_count; i++)
	    plot->points[i].CRD_COLOR = plot->varcolor[i];

    /* Sort the points on x.
     * This allows us to reduce the time required from O(N^2) to
     * approximately O(NlogN).
     */
    qsort(plot->points, N, sizeof(struct coordinate), compare_xyz);

    /* Construct a triangle "sufficiently big" to enclose the set of points.
     * That means each bounding vertex must be far enough away from the data
     * points that a circumcircle does not catch extra data points.
     */
    {
#       define BIG 8
	double xmin = VERYLARGE, xmax = -VERYLARGE;
	double ymin = VERYLARGE, ymax = -VERYLARGE;
	double xdelta, ydelta;

	/* Allocate 3 additional points for the triangle */
	cp_extend(plot, N+3);
	points = plot->points;

	for (p = points; p < &points[N]; p++) {
	    if (p->type == UNDEFINED)
		continue;
	    if (p->x < xmin) xmin = p->x;
	    if (p->x > xmax) xmax = p->x;
	    if (p->y < ymin) ymin = p->y;
	    if (p->y > ymax) ymax = p->y;
	}
	xdelta = xmax - xmin;
	points[N].x = xmin - 2*BIG * xdelta;
	points[N+1].x = xmax + 2*BIG * xdelta;
	points[N+2].x = (xmax+xmin)/2.;
	ydelta = ymax - ymin;
	points[N].y = ymin - BIG * ydelta;
	points[N+1].y = ymin - BIG * ydelta;
	points[N+2].y = ymax + BIG * ydelta;
	t = insert_new_triangle( points, N, N+1, N+2 );
	find_circumcircle(points, t);

#	undef BIG
    }

    /* Add points one by one */
    for (p = points; p < &points[N]; p++) {

	/* Ignore undefined points. */
	if (p->type == UNDEFINED)
	    continue;

	/* Also ignore duplicate points */
	if (p->x == (p+1)->x && p->y == (p+1)->y)
	    continue;

	/* First step is to move all triangles for which the new point
	 * violates the criterion "no other points in bounding circle"
	 * to a separate list bad_triangles.
	 */
	delete_triangles(&bad_triangles);
	prev = &good_triangles;
	for (t = prev->next; t; t = prev->next) {
	    if (in_circumcircle(t, p->x, p->y))
		invalidate_triangle(prev, t);

	    /* Points are sorted on x, so if the circumcircle of this
	     * triangle is entirely to the left of the current point
	     * we needn't check it for subsequent points either.
	     */
	    else if ((t->cx + t->r) < p->x) {
		freeze_triangle(prev, t);
		if (frozen_tail == NULL)
		    frozen_tail = t;
	    } else
		prev = t;
	}

	/* I think this only happens if there is a duplicate point */
	if (bad_triangles.next == NULL)
	    continue;

	/* Second step is to find the edges forming the perimeter
	 * of the bad triangles.  An edge cannot appear twice.
	 * If it does, flag both copies as -1.
	 */
	for (i = 0, t = bad_triangles.next; t; t = t->next)
	    i++;
	edges = gp_realloc(edges, 3*i * sizeof(t_edge), "delaunay edges");
	for (i = 0, t = bad_triangles.next; t; t = t->next) {
	    edges[i].v1 = (t->v1 > t->v2) ? t->v2 : t->v1;
	    edges[i].v2 = (t->v1 > t->v2) ? t->v1 : t->v2;
	    i++;
	    edges[i].v1 = (t->v2 > t->v3) ? t->v3 : t->v2;
	    edges[i].v2 = (t->v2 > t->v3) ? t->v2 : t->v3;
	    i++;
	    edges[i].v1 = (t->v3 > t->v1) ? t->v1 : t->v3;
	    edges[i].v2 = (t->v3 > t->v1) ? t->v3 : t->v1;
	    i++;
	}
	nedges = i;
	qsort(edges, nedges, sizeof(t_edge), compare_edges);
	for (i = 0; i < nedges-1; i++) {
	    if (edges[i].v1 == edges[i+1].v1 && edges[i].v2 == edges[i+1].v2)
		edges[i].v1 = edges[i+1].v1 = -1;
	}

	/* For each edge on the perimeter construct a new triangle
	 * containing the new point as its third vertex.
	 */
	for (i = 0; i < nedges; i++) {
	    if (edges[i].v1 >= 0) {
		t = insert_new_triangle( points, edges[i].v1, edges[i].v2, p - points);
		find_circumcircle(points, t);
	    }
	}
    }

    /* Merge frozen triangles back into the good triangle list */
    if (frozen_tail) {
	frozen_tail->next = good_triangles.next;
	good_triangles.next = frozen_triangles.next;
	frozen_triangles.next = NULL;
	frozen_tail = NULL;
    }

    /* Remove any triangles that contain a vertex of the original "big triangle".
     * The perimeter of the resulting tesselation can be found later by
     * collecting one edge from each of the triangles left in the bad_triangles list.
     */
    delete_triangles(&bad_triangles);
    prev = &good_triangles;
    for (t = prev->next; t; t = prev->next) {
	if ( (t->v1 >= N) || (t->v2 >= N) || (t->v3 >= N) )
	    invalidate_triangle(prev, t);
	else
	    prev = t;
    }

    /* Flag the original points that lie on the bounding hull.
     * The edges making up the hull remain in bounding_edges[]
     * for use by a subsequent call to concave_hull().
     */
    for (i = 0; i < N; i++)
	points[i].extra = 0;
    for (i = 0, t = bad_triangles.next; t; t = t->next)
	i++;
    edges = gp_realloc(edges, 3*i * sizeof(t_edge), "delaunay edges");
    max_bounding_edges = 3*i;
    nedges = 0;
    for (t = bad_triangles.next; t; t = t->next) {
	int v1, v2;
	v1 = (t->v1 >= N) ? t->v3 : t->v1;
	v2 = (t->v2 >= N) ? t->v3 : t->v2;
	if ((v1 == v2) || (v1 >= N) || (v2 >= N))
	    continue;
	edges[nedges].v1 = v1;
	edges[nedges].v2 = v2;
	points[v1].extra = HULL_POINT;
	points[v2].extra = HULL_POINT;
	edges[nedges].length = edge_length( &edges[nedges], plot->points );
	nedges++;
    }
    bounding_edges = edges;
    n_bounding_edges = nedges;

}

static void
invalidate_triangle( triangle *prev, triangle *this )
{
    prev->next = this->next;
    this->next = bad_triangles.next;
    bad_triangles.next = this;
}

static void
freeze_triangle( triangle *prev, triangle *this )
{
    prev->next = this->next;
    this->next = frozen_triangles.next;
    frozen_triangles.next = this;
}

static int
compare_edges(SORTFUNC_ARGS e1, SORTFUNC_ARGS e2)
{
    t_edge *t1 = (t_edge *)e1;
    t_edge *t2 = (t_edge *)e2;

    if (t1->v1 > t2->v1)
	return 1;
    if (t1->v1 < t2->v1)
	return -1;
    if (t1->v2 > t2->v2)
	return 1;
    if (t1->v2 < t2->v2)
	return -1;
    return 0;
}

static triangle *
insert_new_triangle( struct coordinate *points, int v1, int v2, int v3 )
{
    triangle *t = gp_alloc( sizeof(triangle), "triangle" );

    if (v1 < v2 && v1 < v3) {
	t->v1 = v1;
	t->v2 = (v2 < v3) ? v2 : v3;
	t->v3 = (v2 < v3) ? v3 : v2;
    } else if (v2 < v1 && v2 < v3) {
	t->v1 = v2;
	t->v2 = (v1 < v3) ? v1 : v3;
	t->v3 = (v1 < v3) ? v3 : v1;
    } else {
	t->v1 = v3;
	t->v2 = (v1 < v2) ? v1 : v2;
	t->v3 = (v1 < v2) ? v2 : v1;
    }
	
    t->next = good_triangles.next;
    good_triangles.next = t;

    return t;
}

static void
find_circumcircle( struct coordinate *points, triangle *t)
{
    double BB_x, BB_y, CC_x, CC_y, DD;
    double cent_x, cent_y;

    BB_x = points[t->v2].x - points[t->v1].x;
    BB_y = points[t->v2].y - points[t->v1].y;
    CC_x = points[t->v3].x - points[t->v1].x;
    CC_y = points[t->v3].y - points[t->v1].y;

    DD = 2. * (BB_x * CC_y - BB_y * CC_x);
    cent_x = CC_y * (BB_x*BB_x + BB_y*BB_y) - BB_y * (CC_x*CC_x + CC_y*CC_y);
    cent_x /= DD;
    cent_y = BB_x * (CC_x*CC_x + CC_y*CC_y) - CC_x * (BB_x*BB_x + BB_y*BB_y);
    cent_y /= DD;

    t->r = sqrt( cent_x * cent_x + cent_y * cent_y );
    t->cx = cent_x + points[t->v1].x;
    t->cy = cent_y + points[t->v1].y;
}

static TBOOLEAN
in_circumcircle( triangle *t, double x, double y )
{
    if ((t->cx - x)*(t->cx - x) + (t->cy - y)*(t->cy - y) < (t->r * t->r))
	return TRUE;
    else
	return FALSE;
}

static void
delete_triangles( triangle *list_head )
{
    triangle *next;
    triangle *t = list_head->next;

    while (t) {
	next = t->next;
	free(t);
	t = next;
    }
    list_head->next = NULL;
}

static int
compare_edgelength(SORTFUNC_ARGS e1, SORTFUNC_ARGS e2)
{
    if (((t_edge *)e1)->length > ((t_edge *)e2)->length)
	return -1;
    if (((t_edge *)e1)->length < ((t_edge *)e2)->length)
	return 1;
    return 0;
}

/* Adjust current perimeter of triangulated points by incrementally
 * removing single edges to reveal a portion of the interior.
 * Assume
 *	plot->points contains the data points
 *	bounding_edges[] contains the convex hull
 *	l_chi is the characteristic length determining how far we trim
 * Iterative algorithm
 *	Sort bounding edges by length, i.e. bounding_edges[0] is the longest
 *	If there is no removable edge with length > l_chi, stop.
 *	Check whether longest edge belongs to a triangle with 3 exterior points
 *	If so
 *          mark edge as non-removable and continue
 *	If not
 *          Remove it to expose two new edges
 *          Add the two newly exposed edges to the perimeter.
 *          Remove triangle from good_triangles list.
 * Note
 *	On exit some lengths in bounding_edges have been overwritten with -1
 */
static void
chi_reduction( struct curve_points *plot, double l_chi )
{
    /* Each edge removed exposes two others. We will increase max as necessary. */
    bounding_edges = gp_realloc( bounding_edges, 2*n_bounding_edges*sizeof(t_edge),
				"bounding_edges");
    max_bounding_edges = 2*n_bounding_edges;

    qsort(bounding_edges, n_bounding_edges, sizeof(t_edge), compare_edgelength);
    while (dig_one_hole( plot, l_chi )) {
	qsort(bounding_edges, n_bounding_edges, sizeof(t_edge), compare_edgelength);
    }

}

static double
edge_length( t_edge *edge, struct coordinate *p )
{
    double dx = p[edge->v1].x - p[edge->v2].x;
    double dy = p[edge->v1].y - p[edge->v2].y;
    return sqrt(dx*dx + dy*dy);
}

static void
keep_edge( int i, int v1, int v2, struct coordinate *points)
{
    bounding_edges[i].v1 = v1;
    bounding_edges[i].v2 = v2;
    bounding_edges[i].length = edge_length(&bounding_edges[i], points);
}

static TBOOLEAN
dig_one_hole( struct curve_points *plot, double l_chi )
{
    struct coordinate *p = plot->points;
    triangle *t, *prev;

    if (bounding_edges[0].length <= l_chi)
	return FALSE;

    /* Find the triangle this edge belongs to */
    prev = &good_triangles;
    for (t = prev->next; t; t = t->next) {
	TBOOLEAN hit = FALSE;
	if (bounding_edges[0].v1 == t->v1) {
	    if (bounding_edges[0].v2 == t->v2) {
		hit = TRUE;
		if (p[t->v3].extra == HULL_POINT) {
		    bounding_edges[0].length = -1;	/* Flag edge as not removeable */
		} else {
		    p[t->v3].extra = HULL_POINT;	/* Mark uncovered point exterior */
		    keep_edge(0, t->v1, t->v3, p);
		    keep_edge(n_bounding_edges, t->v2, t->v3, p);
		    n_bounding_edges++;
		    invalidate_triangle(prev, t);
		}
	    } else if (bounding_edges[0].v2 == t->v3) {
		hit = TRUE;
		if (p[t->v2].extra == HULL_POINT) {
		    bounding_edges[0].length = -1;	/* Flag edge as not removeable */
		} else {
		    p[t->v2].extra = HULL_POINT;	/* Mark uncovered point exterior */
		    keep_edge(0, bounding_edges[0].v1, t->v2, p);
		    keep_edge( n_bounding_edges, t->v2, t->v3, p);
		    n_bounding_edges++;
		    invalidate_triangle(prev, t);
		}
	    }
	} else if (bounding_edges[0].v1 == t->v2) {
	    if (bounding_edges[0].v2 == t->v3) {
		hit = TRUE;
		if (p[t->v1].extra == HULL_POINT) {
		    bounding_edges[0].length = -1;	/* Flag edge as not removeable */
		} else {
		    p[t->v1].extra = HULL_POINT;	/* Mark uncovered point exterior */
		    keep_edge(0, t->v1, t->v2, p);
		    keep_edge(n_bounding_edges, t->v1, t->v3, p);
		    n_bounding_edges++;
		    invalidate_triangle(prev, t);
		}
	    }
	}

	/* Replaced old perimeter segment with two new ones */
	if (hit) {
	    if (n_bounding_edges >= max_bounding_edges) {
		max_bounding_edges *= 2;
		bounding_edges = gp_realloc(bounding_edges, max_bounding_edges * sizeof(t_edge),
					"bounding_edges");
	    }
	    return(TRUE);
	}
	/* Otherwise continue to search for triangle containing this edge */
	prev = t;
    }

    return(FALSE);
}


void
concave_hull( struct curve_points *plot )
{
    udvt_entry *udv;
    int prev;
    struct coordinate *newpoints;
    double chi_length = 0.0;

    /* Construct χ-shape */
    udv = get_udv_by_name("chi_length");
    if (udv && udv->udv_value.type == CMPLX)
	chi_length = real(&(udv->udv_value));

    /* No chi_length given, make a guesstimate */
    if (chi_length <= 0) {
	for (int ie = 0; ie < n_bounding_edges; ie++) {
	    if (chi_length < bounding_edges[ie].length)
		chi_length = bounding_edges[ie].length;
	}
	chi_length *= chi_shape_default_length;
    }

    chi_reduction( plot, chi_length );
    fill_gpval_float("GPVAL_CHI_LENGTH", chi_length);

    /* Replace original points with perimeter points as a closed curve.
     * This wipes out bounding_edges as it goes.
     */
    newpoints = gp_alloc((n_bounding_edges + 1) * sizeof(struct coordinate), "concave hull");
    newpoints[0] = plot->points[ bounding_edges[0].v1 ];
    newpoints[1] = plot->points[ bounding_edges[0].v2 ];
    prev = bounding_edges[0].v2;
    bounding_edges[0].v1 = bounding_edges[0].v2 = -1;
    for (int n = 2; n < n_bounding_edges; n++) {
	for (int i = 1; i < n_bounding_edges; i++) {
	    if (prev == bounding_edges[i].v1)
		prev = bounding_edges[i].v2;
	    else if (prev == bounding_edges[i].v2)
		prev = bounding_edges[i].v1;
	    else /* Not a match; keep looking */
		continue;
	    newpoints[n] = plot->points[prev];
	    bounding_edges[i].v1 = bounding_edges[i].v2 = -1;
	    break;
	}
    }
    newpoints[n_bounding_edges] = newpoints[0];

    cp_extend(plot, 0);
    plot->p_max = n_bounding_edges+1;
    plot->p_count = n_bounding_edges+1;
    plot->points = newpoints;
}

/*
 * filter option "delaunay"
 *	plot POINTS using 1:2 delaunay with polygons fs empty
 * Replace original list of points with a polygon representation
 * of the individual triangles.
 *
 * This routine was written to help debug implementation of χ-shapes
 * to generate a concave hull.  See "concave_hull.dem".
 * It is currently undocumented as a separate filter option.
 */
void
save_delaunay_triangles( struct curve_points *plot )
{
    struct coordinate *point = plot->points;
    struct coordinate *newpoints = NULL;
    double *newcolor = NULL;
    triangle *t;
    int outp;

    /* Reserve space to store each triangle as three points
     * plus one "empty" spacer.
     */
    for (outp = 0, t = good_triangles.next; t; t = t->next)
	outp++;
    newpoints = gp_alloc( 5 * outp * sizeof(struct coordinate), "delaunay filter" );

    /* Copy full original point so that it retains any extra fields */
    for (outp = 0, t = good_triangles.next; t; t = t->next) {
	newpoints[outp++] = point[t->v1];
	newpoints[outp++] = point[t->v2];
	newpoints[outp++] = point[t->v3];
	newpoints[outp++] = point[t->v1];
	newpoints[outp++] = blank_data_line;
    }

    /* Make a new separate list of colors */
    if (plot->varcolor) {
	newcolor = gp_alloc( 5 * outp * sizeof(double), "delaunay colors" );
	for (int i = 0; i < outp; i++)
	    newcolor[i] = newpoints[i].CRD_COLOR;
    }

    /* Replace original point list with the new one */
    cp_extend(plot, 0);
    plot->points = newpoints;
    plot->varcolor = newcolor;
    plot->p_count = outp;
    plot->p_max = outp;
}

#endif /* WITH_CHI_SHAPES */
