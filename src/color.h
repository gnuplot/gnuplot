/*
 * $Id: color.h,v 1.10 2002/02/25 03:10:41 broeker Exp $
 */

/* GNUPLOT - color.h */

/* invert the gray for negative figure (default is positive) */

/*[
 *
 * Petr Mikulik, December 1998 -- June 1999
 * Copyright: open source as much as possible
 *
]*/


/*
In general, this file deals with colours, and in the current gnuplot
source layout it would correspond structures and routines found in
driver.h, term.h and term.c.

Here we define structures which are required for the communication
of palettes between terminals and making palette routines.
*/


/** NOTICE: currently, this file is included only if PM3D is defined **/
#ifndef COLOR_H
#define COLOR_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef PM3D

#include "gp_types.h"

/* Contains a colour in RGB scheme.
   Values of  r, g and b  are all in range [0;1] */
typedef struct {
    double r, g, b;
} rgb_color;

#ifdef EXTENDED_COLOR_SPECS
typedef struct {
    double gray;
    /* to be extended */
} colorspec_t;
#endif


/* a point (with integer coordinates) for use in polygon drawing */
typedef struct {
    unsigned int x, y;
#ifdef EXTENDED_COLOR_SPECS
    double z;
    colorspec_t spec;
#endif
} gpiPoint;


/* a point (with double coordinates) for use in polygon drawing */
typedef struct {
    double x, y, z;
} gpdPoint;


/*
   colour modes
*/

#define SMPAL_COLOR_MODE_NONE  0
#define SMPAL_COLOR_MODE_GRAY 'g'
#define SMPAL_COLOR_MODE_RGB  'r'

/*
  inverting the colour for negative picture (default is positive picture)
  (for pm3d.positive)
*/
#define SMPAL_NEGATIVE  'n'
#define SMPAL_POSITIVE  'p'


/* Declaration of smooth palette, i.e. palette for smooth colours */

typedef struct {
  /** Constants: **/

  /* (Fixed) number of formulae implemented for gray index to RGB
   * mapping in color.c.  Usage: somewhere in `set' command to check
   * that each of the below-given formula R,G,B are lower than this
   * value. */
  int colorFormulae;

  /** Values that can be changed by `set' and shown by `show' commands: **/

  /* can be SMPAL_COLOR_MODE_GRAY or SMPAL_COLOR_MODE_RGB */
  char colorMode;
  /* mapping formulae for SMPAL_COLOR_MODE_RGB */
  int formulaR, formulaG, formulaB;
  char positive;		/* positive or negative figure */

  /* Now the variables that contain the discrete approximation of the
   * desired palette of smooth colours as created by make_palette in
   * pm3d.c.  This is then passed into terminal's make_palette, who
   * transforms this [0;1] into whatever it supports.  */

  /* Only this number of colour positions will be used even though
   * there are some more available in the discrete palette of the
   * terminal.  Useful for multiplot.  Max. number of colours is taken
   * if this value equals 0.  Unused by: PostScript */
  int use_maxcolors;
  /* Number of colours used for the discrete palette. Equals to the
   * result from term->make_palette(NULL), or restricted by
   * use_maxcolor.  Used by: pm, gif. Unused by: PostScript */
  int colors;
  /* Table of RGB triplets resulted from applying the formulae. Used
   * in the 2nd call to term->make_palette for a terminal with
   * discrete colours. Unused by PostScript which has calculates them
   * analytically. */
  rgb_color *color;

  /** Variables used by some terminals **/
  
  /* Option unique for output to PostScript file.  By default,
   * ps_allcF=0 and only the 3 selected rgb color formulae are written
   * into the header preceding pm3d map in the file.  If ps_allcF is
   * non-zero, then print there all color formulae, so that it is easy
   * to play with choosing manually any color scheme in the PS file
   * (see the definition of "/g"). Like that you can get the
   * Rosenbrock multiplot figure on my gnuplot.html#pm3d demo page.
   * Note: this option is used by all terminals of the postscript
   * family, i.e. postscript, pslatex, epslatex, so it will not be
   * comfortable to move it to the particular .trm files. */
  char ps_allcF;

} t_sm_palette;



/* GLOBAL VARIABLES */

extern t_sm_palette sm_palette;

#ifdef EXTENDED_COLOR_SPECS
extern int supply_extended_color_specs;
#endif

/*
Position of the colour smooth box that shows the colours used.
Currently, everything is default (I haven't figured out how to make it
in gnuplot's usual way (like set labels, set key etc.) so the box is put
somewhere right from the 3d graph.
*/
#define SMCOLOR_BOX_NO      'n'
#define SMCOLOR_BOX_DEFAULT 'd'
#define SMCOLOR_BOX_USER    'u'

typedef struct {
  char where;
    /* where
	SMCOLOR_BOX_NO .. do not draw the colour box
	SMCOLOR_BOX_DEFAULT .. draw it at default position and size
	SMCOLOR_BOX_USER .. draw it at the position given by user by using
			  corners from below ... ! NOT IMPLEMENTED !
    */
#if 0
  /* corners of the box */
PROPOSALS (WHO WANTS TO MAKE THIS?):
  /* coordval ? */ double xlow, xhigh, ylow, yhigh;
			OR
  /* coordval ? */ double xlow, ylow, xsize, ysize; <--- preferred!
  /* or some structure (x,y) or (x,y,z) that can do
	set color_smooth_box from 0.8,0.6 screen to 0.95,0.95 screen
	as well as
	set color_smooth_box from 0.34,0.45,12.4 to 1.0,2.0,45
  */
#endif
  char rotation; /* 'v' or 'h' vertical or horizontal box */

  char border; /* if non-null, a border will be drawn around the box (default) */
  int border_lt_tag;

  float xorigin;
  float yorigin;
  float xsize;
  float ysize;

} color_box_struct;


/* GLOBAL VARIABLES */

extern color_box_struct color_box;


/* ROUTINES */

/*
  Make the colour palette. Return 0 on success
  Put number of allocated colours into sm_palette.colors
*/
int make_palette __PROTO((void));

/*
   Set the colour on the terminal
   Currently, each terminal takes care of remembering the current colour,
   so there is not much to do here---well, except for reversing the gray
   according to sm_palette.positive == SMPAL_POSITIVE or SMPAL_NEGATIVE
*/
void set_color __PROTO(( double gray ));

#if 0
/*
   Makes mapping from real 3D coordinates to 2D terminal coordinates,
   then draws filled polygon
*/
/* HBB 20001109: removed 'static' --- doesn't make sense in headers */
void filled_polygon __PROTO((int points, gpdPoint *corners));
#endif

/*
   The routine above for 4 points explicitly
*/
#ifdef EXTENDED_COLOR_SPECS
void filled_quadrangle __PROTO((gpdPoint *corners, gpiPoint* icorners));
#else
void filled_quadrangle __PROTO((gpdPoint *corners));
#endif

/*
   Makes mapping from real 3D coordinates, passed as coords array,
   to 2D terminal coordinates, then draws filled polygon
*/
/* HBB 20010216: added 'GPHUGE' attribute */
void filled_polygon_3dcoords __PROTO((int points, struct coordinate GPHUGE *coords));

/*
   Makes mapping from real 3D coordinates, passed as coords array, but at z coordinate
   fixed (base_z, for instance) to 2D terminal coordinates, then draws filled polygon
*/
void filled_polygon_3dcoords_zfixed __PROTO((int points, struct coordinate GPHUGE *coords, double z));

/*
  Draw colour smooth box
*/
void draw_color_smooth_box __PROTO((void));


#endif /* PM3D */

#endif /* COLOR_H */

/* eof color.h */
