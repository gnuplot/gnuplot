#ifndef lint
static char *RCSid = "$Id: set.c,v 1.68 1998/06/22 12:24:54 ddenholm Exp $";
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

#include "plot.h"
#include "stdfn.h"
#include "setshow.h"
#include "national.h"
#include "alloc.h"

#define DEF_FORMAT   "%g"	/* default format for tic mark labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */

/*
 * global variables to hold status of 'set' options
 *
 * IMPORTANT NOTE:
 * ===============
 * If you change the default values of one of the variables below, or if
 * you add another global variable, make sure that the change you make is
 * done in reset_command() as well (if that makes sense).
 */

TBOOLEAN autoscale_r = DTRUE;
TBOOLEAN autoscale_t = DTRUE;
TBOOLEAN autoscale_u = DTRUE;
TBOOLEAN autoscale_v = DTRUE;
TBOOLEAN autoscale_x = DTRUE;
TBOOLEAN autoscale_y = DTRUE;
TBOOLEAN autoscale_z = DTRUE;
TBOOLEAN autoscale_x2 = DTRUE;
TBOOLEAN autoscale_y2 = DTRUE;
TBOOLEAN autoscale_lt = DTRUE;
TBOOLEAN autoscale_lu = DTRUE;
TBOOLEAN autoscale_lv = DTRUE;
TBOOLEAN autoscale_lx = DTRUE;
TBOOLEAN autoscale_ly = DTRUE;
TBOOLEAN autoscale_lz = DTRUE;
TBOOLEAN multiplot = FALSE;

double boxwidth = -1.0;		/* box width (automatic) */
TBOOLEAN clip_points = FALSE;
TBOOLEAN clip_lines1 = TRUE;
TBOOLEAN clip_lines2 = FALSE;
struct lp_style_type border_lp = { 0, -2, 0, 1.0, 1.0 };
int draw_border = 31;
TBOOLEAN draw_surface = TRUE;
char dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };
char default_font[MAX_ID_LEN+1] = "";	/* Entry font added by DJL */
char xformat[MAX_ID_LEN+1] = DEF_FORMAT;
char yformat[MAX_ID_LEN+1] = DEF_FORMAT;
char zformat[MAX_ID_LEN+1] = DEF_FORMAT;
char x2format[MAX_ID_LEN+1] = DEF_FORMAT;
char y2format[MAX_ID_LEN+1] = DEF_FORMAT;

/* do formats look like times - use FIRST_X_AXIS etc as index
 * - never saved or shown ...
 */
#if AXIS_ARRAY_SIZE != 10
# error error in initialiser for format_is_numeric
#endif

int format_is_numeric[AXIS_ARRAY_SIZE] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

enum PLOT_STYLE data_style = POINTSTYLE;
enum PLOT_STYLE func_style = LINES;
double bar_size = 1.0;
struct lp_style_type work_grid = { 0, GRID_OFF, 0, 1.0, 1.0 };
struct lp_style_type grid_lp   = { 0, -1, 0, 1.0, 1.0 };
struct lp_style_type mgrid_lp  = { 0, -1, 0, 1.0, 1.0 };
double polar_grid_angle = 0;	/* nonzero means a polar grid */
int key = -1;			/* default position */
struct position key_user_pos;	/* user specified position for key */
TBOOLEAN key_reverse = FALSE;	/* reverse text & sample ? */
struct lp_style_type key_box = { 0, -3, 0, 1.0, 1.0 };		/* -3 = no linetype */
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
FILE *gpoutfile;
char *outstr = NULL;		/* means "STDOUT" */
TBOOLEAN parametric = FALSE;
double pointsize = 1.0;
int encoding;
char *encoding_names[] = { "default", "iso_8859_1", "cp437", "cp850", NULL };
TBOOLEAN polar = FALSE;
TBOOLEAN hidden3d = FALSE;
TBOOLEAN label_contours = TRUE;	/* different linestyles are used for contours when set */
char contour_format[32] = "%8.3g";	/* format for contour key entries */
int angles_format = ANGLES_RADIANS;
double ang2rad = 1.0;		/* 1 or pi/180, tracking angles_format */
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
label_struct title = { "", 0.0, 0.0, "" };
label_struct timelabel = { "", 0.0, 0.0, "" };
label_struct xlabel = { "", 0.0, 0.0, "" };
label_struct ylabel = { "", 0.0, 0.0, "" };
label_struct zlabel = { "", 0.0, 0.0, "" };
label_struct x2label = { "", 0.0, 0.0, "" };
label_struct y2label = { "", 0.0, 0.0, "" };

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
int max_levels = 0;		/* contour level capacity, before enlarging */

int dgrid3d_row_fineness = 10;
int dgrid3d_col_fineness = 10;
int dgrid3d_norm_value = 1;
TBOOLEAN dgrid3d = FALSE;

struct lp_style_type xzeroaxis = { 0, -3, 0, 1.0, 1.0 };
struct lp_style_type yzeroaxis = { 0, -3, 0, 1.0, 1.0 };
struct lp_style_type x2zeroaxis = { 0, -3, 0, 1.0, 1.0 };
struct lp_style_type y2zeroaxis = { 0, -3, 0, 1.0, 1.0 };

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

struct ticdef xticdef = { TIC_COMPUTED };
struct ticdef yticdef = { TIC_COMPUTED };
struct ticdef zticdef = { TIC_COMPUTED };
struct ticdef x2ticdef = { TIC_COMPUTED };
struct ticdef y2ticdef = { TIC_COMPUTED };

TBOOLEAN tic_in = TRUE;

struct text_label *first_label = NULL;
struct arrow_def *first_arrow = NULL;
struct linestyle_def *first_linestyle = NULL;

int lmargin = -1;		/* space between left edge and xleft in chars (-1: computed) */
int bmargin = -1;		/* space between bottom and ybot in chars (-1: computed) */
int rmargin = -1;		/* space between right egde and xright in chars (-1: computed) */
int tmargin = -1;		/* space between top egde and ytop in chars (-1: computed) */

/* string representing missing values in ascii datafiles */
char *missing_val = NULL;

/* date&time language conversions */
				 /* extern struct dtconv *dtc; *//* HBB 980317: unused and not defined anywhere !? */

/*** other things we need *****/

/* input data, parsing variables */

extern TBOOLEAN is_3d_plot;

/* From plot2d.c */
extern struct curve_points *first_plot;
/* From plot3d.c */
extern struct surface_points *first_3dplot;

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

char cur_locale[MAX_ID_LEN+1] = "C";

/* not set or shown directly, but controlled by 'set locale'
 * defined in national.h
 */

char full_month_names[12][32] =
{ FMON01, FMON02, FMON03, FMON04, FMON05, FMON06, FMON07, FMON08, FMON09, FMON10, FMON11, FMON12 };
char abbrev_month_names[12][8] =
{ AMON01, AMON02, AMON03, AMON04, AMON05, AMON06, AMON07, AMON08, AMON09, AMON10, AMON11, AMON12 };

char full_day_names[7][32] =
{ FDAY0, FDAY1, FDAY2, FDAY3, FDAY4, FDAY5, FDAY6 };
char abbrev_day_names[7][8] =
{ ADAY0, ADAY1, ADAY2, ADAY3, ADAY4, ADAY5, ADAY6 };



/******** Local functions ********/
static void get_position __PROTO((struct position * pos));
static void get_position_type __PROTO((enum position_type * type, int *axes));
static void set_xyzlabel __PROTO((label_struct * label));
static void set_label __PROTO((void));
static void set_nolabel __PROTO((void));
static void set_arrow __PROTO((void));
static void set_noarrow __PROTO((void));
static void load_tics __PROTO((int axis, struct ticdef * tdef));
static void load_tic_user __PROTO((int axis, struct ticdef * tdef));
static void free_marklist __PROTO((struct ticmark * list));
static void load_tic_series __PROTO((int axis, struct ticdef * tdef));
static void load_offsets __PROTO((double *a, double *b, double *c, double *d));
static void delete_label __PROTO((struct text_label * prev, struct text_label * this));
static int assign_label_tag __PROTO((void));
static void delete_arrow __PROTO((struct arrow_def * prev, struct arrow_def * this));
static int assign_arrow_tag __PROTO((void));
static void set_linestyle __PROTO((void));
static void set_nolinestyle __PROTO((void));
static int assign_linestyle_tag __PROTO((void));
static void delete_linestyle __PROTO((struct linestyle_def * prev, struct linestyle_def * this));
static TBOOLEAN set_one __PROTO((void));
static TBOOLEAN set_two __PROTO((void));
static TBOOLEAN set_three __PROTO((void));
static int looks_like_numeric __PROTO((char *));
static void set_lp_properties __PROTO((struct lp_style_type * arg, int allow_points, int lt, int pt, double lw, double ps));
static void reset_lp_properties __PROTO((struct lp_style_type *arg));
static void set_locale __PROTO((char *));

static int set_tic_prop __PROTO((int *TICS, int *MTICS, double *FREQ,
     struct ticdef * tdef, int AXIS, TBOOLEAN * ROTATE, char *tic_side));


/* following code segment appears over and over again */
#define GET_NUM_OR_TIME(store,axis) \
do{if ( datatype[axis] == TIME && isstring(c_token) ) { \
    char ss[80]; struct tm tm; \
    quote_str(ss,c_token, 80); ++c_token; \
    if (gstrptime(ss,timefmt,&tm)) store = (double) gtimegm(&tm); else store = 0;\
   } else {\
    struct value value; \
    store = real(const_express(&value));\
  }}while(0)

/******** The 'reset' command ********/
void reset_command()
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
    autoscale_lt = DTRUE;
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
    set_lp_properties(&work_grid, 0, GRID_OFF, 0, 0.5, 1.0);
    set_lp_properties(&grid_lp, 0, -1, 0, 0.5, 1.0);
    set_lp_properties(&mgrid_lp, 0, -1, 0, 0.5, 1.0);
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
    xtics =
	ytics = TICS_ON_BORDER | TICS_MIRROR;
    ztics = TICS_ON_BORDER;	/* no mirror by default */
    x2tics = NO_TICS;
    y2tics = NO_TICS;
    mxtics =
	mytics =
	mztics =
	mx2tics =
	my2tics = MINI_DEFAULT;
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
    lmargin =
	bmargin =
	rmargin =
	tmargin = -1;		/* autocomputed */
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
    encoding = ENCODING_DEFAULT;

    set_locale("C");		/* default */
}

/******** The 'set' command ********/
void set_command()
{
    static char GPFAR setmess[] = "\
valid set options:  [] = choose one, {} means optional\n\n\
\t'angles',  '{no}arrow',  '{no}autoscale',  'bars',  '{no}border',\n\
\t'boxwidth', '{no}clabel', '{no}clip', 'cntrparam', '{no}contour',\n\
\t'data style',  '{no}dgrid3d',  'dummy',  'encoding',  'format',\n\
\t'function style',   '{no}grid',   '{no}hidden3d',   'isosamples',\n\
\t'{no}key', '{no}label', '{no}linestyle', 'locale', '{no}logscale',\n\
\t'[blrt]margin', 'mapping', 'missing', '{no}multiplot', 'offsets',\n\
\t'origin', 'output', '{no}parametric', 'pointsize', '{no}polar',\n\
\t'[rtuv]range',  'samples',  'size',  '{no}surface',  'terminal',\n\
\t'tics',  'ticscale',  'ticslevel',  '{no}timestamp',  'timefmt',\n\
\t'title', 'view', '[xyz]{2}data', '[xyz]{2}label', '[xyz]{2}range',\n\
\t'{no}{m}[xyz]{2}tics', '[xyz]{2}[md]tics', '{no}{[xyz]{2}}zeroaxis',\n\
\t'zero'";

    c_token++;

    if (!set_one() && !set_two() && !set_three())
	int_error(setmess, c_token);
}

/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
set_one()
{
/* save on replication with a macro */
#define PROCESS_AUTO_LETTER(AUTO, STRING,MIN,MAX) \
else if (equals(c_token, STRING))       { AUTO = DTRUE; ++c_token; } \
else if (almost_equals(c_token, MIN)) { AUTO |= 1;    ++c_token; } \
else if (almost_equals(c_token, MAX)) { AUTO |= 2;    ++c_token; }

    if (max_levels == 0)
	levels_list = (double *)gp_alloc((max_levels = 5)*sizeof(double), 
					 "contour levels");

    if (almost_equals(c_token, "ar$row")) {
	c_token++;
	set_arrow();
    } else if (almost_equals(c_token, "noar$row")) {
	c_token++;
	set_noarrow();
    } else if (almost_equals(c_token, "au$toscale")) {
	c_token++;
	if (END_OF_COMMAND) {
	    autoscale_r = autoscale_t = autoscale_x = autoscale_y = autoscale_z = autoscale_x2 = autoscale_y2 = DTRUE;
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
	    int_error("Invalid range", c_token);
    } else if (almost_equals(c_token,"noau$toscale")) {
	c_token++;
	if (END_OF_COMMAND) {
	    autoscale_r = autoscale_t = autoscale_x = autoscale_y = autoscale_z = FALSE;
	} else if (equals(c_token, "xy") || equals(c_token, "tyx")) {
	    autoscale_x = autoscale_y = FALSE;
	    c_token++;
	} else if (equals(c_token, "r")) {
	    autoscale_r = FALSE;
	    c_token++;
	} else if (equals(c_token, "t")) {
	    autoscale_t = FALSE;
	    c_token++;
	} else if (equals(c_token, "u")) {
	    autoscale_u = FALSE;
	    c_token++;
	} else if (equals(c_token, "v")) {
	    autoscale_v = FALSE;
	    c_token++;
	} else if (equals(c_token, "x")) {
	    autoscale_x = FALSE;
	    c_token++;
	} else if (equals(c_token, "y")) {
	    autoscale_y = FALSE;
	    c_token++;
	} else if (equals(c_token, "x2")) {
	    autoscale_x2 = FALSE;
	    c_token++;
	} else if (equals(c_token, "y2")) {
	      autoscale_y2 = FALSE;
	      c_token++;
	} else if (equals(c_token, "z")) {
	    autoscale_z = FALSE;
	    c_token++;
	}
    } else if (almost_equals(c_token,"nobor$der")) {
	draw_border = 0;
	c_token++;
	}
    else if (almost_equals(c_token,"box$width")) {
	struct value a;
	c_token++;
	if (END_OF_COMMAND)
	    boxwidth = -1.0;
	else
/*              if((boxwidth = real(const_express(&a))) != -2.0)*/
/*                      boxwidth = magnitude(const_express(&a));*/
	    boxwidth = real(const_express(&a));
    } else if (almost_equals(c_token,"c$lip")) {
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
	    int_error("expecting 'points', 'one', or 'two'", c_token);
	c_token++;
    } else if (almost_equals(c_token,"noc$lip")) {
	c_token++;
	if (END_OF_COMMAND) {
	    /* same as all three */
	    clip_points = FALSE;
	    clip_lines1 = FALSE;
	    clip_lines2 = FALSE;
	} else if (almost_equals(c_token, "p$oints"))
	    clip_points = FALSE;
	else if (almost_equals(c_token, "o$ne"))
	    clip_lines1 = FALSE;
	else if (almost_equals(c_token, "t$wo"))
	    clip_lines2 = FALSE;
	else
	    int_error("expecting 'points', 'one', or 'two'", c_token);
	c_token++;
    } else if (almost_equals(c_token,"hi$dden3d")) {
#ifdef LITE
	printf(" Hidden Line Removal Not Supported in LITE version\n");
	c_token++;
#else
	/* HBB 970618: new parsing engine for hidden3d options */
	set_hidden3doptions();
	hidden3d = TRUE;
#endif /* LITE */
    } else if (almost_equals(c_token,"nohi$dden3d")) {
#ifdef LITE
	printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
	hidden3d = FALSE;
#endif /* LITE */
	c_token++;
    } else if (almost_equals(c_token,"cla$bel")) {
	label_contours = TRUE;
	c_token++;
	if (isstring(c_token))
	    quote_str(contour_format, c_token++, 30);
    } else if (almost_equals(c_token,"nocla$bel")) {
	label_contours = FALSE;
	c_token++;
    } else if (almost_equals(c_token,"ma$pping3d")) {
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
	    int_error("expecting 'cartesian', 'spherical', or 'cylindrical'", c_token);
	c_token++;
    } else if (almost_equals(c_token,"co$ntour")) {
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
		int_error("expecting 'base', 'surface', or 'both'", c_token);
	    c_token++;
	}
    } else if (almost_equals(c_token,"noco$ntour")) {
	c_token++;
	draw_contour = CONTOUR_NONE;
    } else if (almost_equals(c_token,"cntrp$aram")) {
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
		    int_error("expecting discrete level", c_token);
		else
		    levels_list[i++] = real(const_express(&a));

		while(!END_OF_COMMAND) {
		    if (!equals(c_token, ","))
			int_error("expecting comma to separate discrete levels", c_token);
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
		    int_error("expecting comma to separate start,incr levels", c_token);
		c_token++;
		if((levels_list[i++] = real(const_express(&a))) == 0)
		    int_error("increment cannot be 0", c_token);
		if(!END_OF_COMMAND) {
		    if (!equals(c_token, ","))
			int_error("expecting comma to separate incr,stop levels", c_token);
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
		    int_error("Levels type is discrete, ignoring new number of contour levels", c_token);
		contour_levels = (int) real(const_express(&a));
	    }
	} else if (almost_equals(c_token, "o$rder")) {
	    int order;
	    c_token++;
	    order = (int) real(const_express(&a));
	    if ( order < 2 || order > 10 )
		int_error("bspline order must be in [2..10] range.", c_token);
		contour_order = order;
	} else
	    int_error("expecting 'linear', 'cubicspline', 'bspline', 'points', 'levels' or 'order'", c_token);
    } else if (almost_equals(c_token,"da$ta")) {
	c_token++;
	if (!almost_equals(c_token,"s$tyle"))
	    int_error("expecting keyword 'style'",c_token);
	data_style = get_style();
    } else if (almost_equals(c_token,"dg$rid3d")) {
	int i;
	TBOOLEAN was_comma = TRUE;
	int local_vals[3];
	struct value a;

	local_vals[0] = dgrid3d_row_fineness;
	local_vals[1] = dgrid3d_col_fineness;
	local_vals[2] = dgrid3d_norm_value;
	c_token++;
	for (i = 0; i < 3 && !(END_OF_COMMAND);) {
	    if (equals(c_token,",")) {
		if (was_comma) i++;
		    was_comma = TRUE;
		c_token++;
	    } else {
		if (!was_comma)
		    int_error("',' expected",c_token);
		local_vals[i] = real(const_express(&a));
		i++;
		was_comma = FALSE;
	    }
	}

	if (local_vals[0] < 2 || local_vals[0] > 1000)
	    int_error("Row size must be in [2:1000] range; size unchanged",
		      c_token);
	if (local_vals[1] < 2 || local_vals[1] > 1000)
	    int_error("Col size must be in [2:1000] range; size unchanged",
		      c_token);
	if (local_vals[2] < 1 || local_vals[2] > 100)
	    int_error("Norm must be in [1:100] range; norm unchanged", c_token);

	dgrid3d_row_fineness = local_vals[0];
	dgrid3d_col_fineness = local_vals[1];
	dgrid3d_norm_value = local_vals[2];
	dgrid3d = TRUE;
    } else if (almost_equals(c_token,"nodg$rid3d")) {
	c_token++;
	dgrid3d = FALSE;
    } else if (almost_equals(c_token,"mis$sing")) {
	c_token++;
	if (END_OF_COMMAND) {
	    if (missing_val)
		free(missing_val);
	    missing_val = NULL;
	} else {
	    if (!isstring(c_token))
		int_error("Expected missing-value string", c_token);
	    m_quote_capture(&missing_val, c_token, c_token);
	    c_token++;
	}
    } else if (almost_equals(c_token,"nomis$sing")) {
	++c_token;
	if (missing_val)
	    free(missing_val);
	missing_val = NULL;
    } else if (almost_equals(c_token,"du$mmy")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error("expecting dummy variable name", c_token);
	else {
	    if (!equals(c_token,","))
		copy_str(dummy_var[0],c_token++, MAX_ID_LEN);
	    if (!END_OF_COMMAND && equals(c_token,",")) {
		c_token++;
		if (END_OF_COMMAND)
		    int_error("expecting second dummy variable name", c_token);
		copy_str(dummy_var[1],c_token++, MAX_ID_LEN);
	    }
	}
    } else if (almost_equals(c_token,"fo$rmat")) {
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
		int_error("expecting format string",c_token);
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
    } else if (almost_equals(c_token,"fu$nction")) {
	c_token++;
	if (!almost_equals(c_token,"s$tyle"))
	    int_error("expecting keyword 'style'",c_token);
	func_style = get_style();
    } else if (almost_equals(c_token,"la$bel")) {
	c_token++;
	set_label();
    } else if (almost_equals(c_token,"nola$bel")) {
	c_token++;
	set_nolabel();
    } else if (almost_equals(c_token,"li$nestyle") || equals(c_token, "ls" )) {
	c_token++;
	set_linestyle();
    } else if (almost_equals(c_token,"noli$nestyle") || equals(c_token, "nols" )) {
	c_token++;
	set_nolinestyle();
    } else if (almost_equals(c_token,"lo$gscale")) {
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
		    int_error("log base must be >= 1.1; logscale unchanged",
		c_token);
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
    } else if (almost_equals(c_token,"nolo$gscale")) {
	c_token++;
	if (END_OF_COMMAND) {
	    is_log_x = is_log_y = is_log_z = is_log_x2 = is_log_y2 = FALSE;
	} else if (equals(c_token, "x2")) {
	    is_log_x2 = FALSE; ++c_token;
	} else if (equals(c_token, "y2")) {
	    is_log_y2 = FALSE; ++c_token;
	} else {
	    if (chr_in_str(c_token, 'x')) {
		is_log_x = FALSE;
		base_log_x = 0.0;
		log_base_log_x = 0.0;
	    }
	    if (chr_in_str(c_token, 'y')) {
		is_log_y = FALSE;
		base_log_y = 0.0;
		log_base_log_y = 0.0;
	    }
	    if (chr_in_str(c_token, 'z')) {
		is_log_z = FALSE;
		base_log_z = 0.0;
		log_base_log_z = 0.0;
	    }
	    c_token++;
	}
    } else if (almost_equals(c_token,"of$fsets")) {
	c_token++;
	if (END_OF_COMMAND) {
	    loff = roff = toff = boff = 0.0;  /* Reset offsets */
	} else {
	    load_offsets (&loff,&roff,&toff,&boff);
	}
    } else if (almost_equals(c_token, "noof$fsets")) {
	loff = roff = toff = boff = 0.0;
	++c_token;
    } else if(almost_equals(c_token,"b$ars")) {
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
	    struct value a;
	    bar_size = real(const_express(&a));
	}
    } else if (almost_equals(c_token, "nob$ars")) {
	++c_token;
	bar_size = 0.0;
    } else if (almost_equals(c_token, "enco$ding")) {
	c_token++;
	if(END_OF_COMMAND) {
	    encoding = ENCODING_DEFAULT;
	} else if (almost_equals(c_token,"def$ault")) {
	    c_token++;
	    encoding = ENCODING_DEFAULT;
	} else if (almost_equals(c_token,"iso$_8859_1")) {
	    c_token++;
	    encoding = ENCODING_ISO_8859_1;
	} else if (almost_equals(c_token,"cp4$37")) {
	    c_token++;
	    encoding = ENCODING_CP_437;
	} else if (almost_equals(c_token,"cp8$50")) {
	    c_token++;
	    encoding = ENCODING_CP_850;
	} else {
	    int_error("expecting one of 'default', 'iso_8859_1', 'cp437' or 'cp850'", c_token);
	}
    } else
	return(FALSE);  /* no command match */

    return(TRUE);
}


/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
set_two()
{
    if (almost_equals(c_token,"o$utput")) {
	if (multiplot) {
	    int_error("you can't change the output in multiplot mode", c_token);
	}

	c_token++;
	if (END_OF_COMMAND) {	/* no file specified */
 	    term_set_output(NULL);
 	    if (outstr) {
		free(outstr);
		outstr = NULL; /* means STDOUT */
	    }
	} else if (!isstring(c_token)) {
	    int_error("expecting filename",c_token);
	} else {
	    /* on int_error, we'd like to remember that this is allocated */
	    static char *testfile = NULL;
	    m_quote_capture(&testfile,c_token, c_token); /* reallocs store */
	    gp_expand_tilde(&testfile,strlen(testfile));
	    /* Skip leading whitespace */
	    while (isspace((int)*testfile))
		testfile++;
	    ++c_token;
	    term_set_output(testfile);
	    /* if we get here then it worked, and outstr now = testfile */
	    testfile = NULL;
	}
    } else if (almost_equals(c_token,"origin")) {
	struct value s;
	c_token++;
	if (END_OF_COMMAND) {
	    xoffset = 0.0;
	    yoffset = 0.0;
	} else {
	    xoffset = real(const_express(&s));
	    if (!equals(c_token,","))
		int_error("',' expected",c_token);
	    c_token++;
	    yoffset = real(const_express(&s));
	} 
    } else if (almost_equals(c_token,"tit$le")) {
	set_xyzlabel(&title);
    } else if (almost_equals(c_token,"xl$abel")) {
	set_xyzlabel(&xlabel);
    } else if (almost_equals(c_token,"yl$abel")) {
	set_xyzlabel(&ylabel);
    } else if (almost_equals(c_token,"zl$abel")) {
	set_xyzlabel(&zlabel);
    } else if (almost_equals(c_token,"x2l$abel")) {
	set_xyzlabel(&x2label);
    } else if (almost_equals(c_token,"y2l$abel")) {
	set_xyzlabel(&y2label);
    } else if (almost_equals(c_token,"keyt$itle")) {
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
    } else if (almost_equals(c_token, "nokeyt$itle")) {
	++c_token;
	*key_title = 0;
    } else if (almost_equals(c_token,"timef$mt")) {
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
    } else if (almost_equals(c_token,"loc$ale")) {
	c_token++;
	if (END_OF_COMMAND) {
	    set_locale("C");
	} else if (isstring(c_token)) {
	    char ss[MAX_ID_LEN+1];
	    quote_str(ss,c_token,MAX_ID_LEN);
	    set_locale(ss);
	    ++c_token;
	} else {
	    int_error("Expected string", c_token);
	}
    }

#define DO_ZEROAX(variable, string,neg) \
else if (almost_equals(c_token, string)) { \
   ++c_token; if (END_OF_COMMAND) variable.l_type = -1; \
   else { \
      struct value a; \
      int old_token = c_token;\
      LP_PARSE(variable,1,0,-1,0); \
      if (old_token == c_token) \
         variable.l_type = real(const_express(&a)) - 1; \
   }\
} else if (almost_equals(c_token, neg)) { \
   ++c_token; variable.l_type = -3; \
}

    DO_ZEROAX(xzeroaxis, "xzero$axis", "noxzero$axis")
    DO_ZEROAX(yzeroaxis, "yzero$axis", "noyzero$axis")
    DO_ZEROAX(x2zeroaxis, "x2zero$axis", "nox2zero$axis")
    DO_ZEROAX(y2zeroaxis, "y2zero$axis", "noy2zero$axis")

    else if (almost_equals(c_token,"zeroa$xis")) {
	c_token++;
	LP_PARSE(xzeroaxis,1,0,-1,0);
	memcpy(&yzeroaxis,&xzeroaxis,sizeof(struct lp_style_type));
    } else if (almost_equals(c_token,"nozero$axis")) {
	c_token++;
	xzeroaxis.l_type  = -3;
	yzeroaxis.l_type  = -3;
	x2zeroaxis.l_type = -3;
	y2zeroaxis.l_type = -3;
    } else if (almost_equals(c_token,"par$ametric")) {
	if (!parametric) {
	    parametric = TRUE;
	    if (!polar) { /* already done for polar */
		strcpy (dummy_var[0], "t");
		strcpy (dummy_var[1], "y");
		if (interactive)
		     (void) fprintf(stderr,"\n\tdummy variable is t for curves, u/v for surfaces\n");
	    }
	}
	c_token++;
    } else if (almost_equals(c_token,"nopar$ametric")) {
	if (parametric) {
	    parametric = FALSE;
	    if (!polar) { /* keep t for polar */
		strcpy (dummy_var[0], "x");
		strcpy (dummy_var[1], "y");
		if (interactive)
		   (void) fprintf(stderr,"\n\tdummy variable is x for curves, x/y for surfaces\n");
	    }
	}
	c_token++;
    } else if (almost_equals(c_token, "poi$ntsize")) {
	struct value a;
	c_token++;
	if (END_OF_COMMAND)
	   pointsize = 1.0;
	else
	    pointsize = real(const_express(&a));
	if(pointsize <= 0) pointsize = 1;
    } else if (almost_equals(c_token,"pol$ar")) {
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
	c_token++;
    } else if (almost_equals(c_token,"nopo$lar")) {
	if (polar) {
	    polar = FALSE;
	    if (parametric && autoscale_t) {
		/* only if user has not set an explicit range */
		tmin = -5.0;
		tmax = 5.0;
	    }
	    if (!parametric) {
		strcpy (dummy_var[0], "x");
		if (interactive)
		    (void) fprintf(stderr,"\n\tdummy variable is x for curves\n");
	    }
	}
	c_token++;
    } else if (almost_equals(c_token,"an$gles")) {
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
	    int_error("expecting 'radians' or 'degrees'", c_token);

	if (polar && autoscale_t) {
	    /* set trange if in polar mode and no explicit range */
	    tmin = 0;
	    tmax = 2 * M_PI / ang2rad;
	}
    }

#define GRID_MATCH(string, neg, mask) \
if (almost_equals(c_token, string)) { work_grid.l_type |= mask; ++c_token; } \
else if (almost_equals(c_token, neg)) { work_grid.l_type &= ~(mask); ++c_token; }

    else if (almost_equals(c_token,"g$rid")) {
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

	    LP_PARSE(grid_lp,1,0,-1,1);
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
		LP_PARSE(mgrid_lp,1,0,-1,1);
		if (c_token == old_token) {
		    mgrid_lp.l_type = real(const_express(&a)) -1;
		}
	    }

	    if (!work_grid.l_type)
		work_grid.l_type = GRID_X|GRID_Y;
	    /* probably just  set grid <linetype> */
	}

    } else if (almost_equals(c_token,"nog$rid")) {
	work_grid.l_type = GRID_OFF;
	c_token++;
    } else if (almost_equals(c_token,"su$rface")) {
	draw_surface = TRUE;
	c_token++;
    } else if (almost_equals(c_token,"nosu$rface")) {
	draw_surface = FALSE;
	c_token++;
    } else if (almost_equals(c_token,"bor$der")) {
	struct value a;
	c_token++;
	if(END_OF_COMMAND){
	    draw_border = 31;
	} else {
	    draw_border = (int)real(const_express(&a));
	}
	/* HBB 980609: add linestyle handling for 'set border...' */
	/* For now, you have to give a border bitpattern to be able to specify a linestyle. Sorry for this,
	 * but the gnuplot parser really is too messy for any other solution, currently */
	if(END_OF_COMMAND) {
	    set_lp_properties(&border_lp, 0, -2, 0, 1.0, 1.0);
	} else {
	    LP_PARSE(border_lp, 1, 0, -2, 0);
	}
    } else if (almost_equals(c_token,"k$ey")) {
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
		if (almost_equals(c_token,"t$op")) {
		    key_vpos = TTOP;
		    key = -1;
		} else if (almost_equals(c_token,"b$ottom")) {
		    key_vpos = TBOTTOM;
		    key = -1;
		} else if (almost_equals(c_token,"l$eft")) {
		    key_hpos = TLEFT;
		    /* key_just = TRIGHT; */
		    key = -1;
		} else if (almost_equals(c_token,"r$ight")) {
		    key_hpos = TRIGHT;
		    key = -1;
		} else if (almost_equals(c_token,"u$nder") ||
			   almost_equals(c_token,"be$low")) {
		    key_vpos = TUNDER;
		    if (key_hpos == TOUT)
			key_hpos--;
		    key = -1;
		} else if (almost_equals(c_token,"o$utside")) {
		    key_hpos = TOUT;
		    if (key_vpos == TUNDER)
			key_vpos--;
		    key = -1;
		} else if (almost_equals(c_token,"L$eft")) {
		    /* key_hpos = TLEFT; */
		    key_just = JLEFT;
		    /* key = -1; */
		} else if (almost_equals(c_token,"R$ight")) {
		    /* key_hpos = TLEFT; */
		    key_just = JRIGHT;
		    /* key = -1; */
		} else if (almost_equals(c_token,"rev$erse")) {
		    key_reverse = TRUE;
		} else if (almost_equals(c_token,"norev$erse")) {
		    key_reverse = FALSE;
		} else if (equals(c_token,"box")) {
		    ++c_token;
		    if (END_OF_COMMAND)
			key_box.l_type = -2;
		    else {
			int old_token = c_token;
	
			LP_PARSE(key_box,1,0,-2,0);
			if (old_token == c_token) {
			    key_box.l_type = real(const_express(&a)) -1;
			}
		    }		
		    --c_token;  /* is incremented after loop */
		} else if (almost_equals(c_token,"nob$ox")) {
		    key_box.l_type = -3;
		} else if (almost_equals(c_token, "sa$mplen")) {
		    ++c_token;
		    key_swidth = real(const_express(&a));
		    --c_token; /* it is incremented after loop */
		} else if (almost_equals(c_token, "sp$acing")) {
		    ++c_token;
		    key_vert_factor = real(const_express(&a));
		    if (key_vert_factor < 0.0)
			key_vert_factor = 0.0;
		    --c_token; /* it is incremented after loop */
		} else if (almost_equals(c_token, "w$idth")) {
		    ++c_token;
		    key_width_fix = real(const_express(&a));
		    --c_token; /* it is incremented after loop */
		} else if (almost_equals(c_token,"ti$tle")) {
		    if (isstring(c_token+1)) {
			/* We have string specified - grab it. */
			quote_str(key_title,++c_token, MAX_LINE_LEN);
		    }
		    else
			key_title[0] = 0;
		} else {
		    get_position(&key_user_pos);
		    key = 1;
		    --c_token;  /* will be incremented again soon */
		} 
		c_token++;
 	    } 
	}
    } else if (almost_equals(c_token,"nok$ey")) {
	key = 0;
	c_token++;
    } else if (almost_equals(c_token,"tic$s")) {
	tic_in = TRUE;
	c_token++;
	if (almost_equals(c_token,"i$n")) {
	    tic_in = TRUE;
	    c_token++;
	} else if (almost_equals(c_token,"o$ut")) {
	    tic_in = FALSE;
	    c_token++;
	}
    } else if (almost_equals(c_token,"xda$ta")) {
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
    } else if (almost_equals(c_token,"yda$ta")) {
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
    } else if (almost_equals(c_token,"zda$ta")) {
	c_token++;
	if(END_OF_COMMAND) {
	    datatype[FIRST_Z_AXIS] = FALSE;
	} else {
	    if (almost_equals(c_token,"t$ime")) {
		datatype[FIRST_Z_AXIS] = TIME;
	    } else {
		datatype[FIRST_Z_AXIS] = FALSE;
	    }
	    c_token++;
	}
    } else if (almost_equals(c_token,"x2da$ta")) {
	c_token++;
	if(END_OF_COMMAND) {
	    datatype[SECOND_X_AXIS] = FALSE;
	} else {
	    if (almost_equals(c_token,"t$ime")) {
		datatype[SECOND_X_AXIS] = TIME;
	    } else {
		datatype[SECOND_X_AXIS] = FALSE;
	    }
	    c_token++;
	}
    } else if (almost_equals(c_token,"y2da$ta")) {
	c_token++;
	if(END_OF_COMMAND) {
	    datatype[SECOND_Y_AXIS] = FALSE;
	} else {
	    if (almost_equals(c_token,"t$ime")) {
		datatype[SECOND_Y_AXIS] = TIME;
	    } else {
		datatype[SECOND_Y_AXIS] = FALSE;
	    }
	    c_token++;
	}
    }

/* to save duplicating code for x/y/z/x2/y2, make a macro
 * (should perhaps be a function ?)
 * unfortunately, string concatenation is not supported on all compilers,
 * so we have to explicitly include both 'on' and 'no' strings in
 * the args
 */

/* change to a function: lph 25.09.1998 */

#define PROCESS_TIC_COMMANDS  set_tic_prop

    else if 
	(PROCESS_TIC_COMMANDS(&x2tics, &mx2tics, &mx2tfreq, &x2ticdef, 
	 SECOND_X_AXIS, &rotate_x2tics, "x2") );
    else if 
	(PROCESS_TIC_COMMANDS(&y2tics, &my2tics, &my2tfreq, &y2ticdef,
	 SECOND_Y_AXIS, &rotate_y2tics, "y2") );
    else if
	(PROCESS_TIC_COMMANDS(&xtics,   &mxtics, &mxtfreq,  &xticdef,
	 FIRST_X_AXIS, &rotate_xtics, "x") );
    else if
	(PROCESS_TIC_COMMANDS(&ytics,   &mytics, &mytfreq,  &yticdef, 
	 FIRST_Y_AXIS, &rotate_ytics, "y") );
    else if 
	(PROCESS_TIC_COMMANDS(&ztics,   &mztics, &mztfreq,  &zticdef,
	 FIRST_Z_AXIS, &rotate_ztics, "z") );

    else if (almost_equals(c_token,"ticsl$evel")) {
	double tlvl;
	struct value a;

	c_token++;
	/* is datatype 'time' relevant here ? */
	tlvl = real(const_express(&a));
	ticslevel = tlvl;
    }

#define PROCESS_MARGIN(variable, string) \
else if (almost_equals(c_token,string)) {\
 ++c_token; if (END_OF_COMMAND) variable = -1;\
 else { struct value a; variable = real(const_express(&a)); } \
}

    PROCESS_MARGIN(lmargin, "lmar$gin")
    PROCESS_MARGIN(bmargin, "bmar$gin")
    PROCESS_MARGIN(rmargin, "rmar$gin")
    PROCESS_MARGIN(tmargin, "tmar$gin")

    else
	return(FALSE);	/* no command match */

    return(TRUE);
}
 


/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
set_three()
{
    if (almost_equals(c_token,"sa$mples")) {
	register int tsamp1, tsamp2;
	struct value a;

	c_token++;
	tsamp1 = (int)magnitude(const_express(&a));
	tsamp2 = tsamp1;
	if (!END_OF_COMMAND) {
	    if (!equals(c_token,","))
		int_error("',' expected",c_token);
	    c_token++;
	    tsamp2 = (int)magnitude(const_express(&a));
	}
	if (tsamp1 < 2 || tsamp2 < 2)
	    int_error("sampling rate must be > 1; sampling unchanged",c_token);
	else {
	    register struct surface_points *f_3dp = first_3dplot;

	    first_3dplot = NULL;
	    sp_free(f_3dp);

	    samples = tsamp1;
	    samples_1 = tsamp1;
	    samples_2 = tsamp2;
	}
    } else if (almost_equals(c_token,"isosa$mples")) {
	register int tsamp1, tsamp2;
	struct value a;

	c_token++;
	tsamp1 = (int)magnitude(const_express(&a));
	tsamp2 = tsamp1;
	if (!END_OF_COMMAND) {
	    if (!equals(c_token,","))
		int_error("',' expected",c_token);
	    c_token++;
	    tsamp2 = (int)magnitude(const_express(&a));
	}
	if (tsamp1 < 2 || tsamp2 < 2)
	    int_error("sampling rate must be > 1; sampling unchanged",c_token);
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
    } else if (almost_equals(c_token,"si$ze")) {
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
    } else if (almost_equals(c_token,"ticsc$ale")) {
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
    } else if (almost_equals(c_token,"t$erminal")) {
	if (multiplot) {
 	    int_error("You can't change the terminal in multiplot mode", c_token);
	}

	c_token++;
	if (END_OF_COMMAND) {
	    list_terms();
	    screen_ok = FALSE;
	} else {
	    term_reset();
	    term = 0; /* in case set_term() fails */
	    term = set_term(c_token);
	    c_token++;

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
    } else if (almost_equals(c_token,"tim$estamp")) {
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
    } else if (almost_equals(c_token,"not$imestamp")) {
	*timelabel.text = 0;
	c_token++;
    } else if (almost_equals(c_token,"vi$ew")) {
	int i;
	TBOOLEAN was_comma = TRUE;
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
		    int_error("',' expected",c_token);
		local_vals[i] = real(const_express(&a));
		i++;
		was_comma = FALSE;
	    }
	}

	if (local_vals[0] < 0 || local_vals[0] > 180)
	    int_error("rot_x must be in [0:180] degrees range; view unchanged", c_token);
	if (local_vals[1] < 0 || local_vals[1] > 360)
	    int_error("rot_z must be in [0:360] degrees range; view unchanged", c_token);
	if (local_vals[2] < 1e-6)
	    int_error("scale must be > 0; view unchanged", c_token);
	if (local_vals[3] < 1e-6)
	    int_error("zscale must be > 0; view unchanged", c_token);

	surface_rot_x = local_vals[0];
	surface_rot_z = local_vals[1];
	surface_scale = local_vals[2];
	surface_zscale = local_vals[3];
    }

/* to save replicated code, define a macro */
#define PROCESS_RANGE(AXIS,STRING, MIN, MAX, AUTO) \
else if (almost_equals(c_token, STRING)) { \
 if (!equals(++c_token,"[")) int_error("expecting '['",c_token); \
 c_token++; \
 AUTO = load_range(AXIS,&MIN,&MAX,AUTO); \
 if (!equals(c_token,"]")) int_error("expecting ']'",c_token); \
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

    PROCESS_RANGE(R_AXIS, "rr$ange", rmin, rmax, autoscale_r)
    PROCESS_RANGE(T_AXIS, "tr$ange", tmin, tmax, autoscale_t)
    PROCESS_RANGE(U_AXIS, "ur$ange", umin, umax, autoscale_u)
    PROCESS_RANGE(V_AXIS, "vr$ange", vmin, vmax, autoscale_v)
    PROCESS_RANGE(FIRST_X_AXIS, "xr$ange", xmin, xmax, autoscale_x)
    PROCESS_RANGE(FIRST_Y_AXIS, "yr$ange", ymin, ymax, autoscale_y)
    PROCESS_RANGE(FIRST_Z_AXIS, "zr$ange", zmin, zmax, autoscale_z)
    PROCESS_RANGE(SECOND_X_AXIS, "x2r$ange", x2min, x2max, autoscale_x2)
    PROCESS_RANGE(SECOND_Y_AXIS, "y2r$ange", y2min, y2max, autoscale_y2)

    else if (almost_equals(c_token,"z$ero")) {
	struct value a;
	c_token++;
	zero = magnitude(const_express(&a));
    } else if (almost_equals(c_token,"multi$plot")) {
	term_start_multiplot();
	c_token++;
    } else if (almost_equals(c_token,"nomulti$plot")) {
	term_end_multiplot();
	c_token++;
    } else
	return(FALSE);	/* no command match */

    return(TRUE);
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

static int set_tic_prop(TICS, MTICS, FREQ, tdef, AXIS, ROTATE, tic_side)
/*    generates PROCESS_TIC_PROP strings from tic_side, e.g. "x2" 
   STRING, NOSTRING, MONTH, NOMONTH, DAY, NODAY, MINISTRING, NOMINI
   "nox2t$ics"     "nox2m$tics"  "nox2d$tics"    "nomx2t$ics" 
 */

int *TICS, *MTICS, AXIS;
double *FREQ;
struct ticdef *tdef;
TBOOLEAN *ROTATE;
char *tic_side;


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
static void set_xyzlabel(label)
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
	int_error("Expected font", c_token);

    quote_str(label->font, c_token, MAX_LINE_LEN);
    c_token++;
}

/* process a 'set label' command */
/* set label {tag} {label_text} {at x,y} {pos} {font name,size} */
/* Entry font added by DJL */
static void set_label()
{
    struct value a;
    struct text_label *this_label = NULL;
    struct text_label *new_label = NULL;
    struct text_label *prev_label = NULL;
    struct position pos;
    char text[MAX_LINE_LEN + 1], font[MAX_LINE_LEN + 1];
    enum JUSTIFY just = LEFT;
    int rotate = 0;
    int tag;
    TBOOLEAN set_text, set_position, set_just = FALSE, set_rot = FALSE,
     set_font;
    TBOOLEAN set_layer = FALSE;
    int layer = 0;

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
	    int_error("tag must be > zero", c_token);
    } else
	tag = assign_label_tag();	/* default next tag */

    /* get text */
    if (!END_OF_COMMAND && isstring(c_token)) {
	/* get text */
	quote_str(text, c_token, MAX_LINE_LEN);
	c_token++;
	set_text = TRUE;
    } else {
	text[0] = '\0';		/* default no text */
	set_text = FALSE;
    }

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
	    int_error("bad syntax in set label", c_token);
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
	&& !equals(c_token, "font")) {
	if (set_just)
	    int_error("only one justification is allowed", c_token);
	if (almost_equals(c_token, "l$eft")) {
	    just = LEFT;
	} else if (almost_equals(c_token, "c$entre")
		   || almost_equals(c_token, "c$enter")) {
	    just = CENTRE;
	} else if (almost_equals(c_token, "r$ight")) {
	    just = RIGHT;
	} else
	    int_error("bad syntax in set label", c_token);

	c_token++;
	set_just = TRUE;
    }
    /* get rotation (added by RCC) */
    if (!END_OF_COMMAND && !equals(c_token, "font")
	&& !equals(c_token, "front") && !equals(c_token, "back")) {
	if (almost_equals(c_token, "rot$ate")) {
	    rotate = TRUE;
	} else if (almost_equals(c_token, "norot$ate")) {
	    rotate = FALSE;
	} else
	    int_error("bad syntax in set label", c_token);

	c_token++;
	set_rot = TRUE;
    }
    /* get font */
    font[0] = NUL;
    set_font = FALSE;
    if (!END_OF_COMMAND && equals(c_token, "font") &&
	!equals(c_token, "front") && !equals(c_token, "back")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error("font name and size expected", c_token);
	if (isstring(c_token)) {
	    quote_str(font, c_token, MAX_ID_LEN);
	    /* get 'name,size', no further check */
	    set_font = TRUE;
	} else
	    int_error("'fontname,fontsize' expected", c_token);

	c_token++;
    }				/* Entry font added by DJL */
    /* get front/back (added by JDP) */
    set_layer = FALSE;
    if (!END_OF_COMMAND && equals(c_token, "back") && !equals(c_token, "front")) {
	layer = 0;
	c_token++;
	set_layer = TRUE;
    }
    if(!END_OF_COMMAND && equals(c_token, "front")) {
	if (set_layer)
	    int_error("only one of front or back expected", c_token);
	layer = 1;
	c_token++;
	set_layer = TRUE;
    }
    if (!END_OF_COMMAND)
	int_error("extraenous or out-of-order arguments in set label", c_token);

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
	    (void) strcpy(this_label->text, text);
	if (set_just)
	    this_label->pos = just;
	if (set_rot)
	    this_label->rotate = rotate;
	if (set_layer)
	    this_label->layer = layer;
	if (set_font)
	    (void) strcpy(this_label->font, font);
    } else {
	/* adding the label */
	new_label = (struct text_label *)
	    gp_alloc((unsigned long) sizeof(struct text_label), "label");
	if (prev_label != NULL)
	    prev_label->next = new_label;	/* add it to end of list */
	else
	    first_label = new_label;	/* make it start of list */
	new_label->tag = tag;
	new_label->next = this_label;
	new_label->place = pos;
	(void) strcpy(new_label->text, text);
	new_label->pos = just;
	new_label->rotate = rotate;
	new_label->layer = layer;
	(void) strcpy(new_label->font, font);
    }
}				/* Entry font added by DJL */

/* process 'set nolabel' command */
/* set nolabel {tag} */
static void set_nolabel()
{
    struct value a;
    struct text_label *this_label;
    struct text_label *prev_label;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all labels */
	while (first_label != NULL)
	    delete_label((struct text_label *) NULL, first_label);
    } else {
	/* get tag */
	tag = (int) real(const_express(&a));
	if (!END_OF_COMMAND)
	    int_error("extraneous arguments to set nolabel", c_token);
	for (this_label = first_label, prev_label = NULL;
	     this_label != NULL;
	     prev_label = this_label, this_label = this_label->next) {
	    if (this_label->tag == tag) {
		delete_label(prev_label, this_label);
		return;		/* exit, our job is done */
	    }
	}
	int_error("label not found", c_token);
    }
}

/* assign a new label tag */
/* labels are kept sorted by tag number, so this is easy */
static int /* the lowest unassigned tag number */ assign_label_tag()
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
static void delete_label(prev, this)
struct text_label *prev, *this;
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous label */
	    prev->next = this->next;
	else			/* this = first_label so change first_label */
	    first_label = this->next;
	free((char *) this);
    }
}


/* process a 'set arrow' command */
/* set arrow {tag} {from x,y} {to x,y} {{no}head} */
static void set_arrow()
{
    struct value a;
    struct arrow_def *this_arrow = NULL;
    struct arrow_def *new_arrow = NULL;
    struct arrow_def *prev_arrow = NULL;
    struct position spos, epos;
    struct lp_style_type loc_lp;
    int axes = FIRST_AXES;
    int tag;
    TBOOLEAN set_start, set_end, head = 1, set_axes = 0, set_line = 0, set_layer = 0;
    int layer = 0;

    /* Init struct lp_style_type loc_lp */
    reset_lp_properties (&loc_lp);

    /* get tag */
    if (!END_OF_COMMAND
	&& !equals(c_token, "from")
	&& !equals(c_token, "to")
	&& !equals(c_token, "first")
	&& !equals(c_token, "second")) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error("tag must be > zero", c_token);
    } else
	tag = assign_arrow_tag();	/* default next tag */

    if (!END_OF_COMMAND && equals(c_token, "first")) {
	++c_token;
	axes = FIRST_AXES;
	set_axes = 1;
    } else if (!END_OF_COMMAND && equals(c_token, "second")) {
	++c_token;
	axes = SECOND_AXES;
	set_axes = 1;
    }
    /* get start position */
    if (!END_OF_COMMAND && equals(c_token, "from")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error("start coordinates expected", c_token);
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
	    int_error("end coordinates expected", c_token);
	/* get coordinates */
	get_position(&epos);
	set_end = TRUE;
    } else {
	epos.x = epos.y = epos.z = 0;
	epos.scalex = epos.scaley = epos.scalez = first_axes;
	set_end = FALSE;
    }

    /* get start position - what the heck, either order is ok */
    if (!END_OF_COMMAND && equals(c_token, "from")) {
	if (set_start)
	    int_error("only one 'from' is allowed", c_token);
	c_token++;
	if (END_OF_COMMAND)
	    int_error("start coordinates expected", c_token);
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
    LP_PARSE(loc_lp, 1, 0, 0, 0);
    loc_lp.pointflag = 0;	/* standard value for arrows, don't use points */

    if (!END_OF_COMMAND)
	int_error("extraneous or out-of-order arguments in set arrow", c_token);

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
	}
	this_arrow->head = head;
	if (set_layer) {
	    this_arrow->layer = layer;
	}
	if (set_line) {
	    this_arrow->lp_properties = loc_lp;
	}
    } else {
	/* adding the arrow */
	new_arrow = (struct arrow_def *)
	    gp_alloc((unsigned long) sizeof(struct arrow_def), "arrow");
	if (prev_arrow != NULL)
	    prev_arrow->next = new_arrow;	/* add it to end of list */
	else
	    first_arrow = new_arrow;	/* make it start of list */
	new_arrow->tag = tag;
	new_arrow->next = this_arrow;
	new_arrow->start = spos;
	new_arrow->end = epos;
	new_arrow->head = head;
	new_arrow->layer = layer;
	new_arrow->lp_properties = loc_lp;
    }
}

/* process 'set noarrow' command */
/* set noarrow {tag} */
static void set_noarrow()
{
    struct value a;
    struct arrow_def *this_arrow;
    struct arrow_def *prev_arrow;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all arrows */
	while (first_arrow != NULL)
	    delete_arrow((struct arrow_def *) NULL, first_arrow);
    } else {
	/* get tag */
	tag = (int) real(const_express(&a));
	if (!END_OF_COMMAND)
	    int_error("extraneous arguments to set noarrow", c_token);
	for (this_arrow = first_arrow, prev_arrow = NULL;
	     this_arrow != NULL;
	     prev_arrow = this_arrow, this_arrow = this_arrow->next) {
	    if (this_arrow->tag == tag) {
		delete_arrow(prev_arrow, this_arrow);
		return;		/* exit, our job is done */
	    }
	}
	int_error("arrow not found", c_token);
    }
}

/* assign a new arrow tag */
/* arrows are kept sorted by tag number, so this is easy */
static int /* the lowest unassigned tag number */ assign_arrow_tag()
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
static void delete_arrow(prev, this)
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

/* ======================================================== */
/* process a 'set linestyle' command */
/* set linestyle {tag} {linetype n} {linewidth x} {pointtype n} {pointsize x} */
static void set_linestyle()
{
    struct value a;
    struct linestyle_def *this_linestyle = NULL;
    struct linestyle_def *new_linestyle = NULL;
    struct linestyle_def *prev_linestyle = NULL;
    struct lp_style_type loc_lp;
    int tag;

    /* Init struct lp_style_type loc_lp */
    reset_lp_properties (&loc_lp);

    /* get tag */
    if (!END_OF_COMMAND) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error("tag must be > zero", c_token);
    } else
	tag = assign_linestyle_tag();	/* default next tag */

    /* pick up a line spec : dont allow ls, do allow point type
     * default to same line type = point type = tag
     */
    LP_PARSE(loc_lp, 0, 1, tag - 1, tag - 1);

    if (!END_OF_COMMAND)
	int_error("extraneous or out-of-order arguments in set linestyle", c_token);

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
	    gp_alloc((unsigned long) sizeof(struct linestyle_def), "linestyle");
	if (prev_linestyle != NULL)
	    prev_linestyle->next = new_linestyle;	/* add it to end of list */
	else
	    first_linestyle = new_linestyle;	/* make it start of list */
	new_linestyle->tag = tag;
	new_linestyle->next = this_linestyle;
	new_linestyle->lp_properties = loc_lp;
    }
}

/* process 'set nolinestyle' command */
/* set nolinestyle {tag} */
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
	    int_error("extraneous arguments to set nolinestyle", c_token);
	for (this = first_linestyle, prev = NULL;
	     this != NULL;
	     prev = this, this = this->next) {
	    if (this->tag == tag) {
		delete_linestyle(prev, this);
		return;		/* exit, our job is done */
	    }
	}
	int_error("linestyle not found", c_token);
    }
}

/* assign a new linestyle tag */
/* linestyles are kept sorted by tag number, so this is easy */
static int /* the lowest unassigned tag number */ assign_linestyle_tag()
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
static void delete_linestyle(prev, this)
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

void lp_use_properties(lp, tag, pointflag)
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
    int_error("linestyle not found", NO_CARET);
}

/* ======================================================== */

enum PLOT_STYLE /* not static; used by command.c */ get_style()
{
    register enum PLOT_STYLE ps = LINES;	/* HBB: initial value, for 'gcc -W} */

    c_token++;
    if (almost_equals(c_token, "l$ines"))
	ps = LINES;
    else if (almost_equals(c_token, "i$mpulses"))
	ps = IMPULSES;
    else if (almost_equals(c_token, "p$oints"))
	ps = POINTSTYLE;
    else if (almost_equals(c_token, "linesp$oints") || equals(c_token, "lp"))
	ps = LINESPOINTS;
    else if (almost_equals(c_token, "d$ots"))
	ps = DOTS;
    else if (almost_equals(c_token, "ye$rrorbars"))
	ps = YERRORBARS;
    else if (almost_equals(c_token, "e$rrorbars"))
	ps = YERRORBARS;
    else if (almost_equals(c_token, "xe$rrorbars"))
	ps = XERRORBARS;
    else if (almost_equals(c_token, "xye$rrorbars"))
	ps = XYERRORBARS;
    else if (almost_equals(c_token, "boxes"))
	ps = BOXES;
    else if (almost_equals(c_token, "boxer$rorbars"))
	ps = BOXERROR;
    else if (almost_equals(c_token, "boxx$yerrorbars"))
	ps = BOXXYERROR;
    else if (almost_equals(c_token, "st$eps"))
	ps = STEPS;
    else if (almost_equals(c_token, "fs$teps"))
	ps = FSTEPS;
    else if (almost_equals(c_token, "his$teps"))
	ps = HISTEPS;
    else if (almost_equals(c_token, "vec$tor"))		/* HBB: minor cosmetic change */
	ps = VECTOR;
    else if (almost_equals(c_token, "fin$ancebars"))
	ps = FINANCEBARS;
    else if (almost_equals(c_token, "can$dlesticks"))
	ps = CANDLESTICKS;
    else {
	int_error("expecting 'lines', 'points', 'linespoints', 'dots', \
'impulses',\n'yerrorbars', 'xerrorbars', 'xyerrorbars', 'steps', 'fsteps', \
'histeps',\n'boxes', 'boxerrorbars', 'boxxyerrorbars', 'vector', \
'financebars', 'candlesticks'", c_token);
	return LINES;		/* keep gcc -Wuninitialised happy */
    }
    c_token++;
    return (ps);
}

/* For set [xy]tics... command */
static void load_tics(axis, tdef)
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
static void load_tic_user(axis, tdef)
int axis;
struct ticdef *tdef;
{
    struct ticmark *list = NULL;	/* start of list */
    struct ticmark *last = NULL;	/* end of list */
    struct ticmark *tic = NULL;	/* new ticmark */
    char temp_string[MAX_LINE_LEN];

    while (!END_OF_COMMAND) {
	/* parse a new ticmark */
	tic = (struct ticmark *) gp_alloc((unsigned long) sizeof(struct ticmark), (char *) NULL);
	if (tic == (struct ticmark *) NULL) {
	    free_marklist(list);
	    int_error("out of memory for tic mark", c_token);
	}
	/* syntax is  (  ['format'] value , ... )
	 * but for timedata, the value itself is a string, which
	 * complicates things somewhat
	 */

	/* has a string with it? */
	if (isstring(c_token) &&
	    (datatype[axis] != TIME || isstring(c_token + 1))) {
	    quote_str(temp_string, c_token, MAX_LINE_LEN);
	    tic->label = gp_alloc((unsigned long) strlen(temp_string) + 1, "tic label");
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
	int_error("expecting right parenthesis )", c_token);
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

static void free_marklist(list)
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
static void load_tic_series(axis, tdef)
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
		int_error("expecting comma to separate incr,end", c_token);
	    c_token++;
	    GET_NUM_OR_TIME(end, axis);
	}
	if (!END_OF_COMMAND)
	    int_error("tic series is defined by [start,]increment[,end]", c_token);

	if (start < end && incr <= 0)
	    int_error("increment must be positive", incr_token);
	if (start > end && incr >= 0)
	    int_error("increment must be negative", incr_token);
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

static void load_offsets(a, b, c, d)
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
	int_error("starting range value or ':' or 'to' expected", c_token);
    } else if (!equals(c_token, "to") && !equals(c_token, ":")) {
	if (equals(c_token, "*")) {
	    autosc |= 1;
	    c_token++;
	} else {
	    GET_NUM_OR_TIME(*a, axis);
	    autosc &= 2;
	}
    }
    if (!equals(c_token, "to") && !equals(c_token, ":"))
	int_error("':' or keyword 'to' expected", c_token);
    c_token++;
    if (!equals(c_token, "]")) {
	if (equals(c_token, "*")) {
	    autosc |= 2;
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

static int looks_like_numeric(format)
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
do{if (axes >= 0 && datatype[axes+axis] == TIME && isstring(c_token) ) { \
    char ss[80]; struct tm tm; \
    quote_str(ss,c_token, 80); ++c_token; \
    if (gstrptime(ss,timefmt,&tm)) store = (double) gtimegm(&tm); \
   } else {\
    struct value value; \
    store = real(const_express(&value));\
  }}while(0)


/* get_position_type - for use by get_position().
 * parses first/second/graph/screen keyword
 */

static void get_position_type(type, axes)
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

static void get_position(pos)
struct position *pos;
{
    int axes;
    enum position_type type = first_axes;

    get_position_type(&type, &axes);
    pos->scalex = type;
    GET_NUMBER_OR_TIME(pos->x, axes, FIRST_X_AXIS);
    if (!equals(c_token, ","))
	int_error("Expected comma", c_token);
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

static void set_lp_properties(arg, allow_points, lt, pt, lw, ps)
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

static void reset_lp_properties(arg)
struct lp_style_type *arg;
{
    /* See plot.h for struct lp_style_type */
    arg->pointflag = arg->l_type = arg->p_type = 0;
    arg->l_width = arg->p_size = 1.0;
}

static void set_locale(lcl)
char *lcl;
{
#ifndef NO_LOCALE_H
    int i;
    struct tm tm;

    if (setlocale(LC_TIME, lcl))
	safe_strncpy(cur_locale, lcl, sizeof(cur_locale));
    else
	int_error("Locale not available", c_token);

    /* we can do a *lot* better than this ; eg use system functions
     * where available; create values on first use, etc
     */
    memset(&tm, 0, sizeof(struct tm));
    for (i = 0; i < 7; ++i) {
	tm.tm_wday = i;		/* hope this enough */
	strftime(full_day_names[i], sizeof(full_day_names[i]), "%A", &tm);
	strftime(abbrev_day_names[i], sizeof(abbrev_day_names[i]), "%a", &tm);
    }
    for (i = 0; i < 12; ++i) {
	tm.tm_mon = i;		/* hope this enough */
	strftime(full_month_names[i], sizeof(full_month_names[i]), "%B", &tm);
	strftime(abbrev_month_names[i], sizeof(abbrev_month_names[i]), "%b", &tm);
    }
#else
    safe_strncpy(cur_locale, lcl, sizeof(cur_locale));
#endif /* NO_LOCALE_H */
}
