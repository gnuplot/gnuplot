#ifndef lint
static char *RCSid = "$Id: graphics.c%v 3.50.1.9 1993/08/05 05:38:59 woo Exp $";
#endif


/* GNUPLOT - graphics.c */
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
#include <assert.h>
#if !defined(u3b2)
#include <time.h>
#endif
#include "plot.h"
#include "setshow.h"

#if defined(DJGPP)||defined(sun386)
#define time_t unsigned long
#endif

#ifndef AMIGA_SC_6_1
extern char *strcpy(),*strncpy(),*strcat(),*ctime();
#endif /* !AMIGA_SC_6_1 */
char *tdate;
#ifdef AMIGA_AC_5
time_t dated;
#else
#if defined(apollo) || defined(sequent) || defined(u3b2) || defined(alliant) || defined(sun386)
#include <sys/types.h> /* typedef long time_t; */
#endif
time_t dated; /* ,time(); */
#include <time.h>
#endif

void plot_impulses();
void plot_lines();
void plot_points();
void plot_dots();
void plot_bars();
void plot_boxes();
void edge_intersect();
TBOOLEAN two_edge_intersect();

void plot_steps();			/* JG */
void edge_intersect_steps();     	/* JG */
TBOOLEAN two_edge_intersect_steps();	/* JG */

/* for plotting error bars */
#define ERRORBARTIC (t->h_tic/2) /* half the width of error bar tic mark */

#ifndef max		/* Lattice C has max() in math.h, but shouldn't! */
#define max(a,b) ((a > b) ? a : b)
#endif

#ifndef min
#define min(a,b) ((a < b) ? a : b)
#endif

#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

/* True if a and b have the same sign or zero (positive or negative) */
#define samesign(a,b) ((a) * (b) >= 0)

/* Define the boundary of the plot
 * These are computed at each call to do_plot, and are constant over
 * the period of one do_plot. They actually only change when the term
 * type changes and when the 'set size' factors change. 
 */
static int xleft, xright, ybot, ytop;

/* Boundary and scale factors, in user coordinates */
/* x_min, x_max, y_min, y_max are local to this file and
 * are not the same as variables of the same names in other files
 */
static double x_min, x_max, y_min, y_max;
static double xscale, yscale;

/* And the functions to map from user to terminal coordinates */
#define map_x(x) (int)(xleft+(x-x_min)*xscale+0.5) /* maps floating point x to screen */ 
#define map_y(y) (int)(ybot+(y-y_min)*yscale+0.5)	/* same for y */

/* (DFK) Watch for cancellation error near zero on axes labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
#define NearlyEqual(x,y,tic) (fabs((x)-(y)) < ((tic) * SIGNIF))

/* (DFK) For some reason, the Sun386i compiler screws up with the CheckLog 
 * macro, so I write it as a function on that machine.
 */
#ifndef sun386
/* (DFK) Use 10^x if logscale is in effect, else x */
#define CheckLog(is_log, base_log, x) ((is_log) ? pow(base_log, (x)) : (x))
#else
static double
CheckLog(is_log, base_log, x)
     TBOOLEAN is_log;
     double base_log;
     double x;
{
  if (is_log)
    return(pow(base_log, x));
  else
    return(x);
}
#endif /* sun386 */

double
LogScale(coord, is_log, log_base_log, what, axis)
	double coord;			/* the value */
	TBOOLEAN is_log;			/* is this axis in logscale? */
        double log_base_log;		/* if so, the log of its base */
	char *what;			/* what is the coord for? */
	char *axis;			/* which axis is this for ("x" or "y")? */
{
    if (is_log) {
	   if (coord <= 0.0) {
		  char errbuf[100];		/* place to write error message */
		(void) sprintf(errbuf,"%s has %s coord of %g; must be above 0 for log scale!",
				what, axis, coord);
		  (*term_tbl[term].text)();
		  (void) fflush(outfile);
		  int_error(errbuf, NO_CARET);
	   } else
		return(log(coord)/log_base_log);
    }
    return(coord);
}

/* borders of plotting area */
/* computed once on every call to do_plot */
boundary(scaling)
	TBOOLEAN scaling;		/* TRUE if terminal is doing the scaling */
{
    register struct termentry *t = &term_tbl[term];
    /* luecken@udel.edu modifications 
       sizes the plot according to the presence of labels, title,... */
    if (strlen(ylabel) == 0)
        xleft = (t->h_char)*8;
    else
        xleft = (t->h_char)*10;
    xright = (scaling ? 1 : xsize) * (t->xmax) - (t->h_char)*2 - (t->h_tic);
    if ((strlen(xlabel) != 0) || timedate)
        if ((*t->text_angle)(1))
            ybot = (t->v_char)*5/2 + 1;
        else
            ybot = (t->v_char)*7/2 + 1;	/* allow space for time at bottom */
    else
        ybot = (t->v_char)*3/2 + 1;
    if ( (strlen(title) != 0) || timedate ||
      ((strlen(ylabel) != 0) && ((*t->text_angle)(1) == FALSE)) )
        ytop = (scaling ? 1 : ysize) * (t->ymax) - (t->v_char)*3/2 - 1;
    else
        ytop = (scaling ? 1 : ysize) * (t->ymax) - (t->v_char)/2 - 1;
    (void)(*t->text_angle)(0);
}


double dbl_raise(x,y)
double x;
int y;
{
register int i;
double val;

	val = 1.0;
	for (i=0; i < abs(y); i++)
		val *= x;
	if (y < 0 ) return (1.0/val);
	return(val);
}


double make_tics(tmin,tmax,logscale,base_log)
double tmin,tmax;
TBOOLEAN logscale;
double base_log;
{
register double xr,xnorm,tics,tic,l10;

	xr = fabs(tmin-tmax);
	
	l10 = log10(xr);
	if (logscale) {
		tic = dbl_raise(base_log,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
		if (tic < 1.0)
			tic = 1.0;
	} else {
		xnorm = pow(10.0,l10-(double)((l10 >= 0.0 ) ? (int)l10 : ((int)l10-1)));
		if (xnorm <= 2)
			tics = 0.2;
		else if (xnorm <= 5)
			tics = 0.5;
		else tics = 1.0;	
		tic = tics * dbl_raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
	}
	return(tic);
}


do_plot(plots, pcount, min_x, max_x, min_y, max_y)
struct curve_points *plots;
int pcount;			/* count of plots in linked list */
double min_x, max_x;
double min_y, max_y;
{
register struct termentry *t = &term_tbl[term];
register int curve, xaxis_y, yaxis_x;
register struct curve_points *this_plot;
register double ytic, xtic;
register int xl, yl;
			/* only a Pyramid would have this many registers! */
double xtemp, ytemp;
struct text_label *this_label;
struct arrow_def *this_arrow;
TBOOLEAN scaling;


/* store these in variables global to this file */
/* otherwise, we have to pass them around a lot */
     x_min = min_x;
     x_max = max_x; 
     y_min = min_y;
     y_max = max_y;

	if (polar) {
	    /* will possibly change x_min, x_max, y_min, y_max */
	    polar_xform(plots,pcount);
	}

	if (y_min == VERYLARGE || y_max == -VERYLARGE ||
	    x_min == VERYLARGE || x_max == -VERYLARGE)
		int_error("all points undefined!", NO_CARET);

/*	Apply the desired viewport offsets. */
     if (y_min < y_max) {
	    y_min -= boff;
	    y_max += toff;
	} else {
	    y_max -= boff;
	    y_min += toff;
	}
     if (x_min < x_max) {
	    x_min -= loff;
	    x_max += roff;
	} else {
	    x_max -= loff;
	    x_min += roff;
	}

/* SETUP RANGES, SCALES AND TIC PLACES */
    if (ytics && yticdef.type == TIC_COMPUTED) {
	   ytic = make_tics(y_min,y_max,is_log_y,base_log_y);
    
	   if (autoscale_ly) {
		  if (y_min < y_max) {
			 y_min = ytic * floor(y_min/ytic);       
			 y_max = ytic * ceil(y_max/ytic);
		  }
		  else {			/* reverse axis */
			 y_min = ytic * ceil(y_min/ytic);       
			 y_max = ytic * floor(y_max/ytic);
		  }
	   }
    }

    if (xtics && xticdef.type == TIC_COMPUTED) {
	   xtic = make_tics(x_min,x_max,is_log_x,base_log_x);
	   
	   if (autoscale_lx) {
		  if (x_min < x_max) {
			 x_min = xtic * floor(x_min/xtic);	
			 x_max = xtic * ceil(x_max/xtic);
		  } else {
			 x_min = xtic * ceil(x_min/xtic);
			 x_max = xtic * floor(x_max/xtic);	
		  }
	   }
    }

/*	This used be x_max == x_min, but that caused an infinite loop once. */
	if (fabs(x_max - x_min) < zero)
		int_error("x_min should not equal x_max!",NO_CARET);
	if (fabs(y_max - y_min) < zero)
		int_error("y_min should not equal y_max!",NO_CARET);

/* INITIALIZE TERMINAL */
	if (!term_init) {
		(*t->init)();
		term_init = TRUE;
	}
	screen_ok = FALSE;
	scaling = (*t->scale)(xsize, ysize);
	(*t->graphics)();

     /* now compute boundary for plot (xleft, xright, ytop, ybot) */
     boundary(scaling);

/* SCALE FACTORS */
	yscale = (ytop - ybot)/(y_max - y_min);
	xscale = (xright - xleft)/(x_max - x_min);
	
/* DRAW AXES */
	(*t->linetype)(-1);	/* axis line type */
	xaxis_y = map_y(0.0);
	yaxis_x = map_x(0.0); 

	if (xaxis_y < ybot)
		xaxis_y = ybot;				/* save for impulse plotting */
	else if (xaxis_y >= ytop)
		xaxis_y = ytop ;
	else if (xzeroaxis && !is_log_y) {
		(*t->move)(xleft,xaxis_y);
		(*t->vector)(xright,xaxis_y);
	} else if (is_log_y){
		xaxis_y = ybot;
	}

	if (yzeroaxis && !is_log_x && yaxis_x >= xleft && yaxis_x < xright ) {
		(*t->move)(yaxis_x,ybot);
		(*t->vector)(yaxis_x,ytop);
	}

/* DRAW TICS */
	(*t->linetype)(-2); /* border linetype */

    /* label y axis tics */
     if (ytics) {
	    switch (yticdef.type) {
		   case TIC_COMPUTED: {
 			  if (y_min < y_max)
			    draw_ytics(ytic * floor(y_min/ytic),
						ytic,
						ytic * ceil(y_max/ytic));
			  else
			    draw_ytics(ytic * floor(y_max/ytic),
						ytic,
						ytic * ceil(y_min/ytic));

			  break;
		   }
		    case TIC_MONTH:{
			draw_month_ytics();
			break;
		    }
		    case TIC_DAY: {
			draw_day_ytics();
			break;
		    }
		   case TIC_SERIES: {
			  draw_series_ytics(yticdef.def.series.start, 
							yticdef.def.series.incr, 
							yticdef.def.series.end);
			  break;
		   }
		   case TIC_USER: {
			  draw_set_ytics(yticdef.def.user);
			  break;
		   }
		   default: {
			  (*t->text)();
	  		  (void) fflush(outfile);
			  int_error("unknown tic type in yticdef in do_plot", NO_CARET);
			  break;		/* NOTREACHED */
		   }
	    }
	}

    /* label x axis tics */
     if (xtics) {
	    switch (xticdef.type) {
		   case TIC_COMPUTED: {
 			  if (x_min < x_max)
			    draw_xtics(xtic * floor(x_min/xtic),
						xtic,
						xtic * ceil(x_max/xtic));
			  else
			    draw_xtics(xtic * floor(x_max/xtic),
						xtic,
						xtic * ceil(x_min/xtic));

			  break;
		   }
		    case TIC_MONTH: {
			draw_month_xtics();
			break;
		    }
		    case TIC_DAY : {
			draw_day_xtics();
			break;
		    }
		   case TIC_SERIES: {
			  draw_series_xtics(xticdef.def.series.start, 
							xticdef.def.series.incr, 
							xticdef.def.series.end);
			  break;
		   }
		   case TIC_USER: {
			  draw_set_xtics(xticdef.def.user);
			  break;
		   }
		   default: {
			  (*t->text)();
			  (void) fflush(outfile);
			  int_error("unknown tic type in xticdef in do_plot", NO_CARET);
			  break;		/* NOTREACHED */
		   }
	    }
	}

/* DRAW PLOT BORDER */
	(*t->linetype)(-2); /* border linetype */
	if (draw_border) {
		(*t->move)(xleft,ybot);
		(*t->vector)(xright,ybot);
		(*t->vector)(xright,ytop);
		(*t->vector)(xleft,ytop);
		(*t->vector)(xleft,ybot);
	}

/* PLACE YLABEL */
	if (strlen(ylabel) > 0) {
		int x, y;

		x = ylabel_xoffset * t->h_char;
		y = ylabel_yoffset * t->v_char;
		if ((*t->text_angle)(1)) {
			if ((*t->justify_text)(CENTRE)) {
				(*t->put_text)(x+(t->v_char),
						 y+(ytop+ybot)/2, ylabel);
			}
			else {
				(*t->put_text)(x+(t->v_char),
					       y+(ytop+ybot)/2-(t->h_char)*strlen(ylabel)/2, 
						 ylabel);
			}
		}
		else {
			(void)(*t->justify_text)(LEFT);
			(*t->put_text)(x,y+ytop+(t->v_char), ylabel);
		}
		(void)(*t->text_angle)(0);
	}

/* PLACE XLABEL */
    if (strlen(xlabel) > 0) {
		int x, y;

		x = xlabel_xoffset * t->h_char;
		y = xlabel_yoffset * t->v_char;

    		if ((*t->justify_text)(CENTRE)) 
			(*t->put_text)(x+(xleft+xright)/2,
				       y+ybot-2*(t->v_char), xlabel);
    		else
			(*t->put_text)(x+(xleft+xright)/2 - strlen(xlabel)*(t->h_char)/2,
    				       y+ybot-2*(t->v_char), xlabel);
    }

/* PLACE TITLE */
    if (strlen(title) > 0) {
		int x, y;

		x = title_xoffset * t->h_char;
		y = title_yoffset * t->v_char;

    		if ((*t->justify_text)(CENTRE))
			(*t->put_text)(x+(xleft+xright)/2,
				       y+ytop+(t->v_char), title);
    		else
			(*t->put_text)(x+(xleft+xright)/2 - strlen(title)*(t->h_char)/2,
				       y+ytop+(t->v_char), title);
    }


/* PLACE TIMEDATE */
	if (timedate) {
		int x, y;

		x = time_xoffset * t->h_char;
		y = time_yoffset * t->v_char;
		dated = time( (time_t *) 0);
		tdate = ctime( &dated);
		tdate[24]='\0';
		(void)(*t->justify_text)(LEFT);
		if ((*t->text_angle)(1)) {
			(void)(*t->text_angle)(0);
			(*t->put_text)(x, y+ytop+(t->v_char), tdate);
		}
		else {
			(void)(*t->text_angle)(0);
			(*t->put_text)(x,
						 y+ybot-3*(t->v_char), tdate);
		}
	}

/* PLACE LABELS */
    for (this_label = first_label; this_label!=NULL;
			this_label=this_label->next ) {
	     xtemp = LogScale(this_label->x, is_log_x, log_base_log_x, "label", "x");
	     ytemp = LogScale(this_label->y, is_log_y, log_base_log_y, "label", "y");
		if ((*t->justify_text)(this_label->pos)) {
			(*t->put_text)(map_x(xtemp),map_y(ytemp),this_label->text);
		}
		else {
			switch(this_label->pos) {
				case  LEFT:
					(*t->put_text)(map_x(xtemp),map_y(ytemp),
						this_label->text);
					break;
				case CENTRE:
					(*t->put_text)(map_x(xtemp)-
						(t->h_char)*strlen(this_label->text)/2,
						map_y(ytemp), this_label->text);
					break;
				case RIGHT:
					(*t->put_text)(map_x(xtemp)-
						(t->h_char)*strlen(this_label->text),
						map_y(ytemp), this_label->text);
					break;
			}
		 }
	 }

/* PLACE ARROWS */
    (*t->linetype)(0);	/* arrow line type */
    for (this_arrow = first_arrow; this_arrow!=NULL;
	    this_arrow = this_arrow->next ) {
	   int sx = map_x(LogScale(this_arrow->sx, is_log_x, log_base_log_x, "arrow", "x"));
	   int sy = map_y(LogScale(this_arrow->sy, is_log_y, log_base_log_y, "arrow", "y"));
	   int ex = map_x(LogScale(this_arrow->ex, is_log_x, log_base_log_x, "arrow", "x"));
	   int ey = map_y(LogScale(this_arrow->ey, is_log_y, log_base_log_y, "arrow", "y"));
	   
	   (*t->arrow)(sx, sy, ex, ey, this_arrow->head);
    }


/* DRAW CURVES */
	if (key == -1) {
	    xl = xright  - (t->h_tic) - (t->h_char)*5;
	    yl = ytop - (t->v_tic) - (t->v_char);
	}
	if (key == 1) {
	    xl = map_x( LogScale(key_x, is_log_x, log_base_log_x, "key", "x") );
	    yl = map_y( LogScale(key_y, is_log_y, log_base_log_y, "key", "y") );
	}

	this_plot = plots;
	for (curve = 0; curve < pcount; this_plot = this_plot->next_cp, curve++) {
		int oldkey = key;

		(*t->linetype)(this_plot->line_type);

		if (this_plot->title && !*this_plot->title) {
		    key = 0;
		} else {
		if (key != 0 && this_plot->title) {
			if ((*t->justify_text)(RIGHT)) {
				(*t->put_text)(xl,
					yl,this_plot->title);
			}
			else {
			    if (inrange(xl-(t->h_char)*strlen(this_plot->title), 
						 xleft, xright))
				 (*t->put_text)(xl-(t->h_char)*strlen(this_plot->title),
							 yl,this_plot->title);
			}
		}
		}

		switch(this_plot->plot_style) {
		    case IMPULSES: {
			   if (key != 0 && this_plot->title) {
				  (*t->move)(xl+(t->h_char),yl);
				  (*t->vector)(xl+4*(t->h_char),yl);
			   }
			   plot_impulses(this_plot, yaxis_x, xaxis_y);
			   break;
		    }
		    case LINES: {
			   if (key != 0 && this_plot->title) {
				  (*t->move)(xl+(int)(t->h_char),yl);
				  (*t->vector)(xl+(int)(4*(t->h_char)),yl);
			   }
			   plot_lines(this_plot);
			   break;
		    }
/* JG */	    case STEPS: {
			   if (key != 0 && this_plot->title) {
				  (*t->move)(xl+(int)(t->h_char),yl);
				  (*t->vector)(xl+(int)(4*(t->h_char)),yl);
			   }
			   plot_steps(this_plot);
			   break;
		    }
		    case POINTSTYLE: {
			   if (key != 0 && this_plot->title) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot_points(this_plot);
			   break;
		    }
		    case LINESPOINTS: {
			   /* put lines */
			   if (key != 0 && this_plot->title) {
				  (*t->move)(xl+(t->h_char),yl);
				  (*t->vector)(xl+4*(t->h_char),yl);
			   }
			   plot_lines(this_plot);

			   /* put points */
			   if (key != 0 && this_plot->title) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot_points(this_plot);
			   break;
		    }
		    case DOTS: {
			   if (key != 0 && this_plot->title) {
				  (*t->point)(xl+2*(t->h_char),yl, -1);
			   }
			   plot_dots(this_plot);
			   break;
		    }
		    case ERRORBARS: {
			   if (key != 0 && this_plot->title) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot_points(this_plot);

			   /* for functions, just like POINTSTYLE */
			   if (this_plot->plot_type == DATA) {
				  if (key != 0 && this_plot->title) {
					 (*t->move)(xl+(t->h_char),yl);
					 (*t->vector)(xl+4*(t->h_char),yl);
					 (*t->move)(xl+(t->h_char),yl+ERRORBARTIC);
					 (*t->vector)(xl+(t->h_char),yl-ERRORBARTIC);
					 (*t->move)(xl+4*(t->h_char),yl+ERRORBARTIC);
					 (*t->vector)(xl+4*(t->h_char),yl-ERRORBARTIC);
				  }
				  plot_bars(this_plot);
			   }
			   break;
		    }
			case BOXERROR: {
			   if (this_plot->plot_type == DATA) {
				  if (key != 0 && this_plot->title) {
					 (*t->move)(xl+(t->h_char),yl+ERRORBARTIC);
					 (*t->vector)(xl+(t->h_char),yl-ERRORBARTIC);
					 (*t->move)(xl+4*(t->h_char),yl+ERRORBARTIC);
					 (*t->vector)(xl+4*(t->h_char),yl-ERRORBARTIC);
				  }
				  plot_bars(this_plot);
			   }
			}
			/* no break */
		    case BOXES: {
			   if (key != 0 && this_plot->title) {
				  (*t->move)(xl+(t->h_char),yl);
				  (*t->vector)(xl+4*(t->h_char),yl);
			   }
			   plot_boxes(this_plot,xaxis_y);
			   break;
		    }

		}
		if (key && this_plot->title) {
		    yl = yl - (t->v_char);
		}
		key = oldkey;
	}
	(*t->text)();
	(void) fflush(outfile);
}

/* plot_impulses:
 * Plot the curves in IMPULSES style
 */
void
plot_impulses(plot, yaxis_x, xaxis_y)
	struct curve_points *plot;
	int yaxis_x, xaxis_y;
{
    int i;
    int x,y;
    struct termentry *t = &term_tbl[term];

    for (i = 0; i < plot->p_count; i++) {
	   switch (plot->points[i].type) {
		  case INRANGE: {
			 x = map_x(plot->points[i].x);
			 y = map_y(plot->points[i].y);
			 break;
		  }
		  case OUTRANGE: {
			 if (!inrange(plot->points[i].x, x_min,x_max))
			   continue;
			 x = map_x(plot->points[i].x);
			 if ((y_min < y_max 
				 && plot->points[i].y < y_min)
				|| (y_max < y_min 
				    && plot->points[i].y > y_min))
			   y = map_y(y_min);
			 if ((y_min < y_max 
				 && plot->points[i].y > y_max)
				|| (y_max<y_min 
				    && plot->points[i].y < y_max))
			   y = map_y(y_max);
			 break;
		  }
		  default:		/* just a safety */
		  case UNDEFINED: {
			 continue;
		  }
	   }
				    
	   if (polar)
	      (*t->move)(yaxis_x,xaxis_y);
	   else
	      (*t->move)(x,xaxis_y);
	   (*t->vector)(x,y);
    }

}

/* plot_lines:
 * Plot the curves in LINES style
 */
void
plot_lines(plot)
	struct curve_points *plot;
{
    int i;				/* point index */
    int x,y;				/* point in terminal coordinates */
    struct termentry *t = &term_tbl[term];
    enum coord_type prev = UNDEFINED; /* type of previous point */
    double ex, ey;			/* an edge point */
    double lx[2], ly[2];		/* two edge points */

    for (i = 0; i < plot->p_count; i++) {
	   switch (plot->points[i].type) {
		  case INRANGE: {
			 x = map_x(plot->points[i].x);
			 y = map_y(plot->points[i].y);

			 if (prev == INRANGE) {
				(*t->vector)(x,y);
			 } else if (prev == OUTRANGE) {
				/* from outrange to inrange */
				if (!clip_lines1) {
				    (*t->move)(x,y);
				} else {
				    edge_intersect(plot->points, i, &ex, &ey);
				    (*t->move)(map_x(ex), map_y(ey));
				    (*t->vector)(x,y);
				}
			 } else {		/* prev == UNDEFINED */
				(*t->move)(x,y);
				(*t->vector)(x,y);
			 }
				    
			 break;
		  }
		  case OUTRANGE: {
			 if (prev == INRANGE) {
				/* from inrange to outrange */
				if (clip_lines1) {
				    edge_intersect(plot->points, i, &ex, &ey);
				    (*t->vector)(map_x(ex), map_y(ey));
				}
			 } else if (prev == OUTRANGE) {
				/* from outrange to outrange */
				if (clip_lines2) {
				    if (two_edge_intersect(plot->points, i, lx, ly)) {
					   (*t->move)(map_x(lx[0]), map_y(ly[0]));
					   (*t->vector)(map_x(lx[1]), map_y(ly[1]));
				    }
				}
			 }
			 break;
		  }
		  default:		/* just a safety */
		  case UNDEFINED: {
			 break;
		  }
	   }
	   prev = plot->points[i].type;
    }
}

/* XXX - JG  */
/* plot_steps:				
 * Plot the curves in STEPS style
 */
void
plot_steps(plot)
struct curve_points *plot;
{
    int i;				/* point index */
    int x,y;				/* point in terminal coordinates */
    struct termentry *t = &term_tbl[term];
    enum coord_type prev = UNDEFINED;	/* type of previous point */
    double ex, ey;			/* an edge point */
    double lx[2], ly[2];		/* two edge points */
    int xprev, yprev;			/* previous point coordinates */

    for (i = 0; i < plot->p_count; i++) {
	   switch (plot->points[i].type) {
		  case INRANGE: {
			 x = map_x(plot->points[i].x);
			 y = map_y(plot->points[i].y);

			 if (prev == INRANGE) {
				(*t->vector)(x,yprev);
				(*t->vector)(x,y);
			 } else if (prev == OUTRANGE) {
				/* from outrange to inrange */
				if (!clip_lines1) {
				    (*t->move)(x,y);
				} else {		/* find edge intersection */
				    edge_intersect_steps(plot->points, i, &ex, &ey);
				    (*t->move)(map_x(ex), map_y(ey));
				    (*t->vector)(x,map_y(ey));
				    (*t->vector)(x,y);
				}
			 } else {		/* prev == UNDEFINED */
				(*t->move)(x,y);
				(*t->vector)(x,y);
			 }
			 break;
		  }
		  case OUTRANGE: {
			 if (prev == INRANGE) {
				/* from inrange to outrange */
				if (clip_lines1) {
				    edge_intersect_steps(plot->points, i, &ex, &ey);
				    (*t->vector)(map_x(ex), yprev);
				    (*t->vector)(map_x(ex), map_y(ey));
				}
			 } else if (prev == OUTRANGE) {
				/* from outrange to outrange */
				if (clip_lines2) {
				    if (two_edge_intersect_steps(plot->points, i, lx, ly)) {
					   (*t->move)(map_x(lx[0]), map_y(ly[0]));
					   (*t->vector)(map_x(lx[1]), map_y(ly[0]));
					   (*t->vector)(map_x(lx[1]), map_y(ly[1]));
				    }
				}
			 }
			 break;
		  }
		  default:		/* just a safety */
		  case UNDEFINED: {
			 break;
		  }
	   }
	   prev  = plot->points[i].type;
	   xprev = x;
	   yprev = y;
    }
}

/* plot_bars:
 * Plot the curves in ERRORBARS style
 *  we just plot the bars; the points are plotted in plot_points
 */
void
plot_bars(plot)
	struct curve_points *plot;
{
    int i;				/* point index */
    struct termentry *t = &term_tbl[term];
    double x;				/* position of the bar */
    double ylow, yhigh;		/* the ends of the bars */
    unsigned int xM, ylowM, yhighM; /* the mapped version of above */
    TBOOLEAN low_inrange, high_inrange;
    int tic = ERRORBARTIC;
    
    for (i = 0; i < plot->p_count; i++) {
	   /* undefined points don't count */
	   if (plot->points[i].type == UNDEFINED)
		continue;

	   /* check to see if in xrange */
	   x = plot->points[i].x;
	   if (! inrange(x, x_min, x_max))
		continue;
	   xM = map_x(x);

	   /* find low and high points of bar, and check yrange */
	   yhigh = plot->points[i].yhigh;
	   ylow = plot->points[i].ylow;

	   high_inrange = inrange(yhigh, y_min,y_max);
	   low_inrange = inrange(ylow, y_min,y_max);

	   /* compute the plot position of yhigh */
	   if (high_inrange)
		yhighM = map_y(yhigh);
	   else if (samesign(yhigh-y_max, y_max-y_min))
		yhighM = map_y(y_max);
	   else
		yhighM = map_y(y_min);
	   
	   /* compute the plot position of ylow */
	   if (low_inrange)
		ylowM = map_y(ylow);
	   else if (samesign(ylow-y_max, y_max-y_min))
		ylowM = map_y(y_max);
	   else
		ylowM = map_y(y_min);

	   if (!high_inrange && !low_inrange && ylowM == yhighM)
		/* both out of range on the same side */
		  continue;

	   /* by here everything has been mapped */
	   (*t->move)(xM, ylowM);
	   (*t->vector)(xM, yhighM); /* draw the main bar */
	   (*t->move)(xM-tic, ylowM); /* draw the bottom tic */
	   (*t->vector)(xM+tic, ylowM);
	   (*t->move)(xM-tic, yhighM); /* draw the top tic */
	   (*t->vector)(xM+tic, yhighM);
    }
}

/* plot_boxes:
 * Plot the curves in BOXES style
 */
void
plot_boxes(plot,xaxis_y)
	struct curve_points *plot;
	int xaxis_y;
{
    int i;				/* point index */
    int xl,xr,yt;			/* point in terminal coordinates */
	double dxl,dxr,dyt;
    struct termentry *t = &term_tbl[term];
    enum coord_type prev = UNDEFINED; /* type of previous point */

    for (i = 0; i < plot->p_count; i++) {
	   switch (plot->points[i].type) {
		  case OUTRANGE:
		  case INRANGE: {
			if (plot->points[i].z<0.0) {
			   if (boxwidth<0.0) {
					/* calculate width */
					if (prev!=UNDEFINED)
						dxl = (plot->points[i-1].x - plot->points[i].x)/2.0;
					else
						dxl = 0.0;
					if (i < plot->p_count-1) {
						if (plot->points[i+1].type!=UNDEFINED)
							dxr = (plot->points[i+1].x - plot->points[i].x)/2.0;
						else
							dxr = -dxl;
					}
					else {
						dxr = -dxl;
					}
					if (prev==UNDEFINED)
						dxl = -dxr;
				}
				else {
					dxr = boxwidth/2.0;
					dxl = -dxr;
				}
			}
			else {
				dxr = plot->points[i].z/2.0;
				dxl = -dxr;
			}

			dxl= plot->points[i].x+dxl;
			dxr= plot->points[i].x+dxr;
			dyt= plot->points[i].y;

			/* clip to border */
			if ((y_min < y_max  && dyt < y_min)
				|| (y_max < y_min  && dyt > y_min))
			   dyt = y_min;
			if ((y_min < y_max  && dyt > y_max)
				|| (y_max<y_min  && dyt < y_max))
			   dyt = y_max;
			if ((x_min < x_max  && dxr < x_min)
				|| (x_max < x_min  && dxr > x_min))
			   dxr = x_min;
			if ((x_min < x_max  && dxr > x_max)
				|| (x_max<x_min  && dxr < x_max))
			   dxr = x_max;
			if ((x_min < x_max  && dxl < x_min)
				|| (x_max < x_min  && dxl > x_min))
			   dxl = x_min;
			if ((x_min < x_max  && dxl > x_max)
				|| (x_max<x_min  && dxl < x_max))
			   dxl = x_max;

			xl= map_x(dxl);
			xr= map_x(dxr);
			yt = map_y(dyt);

			(*t->move)(xl,xaxis_y);
			(*t->vector)(xl,yt);
			(*t->vector)(xr,yt);
			(*t->vector)(xr,xaxis_y);
			(*t->vector)(xl,xaxis_y);
			break;
		  }
		  default:		/* just a safety */
		  case UNDEFINED: {
			 break;
		  }
	   }
	   prev = plot->points[i].type;
    }
}

/* plot_points:
 * Plot the curves in POINTSTYLE style
 */
void
plot_points(plot)
	struct curve_points *plot;
{
    int i;
    int x,y;
    struct termentry *t = &term_tbl[term];

    for (i = 0; i < plot->p_count; i++) {
	   if (plot->points[i].type == INRANGE) {
		  x = map_x(plot->points[i].x);
		  y = map_y(plot->points[i].y);
		  /* do clipping if necessary */
		  if (!clip_points ||
			 (   x >= xleft + t->h_tic  && y >= ybot + t->v_tic 
			  && x <= xright - t->h_tic && y <= ytop - t->v_tic))
		    (*t->point)(x,y, plot->point_type);
	   }
    }
}

/* plot_dots:
 * Plot the curves in DOTS style
 */
void
plot_dots(plot)
	struct curve_points *plot;
{
    int i;
    int x,y;
    struct termentry *t = &term_tbl[term];

    for (i = 0; i < plot->p_count; i++) {
	   if (plot->points[i].type == INRANGE) {
		  x = map_x(plot->points[i].x);
		  y = map_y(plot->points[i].y);
		  /* point type -1 is a dot */
		  (*t->point)(x,y, -1);
	   }
    }
}

/* single edge intersection algorithm */
/* Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segment defined 
 * by the two points.
 */
void
edge_intersect(points, i, ex, ey)
	struct coordinate GPHUGE *points; /* the points array */
	int i;				/* line segment from point i-1 to point i */
	double *ex, *ey;		/* the point where it crosses an edge */
{
    /* global x_min, x_max, y_min, x_max */
    double ax = points[i-1].x;
    double ay = points[i-1].y;
    double bx = points[i].x;
    double by = points[i].y;
    double x, y;			/* possible intersection point */

    if (by == ay) {
	   /* horizontal line */
	   /* assume inrange(by, y_min, y_max) */
	   *ey = by;		/* == ay */

	   if (inrange(x_max, ax, bx))
		*ex = x_max;
	   else if (inrange(x_min, ax, bx))
		*ex = x_min;
	   else {
		(*term_tbl[term].text)();
	    (void) fflush(outfile);
		int_error("error in edge_intersect", NO_CARET);
	   }
	   return;
    } else if (bx == ax) {
	   /* vertical line */
	   /* assume inrange(bx, x_min, x_max) */
	   *ex = bx;		/* == ax */

	   if (inrange(y_max, ay, by))
		*ey = y_max;
	   else if (inrange(y_min, ay, by))
		*ey = y_min;
	   else {
		(*term_tbl[term].text)();
	    (void) fflush(outfile);
		int_error("error in edge_intersect", NO_CARET);
	   }
	   return;
    }

    /* slanted line of some kind */

    /* does it intersect y_min edge */
    if (inrange(y_min, ay, by) && y_min != ay && y_min != by) {
	   x = ax + (y_min-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, x_min, x_max)) {
		  *ex = x;
		  *ey = y_min;
		  return;			/* yes */
	   }
    }
    
    /* does it intersect y_max edge */
    if (inrange(y_max, ay, by) && y_max != ay && y_max != by) {
	   x = ax + (y_max-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, x_min, x_max)) {
		  *ex = x;
		  *ey = y_max;
		  return;			/* yes */
	   }
    }

    /* does it intersect x_min edge */
    if (inrange(x_min, ax, bx) && x_min != ax && x_min != bx) {
	   y = ay + (x_min-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, y_min, y_max)) {
		  *ex = x_min;
		  *ey = y;
		  return;
	   }
    }

    /* does it intersect x_max edge */
    if (inrange(x_max, ax, bx) && x_max != ax && x_max != bx) {
	   y = ay + (x_max-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, y_min, y_max)) {
		  *ex = x_max;
		  *ey = y;
		  return;
	   }
    }

    /* It is possible for one or two of the [ab][xy] values to be -VERYLARGE.
	* If ax=bx=-VERYLARGE or ay=by=-VERYLARGE we have already returned 
	* FALSE above. Otherwise we fall through all the tests above. 
	* If two are -VERYLARGE, it is ax=ay=-VERYLARGE or bx=by=-VERYLARGE 
	* since either a or b must be INRANGE. 
	* Note that for ax=ay=-VERYLARGE or bx=by=-VERYLARGE we can do nothing.
	* Handle them carefully here. As yet we have no way for them to be 
	* +VERYLARGE.
	*/
    if (ax == -VERYLARGE) {
	   if (ay != -VERYLARGE) {
		  *ex = min(x_min, x_max);
		  *ey = by;
		  return;
	   }
    } else if (bx == -VERYLARGE) {
	   if (by != -VERYLARGE) {
		  *ex = min(x_min, x_max);
		  *ey = ay;
		  return;
	   }
    } else if (ay == -VERYLARGE) {
	   /* note we know ax != -VERYLARGE */
	   *ex = bx;
	   *ey = min(y_min, y_max);
	   return;
    } else if (by == -VERYLARGE) {
	   /* note we know bx != -VERYLARGE */
	   *ex = ax;
	   *ey = min(y_min, y_max);
	   return;
    }

    /* If we reach here, then either one point is (-VERYLARGE,-VERYLARGE), 
	* or the inrange point is on the edge, and
     * the line segment from the outrange point does not cross any 
	* other edges to get there. In either case, we return the inrange 
	* point as the 'edge' intersection point. This will basically draw
	* line.
	*/
    if (points[i].type == INRANGE) {
	   *ex = bx; 
	   *ey = by;
    } else {
	   *ex = ax; 
	   *ey = ay;
    }
    return;
}

/* XXX - JG  */
/* single edge intersection algorithm for "steps" curves */
/* 
 * Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segments
 * forming the step between the two points. 
 *
 * Recall that if P1 = (x1,y1) and P2 = (x2,y2), the step from  
 * P1 to P2 is drawn as two line segments: (x1,y1)->(x2,y1) and 
 * (x2,y1)->(x2,y2). 
 */
void
edge_intersect_steps(points, i, ex, ey)
	struct coordinate *points; /* the points array */
	int i;				/* line segment from point i-1 to point i */
	double *ex, *ey;		/* the point where it crosses an edge */
{
    /* global x_min, x_max, y_min, x_max */
    double ax = points[i-1].x;
    double ay = points[i-1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if (points[i].type == INRANGE) {	/* from OUTRANGE to INRANG */
	    if (inrange(ay,y_min,y_max)) {
		*ey = ay;
		if (ax > x_max)
			*ex = x_max;
		else			/* x < x_min */
			*ex = x_min;
	    } else {
	    	*ex = bx;
		if (ay > y_max)     
			*ey = y_max;
		else			/* y < y_min */
			*ey = y_min;
	    }
    } else {				/* from INRANGE to OUTRANGE */
	    if (inrange(bx,x_min,x_max)) {
		*ex = bx;
		if (by > y_max)
			*ey = y_max;
		else			/* y < y_min */
			*ey = y_min;
	    } else {
	    	*ey = ay;
		if (bx > x_max)     
			*ex = x_max;
		else			/* x < x_min */
			*ex = x_min;
	    }
    }
    return;
}

/* XXX - JG  */
/* double edge intersection algorithm for "steps" plot */
/* Given two points, both outside the plot, return the points where an 
 * edge of the plot intersects the line segments forming a step 
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to 
 * draw (the one-point case is a degenerate of the two-point case and we do 
 * not distinguish it - we draw it anyway).
 *
 * Recall that if P1 = (x1,y1) and P2 = (x2,y2), the step from  
 * P1 to P2 is drawn as two line segments: (x1,y1)->(x2,y1) and 
 * (x2,y1)->(x2,y2). 
 */
TBOOLEAN				/* any intersection? */
two_edge_intersect_steps(points, i, lx, ly)
	struct coordinate *points; /* the points array */
	int i;				/* line segment from point i-1 to point i */
	double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
{
    /* global x_min, x_max, y_min, x_max */
    double ax = points[i-1].x;
    double ay = points[i-1].y;
    double bx = points[i].x;
    double by = points[i].y;

    if ( max(ax,bx) < x_min || min(ax,bx) > x_max || 
         max(ay,by) < y_min || min(ay,by) > y_max ||
         ( (ay  > y_max || ay < y_min)            &&
           (bx  > x_max || bx < x_min)  ) ) {
	return(FALSE);				
    } else if (inrange(ay,y_min,y_max) && inrange(bx,x_min,x_max)) {	/* corner of step inside plotspace */
    	*ly++ = ay;
	if (ax < x_min) 
		*lx++ = x_min;
	else 
		*lx++ = x_max;

	*lx++ = bx;
	if (by < x_min) 
		*ly++ = y_min;
	else 
		*ly++ = y_max;

	return(TRUE);
    } else if (inrange(ay,y_min,y_max)) {	/* cross plotspace in x-direction */
	*lx++ = x_min;
	*ly++ = ay;
	*lx++ = x_max;
	*ly++ = ay;
	return(TRUE);
    } else if (inrange(ax,x_min,x_max)) {	/* cross plotspace in y-direction */
	*lx++ = bx;
	*ly++ = y_min;
	*lx++ = bx;
	*ly++ = y_max;
	return(TRUE);
    } else
	return(FALSE);
}

/* double edge intersection algorithm */
/* Given two points, both outside the plot, return
 * the points where an edge of the plot intersects the line segment defined 
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to 
 * draw (the one-point case is a degenerate of the two-point case and we do 
 * not distinguish it - we draw it anyway).
 */
TBOOLEAN				/* any intersection? */
two_edge_intersect(points, i, lx, ly)
	struct coordinate GPHUGE *points; /* the points array */
	int i;				/* line segment from point i-1 to point i */
	double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
{
    /* global x_min, x_max, y_min, x_max */
    double ax = points[i-1].x;
    double ay = points[i-1].y;
    double bx = points[i].x;
    double by = points[i].y;
    double x, y;			/* possible intersection point */
    TBOOLEAN intersect = FALSE;

    if (by == ay) {
	   /* horizontal line */
	   /* y coord must be in range, and line must span both x_min and x_max */
	   /* note that spanning x_min implies spanning x_max */
	   if (inrange(by, y_min, y_max) && inrange(x_min, ax, bx)) {
		  *lx++ = x_min;
		  *ly++ = by;
		  *lx++ = x_max;
		  *ly++ = by;
		  return(TRUE);
	   } else
		return(FALSE);
    } else if (bx == ax) {
	   /* vertical line */
	   /* x coord must be in range, and line must span both y_min and y_max */
	   /* note that spanning y_min implies spanning y_max */
	   if (inrange(bx, x_min, x_max) && inrange(y_min, ay, by)) {
		  *lx++ = bx;
		  *ly++ = y_min;
		  *lx++ = bx;
		  *ly++ = y_max;
		  return(TRUE);
	   } else
		return(FALSE);
    }

    /* slanted line of some kind */
    /* there can be only zero or two intersections below */

    /* does it intersect y_min edge */
    if (inrange(y_min, ay, by)) {
	   x = ax + (y_min-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, x_min, x_max)) {
		  *lx++ = x;
		  *ly++ = y_min;
		  intersect = TRUE;
	   }
    }
    
    /* does it intersect y_max edge */
    if (inrange(y_max, ay, by)) {
	   x = ax + (y_max-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, x_min, x_max)) {
		  *lx++ = x;
		  *ly++ = y_max;
		  intersect = TRUE;
	   }
    }

    /* does it intersect x_min edge */
    if (inrange(x_min, ax, bx)) {
	   y = ay + (x_min-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, y_min, y_max)) {
		  *lx++ = x_min;
		  *ly++ = y;
		  intersect = TRUE;
	   }
    }

    /* does it intersect x_max edge */
    if (inrange(x_max, ax, bx)) {
	   y = ay + (x_max-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, y_min, y_max)) {
		  *lx++ = x_max;
		  *ly++ = y;
		  intersect = TRUE;
	   }
    }

    if (intersect)
	 return(TRUE);

    /* It is possible for one or more of the [ab][xy] values to be -VERYLARGE.
	* If ax=bx=-VERYLARGE or ay=by=-VERYLARGE we have already returned
	* FALSE above.
	* Note that for ax=ay=-VERYLARGE or bx=by=-VERYLARGE we can do nothing.
	* Otherwise we fall through all the tests above. 
	* Handle them carefully here. As yet we have no way for them to be +VERYLARGE.
	*/
    if (ax == -VERYLARGE) {
	   if (ay != -VERYLARGE
		  && inrange(by, y_min, y_max) && inrange(x_max, ax, bx)) {
		  *lx++ = x_min;
		  *ly = by;
		  *lx++ = x_max;
		  *ly = by;
		  intersect = TRUE;
	   }
    } else if (bx == -VERYLARGE) {
	   if (by != -VERYLARGE
		  && inrange(ay, y_min, y_max) && inrange(x_max, ax, bx)) {
		  *lx++ = x_min;
		  *ly = ay;
		  *lx++ = x_max;
		  *ly = ay;
		  intersect = TRUE;
	   }
    } else if (ay == -VERYLARGE) {
	   /* note we know ax != -VERYLARGE */
	   if (inrange(bx, x_min, x_max) && inrange(y_max, ay, by)) {
		  *lx++ = bx;
		  *ly = y_min;
		  *lx++ = bx;
		  *ly = y_max;
		  intersect = TRUE;
	   }
    } else if (by == -VERYLARGE) {
	   /* note we know bx != -VERYLARGE */
	   if (inrange(ax, x_min, x_max) && inrange(y_max, ay, by)) {
		  *lx++ = ax;
		  *ly = y_min;
		  *lx++ = ax;
		  *ly = y_max;
		  intersect = TRUE;
	   }
    }

    return(intersect);
}

/* Polar transform of all curves */
/* Original code by John Campbell (CAMPBELL@NAUVAX.bitnet) */
polar_xform (plots, pcount)
	struct curve_points *plots;
	int pcount;			/* count of curves in plots array */
{
     struct curve_points *this_plot;
     int curve;			/* loop var, for curves */
     register int i, p_cnt;	/* loop/limit var, for points */
     struct coordinate GPHUGE *pnts;	/* abbrev. for points array */
	double x, y;			/* new cartesian value */
	TBOOLEAN anydefined = FALSE;
	double d2r;

	if(angles_format == ANGLES_DEGREES){
		d2r = DEG2RAD;
	} else {
		d2r = 1.0;
	}

/*
	Cycle through all the plots converting polar to rectangular.
     If autoscaling, adjust max and mins. Ignore previous values.
	If not autoscaling, use the yrange for both x and y ranges.
*/
	if (autoscale_ly) {
	    x_min = VERYLARGE;
	    y_min = VERYLARGE;
	    x_max = -VERYLARGE;
	    y_max = -VERYLARGE;
	    autoscale_lx = TRUE;
	} else {
	    x_min = y_min;
	    x_max = y_max;
	}
    
	this_plot = plots;
	for (curve = 0; curve < pcount; this_plot = this_plot->next_cp, curve++) {
		p_cnt = this_plot->p_count;
        pnts = &(this_plot->points[0]);

	/*	Convert to cartesian all points in this curve. */
		for (i = 0; i < p_cnt; i++) {
			if (pnts[i].type != UNDEFINED) {
			     anydefined = TRUE;
			     /* modify points to reset origin and from degrees */
			     pnts[i].y -= rmin;
			     pnts[i].x *= d2r;
			     /* convert to cartesian coordinates */
				x = pnts[i].y*cos(pnts[i].x);
				y = pnts[i].y*sin(pnts[i].x);
				pnts[i].x = x;
				pnts[i].y = y;
				if (autoscale_ly) {
				    if (x_min > x) x_min = x;
				    if (x_max < x) x_max = x;
				    if (y_min > y) y_min = y;
				    if (y_max < y) y_max = y;
				    pnts[i].type = INRANGE;
				} else if(inrange(x, x_min, x_max) && inrange(y, y_min, y_max))
				  pnts[i].type = INRANGE;
				else
				  pnts[i].type = OUTRANGE;
			}
		}	
	}

	if (autoscale_lx && anydefined && fabs(x_max - x_min) < zero) {
	    /* This happens at least for the plot of 1/cos(x) (vertical line). */
	    fprintf(stderr, "Warning: empty x range [%g:%g], ", x_min,x_max);
	    if (x_min == 0.0) {
		   x_min = -1; 
		   x_max = 1;
	    } else {
		   x_min *= 0.9;
		   x_max *= 1.1;
	    }
	    fprintf(stderr, "adjusting to [%g:%g]\n", x_min,x_max);
	}
	if (autoscale_ly && anydefined && fabs(y_max - y_min) < zero) {
	    /* This happens at least for the plot of 1/sin(x) (horiz. line). */
	    fprintf(stderr, "Warning: empty y range [%g:%g], ", y_min, y_max);
	    if (y_min == 0.0) {
		   y_min = -1;
		   y_max = 1;
	    } else {
		   y_min *= 0.9;
		   y_max *= 1.1;
	    }
	    fprintf(stderr, "adjusting to [%g:%g]\n", y_min, y_max);
	}
}

/* DRAW_YTICS: draw a regular tic series, y axis */
draw_ytics(start, incr, end)
		double start, incr, end; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace;
	int ltic;			/* for mini log tics */
	double lticplace;	/* for mini log tics */
	double ticmin, ticmax;	/* for checking if tic is almost inrange */

	if (end == VERYLARGE)            /* for user-def series */
		end = max(y_min,y_max);

	/* limit to right side of plot */
	end = min(end, max(y_min,y_max));

	/* to allow for rounding errors */
	ticmin = min(y_min,y_max) - SIGNIF*incr;
	ticmax = max(y_min,y_max) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if ( inrange(ticplace,ticmin,ticmax) )
			ytick(ticplace, yformat, incr, 1.0);
		if (is_log_y && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
		    int lstart, linc;
		    if ((end - start) >= 10)
		    {
			lstart = 10; /* No little ticks */
			linc = 5;
		    }
		    else if((end - start) >= 5)
		    {
			lstart = 2; /* 4 per decade */
			linc = 3;
		    }
		    else
		    {
			lstart = 2; /* 9 per decade */
			linc = 1;
		    }
		    for (ltic = lstart; ltic < (int)base_log_y; ltic += linc) {
				lticplace = ticplace+log((double)ltic)/log_base_log_y;
				if ( inrange(lticplace,ticmin,ticmax) )
					ytick(lticplace, "\0", incr, 0.5);
		    }
		}
	}
}

/* DRAW_XTICS: draw a regular tic series, x axis */
draw_xtics(start, incr, end)
		double start, incr, end; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace;
	int ltic;			/* for mini log tics */
	double lticplace;	/* for mini log tics */
	double ticmin, ticmax;	/* for checking if tic is almost inrange */

	if (end == VERYLARGE)            /* for user-def series */
		end = max(x_min,x_max);

	/* limit to right side of plot */
	end = min(end, max(x_min,x_max));

	/* to allow for rounding errors */
	ticmin = min(x_min,x_max) - SIGNIF*incr;
	ticmax = max(x_min,x_max) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if ( inrange(ticplace,ticmin,ticmax) )
			if(!polar || ticplace == start || ticplace == end) 
				xtick(ticplace, xformat, incr, 1.0);
		if (is_log_x && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
		    int lstart, linc;
		    if ((end - start) >= 10)
		    {
			lstart = 10; /* No little ticks */
			linc = 5;
		    }
		    else if((end - start) >= 5)
		    {
			lstart = 2; /* 4 per decade */
			linc = 3;
		    }
		    else
		    {
			lstart = 2; /* 9 per decade */
			linc = 1;
		    }
		    for (ltic = lstart; ltic < (int)base_log_x; ltic += linc) {
				lticplace = ticplace+log((double)ltic)/log_base_log_x;
				if ( inrange(lticplace,ticmin,ticmax) )
					xtick(lticplace, "\0", incr, 0.5);
			}
		}
	}
}

/* DRAW_SERIES_YTICS: draw a user tic series, y axis */
draw_series_ytics(start, incr, end)
		double start, incr, end; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace, place;
	double ticmin, ticmax;	/* for checking if tic is almost inrange */
	double spacing = is_log_y ? log(incr)/log_base_log_y : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(is_log_y, base_log_y, y_min),
			  CheckLog(is_log_y, base_log_y, y_max));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(is_log_y, base_log_y, y_min),
			     CheckLog(is_log_y, base_log_y, y_max)));

	/* to allow for rounding errors */
	ticmin = min(y_min,y_max) - SIGNIF*incr;
	ticmax = max(y_min,y_max) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (is_log_y ? log(ticplace)/log_base_log_y : ticplace);
	    if ( inrange(place,ticmin,ticmax) )
		 ytick(place, yformat, spacing, 1.0);
	}
}


/* DRAW_SERIES_XTICS: draw a user tic series, x axis */
draw_series_xtics(start, incr, end)
		double start, incr, end; /* tic series definition */
		/* assume start < end, incr > 0 */
{
	double ticplace, place;
	double ticmin, ticmax;	/* for checking if tic is almost inrange */
	double spacing = is_log_x ? log(incr)/log_base_log_x : incr;

	if (end == VERYLARGE)
		end = max(CheckLog(is_log_x, base_log_x, x_min),
			  CheckLog(is_log_x, base_log_x, x_max));
	else
	  /* limit to right side of plot */
	  end = min(end, max(CheckLog(is_log_x, base_log_x, x_min),
			     CheckLog(is_log_x, base_log_x, x_max)));

	/* to allow for rounding errors */
	ticmin = min(x_min,x_max) - SIGNIF*incr;
	ticmax = max(x_min,x_max) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
	    place = (is_log_x ? log(ticplace)/log_base_log_x : ticplace);
	    if ( inrange(place,ticmin,ticmax) )
		 xtick(place, xformat, spacing, 1.0);
	}
}
char GPFAR * GPFAR month[]={
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};
draw_month_ytics()
{ 
    long l_tickplace,l_incr,l_end,m_calc;

    l_tickplace = (long)y_min;
    if((double)l_tickplace<y_min)l_tickplace++;
    l_end=(double)y_max;
    l_incr=(l_end-l_tickplace+1)/12;
    if(l_incr<1)l_incr=1;
    while(l_tickplace<=l_end)
    {
	m_calc = (l_tickplace-1)%12;
	if(m_calc<0)m_calc += 12;
	ytick((double)l_tickplace,month[m_calc],(double)l_incr,1.0);
	l_tickplace += l_incr;
    }
}
draw_month_xtics()
{
    long l_tickplace,l_incr,l_end,m_calc;

    l_tickplace = (long)x_min;
    if((double)l_tickplace<x_min)l_tickplace++;
    l_end=(double)x_max;
    l_incr=(l_end-l_tickplace+1)/12;
    if(l_incr<1)l_incr=1;
    while(l_tickplace<=l_end)
    {
	m_calc = (l_tickplace-1)%12;
	if(m_calc<0)m_calc += 12;
	xtick((double)l_tickplace,month[m_calc],(double)l_incr,1.0);
	l_tickplace += l_incr;
    }
}
char *day[]={
    "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
draw_day_ytics()
{ 
    long l_tickplace,l_incr,l_end,m_calc;

    l_tickplace = (long)y_min;
    if((double)l_tickplace<y_min)l_tickplace++;
    l_end=(double)y_max;
    l_incr=(l_end-l_tickplace+1)/14;
    if(l_incr<1)l_incr=1;
    while(l_tickplace<=l_end)
    {
	m_calc = l_tickplace%7;
	if(m_calc<0)m_calc += 7;
	ytick((double)l_tickplace,day[m_calc],(double)l_incr,1.0);
	l_tickplace += l_incr;
    }
}
draw_day_xtics()
{ 
    long l_tickplace,l_incr,l_end,m_calc;

    l_tickplace = (long)x_min;
    if((double)l_tickplace<x_min)l_tickplace++;
    l_end=(double)x_max;
    l_incr=(l_end-l_tickplace+1)/14;
    if(l_incr<1)l_incr=1;
    while(l_tickplace<=l_end)
    {
	m_calc = l_tickplace%7;
	if(m_calc<0)m_calc += 7;
	xtick((double)l_tickplace,day[m_calc],(double)l_incr,1.0);
	l_tickplace += l_incr;
    }
}
/* DRAW_SET_YTICS: draw a user tic set, y axis */
draw_set_ytics(list)
	struct ticmark *list;	/* list of tic marks */
{
    double ticplace;
    double incr = (y_max - y_min) / 10;
    /* global x_min, x_max, xscale, y_min, y_max, yscale */

    while (list != NULL) {
	   ticplace = (is_log_y ? log(list->position)/log_base_log_y
				: list->position);
	   if ( inrange(ticplace, y_min, y_max) 		/* in range */
		  || NearlyEqual(ticplace, y_min, incr)	/* == y_min */
		  || NearlyEqual(ticplace, y_max, incr))	/* == y_max */
		ytick(ticplace, list->label, incr, 1.0);

	   list = list->next;
    }
}

/* DRAW_SET_XTICS: draw a user tic set, x axis */
draw_set_xtics(list)
	struct ticmark *list;	/* list of tic marks */
{
    double ticplace;
    double incr = (x_max - x_min) / 10;
    /* global x_min, x_max, xscale, y_min, y_max, yscale */

    while (list != NULL) {
	   ticplace = (is_log_x ? log(list->position)/log_base_log_x
				: list->position);
	   if ( inrange(ticplace, x_min, x_max) 		/* in range */
		  || NearlyEqual(ticplace, x_min, incr)	/* == x_min */
		  || NearlyEqual(ticplace, x_max, incr))	/* == x_max */
		xtick(ticplace, list->label, incr, 1.0);

	   list = list->next;
    }
}

/* draw and label a y-axis ticmark */
ytick(place, text, spacing, ticscale)
        double place;                   /* where on axis to put it */
        char *text;                     /* optional text label */
        double spacing;         /* something to use with checkzero */
        double ticscale;         /* scale factor for tic mark (0..1] */
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int ticsize = (int)((t->h_tic) * ticscale);

	place = CheckZero(place,spacing); /* to fix rounding error near zero */
    if (grid) {
           (*t->linetype)(-1);  /* axis line type */
           /* do not put a rectangular grid on a polar plot */
	   if( !polar){
	     (*t->move)(xleft, map_y(place));
	     (*t->vector)(xright, map_y(place));
           } else {   /* put a circular grid for polar -- not clipped! */
             int i;
		(*t->move)(map_x(ZERO), map_y(place));
		for( i=0; i <= 360; i++)
		   (*t->vector)( map_x(place*sin( (double) DEG2RAD*i)),
			map_y(place*cos( (double) DEG2RAD*i)) );
		}
	   (*t->linetype)(-2); /* border linetype */
    }
    if (tic_in) {
      /* if polar plot, put the tics along the axes */
      if( polar){
           (*t->move)(map_x(ZERO),map_y(place));
           (*t->vector)(map_x(ZERO) + ticsize, map_y(place));
           (*t->move)(map_x(ZERO), map_y(place));
           (*t->vector)(map_x(ZERO) - ticsize, map_y(place));
	 } else {
	   (*t->move)(xleft, map_y(place));
           (*t->vector)(xleft + ticsize, map_y(place));
           (*t->move)(xright, map_y(place));
           (*t->vector)(xright - ticsize, map_y(place));
	 }
    } else {
      if( polar){
           (*t->move)(map_x(ZERO), map_y(place));
           (*t->vector)(map_x(ZERO) - ticsize, map_y(place));
	 }else{
           (*t->move)(xleft, map_y(place));
           (*t->vector)(xleft - ticsize, map_y(place));
	 }
    }

    /* label the ticmark */
    if (text == NULL) 
	 text = yformat;
    
    if( polar){
      (void) sprintf(ticlabel, text,
		CheckLog(is_log_y, base_log_y, fabs( place)+rmin));
      if ((*t->justify_text)(RIGHT)) {
	   (*t->put_text)(map_x(ZERO)-(t->h_char),
				   map_y(place), ticlabel);
	 } else {
	   (*t->put_text)(map_x(ZERO)-(t->h_char)*(strlen(ticlabel)+1),
				   map_y(place), ticlabel);
	 }
    } else {
    
      (void) sprintf(ticlabel, text, CheckLog(is_log_y, base_log_y, place));
      if ((*t->justify_text)(RIGHT)) {
	   (*t->put_text)(xleft-(t->h_char),
				   map_y(place), ticlabel);
	 } else {
	   (*t->put_text)(xleft-(t->h_char)*(strlen(ticlabel)+1),
				   map_y(place), ticlabel);
	 }
    }
}

/* draw and label an x-axis ticmark */
xtick(place, text, spacing, ticscale)
        double place;                   /* where on axis to put it */
        char *text;                     /* optional text label */
        double spacing;         /* something to use with checkzero */
        double ticscale;         /* scale factor for tic mark (0..1] */
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int ticsize = (int)((t->v_tic) * ticscale);

	place = CheckZero(place,spacing); /* to fix rounding error near zero */
    if (grid) {
           (*t->linetype)(-1);  /* axis line type */
           if( !polar){  /* do not place a rectangular grid */
	     (*t->move)(map_x(place), ybot);
	     (*t->vector)(map_x(place), ytop);
           } else { /* angular lines only for start and stop */
	     int i;
	     for( i=0; i < 360; i+=10){
		 (*t->move)(map_x(ZERO),map_y(ZERO) );
		 (*t->vector)(map_x(-place*cos((double) DEG2RAD*i)),
			 map_y(-place*sin( (double)DEG2RAD*i)));
		 if( i%90 == 0){
			 (void) sprintf(ticlabel, "%d", i);
		 (*t->put_text)(map_x(-1.05*place*cos((double) DEG2RAD*i))
			 +(t->h_char)*strlen(ticlabel)/2,
			 map_y(-1.05*place*sin( (double)DEG2RAD*i))
				 , ticlabel);
		 }
	     }
           }
	   (*t->linetype)(-2); /* border linetype */
    }
    if (tic_in) {
      if( polar){
           (*t->move)(map_x(place), map_y(ZERO));
           (*t->vector)(map_x(place), map_y(ZERO) + ticsize);
           (*t->move)(map_x(place), map_y(ZERO));
           (*t->vector)(map_x(place), map_y(ZERO) - ticsize);
	 } else{
           (*t->move)(map_x(place), ybot);
           (*t->vector)(map_x(place), ybot + ticsize);
           (*t->move)(map_x(place), ytop);
           (*t->vector)(map_x(place), ytop - ticsize);
	 }
    } else {
      if( polar){
           (*t->move)(map_x(place), map_y(ZERO));
           (*t->vector)(map_x(place), map_y(ZERO) - ticsize);
	 }else{
           (*t->move)(map_x(place), ybot);
           (*t->vector)(map_x(place), ybot - ticsize);
	 }
    }
    
    /* label the ticmark */
    if (text == NULL)
	 text = xformat;

    if(polar){
      (void) sprintf(ticlabel, text, CheckLog(is_log_x, base_log_x, fabs(place)+rmin));
      if ((*t->justify_text)(CENTRE)) {
	   (*t->put_text)(map_x(place),
				   map_y(ZERO)-(t->v_char), ticlabel);
	 } else {
	   (*t->put_text)(map_x(place)-(t->h_char)*strlen(ticlabel)/2,
				   map_y(ZERO)-(t->v_char), ticlabel);
	 }
    }else{

      (void) sprintf(ticlabel, text, CheckLog(is_log_x, base_log_x, place));
      if ((*t->justify_text)(CENTRE)) {
	   (*t->put_text)(map_x(place),
				   ybot-(t->v_char), ticlabel);
	 } else {
	   (*t->put_text)(map_x(place)-(t->h_char)*strlen(ticlabel)/2,
				   ybot-(t->v_char), ticlabel);
	 }
    }
}
