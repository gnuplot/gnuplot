/* GNUPLOT - color.c */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
 * What is here:
 *   - Global variables declared in .h are initialized here
 *   - Palette routines
 *   - Colour box drawing
 *
 * Ethan A Merritt Nov 2021:
 *	move the "set palette" routines here from set.c
]*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "color.h"
#include "getcolor.h"

#include "axis.h"
#include "gadgets.h"
#include "graphics.h"
#include "plot.h"
#include "graph3d.h"
#include "misc.h"
#include "pm3d.h"
#include "term_api.h"
#include "util3d.h"
#include "alloc.h"
#include "command.h"	/* for "set palette" parsing */
#include "datafile.h"	/* for "set palette" parsing */

/* COLOUR MODES - GLOBAL VARIABLES */

t_sm_palette sm_palette;  /* initialized in plot.c on program entry */
int enable_reset_palette = 1;

/* Copy of palette previously in use.
 * Exported so that change_term() can invalidate contents
 */
static t_sm_palette prev_palette = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	(rgb_color *) 0, -1
    };
static palette_color_mode pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;

#define VIRIDIS 1
/* viridis colormap values derived from
 * "viridis - Colorblind-Friendly Color Maps for R"
 * Garnier et al (2021)
 * https://CRAN.R-project.org/package=viridis
 */
static const unsigned int viridis_colormap[256] = {
0x440154, 0x440256, 0x450457, 0x450559, 0x46075a, 0x46085c, 0x460a5d, 0x460b5e, 0x470d60, 0x470e61, 0x471063, 0x471164, 0x471365, 0x481467, 0x481668, 0x481769,
0x48186a, 0x481a6c, 0x481b6d, 0x481c6e, 0x481d6f, 0x481f70, 0x482071, 0x482173, 0x482374, 0x482475, 0x482576, 0x482677, 0x482878, 0x482979, 0x472a7a, 0x472c7a,
0x472d7b, 0x472e7c, 0x472f7d, 0x46307e, 0x46327e, 0x46337f, 0x463480, 0x453581, 0x453781, 0x453882, 0x443983, 0x443a83, 0x443b84, 0x433d84, 0x433e85, 0x423f85,
0x424086, 0x424186, 0x414287, 0x414487, 0x404588, 0x404688, 0x3f4788, 0x3f4889, 0x3e4989, 0x3e4a89, 0x3e4c8a, 0x3d4d8a, 0x3d4e8a, 0x3c4f8a, 0x3c508b, 0x3b518b,
0x3b528b, 0x3a538b, 0x3a548c, 0x39558c, 0x39568c, 0x38588c, 0x38598c, 0x375a8c, 0x375b8d, 0x365c8d, 0x365d8d, 0x355e8d, 0x355f8d, 0x34608d, 0x34618d, 0x33628d,
0x33638d, 0x32648e, 0x32658e, 0x31668e, 0x31678e, 0x31688e, 0x30698e, 0x306a8e, 0x2f6b8e, 0x2f6c8e, 0x2e6d8e, 0x2e6e8e, 0x2e6f8e, 0x2d708e, 0x2d718e, 0x2c718e,
0x2c728e, 0x2c738e, 0x2b748e, 0x2b758e, 0x2a768e, 0x2a778e, 0x2a788e, 0x29798e, 0x297a8e, 0x297b8e, 0x287c8e, 0x287d8e, 0x277e8e, 0x277f8e, 0x27808e, 0x26818e,
0x26828e, 0x26828e, 0x25838e, 0x25848e, 0x25858e, 0x24868e, 0x24878e, 0x23888e, 0x23898e, 0x238a8d, 0x228b8d, 0x228c8d, 0x228d8d, 0x218e8d, 0x218f8d, 0x21908d,
0x21918c, 0x20928c, 0x20928c, 0x20938c, 0x1f948c, 0x1f958b, 0x1f968b, 0x1f978b, 0x1f988b, 0x1f998a, 0x1f9a8a, 0x1e9b8a, 0x1e9c89, 0x1e9d89, 0x1f9e89, 0x1f9f88,
0x1fa088, 0x1fa188, 0x1fa187, 0x1fa287, 0x20a386, 0x20a486, 0x21a585, 0x21a685, 0x22a785, 0x22a884, 0x23a983, 0x24aa83, 0x25ab82, 0x25ac82, 0x26ad81, 0x27ad81,
0x28ae80, 0x29af7f, 0x2ab07f, 0x2cb17e, 0x2db27d, 0x2eb37c, 0x2fb47c, 0x31b57b, 0x32b67a, 0x34b679, 0x35b779, 0x37b878, 0x38b977, 0x3aba76, 0x3bbb75, 0x3dbc74,
0x3fbc73, 0x40bd72, 0x42be71, 0x44bf70, 0x46c06f, 0x48c16e, 0x4ac16d, 0x4cc26c, 0x4ec36b, 0x50c46a, 0x52c569, 0x54c568, 0x56c667, 0x58c765, 0x5ac864, 0x5cc863,
0x5ec962, 0x60ca60, 0x63cb5f, 0x65cb5e, 0x67cc5c, 0x69cd5b, 0x6ccd5a, 0x6ece58, 0x70cf57, 0x73d056, 0x75d054, 0x77d153, 0x7ad151, 0x7cd250, 0x7fd34e, 0x81d34d,
0x84d44b, 0x86d549, 0x89d548, 0x8bd646, 0x8ed645, 0x90d743, 0x93d741, 0x95d840, 0x98d83e, 0x9bd93c, 0x9dd93b, 0xa0da39, 0xa2da37, 0xa5db36, 0xa8db34, 0xaadc32,
0xaddc30, 0xb0dd2f, 0xb2dd2d, 0xb5de2b, 0xb8de29, 0xbade28, 0xbddf26, 0xc0df25, 0xc2df23, 0xc5e021, 0xc8e020, 0xcae11f, 0xcde11d, 0xd0e11c, 0xd2e21b, 0xd5e21a,
0xd8e219, 0xdae319, 0xdde318, 0xdfe318, 0xe2e418, 0xe5e419, 0xe7e419, 0xeae51a, 0xece51b, 0xefe51c, 0xf1e51d, 0xf4e61e, 0xf6e620, 0xf8e621, 0xfbe723, 0xfde725
};


/* Internal prototype declarations: */

static void draw_inside_color_smooth_box_postscript(void);
static void cbtick_callback(struct axis *, double place, char *text, int ticlevel,
			struct lp_style_type grid, struct ticmark *userlabels);
static void check_palette_grayscale(void);
static void set_palette_colormap(void);
static void set_palette_by_name(int nameindex);
static int  set_palette_defined(void);
static void set_palette_file(void);
static void set_palette_function(void);


/* *******************************************************************
  ROUTINES
 */


void
init_color()
{
  /* initialize global palette */
  sm_palette.colorFormulae = 37;  /* const */
  sm_palette.formulaR = 7;
  sm_palette.formulaG = 5;
  sm_palette.formulaB = 15;
  sm_palette.positive = SMPAL_POSITIVE;
  sm_palette.use_maxcolors = 0;
  sm_palette.colors = 0;
  sm_palette.color = NULL;
  sm_palette.ps_allcF = FALSE;
  sm_palette.gradient_num = 0;
  sm_palette.gradient = NULL;
  sm_palette.cmodel = C_MODEL_RGB;
  sm_palette.Afunc.at = sm_palette.Bfunc.at = sm_palette.Cfunc.at = NULL;
  sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
  sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
  sm_palette.gamma = 1.5;
}


/*
   Make the colour palette. Return 0 on success
   Put number of allocated colours into sm_palette.colors
 */
int
make_palette()
{
    int i;
    double gray;

    if (!term->make_palette) {
	return 1;
    }

    /* ask for suitable number of colours in the palette */
    i = term->make_palette(NULL);
    sm_palette.colors = i;
    if (i == 0) {
	/* terminal with its own mapping (PostScript, for instance)
	   It will not change palette passed below, but non-NULL has to be
	   passed there to create the header or force its initialization
	 */
	if (memcmp(&prev_palette, &sm_palette, sizeof(t_sm_palette))) {
	    term->make_palette(&sm_palette);
	    prev_palette = sm_palette;
	    FPRINTF((stderr,"make_palette: calling term->make_palette for term with ncolors == 0\n"));
	} else {
	    FPRINTF((stderr,"make_palette: skipping duplicate palette for term with ncolors == 0\n"));
	}
	return 0;
    }

    /* set the number of colours to be used (allocated) */
    if (CHECK_SMPAL_IS_DISCRETE_GRADIENT) {
	sm_palette.colors = sm_palette.gradient_num;
    } else if (sm_palette.use_maxcolors > 0) {
	if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRADIENT)
	    sm_palette.colors = i;	/* EAM Sep 2010 - could this be a constant? */
	else if (i > sm_palette.use_maxcolors)
	    sm_palette.colors = sm_palette.use_maxcolors;
    }

    if (prev_palette.colorFormulae < 0
	|| sm_palette.colorFormulae != prev_palette.colorFormulae
	|| sm_palette.colorMode != prev_palette.colorMode
	|| sm_palette.formulaR != prev_palette.formulaR
	|| sm_palette.formulaG != prev_palette.formulaG
	|| sm_palette.formulaB != prev_palette.formulaB
	|| sm_palette.positive != prev_palette.positive
	|| sm_palette.colors != prev_palette.colors) {
	/* print the message only if colors have changed */
	if (interactive)
	    fprintf(stderr, "smooth palette in %s: using %i of %i available color positions\n",
	    		term->name, sm_palette.colors, i);
    }

    prev_palette = sm_palette;

    if (sm_palette.color != NULL) {
	free(sm_palette.color);
	sm_palette.color = NULL;
    }
    sm_palette.color = gp_alloc( sm_palette.colors * sizeof(rgb_color),
				 "pm3d palette color");

    if (CHECK_SMPAL_IS_DISCRETE_GRADIENT) {
	for (i = 0; i < sm_palette.colors; i++) {
	    sm_palette.color[i] = sm_palette.gradient[i].col;
	}
    } else {
	/*  fill sm_palette.color[]  */
	for (i = 0; i < sm_palette.colors; i++) {
	    gray = (double) i / (sm_palette.colors - 1);	/* rescale to [0;1] */
	    rgb1_from_gray( gray, &(sm_palette.color[i]) );
	}
    }

    /* let the terminal make the palette from the supplied RGB triplets */
    term->make_palette(&sm_palette);

    return 0;
}

/*
 * Force a mismatch between the current palette and whatever is sent next,
 * so that the new one will always be loaded.
 */
void
invalidate_palette()
{
    prev_palette.colors = -1;
}

/*
   Set the colour on the terminal
   Each terminal takes care of remembering the current colour,
   so there is not much to do here.
 */
void
set_color(double gray)
{
    t_colorspec color;

    if (isnan(gray)) {
	term->linetype(LT_NODRAW);
    } else {
	color.type = TC_FRAC;
	color.value = gray;
	term->set_color(&color);
    }
}

void
set_rgbcolor_var(unsigned int rgbvalue)
{
    t_colorspec color;
    color.type = TC_RGB;
    *(unsigned int *)(&color.lt) = rgbvalue;
    color.value = -1;	/* -1 flags that this came from "rgb variable" */
    apply_pm3dcolor(&color);
}

void
set_rgbcolor_const(unsigned int rgbvalue)
{
    t_colorspec color;
    color.type = TC_RGB;
    *(unsigned int *)(&color.lt) = rgbvalue;
    color.value = 0;	/* 0 flags that this is a constant color */
    apply_pm3dcolor(&color);
}

/*
  diagnose the palette gradient in three types.
    1. Smooth gradient (SMPAL_GRADIENT_TYPE_SMOOTH)
    2. Discrete gradient (SMPAL_GRADIENT_TYPE_DISCRETE)
    3. Smooth and Discrete Mixed gradient (SMPAL_GRADIENT_TYPE_MIXED)
*/
void
check_palette_gradient_type ()
{
    int has_smooth_part   = 0;
    int has_discrete_part = 0;
    rgb_color c1, c2;
    double p1, p2;
    int j;

    if ( sm_palette.colorMode != SMPAL_COLOR_MODE_GRADIENT ) {
       sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
       return;
    }

    p1 = sm_palette.gradient[0].pos;
    c1 = sm_palette.gradient[0].col;
    for (j=1; j<sm_palette.gradient_num; j++) {
	p2 = sm_palette.gradient[j].pos;
	c2 = sm_palette.gradient[j].col;
	if ( p1 == p2 ) {
	    has_discrete_part = 1;
	} else if ( c1.r == c2.r && c1.g == c2.g && c1.b == c2.b ) {
	    has_discrete_part = 1;
	} else {
            has_smooth_part = 1;
	}
	p1 = p2;
	c1 = c2;
   }

   if ( ! has_discrete_part ) {
       sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
   } else if ( ! has_smooth_part ) {
       sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_DISCRETE;
   } else {
       sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_MIXED;
   }

   return;
}



/*
   Draw colour smooth box

   Firstly two helper routines for plotting inside of the box
   for postscript and for other terminals, finally the main routine
 */


/* plot the colour smooth box for from terminal's integer coordinates
   This routine is for postscript files --- actually, it writes a small
   PS routine.
 */
static void
draw_inside_color_smooth_box_postscript()
{
    int scale_x = (color_box.bounds.xright - color_box.bounds.xleft), scale_y = (color_box.bounds.ytop - color_box.bounds.ybot);
    fputs("stroke gsave\t%% draw gray scale smooth box\n"
	  "maxcolors 0 gt {/imax maxcolors def} {/imax 1024 def} ifelse\n", gppsfile);

    /* nb. of discrete steps (counted in the loop) */
    fprintf(gppsfile, "%i %i translate %i %i scale 0 setlinewidth\n", color_box.bounds.xleft, color_box.bounds.ybot, scale_x, scale_y);
    /* define left bottom corner and scale of the box so that all coordinates
       of the box are from [0,0] up to [1,1]. Further, this normalization
       makes it possible to pass y from [0,1] as parameter to setgray */
    fprintf(gppsfile, "/ystep 1 imax div def /y0 0 def /ii 0 def\n");
    /* local variables; y-step, current y position and counter ii;  */
    if (sm_palette.positive == SMPAL_NEGATIVE)	/* inverted gray for negative figure */
	fputs("{ 0.99999 y0 sub g ", gppsfile); /* 1 > x > 1-1.0/1024 */
    else
	fputs("{ y0 g ", gppsfile);
    if (color_box.rotation == 'v')
	fputs("0 y0 N 1 0 V 0 ystep V -1 0 f\n", gppsfile);
    else
	fputs("y0 0 N 0 1 V ystep 0 V 0 -1 f\n", gppsfile);
    fputs("/y0 y0 ystep add def /ii ii 1 add def\n"
	  "ii imax ge {exit} if } loop\n"
	  "grestore 0 setgray\n", gppsfile);
}


static int colorbox_steps()
{
    if ( sm_palette.use_maxcolors != 0 )
        return sm_palette.use_maxcolors;
    if ( sm_palette.gradient_num > 128 )
        return sm_palette.gradient_num;

    /* I think that nobody can distinguish more colours drawn in the palette */
    return 128;
}


static void
colorbox_bounds( // out
                 gpiPoint* corners,
                 int*      xy_from,
                 int*      xy_to,
                 double*   xy_step,
                 // in
                 const int steps)
{
    if (color_box.rotation == 'v') {
	corners[0].x = corners[3].x = color_box.bounds.xleft;
	corners[1].x = corners[2].x = color_box.bounds.xright;
	*xy_from = color_box.bounds.ybot;
	*xy_to = color_box.bounds.ytop;

        if(xy_step)
            *xy_step = (color_box.bounds.ytop - color_box.bounds.ybot) / (double)steps;
    } else {
	corners[0].y = corners[1].y = color_box.bounds.ybot;
	corners[2].y = corners[3].y = color_box.bounds.ytop;
	*xy_from = color_box.bounds.xleft;
	*xy_to = color_box.bounds.xright;
        if(xy_step)
            *xy_step = (color_box.bounds.xright - color_box.bounds.xleft) / (double)steps;
    }
}

static void
colorbox_draw_polygon(// output
                      gpiPoint* corners,
                      // input
                      const int xy,
                      const int xy2,
                      const int xy_to)
{
    if (color_box.rotation == 'v') {
        corners[0].y = corners[1].y = xy;
        corners[2].y = corners[3].y = GPMIN(xy_to,xy2+1);
    } else {
        corners[0].x = corners[3].x = xy;
        corners[1].x = corners[2].x = GPMIN(xy_to,xy2+1);
    }

    /* print the rectangle with the given colour */
    if (default_fillstyle.fillstyle == FS_EMPTY)
        corners->style = FS_OPAQUE;
    else
        corners->style = style_from_fill(&default_fillstyle);
    term->filled_polygon(4, corners);
}


static void colorbox_next_step(// output
                               int* xy,
                               int* xy2,
                               // input
                               const int xy_from,
                               const int i,
                               const double xy_step)
{
    /* Start from one pixel beyond the previous box */
    *xy = *xy2;
    *xy2 = xy_from + (int) (xy_step * (i + 1));
}

/* plot a colour smooth box bounded by the terminal's integer coordinates
   [x_from,y_from] to [x_to,y_to].
   This routine is for non-postscript files and for the Mixed color gradient type
 */
static void
draw_inside_colorbox_bitmap_mixed()
{
    int i, j, xy, xy2, xy_from, xy_to;
    int jmin = 0;
    double xy_step, gray, range;
    gpiPoint corners[4];

    const int steps = colorbox_steps();

    colorbox_bounds( // out
                     corners,
                     &xy_from,
                     &xy_to,
                     &xy_step,
                     // in
                     steps);

    range = (xy_to - xy_from);

    for (i = 0, xy2 = xy_from; i < steps; i++) {

	colorbox_next_step(&xy,&xy2,
	                   xy_from,i,xy_step);

	/* Set the colour for the next range increment */
	/* FIXME - The "1 +" seems wrong, yet it improves the placement in gd */
	gray = (double)(1 + xy - xy_from) / range;
	if (sm_palette.positive == SMPAL_NEGATIVE)
	    gray = 1 - gray;
	set_color(gray);

	/* If this is a defined palette, make sure that the range increment */
	/* does not straddle a palette segment boundary. If it does, split  */
	/* it into two parts.                                               */
	if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRADIENT)
	    for (j=jmin; j<sm_palette.gradient_num; j++) {
		int boundary = xy_from + (int)(sm_palette.gradient[j].pos * range);
		if (xy >= boundary) {
		    jmin = j;
		} else {
		    if (xy2 > boundary) {
			xy2 = boundary;
			i--;
			break;
		    }
		}
		if (xy2 < boundary)
		    break;
	    }

	colorbox_draw_polygon(corners,
	                      xy,xy2,xy_to);
    }
}

/* plot a colour smooth box bounded by the terminal's integer coordinates
   [x_from,y_from] to [x_to,y_to].
   This routine is for non-postscript files and for the Discrete color gradient type
 */
static void
draw_inside_colorbox_bitmap_discrete ()
{
    int steps;
    int i, i0, i1, xy, xy2, xy_from, xy_to;
    double gray, range;
    gpiPoint corners[4];

    steps = sm_palette.gradient_num;
    colorbox_bounds( // out
                     corners,
                     &xy_from,
                     &xy_to,
                     NULL,
                     // in
                     steps);

    range = (xy_to - xy_from);

    for (i = 0; i < steps-1; i++) {

	if (sm_palette.positive == SMPAL_NEGATIVE) {
	    i0 = steps-1 - i;
	    i1 = i0 - 1;
	} else {
	    i0 = i;
	    i1 = i0 + 1;
	}

        xy  = xy_from + (int) (sm_palette.gradient[i0].pos * range);
        xy2 = xy_from + (int) (sm_palette.gradient[i1].pos * range);

	if ( xy2 - xy == 0 ) {
	     continue;
	}

        gray = sm_palette.gradient[i1].pos;
        set_color(gray);

	colorbox_draw_polygon(corners,
	                      xy,xy2,xy_to);
    }
}

/* plot a single gradient to the colorbox at [x_from,y_from] to [x_to,y_to].
 * This routine is for non-postscript files and for the Smooth color gradient type.
 * It uses term->filled_polygon() to build the gradient, one box per color segment.
 */
static void
draw_inside_colorbox_bitmap_smooth__filled_polygon()
{
    int i, xy, xy2, xy_from, xy_to;
    double xy_step, gray;
    gpiPoint corners[4];

    const int steps = colorbox_steps();

    colorbox_bounds( // out
                     corners,
                     &xy_from,
                     &xy_to,
                     &xy_step,
                     // in
                     steps);

    for (i = 0, xy2 = xy_from; i < steps; i++) {

	colorbox_next_step(&xy,&xy2,
	                   xy_from,i,xy_step);

	gray = i / (double)steps;

	if ( sm_palette.use_maxcolors != 0 ) {
	    gray = quantize_gray(gray);
	}
	if (sm_palette.positive == SMPAL_NEGATIVE)
	    gray = 1 - gray;
        set_color(gray);

	colorbox_draw_polygon(corners,
	                      xy,xy2,xy_to);
    }
}

/* plot a single gradient to the colorbox at [x_from,y_from] to [x_to,y_to].
 * This routine is for non-postscript files and for the Smooth color gradient type.
 * It uses term->image() render the gradient, one pixel per color segment.
 */
static void
draw_inside_colorbox_bitmap_smooth__image()
{
    gpiPoint corners[4] = {
         {.x = color_box.bounds.xleft,  .y = color_box.bounds.ytop},
         {.x = color_box.bounds.xright, .y = color_box.bounds.ybot},
         {.x = color_box.bounds.xleft,  .y = color_box.bounds.ytop},
         {.x = color_box.bounds.xright, .y = color_box.bounds.ybot}
    };

    const int steps = colorbox_steps();

    coordval image[3*steps];

    FPRINTF((stderr, "...using draw_inside_colorbox_bitmap_smooth__image\n"));

    for (int i = 0; i < steps; i++) {
	rgb_color rgb1;
        double gray = (double)i / (double)(steps-1);

	if ( sm_palette.use_maxcolors != 0 ) {
	    gray = quantize_gray(gray);
	}
	if (sm_palette.positive == SMPAL_NEGATIVE)
	    gray = 1 - gray;

        // Need to unconditionally invert this for some reason
        if (color_box.rotation == 'v')
            gray = 1 - gray;

        rgb1maxcolors_from_gray( gray, &rgb1 );
        image[3*i + 0] = rgb1.r;
        image[3*i + 1] = rgb1.g;
        image[3*i + 2] = rgb1.b;
    }


    if (color_box.rotation == 'v')
        term->image(1, steps, image, corners, IC_RGB);
    else
        term->image(steps, 1, image, corners, IC_RGB);
}

static void
draw_inside_colorbox_bitmap_smooth()
{
    /* The primary beneficiary of the image variant is cairo + pdf,
     * since it avoids banding artifacts in the filled_polygon variant.
     * FIXME: if this causes problems for other terminal types we can
     *        switch the condition to if (!strcmp(term->name,"pdfcairo"))
     */
    if ((term->flags & TERM_COLORBOX_IMAGE))
        draw_inside_colorbox_bitmap_smooth__image();
    else
        draw_inside_colorbox_bitmap_smooth__filled_polygon();
}


static void
cbtick_callback(
    struct axis *this_axis,
    double place,
    char *text,
    int ticlevel,
    struct lp_style_type grid, /* linetype or -2 for no grid */
    struct ticmark *userlabels)
{
    int len = tic_scale(ticlevel, this_axis)
	* (this_axis->tic_in ? -1 : 1) * (term->h_tic);
    unsigned int x1, y1, x2, y2;
    double cb_place;

    /* position of tic as a fraction of the full palette range */
    if (this_axis->linked_to_primary) {
	AXIS * primary = this_axis->linked_to_primary;
	place = eval_link_function(primary, place);
	cb_place = (place - primary->min) / (primary->max - primary->min);
    } else
	cb_place = (place - this_axis->min) / (this_axis->max - this_axis->min);

    /* calculate tic position */
    if (color_box.rotation == 'h') {
	x1 = x2 = color_box.bounds.xleft + cb_place * (color_box.bounds.xright - color_box.bounds.xleft);
	y1 = color_box.bounds.ybot;
	y2 = color_box.bounds.ybot - len;
    } else {
	x1 = color_box.bounds.xright;
	x2 = color_box.bounds.xright + len;
	y1 = y2 = color_box.bounds.ybot + cb_place * (color_box.bounds.ytop - color_box.bounds.ybot);
    }

    /* draw grid line */
    if (grid.l_type > LT_NODRAW) {
	term_apply_lp_properties(&grid);	/* grid linetype */
	if (color_box.rotation == 'h') {
	    (*term->move) (x1, color_box.bounds.ybot);
	    (*term->vector) (x1, color_box.bounds.ytop);
	} else {
	    (*term->move) (color_box.bounds.xleft, y1);
	    (*term->vector) (color_box.bounds.xright, y1);
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
    }

    /* draw tic */
    if (len != 0) {
	int lt = color_box.cbtics_lt_tag;
	if (lt <= 0)
	    lt = color_box.border_lt_tag;
	if (lt > 0) {
	    lp_style_type lp = border_lp;
	    lp_use_properties(&lp, lt);
	    term_apply_lp_properties(&lp);
	}
	(*term->move) (x1, y1);
	(*term->vector) (x2, y2);
	if (this_axis->ticmode & TICS_MIRROR) {
	    if (color_box.rotation == 'h') {
		y1 = color_box.bounds.ytop;
		y2 = color_box.bounds.ytop + len;
	    } else {
		x1 = color_box.bounds.xleft;
		x2 = color_box.bounds.xleft - len;
	    }
	    (*term->move) (x1, y1);
	    (*term->vector) (x2, y2);
	}
	if (lt != 0)
	    term_apply_lp_properties(&border_lp);
    }

    /* draw label */
    if (text) {
	int just;
	int offsetx, offsety;

	/* Skip label if we've already written a user-specified one here */
#	define MINIMUM_SEPARATION 0.001
	while (userlabels) {
	    if (fabs((place - userlabels->position) / (CB_AXIS.max - CB_AXIS.min))
		<= MINIMUM_SEPARATION) {
		text = NULL;
		break;
	    }
	    userlabels = userlabels->next;
	}
#	undef MINIMUM_SEPARATION

	/* get offset */
	map3d_position_r(&(this_axis->ticdef.offset),
			 &offsetx, &offsety, "cbtics");
	/* User-specified different color for the tics text */
	if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	    apply_pm3dcolor(&(this_axis->ticdef.textcolor));
	if (color_box.rotation == 'h') {
	    int y3 = color_box.bounds.ybot - (term->v_char);
	    int hrotate = 0;

	    if (this_axis->tic_rotate
		&& (*term->text_angle)(this_axis->tic_rotate))
		    hrotate = this_axis->tic_rotate;
	    if (len > 0) y3 -= len; /* add outer tics len */
	    if (y3<0) y3 = 0;
	    just = hrotate ? LEFT : CENTRE;
	    if (this_axis->manual_justify)
		just = this_axis->tic_pos;
	    write_multiline(x2+offsetx, y3+offsety, text,
			    just, JUST_CENTRE, hrotate,
			    this_axis->ticdef.font);
	    if (hrotate)
		(*term->text_angle)(0);
	} else {
	    unsigned int x3 = color_box.bounds.xright + (term->h_char);
	    if (len > 0) x3 += len; /* add outer tics len */
	    just = LEFT;
	    if (this_axis->manual_justify)
		just = this_axis->tic_pos;
	    write_multiline(x3+offsetx, y2+offsety, text,
			    just, JUST_CENTRE, 0.0,
			    this_axis->ticdef.font);
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
    }
}

/*
   Finally the main colour smooth box drawing routine
 */
void
draw_color_smooth_box(int plot_mode)
{
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
	if (!is_3d_plot) {
	    double xtemp, ytemp;
	    map_position(&color_box.origin, &color_box.bounds.xleft, &color_box.bounds.ybot, "cbox");
	    map_position_r(&color_box.size, &xtemp, &ytemp, "cbox");
	    color_box.bounds.xright = xtemp;
	    color_box.bounds.ytop = ytemp;
	} else if (splot_map && is_3d_plot) {
	    /* In map view mode we allow any coordinate system for placement */
	    double xtemp, ytemp;
	    map3d_position_double(&color_box.origin, &xtemp, &ytemp, "cbox");
	    color_box.bounds.xleft = xtemp;
	    color_box.bounds.ybot = ytemp;
	    map3d_position_r(&color_box.size, &color_box.bounds.xright, &color_box.bounds.ytop, "cbox");
	} else {
	    /* But in full 3D mode we only allow screen coordinates */
	    color_box.bounds.xleft = color_box.origin.x * (term->xmax) + 0.5;
	    color_box.bounds.ybot = color_box.origin.y * (term->ymax) + 0.5;
	    color_box.bounds.xright = color_box.size.x * (term->xmax-1) + 0.5;
	    color_box.bounds.ytop = color_box.size.y * (term->ymax-1) + 0.5;
	}
	color_box.bounds.xright += color_box.bounds.xleft;
	color_box.bounds.ytop += color_box.bounds.ybot;

    } else { /* color_box.where == SMCOLOR_BOX_DEFAULT */
	if (plot_mode == MODE_SPLOT && !splot_map) {
	    /* general 3D plot */
	    color_box.bounds.xleft = xmiddle + 0.709 * xscaler;
	    color_box.bounds.xright   = xmiddle + 0.778 * xscaler;
	    color_box.bounds.ybot = ymiddle - 0.147 * yscaler;
	    color_box.bounds.ytop   = ymiddle + 0.497 * yscaler;
	} else {
	    /* 2D plot (including splot map) */
	    struct position default_origin = {graph,graph,graph, 1.025, 0, 0};
	    struct position default_size = {graph,graph,graph, 0.05, 1.0, 0};
	    double xtemp, ytemp;
	    map_position(&default_origin, &color_box.bounds.xleft, &color_box.bounds.ybot, "cbox");
	    color_box.bounds.xleft += color_box.xoffset;
	    map_position_r(&default_size, &xtemp, &ytemp, "cbox");
	    color_box.bounds.xright = xtemp + color_box.bounds.xleft;
	    color_box.bounds.ytop = ytemp + color_box.bounds.ybot;
	}

	/* now corrections for outer tics */
	if (color_box.rotation == 'v') {
	    int cblen = (CB_AXIS.tic_in ? -1 : 1) * CB_AXIS.ticscale * 
		(term->h_tic); /* positive for outer tics */
	    int ylen = (Y_AXIS.tic_in ? -1 : 1) * Y_AXIS.ticscale * 
		(term->h_tic); /* positive for outer tics */
	    if ((cblen > 0) && (CB_AXIS.ticmode & TICS_MIRROR)) {
		color_box.bounds.xleft += cblen;
		color_box.bounds.xright += cblen;
	    }
	    if ((ylen > 0) && 
		(axis_array[FIRST_Y_AXIS].ticmode & TICS_MIRROR)) {
		color_box.bounds.xleft += ylen;
		color_box.bounds.xright += ylen;
	    }
	}
    }

    if (color_box.bounds.ybot > color_box.bounds.ytop) {
	double tmp = color_box.bounds.ytop;
	color_box.bounds.ytop = color_box.bounds.ybot;
	color_box.bounds.ybot = tmp;
    }
    if (color_box.invert && color_box.rotation == 'v') {
	double tmp = color_box.bounds.ytop;
	color_box.bounds.ytop = color_box.bounds.ybot;
	color_box.bounds.ybot = tmp;
    }

    term->layer(TERM_LAYER_BEGIN_COLORBOX);

    if (sm_palette.gradient_type == SMPAL_GRADIENT_TYPE_DISCRETE) {
        draw_inside_colorbox_bitmap_discrete();
    } else {
        /* The PostScript terminal has an Optimized version */
        if ((term->flags & TERM_IS_POSTSCRIPT) != 0)
            draw_inside_color_smooth_box_postscript();
        else if (sm_palette.gradient_type == SMPAL_GRADIENT_TYPE_SMOOTH)
	    draw_inside_colorbox_bitmap_smooth();
	else
	    draw_inside_colorbox_bitmap_mixed();
    }

    term->layer(TERM_LAYER_END_COLORBOX);

    if (color_box.border) {
	/* now make boundary around the colour box */
	if (color_box.border_lt_tag >= 0) {
	    /* user specified line type */
	    struct lp_style_type lp = border_lp;
	    lp_use_properties(&lp, color_box.border_lt_tag);
	    term_apply_lp_properties(&lp);
	} else {
	    /* black solid colour should be chosen, so it's border linetype */
	    term_apply_lp_properties(&border_lp);
	}
	newpath();
	(term->move) (color_box.bounds.xleft, color_box.bounds.ybot);
	(term->vector) (color_box.bounds.xright, color_box.bounds.ybot);
	(term->vector) (color_box.bounds.xright, color_box.bounds.ytop);
	(term->vector) (color_box.bounds.xleft, color_box.bounds.ytop);
	(term->vector) (color_box.bounds.xleft, color_box.bounds.ybot);
	closepath();

	/* Set line properties to some value, this also draws lines in postscript terminals. */
	    term_apply_lp_properties(&border_lp);
	}

    /* draw tics */
    if (axis_array[COLOR_AXIS].ticmode) {
	term_apply_lp_properties(&border_lp); /* border linetype */
	gen_tics(&axis_array[COLOR_AXIS], cbtick_callback );
    }

    /* write the colour box label */
    if (CB_AXIS.label.text) {
	int x, y;
	int len;
	float save_rotation = CB_AXIS.label.rotate;
	apply_pm3dcolor(&(CB_AXIS.label.textcolor));
	if (color_box.rotation == 'h') {
	    len = CB_AXIS.ticscale * (CB_AXIS.tic_in ? 1 : -1) * (term->v_tic);

	    x = (color_box.bounds.xleft + color_box.bounds.xright) / 2;
	    y = color_box.bounds.ybot - 2.7 * term->v_char;

	    if (len < 0) y += len;
	    if (CB_AXIS.label.rotate == TEXT_VERTICAL)
		CB_AXIS.label.rotate = 0;
	} else {
	    len = CB_AXIS.ticscale * (CB_AXIS.tic_in ? -1 : 1) * (term->h_tic);
	    /* calculate max length of cb-tics labels */
	    widest_tic_strlen = 0;
	    if (CB_AXIS.ticmode & TICS_ON_BORDER) /* Recalculate widest_tic_strlen */
		gen_tics(&axis_array[COLOR_AXIS], widest_tic_callback);
	    x = color_box.bounds.xright + (widest_tic_strlen + 1.5) * term->h_char;
	    if (len > 0) x += len;
	    y = (color_box.bounds.ybot + color_box.bounds.ytop) / 2;
	}
	if (x<0) x = 0;
	if (y<0) y = 0;
	write_label(x, y, &(CB_AXIS.label));
	reset_textcolor(&(CB_AXIS.label.textcolor));
	CB_AXIS.label.rotate = save_rotation;
    }

}

/*
 * User-callable builtin color conversion 
 */
void
f_hsv2rgb(union argument *arg)
{
    struct value h, s, v, result;
    rgb_color color = {0., 0., 0.};

    (void) arg;
    (void) pop(&v);
    (void) pop(&s);
    (void) pop(&h);

    if (h.type == INTGR)
	color.r = h.v.int_val;
    else if (h.type == CMPLX)
	color.r = h.v.cmplx_val.real;
    if (s.type == INTGR)
	color.g = s.v.int_val;
    else if (s.type == CMPLX)
	color.g = s.v.cmplx_val.real;
    if (v.type == INTGR)
	color.b = v.v.int_val;
    else if (v.type == CMPLX)
	color.b = v.v.cmplx_val.real;

    if (color.r < 0)
	color.r = 0;
    if (color.g < 0)
	color.g = 0;
    if (color.b < 0)
	color.b = 0;
    if (color.r > 1.)
	color.r = 1.;
    if (color.g > 1.)
	color.g = 1.;
    if (color.b > 1.)
	color.b = 1.;

    (void) Ginteger(&result, hsv2rgb(&color));
    push(&result);
}

/*
 * user-callable lookup of palette color for specific z-value
 */
void
f_palette(union argument *arg)
{
    struct value result;
    double z;
    unsigned int rgb;

    pop(&result);
    z = real(&result);
    if (((CB_AXIS.set_autoscale & AUTOSCALE_BOTH) != 0)
    && (fabs(CB_AXIS.min) >= VERYLARGE || fabs(CB_AXIS.max) >= VERYLARGE))
	int_error(NO_CARET, "palette(z) requires known cbrange");
    rgb = rgb_from_gray(cb2gray(z));

    push(Ginteger(&result, rgb));
}

/*
 * User-callable interpretation of a string as a 24bit RGB color
 * replicating the colorspec interpretation in e.g. 'linecolor rgb "foo"'.
 */
void
f_rgbcolor(union argument *arg)
{
    struct value a;
    long rgb;

    pop(&a);
    if (a.type == STRING) {
	rgb = lookup_color_name(a.v.string_val);
	if (rgb == -2)
	    rgb = 0;
	free(a.v.string_val);
    } else {
	rgb = 0;	
    }

    push(Ginteger(&a, rgb));
}

unsigned int
rgb_from_gray( double gray )
{
    rgb255_color color;
    rgb255maxcolors_from_gray( gray, &color );
    return (unsigned int)color.r << 16 | (unsigned int)color.g << 8 | (unsigned int)color.b;
}

/*
 * A colormap can have specific min/max stored internally,
 * but otherwise we use the current cbrange
 */
double
map2gray(double z, udvt_entry *colormap)
{
    double gray;
    double cm_min, cm_max;

	get_colormap_range(colormap, &cm_min, &cm_max);
	if (cm_min == cm_max)
	    gray = cb2gray(z);
	else
	    gray = (z - cm_min) / (cm_max - cm_min);

    return gray;
}

void
get_colormap_range( udvt_entry *colormap, double *cm_min, double *cm_max )
{
	*cm_min = colormap->udv_value.v.value_array[1].v.cmplx_val.imag;
	*cm_max = colormap->udv_value.v.value_array[2].v.cmplx_val.imag;
}

/*
 * gray is in the interval [0:1]
 * colormap is an ARRAY containing a palette of 32-bit ARGB values
 */
unsigned int
rgb_from_colormap(double gray, udvt_entry *colormap)
{
    struct value *palette = colormap->udv_value.v.value_array;
    int size = palette[0].v.int_val;
    unsigned int rgb;

	rgb = (gray <= 0.0) ? palette[1].v.int_val
	    : (gray >= 1.0) ? palette[size].v.int_val
	    : palette[ (int)(floor(size * gray)) + 1].v.int_val
	    ;
    return rgb;
}

/*
 * Interpret the colorspec of a linetype to yield an RGB packed integer.
 * This is not guaranteed to handle colorspecs that were not part of a linetype.
 */
unsigned int
rgb_from_colorspec(struct t_colorspec *tc)
{
    double cbval;

    switch (tc->type) {
	case TC_DEFAULT:
		return 0;
	case TC_RGB:
		return tc->lt;
	case TC_Z:
		cbval = cb2gray(tc->value);
		break;
	case TC_CB:
		cbval = (CB_AXIS.log && tc->value <= 0) ? CB_AXIS.min : tc->value;
		cbval = cb2gray(cbval);
		break;
	case TC_FRAC:
		cbval = (sm_palette.positive == SMPAL_POSITIVE) ?  tc->value : 1-tc->value;
		break;
	case TC_COLORMAP:
		/* not handled but perhaps it should be? */
	default:
		/* cannot happen in a linetype */
		return 0;
    }

    return rgb_from_gray(cbval);
}

/*
 * "set palette" routines (moved from set.c)
 */

/* Process 'set palette defined' gradient specification */
/* Syntax
 *   set palette defined   -->  use default palette
 *   set palette defined ( <pos1> <colorspec1>, ... , <posN> <colorspecN> )
 *     <posX>  gray value, automatically rescaled to [0, 1]
 *     <colorspecX>   :=  { "<color_name>" | "<X-style-color>" |  <r> <g> <b> }
 *        <color_name>     predefined colors (see below)
 *        <X-style-color>  "#rrggbb" with 2char hex values for red, green, blue
 *        <r> <g> <b>      three values in [0, 1] for red, green and blue
 *   return 1 if named colors where used, 0 otherwise
 */
static int
set_palette_defined()
{
    double p=0, r=0, g=0, b=0;
    int num, named_colors=0;
    int actual_size=8;

    /* Invalidate previous gradient */
    invalidate_palette();

    free( sm_palette.gradient );
    sm_palette.gradient = gp_alloc( actual_size*sizeof(gradient_struct), "pm3d gradient" );
    sm_palette.smallest_gradient_interval = 1;

    if (END_OF_COMMAND) {
	/* some default gradient */
	double pal[][4] = { {0.0, 0.05, 0.05, 0.2}, {0.1, 0, 0, 1},
			    {0.3, 0.05, 0.9, 0.4}, {0.5, 0.9, 0.9, 0},
			    {0.7, 1, 0.6471, 0}, {0.8, 1, 0, 0},
			    {0.9, 0.94, 0.195, 0.195}, {1.0, 1, .8, .8} };

	int i;
	for (i=0; i<8; i++) {
	    sm_palette.gradient[i].pos = pal[i][0];
	    sm_palette.gradient[i].col.r = pal[i][1];
	    sm_palette.gradient[i].col.g = pal[i][2];
	    sm_palette.gradient[i].col.b = pal[i][3];
	}
	sm_palette.gradient_num = 8;
	sm_palette.cmodel = C_MODEL_RGB;
	sm_palette.smallest_gradient_interval = 0.1;  /* From pal[][] */
	c_token--; /* Caller will increment! */
	return 0;
    }

    if ( !equals(c_token,"(") )
	int_error( c_token, "expected ( to start gradient definition" );

    ++c_token;
    num = -1;

    while (!END_OF_COMMAND) {
	char *col_str;
	p = real_expression();
	col_str = try_to_get_string();
	if (col_str) {
	    /* Hex constant or X-style rgb value "#rrggbb" */
	    if (col_str[0] == '#' || col_str[0] == '0') {
		/* X-style specifier */
		int rr,gg,bb;
		if ((sscanf( col_str, "#%2x%2x%2x", &rr, &gg, &bb ) != 3 )
		&&  (sscanf( col_str, "0x%2x%2x%2x", &rr, &gg, &bb ) != 3 ))
		    int_error( c_token-1,
			       "Unknown color specifier. Use '#RRGGBB' of '0xRRGGBB'." );
		r = (double)(rr)/255.;
		g = (double)(gg)/255.;
		b = (double)(bb)/255.;

	    /* Predefined color names.
	     * Could we move these definitions to some file that is included
	     * somehow during compilation instead hardcoding them?
	     * NB: could use lookup_table_nth() but it would not save much.
	     */
	    } else {
		const struct gen_table *tbl = pm3d_color_names_tbl;
		while (tbl->key) {
		    if (!strcmp(col_str, tbl->key)) {
			r = (double)((tbl->value >> 16) & 255) / 255.;
			g = (double)((tbl->value >>  8) & 255) / 255.;
			b = (double)(tbl->value & 255) / 255.;
			break;
		    }
		    tbl++;
		}
		if (!tbl->key)
		    int_error( c_token-1, "Unknown color name." );
		named_colors = 1;
	    }
	    free(col_str);
	} else {
	    /* numerical rgb, hsv, xyz, ... values  [0,1] */
	    r = real_expression();
	    if (r<0 || r>1 )  int_error(c_token-1,"Value out of range [0,1].");
	    g = real_expression();
	    if (g<0 || g>1 )  int_error(c_token-1,"Value out of range [0,1].");
	    b = real_expression();
	    if (b<0 || b>1 )  int_error(c_token-1,"Value out of range [0,1].");
	}
	++num;

	if ( num >= actual_size ) {
	    /* get more space for the gradient */
	    actual_size += 10;
	    sm_palette.gradient = gp_realloc( sm_palette.gradient,
			  actual_size*sizeof(gradient_struct),
			  "pm3d gradient" );
	}
	sm_palette.gradient[num].pos = p;
	sm_palette.gradient[num].col.r = r;
	sm_palette.gradient[num].col.g = g;
	sm_palette.gradient[num].col.b = b;
	if (equals(c_token,")") ) break;
	if ( !equals(c_token,",") )
	    int_error( c_token, "expected comma" );
	++c_token;

    }

    if (num <= 0) {
	reset_palette();
	int_error(c_token, "invalid palette syntax");
    }

    sm_palette.gradient_num = num + 1;
    check_palette_grayscale();

    return named_colors;
}

/*
 *  process 'set palette colormap' command
 */
static void
set_palette_colormap()
{
    int i, actual_size;
    udvt_entry *colormap = get_colormap(c_token);

    if (!colormap)
	int_error(c_token, "expecting colormap name");

    free(sm_palette.gradient);
    sm_palette.gradient = NULL;
    actual_size = colormap->udv_value.v.value_array[0].v.int_val;
    sm_palette.gradient = gp_alloc( actual_size*sizeof(gradient_struct), "gradient" );
    sm_palette.gradient_num = actual_size;

    for (i = 0; i < actual_size; i++) {
	unsigned int rgb24 = colormap->udv_value.v.value_array[i+1].v.int_val;
	sm_palette.gradient[i].col.r = ((rgb24 >> 16) & 0xff) / 255.;
	sm_palette.gradient[i].col.g = ((rgb24 >> 8) & 0xff) / 255.;
	sm_palette.gradient[i].col.b = ((rgb24) & 0xff) / 255.;
	sm_palette.gradient[i].pos = i;
    }

    check_palette_grayscale();
}

/*
 *  process 'set palette <predefined name>' command
 *     currently knows about viridis
 */
static void
set_palette_by_name(int nameindex)
{
    const unsigned int *colormap;
    int colormap_size = 256;
    int i;

    if (nameindex == VIRIDIS)
	colormap = viridis_colormap;
    else
	return;

    free(sm_palette.gradient);
    sm_palette.gradient = NULL;
    sm_palette.gradient = gp_alloc( colormap_size*sizeof(gradient_struct), "gradient" );
    sm_palette.gradient_num = colormap_size;

    for (i = 0; i < colormap_size; i++) {
	unsigned int rgb24 = colormap[i];
	sm_palette.gradient[i].col.r = ((rgb24 >> 16) & 0xff) / 255.;
	sm_palette.gradient[i].col.g = ((rgb24 >> 8) & 0xff) / 255.;
	sm_palette.gradient[i].col.b = ((rgb24) & 0xff) / 255.;
	sm_palette.gradient[i].pos = i;
    }

    check_palette_grayscale();
}


/*  process 'set palette file' command
 *  load a palette from file, honor datafile modifiers
 */
static void
set_palette_file()
{
    double v[4];
    int i, j, actual_size;
    char *name_str;

    ++c_token;

    /* WARNING: do NOT free name_str */
    if (!(name_str = string_or_express(NULL)))
	int_error(c_token, "expecting filename or datablock");

    df_set_plot_mode(MODE_QUERY);	/* Needed only for binary datafiles */
    df_open(name_str, 4, NULL);

    free(sm_palette.gradient);
    sm_palette.gradient = NULL;
    actual_size = 10;
    sm_palette.gradient = gp_alloc( actual_size*sizeof(gradient_struct), "gradient" );

    i = 0;

    /* values are clipped to [0,1] without notice */
    while ((j = df_readline(v, 4)) != DF_EOF) {
	if (i >= actual_size) {
	    actual_size += 10;
	    sm_palette.gradient = gp_realloc( sm_palette.gradient,
		    actual_size*sizeof(gradient_struct), "pm3d gradient" );
	}
	switch (j) {
	    case 1:
		sm_palette.gradient[i].col.r = (0xff & ((int)(v[0]) >> 16)) / 255.;
		sm_palette.gradient[i].col.g = (0xff & ((int)(v[0]) >> 8))  / 255.;
		sm_palette.gradient[i].col.b = (0xff & ((int)(v[0])))       / 255.;
		sm_palette.gradient[i].pos = i;
		break;
	    case 2:
		sm_palette.gradient[i].col.r = (0xff & ((int)(v[1]) >> 16)) / 255.;
		sm_palette.gradient[i].col.g = (0xff & ((int)(v[1]) >> 8))  / 255.;
		sm_palette.gradient[i].col.b = (0xff & ((int)(v[1])))       / 255.;
		sm_palette.gradient[i].pos = v[0];
		break;
	    case 3:
		sm_palette.gradient[i].col.r = clip_to_01(v[0]);
		sm_palette.gradient[i].col.g = clip_to_01(v[1]);
		sm_palette.gradient[i].col.b = clip_to_01(v[2]);
		sm_palette.gradient[i].pos = i;
		break;
	    case 4:
		sm_palette.gradient[i].col.r = clip_to_01(v[1]);
		sm_palette.gradient[i].col.g = clip_to_01(v[2]);
		sm_palette.gradient[i].col.b = clip_to_01(v[3]);
		sm_palette.gradient[i].pos = v[0];
		break;
	    case DF_UNDEFINED:
	    case DF_MISSING:
	    case DF_COLUMN_HEADERS:
		continue;
		break;
	    default:
		df_close();
		int_error(c_token, "Bad data on line %d", df_line_number);
		break;
	}
	++i;
    }
    df_close();
    if (i==0)
	int_error( c_token, "No valid palette found" );

    sm_palette.gradient_num = i;
    check_palette_grayscale();
}


/* Process a 'set palette function' command.
 *  Three functions with fixed dummy variable gray are registered which
 *  map gray to the different color components.
 *  The dummy variable must be 'gray'.
 */
static void
set_palette_function()
{
    int start_token;
    char saved_dummy_var[MAX_ID_LEN+1];

    ++c_token;
    strcpy( saved_dummy_var, c_dummy_var[0]);

    /* set dummy variable */
    strncpy( c_dummy_var[0], "gray", MAX_ID_LEN );

    /* Afunc */
    start_token = c_token;
    if (sm_palette.Afunc.at) {
	free_at( sm_palette.Afunc.at );
	sm_palette.Afunc.at = NULL;
    }
    dummy_func = &sm_palette.Afunc;
    sm_palette.Afunc.at = perm_at();
    if (! sm_palette.Afunc.at)
	int_error(start_token, "not enough memory for function");
    m_capture(&(sm_palette.Afunc.definition), start_token, c_token-1);
    dummy_func = NULL;
    if (!equals(c_token,","))
	int_error(c_token,"expected comma" );
    ++c_token;

    /* Bfunc */
    start_token = c_token;
    if (sm_palette.Bfunc.at) {
	free_at( sm_palette.Bfunc.at );
	sm_palette.Bfunc.at = NULL;
    }
    dummy_func = &sm_palette.Bfunc;
    sm_palette.Bfunc.at = perm_at();
    if (! sm_palette.Bfunc.at)
	int_error(start_token, "not enough memory for function");
    m_capture(&(sm_palette.Bfunc.definition), start_token, c_token-1);
    dummy_func = NULL;
    if (!equals(c_token,","))
	int_error(c_token,"expected comma" );
    ++c_token;

    /* Cfunc */
    start_token = c_token;
    if (sm_palette.Cfunc.at) {
	free_at( sm_palette.Cfunc.at );
	sm_palette.Cfunc.at = NULL;
    }
    dummy_func = &sm_palette.Cfunc;
    sm_palette.Cfunc.at = perm_at();
    if (! sm_palette.Cfunc.at)
	int_error(start_token, "not enough memory for function");
    m_capture(&(sm_palette.Cfunc.definition), start_token, c_token-1);
    dummy_func = NULL;

    strcpy( c_dummy_var[0], saved_dummy_var);
}


/*
 *  Normalize gray scale of gradient to fill [0,1] and
 *  complain if gray values are not strictly increasing.
 *  Maybe automatic sorting of the gray values could be a
 *  feature.
 */
static void
check_palette_grayscale()
{
    int i;
    double off, f;
    gradient_struct *gradient = sm_palette.gradient;

    /* check if gray values are sorted */
    for (i=0; i<sm_palette.gradient_num-1; ++i ) {
	if (gradient[i].pos > gradient[i+1].pos)
	    int_error( c_token, "Palette gradient not monotonic" );
    }

    /* fit gray axis into [0:1]:  subtract offset and rescale */
    off = gradient[0].pos;
    f = 1.0 / ( gradient[sm_palette.gradient_num-1].pos-off );
    for (i=1; i<sm_palette.gradient_num-1; ++i ) {
	gradient[i].pos = f*(gradient[i].pos-off);
    }

    /* paranoia on the first and last entries */
    gradient[0].pos = 0.0;
    gradient[sm_palette.gradient_num-1].pos = 1.0;

    /* save smallest interval */
    sm_palette.smallest_gradient_interval = 1.0;
    for (i=1; i<sm_palette.gradient_num-1; ++i ) {
	if (((gradient[i].pos - gradient[i-1].pos) > 0)
	&&  (sm_palette.smallest_gradient_interval > (gradient[i].pos - gradient[i-1].pos)))
	     sm_palette.smallest_gradient_interval = (gradient[i].pos - gradient[i-1].pos);
    }
}

#define CHECK_TRANSFORM  do {				  \
    if (transform_defined)				  \
	int_error(c_token, "inconsistent palette options" ); \
    transform_defined = 1;				  \
}  while(0)

/* 'set palette' command called from set.c */
void
set_palette()
{
    int transform_defined = 0;
    int named_color = 0;

    c_token++;

    if (END_OF_COMMAND) /* reset to default settings */
	reset_palette();
    else { /* go through all options of 'set palette' */
	for ( ; !END_OF_COMMAND; c_token++ ) {
	    switch (lookup_table(&set_palette_tbl[0],c_token)) {
	    /* positive and negative picture */
	    case S_PALETTE_POSITIVE: /* "pos$itive" */
		sm_palette.positive = SMPAL_POSITIVE;
		continue;
	    case S_PALETTE_NEGATIVE: /* "neg$ative" */
		sm_palette.positive = SMPAL_NEGATIVE;
		continue;
	    /* Now the options that determine the palette of smooth colours */
	    /* gray or rgb-coloured */
	    case S_PALETTE_GRAY: /* "gray" */
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRAY;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
		continue;
	    case S_PALETTE_GAMMA: /* "gamma" */
		++c_token;
		sm_palette.gamma = real_expression();
		--c_token;
		continue;
	    case S_PALETTE_COLOR: /* "col$or" */
		if (pm3d_last_set_palette_mode != SMPAL_COLOR_MODE_NONE) {
		    sm_palette.colorMode = pm3d_last_set_palette_mode;
		} else {
		    sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
                    sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
		}
		continue;
	    /* rgb color mapping formulae: rgb$formulae r,g,b (3 integers) */
	    case S_PALETTE_RGBFORMULAE: { /* "rgb$formulae" */
		int i;
		char * formerr = "color formula out of range (use `show palette rgbformulae' to display the range)";

		CHECK_TRANSFORM;
		c_token++;
		i = int_expression();
		if (abs(i) >= sm_palette.colorFormulae)
		    int_error(c_token, formerr);
		sm_palette.formulaR = i;
		if (!equals(c_token--,","))
		    continue;
		c_token += 2;
		i = int_expression();
		if (abs(i) >= sm_palette.colorFormulae)
		    int_error(c_token, formerr);
		sm_palette.formulaG = i;
		if (!equals(c_token--,","))
		    continue;
		c_token += 2;
		i = int_expression();
		if (abs(i) >= sm_palette.colorFormulae)
		    int_error(c_token, formerr);
		sm_palette.formulaB = i;
		c_token--;
		sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_RGB;
		continue;
	    } /* rgbformulae */
	    /* rgb color mapping based on the "cubehelix" scheme proposed by */
	    /* D A Green (2011)  http://arxiv.org/abs/1108.5083		     */
	    case S_PALETTE_CUBEHELIX: { /* cubehelix */
		TBOOLEAN done = FALSE;
		CHECK_TRANSFORM;
		sm_palette.colorMode = SMPAL_COLOR_MODE_CUBEHELIX;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
		sm_palette.cmodel = C_MODEL_RGB;
		sm_palette.cubehelix_start = 0.5;
		sm_palette.cubehelix_cycles = -1.5;
		sm_palette.cubehelix_saturation = 1.0;
		c_token++;
		do {
		    if (equals(c_token,"start")) {
			c_token++;
			sm_palette.cubehelix_start = real_expression();
		    }
		    else if (almost_equals(c_token,"cyc$les")) {
			c_token++;
			sm_palette.cubehelix_cycles = real_expression();
		    }
		    else if (almost_equals(c_token, "sat$uration")) {
			c_token++;
			sm_palette.cubehelix_saturation = real_expression();
		    }
		    else
			done = TRUE;
		} while (!done);
		--c_token;
		continue;
	    } /* cubehelix */
	    case S_PALETTE_VIRIDIS: {
		CHECK_TRANSFORM;
		set_palette_by_name(VIRIDIS);
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
		continue;
	    }
	    case S_PALETTE_COLORMAP: { /* colormap */
		CHECK_TRANSFORM;
		++c_token;
		set_palette_colormap();
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
                check_palette_gradient_type();
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
		continue;
	    }
	    case S_PALETTE_DEFINED: { /* "def$ine" */
		CHECK_TRANSFORM;
		++c_token;
		named_color = set_palette_defined();
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_NONE;
                check_palette_gradient_type();
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
		continue;
	    }
	    case S_PALETTE_FILE: { /* "file" */
		CHECK_TRANSFORM;
		set_palette_file();
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_NONE;
                check_palette_gradient_type();
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
		--c_token;
		continue;
	    }
	    case S_PALETTE_FUNCTIONS: { /* "func$tions" */
		CHECK_TRANSFORM;
		set_palette_function();
		sm_palette.colorMode = SMPAL_COLOR_MODE_FUNCTIONS;
                sm_palette.gradient_type = SMPAL_GRADIENT_TYPE_SMOOTH;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_FUNCTIONS;
		--c_token;
		continue;
	    }
	    case S_PALETTE_MODEL: { /* "mo$del" */
		int model;
		++c_token;
		if (END_OF_COMMAND)
		    int_error( c_token, "expected color model" );
		model = lookup_table(&color_model_tbl[0],c_token);
		if (model == -1)
		    int_error(c_token,"unknown color model");
		sm_palette.cmodel = model;
		sm_palette.HSV_offset = 0.0;
		if (model == C_MODEL_HSV && equals(c_token+1,"start")) {
		    c_token += 2;
		    sm_palette.HSV_offset = real_expression();
		    sm_palette.HSV_offset = clip_to_01(sm_palette.HSV_offset);
		    c_token--;
		}
		continue;
	    }
	    /* ps_allcF: write all rgb formulae into PS file? */
	    case S_PALETTE_NOPS_ALLCF: /* "nops_allcF" */
		sm_palette.ps_allcF = FALSE;
		continue;
	    case S_PALETTE_PS_ALLCF: /* "ps_allcF" */
		sm_palette.ps_allcF = TRUE;
		continue;
	    /* max colors used */
	    case S_PALETTE_MAXCOLORS: { /* "maxc$olors" */
		int i;

		c_token++;
		i = int_expression();
		if (i<0 || i==1)
		    int_warn(c_token,"maxcolors must be > 1");
		else
		    sm_palette.use_maxcolors = i;
		--c_token;
		continue;
	    }

	    } /* switch over palette lookup table */
	    int_error(c_token,"invalid palette option");
	} /* end of while !end of command over palette options */
    } /* else(arguments found) */

    if (named_color && sm_palette.cmodel != C_MODEL_RGB && interactive)
	int_warn(NO_CARET,
		 "Named colors will produce strange results if not in color mode RGB." );

    /* Invalidate previous palette */
    invalidate_palette();
}

#undef CHECK_TRANSFORM

/* default settings for palette */
void
reset_palette()
{
    if (!enable_reset_palette) return;
    free(sm_palette.gradient);
    free(sm_palette.color);
    free_at(sm_palette.Afunc.at);
    free_at(sm_palette.Bfunc.at);
    free_at(sm_palette.Cfunc.at);
    init_color();
    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;
}

