#ifndef lint
static char *RCSid = "$Id: show.c,v 1.50 1998/06/18 14:55:18 ddenholm Exp $";
#endif

/* GNUPLOT - show.c */

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
#include "setshow.h"

#define DEF_FORMAT   "%g"	/* default format for tic mark labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */


/* input data, parsing variables */

extern TBOOLEAN is_3d_plot;

/* Used in show_version () */
extern char version[];
extern char patchlevel[];
extern char date[];
extern char gnuplot_copyright[];
extern char faq_location[];
extern char bug_email[];
extern char help_email[];


#ifndef TIMEFMT
#define TIMEFMT "%d/%m/%y,%H:%M"
#endif
/* format for date/time for reading time in datafile */



/******** Local functions ********/

static void show_style __PROTO((char name[], enum PLOT_STYLE style));
static void show_range __PROTO((int axis, double min, double max, int autosc, char *text));
static void show_zero __PROTO((void));
static void show_border __PROTO((void));
static void show_dgrid3d __PROTO((void));
static void show_offsets __PROTO((void));
static void show_samples __PROTO((void));
static void show_isosamples __PROTO((void));
static void show_output __PROTO((void));
static void show_view __PROTO((void));
static void show_size __PROTO((void));
static void show_origin __PROTO((void));
static void show_title __PROTO((void));
static void show_xyzlabel __PROTO((char *name, label_struct *label));
static void show_angles __PROTO((void));
static void show_boxwidth __PROTO((void));
static void show_bars __PROTO((void));
static void show_xzeroaxis __PROTO((void));
static void show_yzeroaxis __PROTO((void));
static void show_label __PROTO((int tag));
static void show_linestyle __PROTO((int tag));
static void show_arrow __PROTO((int tag));
static void show_grid __PROTO((void));
static void show_key __PROTO((void));
static void show_keytitle __PROTO((void));
static void show_mtics __PROTO((int mini, double freq, char *name));
static void show_pointsize __PROTO((void));
static void show_encoding __PROTO((void));
static void show_polar __PROTO((void));
static void show_parametric __PROTO((void));
static void show_tics __PROTO((int showx, int showy, int showz, int showx2, int showy2));
static void show_ticdef __PROTO((int tics, int axis, struct ticdef *tdef, char *text, int rotate_tics));
static void show_term __PROTO((void));
static void show_plot __PROTO((void));
static void show_autoscale __PROTO((void));
static void show_clip __PROTO((void));
static void show_contour __PROTO((void));
static void show_mapping __PROTO((void));
static void show_format __PROTO((void));
static void show_logscale __PROTO((void));
static void show_variables __PROTO((void));
static void show_surface __PROTO((void));
static void show_hidden3d __PROTO((void));
static void show_label_contours __PROTO((void));
static void show_margin __PROTO((void));
static void show_position __PROTO((struct position *pos));

static TBOOLEAN show_one __PROTO((void));
static TBOOLEAN show_two __PROTO((void));
static void show_timefmt __PROTO((void));
static void show_locale __PROTO((void));
static void show_missing __PROTO((void));
static void show_datatype __PROTO((char *name, int axis));
static void show_locale __PROTO((void));

/* following code segment appears over and over again */

#define SHOW_NUM_OR_TIME(x, axis) \
do{if (datatype[axis]==TIME) { \
  char s[80]; char *p; \
  putc('"', stderr);   \
  gstrftime(s,80,timefmt,(double)(x)); \
  for(p=s; *p; ++p) {\
   if ( *p == '\t' ) fprintf(stderr,"\\t");\
   else if (*p == '\n') fprintf(stderr, "\\n"); \
   else if ( *p > 126 || *p < 32 ) fprintf(stderr,"\\%03o",*p);\
   else putc(*p, stderr);\
  }\
  putc('"', stderr);\
 } else {\
  fprintf(stderr,"%g",x);\
}} while(0)


/******* The 'show' command *******/
void
show_command()
{
	/* show at is undocumented/hidden... */
    static char GPFAR showmess[] =
"valid set options:  [] = choose one, {} means optional\n\n\
\t'all',  'angles',  'arrow',  'autoscale',  'bar', 'border',  'boxwidth',\n\
\t'clip',  'contour',  'data',  'dgrid3d',  'dummy',  'encoding',\n\
\t'format', 'function',  'grid',  'hidden',  'isosamples',  'key',\n\
\t'label', 'linestyle', 'locale', 'logscale', 'mapping', 'margin',\n\
\t'missing', 'offsets', 'origin', 'output', 'plot', 'parametric',\n\
\t'pointsize', 'polar', '[rtuv]range', 'samples', 'size', 'terminal',\n\
\t'tics', 'timestamp', 'timefmt', 'title', 'variables', 'version',\n\
\t'view',   '[xyz]{2}label',   '[xyz]{2}range',   '{m}[xyz]{2}tics',\n\
\t'[xyz]{2}[md]tics',   '[xyz]{2}zeroaxis',   '[xyz]data',   'zero',\n\
\t'zeroaxis'";


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
        else if (almost_equals(c_token,"b$ars")){
           (void) putc('\n',stderr);
           show_bars();
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
	else if (almost_equals(c_token,"ma$rgin")) {
		(void) putc('\n',stderr);
		show_margin();
		c_token++;
	}
	else if (almost_equals(c_token,"o$utput")) {
		(void) putc('\n',stderr);
		show_output();
		c_token++;
	}
	else if (almost_equals(c_token,"tit$le")) {
		(void) putc('\n',stderr);
		show_xyzlabel("title", &title);
		c_token++;
	}
	else if (almost_equals(c_token,"mis$sing")) {
		(void) putc('\n',stderr);
		show_missing();
		c_token++;
	}
	else if (almost_equals(c_token,"xl$abel")) {
		(void) putc('\n',stderr);
		show_xyzlabel("xlabel", &xlabel);
		c_token++;
	}
	else if (almost_equals(c_token,"x2l$abel")) {
		(void) putc('\n',stderr);
		show_xyzlabel("x2label", &x2label);
		c_token++;
	}
	else if (almost_equals(c_token,"yl$abel")) {
		(void) putc('\n',stderr);
		show_xyzlabel("ylabel", &ylabel);
		c_token++;
	}
	else if (almost_equals(c_token,"y2l$abel")) {
		(void) putc('\n',stderr);
		show_xyzlabel("y2label", &y2label);
		c_token++;
	}
	else if (almost_equals(c_token,"zl$abel")) {
		(void) putc('\n',stderr);
		show_xyzlabel("zlabel", &zlabel);
		c_token++;
	}
	else if (almost_equals(c_token,"keyt$itle")) {
		(void) putc('\n',stderr);
		show_keytitle();
		c_token++;
	}
	else if (almost_equals(c_token,"xda$ta")) {
		(void) putc('\n',stderr);
		show_datatype("xdata",FIRST_X_AXIS);
		c_token++;
	}
	else if (almost_equals(c_token,"yda$ta")) {
		(void) putc('\n',stderr);
		show_datatype("ydata",FIRST_Y_AXIS);
		c_token++;
	}
	else if (almost_equals(c_token,"x2da$ta")) {
		(void) putc('\n',stderr);
		show_datatype("x2data",SECOND_X_AXIS);
		c_token++;
	}
	else if (almost_equals(c_token,"y2da$ta")) {
		(void) putc('\n',stderr);
		show_datatype("y2data", SECOND_Y_AXIS);
		c_token++;
	}
	else if (almost_equals(c_token,"zda$ta")) {
		(void) putc('\n',stderr);
		show_datatype("zdata",FIRST_Z_AXIS);
		c_token++;
	}
	else if (almost_equals(c_token,"timef$mt")) {
		(void) putc('\n',stderr);
		show_timefmt();
		c_token++;
	}
	else if (almost_equals(c_token,"loca$le")) {
		(void) putc('\n',stderr);
		show_locale();
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
	else if (almost_equals(c_token,"li$nestyle") || equals(c_token, "ls" )) {
	    struct value a;
	    int tag = 0;

	    c_token++;
	    if (!END_OF_COMMAND) {
		   tag = (int)real(const_express(&a));
		   if (tag <= 0)
			int_error("tag must be > zero", c_token);
	    }

	    (void) putc('\n',stderr);
	    show_linestyle(tag);
	}
	else if (almost_equals(c_token,"g$rid")) {
		(void) putc('\n',stderr);
		show_grid();
		c_token++;
	}
        else if (almost_equals(c_token,"mxt$ics")) {
                (void) putc('\n',stderr);
                show_mtics(mxtics, mxtfreq, "x");
                c_token++;
        }
        else if (almost_equals(c_token,"myt$ics")) {
                (void) putc('\n',stderr);
                show_mtics(mytics, mytfreq, "y");
                c_token++;
        }
       else if (almost_equals(c_token,"mzt$ics")) {
                (void) putc('\n',stderr);
                show_mtics(mztics, mztfreq, "z");
                c_token++;
        }
       else if (almost_equals(c_token,"mx2t$ics")) {
                (void) putc('\n',stderr);
                show_mtics(mx2tics, mx2tfreq, "x2");
                c_token++;
        }
       else if (almost_equals(c_token,"my2t$ics")) {
                (void) putc('\n',stderr);
                show_mtics(my2tics, my2tfreq, "y2");
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
	else if (almost_equals(c_token, "poi$ntsize")) {
		(void) putc('\n', stderr);
		show_pointsize();
		c_token++;
	}
	else if (almost_equals(c_token, "enc$oding")) {
		(void) putc('\n', stderr);
		show_encoding();
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
		show_tics(TRUE,TRUE,TRUE,TRUE,TRUE);
		c_token++;
	}
	else if (almost_equals(c_token,"tim$estamp")) {
		(void) putc('\n',stderr);
		show_xyzlabel("time", &timelabel);
                fprintf(stderr, "\twritten in %s corner\n", (timelabel_bottom?"bottom":"top"));
                if(timelabel_rotate)
                    fprintf(stderr, "\trotated if the terminal allows it\n\t");
                else
                    fprintf(stderr, "\tnot rotated\n\t");
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
	    show_tics(TRUE,FALSE,FALSE,TRUE,FALSE);
	    c_token++;
	}
	else if (almost_equals(c_token,"yti$cs")) {
	    show_tics(FALSE,TRUE,FALSE,FALSE,TRUE);
	    c_token++;
	}
	else if (almost_equals(c_token,"zti$cs")) {
	    show_tics(FALSE,FALSE,TRUE,FALSE, FALSE);
	    c_token++;
	}
	else if (almost_equals(c_token,"x2ti$cs")) {
	    show_tics(FALSE,FALSE,FALSE,TRUE,FALSE);
	    c_token++;
	}
	else if (almost_equals(c_token,"y2ti$cs")) {
	    show_tics(FALSE,FALSE,FALSE,FALSE,TRUE);
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
	else if (almost_equals(c_token,"orig$in")) {
		(void) putc('\n',stderr);
		show_origin();
		c_token++;
	}
	else if (almost_equals(c_token,"t$erminal")) {
		(void) putc('\n',stderr);
		show_term();
		c_token++;
	}
	else if (almost_equals(c_token,"rr$ange")) {
		(void) putc('\n',stderr);
		show_range(R_AXIS,rmin,rmax,autoscale_r, "r");
		c_token++;
	}
	else if (almost_equals(c_token,"tr$ange")) {
		(void) putc('\n',stderr);
		show_range(T_AXIS,tmin,tmax,autoscale_t, "t");
		c_token++;
	}
	else if (almost_equals(c_token,"ur$ange")) {
		(void) putc('\n',stderr);
		show_range(U_AXIS,umin,umax,autoscale_u, "u");
		c_token++;
	}
	else if (almost_equals(c_token,"vi$ew")) {
		(void) putc('\n',stderr);
		show_view();
		c_token++;
	}
	else if (almost_equals(c_token,"vr$ange")) {
		(void) putc('\n',stderr);
		show_range(V_AXIS,vmin,vmax,autoscale_v, "v");
		c_token++;
	}
	else if (almost_equals(c_token,"v$ariables")) {
		show_variables();
		c_token++;
	}
	else if (almost_equals(c_token,"ve$rsion")) {
		show_version(stderr);
		c_token++;
		if (almost_equals(c_token,"l$ong")) {
		       show_version_long();
		       c_token++;
		}
	}
	else if (almost_equals(c_token,"xr$ange")) {
		(void) putc('\n',stderr);
		show_range(FIRST_X_AXIS,xmin,xmax,autoscale_x, "x");
		c_token++;
	}
	else if (almost_equals(c_token,"yr$ange")) {
		(void) putc('\n',stderr);
		show_range(FIRST_Y_AXIS,ymin,ymax,autoscale_y, "y");
		c_token++;
	}
	else if (almost_equals(c_token,"x2r$ange")) {
		(void) putc('\n',stderr);
		show_range(SECOND_X_AXIS,x2min,x2max,autoscale_x2, "x2");
		c_token++;
	}
	else if (almost_equals(c_token,"y2r$ange")) {
		(void) putc('\n',stderr);
		show_range(SECOND_Y_AXIS,y2min,y2max,autoscale_y2, "y2");
		c_token++;
	}
	else if (almost_equals(c_token,"zr$ange")) {
		(void) putc('\n',stderr);
		show_range(FIRST_Z_AXIS,zmin,zmax,autoscale_z, "z");
		c_token++;
	}
	else if (almost_equals(c_token,"z$ero")) {
		(void) putc('\n',stderr);
		show_zero();
		c_token++;
	}
	else if (almost_equals(c_token,"a$ll")) {
		c_token++;
		show_version(stderr);
		show_autoscale();
                show_bars();
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
		show_xzeroaxis();
		show_yzeroaxis();
		show_label(0);
		show_arrow(0);
		show_linestyle(0);
		show_keytitle();
		show_key();
		show_logscale();
		show_offsets();
		show_margin();
		show_output();
		show_parametric();
		show_pointsize();
		show_encoding();
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
		show_origin();
		show_term();
		show_tics(TRUE,TRUE,TRUE,TRUE,TRUE);
                show_mtics(mxtics, mxtfreq, "x");
                show_mtics(mytics, mytfreq, "y");
                show_mtics(mztics, mztfreq, "z");
                show_mtics(mx2tics, mx2tfreq, "x2");
                show_mtics(my2tics, my2tfreq, "y2");
		show_xyzlabel("time", &timelabel);
		if (parametric || polar) {
			if (!is_3d_plot)
				show_range(T_AXIS,tmin,tmax,autoscale_t, "t");
			else {
				show_range(U_AXIS,umin,umax,autoscale_u, "u");
				show_range(V_AXIS,vmin,vmax,autoscale_v, "v");
			}
		}
		show_range(FIRST_X_AXIS,xmin,xmax,autoscale_x, "x");
		show_range(FIRST_Y_AXIS,ymin,ymax,autoscale_y, "y");
		show_range(SECOND_X_AXIS,x2min,x2max,autoscale_x2, "x2");
		show_range(SECOND_Y_AXIS,y2min,y2max,autoscale_y2, "y2");
		show_range(FIRST_Z_AXIS,zmin,zmax,autoscale_z, "z");
		show_xyzlabel("title", &title);
		show_xyzlabel("xlabel", &xlabel);
		show_xyzlabel("ylabel", &ylabel);
		show_xyzlabel("zlabel", &zlabel);
		show_xyzlabel("x2label", &x2label);
		show_xyzlabel("y2label", &y2label);
		show_datatype("xdata", FIRST_X_AXIS);
		show_datatype("ydata", FIRST_Y_AXIS);
		show_datatype("x2data", SECOND_X_AXIS);
		show_datatype("y2data", SECOND_Y_AXIS);
		show_datatype("zdata", FIRST_Z_AXIS);
		show_timefmt();
		show_locale();
		show_zero();
		show_missing();
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
/* perhaps these need to be put into a constant array ? */
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
		case YERRORBARS: fprintf(stderr,"yerrorbars\n"); break;
		case XERRORBARS: fprintf(stderr,"xerrorbars\n"); break;
		case XYERRORBARS: fprintf(stderr,"xyerrorbars\n"); break;
		case BOXES: fprintf(stderr,"boxes\n"); break;
		case BOXERROR: fprintf(stderr,"boxerrorbars\n"); break;
		case BOXXYERROR: fprintf(stderr,"boxxyerrorbars\n"); break;
		case STEPS: fprintf(stderr,"steps\n"); break;
		case FSTEPS: fprintf(stderr,"fsteps\n"); break;
		case HISTEPS: fprintf(stderr, "histeps\n"); break;
		case VECTOR: fprintf(stderr,"vector\n"); break;
		case FINANCEBARS: fprintf(stderr, "financebars\n"); break;
		case CANDLESTICKS: fprintf(stderr, "candlesticsks\n"); break;
	}
}

static void
show_bars()
{
  /* I really like this: "terrorbars" ;-) */
  if (bar_size > 0.0)
	fprintf(stderr,"\terrorbars are plotted with bars of size %f\n", bar_size);
  else
   fprintf(stderr, "\terrors are plotted without bars\n");
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
show_range(axis,min,max,autosc,text)
int axis;
double min,max;
TBOOLEAN autosc;
char *text;
{
	/* this probably ought to just invoke save_range(stderr) from misc.c
	 * since I think it is identical
	 */

	if ( datatype[axis] == TIME ) 
		fprintf(stderr,"\tset %sdata time\n",text);
	fprintf(stderr,"\tset %srange [",text);
	if ( autosc & 1 ) {
		fprintf(stderr,"*");
	} else {
		SHOW_NUM_OR_TIME(min, axis);
	}
	fprintf(stderr," : ");
	if ( autosc & 2 ) {
		fprintf(stderr,"*");
	} else {
		SHOW_NUM_OR_TIME(max, axis);
	}
	fprintf(stderr,"] %sreverse %swriteback",
		(range_flags[axis] & RANGE_REVERSE) ? "" : "no",
		(range_flags[axis] & RANGE_WRITEBACK) ? "" : "no");

	if (autosc) {
		/* add current (hidden) range as comments */
		fprintf(stderr, "  # (currently [");
		if (autosc&1) {
			SHOW_NUM_OR_TIME(min, axis);
		}
		putc(':',stderr);
		if (autosc&2) {
			SHOW_NUM_OR_TIME(max,axis);
		}
		fprintf(stderr, "] )\n");
	}
	else
	{
		putc('\n', stderr);
	}
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
	fprintf(stderr,"\tborder is %sdrawn %d\n", draw_border ? "" : "not ",
		draw_border);
	/* HBB 980609: added facilities to specify border linestyle */
	fprintf(stderr, "\tBorder drawn with linetype %d, linewidth %.3f\n", border_lp.l_type+1, border_lp.l_width);
}

static void
show_output()
{
	if (outstr)
		fprintf(stderr,"\toutput is sent to '%s'\n",outstr);
	else
		fprintf(stderr,"\toutput is sent to STDOUT\n");
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
	show_hidden3doptions();
#endif /* LITE */
}

static void
show_label_contours()
{
	if (label_contours)
		fprintf(stderr,"\tcontour line types are varied & labeled with format '%s'\n", contour_format);
	else
		fprintf(stderr,"\tcontour line types are all the same\n");
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
	if (aspect_ratio > 0)
		fprintf(stderr, "\tTry to set aspect ratio to %g:1.0\n", aspect_ratio);
	else if (aspect_ratio == 0)
		fputs("\tNo attempt to control aspect ratio\n", stderr);
	else
		fprintf(stderr, "\tTry to set LOCKED aspect ratio to %g:1.0\n", -aspect_ratio);
}

static void
show_origin()
{
	fprintf(stderr,"\torigin is set to %g,%g\n",xoffset, yoffset);
}

static void
show_xyzlabel(name, label)
char *name;
label_struct *label;
{
	char str[MAX_LINE_LEN+1];
	fprintf(stderr,"\t%s is \"%s\", offset at %f, %f",
		name, conv_text(str,label->text),label->xoffset,label->yoffset);
	if (*label->font)
		fprintf(stderr, ", using font \"%s\"", conv_text(str, label->font));
	putc('\n', stderr);
}

static void
show_keytitle()
{
	char str[MAX_LINE_LEN+1];
	fprintf(stderr,"\tkeytitle is \"%s\"\n",
		conv_text(str,key_title));
}

static void
show_timefmt()
{
	char str[MAX_LINE_LEN+1];
	fprintf(stderr,"\tread format for time is \"%s\"\n",
		conv_text(str,timefmt));
}

static void
show_locale()
{
	fprintf(stderr,"\tlocale is \"%s\"\n", cur_locale);
}

static void
show_xzeroaxis()
{
	if (xzeroaxis.l_type > -3)
		fprintf(stderr,"\txzeroaxis is drawn with linestyle %d, linewidth %.3f\n",xzeroaxis.l_type+1, xzeroaxis.l_width);
	else
		fputs("\txzeroaxis is OFF\n", stderr);
	if (x2zeroaxis.l_type > -3)
		fprintf(stderr,"\tx2zeroaxis is drawn with linestyle %d, linewidth %.3f\n",x2zeroaxis.l_type+1, x2zeroaxis.l_width);
	else
		fputs("\tx2zeroaxis is OFF\n", stderr);
}

static void
show_yzeroaxis()
{
	if (yzeroaxis.l_type > -3)
		fprintf(stderr,"\tyzeroaxis is drawn with linestyle %d, linewidth %.3f\n",yzeroaxis.l_type+1, yzeroaxis.l_width);
	else
		fputs("\tyzeroaxis is OFF\n", stderr);
	if (y2zeroaxis.l_type > -3)
		fprintf(stderr,"\ty2zeroaxis is drawn with linestyle %d, linewidth %.3f\n",y2zeroaxis.l_type+1, y2zeroaxis.l_width);
	else
		fputs("\ty2zeroaxis is OFF\n", stderr);
}

static void
show_label(tag)
    int tag;				/* 0 means show all */
{
    struct text_label *this_label;
    TBOOLEAN showed = FALSE;
    char str[MAX_LINE_LEN+1];

    for (this_label = first_label; this_label != NULL;
	    this_label = this_label->next) {
	   if (tag == 0 || tag == this_label->tag) {
		  showed = TRUE;
		  fprintf(stderr,"\tlabel %d \"%s\" at ",
				this_label->tag, conv_text(str,this_label->text));
		  show_position(&this_label->place);
		  switch(this_label->pos) {
			 case LEFT : {
				fprintf(stderr," left");
				break;
			 }
			 case CENTRE : {
				fprintf(stderr," centre");
				break;
			 }
			 case RIGHT : {
				fprintf(stderr," right");
				break;
			 }
		  }
                  fprintf(stderr," %s ",this_label->rotate?"rotated (if possible)":"not rotated");
		  if ((this_label->font)[0]!='\0')
                         fprintf(stderr," font \"%s\"",this_label->font);
		  /* Entry font added by DJL */
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
		  fprintf(stderr,"\tarrow %d, linetype %d, linewidth %.3f %s\n\t  from ",
				this_arrow->tag, 
				this_arrow->lp_properties.l_type+1,
				this_arrow->lp_properties.l_width,
				this_arrow->head ? "" : " (nohead)");
			show_position(&this_arrow->start);
			fputs(" to ", stderr);
			show_position(&this_arrow->end);
			putc('\n', stderr);
	   }
    }
    if (tag > 0 && !showed)
	 int_error("arrow not found", c_token);
}

static void
show_linestyle(tag)
    int tag;				/* 0 means show all */
{
    struct linestyle_def *this_linestyle;
    TBOOLEAN showed = FALSE;

    for (this_linestyle = first_linestyle; this_linestyle != NULL;
	    this_linestyle = this_linestyle->next) {
	   if (tag == 0 || tag == this_linestyle->tag) {
		  showed = TRUE;
		  fprintf(stderr,"\tlinestyle %d, linetype %d, linewidth %.3f, pointtype %d, pointsize %.3f\n",
				this_linestyle->tag, 
				this_linestyle->lp_properties.l_type+1,
				this_linestyle->lp_properties.l_width,
				this_linestyle->lp_properties.p_type+1,
				this_linestyle->lp_properties.p_size);
	   }
    }
    if (tag > 0 && !showed)
	 int_error("linestyle not found", c_token);
}

static void
show_grid()
{
	if (!work_grid.l_type) {
		fputs("\tgrid is OFF\n", stderr);
		return;
	}

	fprintf(stderr, "\t%s grid drawn at%s%s%s%s%s%s%s%s%s%s tics\n",
	  (polar_grid_angle != 0) ? "Polar" : "Rectangular",
	  work_grid.l_type&GRID_X ? " x" : "",
	  work_grid.l_type&GRID_Y ? " y" : "",
	  work_grid.l_type&GRID_Z ? " z" : "",
	  work_grid.l_type&GRID_X2 ? " x2" : "",
	  work_grid.l_type&GRID_Y2 ? " y2" : "",
	  work_grid.l_type&GRID_MX ? " mx" : "",
	  work_grid.l_type&GRID_MY ? " my" : "",
	  work_grid.l_type&GRID_MZ ? " mz" : "",
	  work_grid.l_type&GRID_MX2 ? " mx2" : "",
	  work_grid.l_type&GRID_MY2 ? " my2" : "");

	fprintf(stderr, "\tMajor grid drawn with linetype %d, linewidth %.3f\n", grid_lp.l_type+1,  grid_lp.l_width);
	fprintf(stderr, "\tMinor grid drawn with linetype %d, linewidth %.3f\n", mgrid_lp.l_type+1, mgrid_lp.l_width);

	if (polar_grid_angle)
		fprintf(stderr, "\tGrid radii drawn every %f %s\n",
		   polar_grid_angle/ang2rad,
		   angles_format==ANGLES_DEGREES ? "degrees" : "radians");
}

static void
show_mtics(minitic, minifreq, name)
int minitic;
double minifreq;
char *name;
{
   switch(minitic) {
      case MINI_OFF:
	fprintf(stderr, "\tminor %stics are off\n", name); break;
      case MINI_DEFAULT:
	fprintf(stderr, "\tminor %stics are computed automatically for log scales\n", name);
	break;
      case MINI_AUTO:
	fprintf(stderr, "\tminor %stics are computed automatically\n", name);
	break;
      case MINI_USER:
	fprintf(stderr,"\tminor %stics are drawn with %d subintervals between major xtic marks\n",name, (int) minifreq);
	break;
      default:
	int_error("Unknown minitic type in show_mtics()", NO_CARET);
   }
}

static void
show_key()
{
	char str[80];
	*str = '\0';
	switch (key) {
		case -1 : 
			if (key_vpos == TUNDER) {
				strcpy(str,"below"); 
			} else if (key_vpos == TTOP ) {
				strcpy(str,"top"); 
			} else {
				strcpy(str,"bottom"); 
			}
			if (key_hpos == TOUT) {
				strcpy(str,"outside (right)"); 
			} else if (key_hpos == TLEFT ) {
				strcat(str," left"); 
			} else {
				strcat(str," right"); 
			}
			if ( key_vpos != TUNDER && key_hpos != TOUT ) {
				strcat(str," corner");
			}
			fprintf(stderr,"\tkey is ON, position: %s\n", str);
			break;
		case 0 :
			fprintf(stderr,"\tkey is OFF\n");
			break;
		case 1 :
			fprintf(stderr,"\tkey is at ");
			show_position(&key_user_pos);
			putc('\n', stderr);
			break;
	}
	if (key) {
		fprintf(stderr, "\tkey is %s justified, %s reversed and ",
		  key_just==JLEFT ? "left" : "right",
		  key_reverse ? "" : "not");
		if (key_box.l_type>=-2)
			fprintf(stderr, "boxed\n\twith linetype %d, linewidth %.3f\n",
                                         key_box.l_type+1, key_box.l_width);
		else
			fprintf(stderr, "not boxed\n");
	        fprintf(stderr,"\tsample length is %g characters\n",key_swidth);
	        fprintf(stderr,"\tvertical spacing is %g characters\n",key_vert_factor);
	        fprintf(stderr,"\twidth adjustment is %g characters\n",key_width_fix);
		fprintf(stderr, "\tkey title is \"%s\"\n", key_title);
	}
}

static void
show_parametric()
{
	fprintf(stderr,"\tparametric is %s\n",(parametric)? "ON" : "OFF");
}

static void
show_pointsize()
{
	fprintf(stderr, "\tpointsize is %g\n", pointsize);
}

static void
show_encoding()
{
	fprintf(stderr,"\tencoding is %s\n",encoding_names[encoding]);
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
show_tics(showx, showy, showz,showx2, showy2)
	TBOOLEAN showx, showy, showz, showx2, showy2;
{
    fprintf(stderr,"\ttics are %s, ",(tic_in)? "IN" : "OUT");
    fprintf(stderr,"\tticslevel is %g\n",ticslevel);
    fprintf(stderr,"\tticscale is %g and miniticscale is %g\n",
      ticscale, miniticscale);

    if (showx)
	 show_ticdef(xtics, FIRST_X_AXIS, &xticdef, "x", rotate_xtics);
    if (showx2)
	 show_ticdef(x2tics, SECOND_X_AXIS, &x2ticdef, "second x", rotate_x2tics);
    if (showy)
	 show_ticdef(ytics, FIRST_Y_AXIS, &yticdef, "y", rotate_ytics);
    if (showy2)
	 show_ticdef(y2tics, SECOND_Y_AXIS, &y2ticdef, "second y", rotate_y2tics);
    if (showz)
	 show_ticdef(ztics, FIRST_Z_AXIS, &zticdef, "z", rotate_ztics);
    screen_ok = FALSE;
}

/* called by show_tics */
static void
show_ticdef(tics, axis, tdef, text, rotate_tics)
	int tics;			/* xtics ytics or ztics */
	int axis;
	struct ticdef *tdef;	/* xticdef yticdef or zticdef */
	char *text;  /* "x", ..., "x2", "y2" */
        int rotate_tics;
{
    register struct ticmark *t;

    fprintf(stderr, "\t%s-axis tic labelling is ", text);
    switch(tics & TICS_MASK) {
	case NO_TICS :        fprintf(stderr, "OFF\n"); return;
	case TICS_ON_AXIS :   fprintf(stderr, "on axis, "); break;
	case TICS_ON_BORDER : fprintf(stderr, "on border, "); break;
    }

    if (tics & TICS_MIRROR)
    	fprintf(stderr, "mirrored on opposite border,\n\t");

    if(rotate_tics)
        fprintf(stderr, "labels are rotated in 2D mode, if the terminal allows it\n\t");
    else
        fprintf(stderr, "labels are not rotated\n\t");

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
		break;
	    }
	   case TIC_SERIES: {
	   	fprintf(stderr, "series");
	   	if (tdef->def.series.start != -VERYLARGE) {
	   		fprintf(stderr, " from ");
	   		SHOW_NUM_OR_TIME(tdef->def.series.start, axis);
	   	}
	   		
			fprintf(stderr, " by %g%s", tdef->def.series.incr, datatype[axis]==TIME ? " secs":"");
		  if (tdef->def.series.end != VERYLARGE) {
		  	fprintf(stderr, " until ");
		  	SHOW_NUM_OR_TIME(tdef->def.series.end, axis);
		  }
		  putc('\n', stderr);
		  break;
	   }
	   case TIC_USER: {
/* this appears to be unused? lh
		  int time;
		  time = (datatype[axis] == TIME);
*/
		  fprintf(stderr, "list (");
		  for (t = tdef->def.user; t != NULL; t=t->next) {
			 if (t->label) {
				char str[MAX_LINE_LEN+1];	
				fprintf(stderr, "\"%s\" ", conv_text(str,t->label));
			 }
			 SHOW_NUM_OR_TIME(t->position, axis);
			 if (t->next)
				fprintf(stderr, ", ");
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
show_margin()
{
	if ( lmargin >= 0 )
		fprintf(stderr,"\tlmargin is set to %d\n", lmargin);
	else
		fprintf(stderr,"\tlmargin is computed automatically\n");
	if ( bmargin >= 0 )
		fprintf(stderr,"\tbmargin is set to %d\n", bmargin);
	else
		fprintf(stderr,"\tbmargin is computed automatically\n");
	if ( rmargin >= 0 )
		fprintf(stderr,"\trmargin is set to %d\n", rmargin);
	else
		fprintf(stderr,"\trmargin is computed automatically\n");
	if ( tmargin >= 0 )
		fprintf(stderr,"\ttmargin is set to %d\n", tmargin);
	else
		fprintf(stderr,"\ttmargin is computed automatically\n");
}

static void
show_term()
{
	if (term)
		fprintf(stderr,"\tterminal type is %s %s\n",
			term->name, term_options);
	else
		fprintf(stderr,"\tterminal type is unknown\n");
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
	if (parametric) {
		if (is_3d_plot) {
			fprintf(stderr,"\tt: %s%s%s, ",
				(autoscale_t)? "ON" : "OFF",
				(autoscale_t == 1)? " (min)":"",
				(autoscale_t == 2)? " (max)":"");
		} else {
			fprintf(stderr,"\tu: %s%s%s, ",
				(autoscale_u)? "ON" : "OFF",
				(autoscale_u == 1)? " (min)":"",
				(autoscale_u == 2)? " (max)":"");
			fprintf(stderr,"v: %s%s%s, ",
				(autoscale_v)? "ON" : "OFF",
				(autoscale_v == 1)? " (min)":"",
				(autoscale_v == 2)? " (max)":"");
		}
	} else fprintf(stderr,"\t");

	if (polar) {
		fprintf(stderr,"r: %s%s%s, ",(autoscale_r)? "ON" : "OFF",
				(autoscale_r == 1)? " (min)":"",
				(autoscale_r == 2)? " (max)":"");
	}
	fprintf(stderr,"x: %s%s%s, ",(autoscale_x)? "ON" : "OFF",
				(autoscale_x == 1)? " (min)":"",
				(autoscale_x == 2)? " (max)":"");
	fprintf(stderr,"y: %s%s%s, ",(autoscale_y)? "ON" : "OFF",
				(autoscale_y == 1)? " (min)":"",
				(autoscale_y == 2)? " (max)":"");
	fprintf(stderr,"z: %s%s%s\n",(autoscale_z)? "ON" : "OFF",
				(autoscale_z == 1)? " (min)":"",
				(autoscale_z == 2)? " (max)":"");
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
			case LEVELS_AUTO:
				fprintf(stderr,"\t\tapprox. %d automatic levels\n", contour_levels);
				break;
			case LEVELS_DISCRETE:
			{	int i;
				fprintf(stderr,"\t\t%d discrete levels at ", contour_levels);
				fprintf(stderr, "%g", levels_list[0]);
				for(i = 1; i < contour_levels; i++)
					fprintf(stderr,",%g ", levels_list[i]);
				fprintf(stderr,"\n");
				break;
			}
			case LEVELS_INCREMENTAL:
				fprintf(stderr,"\t\t%d incremental levels starting at %g, step %g, end %g\n",
					contour_levels, levels_list[0], levels_list[1],
					levels_list[0]+(contour_levels-1)*levels_list[1]);
					/* contour-levels counts both ends */
				break;
		}
		/* fprintf(stderr,"\t\tcontour line types are %s\n", label_contours ? "varied" : "all the same"); */
		show_label_contours();
	}
}

static void
show_format()
{
	char str[5][MAX_LINE_LEN+1];

	fprintf(stderr, "\ttic format is x-axis: \"%s\", y-axis: \"%s\", z-axis: \"%s\", x2-axis: \"%s\", y2-axis: \"%s\"\n",
		conv_text(str[0],xformat), conv_text(str[1],yformat), conv_text(str[2],zformat), conv_text(str[3], x2format), conv_text(str[4], y2format));
}

#define SHOW_LOG(FLAG, BASE, TEXT) \
if (FLAG) fprintf(stderr, "%s %s (base %g)", !count++ ? "\tlogscaling" : " and", TEXT,BASE)

static void
show_logscale()
{
	int count=0;

	SHOW_LOG(is_log_x, base_log_x, "x");
	SHOW_LOG(is_log_y, base_log_y, "y");
	SHOW_LOG(is_log_z, base_log_z, "z");
	SHOW_LOG(is_log_x2, base_log_x2, "x2");
	SHOW_LOG(is_log_y2, base_log_y2, "y2");

	if (count==0)
		fputs("\tno logscaling\n", stderr);
	else if (count==1)
		fputs(" only\n", stderr);
	else
		putc('\n', stderr);
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

/*
 * Used in plot.c and misc.c
 * By adding FILE *fp, we can now use show_version()
 * to save version information to a file.
 */
void
show_version(fp)
FILE *fp;
{
	/* If printed to a file, we prefix everything with
	 * a hash mark to comment out the version information.
	 */
	char prefix[6];		/* "#    " */
	char *p = prefix;

	prefix[0] = '#'; prefix[1] = prefix[2] = prefix[3] = prefix[4] = ' ';
	prefix[5] = NUL;

	if (fp == stderr) {
		/* No hash mark - let p point to the trailing '\0' */
		p += sizeof(prefix) - 1;
	} else {
#ifdef GNUPLOT_BINDIR
# ifdef X11
		fprintf(fp, "#!%s/gnuplot -persist\n#\n", GNUPLOT_BINDIR);
#  else
		fprintf(fp, "#!%s/gnuplot\n#\n", GNUPLOT_BINDIR);
# endif /* not X11 */
#endif /* GNUPLOT_BINDIR */
	}
	fprintf(fp, "%s\n\
%s\t%s\n\
%s\t%sversion %s\n\
%s\tpatchlevel %s\n\
%s\tlast modified %s\n\
%s\n\
%s\t%s\n\
%s\tThomas Williams, Colin Kelley and many others\n\
%s\n\
%s\tType `help` to access the on-line reference manual\n\
%s\tThe gnuplot FAQ is available from\n\
%s\t\t<%s>\n\
%s\n\
%s\tSend comments and requests for help to <%s>\n\
%s\tSend bugs, suggestions and mods to <%s>\n\
%s\n",
		p,			/* empty line */
		p, PROGRAM,
		p, OS, version,
		p, patchlevel,
		p, date,
		p,			/* empty line */
		p, gnuplot_copyright,
		p,			/* authors */
		p,			/* empty line */
		p,			/* Type help */
		p,			/* FAQ is at */
		p, faq_location,
		p,			/* empty line */
		p, help_email,
		p, bug_email,
		p);			/* empty line */
}

void
show_version_long ()
{
	char *helpfile = NULL;
#ifdef HAVE_SYS_UTSNAME_H
	struct utsname uts;

	/* something is fundamentally wrong if this fails ... */
	if (uname (&uts) > -1) {
# ifdef _AIX
		fprintf (stderr,"\nSystem: %s %s.%s", uts.sysname, uts.version, uts.release);
# elif defined (SCO)
		fprintf (stderr,"\nSystem: SCO %s", uts.release);
# else
		fprintf (stderr,"\nSystem: %s %s", uts.sysname, uts.release);
# endif
	}
	else {
		fprintf(stderr,"\n%s\n",OS);
	}

#else /* ! HAVE_SYS_UTSNAME_H */

	fprintf(stderr,"\n%s\n",OS);

#endif /* HAVE_SYS_UTSNAME_H */

	fprintf(stderr,"\nCompile options:\n");

	{
		/* The following code could be a lot simper if
		 * it wasn't for Borland's broken compiler ...
		 */
		const char *readline, *gnu_readline, *libgd, *libpng,
			*linuxvga, *nocwdrc, *x11, *unixplot, *gnugraph;

		readline =
#ifdef READLINE
		"+READLINE  "
#else
		"-READLINE  "
#endif
		, gnu_readline =
#ifdef GNU_READLINE
		"+GNU_READLINE  "
#else
		"-GNU_READLINE  "
#endif
		, libgd =
#ifdef HAVE_LIBGD
		"+LIBGD  "
#else
		"-LIBGD  "
#endif
		, libpng =
#ifdef HAVE_LIBPNG
		"+LIBPNG  "
#else
		"-LIBPNG  "
#endif
		, linuxvga =
#ifdef LINUXVGA
		"+LINUXVGA  "
#else
		""
#endif
		, nocwdrc =
#ifdef NOCWDRC
		"+NOCWDRC  "
#else
		"-NOCWDRC  "
#endif
		, x11 =

#ifdef X11
		"+X11  "
#else
		""
#endif
		, unixplot =
#ifdef UNIXPLOT
		"+UNIXPLOT  "
#else
		""
#endif
		, gnugraph =
#ifdef GNUGRAPH
		"+GNUGRAPH  "
#else
		""
#endif
		;
		fprintf(stderr,"%s%s%s%s%s%s%s%s%s\n\n",readline,gnu_readline,
			libgd,libpng,linuxvga,nocwdrc,x11,unixplot,gnugraph);
	}

	if ((helpfile = getenv ("GNUHELP")) == NULL) {
#if defined(ATARI) || defined(MTOS)
		if ((helpfile = getenv ("GNUPLOTPATH")) == NULL) {
			helpfile = HELPFILE;
		}
#else
		helpfile = HELPFILE;
#endif
	}

	fprintf (stderr,"HELPFILE     = \"%s\"\n\
FAQ location = <%s>\n\
CONTACT      = <%s>\n\
HELPMAIL     = <%s>\n",helpfile,faq_location,bug_email,help_email);
}

static void
show_datatype(name,axis)
char *name;
int axis;
{
	fprintf(stderr,"\t%s is set to %s\n",name, 
		datatype[axis] == TIME ? "time" : "numerical");
}

char *
conv_text(s,t)
char *s, *t;
{
	/* convert unprintable characters as \okt, tab as \t, newline \n .. */
	char *r;
	r = s;
	while (*t != '\0' ) {
		switch ( *t ) {
			case '\t' :
				*s++ = '\\';
				*s++ = 't';
				break;
			case '\n' :
				*s++ = '\\';
				*s++ = 'n';
				break;
#ifndef OSK
			case '\r' :
				*s++ = '\\';
				*s++ = 'r';
				break;
#endif
			case '"' :
			case '\\':
				*s++ = '\\';
				*s++ = *t;
				break;
				
			default: {
				if ( (*t & 0177) > 31 && (*t & 0177) < 127 ) 
					*s++ = *t;
				else {
					*s++ = '\\';
					sprintf(s,"%o",*t);
					while ( *s != '\0' ) s++;
				}
			}
		}
		t++;
	}
	*s = '\0';
	return(r);
}


static void show_position(pos)
struct position *pos;
{
	static char *msg[]={"(first axes) ","(second axes) ", "(graph units) ", "(screen units) "};

	assert(first_axes==0 && second_axes==1 && graph==2 && screen==3);
	
	fprintf(stderr, "(%s%g, %s%g, %s%g)",
		pos->scalex == first_axes  ? "" : msg[pos->scalex], pos->x,
		pos->scaley == pos->scalex ? "" : msg[pos->scaley], pos->y,
		pos->scalez == pos->scaley ? "" : msg[pos->scalez], pos->z);

}

static void
show_missing()
{
	if ( missing_val == NULL )
		fprintf(stderr,"\tNo string is interpreted as missing data\n");
	else
		fprintf(stderr,"\t\"%s\" is interpreted as missing value\n",missing_val);
}
