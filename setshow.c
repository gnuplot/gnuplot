#ifndef lint
static char *RCSid = "$Id: setshow.c,v 3.26 92/03/24 22:34:20 woo Exp Locker: woo $";
#endif

/* GNUPLOT - setshow.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991, 1992   Thomas Williams, Colin Kelley
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
 * Send your comments or suggestions to 
 *  info-gnuplot@ames.arc.nasa.gov.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@ames.arc.nasa.gov.  
 * Send bug reports to
 *  bug-gnuplot@ames.arc.nasa.gov.
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
BOOLEAN			autoscale_r	= TRUE;
BOOLEAN			autoscale_t	= TRUE;
BOOLEAN			autoscale_u	= TRUE;
BOOLEAN			autoscale_v	= TRUE;
BOOLEAN			autoscale_x	= TRUE;
BOOLEAN			autoscale_y	= TRUE;
BOOLEAN			autoscale_z	= TRUE;
BOOLEAN			autoscale_lt	= TRUE;
BOOLEAN			autoscale_lu	= TRUE;
BOOLEAN			autoscale_lv	= TRUE;
BOOLEAN			autoscale_lx	= TRUE;
BOOLEAN			autoscale_ly	= TRUE;
BOOLEAN			autoscale_lz	= TRUE;
BOOLEAN 	  	clip_points	= FALSE;
BOOLEAN 	  	clip_lines1	= TRUE;
BOOLEAN 	  	clip_lines2	= FALSE;
BOOLEAN			draw_border	= TRUE;
BOOLEAN			draw_surface    = TRUE;
BOOLEAN			timedate    = FALSE;
char			dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };
char			xformat[MAX_ID_LEN+1] = DEF_FORMAT;
char			yformat[MAX_ID_LEN+1] = DEF_FORMAT;
char			zformat[MAX_ID_LEN+1] = DEF_FORMAT;
enum			PLOT_STYLE data_style	= POINTS,
			func_style	= LINES;
BOOLEAN			grid		= FALSE;
int				key			= -1;	/* default position */
double			key_x, key_y, key_z;	/* user specified position for key */
BOOLEAN			log_x		= FALSE,
			log_y		= FALSE,
			log_z		= FALSE;
FILE*			outfile;
char			outstr[MAX_ID_LEN+1] = "STDOUT";
BOOLEAN			parametric	= FALSE;
BOOLEAN			polar		= FALSE;
BOOLEAN			hidden3d	= FALSE;
int			angles_format	= ANGLES_RADIANS;
int			mapping3d	= MAP3D_CARTESIAN;
int			samples		= SAMPLES;
int			iso_samples	= ISO_SAMPLES;
float			xsize		= 1.0;  /* scale factor for size */
float			ysize		= 1.0;  /* scale factor for size */
float			zsize		= 1.0;  /* scale factor for size */
float			surface_rot_z   = 30.0; /* Default 3d transform. */
float			surface_rot_x   = 60.0;
float			surface_scale   = 1.0;
float			surface_zscale  = 1.0;
int			term		= 0;		/* unknown term is 0 */
char			term_options[MAX_ID_LEN+1] = "";
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
double			rmin		= -0.0,
			rmax		=  10.0,
			tmin		= -5.0,
			tmax		=  5.0,
			umin		= -5.0,
			umax		= 5.0,
			vmin		= -5.0,
			vmax		= 5.0,
			xmin		= -10.0,
			xmax		= 10.0,
			ymin		= -10.0,
			ymax		= 10.0,
			zmin		= -10.0,
			zmax		= 10.0;
double			loff		= 0.0,
			roff		= 0.0,
			toff		= 0.0,
			boff		= 0.0;
int			draw_contour	= CONTOUR_NONE;
int			contour_pts	= 5;
int			contour_kind	= CONTOUR_KIND_LINEAR;
int			contour_order	= 4;
int			contour_levels	= 5;
double			zero = ZERO;			/* zero threshold, not 0! */

BOOLEAN xzeroaxis = TRUE;
BOOLEAN yzeroaxis = TRUE;

BOOLEAN xtics = TRUE;
BOOLEAN ytics = TRUE;
BOOLEAN ztics = TRUE;

float ticslevel = 0.5;

struct ticdef xticdef = {TIC_COMPUTED};
struct ticdef yticdef = {TIC_COMPUTED};
struct ticdef zticdef = {TIC_COMPUTED};

BOOLEAN			tic_in		= TRUE;

struct text_label *first_label = NULL;
struct arrow_def *first_arrow = NULL;

/*** other things we need *****/
extern char *strcpy(),*strcat();
extern int strlen();
extern FILE *popen();

/* input data, parsing variables */
extern struct lexical_unit token[];
extern char input_line[];
extern int num_tokens, c_token;
extern BOOLEAN interactive;	/* from plot.c */

extern char replot_line[];
extern struct udvt_entry *first_udv;
extern BOOLEAN is_3d_plot;

extern double magnitude(),real();
extern struct value *const_express();

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

static void show_style(), show_range(), show_zero(), show_border();
static void show_offsets(), show_output(), show_samples(), show_isosamples();
static void show_view(), show_size(), show_title(), show_xlabel();
static void show_angles();
static void show_ylabel(), show_zlabel(), show_xzeroaxis(), show_yzeroaxis();
static void show_label(), show_arrow(), show_grid(), show_key();
static void show_polar(), show_parametric(), show_tics(), show_ticdef();
static void show_time(), show_term(), show_plot(), show_autoscale(), show_clip();
static void show_contour(), show_mapping(), show_format(), show_logscale();
static void show_variables(), show_surface(), show_hidden3d();
static void delete_label();
static int assign_label_tag();
static void delete_arrow();
static int assign_arrow_tag();
static BOOLEAN set_one(), set_two(), set_three();
static BOOLEAN show_one(), show_two();

/******** The 'set' command ********/
void
set_command()
{
    c_token++;

    if (!set_one() && !set_two() && !set_three())
	int_error(
	"valid set options:  'angles' '{no}arrow', {no}autoscale', \n\
	'{no}border', '{no}clip', 'cntrparam', '{no}contour', 'data style', \n\
	'dummy', 'format', 'function style', '{no}grid', '{no}hidden3d', \n\
	'isosamples', '{no}key', '{no}label', '{no}logscale', 'mapping', \n\
	'offsets', 'output', '{no}parametric', '{no}polar', 'rrange', \n\
	'samples', 'size', '{no}surface', 'terminal', 'tics', 'ticslevel', \n\
	'{no}time', 'title', 'trange', 'urange', 'view', 'vrange', 'xlabel', \n\
	'xrange', '{no}xtics', '{no}xzeroaxis', 'ylabel', 'yrange', \n\
	'{no}ytics', '{no}yzeroaxis', 'zero', '{no}zeroaxis', 'zlabel', \n\
	'zrange', '{no}ztics'", c_token);
}

/* return TRUE if a command match, FALSE if not */
static BOOLEAN
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
	    hidden3d = TRUE;
	    c_token++;
	}
	else if (almost_equals(c_token,"nohi$dden3d")) {
	    hidden3d = FALSE;
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
		 c_token++;
		 contour_levels = (int) real(const_express(&a));
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
	else if (almost_equals(c_token,"d$ata")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		data_style = get_style();
	}
	else if (almost_equals(c_token,"d$ummy")) {
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
		BOOLEAN setx, sety, setz;
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
		   log_x = log_y = log_z = TRUE;
	    } else {
		   if (chr_in_str(c_token, 'x'))
		       log_x = TRUE;
		   if (chr_in_str(c_token, 'y'))
		       log_y = TRUE;
		   if (chr_in_str(c_token, 'z'))
		       log_z = TRUE;
		   c_token++;
	    }
	}
	else if (almost_equals(c_token,"nolo$gscale")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		log_x = log_y = log_z = FALSE;
	    } else {
		if (chr_in_str(c_token, 'x'))
		    log_x = FALSE;
		if (chr_in_str(c_token, 'y'))
		    log_y = FALSE;
		if (chr_in_str(c_token, 'z'))
		    log_z = FALSE;
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
static BOOLEAN
set_two()
{
     char testfile[MAX_LINE_LEN+1];
#ifdef unix
     static BOOLEAN pipe_open = FALSE;
#endif

	if (almost_equals(c_token,"o$utput")) {
		register FILE *f;

		c_token++;
		if (term && term_init)
			(*term_tbl[term].reset)();
		if (END_OF_COMMAND) {	/* no file specified */
 			UP_redirect (4);
			if (outfile != stdout) { /* Never close stdout */
#ifdef unix
				if ( pipe_open ) {
					(void) pclose(outfile);
					pipe_open = FALSE;
				} else
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
#ifdef unix
			if ( *testfile == '|' ) {
			  if ((f = popen(testfile+1,"w")) == (FILE *)NULL)
			    os_error("cannot create pipe; output not changed",c_token);
			  else
			    pipe_open = TRUE;
			} else
#endif
			  if ((f = fopen(testfile,"w")) == (FILE *)NULL)
			    os_error("cannot open file; output not changed",c_token);
			if (outfile != stdout) /* Never close stdout */
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
	  	   (void) fprintf(stderr,"\n\tdummy variable is t for curves, u/v for surfaces\n");
	    }
	    c_token++;
	}
	else if (almost_equals(c_token,"nopar$ametric")) {
	    if (parametric) {
		   parametric = FALSE;
		   strcpy (dummy_var[0], "x");
		   strcpy (dummy_var[1], "y");
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
static BOOLEAN
set_three()
{
     if (almost_equals(c_token,"sa$mples")) {
		register int tsamp;
		struct value a;

		c_token++;
		tsamp = (int)magnitude(const_express(&a));
		if (tsamp < 2)
			int_error("sampling rate must be > 1; sampling unchanged",
				c_token);
		else {
		        extern struct surface_points *first_3dplot;
			register struct surface_points *f_3dp = first_3dplot;

			first_3dplot = NULL;
			sp_free(f_3dp);

			samples = tsamp;
		}
    }
    else if (almost_equals(c_token,"isosa$mples")) {
		register int tsamp;
		struct value a;

		c_token++;
		tsamp = (int)magnitude(const_express(&a));
		if (tsamp < 2)
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

			iso_samples = tsamp;
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
			int x, y;

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
	     BOOLEAN changed;
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
	     BOOLEAN changed;
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
	     BOOLEAN changed;
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
		BOOLEAN was_comma = TRUE;
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
	     BOOLEAN changed;
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
	     BOOLEAN changed;
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
	     BOOLEAN changed;
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
	     BOOLEAN changed;
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
			int x, y;

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
    BOOLEAN set_text, set_position, set_just;

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
		alloc ( (unsigned int) sizeof(struct text_label), "label");
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
    BOOLEAN set_start, set_end, head = 1;

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
		alloc ( (unsigned int) sizeof(struct arrow_def), "arrow");
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
		ps = POINTS;
	else if (almost_equals(c_token,"linesp$oints"))
		ps = LINESPOINTS;
	else if (almost_equals(c_token,"d$ots"))
		ps = DOTS;
	else if (almost_equals(c_token,"e$rrorbars"))
		ps = ERRORBARS;
	else
		int_error("expecting 'lines', 'points', 'linespoints', 'dots', 'impulses', or 'errorbars'",c_token);
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
	   tic = (struct ticmark *)alloc(sizeof(struct ticmark), (char *)NULL);
	   if (tic == (struct ticmark *)NULL) {
		  free_marklist(list);
		  int_error("out of memory for tic mark", c_token);
	   }

	   /* has a string with it? */
	   if (isstring(c_token)) {
		  quote_str(temp_string,c_token);
		  tic->label = alloc((unsigned int)strlen(temp_string)+1, "tic label");
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


BOOLEAN					/* TRUE if a or b were changed */
load_range(a,b)			/* also used by command.c */
double *a,*b;
{
struct value t;
BOOLEAN changed = FALSE;

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
    c_token++;

    if (!show_one() && !show_two())
	int_error(
	"valid show options:  'action_table', 'all', 'angles', 'arrow', \n\
	'autoscale', 'border', 'clip', 'contour', 'data', 'dummy', 'format', \n\
	'function', 'grid', 'hidden', 'key', 'label', 'logscale', 'mapping', \n\
	'offsets', 'output', 'plot', 'parametric', 'polar', 'rrange', \n\
	'samples', 'isosamples', 'view', 'size', 'terminal', 'tics', \n\
	'ticslevel', 'time', 'title', 'trange', 'urange', 'vrange', \n\
	'variables', 'version', 'xlabel', 'xrange', 'xtics', 'xzeroaxis', \n\
	'ylabel', 'yrange', 'ytics', 'yzeroaxis', 'zlabel', 'zrange', \n\
	'ztics', 'zero', 'zeroaxis'", c_token);
	screen_ok = FALSE;
	(void) putc('\n',stderr);
}

/* return TRUE if a command match, FALSE if not */
static BOOLEAN
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
	else if (almost_equals(c_token,"d$ata")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		(void) putc('\n',stderr);
		show_style("data",data_style);
		c_token++;
	}
	else if (almost_equals(c_token,"d$ummy")) {
	  	(void) fprintf(stderr,"\n\tdummy variables are \"%s\" and \"%s\"\n",
	    				dummy_var[0], dummy_var[1]);
		c_token++;
	}
	else if (almost_equals(c_token,"fo$rmat")) {
		show_format();
		c_token++;
	}
	else if (almost_equals(c_token,"f$unctions")) {
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
static BOOLEAN
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
		show_clip();
		show_contour();
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
		show_hidden3d();
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
		case POINTS: fprintf(stderr,"points\n"); break;
		case IMPULSES: fprintf(stderr,"impulses\n"); break;
		case LINESPOINTS: fprintf(stderr,"linespoints\n"); break;
		case DOTS: fprintf(stderr,"dots\n"); break;
		case ERRORBARS: fprintf(stderr,"errorbars\n"); break;
	}
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
	fprintf(stderr,"\tsampling rate is %d\n",samples);
}

static void
show_isosamples()
{
	fprintf(stderr,"\tiso sampling rate is %d\n",iso_samples);
}

static void
show_surface()
{
	fprintf(stderr,"\tsurface is %sdrawn\n", draw_surface ? "" : "not ");
}

static void
show_hidden3d()
{
	fprintf(stderr,"\thidden surface is %s\n", hidden3d ? "removed" : "drawn");
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
    BOOLEAN showed = FALSE;

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
    BOOLEAN showed = FALSE;

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
	BOOLEAN showx, showy, showz;
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
	BOOLEAN tics;			/* xtics ytics or ztics */
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
	char *str;

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
	char *p;

	if (log_x && log_y && log_z)
		fprintf(stderr,"\tlogscaling all x, y and z axes\n");
	else {
		p = (log_x && log_y)			  /* Look for pairs. */
			 ? "x and y"
			 :  (log_x && log_z)
			         ? "x and z"
			         :  (log_y && log_z)
					 ? "y and z"
					 : NULL;
		if (p != NULL)
			fprintf(stderr,"\tlogscaling both %s axes\n",p);
		else {
			if (log_x)
				fprintf(stderr,"\tlogscaling x axis\n");
			if (log_y)
				fprintf(stderr,"\tlogscaling y axis\n");
			if (log_z)
				fprintf(stderr,"\tlogscaling z axis\n");
			if (!(log_x || log_y || log_z))
				fprintf(stderr,"\tno logscaling\n");
		}
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

void				/* used by plot.c */
show_version()
{
extern char version[];
extern char patchlevel[];
extern char date[];
extern char bug_email[];
static char *authors[] = {"Thomas Williams","Colin Kelley"}; /* primary */
int x;
long time();

	x = time((long *)NULL) & 1;
	fprintf(stderr,"\n\t%s\n\t%sversion %s\n",
		PROGRAM, OS, version); 
	fprintf(stderr,"\tpatchlevel %s\n",patchlevel);
     fprintf(stderr, "\tlast modified %s\n", date);
	fprintf(stderr,"\nCopyright(C) 1986, 1987, 1990, 1991, 1992  %s, %s\n",
		authors[x],authors[1-x]);
    fprintf(stderr, "\n\tSend bugs and comments to %s\n", bug_email);
}
