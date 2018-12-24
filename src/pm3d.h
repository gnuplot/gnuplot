/*
 * $Id: pm3d.h,v 1.35 2016/11/05 21:21:07 sfeam Exp $
 */

/* GNUPLOT - pm3d.h */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
 *
 * What is here: #defines, global variables and declaration of routines for
 * the pm3d plotting mode
 *
]*/


/* avoid multiple includes */
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#ifndef TERM_HELP

#ifndef PM3D_H
#define PM3D_H

#include "axis.h"	/* only for NONLINEAR_AXES */
#include "graph3d.h"	/* struct surface_points */



/****
  Global options for pm3d algorithm (to be accessed by set / show)
****/

/*
  where to plot pm3d: base or top (color map) or surface (color surface)
  The string pm3d.where can be any combination of the #defines below.
  For instance, "b" plot at botton only, "st" plots firstly surface, then top, etc.
*/
#define PM3D_AT_BASE	'b'
#define PM3D_AT_TOP	't'
#define PM3D_AT_SURFACE	's'

/*
  options for flushing scans (for pm3d.flush)
  Note: new terminology compared to my pm3d program; in gnuplot it became
  begin and right instead of left and right
*/
#define PM3D_FLUSH_BEGIN   'b'
#define PM3D_FLUSH_END     'r'
#define PM3D_FLUSH_CENTER  'c'

/*
  direction of taking the scans: forward = as the scans are stored in the
  file; backward = opposite direction, i.e. like from the end of the file
*/
typedef enum {
    PM3D_SCANS_AUTOMATIC,
    PM3D_SCANS_FORWARD,
    PM3D_SCANS_BACKWARD,
    PM3D_DEPTH
} pm3d_scandir;

/*
  clipping method:
    PM3D_CLIP_1IN: all 4 points of the quadrangle must be defined and at least
		   1 point of the quadrangle must be in the x and y ranges
    PM3D_CLIP_4IN: all 4 points of the quadrangle must be in the x and y ranges
*/
#define PM3D_CLIP_1IN '1'
#define PM3D_CLIP_4IN '4'

/*
  is pm3d plotting style implicit or explicit?
*/
typedef enum {
    PM3D_EXPLICIT = 0,
    PM3D_IMPLICIT = 1
} PM3D_IMPL_MODE;

/*
  from which corner take the color?
*/
typedef enum {
    /* keep the following order of PM3D_WHICHCORNER_C1 .. _C4 */
    PM3D_WHICHCORNER_C1 = 0, 	/* corner 1: first scan, first point   */
    PM3D_WHICHCORNER_C2 = 1, 	/* corner 2: first scan, second point  */
    PM3D_WHICHCORNER_C3 = 2, 	/* corner 3: second scan, first point  */
    PM3D_WHICHCORNER_C4 = 3,	/* corner 4: second scan, second point */
    /* the rest can be in any order */
    PM3D_WHICHCORNER_MEAN    = 4, /* average z-value from all 4 corners */
    PM3D_WHICHCORNER_GEOMEAN = 5, /* geometrical mean of 4 corners */
    PM3D_WHICHCORNER_HARMEAN = 6, /* harmonic mean of 4 corners */
    PM3D_WHICHCORNER_MEDIAN  = 7, /* median of 4 corners */
    PM3D_WHICHCORNER_RMS     = 8, /* root mean square of 4 corners*/
    PM3D_WHICHCORNER_MIN     = 9, /* minimum of 4 corners */
    PM3D_WHICHCORNER_MAX     = 10,/* maximum of 4 corners */
    PM3D_COLOR_BY_NORMAL     = 11 /* derive color from surface normal (not currently used) */
} PM3D_WHICH_CORNERS2COLOR;

/*
  structure defining all properties of pm3d plotting mode
  (except for the properties of the smooth color box, see color_box instead)
*/
typedef struct {
  char where[7];	/* base, top, surface */
  char flush;   	/* left, right, center */
  char ftriangles;   	/* 0/1 (don't) draw flushing triangles */
  char clip;		/* 1in, 4in */
  TBOOLEAN no_clipcb;	/* FALSE: cb<0 treated as 0  TRUE: cb<0 treated as NaN */
  pm3d_scandir direction;
  TBOOLEAN base_sort;	/* default: depth sort by mean z;   true: use z=0.0 */
  PM3D_IMPL_MODE implicit;
			/* 1: [default] draw ALL surfaces with pm3d
			   0: only surfaces specified with 'with pm3d' */
  PM3D_WHICH_CORNERS2COLOR which_corner_color;
			/* default: average color from all 4 points */
  int interp_i;		/* # of interpolation steps along scanline */
  int interp_j;		/* # of interpolation steps between scanlines */
  lp_style_type border;	/* LT_NODRAW to disable.  From `set pm3d border <linespec> */
} pm3d_struct;

extern pm3d_struct pm3d;

typedef struct lighting_model {
  double strength;	/* 0 = no lighting model; 1 = full shading */
  double spec;		/* specular component 0-1 */
  double ambient;	/* ambient component 0-1 */
  double Phong;		/* Phong exponent */
  int rot_z;		/* illumination angle */
  int rot_x;		/* illumination angle */
  TBOOLEAN fixed;	/* TRUE means the light does not rotate */
} lighting_model;

extern lighting_model pm3d_shade;

/* Used to initialize `set pm3d border` */
extern struct lp_style_type default_pm3d_border;

/* Used by routine filled_quadrangle() in color.c */
extern struct lp_style_type pm3d_border_lp;
extern TBOOLEAN track_pm3d_quadrangles;

#if defined(NONLINEAR_AXES) && (NONLINEAR_AXES > 0)
#   define z2cb(z) (z)
#else
    /* The original routine, with log/unlog dance steps */
#   define z2cb(z) z2cb_with_logs(z)
#endif


/****
  Declaration of routines
****/

int get_pm3d_at_option __PROTO((char *pm3d_where));
void pm3d_depth_queue_clear __PROTO((void));
void pm3d_depth_queue_flush __PROTO((void));
void pm3d_reset __PROTO((void));
void pm3d_draw_one __PROTO((struct surface_points* plots));
void pm3d_add_quadrangle __PROTO((struct surface_points* plot, gpdPoint *corners));
double z2cb_with_logs __PROTO((double z));
double cb2gray __PROTO((double cb));
void
pm3d_rearrange_scan_array __PROTO((struct surface_points* this_plot,
    struct iso_curve*** first_ptr, int* first_n, int* first_invert,
    struct iso_curve*** second_ptr, int* second_n, int* second_invert));

void set_plot_with_palette __PROTO((int plot_num, int plot_mode));

TBOOLEAN is_plot_with_palette __PROTO((void));
TBOOLEAN is_plot_with_colorbox __PROTO((void));

#endif /* PM3D_H */

#endif /* TERM_HELP */

/* eof pm3d.h */
