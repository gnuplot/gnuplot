/* GNUPLOT - filters.c */

/*[
 * Copyright Ethan A Merritt 2013 - 2022
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

/* local prototypes */

static int do_curve_cleanup(struct coordinate *point, int npoints);
static void winnow_interior_points (struct curve_points *plot);
/*
 * EXPERIMENTAL
 * find outliers (distance from centroid > FOO * rmsd) in a cluster of points
 */
static int find_cluster_outliers (struct coordinate *points, t_cluster *cluster);


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
	/* FIXME:  simpler test for outrange would be sufficient */
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
    plot->points = gp_realloc( plot->points, nbins * sizeof(struct coordinate), "curve_points");
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
    splined_points = gp_alloc( (samples_1 * curves) * sizeof(struct coordinate), NULL );
    memset( splined_points, 0, (samples_1 * curves) * sizeof(struct coordinate));

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
 * winnow_interior_points() is a helper routine that can greatly reduce
 * processing time for large data sets but is otherwise not necessary.
 * expand_hull() pushes the hull points away from the centroid for the
 *	purpose of drawing a smooth bounding curve.
 * - Ethan A Merritt 2021
 *
 * find_cluster_outliers() calculates the centroid of plot->points,
 *	and if cluster_outlier_threshold > 0 calculates the rmsd distance
 *	of all points to the centroid so that outliers can be flagged.
 * - Ethan A Merritt 2022
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

    /* Special cases */
    if (np < 3)
	return;
    if (np == 3) {
	points = gp_realloc( points, 4*sizeof(struct coordinate), "HULL" );
	points[3] = points[0];
	plot->points = points;
	plot->p_count = 4;
	return;
    }

    /* Find centroid (used by "smooth convexhull").
     * Flag and remove outliers.
     */
    if (plot->plot_filter == FILTER_CONVEX_HULL) {
	t_cluster cluster;
	int outliers = 0;
	/* FIXME:  Needs a set option or a plot keyword to choose threshold */
	if (debug)
	    cluster.threshold = 3.0;
	else
	    cluster.threshold = 0.0;
	cluster.npoints = plot->p_count;
	do {
	    outliers = find_cluster_outliers(plot->points, &cluster);
	    plot->filledcurves_options.at = cluster.cx;
	    plot->filledcurves_options.aty = cluster.cy;
	} while (outliers > 0);
    }

    /* This is not strictly necessary, but greatly reduces the number
     * of points to be sorted and tested for the hull boundary.
     */
    winnow_interior_points(plot);

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
    free(plot->points);
    free(stack);
    plot->points = points;
    plot->p_count = np;
}
#undef CROSS

/*
 * winnow_interior_points() is an optional helper routine for convex_hull.
 * It reduces the number of points to be sorted and processed by removing
 * points in a quadrilateral bounded by the four points with max/min x/y.
 */
static void
winnow_interior_points (struct curve_points *plot)
{
#define TOLERANCE -1.e-10

    struct coordinate *p, *pp1, *pp2, *pp3, *pp4;
    double xmin = VERYLARGE, xmax = -VERYLARGE;
    double ymin = VERYLARGE, ymax = -VERYLARGE;
    struct coordinate *points = plot->points;
    double area;
    int i, np;

    /* Find maximal extent on x and y, centroid */
    pp1 = pp2 = pp3 = pp4 = plot->points;
    for (p = plot->points; p < &(plot->points[plot->p_count]); p++) {
	if (p->type == EXCLUDEDRANGE)	/* e.g. outlier or category mismatch */
	    continue;
	if (p->x < xmin) { xmin = p->x; pp1 = p; }
	if (p->x > xmax) { xmax = p->x; pp3 = p; }
	if (p->y < ymin) { ymin = p->y; pp4 = p; }
	if (p->y > ymax) { ymax = p->y; pp2 = p; }
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
	if (points[i].type != EXCLUDEDRANGE)
	    p[np++] = points[i];
    }
    plot->p_count = np;

#undef TOLERANCE
}

/*
 * expand_hull() "inflates" a smooth convex hull by pushing each
 * perimeter point further away from the centroid of the bounded points.
 * FIXME: The expansion value is currently interpreted as a scale
 *        factor; i.e. 1.0 is no expansion.  It might be better to
 *	  instead accept a fixed value in user coordinates.
 */
void
expand_hull(struct curve_points *plot)
{
    struct coordinate *points = plot->points;
    double xcent = plot->filledcurves_options.at;
    double ycent = plot->filledcurves_options.aty;
    double scale = plot->smooth_parameter;
    int i;

    if (scale <=0 || scale == 1.0)
	return;

    for (i=0; i<plot->p_count; i++) {
	points[i].x = xcent + scale * (points[i].x - xcent);
	points[i].y = ycent + scale * (points[i].y - ycent);
    }

}

/*
 * Find center of mass of plot->points.
 * All points not marked EXCLUDEDRANGE are assumed to be in a single cluster.
 * If a threshold has been set for cluster outlier detection, then flag
 * any points whose distance from the center of mass is greater than
 * threshold * cluster.rmsd.
 * Return the number of outliers found.
 */
static int
find_cluster_outliers( struct coordinate *points, t_cluster *cluster )
{
    double xsum = 0;
    double ysum = 0;
    double d2 = 0;
    int i;
    int outliers = 0;

    for (i = 0; i < cluster->npoints; i++) {
	xsum += points[i].x;
	ysum += points[i].y;
    }
    cluster->cx = xsum / cluster->npoints;
    cluster->cy = ysum / cluster->npoints;

    if (cluster->threshold <= 0)
	return 0;

    for (i = 0; i < cluster->npoints; i++) {
	if (points[i].type == EXCLUDEDRANGE)
	    continue;
	d2 += (points[i].x - cluster->cx) * (points[i].x - cluster->cx)
	    + (points[i].y - cluster->cy) * (points[i].y - cluster->cy);
    }
    cluster->rmsd = sqrt(d2 / cluster->npoints);
    for (i = 0; i < cluster->npoints; i++) {
	if (points[i].type == EXCLUDEDRANGE)
	    continue;
	d2 = (points[i].x - cluster->cx) *(points[i].x - cluster->cx)
	   + (points[i].y - cluster->cy) *(points[i].y - cluster->cy);
	if (sqrt(d2) > cluster->rmsd * cluster->threshold) {
	    points[i].type = EXCLUDEDRANGE;
	    outliers++;
	}
    }

    FPRINTF((stderr, "find_cluster_outliers: rmsd = %g, %d outliers\n",
	    cluster->rmsd, outliers));
    return outliers;
}

