/*
 * $Id: setshow.h,v 1.11 1999/07/20 15:32:45 lhecking Exp $
 *
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

/*
 * global variables to hold status of 'set' options
 *
 */

typedef struct {
    char text[MAX_LINE_LEN + 1];
    char font[MAX_LINE_LEN + 1];
    double xoffset, yoffset;
} label_struct;

/* Tic-mark labelling definition; see set xtics */
struct ticdef {
    int type;			/* one of five values below */
#define TIC_COMPUTED 1		/* default; gnuplot figures them */
#define TIC_SERIES   2		/* user-defined series */
#define TIC_USER     3		/* user-defined points */
#define TIC_MONTH    4		/* print out month names ((mo-1[B)%12)+1 */
#define TIC_DAY      5		/* print out day of week */
    union {
	struct {		/* for TIC_SERIES */
	    double start, incr;
	    double end;		/* ymax, if VERYLARGE */
	} series;
	struct ticmark *user;	/* for TIC_USER */
    } def;
};

/* Defines one ticmark for TIC_USER style.
 * If label==NULL, the value is printed with the usual format string.
 * else, it is used as the format string (note that it may be a constant
 * string, like "high" or "low").
 */
struct ticmark {
    double position;		/* where on axis is this */
    char *label;		/* optional (format) string label */
    struct ticmark *next;	/* linked list */
};

/* Axis properties, label, scaling, tics etc. */
struct axis_properties {
    /* values for the axis label */
    label_struct label;
    /* name of axis */
    char *name;
    TBOOLEAN autoscale;
    char *format;
    int format_is_numeric;
    TBOOLEAN is_log;
    /* base, for computing pow(base,x) */
    double base_log;
    /* log of base, for computing logbase(base,x) */
    double log_base_log;
    /* scale factor for size */
    float size;
    /* minimum, maximum values */
    double min, max;
    /* tics, minor tics, minor tics frequency (?) */
    int tics, mtics;
    /* # intervals between major tic marks */
    double mtfreq;
    TBOOLEAN rotate_tics;
    struct ticdef ticdef;
};

extern struct axis_properties x_props;
extern struct axis_properties y_props;
extern struct axis_properties z_props;
extern struct axis_properties x2_props;
extern struct axis_properties y2_props;

extern TBOOLEAN multiplot;

extern TBOOLEAN autoscale_r;
extern TBOOLEAN autoscale_t;
extern TBOOLEAN autoscale_u;
extern TBOOLEAN autoscale_v;
extern TBOOLEAN autoscale_lt;
extern TBOOLEAN autoscale_lu;
extern TBOOLEAN autoscale_lv;
extern TBOOLEAN autoscale_lx;
extern TBOOLEAN autoscale_ly;
extern TBOOLEAN autoscale_lz;
extern double boxwidth;
extern TBOOLEAN clip_points;
extern TBOOLEAN clip_lines1;
extern TBOOLEAN clip_lines2;
extern struct lp_style_type border_lp;
extern int draw_border;
#define SOUTH			1	/* 0th bit */
#define WEST			2	/* 1th bit */
#define NORTH			4	/* 2th bit */
#define EAST			8	/* 3th bit */
#define border_east		(draw_border & EAST)
#define border_west		(draw_border & WEST)
#define border_south		(draw_border & SOUTH)
#define border_north		(draw_border & NORTH)
extern TBOOLEAN draw_surface;
extern char dummy_var[MAX_NUM_VAR][MAX_ID_LEN + 1];
extern char default_font[];	/* Entry font added by DJL */
/* do these formats look like printf or time ? */
extern int format_is_numeric[];

extern char key_title[];
extern enum PLOT_STYLE data_style, func_style;
extern double bar_size;
extern struct lp_style_type work_grid, grid_lp, mgrid_lp;
extern double polar_grid_angle;	/* angle step in polar grid in radians */
extern int key;
extern struct position key_user_pos;	/* user specified position for key */
extern int key_vpos, key_hpos, key_just;
extern double key_swidth, key_vert_factor;	/* user specified vertical spacing multiplier */
extern double key_width_fix;	/* user specified additional (+/-) width of key titles */
extern TBOOLEAN key_reverse;	/* key back to front */
extern struct lp_style_type key_box;	/* linetype round box < -2 = none */

extern char *outstr;
extern TBOOLEAN parametric;
extern double pointsize;
extern TBOOLEAN polar;
extern TBOOLEAN hidden3d;
extern int angles_format;
extern double ang2rad;		/* 1 or pi/180 */
extern int mapping3d;
extern int samples;
extern int samples_1;
extern int samples_2;
extern int iso_samples_1;
extern int iso_samples_2;
extern float xoffset;
extern float yoffset;
extern float aspect_ratio;	/* 1.0 for square */
extern float surface_rot_z;
extern float surface_rot_x;
extern float surface_scale;
extern float surface_zscale;
extern char term_options[];

extern label_struct title, timelabel;

extern int timelabel_rotate;
extern int timelabel_bottom;
extern int range_flags[];
extern double rmin, rmax;
extern double tmin, tmax, umin, umax, vmin, vmax;
extern double loff, roff, toff, boff;
extern int draw_contour;
extern TBOOLEAN label_contours;
extern char contour_format[];
extern int contour_pts;
extern int contour_kind;
extern int contour_order;
extern int contour_levels;
extern double zero;		/* zero threshold, not 0! */
extern int levels_kind;
extern double *levels_list;
extern int max_levels;

extern int dgrid3d_row_fineness;
extern int dgrid3d_col_fineness;
extern int dgrid3d_norm_value;
extern TBOOLEAN dgrid3d;

#define ENCODING_DEFAULT	0
#define ENCODING_ISO_8859_1	1
#define ENCODING_CP_437		2
#define ENCODING_CP_850		3	/* JFi */

extern int encoding;
extern const char *encoding_names[];

/* -3 for no axis, or linetype */
extern struct lp_style_type xzeroaxis;
extern struct lp_style_type yzeroaxis;
extern struct lp_style_type x2zeroaxis;
extern struct lp_style_type y2zeroaxis;

extern float ticslevel;
extern double ticscale;		/* scale factor for tic marks (was (0..1]) */
extern double miniticscale;	/* and for minitics */

extern TBOOLEAN tic_in;

extern struct text_label *first_label;
extern struct arrow_def *first_arrow;
extern struct linestyle_def *first_linestyle;

extern int lmargin, bmargin, rmargin, tmargin;	/* plot border in characters */

extern char full_month_names[12][32];
extern char abbrev_month_names[12][8];

extern char full_day_names[7][32];
extern char abbrev_day_names[7][8];

/* The set and show commands, in setshow.c */
void set_command __PROTO((void));
void reset_command __PROTO((void));
void show_command __PROTO((void));
/* and some accessible support functions */
enum PLOT_STYLE get_style __PROTO((void));
TBOOLEAN load_range __PROTO((int axis, double *a, double *b, int autosc));
void show_version __PROTO((FILE * fp));
void show_version_long __PROTO((void));
char *conv_text __PROTO((const char *));
void lp_use_properties __PROTO((struct lp_style_type * lp, int tag, int pointflag));
void reset_axis_properties __PROTO((struct axis_properties *, char *, int));

/* string representing missing values, ascii datafiles */
extern char *missing_val;

/* */
#define SETSHOWMSG \
"valid set options:  [] = choose one, {} means optional 'all',\n\n\
\t'all',  'angles',  'arrow',  'autoscale',  'bar', 'border',\n\
\t'boxwidth', 'clip', 'cntrparam', 'contour',  'data',  'dgrid3d',\n\
\t'dummy', 'encoding', 'format', 'function',  'grid',  'hidden',\n\
\t'isosamples', 'key', 'label', 'linestyle', 'loadpath', 'locale',\n\
\t'logscale', 'mapping', 'margin', 'missing', 'offsets', 'origin',\n\
\t'output', 'plot', 'parametric', 'pointsize', 'polar', '[rtuv]range',\n\
\t'samples', 'size', 'terminal', 'tics', 'timestamp', 'timefmt', 'title',\n\
\t'variables', 'version', 'view', '[xyz]{2}label', '[xyz]{2}range',\n\
\t'{m}[xyz]{2}tics', '[xyz]{2}[md]tics', '[xyz]{2}zeroaxis',\n\
\t'[xyz]data',   'zero', 'zeroaxis'"

/* */
enum setshow_id {
    S_INVALID,
    S_ACTIONTABLE, S_ALL, S_ANGLES, S_ARROW, S_AUTOSCALE, S_BARS, S_BORDER,
    S_BOXWIDTH, S_CLABEL, S_CLIP, S_CNTRPARAM, S_CONTOUR, S_DATA, S_DGRID3D,
    S_DUMMY, S_ENCODING, S_FORMAT, S_FUNCTIONS, S_GRID, S_HIDDEN3D,
    S_ISOSAMPLES, S_KEY, S_KEYTITLE, S_LABEL, S_LINESTYLE, S_LOADPATH,
    S_LOCALE, S_LOGSCALE, S_LS, S_MAPPING, S_MARGIN, S_MISSING, S_MX2TICS,
    S_MXTICS, S_MY2TICS, S_MYTICS, S_MZTICS, S_OFFSETS, S_ORIGIN, S_OUTPUT,
    S_PARAMETRIC, S_PLOT, S_POINTSIZE, S_POLAR, S_RRANGE, S_SAMPLES, S_SIZE,
    S_SURFACE, S_TERMINAL, S_TICS, S_TIMEFMT, S_TIMESTAMP, S_TITLE, S_TRANGE,
    S_URANGE, S_VARIABLES, S_VERSION, S_VIEW, S_VRANGE, S_X2DATA, S_X2DTICS,
    S_X2LABEL, S_X2MTICS, S_X2RANGE, S_X2TICS, S_XDATA, S_XDTICS, S_XLABEL,
    S_XMTICS, S_XRANGE, S_XTICS, S_XZEROAXIS, S_Y2DATA, S_Y2DTICS, S_Y2LABEL,
    S_Y2MTICS, S_Y2RANGE, S_Y2TICS, S_YDATA, S_YDTICS, S_YLABEL, S_YMTICS,
    S_YRANGE, S_YTICS, S_YZEROAXIS, S_ZDATA, S_ZDTICS, S_ZERO, S_ZEROAXIS,
    S_ZLABEL, S_ZMTICS, S_ZRANGE, S_ZTICS
};

#endif /* GNUPLOT_SETSHOW_H */
