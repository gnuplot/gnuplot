/* GNUPLOT - filters.h */

#ifndef GNUPLOT_FILTERS_H
# define GNUPLOT_FILTERS_H

#include "syscfg.h"
#include "graphics.h"
#include "graph3d.h"

/* Type definitions */

/* If gnuplot ever gets a subsystem to do clustering, this would belong there.
 * As of now, however, the only clustering operations are outlier detection
 * and finding a convex hull, for which the code is in filters.c.
 */
typedef struct cluster {
    int npoints;
    double cx;		/* x coordinate of center-of-mass */
    double cy;		/* y coordinate of center-of-mass */
    double rmsd;	/* root mean square distance from center of mass */
    double threshold;	/* criterion for outlier detection */
} t_cluster;

/* Exported functions */

void mcs_interp(struct curve_points *plot);
void make_bins(struct curve_points *plot,
		int nbins, double binlow, double binhigh, double binwidth, int binopt);
void gen_3d_splines(struct surface_points *plot);
void gen_2d_path_splines(struct curve_points *plot);
void convex_hull(struct curve_points *plot);
void expand_hull(struct curve_points *plot);

#endif /* GNUPLOT_FILTERS_H */
