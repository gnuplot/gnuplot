/*
 * $Id: setshow.h,v 1.23.2.1 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - setshow.h */

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


#ifndef GNUPLOT_SETSHOW_H
# define GNUPLOT_SETSHOW_H

#include "variable.h"

#ifndef DEFAULT_TIMESTAMP_FORMAT
/* asctime() format */
# define DEFAULT_TIMESTAMP_FORMAT "%a %b %d %H:%M:%S %Y"
#endif

#ifndef TIMEFMT
# define TIMEFMT "%d/%m/%y,%H:%M"
#endif

/* default format for tic mark labels */
#define DEF_FORMAT "%g"

/* less than one hundredth of a tic mark */
#define SIGNIF (0.01)

typedef struct {
    char text[MAX_LINE_LEN+1];
    char font[MAX_LINE_LEN+1];
    double xoffset, yoffset;
} label_struct;


/*
 * global variables to hold status of 'set' options
 *
 */

extern int angles_format;
extern double ang2rad; /* 1 or pi/180 */
extern struct arrow_def *first_arrow;
extern TBOOLEAN autoscale_x, autoscale_y, autoscale_z, autoscale_x2,
    autoscale_y2;
extern TBOOLEAN autoscale_r, autoscale_t, autoscale_u, autoscale_v;
extern TBOOLEAN autoscale_lu, autoscale_lv, autoscale_lx,
    autoscale_ly, autoscale_lz;
extern double bar_size;

extern struct lp_style_type     border_lp;
extern int			draw_border;
#define SOUTH			1 /* 0th bit */
#define WEST			2 /* 1th bit */
#define NORTH			4 /* 2th bit */
#define EAST			8 /* 3th bit */
#define border_east		(draw_border & EAST)
#define border_west		(draw_border & WEST)
#define border_south		(draw_border & SOUTH)
#define border_north		(draw_border & NORTH)

extern TBOOLEAN                 multiplot;

extern double			boxwidth;
extern TBOOLEAN			clip_points;
extern TBOOLEAN			clip_lines1;
extern TBOOLEAN			clip_lines2;
extern TBOOLEAN			draw_surface;
extern char			dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
extern char			xformat[];
extern char			yformat[];
extern char			zformat[];
extern char			x2format[];
extern char			y2format[];

/* format for date/time for reading time in datafile */
extern char timefmt[];
extern int datatype[];

/* do these formats look like printf or time ? */
extern int format_is_numeric[];

extern char			key_title[];
extern enum PLOT_STYLE data_style, func_style;
extern struct lp_style_type     work_grid, grid_lp, mgrid_lp;
extern double     polar_grid_angle; /* angle step in polar grid in radians */
extern int			key;
extern struct position key_user_pos; /* user specified position for key */
extern int 			key_vpos, key_hpos, key_just;
extern double       key_swidth, key_vert_factor; /* user specified vertical spacing multiplier */
extern double                   key_width_fix; /* user specified additional (+/-) width of key titles */
extern TBOOLEAN			key_reverse;  /* key back to front */
extern struct lp_style_type 	key_box;  /* linetype round box < -2 = none */
extern TBOOLEAN			is_log_x, is_log_y, is_log_z;
extern double			base_log_x, base_log_y, base_log_z;
				/* base, for computing pow(base,x) */
extern double			log_base_log_x, log_base_log_y, log_base_log_z;
				/* log of base, for computing logbase(base,x) */
extern TBOOLEAN			is_log_x2, is_log_y2;
extern double			base_log_x2, base_log_y2;
				/* base, for computing pow(base,x) */
extern double			log_base_log_x2, log_base_log_y2;
				/* log of base, for computing logbase(base,x) */
extern char			*outstr;
extern TBOOLEAN			parametric;
extern double			pointsize;
extern TBOOLEAN			polar;
extern TBOOLEAN			hidden3d;
extern int			mapping3d;
extern int			samples;
extern int			samples_1;
extern int			samples_2;
extern int			iso_samples_1;
extern int			iso_samples_2;
extern float			xsize; /* scale factor for size */
extern float                    xoffset;
extern float                    yoffset;
extern float			ysize; /* scale factor for size */
extern float			zsize; /* scale factor for size */
extern float			aspect_ratio; /* 1.0 for square */
extern float			surface_rot_z;
extern float			surface_rot_x;
extern float			surface_scale;
extern float			surface_zscale;
extern char			term_options[];

extern label_struct title, timelabel;
extern label_struct xlabel, ylabel, zlabel;
extern label_struct x2label, y2label;

extern int			timelabel_rotate;
extern int			timelabel_bottom;
extern int			range_flags[];
extern double			rmin, rmax;
extern double			tmin, tmax, umin, umax, vmin, vmax;
extern double			xmin, xmax, ymin, ymax, zmin, zmax;
extern double			x2min, x2max, y2min, y2max;
extern double			loff, roff, toff, boff;
extern int			draw_contour;
extern TBOOLEAN      label_contours;
extern char			contour_format[];
extern int			contour_pts;
extern int			contour_kind;
extern int			contour_order;
extern int			contour_levels;
extern double			zero; /* zero threshold, not 0! */
extern int			levels_kind;
extern double			*levels_list;

extern int			dgrid3d_row_fineness;
extern int			dgrid3d_col_fineness;
extern int			dgrid3d_norm_value;
extern TBOOLEAN			dgrid3d;

extern int			encoding;
extern const char		*encoding_names[];

/* -3 for no axis, or linetype */
extern struct lp_style_type xzeroaxis;
extern struct lp_style_type yzeroaxis;
extern struct lp_style_type x2zeroaxis;
extern struct lp_style_type y2zeroaxis;

extern int xtics;
extern int ytics;
extern int ztics;
extern int mxtics;
extern int mytics;
extern int mztics;
extern int x2tics;
extern int y2tics;
extern int mx2tics;
extern int my2tics;
extern double mxtfreq;
extern double mytfreq;
extern double mztfreq;
extern double mx2tfreq;
extern double my2tfreq;
extern TBOOLEAN rotate_xtics;
extern TBOOLEAN rotate_ytics;
extern TBOOLEAN rotate_ztics;
extern TBOOLEAN rotate_x2tics;
extern TBOOLEAN rotate_y2tics;

extern float ticslevel;
extern double ticscale; /* scale factor for tic marks (was (0..1])*/
extern double miniticscale; /* and for minitics */

extern struct ticdef xticdef;
extern struct ticdef yticdef;
extern struct ticdef zticdef;
extern struct ticdef x2ticdef;
extern struct ticdef y2ticdef;

extern TBOOLEAN			tic_in;

extern struct text_label *first_label;
extern struct linestyle_def *first_linestyle;

extern int lmargin, bmargin,rmargin,tmargin; /* plot border in characters */

/* HBB 19991108 FIXME: this is way off. There is no setshow.c, and
 * these functions ought to be prototyped in their own .h files, each!
 * On a related note, all the status variables don't really belong in
 * here, I think... */
/* The set and show commands, in setshow.c */
void set_command __PROTO((void));
void unset_command __PROTO((void));
void reset_command __PROTO((void));
void show_command __PROTO((void));
/* and some accessible support functions */
enum PLOT_STYLE get_style __PROTO((void));
TBOOLEAN load_range __PROTO((int axis, double *a, double *b, TBOOLEAN autosc));
void show_version __PROTO((FILE *fp));
char *conv_text __PROTO((const char *s));
void lp_parse __PROTO((struct lp_style_type *, int, int, int, int));
void delete_linestyle __PROTO((struct linestyle_def *, struct linestyle_def *));

/* should be in set.h */
int set_label_tag __PROTO((void));
double get_writeback_min __PROTO((int axis));
double get_writeback_max __PROTO((int axis));
void set_writeback_min __PROTO((int axis, double val));
void set_writeback_max __PROTO((int axis, double val));

/* string representing missing values, ascii datafiles */
extern char *missing_val;

#endif /* GNUPLOT_SETSHOW_H */
