#ifndef lint
static char *RCSid() { return RCSid("$Id: plot3d.c,v 1.17.2.4 2000/10/23 04:35:28 joze Exp $"); }
#endif

/* GNUPLOT - plot3d.c */

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

#include "plot3d.h"

#include "alloc.h"
#include "binary.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "graphics.h"
#include "graph3d.h"
#include "misc.h"
#include "parse.h"
#include "plot2d.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"

#ifdef THIN_PLATE_SPLINES_GRID
#include "matrix.h"
#endif

#ifndef _Windows
# include "help.h"
#endif

/* static prototypes */

static void get_3ddata __PROTO((struct surface_points * this_plot));
static void print_3dtable __PROTO((int pcount));
static void eval_3dplots __PROTO((void));
static void grid_nongrid_data __PROTO((struct surface_points * this_plot));
static void parametric_3dfixup __PROTO((struct surface_points * start_plot, int *plot_num));

/* the curves/surfaces of the plot */
struct surface_points *first_3dplot = NULL;
static struct udft_entry plot_func;

/* in order to support multiple axes, and to
 * simplify ranging in parametric plots, we use
 * arrays to store some things. For 2d plots,
 * elements are  y1 = 0 x1 = 1 y2 = 2 x2 = 3
 * for 3d,  z = 0, x = 1, y = 2
 * these are given symbolic names in plot.h
 */

/* some file-wide variables to store which axis we are using */
static int x_axis, y_axis, z_axis;

/*
 * IMHO, code is getting too cluttered with repeated chunks of
 * code. Some macros to simplify, I hope.
 *
 * do { } while(0) is comp.lang.c recommendation for complex macros
 * also means that break can be specified as an action, and it will
 * 
 */

/*  copy scalar data to arrays
 * optimiser should optimise infinite away
 * dont know we have to support ranges [10:-10] - lets reverse
 * it for now, then fix it at the end.
 */
#define INIT_ARRAYS(axis, min, max, auto, is_log, base, log_base, infinite) \
do{if ((auto_array[axis] = auto) == 0 && max<min) {\
	min_array[axis] = max;\
   max_array[axis] = min; /* we will fix later */ \
 } else { \
	min_array[axis] = (infinite && (auto&1)) ? VERYLARGE : min; \
	max_array[axis] = (infinite && (auto&2)) ? -VERYLARGE : max; \
 } \
 log_array[axis] = is_log; base_array[axis] = base; log_base_array[axis] = log_base;\
}while(0)

/* handle reversed ranges */
#define CHECK_REVERSE(axis) \
do{\
 if (auto_array[axis] == 0 && max_array[axis] < min_array[axis]) {\
  double temp = min_array[axis]; min_array[axis] = max_array[axis]; max_array[axis] = temp;\
  reverse_range[axis] = 1; \
 } else reverse_range[axis] = (range_flags[axis]&RANGE_REVERSE); \
}while(0)

int plot3d_num = 0;


/* get optional [min:max] */
#define LOAD_RANGE(axis) \
do {\
 if (equals(c_token, "[")) { \
  c_token++; \
  auto_array[axis] = load_range(axis,&min_array[axis], &max_array[axis], auto_array[axis]);\
  if (!equals(c_token, "]"))\
   int_error(c_token, "']' expected");\
  c_token++;\
 }\
} while (0)


/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate
 * Do OUT_ACTION or UNDEF_ACTION as appropriate
 * adjust range provided type is INRANGE (ie dont adjust y if x is outrange
 * VALUE must not be same as STORE
 */

#define STORE_WITH_LOG_AND_FIXUP_RANGE(STORE, VALUE, TYPE, AXIS, OUT_ACTION, UNDEF_ACTION)\
do { if (log_array[AXIS]) { if (VALUE<0.0) {TYPE = UNDEFINED; UNDEF_ACTION; break;} \
              else if (VALUE == 0.0){STORE = -VERYLARGE; TYPE = OUTRANGE; OUT_ACTION; break;} \
              else { STORE = log(VALUE)/log_base_array[AXIS]; } \
     } else STORE = VALUE; \
     if (TYPE != INRANGE) break;  /* dont set y range if x is outrange, for example */ \
     if ( VALUE<min_array[AXIS] ) { \
      if (auto_array[AXIS] & 1) min_array[AXIS] = VALUE; else { TYPE = OUTRANGE; OUT_ACTION; break; }  \
     } \
     if ( VALUE>max_array[AXIS] ) { \
      if (auto_array[AXIS] & 2) max_array[AXIS] = VALUE; else { TYPE = OUTRANGE; OUT_ACTION; }   \
     } \
} while(0)

/* use this instead empty macro arguments to work around NeXT cpp bug */
/* if this fails on any system, we might use ((void)0) */
#define NOOP			/* */

/* check range and take logs of min and max if logscale
 * this also restores min and max for ranges like [10:-10]
 */

#ifdef HAVE_STRINGIZE
# define RANGE_MSG(x) #x " range is less than threshold : see `set zero`"
#else
# define RANGE_MSG(x) "x range is less than threshold : see `set zero`"
#endif

#define FIXUP_RANGE_FOR_LOG(AXIS, WHICH) \
do { if (reverse_range[AXIS]) { \
      double temp = min_array[AXIS]; \
      min_array[AXIS] = max_array[AXIS]; \
      max_array[AXIS] = temp; \
     }\
     if (log_array[AXIS]) { \
      if (min_array[AXIS] <= 0.0 || max_array[AXIS] <= 0.0) \
       int_error(NO_CARET, RANGE_MSG(WHICH)); \
      min_array[AXIS] = log(min_array[AXIS])/log_base_array[AXIS]; \
      max_array[AXIS] = log(max_array[AXIS])/log_base_array[AXIS];  \
    } \
} while(0)



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
    TBOOLEAN changed;
    int dummy_token0 = -1, dummy_token1 = -1;

    is_3d_plot = TRUE;

    if (parametric && strcmp(dummy_var[0], "t") == 0) {
	strcpy(dummy_var[0], "u");
	strcpy(dummy_var[1], "v");
    }
    autoscale_lx = autoscale_x;
    autoscale_ly = autoscale_y;
    autoscale_lz = autoscale_z;

    if (!term)			/* unknown */
	int_error(c_token, "use 'set term' to set terminal type first");

    if (equals(c_token, "[")) {
	c_token++;
	if (isletter(c_token)) {
	    if (equals(c_token + 1, "=")) {
		dummy_token0 = c_token;
		c_token += 2;
	    } else {
		/* oops; probably an expression with a variable. */
		/* Parse it as an xmin expression. */
		/* used to be: int_error("'=' expected",c_token); */
	    }
	}
	changed = parametric ? load_range(U_AXIS, &umin, &umax, autoscale_lu) :
	    load_range(FIRST_X_AXIS, &xmin, &xmax, autoscale_lx);
	if (!equals(c_token, "]"))
	    int_error(c_token, "']' expected");
	c_token++;
	/* if (changed) */
	if (parametric)
	    /* autoscale_lu = FALSE; */
	    autoscale_lu = changed;
	else
	    /* autoscale_lx = FALSE; */
	    autoscale_lx = changed;
    }
    if (equals(c_token, "[")) {
	c_token++;
	if (isletter(c_token)) {
	    if (equals(c_token + 1, "=")) {
		dummy_token1 = c_token;
		c_token += 2;
	    } else {
		/* oops; probably an expression with a variable. */
		/* Parse it as an xmin expression. */
		/* used to be: int_error("'=' expected",c_token); */
	    }
	}
	changed = parametric ? load_range(V_AXIS, &vmin, &vmax, autoscale_lv) :
	    load_range(FIRST_Y_AXIS, &ymin, &ymax, autoscale_ly);
	if (!equals(c_token, "]"))
	    int_error(c_token, "']' expected");
	c_token++;
	/* if (changed) */
	if (parametric)
	    /* autoscale_lv = FALSE; */
	    autoscale_lv = changed;
	else
	    /* autoscale_ly = FALSE; */
	    autoscale_ly = changed;
    }
    if (parametric) {
	/* set optional x (parametric) or z ranges */
	if (equals(c_token, "[")) {
	    c_token++;
	    autoscale_lx = load_range(FIRST_X_AXIS, &xmin, &xmax, autoscale_lx);
	    if (!equals(c_token, "]"))
		int_error(c_token, "']' expected");
	    c_token++;
	}
	/* set optional y ranges */
	if (equals(c_token, "[")) {
	    c_token++;
	    autoscale_ly = load_range(FIRST_Y_AXIS, &ymin, &ymax, autoscale_ly);
	    if (!equals(c_token, "]"))
		int_error(c_token, "']' expected");
	    c_token++;
	}
    }				/* parametric */
    if (equals(c_token, "[")) {	/* set optional z ranges */
	c_token++;
	autoscale_lz = load_range(FIRST_Z_AXIS, &zmin, &zmax, autoscale_lz);
	if (!equals(c_token, "]"))
	    int_error(c_token, "']' expected");
	c_token++;
    }
    CHECK_REVERSE(FIRST_X_AXIS);
    CHECK_REVERSE(FIRST_Y_AXIS);
    CHECK_REVERSE(FIRST_Z_AXIS);

    /* use the default dummy variable unless changed */
    if (dummy_token0 >= 0)
	copy_str(c_dummy_var[0], dummy_token0, MAX_ID_LEN);
    else
	(void) strcpy(c_dummy_var[0], dummy_var[0]);

    if (dummy_token1 >= 0)
	copy_str(c_dummy_var[1], dummy_token1, MAX_ID_LEN);
    else
	(void) strcpy(c_dummy_var[1], dummy_var[1]);

    eval_3dplots();
}

#ifdef THIN_PLATE_SPLINES_GRID

static double splines_kernel __PROTO((double h));
/* HBB 991025 FIXME: these don't belong in here --> move to 'matrix' */
static void lu_decomp __PROTO((double **, int, int *, double *));
static void lu_backsubst __PROTO((double **, int n, int *, double *));

static double
splines_kernel(h)
double h;
{
    /* this is normaly not usefull ... */
    h = fabs(h);

    if (h != 0.0) {
	return h * h * log(h);
    } else {
	return 0;
    }
}

#define Swap(a,b) {double tmp; tmp=a; a=b; b=tmp;}

static void
lu_decomp(a, n, indx, d)
double **a;
int n;
int *indx;
double *d;
{
    int i, imax = -1, j, k;	/* HBB: added initial value, to shut up gcc -Wall */
    double large, dummy, temp, **ar, **lim, *limc, *ac, *dp, *vscal;

    dp = vscal = vec(n);
    *d = 1.0;
    for (ar = a, lim = &(a[n]); ar < lim; ar++) {
	large = 0.0;
	for (ac = *ar, limc = &(ac[n]); ac < limc;)
	    if ((temp = fabs(*ac++)) > large)
		large = temp;
	if (large == 0.0)
	    int_error(NO_CARET, "Singular matrix in LU-DECOMP");
	*dp++ = 1 / large;
    }
    ar = a;
    for (j = 0; j < n; j++, ar++) {
	for (i = 0; i < j; i++) {
	    ac = &(a[i][j]);
	    for (k = 0; k < i; k++)
		*ac -= a[i][k] * a[k][j];
	}
	large = 0.0;
	dp = &(vscal[j]);
	for (i = j; i < n; i++) {
	    ac = &(a[i][j]);
	    for (k = 0; k < j; k++)
		*ac -= a[i][k] * a[k][j];
	    if ((dummy = *dp++ * fabs(*ac)) >= large) {
		large = dummy;
		imax = i;
	    }
	}
	if (j != imax) {
	    ac = a[imax];
	    dp = *ar;
	    for (k = 0; k < n; k++, ac++, dp++)
		Swap(*ac, *dp);
	    *d = -(*d);
	    vscal[imax] = vscal[j];
	}
	indx[j] = imax;
	if (*(dp = &(*ar)[j]) == 0)
	    *dp = 1e-30;

	if (j != n - 1) {
	    dummy = 1 / (*ar)[j];
	    for (i = j + 1; i < n; i++)
		a[i][j] *= dummy;
	}
    }
    free(vscal);
}

static void
lu_backsubst(a, n, indx, b)
double **a;
int n;
int *indx;
double *b;
{
    int i, memi = -1, ip, j;

    double sum, *bp, *bip, **ar, *ac;

    ar = a;

    for (i = 0; i < n; i++, ar++) {
	ip = indx[i];
	sum = b[ip];
	b[ip] = b[i];
	if (memi >= 0) {
	    ac = &((*ar)[memi]);
	    bp = &(b[memi]);
	    for (j = memi; j <= i - 1; j++)
		sum -= *ac++ * *bp++;
	} else if (sum)
	    memi = i;
	b[i] = sum;
    }
    ar--;
    for (i = n - 1; i >= 0; i--) {
	ac = &(*ar)[i + 1];
	bp = &(b[i + 1]);
	bip = &(b[i]);
	for (j = i + 1; j < n; j++)
	    *bip -= *ac++ * *bp++;
	*bip /= (*ar--)[i];
    }
}
#endif

static void
grid_nongrid_data(this_plot)
struct surface_points *this_plot;
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
    xmin = xmax = old_iso_crvs->points[0].x;
    ymin = ymax = old_iso_crvs->points[0].y;
    for (icrv = old_iso_crvs; icrv != NULL; icrv = icrv->next) {
	struct coordinate GPHUGE *points = icrv->points;

	for (i = 0; i < icrv->p_count; i++, points++) {
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
    xx = (double *) gp_alloc(sizeof(double) * (numpoints + 3) * (numpoints + 8),
			     "thin plate splines in dgrid3d");
    /* the memory needed is not really (n+3)*(n+8) for now,
       but might be if I take into account errors ... */
    K = (double **) gp_alloc(sizeof(double *) * (numpoints + 3),
			     "matrix : thin plate splines 2d");
    yy = xx + numpoints;
    zz = yy + numpoints;
    b = zz + numpoints;
    i = 0;
    for (oicrv = old_iso_crvs; oicrv != NULL; oicrv = oicrv->next) {
	struct coordinate GPHUGE *opoints = oicrv->points;
	for (k = 0; k < oicrv->p_count; k++, opoints++) {
	    xx[i] = opoints->x;
	    yy[i] = opoints->y;
	    zz[i] = opoints->z;
	    i++;
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
    indx = (int *) gp_alloc(sizeof(int) * (numpoints + 3), "indexes lu");
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

#ifndef BUGGY_DGRID_RANGING	/* HBB 981209 */
	    /* as soon as ->type is changed to UNDEFINED, break out of
	     * two inner loops! */
	    points->type = INRANGE;
#endif
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
#ifndef BUGGY_DGRID_RANGING
			/* HBB 981209: revised flagging as undefined */
			/* Supporting all those infinities on various
			 * platforms becomes tiresome, to say the least :-(
			 * Let's just return the first z where this happens,
			 * unchanged, and be done with this, period. */
			points->type = UNDEFINED;
			z = opoints->z;
			w = 1.0;
			break;	/* out of for (k...) loop */
#else
#if !defined(AMIGA_SC_6_1) && !defined(__PUREC__)
			dist = VERYLARGE;
#else /* !AMIGA_SC_6_1 && !__PUREC__ */
			/* Multiplying VERYLARGE by opoints->z below
			 * might yield Inf (i.e. a number that can't
			 * be represented on the machine). This will
			 * result in points->z being set to NaN. It's
			 * better to have a pretty large number that is
			 * also on the safe side... The numbers that are
			 * read by gnuplot are float values anyway, so
			 * they can't be bigger than FLT_MAX. So setting
			 * dist to FLT_MAX^2 will make dist pretty large
			 * with respect to any value that has been read. */
			dist = ((double) FLT_MAX) * ((double) FLT_MAX);
#endif /* !AMIGA_SC_6_1 && !__PUREC__ */
#endif /* BUGGY_DGRID_RANGING */
		    } else
			dist = 1.0 / dist;

		    z += opoints->z * dist;
		    w += dist;
		}
#ifndef BUGGY_DGRID_RANGING
		if (points->type != INRANGE)
		    break;	/* out of the second-inner loop as well ... */
#endif
	    }
#endif /* THIN_PLATE_SPLINES_GRID */

#ifndef BUGGY_DGRID_RANGING
	    /* Now that we've escaped the loops safely, we know that we
	     * do have a good value in z and w, so we can proceed just as
	     * if nothing had happened at all. Nice, isn't it? */
	    points->type = INRANGE;

	    STORE_WITH_LOG_AND_FIXUP_RANGE(points->x, x, points->type, x_axis, NOOP, continue);
	    STORE_WITH_LOG_AND_FIXUP_RANGE(points->y, y, points->type, y_axis, NOOP, continue);
#ifndef THIN_PLATE_SPLINES_GRID
	    STORE_WITH_LOG_AND_FIXUP_RANGE(points->z, z / w, points->type, z_axis, NOOP, continue);
#else
	    STORE_WITH_LOG_AND_FIXUP_RANGE(points->z, z, points->type, z_axis, NOOP, continue);
#endif
#else
	    /* HBB 981026: original, short version of this code */
	    points->x = x;
	    points->y = y;
	    points->z = z / w;
	    points->type = INRANGE;
#endif
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

static void
get_3ddata(this_plot)
struct surface_points *this_plot;
/* this_plot->token is end of datafile spec, before title etc
 * will be moved passed title etc after we return
 */
{
    int xdatum = 0;
    int ydatum = 0;
    int i, j;
    double v[3];
    int pt_in_iso_crv = 0;
    struct iso_curve *this_iso;

    if (mapping3d == MAP3D_CARTESIAN) {
	if (df_no_use_specs == 2)
	    int_error(this_plot->token, "Need 1 or 3 columns for cartesian data");
    } else {
	if (df_no_use_specs == 1)
	    int_error(this_plot->token, "Need 2 or 3 columns for polar data");
    }

    this_plot->num_iso_read = 0;
    this_plot->has_grid_topology = TRUE;

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
	xdatum = df_3dmatrix(this_plot);
    else {
	/*{{{  read surface from text file */
	struct iso_curve *local_this_iso = iso_alloc(samples);
	struct coordinate GPHUGE *cp;
	double x, y, z;

	while ((j = df_readline(v, 3)) != DF_EOF) {
	    if (j == DF_SECOND_BLANK)
		break;		/* two blank lines */
	    if (j == DF_FIRST_BLANK) {
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
		/*
		 * overflow about to occur. Extend size of points[] array. We
		 * either double the size, or add 1000 points, whichever is a
		 * smaller increment. Note i = p_max.
		 */
		iso_extend(local_this_iso,
			   xdatum + (xdatum < 1000 ? xdatum : 1000));
	    }
	    cp = local_this_iso->points + xdatum;

	    if (j == DF_UNDEFINED) {
		cp->type = UNDEFINED;
		continue;
	    }
	    cp->type = INRANGE;	/* unless we find out different */

	    switch (mapping3d) {
	    case MAP3D_CARTESIAN:
		switch (j) {
		case 1:
		    x = xdatum;
		    y = ydatum;
		    z = v[0];
		    break;
		case 3:
		    x = v[0];
		    y = v[1];
		    z = v[2];
		    break;
		default:
		    {
			int_error(this_plot->token,
				  "Need 1 or 3 columns - line %d",
				  df_line_number);
			return;	/* avoid gcc -Wuninitialised for x,y,z */
		    }
		}
		break;
	    case MAP3D_SPHERICAL:
		if (j < 2)
		    int_error(this_plot->token, "Need 2 or 3 columns");
		if (j < 3)
		    v[2] = 1;	/* default radius */
		if (angles_format == ANGLES_DEGREES) {
		    v[0] *= DEG2RAD;	/* Convert to radians. */
		    v[1] *= DEG2RAD;
		}
		x = v[2] * cos(v[0]) * cos(v[1]);
		y = v[2] * sin(v[0]) * cos(v[1]);
		z = v[2] * sin(v[1]);
		break;
	    case MAP3D_CYLINDRICAL:
		if (j < 2)
		    int_error(this_plot->token, "Need 2 or 3 columns");
		if (j < 3)
		    v[2] = 1;	/* default radius */
		if (angles_format == ANGLES_DEGREES) {
		    v[0] *= DEG2RAD;	/* Convert to radians. */
		}
		x = v[2] * cos(v[0]);
		y = v[2] * sin(v[0]);
		z = v[1];
		break;
	    default:
		int_error(NO_CARET, "Internal error : Unknown mapping type");
		return;
	    }

	    /* adjust for logscales. Set min/max and point types.
	     * store in cp
	     */
	    cp->type = INRANGE;
	    /* cannot use continue, as macro is wrapped in a loop.
	     * I regard this as correct goto use
	     */
	    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->x, x, cp->type, x_axis, NOOP, goto come_here_if_undefined);
	    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->y, y, cp->type, y_axis, NOOP, goto come_here_if_undefined);
	    STORE_WITH_LOG_AND_FIXUP_RANGE(cp->z, z, cp->type, z_axis, NOOP, goto come_here_if_undefined);

	    /* some may complain, but I regard this as the correct use
	     * of goto
	     */
	  come_here_if_undefined:
	    ++xdatum;
	}			/* end of whileloop - end of surface */

	if (xdatum > 0) {
	    this_plot->num_iso_read++;	/* Update last iso. */
	    local_this_iso->p_count = xdatum;

	    local_this_iso->next = this_plot->iso_crvs;
	    this_plot->iso_crvs = local_this_iso;

	    if (xdatum != pt_in_iso_crv)
		this_plot->has_grid_topology = FALSE;

	} else {
	    iso_free(local_this_iso);	/* Free last allocation. */
	}

	if (dgrid3d && this_plot->num_iso_read > 0)
	    grid_nongrid_data(this_plot);
	/*}}} */
    }

    if (this_plot->num_iso_read <= 1)
	this_plot->has_grid_topology = FALSE;
    if (this_plot->has_grid_topology && !hidden3d) {
	struct iso_curve *new_icrvs = NULL;
	int num_new_iso = this_plot->iso_crvs->p_count, len_new_iso = this_plot->num_iso_read;

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



static void
print_3dtable(pcount)
int pcount;
{
    register struct surface_points *this_plot;
    int i, curve, surface;
    struct iso_curve *icrvs;
    struct coordinate GPHUGE *points;
    char *table_format = NULL;
    char *pcat;

    table_format = gp_alloc(strlen(xformat)+strlen(yformat)+strlen(zformat)+6,
			    "table format");
    strcpy(table_format, xformat);
    strcat(table_format, " ");
    strcat(table_format, yformat);
    strcat(table_format, " ");
    strcat(table_format, zformat);
    pcat = &table_format[strlen(table_format)];

    for (surface = 0, this_plot = first_3dplot; surface < pcount;
	 this_plot = this_plot->next_sp, surface++) {
	fprintf(gpoutfile, "\n#Surface %d of %d surfaces\n", surface, pcount);
	icrvs = this_plot->iso_crvs;
	curve = 0;

	if (draw_surface) {
	    strcpy(pcat," %c\n");
	    /* only the curves in one direction */
	    while (icrvs && curve < this_plot->num_iso_read) {
		fprintf(gpoutfile, "\n#IsoCurve %d, %d points\n#x y z type\n",
			curve, icrvs->p_count);
		for (i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
		    fprintf(gpoutfile, table_format,
			    points[i].x,
			    points[i].y,
			    points[i].z,
			    points[i].type == INRANGE ? 'i'
			    : points[i].type == OUTRANGE ? 'o'
			    : 'u');
		}
		icrvs = icrvs->next;
		curve++;
	    }
	    putc('\n', gpoutfile);
	}
	if (draw_contour) {
	    int number = 0;
	    struct gnuplot_contours *c = this_plot->contours;
	    strcpy(pcat,"\n");
	    while (c) {
		int count = c->num_pts;
		struct coordinate GPHUGE *p = c->coords;
		if (c->isNewLevel)
		    /* dont display count - contour split across chunks */
		    /* put # in case user wants to use it for a plot */
		    /* double blank line to allow plot ... index ... */
		    fprintf(gpoutfile, "\n# Contour %d, label: %s\n", number++, c->label);
		for (; --count >= 0; ++p)
		    fprintf(gpoutfile, table_format, p->x, p->y, p->z);

		/* blank line between segments of same contour */
		putc('\n', gpoutfile);
		c = c->next;
	    }
	}
    }
    fflush(gpoutfile);

    free(table_format);
}



#define SET_DUMMY_RANGE(AXIS) \
do{\
 if (parametric || polar) { \
  t_min = tmin; t_max = tmax;\
 } else if (log_array[AXIS]) {\
  if (min_array[AXIS] <= 0.0 || max_array[AXIS] <= 0.0)\
   int_error(NO_CARET, "x/x2 range must be greater than 0 for log scale!");\
  t_min = log(min_array[AXIS])/log_base_array[AXIS]; t_max = log(max_array[AXIS])/log_base_array[AXIS];\
 } else {\
  t_min = min_array[AXIS]; t_max = max_array[AXIS];\
 }\
 t_step = (t_max - t_min) / (samples - 1); \
}while(0)


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
    int i, j;
    struct surface_points **tp_3d_ptr;
    int start_token, end_token;
    int begin_token;
    TBOOLEAN some_data_files = FALSE, some_functions = FALSE;
    int plot_num, line_num, point_num, crnt_param = 0;	/* 0 = z, 1 = y, 2 = x */
    char *xtitle;
    char *ytitle;


    /* Reset first_3dplot. This is usually done at the end of this function.
     * If there is an error within this function, the memory is left allocated,
     * since we cannot call sp_free if the list is incomplete
     */
    if (first_3dplot && plot3d_num>0) sp_free(first_3dplot);
    plot3d_num=0;
    first_3dplot = NULL;

    /* put stuff into arrays to simplify access */
    INIT_ARRAYS(FIRST_X_AXIS, xmin, xmax, autoscale_lx, is_log_x, base_log_x, log_base_log_x, 0);
    INIT_ARRAYS(FIRST_Y_AXIS, ymin, ymax, autoscale_ly, is_log_y, base_log_y, log_base_log_y, 0);
    INIT_ARRAYS(FIRST_Z_AXIS, zmin, zmax, autoscale_lz, is_log_z, base_log_z, log_base_log_z, 1);

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
    while (TRUE) {
	if (END_OF_COMMAND)
	    int_error(c_token, "function to plt3d expected");

	start_token = c_token;

	if (is_definition(c_token)) {
	    define();
	} else {
	    int specs;
	    struct surface_points *this_plot;

	    if (isstring(c_token)) {	/* data file to plot */

		/*{{{  data file */
		if (parametric && crnt_param != 0)
		    int_error(c_token, "previous parametric function not fully specified");

		if (!some_data_files) {
		    if (autoscale_lx & 1) {
			min_array[FIRST_X_AXIS] = VERYLARGE;
		    }
		    if (autoscale_lx & 2) {
			max_array[FIRST_X_AXIS] = -VERYLARGE;
		    }
		    if (autoscale_ly & 1) {
			min_array[FIRST_Y_AXIS] = VERYLARGE;
		    }
		    if (autoscale_ly & 2) {
			max_array[FIRST_Y_AXIS] = -VERYLARGE;
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

		specs = df_open(3);
		/* parses all datafile-specific modifiers */
		/* we will load the data after parsing title,with,... */

		/* for capture to key */
		this_plot->token = end_token = c_token - 1;

		/* this_plot->token is temporary, for errors in get_3ddata() */

		if (datatype[FIRST_X_AXIS] == TIME) {
		    if (specs < 3)
			int_error(c_token, "Need full using spec for x time data");
		    df_timecol[0] = 1;
		}
		if (datatype[FIRST_Y_AXIS] == TIME) {
		    if (specs < 3)
			int_error(c_token, "Need full using spec for y time data");
		    df_timecol[1] = 1;
		}
		if (datatype[FIRST_Z_AXIS] == TIME) {
		    if (specs < 3)
			df_timecol[0] = 1;
		    else
			df_timecol[2] = 1;
		}
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
		dummy_func = &plot_func;
		plot_func.at = temp_at();
		dummy_func = NULL;
		/* ignore it for now */
		some_functions = TRUE;
		end_token = c_token - 1;
		/*}}} */

	    }			/* end of IS THIS A FILE OR A FUNC block */


	    /*{{{  title */
	    if (this_plot->title) {
		free(this_plot->title);
		this_plot->title = NULL;
	    }
	    if (almost_equals(c_token, "t$itle")) {
		this_plot->title_no_enhanced = 0; /* can be enhanced */
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
		if (isstring(++c_token))
		    m_quote_capture(&(this_plot->title), c_token, c_token);
		else
		    int_error(c_token, "expecting \"title\" for plot");
		/* end of new method */
		++c_token;
	    } else if (almost_equals(c_token, "not$itle")) {
		if (xtitle != NULL)
		    xtitle[0] = '\0';
		if (ytitle != NULL)
		    ytitle[0] = '\0';
		/*   this_plot->title = NULL;   */
		++c_token;
	    } else {
		this_plot->title_no_enhanced = 1; /* filename or function cannot be enhanced */
		m_capture(&(this_plot->title), start_token, end_token);
		if (crnt_param == 2)
		    xtitle = this_plot->title;
		else if (crnt_param == 1)
		    ytitle = this_plot->title;
	    }
	    /*}}} */


	    /*{{{  line types, widths, ... */
#ifdef PM3D
	    this_plot->lp_properties.use_palette = 0;
#endif
	    this_plot->lp_properties.l_type = line_num;
	    this_plot->lp_properties.p_type = point_num;

	    if (almost_equals(c_token, "w$ith")) {
		this_plot->plot_style = get_style();
	    }
	    /* pick up line/point specs
	     * - point spec allowed if style uses points, ie style&2 != 0
	     * - keywords are optional
	     */
	    lp_parse(&this_plot->lp_properties, 1, this_plot->plot_style & 2,
		     line_num, point_num);

	    /* allow old-style syntax too - ignore case lt 3 4 for example */
	    if (!equals(c_token, ",") && !END_OF_COMMAND) {
		struct value t;
		this_plot->lp_properties.l_type =
		    this_plot->lp_properties.p_type = (int) real(const_express(&t)) - 1;

		if (!equals(c_token, ",") && !END_OF_COMMAND)
		    this_plot->lp_properties.p_type = (int) real(const_express(&t)) - 1;
	    }
	    if (this_plot->plot_style & 2)	/* lines, linesp, ... */
		if (crnt_param == 0)
		    point_num +=
			1 + (draw_contour != 0)
			+ (hidden3d != 0);

	    if (crnt_param == 0)
		line_num += 1 + (draw_contour != 0)
		    + (hidden3d != 0);
	    /*}}} */


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

		int this_token = this_plot->token;
		while (!df_eof) {
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

		    if (this_plot->num_iso_read == 0)
			/* probably df_eof, in which case we
			 * will leave loop. if not eof, then
			 * how come we got no surface ? - retry
			 * in neither case do we update tp_3d_ptr
			 */
			continue;

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
		}
		df_close();
		/*}}} */

	    } else {		/* not a data file */
		tp_3d_ptr = &(this_plot->next_sp);
		this_plot->token = c_token;	/* store for second pass */
	    }

	}			/* !is_definition() : end of scope of this_plot */


	if (equals(c_token, ","))
	    c_token++;
	else
	    break;

    }				/* while(TRUE), ie first pass */


    if (parametric && crnt_param != 0)
	int_error(NO_CARET, "parametric function not fully specified");


/*** Second Pass: Evaluate the functions ***/
    /*
     * Everything is defined now, except the function data. We expect no
     * syntax errors, etc, since the above parsed it all. This makes the code
     * below simpler. If autoscale_ly, the yrange may still change.
     * - eh ?  - z can still change.  x/y/z can change if we are parametric ??
     */

    if (some_functions) {

	/* I've changed the controlled variable in fn plots to u_min etc since
	 * it's easier for me to think parametric - 'normal' plot is after all
	 * a special case. I was confused about x_min being both minimum of
	 * x values found, and starting value for fn plots.
	 */
	register double u_min, u_max, u_step, v_min, v_max, v_step;
	double uisodiff, visodiff;
	struct surface_points *this_plot;


	if (!parametric) {
	    /*{{{  check ranges */
	    /* give error if xrange badly set from missing datafile error
	     * parametric fn can still set ranges
	     * if there are no fns, we'll report it later as 'nothing to plot'
	     */

	    if (min_array[FIRST_X_AXIS] == VERYLARGE ||
		max_array[FIRST_X_AXIS] == -VERYLARGE) {
		int_error(c_token, "x range is invalid");
	    }
	    if (min_array[FIRST_Y_AXIS] == VERYLARGE ||
		max_array[FIRST_Y_AXIS] == -VERYLARGE) {
		int_error(c_token, "y range is invalid");
	    }
	    /* check that xmin -> xmax is not too small */
	    fixup_range(FIRST_X_AXIS, "x");
	    fixup_range(FIRST_Y_AXIS, "y");
	    /*}}} */
	}
	if (parametric && !some_data_files) {
	    /*{{{  set ranges */
	    /* parametric fn can still change x/y range */
	    if (autoscale_lx & 1)
		min_array[FIRST_X_AXIS] = VERYLARGE;
	    if (autoscale_lx & 2)
		max_array[FIRST_X_AXIS] = -VERYLARGE;
	    if (autoscale_ly & 1)
		min_array[FIRST_Y_AXIS] = VERYLARGE;
	    if (autoscale_ly & 2)
		max_array[FIRST_Y_AXIS] = -VERYLARGE;
	    /*}}} */
	}
	if (parametric) {
	    u_min = umin;
	    u_max = umax;
	    v_min = vmin;
	    v_max = vmax;
	} else {
	    /*{{{  figure ranges, taking logs etc into account */
	    if (is_log_x) {
		if (min_array[FIRST_X_AXIS] <= 0.0 ||
		    max_array[FIRST_X_AXIS] <= 0.0)
		    int_error(NO_CARET, "x range must be greater than 0 for log scale!");
		u_min = log(min_array[FIRST_X_AXIS]) / log_base_log_x;
		u_max = log(max_array[FIRST_X_AXIS]) / log_base_log_x;
	    } else {
		u_min = min_array[FIRST_X_AXIS];
		u_max = max_array[FIRST_X_AXIS];
	    }

	    if (is_log_y) {
		if (min_array[FIRST_Y_AXIS] <= 0.0 ||
		    max_array[FIRST_Y_AXIS] <= 0.0) {
		    int_error(NO_CARET, "y range must be greater than 0 for log scale!");
		}
		v_min = log(min_array[FIRST_Y_AXIS]) / log_base_log_y;
		v_max = log(max_array[FIRST_Y_AXIS]) / log_base_log_y;
	    } else {
		v_min = min_array[FIRST_Y_AXIS];
		v_max = max_array[FIRST_Y_AXIS];
	    }
	    /*}}} */
	}


	if (samples_1 < 2 || samples_2 < 2 || iso_samples_1 < 2 ||
	    iso_samples_2 < 2) {
	    int_error(NO_CARET, "samples or iso_samples < 2. Must be at least 2.");
	}
	/* start over */
	this_plot = first_3dplot;
	c_token = begin_token;

	/* why do attributes of this_plot matter ? */

	if (this_plot && this_plot->has_grid_topology && hidden3d) {
	    u_step = (u_max - u_min) / (iso_samples_1 - 1);
	    v_step = (v_max - v_min) / (iso_samples_2 - 1);
	} else {
	    u_step = (u_max - u_min) / (samples_1 - 1);
	    v_step = (v_max - v_min) / (samples_2 - 1);
	}

	uisodiff = (u_max - u_min) / (iso_samples_1 - 1);
	visodiff = (v_max - v_min) / (iso_samples_2 - 1);


	/* Read through functions */
	while (TRUE) {
	    if (is_definition(c_token)) {
		define();
	    } else {

		if (!isstring(c_token)) {	/* func to plot */
		    /*{{{  evaluate function */
		    struct iso_curve *this_iso = this_plot->iso_crvs;
		    struct coordinate GPHUGE *points = this_iso->points;
		    int num_sam_to_use, num_iso_to_use;

		    if (parametric)
			crnt_param = (crnt_param + 2) % 3;

		    dummy_func = &plot_func;
		    plot_func.at = temp_at();	/* reparse function */
		    dummy_func = NULL;
		    num_iso_to_use = iso_samples_2;

		    if (!(this_plot->has_grid_topology && hidden3d))
			num_sam_to_use = samples_1;
		    else
			num_sam_to_use = iso_samples_1;

		    for (j = 0; j < num_iso_to_use; j++) {
			double y = v_min + j * visodiff;
			/* if (is_log_y) PEM fix logscale y axis */
			/* y = pow(log_base_log_y,y); 26-Sep-89 */
			/* parametric => NOT a log quantity (?) */
			(void) Gcomplex(&plot_func.dummy_values[1],
					!parametric && is_log_y ? pow(base_log_y, y) : y,
					0.0);

			for (i = 0; i < num_sam_to_use; i++) {
			    double x = u_min + i * u_step;
			    struct value a;
			    double temp;

			    /* if (is_log_x) PEM fix logscale x axis */
			    /* x = pow(base_log_x,x); 26-Sep-89 */
			    /* parametric => NOT a log quantity (?) */
			    (void) Gcomplex(&plot_func.dummy_values[0],
					    !parametric && is_log_x ? pow(base_log_x, x) : x, 0.0);

			    points[i].x = x;
			    points[i].y = y;

			    evaluate_at(plot_func.at, &a);

			    if (undefined || (fabs(imag(&a)) > zero)) {
				points[i].type = UNDEFINED;
				continue;
			    }
			    temp = real(&a);

			    points[i].type = INRANGE;
			    STORE_WITH_LOG_AND_FIXUP_RANGE(points[i].z, temp, points[i].type,
							   crnt_param, NOOP, NOOP);

			}
			this_iso->p_count = num_sam_to_use;
			this_iso = this_iso->next;
			points = this_iso ? this_iso->points : NULL;
		    }

		    if (!(this_plot->has_grid_topology && hidden3d)) {
			num_iso_to_use = iso_samples_1;
			num_sam_to_use = samples_2;
			for (i = 0; i < num_iso_to_use; i++) {
			    double x = u_min + i * uisodiff;
			    /* if (is_log_x) PEM fix logscale x axis */
			    /* x = pow(base_log_x,x); 26-Sep-89 */
			    /* if parametric, no logs involved - 3.6 */
			    (void) Gcomplex(&plot_func.dummy_values[0],
					    (!parametric && is_log_x) ? pow(base_log_x, x) : x, 0.0);

			    for (j = 0; j < num_sam_to_use; j++) {
				double y = v_min + j * v_step;
				struct value a;
				double temp;
				/* if (is_log_y) PEM fix logscale y axis */
				/* y = pow(base_log_y,y); 26-Sep-89 */
				(void) Gcomplex(&plot_func.dummy_values[1],
						(!parametric && is_log_y) ? pow(base_log_y, y) :
						y, 0.0);

				points[j].x = x;
				points[j].y = y;

				evaluate_at(plot_func.at, &a);

				if (undefined || (fabs(imag(&a)) > zero)) {
				    points[j].type = UNDEFINED;
				    continue;
				}
				temp = real(&a);

				points[j].type = INRANGE;
				STORE_WITH_LOG_AND_FIXUP_RANGE(points[j].z, temp, points[j].type,
							       crnt_param, NOOP, NOOP);
			    }
			    this_iso->p_count = num_sam_to_use;
			    this_iso = this_iso->next;
			    points = this_iso ? this_iso->points : NULL;
			}
		    }
		    /*}}} */
		}		/* end of ITS A FUNCTION TO PLOT */
		/* we saved it from first pass */
		c_token = this_plot->token;

		/* one data file can make several plots */
		do
		    this_plot = this_plot->next_sp;
		while (this_plot && this_plot->token == c_token);

	    }			/* !is_definition */

	    if (equals(c_token, ","))
		c_token++;
	    else
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
    if (min_array[FIRST_X_AXIS] == VERYLARGE ||
	max_array[FIRST_X_AXIS] == -VERYLARGE ||
	min_array[FIRST_Y_AXIS] == VERYLARGE ||
	max_array[FIRST_Y_AXIS] == -VERYLARGE ||
	min_array[FIRST_Z_AXIS] == VERYLARGE ||
	max_array[FIRST_Z_AXIS] == -VERYLARGE)
	int_error(NO_CARET, "All points undefined");

    fixup_range(FIRST_X_AXIS, "x");
    fixup_range(FIRST_Y_AXIS, "y");
    fixup_range(FIRST_Z_AXIS, "z");

    FIXUP_RANGE_FOR_LOG(FIRST_X_AXIS, x);
    FIXUP_RANGE_FOR_LOG(FIRST_Y_AXIS, y);
    FIXUP_RANGE_FOR_LOG(FIRST_Z_AXIS, z);

    /* last parameter should take plot size into effect...
     * probably needs to be moved to graph3d.c
     * in the meantime, a value of 20 gives same behaviour
     * as 3.5 which will do for the moment
     */

    if (xtics)
	setup_tics(FIRST_X_AXIS, &xticdef, xformat, 20);
    if (ytics)
	setup_tics(FIRST_Y_AXIS, &yticdef, yformat, 20);
    if (ztics)
	setup_tics(FIRST_Z_AXIS, &zticdef, zformat, 20);

#define WRITEBACK(axis,min,max) \
if(range_flags[axis]&RANGE_WRITEBACK) \
  {if (auto_array[axis]&1) min = min_array[axis]; \
   if (auto_array[axis]&2) max = max_array[axis]; \
  }

    WRITEBACK(FIRST_X_AXIS, xmin, xmax);
    WRITEBACK(FIRST_Y_AXIS, ymin, ymax);
    WRITEBACK(FIRST_Z_AXIS, zmin, zmax);

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
		fputs("Notice: cannot contour non grid data!\n", stderr);
		/* changed from int_error by recommendation of
		 * rkc@xn.ll.mit.edu
		 */
	    } else if (this_plot->plot_type == DATA3D) {
		this_plot->contours = contour(
						 this_plot->num_iso_read,
						 this_plot->iso_crvs,
						 contour_levels, contour_pts,
						 contour_kind, contour_order,
						 levels_kind, levels_list);
	    } else {
		this_plot->contours = contour(iso_samples_2,
					      this_plot->iso_crvs,
					      contour_levels, contour_pts,
					      contour_kind, contour_order,
					      levels_kind, levels_list);
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
    if (strcmp(term->name, "table") == 0)
	print_3dtable(plot_num);
    else {
	START_LEAK_CHECK();	/* assert no memory leaks here ! */
	do_3dplot(first_3dplot, plot_num, 0);
	END_LEAK_CHECK();

        /* after do_3dplot(), min_array[] and max_array[]
         * contain the plotting range actually used (rounded
         * to tic marks, not only the min/max data values)
         * --> save them now for writeback if requested
	 */

#define SAVE_WRITEBACK(axis) \
  if(range_flags[axis]&RANGE_WRITEBACK) { \
    set_writeback_min(axis,min_array[axis]); \
    set_writeback_max(axis,max_array[axis]); \
  }
        SAVE_WRITEBACK(FIRST_X_AXIS);
        SAVE_WRITEBACK(FIRST_Y_AXIS);
        SAVE_WRITEBACK(FIRST_Z_AXIS);
        SAVE_WRITEBACK(SECOND_X_AXIS);
        SAVE_WRITEBACK(SECOND_Y_AXIS);
        SAVE_WRITEBACK(SECOND_Z_AXIS);
        SAVE_WRITEBACK(T_AXIS);
        SAVE_WRITEBACK(R_AXIS);
        SAVE_WRITEBACK(U_AXIS);
	SAVE_WRITEBACK(V_AXIS);
    }
}



static void
parametric_3dfixup(start_plot, plot_num)
struct surface_points *start_plot;
int *plot_num;
/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are gauranteed to occur in (x,y,z) triplets while preserving
 * the non-FUNC type plots intact.  This means we have to work our way
 * through various lists.  Examples (hand checked):
 * start_plot:F1->F2->F3->NULL ==> F3->NULL
 * start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F3->F6->NULL
 * start_plot:F1->F2->F3->D1->D2->F4->F5->F6->D3->NULL ==>
 * F3->D1->D2->F6->D3->NULL
 */
{
/*
 * I initialized *free_list with NULL, because my compiler warns some lines
 * later that it might be uninited. The code however seems to not access that
 * line in that case, but if I'm right, my change is OK and if not, this is a
 * serious bug in the code.
 *
 * x and y ranges now fixed in eval_3dplots
 */
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
		struct coordinate GPHUGE *xpoints = xicrvs->points, GPHUGE * ypoints =
		yicrvs->points, GPHUGE * zpoints = zicrvs->points;
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
