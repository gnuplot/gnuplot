/* GNUPLOT - color.c */

/*[
 *
 * Petr Mikulik, December 1998 -- June 2000
 * Copyright: open source as much as possible
 * 
 * The implementation for the drivers X11 and vgagl
 * was done by Johannes Zellner <johannes@zellner.org>
 * (Oct 1999 - Jan 2000)
 *
 * 
 * What is here: #defines, global variables and declaration of routines for 
 * colours---required by the pm3d splotting mode and coloured filled contours
 *
]*/


/*
 *
 * What is here:
 *   - Global variables declared in .h are initialized here
 *   - Palette routines
 *   - Colour box drawing
 *
 */


/** NOTICE: currently, this file is included only if PM3D is defined **/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef PM3D

#include "plot.h"
#include "setshow.h"
#include "graphics.h"
#include "graph3d.h"
#include "pm3d.h"
#include "graphics.h"
#include "term_api.h"
/* need to access used_pm3d_zmin, used_pm3d_zmax; */

/* COLOUR MODES - GLOBAL VARIABLES */

t_sm_palette sm_palette = {
    37, /* colorFormulae---must be changed if changed GetColorValueFromFormula() */
    SMPAL_COLOR_MODE_RGB, /* colorMode */
    7, 5, 15, /* formulaR, formulaG, formulaB */
    SMPAL_POSITIVE, /* positive */
    0, 0, 0, /* use_maxcolors, colors, rgb_table */
    0, /* offset */
    0 /* ps_allcF */
};


/* SMOOTH COLOUR BOX - GLOBAL VARIABLE */

color_box_struct color_box = {
    SMCOLOR_BOX_DEFAULT, /* draw at default_pos */
    'v', /* vertical change (gradient) of colours */
    1, /* border is on by default */
    -1, /* use default border */

    0, 0, /* origin */
    1, 1  /* size */
};

#ifdef EXTENDED_COLOR_SPECS
int supply_extended_color_specs = 0;
#endif

/********************************************************************
  ROUTINES
 */

/*
   Make the colour palette. Return 0 on success
   Put number of allocated colours into sm_palette.colors
 */
int
make_palette(void)
{
    int i;
    double gray;

    /* this is simpy for deciding, if we print
     * a * message after allocating new colors */
    static t_sm_palette save_pal = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	(rgb_color*) 0, -1, -1
    };

#if 0
    GIF_show_current_palette();
#endif

    if (!term->make_palette) {
	fprintf(stderr,"Error: your terminal does not support continous colors!\nSee pm3d/README for more details.\n");
	return 1;
    }

    /* ask for suitable number of colours in the palette */
    i = term->make_palette(NULL);

    if (i == 0) {
	/* terminal with its own mapping (PostScript, for instance)
	   It will not change palette passed below, but non-NULL has to be
	   passed there to create the header or force its initialization
	 */
	term->make_palette(&sm_palette);
	return 0;
    }

    /* set the number of colours to be used (allocated) */
    sm_palette.colors = i;
    if (sm_palette.use_maxcolors>0 && i>sm_palette.use_maxcolors)
	sm_palette.colors = sm_palette.use_maxcolors;

    if (save_pal.colorFormulae < 0
	    || sm_palette.colorFormulae != save_pal.colorFormulae
	    || sm_palette.colorMode != save_pal.colorMode
	    || sm_palette.formulaR != save_pal.formulaR
	    || sm_palette.formulaG != save_pal.formulaG
	    || sm_palette.formulaB != save_pal.formulaB
	    || sm_palette.positive != save_pal.positive
	    || sm_palette.colors != save_pal.colors) {
	/* print the message only if the colors have changed */
	fprintf(stderr,"smooth palette in %s: available %i color positions; using %i of them\n",
	    term->name,i,sm_palette.colors);
    }

    save_pal = sm_palette;

#if TODOSOMEHOW_OTHERWISE_MEMORY_LEAKS
    if (sm_palette.color != NULL ) free( sm_palette.color );
    /* is it sure that there is NULL in the beginning??!!
       That should be released somewhen... after the whole plot
       is there end_plot somewhere? Will there be several palettes
       in the future, i.e. multiplots with various colour schemes?
     */
#endif
    sm_palette.color = malloc( sm_palette.colors * sizeof(rgb_color) );

    for (i = 0; i < sm_palette.colors; i++) {
	gray = (double)i / (sm_palette.colors - 1); /* rescale to [0;1] */
	if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY) /* gray scale only */
	    sm_palette.color[i].r = sm_palette.color[i].g = sm_palette.color[i].b = gray;
	else { /* i.e. sm_palette.colorMode == SMPAL_COLOR_MODE_RGB */
	    sm_palette.color[i].r = GetColorValueFromFormula(sm_palette.formulaR, gray);
	    sm_palette.color[i].g = GetColorValueFromFormula(sm_palette.formulaG, gray);
	    sm_palette.color[i].b = GetColorValueFromFormula(sm_palette.formulaB, gray);
	}
    }
    /* let the terminal make the palette from the supplied RGB triplets */
    term->make_palette(&sm_palette);

#if 0
    GIF_show_current_palette();
#endif

    return 0;
}


/*
   Set the colour on the terminal
   Currently, each terminal takes care of remembering the current colour,
   so there is not much to do here---well, except for reversing the gray
   according to sm_palette.positive == SMPAL_POSITIVE or SMPAL_NEGATIVE
 */
void set_color ( double gray )
{
    if (sm_palette.positive == SMPAL_NEGATIVE)
	gray = 1 - gray;
    term->set_color( gray );
}


#if 0
/*
   Makes mapping from real 3D coordinates to 2D terminal coordinates,
   then draws filled polygon
 */
static void filled_polygon ( int points, gpdPoint *corners )
{
    int i;
    gpiPoint *icorners;
    icorners = malloc(points * sizeof(gpiPoint));
    for (i = 0; i < points; i++)
	map3d_xy(corners[i].x, corners[i].y, corners[i].z,
	    &icorners[i].x, &icorners[i].y);
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
	icorners[0].spec.gray = -1; /* force solid color */
    }
#endif
    term->filled_polygon( points, icorners );
    free( icorners );
}
#endif


/* The routine above for 4 points explicitly.
 * This is the only routine which supportes extended
 * color specs currently.
 */
void
filled_quadrangle(gpdPoint *corners
#ifdef EXTENDED_COLOR_SPECS
    , gpiPoint* icorners
#endif
		 )
{
    int i;
#ifndef EXTENDED_COLOR_SPECS
    gpiPoint icorners[4];
#endif
    for (i = 0; i < 4; i++) {
	map3d_xy(corners[i].x, corners[i].y, corners[i].z,
	    &icorners[i].x, &icorners[i].y);
    }

    term->filled_polygon(4, icorners);

    if (pm3d.hidden3d_tag) {


	/* Colour has changed, thus must apply properties again. That's because
	   gnuplot has no inner notion of color.
	*/
	struct lp_style_type lp;
	lp_use_properties(&lp, pm3d.hidden3d_tag, 1);
	term_apply_lp_properties(&lp);

	term->move(icorners[0].x, icorners[0].y);
	for (i = 3; i >= 0; i--) {
	    term->vector(icorners[i].x, icorners[i].y);
	}
    }
}


/*
   Makes mapping from real 3D coordinates, passed as coords array,
   to 2D terminal coordinates, then draws filled polygon
 */
void filled_polygon_3dcoords ( int points, struct coordinate GPHUGE *coords )
{
    int i;
    gpiPoint *icorners;
    icorners = malloc(points * sizeof(gpiPoint));
    for (i = 0; i < points; i++)
	map3d_xy(coords[i].x, coords[i].y, coords[i].z,
	    &icorners[i].x, &icorners[i].y);
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
	icorners[0].spec.gray = -1; /* force solid color */
    }
#endif
    term->filled_polygon( points, icorners );
    free( icorners );
}


/*
   Makes mapping from real 3D coordinates, passed as coords array, but at z coordinate
   fixed (base_z, for instance) to 2D terminal coordinates, then draws filled polygon
 */
void filled_polygon_3dcoords_zfixed ( int points, struct coordinate GPHUGE *coords, double z )
{
    int i;
    gpiPoint *icorners;
    icorners = malloc(points * sizeof(gpiPoint));
    for (i = 0; i < points; i++)
	map3d_xy(coords[i].x, coords[i].y, z,
	    &icorners[i].x, &icorners[i].y);
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
	icorners[0].spec.gray = -1; /* force solid color */
    }
#endif
    term->filled_polygon( points, icorners );
    free( icorners );
}


/*
   Draw colour smooth box

   Firstly two helper routines for plotting inside of the box
   for postscript and for other terminals, finally the main routine
 */


/* plot the colour smooth box for from terminal's integer coordinates
   [x_from,y_from] to [x_to,y_to].
   This routine is for postscript files --- actually, it writes a small
   PS routine
 */
static void draw_inside_color_smooth_box_postscript
(FILE *out, int x_from, int y_from, int x_to, int y_to)
{
    int scale_x = (x_to-x_from), scale_y = (y_to-y_from);
    fprintf(out,"stroke gsave /imax 1024 def\t%% draw gray scale smooth box\n");
    /* nb. of discrete steps (counted in the loop) */
    fprintf(out,"%i %i translate %i %i scale 0 setlinewidth\n",
	x_from, y_from, scale_x, scale_y);
    /* define left bottom corner and scale of the box so that all coordinates
       of the box are from [0,0] up to [1,1]. Further, this normalization
       makes it possible to pass y from [0,1] as parameter to setgray */
    fprintf(out,"/ystep 1 imax div def /y0 0 def /ii 0 def\n");
    /* local variables; y-step, current y position and counter ii;  */
    if (sm_palette.positive == SMPAL_NEGATIVE) /* inverted gray for negative figure */
	fprintf(out,"{ 1 y0 sub g ");
    else fprintf(out,"{ y0 g ");
    if (color_box.rotation == 'v')
	fprintf(out,"0 y0 N 1 0 V 0 ystep V -1 0 f\n");
    else
	fprintf(out,"y0 0 N 0 1 V ystep 0 V 0 -1 f\n");
    fprintf(out,"/y0 y0 ystep add def /ii ii 1 add def\n");
    fprintf(out,"ii imax gt {exit} if } loop\n");
    fprintf(out,"grestore 0 setgray\n");
} /* end of optimized PS output */

/* plot the colour smooth box for from terminal's integer coordinates
   [x_from,y_from] to [x_to,y_to].
   This routine is for non-postscript files, as it does explicitly the loop
   over all thin rectangles
 */
static void draw_inside_color_smooth_box_bitmap
(FILE * out , int x_from, int y_from, int x_to, int y_to)
{
  int steps = 128; /* I think that nobody can distinguish more colours from the palette */
  int i, xy, xy2, xy_from, xy_to;
  double xy_step, gray;
  gpiPoint corners[4];
  if (color_box.rotation == 'v') {
    corners[0].x = corners[3].x = x_from;
    corners[1].x = corners[2].x = x_to;
    xy_from = y_from;
    xy_to = y_to;
  }
  else {
    corners[0].y = corners[1].y = y_from;
    corners[2].y = corners[3].y = y_to;
    xy_from = x_from;
    xy_to = x_to;
  }
  xy_step = ( color_box.rotation == 'h' ? x_to - x_from : y_to - y_from ) / (double)steps;

  for (i = 0; i < steps; i++) {
    gray = (double)i / (steps-1); /* colours equidistantly from [0,1] */
    /* set the colour (also for terminals which support extended specs) */
    set_color( gray );
    xy  = xy_from + (int)(xy_step * i);
    xy2 = xy_from + (int)(xy_step * (i+1));
    if (color_box.rotation == 'v') {
      corners[0].y = corners[1].y = xy;
      corners[2].y = corners[3].y = (i==steps-1) ? xy_to : xy2;
    }
    else {
      corners[0].x = corners[3].x = xy;
      corners[1].x = corners[2].x = (i==steps-1) ? xy_to : xy2;
    }
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
      corners[0].spec.gray = -1; /* force solid color */
    }
#endif
    /* print the rectangle with the given colour */
    term->filled_polygon( 4, corners );
  }
}

static float
color_box_text_displacement(const char* str, int just)
{
  if (JUST_TOP) {
    if (just && strchr(str, '^') != NULL) /* adjust it = sth like JUST_TOP */
      return 1.15; /* the string contains upper index, so shift the string down */
    else
      return 0.6;
  } else {
    if (strchr(str, '_') != NULL) /* adjust it = sth like JUST_BOT */
      return 1.0; /* the string contains lower index, so shift the string up */
    else
      return 0.75; 
  }
}

/*
   Finally the main colour smooth box drawing routine
 */
void draw_color_smooth_box ()
{
  unsigned int x_from, x_to, y_from, y_to;
  double tmp;
  char s[64];
  extern double base_z, ceiling_z; /* defined in graph3d.c */
  extern FILE *PSLATEX_auxfile; /* defined in pslatex.trm */
  FILE *out = PSLATEX_auxfile ? PSLATEX_auxfile : gpoutfile;
  
  if (color_box.where == SMCOLOR_BOX_NO)
      return;
  if (!term->filled_polygon)
      return;
  
  /*
    firstly, choose some good position of the color box
    
    user's position like that (?):
    else {
    x_from = color_box.xlow;
    x_to   = color_box.xhigh;
    }
  */
  if (color_box.where == SMCOLOR_BOX_USER) {
    x_from = color_box.xorigin * (term->xmax) + 0.5;
    y_from = color_box.yorigin * (term->ymax) + 0.5;
    x_to   = color_box.xsize   * (term->xmax) + 0.5;
    y_to   = color_box.ysize   * (term->ymax) + 0.5;
    x_to += x_from;
    y_to += y_from;
  }
  else { /* color_box.where == SMCOLOR_BOX_DEFAULT */
    double dx = ( max_array[FIRST_X_AXIS] - min_array[FIRST_X_AXIS] );
    double dz = ( used_pm3d_zmax - used_pm3d_zmin );
    map3d_xy(max_array[FIRST_X_AXIS]+dx*0.05,max_array[FIRST_Y_AXIS],base_z+dz*0.35, &x_from,&y_from);
    map3d_xy(max_array[FIRST_X_AXIS]+dx*0.20,max_array[FIRST_Y_AXIS],ceiling_z-dz*0.0, &x_to,&y_to);
    if (y_from == y_to || x_from == x_to) { /* map, i.e. plot with "set view 0,0 or 180,0" */
      dz = max_array[FIRST_Y_AXIS] - min_array[FIRST_Y_AXIS];
      map3d_xy(max_array[FIRST_X_AXIS]+dx*0.04,min_array[FIRST_Y_AXIS]+dz*0.25,base_z, &x_from,&y_from);
      map3d_xy(max_array[FIRST_X_AXIS]+dx*0.18,max_array[FIRST_Y_AXIS]-dz*0.25,ceiling_z, &x_to,&y_to);
    }
  }
  
  if (y_from > y_to) { /* switch them */
    tmp = y_to;
    y_to = y_from;
    y_from = tmp;
  }
  
  /* optimized version of the smooth colour box in postscript. Advantage:
     only few lines of code is written into the output file.
  */
  if (!strcmp(term->name,"postscript") ||
      !strcmp(term->name,"pslatex") || !strcmp(term->name,"pstex")
#ifdef USE_EPSLATEX_DRIVER
      || !strcmp(term->name, "epslatex")
#endif
      )
    draw_inside_color_smooth_box_postscript( out, x_from, y_from, x_to, y_to);
  /* color_box.border , color_box.border_lt_tag); */
  else
    draw_inside_color_smooth_box_bitmap (out, x_from, y_from, x_to, y_to);
  
  if (color_box.border) {
    /* now make boundary around the colour box */
    if (color_box.border_lt_tag >= 0) {
      /* user specified line type */
      struct lp_style_type lp;
      lp_use_properties(&lp, color_box.border_lt_tag, 1);
      term_apply_lp_properties(&lp);
    }  else {
      /* black solid colour should be chosen, so it's border linetype */
      extern struct lp_style_type border_lp;
      term_apply_lp_properties(&border_lp);
    }
    (term->move) (x_from,y_from);
    (term->vector) (x_to,y_from);
    (term->vector) (x_to,y_to);
    (term->vector) (x_from,y_to);
    (term->vector) (x_from,y_from);
    
    /* Ugly */
    /*Set line properties to some value, this also draws lines in postscript terminals!*/
    {
      extern struct lp_style_type border_lp;
      term_apply_lp_properties(&border_lp);
    }
  }

    
  /* and finally place text of min z and max z below and above wrt
     colour box, respectively
  */

  tmp = is_log_z ? exp( used_pm3d_zmin * log_base_array[FIRST_Z_AXIS] ) : used_pm3d_zmin;
#if 0
  sprintf(s,"%g",tmp);
#else /* format the label using `set format z` */
  gprintf(s, sizeof(s), zformat, log_base_array[FIRST_Z_AXIS], tmp);
#endif
  if (color_box.rotation == 'v') {
    if (term->justify_text) term->justify_text(LEFT);
    tmp = color_box_text_displacement(s, JUST_TOP);
    (term->put_text) (x_from, y_from - term->v_char * tmp, s);
  } else {
    if (term->justify_text) term->justify_text(CENTRE);
    if (y_to > term->ymax / 2) {
      /* color box is somewhere at the top, draw the text below */
      tmp = color_box_text_displacement(s, JUST_TOP);
      (term->put_text) (x_from, y_from - term->v_char * tmp, s);
    } else {
      /* color box is somewhere at the bottom, draw the text above */
      tmp = color_box_text_displacement(s, JUST_BOT);
      (term->put_text) (x_from, y_to + term->v_char * tmp, s);
    }
  }

  tmp = is_log_z ? exp( used_pm3d_zmax * log_base_array[FIRST_Z_AXIS] ) : used_pm3d_zmax;
#if 0
  sprintf(s,"%g",tmp);
#else
  gprintf(s, sizeof(s), zformat, log_base_array[FIRST_Z_AXIS], tmp);
#endif
  if (color_box.rotation == 'v') {
    /* text was eventually already left-justified above */
    tmp = color_box_text_displacement(s, JUST_BOT);
    (term->put_text) (x_from,y_to + term->v_char * tmp, s);
  } else {
    if (term->justify_text) term->justify_text(CENTRE);
    if (y_to > term->ymax / 2) {
      tmp = color_box_text_displacement(s, JUST_TOP);
      /* color box is somewhere at the top, draw the text below */
      (term->put_text) (x_to, y_from - term->v_char * tmp, s);
    } else {
      tmp = color_box_text_displacement(s, JUST_BOT);
      /* color box is somewhere at the top, draw the text below */
      (term->put_text) (x_to, y_to + term->v_char * tmp, s);
    }
  }

}


#endif /* PM3D */

/* eof color.c */
