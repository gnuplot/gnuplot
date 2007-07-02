#ifndef lint
static char *RCSid() { return RCSid("$Id: plot3d.c,v 1.144 2007/05/10 22:52:46 sfeam Exp $"); }
#endif

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
#include "binary.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "eval.h"
#include "graph3d.h"
#include "misc.h"
#include "parse.h"
#include "setshow.h"
#include "term_api.h"
#include "tabulate.h"
#include "util.h"
#include "pm3d.h"
#ifdef BINARY_DATA_FILE
#include "plot.h"
#endif

#ifdef EAM_DATASTRINGS
#include "plot2d.h" /* Only for store_label() */
#endif

#ifdef THIN_PLATE_SPLINES_GRID
# include "matrix.h"
#endif

#ifndef _Windows
# include "help.h"
#endif

/* global variables exported by this module */

t_data_mapping mapping3d = MAP3D_CARTESIAN;

int dgrid3d_row_fineness = 10;
int dgrid3d_col_fineness = 10;
int dgrid3d_norm_value = 1;
TBOOLEAN dgrid3d = FALSE;

/* static prototypes */

static void calculate_set_of_isolines __PROTO((AXIS_INDEX value_axis, TBOOLEAN cross, struct iso_curve **this_iso,
					       AXIS_INDEX iso_axis, double iso_min, double iso_step, int num_iso_to_use,
					       AXIS_INDEX sam_axis, double sam_min, double sam_step, int num_sam_to_use,
					       TBOOLEAN need_palette));
static void get_3ddata __PROTO((struct surface_points * this_plot));
static void eval_3dplots __PROTO((void));
static void grid_nongrid_data __PROTO((struct surface_points * this_plot));
static void parametric_3dfixup __PROTO((struct surface_points * start_plot, int *plot_num));
static struct surface_points * sp_alloc __PROTO((int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2));
static void sp_replace __PROTO((struct surface_points *sp, int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2));
#ifdef THIN_PLATE_SPLINES_GRID
static double splines_kernel __PROTO((double h));
#endif

/* the curves/surfaces of the plot */
struct surface_points *first_3dplot = NULL;
static struct udft_entry plot_func;

int plot3d_num=0;

/* HBB 20000508: moved these functions to the only module that uses them
 * so they can be turned 'static' */
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
 * cleanup unstead */
void
sp_free(struct surface_points *sp)
{
    while (sp) {
	struct surface_points *next = sp->next_sp;
	if (sp->title)
	    free(sp->title);

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

#ifdef EAM_DATASTRINGS
	if (sp->labels) {
	    free_labels(sp->labels);
	    sp->labels = (struct text_label *)NULL;
	}
#endif

	free(sp);
	sp = next;
    }
}



/* support for dynamic size of input line */

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
    AXIS_INDEX u_axis, v_axis;

    is_3d_plot = TRUE;

    /* change view to become map if requested by 'set view map' */
    if (splot_map == TRUE)
	splot_map_activate();

    if (parametric && strcmp(set_dummy_var[0], "t") == 0) {
	strcpy(set_dummy_var[0], "u");
	strcpy(set_dummy_var[1], "v");
    }

    /* put stuff into arrays to simplify access */
    AXIS_INIT3D(FIRST_X_AXIS, 0, 0);
    AXIS_INIT3D(FIRST_Y_AXIS, 0, 0);
    AXIS_INIT3D(FIRST_Z_AXIS, 0, 1);
    AXIS_INIT3D(U_AXIS, 1, 0);
    AXIS_INIT3D(V_AXIS, 1, 0);
    AXIS_INIT3D(COLOR_AXIS, 0, 1);

    if (!term)			/* unknown */
	int_error(c_token, "use 'set term' to set terminal type first");

    u_axis = (parametric ? U_AXIS : FIRST_X_AXIS);
    v_axis = (parametric ? V_AXIS : FIRST_Y_AXIS);

    PARSE_NAMED_RANGE(u_axis, dummy_token0);
    if (splot_map == TRUE && !parametric) /* v_axis==FIRST_Y_AXIS */
	splot_map_deactivate();
    PARSE_NAMED_RANGE(v_axis, dummy_token1);
    if (splot_map == TRUE && !parametric) /* v_axis==FIRST_Y_AXIS */
	splot_map_activate();

    if (parametric) {
	PARSE_RANGE(FIRST_X_AXIS);
	if (splot_map == TRUE)
	    splot_map_deactivate();
	PARSE_RANGE(FIRST_Y_AXIS);
	if (splot_map == TRUE)
	    splot_map_activate();
    }				/* parametric */
    PARSE_RANGE(FIRST_Z_AXIS);
    CHECK_REVERSE(FIRST_X_AXIS);
    CHECK_REVERSE(FIRST_Y_AXIS);
    CHECK_REVERSE(FIRST_Z_AXIS);

    /* use the default dummy variable unless changed */
    if (dummy_token0 >= 0)
	copy_str(c_dummy_var[0], dummy_token0, MAX_ID_LEN);
    else
	(void) strcpy(c_dummy_var[0], set_dummy_var[0]);

    if (dummy_token1 >= 0)
	copy_str(c_dummy_var[1], dummy_token1, MAX_ID_LEN);
    else
	(void) strcpy(c_dummy_var[1], set_dummy_var[1]);

    eval_3dplots();
}

#ifdef THIN_PLATE_SPLINES_GRID

static double
splines_kernel(double h)
{
#if 0
    /* this is normaly not useful ... */
    h = fabs(h);

    if (h != 0.0) {
#else
    if (h > 0) {
#endif
	return h * h * log(h);
    } else {
	return 0;
    }
}

#endif

static void
grid_nongrid_data(struct surface_points *this_plot)
{
    int i, j, k;
    double x, y, z, w, dx, dy, xmin, xmax, ymin, ymax;
    struct iso_curve *old_iso_crvs = this_plot->iso_crvs;
    struct iso_curve *icrv, *oicrv, *oicrvs;

#ifdef THIN_PLATE_SPLINES_GRID
    double *b, **K, *xx, *yy, *zz, d;
    int *indx, numpoints;
#endif

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
	struct coordinate GPHUGE *points = icrv->points;

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

#ifdef THIN_PLATE_SPLINES_GRID
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
	struct coordinate GPHUGE *opoints = oicrv->points;

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
#endif /* THIN_PLATE_SPLINES_GRID */

    for (i = 0, x = xmin; i < dgrid3d_col_fineness; i++, x += dx) {
	struct coordinate GPHUGE *points;

	icrv = iso_alloc(dgrid3d_row_fineness + 1);
	icrv->p_count = dgrid3d_row_fineness;
	icrv->next = this_plot->iso_crvs;
	this_plot->iso_crvs = icrv;
	points = icrv->points;

	for (j = 0, y = ymin; j < dgrid3d_row_fineness; j++, y += dy, points++) {
	    z = w = 0.0;

	    /* as soon as ->type is changed to UNDEFINED, break out of
	     * two inner loops! */
	    points->type = INRANGE;

#ifdef THIN_PLATE_SPLINES_GRID
	    z = b[numpoints];
	    for (k = 0; k < numpoints; k++) {
		double dx = xx[k] - x, dy = yy[k] - y;
		z = z - b[k] * splines_kernel(sqrt(dx * dx + dy * dy));
	    }
	    z = z + b[numpoints + 1] * x + b[numpoints + 2] * y;
#else
	    for (oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
		struct coordinate GPHUGE *opoints = oicrv->points;
		for (k = 0; k < oicrv->p_count; k++, opoints++) {
		    double dist, dist_x = fabs(opoints->x - x), dist_y = fabs(opoints->y - y);
		    switch (dgrid3d_norm_value) {
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
			dist = pow(dist_x, (double) dgrid3d_norm_value) +
			    pow(dist_y, (double) dgrid3d_norm_value);
			break;
		    }

		    /* The weight of this point is inverse proportional
		     * to the distance.
		     */
		    if (dist == 0.0) {
			/* HBB 981209: revised flagging as undefined */
			/* Supporting all those infinities on various
			 * platforms becomes tiresome, to say the least :-(
			 * Let's just return the first z where this happens,
			 * unchanged, and be done with this, period. */
			points->type = UNDEFINED;
			z = opoints->z;
			w = 1.0;
			break;	/* out of inner loop */
		    } else {
			dist = 1.0 / dist;
			z += opoints->z * dist;
			w += dist;
		    }
		}
		if (points->type != INRANGE)
		    break;	/* out of the second-inner loop as well ... */
	    }
#endif /* THIN_PLATE_SPLINES_GRID */

	    /* Now that we've escaped the loops safely, we know that we
	     * do have a good value in z and w, so we can proceed just as
	     * if nothing had happened at all. Nice, isn't it? */
	    points->type = INRANGE;

	    /* HBB 20010424: if log x or log y axis, we don't want to
	     * log() the value again --> just store it, and trust that
	     * it's always inrange */
	    points->x = x;
	    points->y = y;

#ifndef THIN_PLATE_SPLINES_GRID
	    z = z / w;
#endif

	    STORE_WITH_LOG_AND_UPDATE_RANGE(points->z, z, points->type, z_axis, NOOP, continue);

	    if (this_plot->pm3d_color_from_column)
		int_error(NO_CARET, "Gridding of the color column is not implemented");
	    else {
		COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(points->CRD_COLOR, z, points->type, COLOR_AXIS, NOOP, continue);
	    }
	}
    }

#ifdef THIN_PLATE_SPLINES_GRID
    free(K);
    free(xx);
    free(indx);
#endif

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
static void
get_3ddata(struct surface_points *this_plot)
{
    int xdatum = 0;
    int ydatum = 0;
    int j;
    double v[MAXDATACOLS];
    int pt_in_iso_crv = 0;
    struct iso_curve *this_iso;

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

#ifndef BINARY_DATA_FILE /* NO LONGER REQUIRED FOR GENERAL BINARY OR MATRIX BINARY DATA */
    if (df_matrix)
	xdatum = df_3dmatrix(this_plot, NEED_PALETTE(this_plot));
    else {
#else
    if (df_matrix) {
	this_plot->has_grid_topology = TRUE;
    }

    {
#endif
	/*{{{  read surface from text file */
	struct iso_curve *local_this_iso = iso_alloc(samples_1);
	struct coordinate GPHUGE *cp;
	struct coordinate GPHUGE *cptail = NULL; /* Only for VECTOR plots */
	double x, y, z;
	double xtail, ytail, ztail;
	double color = VERYLARGE;
	int pm3d_color_from_column = FALSE;
#define color_from_column(x) pm3d_color_from_column = x

#ifdef EAM_DATASTRINGS
	if (this_plot->plot_style == LABELPOINTS)
	    expect_string( 4 );
#endif

	if (this_plot->plot_style == VECTOR) {
	    local_this_iso->next = iso_alloc(samples_1);
	    local_this_iso->next->p_count = 0;
	}

#ifdef HAVE_LOCALE_H
	/* If the user has set an explicit locale for numeric input, apply it */
	/* here so that it affects data fields read from the input file.      */
	if (numeric_locale)
	    setlocale(LC_NUMERIC,numeric_locale);
#endif

	while ((j = df_readline(v,MAXDATACOLS)) != DF_EOF) {
	    if (j == DF_SECOND_BLANK)
		break;		/* two blank lines */
	    if (j == DF_FIRST_BLANK) {

#if defined(WITH_IMAGE) && defined(BINARY_DATA_FILE)
		/* Images are in a sense similar to isocurves.
		 * However, the routine for images is written to
		 * compute the two dimensions of coordinates by
		 * examining the data alone.  That way it can be used
		 * in the 2D plots, for which there is no isoline
		 * record.  So, toss out isoline information for
		 * images.
		 */
		if ((this_plot->plot_style == IMAGE)
		    || (this_plot->plot_style == RGBIMAGE))
		    continue;
#endif
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
		cptail = local_this_iso->next->points + xdatum;
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
		return;
	    }

	    if (j < df_no_use_specs)
		int_error(this_plot->token,
			"Wrong number of columns in input data - line %d",
			df_line_number);

	    /* After the first three columns it gets messy because */
	    /* different plot styles assume different contents in the columns */
	    if (j >= 4) {
		if (( this_plot->plot_style == POINTSTYLE
		   || this_plot->plot_style == LINESPOINTS)
		&&  this_plot->lp_properties.p_size == PTSZ_VARIABLE) {
		    cp->CRD_PTSIZE = v[3];
		    color = z;
		    color_from_column(FALSE);
		}
#ifdef EAM_DATASTRINGS
		else if (this_plot->plot_style == LABELPOINTS) {
		/* 4th column holds label text rather than color */
		/* text = df_tokens[3]; */
		    color = z;
		    color_from_column(FALSE);
		}
#endif
		else {
		    color = v[3];
		    color_from_column(TRUE);
		}
	    }

	    if (j >= 5) {
		if ((this_plot->plot_style == POINTSTYLE
		   || this_plot->plot_style == LINESPOINTS)
		&&  this_plot->lp_properties.p_size == PTSZ_VARIABLE) {
		    color = v[4];
		    color_from_column(TRUE);
		}
#ifdef EAM_DATASTRINGS
		if (this_plot->plot_style == LABELPOINTS) {
		    /* take color from an explicitly given 5th column */
		    color = v[4];
		    color_from_column(TRUE);
		}
#endif
	    }

	    if (j >= 6) {
		if (this_plot->plot_style == VECTOR) {
		    xtail = x + v[3];
		    ytail = y + v[4];
		    ztail = z + v[5];
		    color_from_column(FALSE);
		}
	    }
#undef color_from_column


	    /* Adjust for logscales. Set min/max and point types. Store in cp.
	     * The macro cannot use continue, as it is wrapped in a loop.
	     * I regard this as correct goto use
	     */
	    cp->type = INRANGE;
	    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->x, x, cp->type, x_axis, NOOP, goto come_here_if_undefined);
	    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->y, y, cp->type, y_axis, NOOP, goto come_here_if_undefined);
	    if (this_plot->plot_style == VECTOR) {
		cptail->type = INRANGE;	    
		STORE_WITH_LOG_AND_UPDATE_RANGE(cptail->x, xtail, cp->type, x_axis, NOOP, goto come_here_if_undefined);
		STORE_WITH_LOG_AND_UPDATE_RANGE(cptail->y, ytail, cp->type, y_axis, NOOP, goto come_here_if_undefined);
	    }

	    if (dgrid3d) {
		/* HBB 20010424: in dgrid3d mode, delay log() taking
		 * and scaling until after the dgrid process. Only for
		 * z, not for x and y, so we can layout the newly
		 * created created grid more easily. */
		cp->z = z;
		if (this_plot->plot_style == VECTOR)
		    cptail->z = ztail;
	    } else {
		STORE_WITH_LOG_AND_UPDATE_RANGE(cp->z, z, cp->type, z_axis, NOOP, goto come_here_if_undefined);
		if (this_plot->plot_style == VECTOR)
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cptail->z, ztail, cp->type, z_axis, NOOP, goto come_here_if_undefined);

		if (NEED_PALETTE(this_plot)) {
		    if (pm3d_color_from_column) {
			COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, color, cp->type, COLOR_AXIS, NOOP, goto come_here_if_undefined);
		    } else {
			COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, z, cp->type, COLOR_AXIS, NOOP, goto come_here_if_undefined);
		    }
		}
	    }

#ifdef EAM_DATASTRINGS
	    /* At this point we have stored the point coordinates. Now we need to copy */
	    /* x,y,z into the text_label structure and add the actual text string.     */
	    if (this_plot->plot_style == LABELPOINTS)
		store_label(this_plot->labels, cp, xdatum, df_tokens[3], color);
#endif

#ifdef WITH_IMAGE
	    if (this_plot->plot_style == RGBIMAGE) {
		/* There is only one color axis, but we are storing components in
		 * different variables.  Place all components on the same axis.
		 * That will maintain a consistent mapping amongst the components.
		 */
		COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_R, v[3], cp->type, COLOR_AXIS, NOOP, cp->CRD_COLOR=-VERYLARGE);
		COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_G, v[4], cp->type, COLOR_AXIS, NOOP, cp->CRD_COLOR=-VERYLARGE);
		COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_B, v[5], cp->type, COLOR_AXIS, NOOP, cp->CRD_COLOR=-VERYLARGE);
	    }
#endif

	come_here_if_undefined:
	    /* some may complain, but I regard this as the correct use of goto */
	    ++xdatum;
	}			/* end of whileloop - end of surface */

#ifdef HAVE_LOCALE_H
	/* We are finished reading user input; return to C locale for internal use */
	if (numeric_locale)
	    setlocale(LC_NUMERIC,"C");
#endif

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

    if (dgrid3d && this_plot->num_iso_read > 0)
        grid_nongrid_data(this_plot);

    /* This check used to be done in graph3d */
    if (X_AXIS.min == VERYLARGE || X_AXIS.max == -VERYLARGE ||
	Y_AXIS.min == VERYLARGE || Y_AXIS.max == -VERYLARGE || 
	Z_AXIS.min == VERYLARGE || Z_AXIS.max == -VERYLARGE) {
	    /* FIXME: Should we set plot type to NODATA? */
	    /* But in the case of 'set view map' we may not care about Z */
	    int_warn(NO_CARET,
		"No usable data in this plot to auto-scale axis range");
	    }

    if (this_plot->num_iso_read <= 1)
	this_plot->has_grid_topology = FALSE;
    if (this_plot->has_grid_topology && !hidden3d) {
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
}

/* HBB 20000501: code isolated from eval_3dplots(), where practically
 * identical code occured twice, for direct and crossing isolines,
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
    int num_sam_to_use,
    TBOOLEAN need_palette)
{
    int i, j;
    struct coordinate GPHUGE *points = (*this_iso)->points;
    int do_update_color = need_palette && (!parametric || (parametric && value_axis == FIRST_Z_AXIS));

    for (j = 0; j < num_iso_to_use; j++) {
	double iso = iso_min + j * iso_step;
	/* HBB 20000501: with the new code, it should
	 * be safe to rely on the actual 'v' axis not
	 * to be improperly logscaled... */
	(void) Gcomplex(&plot_func.dummy_values[cross ? 0 : 1],
			AXIS_DE_LOG_VALUE(iso_axis, iso), 0.0);

	for (i = 0; i < num_sam_to_use; i++) {
	    double sam = sam_min + i * sam_step;
	    struct value a;
	    double temp;

	    (void) Gcomplex(&plot_func.dummy_values[cross ? 1 : 0],
			    AXIS_DE_LOG_VALUE(sam_axis, sam), 0.0);

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
	    STORE_WITH_LOG_AND_UPDATE_RANGE(points[i].z, temp, points[i].type,
					    value_axis, NOOP, NOOP);
	    if (do_update_color) {
		COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(points[i].CRD_COLOR, temp, points[i].type, COLOR_AXIS, NOOP, NOOP);
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
    int begin_token;
    TBOOLEAN some_data_files = FALSE, some_functions = FALSE;
    int plot_num, line_num, point_num;
    /* part number of parametric function triplet: 0 = z, 1 = y, 2 = x */
    int crnt_param = 0;
    char *xtitle;
    char *ytitle;
    legend_key *key = &keyT;

    /* Reset first_3dplot. This is usually done at the end of this function.
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
    point_num = 0;		/* default point type */

    xtitle = NULL;
    ytitle = NULL;

    begin_token = c_token;

/*** First Pass: Read through data files ***/
    /*
     * This pass serves to set the x/yranges and to parse the command, as
     * well as filling in every thing except the function data. That is done
     * after the x/yrange is defined.
     */
    check_for_iteration();

    while (TRUE) {
	if (END_OF_COMMAND)
	    int_error(c_token, "function to plot expected");

	if (crnt_param == 0)
	    start_token = c_token;

	if (is_definition(c_token)) {
	    define();
	} else {
	    int specs = -1;
	    struct surface_points *this_plot;

	    char *name_str;
	    TBOOLEAN duplication = FALSE;
	    TBOOLEAN set_title = FALSE, set_with = FALSE;
	    TBOOLEAN set_lpstyle = FALSE;
	    TBOOLEAN checked_once = FALSE;
#ifdef EAM_DATASTRINGS
	    TBOOLEAN set_labelstyle = FALSE;
#endif
	    if (!parametric || crnt_param == 0)
		start_token = c_token;

	    dummy_func = &plot_func;
	    /* WARNING: do NOT free name_str */
	    /* FIXME: could also be saved in this_plot */
	    name_str = string_or_express(NULL);
	    dummy_func = NULL;
	    if (name_str) {
		/*{{{  data file to plot */
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

		df_set_plot_mode(MODE_SPLOT);
		specs = df_open(name_str, MAXDATACOLS);
#ifdef BINARY_DATA_FILE
		if (df_matrix)
		    this_plot->has_grid_topology = TRUE;
#endif

		/* parses all datafile-specific modifiers */
		/* we will load the data after parsing title,with,... */

		/* for capture to key */
		this_plot->token = end_token = c_token - 1;
		this_plot->plot_num = plot_num;

		/* this_plot->token is temporary, for errors in get_3ddata() */

		if (specs < 3) {
		    if (axis_array[FIRST_X_AXIS].is_timedata) {
			int_error(c_token, "Need full using spec for x time data");
		    }
		    if (axis_array[FIRST_Y_AXIS].is_timedata) {
			int_error(c_token, "Need full using spec for y time data");
		    }
		} 
		df_axis[0] = FIRST_X_AXIS;
		df_axis[1] = FIRST_Y_AXIS;
		df_axis[2] = FIRST_Z_AXIS;

		/*}}} */

	    } else {		/* function to plot */

		/*{{{  function */
		++plot_num;
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
		/* ignore it for now */
		some_functions = TRUE;
		end_token = c_token - 1;
		/*}}} */

	    }			/* end of IS THIS A FILE OR A FUNC block */

	    /* clear current title, if exist */
	    if (this_plot->title) {
		free(this_plot->title);
		this_plot->title = NULL;
	    }

	    /* default line and point types, no palette */
	    this_plot->lp_properties.use_palette = 0;
	    this_plot->lp_properties.l_type = line_num;
	    this_plot->lp_properties.p_type = point_num;

	    /* user may prefer explicit line styles */
	    if (prefer_line_styles)
		lp_use_properties(&this_plot->lp_properties, line_num+1, TRUE);

	    /* pm 25.11.2001 allow any order of options */
	    while (!END_OF_COMMAND || !checked_once) {

		/* deal with title */
		if (almost_equals(c_token, "t$itle")) {
		    if (set_title) {
			duplication=TRUE;
			break;
		    }
		    this_plot->title_no_enhanced = !key->enhanced;
			/* title can be enhanced if not explicitly disabled */
		    if (parametric) {
			if (crnt_param != 0)
			    int_error(c_token, "\"title\" allowed only after parametric function fully specified");
			else {
			    if (xtitle != NULL)
				xtitle[0] = NUL;	/* Remove default title . */
			    if (ytitle != NULL)
				ytitle[0] = NUL;	/* Remove default title . */
			}
		    }
		    c_token++;
		    if (!(this_plot->title = try_to_get_string()))
			int_error(c_token, "expecting \"title\" for plot");
		    set_title = TRUE;
		    continue;
		}

		if (almost_equals(c_token, "not$itle")) {
		    if (set_title) {
			duplication=TRUE;
			break;
		    }
		    c_token++;
		    if (isstringvalue(c_token))
			try_to_get_string(); /* ignore optionally given title string */
		    if (xtitle != NULL)
			xtitle[0] = '\0';
		    if (ytitle != NULL)
			ytitle[0] = '\0';
		    /*   this_plot->title = NULL;   */
		    set_title = TRUE;
		    continue;
		}

		/* deal with style */
		if (almost_equals(c_token, "w$ith")) {
		    if (set_with) {
			duplication=TRUE;
			break;
		    }
		    this_plot->plot_style = get_style();
		    if ((this_plot->plot_type == FUNC3D) &&
			((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
#ifdef EAM_DATASTRINGS
			|| (this_plot->plot_style == LABELPOINTS)
#endif
			)) {
			int_warn(c_token, "This plot style is only for datafiles , reverting to \"points\"");
			this_plot->plot_style = POINTSTYLE;
		    }

		    if ((this_plot->plot_style | data_style) & PM3DSURFACE) {
			if (equals(c_token, "at")) {
			/* option 'with pm3d [at ...]' is explicitly specified */
			c_token++;
			if (get_pm3d_at_option(&this_plot->pm3d_where[0]))
			    return; /* error */
			}
		    }

#ifdef WITH_IMAGE
		    if (this_plot->plot_style == IMAGE || this_plot->plot_style == RGBIMAGE)
			get_image_options(&this_plot->image_properties);
#endif
		    set_with = TRUE;
		    continue;
		}

		/* EAM Dec 2005 - Hidden3D code by default includes points,  */
		/* labels and vectors in the hidden3d processing. Check here */
		/* if this particular plot wants to be excluded.             */
		if (almost_equals(c_token, "nohidden$3d")) {
		    c_token++;
		    this_plot->opt_out_of_hidden3d = TRUE;
		    continue;
		}

#ifdef EAM_DATASTRINGS
		/* Labels can have font and text property info as plot options */
		/* In any case we must allocate one instance of the text style */
		/* that all labels in the plot will share.                     */
		if (this_plot->plot_style == LABELPOINTS)  {
		    int stored_token = c_token;
		    if (this_plot->labels == NULL) {
			this_plot->labels = new_text_label(-1);
			this_plot->labels->pos = JUST_CENTRE;
			this_plot->labels->layer = LAYER_PLOTLABELS;
		    }
		    parse_label_options(this_plot->labels);
		    checked_once = TRUE;
		    if (stored_token != c_token) {
			if (set_labelstyle) {
			    duplication = TRUE;
			    break;
			} else {
			    set_labelstyle = TRUE;
			    continue;
			}
		    }
		}
#endif

		/* pick up line/point specs
		 * - point spec allowed if style uses points, ie style&2 != 0
		 * - keywords are optional
		 */
		if (this_plot->plot_style == VECTOR) {
		    int stored_token = c_token;

		    if (!checked_once) {
			default_arrow_style(&this_plot->arrow_properties);
			this_plot->arrow_properties.lp_properties.l_type = line_num;
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
		} else {
		    int stored_token = c_token;
		    struct lp_style_type lp = DEFAULT_LP_STYLE_TYPE;

		    lp.l_type = line_num;
		    lp.p_type = point_num;

		    /* user may prefer explicit line styles */
		    if (prefer_line_styles)
			lp_use_properties(&lp, line_num+1, TRUE);
		    
 		    lp_parse(&lp, TRUE,
			     this_plot->plot_style & PLOT_STYLE_HAS_POINT);
		    checked_once = TRUE;
		    if (stored_token != c_token) {
			if (set_lpstyle) {
			    duplication=TRUE;
			    break;
			} else {
			    this_plot->lp_properties = lp;
			    set_lpstyle = TRUE;
			    continue;
			}
		    }
		}

		break; /* unknown option */

	    }  /* while (!END_OF_COMMAND)*/

	    if (duplication)
		int_error(c_token, "duplicated or contradicting arguments in plot options");

	    /* set default values for title if this has not been specified */
	    if (!set_title) {
		this_plot->title_no_enhanced = TRUE; /* filename or function cannot be enhanced */
		if (key->auto_titles == FILENAME_KEYTITLES) {
#ifdef BINARY_DATA_FILE
		    /* DJS (20 Aug 2004) I'd prefer that the df_binary flag be discarded.  There
		     * is nothing special about the file being binary that its title should be
		     * different.  Can't the decision to do this be based on some other criteria,
		     * like the presence of a nonconventional `using`?
		     */
#endif
		    if (this_plot->plot_type == DATA3D && df_binary==TRUE && end_token==start_token+1)
			/* let default title for  splot 'a.dat' binary  is 'a.dat'
			 * while for  'a.dat' binary using 2:1:3  will be all 4 words */
			m_capture(&(this_plot->title), start_token, start_token);
		    else
			m_capture(&(this_plot->title), start_token, end_token);
		    if (crnt_param == 2)
			xtitle = this_plot->title;
		    else if (crnt_param == 1)
			ytitle = this_plot->title;
		} else {
		    if (xtitle != NULL)
			xtitle[0] = '\0';
		    if (ytitle != NULL)
			ytitle[0] = '\0';
		    /*   this_plot->title = NULL;   */
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
		} else {
		    this_plot->lp_properties.l_type = line_num;
		    this_plot->lp_properties.l_width = 1.0;
		    this_plot->lp_properties.p_type = point_num;
		    this_plot->lp_properties.p_size = pointsize;
		    this_plot->lp_properties.use_palette = 0;

		    /* user may prefer explicit line styles */
		    if (prefer_line_styles)
			lp_use_properties(&this_plot->lp_properties, line_num+1, TRUE);

		    lp_parse(&this_plot->lp_properties, TRUE,
			 this_plot->plot_style & PLOT_STYLE_HAS_POINT);
		}

#ifdef BACKWARDS_COMPATIBLE
		/* allow old-style syntax - ignore case lt 3 4 for example */
		if (!END_OF_COMMAND && isanumber(c_token)) {
		    this_plot->lp_properties.l_type =
			this_plot->lp_properties.p_type = int_expression() - 1;

		    if (isanumber(c_token))
			this_plot->lp_properties.p_type = int_expression() - 1;
		}
#endif

	    }

	    /* Rule out incompatible line/point/style options */
	    if (this_plot->plot_type == FUNC3D) {
		if ((this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		&&  (this_plot->lp_properties.p_size == PTSZ_VARIABLE))
		    this_plot->lp_properties.p_size = 1;
	    }
	    if (this_plot->plot_style == LINES) {
		this_plot->opt_out_of_hidden3d = FALSE;
	    }

	    if (crnt_param == 0
		&& this_plot->plot_style != PM3DSURFACE
		/* don't increment the default line/point properties if
		 * this_plot is an EXPLICIT pm3d surface plot */
#ifdef WITH_IMAGE
		&& this_plot->plot_style != IMAGE
		&& this_plot->plot_style != RGBIMAGE
		/* same as above, for an (rgb)image plot */
#endif
		) {
		if (this_plot->plot_style & PLOT_STYLE_HAS_POINT)
		    point_num += 1 + (draw_contour != 0) + (hidden3d != 0);
		line_num += 1 + (draw_contour != 0) + (hidden3d != 0);
	    }

#ifdef WITH_IMAGE
	    /* Styles that utilize palettes. */
	    if (this_plot->plot_style == IMAGE)
		this_plot->lp_properties.use_palette = 1;
#endif

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
		/* remember settings for second surface in file */
		struct lp_style_type *these_props = &(this_plot->lp_properties);
		enum PLOT_STYLE this_style = this_plot->plot_style;
		struct surface_points *first_dataset = this_plot;
		    /* pointer to the plot of the first dataset (surface) in the file */
		int this_token = this_plot->token;

		do {
		    this_plot = *tp_3d_ptr;
		    assert(this_plot != NULL);

		    /* dont move tp_3d_ptr until we are sure we
		     * have read a surface
		     */

		    /* used by get_3ddata() */
		    this_plot->token = this_token;

		    get_3ddata(this_plot);
		    /* for second pass */
		    this_plot->token = c_token;
		    this_plot->plot_num = plot_num;

		    if (this_plot->num_iso_read == 0)
			this_plot->plot_type = NODATA;

		    if (this_plot != first_dataset)
			/* copy (explicit) "with pm3d at ..." option from the first dataset in the file */
			strcpy(this_plot->pm3d_where, first_dataset->pm3d_where);

		    /* okay, we have read a surface */
		    ++plot_num;
		    tp_3d_ptr = &(this_plot->next_sp);
		    if (df_eof)
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
		    this_plot->plot_style = this_style;
		    /* Struct copy */
		    this_plot->lp_properties = *these_props;
		} while (!df_eof);

		df_close();
		/*}}} */

	    } else {		/* not a data file */
		tp_3d_ptr = &(this_plot->next_sp);
		this_plot->token = c_token;	/* store for second pass */
		this_plot->plot_num = plot_num;
	    }

	    if (empty_iteration())
		this_plot->plot_type = NODATA;

	}			/* !is_definition() : end of scope of this_plot */

	if (crnt_param != 0) {
	    if (equals(c_token, ",")) {
		c_token++;
		continue;
	    } else
		break;
	}

	/* Iterate-over-plot mechanisms */
	if (next_iteration()) {
	    c_token = start_token;
	    continue;
	}

	if (equals(c_token, ",")) {
	    c_token++;
	    check_for_iteration();
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
	    /*{{{  check ranges */
	    /* give error if xrange badly set from missing datafile error
	     * parametric fn can still set ranges
	     * if there are no fns, we'll report it later as 'nothing to plot'
	     */

	    /* check that xmin -> xmax is not too small */
	    axis_checked_extend_empty_range(FIRST_X_AXIS, "x range is invalid");
	    axis_checked_extend_empty_range(FIRST_Y_AXIS, "y range is invalid");
	    /*}}} */
	}
	if (parametric && !some_data_files) {
	    /*{{{  set ranges */
	    /* parametric fn can still change x/y range */
	    if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN)
		axis_array[FIRST_X_AXIS].min = VERYLARGE;
	    if (axis_array[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX)
		axis_array[FIRST_X_AXIS].max = -VERYLARGE;
	    if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MIN)
		axis_array[FIRST_Y_AXIS].min = VERYLARGE;
	    if (axis_array[FIRST_Y_AXIS].autoscale & AUTOSCALE_MAX)
		axis_array[FIRST_Y_AXIS].max = -VERYLARGE;
	    /*}}} */
	}

	/*{{{  figure ranges, taking logs etc into account */
	u_min = axis_log_value_checked(u_axis, axis_array[u_axis].min, "x range");
	u_max = axis_log_value_checked(u_axis, axis_array[u_axis].max, "x range");
	v_min = axis_log_value_checked(v_axis, axis_array[v_axis].min, "y range");
	v_max = axis_log_value_checked(v_axis, axis_array[v_axis].max, "y range");
	/*}}} */


	if (samples_1 < 2 || samples_2 < 2 || iso_samples_1 < 2 ||
	    iso_samples_2 < 2) {
	    int_error(NO_CARET, "samples or iso_samples < 2. Must be at least 2.");
	}

	/* start over */
	this_plot = first_3dplot;
	c_token = begin_token;
	check_for_iteration();

	/* why do attributes of this_plot matter ? */
	/* FIXME HBB 20000501: I think they don't, actually. I'm
	 * taking out references to has_grid_topology in this part of
	 * the code.  It only deals with function, which always is
	 * gridded */

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
	    if (is_definition(c_token)) {
		define();
	    } else {
		struct at_type *at_ptr;
		char *name_str;

		if (crnt_param == 0)
		    start_token = c_token;

		dummy_func = &plot_func;
		name_str = string_or_express(&at_ptr);

		if (!name_str) {                /* func to plot */
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
					      num_sam_to_use,
					      NEED_PALETTE(this_plot));

		    if (!hidden3d) {
			num_iso_to_use = iso_samples_1;
			num_sam_to_use = samples_2;

			calculate_set_of_isolines(crnt_param, TRUE, &this_iso,
						  u_axis, u_min, u_isostep,
						  num_iso_to_use,
						  v_axis, v_min, v_step,
						  num_sam_to_use,
						  NEED_PALETTE(this_plot));
		    }
		    /*}}} */
		}		/* end of ITS A FUNCTION TO PLOT */
		/* we saved it from first pass */
		c_token = this_plot->token;

		/* one data file can make several plots */
		i = this_plot->plot_num;
		do
		    this_plot = this_plot->next_sp;
		while (this_plot 
			&& this_plot->token == c_token
			&& this_plot->plot_num == i);

	    }			/* !is_definition */

	    /* Iterate-over-plot mechanism */
	    if (crnt_param == 0 && next_iteration()) {
		c_token = start_token;
		continue;
	    }

	    if (equals(c_token, ",")) {
		c_token++;
		if (crnt_param == 0)
		    check_for_iteration();
	    } else
		break;

	}			/* while(TRUE) */


	if (parametric) {
	    /* Now actually fix the plot triplets to be single plots. */
	    parametric_3dfixup(first_3dplot, &plot_num);
	}
    }				/* some functions */

    /* if first_3dplot is NULL, we have no functions or data at all.
       * This can happen, if you type "splot x=5", since x=5 is a
       * variable assignment
     */
    if (plot_num == 0 || first_3dplot == NULL) {
	int_error(c_token, "no functions or data to plot");
    }

    axis_checked_extend_empty_range(FIRST_X_AXIS, "All points x value undefined");
    axis_checked_extend_empty_range(FIRST_Y_AXIS, "All points y value undefined");
    if (splot_map)
	axis_checked_extend_empty_range(FIRST_Z_AXIS, NULL); /* Suppress warning message */
    else
	axis_checked_extend_empty_range(FIRST_Z_AXIS, "All points z value undefined");

    axis_revert_and_unlog_range(FIRST_X_AXIS);
    axis_revert_and_unlog_range(FIRST_Y_AXIS);
    axis_revert_and_unlog_range(FIRST_Z_AXIS);

    setup_tics(FIRST_X_AXIS, 20);
    setup_tics(FIRST_Y_AXIS, 20);
    setup_tics(FIRST_Z_AXIS, 20);

    set_plot_with_palette(plot_num, MODE_SPLOT);
    if (is_plot_with_palette()) {
	set_cbminmax();
	axis_checked_extend_empty_range(COLOR_AXIS, "All points of colorbox value undefined");
	setup_tics(COLOR_AXIS, 20);
	/* axis_revert_and_unlog_range(COLOR_AXIS); */
	/* fprintf(stderr,"plot3d.c: CB_AXIS.min=%g\tCB_AXIS.max=%g\n",CB_AXIS.min,CB_AXIS.max); */
    }

    AXIS_WRITEBACK(FIRST_X_AXIS);

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
	    }
	    /* Make sure this one can be contoured. */
	    if (!this_plot->has_grid_topology) {
		this_plot->contours = NULL;
		fputs("Notice: Cannot contour non grid data. Please use \"set dgrid3d\".\n", stderr);
		/* changed from int_error by recommendation of
		 * rkc@xn.ll.mit.edu
		 */
	    } else if (this_plot->plot_type == DATA3D) {
		this_plot->contours = contour(this_plot->num_iso_read,
					      this_plot->iso_crvs);
	    } else {
		this_plot->contours = contour(iso_samples_2,
					      this_plot->iso_crvs);
	    }
	}
    }				/* draw_contour */

    /* the following ~9 lines were moved from the end of the
     * function to here, as do_3dplot calles term->text, which
     * itself might process input events in mouse enhanced
     * terminals. For redrawing to work, line capturing and
     * setting the plot3d_num must already be done before
     * entering do_3dplot(). Thu Jan 27 23:54:49 2000 (joze) */

    /* if we get here, all went well, so record the line for replot */
    if (plot_token != -1) {
	/* note that m_capture also frees the old replot_line */
	m_capture(&replot_line, plot_token, c_token - 1);
	plot_token = -1;
    }

    /* record that all went well */
    plot3d_num=plot_num;

    /* perform the plot */
    if (table_mode)
	print_3dtable(plot_num);
    else {
	START_LEAK_CHECK();	/* assert no memory leaks here ! */
	do_3dplot(first_3dplot, plot_num, 0);
	END_LEAK_CHECK();

        /* after do_3dplot(), axis_array[] and max_array[].min
         * contain the plotting range actually used (rounded
         * to tic marks, not only the min/max data values)
         * --> save them now for writeback if requested
	 */
	SAVE_WRITEBACK_ALL_AXES;
	/* update GPVAL_ variables available to user */
	update_gpval_variables(1);
    }
}



/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are gauranteed to occur in (x,y,z) triplets while preserving
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
    size_t tlen;
    int i, surface;
    char *new_title;

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
		struct coordinate GPHUGE *xpoints = xicrvs->points;
		struct coordinate GPHUGE *ypoints = yicrvs->points;
		struct coordinate GPHUGE *zpoints = zicrvs->points;

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

#if 0 /* FIXME HBB 20001101: seems to cause a crash */
	    if (first_3dplot)
		sp_free(first_3dplot);
	    first_3dplot = NULL;
#endif

	    /* Ok, fix up the title to include xp and yp plots. */
	    if (((xp->title && xp->title[0] != '\0') ||
		 (yp->title && yp->title[0] != '\0')) && zp->title) {
		tlen = (xp->title ? strlen(xp->title) : 0) +
		    (yp->title ? strlen(yp->title) : 0) +
		    (zp->title ? strlen(zp->title) : 0) + 5;
		new_title = gp_alloc(tlen, "string");
		new_title[0] = 0;
		if (xp->title && xp->title[0] != '\0') {
		    strcat(new_title, xp->title);
		    strcat(new_title, ", ");	/* + 2 */
		}
		if (yp->title && yp->title[0] != '\0') {
		    strcat(new_title, yp->title);
		    strcat(new_title, ", ");	/* + 2 */
		}
		strcat(new_title, zp->title);
		free(zp->title);
		zp->title = new_title;
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

