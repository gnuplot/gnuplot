/*
 * $Id: pm3d.h,v 1.9 2002/01/26 17:55:08 joze Exp $
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

#include "graph3d.h" /* struct surface_points */



/****
  Global options for pm3d algorithm (to be accessed by set / show)
****/

/*
  where to plot pm3d: base or top (color map) or surface (color surface)
    - if pm3d.where is "", then don't plot in pm3d mode
    - pm3d_at_where can be any combination of the #defines below. For instance,
	"b" plot at botton only, "st" plots firstly surface, then top, etc.
  (for pm3d.where)	
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
#define PM3D_SCANS_AUTOMATIC  'a'
#define PM3D_SCANS_FORWARD    'f'
#define PM3D_SCANS_BACKWARD   'b'

/*
  clipping method:
    PM3D_CLIP_1IN: all 4 points of the quadrangle must be defined and at least
		   1 point of the quadrangle must be in the x and y ranges
    PM3D_CLIP_4IN: all 4 points of the quadrangle must be in the x and y ranges
*/
#define PM3D_CLIP_1IN '1'
#define PM3D_CLIP_4IN '4'

typedef enum {
    PM3D_EXPLICIT = 0,
    PM3D_IMPLICIT = 1
} PM3D_IMPL_MODE;

/*
  structure defining all properties of pm3d plotting mode
  (except for the properties of the smooth color box, see color_box instead)
*/
typedef struct {
  char where[7];	/* base, top, surface */
  char map;		/* set to 1 during 'set pm3d map' (workaround for
			   non-existing 2D map plot) */
  char flush;   	/* left, right, center */
  char direction;	/* forward, backward */
  char clip;		/* 1in, 4in */
  int hidden3d_tag;     /* if this is > 0, pm3d hidden lines are drawn with
			   this linestyle (which must naturally present). */
  int solid;            /* if this is != 0, border tics and labels might be
			   hidden by the surface */
  PM3D_IMPL_MODE implicit;
			/* 1: [default] draw ALL surfaces with pm3d
			   0: only surfaces specified with 'with pm3d' */
  int use_column;	/* use color value from column 2 or 4
			   ('using 1:2' or 'using 1:2:3:4') */
} pm3d_struct;


extern pm3d_struct pm3d;


/****
  Declaration of routines
****/

int set_pm3d_zminmax __PROTO((void));
void pm3d_plot __PROTO((struct surface_points * plot, char at_which_z));
void filled_color_contour_plot __PROTO((struct surface_points *plot, int contours_where));
void pm3d_reset __PROTO((void));
void pm3d_draw_one __PROTO((struct surface_points* plots));
double z2cb __PROTO((double z));
double cb2gray __PROTO((double cb));
void
pm3d_rearrange_scan_array __PROTO((struct surface_points* this_plot,
    struct iso_curve*** first_ptr, int* first_n, int* first_invert,
    struct iso_curve*** second_ptr, int* second_n, int* second_invert));


#endif /* PM3D_H */

#endif /* TERM_HELP */

/* eof pm3d.h */
