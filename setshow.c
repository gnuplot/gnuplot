static char *RCSid = "$Id: setshow.c%v 3.50.1.11 1993/08/10 03:55:03 woo Exp $";


/* GNUPLOT - setshow.c */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 *
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 * 
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

#include <stdio.h>
#include <math.h>
#include "plot.h"
#include "setshow.h"

#define DEF_FORMAT   "%g"	/* default format for tic mark labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */

/*
 * global variables to hold status of 'set' options
 *
 */
TBOOLEAN		autoscale_r	= TRUE;
TBOOLEAN		autoscale_t	= TRUE;
TBOOLEAN		autoscale_u	= TRUE;
TBOOLEAN		autoscale_v	= TRUE;
TBOOLEAN		autoscale_x	= TRUE;
TBOOLEAN		autoscale_y	= TRUE;
TBOOLEAN		autoscale_z	= TRUE;
TBOOLEAN		autoscale_lt	= TRUE;
TBOOLEAN		autoscale_lu	= TRUE;
TBOOLEAN		autoscale_lv	= TRUE;
TBOOLEAN		autoscale_lx	= TRUE;
TBOOLEAN		autoscale_ly	= TRUE;
TBOOLEAN		autoscale_lz	= TRUE;
double			boxwidth	= -1.0; /* box width (automatic) */
TBOOLEAN 		clip_points	= FALSE;
TBOOLEAN 		clip_lines1	= TRUE;
TBOOLEAN 		clip_lines2	= FALSE;
TBOOLEAN		draw_border	= TRUE;
TBOOLEAN		draw_surface    = TRUE;
TBOOLEAN		timedate	= FALSE;
char			dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };
char			xformat[MAX_ID_LEN+1] = DEF_FORMAT;
char			yformat[MAX_ID_LEN+1] = DEF_FORMAT;
char			zformat[MAX_ID_LEN+1] = DEF_FORMAT;
enum PLOT_STYLE		data_style	= POINTSTYLE;
enum PLOT_STYLE		func_style	= LINES;
TBOOLEAN		grid		= FALSE;
int			key		= -1;	/* default position */
double			key_x;
double			key_y;	/* user specified position for key */
double			key_z;
TBOOLEAN		is_log_x	= FALSE;
TBOOLEAN		is_log_y	= FALSE;
TBOOLEAN		is_log_z	= FALSE;
double			base_log_x	= 0.0;
double			base_log_y	= 0.0;
double			base_log_z	= 0.0;
double			log_base_log_x	= 0.0;
double			log_base_log_y	= 0.0;
double			log_base_log_z	= 0.0;
FILE*			outfile;
char			outstr[MAX_ID_LEN+1] = "STDOUT";
TBOOLEAN		parametric	= FALSE;
TBOOLEAN		polar		= FALSE;
TBOOLEAN		hidden3d	= FALSE;
TBOOLEAN		label_contours	= TRUE; /* different linestyles are used for contours when set */
int			angles_format	= ANGLES_RADIANS;
int			mapping3d	= MAP3D_CARTESIAN;
int			samples		= SAMPLES; /* samples is always equal to samples_1 */
int			samples_1	= SAMPLES;
int			samples_2	= SAMPLES;
int			iso_samples_1	= ISO_SAMPLES;
int			iso_samples_2	= ISO_SAMPLES;
float			xsize		= 1.0;  /* scale factor for size */
float			ysize		= 1.0;  /* scale factor for size */
float			zsize		= 1.0;  /* scale factor for size */
float			surface_rot_z   = 30.0; /* Default 3d transform. */
float			surface_rot_x   = 60.0;
float			surface_scale   = 1.0;
float			surface_zscale  = 1.0;
int			term		= 0;		/* unknown term is 0 */
char			term_options[MAX_LINE_LEN+1] = "";
char			title[MAX_LINE_LEN+1] = "";
char			xlabel[MAX_LINE_LEN+1] = "";
char			ylabel[MAX_LINE_LEN+1] = "";
char			zlabel[MAX_LINE_LEN+1] = "";
int			time_xoffset	= 0;
int			time_yoffset	= 0;
int			title_xoffset	= 0;
int			title_yoffset	= 0;
int			xlabel_xoffset	= 0;
int			xlabel_yoffset	= 0;
int			ylabel_xoffset	= 0;
int			ylabel_yoffset	= 0;
int			zlabel_xoffset	= 0;
int			zlabel_yoffset	= 0;
double			rmin		= -0.0;
double			rmax		=  10.0;
double			tmin		= -5.0;
double			tmax		=  5.0;
double			umin		= -5.0;
double			umax		= 5.0;
double			vmin		= -5.0;
double			vmax		= 5.0;
double			xmin		= -10.0;
double			xmax		= 10.0;
double			ymin		= -10.0;
double			ymax		= 10.0;
double			zmin		= -10.0;
double			zmax		= 10.0;
double			loff		= 0.0;
double			roff		= 0.0;
double			toff		= 0.0;
double			boff		= 0.0;
int			draw_contour	= CONTOUR_NONE;
int			contour_pts	= 5;
int			contour_kind	= CONTOUR_KIND_LINEAR;
int			contour_order	= 4;
int			contour_levels	= 5;
double			zero		= ZERO;	/* zero threshold, not 0! */
int			levels_kind	= LEVELS_AUTO;
double			levels_list[MAX_DISCRETE_LEVELS];  /* storage for z levels to draw contours at */

int			dgrid3d_row_fineness = 10;
int			dgrid3d_col_fineness = 10;
int			dgrid3d_norm_value = 1;
TBOOLEAN		dgrid3d		= FALSE;

TBOOLEAN 		xzeroaxis	= TRUE;
TBOOLEAN 		yzeroaxis	= TRUE;

TBOOLEAN 		xtics		= TRUE;
TBOOLEAN 		ytics		= TRUE;
TBOOLEAN 		ztics		= TRUE;

float 			ticslevel	= 0.5;

struct ticdef		xticdef		= {TIC_COMPUTED};
struct ticdef		yticdef		= {TIC_COMPUTED};
struct ticdef		zticdef		= {TIC_COMPUTED};

TBOOLEAN		tic_in		= TRUE;

struct text_label 	*first_label	= NULL;
struct arrow_def 	*first_arrow	= NULL;

/*** other things we need *****/
#ifdef _Windows
#include <string.h>
#else
#if !defined(ATARI) && !defined (AMIGA_SC_6_1)
extern char *strcpy(),*strcat();
extern int      strlen();
#endif
#endif

#if defined(unix) || defined(PIPES)
extern FILE *popen();
#endif

/* input data, parsing variables */
extern struct lexical_unit token[];
extern char input_line[];
extern int num_tokens, c_token;
extern TBOOLEAN interactive;	/* from plot.c */

extern char replot_line[];
extern struct udvt_entry *first_udv;
extern TBOOLEAN is_3d_plot;

extern double magnitude(),real();
extern struct value *const_express();

#ifdef _Windows
extern FILE * open_printer();
extern void close_printer();
#endif

/******** Local functions ********/
static void set_xyzlabel();
static void set_label();
static void set_nolabel();
static void set_arrow();
static void set_noarrow();
static void load_tics();
static void load_tic_user();
static void free_marklist();
static void load_tic_series();
static void load_offsets();

static void show_style(), show_range(), show_zero(), show_border(), show_dgrid3d();
static void show_offsets(), show_output(), show_samples(), show_isosamples();
static void show_view(), show_size(), show_title(), show_xlabel();
static void show_angles(), show_boxwidth();
static void show_ylabel(), show_zlabel(), show_xzeroaxis(), show_yzeroaxis();
static void show_label(), show_arrow(), show_grid(), show_key();
static void show_polar(), show_parametric(), show_tics(), show_ticdef();
static void show_time(), show_term(), show_plot(), show_autoscale(), show_clip();
static void show_contour(), show_mapping(), show_format(), show_logscale();
static void show_variables(), show_surface(), show_hidden3d(), show_label_contours();
static void delete_label();
static int assign_label_tag();
static void delete_arrow();
static int assign_arrow_tag();
static TBOOLEAN set_one(), set_two(), set_three();
static TBOOLEAN show_one(), show_two();

/******** The 'set' command ********/
void
set_command()
{
static char GPFAR setmess[] =
	"valid set options:  'angles' '{no}arrow', {no}autoscale', \n\
	'{no}border', 'boxwidth', '{no}clabel', '{no}clip', 'cntrparam', \n\
        '{no}contour', 'data style', '{no}dgrid3d', 'dummy', 'format', \n\
        'function style', '{no}grid', '{no}hidden3d', 'isosamples', '{no}key', \n\
	'{no}label', '{no}logscale', 'mapping', 'offsets', 'output', \n\
	'{no}parametric', '{no}polar', 'rrange', 'samples', 'size', \n\
	'{no}surface', 'terminal', 'tics', 'ticslevel', '{no}time', 'title', \n\
	'trange', 'urange', 'view', 'vrange', 'xlabel', 'xrange', '{no}xtics', \n\
	'xmtics', 'xdtics', '{no}xzeroaxis', 'ylabel', 'yrange', '{no}ytics', \n\
	'ymtics', 'ydtics', '{no}yzeroaxis', 'zero', '{no}zeroaxis', 'zlabel', \n\
	'zrange', '{no}ztics', 'zmtics', 'zdtics'";

    c_token++;

    if (!set_one() && !set_two() && !set_three())
	int_error(setmess, c_token);
}

/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
set_one()
{
	if (almost_equals(c_token,"ar$row")) {
		c_token++;
		set_arrow();
	}
	else if (almost_equals(c_token,"noar$row")) {
		c_token++;
		set_noarrow();
	}
     else if (almost_equals(c_token,"au$toscale")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		   autoscale_r=autoscale_t = autoscale_x = autoscale_y = autoscale_z = TRUE;
	    } else if (equals(c_token, "xy") || equals(c_token, "yx")) {
		   autoscale_x = autoscale_y = TRUE;
		   c_token++;
	    } else if (equals(c_token, "r")) {
		   autoscale_r = TRUE;
		   c_token++;
	    } else if (equals(c_token, "t")) {
		   autoscale_t = TRUE;
		   c_token++;
	    } else if (equals(c_token, "x")) {
		   autoscale_x = TRUE;
		   c_token++;
	    } else if (equals(c_token, "y")) {
		   autoscale_y = TRUE;
		   c_token++;
	    } else if (equals(c_token, "z")) {
		   autoscale_z = TRUE;
		   c_token++;
	    }
	} 
	else if (almost_equals(c_token,"noau$toscale")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		   autoscale_r=autoscale_t = autoscale_x = autoscale_y = autoscale_z = FALSE;
	    } else if (equals(c_token, "xy") || equals(c_token, "tyx")) {
		   autoscale_x = autoscale_y = FALSE;
		   c_token++;
	    } else if (equals(c_token, "r")) {
		   autoscale_r = FALSE;
		   c_token++;
	    } else if (equals(c_token, "t")) {
		   autoscale_t = FALSE;
		   c_token++;
	    } else if (equals(c_token, "x")) {
		   autoscale_x = FALSE;
		   c_token++;
	    } else if (equals(c_token, "y")) {
		   autoscale_y = FALSE;
		   c_token++;
	    } else if (equals(c_token, "z")) {
		   autoscale_z = FALSE;
		   c_token++;
	    }
	} 
	else if (almost_equals(c_token,"bor$der")) {
	    draw_border = TRUE;
	    c_token++;
	}
	else if (almost_equals(c_token,"nobor$der")) {
	    draw_border = FALSE;
	    c_token++;
	}
	else if (almost_equals(c_token,"box$width")) {
		struct value a;
		c_token++;
	    if (END_OF_COMMAND)
	    	boxwidth = -1.0;
	    else
			boxwidth = magnitude(const_express(&a));
	}
	else if (almost_equals(c_token,"c$lip")) {
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
	}
	else if (almost_equals(c_token,"noc$lip")) {
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
	}
	else if (almost_equals(c_token,"hi$dden3d")) {
#ifdef LITE
		printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
	    hidden3d = TRUE;
#endif /* LITE */
	    c_token++;
	}
	else if (almost_equals(c_token,"nohi$dden3d")) {
#ifdef LITE
		printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
	    hidden3d = FALSE;
#endif /* LITE */
	    c_token++;
	}
 	else if (almost_equals(c_token,"cla$bel")) {
 		label_contours = TRUE;
 		c_token++;
 	}
 	else if (almost_equals(c_token,"nocla$bel")) {
 		label_contours = FALSE;
 		c_token++;
  	}
	else if (almost_equals(c_token,"ma$pping3d")) {
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
	}
	else if (almost_equals(c_token,"co$ntour")) {
	    c_token++;
	    if (END_OF_COMMAND)
		 /* assuming same as points */
		 draw_contour = CONTOUR_BASE;
	    else if (almost_equals(c_token, "ba$se"))
		 draw_contour = CONTOUR_BASE;
	    else if (almost_equals(c_token, "s$urface"))
		 draw_contour = CONTOUR_SRF;
	    else if (almost_equals(c_token, "bo$th"))
		 draw_contour = CONTOUR_BOTH;
	    else
		 int_error("expecting 'base', 'surface', or 'both'", c_token);
	    c_token++;
	}
	else if (almost_equals(c_token,"noco$ntour")) {
	    c_token++;
	    draw_contour = CONTOUR_NONE;
	}
	else if (almost_equals(c_token,"cntrp$aram")) {
	    struct value a;

	    c_token++;
	    if (END_OF_COMMAND) {
		 /* assuming same as defaults */
		 contour_pts = 5;
		 contour_kind = CONTOUR_KIND_LINEAR;
		 contour_order = 4;
		 contour_levels = 5;
 		 levels_kind = LEVELS_AUTO;
	    }
	    else if (almost_equals(c_token, "p$oints")) {
		 c_token++;
		 contour_pts = (int) real(const_express(&a));
	    }
	    else if (almost_equals(c_token, "li$near")) {
		 c_token++;
		 contour_kind = CONTOUR_KIND_LINEAR;
	    }
	    else if (almost_equals(c_token, "c$ubicspline")) {
		 c_token++;
		 contour_kind = CONTOUR_KIND_CUBIC_SPL;
	    }
	    else if (almost_equals(c_token, "b$spline")) {
		 c_token++;
		 contour_kind = CONTOUR_KIND_BSPLINE;
	    }

   		else if (almost_equals(c_token, "le$vels")) {
   			int i=0;  /* local counter */
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
 			   while(!END_OF_COMMAND){
 			     if (!equals(c_token, ","))
 			       int_error("expecting comma to separate discrete levels", c_token);
 			     c_token++;
 			     levels_list[i++] =  real(const_express(&a));
 			   }
   			   contour_levels = i;
   			}
   			else if (almost_equals(c_token, "in$cremental")) {
   			   levels_kind = LEVELS_INCREMENTAL;
   			   c_token++;
 			   levels_list[i++] =  real(const_express(&a));
 			   if (!equals(c_token, ","))
 			     int_error("expecting comma to separate start,incr levels", c_token);
 			   c_token++;
 			   if((levels_list[i++] = real(const_express(&a)))==0)
 			     int_error("increment cannot be 0", c_token);
  			   if(!END_OF_COMMAND){
 			     if (!equals(c_token, ","))
 			       int_error("expecting comma to separate incr,stop levels", c_token);
 			     c_token++;
			     contour_levels = (real(const_express(&a))-levels_list[0])/levels_list[1];
 			     }
   			}
   			else if (almost_equals(c_token, "au$to")) {
   				levels_kind = LEVELS_AUTO;
 				c_token++;
 				if(!END_OF_COMMAND)
 					contour_levels = (int) real(const_express(&a));
 			}
 			else {
			  if(levels_kind == LEVELS_DISCRETE)
			    int_error("Levels type is discrete, ignoring new number of contour levels", c_token);
			    contour_levels = (int) real(const_express(&a));
			}
 		}
	    else if (almost_equals(c_token, "o$rder")) {
		 int order;
		 c_token++;
		 order = (int) real(const_express(&a));
		 if ( order < 2 || order > 10 )
		     int_error("bspline order must be in [2..10] range.", c_token);
		 contour_order = order;
	    }
	    else
		 int_error("expecting 'linear', 'cubicspline', 'bspline', 'points', 'levels' or 'order'", c_token);
	    c_token++;
	}
	else if (almost_equals(c_token,"da$ta")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		data_style = get_style();
	}
	else if (almost_equals(c_token,"dg$rid3d")) {
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
			}
			else {
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
	}
	else if (almost_equals(c_token,"nodg$rid3d")) {
		c_token++;
		dgrid3d = FALSE;
	}
	else if (almost_equals(c_token,"du$mmy")) {
		c_token++;
		if (END_OF_COMMAND)
			int_error("expecting dummy variable name", c_token);
		else {
			if (!equals(c_token,","))
				copy_str(dummy_var[0],c_token++);
			if (!END_OF_COMMAND && equals(c_token,",")) {
				c_token++;
				if (END_OF_COMMAND)
					int_error("expecting second dummy variable name", c_token);
				copy_str(dummy_var[1],c_token++);
		    	}
		}
	}
	else if (almost_equals(c_token,"fo$rmat")) {
		TBOOLEAN setx, sety, setz;
		c_token++;
		if (equals(c_token,"x")) {
			setx = TRUE; sety = setz = FALSE;
			c_token++;
		}
		else if (equals(c_token,"y")) {
			setx = setz = FALSE; sety = TRUE;
			c_token++;
		}
		else if (equals(c_token,"z")) {
			setx = sety = FALSE; setz = TRUE;
			c_token++;
		}
		else if (equals(c_token,"xy") || equals(c_token,"yx")) {
			setx = sety = TRUE; setz = FALSE;
			c_token++;
		}
		else if (isstring(c_token) || END_OF_COMMAND) {
			/* Assume he wants all */
			setx = sety = setz = TRUE;
		}
		if (END_OF_COMMAND) {
			if (setx)
				(void) strcpy(xformat,DEF_FORMAT);
			if (sety)
				(void) strcpy(yformat,DEF_FORMAT);
			if (setz)
				(void) strcpy(zformat,DEF_FORMAT);
		}
		else {
			if (!isstring(c_token))
			  int_error("expecting format string",c_token);
			else {
				if (setx)
				 quote_str(xformat,c_token);
				if (sety)
				 quote_str(yformat,c_token);
				if (setz)
				 quote_str(zformat,c_token);
				c_token++;
			}
		}
	}
	else if (almost_equals(c_token,"fu$nction")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		func_style = get_style();
	}
	else if (almost_equals(c_token,"la$bel")) {
		c_token++;
		set_label();
	}
	else if (almost_equals(c_token,"nola$bel")) {
		c_token++;
		set_nolabel();
	}
	else if (almost_equals(c_token,"lo$gscale")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		is_log_x = is_log_y = is_log_z = TRUE;
		base_log_x = base_log_y = base_log_z = 10.0;
		log_base_log_x = log_base_log_y = log_base_log_z = log(10.0);
	    } else {
		TBOOLEAN change_x = FALSE;
		TBOOLEAN change_y = FALSE;
		TBOOLEAN change_z = FALSE;
		if (chr_in_str(c_token, 'x'))
		    change_x = TRUE;
		if (chr_in_str(c_token, 'y'))
		    change_y = TRUE;
		if (chr_in_str(c_token, 'z'))
		    change_z = TRUE;
		c_token++;
                if (END_OF_COMMAND) {
		    if (change_x) {
			is_log_x = TRUE;
			base_log_x = 10.0;
			log_base_log_x = log(10.0);
		    }
		    if (change_y) {
			is_log_y = TRUE;
			base_log_y = 10.0;
			log_base_log_y = log(10.0);
		    }
		    if (change_z) {
			is_log_z = TRUE;
			base_log_z = 10.0;
			log_base_log_z = log(10.0);
		    }
		} else {
		    struct value a;
		    double newbase = magnitude(const_express(&a));
		    c_token++;
		    if (newbase < 1.1)
			int_error("log base must be >= 1.1; logscale unchanged",
				c_token);
		    else {
			if (change_x) {
			    is_log_x = TRUE;
			    base_log_x = newbase;
			    log_base_log_x = log(newbase);
			}
			if (change_y) {
			    is_log_y = TRUE;
			    base_log_y = newbase;
			    log_base_log_y = log(newbase);
			}
			if (change_z) {
			    is_log_z = TRUE;
			    base_log_z = newbase;
			    log_base_log_z = log(newbase);
			}
		    }
		}
	    }
	}
	else if (almost_equals(c_token,"nolo$gscale")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		is_log_x = is_log_y = is_log_z = FALSE;
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
	} 
	else if (almost_equals(c_token,"of$fsets")) {
		c_token++;
		if (END_OF_COMMAND) {
			loff = roff = toff = boff = 0.0;  /* Reset offsets */
		}
		else {
			load_offsets (&loff,&roff,&toff,&boff);
		}
	}
	else
		return(FALSE);	/* no command match */
	return(TRUE);
}


/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
set_two()
{
     char testfile[MAX_LINE_LEN+1];
#if defined(unix) || defined(PIPES)
     static TBOOLEAN pipe_open = FALSE;
#endif /* unix || PIPES */

	if (almost_equals(c_token,"o$utput")) {
		register FILE *f;

		c_token++;
		if (term && term_init)
			(*term_tbl[term].reset)();
		if (END_OF_COMMAND) {	/* no file specified */
 			UP_redirect (4);
			if (outfile != stdout) { /* Never close stdout */
#if defined(unix) || defined(PIPES)
				if ( pipe_open ) {
					(void) pclose(outfile);
					pipe_open = FALSE;
				} else
#endif /* unix || PIPES */
#ifdef _Windows
				  if ( !stricmp(outstr,"'PRN'") )
					close_printer();
				  else
#endif
					(void) fclose(outfile);
			}
			outfile = stdout; /* Don't dup... */
			term_init = FALSE;
			(void) strcpy(outstr,"STDOUT");
		} else if (!isstring(c_token))
			int_error("expecting filename",c_token);
		else {
			quote_str(testfile,c_token);
#if defined(unix) || defined(PIPES)
			if ( *testfile == '|' ) {
			  if ((f = popen(testfile+1,"w")) == (FILE *)NULL)
			    os_error("cannot create pipe; output not changed",c_token);
			  else
			    pipe_open = TRUE;
			} else
#endif /* unix || PIPES */
#ifdef _Windows
			if ( !stricmp(outstr,"'PRN'") ) {
				/* we can't call open_printer() while printer is open, so */
				close_printer();	/* close printer immediately if open */
			    outfile = stdout;	/* and reset output to stdout */
			    term_init = FALSE;
				(void) strcpy(outstr,"STDOUT");
			}
			if ( !stricmp(testfile,"PRN") ) {
			  if ((f = open_printer()) == (FILE *)NULL)
			    os_error("cannot open printer temporary file; output may have changed",c_token);
			} else
#endif
			  if ((f = fopen(testfile,"w")) == (FILE *)NULL)
			    os_error("cannot open file; output not changed",c_token);
			if (outfile != stdout) /* Never close stdout */
#if defined(unix) || defined(PIPES)
			    if( pipe_open ) {
				(void) pclose(outfile);
				pipe_open=FALSE;
			    } else
#endif /* unix || PIPES */
				(void) fclose(outfile);
			outfile = f;
			term_init = FALSE;
			outstr[0] = '\'';
			(void) strcat(strcpy(outstr+1,testfile),"'");
 			UP_redirect (1);
		}
		c_token++;
	}
	else if (almost_equals(c_token,"tit$le")) {
		set_xyzlabel(title,&title_xoffset,&title_yoffset);
	}
	else if (almost_equals(c_token,"xl$abel")) {
		set_xyzlabel(xlabel,&xlabel_xoffset,&xlabel_yoffset);
	}
	else if (almost_equals(c_token,"yl$abel")) {
		set_xyzlabel(ylabel,&ylabel_xoffset,&ylabel_yoffset);
	}
	else if (almost_equals(c_token,"zl$abel")) {
		set_xyzlabel(zlabel,&zlabel_xoffset,&zlabel_yoffset);
	}
	else if (almost_equals(c_token,"xzero$axis")) {
		c_token++;
		xzeroaxis = TRUE;
	} 
	else if (almost_equals(c_token,"yzero$axis")) {
		c_token++;
		yzeroaxis = TRUE;
	} 
	else if (almost_equals(c_token,"zeroa$xis")) {
		c_token++;
		yzeroaxis = TRUE;
		xzeroaxis = TRUE;
	} 
	else if (almost_equals(c_token,"noxzero$axis")) {
		c_token++;
		xzeroaxis = FALSE;
	} 
	else if (almost_equals(c_token,"noyzero$axis")) {
		c_token++;
		yzeroaxis = FALSE;
	} 
	else if (almost_equals(c_token,"nozero$axis")) {
		c_token++;
		xzeroaxis = FALSE;
		yzeroaxis = FALSE;
	} 
	else if (almost_equals(c_token,"par$ametric")) {
		if (!parametric) {
		   parametric = TRUE;
		   strcpy (dummy_var[0], "t");
		   strcpy (dummy_var[1], "y");
		   if (interactive)
		     (void) fprintf(stderr,"\n\tdummy variable is t for curves, u/v for surfaces\n");
	    }
	    c_token++;
	}
	else if (almost_equals(c_token,"nopar$ametric")) {
		if (parametric) {
		   parametric = FALSE;
		   strcpy (dummy_var[0], "x");
		   strcpy (dummy_var[1], "y");
		   if (interactive)
		     (void) fprintf(stderr,"\n\tdummy variable is x for curves, x/y for surfaces\n");
	    }
	    c_token++;
	}
	else if (almost_equals(c_token,"pol$ar")) {
	    if (!polar) {
			polar = TRUE;
			if (parametric) {
				tmin = 0.0;
				tmax = 2*Pi;
			} else if (angles_format == ANGLES_DEGREES) {
				xmin = 0.0;
				xmax = 360.0;
			} else {
				xmin = 0.0;
				xmax = 2*Pi;
			}
	    }
	    c_token++;
	}
	else if (almost_equals(c_token,"nopo$lar")) {
	    if (polar) {
			polar = FALSE;
			if (parametric) {
				tmin = -5.0;
				tmax = 5.0;
			} else {
				xmin = -10.0;
				xmax = 10.0;
			}
	    }
	    c_token++;
	}
	else if (almost_equals(c_token,"an$gles")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		/* assuming same as defaults */
		angles_format = ANGLES_RADIANS;
	    }
	    else if (almost_equals(c_token, "r$adians")) {
		angles_format = ANGLES_RADIANS;
		c_token++;
	    }
	    else if (almost_equals(c_token, "d$egrees")) {
		angles_format = ANGLES_DEGREES;
		c_token++;
	    }
	    else
		 int_error("expecting 'radians' or 'degrees'", c_token);
	}
	else if (almost_equals(c_token,"g$rid")) {
		grid = TRUE;
		c_token++;
	}
	else if (almost_equals(c_token,"nog$rid")) {
		grid = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"su$rface")) {
		draw_surface = TRUE;
		c_token++;
	}
	else if (almost_equals(c_token,"nosu$rface")) {
		draw_surface = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"k$ey")) {
		struct value a;
		c_token++;
		if (END_OF_COMMAND) {
			key = -1;
		} 
		else {
			key_x = real(const_express(&a));
			if (!equals(c_token,","))
				int_error("',' expected",c_token);
			c_token++;
			key_y = real(const_express(&a));
			if (equals(c_token,","))
			{
			        c_token++;
				key_z = real(const_express(&a));
			}
			key = 1;
		} 
	}
	else if (almost_equals(c_token,"nok$ey")) {
		key = 0;
		c_token++;
	}
	else if (almost_equals(c_token,"tic$s")) {
		tic_in = TRUE;
		c_token++;
		if (almost_equals(c_token,"i$n")) {
			tic_in = TRUE;
			c_token++;
		}
		else if (almost_equals(c_token,"o$ut")) {
			tic_in = FALSE;
			c_token++;
		}
	}
     else if (almost_equals(c_token,"xmt$ics")) {
	 xtics = TRUE;
	 c_token++;
	 if(xticdef.type == TIC_USER){
	     free_marklist(xticdef.def.user);
	     xticdef.def.user = NULL;
	 }
	xticdef.type = TIC_MONTH;
     }
     else if (almost_equals(c_token,"xdt$ics")) {
	 xtics = TRUE;
	 c_token++;
	 if(xticdef.type == TIC_USER){
	     free_marklist(xticdef.def.user);
	     xticdef.def.user = NULL;
	 }
	xticdef.type = TIC_DAY;
     }
     else if (almost_equals(c_token,"xt$ics")) {
	    xtics = TRUE;
	    c_token++;
	    if (END_OF_COMMAND) { /* reset to default */
		   if (xticdef.type == TIC_USER) {
			  free_marklist(xticdef.def.user);
			  xticdef.def.user = NULL;
		   }
		   xticdef.type = TIC_COMPUTED;
	    }
	    else
		 load_tics(&xticdef);
	} 
     else if (almost_equals(c_token,"noxt$ics")) {
	    xtics = FALSE;
	    c_token++;
	} 
     else if (almost_equals(c_token,"ymt$ics")) {
	 ytics = TRUE;
	 c_token++;
	 if(yticdef.type == TIC_USER){
	     free_marklist(yticdef.def.user);
	     yticdef.def.user = NULL;
	 }
	yticdef.type = TIC_MONTH;
     }
     else if (almost_equals(c_token,"ydt$ics")) {
	 ytics = TRUE;
	 c_token++;
	 if(yticdef.type == TIC_USER){
	     free_marklist(yticdef.def.user);
	     yticdef.def.user = NULL;
	 }
	yticdef.type = TIC_DAY;
     }
     else if (almost_equals(c_token,"yt$ics")) {
	    ytics = TRUE;
	    c_token++;
	    if (END_OF_COMMAND) { /* reset to default */
		   if (yticdef.type == TIC_USER) {
			  free_marklist(yticdef.def.user);
			  yticdef.def.user = NULL;
		   }
		   yticdef.type = TIC_COMPUTED;
	    }
	    else
		 load_tics(&yticdef);
	} 
     else if (almost_equals(c_token,"noyt$ics")) {
	    ytics = FALSE;
	    c_token++;
	} 
     else if (almost_equals(c_token,"zmt$ics")) {
	 ztics = TRUE;
	 c_token++;
	 if(zticdef.type == TIC_USER){
	     free_marklist(zticdef.def.user);
	     zticdef.def.user = NULL;
	 }
	zticdef.type = TIC_MONTH;
     }
     else if (almost_equals(c_token,"zdt$ics")) {
	 ztics = TRUE;
	 c_token++;
	 if(zticdef.type == TIC_USER){
	     free_marklist(zticdef.def.user);
	     zticdef.def.user = NULL;
	 }
	zticdef.type = TIC_DAY;
     }
     else if (almost_equals(c_token,"zt$ics")) {
	    ztics = TRUE;
	    c_token++;
	    if (END_OF_COMMAND) { /* reset to default */
		   if (zticdef.type == TIC_USER) {
			  free_marklist(zticdef.def.user);
			  zticdef.def.user = NULL;
		   }
		   zticdef.type = TIC_COMPUTED;
	    }
	    else
		 load_tics(&zticdef);
	} 
     else if (almost_equals(c_token,"nozt$ics")) {
	    ztics = FALSE;
	    c_token++;
	} 
    else if (almost_equals(c_token,"ticsl$evel")) {
		double tlvl;
		struct value a;

		c_token++;
		tlvl = real(const_express(&a));
		if (tlvl < 0.0)
			int_error("tics level must be > 0; ticslevel unchanged",
				c_token);
		else {
			ticslevel = tlvl;
		}
    }
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
			int_error("sampling rate must be > 1; sampling unchanged",
				c_token);
		else {
		        extern struct surface_points *first_3dplot;
			register struct surface_points *f_3dp = first_3dplot;

			first_3dplot = NULL;
			sp_free(f_3dp);

			samples = tsamp1;
			samples_1 = tsamp1;
			samples_2 = tsamp2;
		}
    }
    else if (almost_equals(c_token,"isosa$mples")) {
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
			int_error("sampling rate must be > 1; sampling unchanged",
				c_token);
		else {
		        extern struct curve_points *first_plot;
		        extern struct surface_points *first_3dplot;
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
    else if (almost_equals(c_token,"si$ze")) {
		struct value s;
		c_token++;
		if (END_OF_COMMAND) {
			xsize = 1.0;
			ysize = 1.0;
		} 
		else {
				xsize=real(const_express(&s));
				if (!equals(c_token,","))
					int_error("',' expected",c_token);
				c_token++;
				ysize=real(const_express(&s));
		} 
	} 
	else if (almost_equals(c_token,"t$erminal")) {
		c_token++;
		if (END_OF_COMMAND) {
			list_terms();
			screen_ok = FALSE;
		}
		else {
			if (term && term_init) {
				(*term_tbl[term].reset)();
				(void) fflush(outfile);
			}
			term = set_term(c_token);
			c_token++;

			/* get optional mode parameters */
			if (term)
				(*term_tbl[term].options)();
			if (interactive && *term_options)
				fprintf(stderr,"Options are '%s'\n",term_options);
		}
	}
	else if (almost_equals(c_token,"tim$e")) {
		timedate = TRUE;
		c_token++;
		if (!END_OF_COMMAND) {
			struct value a;

			/* We have x,y offsets specified */
			if (!equals(c_token,","))
			    time_xoffset = (int)real(const_express(&a));
			if (!END_OF_COMMAND && equals(c_token,",")) {
				c_token++;
				time_yoffset = (int)real(const_express(&a));
			}
		}
	}
	else if (almost_equals(c_token,"not$ime")) {
		timedate = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"rr$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&rmin,&rmax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_r = FALSE;
	}
	else if (almost_equals(c_token,"tr$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&tmin,&tmax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_t = FALSE;
	}
	else if (almost_equals(c_token,"ur$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&umin,&umax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_u = FALSE;
	}
	else if (almost_equals(c_token,"vi$ew")) {
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
			}
			else {
				if (!was_comma)
					int_error("',' expected",c_token);
				local_vals[i] = real(const_express(&a));
				i++;
				was_comma = FALSE;
			}
		}

		if (local_vals[0] < 0 || local_vals[0] > 180)
			int_error("rot_x must be in [0:180] degrees range; view unchanged",
				  c_token);
		if (local_vals[1] < 0 || local_vals[1] > 360)
			int_error("rot_z must be in [0:360] degrees range; view unchanged",
				  c_token);
		if (local_vals[2] < 1e-6)
			int_error("scale must be > 0; view unchanged", c_token);
		if (local_vals[3] < 1e-6)
			int_error("zscale must be > 0; view unchanged", c_token);

		surface_rot_x = local_vals[0];
		surface_rot_z = local_vals[1];
		surface_scale = local_vals[2];
		surface_zscale = local_vals[3];
	}
	else if (almost_equals(c_token,"vr$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&vmin,&vmax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_v = FALSE;
	}
	else if (almost_equals(c_token,"xr$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_x = FALSE;
	}
	else if (almost_equals(c_token,"yr$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&ymin,&ymax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_y = FALSE;
	}
	else if (almost_equals(c_token,"zr$ange")) {
	     TBOOLEAN changed;
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		changed = load_range(&zmin,&zmax);
		if (!equals(c_token,"]"))
		  int_error("expecting ']'",c_token);
		c_token++;
		if (changed)
		  autoscale_z = FALSE;
	}
	else if (almost_equals(c_token,"z$ero")) {
		struct value a;
		c_token++;
		zero = magnitude(const_express(&a));
	}
	else
		return(FALSE);	/* no command match */
	return(TRUE);
}

/*********** Support functions for set_command ***********/

/* process a 'set {x/y/z}label command */
/* set {x/y/z}label {label_text} {x}{,y} */
static void set_xyzlabel(str,xpos,ypos)
char *str;
int *xpos,*ypos;
{
	c_token++;
	if (END_OF_COMMAND) {	/* no label specified */
		str[0] = '\0';
	} else {
		if (isstring(c_token)) {
			/* We have string specified - grab it. */
			quotel_str(str,c_token);
			c_token++;
		}
		if (!END_OF_COMMAND) {
			struct value a;

			/* We have x,y offsets specified */
			if (!equals(c_token,","))
			    *xpos = (int)real(const_express(&a));
			if (!END_OF_COMMAND && equals(c_token,",")) {
				c_token++;
				*ypos = (int)real(const_express(&a));
			}
		}
	}
}

/* process a 'set label' command */
/* set label {tag} {label_text} {at x,y} {pos} */
static void
set_label()
{
    struct value a;
    struct text_label *this_label = NULL;
    struct text_label *new_label = NULL;
    struct text_label *prev_label = NULL;
    double x, y, z;
    char text[MAX_LINE_LEN+1];
    enum JUSTIFY just;
    int tag;
    TBOOLEAN set_text, set_position, set_just;

    /* get tag */
    if (!END_OF_COMMAND 
	   && !isstring(c_token) 
	   && !equals(c_token, "at")
	   && !equals(c_token, "left")
	   && !equals(c_token, "center")
	   && !equals(c_token, "centre")
	   && !equals(c_token, "right")) {
	   /* must be a tag expression! */
	   tag = (int)real(const_express(&a));
	   if (tag <= 0)
		int_error("tag must be > zero", c_token);
    } else
	 tag = assign_label_tag(); /* default next tag */
	 
    /* get text */
    if (!END_OF_COMMAND && isstring(c_token)) {
	   /* get text */
	   quotel_str(text, c_token);
	   c_token++;
	   set_text = TRUE;
    } else {
	   text[0] = '\0';		/* default no text */
	   set_text = FALSE;
    }
	 
    /* get justification - what the heck, let him put it here */
    if (!END_OF_COMMAND && !equals(c_token, "at")) {
	   if (almost_equals(c_token,"l$eft")) {
		  just = LEFT;
	   }
	   else if (almost_equals(c_token,"c$entre")
			  || almost_equals(c_token,"c$enter")) {
		  just = CENTRE;
	   }
	   else if (almost_equals(c_token,"r$ight")) {
		  just = RIGHT;
	   }
	   else
		int_error("bad syntax in set label", c_token);
	   c_token++;
	   set_just = TRUE;
    } else {
	   just = LEFT;			/* default left justified */
	   set_just = FALSE;
    } 

    /* get position */
    if (!END_OF_COMMAND && equals(c_token, "at")) {
	   c_token++;
	   if (END_OF_COMMAND)
		int_error("coordinates expected", c_token);
	   /* get coordinates */
	   x = real(const_express(&a));
	   if (!equals(c_token,","))
		int_error("',' expected",c_token);
	   c_token++;
	   y = real(const_express(&a));
	   if (equals(c_token,",")) {
		c_token++;
		z = real(const_express(&a));
	   }
	   else
	        z = 0;
	   set_position = TRUE;
    } else {
	   x = y = z = 0;			/* default at origin */
	   set_position = FALSE;
    }

    /* get justification */
    if (!END_OF_COMMAND) {
	   if (set_just)
		int_error("only one justification is allowed", c_token);
	   if (almost_equals(c_token,"l$eft")) {
		  just = LEFT;
	   }
	   else if (almost_equals(c_token,"c$entre")
			  || almost_equals(c_token,"c$enter")) {
		  just = CENTRE;
	   }
	   else if (almost_equals(c_token,"r$ight")) {
		  just = RIGHT;
	   }
	   else
		int_error("bad syntax in set label", c_token);
	   c_token++;
	   set_just = TRUE;
    } 

    if (!END_OF_COMMAND)
	 int_error("extraenous or out-of-order arguments in set label", c_token);

    /* OK! add label */
    if (first_label != NULL) { /* skip to last label */
	   for (this_label = first_label; this_label != NULL ; 
		   prev_label = this_label, this_label = this_label->next)
		/* is this the label we want? */
		if (tag <= this_label->tag)
		  break;
    }
    if (this_label != NULL && tag == this_label->tag) {
	   /* changing the label */
	   if (set_position) {
		  this_label->x = x;
		  this_label->y = y;
		  this_label->z = z;
	   }
	   if (set_text)
		(void) strcpy(this_label->text, text);
	   if (set_just)
		this_label->pos = just;
    } else {
	   /* adding the label */
	   new_label = (struct text_label *) 
		alloc ( (unsigned long) sizeof(struct text_label), "label");
	   if (prev_label != NULL)
		prev_label->next = new_label; /* add it to end of list */
	   else 
		first_label = new_label; /* make it start of list */
	   new_label->tag = tag;
	   new_label->next = this_label;
	   new_label->x = x;
	   new_label->y = y;
	   new_label->z = z;
	   (void) strcpy(new_label->text, text);
	   new_label->pos = just;
    }
}

/* process 'set nolabel' command */
/* set nolabel {tag} */
static void
set_nolabel()
{
    struct value a;
    struct text_label *this_label;
    struct text_label *prev_label; 
    int tag;

    if (END_OF_COMMAND) {
	   /* delete all labels */
	   while (first_label != NULL)
		delete_label((struct text_label *)NULL,first_label);
    }
    else {
	   /* get tag */
	   tag = (int)real(const_express(&a));
	   if (!END_OF_COMMAND)
		int_error("extraneous arguments to set nolabel", c_token);
	   for (this_label = first_label, prev_label = NULL;
		   this_label != NULL;
		   prev_label = this_label, this_label = this_label->next) {
		  if (this_label->tag == tag) {
			 delete_label(prev_label,this_label);
			 return;		/* exit, our job is done */
		  }
	   }
	   int_error("label not found", c_token);
    }
}

/* assign a new label tag */
/* labels are kept sorted by tag number, so this is easy */
static int				/* the lowest unassigned tag number */
assign_label_tag()
{
    struct text_label *this_label;
    int last = 0;			/* previous tag value */

    for (this_label = first_label; this_label != NULL;
	    this_label = this_label->next)
	 if (this_label->tag == last+1)
	   last++;
	 else
	   break;
    
    return (last+1);
}

/* delete label from linked list started by first_label.
 * called with pointers to the previous label (prev) and the 
 * label to delete (this).
 * If there is no previous label (the label to delete is
 * first_label) then call with prev = NULL.
 */
static void
delete_label(prev,this)
	struct text_label *prev, *this;
{
    if (this!=NULL)	{		/* there really is something to delete */
	   if (prev!=NULL)		/* there is a previous label */
		prev->next = this->next; 
	   else				/* this = first_label so change first_label */
		first_label = this->next;
	   free((char *)this);
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
    double sx, sy, sz;
    double ex, ey, ez;
    int tag;
    TBOOLEAN set_start, set_end, head = 1;

    /* get tag */
    if (!END_OF_COMMAND 
	   && !equals(c_token, "from")
	   && !equals(c_token, "to")) {
	   /* must be a tag expression! */
	   tag = (int)real(const_express(&a));
	   if (tag <= 0)
		int_error("tag must be > zero", c_token);
    } else
	 tag = assign_arrow_tag(); /* default next tag */
	 
    /* get start position */
    if (!END_OF_COMMAND && equals(c_token, "from")) {
	   c_token++;
	   if (END_OF_COMMAND)
		int_error("start coordinates expected", c_token);
	   /* get coordinates */
	   sx = real(const_express(&a));
	   if (!equals(c_token,","))
		int_error("',' expected",c_token);
	   c_token++;
	   sy = real(const_express(&a));
	   if (equals(c_token,",")) {
		c_token++;
		sz = real(const_express(&a));
	   }
	   else
	       sz = 0;
	   set_start = TRUE;
    } else {
	   sx = sy = sz = 0;			/* default at origin */
	   set_start = FALSE;
    }

    /* get end position */
    if (!END_OF_COMMAND && equals(c_token, "to")) {
	   c_token++;
	   if (END_OF_COMMAND)
		int_error("end coordinates expected", c_token);
	   /* get coordinates */
	   ex = real(const_express(&a));
	   if (!equals(c_token,","))
		int_error("',' expected",c_token);
	   c_token++;
	   ey = real(const_express(&a));
	   if (equals(c_token,",")) {
		c_token++;
		ez = real(const_express(&a));
	   }
	   else
		ez = 0;
	   set_end = TRUE;
    } else {
	   ex = ey = ez = 0;			/* default at origin */
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
	   sx = real(const_express(&a));
	   if (!equals(c_token,","))
		int_error("',' expected",c_token);
	   c_token++;
	   sy = real(const_express(&a));
	   if (equals(c_token,",")) {
		c_token++;
		sz = real(const_express(&a));
	   }
	   else
	       sz = 0;
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

    if (!END_OF_COMMAND)
	 int_error("extraneous or out-of-order arguments in set arrow", c_token);

    /* OK! add arrow */
    if (first_arrow != NULL) { /* skip to last arrow */
	   for (this_arrow = first_arrow; this_arrow != NULL ; 
		   prev_arrow = this_arrow, this_arrow = this_arrow->next)
		/* is this the arrow we want? */
		if (tag <= this_arrow->tag)
		  break;
    }
    if (this_arrow != NULL && tag == this_arrow->tag) {
	   /* changing the arrow */
	   if (set_start) {
		  this_arrow->sx = sx;
		  this_arrow->sy = sy;
		  this_arrow->sz = sz;
	   }
	   if (set_end) {
		  this_arrow->ex = ex;
		  this_arrow->ey = ey;
		  this_arrow->ez = ez;
	   }
	   this_arrow->head = head;
    } else {
	   /* adding the arrow */
	   new_arrow = (struct arrow_def *) 
		alloc ( (unsigned long) sizeof(struct arrow_def), "arrow");
	   if (prev_arrow != NULL)
		prev_arrow->next = new_arrow; /* add it to end of list */
	   else 
		first_arrow = new_arrow; /* make it start of list */
	   new_arrow->tag = tag;
	   new_arrow->next = this_arrow;
	   new_arrow->sx = sx;
	   new_arrow->sy = sy;
	   new_arrow->sz = sz;
	   new_arrow->ex = ex;
	   new_arrow->ey = ey;
	   new_arrow->ez = ez;
	   new_arrow->head = head;
    }
}

/* process 'set noarrow' command */
/* set noarrow {tag} */
static void
set_noarrow()
{
    struct value a;
    struct arrow_def *this_arrow;
    struct arrow_def *prev_arrow; 
    int tag;

    if (END_OF_COMMAND) {
	   /* delete all arrows */
	   while (first_arrow != NULL)
		delete_arrow((struct arrow_def *)NULL,first_arrow);
    }
    else {
	   /* get tag */
	   tag = (int)real(const_express(&a));
	   if (!END_OF_COMMAND)
		int_error("extraneous arguments to set noarrow", c_token);
	   for (this_arrow = first_arrow, prev_arrow = NULL;
		   this_arrow != NULL;
		   prev_arrow = this_arrow, this_arrow = this_arrow->next) {
		  if (this_arrow->tag == tag) {
			 delete_arrow(prev_arrow,this_arrow);
			 return;		/* exit, our job is done */
		  }
	   }
	   int_error("arrow not found", c_token);
    }
}

/* assign a new arrow tag */
/* arrows are kept sorted by tag number, so this is easy */
static int				/* the lowest unassigned tag number */
assign_arrow_tag()
{
    struct arrow_def *this_arrow;
    int last = 0;			/* previous tag value */

    for (this_arrow = first_arrow; this_arrow != NULL;
	    this_arrow = this_arrow->next)
	 if (this_arrow->tag == last+1)
	   last++;
	 else
	   break;

    return (last+1);
}

/* delete arrow from linked list started by first_arrow.
 * called with pointers to the previous arrow (prev) and the 
 * arrow to delete (this).
 * If there is no previous arrow (the arrow to delete is
 * first_arrow) then call with prev = NULL.
 */
static void
delete_arrow(prev,this)
	struct arrow_def *prev, *this;
{
    if (this!=NULL)	{		/* there really is something to delete */
	   if (prev!=NULL)		/* there is a previous arrow */
		prev->next = this->next; 
	   else				/* this = first_arrow so change first_arrow */
		first_arrow = this->next;
	   free((char *)this);
    }
}


enum PLOT_STYLE			/* not static; used by command.c */
get_style()
{
register enum PLOT_STYLE ps;

	c_token++;
	if (almost_equals(c_token,"l$ines"))
		ps = LINES;
	else if (almost_equals(c_token,"i$mpulses"))
		ps = IMPULSES;
	else if (almost_equals(c_token,"p$oints"))
		ps = POINTSTYLE;
	else if (almost_equals(c_token,"linesp$oints"))
		ps = LINESPOINTS;
	else if (almost_equals(c_token,"d$ots"))
		ps = DOTS;
	else if (almost_equals(c_token,"e$rrorbars"))
		ps = ERRORBARS;
	else if (almost_equals(c_token,"b$oxes"))
		ps = BOXES;
	else if (almost_equals(c_token,"boxer$rorbars"))
		ps = BOXERROR;
	else if (almost_equals(c_token,"s$teps"))
		ps = STEPS;
	else
		int_error("expecting 'lines', 'points', 'linespoints', 'dots', 'impulses', \n\
        'errorbars', 'steps', 'boxes' or 'boxerrorbars'",c_token);
	c_token++;
	return(ps);
}

/* For set [xy]tics... command*/
static void
load_tics(tdef)
	struct ticdef *tdef;	/* change this ticdef */
{
    if (equals(c_token,"(")) { /* set : TIC_USER */
	   c_token++;
	   load_tic_user(tdef);
    } else {				/* series : TIC_SERIES */
	   load_tic_series(tdef);
    }
}

/* load TIC_USER definition */
/* (tic[,tic]...)
 * where tic is ["string"] value
 * Left paren is already scanned off before entry.
 */
static void
load_tic_user(tdef)
	struct ticdef *tdef;
{
    struct ticmark *list = NULL; /* start of list */
    struct ticmark *last = NULL; /* end of list */
    struct ticmark *tic = NULL; /* new ticmark */
    char temp_string[MAX_LINE_LEN];
    struct value a;

    while (!END_OF_COMMAND) {
	   /* parse a new ticmark */
	   tic = (struct ticmark *)alloc((unsigned long)sizeof(struct ticmark), (char *)NULL);
	   if (tic == (struct ticmark *)NULL) {
		  free_marklist(list);
		  int_error("out of memory for tic mark", c_token);
	   }

	   /* has a string with it? */
	   if (isstring(c_token)) {
		  quote_str(temp_string,c_token);
		  tic->label = alloc((unsigned long)strlen(temp_string)+1, "tic label");
		  (void) strcpy(tic->label, temp_string);
		  c_token++;
	   } else
		tic->label = NULL;

	   /* in any case get the value */
	   tic->position = real(const_express(&a));
	   tic->next = NULL;

	   /* append to list */
	   if (list == NULL)
		last = list = tic;	/* new list */
	   else {				/* append to list */
		  last->next = tic;
		  last = tic;
	   }

	   /* expect "," or ")" here */
	   if (!END_OF_COMMAND && equals(c_token, ","))
		c_token++;		/* loop again */
	   else
		break;			/* hopefully ")" */
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

static void
free_marklist(list)
	struct ticmark *list;
{
    register struct ticmark *freeable;

    while (list != NULL) {
	   freeable = list;
	   list = list->next;
	   if (freeable->label != NULL)
		free( (char *)freeable->label );
	   free( (char *)freeable );
    }
}

/* load TIC_SERIES definition */
/* start,incr[,end] */
static void
load_tic_series(tdef)
	struct ticdef *tdef;
{
    double start, incr, end;
    struct value a;
    int incr_token;

    start = real(const_express(&a));
    if (!equals(c_token, ","))
	 int_error("expecting comma to separate start,incr", c_token);
    c_token++;

    incr_token = c_token;
    incr = real(const_express(&a));

    if (END_OF_COMMAND)
	 end = VERYLARGE;
    else {
	   if (!equals(c_token, ","))
		int_error("expecting comma to separate incr,end", c_token);
	   c_token++;

	   end = real(const_express(&a));
    }
    if (!END_OF_COMMAND)
	 int_error("tic series is defined by start,increment[,end]", 
			 c_token);
    
    if (start < end && incr <= 0)
	 int_error("increment must be positive", incr_token);
    if (start > end && incr >= 0)
	 int_error("increment must be negative", incr_token);
    if (start > end) {
	   /* put in order */
		double numtics;
		numtics = floor( (end*(1+SIGNIF) - start)/incr );
		end = start;
		start = end + numtics*incr;
		incr = -incr;
/*
	   double temp = start;
	   start = end;
	   end = temp;
	   incr = -incr;
 */
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
load_offsets (a, b, c, d)
double *a,*b, *c, *d;
{
struct value t;

	*a = real (const_express(&t));  /* loff value */
	c_token++;
	if (equals(c_token,","))
		c_token++;
	if (END_OF_COMMAND) 
	    return;

	*b = real (const_express(&t));  /* roff value */
	c_token++;
	if (equals(c_token,","))
		c_token++;
	if (END_OF_COMMAND) 
	    return;

	*c = real (const_express(&t));  /* toff value */
	c_token++;
	if (equals(c_token,","))
		c_token++;
	if (END_OF_COMMAND) 
	    return;

	*d = real (const_express(&t));  /* boff value */
	c_token++;
}


TBOOLEAN					/* TRUE if a or b were changed */
load_range(a,b)			/* also used by command.c */
double *a,*b;
{
struct value t;
TBOOLEAN changed = FALSE;

	if (equals(c_token,"]"))
		return(FALSE);
	if (END_OF_COMMAND) {
	    int_error("starting range value or ':' or 'to' expected",c_token);
	} else if (!equals(c_token,"to") && !equals(c_token,":"))  {
		*a = real(const_express(&t));
		changed = TRUE;
	}	
	if (!equals(c_token,"to") && !equals(c_token,":"))
		int_error("':' or keyword 'to' expected",c_token);
	c_token++;
	if (!equals(c_token,"]")) {
		*b = real(const_express(&t));
		changed = TRUE;
	 }
     return(changed);
}



/******* The 'show' command *******/
void
show_command()
{
    static char GPFAR showmess[] = 
	"valid show options:  'action_table', 'all', 'angles', 'arrow', \n\
	'autoscale', 'border', 'boxwidth', 'clip', 'contour', 'data', \n\
	'dgrid3d', 'dummy', 'format', 'function', 'grid', 'hidden', 'key', \n\
	'label', 'logscale', 'mapping',  'offsets', 'output', 'plot', \n\
	'parametric', 'polar', 'rrange', 'samples', 'isosamples', 'view', \n\
	'size', 'terminal', 'tics', 'ticslevel', 'time', 'title', 'trange', \n\
	'urange', 'vrange', 'variables', 'version', \n\
	'xlabel', 'xrange', '{no}xtics', 'xmtics', 'xdtics', '{no}xzeroaxis',\n\
	'ylabel', 'yrange', '{no}ytics', 'ymtics', 'ydtics', '{no}yzeroaxis',\n\
 	'zero', '{no}zeroaxis', 'zlabel', 'zrange', '{no}ztics',\n\
 	'zmtics', 'zdtics'";

    c_token++;

    if (!show_one() && !show_two())
	int_error(showmess, c_token);
	screen_ok = FALSE;
	(void) putc('\n',stderr);
}

/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
show_one()
{
	if (almost_equals(c_token,"ac$tion_table") ||
			 equals(c_token,"at") ) {
		c_token++; 
		show_at();
		c_token++;
	}
	else if (almost_equals(c_token,"ar$row")) {
	    struct value a;
	    int tag = 0;

	    c_token++;
	    if (!END_OF_COMMAND) {
		   tag = (int)real(const_express(&a));
		   if (tag <= 0)
			int_error("tag must be > zero", c_token);
	    }

	    (void) putc('\n',stderr);
	    show_arrow(tag);
	}
	else if (almost_equals(c_token,"au$toscale")) {
		(void) putc('\n',stderr);
		show_autoscale();
		c_token++;
	}
	else if (almost_equals(c_token,"bor$der")) {
		(void) putc('\n',stderr);
		show_border();
		c_token++;
	}
	else if (almost_equals(c_token,"box$width")) {
		(void) putc('\n',stderr);
		show_boxwidth();
		c_token++;
	}
	else if (almost_equals(c_token,"c$lip")) {
		(void) putc('\n',stderr);
		show_clip();
		c_token++;
	}
	else if (almost_equals(c_token,"ma$pping")) {
		(void) putc('\n',stderr);
		show_mapping();
		c_token++;
	}
	else if (almost_equals(c_token,"co$ntour")) {
		(void) putc('\n',stderr);
		show_contour();
		c_token++;
	}
	else if (almost_equals(c_token,"da$ta")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		(void) putc('\n',stderr);
		show_style("data",data_style);
		c_token++;
	}
	else if (almost_equals(c_token,"dg$rid3d")) {
		(void) putc('\n',stderr);
		show_dgrid3d();
		c_token++;
	}
	else if (almost_equals(c_token,"du$mmy")) {
	  	(void) fprintf(stderr,"\n\tdummy variables are \"%s\" and \"%s\"\n",
	    				dummy_var[0], dummy_var[1]);
		c_token++;
	}
	else if (almost_equals(c_token,"fo$rmat")) {
		show_format();
		c_token++;
	}
	else if (almost_equals(c_token,"fu$nctions")) {
		c_token++;
		if (almost_equals(c_token,"s$tyle"))  {
			(void) putc('\n',stderr);
			show_style("functions",func_style);
			c_token++;
		}
		else
			show_functions();
	}
	else if (almost_equals(c_token,"lo$gscale")) {
		(void) putc('\n',stderr);
		show_logscale();
		c_token++;
	}
	else if (almost_equals(c_token,"of$fsets")) {
		(void) putc('\n',stderr);
		show_offsets();
		c_token++;
	}
	else if (almost_equals(c_token,"o$utput")) {
		(void) putc('\n',stderr);
		show_output();
		c_token++;
	}
	else if (almost_equals(c_token,"tit$le")) {
		(void) putc('\n',stderr);
		show_title();
		c_token++;
	}
	else if (almost_equals(c_token,"xl$abel")) {
		(void) putc('\n',stderr);
		show_xlabel();
		c_token++;
	}
	else if (almost_equals(c_token,"yl$abel")) {
		(void) putc('\n',stderr);
		show_ylabel();
		c_token++;
	}
	else if (almost_equals(c_token,"zl$abel")) {
		(void) putc('\n',stderr);
		show_zlabel();
		c_token++;
	}
	else if (almost_equals(c_token,"xzero$axis")) {
		(void) putc('\n',stderr);
		show_xzeroaxis();
		c_token++;
	}
	else if (almost_equals(c_token,"yzero$axis")) {
		(void) putc('\n',stderr);
		show_yzeroaxis();
		c_token++;
	}
	else if (almost_equals(c_token,"zeroa$xis")) {
		(void) putc('\n',stderr);
		show_xzeroaxis();
		show_yzeroaxis();
		c_token++;
	}
	else if (almost_equals(c_token,"la$bel")) {
	    struct value a;
	    int tag = 0;

	    c_token++;
	    if (!END_OF_COMMAND) {
		   tag = (int)real(const_express(&a));
		   if (tag <= 0)
			int_error("tag must be > zero", c_token);
	    }

	    (void) putc('\n',stderr);
	    show_label(tag);
	}
	else if (almost_equals(c_token,"g$rid")) {
		(void) putc('\n',stderr);
		show_grid();
		c_token++;
	}
	else if (almost_equals(c_token,"k$ey")) {
		(void) putc('\n',stderr);
		show_key();
		c_token++;
	}
	else
		return (FALSE);
	return TRUE;
}

/* return TRUE if a command match, FALSE if not */
static TBOOLEAN
show_two()
{
	if (almost_equals(c_token,"p$lot")) {
		(void) putc('\n',stderr);
		show_plot();
		c_token++;
	}
	else if (almost_equals(c_token,"par$ametric")) {
		(void) putc('\n',stderr);
		show_parametric();
		c_token++;
	}
	else if (almost_equals(c_token,"pol$ar")) {
		(void) putc('\n',stderr);
		show_polar();
		c_token++;
	}
	else if (almost_equals(c_token,"an$gles")) {
		(void) putc('\n',stderr);
		show_angles();
		c_token++;
	}
	else if (almost_equals(c_token,"ti$cs")) {
		(void) putc('\n',stderr);
		show_tics(TRUE,TRUE,TRUE);
		c_token++;
	}
	else if (almost_equals(c_token,"tim$e")) {
		(void) putc('\n',stderr);
		show_time();
		c_token++;
	}
	else if (almost_equals(c_token,"su$rface")) {
		(void) putc('\n',stderr);
		show_surface();
		c_token++;
	}
	else if (almost_equals(c_token,"hi$dden3d")) {
		(void) putc('\n',stderr);
		show_hidden3d();
		c_token++;
	}
 	else if (almost_equals(c_token,"cla$bel")) {
 		(void) putc('\n',stderr);
 		show_label_contours();
 		c_token++;
 	}
	else if (almost_equals(c_token,"xti$cs")) {
	    show_tics(TRUE,FALSE,FALSE);
	    c_token++;
	}
	else if (almost_equals(c_token,"yti$cs")) {
	    show_tics(FALSE,TRUE,FALSE);
	    c_token++;
	}
	else if (almost_equals(c_token,"zti$cs")) {
	    show_tics(FALSE,FALSE,TRUE);
	    c_token++;
	}
	else if (almost_equals(c_token,"sa$mples")) {
		(void) putc('\n',stderr);
		show_samples();
		c_token++;
	}
	else if (almost_equals(c_token,"isosa$mples")) {
		(void) putc('\n',stderr);
		show_isosamples();
		c_token++;
	}
	else if (almost_equals(c_token,"si$ze")) {
		(void) putc('\n',stderr);
		show_size();
		c_token++;
	}
	else if (almost_equals(c_token,"t$erminal")) {
		(void) putc('\n',stderr);
		show_term();
		c_token++;
	}
	else if (almost_equals(c_token,"rr$ange")) {
		(void) putc('\n',stderr);
		show_range('r',rmin,rmax);
		c_token++;
	}
	else if (almost_equals(c_token,"tr$ange")) {
		(void) putc('\n',stderr);
		show_range('t',tmin,tmax);
		c_token++;
	}
	else if (almost_equals(c_token,"ur$ange")) {
		(void) putc('\n',stderr);
		show_range('u',umin,umax);
		c_token++;
	}
	else if (almost_equals(c_token,"vi$ew")) {
		(void) putc('\n',stderr);
		show_view();
		c_token++;
	}
	else if (almost_equals(c_token,"vr$ange")) {
		(void) putc('\n',stderr);
		show_range('v',vmin,vmax);
		c_token++;
	}
	else if (almost_equals(c_token,"v$ariables")) {
		show_variables();
		c_token++;
	}
	else if (almost_equals(c_token,"ve$rsion")) {
		show_version();
		c_token++;
	}
	else if (almost_equals(c_token,"xr$ange")) {
		(void) putc('\n',stderr);
		show_range('x',xmin,xmax);
		c_token++;
	}
	else if (almost_equals(c_token,"yr$ange")) {
		(void) putc('\n',stderr);
		show_range('y',ymin,ymax);
		c_token++;
	}
	else if (almost_equals(c_token,"zr$ange")) {
		(void) putc('\n',stderr);
		show_range('z',zmin,zmax);
		c_token++;
	}
	else if (almost_equals(c_token,"z$ero")) {
		(void) putc('\n',stderr);
		show_zero();
		c_token++;
	}
	else if (almost_equals(c_token,"a$ll")) {
		c_token++;
		show_version();
		show_autoscale();
		show_border();
		show_boxwidth();
		show_clip();
		show_contour();
		show_dgrid3d();
		show_mapping();
	  	(void) fprintf(stderr,"\tdummy variables are \"%s\" and \"%s\"\n",
	    				dummy_var[0], dummy_var[1]);
		show_format();
		show_style("data",data_style);
		show_style("functions",func_style);
		show_grid();
		show_label(0);
		show_arrow(0);
		show_key();
		show_logscale();
		show_offsets();
		show_output();
		show_parametric();
		show_polar();
		show_angles();
		show_samples();
		show_isosamples();
		show_view();
		show_surface();
#ifndef LITE
		show_hidden3d();
#endif
		show_size();
		show_term();
		show_tics(TRUE,TRUE,TRUE);
		show_time();
		if (parametric)
			if (!is_3d_plot)
				show_range('t',tmin,tmax);
			else {
				show_range('u',umin,umax);
				show_range('v',vmin,vmax);
			}
		if (polar)
		  show_range('r',rmin,rmax);
		show_range('x',xmin,xmax);
		show_range('y',ymin,ymax);
		show_range('z',zmin,zmax);
		show_title();
		show_xlabel();
		show_ylabel();
		show_zlabel();
		show_zero();
		show_plot();
		show_variables();
		show_functions();
		c_token++;
	}
	else
		return (FALSE);
	return (TRUE);
}


/*********** support functions for 'show'  **********/
static void
show_style(name,style)
char name[];
enum PLOT_STYLE style;
{
	fprintf(stderr,"\t%s are plotted with ",name);
	switch (style) {
		case LINES: fprintf(stderr,"lines\n"); break;
		case POINTSTYLE: fprintf(stderr,"points\n"); break;
		case IMPULSES: fprintf(stderr,"impulses\n"); break;
		case LINESPOINTS: fprintf(stderr,"linespoints\n"); break;
		case DOTS: fprintf(stderr,"dots\n"); break;
		case ERRORBARS: fprintf(stderr,"errorbars\n"); break;
		case BOXES: fprintf(stderr,"boxes\n"); break;
		case BOXERROR: fprintf(stderr,"boxerrorbars\n"); break;
		case STEPS: fprintf(stderr,"steps\n"); break;
	}
}

static void
show_boxwidth()
{
	if (boxwidth<0.0)
		fprintf(stderr,"\tboxwidth is auto\n");
	else
		fprintf(stderr,"\tboxwidth is %g\n",boxwidth);
}
static void
show_dgrid3d()
{
	if (dgrid3d)
		fprintf(stderr,"\tdata grid3d is enabled for mesh of size %dx%d, norm=%d\n",
			dgrid3d_row_fineness,
			dgrid3d_col_fineness,
			dgrid3d_norm_value);
	else
		fprintf(stderr,"\tdata grid3d is disabled\n");
}

static void
show_range(name,min,max)
char name;
double min,max;
{
	fprintf(stderr,"\t%crange is [%g : %g]\n",name,min,max);
}

static void
show_zero()
{
	fprintf(stderr,"\tzero is %g\n",zero);
}

static void
show_offsets()
{
	fprintf(stderr,"\toffsets are %g, %g, %g, %g\n",loff,roff,toff,boff);
}

static void
show_border()
{
	fprintf(stderr,"\tborder is %sdrawn\n", draw_border ? "" : "not ");
}

static void
show_output()
{
	fprintf(stderr,"\toutput is sent to %s\n",outstr);
}

static void
show_samples()
{
	fprintf(stderr,"\tsampling rate is %d, %d\n",samples_1, samples_2);
}

static void
show_isosamples()
{
	fprintf(stderr,"\tiso sampling rate is %d, %d\n",
		iso_samples_1, iso_samples_2);
}

static void
show_surface()
{
	fprintf(stderr,"\tsurface is %sdrawn\n", draw_surface ? "" : "not ");
}

static void
show_hidden3d()
{
#ifdef LITE
	printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
	fprintf(stderr,"\thidden surface is %s\n", hidden3d ? "removed" : "drawn");
#endif /* LITE */
}

static void
show_label_contours()
{
	fprintf(stderr,"\tcontour line types are %s\n", label_contours ? "varied & labeled" : "all the same");
}

static void
show_view()
{
	fprintf(stderr,"\tview is %g rot_x, %g rot_z, %g scale, %g scale_z\n",
		surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
}

static void
show_size()
{
	fprintf(stderr,"\tsize is scaled by %g,%g\n",xsize,ysize);
}

static void
show_title()
{
	fprintf(stderr,"\ttitle is \"%s\", offset at %d, %d\n",
		title,title_xoffset,title_yoffset);
}

static void
show_xlabel()
{
	fprintf(stderr,"\txlabel is \"%s\", offset at %d, %d\n",
		xlabel,xlabel_xoffset,xlabel_yoffset);
}

static void
show_ylabel()
{
	fprintf(stderr,"\tylabel is \"%s\", offset at %d, %d\n",
		ylabel,ylabel_xoffset,ylabel_yoffset);
}
static void
show_zlabel()
{
	fprintf(stderr,"\tzlabel is \"%s\", offset at %d, %d\n",
		zlabel,zlabel_xoffset,zlabel_yoffset);
}

static void
show_xzeroaxis()
{
	fprintf(stderr,"\txzeroaxis is %s\n",(xzeroaxis)? "ON" : "OFF");
}

static void
show_yzeroaxis()
{
	fprintf(stderr,"\tyzeroaxis is %s\n",(yzeroaxis)? "ON" : "OFF");
}

static void
show_label(tag)
    int tag;				/* 0 means show all */
{
    struct text_label *this_label;
    TBOOLEAN showed = FALSE;

    for (this_label = first_label; this_label != NULL;
	    this_label = this_label->next) {
	   if (tag == 0 || tag == this_label->tag) {
		  showed = TRUE;
		  fprintf(stderr,"\tlabel %d \"%s\" at %g,%g,%g ",
				this_label->tag, this_label->text, 
				this_label->x, this_label->y, this_label->z);
		  switch(this_label->pos) {
			 case LEFT : {
				fprintf(stderr,"left");
				break;
			 }
			 case CENTRE : {
				fprintf(stderr,"centre");
				break;
			 }
			 case RIGHT : {
				fprintf(stderr,"right");
				break;
			 }
		  }
		  fputc('\n',stderr);
	   }
    }
    if (tag > 0 && !showed)
	 int_error("label not found", c_token);
}

static void
show_arrow(tag)
    int tag;				/* 0 means show all */
{
    struct arrow_def *this_arrow;
    TBOOLEAN showed = FALSE;

    for (this_arrow = first_arrow; this_arrow != NULL;
	    this_arrow = this_arrow->next) {
	   if (tag == 0 || tag == this_arrow->tag) {
		  showed = TRUE;
		  fprintf(stderr,"\tarrow %d from %g,%g,%g to %g,%g,%g%s\n",
				this_arrow->tag, 
				this_arrow->sx, this_arrow->sy, this_arrow->sz,
				this_arrow->ex, this_arrow->ey, this_arrow->ez,
				this_arrow->head ? "" : " (nohead)");
	   }
    }
    if (tag > 0 && !showed)
	 int_error("arrow not found", c_token);
}

static void
show_grid()
{
	fprintf(stderr,"\tgrid is %s\n",(grid)? "ON" : "OFF");
}

static void
show_key()
{
	switch (key) {
		case -1 : 
			fprintf(stderr,"\tkey is ON\n");
			break;
		case 0 :
			fprintf(stderr,"\tkey is OFF\n");
			break;
		case 1 :
			fprintf(stderr,"\tkey is at %g,%g,%g\n",key_x,key_y,key_z);
			break;
	}
}

static void
show_parametric()
{
	fprintf(stderr,"\tparametric is %s\n",(parametric)? "ON" : "OFF");
}

static void
show_polar()
{
	fprintf(stderr,"\tpolar is %s\n",(polar)? "ON" : "OFF");
}

static void
show_angles()
{
	fprintf(stderr,"\tAngles are in ");
	switch (angles_format) {
	    case ANGLES_RADIANS:
	        fprintf(stderr, "radians\n");
		break;
	    case ANGLES_DEGREES:
	        fprintf(stderr, "degrees\n");
		break;
	}
}


static void
show_tics(showx, showy, showz)
	TBOOLEAN showx, showy, showz;
{
    fprintf(stderr,"\ttics are %s, ",(tic_in)? "IN" : "OUT");
    fprintf(stderr,"\tticslevel is %g\n",ticslevel);

    if (showx)
	 show_ticdef(xtics, 'x', &xticdef);
    if (showy)
	 show_ticdef(ytics, 'y', &yticdef);
    if (showz)
	 show_ticdef(ztics, 'z', &zticdef);
    screen_ok = FALSE;
}

/* called by show_tics */
static void
show_ticdef(tics, axis, tdef)
	TBOOLEAN tics;			/* xtics ytics or ztics */
	char axis;			/* 'x' 'y' or 'z' */
	struct ticdef *tdef;	/* xticdef yticdef or zticdef */
{
    register struct ticmark *t;

    fprintf(stderr, "\t%c-axis tic labelling is ", axis);
    if (!tics) {
	   fprintf(stderr, "OFF\n");
	   return;
    }

    switch(tdef->type) {
	   case TIC_COMPUTED: {
		  fprintf(stderr, "computed automatically\n");
		  break;
	   }
	    case TIC_MONTH: {
		fprintf(stderr, "Months computed automatically\n");
		break;
	   }
	    case TIC_DAY:{
		fprintf(stderr, "Days computed automatically\n");
	    }
	   case TIC_SERIES: {
		  if (tdef->def.series.end == VERYLARGE)
		    fprintf(stderr, "series from %g by %g\n", 
				  tdef->def.series.start, tdef->def.series.incr);
		  else
		    fprintf(stderr, "series from %g by %g until %g\n", 
				  tdef->def.series.start, tdef->def.series.incr, 
				  tdef->def.series.end);
		  break;
	   }
	   case TIC_USER: {
		  fprintf(stderr, "list (");
		  for (t = tdef->def.user; t != NULL; t=t->next) {
			 if (t->label)
			   fprintf(stderr, "\"%s\" ", t->label);
			 if (t->next)
			   fprintf(stderr, "%g, ", t->position);
			 else
			   fprintf(stderr, "%g", t->position);
		  }
		  fprintf(stderr, ")\n");
		  break;
	   }
	   default: {
		  int_error("unknown ticdef type in show_ticdef()", NO_CARET);
		  /* NOTREACHED */
	   }
    }
}

static void
show_time()
{
	fprintf(stderr,"\ttime is %s, offset at %d, %d\n",
		(timedate)? "ON" : "OFF",
		time_xoffset,time_yoffset);
}

static void
show_term()
{
	fprintf(stderr,"\tterminal type is %s %s\n",
		term_tbl[term].name, term_options);
}

static void
show_plot()
{
	fprintf(stderr,"\tlast plot command was: %s\n",replot_line);
}

static void
show_autoscale()
{
	fprintf(stderr,"\tautoscaling is ");
	if (parametric)
		if (is_3d_plot)
			fprintf(stderr,"\tt: %s, ",(autoscale_t)? "ON" : "OFF");
		else
			fprintf(stderr,"\tu: %s, v: %s, ",
						(autoscale_u)? "ON" : "OFF",
						(autoscale_v)? "ON" : "OFF");
	else fprintf(stderr,"\t");

	if (polar) fprintf(stderr,"r: %s, ",(autoscale_r)? "ON" : "OFF");
	fprintf(stderr,"x: %s, ",(autoscale_x)? "ON" : "OFF");
	fprintf(stderr,"y: %s, ",(autoscale_y)? "ON" : "OFF");
	fprintf(stderr,"z: %s\n",(autoscale_z)? "ON" : "OFF");
}

static void
show_clip()
{
	fprintf(stderr,"\tpoint clip is %s\n",(clip_points)? "ON" : "OFF");

	if (clip_lines1)
	  fprintf(stderr,
         "\tdrawing and clipping lines between inrange and outrange points\n");
	else
	  fprintf(stderr,
         "\tnot drawing lines between inrange and outrange points\n");

	if (clip_lines2)
	  fprintf(stderr,
         "\tdrawing and clipping lines between two outrange points\n");
	else
	  fprintf(stderr,
         "\tnot drawing lines between two outrange points\n");
}

static void
show_mapping()
{
	fprintf(stderr,"\tmapping for 3-d data is ");

	switch (mapping3d) {
		case MAP3D_CARTESIAN:
			fprintf(stderr,"cartesian\n");
			break;
		case MAP3D_SPHERICAL:
			fprintf(stderr,"spherical\n");
			break;
		case MAP3D_CYLINDRICAL:
			fprintf(stderr,"cylindrical\n");
			break;
	}
}

static void
show_contour()
{
	fprintf(stderr,"\tcontour for surfaces are %s",
		(draw_contour)? "drawn" : "not drawn\n");

	if (draw_contour) {
	        fprintf(stderr, " in %d levels on ", contour_levels);
		switch (draw_contour) {
			case CONTOUR_BASE:
				fprintf(stderr,"grid base\n");
				break;
			case CONTOUR_SRF:
				fprintf(stderr,"surface\n");
				break;
			case CONTOUR_BOTH:
				fprintf(stderr,"grid base and surface\n");
				break;
		}
		switch (contour_kind) {
			case CONTOUR_KIND_LINEAR:
				fprintf(stderr,"\t\tas linear segments\n");
				break;
			case CONTOUR_KIND_CUBIC_SPL:
				fprintf(stderr,"\t\tas cubic spline interpolation segments with %d pts\n",
					contour_pts);
				break;
			case CONTOUR_KIND_BSPLINE:
				fprintf(stderr,"\t\tas bspline approximation segments of order %d with %d pts\n",
					contour_order, contour_pts);
				break;
		}
		switch (levels_kind) {
			int i;
			case LEVELS_AUTO:
				fprintf(stderr,"\t\t%d automatic levels\n", contour_levels);
				break;
			case LEVELS_DISCRETE:
				fprintf(stderr,"\t\t%d discrete levels at ", contour_levels);
		                fprintf(stderr, "%g", levels_list[0]);
				for(i = 1; i < contour_levels; i++)
					fprintf(stderr,",%g ", levels_list[i]);
				fprintf(stderr,"\n");
				break;
			case LEVELS_INCREMENTAL:
				fprintf(stderr,"\t\t%d incremental levels starting at %g, step %g, end %g\n",
					contour_levels, levels_list[0], levels_list[1],
					levels_list[0]+contour_levels*levels_list[1]);
				break;
		}
		fprintf(stderr,"\t\tcontour line types are %s\n", label_contours ? "varied" : "all the same");
	}
}

static void
show_format()
{
	fprintf(stderr, "\ttic format is x-axis: \"%s\", y-axis: \"%s\", z-axis: \"%s\"\n",
		xformat, yformat, zformat);
}

static void
show_logscale()
{
	if (is_log_x) {
		fprintf(stderr,"\tlogscaling x (base %g)", base_log_x);
		if (is_log_y && is_log_z)
			fprintf(stderr,", y (base %g) and z (base %g)\n",
				base_log_y, base_log_z);
		else if (is_log_y)
			fprintf(stderr," and y (base %g)\n", base_log_y);
		else if (is_log_z)
			fprintf(stderr," and z (base %g)\n", base_log_z);
		else
			fprintf(stderr," only\n");
	} else if (is_log_y) {
		fprintf(stderr,"\tlogscaling y (base %g)", base_log_y);
		if (is_log_z)
			fprintf(stderr," and z (base %g)\n", base_log_z);
		else
			fprintf(stderr," only\n");
	} else if (is_log_z) {
		fprintf(stderr,"\tlogscaling z (base %g) only\n", base_log_z);
	} else {
		fprintf(stderr,"\tno logscaling\n");
	}
}

static void
show_variables()
{
register struct udvt_entry *udv = first_udv;
int len;

	fprintf(stderr,"\n\tVariables:\n");
	while (udv) {
	     len = instring(udv->udv_name, ' ');
		fprintf(stderr,"\t%-*s ",len,udv->udv_name);
		if (udv->udv_undef)
			fputs("is undefined\n",stderr);
		else {
			fputs("= ",stderr);
			disp_value(stderr,&(udv->udv_value));
			(void) putc('\n',stderr);
		}
		udv = udv->next_udv;
	}
}

char *authors[] = {"Thomas Williams","Colin Kelley"}; /* primary */
void				/* used by plot.c */
show_version()
{
extern char version[];
extern char patchlevel[];
extern char date[];
extern char copyright[];
extern char bug_email[];
extern char help_email[];
int x;
long time();

	x = time((long *)NULL) & 1;
	fprintf(stderr,"\n\t%s\n\t%sversion %s\n",
		PROGRAM, OS, version); 
	fprintf(stderr,"\tpatchlevel %s\n",patchlevel);
     fprintf(stderr, "\tlast modified %s\n", date);
	fprintf(stderr,"\n\t%s   %s, %s\n", copyright,authors[x],authors[1-x]);
    fprintf(stderr, "\n\tSend comments and requests for help to %s", help_email);
    fprintf(stderr, "\n\tSend bugs, suggestions and mods to %s\n", bug_email);
}
