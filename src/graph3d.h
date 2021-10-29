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

/* Type definitions */

typedef enum en_dgrid3d_mode {
    DGRID3D_DEFAULT,
    DGRID3D_QNORM,
    DGRID3D_SPLINES,
    DGRID3D_GAUSS,
    DGRID3D_EXP,
    DGRID3D_CAUCHY,
    DGRID3D_BOX,
    DGRID3D_HANN,
    DGRID3D_OTHER
} t_dgrid3d_mode;

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
    struct coordinate *coords;
    char isNewLevel;
    char label[32];
    int num_pts;
    double z;
} gnuplot_contours;

typedef struct iso_curve {
    struct iso_curve *next;
    int p_max;			/* how many points are allocated */
    int p_count;			/* count of points in points */
    struct coordinate *points;
} iso_curve;

typedef struct surface_points {

    struct surface_points *next_sp; /* pointer to next plot in linked list */
    int token;			/* last token used, for second parsing pass */
    enum PLOT_TYPE plot_type;	/* DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? */
    enum PLOT_STYLE plot_style;	/* style set by "with" or by default */
    char *title;		/* plot title, a.k.a. key entry */
    t_position *title_position;	/* title at {beginning|end|<xpos>,<ypos>} */
    TBOOLEAN title_no_enhanced;	/* don't typeset title in enhanced mode */
    TBOOLEAN title_is_automated;/* TRUE if title was auto-generated */
    TBOOLEAN title_is_suppressed;/* TRUE if 'notitle' was specified */
    TBOOLEAN noautoscale;	/* ignore data from this plot during autoscaling */
    struct lp_style_type lp_properties;
    struct arrow_style_type arrow_properties;
    struct fill_style_type fill_properties;
    struct text_label *labels;	/* Only used if plot_style == LABELPOINTS */
    struct t_image image_properties;	/* only used if plot_style is IMAGE, RGBIMAGE or RGBA_IMAGE */
    struct udvt_entry *sample_var;	/* used by '+' if plot has private sampling range */
    struct udvt_entry *sample_var2;	/* used by '++' if plot has private sampling range */

    /* 2D and 3D plot structure fields overlay only to this point */

    TBOOLEAN opt_out_of_hidden3d; /* set by "nohidden" option to splot command */
    TBOOLEAN opt_out_of_contours; /* set by "nocontours" option to splot command */
    TBOOLEAN opt_out_of_surface;  /* set by "nosurface" option to splot command */
    TBOOLEAN opt_out_of_dgrid3d;  /* set by "nogrid" option to splot command */
    TBOOLEAN pm3d_color_from_column;
    TBOOLEAN has_grid_topology;
    int hidden3d_top_linetype;	/* before any calls to load_linetype() */
    int iteration;		/* needed for tracking iteration */

    struct vgrid *vgrid;	/* used only for voxel plots */
    double iso_level;		/* used only for voxel plots */

    /* Data files only - num of isolines read from file. For functions,  */
    /* num_iso_read is the number of 'primary' isolines (in x direction) */
    int num_iso_read;
    struct gnuplot_contours *contours; /* NULL if not doing contours. */
    struct iso_curve *iso_crvs;	/* the actual data */
    char pm3d_where[7];		/* explicitly given base, top, surface */

} surface_points;

/* Variables of graph3d.c needed by other modules: */

extern int xmiddle, ymiddle, xscaler, yscaler;
extern double floor_z, floor_z1;
extern double ceiling_z, base_z; /* made exportable for PM3D */
extern transform_matrix trans_mat;
extern double xscale3d, yscale3d, zscale3d;
extern double xcenter3d, ycenter3d, zcenter3d;
extern double radius_scaler;

extern t_contour_placement draw_contour;
extern TBOOLEAN	clabel_onecolor;
extern int clabel_start;
extern int clabel_interval;
extern char *clabel_font;

extern TBOOLEAN	draw_surface;
extern TBOOLEAN	implicit_surface;

/* is hidden3d display wanted? */
extern TBOOLEAN	hidden3d;
extern int hidden3d_layer;	/* LAYER_FRONT or LAYER_BACK */

extern float surface_rot_z;
extern float surface_rot_x;
extern float surface_scale;
extern float surface_zscale;
extern float surface_lscale;
extern float mapview_scale;
extern float azimuth;
extern TBOOLEAN splot_map, xz_projection, yz_projection;

typedef struct { 
    double z; 
    TBOOLEAN absolute;
} t_xyplane;

extern t_xyplane xyplane;

#define ISO_SAMPLES 10		/* default number of isolines per splot */
extern int iso_samples_1;
extern int iso_samples_2;

#ifdef USE_MOUSE
extern int axis3d_o_x, axis3d_o_y, axis3d_x_dx, axis3d_x_dy, axis3d_y_dx, axis3d_y_dy;
#endif

typedef enum {
    NORMAL_REPLOT = 0,	/* e.g. "replot" command */
    AXIS_ONLY_ROTATE,	/* suppress replots during 3D rotation by ctrl-left-mouse */
    NORMAL_REFRESH,	/* e.g. "refresh" command */
    QUICK_REFRESH	/* auto-generated refresh during 3D rotation */
} REPLOT_TYPE;
    

/* Prototypes from file "graph3d.c" */

void do_3dplot(struct surface_points *plots, int pcount, REPLOT_TYPE quick);
void map3d_position(struct position *pos, int *x, int *y, const char *what);
void map3d_position_double(struct position *pos, double *x, double *y, const char *what);
void map3d_position_r(struct position *pos, int *x, int *y, const char *what);
void map3d_position_r_double(struct position *pos, double *x, double *y, const char *what);


#endif /* GNUPLOT_GRAPH3D_H */
