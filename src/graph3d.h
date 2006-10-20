/*
 * $Id: graph3d.h,v 1.24 2006/03/26 05:08:27 sfeam Exp $
 */

/* GNUPLOT - graph3d.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_GRAPH3D_H
# define GNUPLOT_GRAPH3D_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "gp_types.h"

#include "gadgets.h"
#include "term_api.h"

/* Function macros to map from user 3D space into normalized -1..1 */
#define map_x3d(x) ((x-X_AXIS.min)*xscale3d-1.0)
#define map_y3d(y) ((y-Y_AXIS.min)*yscale3d-1.0)
#define map_z3d(z) ((z-floor_z)*zscale3d-1.0)

/* Type definitions */

typedef enum en_contour_placement {
    /* Where to place contour maps if at all. */
    CONTOUR_NONE,
    CONTOUR_BASE,
    CONTOUR_SRF,
    CONTOUR_BOTH
} t_contour_placement;

typedef double transform_matrix[4][4]; /* HBB 990826: added */

typedef struct gnuplot_contours {
    struct gnuplot_contours *next;
    struct coordinate GPHUGE *coords;
    char isNewLevel;
    char label[32];
    int num_pts;
    double z;
} gnuplot_contours;

typedef struct iso_curve {
    struct iso_curve *next;
    int p_max;			/* how many points are allocated */
    int p_count;			/* count of points in points */
    struct coordinate GPHUGE *points;
} iso_curve;

typedef struct surface_points {
    struct surface_points *next_sp; /* linked list */
    int token;			/* last token nr, for second pass */
    int plot_num;		/* needed for tracking iteration */
    enum PLOT_TYPE plot_type;
    enum PLOT_STYLE plot_style;
    char *title;
    TBOOLEAN title_no_enhanced;	/* don't typeset title in enhanced mode */
    TBOOLEAN opt_out_of_hidden3d; /* set by "with nohidden" opeion */
    struct lp_style_type lp_properties;
    struct arrow_style_type arrow_properties;
    int has_grid_topology;

    /* Data files only - num of isolines read from file. For
     * functions, num_iso_read is the number of 'primary' isolines (in
     * x direction) */
    int num_iso_read;
    struct gnuplot_contours *contours; /* NULL if not doing contours. */
    struct iso_curve *iso_crvs;	/* the actual data */
#ifdef EAM_DATASTRINGS
    struct text_label *labels;	/* Only used if plot_style == LABELPOINTS */
#endif
    TBOOLEAN pm3d_color_from_column;
    char pm3d_where[7];		/* explicitly given base, top, surface */

} surface_points;

/* Variables of graph3d.c needed by other modules: */

extern int xmiddle, ymiddle, xscaler, yscaler;
extern double floor_z;
extern double ceiling_z, base_z; /* made exportable for PM3D */
extern transform_matrix trans_mat;
extern double xscale3d, yscale3d, zscale3d;

extern t_contour_placement draw_contour;
extern TBOOLEAN	label_contours;

extern TBOOLEAN	draw_surface;

/* is hidden3d display wanted? */
extern TBOOLEAN	hidden3d;

extern float surface_rot_z;
extern float surface_rot_x;
extern float surface_scale;
extern float surface_zscale;
extern int splot_map;

typedef struct { 
    double ticslevel; 
    double xyplane_z; 
    TBOOLEAN absolute;
} t_xyplane;

extern t_xyplane xyplane;

#define ISO_SAMPLES 10		/* default number of isolines per splot */
extern int iso_samples_1;
extern int iso_samples_2;

#ifdef USE_MOUSE
extern int axis3d_o_x, axis3d_o_y, axis3d_x_dx, axis3d_x_dy, axis3d_y_dx, axis3d_y_dy;
#endif

/* Prototypes from file "graph3d.c" */

void do_3dplot __PROTO((struct surface_points *plots, int pcount, int quick));
void map3d_position __PROTO((struct position *pos, int *x, int *y, const char *what));
void map3d_position_double __PROTO((struct position *pos, double *x, double *y, const char *what));
void map3d_position_r __PROTO((struct position *pos, int *x, int *y, const char *what));


#endif /* GNUPLOT_GRAPH3D_H */
