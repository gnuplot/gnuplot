#ifndef lint
static char *RCSid() { return RCSid("$Id: color.c,v 1.42 2003/11/18 09:36:04 mikulik Exp $"); }
#endif

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
 * This file is used only if PM3D is defined.
 *
]*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef PM3D

#include "color.h"
#include "getcolor.h"

#include "axis.h"
#include "gadgets.h"
#include "graphics.h"
#include "plot.h"
#include "graph3d.h"
#include "pm3d.h"
#include "graphics.h"
#include "term_api.h"
#include "util3d.h"
#include "alloc.h"

/* COLOUR MODES - GLOBAL VARIABLES */
t_sm_palette sm_palette;  /* initialized in init_color() */

/* SMOOTH COLOUR BOX - GLOBAL VARIABLES */
color_box_struct color_box; /* initialized in init_color() */

#ifdef EXTENDED_COLOR_SPECS
int supply_extended_color_specs = 0;
#endif

/* Corners of the colour box. */
static unsigned int cb_x_from, cb_x_to, cb_y_from, cb_y_to;

/* Internal prototype declarations: */

static void draw_inside_color_smooth_box_postscript __PROTO((FILE * out));
static void draw_inside_color_smooth_box_bitmap __PROTO((FILE * out));
void cbtick_callback __PROTO((AXIS_INDEX axis, double place, char *text, struct lp_style_type grid));



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
  sm_palette.ps_allcF = 0;
  sm_palette.gradient_num = 0;
  sm_palette.gradient = NULL;
  sm_palette.cmodel = C_MODEL_RGB;
  sm_palette.Afunc.at = sm_palette.Bfunc.at = sm_palette.Cfunc.at = NULL;
  sm_palette.gamma = 1.5;

  /* initialisation of smooth color box */
  color_box.where = SMCOLOR_BOX_DEFAULT;
  color_box.rotation = 'v';
  color_box.border = 1;
  color_box.border_lt_tag = -1;
  color_box.xorigin = 0.9;
  color_box.yorigin = 0.2;
  color_box.xsize = 0.1;
  color_box.ysize = 0.63;
}


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
     * a message after allocating new colors */
    static t_sm_palette save_pal = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	(rgb_color *) 0, -1
    };

#if 0
    GIF_show_current_palette();
#endif

    if (!term->make_palette) {
	fprintf(stderr, "Error: terminal \"%s\" does not support continuous colors.\n",term->name);
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
    if (sm_palette.use_maxcolors > 0 && i > sm_palette.use_maxcolors)
	sm_palette.colors = sm_palette.use_maxcolors;

    if (save_pal.colorFormulae < 0
	|| sm_palette.colorFormulae != save_pal.colorFormulae
	|| sm_palette.colorMode != save_pal.colorMode
	|| sm_palette.formulaR != save_pal.formulaR
	|| sm_palette.formulaG != save_pal.formulaG
	|| sm_palette.formulaB != save_pal.formulaB
	|| sm_palette.positive != save_pal.positive 
	|| sm_palette.colors != save_pal.colors) {
	/* print the message only if colors have changed */
	if (interactive)
	fprintf(stderr, "smooth palette in %s: available %i color positions; using %i of them\n", term->name, i, sm_palette.colors);
    }

    save_pal = sm_palette;

    if (sm_palette.color != NULL) {
	free(sm_palette.color);
	sm_palette.color = NULL;
    }
    sm_palette.color = gp_alloc( sm_palette.colors * sizeof(rgb_color), 
				 "pm3d palette color");

    /*  fill sm_palette.color[]  */
    for (i = 0; i < sm_palette.colors; i++) {
	gray = (double) i / (sm_palette.colors - 1);	/* rescale to [0;1] */
	rgb1_from_gray( gray, &(sm_palette.color[i]) );
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
void
set_color(double gray)
{
    if (!(term->set_color))
	return;
    if (sm_palette.positive == SMPAL_NEGATIVE)
	gray = 1 - gray;
    term->set_color(gray);
}


#if 0
/*
   Makes mapping from real 3D coordinates to 2D terminal coordinates,
   then draws filled polygon
 */
static void
filled_polygon(int points, gpdPoint * corners)
{
    int i;
    gpiPoint *icorners;
    icorners = gp_alloc(points * sizeof(gpiPoint), "filled_polygon corners");
    for (i = 0; i < points; i++)
	map3d_xy(corners[i].x, corners[i].y, corners[i].z, &icorners[i].x, &icorners[i].y);
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
	icorners[0].spec.gray = -1;	/* force solid color */
    }
#endif
    term->filled_polygon(points, icorners);
    free(icorners);
}
#endif


/* The routine above for 4 points explicitly.
 * This is the only routine which supportes extended
 * color specs currently.
 */
#ifdef EXTENDED_COLOR_SPECS
void
filled_quadrangle(gpdPoint * corners, gpiPoint * icorners)
#else
void
filled_quadrangle(gpdPoint * corners)
#endif
{
    int i;
#ifndef EXTENDED_COLOR_SPECS
    gpiPoint icorners[4];
#endif
    for (i = 0; i < 4; i++) {
	map3d_xy(corners[i].x, corners[i].y, corners[i].z, &icorners[i].x, &icorners[i].y);
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
void
filled_polygon_3dcoords(int points, struct coordinate GPHUGE * coords)
{
    int i;
    gpiPoint *icorners;
    icorners = gp_alloc(points * sizeof(gpiPoint), "filled_polygon3d corners");
    for (i = 0; i < points; i++)
	map3d_xy(coords[i].x, coords[i].y, coords[i].z, &icorners[i].x, &icorners[i].y);
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
	icorners[0].spec.gray = -1;	/* force solid color */
    }
#endif
    term->filled_polygon(points, icorners);
    free(icorners);
}


/*
   Makes mapping from real 3D coordinates, passed as coords array, but at z coordinate
   fixed (base_z, for instance) to 2D terminal coordinates, then draws filled polygon
 */
void
filled_polygon_3dcoords_zfixed(int points, struct coordinate GPHUGE * coords, double z)
{
    int i;
    gpiPoint *icorners;
    icorners = gp_alloc(points * sizeof(gpiPoint), "filled_polygon_zfix corners");
    for (i = 0; i < points; i++)
	map3d_xy(coords[i].x, coords[i].y, z, &icorners[i].x, &icorners[i].y);
#ifdef EXTENDED_COLOR_SPECS
    if (supply_extended_color_specs) {
	icorners[0].spec.gray = -1;	/* force solid color */
    }
#endif
    term->filled_polygon(points, icorners);
    free(icorners);
}


/*
   Draw colour smooth box

   Firstly two helper routines for plotting inside of the box
   for postscript and for other terminals, finally the main routine
 */


/* plot the colour smooth box for from terminal's integer coordinates
   [cb_x_from,cb_y_from] to [cb_x_to,cb_y_to].
   This routine is for postscript files --- actually, it writes a small
   PS routine.
 */
static void
draw_inside_color_smooth_box_postscript(out)
FILE * out;
{
    int scale_x = (cb_x_to - cb_x_from), scale_y = (cb_y_to - cb_y_from);
    fputs("stroke gsave\t%% draw gray scale smooth box\n"
	  "maxcolors 0 gt {/imax maxcolors def} {/imax 1024 def} ifelse\n", out);

    /* nb. of discrete steps (counted in the loop) */
    fprintf(out, "%i %i translate %i %i scale 0 setlinewidth\n", cb_x_from, cb_y_from, scale_x, scale_y);
    /* define left bottom corner and scale of the box so that all coordinates
       of the box are from [0,0] up to [1,1]. Further, this normalization
       makes it possible to pass y from [0,1] as parameter to setgray */
    fprintf(out, "/ystep 1 imax div def /y0 0 def /ii 0 def\n");
    /* local variables; y-step, current y position and counter ii;  */
    if (sm_palette.positive == SMPAL_NEGATIVE)	/* inverted gray for negative figure */
	fputs("{ 1 y0 sub g ", out);
    else
	fputs("{ y0 g ", out);
    if (color_box.rotation == 'v')
	fputs("0 y0 N 1 0 V 0 ystep V -1 0 f\n", out);
    else
	fputs("y0 0 N 0 1 V ystep 0 V 0 -1 f\n", out);
    fputs("/y0 y0 ystep add def /ii ii 1 add def\n"
	  "ii imax ge {exit} if } loop\n"
	  "grestore 0 setgray\n", out);
}	



/* plot the colour smooth box for from terminal's integer coordinates
   [x_from,y_from] to [x_to,y_to].
   This routine is for non-postscript files, as it does explicitly the loop
   over all thin rectangles
 */
static void
draw_inside_color_smooth_box_bitmap(out)
FILE * out;
{
    int steps = 128; /* I think that nobody can distinguish more colours drawn in the palette */
    int i, xy, xy2, xy_from, xy_to;
    double xy_step, gray;
    gpiPoint corners[4];
    
    (void) out;			/* to avoid "unused parameter" warning */
    if (color_box.rotation == 'v') {
	corners[0].x = corners[3].x = cb_x_from;
	corners[1].x = corners[2].x = cb_x_to;
	xy_from = cb_y_from;
	xy_to = cb_y_to;
    } else {
	corners[0].y = corners[1].y = cb_y_from;
	corners[2].y = corners[3].y = cb_y_to;
	xy_from = cb_x_from;
	xy_to = cb_x_to;
    }
    xy_step = (color_box.rotation == 'h' ? cb_x_to - cb_x_from : cb_y_to - cb_y_from) / (double) steps;

    for (i = 0; i < steps; i++) {
	gray = (double) i / steps;	/* colours equidistantly from [0,1] */
	/* Set the colour (also for terminals which support extended specs). */
	set_color(gray);
	xy = xy_from + (int) (xy_step * i);
	xy2 = xy_from + (int) (xy_step * (i + 1));
	if (color_box.rotation == 'v') {
	    corners[0].y = corners[1].y = xy;
	    corners[2].y = corners[3].y = (i == steps - 1) ? xy_to : xy2;
	} else {
	    corners[0].x = corners[3].x = xy;
	    corners[1].x = corners[2].x = (i == steps - 1) ? xy_to : xy2;
	}
#ifdef EXTENDED_COLOR_SPECS
	if (supply_extended_color_specs) {
	    corners[0].spec.gray = -1;	/* force solid color */
	}
#endif
	/* print the rectangle with the given colour */
	term->filled_polygon(4, corners);
    }
}

/* Notice HBB 20010720: would be static, but HP-UX gcc bug forbids
 * this, for now */
void
cbtick_callback(axis, place, text, grid)
    AXIS_INDEX axis;
    double place;
    char *text;
    struct lp_style_type grid; /* linetype or -2 for no grid */
{
    int len = (text ? ticscale : miniticscale)
	* (tic_in ? -1 : 1) * (term->h_tic);
    double cb_place = (place - CB_AXIS.min) / (CB_AXIS.max - CB_AXIS.min);
	/* relative z position along the colorbox axis */
    unsigned int x1, y1, x2, y2;

#if 0
    printf("cbtick_callback:  place=%g\ttext=\"%s\"\tgrid.l_type=%i\n",place,text,grid.l_type);
#endif
    (void) axis;		/* to avoid 'unused' warning */

    /* calculate tic position */
    if (color_box.rotation == 'h') {
	x1 = x2 = cb_x_from + cb_place * (cb_x_to - cb_x_from);
	y1 = cb_y_from;
	y2 = cb_y_from - len;
    } else {
	x1 = cb_x_to;
	x2 = cb_x_to + len;
	y1 = y2 = cb_y_from + cb_place * (cb_y_to - cb_y_from);
    }

    /* draw grid line */
    if (grid.l_type > L_TYPE_NODRAW) {
	term_apply_lp_properties(&grid);	/* grid linetype */
	if (color_box.rotation == 'h') {
	    (*term->move) (x1, cb_y_from);
	    (*term->vector) (x1, cb_y_to);
	} else {
	    (*term->move) (cb_x_from, y1);
	    (*term->vector) (cb_x_to, y1);
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
    }

    /* draw tic */
    (*term->move) (x1, y1);
    (*term->vector) (x2, y2);

    /* draw label */
    if (text) {
	/* User-specified different color for the tics text */
	if (axis_array[axis].ticdef.textcolor.lt != TC_DEFAULT)
	    apply_textcolor(&(axis_array[axis].ticdef.textcolor), term);
	if (color_box.rotation == 'h') {
	    int y3 = cb_y_from - (term->v_char);
	    if (len > 0) y3 -= len; /* add outer tics len */
	    if (y3<0) y3 = 0;
#if 1
	    if (term->justify_text)
		term->justify_text(CENTRE);
	    (*term->put_text)(x2, y3, text);
#else /* clipping does not work properly for text around 3d graph */
	    clip_put_text_just(x2, y3, text, CENTRE, JUST_TOP, 
			       axis_array[axis].ticdef.font);
#endif
	} else {
	    unsigned int x3 = cb_x_to + (term->h_char);
	    if (len > 0) x3 += len; /* add outer tics len */
#if 1
	    if (term->justify_text)
		term->justify_text(LEFT);
	    (*term->put_text)(x3, y2, text);
#else /* clipping does not work properly for text around 3d graph */
	    clip_put_text_just(x3, y2, text, LEFT, JUST_CENTRE, 
			       axis_array[axis].ticdef.font);
#endif
	}
	term_apply_lp_properties(&border_lp);	/* border linetype */
    }

    /* draw tic on the mirror side */
    if (CB_AXIS.ticmode & TICS_MIRROR) {
	if (color_box.rotation == 'h') {
	    y1 = cb_y_to;
	    y2 = cb_y_to + len;
	} else {
	    x1 = cb_x_from;
	    x2 = cb_x_from - len;
	}
	(*term->move) (x1, y1);
	(*term->vector) (x2, y2);
    }
}

/*
   Finally the main colour smooth box drawing routine
 */
void
draw_color_smooth_box()
{
    double tmp;
    FILE *out = postscript_gpoutfile;	/* either gpoutfile or PSLATEX_auxfile */

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
	cb_x_from = color_box.xorigin * (term->xmax) + 0.5;
	cb_y_from = color_box.yorigin * (term->ymax) + 0.5;
	cb_x_to = color_box.xsize * (term->xmax) + 0.5;
	cb_y_to = color_box.ysize * (term->ymax) + 0.5;
	cb_x_to += cb_x_from;
	cb_y_to += cb_y_from;
    } else {			/* color_box.where == SMCOLOR_BOX_DEFAULT */
#if 0 
	/* HBB 20031215: replaced this by view-independant placment method */
	double dx = (X_AXIS.max - X_AXIS.min);
	/* don't use CB_AXIS here, CB_AXIS might be completely
	 * unrelated to the Z axis 26 Jan 2002 (joze) */
	double dz = Z_AXIS.max - Z_AXIS.min;

	/* note: [0.05 0.35; 0.20 0.00] were the values before cbaxis */
	map3d_xy(X_AXIS.max + dx * 0.03, Y_AXIS.max, base_z + dz * 0.35,
		 &cb_x_from, &cb_y_from);
	map3d_xy(X_AXIS.max + dx * 0.11, Y_AXIS.max, ceiling_z - dz * 0,
		 &cb_x_to, &cb_y_to);
	if (cb_y_from == cb_y_to || cb_x_from == cb_x_to) { /* map, i.e. plot with "set view 0,0 or 180,0" */
	    dz = Y_AXIS.max - Y_AXIS.min;
	    /* note: [0.04 0.25; 0.18 0.25] were the values before cbaxis */
	    map3d_xy(X_AXIS.max + dx * 0.025, Y_AXIS.min, base_z, & cb_x_from, &cb_y_from);
	    map3d_xy(X_AXIS.max + dx * 0.075, Y_AXIS.max, ceiling_z, &cb_x_to, &cb_y_to);
	}
#else
	/* HBB 20031215: new code.  Constants fixed to what the result
	 * of the old code in default view (set view 60,30,1,1)
	 * happened to be. Somebody fix them if they're not right!*/
	cb_x_from = xmiddle + 0.709 * xscaler;
	cb_x_to   = xmiddle + 0.778 * xscaler;
	cb_y_from = ymiddle - 0.147 * yscaler;
	cb_y_to   = ymiddle + 0.497 * yscaler;
#endif
	/* now corrections for outer tics */
	if (color_box.rotation == 'v') {
	    int len = (tic_in ? -1 : 1) * ticscale * (term->h_tic); /* positive for outer tics */
	    if (len > 0) {
		if (CB_AXIS.ticmode & TICS_MIRROR) {
		    cb_x_from += len;
		    cb_x_to += len;
		}
		if (axis_array[FIRST_Y_AXIS].ticmode & TICS_MIRROR) {
		    cb_x_from += len;
		    cb_x_to += len;
		}
	    }
	}
    }

    if (cb_y_from > cb_y_to) { /* switch them */
	tmp = cb_y_to;
	cb_y_to = cb_y_from;
	cb_y_from = tmp;
    }

    /* Optimized version of the smooth colour box in postscript. Advantage:
       only few lines of code is written into the output file.
     */
    if (postscript_gpoutfile)
	draw_inside_color_smooth_box_postscript(out);
    else
	draw_inside_color_smooth_box_bitmap(out);

    if (color_box.border) {
	/* now make boundary around the colour box */
	if (color_box.border_lt_tag >= 0) {
	    /* user specified line type */
	    struct lp_style_type lp;
	    lp_use_properties(&lp, color_box.border_lt_tag, 1);
	    term_apply_lp_properties(&lp);
	} else {
	    /* black solid colour should be chosen, so it's border linetype */
	    term_apply_lp_properties(&border_lp);
	}
	(term->move) (cb_x_from, cb_y_from);
	(term->vector) (cb_x_to, cb_y_from);
	(term->vector) (cb_x_to, cb_y_to);
	(term->vector) (cb_x_from, cb_y_to);
	(term->vector) (cb_x_from, cb_y_from);

	/* Set line properties to some value, this also draws lines in postscript terminals. */
	    term_apply_lp_properties(&border_lp);
	}

    /* draw tics */
    if (axis_array[COLOR_AXIS].ticmode) {
	term_apply_lp_properties(&border_lp); /* border linetype */
	setup_tics(COLOR_AXIS, 20);
	gen_tics(COLOR_AXIS, cbtick_callback );
    }

    /* write the colour box label */
    if (*CB_AXIS.label.text) {
	int x, y;
	apply_textcolor(&(CB_AXIS.label.textcolor),term);
	if (color_box.rotation == 'h') {
	    int len = ticscale * (tic_in ? 1 : -1) * (term->v_tic);
	    x = (cb_x_from + cb_x_to) / 2 + CB_AXIS.label.xoffset * term->h_char;
#define DEFAULT_Y_DISTANCE 1.0
	    y = cb_y_from + (CB_AXIS.label.yoffset - DEFAULT_Y_DISTANCE - 1.7) * term->v_char;
#undef DEFAULT_Y_DISTANCE
	    if (len < 0) y += len;
	    if (x<0) x = 0;
	    if (y<0) y = 0;
	    write_multiline(x, y, CB_AXIS.label.text, CENTRE, JUST_CENTRE, 0,
			    CB_AXIS.label.font);
	} else {
	    int len = ticscale * (tic_in ? -1 : 1) * (term->h_tic);
	    /* calculate max length of cb-tics labels */
	    widest_tic_strlen = 0;
	    if (CB_AXIS.ticmode & TICS_ON_BORDER) {
	      	widest_tic_strlen = 0; /* reset the global variable */
		gen_tics(COLOR_AXIS, /* 0, */ widest_tic_callback);
	    }
#define DEFAULT_X_DISTANCE 1.0
	    x = cb_x_to + (CB_AXIS.label.xoffset + widest_tic_strlen + DEFAULT_X_DISTANCE + 1.5) * term->h_char;
#undef DEFAULT_X_DISTANCE
	    if (len > 0) x += len;
	    y = (cb_y_from + cb_y_to) / 2 + CB_AXIS.label.yoffset * term->v_char;
	    if (x<0) x = 0;
	    if (y<0) y = 0;
	    if ((*term->text_angle)(TEXT_VERTICAL)) {
		write_multiline(x, y, CB_AXIS.label.text, CENTRE, JUST_TOP, 
				TEXT_VERTICAL, CB_AXIS.label.font);
		(*term->text_angle)(0);
	    } else {
		write_multiline(x, y, CB_AXIS.label.text, LEFT, JUST_TOP, 0, CB_AXIS.label.font);
	    }
	}
	reset_textcolor(&(CB_AXIS.label.textcolor),term);
    }

}




#endif /* PM3D */
