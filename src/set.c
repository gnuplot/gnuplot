#ifndef lint
static char *RCSid() { return RCSid("$Id: set.c,v 1.32.2.10 2000/10/23 04:35:28 joze Exp $"); }
#endif

/* GNUPLOT - set.c */

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


/*
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */

#include "setshow.h"

#include "alloc.h"
#include "command.h"
#include "gp_time.h"
#include "hidden3d.h"
#include "misc.h"
#include "parse.h"
#include "plot2d.h"
#include "plot3d.h"
#include "tables.h"
#include "term_api.h"
#include "util.h"
#ifdef USE_MOUSE
#   include "mouse.h"
#endif

#ifdef PM3D
#include "pm3d.h"
#endif

#define BACKWARDS_COMPATIBLE

/*
 * global variables to hold status of 'set' options
 *
 * IMPORTANT NOTE:
 * ===============
 * If you change the default values of one of the variables below, or if
 * you add another global variable, make sure that the change you make is
 * done in reset_command() as well (if that makes sense).
 */

/* set angles */
int angles_format = ANGLES_RADIANS;
double ang2rad = 1.0;		/* 1 or pi/180, tracking angles_format */

/* set arrow */
struct arrow_def *first_arrow = NULL;

/* set autoscale */
TBOOLEAN autoscale_x = DTRUE;
TBOOLEAN autoscale_y = DTRUE;
TBOOLEAN autoscale_z = DTRUE;
TBOOLEAN autoscale_x2 = DTRUE;
TBOOLEAN autoscale_y2 = DTRUE;
TBOOLEAN autoscale_r = DTRUE;
TBOOLEAN autoscale_t = DTRUE;
TBOOLEAN autoscale_u = DTRUE;
TBOOLEAN autoscale_v = DTRUE;
TBOOLEAN autoscale_lu = DTRUE;
TBOOLEAN autoscale_lv = DTRUE;
TBOOLEAN autoscale_lx = DTRUE;
TBOOLEAN autoscale_ly = DTRUE;
TBOOLEAN autoscale_lz = DTRUE;

/* set bars */
double bar_size = 1.0;

/* set border */
#ifdef PM3D
struct lp_style_type border_lp = { 0, -2, 0, 1.0, 1.0, 0 };
#else
struct lp_style_type border_lp = { 0, -2, 0, 1.0, 1.0 };
#endif
int draw_border = 31;

TBOOLEAN multiplot = FALSE;

double boxwidth = -1.0;		/* box width (automatic) */
TBOOLEAN clip_points = FALSE;
TBOOLEAN clip_lines1 = TRUE;
TBOOLEAN clip_lines2 = FALSE;
TBOOLEAN draw_surface = TRUE;
char dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };
char xformat[MAX_ID_LEN+1] = DEF_FORMAT;
char yformat[MAX_ID_LEN+1] = DEF_FORMAT;
char zformat[MAX_ID_LEN+1] = DEF_FORMAT;
char x2format[MAX_ID_LEN+1] = DEF_FORMAT;
char y2format[MAX_ID_LEN+1] = DEF_FORMAT;

/* do formats look like times - use FIRST_X_AXIS etc as index
 * - never saved or shown ...
 */
#if AXIS_ARRAY_SIZE != 10
/* # error error in initialiser for format_is_numeric */
# define AXIS_ARRAY_SIZE  AXIS_ARRAY_SIZE_not_defined
#endif

int format_is_numeric[AXIS_ARRAY_SIZE] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

enum PLOT_STYLE data_style = POINTSTYLE;
enum PLOT_STYLE func_style = LINES;

#ifdef PM3D
struct lp_style_type work_grid = { 0, GRID_OFF, 0, 1.0, 1.0, 0 };
struct lp_style_type grid_lp   = { 0, -1, 0, 1.0, 1.0, 0 };
struct lp_style_type mgrid_lp  = { 0, -1, 0, 1.0, 1.0, 0 };
struct lp_style_type key_box = { 0, -3, 0, 1.0, 1.0, 0 };
#else
struct lp_style_type work_grid = { 0, GRID_OFF, 0, 1.0, 1.0 };
struct lp_style_type grid_lp   = { 0, -1, 0, 1.0, 1.0 };
struct lp_style_type mgrid_lp  = { 0, -1, 0, 1.0, 1.0 };
struct lp_style_type key_box = { 0, -3, 0, 1.0, 1.0 };		/* -3 = no linetype */
#endif
double polar_grid_angle = 0;	/* nonzero means a polar grid */
int key = -1;			/* default position */
struct position key_user_pos;	/* user specified position for key */
TBOOLEAN key_reverse = FALSE;	/* reverse text & sample ? */
double key_swidth = 4.0;
double key_vert_factor = 1.0;
double key_width_fix = 0.0;
TBOOLEAN is_log_x = FALSE;
TBOOLEAN is_log_y = FALSE;
TBOOLEAN is_log_z = FALSE;
TBOOLEAN is_log_x2 = FALSE;
TBOOLEAN is_log_y2 = FALSE;
double base_log_x = 0.0;
double base_log_y = 0.0;
double base_log_z = 0.0;
double base_log_x2 = 0.0;
double base_log_y2 = 0.0;
double log_base_log_x = 0.0;
double log_base_log_y = 0.0;
double log_base_log_z = 0.0;
double log_base_log_x2 = 0.0;
double log_base_log_y2 = 0.0;
char *outstr = NULL;		/* means "STDOUT" */
TBOOLEAN parametric = FALSE;
double pointsize = 1.0;

int encoding;

TBOOLEAN polar = FALSE;
TBOOLEAN hidden3d = FALSE;
TBOOLEAN label_contours = TRUE;	/* different linestyles are used for contours when set */
char contour_format[32] = "%8.3g";	/* format for contour key entries */
int mapping3d = MAP3D_CARTESIAN;
int samples = SAMPLES;		/* samples is always equal to samples_1 */
int samples_1 = SAMPLES;
int samples_2 = SAMPLES;
int iso_samples_1 = ISO_SAMPLES;
int iso_samples_2 = ISO_SAMPLES;
float xsize = 1.0;		/* scale factor for size */
float ysize = 1.0;		/* scale factor for size */
float zsize = 1.0;		/* scale factor for size */
float xoffset = 0.0;		/* x origin */
float yoffset = 0.0;		/* y origin */
float aspect_ratio = 0.0;	/* don't attempt to force it */
float surface_rot_z = 30.0;	/* Default 3d transform. */
float surface_rot_x = 60.0;
float surface_scale = 1.0;
float surface_zscale = 1.0;
struct termentry *term = NULL;	/* unknown */
char term_options[MAX_LINE_LEN+1] = "";
label_struct title = { "", "", 0.0, 0.0 };
label_struct timelabel = { "", "", 0.0, 0.0 };
label_struct xlabel = { "", "", 0.0, 0.0 };
label_struct ylabel = { "", "", 0.0, 0.0 };
label_struct zlabel = { "", "", 0.0, 0.0 };
label_struct x2label = { "", "", 0.0, 0.0 };
label_struct y2label = { "", "", 0.0, 0.0 };

int timelabel_rotate = FALSE;
int timelabel_bottom = TRUE;
char key_title[MAX_LINE_LEN+1] = "";
double rmin = -0.0;
double rmax = 10.0;
double tmin = -5.0;
double tmax = 5.0;
double umin = -5.0;
double umax = 5.0;
double vmin = -5.0;
double vmax = 5.0;
double xmin = -10.0;
double xmax = 10.0;
double ymin = -10.0;
double ymax = 10.0;
double zmin = -10.0;
double zmax = 10.0;
double x2min = -10.0;
double x2max = 10.0;
double y2min = -10.0;
double y2max = 10.0;
/* ULIG                  from plot.h:       z      y      x      t     z2     y2     x2     r     u     v  */
double writeback_min[AXIS_ARRAY_SIZE] = {-10.0, -10.0, -10.0, -5.0, -10.0, -10.0, -10.0, -0.0, -5.0, -5.0};
double writeback_max[AXIS_ARRAY_SIZE] = {+10.0, +10.0, +10.0, +5.0, +10.0, +10.0, +10.0, 10.0, +5.0, +5.0};
double loff = 0.0;
double roff = 0.0;
double toff = 0.0;
double boff = 0.0;
int draw_contour = CONTOUR_NONE;
int contour_pts = 5;
int contour_kind = CONTOUR_KIND_LINEAR;
int contour_order = 4;
int contour_levels = 5;
double zero = ZERO;		/* zero threshold, not 0! */
int levels_kind = LEVELS_AUTO;
double *levels_list;		/* storage for z levels to draw contours at */
static int max_levels = 0;	/* contour level capacity, before enlarging */

int dgrid3d_row_fineness = 10;
int dgrid3d_col_fineness = 10;
int dgrid3d_norm_value = 1;
TBOOLEAN dgrid3d = FALSE;

#ifdef PM3D
struct lp_style_type xzeroaxis = { 0, -3, 0, 1.0, 1.0, 0 };
struct lp_style_type yzeroaxis = { 0, -3, 0, 1.0, 1.0, 0 };
struct lp_style_type x2zeroaxis = { 0, -3, 0, 1.0, 1.0, 0 };
struct lp_style_type y2zeroaxis = { 0, -3, 0, 1.0, 1.0, 0 };
#else
struct lp_style_type xzeroaxis = { 0, -3, 0, 1.0, 1.0 };
struct lp_style_type yzeroaxis = { 0, -3, 0, 1.0, 1.0 };
struct lp_style_type x2zeroaxis = { 0, -3, 0, 1.0, 1.0 };
struct lp_style_type y2zeroaxis = { 0, -3, 0, 1.0, 1.0 };
#endif

/* perhaps make these into an array one day */

int xtics = TICS_ON_BORDER | TICS_MIRROR;
int ytics = TICS_ON_BORDER | TICS_MIRROR;
int ztics = TICS_ON_BORDER;	/* no mirror by default for ztics */
int x2tics = NO_TICS;
int y2tics = NO_TICS;

TBOOLEAN rotate_xtics = FALSE;
TBOOLEAN rotate_ytics = FALSE;
TBOOLEAN rotate_ztics = FALSE;
TBOOLEAN rotate_x2tics = FALSE;
TBOOLEAN rotate_y2tics = FALSE;

int range_flags[AXIS_ARRAY_SIZE];	/* = {0,0,...} */

int mxtics = MINI_DEFAULT;
int mytics = MINI_DEFAULT;
int mztics = MINI_DEFAULT;
int mx2tics = MINI_DEFAULT;
int my2tics = MINI_DEFAULT;

double mxtfreq = 10;		/* # intervals between major */
double mytfreq = 10;		/* tic marks */
double mztfreq = 10;
double mx2tfreq = 10;
double my2tfreq = 10;

double ticscale = 1.0;		/* scale factor for tic mark */
double miniticscale = 0.5;	/* and for minitics */

float ticslevel = 0.5;

struct ticdef xticdef = { TIC_COMPUTED, { NULL } };
struct ticdef yticdef = { TIC_COMPUTED, { NULL } };
struct ticdef zticdef = { TIC_COMPUTED, { NULL } };
struct ticdef x2ticdef = { TIC_COMPUTED, { NULL } };
struct ticdef y2ticdef = { TIC_COMPUTED, { NULL } };

TBOOLEAN tic_in = TRUE;

struct text_label *first_label = NULL;
struct linestyle_def *first_linestyle = NULL;

/* space between left edge and xleft in chars (-1: computed) */
int lmargin = -1;
/* space between bottom and ybot in chars (-1: computed) */
int bmargin = -1;
/* space between right egde and xright in chars (-1: computed) */
int rmargin = -1;
/* space between top egde and ytop in chars (-1: computed) */
int tmargin = -1;

/* string representing missing values in ascii datafiles */
char *missing_val = NULL;

/*** other things we need *****/

/* input data, parsing variables */

int key_hpos = TRIGHT;		/* place for curve-labels, corner or outside */
int key_vpos = TTOP;		/* place for curve-labels, corner or below */
int key_just = JRIGHT;		/* alignment of key labels, left or right */

#ifndef TIMEFMT
#define TIMEFMT "%d/%m/%y\n%H:%M"
#endif
/* format for date/time for reading time in datafile */
char timefmt[25] = TIMEFMT;

/* array of datatypes (x in 0,y in 1,z in 2,..(rtuv)) */
/* not sure how rtuv come into it ?
 * oh well, make first six compatible with FIRST_X_AXIS, etc
 */
int datatype[DATATYPE_ARRAY_SIZE];

static void set_angles __PROTO((void));
static void set_arrow __PROTO((void));
static int assign_arrow_tag __PROTO((void));
static void delete_arrow __PROTO((struct arrow_def *, struct arrow_def *));
static void set_autoscale __PROTO((void));
static void set_bars __PROTO((void));

static void set_border __PROTO((void));
static void set_boxwidth __PROTO((void));
static void set_clabel __PROTO((void));
static void set_clip __PROTO((void));
static void set_cntrparam __PROTO((void));
static void set_contour __PROTO((void));
static void set_dgrid3d __PROTO((void));
static void set_dummy __PROTO((void));
static void set_encoding __PROTO((void));
static void set_format __PROTO((void));
static void set_grid __PROTO((void));
static void set_hidden3d __PROTO((void));
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
static void set_historysize __PROTO((void));
#endif
static void set_isosamples __PROTO((void));
static void set_key __PROTO((void));
static void set_keytitle __PROTO((void));
static void set_label __PROTO((void));
static int assign_label_tag __PROTO((void));
static void delete_label __PROTO((struct text_label * prev, struct text_label * this));
static void set_loadpath __PROTO((void));
static void set_locale __PROTO((void));
static void set_logscale __PROTO((void));
static void set_mapping __PROTO((void));
static void set_bmargin __PROTO((void));
static void set_lmargin __PROTO((void));
static void set_rmargin __PROTO((void));
static void set_tmargin __PROTO((void));
static void set_missing __PROTO((void));
#ifdef USE_MOUSE
static void set_mouse __PROTO((void));
#endif
static void set_offsets __PROTO((void));
static void set_origin __PROTO((void));
static void set_output __PROTO((void));
static void set_parametric __PROTO((void));
#ifdef PM3D
static void set_palette __PROTO((void));
static void set_pm3d __PROTO((void));
#endif
static void set_pointsize __PROTO((void));
static void set_polar __PROTO((void));
static void set_samples __PROTO((void));
static void set_size __PROTO((void));
static void set_style __PROTO((void));
static void set_surface __PROTO((void));
static void set_terminal __PROTO((void));
static void set_termoptions __PROTO((void));
static void set_tics __PROTO((void));
static void set_ticscale __PROTO((void));
static void set_ticslevel __PROTO((void));
static void set_timefmt __PROTO((void));
static void set_timestamp __PROTO((void));
static void set_view __PROTO((void));
static void set_zero __PROTO((void));
static void set_xdata __PROTO((void));
static void set_ydata __PROTO((void));
static void set_zdata __PROTO((void));
static void set_x2data __PROTO((void));
static void set_y2data __PROTO((void));
static void set_xrange __PROTO((void));
static void set_x2range __PROTO((void));
static void set_yrange __PROTO((void));
static void set_y2range __PROTO((void));
static void set_zrange __PROTO((void));
static void set_rrange __PROTO((void));
static void set_trange __PROTO((void));
static void set_urange __PROTO((void));
static void set_vrange __PROTO((void));
static void set_xzeroaxis __PROTO((void));
static void set_yzeroaxis __PROTO((void));
static void set_x2zeroaxis __PROTO((void));
static void set_y2zeroaxis __PROTO((void));
static void set_zeroaxis __PROTO((void));


/******** Local functions ********/

static void get_position __PROTO((struct position * pos));
static void get_position_type __PROTO((enum position_type * type, int *axes));
static void set_xyzlabel __PROTO((label_struct * label));
static void load_tics __PROTO((int axis, struct ticdef * tdef));
static void load_tic_user __PROTO((int axis, struct ticdef * tdef));
static void free_marklist __PROTO((struct ticmark * list));
static void load_tic_series __PROTO((int axis, struct ticdef * tdef));
static void load_offsets __PROTO((double *a, double *b, double *c, double *d));

static void set_linestyle __PROTO((void));
static int assign_linestyle_tag __PROTO((void));
static int looks_like_numeric __PROTO((char *));
static void set_lp_properties __PROTO((struct lp_style_type *, int, int, int, double, double));
static void reset_lp_properties __PROTO((struct lp_style_type *arg));

static int set_tic_prop __PROTO((int *TICS, int *MTICS, double *FREQ,
     struct ticdef * tdef, int AXIS, TBOOLEAN * ROTATE, const char *tic_side));

/* Backwards compatibility ... */
static void set_nolinestyle __PROTO((void));

/* following code segment appears over and over again */
#define GET_NUM_OR_TIME(store,axis) \
 do { \
  if (datatype[axis] == TIME && isstring(c_token) ){ \
   char ss[80]; \
   struct tm tm; \
   quote_str(ss,c_token, 80); \
   ++c_token; \
   if (gstrptime(ss,timefmt,&tm)) \
    store = (double) gtimegm(&tm); \
   else \
    store = 0; \
  } else { \
   struct value value; \
   store = real(const_express(&value)); \
  } \
 }while(0)

/******** The 'reset' command ********/
void
reset_command()
{
    register struct curve_points *f_p = first_plot;
    register struct surface_points *f_3dp = first_3dplot;

    c_token++;
    first_plot = NULL;
    first_3dplot = NULL;
    cp_free(f_p);
    sp_free(f_3dp);
    /* delete arrows */
    while (first_arrow != NULL)
	delete_arrow((struct arrow_def *) NULL, first_arrow);
    /* delete labels */
    while (first_label != NULL)
	delete_label((struct text_label *) NULL, first_label);
    /* delete linestyles */
    while (first_linestyle != NULL)
	delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
    strcpy(dummy_var[0], "x");
    strcpy(dummy_var[1], "y");
    strcpy(title.text, "");
    strcpy(xlabel.text, "");
    strcpy(ylabel.text, "");
    strcpy(zlabel.text, "");
    strcpy(x2label.text, "");
    strcpy(y2label.text, "");
    *title.font = 0;
    *xlabel.font = 0;
    *ylabel.font = 0;
    *zlabel.font = 0;
    *x2label.font = 0;
    *y2label.font = 0;
    strcpy(key_title, "");
    strcpy(timefmt, TIMEFMT);
    strcpy(xformat, DEF_FORMAT);
    strcpy(yformat, DEF_FORMAT);
    strcpy(zformat, DEF_FORMAT);
    strcpy(x2format, DEF_FORMAT);
    strcpy(y2format, DEF_FORMAT);
    format_is_numeric[FIRST_X_AXIS] = format_is_numeric[SECOND_X_AXIS] = 1;
    format_is_numeric[FIRST_Y_AXIS] = format_is_numeric[SECOND_Y_AXIS] = 1;
    format_is_numeric[FIRST_Z_AXIS] = format_is_numeric[SECOND_Z_AXIS] = 1;
    autoscale_r = DTRUE;
    autoscale_t = DTRUE;
    autoscale_u = DTRUE;
    autoscale_v = DTRUE;
    autoscale_x = DTRUE;
    autoscale_y = DTRUE;
    autoscale_z = DTRUE;
    autoscale_x2 = DTRUE;
    autoscale_y2 = DTRUE;
    autoscale_lu = DTRUE;
    autoscale_lv = DTRUE;
    autoscale_lx = DTRUE;
    autoscale_ly = DTRUE;
    autoscale_lz = DTRUE;
    boxwidth = -1.0;
    clip_points = FALSE;
    clip_lines1 = TRUE;
    clip_lines2 = FALSE;
    set_lp_properties(&border_lp, 0, -2, 0, 1.0, 1.0);
    draw_border = 31;
    draw_surface = TRUE;
    data_style = POINTSTYLE;
    func_style = LINES;
    bar_size = 1.0;
    set_lp_properties(&work_grid, 0, GRID_OFF, 0, 1.0, 1.0);
    set_lp_properties(&grid_lp, 0, -1, 0, 1.0, 1.0);
    set_lp_properties(&mgrid_lp, 0, -1, 0, 1.0, 1.0);
    polar_grid_angle = 0;
    key = -1;
    is_log_x = FALSE;
    is_log_y = FALSE;
    is_log_z = FALSE;
    is_log_x2 = FALSE;
    is_log_y2 = FALSE;
    base_log_x = 0.0;
    base_log_y = 0.0;
    base_log_z = 0.0;
    base_log_x2 = 0.0;
    base_log_y2 = 0.0;
    log_base_log_x = 0.0;
    log_base_log_y = 0.0;
    log_base_log_z = 0.0;
    log_base_log_x2 = 0.0;
    log_base_log_y2 = 0.0;
    parametric = FALSE;
    polar = FALSE;
    hidden3d = FALSE;
    label_contours = TRUE;
    strcpy(contour_format, "%8.3g");
    angles_format = ANGLES_RADIANS;
    ang2rad = 1.0;
    mapping3d = MAP3D_CARTESIAN;
    samples = SAMPLES;
    samples_1 = SAMPLES;
    samples_2 = SAMPLES;
    iso_samples_1 = ISO_SAMPLES;
    iso_samples_2 = ISO_SAMPLES;
    xsize = 1.0;
    ysize = 1.0;
    zsize = 1.0;
    xoffset = 0.0;
    yoffset = 0.0;
    aspect_ratio = 0.0;		/* dont force it */
    surface_rot_z = 30.0;
    surface_rot_x = 60.0;
    surface_scale = 1.0;
    surface_zscale = 1.0;
    *timelabel.text = 0;
    timelabel.xoffset = 0;
    timelabel.yoffset = 0;
    *timelabel.font = 0;
    timelabel_rotate = FALSE;
    timelabel_bottom = TRUE;
    title.xoffset = 0;
    title.yoffset = 0;
    xlabel.xoffset = 0;
    xlabel.yoffset = 0;
    ylabel.xoffset = 0;
    ylabel.yoffset = 0;
    zlabel.xoffset = 0;
    zlabel.yoffset = 0;
    x2label.xoffset = 0;
    x2label.yoffset = 0;
    y2label.xoffset = 0;
    y2label.yoffset = 0;
    rmin = -0.0;
    rmax = 10.0;
    tmin = -5.0;
    tmax = 5.0;
    umin = -5.0;
    umax = 5.0;
    vmin = -5.0;
    vmax = 5.0;
    xmin = -10.0;
    xmax = 10.0;
    ymin = -10.0;
    ymax = 10.0;
    zmin = -10.0;
    zmax = 10.0;
    x2min = -10.0;
    x2max = 10.0;
    y2min = -10.0;
    y2max = 10.0;
    writeback_min[FIRST_Z_AXIS] = zmin; /* ULIG */
    writeback_max[FIRST_Z_AXIS] = zmax;
    writeback_min[FIRST_Y_AXIS] = ymin;
    writeback_max[FIRST_Y_AXIS] = ymax;
    writeback_min[FIRST_X_AXIS] = xmin;
    writeback_max[FIRST_X_AXIS] = xmax;
    writeback_min[SECOND_Z_AXIS] = zmin; /* no z2min (see plot.h) */
    writeback_max[SECOND_Z_AXIS] = zmax; /* no z2max */
    writeback_min[SECOND_Y_AXIS] = y2min;
    writeback_max[SECOND_Y_AXIS] = y2max;
    writeback_min[SECOND_X_AXIS] = x2min;
    writeback_max[SECOND_X_AXIS] = x2max;
    writeback_min[T_AXIS] = tmin;
    writeback_max[T_AXIS] = tmax;
    writeback_min[R_AXIS] = rmin;
    writeback_max[R_AXIS] = rmax;
    writeback_min[U_AXIS] = umin;
    writeback_max[U_AXIS] = umax;
    writeback_min[V_AXIS] = vmin;
    writeback_max[V_AXIS] = vmax;
    memset(range_flags, 0, sizeof(range_flags));	/* all = 0 */

    loff = 0.0;
    roff = 0.0;
    toff = 0.0;
    boff = 0.0;
    draw_contour = CONTOUR_NONE;
    contour_pts = 5;
    contour_kind = CONTOUR_KIND_LINEAR;
    contour_order = 4;
    contour_levels = 5;
    zero = ZERO;
    levels_kind = LEVELS_AUTO;
    dgrid3d_row_fineness = 10;
    dgrid3d_col_fineness = 10;
    dgrid3d_norm_value = 1;
    dgrid3d = FALSE;
    set_lp_properties(&xzeroaxis, 0, -3, 0, 1.0, 1.0);
    set_lp_properties(&yzeroaxis, 0, -3, 0, 1.0, 1.0);
    set_lp_properties(&x2zeroaxis, 0, -3, 0, 1.0, 1.0);
    set_lp_properties(&y2zeroaxis, 0, -3, 0, 1.0, 1.0);
    xtics = ytics = TICS_ON_BORDER | TICS_MIRROR;
    ztics = TICS_ON_BORDER;	/* no mirror by default */
    x2tics = NO_TICS;
    y2tics = NO_TICS;
    mxtics = mytics = mztics = mx2tics = my2tics = MINI_DEFAULT;
    mxtfreq = 10.0;
    mytfreq = 10.0;
    mztfreq = 10.0;
    mx2tfreq = 10.0;
    my2tfreq = 10.0;
    ticscale = 1.0;
    miniticscale = 0.5;
    ticslevel = 0.5;
    xticdef.type = TIC_COMPUTED;
    yticdef.type = TIC_COMPUTED;
    zticdef.type = TIC_COMPUTED;
    x2ticdef.type = TIC_COMPUTED;
    y2ticdef.type = TIC_COMPUTED;
    tic_in = TRUE;
    lmargin = bmargin =	rmargin = tmargin = -1;		/* autocomputed */
    key_hpos = TRIGHT;
    key_vpos = TTOP;
    key_just = JRIGHT;
    key_reverse = FALSE;
    set_lp_properties(&key_box, 0, -3, 0, 1.0, 1.0);
    key_swidth = 4;
    key_vert_factor = 1;
    key_width_fix = 0;
    datatype[FIRST_X_AXIS] = FALSE;
    datatype[FIRST_Y_AXIS] = FALSE;
    datatype[FIRST_Z_AXIS] = FALSE;
    datatype[SECOND_X_AXIS] = FALSE;
    datatype[SECOND_Y_AXIS] = FALSE;
    datatype[SECOND_Z_AXIS] = FALSE;
    datatype[R_AXIS] = FALSE;
    datatype[T_AXIS] = FALSE;
    datatype[U_AXIS] = FALSE;
    datatype[V_AXIS] = FALSE;

    pointsize = 1.0;
    encoding = S_ENC_DEFAULT;

#ifdef PM3D
    pm3d_reset();
#endif

    init_locale();
    clear_loadpath();

}

/******** The 'set' command ********/
void
set_command()
{
    static char GPFAR setmess[] = 
    "valid set options:  [] = choose one, {} means optional\n\n\
\t'angles',  'arrow',  'autoscale',  'bars',  'border', 'boxwidth',\n\
\t'clabel', 'clip', 'cntrparam', 'contour', 'data style',  'dgrid3d',\n\
\t'dummy',  'encoding',  'format', 'function style',   'grid',\n\
\t'hidden3d',  'historysize', 'isosamples', 'key', 'label', 'linestyle',\n\
\t'locale',  'logscale', '[blrt]margin', 'mapping', 'missing', 'mouse',\n\
\t'multiplot',  'offsets', 'origin', 'output', 'palette', 'parametric',\n\
\t'pm3d', 'pointsize', 'polar', '[rtuv]range', 'samples', 'size',\n\
\t'surface', 'terminal', 'tics', 'ticscale', 'ticslevel', 'timestamp',\n\
\t'timefmt', 'title', 'view', '[xyz]{2}data', '[xyz]{2}label',\n\
\t'[xyz]{2}range', '{no}{m}[xyz]{2}tics', '[xyz]{2}[md]tics',\n\
\t'{[xyz]{2}}zeroaxis', 'zero'";

    c_token++;

#ifdef BACKWARDS_COMPATIBLE

    /* retain backwards compatibility to the old syntax for now
     * Oh, such ugliness ...
     */

    if (almost_equals(c_token,"da$ta")) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set style data\"");
	if (!almost_equals(++c_token,"s$tyle"))
            int_error(c_token,"expecting keyword 'style'");
	else
	    data_style = get_style();
    } else if (almost_equals(c_token,"fu$nction")) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set style function\"");
        if (!almost_equals(++c_token,"s$tyle"))
            int_error(c_token,"expecting keyword 'style'");
	else
	    func_style = get_style();
    } else if (almost_equals(c_token,"li$nestyle") || equals(c_token, "ls" )) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set style line\"");
        c_token++;
        set_linestyle();
    } else if (almost_equals(c_token,"noli$nestyle") || equals(c_token, "nols" )) {
        c_token++;
        set_nolinestyle();
    } else if (input_line[token[c_token].start_index] == 'n' &&
	       input_line[token[c_token].start_index+1] == 'o') {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"unset\"");
	token[c_token].start_index += 2;
	token[c_token].length -= 2;
	c_token--;
	unset_command();
    } else {

#endif /* BACKWARDS_COMPATIBLE */

	if (max_levels == 0)
	    levels_list = (double *)gp_alloc((max_levels = 5)*sizeof(double), 
					     "contour levels");

	switch(lookup_table(&set_tbl[0],c_token)) {
	case S_ANGLES:
	    set_angles();
	    break;
	case S_ARROW:
	    set_arrow();
	    break;
	case S_AUTOSCALE:
	    set_autoscale();
	    break;
	case S_BARS:
	    set_bars();
	    break;
	case S_BORDER:
	    set_border();
	    break;
	case S_BOXWIDTH:
	    set_boxwidth();
	    break;
	case S_CLABEL:
	    set_clabel();
	    break;
	case S_CLIP:
	    set_clip();
	    break;
	case S_CNTRPARAM:
	    set_cntrparam();
	    break;
	case S_CONTOUR:
	    set_contour();
	    break;
	case S_DGRID3D:
	    set_dgrid3d();
	    break;
	case S_DUMMY:
	    set_dummy();
	    break;
	case S_ENCODING:
	    set_encoding();
	    break;
	case S_FORMAT:
	    set_format();
	    break;
	case S_GRID:
	    set_grid();
	    break;
	case S_HIDDEN3D:
	    set_hidden3d();
	    break;
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
	case S_HISTORYSIZE:
	    set_historysize();
	    break;
#endif
	case S_ISOSAMPLES:
	    set_isosamples();
	    break;
	case S_KEY:
	    set_key();
	    break;
	case S_KEYTITLE:
	    set_keytitle();
	    break;
	case S_LABEL:
	    set_label();
	    break;
	case S_LOADPATH:
	    set_loadpath();
	    break;
	case S_LOCALE:
	    set_locale();
	    break;
	case S_LOGSCALE:
	    set_logscale();
	    break;
	case S_MAPPING:
	    set_mapping();
	    break;
	case S_BMARGIN:
	    set_bmargin();
	    break;
	case S_LMARGIN:
	    set_lmargin();
	    break;
	case S_RMARGIN:
	    set_rmargin();
	    break;
	case S_TMARGIN:
	    set_tmargin();
	    break;
	case S_MISSING:
	    set_missing();
	    break;
#ifdef USE_MOUSE
	case S_MOUSE:
	    set_mouse();
	    break;
#endif
	case S_MULTIPLOT:
	    term_start_multiplot();
	    break;
	case S_OFFSETS:
	    set_offsets();
	    break;
	case S_ORIGIN:
	    set_origin();
	    break;
	case S_OUTPUT:
	    set_output();
	    break;
	case S_PARAMETRIC:
	    set_parametric();
	    break;
#ifdef PM3D
    case S_PALETTE:
	set_palette();
	break;
    case S_PM3D:
	set_pm3d();
	break;
#endif
	case S_POINTSIZE:
	    set_pointsize();
	    break;
	case S_POLAR:
	    set_polar();
	    break;
	case S_SAMPLES:
	    set_samples();
	    break;
	case S_SIZE:
	    set_size();
	    break;
	case S_STYLE:
	    set_style();
	    break;
	case S_SURFACE:
	    set_surface();
	    break;
	case S_TERMINAL:
	    set_terminal();
	    break;
	case S_TERMOPTIONS:
	    set_termoptions();
	    break;
	case S_TICS:
	    set_tics();
	    break;
	case S_TICSCALE:
	    set_ticscale();
	    break;
	case S_TICSLEVEL:
	    set_ticslevel();
	    break;
	case S_TIMEFMT:
	    set_timefmt();
	    break;
	case S_TIMESTAMP:
	    set_timestamp();
	    break;
	case S_TITLE:
	    set_xyzlabel(&title);
	    break;
	case S_VIEW:
	    set_view();
	    break;
	case S_ZERO:
	    set_zero();
	    break;
/* FIXME */
	case S_MXTICS:
	case S_NOMXTICS:
	case S_XTICS:
	case S_NOXTICS:
	case S_XDTICS:
	case S_NOXDTICS:
	case S_XMTICS: 
	case S_NOXMTICS:
	    set_tic_prop(&xtics, &mxtics, &mxtfreq, &xticdef, FIRST_X_AXIS,
			 &rotate_xtics, "x");
	    break;
	case S_MYTICS:
	case S_NOMYTICS:
	case S_YTICS:
	case S_NOYTICS:
	case S_YDTICS:
	case S_NOYDTICS:
	case S_YMTICS: 
	case S_NOYMTICS:
	    set_tic_prop(&ytics, &mytics, &mytfreq, &yticdef, FIRST_Y_AXIS,
			 &rotate_ytics, "y");
	    break;
	case S_MX2TICS:
	case S_NOMX2TICS:
	case S_X2TICS:
	case S_NOX2TICS:
	case S_X2DTICS:
	case S_NOX2DTICS:
	case S_X2MTICS: 
	case S_NOX2MTICS:
	    set_tic_prop(&x2tics, &mx2tics, &mx2tfreq, &x2ticdef, SECOND_X_AXIS,
			 &rotate_x2tics, "x2");
	    break;
	case S_MY2TICS:
	case S_NOMY2TICS:
	case S_Y2TICS:
	case S_NOY2TICS:
	case S_Y2DTICS:
	case S_NOY2DTICS:
	case S_Y2MTICS: 
	case S_NOY2MTICS:
	    set_tic_prop(&y2tics, &my2tics, &my2tfreq, &y2ticdef, SECOND_Y_AXIS,
			 &rotate_y2tics, "y2");
	    break;
	case S_MZTICS:
	case S_NOMZTICS:
	case S_ZTICS:
	case S_NOZTICS:
	case S_ZDTICS:
	case S_NOZDTICS:
	case S_ZMTICS: 
	case S_NOZMTICS:
	    set_tic_prop(&ztics, &mztics, &mztfreq, &zticdef, FIRST_Z_AXIS,
			 &rotate_ztics, "z");
	    break;
	case S_XDATA:
	    set_xdata();
	    break;
	case S_YDATA:
	    set_ydata();
	    break;
	case S_ZDATA:
	    set_zdata();
	    break;
	case S_X2DATA:
	    set_x2data();
	    break;
	case S_Y2DATA:
	    set_y2data();
	    break;
	case S_XLABEL:
	    set_xyzlabel(&xlabel);
	    break;
	case S_YLABEL:
	    set_xyzlabel(&ylabel);
	    break;
	case S_ZLABEL:
	    set_xyzlabel(&zlabel);
	    break;
	case S_X2LABEL:
	    set_xyzlabel(&x2label);
	    break;
	case S_Y2LABEL:
	    set_xyzlabel(&y2label);
	    break;
	case S_XRANGE:
	    set_xrange();
	    break;
	case S_X2RANGE:
	    set_x2range();
	    break;
	case S_YRANGE:
	    set_yrange();
	    break;
	case S_Y2RANGE:
	    set_y2range();
	    break;
	case S_ZRANGE:
	    set_zrange();
	    break;
	case S_RRANGE:
	    set_rrange();
	    break;
	case S_TRANGE:
	    set_trange();
	    break;
	case S_URANGE:
	    set_urange();
	    break;
	case S_VRANGE:
	    set_vrange();
	    break;
	case S_XZEROAXIS:
	    set_xzeroaxis();
	    break;
	case S_YZEROAXIS:
	    set_yzeroaxis();
	    break;
	case S_X2ZEROAXIS:
	    set_x2zeroaxis();
	    break;
	case S_Y2ZEROAXIS:
	    set_y2zeroaxis();
	    break;
	case S_ZEROAXIS:
	    set_zeroaxis();
	    break;
	default:
	    int_error(c_token, setmess);
	    break;
	}

#ifdef BACKWARDS_COMPATIBLE
    }
#endif

}


/* process 'set angles' command */
static void
set_angles()
{
    c_token++;
    if (END_OF_COMMAND) {
	/* assuming same as defaults */
	angles_format = ANGLES_RADIANS;
	ang2rad = 1;
    } else if (almost_equals(c_token, "r$adians")) {
	angles_format = ANGLES_RADIANS;
	c_token++;
	ang2rad = 1;
    } else if (almost_equals(c_token, "d$egrees")) {
	angles_format = ANGLES_DEGREES;
	c_token++;
	ang2rad = DEG2RAD;
    } else
	int_error(c_token, "expecting 'radians' or 'degrees'");

    if (polar && autoscale_t) {
	/* set trange if in polar mode and no explicit range */
	tmin = 0;
	tmax = 2 * M_PI / ang2rad;
    }
}


/* process a 'set arrow' command */
/* set arrow {tag} {from x,y} {to x,y} {{no}head} */
static void
set_arrow()
{
    struct value a;
    struct arrow_def *this_arrow = NULL;
    struct arrow_def *new_arrow = NULL;
    struct arrow_def *prev_arrow = NULL;
    struct position spos, epos, headsize;
    struct lp_style_type loc_lp;
    int tag;
    TBOOLEAN set_start, set_end, head = 1;
    TBOOLEAN set_line = 0, set_headsize = 0, set_layer = 0;
    TBOOLEAN relative = 0;
    int layer = 0;
    headsize.x = 0; /* length being zero means the default head size */

    c_token++;

    /* Init struct lp_style_type loc_lp */
    reset_lp_properties (&loc_lp);

    /* get tag */
    if (!END_OF_COMMAND
	&& !equals(c_token, "from")
	&& !equals(c_token, "to")
	&& !equals(c_token, "rto")
	) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_arrow_tag();	/* default next tag */

    /* HBB 20001018: removed code here that accepted 'first' or
     * 'second' keywords. The resulting variables 'axes' and
     * 'set_axes' effected nothing, anyway --> deleted them, too. */

    /* get start position */
    if (!END_OF_COMMAND && equals(c_token, "from")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "start coordinates expected");
	/* get coordinates */
	get_position(&spos);
	set_start = TRUE;
    } else {
	spos.x = spos.y = spos.z = 0;
	spos.scalex = spos.scaley = spos.scalez = first_axes;
	set_start = FALSE;
    }

    /* get end position */
    if (!END_OF_COMMAND && equals(c_token, "to")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "end coordinates expected");
	/* get coordinates */
	get_position(&epos);
	set_end = TRUE;
    } else if (!END_OF_COMMAND && equals(c_token, "rto")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "end coordinates expected");
	/* get coordinates */
	get_position(&epos);
	set_end = TRUE;
	relative = 1;
    } else {
	epos.x = epos.y = epos.z = 0;
	epos.scalex = epos.scaley = epos.scalez = first_axes;
	set_end = FALSE;
    }

    /* get start position - what the heck, either order is ok */
    if (!END_OF_COMMAND && equals(c_token, "from")) {
	if (set_start)
	    int_error(c_token, "only one 'from' is allowed");
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "start coordinates expected");
	/* get coordinates */
	get_position(&spos);
	set_start = TRUE;
    }

    if (!END_OF_COMMAND && equals(c_token, "nohead")) {
	c_token++;
	head = 0;
    }
    if (!END_OF_COMMAND && equals(c_token, "head")) {
	c_token++;
	head = 1;
    }
    if (!END_OF_COMMAND && equals(c_token, "heads")) {
	c_token++;
	head = 2;
    }
    if (!END_OF_COMMAND && equals(c_token, "size")) {
	headsize.scalex = headsize.scaley = headsize.scalez = first_axes;
	  /* only scalex used; scaley is angle of the head in [deg] */
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "head size expected");
	get_position(&headsize);
	set_headsize = TRUE;
    }
    set_layer = FALSE;
    if (!END_OF_COMMAND && equals(c_token, "back")) {
	c_token++;
	layer = 0;
	set_layer = TRUE;
    }
    if (!END_OF_COMMAND && equals(c_token, "front")) {
	c_token++;
	layer = 1;
	set_layer = TRUE;
    }
    set_line = 1;

    /* pick up a line spec - allow ls, but no point. */
    lp_parse(&loc_lp, 1, 0, 0, 0);
    loc_lp.pointflag = 0;	/* standard value for arrows, don't use points */

    if (!END_OF_COMMAND)
	int_error(c_token, "extraneous or out-of-order arguments in set arrow");

    /* OK! add arrow */
    if (first_arrow != NULL) {	/* skip to last arrow */
	for (this_arrow = first_arrow; this_arrow != NULL;
	     prev_arrow = this_arrow, this_arrow = this_arrow->next)
	    /* is this the arrow we want? */
	    if (tag <= this_arrow->tag)
		break;
    }
    if (this_arrow != NULL && tag == this_arrow->tag) {
	/* changing the arrow */
	if (set_start) {
	    this_arrow->start = spos;
	}
	if (set_end) {
	    this_arrow->end = epos;
	    this_arrow->relative = relative;
	}
	this_arrow->head = head;
	if (set_layer) {
	    this_arrow->layer = layer;
	}
	if (set_headsize) {
	    this_arrow->headsize = headsize;
	}
	if (set_line) {
	    this_arrow->lp_properties = loc_lp;
	}
    } else {	/* adding the arrow */
	new_arrow = (struct arrow_def *)
	    gp_alloc(sizeof(struct arrow_def), "arrow");
	if (prev_arrow != NULL)
	    prev_arrow->next = new_arrow;	/* add it to end of list */
	else
	    first_arrow = new_arrow;	/* make it start of list */
	new_arrow->tag = tag;
	new_arrow->next = this_arrow;
	new_arrow->start = spos;
	new_arrow->end = epos;
	new_arrow->head = head;
	new_arrow->headsize = headsize;
	new_arrow->layer = layer;
	new_arrow->relative = relative;
	new_arrow->lp_properties = loc_lp;
    }
}


/* assign a new arrow tag
 * arrows are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_arrow_tag()
{
    struct arrow_def *this_arrow;
    int last = 0;		/* previous tag value */

    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next)
	if (this_arrow->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}


/* delete arrow from linked list started by first_arrow.
 * called with pointers to the previous arrow (prev) and the 
 * arrow to delete (this).
 * If there is no previous arrow (the arrow to delete is
 * first_arrow) then call with prev = NULL.
 */
static void
delete_arrow(prev, this)
struct arrow_def *prev, *this;
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous arrow */
	    prev->next = this->next;
	else			/* this = first_arrow so change first_arrow */
	    first_arrow = this->next;
	free((char *) this);
    }
}


/* save on replication with a macro */
#define PROCESS_AUTO_LETTER(AUTO, STRING,MIN,MAX) \
 else if (equals(c_token, STRING))       { AUTO = DTRUE; ++c_token; } \
 else if (almost_equals(c_token, MIN)) { AUTO |= 1;    ++c_token; } \
 else if (almost_equals(c_token, MAX)) { AUTO |= 2;    ++c_token; }

/* process 'set autoscale' command */
static void
set_autoscale()
{
    c_token++;
    if (END_OF_COMMAND) {
	autoscale_r = autoscale_t = autoscale_x = autoscale_y = autoscale_z =
	    autoscale_x2 = autoscale_y2 = DTRUE;
    } else if (equals(c_token, "xy") || equals(c_token, "yx")) {
	autoscale_x = autoscale_y = DTRUE;
	c_token++;
    }
    PROCESS_AUTO_LETTER(autoscale_r, "r", "rmi$n", "rma$x")
    PROCESS_AUTO_LETTER(autoscale_t, "t", "tmi$n", "tma$x")
    PROCESS_AUTO_LETTER(autoscale_u, "u", "umi$n", "uma$x")
    PROCESS_AUTO_LETTER(autoscale_v, "v", "vmi$n", "vma$x")
    PROCESS_AUTO_LETTER(autoscale_x, "x", "xmi$n", "xma$x")
    PROCESS_AUTO_LETTER(autoscale_y, "y", "ymi$n", "yma$x")
    PROCESS_AUTO_LETTER(autoscale_z, "z", "zmi$n", "zma$x")
    PROCESS_AUTO_LETTER(autoscale_x2, "x2", "x2mi$n", "x2ma$x")
    PROCESS_AUTO_LETTER(autoscale_y2, "y2", "y2mi$n", "y2ma$x")
    else
	int_error(c_token, "Invalid range");
}


/* process 'set bars' command */
static void
set_bars()
{
    struct value a;

    c_token++;
    if(END_OF_COMMAND) {
	bar_size = 1.0;
    } else if(almost_equals(c_token,"s$mall")) {
	bar_size = 0.0;
	++c_token;
    } else if(almost_equals(c_token,"l$arge")) {
	bar_size = 1.0;
	++c_token;
    } else {
	bar_size = real(const_express(&a));
    }
}


/* process 'set border' command */
static void
set_border()
{
    struct value a;

    c_token++;
    if(END_OF_COMMAND){
	draw_border = 31;
    } else {
	draw_border = (int)real(const_express(&a));
    }
    /* HBB 980609: add linestyle handling for 'set border...' */
    /* For now, you have to give a border bitpattern to be able to specify
     * a linestyle. Sorry for this, but the gnuplot parser really is too messy
     * for any other solution, currently */
    /* not for long, I hope. Lars */
    if(END_OF_COMMAND) {
	set_lp_properties(&border_lp, 0, -2, 0, 1.0, 1.0);
    } else {
	lp_parse(&border_lp, 1, 0, -2, 0);
    }
}


/* process 'set boxwidth' command */
static void
set_boxwidth()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND)
	boxwidth = -1.0;
    else {
	/* if((boxwidth = real(const_express(&a))) != -2.0) */
	/* boxwidth = magnitude(const_express(&a));*/
	boxwidth = real(const_express(&a));
    }
}


/* process 'set clabel' command */
static void
set_clabel()
{
    c_token++;
    label_contours = TRUE;
    if (isstring(c_token))
	quote_str(contour_format, c_token++, 30);
}


/* process 'set clip' command */
static void
set_clip()
{
    c_token++;
    if (END_OF_COMMAND)
	/* assuming same as points */
	clip_points = TRUE;
    else if (almost_equals(c_token, "p$oints"))
	clip_points = TRUE;
    else if (almost_equals(c_token, "o$ne"))
	clip_lines1 = TRUE;
    else if (almost_equals(c_token, "t$wo"))
	clip_lines2 = TRUE;
    else
	int_error(c_token, "expecting 'points', 'one', or 'two'");
    c_token++;
}


/* process 'set cntrparam' command */
static void
set_cntrparam()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	/* assuming same as defaults */
	contour_pts = 5;
	contour_kind = CONTOUR_KIND_LINEAR;
	contour_order = 4;
	contour_levels = 5;
	levels_kind = LEVELS_AUTO;
    } else if (almost_equals(c_token, "p$oints")) {
	c_token++;
	contour_pts = (int) real(const_express(&a));
    } else if (almost_equals(c_token, "li$near")) {
	c_token++;
	contour_kind = CONTOUR_KIND_LINEAR;
    } else if (almost_equals(c_token, "c$ubicspline")) {
	c_token++;
	contour_kind = CONTOUR_KIND_CUBIC_SPL;
    } else if (almost_equals(c_token, "b$spline")) {
	c_token++;
	contour_kind = CONTOUR_KIND_BSPLINE;
    } else if (almost_equals(c_token, "le$vels")) {
	int i = 0;  /* local counter */
	c_token++;
	/*  RKC: I have modified the next two:
	 *   to use commas to separate list elements as in xtics
	 *   so that incremental lists start,incr[,end]as in "
	 */
	if (almost_equals(c_token, "di$screte")) {
	    levels_kind = LEVELS_DISCRETE;
	    c_token++;
	    if(END_OF_COMMAND)
		int_error(c_token, "expecting discrete level");
	    else
		levels_list[i++] = real(const_express(&a));

	    while(!END_OF_COMMAND) {
		if (!equals(c_token, ","))
		    int_error(c_token,
			      "expecting comma to separate discrete levels");
		c_token++;
		if (i == max_levels)
		    levels_list = 
			gp_realloc(levels_list,
				   (max_levels += 10)*sizeof(double),
				   "contour levels");
		levels_list[i++] = real(const_express(&a));
	    }
	    contour_levels = i;
	} else if (almost_equals(c_token, "in$cremental")) {
	    levels_kind = LEVELS_INCREMENTAL;
	    c_token++;
	    levels_list[i++] = real(const_express(&a));
	    if (!equals(c_token, ","))
		int_error(c_token,
			  "expecting comma to separate start,incr levels");
	    c_token++;
	    if((levels_list[i++] = real(const_express(&a))) == 0)
		int_error(c_token, "increment cannot be 0");
	    if(!END_OF_COMMAND) {
		if (!equals(c_token, ","))
		    int_error(c_token,
			      "expecting comma to separate incr,stop levels");
		c_token++;
		/* need to round up, since 10,10,50 is 5 levels, not four,
		 * but 10,10,49 is four
		 */
		contour_levels = (int) ( (real(const_express(&a))-levels_list[0])/levels_list[1] + 1.0);
	    }
	} else if (almost_equals(c_token, "au$to")) {
	    levels_kind = LEVELS_AUTO;
	    c_token++;
	    if(!END_OF_COMMAND)
		contour_levels = (int) real(const_express(&a));
	} else {
	    if(levels_kind == LEVELS_DISCRETE)
		int_error(c_token, "Levels type is discrete, ignoring new number of contour levels");
	    contour_levels = (int) real(const_express(&a));
	}
    } else if (almost_equals(c_token, "o$rder")) {
	int order;
	c_token++;
	order = (int) real(const_express(&a));
	if ( order < 2 || order > 10 )
	    int_error(c_token, "bspline order must be in [2..10] range.");
	contour_order = order;
    } else
	int_error(c_token, "expecting 'linear', 'cubicspline', 'bspline', 'points', 'levels' or 'order'");
}


/* process 'set contour' command */
static void
set_contour()
{
    c_token++;
    if (END_OF_COMMAND)
	/* assuming same as points */
	draw_contour = CONTOUR_BASE;
    else {
	if (almost_equals(c_token, "ba$se"))
	    draw_contour = CONTOUR_BASE;
	else if (almost_equals(c_token, "s$urface"))
	    draw_contour = CONTOUR_SRF;
	else if (almost_equals(c_token, "bo$th"))
	    draw_contour = CONTOUR_BOTH;
	else
	    int_error(c_token, "expecting 'base', 'surface', or 'both'");
	c_token++;
    }
}


/* process 'set dgrid3d' command */
static void
set_dgrid3d()
{
    struct value a;
    int local_vals[3];
    int i;
    TBOOLEAN was_comma = TRUE;

    c_token++;
    local_vals[0] = dgrid3d_row_fineness;
    local_vals[1] = dgrid3d_col_fineness;
    local_vals[2] = dgrid3d_norm_value;

    for (i = 0; i < 3 && !(END_OF_COMMAND);) {
	if (equals(c_token,",")) {
	    if (was_comma) i++;
	    was_comma = TRUE;
	    c_token++;
	} else {
	    if (!was_comma)
		int_error(c_token, "',' expected");
	    local_vals[i] = real(const_express(&a));
	    i++;
	    was_comma = FALSE;
	}
    }

    if (local_vals[0] < 2 || local_vals[0] > 1000)
	int_error(c_token, "Row size must be in [2:1000] range; size unchanged");
    if (local_vals[1] < 2 || local_vals[1] > 1000)
	int_error(c_token, "Col size must be in [2:1000] range; size unchanged");
    if (local_vals[2] < 1 || local_vals[2] > 100)
	int_error(c_token, "Norm must be in [1:100] range; norm unchanged");

    dgrid3d_row_fineness = local_vals[0];
    dgrid3d_col_fineness = local_vals[1];
    dgrid3d_norm_value = local_vals[2];
    dgrid3d = TRUE;
}


/* process 'set dummy' command */
static void
set_dummy()
{
    c_token++;
    if (END_OF_COMMAND)
	int_error(c_token, "expecting dummy variable name");
    else {
	if (!equals(c_token,","))
	    copy_str(dummy_var[0],c_token++, MAX_ID_LEN);
	if (!END_OF_COMMAND && equals(c_token,",")) {
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expecting second dummy variable name");
	    copy_str(dummy_var[1],c_token++, MAX_ID_LEN);
	}
    }
}


/* process 'set encoding' command */
static void
set_encoding()
{
    int temp;

    c_token++;

    if(END_OF_COMMAND)
	temp = S_ENC_DEFAULT;
    else {
	temp = lookup_table(&set_encoding_tbl[0],c_token);

	if (temp == S_ENC_INVALID)
	    int_error(c_token, "expecting one of 'default', 'iso_8859_1', 'cp437' or 'cp850'");
	c_token++;
    }
    encoding = temp;
}


/* process 'set format' command */
static void
set_format()
{
    TBOOLEAN setx = FALSE, sety = FALSE, setz = FALSE;
    TBOOLEAN setx2 = FALSE, sety2 = FALSE;

    c_token++;
    if (equals(c_token,"x")) {
	setx = TRUE;
	c_token++;
    } else if (equals(c_token,"y")) {
	sety = TRUE;
	c_token++;
    } else if (equals(c_token,"x2")) {
	setx2 = TRUE;
	c_token++;
    } else if (equals(c_token,"y2")) {
	sety2 = TRUE;
	c_token++;
    } else if (equals(c_token,"z")) {
	setz = TRUE;
	c_token++;
    } else if (equals(c_token,"xy") || equals(c_token,"yx")) {
	setx = sety = TRUE;
	c_token++;
    } else if (isstring(c_token) || END_OF_COMMAND) {
	/* Assume he wants all */
	setx = sety = setz = setx2 = sety2 = TRUE;
    }

    if (END_OF_COMMAND) {
	if (setx) {
	    (void) strcpy(xformat,DEF_FORMAT);
	    format_is_numeric[FIRST_X_AXIS] = 1;
	}
	if (sety) {
	    (void) strcpy(yformat,DEF_FORMAT);
	    format_is_numeric[FIRST_Y_AXIS] = 1;
	}
	if (setz) {
	    (void) strcpy(zformat,DEF_FORMAT);
	    format_is_numeric[FIRST_Z_AXIS] = 1;
	}
	if (setx2) {
	    (void) strcpy(x2format,DEF_FORMAT);
	    format_is_numeric[SECOND_X_AXIS] = 1;
	}
	if (sety2) {
	    (void) strcpy(y2format,DEF_FORMAT);
	    format_is_numeric[SECOND_Y_AXIS] = 1;
	}
    } else {
	if (!isstring(c_token))
	    int_error(c_token, "expecting format string");
	else {
	    if (setx) {
		quote_str(xformat,c_token, MAX_ID_LEN);
		format_is_numeric[FIRST_X_AXIS] = looks_like_numeric(xformat);
	    }
	    if (sety) {
		quote_str(yformat,c_token, MAX_ID_LEN);
		format_is_numeric[FIRST_Y_AXIS] = looks_like_numeric(yformat);
	    }
	    if (setz) {
		quote_str(zformat,c_token, MAX_ID_LEN);
		format_is_numeric[FIRST_Z_AXIS] =looks_like_numeric(zformat);
	    }
	    if (setx2) {
		quote_str(x2format,c_token, MAX_ID_LEN);
		format_is_numeric[SECOND_X_AXIS] = looks_like_numeric(x2format);
	    }
	    if (sety2) {
		quote_str(y2format,c_token, MAX_ID_LEN);
		format_is_numeric[SECOND_Y_AXIS] = looks_like_numeric(y2format);
	    }
	    c_token++;
	}
    }
}


/* process 'set grid' command */

#define GRID_MATCH(string, neg, mask) \
 if (almost_equals(c_token, string)) { \
  work_grid.l_type |= mask; \
  ++c_token; \
 } else if (almost_equals(c_token, neg)) { \
  work_grid.l_type &= ~(mask); \
  ++c_token; \
 }

static void
set_grid()
{
    c_token++;
    if (END_OF_COMMAND && !work_grid.l_type)
	work_grid.l_type = GRID_X|GRID_Y;
    else
	while (!END_OF_COMMAND) {
	    GRID_MATCH("x$tics", "nox$tics", GRID_X)
	    else GRID_MATCH("y$tics", "noy$tics", GRID_Y)
	    else GRID_MATCH("z$tics", "noz$tics", GRID_Z)
	    else GRID_MATCH("x2$tics", "nox2$tics", GRID_X2)
	    else GRID_MATCH("y2$tics", "noy2$tics", GRID_Y2)
	    else GRID_MATCH("mx$tics", "nomx$tics", GRID_MX)
	    else GRID_MATCH("my$tics", "nomy$tics", GRID_MY)
	    else GRID_MATCH("mz$tics", "nomz$tics", GRID_MZ)
	    else GRID_MATCH("mx2$tics", "nomx2$tics", GRID_MX2)
	    else GRID_MATCH("my2$tics", "nomy2$tics", GRID_MY2)
	    else if (almost_equals(c_token,"po$lar")) {
		if (!work_grid.l_type)
		    work_grid.l_type = GRID_X;
		c_token++;
		if (END_OF_COMMAND) {
		    polar_grid_angle = 30*DEG2RAD;
		} else {
		    /* get radial interval */
		    struct value a;
		    polar_grid_angle = ang2rad*real(const_express(&a));
		}
	    } else if (almost_equals(c_token,"nopo$lar")) {
		polar_grid_angle = 0; /* not polar grid */
		c_token++;
	    } else
		break; /* might be a linetype */
	}

    if (!END_OF_COMMAND) {
	struct value a;
	int old_token = c_token;

	lp_parse(&grid_lp,1,0,-1,1);
	if (c_token == old_token) { /* nothing parseable found... */
	    grid_lp.l_type = real(const_express(&a)) - 1;
	}
			
	if (!work_grid.l_type)
	    work_grid.l_type = GRID_X|GRID_Y;
	/* probably just  set grid <linetype> */

	if (END_OF_COMMAND) {
	    memcpy(&mgrid_lp,&grid_lp,sizeof(struct lp_style_type));
	} else {
	    if (equals(c_token,",")) 
		c_token++;
	    old_token = c_token;
	    lp_parse(&mgrid_lp,1,0,-1,1);
	    if (c_token == old_token) {
		mgrid_lp.l_type = real(const_express(&a)) -1;
	    }
	}

	if (!work_grid.l_type)
	    work_grid.l_type = GRID_X|GRID_Y;
	/* probably just  set grid <linetype> */
    }
}


/* process 'set hidden3d' command */
static void
set_hidden3d()
{
    c_token++;
#ifdef LITE
    printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
    /* HBB 970618: new parsing engine for hidden3d options */
    set_hidden3doptions();
    hidden3d = TRUE;
#endif
}


#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
/* process 'set historysize' command */
static void
set_historysize()
{
    struct value a;

    c_token++;

    gnuplot_history_size = (int) real(const_express(&a));
    if (gnuplot_history_size < 0) {
	gnuplot_history_size = 0;
    }
}
#endif


/* process 'set isosamples' command */
static void
set_isosamples()
{
    register int tsamp1, tsamp2;
    struct value a;

    c_token++;
    tsamp1 = (int)magnitude(const_express(&a));
    tsamp2 = tsamp1;
    if (!END_OF_COMMAND) {
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	tsamp2 = (int)magnitude(const_express(&a));
    }
    if (tsamp1 < 2 || tsamp2 < 2)
	int_error(c_token, "sampling rate must be > 1; sampling unchanged");
    else {
	register struct curve_points *f_p = first_plot;
	register struct surface_points *f_3dp = first_3dplot;

	first_plot = NULL;
	first_3dplot = NULL;
	cp_free(f_p);
	sp_free(f_3dp);

	iso_samples_1 = tsamp1;
	iso_samples_2 = tsamp2;
    }
}


/* FIXME - old bug in this parser
 * to exercise: set key left hiho
 */

/* process 'set key' command */
static void
set_key()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	key = -1;
	key_vpos = TTOP;
	key_hpos = TRIGHT;
	key_just = JRIGHT;
	key_reverse = FALSE;
	set_lp_properties(&key_box,0,-3,0,1.0,1.0);
	key_swidth = 4;
	key_vert_factor = 1;
	key_width_fix = 0;
	key_title[0] = 0;
    } else {
	while (!END_OF_COMMAND) {
	    switch(lookup_table(&set_key_tbl[0],c_token)) {
	    case S_KEY_TOP:
		key_vpos = TTOP;
		key = -1;
		break;
	    case S_KEY_BOTTOM:
		key_vpos = TBOTTOM;
		key = -1;
		break;
	    case S_KEY_LEFT:
		key_hpos = TLEFT;
		/* key_just = TRIGHT; */
		key = -1;
		break;
	    case S_KEY_RIGHT:
		key_hpos = TRIGHT;
		key = -1;
		break;
	    case S_KEY_UNDER:
		key_vpos = TUNDER;
		if (key_hpos == TOUT)
		    key_hpos--;
		key = -1;
		break;
	    case S_KEY_OUTSIDE:
		key_hpos = TOUT;
		if (key_vpos == TUNDER)
		    key_vpos--;
		key = -1;
		break;
	    case S_KEY_LLEFT:
		/* key_hpos = TLEFT; */
		key_just = JLEFT;
		/* key = -1; */
		break;
	    case S_KEY_RRIGHT:
		/* key_hpos = TLEFT; */
		key_just = JRIGHT;
		/* key = -1; */
		break;
	    case S_KEY_REVERSE:
		key_reverse = TRUE;
		break;
	    case S_KEY_NOREVERSE:
		key_reverse = FALSE;
		break;
	    case S_KEY_BOX:
		c_token++;
		if (END_OF_COMMAND)
		    key_box.l_type = -2;
		else {
		    int old_token = c_token;

		    lp_parse(&key_box,1,0,-2,0);
		    if (old_token == c_token)
			key_box.l_type = real(const_express(&a)) -1;
		}
		c_token--;  /* is incremented after loop */
		break;
	    case S_KEY_NOBOX:
		key_box.l_type = -3;
		break;
	    case S_KEY_SAMPLEN:
		c_token++;
		key_swidth = real(const_express(&a));
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_SPACING:
		c_token++;
		key_vert_factor = real(const_express(&a));
		if (key_vert_factor < 0.0)
		    key_vert_factor = 0.0;
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_WIDTH:
		c_token++;
		key_width_fix = real(const_express(&a));
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_TITLE:
		if (isstring(c_token+1)) {
		    /* We have string specified - grab it. */
		    quote_str(key_title,++c_token, MAX_LINE_LEN);
		}
		else
		    key_title[0] = 0;
		break;
	    case S_KEY_INVALID:
	    default:
		get_position(&key_user_pos);
		key = 1;
		c_token--;  /* will be incremented again soon */
		break;
	    }
	    c_token++;
	}
    }
}


/* process 'set keytitle' command */
static void
set_keytitle()
{
    c_token++;
    if (END_OF_COMMAND) {	/* set to default */
	key_title[0] = NUL;
    } else {
	if (isstring(c_token)) {
	    /* We have string specified - grab it. */
	    quote_str(key_title,c_token, MAX_LINE_LEN);
	    c_token++;
	}
	/* c_token++; */
    }
}

/* process 'set label' command */
/* set label {tag} {label_text} {at x,y} {pos} {font name,size} */
/* Entry font added by DJL */
static void
set_label()
{
    set_label_tag(); /* discard return value */
}

int
set_label_tag()
{
    struct value a;
    struct text_label *this_label = NULL;
    struct text_label *new_label = NULL;
    struct text_label *prev_label = NULL;
    struct position pos;
    char *text = NULL, *font = NULL;
    enum JUSTIFY just = LEFT;
    int rotate = 0;
    int tag;
    TBOOLEAN set_text, set_position, set_just = FALSE, set_rot = FALSE,
     set_font, set_offset;
    TBOOLEAN set_layer = FALSE;
    int layer = 0;
    int pointstyle = -2; /* untouched (this is /not/ -1) */
    float hoff = 1.0;
    float voff = 1.0;

    c_token++;
    /* get tag */
    if (!END_OF_COMMAND
	&& !isstring(c_token)
	&& !equals(c_token, "at")
	&& !equals(c_token, "left")
	&& !equals(c_token, "center")
	&& !equals(c_token, "centre")
	&& !equals(c_token, "right")
	&& !equals(c_token, "front")
	&& !equals(c_token, "back")
	&& !almost_equals(c_token, "rot$ate")
	&& !almost_equals(c_token, "norot$ate")
	&& !equals(c_token, "font")) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_label_tag();	/* default next tag */

    /* get text */
    if (!END_OF_COMMAND && isstring(c_token)) {
	/* get text */
	text = gp_alloc (token_len(c_token), "text_label->text");
	quote_str(text, c_token, token_len(c_token));
	c_token++;
	set_text = TRUE;
    } else
	set_text = FALSE; /* default no text */

    /* get justification - what the heck, let him put it here */
    if (!END_OF_COMMAND && !equals(c_token, "at") && !equals(c_token, "font")
	&& !almost_equals(c_token, "rot$ate") && !almost_equals(c_token, "norot$ate")
	&& !equals(c_token, "front") && !equals(c_token, "back")) {
	if (almost_equals(c_token, "l$eft")) {
	    just = LEFT;
	} else if (almost_equals(c_token, "c$entre")
		   || almost_equals(c_token, "c$enter")) {
	    just = CENTRE;
	} else if (almost_equals(c_token, "r$ight")) {
	    just = RIGHT;
	} else
	    int_error(c_token, "bad syntax in set label");
	c_token++;
	set_just = TRUE;
    }
    /* get position */
    if (!END_OF_COMMAND && equals(c_token, "at")) {
	c_token++;

	get_position(&pos);
	set_position = TRUE;
    } else {
	pos.x = pos.y = pos.z = 0;
	pos.scalex = pos.scaley = pos.scalez = first_axes;
	set_position = FALSE;
    }

    /* get justification */
    if (!END_OF_COMMAND
	&& !almost_equals(c_token, "rot$ate") && !almost_equals(c_token, "norot$ate")
	&& !equals(c_token, "front") && !equals(c_token, "back")
	&& !equals(c_token, "font")
	&& !almost_equals(c_token, "po$intstyle")
	&& !almost_equals(c_token, "nopo$intstyle")) {
	if (set_just)
	    int_error(c_token, "only one justification is allowed");
	if (almost_equals(c_token, "l$eft")) {
	    just = LEFT;
	} else if (almost_equals(c_token, "c$entre")
		   || almost_equals(c_token, "c$enter")) {
	    just = CENTRE;
	} else if (almost_equals(c_token, "r$ight")) {
	    just = RIGHT;
	} else
	    int_error(c_token, "bad syntax in set label");

	c_token++;
	set_just = TRUE;
    }
    /* get rotation (added by RCC) */
    if (!END_OF_COMMAND && !equals(c_token, "font")
	&& !equals(c_token, "front") && !equals(c_token, "back")
	&& !almost_equals(c_token, "po$intstyle")
	&& !almost_equals(c_token, "nopo$intstyle")) {
	if (almost_equals(c_token, "rot$ate")) {
	    rotate = TRUE;
	} else if (almost_equals(c_token, "norot$ate")) {
	    rotate = FALSE;
	} else
	    int_error(c_token, "bad syntax in set label");

	c_token++;
	set_rot = TRUE;
    }
    /* get font */
    set_font = FALSE;
    if (!END_OF_COMMAND && equals(c_token, "font") &&
	!equals(c_token, "front") && !equals(c_token, "back")
	&& !almost_equals(c_token, "po$intstyle")
	&& !almost_equals(c_token, "nopo$intstyle")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "font name and size expected");
	if (isstring(c_token)) {
	    font = gp_alloc (token_len(c_token), "text_label->font");
	    quote_str(font, c_token, token_len(c_token));
	    /* get 'name,size', no further check */
	    set_font = TRUE;
	} else
	    int_error(c_token, "'fontname,fontsize' expected");

	c_token++;
    }				/* Entry font added by DJL */
    /* get front/back (added by JDP) */
    set_layer = FALSE;
    if (!END_OF_COMMAND
	&& equals(c_token, "back") && !equals(c_token, "front")
	&& !almost_equals(c_token, "po$intstyle")
	&& !almost_equals(c_token, "nopo$intstyle")) {
	layer = 0;
	c_token++;
	set_layer = TRUE;
    }
    if(!END_OF_COMMAND && equals(c_token, "front")
	&& !almost_equals(c_token, "po$intstyle")
	&& !almost_equals(c_token, "nopo$intstyle")) {
	if (set_layer)
	    int_error(c_token, "only one of front or back expected");
	layer = 1;
	c_token++;
	set_layer = TRUE;
    }

    if(!END_OF_COMMAND && almost_equals(c_token, "po$intstyle")) {
	c_token++;
	pointstyle = (int) real(const_express(&a));
	if (pointstyle < 0)
	    pointstyle = -1; /* UNSET */
	else
#define POINT_TYPES 6 /* compare with term.c */
	    pointstyle %= POINT_TYPES;
    }

    if(!END_OF_COMMAND && almost_equals(c_token, "nopo$intstyle")) {
	pointstyle = -1; /* UNSET */
	c_token++;
    }

    if(!END_OF_COMMAND && almost_equals(c_token, "of$fset")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "Expected horizontal offset");
	hoff = real(const_express(&a));

	/* c_token++; */
	if (!equals(c_token, ","))
	    int_error(c_token, "Expected comma");

	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "Expected vertical offset");
	voff = real(const_express(&a));
	set_offset = TRUE;
    }

    if (!END_OF_COMMAND)
	int_error(c_token, "extraenous or out-of-order arguments in set label");

    /* OK! add label */
    if (first_label != NULL) {	/* skip to last label */
	for (this_label = first_label; this_label != NULL;
	     prev_label = this_label, this_label = this_label->next)
	    /* is this the label we want? */
	    if (tag <= this_label->tag)
		break;
    }
    if (this_label != NULL && tag == this_label->tag) {
	/* changing the label */
	if (set_position) {
	    this_label->place = pos;
	}
	if (set_text)
	    this_label->text = text;
	if (set_just)
	    this_label->pos = just;
	if (set_rot)
	    this_label->rotate = rotate;
	if (set_layer)
	    this_label->layer = layer;
	if (set_font)
	    this_label->font = font;
	if (pointstyle != -2)
	    this_label->pointstyle = pointstyle;
	if (set_offset) {
	    this_label->hoffset = hoff;
	    this_label->voffset = voff;
	}
    } else {
	/* adding the label */
	new_label = (struct text_label *)
	    gp_alloc(sizeof(struct text_label), "label");
	if (prev_label != NULL)
	    prev_label->next = new_label;	/* add it to end of list */
	else
	    first_label = new_label;	/* make it start of list */
	new_label->tag = tag;
	new_label->next = this_label;
	new_label->place = pos;
	new_label->text = text;
	new_label->pos = just;
	new_label->rotate = rotate;
	new_label->layer = layer;
	new_label->font = font;
	if (pointstyle == -2)
	    new_label->pointstyle = -1; /* unset */
	else
	    new_label->pointstyle = pointstyle;
	new_label->hoffset = hoff;
	new_label->voffset = voff;
    }
    return tag;
}				/* Entry font added by DJL */


/* assign a new label tag
 * labels are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_label_tag()
{
    struct text_label *this_label;
    int last = 0;		/* previous tag value */

    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next)
	if (this_label->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}


/* delete label from linked list started by first_label.
 * called with pointers to the previous label (prev) and the 
 * label to delete (this).
 * If there is no previous label (the label to delete is
 * first_label) then call with prev = NULL.
 */
static void
delete_label(prev, this)
struct text_label *prev, *this;
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous label */
	    prev->next = this->next;
	else			/* this = first_label so change first_label */
	    first_label = this->next;
	free (this->text);
	free (this->font);
	free (this);
    }
}


/* process 'set loadpath' command */
static void
set_loadpath()
{
    /* We pick up all loadpath elements here before passing
     * them on to set_var_loadpath()
     */
    char *collect = NULL;

    c_token++;
    if (END_OF_COMMAND) {
	clear_loadpath();
    } else while (!END_OF_COMMAND) {
	if (isstring(c_token)) {
	    int len;
	    char *ss = gp_alloc(token_len(c_token), "tmp storage");
	    len = (collect? strlen(collect) : 0);
	    quote_str(ss,c_token,token_len(c_token));
	    collect = gp_realloc(collect, len+1+strlen(ss)+1, "tmp loadpath");
	    if (len != 0) {
		strcpy(collect+len+1,ss);
		*(collect+len) = PATHSEP;
	    }
	    else
		strcpy(collect,ss);
	    free(ss);
	    ++c_token;
	} else {
	    int_error(c_token, "Expected string");
	}
    }
    if (collect) {
	set_var_loadpath(collect);
	free(collect);
    }
}


/* process 'set locale' command */
static void
set_locale()
{
    c_token++;
    if (END_OF_COMMAND) {
	init_locale();
    } else if (isstring(c_token)) {
	char *ss = gp_alloc (token_len(c_token), "tmp locale");
	quote_str(ss,c_token,token_len(c_token));
	set_var_locale(ss);
	free(ss);
	++c_token;
    } else {
	int_error(c_token, "Expected string");
    }
}


/* FIXME - check whether the next two functions can be merged */

/* process 'set logscale' command */
static void
set_logscale()
{
    c_token++;
    if (END_OF_COMMAND) {
	is_log_x = is_log_y = is_log_z = is_log_x2 = is_log_y2 = TRUE;
	base_log_x = base_log_y = base_log_z = base_log_x2 = base_log_y2 = 10.0;
	log_base_log_x = log_base_log_y = log_base_log_z = log_base_log_x2 = log_base_log_y2 = M_LN10;
    } else {
	TBOOLEAN change_x = FALSE;
	TBOOLEAN change_y = FALSE;
	TBOOLEAN change_z = FALSE;
	TBOOLEAN change_x2 = FALSE;
	TBOOLEAN change_y2 = FALSE;
	double newbase = 10, log_newbase;

	if (equals(c_token, "x2"))
	    change_x2 = TRUE;
	else if (equals(c_token, "y2"))
	    change_y2 = TRUE;
	else { /* must not see x when x2, etc */
	    if (chr_in_str(c_token, 'x'))
		change_x = TRUE;
	    if (chr_in_str(c_token, 'y'))
		change_y = TRUE;
	    if (chr_in_str(c_token, 'z'))
		change_z = TRUE;
	}
	c_token++;
	if (!END_OF_COMMAND) {
	    struct value a;
	    newbase = magnitude(const_express(&a));
	    if (newbase < 1.1)
		int_error(c_token,
			  "log base must be >= 1.1; logscale unchanged");
	}
	log_newbase = log(newbase);

	if (change_x) {
	    is_log_x = TRUE;
	    base_log_x = newbase;
	    log_base_log_x = log_newbase;
	}
	if (change_y) {
	    is_log_y = TRUE;
	    base_log_y = newbase;
	    log_base_log_y = log_newbase;
	}
	if (change_z) {
	    is_log_z = TRUE;
	    base_log_z = newbase;
	    log_base_log_z = log_newbase;
	}
	if (change_x2) {
	    is_log_x2 = TRUE;
	    base_log_x2 = newbase;
	    log_base_log_x2 = log_newbase;
	}
	if (change_y2) {
	    is_log_y2 = TRUE;
	    base_log_y2 = newbase;
	    log_base_log_y2 = log_newbase;
	}
    }
}


/* process 'set mapping3d' command */
static void
set_mapping()
{
    c_token++;
    if (END_OF_COMMAND)
	/* assuming same as points */
	mapping3d = MAP3D_CARTESIAN;
    else if (almost_equals(c_token, "ca$rtesian"))
	mapping3d = MAP3D_CARTESIAN;
    else if (almost_equals(c_token, "s$pherical"))
	mapping3d = MAP3D_SPHERICAL;
    else if (almost_equals(c_token, "cy$lindrical"))
	mapping3d = MAP3D_CYLINDRICAL;
    else
	int_error(c_token,
		  "expecting 'cartesian', 'spherical', or 'cylindrical'");
        c_token++;
}


/* FIXME - merge set_*margin() functions */

#define PROCESS_MARGIN(MARGIN) \
 c_token++; \
 if (END_OF_COMMAND) \
  MARGIN = -1; \
 else { \
  struct value a; \
  MARGIN = real(const_express(&a)); \
 }

/* process 'set bmargin' command */
static void
set_bmargin()
{
    PROCESS_MARGIN(bmargin); 
}


/* process 'set lmargin' command */
static void
set_lmargin()
{
    PROCESS_MARGIN(lmargin);
}


/* process 'set rmargin' command */
static void
set_rmargin()
{
    PROCESS_MARGIN(rmargin);
}


/* process 'set tmargin' command */
static void
set_tmargin()
{
    PROCESS_MARGIN(tmargin);
}


/* process 'set missing' command */
static void
set_missing()
{
    c_token++;
    if (END_OF_COMMAND) {
	if (missing_val)
	    free(missing_val);
	missing_val = NULL;
    } else {
	if (!isstring(c_token))
	    int_error(c_token, "Expected missing-value string");
	m_quote_capture(&missing_val, c_token, c_token);
	c_token++;
    }
}

#ifdef USE_MOUSE
static void
set_mouse()
{
    c_token++;
    mouse_setting.on = 1;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "do$ubleclick")) {
	    struct value a;
	    ++c_token;
	    mouse_setting.doubleclick = real(const_express(&a));
	    if (mouse_setting.doubleclick < 0)
		mouse_setting.doubleclick = 0;
	} else if (almost_equals(c_token, "nodo$ubleclick")) {
	    mouse_setting.doubleclick = 0; /* double click off */
	    ++c_token;
	} else if (almost_equals(c_token, "zoomco$ordinates")) {
	    mouse_setting.annotate_zoom_box = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nozoomco$ordinates")) {
	    mouse_setting.annotate_zoom_box = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "po$lardistance")) {
	    mouse_setting.polardistance = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nopo$lardistance")) {
	    mouse_setting.polardistance = 0;
	    ++c_token;
	} else if (equals(c_token, "labels")) {
	    mouse_setting.label = 1;
	    ++c_token;
	    /* check if the optional argument "<label options>" is present. */
	    if (isstring(c_token)) {
		if (token_len(c_token) >= sizeof(mouse_setting.labelopts)) {
		    int_error(c_token, "option string too long");
		} else {
		    quote_str(mouse_setting.labelopts,
			c_token, token_len(c_token));
		}
		++c_token;
	    }
	} else if (almost_equals(c_token, "nola$bels")) {
	    mouse_setting.label = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "ve$rbose")) {
	    mouse_setting.verbose = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nove$rbose")) {
	    mouse_setting.verbose = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "zoomju$mp")) {
	    mouse_setting.warp_pointer = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nozoomju$mp")) {
	    mouse_setting.warp_pointer = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "fo$rmat")) {
	    ++c_token;
	    if (isstring(c_token)) {
		if (token_len(c_token) >= sizeof(mouse_setting.fmt)) {
		    int_error(c_token, "format string too long");
		} else {
		    quote_str(mouse_setting.fmt, c_token, token_len(c_token));
		}
	    } else {
		int_error(c_token, "expecting string format");
	    }
	    ++c_token;
	} else if (almost_equals(c_token, "cl$ipboardformat")) {
	    ++c_token;
	    if (isstring(c_token)) {
		if (clipboard_alt_string)
		    free(clipboard_alt_string);
		clipboard_alt_string = gp_alloc
		    (token_len(c_token), "set->mouse->clipboardformat");
		quote_str(clipboard_alt_string, c_token, token_len(c_token));
		if (clipboard_alt_string && !strlen(clipboard_alt_string)) {
		    free(clipboard_alt_string);
		    clipboard_alt_string = (char*) 0;
		    if (MOUSE_COORDINATES_ALT == mouse_mode) {
			mouse_mode = MOUSE_COORDINATES_REAL;
		    }
		} else {
		    clipboard_mode = MOUSE_COORDINATES_ALT;
		}
		c_token++;
	    } else {
		struct value a;
		int itmp = (int) real(const_express(&a));
		if (itmp >= MOUSE_COORDINATES_REAL
		    && itmp <= MOUSE_COORDINATES_XDATETIME) {
		    if (MOUSE_COORDINATES_ALT == itmp
			&& !clipboard_alt_string) {
			fprintf(stderr,
			    "please 'set mouse clipboard <fmt>' first.\n");
		    } else {
			clipboard_mode = itmp;
		    }
		} else {
		    fprintf(stderr, "should be: %d <= clipboardformat <= %d\n",
			MOUSE_COORDINATES_REAL, MOUSE_COORDINATES_XDATETIME);
		}
	    }
	} else if (almost_equals(c_token, "mo$useformat")) {
	    ++c_token;
	    if (isstring(c_token)) {
		if (mouse_alt_string)
		    free(mouse_alt_string);
		mouse_alt_string = gp_alloc
		    (token_len(c_token), "set->mouse->mouseformat");
		quote_str(mouse_alt_string, c_token, token_len(c_token));
		if (mouse_alt_string && !strlen(mouse_alt_string)) {
		    free(mouse_alt_string);
		    mouse_alt_string = (char*) 0;
		    if (MOUSE_COORDINATES_ALT == mouse_mode) {
			mouse_mode = MOUSE_COORDINATES_REAL;
		    }
		} else {
		    mouse_mode = MOUSE_COORDINATES_ALT;
		}
		c_token++;
	    } else {
		struct value a;
		int itmp = (int) real(const_express(&a));
		if (itmp >= MOUSE_COORDINATES_REAL
		    && itmp <= MOUSE_COORDINATES_XDATETIME) {
		    if (MOUSE_COORDINATES_ALT == itmp && !mouse_alt_string) {
			fprintf(stderr,
			    "please 'set mouse mouseformat <fmt>' first.\n");
		    } else {
			mouse_mode = itmp;
		    }
		} else {
		    fprintf(stderr, "should be: %d <= mouseformat <= %d\n",
			MOUSE_COORDINATES_REAL, MOUSE_COORDINATES_XDATETIME);
		}
	    }
	} else {
	    /* discard unknown options (joze) */
	    break;
	}
    }
#if defined(USE_MOUSE) && defined(OS2)
    update_menu_items_PM_terminal();
#endif
}
#endif

/* process 'set offsets' command */
static void
set_offsets()
{
    c_token++;
    if (END_OF_COMMAND) {
	loff = roff = toff = boff = 0.0;  /* Reset offsets */
    } else {
	load_offsets (&loff,&roff,&toff,&boff);
    }
}


/* process 'set origin' command */
static void
set_origin()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	xoffset = 0.0;
	yoffset = 0.0;
    } else {
	xoffset = real(const_express(&a));
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	yoffset = real(const_express(&a));
    } 
}


/* process 'set output' command */
static void
set_output()
{
    c_token++;
    if (multiplot)
	int_error(c_token, "you can't change the output in multiplot mode");

    if (END_OF_COMMAND) {	/* no file specified */
	term_set_output(NULL);
	if (outstr) {
	    free(outstr);
	    outstr = NULL; /* means STDOUT */
	}
    } else if (!isstring(c_token)) {
	int_error(c_token, "expecting filename");
    } else {
	/* on int_error, we'd like to remember that this is allocated */
	static char *testfile = NULL;
	m_quote_capture(&testfile,c_token, c_token); /* reallocs store */
	gp_expand_tilde(&testfile);
	/* Skip leading whitespace */
	while (isspace((int)*testfile))
	    testfile++;
	c_token++;
	term_set_output(testfile);
	/* if we get here then it worked, and outstr now = testfile */
	testfile = NULL;
    }
}


/* process 'set parametric' command */
static void
set_parametric()
{
    c_token++;

    if (!parametric) {
	parametric = TRUE;
	if (!polar) { /* already done for polar */
	    strcpy (dummy_var[0], "t");
	    strcpy (dummy_var[1], "y");
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is t for curves, u/v for surfaces\n");
	}
    }
}

#ifdef PM3D
/* process 'set palette' command */
static void
set_palette()
{
    c_token++;

    if (END_OF_COMMAND) { /* assume default settings */
	sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
	sm_palette.formulaR = 7; sm_palette.formulaG = 5;
	sm_palette.formulaB = 15;
	sm_palette.positive = SMPAL_POSITIVE;
	sm_palette.ps_allcF = 0;
	sm_palette.use_maxcolors = 0;

	color_box.where = SMCOLOR_BOX_DEFAULT;
	color_box.rotation = 'v';
	color_box.border = 1;
	color_box.border_lt_tag = -1; /* use default border */
    }
    else { /* go through all options of 'set palette' */
	for ( ; !END_OF_COMMAND && !equals(c_token,";"); c_token++ ) {
	    /* positive and negative picture */
	    if (almost_equals(c_token, "pos$itive")) {
		sm_palette.positive = SMPAL_POSITIVE;
		continue;
	    }
	    if (almost_equals(c_token, "neg$ative")) {
		sm_palette.positive = SMPAL_NEGATIVE;
		continue;
	    }
	    /* Now the options that determine the palette of smooth colours */
	    /* gray or rgb-coloured */
	    if (equals(c_token, "gray")) {
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRAY;
		continue;
	    }
	    if (almost_equals(c_token, "col$or")) {
		sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
		continue;
	    }
	    /* rgb color mapping formulae: rgb$formulae r,g,b (three integers) */
	    if (almost_equals(c_token, "rgb$formulae")) {
		struct value a;
		int i;
		c_token++;
		i = (int)real(const_express(&a));
		if ( abs(i) >= sm_palette.colorFormulae )
		    int_error(c_token,"color formula out of range (use `show pm3d' to display the range)");
		sm_palette.formulaR = i;
		if (!equals(c_token,",")) { c_token--; continue; }
		c_token++;
		i = (int)real(const_express(&a));
		if ( abs(i) >= sm_palette.colorFormulae )
		    int_error(c_token,"color formula out of range (use `show pm3d' to display the range)");
		sm_palette.formulaG = i;
		if (!equals(c_token,",")) { c_token--; continue; }
		c_token++;
		i = (int)real(const_express(&a));
		if ( abs(i) >= sm_palette.colorFormulae )
		    int_error(c_token,"color formula out of range (`show pm3d' displays the range)");
		sm_palette.formulaB = i;
		c_token--;
		continue;
	    } /* rgbformulae */
	    /* ps_allcF: write all rgb formulae into PS file? */
	    if (equals(c_token, "nops_allcF")) {
		sm_palette.ps_allcF = 0;
		continue;
	    }
	    if (equals(c_token, "ps_allcF")) {
		sm_palette.ps_allcF = 1;
		continue;
	    }
	    /* max colors used */
	    if (almost_equals(c_token, "maxc$olors")) {
		struct value a;
		int i;
		c_token++;
		i = (int)real(const_express(&a));
		if (i<0) int_error(c_token,"non-negative number required");
		sm_palette.use_maxcolors = i;
		continue;
	    }
	    /* Now color box properties */
	    /* vertical or horizontal color gradient */
	    if (almost_equals(c_token, "cbv$ertical")) {
		color_box.rotation = 'v';
		continue;
	    }
	    if (almost_equals(c_token, "cbh$orizontal")) {
		color_box.rotation = 'h';
		continue;
	    }
	    /* color box where: no box, default position, position by user */
	    if (equals(c_token, "nocb")) {
		color_box.where = SMCOLOR_BOX_NO;
		continue;
	    }
	    if (almost_equals(c_token, "cbdef$ault")) {
		color_box.where = SMCOLOR_BOX_DEFAULT;
		continue;
	    }
	    if (almost_equals(c_token, "cbu$ser")) {
		color_box.where = SMCOLOR_BOX_USER;
		continue;
	    }
	    if (almost_equals(c_token, "bo$rder")) {

		color_box.border = 1;
		c_token++;

		if (!END_OF_COMMAND) {
		    /* expecting a border line type */
		    struct value a;
		    color_box.border_lt_tag = real(const_express(&a));
		    if (color_box.border_lt_tag <= 0) {
			color_box.border_lt_tag = 0;
			int_error(c_token, "tag must be strictly positive (see `help set style line')");
		    }
		    --c_token; /* why ? (joze) */
		}
		continue;
	    }
	    if (almost_equals(c_token, "bd$efault")) {
		color_box.border_lt_tag = -1; /* use default border */
		continue;
	    }
	    if (almost_equals(c_token, "nob$order")) {
		color_box.border = 0;
		continue;
	    }
	    if (almost_equals(c_token, "o$rigin")) {
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting screen value [0 - 1]");
		} else {
		    struct value a;
		    color_box.xorigin = real(const_express(&a));
		    if (!equals(c_token,","))
			int_error(c_token, "',' expected");
		    c_token++;
		    color_box.yorigin = real(const_express(&a));
		    c_token--;
		} 
		continue;
	    }
	    if (almost_equals(c_token, "s$ize")) {
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting screen value [0 - 1]");
		} else {
		    struct value a;
		    color_box.xsize = real(const_express(&a));
		    if (!equals(c_token,","))
			int_error(c_token, "',' expected");
		    c_token++;
		    color_box.ysize = real(const_express(&a));
		    c_token--;
		} 
		continue;
	    }
	    int_error(c_token,"invalid palette option");
	} /* end of while over palette options */
    }
}


/* process 'set pm3d' command */
static void
set_pm3d()
{
    c_token++;

    if (END_OF_COMMAND) { /* assume default settings */
	pm3d_reset();
	strcpy(pm3d.where,"s"); /* draw at surface */
    }
    else { /* go through all options of 'set pm3d' */
	for ( ; !END_OF_COMMAND && !equals(c_token,";"); c_token++ ) {
	    if (equals(c_token, "at")) {
		char* c;
		c_token++;
		if (token[c_token].length >= sizeof(pm3d.where)) {
		    int_error(c_token,"ignoring so many redrawings");
		    return /* (TRUE) */;
		}
		strncpy(pm3d.where, input_line + token[c_token].start_index, token[c_token].length);
		pm3d.where[ token[c_token].length ] = 0;
		for (c = pm3d.where; *c; c++) {
		    if (*c != 'C') /* !!!!! CONTOURS, UNDOCUMENTED, THIS LINE IS TEMPORARILY HERE !!!!! */
			if (*c != PM3D_AT_BASE && *c != PM3D_AT_TOP && *c != PM3D_AT_SURFACE) {
			    int_error(c_token,"parameter to pm3d requires combination of characters b,s,t\n\t(drawing at bottom, surface, top)");
			    return /* (TRUE) */;
			}
		}
		continue;
	    }  /* at */
	    /* forward and backward drawing direction */
	    if (almost_equals(c_token, "scansfor$ward")) {
		pm3d.direction = PM3D_SCANS_FORWARD;
		continue;
	    }
	    if (almost_equals(c_token, "scansback$ward")) {
		pm3d.direction = PM3D_SCANS_BACKWARD;
		continue;
	    }
	    if (almost_equals(c_token, "scansauto$matic")) {
		pm3d.direction = PM3D_SCANS_AUTOMATIC;
		continue;
	    }
	    /* flush scans: left, right or center */
	    if (almost_equals(c_token, "fl$ush")) {
		c_token++;
		if (almost_equals(c_token, "b$egin"))
		    pm3d.flush = PM3D_FLUSH_BEGIN;
		else if (almost_equals(c_token, "c$enter"))
		    pm3d.flush = PM3D_FLUSH_CENTER;
		else if (almost_equals(c_token, "e$nd"))
		    pm3d.flush = PM3D_FLUSH_END;
		else int_error(c_token,"expecting flush 'begin', 'center' or 'end'");
		continue;
	    }
	    /* clipping method */
	    if (almost_equals(c_token, "clip1$in")) {
		pm3d.clip = PM3D_CLIP_1IN;
		continue;
	    }
	    if (almost_equals(c_token, "clip4$in")) {
		pm3d.clip = PM3D_CLIP_4IN;
		continue;
	    }
	    /* zrange [{zmin|*}:{zmax|*}] */
	    /* Note: here, we cannot use neither PROCESS_RANGE or load_range */
	    if (almost_equals(c_token, "zr$ange")) {
		struct value a;
		if (!equals(++c_token,"[")) int_error(c_token,"expecting '['");
		c_token++;
		if (!equals(c_token,":")) { /* no change for zmin */
		    if (equals(c_token,"*")) {
			pm3d.pm3d_zmin = 0; /* from gnuplot's set zrange */
			c_token++;
		    }
		    else {
			pm3d.pm3d_zmin = 1; /* use pm3d's zmin */
			pm3d.zmin = real(const_express(&a)); /* explicit value given */
		    }
		}
		if (!equals(c_token,":")) int_error(c_token,"expecting ':'");
		c_token++;
		if (!equals(c_token,"]")) { /* no change for zmax */
		    if (equals(c_token,"*")) {
			pm3d.pm3d_zmax = 0; /* from gnuplot's set zrange */
			c_token++;
		    }
		    else {
			pm3d.pm3d_zmax = 1; /* use pm3d's zmin */
			pm3d.zmax = real(const_express(&a)); /* explicit value given */
		    }
		}
		if (!equals(c_token,"]")) int_error(c_token,"expecting ']'");
		continue;
	    }
	    /* setup everything for plotting a map */
	    if (equals(c_token, "map")) {
		pm3d.where[0] = 'b'; pm3d.where[1] = 0; /* set pm3d at b */
		draw_surface = FALSE;        /* set nosurface */
		draw_contour = CONTOUR_NONE; /* set nocontour */
		surface_rot_x = 180;         /* set view 180,0,1.3 */
		surface_rot_z = 0;
		surface_scale = 1.3;
		range_flags[FIRST_Y_AXIS] |= RANGE_REVERSE; /* set yrange reverse */
		pm3d_map_rotate_ylabel = 1;  /* trick for rotating ylabel */
		continue;
	    }
	    if (almost_equals(c_token, "hi$dden3d")) {
		struct value a;
		c_token++;
		pm3d.hidden3d_tag = real(const_express(&a));
		if (pm3d.hidden3d_tag <= 0) {
		    pm3d.hidden3d_tag = 0;
		    int_error(c_token,"tag must be strictly positive (see `help set style line')");
		}
		--c_token; /* why ? (joze) */
		continue;
	    }
	    if (almost_equals(c_token, "nohi$dden3d")) {
		pm3d.hidden3d_tag = 0;
		continue;
	    }
	    if (almost_equals(c_token, "so$lid") || almost_equals(c_token, "notr$ansparent")) {
		pm3d.solid = 1;
		continue;
	    }
	    if (almost_equals(c_token, "noso$lid") || almost_equals(c_token, "tr$ansparent")) {
		pm3d.solid = 0;
		continue;
	    }
	    int_error(c_token,"invalid pm3d option");
	} /* end of while over pm3d options */
	if (PM3D_SCANS_AUTOMATIC == pm3d.direction
	    && PM3D_FLUSH_BEGIN != pm3d.flush) {
	    pm3d.direction = PM3D_SCANS_FORWARD;
	    /* Petr Mikulik said that he wants no warning message here (joze) */
#if 0
	    fprintf(stderr, "pm3d: `scansautomatic' and %s are incompatible\n",
		PM3D_FLUSH_END == pm3d.flush ? "`flush end'": "`flush center'");
	    fprintf(stderr, "      setting scansforward\n");
#endif
	}
    }
}
#endif

/* process 'set pointsize' command */
static void
set_pointsize()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND)
	pointsize = 1.0;
    else
	pointsize = real(const_express(&a));
    if(pointsize <= 0) pointsize = 1;
}


/* process 'set polar' command */
static void
set_polar()
{
    c_token++;

    if (!polar) {
	if (!parametric) {
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is t for curves\n");
	    strcpy (dummy_var[0], "t");
	}
	polar = TRUE;
	if (autoscale_t) {
	    /* only if user has not set a range manually */
	    tmin = 0.0;
	    tmax = 2 * M_PI / ang2rad;  /* 360 if degrees, 2pi if radians */
	}
    }
}


/* process 'set samples' command */
static void
set_samples()
{
    register int tsamp1, tsamp2;
    struct value a;

    c_token++;
    tsamp1 = (int)magnitude(const_express(&a));
    tsamp2 = tsamp1;
    if (!END_OF_COMMAND) {
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	tsamp2 = (int)magnitude(const_express(&a));
    }
    if (tsamp1 < 2 || tsamp2 < 2)
	int_error(c_token, "sampling rate must be > 1; sampling unchanged");
    else {
	register struct surface_points *f_3dp = first_3dplot;

	first_3dplot = NULL;
	sp_free(f_3dp);

	samples = tsamp1;
	samples_1 = tsamp1;
	samples_2 = tsamp2;
    }
}


/* process 'set size' command */
static void
set_size()
{
    struct value s;

    c_token++;
    if (END_OF_COMMAND) {
	xsize = 1.0;
	ysize = 1.0;
    } else {
	if (almost_equals(c_token, "sq$uare")) {
	    aspect_ratio = 1.0;
	    ++c_token;
	} else if (almost_equals(c_token,"ra$tio")) {
	    ++c_token;
	    aspect_ratio = real(const_express(&s));
	} else if (almost_equals(c_token, "nora$tio") || almost_equals(c_token, "nosq$uare")) {
	    aspect_ratio = 0.0;
	    ++c_token;
	}

	if (!END_OF_COMMAND) {
	    xsize = real(const_express(&s));
	    if (equals(c_token,",")) {
		c_token++;
		ysize = real(const_express(&s));
	    } else {
		ysize = xsize;
	    }
	}
    }
}


/* process 'set style' command */
static void
set_style()
{
    c_token++;

    if (almost_equals(c_token, "d$ata"))
	data_style = get_style();
    else if (almost_equals(c_token, "f$unction"))
	func_style = get_style();
    else if (almost_equals(c_token, "l$ine")) {
	set_linestyle();
    } else
	int_error(c_token, "expecting 'data', 'function', or 'line'");
}


/* process 'set surface' command */
static void
set_surface()
{
    c_token++;
    draw_surface = TRUE;
}


/* process 'set terminal' comamnd */
static void
set_terminal()
{
    c_token++;

    if (multiplot)
	int_error(c_token, "You can't change the terminal in multiplot mode");

    if (END_OF_COMMAND) {
	list_terms();
	screen_ok = FALSE;
    } else {
#ifdef EXTENDED_COLOR_SPECS
	/* each terminal is supposed to turn this on, probably
	 * somewhere when the graphics is initialized */
	supply_extended_color_specs = 0;
#endif
#ifdef USE_MOUSE
        event_reset((void *)1);   /* cancel zoombox etc. */
#endif
	term_reset();
	term = 0; /* in case set_term() fails */
	term = set_term(c_token);
	c_token++;

	/* FIXME
	 * handling of common terminal options before term specific options
	 * as per HBB's suggestion
	 * new `set termoptions' command
	 */
	/* get optional mode parameters
	 * not all drivers reset the option string before
	 * strcat-ing to it, so we reset it for them
	 */
	*term_options = 0;
	if (term)
	    (*term->options)();
	if (interactive && *term_options)
	    fprintf(stderr,"Options are '%s'\n",term_options);
    }
}

/* process 'set termoptions' command */
static void
set_termoptions()
{
    int_error(c_token,"Command not yet supported");

    /* if called from term.c:
     * scan input_line for common options
     * filter out common options
     * reset input_line (num_tokens = scanner(&input_line, &input_line_len);
     * c_token=0 (1? 2)
     */
}


/* process 'set tics' command */
static void
set_tics()
{
    c_token++;
    tic_in = TRUE;

    if (almost_equals(c_token,"i$n")) {
	tic_in = TRUE;
	c_token++;
    } else if (almost_equals(c_token,"o$ut")) {
	tic_in = FALSE;
	c_token++;
    }
}


/* process 'set ticscale' command */
static void
set_ticscale()
{
    struct value tscl;

    c_token++;
    if (END_OF_COMMAND) {
	ticscale = 1.0;
	miniticscale = 0.5;
    } else {
	ticscale = real(const_express(&tscl));
	if (END_OF_COMMAND) {
	    miniticscale = ticscale*0.5;
	} else {
	    miniticscale = real(const_express(&tscl));
	}
    }
}


/* process 'set ticslevel' command */
static void
set_ticslevel()
{
    struct value a;
    double tlvl;

    c_token++;
    /* is datatype 'time' relevant here ? */
    tlvl = real(const_express(&a));
    ticslevel = tlvl;
}


/* Process 'set timefmt' command */
static void
set_timefmt()
{
    c_token++;
    if (END_OF_COMMAND) {	/* set to default */
	strcpy(timefmt,TIMEFMT);
    } else {
	if (isstring(c_token)) {
	    /* We have string specified - grab it. */
	    quote_str(timefmt,c_token, 25);
	}
	c_token++;
    }
}


/* process 'set timestamp' command */
static void
set_timestamp()
{
    c_token++;
    if (END_OF_COMMAND || !isstring(c_token))
	strcpy(timelabel.text, DEFAULT_TIMESTAMP_FORMAT);

    if (!END_OF_COMMAND) {
	struct value a;

	if (isstring(c_token)) {
	    /* we have a format string */
	    quote_str(timelabel.text, c_token, MAX_LINE_LEN);
	    ++c_token;
	} else {
	    strcpy(timelabel.text, DEFAULT_TIMESTAMP_FORMAT);
	}
	if (almost_equals(c_token,"t$op")) {
	    timelabel_bottom = FALSE;
	    ++c_token;
	} else if (almost_equals(c_token, "b$ottom")) {
	    timelabel_bottom = TRUE;
	    ++c_token;
	}
	if (almost_equals(c_token,"r$otate")) {
	    timelabel_rotate = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token, "n$orotate")) {
	    timelabel_rotate = FALSE;
	    ++c_token;
	}
	/* We have x,y offsets specified */
	if (!END_OF_COMMAND && !equals(c_token,","))
	    timelabel.xoffset = real(const_express(&a));
	if (!END_OF_COMMAND && equals(c_token,",")) {
	    c_token++;
	    timelabel.yoffset = real(const_express(&a));
	}
	if (!END_OF_COMMAND && isstring(c_token)) {
	    quote_str(timelabel.font, c_token, MAX_LINE_LEN);
	    ++c_token;
	} else {
	    *timelabel.font = 0;
	}
    }
}


/* process 'set view' command */
static void
set_view()
{
    int i;
    TBOOLEAN was_comma = TRUE;
    char *errmsg1 = "rot_%c must be in [0:%d] degrees range; view unchanged";
    char *errmsg2 = "%sscale must be > 0; view unchanged";
    double local_vals[4];
    struct value a;

    local_vals[0] = surface_rot_x;
    local_vals[1] = surface_rot_z;
    local_vals[2] = surface_scale;
    local_vals[3] = surface_zscale;
    c_token++;
    for (i = 0; i < 4 && !(END_OF_COMMAND);) {
	if (equals(c_token,",")) {
	    if (was_comma) i++;
	    was_comma = TRUE;
	    c_token++;
	} else {
	    if (!was_comma)
		int_error(c_token, "',' expected");
	    local_vals[i] = real(const_express(&a));
	    i++;
	    was_comma = FALSE;
	}
    }

    if (local_vals[0] < 0 || local_vals[0] > 180)
	int_error(c_token, errmsg1, 'x', 180);
    if (local_vals[1] < 0 || local_vals[1] > 360)
	int_error(c_token, errmsg1, 'z', 360);
    if (local_vals[2] < 1e-6)
	int_error(c_token, errmsg2, "");
    if (local_vals[3] < 1e-6)
	int_error(c_token, errmsg2, "z");

    surface_rot_x = local_vals[0];
    surface_rot_z = local_vals[1];
    surface_scale = local_vals[2];
    surface_zscale = local_vals[3];

}


/* process 'set zero' command */
static void
set_zero()
{
    struct value a;
    c_token++;
    zero = magnitude(const_express(&a));
}

/* FIXME - merge set_*data() functions into one */

/* process 'set xdata' command */
static void
set_xdata()
{
    c_token++;
    if(END_OF_COMMAND) {
	datatype[FIRST_X_AXIS] = FALSE;
	/* eh ? - t and u have nothing to do with x */
	datatype[T_AXIS] = FALSE;
	datatype[U_AXIS] = FALSE;
    } else {
	if (almost_equals(c_token,"t$ime")) {
	    datatype[FIRST_X_AXIS] = TIME;
	    datatype[T_AXIS] = TIME;
	    datatype[U_AXIS] = TIME;
	} else {
	    datatype[FIRST_X_AXIS] = FALSE;
	    datatype[T_AXIS] = FALSE;
	    datatype[U_AXIS] = FALSE;
	}
	c_token++;
    }
}


/* process 'set ydata' command */
static void
set_ydata()
{
    c_token++;
    if(END_OF_COMMAND) {
	datatype[FIRST_Y_AXIS] = FALSE;
	datatype[V_AXIS] = FALSE;
    } else {
	if (almost_equals(c_token,"t$ime")) {
	    datatype[FIRST_Y_AXIS] = TIME;
	    datatype[V_AXIS] = TIME;
	} else {
	    datatype[FIRST_Y_AXIS] = FALSE;
	    datatype[V_AXIS] = FALSE;
	}
	c_token++;
    }
}

#define PROCESS_AXIS_DATA(AXIS) \
 c_token++; \
 if(END_OF_COMMAND) { \
  datatype[AXIS] = FALSE; \
 } else { \
  if (almost_equals(c_token,"t$ime")) { \
   datatype[AXIS] = TIME; \
  } else { \
    datatype[AXIS] = FALSE; \
  } \
  c_token++; \
 }

/* process 'set zdata' command */
static void
set_zdata()
{
    PROCESS_AXIS_DATA(FIRST_Z_AXIS);
}


/* process 'set x2data' command */
static void
set_x2data()
{
    PROCESS_AXIS_DATA(SECOND_X_AXIS);
}


/* process 'set y2data' command */
static void
set_y2data()
{
    PROCESS_AXIS_DATA(SECOND_Y_AXIS);
}


/* FIXME - merge set_*range() functions into one */

#define PROCESS_RANGE(AXIS,MIN,MAX,AUTO) \
  if(almost_equals(++c_token,"re$store")) { /* ULIG */ \
    c_token++; \
    MIN = get_writeback_min(AXIS); \
    MAX = get_writeback_max(AXIS); \
    AUTO = 0; \
  } else { \
    if (!equals(c_token,"[")) int_error(c_token,"expecting '[' or 'restore'"); \
    c_token++; \
    AUTO = load_range(AXIS,&MIN,&MAX,AUTO); \
    if (!equals(c_token,"]")) int_error(c_token,"expecting ']'"); \
    c_token++; \
    if (almost_equals(c_token, "rev$erse")) { \
      ++c_token; range_flags[AXIS] |= RANGE_REVERSE;\
    } else if (almost_equals(c_token, "norev$erse")) { \
      ++c_token; range_flags[AXIS] &= ~RANGE_REVERSE;\
    } if (almost_equals(c_token, "wr$iteback")) { \
      ++c_token; range_flags[AXIS] |= RANGE_WRITEBACK;\
    } else if (almost_equals(c_token, "nowri$teback")) { \
      ++c_token; range_flags[AXIS] &= ~RANGE_WRITEBACK;\
  }}

/* process 'set xrange' command */
static void
set_xrange()
{
    PROCESS_RANGE(FIRST_X_AXIS,xmin,xmax,autoscale_x);
}


/* process 'set x2range' command */
static void
set_x2range()
{
    PROCESS_RANGE(SECOND_X_AXIS,x2min,x2max,autoscale_x2);
}


/* process 'set yrange' command */
static void
set_yrange()
{
    PROCESS_RANGE(FIRST_Y_AXIS,ymin,ymax,autoscale_y);
}


/* process 'set y2range' command */
static void
set_y2range()
{
    PROCESS_RANGE(SECOND_Y_AXIS,y2min,y2max,autoscale_y2);
}


/* process 'set zrange' command */
static void
set_zrange()
{
    PROCESS_RANGE(FIRST_Z_AXIS,zmin,zmax,autoscale_z);
}


/* process 'set rrange' command */
static void
set_rrange()
{
    PROCESS_RANGE(R_AXIS,rmin,rmax,autoscale_r);
}


/* process 'set trange' command */
static void
set_trange()
{
    PROCESS_RANGE(T_AXIS,tmin,tmax,autoscale_t);
}


/* process 'set urange' command */
static void
set_urange()
{
    PROCESS_RANGE(U_AXIS,umin,umax,autoscale_u);
}


/* process 'set vrange' command */
static void
set_vrange()
{
    PROCESS_RANGE(V_AXIS,vmin,vmax,autoscale_v);
}


/* FIXME - merge *zeroaxis() functions into one */

#define PROCESS_ZEROAXIS(ZAXIS) \
 c_token++; \
  if (END_OF_COMMAND) \
   ZAXIS.l_type = -1; \
  else { \
   struct value a; \
   int old_token = c_token; \
   lp_parse(&ZAXIS,1,0,-1,0); \
   if (old_token == c_token) \
    ZAXIS.l_type = real(const_express(&a)) - 1; \
  }

/* process 'set xzeroaxis' command */
static void
set_xzeroaxis()
{
    PROCESS_ZEROAXIS(xzeroaxis);
}


/* process 'set yzeroaxis' command */
static void
set_yzeroaxis()
{
    PROCESS_ZEROAXIS(yzeroaxis);
}

/* process 'set x2zeroaxis' command */
static void
set_x2zeroaxis()
{
    PROCESS_ZEROAXIS(x2zeroaxis);
}


/* process 'set y2zeroaxis' command */
static void
set_y2zeroaxis()
{
    PROCESS_ZEROAXIS(y2zeroaxis);
}


/* process 'set zeroaxis' command */
static void
set_zeroaxis()
{
    c_token++;
    lp_parse(&xzeroaxis,1,0,-1,0);
    memcpy(&yzeroaxis,&xzeroaxis,sizeof(struct lp_style_type));
}


/*********** Support functions for set_command ***********/

/*
 * The set.c PROCESS_TIC_PROP macro has the following characteristics:
 *   (a) options must in the correct order
 *   (b) 'set xtics' (no option) resets only the interval (FREQ)
 *       {it will also negate NO_TICS, see (d)} 
 *   (c) changing any property also resets the interval to automatic
 *   (d) set no[xy]tics; set [xy]tics changes border to nomirror, rather 
 *       than to the default, mirror.
 *   (e) effect of 'set no[]tics; set []tics border ...' is compiler
 *       dependent;  if '!(TICS)' is evaluated first, 'border' is an 
 *       undefined variable :-(
 *
 * This function replaces the macro, and introduces a new option
 * 'au$tofreq' to give somewhat different behaviour:
 *   (a) no change
 *   (b) 'set xtics' (no option) only affects NO_TICS;  'autofreq' resets 
 *       the interval calulation to automatic
 *   (c) the interval mode is not affected by changing some other option
 *   (d) if NO_TICS, set []tics will restore defaults (borders, mirror
 *       where appropriate)
 *   (e) if (NO_TICS), border option is processed.
 *
 *  A 'default' option could easily be added to reset all options to
 *  the initial values - mostly book-keeping.
 *
 *  To retain tic properties after setting no[]tics may also be
 *  straightforward (save value as negative), but requires changes
 *  in other code ( e.g. for  'if (xtics)', use 'if (xtics > 0)'
 */

/*    generates PROCESS_TIC_PROP strings from tic_side, e.g. "x2" 
 *  STRING, NOSTRING, MONTH, NOMONTH, DAY, NODAY, MINISTRING, NOMINI
 *  "nox2t$ics"     "nox2m$tics"  "nox2d$tics"    "nomx2t$ics" 
 */

static int
set_tic_prop(TICS, MTICS, FREQ, tdef, AXIS, ROTATE, tic_side)
int *TICS, *MTICS, AXIS;
double *FREQ;
struct ticdef *tdef;
TBOOLEAN *ROTATE;
const char *tic_side;
{
    int match = 0;		/* flag, set by matching a tic command */
    char nocmd[12];		/* fill w/ "no"+'tic_side'+suffix */
    char *cmdptr, *sfxptr;

    (void) strcpy(nocmd, "no");
    cmdptr = &nocmd[2];
    (void) strcpy(cmdptr, tic_side);
    sfxptr = &nocmd[strlen(nocmd)];
    (void) strcpy(sfxptr, "t$ics");	/* STRING */

    if (almost_equals(c_token, cmdptr)) {
	match = 1;
	if (almost_equals(++c_token, "ax$is")) {
	    *TICS &= ~TICS_ON_BORDER;
	    *TICS |= TICS_ON_AXIS;
	    ++c_token;
	}
	/* if tics are off, reset to default (border) */
	if (*TICS == NO_TICS) {
	    *TICS = TICS_ON_BORDER;
	    if (!strcmp(tic_side, "x") || !strcmp(tic_side, "y")) {
		*TICS |= TICS_MIRROR;
	    }
	}
	if (almost_equals(c_token, "bo$rder")) {
	    *TICS &= ~TICS_ON_AXIS;
	    *TICS |= TICS_ON_BORDER;
	    ++c_token;
	}
	if (almost_equals(c_token, "mi$rror")) {
	    *TICS |= TICS_MIRROR;
	    ++c_token;
	} else if (almost_equals(c_token, "nomi$rror")) {
	    *TICS &= ~TICS_MIRROR;
	    ++c_token;
	}
	if (almost_equals(c_token, "ro$tate")) {
	    *ROTATE = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token, "noro$tate")) {
	    *ROTATE = FALSE;
	    ++c_token;
	}
	if (almost_equals(c_token, "au$tofreq")) {	/* auto tic interval */
	    ++c_token;
	    if (tdef->type == TIC_USER) {
		free_marklist(tdef->def.user);
		tdef->def.user = NULL;
	    }
	    tdef->type = TIC_COMPUTED;
	}
	/* user spec. is last */ 
	else if (!END_OF_COMMAND) {
	    load_tics(AXIS, tdef);
	}
    }
    if (almost_equals(c_token, nocmd)) {	/* NOSTRING */
	*TICS = NO_TICS;
	c_token++;
	match = 1;
    }
/* other options */

    (void) strcpy(sfxptr, "m$tics");	/* MONTH */
    if (almost_equals(c_token, cmdptr)) {
	if (tdef->type == TIC_USER) {
	    free_marklist(tdef->def.user);
	    tdef->def.user = NULL;
	}
	tdef->type = TIC_MONTH;
	++c_token;
	match = 1;
    }
    if (almost_equals(c_token, nocmd)) {	/* NOMONTH */
	tdef->type = TIC_COMPUTED;
	++c_token;
	match = 1;
    }
    (void) strcpy(sfxptr, "d$tics");	/* DAYS */
    if (almost_equals(c_token, cmdptr)) {
	match = 1;
	if (tdef->type == TIC_USER) {
	    free_marklist(tdef->def.user);
	    tdef->def.user = NULL;
	}
	tdef->type = TIC_DAY;
	++c_token;
    }
    if (almost_equals(c_token, nocmd)) {	/* NODAYS */
	tdef->type = TIC_COMPUTED;
	++c_token;
	match = 1;
    }
    *cmdptr = 'm';
    (void) strcpy(cmdptr + 1, tic_side);
    (void) strcat(cmdptr, "t$ics");	/* MINISTRING */

    if (almost_equals(c_token, cmdptr)) {
	struct value freq;
	c_token++;
	match = 1;
	if (END_OF_COMMAND) {
	    *MTICS = MINI_AUTO;
	} else if (almost_equals(c_token, "def$ault")) {
	    *MTICS = MINI_DEFAULT;
	    ++c_token;
	} else {
	    *FREQ = real(const_express(&freq));
	    *FREQ = floor(*FREQ);
	    *MTICS = MINI_USER;
	}
    }
    if (almost_equals(c_token, nocmd)) {	/* NOMINI */
	*MTICS = FALSE;
	c_token++;
	match = 1;
    }
    return (match);
}


/* process a 'set {x/y/z}label command */
/* set {x/y/z}label {label_text} {x}{,y} */
static void
set_xyzlabel(label)
label_struct *label;
{
    c_token++;
    if (END_OF_COMMAND) {	/* no label specified */
	*label->text = '\0';
	return;
    }
    if (isstring(c_token)) {
	/* We have string specified - grab it. */
	quote_str(label->text, c_token, MAX_LINE_LEN);
	c_token++;
    }
    if (END_OF_COMMAND)
	return;

    if (!almost_equals(c_token, "font") && !isstring(c_token)) {
	/* We have x,y offsets specified */
	struct value a;
	if (!equals(c_token, ","))
	    label->xoffset = real(const_express(&a));

	if (END_OF_COMMAND)
	    return;

	if (equals(c_token, ",")) {
	    c_token++;
	    label->yoffset = real(const_express(&a));
	}
    }
    if (END_OF_COMMAND)
	return;

    /* optional keyword 'font' can go here */

    if (almost_equals(c_token, "f$ont"))
	++c_token;		/* skip it */

    if (!isstring(c_token))
	int_error(c_token, "Expected font");

    quote_str(label->font, c_token, MAX_LINE_LEN);
    c_token++;
}



/* ======================================================== */
/* process a 'set linestyle' command */
/* set linestyle {tag} {linetype n} {linewidth x} {pointtype n} {pointsize x} */
static void
set_linestyle()
{
    struct value a;
    struct linestyle_def *this_linestyle = NULL;
    struct linestyle_def *new_linestyle = NULL;
    struct linestyle_def *prev_linestyle = NULL;
    struct lp_style_type loc_lp;
    int tag;

    c_token++;

    /* Init struct lp_style_type loc_lp */
    reset_lp_properties (&loc_lp);

    /* get tag */
    if (!END_OF_COMMAND) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_linestyle_tag();	/* default next tag */

    /* pick up a line spec : dont allow ls, do allow point type
     * default to same line type = point type = tag
     */
    lp_parse(&loc_lp, 0, 1, tag - 1, tag - 1);

    if (!END_OF_COMMAND)
	int_error(c_token, "extraneous or out-of-order arguments in set linestyle");

    /* OK! add linestyle */
    if (first_linestyle != NULL) {	/* skip to last linestyle */
	for (this_linestyle = first_linestyle; this_linestyle != NULL;
	     prev_linestyle = this_linestyle, this_linestyle = this_linestyle->next)
	    /* is this the linestyle we want? */
	    if (tag <= this_linestyle->tag)
		break;
    }
    if (this_linestyle != NULL && tag == this_linestyle->tag) {
	/* changing the linestyle */
	this_linestyle->lp_properties = loc_lp;
    } else {
	/* adding the linestyle */
	new_linestyle = (struct linestyle_def *)
	    gp_alloc(sizeof(struct linestyle_def), "linestyle");
	if (prev_linestyle != NULL)
	    prev_linestyle->next = new_linestyle;	/* add it to end of list */
	else
	    first_linestyle = new_linestyle;	/* make it start of list */
	new_linestyle->tag = tag;
	new_linestyle->next = this_linestyle;
	new_linestyle->lp_properties = loc_lp;
    }
}

/* assign a new linestyle tag
 * linestyles are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_linestyle_tag()
{
    struct linestyle_def *this;
    int last = 0;		/* previous tag value */

    for (this = first_linestyle; this != NULL; this = this->next)
	if (this->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}

/* delete linestyle from linked list started by first_linestyle.
 * called with pointers to the previous linestyle (prev) and the 
 * linestyle to delete (this).
 * If there is no previous linestyle (the linestyle to delete is
 * first_linestyle) then call with prev = NULL.
 */
void
delete_linestyle(prev, this)
struct linestyle_def *prev, *this;
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous linestyle */
	    prev->next = this->next;
	else			/* this = first_linestyle so change first_linestyle */
	    first_linestyle = this->next;
	free((char *) this);
    }
}

/*
 * auxiliary functions for the `set linestyle` command
 */

void
lp_use_properties(lp, tag, pointflag)
struct lp_style_type *lp;
int tag, pointflag;
{
    /*  This function looks for a linestyle defined by 'tag' and copies
     *  its data into the structure 'lp'.
     *
     *  If 'pointflag' equals ZERO, the properties belong to a linestyle
     *  used with arrows.  In this case no point properties will be
     *  passed to the terminal (cf. function 'term_apply_lp_properties' below).
     */

    struct linestyle_def *this;

    this = first_linestyle;
    while (this != NULL) {
	if (this->tag == tag) {
	    *lp = this->lp_properties;
	    lp->pointflag = pointflag;
	    return;
	} else {
	    this = this->next;
	}
    }

    /* tag not found: */
    int_error(NO_CARET,"linestyle not found", NO_CARET);
}


/* not static; used by command.c */
enum PLOT_STYLE
get_style()
{
    /* defined in plot.h */
    register enum PLOT_STYLE ps;

    c_token++;

    ps = lookup_table(&plotstyle_tbl[0],c_token);

    c_token++;

    if (ps == -1) {
	int_error(c_token,"\
expecting 'lines', 'points', 'linespoints', 'dots', 'impulses',\n\
\t'yerrorbars', 'xerrorbars', 'xyerrorbars', 'steps', 'fsteps',\n\
\t'histeps', 'boxes', 'boxerrorbars', 'boxxyerrorbars', 'vector',\n\
\t'financebars', 'candlesticks', 'errorlines', 'xerrorlines',\n\
\t'yerrorlines', 'xyerrorlines'");
	ps = LINES;
    }

    return ps;
}

/* For set [xy]tics... command */
static void
load_tics(axis, tdef)
int axis;
struct ticdef *tdef;		/* change this ticdef */
{
    if (equals(c_token, "(")) {	/* set : TIC_USER */
	c_token++;
	load_tic_user(axis, tdef);
    } else {			/* series : TIC_SERIES */
	load_tic_series(axis, tdef);
    }
}

/* load TIC_USER definition */
/* (tic[,tic]...)
 * where tic is ["string"] value
 * Left paren is already scanned off before entry.
 */
static void
load_tic_user(axis, tdef)
int axis;
struct ticdef *tdef;
{
    struct ticmark *list = NULL;	/* start of list */
    struct ticmark *last = NULL;	/* end of list */
    struct ticmark *tic = NULL;	/* new ticmark */
    char temp_string[MAX_LINE_LEN];

    while (!END_OF_COMMAND) {
	/* parse a new ticmark */
	tic = (struct ticmark *) gp_alloc(sizeof(struct ticmark), (char *) NULL);
	if (tic == (struct ticmark *) NULL) {
	    free_marklist(list);
	    int_error(c_token, "out of memory for tic mark");
	}
	/* syntax is  (  ['format'] value , ... )
	 * but for timedata, the value itself is a string, which
	 * complicates things somewhat
	 */

	/* has a string with it? */
	if (isstring(c_token) &&
	    (datatype[axis] != TIME || isstring(c_token + 1))) {
	    quote_str(temp_string, c_token, MAX_LINE_LEN);
	    tic->label = gp_alloc(strlen(temp_string) + 1, "tic label");
	    (void) strcpy(tic->label, temp_string);
	    c_token++;
	} else
	    tic->label = NULL;

	/* in any case get the value */
	GET_NUM_OR_TIME(tic->position, axis);
	tic->next = NULL;

	/* append to list */
	if (list == NULL)
	    last = list = tic;	/* new list */
	else {			/* append to list */
	    last->next = tic;
	    last = tic;
	}

	/* expect "," or ")" here */
	if (!END_OF_COMMAND && equals(c_token, ","))
	    c_token++;		/* loop again */
	else
	    break;		/* hopefully ")" */
    }

    if (END_OF_COMMAND || !equals(c_token, ")")) {
	free_marklist(list);
	int_error(c_token, "expecting right parenthesis )");
    }
    c_token++;

    /* successful list */
    if (tdef->type == TIC_USER) {
	/* remove old list */
	/* VAX Optimiser was stuffing up following line. Turn Optimiser OFF */
	free_marklist(tdef->def.user);
	tdef->def.user = NULL;
    }
    tdef->type = TIC_USER;
    tdef->def.user = list;
}

static void
free_marklist(list)
struct ticmark *list;
{
    register struct ticmark *freeable;

    while (list != NULL) {
	freeable = list;
	list = list->next;
	if (freeable->label != NULL)
	    free((char *) freeable->label);
	free((char *) freeable);
    }
}

/* load TIC_SERIES definition */
/* [start,]incr[,end] */
static void
load_tic_series(axis, tdef)
int axis;
struct ticdef *tdef;
{
    double start, incr, end;
    int incr_token;

    GET_NUM_OR_TIME(start, axis);

    if (!equals(c_token, ",")) {
	/* only step specified */
	incr = start;
	start = -VERYLARGE;
	end = VERYLARGE;
    } else {

	c_token++;

	incr_token = c_token;
	GET_NUM_OR_TIME(incr, axis);

	if (END_OF_COMMAND)
	    end = VERYLARGE;
	else {
	    if (!equals(c_token, ","))
		int_error(c_token, "expecting comma to separate incr,end");
	    c_token++;
	    GET_NUM_OR_TIME(end, axis);
	}
	if (!END_OF_COMMAND)
	    int_error(c_token, "tic series is defined by [start,]increment[,end]");

	if (start < end && incr <= 0)
	    int_error(incr_token, "increment must be positive");
	if (start > end && incr >= 0)
	    int_error(incr_token, "increment must be negative");
	if (start > end) {
	    /* put in order */
	    double numtics;
	    numtics = floor((end * (1 + SIGNIF) - start) / incr);
	    end = start;
	    start = end + numtics * incr;
	    incr = -incr;
/*
   double temp = start;
   start = end;
   end = temp;
   incr = -incr;
 */
	}
    }

    if (tdef->type == TIC_USER) {
	/* remove old list */
	/* VAX Optimiser was stuffing up following line. Turn Optimiser OFF */
	free_marklist(tdef->def.user);
	tdef->def.user = NULL;
    }
    tdef->type = TIC_SERIES;
    tdef->def.series.start = start;
    tdef->def.series.incr = incr;
    tdef->def.series.end = end;
}

static void
load_offsets(a, b, c, d)
double *a, *b, *c, *d;
{
    struct value t;

    *a = real(const_express(&t));	/* loff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *b = real(const_express(&t));	/* roff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *c = real(const_express(&t));	/* toff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *d = real(const_express(&t));	/* boff value */
}

TBOOLEAN			/* new value for autosc */
load_range(axis, a, b, autosc)		/* also used by command.c */
int axis;
double *a, *b;
TBOOLEAN autosc;
{
    if (equals(c_token, "]"))
	return (autosc);
    if (END_OF_COMMAND) {
	int_error(c_token, "starting range value or ':' or 'to' expected");
    } else if (!equals(c_token, "to") && !equals(c_token, ":")) {
	if (equals(c_token, "*")) {
	    autosc |= 1;
	    c_token++;
        } else if (equals(c_token, "?")) { /* ULIG */
            *a = get_writeback_min(axis);
            autosc &= 2;
            c_token++;
	} else {
	    GET_NUM_OR_TIME(*a, axis);
	    autosc &= 2;
	}
    }
    if (!equals(c_token, "to") && !equals(c_token, ":"))
	int_error(c_token, "':' or keyword 'to' expected");
    c_token++;
    if (!equals(c_token, "]")) {
	if (equals(c_token, "*")) {
	    autosc |= 2;
	    c_token++;
        } else if (equals(c_token, "?")) { /* ULIG */
            *b = get_writeback_max(axis);
            autosc &= 1;
            c_token++;
	} else {
	    GET_NUM_OR_TIME(*b, axis);
	    autosc &= 1;
	}
    }
    return (autosc);
}

/* return 1 if format looks like a numeric format
 * ie more than one %{efg}, or %something-else
 */

static int
looks_like_numeric(format)
char *format;
{
    if (!(format = strchr(format, '%')))
	return 0;

    while (++format, (*format >= '0' && *format <= '9') || *format == '.');

    return (*format == 'f' || *format == 'g' || *format == 'e');
}


/* parse a position of the form
 *    [coords] x, [coords] y {,[coords] z}
 * where coords is one of first,second.graph,screen
 * if first or second, we need to take datatype into account
 * mixed co-ordinates are for specialists, but it's not particularly
 * hard to implement...
 */

#define GET_NUMBER_OR_TIME(store,axes,axis) \
 do { \
  if (axes >= 0 && datatype[axes+axis] == TIME && isstring(c_token) ) { \
   char ss[80]; \
   struct tm tm; \
   quote_str(ss,c_token, 80); \
   ++c_token; \
   if (gstrptime(ss,timefmt,&tm)) \
    store = (double) gtimegm(&tm); \
  } else { \
   struct value value; \
   store = real(const_express(&value)); \
  } \
 } while(0)


/* get_position_type - for use by get_position().
 * parses first/second/graph/screen keyword
 */

static void
get_position_type(type, axes)
enum position_type *type;
int *axes;
{
    if (almost_equals(c_token, "fir$st")) {
	++c_token;
	*type = first_axes;
    } else if (almost_equals(c_token, "sec$ond")) {
	++c_token;
	*type = second_axes;
    } else if (almost_equals(c_token, "gr$aph")) {
	++c_token;
	*type = graph;
    } else if (almost_equals(c_token, "sc$reen")) {
	++c_token;
	*type = screen;
    }
    switch (*type) {
    case first_axes:
	*axes = FIRST_AXES;
	return;
    case second_axes:
	*axes = SECOND_AXES;
	return;
    default:
	*axes = (-1);
	return;
    }
}

/* get_position() - reads a position for label,arrow,key,... */

static void
get_position(pos)
struct position *pos;
{
    int axes;
    enum position_type type = first_axes;

    get_position_type(&type, &axes);
    pos->scalex = type;
    GET_NUMBER_OR_TIME(pos->x, axes, FIRST_X_AXIS);
    if (!equals(c_token, ","))
	int_error(c_token, "Expected comma");
    ++c_token;
    get_position_type(&type, &axes);
    pos->scaley = type;
    GET_NUMBER_OR_TIME(pos->y, axes, FIRST_Y_AXIS);

    /* z is not really allowed for a screen co-ordinate, but keep it simple ! */
    if (equals(c_token, ",")) {
	++c_token;
	get_position_type(&type, &axes);
	pos->scalez = type;
	GET_NUMBER_OR_TIME(pos->z, axes, FIRST_Z_AXIS);
    } else {
	pos->z = 0;
	pos->scalez = type;	/* same as y */
    }
}

static void
set_lp_properties(arg, allow_points, lt, pt, lw, ps)
struct lp_style_type *arg;
int allow_points, lt, pt;
double lw, ps;
{
    arg->pointflag = allow_points;
    arg->l_type = lt;
    arg->p_type = pt;
    arg->l_width = lw;
    arg->p_size = ps;
}

static void
reset_lp_properties(arg)
struct lp_style_type *arg;
{
    /* See plot.h for struct lp_style_type */
    arg->pointflag = arg->l_type = arg->p_type = 0;
    arg->l_width = arg->p_size = 1.0;
#ifdef PM3D
    arg->use_palette = 0;
#endif
}


/* was a macro in plot.h */
void
lp_parse(lp, allow_ls, allow_point, def_line, def_point)
struct lp_style_type *lp;
int allow_ls, allow_point, def_line, def_point;
{
    struct value t;

    if (allow_ls && (almost_equals(c_token, "lines$tyle") ||
		     equals(c_token, "ls" ))) {
	c_token++;
	lp_use_properties(lp, (int) real(const_express(&t)), allow_point);
    } else {
	if (almost_equals(c_token, "linet$ype") || equals(c_token, "lt" )) {
	    c_token++;
#ifdef PM3D
	    if (almost_equals(c_token, "pal$ette")) {
		lp->use_palette = 1;
		lp->l_type = def_line;
		++c_token;
	    } else {
		lp->use_palette = 0;
#endif
		lp->l_type = (int) real(const_express(&t))-1;
#ifdef PM3D
	    }
#endif
	} else lp->l_type = def_line;
	if (almost_equals(c_token, "linew$idth") || equals(c_token, "lw" )) {
	    c_token++;
	    lp->l_width = real(const_express(&t));
	} else lp->l_width = 1.0;
	if ( (lp->pointflag = allow_point) != 0) {
	    if (almost_equals(c_token, "pointt$ype") ||
		equals(c_token, "pt" )) {
		c_token++;
		lp->p_type = (int) real(const_express(&t))-1;
	    } else lp->p_type = def_point;
	    if (almost_equals(c_token, "points$ize") ||
		equals(c_token, "ps" )) {
		c_token++;
		lp->p_size = real(const_express(&t));
	    } else lp->p_size = pointsize; /* as in "set pointsize" */
	} else lp->p_size = pointsize; /* give it a value */
	LP_DUMP(lp);
    }
}

/*
 * Backwards compatibility ...
 */
static void set_nolinestyle()
{
    struct value a;
    struct linestyle_def *this, *prev;
    int tag;

    if (END_OF_COMMAND) {
        /* delete all linestyles */
        while (first_linestyle != NULL)
            delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
    } else {
        /* get tag */
        tag = (int) real(const_express(&a));
        if (!END_OF_COMMAND)
            int_error(c_token, "extraneous arguments to set nolinestyle");
        for (this = first_linestyle, prev = NULL;
             this != NULL;
             prev = this, this = this->next) {
            if (this->tag == tag) {
                delete_linestyle(prev, this);
                return;         /* exit, our job is done */
            }
        }
        int_error(c_token, "linestyle not found");
    }
}


/*
 * get and set routines for range writeback
 * ULIG *
 */

static double rm_log __PROTO((int axis, double val));

double get_writeback_min(axis)
int axis;
{
    /* printf("get min(%d)=%g\n",axis,writeback_min[axis]); */
    return writeback_min[axis];
}

double get_writeback_max(axis)
int axis;
{
    /* printf("get max(%d)=%g\n",axis,writeback_min[axis]); */
    return writeback_max[axis];
}

void set_writeback_min(axis, val)
int axis;
double val;
{
    val = rm_log(axis,val);
    /* printf("set min(%d)=%g\n",axis,val); */
    writeback_min[axis] = val;
}

void set_writeback_max(axis, val)
int axis;
double val;
{
    val = rm_log(axis,val);
    /* printf("set max(%d)=%g\n",axis,val); */
    writeback_max[axis] = val;
}

double rm_log(axis, val)
int axis;
double val;
{
    TBOOLEAN islog;
    double logbase = 0;

    /* check whether value is in logscale */
    switch( axis ) {
    case FIRST_X_AXIS:
	logbase = base_log_x;
	islog = is_log_x;
	break;
    case FIRST_Y_AXIS:
	logbase = base_log_y;
	islog = is_log_y;
	break;
    case FIRST_Z_AXIS:
	logbase = base_log_z;
	islog = is_log_z;
	break;
    case SECOND_X_AXIS:
	logbase = base_log_y2;
	islog = is_log_y2;
	break;
    default:
	islog = FALSE;
    }
  
    if(!islog)
	return val;

    /* remove logscale from value */
    return pow(logbase,val);
}
