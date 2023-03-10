/* GNUPLOT - filters.h */

#ifndef GNUPLOT_FILTERS_H
# define GNUPLOT_FILTERS_H

#include "syscfg.h"
#include "graphics.h"
#include "graph3d.h"

/* Type definitions */

/* If gnuplot ever gets a subsystem to do clustering, this would belong there.
 * As of now, however, the only clustering operations are outlier detection
 * and finding a convex or concave hull, for which the code is in filters.c.
 */
typedef struct cluster {
    int npoints;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double cx;		/* x coordinate of center-of-mass */
    double cy;		/* y coordinate of center-of-mass */
    int pin;		/* index of point with minimum x */
} t_cluster;

/* Exported functions */

void mcs_interp(struct curve_points *plot);
void make_bins(struct curve_points *plot,
		int nbins, double binlow, double binhigh, double binwidth, int binopt);
void gen_3d_splines(struct surface_points *plot);
void gen_2d_path_splines(struct curve_points *plot);
void convex_hull(struct curve_points *plot);
void expand_hull(struct curve_points *plot);
void sharpen(struct curve_points *plot);

#ifdef WITH_CHI_SHAPES
void delaunay_triangulation( struct curve_points *plot );
void save_delaunay_triangles( struct curve_points *plot );
void concave_hull( struct curve_points *plot );
void reset_hulls( TBOOLEAN reset );

extern double chi_shape_default_fraction;	/* This is fraction of longest edge */
#else
#define reset_hulls(foo) /* NOOP */
#endif /* WITH_CHI_SHAPES */

#endif /* GNUPLOT_FILTERS_H */
