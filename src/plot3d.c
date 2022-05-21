/* GNUPLOT - plot3d.c */

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

#include "plot3d.h"
#include "gp_types.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "datablock.h"
#include "encoding.h"
#include "eval.h"
#include "getcolor.h"
#include "graph3d.h"
#include "hidden3d.h"
#include "misc.h"
#include "parse.h"
#include "pm3d.h"
#include "setshow.h"
#include "term_api.h"
#include "tabulate.h"
#include "util.h"
#include "variable.h" /* For locale handling */
#include "voxelgrid.h"

#include "plot2d.h" /* Only for store_label() */

#include "matrix.h" /* Used by thin-plate-splines in dgrid3d */

#include "help.h"

/* global variables exported by this module */

t_data_mapping mapping3d = MAP3D_CARTESIAN;

int dgrid3d_row_fineness = 10;
int dgrid3d_col_fineness = 10;
int dgrid3d_norm_value = 1;
int dgrid3d_mode = DGRID3D_QNORM;
double dgrid3d_x_scale = 1.0;
double dgrid3d_y_scale = 1.0;
TBOOLEAN dgrid3d = FALSE;
TBOOLEAN dgrid3d_kdensity = FALSE;
double boxdepth = 0.0;

/* static prototypes */

static void calculate_set_of_isolines(AXIS_INDEX value_axis, TBOOLEAN cross, struct iso_curve **this_iso,
				      AXIS_INDEX iso_axis, double iso_min, double iso_step, int num_iso_to_use,
				      AXIS_INDEX sam_axis, double sam_min, double sam_step, int num_sam_to_use
					       );
static int get_3ddata(struct surface_points * this_plot);
static void eval_3dplots(void);
static void grid_nongrid_data(struct surface_points * this_plot);
static void parametric_3dfixup(struct surface_points * start_plot, int *plot_num);
static struct surface_points * sp_alloc(int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2);
static void sp_replace(struct surface_points *sp, int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2);

static struct iso_curve * iso_alloc(int num);
static void iso_extend(struct iso_curve *ip, int num);
static void iso_free(struct iso_curve *ip);

/* helper functions for grid_nongrid_data() */
static double splines_kernel(double h);
static void thin_plate_splines_setup( struct iso_curve *old_iso_crvs, double **p_xx, int *p_numpoints );
static double qnorm( double dist_x, double dist_y, int q );
static double pythag( double dx, double dy );

/* helper function to detect empty data sets */
static void count_3dpoints(struct surface_points *plot, int *nt, int *ni, int *nu);

/* helper functions for parsing */
static void load_contour_label_options(struct text_label *contour_label);

typedef enum splot_component {
	SP_FUNCTION, SP_DATAFILE, SP_DATABLOCK, SP_KEYENTRY, SP_VOXELGRID
} splot_component;

/* the curves/surfaces of the plot */
struct surface_points *first_3dplot = NULL;
static struct udft_entry plot_func;

int plot3d_num=0;

/* FIXME:
 * Because this is global, it gets clobbered if there is more than
 * one unbounded iteration in the splot command, e.g.
 *	splot for [i=0:*] A index i, for [j=0:*] B index j
 * Moving it into (struct surface_points) would be nice but would require
 * extra bookkeeping to track which plot header it is stored in.
 */
static int last_iteration_in_first_pass = INT_MAX;

/*
 * sp_alloc() allocates a surface_points structure that can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
static struct surface_points *
sp_alloc(int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2)
{
    struct lp_style_type default_lp_properties = DEFAULT_LP_STYLE_TYPE;

    struct surface_points *sp = gp_alloc(sizeof(*sp), "surface");
    memset(sp,0,sizeof(struct surface_points));

    /* Initialize various fields */
    sp->lp_properties = default_lp_properties;
    sp->fill_properties = default_fillstyle;
    if (sp->fill_properties.fillstyle == FS_EMPTY)
	sp->fill_properties.fillstyle = FS_SOLID;
    default_arrow_style(&(sp->arrow_properties));

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	int i;
	struct iso_curve *icrv;

	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    }

    return (sp);
}

/*
 * sp_replace() updates a surface_points structure so it can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
static void
sp_replace(
    struct surface_points *sp,
    int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2)
{
    int i;
    struct iso_curve *icrv, *icrvs = sp->iso_crvs;

    while (icrvs) {
	icrv = icrvs;
	icrvs = icrvs->next;
	iso_free(icrv);
    }
    sp->iso_crvs = NULL;

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = NULL;
}

/*
 * sp_free() releases any memory which was previously malloc()'d to hold
 *   surface points.
 */
/* HBB 20000506: don't risk stack havoc by recursion, use iterative list
 * cleanup instead */
void
sp_free(struct surface_points *sp)
{
    while (sp) {
	struct surface_points *next = sp->next_sp;
	free(sp->title);
	free(sp->title_position);
	sp->title_position = NULL;

	while (sp->contours) {
	    struct gnuplot_contours *next_cntrs = sp->contours->next;

	    free(sp->contours->coords);
	    free(sp->contours);
	    sp->contours = next_cntrs;
	}

	while (sp->iso_crvs) {
	    struct iso_curve *next_icrvs = sp->iso_crvs->next;

	    iso_free(sp->iso_crvs);
	    sp->iso_crvs = next_icrvs;
	}

	if (sp->labels) {
	    free_labels(sp->labels);
	    sp->labels = NULL;
	}

	free(sp);
	sp = next;
    }
}

/*
 * iso_alloc() allocates a iso_curve structure that can hold 'num'
 * points.
 */
struct iso_curve *
iso_alloc(int num)
{
    struct iso_curve *ip;
    ip = gp_alloc(sizeof(struct iso_curve), "iso curve");
    ip->p_max = (num >= 0 ? num : 0);
    ip->p_count = 0;
    if (num > 0) {
	ip->points = gp_alloc(num * sizeof(struct coordinate), "iso curve points");
	memset(ip->points, 0, num * sizeof(struct coordinate));
    } else
	ip->points = (struct coordinate *) NULL;
    ip->next = NULL;
    return (ip);
}

/*
 * iso_extend() reallocates a iso_curve structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void
iso_extend(struct iso_curve *ip, int num)
{
    if (num == ip->p_max)
	return;

    if (num > 0) {
	ip->points = gp_realloc(ip->points, num * sizeof(struct coordinate),
				"expanding 3D points");
	if (num > ip->p_max)
	    memset( &(ip->points[ip->p_max]), 0, (num - ip->p_max) * sizeof(struct coordinate));
	ip->p_max = num;
    } else {
	free(ip->points);
	ip->points = NULL;
	ip->p_max = 0;
    }
}

/*
 * iso_free() releases any memory which was previously malloc()'d to hold
 *   iso curve points.
 */
void
iso_free(struct iso_curve *ip)
{
    if (ip)
	free(ip->points);
    free(ip);
}


void
plot3drequest()
/*
 * in the parametric case we would say splot [u= -Pi:Pi] [v= 0:2*Pi] [-1:1]
 * [-1:1] [-1:1] sin(v)*cos(u),sin(v)*cos(u),sin(u) in the non-parametric
 * case we would say only splot [x= -2:2] [y= -5:5] sin(x)*cos(y)
 *
 */
{
    int dummy_token0 = -1, dummy_token1 = -1;
    AXIS_INDEX axis, u_axis, v_axis;

    is_3d_plot = TRUE;

    if (parametric && strcmp(set_dummy_var[0], "t") == 0) {
	strcpy(set_dummy_var[0], "u");
	strcpy(set_dummy_var[1], "v");
    }

    /* initialize the arrays from the 'set' scalars */
    axis_init(&axis_array[FIRST_X_AXIS], FALSE);
    axis_init(&axis_array[FIRST_Y_AXIS], FALSE);
    axis_init(&axis_array[FIRST_Z_AXIS], TRUE);
    axis_init(&axis_array[U_AXIS], FALSE);
    axis_init(&axis_array[V_AXIS], FALSE);
    axis_init(&axis_array[COLOR_AXIS], TRUE);
    if (splot_map) {
	axis_init(&axis_array[SECOND_X_AXIS], FALSE);
	axis_init(&axis_array[SECOND_Y_AXIS], FALSE);
    }

    /* Always be prepared to restore the autoscaled values on "refresh"
     * Dima Kogan April 2018
     * FIXME: Could we fold this into axis_init?
     */
    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	AXIS *this_axis = &axis_array[axis];
	if (this_axis->set_autoscale != AUTOSCALE_NONE)
	    this_axis->range_flags |= RANGE_WRITEBACK;
    }

    /* Nonlinear mapping of x or y via linkage to a hidden primary axis. */
    /* The user set autoscale for the visible axis; apply it also to the hidden axis. */
    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	AXIS *secondary = &axis_array[axis];
	if (axis == SAMPLE_AXIS)
	    continue;
	if (secondary->linked_to_primary
 	&&  secondary->linked_to_primary->index == -secondary->index) {
	    AXIS *primary = secondary->linked_to_primary;
	    primary->set_autoscale = secondary->set_autoscale;
	    axis_init(primary, 1);
	}
    }

    if (!term)			/* unknown */
	int_error(c_token, "use 'set term' to set terminal type first");

    /* Range limits for the entire plot are optional but must be given	*/
    /* in a fixed order. The keyword 'sample' terminates range parsing.	*/
    u_axis = (parametric ? U_AXIS : FIRST_X_AXIS);
    v_axis = (parametric ? V_AXIS : FIRST_Y_AXIS);

    dummy_token0 = parse_range(u_axis);
    dummy_token1 = parse_range(v_axis);

    if (parametric) {
	parse_range(FIRST_X_AXIS);
	parse_range(FIRST_Y_AXIS);
    }
    parse_range(FIRST_Z_AXIS);
    check_axis_reversed(FIRST_X_AXIS);
    check_axis_reversed(FIRST_Y_AXIS);
    check_axis_reversed(FIRST_Z_AXIS);
    if (equals(c_token,"sample") && equals(c_token+1,"["))
	c_token++;

    /* Clear out any tick labels read from data files in previous plot */
    for (axis=0; axis<AXIS_ARRAY_SIZE; axis++) {
	struct ticdef *ticdef = &axis_array[axis].ticdef;
	if (ticdef->def.user)
	    ticdef->def.user = prune_dataticks(ticdef->def.user);
	if (!ticdef->def.user && ticdef->type == TIC_USER)
	    ticdef->type = TIC_COMPUTED;
    }

    /* use the default dummy variable unless changed */
    if (dummy_token0 > 0)
	copy_str(c_dummy_var[0], dummy_token0, MAX_ID_LEN);
    else
	strcpy(c_dummy_var[0], set_dummy_var[0]);

    if (dummy_token1 > 0)
	copy_str(c_dummy_var[1], dummy_token1, MAX_ID_LEN);
    else
	strcpy(c_dummy_var[1], set_dummy_var[1]);

    /* In "set view map" mode the x2 and y2 axes are legal */
    /* but must be linked to the respective primary axis. */
    if (splot_map) {
	if ((axis_array[SECOND_X_AXIS].ticmode && !axis_array[SECOND_X_AXIS].linked_to_primary)
	||  (axis_array[SECOND_Y_AXIS].ticmode && !axis_array[SECOND_Y_AXIS].linked_to_primary))
	    int_error(NO_CARET,
		"Secondary axis must be linked to primary axis in order to draw tics");
    }

    eval_3dplots();
}


/* Helper function for refresh command.  Reexamine each data point and update the
 * flags for INRANGE/OUTRANGE/UNDEFINED based on the current limits for that axis.
 * Normally the axis limits are already known at this point. But if the user has
 * forced "set autoscale" since the previous plot or refresh, we need to reset the
 * axis limits and try to approximate the full auto-scaling behaviour.
 */
void
refresh_3dbounds(struct surface_points *first_plot, int nplots)
{
    struct surface_points *this_plot = first_plot;
    int iplot;		/* plot index */

    for (iplot = 0;  iplot < nplots; iplot++, this_plot = this_plot->next_sp) {
	int i;		/* point index */
	struct axis *x_axis = &axis_array[FIRST_X_AXIS];
	struct axis *y_axis = &axis_array[FIRST_Y_AXIS];
	struct axis *z_axis = &axis_array[FIRST_Z_AXIS];
	struct iso_curve *this_curve;

	/* IMAGE clipping is done elsewhere, so we don't need INRANGE/OUTRANGE checks */
	if (this_plot->plot_style == IMAGE 
	||  this_plot->plot_style == RGBIMAGE
	||  this_plot->plot_style == RGBA_IMAGE) {
	    if (x_axis->set_autoscale)
		process_image(this_plot, IMG_UPDATE_AXES);
	    continue;
	}

	for (this_curve = this_plot->iso_crvs; this_curve; this_curve = this_curve->next) {

	    /* VECTOR plots consume two ico_crvs structures, one for heads and one for tails.
	     * Only the first one has the true point count; the second one claims zero.
	     * FIXME: Maybe we should change this?
	     */
	    int n_points;
	    if (this_plot->plot_style == VECTOR)
		n_points = this_plot->iso_crvs->p_count;
	    else
		n_points = this_curve->p_count;

	    for (i=0; i<n_points; i++) {
		struct coordinate *point = &this_curve->points[i];

		if (point->type == UNDEFINED)
		    continue;
		else
		    point->type = INRANGE;

		/* If the state has been set to autoscale since the last plot,
		 * mark everything INRANGE and re-evaluate the axis limits now.
		 * Otherwise test INRANGE/OUTRANGE against previous axis limits.
		 */

		/* This autoscaling logic is parallel to that in
		 * refresh_bounds() in plot2d.c
		 */
		if (!this_plot->noautoscale) {
		    autoscale_one_point(x_axis, point->x);
		    autoscale_one_point(y_axis, point->y);
		}
		if (!inrange(point->x, x_axis->min, x_axis->max)
		||  !inrange(point->y, y_axis->min, y_axis->max)) {
		    point->type = OUTRANGE;
		    continue;
		}
		if (!this_plot->noautoscale) {
		    autoscale_one_point(z_axis, point->z);
		}
		if (!inrange(point->z, z_axis->min, z_axis->max)) {
		    point->type = OUTRANGE;
		    continue;
		}

	    }	/* End of points in this curve */
	}   /* End of curves in this plot */

    }	/* End of plots in this splot command */

    /* handle 'reverse' ranges */
    axis_check_range(FIRST_X_AXIS);
    axis_check_range(FIRST_Y_AXIS);
    axis_check_range(FIRST_Z_AXIS);

    /* Make sure the bounds are reasonable, and tweak them if they aren't */
    axis_checked_extend_empty_range(FIRST_X_AXIS, NULL);
    axis_checked_extend_empty_range(FIRST_Y_AXIS, NULL);
    axis_checked_extend_empty_range(FIRST_Z_AXIS, NULL);
}


static double
splines_kernel(double h)
{
    if (h > 0.0) { return h * h * log(h); }
    return 0.0;
}

/* PKJ:
   This function has been hived off out of the original grid_nongrid_data().
   No changes have been made, but variables only needed locally have moved
   out of grid_nongrid_data() into this function. */
static void 
thin_plate_splines_setup( struct iso_curve *old_iso_crvs,
			  double **p_xx, int *p_numpoints )
{
    int i, j, k;
    double *xx, *yy, *zz, *b, **K, d;
    int numpoints, *indx;
    struct iso_curve *oicrv;
    
    numpoints = 0;
    for (oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
	numpoints += oicrv->p_count;
    }
    xx = gp_alloc(sizeof(xx[0]) * (numpoints + 3) * (numpoints + 8),
		  "thin plate splines in dgrid3d");
    /* the memory needed is not really (n+3)*(n+8) for now,
       but might be if I take into account errors ... */
    K = gp_alloc(sizeof(K[0]) * (numpoints + 3),
		 "matrix : thin plate splines 2d");
    yy = xx + numpoints;
    zz = yy + numpoints;
    b = zz + numpoints;
    
    /* HBB 20010424: Count actual input points without the UNDEFINED
     * ones, as we copy them */
    numpoints = 0;
    for (oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
	struct coordinate *opoints = oicrv->points;
	
	for (k = 0; k < oicrv->p_count; k++, opoints++) {
	    /* HBB 20010424: avoid crashing for undefined input */
	    if (opoints->type == UNDEFINED)
		continue;
	    xx[numpoints] = opoints->x;
	    yy[numpoints] = opoints->y;
	    zz[numpoints] = opoints->z;
	    numpoints++;
	}
    }
    
    for (i = 0; i < numpoints + 3; i++) {
	K[i] = b + (numpoints + 3) * (i + 1);
    }
    
    for (i = 0; i < numpoints; i++) {
	for (j = i + 1; j < numpoints; j++) {
	    double dx = xx[i] - xx[j], dy = yy[i] - yy[j];
	    K[i][j] = K[j][i] = -splines_kernel(sqrt(dx * dx + dy * dy));
	}
	K[i][i] = 0.0;		/* here will come the weights for errors */
	b[i] = zz[i];
    }
    for (i = 0; i < numpoints; i++) {
	K[i][numpoints] = K[numpoints][i] = 1.0;
	K[i][numpoints + 1] = K[numpoints + 1][i] = xx[i];
	K[i][numpoints + 2] = K[numpoints + 2][i] = yy[i];
    }
    b[numpoints] = 0.0;
    b[numpoints + 1] = 0.0;
    b[numpoints + 2] = 0.0;
    K[numpoints][numpoints] = 0.0;
    K[numpoints][numpoints + 1] = 0.0;
    K[numpoints][numpoints + 2] = 0.0;
    K[numpoints + 1][numpoints] = 0.0;
    K[numpoints + 1][numpoints + 1] = 0.0;
    K[numpoints + 1][numpoints + 2] = 0.0;
    K[numpoints + 2][numpoints] = 0.0;
    K[numpoints + 2][numpoints + 1] = 0.0;
    K[numpoints + 2][numpoints + 2] = 0.0;
    indx = gp_alloc(sizeof(indx[0]) * (numpoints + 3), "indexes lu");
    /* actually, K is *not* positive definite, but
       has only non zero real eigenvalues ->
       we can use an lu_decomp safely */
    lu_decomp(K, numpoints + 3, indx, &d);
    lu_backsubst(K, numpoints + 3, indx, b);
    
    free( K );
    free( indx );
    
    *p_xx = xx;
    *p_numpoints = numpoints;
}

static double
qnorm( double dist_x, double dist_y, int q ) 
{
    double dist = 0.0;

#if (1)
    switch (q) {
    case 1:
	dist = pythag(dist_x, dist_y);
	break;
    case 2:
	dist = dist_x*dist_x + dist_y*dist_y;
	break;
    default:
	dist = pow( pythag(dist_x, dist_y), q);
	break;
    }
#else	/* old code (versions 3.5 - 5.2) */
    switch (q) {
    case 1:
	dist = dist_x + dist_y;
	break;
    case 2:
	dist = dist_x * dist_x + dist_y * dist_y;
	break;
    case 4:
	dist = dist_x * dist_x + dist_y * dist_y;
	dist *= dist;
	break;
    case 8:
	dist = dist_x * dist_x + dist_y * dist_y;
	dist *= dist;
	dist *= dist;
	break;
    case 16:
	dist = dist_x * dist_x + dist_y * dist_y;
	dist *= dist;
	dist *= dist;
	dist *= dist;
	break;
    default:
	dist = pow(dist_x, (double)q ) + pow(dist_y, (double)q );
	break;
    }
#endif
    return dist;
}

/* This is from Numerical Recipes in C, 2nd ed, p70 */
static double
pythag( double dx, double dy )
{
    double x, y;
    x = fabs(dx);
    y = fabs(dy);
    
    if (x > y) { return x*sqrt(1.0 + (y*y)/(x*x)); }
    if (y==0.0) { return 0.0; }
    return y*sqrt(1.0 + (x*x)/(y*y));
}

static void
grid_nongrid_data(struct surface_points *this_plot)
{
    int i, j, k;
    double x, y, z, w, dx, dy, xmin, xmax, ymin, ymax;
    double c;
    coord_type dummy_type;
    struct iso_curve *old_iso_crvs = this_plot->iso_crvs;
    struct iso_curve *icrv, *oicrv, *oicrvs;
    
    /* these are only needed for thin_plate_splines */
    double *xx = NULL, *yy = NULL, *zz = NULL, *b = NULL;
    int numpoints = 0;

    /* nothing to grid */
    if (this_plot->num_iso_read == 0)
	return;

    /* Version 5.3 - allow gridding of separate color column so long as
     * we don't need to generate splines for it
     */
    if (this_plot->pm3d_color_from_column && dgrid3d_mode == DGRID3D_SPLINES)
	int_error(NO_CARET, "Spline gridding of a separate color column is not implemented");

    /* Compute XY bounding box on the original data. */
    /* FIXME HBB 20010424: Does this make any sense? Shouldn't we just
     * use whatever the x and y ranges have been found to be, and
     * that's that? The largest difference this is going to make is if
     * we plot a datafile that doesn't span the whole x/y range
     * used. Do we want a dgrid3d over the actual data rectangle, or
     * over the xrange/yrange area? */
    xmin = xmax = old_iso_crvs->points[0].x;
    ymin = ymax = old_iso_crvs->points[0].y;
    for (icrv = old_iso_crvs; icrv != NULL; icrv = icrv->next) {
	struct coordinate *points = icrv->points;

	for (i = 0; i < icrv->p_count; i++, points++) {
	    /* HBB 20010424: avoid crashing for undefined input */
	    if (points->type == UNDEFINED)
		continue;
	    if (xmin > points->x)
		xmin = points->x;
	    if (xmax < points->x)
		xmax = points->x;
	    if (ymin > points->y)
		ymin = points->y;
	    if (ymax < points->y)
		ymax = points->y;
	}
    }

    dx = (xmax - xmin) / (dgrid3d_col_fineness - 1);
    dy = (ymax - ymin) / (dgrid3d_row_fineness - 1);

    /* Create the new grid structure, and compute the low pass filtering from
     * non grid to grid structure.
     */
    this_plot->iso_crvs = NULL;
    this_plot->num_iso_read = dgrid3d_col_fineness;
    this_plot->has_grid_topology = TRUE;
    
    if (dgrid3d_mode == DGRID3D_SPLINES) {
	thin_plate_splines_setup( old_iso_crvs, &xx, &numpoints );
	yy = xx + numpoints;
	zz = yy + numpoints;
	b  = zz + numpoints;
    }
    
    for (i = 0, x = xmin; i < dgrid3d_col_fineness; i++, x += dx) {
	struct coordinate *points;

	icrv = iso_alloc(dgrid3d_row_fineness + 1);
	icrv->p_count = dgrid3d_row_fineness;
	icrv->next = this_plot->iso_crvs;
	this_plot->iso_crvs = icrv;
	points = icrv->points;

	for (j=0, y=ymin; j<dgrid3d_row_fineness; j++, y+=dy, points++) {
	    z = w = 0.0;
	    c = 0.0;

	    /* as soon as ->type is changed to UNDEFINED, break out of
	     * two inner loops! */
	    points->type = INRANGE;

	    if (dgrid3d_mode == DGRID3D_SPLINES) {
		z = b[numpoints];
		for (k = 0; k < numpoints; k++) {
		    double dx = xx[k] - x, dy = yy[k] - y;
		    z = z - b[k] * splines_kernel(sqrt(dx * dx + dy * dy));
		}
		z = z + b[numpoints + 1] * x + b[numpoints + 2] * y;
	    } else { /* everything, except splines */
		for (oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
		    struct coordinate *opoints = oicrv->points;
		    for (k = 0; k < oicrv->p_count; k++, opoints++) {

			if (dgrid3d_mode == DGRID3D_QNORM) {
			    double dist = qnorm( fabs(opoints->x - x),
						 fabs(opoints->y - y),
						 dgrid3d_norm_value );

			    if (dist == 0.0) {
				/* HBB 981209:  flag the first undefined z and return */
				points->type = UNDEFINED;
				z = opoints->z;
				c = opoints->CRD_COLOR;
				w = 1.0;
				break;	/* out of inner loop */
			    } else {
				z += opoints->z / dist;
				c += opoints->CRD_COLOR / dist;
				w += 1.0/dist;
			    }

			} else { /* ALL else: not spline, not qnorm! */
			    double weight = 0.0;
			    double dist=pythag((opoints->x-x)/dgrid3d_x_scale, 
					       (opoints->y-y)/dgrid3d_y_scale);

			    if (dgrid3d_mode == DGRID3D_GAUSS) {
				weight = exp( -dist*dist );
			    } else if (dgrid3d_mode == DGRID3D_CAUCHY) {
				weight = 1.0/(1.0 + dist*dist );
			    } else if (dgrid3d_mode == DGRID3D_EXP) {
				weight = exp( -dist );
			    } else if (dgrid3d_mode == DGRID3D_BOX) {
				weight = (dist<1.0) ? 1.0 : 0.0;
			    } else if (dgrid3d_mode == DGRID3D_HANN) {
				if (dist < 1.0)
				    weight = 0.5*(1+cos(M_PI*dist));
			    }
			    z += opoints->z * weight;
			    c += opoints->CRD_COLOR * weight;
			    w += weight;
			}
		    }
		    
		    /* PKJ: I think this is only relevant for qnorm */
		    if (points->type != INRANGE)
			break;	/* out of the second-inner loop as well ... */
		}
	    } /* endif( dgrid3d_mode == DGRID3D_SPLINES ) */
		 
	      /* Now that we've escaped the loops safely, we know that we
	       * do have a good value in z and w, so we can proceed just as
	       * if nothing had happened at all. Nice, isn't it? */
	    points->type = INRANGE;
	    points->x = x;
	    points->y = y;

	    /* Honor requested x and y limits */
	    /* Historical note: This code was not in 4.0 or 4.2. It imperfectly */
	    /* restores the clipping behaviour of version 3.7 and earlier. */
	    if ((x < axis_array[x_axis].min && !(axis_array[x_axis].autoscale & AUTOSCALE_MIN))
	    ||  (x > axis_array[x_axis].max && !(axis_array[x_axis].autoscale & AUTOSCALE_MAX))
	    ||  (y < axis_array[y_axis].min && !(axis_array[y_axis].autoscale & AUTOSCALE_MIN))
	    ||  (y > axis_array[y_axis].max && !(axis_array[y_axis].autoscale & AUTOSCALE_MAX)))
		points->type = OUTRANGE;

	    if (dgrid3d_mode != DGRID3D_SPLINES && !dgrid3d_kdensity) {
		z = z / w;
		c = c / w;
	    }
	    
	    STORE_AND_UPDATE_RANGE(points->z, z, points->type, z_axis,
				   this_plot->noautoscale, continue);
	    
	    if (!this_plot->pm3d_color_from_column)
		c = z;
	    dummy_type = points->type;
	    STORE_AND_UPDATE_RANGE( points->CRD_COLOR, c, dummy_type, COLOR_AXIS, 
				    this_plot->noautoscale, continue);
	}
    }
    
    free(xx); /* safe to call free on NULL pointer if splines not used */
    
    /* Delete the old non grid data. */
    for (oicrvs = old_iso_crvs; oicrvs != NULL;) {
	oicrv = oicrvs;
	oicrvs = oicrvs->next;
	iso_free(oicrv);
    }
}

/* Get 3D data from file, and store into this_plot data
 * structure. Takes care of 'set mapping' and 'set dgrid3d'.
 *
 * Notice: this_plot->token is end of datafile spec, before title etc
 * will be moved past title etc after we return */
static int
get_3ddata(struct surface_points *this_plot)
{
    int xdatum = 0;
    int ydatum = 0;
    int j;
    int pt_in_iso_crv = 0;
    struct iso_curve *this_iso;
    int retval = 0;
    double v[MAXDATACOLS];

    /* Initialize the space that will hold input data values */
    memset(v, 0, sizeof(v));

    if (mapping3d == MAP3D_CARTESIAN) {
	/* do this check only, if we have PM3D / PM3D-COLUMN not compiled in */
	if (df_no_use_specs == 2)
	    int_error(this_plot->token, "Need 1 or 3 columns for cartesian data");
	/* HBB NEW 20060427: if there's only one, explicit using
	 * column, it's z data.  df_axis[] has to reflect that, so
	 * df_readline() will expect time/date input. */
	if (df_no_use_specs == 1)
	    df_axis[0] = FIRST_Z_AXIS;
    } else {
	if (df_no_use_specs == 1)
	    int_error(this_plot->token, "Need 2 or 3 columns for polar data");
    }

    this_plot->num_iso_read = 0;
    this_plot->has_grid_topology = TRUE;
    this_plot->pm3d_color_from_column = FALSE;

    /* we ought to keep old memory - most likely case
     * is a replot, so it will probably exactly fit into
     * memory already allocated ?
     */
    if (this_plot->iso_crvs != NULL) {
	struct iso_curve *icrv, *icrvs = this_plot->iso_crvs;

	while (icrvs) {
	    icrv = icrvs;
	    icrvs = icrvs->next;
	    iso_free(icrv);
	}
	this_plot->iso_crvs = NULL;
    }
    /* data file is already open */

    if (df_matrix)
	this_plot->has_grid_topology = TRUE;

    {
	/*{{{  read surface from text file */
	struct iso_curve *local_this_iso = iso_alloc(samples_1);
	struct coordinate *cp;
	struct coordinate *cphead = NULL; /* Only for VECTOR plots */
	double x, y, z;
	double xlow = 0, xhigh = 0;
	double xtail, ytail, ztail;
	double zlow = VERYLARGE, zhigh = -VERYLARGE;
	double color = VERYLARGE;
	int arrowstyle = 0;
	int pm3d_color_from_column = FALSE;
#define color_from_column(x) pm3d_color_from_column = x

	if (this_plot->plot_style == LABELPOINTS)
	    expect_string( 4 );

	if (this_plot->plot_style == VECTOR) {
	    local_this_iso->next = iso_alloc(samples_1);
	    local_this_iso->next->p_count = 0;
	}

	if (this_plot->plot_style == POLYGONS) {
	    this_plot->has_grid_topology = FALSE;
	    this_plot->opt_out_of_dgrid3d = TRUE;
	    track_pm3d_quadrangles = TRUE;
	}

	/* If the user has set an explicit locale for numeric input, apply it */
	/* here so that it affects data fields read from the input file.      */
	set_numeric_locale();

	/* Initial state */
	df_warn_on_missing_columnheader = TRUE;

	while ((retval = df_readline(v,MAXDATACOLS)) != DF_EOF) {
	    j = retval;

	    if (j == 0) /* not blank line, but df_readline couldn't parse it */
		int_warn(NO_CARET, "Bad data on line %d of file %s",
			  df_line_number, df_filename ? df_filename : ""); 

	    if (j == DF_SECOND_BLANK)
		break;		/* two blank lines */
	    if (j == DF_FIRST_BLANK) {

		/* Images are in a sense similar to isocurves.
		 * However, the routine for images is written to
		 * compute the two dimensions of coordinates by
		 * examining the data alone.  That way it can be used
		 * in the 2D plots, for which there is no isoline
		 * record.  So, toss out isoline information for
		 * images.
		 */
		if ((this_plot->plot_style == IMAGE)
		||  (this_plot->plot_style == RGBIMAGE)
		||  (this_plot->plot_style == RGBA_IMAGE))
		    continue;

		if (this_plot->plot_style == VECTOR)
		    continue;

		/* one blank line */
		if (pt_in_iso_crv == 0) {
		    if (xdatum == 0)
			continue;
		    pt_in_iso_crv = xdatum;
		}
		if (xdatum > 0) {
		    local_this_iso->p_count = xdatum;
		    local_this_iso->next = this_plot->iso_crvs;
		    this_plot->iso_crvs = local_this_iso;
		    this_plot->num_iso_read++;

		    if (xdatum != pt_in_iso_crv)
			this_plot->has_grid_topology = FALSE;

		    local_this_iso = iso_alloc(pt_in_iso_crv);
		    xdatum = 0;
		    ydatum++;
		}
		continue;
	    }

	    else if (j == DF_FOUND_KEY_TITLE){
		/* only the shared part of the 2D and 3D headers is used */
		df_set_key_title((struct curve_points *)this_plot);
		continue;
	    }
	    else if (j == DF_KEY_TITLE_MISSING){
		fprintf(stderr,
			"get_data: key title not found in requested column\n"
		    );
		continue;
	    }

	    else if (j == DF_COLUMN_HEADERS) {
		continue;
	    }

	    /* its a data point or undefined */
	    if (xdatum >= local_this_iso->p_max) {
		/* overflow about to occur. Extend size of points[]
		 * array. Double the size, and add 1000 points, to
		 * avoid needlessly small steps. */
		iso_extend(local_this_iso, xdatum + xdatum + 1000);
		if (this_plot->plot_style == VECTOR) {
		    iso_extend(local_this_iso->next, xdatum + xdatum + 1000);
		    local_this_iso->next->p_count = 0;
		}
	    }
	    cp = local_this_iso->points + xdatum;

	    if (this_plot->plot_style == VECTOR) {
		if (j < 6) {
		    cp->type = UNDEFINED;
		    continue;
		}
		cphead = local_this_iso->next->points + xdatum;
	    }

	    if (j == DF_UNDEFINED || j == DF_MISSING) {
		cp->type = UNDEFINED;
		goto come_here_if_undefined;
	    }
	    cp->type = INRANGE;	/* unless we find out different */

	    /* EAM Oct 2004 - Substantially rework this section */
	    /* now that there are many more plot types.         */

	    x = y = z = 0.0;
	    xtail = ytail = ztail = 0.0;

	    /* The x, y, z coordinates depend on the mapping type */
	    switch (mapping3d) {

	    case MAP3D_CARTESIAN:
		if (j == 1) {
		    x = xdatum;
		    y = ydatum;
		    z = v[0];
		    j = 3;
		    break;
		}

		if (j == 2) {
		    if (PM3DSURFACE != this_plot->plot_style)
			int_error(this_plot->token,
				  "2 columns only possible with explicit pm3d style (line %d)",
				  df_line_number);
		    x = xdatum;
		    y = ydatum;
		    z = v[0];
		    color_from_column(TRUE);
		    color = v[1];
		    j = 3;
		    break;
		}

		/* Assume everybody agrees that x,y,z are the first three specs */
		if (j >= 3) {
		    x = v[0];
		    y = v[1];
		    z = v[2];
		    break;
		}

		break;

	    case MAP3D_SPHERICAL:
		if (j < 2)
		    int_error(this_plot->token, "Need 2 or 3 columns");
		if (j < 3) {
		    v[2] = 1;	/* default radius */
		    j = 3;
		}

		/* Convert to radians. */
		v[0] *= ang2rad;
		v[1] *= ang2rad;

		x = v[2] * cos(v[0]) * cos(v[1]);
		y = v[2] * sin(v[0]) * cos(v[1]);
		z = v[2] * sin(v[1]);

		break;

	    case MAP3D_CYLINDRICAL:
		if (j < 2)
		    int_error(this_plot->token, "Need 2 or 3 columns");
		if (j < 3) {
		    v[2] = 1;	/* default radius */
		    j = 3;
		}

		/* Convert to radians. */
		v[0] *= ang2rad;

		x = v[2] * cos(v[0]);
		y = v[2] * sin(v[0]);
		z = v[1];

		break;

	    default:
		int_error(NO_CARET, "Internal error: Unknown mapping type");
		return retval;
	    }

	    if (j < df_no_use_specs)
		int_error(this_plot->token,
			"Wrong number of columns in input data - line %d",
			df_line_number);

	    /* Work-around for hidden3d, which otherwise would use the */
	    /* color of the vector midpoint rather than the endpoint. */
	    if (this_plot->plot_style == IMPULSES) {
		if (this_plot->lp_properties.pm3d_color.type == TC_Z) {
		    color = z;
		    color_from_column(TRUE);
		}
	    }

	    /* After the first three columns it gets messy because */
	    /* different plot styles assume different contents in the columns */

	    if (( this_plot->plot_style == POINTSTYLE || this_plot->plot_style == LINESPOINTS)) {
		int varcol = 3;
		coordval var_char = 0;
		if (this_plot->lp_properties.p_size == PTSZ_VARIABLE)
		    cp->CRD_PTSIZE = v[varcol++];
		if (this_plot->lp_properties.p_type == PT_VARIABLE) {
		    if (isnan(v[varcol]) && df_tokens[varcol]) {
			safe_strncpy( (char *)(&var_char), df_tokens[varcol], sizeof(coordval));
			truncate_to_one_utf8_char((char *)(&var_char));
			cp->CRD_PTCHAR = var_char;
		    }
		    cp->CRD_PTTYPE = v[varcol++];
		}
		if (j < varcol)
		    int_error(NO_CARET, "Not enough input columns");
		else if (j == varcol) {
		    color = z;
		    color_from_column(FALSE);
		} else {
		    color = v[varcol];
		    color_from_column(TRUE);
		}

	    } else if (this_plot->plot_style == LABELPOINTS) {
		if (draw_contour && !this_plot->opt_out_of_contours) {
		    /* j is 3 for some reason */ ;
		} else {
		    int varcol = 4;
		    cp->CRD_ROTATE = this_plot->labels->rotate;
		    cp->CRD_PTSIZE = this_plot->labels->lp_properties.p_size;
		    cp->CRD_PTTYPE = this_plot->labels->lp_properties.p_type;
		    if (cp->CRD_PTSIZE == PTSZ_VARIABLE)
			cp->CRD_PTSIZE = v[varcol++];
		    if (cp->CRD_PTTYPE == PT_VARIABLE)
			cp->CRD_PTTYPE = v[varcol++] - 1;
		    if (j < varcol)
			int_error(NO_CARET, "Not enough input columns");
		    if (j == varcol) {
			/* 4th column holds label text rather than color */
			color = z;
			color_from_column(FALSE);
		    } else {
			color = v[varcol];
			color_from_column(TRUE);
		    }
		}

	    } else if (this_plot->plot_style == CIRCLES) {
		int varcol = 3;
		/* require 4th column = radius */
		cp->CRD_PTSIZE = v[varcol++];
		if (j < varcol)
		    int_error(NO_CARET, "Not enough input columns");
		if (j == varcol)
		    color = this_plot->fill_properties.border_color.lt;
		else
		    color = v[varcol];
		color_from_column(TRUE);

	    } else if (this_plot->plot_style == VECTOR) {
		/* We already enforced that j >= 6 */
		xtail = x + v[3];
		ytail = y + v[4];
		ztail = z + v[5];
		/* variable color is always last but there can be intervening variable styles */
		arrowstyle = v[6];
		if (j >= 7) {
		    color = v[j-1];
		    color_from_column(TRUE);
		} else {
		    color = z;
		    color_from_column(FALSE);
		}

	    } else if (this_plot->plot_style == ZERRORFILL) {
		if (j == 4) {
		    zlow = v[2] - v[3];
		    zhigh = v[2] + v[3];
		} else if (j == 5) {
		    zlow = v[3];
		    zhigh = v[4];
		} else {
		    int_error(NO_CARET, "this plot style wants 4 or 5 input columns");
		}
		color_from_column(FALSE);
		track_pm3d_quadrangles = TRUE;

	    } else if (this_plot->plot_style == BOXES) {
		/* Pop last using value to use as variable color */
		if (this_plot->fill_properties.border_color.type == TC_RGB
		&&  this_plot->fill_properties.border_color.value < 0) {
		    color_from_column(TRUE);
		    color = v[--j];

		} else if (this_plot->fill_properties.border_color.type == TC_Z
			&& j >= 4) {
		    rgb255_color rgbcolor;
		    unsigned int rgb;
		    rgb255maxcolors_from_gray(cb2gray(v[--j]), &rgbcolor);
		    rgb = (unsigned int)rgbcolor.r << 16
			| (unsigned int)rgbcolor.g << 8
			| (unsigned int)rgbcolor.b;
		    color = rgb;
		    color_from_column(TRUE);

		} else if (this_plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
		    struct lp_style_type lptmp;
		    color_from_column(TRUE);
		    load_linetype(&lptmp, (int)v[--j]);
		    color = lptmp.pm3d_color.lt;
		}

		if (j >= 4) {
		    xlow = x - v[3]/2.;
		    xhigh = x + v[3]/2.;
		} else if (X_AXIS.log) {
		    double width = (boxwidth > 0) ? boxwidth : 0.1;
		    xlow = x * pow(X_AXIS.base, -width/2.);
		    xhigh = x * pow(X_AXIS.base, width/2.);
		} else {
		    double width = (boxwidth > 0) ? boxwidth : 1.0;
		    xlow = x - width/2.;
		    xhigh = x + width/2.;;
		}
		track_pm3d_quadrangles = TRUE;

	    } else {	/* all other plot styles */
		if (j >= 4) {
		    color = v[3];
		    color_from_column(TRUE);
		}
	    }
#undef color_from_column


	    /* The STORE_AND_UPDATE_RANGE macro cannot use "continue" as 
	     * an action statement because it is wrapped in a loop.
	     * I regard this as correct goto use
	     */
	    cp->type = INRANGE;
	    STORE_AND_UPDATE_RANGE(cp->x, x, cp->type, x_axis, this_plot->noautoscale,
				goto come_here_if_undefined);
	    STORE_AND_UPDATE_RANGE(cp->y, y, cp->type, y_axis, this_plot->noautoscale,
				goto come_here_if_undefined);
	    if (this_plot->plot_style == VECTOR) {
		cphead->type = INRANGE;
		STORE_AND_UPDATE_RANGE(cphead->x, xtail, cphead->type, x_axis,
				this_plot->noautoscale, goto come_here_if_undefined);
		STORE_AND_UPDATE_RANGE(cphead->y, ytail, cphead->type, y_axis,
				this_plot->noautoscale, goto come_here_if_undefined);
	    }

	    if (dgrid3d && !this_plot->opt_out_of_dgrid3d) {
		/* No point in auto-scaling before we re-grid the data */
		cp->z = z;
		cp->CRD_COLOR = (pm3d_color_from_column) ? color : z;
	    } else {
		coord_type dummy_type;

		/* Without this,  z=Nan or z=Inf or DF_MISSING fails to set
		 * CRD_COLOR at all, since the z test bails to a goto.
		 */
		if (this_plot->plot_style == IMAGE) {
		    dummy_type = INRANGE;
		    store_and_update_range(&cp->CRD_COLOR, (pm3d_color_from_column) ? color : z,
					&dummy_type, &CB_AXIS, this_plot->noautoscale);
		    /* Rejecting z because x or y is OUTRANGE causes lost pixels
		     * and possilbe non-rectangular image arrays, which then fail.
		     * Better to mark all pixels INRANGE and clip them later.
		     */
		    cp->type = INRANGE;
		}

		/* Version 5: cp->z=0 in the UNDEF_ACTION recovers what	version 4 did */
		STORE_AND_UPDATE_RANGE(cp->z, z, cp->type, z_axis,
				this_plot->noautoscale,
				cp->z=0;goto come_here_if_undefined);

		if (this_plot->plot_style == ZERRORFILL) {
		    STORE_AND_UPDATE_RANGE(cp->CRD_ZLOW, zlow, cp->type, z_axis,
				this_plot->noautoscale, goto come_here_if_undefined);
		    STORE_AND_UPDATE_RANGE(cp->CRD_ZHIGH, zhigh, cp->type, z_axis,
				this_plot->noautoscale, goto come_here_if_undefined);
		}

		if (this_plot->plot_style == BOXES) {
		    STORE_AND_UPDATE_RANGE(cp->xlow, xlow, cp->type, x_axis,
				this_plot->noautoscale, goto come_here_if_undefined);
		    STORE_AND_UPDATE_RANGE(cp->xhigh, xhigh, cp->type, x_axis,
				this_plot->noautoscale, goto come_here_if_undefined);
		    /* autoscale on y without losing color information stored in CRD_COLOR */
		    if (boxdepth > 0) {
			double dummy;
			STORE_AND_UPDATE_RANGE(dummy, y - boxdepth, cp->type, y_axis,
				this_plot->noautoscale, {});
			STORE_AND_UPDATE_RANGE(dummy, y + boxdepth, cp->type, y_axis,
				this_plot->noautoscale, {});
		    }
		    /* We converted linetype colors (lc variable) to RGB colors on input.
		     * Other plot style do not do this.
		     * When we later call check3d_for_variable_color it needs to know this.
		     */
		    if (this_plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
			this_plot->lp_properties.pm3d_color.type = TC_RGB;
			this_plot->lp_properties.pm3d_color.value = -1.0;
		    }
		}

		if (this_plot->plot_style == VECTOR)
		    STORE_AND_UPDATE_RANGE(cphead->z, ztail, cphead->type, z_axis,
				this_plot->noautoscale, goto come_here_if_undefined);

		if (pm3d_color_from_column) {
		    if (this_plot->plot_style == VECTOR) {
			cphead->CRD_COLOR = color;
			cphead->CRD_ASTYLE = arrowstyle;
			cp->CRD_ASTYLE = arrowstyle;
		    }
		} else {
		    color = z;
		}

		dummy_type = cp->type;
		STORE_AND_UPDATE_RANGE(cp->CRD_COLOR, color, dummy_type,
			    COLOR_AXIS, this_plot->noautoscale,
			    goto come_here_if_undefined);
	    }

	    /* At this point we have stored the point coordinates. Now we need to copy */
	    /* x,y,z into the text_label structure and add the actual text string.     */
	    if (this_plot->plot_style == LABELPOINTS) {
		if (draw_contour && !this_plot->opt_out_of_contours)
		    /* Contour labels are calculated and added later */
		    ;
		else if (cp->type == INRANGE)
		    store_label(this_plot->labels, cp, xdatum, df_tokens[3], color);
	    }

	    if (this_plot->plot_style == RGBIMAGE || this_plot->plot_style == RGBA_IMAGE) {
		/* We will autoscale the RGB components to  a total range [0:255]
		 * so we don't need to do any fancy scaling here.
		 */
		/* If there is only one column of image data, it must be 32-bit ARGB */
		if (j==4) {
		    unsigned int argb = v[3];
		    v[3] = (argb >> 16) & 0xff;
		    v[4] = (argb >> 8) & 0xff;
		    v[5] = (argb) & 0xff;
		    /* The alpha channel convention is unfortunate */
		    v[6] = 255 - (unsigned int)((argb >> 24) & 0xff);
		}
		cp->CRD_R = v[3];
		cp->CRD_G = v[4];
		cp->CRD_B = v[5];
		cp->CRD_A = v[6];	/* Alpha channel */
	    }

	come_here_if_undefined:
	    /* some may complain, but I regard this as the correct use of goto */
	    ++xdatum;

	}			/* end of whileloop - end of surface */

	/* We are finished reading user input; return to C locale for internal use */
	reset_numeric_locale();

	if (pm3d_color_from_column) {
	    this_plot->pm3d_color_from_column = pm3d_color_from_column;
	}

	if (xdatum > 0) {
	    this_plot->num_iso_read++;	/* Update last iso. */
	    local_this_iso->p_count = xdatum;

	    /* If this is a VECTOR plot then iso->next is already */
	    /* occupied by the vector tail coordinates.           */
	    if (this_plot->plot_style != VECTOR)
		local_this_iso->next = this_plot->iso_crvs;
	    this_plot->iso_crvs = local_this_iso;

	    if (xdatum != pt_in_iso_crv)
		this_plot->has_grid_topology = FALSE;

	} else { /* Free last allocation */
	    if (this_plot->plot_style == VECTOR)
		iso_free(local_this_iso->next);
	    iso_free(local_this_iso);
	}

	/*}}} */
    }

    if (dgrid3d && !(this_plot->opt_out_of_dgrid3d))
	grid_nongrid_data(this_plot);

    if (this_plot->num_iso_read <= 1)
	this_plot->has_grid_topology = FALSE;

    if (this_plot->has_grid_topology && !hidden3d 
    &&   (implicit_surface || this_plot->plot_style == SURFACEGRID)) {
	struct iso_curve *new_icrvs = NULL;
	int num_new_iso = this_plot->iso_crvs->p_count;
	int len_new_iso = this_plot->num_iso_read;
	int i;

	/* Now we need to set the other direction (pseudo) isolines. */
	for (i = 0; i < num_new_iso; i++) {
	    struct iso_curve *new_icrv = iso_alloc(len_new_iso);

	    new_icrv->p_count = len_new_iso;

	    for (j = 0, this_iso = this_plot->iso_crvs;
		 this_iso != NULL;
		 j++, this_iso = this_iso->next) {
		/* copy whole point struct to get type too.
		 * wasteful for windows, with padding */
		/* more efficient would be extra pointer to same struct */
		new_icrv->points[j] = this_iso->points[i];
	    }

	    new_icrv->next = new_icrvs;
	    new_icrvs = new_icrv;
	}

	/* Append the new iso curves after the read ones. */
	for (this_iso = this_plot->iso_crvs;
	     this_iso->next != NULL;
	     this_iso = this_iso->next);
	this_iso->next = new_icrvs;
    }

    /* Deferred evaluation of plot title now that we know column headers */
    reevaluate_plot_title( (struct curve_points *)this_plot );

    return retval;
}

/* HBB 20000501: code isolated from eval_3dplots(), where practically
 * identical code occurred twice, for direct and crossing isolines,
 * respectively.  The latter only are done for in non-hidden3d
 * mode. */
static void
calculate_set_of_isolines(
    AXIS_INDEX value_axis,
    TBOOLEAN cross,
    struct iso_curve **this_iso,
    AXIS_INDEX iso_axis,
    double iso_min, double iso_step,
    int num_iso_to_use,
    AXIS_INDEX sam_axis,
    double sam_min, double sam_step,
    int num_sam_to_use)
{
    int i, j;
    struct coordinate *points = (*this_iso)->points;
    int do_update_color = (!parametric || (parametric && value_axis == FIRST_Z_AXIS));

    for (j = 0; j < num_iso_to_use; j++) {
	double iso = iso_min + j * iso_step;
	double isotemp;

	if (nonlinear(&axis_array[iso_axis]))
	    isotemp = iso = eval_link_function(&axis_array[iso_axis], iso);
	else
	    isotemp = iso;

	(void) Gcomplex(&plot_func.dummy_values[cross ? 0 : 1], isotemp, 0.0);

	for (i = 0; i < num_sam_to_use; i++) {
	    double sam = sam_min + i * sam_step;
	    struct value a;
	    double temp;

	    if (nonlinear(&axis_array[sam_axis]))
		sam = eval_link_function(&axis_array[sam_axis], sam);
	    temp = sam;

	    (void) Gcomplex(&plot_func.dummy_values[cross ? 1 : 0], temp, 0.0);

	    if (cross) {
		points[i].x = iso;
		points[i].y = sam;
	    } else {
		points[i].x = sam;
		points[i].y = iso;
	    }

	    evaluate_at(plot_func.at, &a);

	    if (undefined || (fabs(imag(&a)) > zero)) {
		points[i].type = UNDEFINED;
		continue;
	    }

	    temp = real(&a);
	    points[i].type = INRANGE;
	    STORE_AND_UPDATE_RANGE(points[i].z, temp, points[i].type,
					    value_axis, FALSE, NOOP);
	    if (do_update_color) {
		coord_type dummy_type = points[i].type;
		STORE_AND_UPDATE_RANGE( points[i].CRD_COLOR, temp, dummy_type,
					COLOR_AXIS, FALSE, NOOP);
	    }
	}
	(*this_iso)->p_count = num_sam_to_use;
	*this_iso = (*this_iso)->next;
	points = (*this_iso) ? (*this_iso)->points : NULL;
    }
}


/*
 * This parses the splot command after any range specifications. To support
 * autoscaling on the x/z axis, we want any data files to define the x/y
 * range, then to plot any functions using that range. We thus parse the
 * input twice, once to pick up the data files, and again to pick up the
 * functions. Definitions are processed twice, but that won't hurt.
 * div - okay, it doesn't hurt, but every time an option as added for
 * datafiles, code to parse it has to be added here. Change so that
 * we store starting-token in the plot structure.
 */
static void
eval_3dplots()
{
    int i;
    struct surface_points **tp_3d_ptr;
    int start_token=0, end_token;
    TBOOLEAN eof_during_iteration = FALSE;  /* set when for [n=start:*] hits NODATA */
    int begin_token;
    TBOOLEAN some_data_files = FALSE, some_functions = FALSE;
    TBOOLEAN was_definition = FALSE;
    int df_return = 0;
    int plot_num, line_num;
    /* part number of parametric function triplet: 0 = z, 1 = y, 2 = x */
    int crnt_param = 0;
    char *xtitle;
    char *ytitle;
    legend_key *key = &keyT;
    char orig_dummy_u_var[MAX_ID_LEN+1], orig_dummy_v_var[MAX_ID_LEN+1];

    /* Free memory from previous splot.
     * If there is an error within this function, the memory is left allocated,
     * since we cannot call sp_free if the list is incomplete
     */
    if (first_3dplot && plot3d_num>0)
      sp_free(first_3dplot);
    plot3d_num=0;
    first_3dplot = NULL;

    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;
    z_axis = FIRST_Z_AXIS;

    tp_3d_ptr = &(first_3dplot);
    plot_num = 0;
    line_num = 0;		/* default line type */

    /* Assume that the input data can be re-read later */
    volatile_data = FALSE;

    /* Normally we only need to initialize pm3d quadrangles if pm3d mode is active */
    /* but there are a few special cases that use them outside of pm3d mode.       */
    track_pm3d_quadrangles = pm3d_objects() ? TRUE : FALSE;

    /* Explicit ranges in the splot command may temporarily rename dummy variables */
    strcpy(orig_dummy_u_var, c_dummy_var[0]);
    strcpy(orig_dummy_v_var, c_dummy_var[1]);

    xtitle = NULL;
    ytitle = NULL;

    begin_token = c_token;

/*** First Pass: Read through data files ***/
    /*
     * This pass serves to set the x/yranges and to parse the command, as
     * well as filling in everything except the function data. That is done
     * after the x/yrange is defined.
     */
    plot_iterator = check_for_iteration();
    last_iteration_in_first_pass = INT_MAX;

    while (TRUE) {

	/* Forgive trailing comma on a multi-element plot command */
	if (END_OF_COMMAND) {
	    if (plot_num == 0)
		int_error(c_token, "function to plot expected");
	    break;
	}

	if (crnt_param == 0 && !was_definition)
	    start_token = c_token;

	if (is_definition(c_token)) {
	    define();
	    if (equals(c_token, ","))
		c_token++;
	    was_definition = TRUE;
	    continue;

	} else {
	    int specs = -1;
	    struct surface_points *this_plot;
	    splot_component this_component;

	    char *name_str;
	    TBOOLEAN duplication = FALSE;
	    TBOOLEAN set_title = FALSE, set_with = FALSE;
	    TBOOLEAN set_lpstyle = FALSE;
	    TBOOLEAN checked_once = FALSE;
	    TBOOLEAN set_labelstyle = FALSE;
	    TBOOLEAN set_fillstyle = FALSE;

	    int u_sample_range_token, v_sample_range_token;
	    t_value original_value_u, original_value_v;

	    if (!was_definition && (!parametric || crnt_param == 0))
		start_token = c_token;
	    was_definition = FALSE;

	    /* Check for sampling range[s]
	     * Note: we must allow both for '+', which uses SAMPLE_AXIS,
	     *       and '++', which uses U_AXIS and V_AXIS
	     */
	    u_sample_range_token = parse_range(SAMPLE_AXIS);
	    if (u_sample_range_token != 0) {
		axis_array[U_AXIS].min = axis_array[SAMPLE_AXIS].min;
		axis_array[U_AXIS].max = axis_array[SAMPLE_AXIS].max;
		axis_array[U_AXIS].autoscale = axis_array[SAMPLE_AXIS].autoscale;
		axis_array[U_AXIS].SAMPLE_INTERVAL = axis_array[SAMPLE_AXIS].SAMPLE_INTERVAL;
	    }
	    v_sample_range_token = parse_range(V_AXIS);

	    if (u_sample_range_token > 0)
		axis_array[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;
	    if (u_sample_range_token > 0 && axis_array[U_AXIS].SAMPLE_INTERVAL != 0)
		axis_array[U_AXIS].range_flags |= RANGE_SAMPLED;
	    if (v_sample_range_token > 0 && axis_array[V_AXIS].SAMPLE_INTERVAL != 0)
		axis_array[V_AXIS].range_flags |= RANGE_SAMPLED;

	    /* Allow replacement of the dummy variables in a function */
	    if (u_sample_range_token > 0)
		copy_str(c_dummy_var[0], u_sample_range_token, MAX_ID_LEN);
	    else if (u_sample_range_token < 0)
		strcpy(c_dummy_var[0], set_dummy_var[0]);
	    else
		strcpy(c_dummy_var[0], orig_dummy_u_var);
	    if (v_sample_range_token > 0)
		copy_str(c_dummy_var[1], v_sample_range_token, MAX_ID_LEN);
	    else if (v_sample_range_token < 0)
		strcpy(c_dummy_var[1], set_dummy_var[1]);
	    else
		strcpy(c_dummy_var[1], orig_dummy_v_var);

	    /* Determine whether this plot component is a
	     *   function (name_str == NULL)
	     *   data file (name_str is "filename")
	     *   datablock (name_str is "$datablock")
	     *   voxel grid (name_str is "$gridname")
	     *   key entry (keyword 'keyentry')
	     */
	    dummy_func = &plot_func;
	    name_str = string_or_express(NULL);
	    dummy_func = NULL;
	    if (equals(c_token, "keyentry"))
		this_component = SP_KEYENTRY;
	    else if (!name_str)
		this_component = SP_FUNCTION;
	    else if ((*name_str == '$') && get_vgrid_by_name(name_str))
		this_component = SP_VOXELGRID;
	    else if (*name_str == '$')
		this_component = SP_DATABLOCK;
	    else
		this_component = SP_DATAFILE;

	    switch (this_component) {

	    case SP_DATAFILE:
	    case SP_DATABLOCK:
		/* data file or datablock to plot */
		if (parametric && crnt_param != 0)
		    int_error(c_token, "previous parametric function not fully specified");

		if (!some_data_files) {
		    if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN) {
			axis_array[FIRST_X_AXIS].min = VERYLARGE;
		    }
		    if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX) {
			axis_array[FIRST_X_AXIS].max = -VERYLARGE;
		    }
		    if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MIN) {
			axis_array[FIRST_Y_AXIS].min = VERYLARGE;
		    }
		    if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MAX) {
			axis_array[FIRST_Y_AXIS].max = -VERYLARGE;
		    }
		    some_data_files = TRUE;
		}
		if (*tp_3d_ptr)
		    this_plot = *tp_3d_ptr;
		else {		/* no memory malloc()'d there yet */
		    /* Allocate enough isosamples and samples */
		    this_plot = sp_alloc(0, 0, 0, 0);
		    *tp_3d_ptr = this_plot;
		}

		this_plot->plot_type = DATA3D;
		this_plot->plot_style = data_style;
		eof_during_iteration = FALSE;

		/* FIXME: additional fields may need to be reset */
		this_plot->opt_out_of_hidden3d = FALSE;
		this_plot->opt_out_of_dgrid3d = FALSE;
		this_plot->title_is_suppressed = FALSE;

		/* Mechanism for deferred evaluation of plot title */
		free_at(df_plot_title_at);
		df_plot_title_at = NULL;

		df_set_plot_mode(MODE_SPLOT);
		specs = df_open(name_str, MAXDATACOLS, (struct curve_points *)this_plot);

		if (df_matrix)
		    this_plot->has_grid_topology = TRUE;

		/* Store pointers to the named variables used for sampling */
		if (u_sample_range_token > 0)
		    this_plot->sample_var = add_udv(u_sample_range_token);
		else
		    this_plot->sample_var = add_udv_by_name(c_dummy_var[0]);
		if (v_sample_range_token > 0)
		    this_plot->sample_var2 = add_udv(v_sample_range_token);
		else
		    this_plot->sample_var2 = add_udv_by_name(c_dummy_var[1]);

		/* Save prior values of u, v so we can restore later */
		original_value_u = this_plot->sample_var->udv_value;
		original_value_v = this_plot->sample_var2->udv_value;

		/* for capture to key */
		this_plot->token = end_token = c_token - 1;
		/* FIXME: Is this really needed? */
		this_plot->iteration = plot_iterator ? plot_iterator->iteration : 0;

		/* this_plot->token is temporary, for errors in get_3ddata() */

		/* NB: df_axis is used only for timedate data and 3D cbticlabels */
		if (specs < 3) {
		    if (axis_array[FIRST_X_AXIS].datatype == DT_TIMEDATE)
			int_error(c_token, "Need full using spec for x time data");
		    if (axis_array[FIRST_Y_AXIS].datatype == DT_TIMEDATE)
			int_error(c_token, "Need full using spec for y time data");
		}
		df_axis[0] = FIRST_X_AXIS;
		df_axis[1] = FIRST_Y_AXIS;
		df_axis[2] = FIRST_Z_AXIS;
		break;

	    case SP_KEYENTRY:
		c_token++;
		plot_num++;
		if (*tp_3d_ptr)
		    this_plot = *tp_3d_ptr;
		else {		/* no memory malloc()'d there yet */
		    /* Allocate enough isosamples and samples */
		    this_plot = sp_alloc(0, 0, 0, 0);
		    *tp_3d_ptr = this_plot;
		}
		this_plot->plot_type = KEYENTRY;
		this_plot->plot_style = LABELPOINTS;
		this_plot->token = end_token = c_token - 1;
		break;

	    case SP_FUNCTION:
		plot_num++;
		if (parametric) {
		    /* Rotate between x/y/z axes */
		    /* +2 same as -1, but beats -ve problem */
		    crnt_param = (crnt_param + 2) % 3;
		}
		if (*tp_3d_ptr) {
		    this_plot = *tp_3d_ptr;
		    if (!hidden3d)
			sp_replace(this_plot, samples_1, iso_samples_1,
				   samples_2, iso_samples_2);
		    else
			sp_replace(this_plot, iso_samples_1, 0,
				   0, iso_samples_2);

		} else {	/* no memory malloc()'d there yet */
		    /* Allocate enough isosamples and samples */
		    if (!hidden3d)
			this_plot = sp_alloc(samples_1, iso_samples_1,
					     samples_2, iso_samples_2);
		    else
			this_plot = sp_alloc(iso_samples_1, 0,
					     0, iso_samples_2);
		    *tp_3d_ptr = this_plot;
		}

		this_plot->plot_type = FUNC3D;
		this_plot->has_grid_topology = TRUE;
		this_plot->plot_style = func_style;
		this_plot->num_iso_read = iso_samples_2;
		/* FIXME: additional fields may need to be reset */
		this_plot->opt_out_of_hidden3d = FALSE;
		this_plot->opt_out_of_dgrid3d = FALSE;
		this_plot->title_is_suppressed = FALSE;
		/* ignore it for now */
		some_functions = TRUE;
		end_token = c_token - 1;
		break;

	    case SP_VOXELGRID:
		plot_num++;
		if (*tp_3d_ptr)
		    this_plot = *tp_3d_ptr;
		else {
		    /* No actual space is needed since we will not store data */
		    this_plot = sp_alloc(0, 0, 0, 0);
		    *tp_3d_ptr = this_plot;
		}
		this_plot->vgrid = get_vgrid_by_name(name_str)->udv_value.v.vgrid;
		this_plot->plot_type = VOXELDATA;
		this_plot->opt_out_of_hidden3d = TRUE;
		this_plot->token = end_token = c_token - 1;
		break;

	    default:
		int_error(c_token-1, "unrecognized data source");
		break;

	    } /* End of switch(this_component) */

	    /* clear current title, if it exists */
	    if (this_plot->title) {
		free(this_plot->title);
		this_plot->title = NULL;
	    }

	    /* default line and point types */
	    this_plot->lp_properties.l_type = line_num;
	    this_plot->lp_properties.p_type = line_num;
	    this_plot->lp_properties.d_type = line_num;

	    /* user may prefer explicit line styles */
	    this_plot->hidden3d_top_linetype = line_num;
	    if (prefer_line_styles)
		lp_use_properties(&this_plot->lp_properties, line_num+1);
	    else
		load_linetype(&this_plot->lp_properties, line_num+1);

	    /* pm 25.11.2001 allow any order of options */
	    while (!END_OF_COMMAND || !checked_once) {
		int save_token = c_token;

		/* Allow this plot not to affect autoscaling */
		if (almost_equals(c_token, "noauto$scale")) {
		    c_token++;
		    this_plot->noautoscale = TRUE;
		    continue;
		}

		/* deal with title */
		parse_plot_title((struct curve_points *)this_plot, xtitle, ytitle, &set_title);
		if (save_token != c_token)
		    continue;

		/* deal with style */
		if (almost_equals(c_token, "w$ith")) {
		    if (set_with) {
			duplication=TRUE;
			break;
		    }
		    this_plot->plot_style = get_style();
		    if ((this_plot->plot_type == FUNC3D) &&
			((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
			|| (this_plot->plot_style == LABELPOINTS && !draw_contour)
			|| (this_plot->plot_style == VECTOR)
			)) {
			int_warn(c_token-1, "This style cannot be used to plot a surface defined by a function");
			this_plot->plot_style = POINTSTYLE;
			this_plot->plot_type = NODATA;
		    }

		    if (this_plot->plot_style == IMAGE
		    ||  this_plot->plot_style == RGBA_IMAGE
		    ||  this_plot->plot_style == RGBIMAGE) {
			if (this_plot->plot_type == FUNC3D)
			    int_error(c_token-1, "a function cannot be plotted as an image");
			else
			    get_image_options(&this_plot->image_properties);
		    }

		    if ((this_plot->plot_style | data_style) & PM3DSURFACE) {
			if (equals(c_token, "at")) {
			/* option 'with pm3d [at ...]' is explicitly specified */
			c_token++;
			if (get_pm3d_at_option(&this_plot->pm3d_where[0]))
			    return; /* error */
			}
		    }

		    if ((this_plot->plot_type == VOXELDATA)
		    &&  (this_plot->plot_style == DOTS || this_plot->plot_style == POINTSTYLE)) {
			this_plot->iso_level = 0.0;
			if (equals(c_token, "above")) {
			    c_token++;
			    this_plot->iso_level = real_expression();
			}
		    }

		    if  (this_plot->plot_style == ISOSURFACE) {
			if (this_plot->plot_type == VOXELDATA) {
			    if (equals(c_token, "level")) {
				c_token++;
				this_plot->iso_level = real_expression();
			    } else {
				int_error(c_token, "isosurface requires a level");
			    }
			} else
			    int_error(c_token, "isosurface requires a voxel grid");

			this_plot->pm3d_color_from_column = FALSE;
			track_pm3d_quadrangles = TRUE;
		    }

		    set_with = TRUE;
		    continue;
		}

		/* Hidden3D code by default includes points, labels and vectors	*/
		/* in the hidden3d processing. Check here if this particular	*/
		/* plot wants to be excluded.					*/
		if (almost_equals(c_token, "nohidden$3d")) {
		    c_token++;
		    this_plot->opt_out_of_hidden3d = TRUE;
		    continue;
		}

		/* "set dgrid3d" tries to grid *everything*, which isn't always wanted */
		if (equals(c_token, "nogrid")) {
		    c_token++;
		    this_plot->opt_out_of_dgrid3d = TRUE;
		    continue;
		}

		/* "set contour" is global.  Allow individual plots to opt out */
		if (almost_equals(c_token, "nocon$tours")) {
		    c_token++;
		    this_plot->opt_out_of_contours = TRUE;
		    continue;
		}

		/* "set surface" is global.  Allow individual plots to opt out */
		if (almost_equals(c_token, "nosur$face")) {
		    c_token++;
		    this_plot->opt_out_of_surface = TRUE;
		    continue;
		}

		/* Most plot styles accept line and point properties but do not */
		/* want font or text properties.				*/
		if (this_plot->plot_style == VECTOR) {
		    int stored_token = c_token;

		    if (!checked_once) {
			default_arrow_style(&this_plot->arrow_properties);
			load_linetype(&(this_plot->arrow_properties.lp_properties),
					line_num+1);
			checked_once = TRUE;
		    }
		    arrow_parse(&this_plot->arrow_properties, TRUE);
		    if (stored_token != c_token) {
			 if (set_lpstyle) {
			    duplication = TRUE;
			    break;
			 } else {
			    set_lpstyle = TRUE;
			    this_plot->lp_properties = this_plot->arrow_properties.lp_properties;
			    continue;
			}
		    }

		}

		if (this_plot->plot_style == PM3DSURFACE) {
		    /* both previous and subsequent line properties override pm3d default border */
		    int stored_token = c_token;
		    if (!set_lpstyle)
			this_plot->lp_properties = pm3d.border;
 		    lp_parse(&this_plot->lp_properties, LP_ADHOC, FALSE);
		    if (stored_token != c_token) {
			set_lpstyle = TRUE;
			continue;
		    }

		}

		if (this_plot->plot_style != LABELPOINTS) {
		    int stored_token = c_token;
		    struct lp_style_type lp = DEFAULT_LP_STYLE_TYPE;
		    int new_lt = 0;

		    lp.l_type = line_num;
		    lp.p_type = line_num;
		    lp.d_type = line_num;

		    /* user may prefer explicit line styles */
		    if (prefer_line_styles)
			lp_use_properties(&lp, line_num+1);
		    else
			load_linetype(&lp, line_num+1);

 		    new_lt = lp_parse(&lp, LP_ADHOC,
			     this_plot->plot_style & PLOT_STYLE_HAS_POINT);

		    checked_once = TRUE;
		    if (stored_token != c_token) {
			if (set_lpstyle) {
			    duplication=TRUE;
			    break;
			} else {
			    this_plot->lp_properties = lp;
			    set_lpstyle = TRUE;
			    if (new_lt)
				this_plot->hidden3d_top_linetype = new_lt - 1;
			    if (this_plot->lp_properties.p_type != PT_CHARACTER)
				continue;
			}
		    }
		}

		/* Labels can have font and text property info as plot options */
		/* In any case we must allocate one instance of the text style */
		/* that all labels in the plot will share.                     */
		if ((this_plot->plot_style == LABELPOINTS)
		||  (this_plot->plot_style & PLOT_STYLE_HAS_POINT
			&& this_plot->lp_properties.p_type == PT_CHARACTER)) {
		    int stored_token = c_token;
		    if (this_plot->labels == NULL) {
			this_plot->labels = new_text_label(-1);
			this_plot->labels->pos = CENTRE;
			this_plot->labels->layer = LAYER_PLOTLABELS;
		    }
		    parse_label_options(this_plot->labels, 3);
		    if (draw_contour)
			load_contour_label_options(this_plot->labels);
		    checked_once = TRUE;
		    if (stored_token != c_token) {
			if (set_labelstyle) {
			    duplication = TRUE;
			    break;
			} else {
			    set_labelstyle = TRUE;
			    continue;
			}
		    } else if (this_plot->lp_properties.p_type == PT_CHARACTER) {
			if  (equals(c_token, ","))
			    break;
			else
			    continue;
		    }
		}

		/* Some plots have a fill style as well */
		if ((this_plot->plot_style & PLOT_STYLE_HAS_FILL) && !set_fillstyle){
		    int stored_token = c_token;
		    if (equals(c_token,"fs") || almost_equals(c_token,"fill$style")) {
			this_plot->fill_properties.fillstyle = default_fillstyle.fillstyle;
			this_plot->fill_properties.filldensity = default_fillstyle.filldensity;
			this_plot->fill_properties.fillpattern = 1;
			this_plot->fill_properties.border_color = this_plot->lp_properties.pm3d_color;
			parse_fillstyle(&this_plot->fill_properties);
			set_fillstyle = TRUE;
		    }
		    if (this_plot->plot_style == PM3DSURFACE)
			this_plot->fill_properties.border_color.type = TC_DEFAULT;
		    if (equals(c_token,"fc") || almost_equals(c_token,"fillc$olor")) {
			parse_colorspec(&this_plot->fill_properties.border_color, TC_RGB);
			set_fillstyle = TRUE;
		    }
		    if (stored_token != c_token)
			continue;
		}

		break; /* unknown option */

	    }  /* while (!END_OF_COMMAND)*/

	    if (duplication)
		int_error(c_token, "duplicated or contradicting arguments in plot options");

	    if (this_plot->plot_style == TABLESTYLE)
		int_error(NO_CARET, "use `plot with table` rather than `splot with table`"); 

	    /* Various other non-supported styles can be treated as if they were POINTS.
	     * These are styles whose data format is sufficiently different that it's
	     * better to not even try.
	     */
	    if ((this_plot->plot_style == HISTOGRAMS)
	    ||  (this_plot->plot_style == SPIDERPLOT)
	    ||  (this_plot->plot_style == PARALLELPLOT)
	    ||  (this_plot->plot_style == CANDLESTICKS)
	    ||  (this_plot->plot_style == FINANCEBARS)
	    ||  (this_plot->plot_style == BOXPLOT))
		int_error(NO_CARET, "plot style not supported in 3D");

	    /* set default values for title if this has not been specified */
	    this_plot->title_is_automated = FALSE;
	    if (!set_title) {
		this_plot->title_no_enhanced = TRUE; /* filename or function cannot be enhanced */
		if (key->auto_titles == FILENAME_KEYTITLES) {
		    m_capture(&(this_plot->title), start_token, end_token);
		    if (crnt_param == 2)
			xtitle = this_plot->title;
		    else if (crnt_param == 1)
			ytitle = this_plot->title;
		    this_plot->title_is_automated = TRUE;
		} else {
		    if (xtitle != NULL)
			xtitle[0] = '\0';
		    if (ytitle != NULL)
			ytitle[0] = '\0';
		}
	    }

	    /* No line/point style given. As lp_parse also supplies
	     * the defaults for linewidth and pointsize, call it now
	     * to define them. */
	    if (! set_lpstyle) {
		if (this_plot->plot_style == VECTOR) {
		    this_plot->arrow_properties.lp_properties.l_type = line_num;
		    arrow_parse(&this_plot->arrow_properties, TRUE);
		    this_plot->lp_properties = this_plot->arrow_properties.lp_properties;

		} else if (this_plot->plot_style == PM3DSURFACE) {
		    /* Use default pm3d border unless we see explicit line properties */
		    this_plot->lp_properties = pm3d.border;
 		    lp_parse(&this_plot->lp_properties, LP_ADHOC, FALSE);

		} else {
		    int new_lt = 0;
		    this_plot->lp_properties.l_type = line_num;
		    this_plot->lp_properties.l_width = 1.0;
		    this_plot->lp_properties.p_type = line_num;
			this_plot->lp_properties.d_type = line_num;
		    this_plot->lp_properties.p_size = pointsize;

		    /* user may prefer explicit line styles */
		    if (prefer_line_styles)
			lp_use_properties(&this_plot->lp_properties, line_num+1);
		    else
			load_linetype(&this_plot->lp_properties, line_num+1);

		    new_lt = lp_parse(&this_plot->lp_properties, LP_ADHOC,
			 this_plot->plot_style & PLOT_STYLE_HAS_POINT);
		    if (new_lt)
			this_plot->hidden3d_top_linetype = new_lt - 1;
		    else
			this_plot->hidden3d_top_linetype = line_num;
		}
	    }

	    /* No fillcolor given; use the line color for fill also */
	    if (((this_plot->plot_style & PLOT_STYLE_HAS_FILL) && !set_fillstyle)
	    &&  !(this_plot->plot_style == PM3DSURFACE))
		this_plot->fill_properties.border_color
		    = this_plot->lp_properties.pm3d_color;

	    /* Some low-level routines expect to find the pointflag attribute */
	    /* in lp_properties (they don't have access to the full header).  */
	    if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		this_plot->lp_properties.flags |= LP_SHOW_POINTS;

	    /* Rule out incompatible line/point/style options */
	    if (this_plot->plot_type == FUNC3D) {
		if ((this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		&&  (this_plot->lp_properties.p_size == PTSZ_VARIABLE))
		    this_plot->lp_properties.p_size = 1;
	    }

	    /* FIXME: Leaving an explicit font in the label style for contour */
	    /* labels causes a double-free segfault.  Clear it preemptively.  */
	    if (this_plot->plot_style == LABELPOINTS
	    &&  (draw_contour && !this_plot->opt_out_of_contours)) {
		free(this_plot->labels->font);
		this_plot->labels->font = NULL;
	    }

	    if (crnt_param == 0
		&& this_plot->plot_style != PM3DSURFACE
		/* don't increment the default line/point properties if
		 * this_plot is an EXPLICIT pm3d surface plot */
		&& this_plot->plot_style != IMAGE
		&& this_plot->plot_style != RGBIMAGE
		&& this_plot->plot_style != RGBA_IMAGE
		/* same as above, for an (rgb)image plot */
		) {
		line_num++;
		if (draw_contour != CONTOUR_NONE)
		    line_num++;
		/* This reserves a second color for the back of a hidden3d surface */
		if (hidden3d && hiddenBacksideLinetypeOffset != 0)
		    line_num++;
	    }

	    /* now get the data... having to think hard here...
	     * first time through, we fill in this_plot. For second
	     * surface in file, we have to allocate another surface
	     * struct. BUT we may allocate this store only to
	     * find that it is merely some blank lines at end of file
	     * tp_3d_ptr is still pointing at next field of prev. plot,
	     * before :    prev_or_first -> this_plot -> possible_preallocated_store
	     *                tp_3d_ptr--^
	     * after  :    prev_or_first -> first -> second -> last -> possibly_more_store
	     *                                        tp_3d_ptr ----^
	     * if file is empty, tp_3d_ptr is not moved. this_plot continues
	     * to point at allocated storage, but that will be reused later
	     */

	    assert(this_plot == *tp_3d_ptr);

	    if (this_plot->plot_type == DATA3D) {

		/*{{{  read data */
		/* pointer to the plot of the first dataset (surface) in the file */
		struct surface_points *first_dataset = this_plot;
		int this_token = this_plot->token;

		/* Error check to handle missing or unreadable file */
		if (specs == DF_EOF) {
		    /* FIXME: plot2d does ++line_num here; needed in 3D also? */
		    this_plot->plot_type = NODATA;
		    goto SKIPPED_EMPTY_FILE;
		}

		do {
		    this_plot = *tp_3d_ptr;
		    assert(this_plot != NULL);

		    /* dont move tp_3d_ptr until we are sure we
		     * have read a surface
		     */

		    /* used by get_3ddata() */
		    this_plot->token = this_token;

		    df_return = get_3ddata(this_plot);

		    /* for second pass */
		    this_plot->token = c_token;
		    this_plot->iteration = plot_iterator ? plot_iterator->iteration : 0;

		    if (this_plot->num_iso_read == 0)
			this_plot->plot_type = NODATA;

		    /* Sep 2017 - Check for all points bad or out of range  */
		    /* (normally harmless but must not cause infinite loop) */
		    if (forever_iteration(plot_iterator)) {
			int ntotal, ninrange, nundefined;
			count_3dpoints(this_plot, &ntotal, &ninrange, &nundefined);
			if (ninrange == 0) {
			    this_plot->plot_type = NODATA;
			    goto SKIPPED_EMPTY_FILE;
			}
		    }

		    /* okay, we have read a surface */
		    plot_num++;
		    tp_3d_ptr = &(this_plot->next_sp);
		    if (df_return == DF_EOF)
			break;

		    /* there might be another surface so allocate
		     * and prepare another surface structure
		     * This does no harm if in fact there are
		     * no more surfaces to read
		     */

		    if ((this_plot = *tp_3d_ptr) != NULL) {
			if (this_plot->title) {
			    free(this_plot->title);
			    this_plot->title = NULL;
			}
		    } else {
			/* Allocate enough isosamples and samples */
			this_plot = *tp_3d_ptr = sp_alloc(0, 0, 0, 0);
		    }

		    this_plot->plot_type = DATA3D;
		    this_plot->iteration = plot_iterator ? plot_iterator->iteration : 0;
		    this_plot->plot_style = first_dataset->plot_style;
		    this_plot->lp_properties = first_dataset->lp_properties;
		    this_plot->fill_properties = first_dataset->fill_properties;
		    this_plot->arrow_properties = first_dataset->arrow_properties;
		    if ((this_plot->plot_style == LABELPOINTS)
		    ||  (this_plot->plot_style & PLOT_STYLE_HAS_POINT
			    && this_plot->lp_properties.p_type == PT_CHARACTER)) {
			this_plot->labels = new_text_label(-1);
			*(this_plot->labels) = *(first_dataset->labels);
			this_plot->labels->next = NULL;
		    }
		    strcpy(this_plot->pm3d_where, first_dataset->pm3d_where);
		} while (df_return != DF_EOF);

		df_close();

		/* Plot-type specific range-fiddling */
		if (!axis_array[FIRST_Z_AXIS].log
		&&  (this_plot->plot_style == IMPULSES
		    || this_plot->plot_style == BOXES))
		{
		    if (axis_array[FIRST_Z_AXIS].autoscale & AUTOSCALE_MIN) {
			if (axis_array[FIRST_Z_AXIS].min > 0)
			    axis_array[FIRST_Z_AXIS].min = 0;
		    }
		    if (axis_array[FIRST_Z_AXIS].autoscale & AUTOSCALE_MAX) {
			if (axis_array[FIRST_Z_AXIS].max < 0)
			    axis_array[FIRST_Z_AXIS].max = 0;
		    }
		}

		/*}}} */

	    } else if (this_plot->plot_type == FUNC3D
		   ||  this_plot->plot_type == KEYENTRY
		   ||  this_plot->plot_type == NODATA) {
		tp_3d_ptr = &(this_plot->next_sp);
		this_plot->token = c_token;	/* store for second pass */
		this_plot->iteration = plot_iterator ? plot_iterator->iteration : 0;

	    } else if (this_plot->plot_type == VOXELDATA){
		/* voxel data in an active vgrid must already be present */
		tp_3d_ptr = &(this_plot->next_sp);
		this_plot->token = c_token;	/* store for second pass */
		this_plot->iteration = plot_iterator ? plot_iterator->iteration : 0;
		/* FIXME: I worry that vxrange autoscales xrange and xrange autoscales vxrange */
		autoscale_one_point((&axis_array[FIRST_X_AXIS]), this_plot->vgrid->vxmin);
		autoscale_one_point((&axis_array[FIRST_X_AXIS]), this_plot->vgrid->vxmax);
		autoscale_one_point((&axis_array[FIRST_Y_AXIS]), this_plot->vgrid->vymin);
		autoscale_one_point((&axis_array[FIRST_Y_AXIS]), this_plot->vgrid->vymax);
		autoscale_one_point((&axis_array[FIRST_Z_AXIS]), this_plot->vgrid->vzmin);
		autoscale_one_point((&axis_array[FIRST_Z_AXIS]), this_plot->vgrid->vzmax);

	    } else {
		int_error(NO_CARET, "unexpected plot_type %d at plot3d:%d\n",
			this_plot->plot_type, __LINE__);
	    }

	    SKIPPED_EMPTY_FILE:
	    if (empty_iteration(plot_iterator) && this_plot)
		this_plot->plot_type = NODATA;
	    if (forever_iteration(plot_iterator) && !this_plot)
		int_error(NO_CARET, "unbounded iteration in something other than a data plot");
	    else if (forever_iteration(plot_iterator) && (this_plot->plot_type == NODATA))
		eof_during_iteration = TRUE;
	    else if (forever_iteration(plot_iterator) && (this_plot->plot_type != DATA3D))
		int_error(NO_CARET, "unbounded iteration in something other than a data plot");
	    else if (forever_iteration(plot_iterator))
		last_iteration_in_first_pass = plot_iterator->iteration_current;

	    /* restore original value of sample variables */
	    /* FIXME: somehow this_plot has changed since we saved sample_var! */
	    if (name_str && this_plot->sample_var) {
		this_plot->sample_var->udv_value = original_value_u;
		this_plot->sample_var2->udv_value = original_value_v;
	    }

	}			/* !is_definition() : end of scope of this_plot */

	if (crnt_param != 0) {
	    if (equals(c_token, ",")) {
		c_token++;
		continue;
	    } else
		break;
	}

	/* Iterate-over-plot mechanisms */
	if (eof_during_iteration) {
	    FPRINTF((stderr, "eof during iteration current %d\n", plot_iterator->iteration_current));
	    FPRINTF((stderr, "    last_iteration_in_first_pass %d\n", last_iteration_in_first_pass));
	    eof_during_iteration = FALSE;
	} else if (next_iteration(plot_iterator)) {
	    c_token = start_token;
	    continue;
	}

	plot_iterator = cleanup_iteration(plot_iterator);
	if (equals(c_token, ",")) {
	    c_token++;
	    plot_iterator = check_for_iteration();
	    if (forever_iteration(plot_iterator))
		if (last_iteration_in_first_pass != INT_MAX)
		    int_warn(NO_CARET, "splot does not support multiple unbounded iterations");
	} else
	    break;

    }				/* while(TRUE), ie first pass */


    if (parametric && crnt_param != 0)
	int_error(NO_CARET, "parametric function not fully specified");


/*** Second Pass: Evaluate the functions ***/
    /*
     * Everything is defined now, except the function data. We expect no
     * syntax errors, etc, since the above parsed it all. This makes the code
     * below simpler. If axis_array[FIRST_Y_AXIS].autoscale, the yrange may still change.
     * - eh ?  - z can still change.  x/y/z can change if we are parametric ??
     */

    if (some_functions) {
	/* I've changed the controlled variable in fn plots to u_min etc since
	 * it's easier for me to think parametric - 'normal' plot is after all
	 * a special case. I was confused about x_min being both minimum of
	 * x values found, and starting value for fn plots.
	 */
	double u_min, u_max, u_step, v_min, v_max, v_step;
	double u_isostep, v_isostep;
	AXIS_INDEX u_axis, v_axis;
	struct surface_points *this_plot;

	/* Make these point out the right 'u' and 'v' axis. In
	 * non-parametric mode, x is used as u, and y as v */
	u_axis = parametric ? U_AXIS : FIRST_X_AXIS;
	v_axis = parametric ? V_AXIS : FIRST_Y_AXIS;

	if (!parametric) {
	    /* Autoscaling tracked the visible axis coordinates.	*/
	    /* For nonlinear axes we must transform the limits back to the primary axis */
	    update_primary_axis_range(&axis_array[FIRST_X_AXIS]);
	    update_primary_axis_range(&axis_array[FIRST_Y_AXIS]);

	    /* Check that xmin -> xmax is not too small.
	     * Give error if xrange badly set from missing datafile error.
	     * Parametric fn can still set ranges.
	     * If there are no fns, we'll report it later as 'nothing to plot'.
	     */
	    axis_checked_extend_empty_range(FIRST_X_AXIS, "x range is invalid");
	    axis_checked_extend_empty_range(FIRST_Y_AXIS, "y range is invalid");
	}
	if (parametric && !some_data_files) {
	    /* parametric fn can still change x/y range */
	    if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN)
		axis_array[FIRST_X_AXIS].min = VERYLARGE;
	    if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX)
		axis_array[FIRST_X_AXIS].max = -VERYLARGE;
	    if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MIN)
		axis_array[FIRST_Y_AXIS].min = VERYLARGE;
	    if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MAX)
		axis_array[FIRST_Y_AXIS].max = -VERYLARGE;
	}

	/*{{{  figure ranges, restricting logscale limits to be positive */
	u_min = axis_log_value_checked(u_axis, axis_array[u_axis].min, "x range");
	u_max = axis_log_value_checked(u_axis, axis_array[u_axis].max, "x range");
	if (nonlinear(&axis_array[u_axis])) {
	    u_min = axis_array[u_axis].linked_to_primary->min;
	    u_max = axis_array[u_axis].linked_to_primary->max;
	} 
	v_min = axis_log_value_checked(v_axis, axis_array[v_axis].min, "y range");
	v_max = axis_log_value_checked(v_axis, axis_array[v_axis].max, "y range");
	if (nonlinear(&axis_array[v_axis])) {
	    v_min = axis_array[v_axis].linked_to_primary->min;
	    v_max = axis_array[v_axis].linked_to_primary->max;
	}
	/*}}} */

	if (samples_1 < 2 || samples_2 < 2 || iso_samples_1 < 2 ||
	    iso_samples_2 < 2) {
	    int_error(NO_CARET, "samples or iso_samples < 2. Must be at least 2.");
	}

	/* start over */
	this_plot = first_3dplot;
	c_token = begin_token;
	plot_iterator = check_for_iteration();

	/* We kept track of the last productive iteration in the first pass */
	if (forever_iteration(plot_iterator))
	    plot_iterator->iteration_end = last_iteration_in_first_pass;

	if (hidden3d) {
	    u_step = (u_max - u_min) / (iso_samples_1 - 1);
	    v_step = (v_max - v_min) / (iso_samples_2 - 1);
	} else {
	    u_step = (u_max - u_min) / (samples_1 - 1);
	    v_step = (v_max - v_min) / (samples_2 - 1);
	}

	u_isostep = (u_max - u_min) / (iso_samples_1 - 1);
	v_isostep = (v_max - v_min) / (iso_samples_2 - 1);


	/* Read through functions */
	while (TRUE) {
	    if (crnt_param == 0 && !was_definition)
		start_token = c_token;

	    if (is_definition(c_token)) {
		define();
		if (equals(c_token,","))
		    c_token++;
		was_definition = TRUE;
		continue;

	    } else {
		struct at_type *at_ptr;
		char *name_str;
		was_definition = FALSE;

		/* Forgive trailing comma on a multi-element plot command */
		if (END_OF_COMMAND || this_plot == NULL) {
		    int_warn(c_token, "ignoring trailing comma in plot command");
		    break;
		}

		/* Check for a sampling range
		 * Currently we only support sampling of pseudofiles '+' and '++'.
		 * This loop is for functions only, so the sampling range is ignored.
		 */
		(void)parse_range(U_AXIS);
		(void)parse_range(V_AXIS);

		dummy_func = &plot_func;
		name_str = string_or_express(&at_ptr);

		if (equals(c_token, "keyentry")) {

		} else if (!name_str) {                /* func to plot */
		    /*{{{  evaluate function */
		    struct iso_curve *this_iso = this_plot->iso_crvs;
		    int num_sam_to_use, num_iso_to_use;

		    /* crnt_param is used as the axis number.  As the
		     * axis array indices are ordered z, y, x, we have
		     * to count *backwards*, starting starting at 2,
		     * to properly store away contents to x, y and
		     * z. The following little gimmick does that. */
		    if (parametric)
			crnt_param = (crnt_param + 2) % 3;

		    plot_func.at = at_ptr;

		    num_iso_to_use = iso_samples_2;
		    num_sam_to_use = hidden3d ? iso_samples_1 : samples_1;

		    calculate_set_of_isolines(crnt_param, FALSE, &this_iso,
					      v_axis, v_min, v_isostep,
					      num_iso_to_use,
					      u_axis, u_min, u_step,
					      num_sam_to_use);

		    if (!hidden3d) {
			num_iso_to_use = iso_samples_1;
			num_sam_to_use = samples_2;

			calculate_set_of_isolines(crnt_param, TRUE, &this_iso,
						  u_axis, u_min, u_isostep,
						  num_iso_to_use,
						  v_axis, v_min, v_step,
						  num_sam_to_use);
		    }
		    /*}}} */
		}		/* end of ITS A FUNCTION TO PLOT */
		/* we saved it from first pass */
		c_token = this_plot->token;

		/* we may have seen this one data file in multiple iterations */
		i = this_plot->iteration;
		do {
		    this_plot = this_plot->next_sp;
		} while (this_plot
			&& this_plot->token == c_token
			&& this_plot->iteration == i
			);

	    }			/* !is_definition */

	    /* Iterate-over-plot mechanism */
	    if (crnt_param == 0 && next_iteration(plot_iterator)) {
		c_token = start_token;
		continue;
	    }

	    if (crnt_param == 0)
		plot_iterator = cleanup_iteration(plot_iterator);

	    if (equals(c_token, ",")) {
		c_token++;
		if (crnt_param == 0)
		    plot_iterator = check_for_iteration();
		if (forever_iteration(plot_iterator))
		    plot_iterator->iteration_end = last_iteration_in_first_pass;
	    } else
		break;

	}			/* while(TRUE) */


	if (parametric) {
	    /* Now actually fix the plot triplets to be single plots. */
	    parametric_3dfixup(first_3dplot, &plot_num);
	}
    }				/* some functions */

    /* if first_3dplot is NULL, we have no functions or data at all.
     * This can happen if you type "splot x=5", since x=5 is a variable assignment.
     */
    if (plot_num == 0 || first_3dplot == NULL) {
	int_error(c_token, "no functions or data to plot");
    }

    /* In "set view map" mode it is possible for the x1/x2 or y1/y2 axes
     * to be linked.  If so, reconcile their data limits before plotting.
     */
    if (splot_map) {
	if (axis_array[FIRST_X_AXIS].linked_to_secondary) {
	    AXIS *primary = &axis_array[FIRST_X_AXIS];
	    AXIS *secondary = &axis_array[SECOND_X_AXIS];
	    reconcile_linked_axes(primary, secondary);
	}
	if (axis_array[FIRST_Y_AXIS].linked_to_secondary) {
	    AXIS *primary = &axis_array[FIRST_Y_AXIS];
	    AXIS *secondary = &axis_array[SECOND_Y_AXIS];
	    reconcile_linked_axes(primary, secondary);
	}
    }

    if (nonlinear(&axis_array[FIRST_X_AXIS])) {
	/* Transfer observed data or function ranges back to primary axes */
	update_primary_axis_range(&axis_array[FIRST_X_AXIS]);
	extend_primary_ticrange(&axis_array[FIRST_X_AXIS]);
	axis_check_empty_nonlinear(&axis_array[FIRST_X_AXIS]);
    } else {
	axis_checked_extend_empty_range(FIRST_X_AXIS, "All points x value undefined");
	axis_check_range(FIRST_X_AXIS);
    }
    if (nonlinear(&axis_array[FIRST_Y_AXIS])) {
	/* Transfer observed data or function ranges back to primary axes */
	update_primary_axis_range(&axis_array[FIRST_Y_AXIS]);
	extend_primary_ticrange(&axis_array[FIRST_Y_AXIS]);
	axis_check_empty_nonlinear(&axis_array[FIRST_Y_AXIS]);
    } else {
	axis_checked_extend_empty_range(FIRST_Y_AXIS, "All points y value undefined");
	axis_check_range(FIRST_Y_AXIS);
    }
    if (nonlinear(&axis_array[FIRST_Z_AXIS])) {
	update_primary_axis_range(&axis_array[FIRST_Z_AXIS]);
	extend_primary_ticrange(&axis_array[FIRST_Z_AXIS]);
    } else {
	axis_checked_extend_empty_range(FIRST_Z_AXIS, splot_map ? NULL : "All points z value undefined");
	axis_check_range(FIRST_Z_AXIS);
    }

    setup_tics(&axis_array[FIRST_X_AXIS], 20);
    setup_tics(&axis_array[FIRST_Y_AXIS], 20);
    setup_tics(&axis_array[FIRST_Z_AXIS], 20);
    if (splot_map) {
	setup_tics(&axis_array[SECOND_X_AXIS], 20);
	setup_tics(&axis_array[SECOND_Y_AXIS], 20);
    }

    set_plot_with_palette(plot_num, MODE_SPLOT);
    if (is_plot_with_palette()) {
	set_cbminmax();
	axis_checked_extend_empty_range(COLOR_AXIS, "All points of colorbox value undefined");
	setup_tics(&axis_array[COLOR_AXIS], 20);
    }

    if (plot_num == 0 || first_3dplot == NULL) {
	int_error(c_token, "no functions or data to plot");
    }
    /* Creates contours if contours are to be plotted as well. */

    if (draw_contour) {
	struct surface_points *this_plot;
	for (this_plot = first_3dplot, i = 0;
	     i < plot_num;
	     this_plot = this_plot->next_sp, i++) {

	    if (this_plot->contours) {
		struct gnuplot_contours *cntrs = this_plot->contours;

		while (cntrs) {
		    struct gnuplot_contours *cntr = cntrs;
		    cntrs = cntrs->next;
		    free(cntr->coords);
		    free(cntr);
		}
		this_plot->contours = NULL;
	    }

	    /* Make sure this one can be contoured. */
	    if (this_plot->plot_style == VECTOR
	    ||  this_plot->plot_style == IMAGE
	    ||  this_plot->plot_style == RGBIMAGE
	    ||  this_plot->plot_style == RGBA_IMAGE)
		continue;
	    if (this_plot->plot_type == NODATA)
		continue;
	    if (this_plot->plot_type == KEYENTRY)
		continue;

	    /* Allow individual surfaces to opt out of contouring */
	    if (this_plot->opt_out_of_contours)
		continue;

	    if (!this_plot->has_grid_topology) {
		int_warn(NO_CARET,"Cannot contour non grid data. Please use \"set dgrid3d\".");
	    } else if (this_plot->plot_type == DATA3D) {
		this_plot->contours = contour(this_plot->num_iso_read,
					      this_plot->iso_crvs);
	    } else {
		this_plot->contours = contour(iso_samples_2,
					      this_plot->iso_crvs);
	    }
	}
    }				/* draw_contour */

    /* Images don't fit the grid model.  (The image data correspond
     * to pixel centers.)  To make image work in hidden 3D, add
     * another non-visible phantom surface of only four points
     * outlining the image.  Opt out of hidden3d for the {RGB}IMAGE
     * to avoid processing large amounts of data.
     */
    if (hidden3d && plot_num) {
	struct surface_points *this_plot = first_3dplot;
	do {
	    if ((this_plot->plot_style == IMAGE || this_plot->plot_style == RGBIMAGE)
	    && (this_plot->image_properties.nrows > 0 && this_plot->image_properties.ncols > 0)
	    && !(this_plot->opt_out_of_hidden3d)) {

		struct surface_points *new_plot = sp_alloc(2, 0, 0, 2);

		/* Construct valid 2 x 2 parallelogram. */
		new_plot->num_iso_read = 2;
		new_plot->iso_crvs->p_count = 2;
		new_plot->iso_crvs->next->p_count = 2;
		new_plot->next_sp = this_plot->next_sp;
		this_plot->next_sp = new_plot;

		/* Set up hidden3d behavior, no visible lines but
		 * opaque to items behind the parallelogram. */
		new_plot->plot_style = SURFACEGRID;
		new_plot->opt_out_of_surface = TRUE;
		new_plot->opt_out_of_contours = TRUE;
		new_plot->has_grid_topology = TRUE;
		new_plot->hidden3d_top_linetype = LT_NODRAW;
		new_plot->plot_type = DATA3D;
		new_plot->opt_out_of_hidden3d = FALSE;
		new_plot->opt_out_of_dgrid3d = FALSE;

		/* Compute the geometry of the phantom */
		process_image(this_plot, IMG_UPDATE_CORNERS);

		/* Advance over the phantom */
		plot_num++;
		this_plot = this_plot->next_sp;
	    }
	    this_plot = this_plot->next_sp;
	} while (this_plot);
    }

    /* if we get here, all went well, so record the line for replot */
    if (plot_token != -1) {
	/* note that m_capture also frees the old replot_line */
	m_capture(&replot_line, plot_token, c_token - 1);
	plot_token = -1;
	fill_gpval_string("GPVAL_LAST_PLOT", replot_line);
    }
    /* record that all went well */
    plot3d_num=plot_num;

    /* perform the plot */
    if (table_mode) {
	print_3dtable(plot_num);

    } else {
	do_3dplot(first_3dplot, plot_num, NORMAL_REPLOT);

	/* after do_3dplot(), axis_array[].min and .max
	 * contain the plotting range actually used (rounded
	 * to tic marks, not only the min/max data values)
	 * --> save them now for writeback if requested
	 */
	save_writeback_all_axes();

	/* Mark these plots as safe for quick refresh */
	SET_REFRESH_OK(E_REFRESH_OK_3D, plot_num);
    }

    /* update GPVAL_ variables available to user */
    update_gpval_variables(1);
}



/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are guaranteed to occur in (x,y,z) triplets while preserving
 * the non-FUNC type plots intact.  This means we have to work our way
 * through various lists.  Examples (hand checked):
 * start_plot:F1->F2->F3->NULL ==> F3->NULL
 * start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F3->F6->NULL
 * start_plot:F1->F2->F3->D1->D2->F4->F5->F6->D3->NULL ==>
 * F3->D1->D2->F6->D3->NULL
 *
 * x and y ranges now fixed in eval_3dplots
 */
static void
parametric_3dfixup(struct surface_points *start_plot, int *plot_num)
{
    struct surface_points *xp, *new_list, *free_list = NULL;
    struct surface_points **last_pointer = &new_list;
    int i, surface;

    /*
     * Ok, go through all the plots and move FUNC3D types together.  Note:
     * this originally was written to look for a NULL next pointer, but
     * gnuplot wants to be sticky in grabbing memory and the right number of
     * items in the plot list is controlled by the plot_num variable.
     *
     * Since gnuplot wants to do this sticky business, a free_list of
     * surface_points is kept and then tagged onto the end of the plot list
     * as this seems more in the spirit of the original memory behavior than
     * simply freeing the memory.  I'm personally not convinced this sort of
     * concern is worth it since the time spent computing points seems to
     * dominate any garbage collecting that might be saved here...
     */
    new_list = xp = start_plot;
    for (surface = 0; surface < *plot_num; surface++) {
	if (xp->plot_type == FUNC3D) {
	    struct surface_points *yp = xp->next_sp;
	    struct surface_points *zp = yp->next_sp;

	    /* Here's a FUNC3D parametric function defined as three parts.
	     * Go through all the points and assign the x's and y's from xp and
	     * yp to zp. min/max already done
	     */
	    struct iso_curve *xicrvs = xp->iso_crvs;
	    struct iso_curve *yicrvs = yp->iso_crvs;
	    struct iso_curve *zicrvs = zp->iso_crvs;

	    (*plot_num) -= 2;

	    assert(INRANGE < OUTRANGE && OUTRANGE < UNDEFINED);

	    while (zicrvs) {
		struct coordinate *xpoints = xicrvs->points;
		struct coordinate *ypoints = yicrvs->points;
		struct coordinate *zpoints = zicrvs->points;

		for (i = 0; i < zicrvs->p_count; ++i) {
		    zpoints[i].x = xpoints[i].z;
		    zpoints[i].y = ypoints[i].z;
		    if (zpoints[i].type < xpoints[i].type)
			zpoints[i].type = xpoints[i].type;
		    if (zpoints[i].type < ypoints[i].type)
			zpoints[i].type = ypoints[i].type;

		}
		xicrvs = xicrvs->next;
		yicrvs = yicrvs->next;
		zicrvs = zicrvs->next;
	    }

	    /* add xp and yp to head of free list */
	    assert(xp->next_sp == yp);
	    yp->next_sp = free_list;
	    free_list = xp;

	    /* add zp to tail of new_list */
	    *last_pointer = zp;
	    last_pointer = &(zp->next_sp);
	    xp = zp->next_sp;
	} else {		/* its a data plot */
	    assert(*last_pointer == xp);	/* think this is true ! */
	    last_pointer = &(xp->next_sp);
	    xp = xp->next_sp;
	}
    }

    /* Ok, append free list and write first_plot */
    *last_pointer = free_list;
    first_3dplot = new_list;
}

static void load_contour_label_options (struct text_label *contour_label)
{
    struct lp_style_type *lp = &(contour_label->lp_properties);
    lp->p_interval = clabel_interval;
    lp_parse(lp, LP_ADHOC, TRUE);
}

/* Count the number of data points but do nothing with them */
static void
count_3dpoints(struct surface_points *plot, int *ntotal, int *ninrange, int *nundefined)
{
    int i;
    struct iso_curve *icrvs = plot->iso_crvs;
    *ntotal = *ninrange = *nundefined = 0;

    while (icrvs) {
	struct coordinate *point;
	for (i = 0; i < icrvs->p_count; i++) {
	    point = &(icrvs->points[i]);
	    (*ntotal)++;
	    if (point->type == INRANGE)
		(*ninrange)++;
	    if (point->type == UNDEFINED)
		(*nundefined)++;
	}
	icrvs = icrvs->next;
    }
}

