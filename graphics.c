/* GNUPLOT - graphics.c */
/*
 * Copyright (C) 1986, 1987, 1990   Thomas Williams, Colin Kelley
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
 * This software  is provided "as is" without express or implied warranty.
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
 * send your comments or suggestions to (pixar!info-gnuplot@sun.com).
 * 
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "plot.h"
#include "setshow.h"

extern char *strcpy(),*strncpy(),*strcat();

void plot_impulses();
void plot_lines();
void plot_points();
void plot_dots();
void edge_intersect();
BOOLEAN two_edge_intersect();

#ifndef max		/* Lattice C has max() in math.h, but shouldn't! */
#define max(a,b) ((a > b) ? a : b)
#endif

#ifndef min
#define min(a,b) ((a < b) ? a : b)
#endif

#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

/* Define the boundary of the plot
 * These are computed at each call to do_plot, and are constant over
 * the period of one do_plot. They actually only change when the term
 * type changes and when the 'set size' factors change. 
 */
static int xleft, xright, ybot, ytop;

/* Boundary and scale factors, in user coordinates */
static double xmin, xmax, ymin, ymax;
static double xscale, yscale;

/* And the functions to map from user to terminal coordinates */
#define map_x(x) (int)(xleft+(x-xmin)*xscale+0.5) /* maps floating point x to screen */ 
#define map_y(y) (int)(ybot+(y-ymin)*yscale+0.5)	/* same for y */

/* (DFK) Watch for cancellation error near zero on axes labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
#define NearlyEqual(x,y,tic) (fabs((x)-(y)) < ((tic) * SIGNIF))

/* (DFK) For some reason, the Sun386i compiler screws up with the CheckLog 
 * macro, so I write it as a function on that machine.
 */
#ifndef sun386
/* (DFK) Use 10^x if logscale is in effect, else x */
#define CheckLog(log, x) ((log) ? pow(10., (x)) : (x))
#else
static double
CheckLog(log, x)
     BOOLEAN log;
     double x;
{
  if (log)
    return(pow(10., x));
  else
    return(x);
}
#endif /* sun386 */

double
LogScale(coord, islog, what, axis)
	double coord;			/* the value */
	BOOLEAN islog;			/* is this axis in logscale? */
	char *what;			/* what is the coord for? */
	char *axis;			/* which axis is this for ("x" or "y")? */
{
    if (islog) {
	   if (coord <= 0.0) {
		  char errbuf[100];		/* place to write error message */
		(void) sprintf(errbuf,"%s has %s coord of %g; must be above 0 for log scale!",
				what, axis, coord);
		  (*term_tbl[term].text)();
		  (void) fflush(outfile);
		  int_error(errbuf, NO_CARET);
	   } else
		return(log10(coord));
    } else {
	   return(coord);
    }
	return((double)NULL); /* shut lint up */
}

/* borders of plotting area */
/* computed once on every call to do_plot */
boundary(scaling)
	BOOLEAN scaling;		/* TRUE if terminal is doing the scaling */
{
    register struct termentry *t = &term_tbl[term];
    xleft = (t->h_char)*12;
    xright = (scaling ? 1 : xsize) * (t->xmax) - (t->h_char)*2 - (t->h_tic);
    ybot = (t->v_char)*5/2 + 1;
    ytop = (scaling ? 1 : ysize) * (t->ymax) - (t->v_char)*3/2 - 1;
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


double make_tics(tmin,tmax,logscale)
double tmin,tmax;
BOOLEAN logscale;
{
register double xr,xnorm,tics,tic,l10;

	xr = fabs(tmin-tmax);
	
	l10 = log10(xr);
	if (logscale) {
		tic = dbl_raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
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
BOOLEAN scaling;

/* store these in variables global to this file */
/* otherwise, we have to pass them around a lot */
     xmin = min_x;
     xmax = max_x; 
     ymin = min_y;
     ymax = max_y;

	if (polar) {
	    /* will possibly change xmin, xmax, ymin, ymax */
	    polar_xform(plots,pcount);
	}

	if (ymin == VERYLARGE || ymax == -VERYLARGE)
		int_error("all points undefined!", NO_CARET);

	if (xmin == VERYLARGE || xmax == -VERYLARGE)
        int_error("all points undefined!", NO_CARET);

/*	Apply the desired viewport offsets. */
     if (ymin < ymax) {
	    ymin -= boff;
	    ymax += toff;
	} else {
	    ymax -= boff;
	    ymin += toff;
	}
     if (xmin < xmax) {
	    xmin -= loff;
	    xmax += roff;
	} else {
	    xmax -= loff;
	    xmin += roff;
	}

/* SETUP RANGES, SCALES AND TIC PLACES */
    if (ytics && yticdef.type == TIC_COMPUTED) {
	   ytic = make_tics(ymin,ymax,log_y);
    
	   if (autoscale_ly) {
		  if (ymin < ymax) {
			 ymin = ytic * floor(ymin/ytic);       
			 ymax = ytic * ceil(ymax/ytic);
		  }
		  else {			/* reverse axis */
			 ymin = ytic * ceil(ymin/ytic);       
			 ymax = ytic * floor(ymax/ytic);
		  }
	   }
    }

    if (xtics && xticdef.type == TIC_COMPUTED) {
	   xtic = make_tics(xmin,xmax,log_x);
	   
	   if (autoscale_lx) {
		  if (xmin < xmax) {
			 xmin = xtic * floor(xmin/xtic);	
			 xmax = xtic * ceil(xmax/xtic);
		  } else {
			 xmin = xtic * ceil(xmin/xtic);
			 xmax = xtic * floor(xmax/xtic);	
		  }
	   }
    }

/*	This used be xmax == xmin, but that caused an infinite loop once. */
	if (fabs(xmax - xmin) < zero)
		int_error("xmin should not equal xmax!",NO_CARET);
	if (fabs(ymax - ymin) < zero)
		int_error("ymin should not equal ymax!",NO_CARET);

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
	yscale = (ytop - ybot)/(ymax - ymin);
	xscale = (xright - xleft)/(xmax - xmin);
	
/* DRAW AXES */
	(*t->linetype)(-1);	/* axis line type */
	xaxis_y = map_y(0.0);
	yaxis_x = map_x(0.0); 

	if (xaxis_y < ybot)
		xaxis_y = ybot;				/* save for impulse plotting */
	else if (xaxis_y >= ytop)
		xaxis_y = ytop ;
	else if (!log_y) {
		(*t->move)(xleft,xaxis_y);
		(*t->vector)(xright,xaxis_y);
	}

	if (!log_x && yaxis_x >= xleft && yaxis_x < xright ) {
		(*t->move)(yaxis_x,ybot);
		(*t->vector)(yaxis_x,ytop);
	}

/* DRAW TICS */
	(*t->linetype)(-2); /* border linetype */

    /* label y axis tics */
     if (ytics) {
	    switch (yticdef.type) {
		   case TIC_COMPUTED: {
 			  if (ymin < ymax)
			    draw_ytics(ytic * floor(ymin/ytic),
						ytic,
						ytic * ceil(ymax/ytic));
			  else
			    draw_ytics(ytic * floor(ymax/ytic),
						ytic,
						ytic * ceil(ymin/ytic));

			  break;
		   }
		   case TIC_SERIES: {
			  draw_ytics(yticdef.def.series.start, 
					  yticdef.def.series.incr, 
					  yticdef.def.series.end);
			  break;
		   }
		   case TIC_USER: {
			  draw_user_ytics(yticdef.def.user);
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
 			  if (xmin < xmax)
			    draw_xtics(xtic * floor(xmin/xtic),
						xtic,
						xtic * ceil(xmax/xtic));
			  else
			    draw_xtics(xtic * floor(xmax/xtic),
						xtic,
						xtic * ceil(xmin/xtic));

			  break;
		   }
		   case TIC_SERIES: {
			  draw_xtics(xticdef.def.series.start, 
					  xticdef.def.series.incr, 
					  xticdef.def.series.end);
			  break;
		   }
		   case TIC_USER: {
			  draw_user_xtics(xticdef.def.user);
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
	(*t->move)(xleft,ybot);	
	(*t->vector)(xright,ybot);	
	(*t->vector)(xright,ytop);	
	(*t->vector)(xleft,ytop);	
	(*t->vector)(xleft,ybot);

/* PLACE YLABEL */
    if (*ylabel != NULL) {
	   if ((*t->text_angle)(1)) { 
		  if ((*t->justify_text)(CENTRE)) { 
			 (*t->put_text)((t->v_char),
						 (ytop+ybot)/2, ylabel);
		  }
		  else {
			 (*t->put_text)((t->v_char),
						 (ytop+ybot)/2-(t->h_char)*strlen(ylabel)/2, 
						 ylabel);
		  }
	   }
	   else {
		  (void)(*t->justify_text)(LEFT);
		  (*t->put_text)(0,ytop+(t->v_char), ylabel);
	   }
	   (void)(*t->text_angle)(0);
    }

/* PLACE XLABEL */
    if (*xlabel != NULL) {
	   if ((*t->justify_text)(CENTRE)) 
		(*t->put_text)( (xleft+xright)/2,
					ybot-2*(t->v_char), xlabel);
	   else
		(*t->put_text)( (xleft+xright)/2 - strlen(xlabel)*(t->h_char)/2,
					ybot-2*(t->v_char), xlabel);
    }

/* PLACE TITLE */
    if (*title != NULL) {
	   if ((*t->justify_text)(CENTRE)) 
		(*t->put_text)( (xleft+xright)/2, 
					ytop+(t->v_char), title);
	   else
		(*t->put_text)( (xleft+xright)/2 - strlen(title)*(t->h_char)/2,
					ytop+(t->v_char), title);
    }

/* PLACE LABELS */
    for (this_label = first_label; this_label!=NULL;
			this_label=this_label->next ) {
	     xtemp = LogScale(this_label->x, log_x, "label", "x");
	     ytemp = LogScale(this_label->y, log_y, "label", "y");
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
	   int sx = map_x(LogScale(this_arrow->sx, log_x, "arrow", "x"));
	   int sy = map_y(LogScale(this_arrow->sy, log_y, "arrow", "y"));
	   int ex = map_x(LogScale(this_arrow->ex, log_x, "arrow", "x"));
	   int ey = map_y(LogScale(this_arrow->ey, log_y, "arrow", "y"));
	   
	   (*t->arrow)(sx, sy, ex, ey);
    }


/* DRAW CURVES */
	if (key == -1) {
	    xl = xright  - (t->h_tic) - (t->h_char)*5;
	    yl = ytop - (t->v_tic) - (t->v_char);
	}
	if (key == 1) {
	    xl = map_x( LogScale(key_x, log_x, "key", "x") );
	    yl = map_y( LogScale(key_y, log_y, "key", "y") );
	}

	this_plot = plots;
	for (curve = 0; curve < pcount; this_plot = this_plot->next_cp, curve++) {
		(*t->linetype)(this_plot->line_type);
		if (key != 0) {
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

		switch(this_plot->plot_style) {
		    case IMPULSES: {
			   if (key != 0) {
				  (*t->move)(xl+(t->h_char),yl);
				  (*t->vector)(xl+4*(t->h_char),yl);
			   }
			   plot_impulses(this_plot, xaxis_y);
			   break;
		    }
		    case LINES: {
			   if (key != 0) {
				  (*t->move)(xl+(int)(t->h_char),yl);
				  (*t->vector)(xl+(int)(4*(t->h_char)),yl);
			   }
			   plot_lines(this_plot);
			   break;
		    }
		    case POINTS: {
			   if (key != 0) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot_points(this_plot);
			   break;
		    }
		    case LINESPOINTS: {
			   /* put lines */
			   if (key != 0) {
				  (*t->move)(xl+(t->h_char),yl);
				  (*t->vector)(xl+4*(t->h_char),yl);
			   }
			   plot_lines(this_plot);

			   /* put points */
			   if (key != 0) {
				  (*t->point)(xl+2*(t->h_char),yl,
						    this_plot->point_type);
			   }
			   plot_points(this_plot);
			   break;
		    }
		    case DOTS: {
			   if (key != 0) {
				  (*t->point)(xl+2*(t->h_char),yl, -1);
			   }
			   plot_dots(this_plot);
			   break;
		    }
		}
		yl = yl - (t->v_char);
	}
	(*t->text)();
	(void) fflush(outfile);
}

/* plot_impulses:
 * Plot the curves in IMPULSES style
 */
void
plot_impulses(plot, xaxis_y)
	struct curve_points *plot;
	int xaxis_y;
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
			 if (!inrange(plot->points[i].x, xmin,xmax))
			   continue;
			 x = map_x(plot->points[i].x);
			 if ((ymin < ymax 
				 && plot->points[i].y < ymin)
				|| (ymax < ymin 
				    && plot->points[i].y > ymin))
			   y = map_y(ymin);
			 if ((ymin < ymax 
				 && plot->points[i].y > ymax)
				|| (ymax<ymin 
				    && plot->points[i].y < ymax))
			   y = map_y(ymax);
			 break;
		  }
		  default:		/* just a safety */
		  case UNDEFINED: {
			 continue;
		  }
	   }
				    
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

/* plot_points:
 * Plot the curves in POINTS style
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
	struct coordinate *points; /* the points array */
	int i;				/* line segment from point i-1 to point i */
	double *ex, *ey;		/* the point where it crosses an edge */
{
    /* global xmin, xmax, ymin, xmax */
    double ax = points[i-1].x;
    double ay = points[i-1].y;
    double bx = points[i].x;
    double by = points[i].y;
    double x, y;			/* possible intersection point */

    if (by == ay) {
	   /* horizontal line */
	   /* assume inrange(by, ymin, ymax) */
	   *ey = by;		/* == ay */

	   if (inrange(xmax, ax, bx))
		*ex = xmax;
	   else if (inrange(xmin, ax, bx))
		*ex = xmin;
	   else {
		(*term_tbl[term].text)();
	    (void) fflush(outfile);
		int_error("error in edge_intersect", NO_CARET);
	   }
	   return;
    } else if (bx == ax) {
	   /* vertical line */
	   /* assume inrange(bx, xmin, xmax) */
	   *ex = bx;		/* == ax */

	   if (inrange(ymax, ay, by))
		*ey = ymax;
	   else if (inrange(ymin, ay, by))
		*ey = ymin;
	   else {
		(*term_tbl[term].text)();
	    (void) fflush(outfile);
		int_error("error in edge_intersect", NO_CARET);
	   }
	   return;
    }

    /* slanted line of some kind */

    /* does it intersect ymin edge */
    if (inrange(ymin, ay, by) && ymin != ay && ymin != by) {
	   x = ax + (ymin-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, xmin, xmax)) {
		  *ex = x;
		  *ey = ymin;
		  return;			/* yes */
	   }
    }
    
    /* does it intersect ymax edge */
    if (inrange(ymax, ay, by) && ymax != ay && ymax != by) {
	   x = ax + (ymax-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, xmin, xmax)) {
		  *ex = x;
		  *ey = ymax;
		  return;			/* yes */
	   }
    }

    /* does it intersect xmin edge */
    if (inrange(xmin, ax, bx) && xmin != ax && xmin != bx) {
	   y = ay + (xmin-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, ymin, ymax)) {
		  *ex = xmin;
		  *ey = y;
		  return;
	   }
    }

    /* does it intersect xmax edge */
    if (inrange(xmax, ax, bx) && xmax != ax && xmax != bx) {
	   y = ay + (xmax-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, ymin, ymax)) {
		  *ex = xmax;
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
		  *ex = min(xmin, xmax);
		  *ey = by;
		  return;
	   }
    } else if (bx == -VERYLARGE) {
	   if (by != -VERYLARGE) {
		  *ex = min(xmin, xmax);
		  *ey = ay;
		  return;
	   }
    } else if (ay == -VERYLARGE) {
	   /* note we know ax != -VERYLARGE */
	   *ex = bx;
	   *ey = min(ymin, ymax);
	   return;
    } else if (by == -VERYLARGE) {
	   /* note we know bx != -VERYLARGE */
	   *ex = ax;
	   *ey = min(ymin, ymax);
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
BOOLEAN				/* any intersection? */
two_edge_intersect(points, i, lx, ly)
	struct coordinate *points; /* the points array */
	int i;				/* line segment from point i-1 to point i */
	double *lx, *ly;		/* lx[2], ly[2]: points where it crosses edges */
{
    /* global xmin, xmax, ymin, xmax */
    double ax = points[i-1].x;
    double ay = points[i-1].y;
    double bx = points[i].x;
    double by = points[i].y;
    double x, y;			/* possible intersection point */
    BOOLEAN intersect = FALSE;

    if (by == ay) {
	   /* horizontal line */
	   /* y coord must be in range, and line must span both xmin and xmax */
	   /* note that spanning xmin implies spanning xmax */
	   if (inrange(by, ymin, ymax) && inrange(xmin, ax, bx)) {
		  *lx++ = xmin;
		  *ly++ = by;
		  *lx++ = xmax;
		  *ly++ = by;
		  return(TRUE);
	   } else
		return(FALSE);
    } else if (bx == ax) {
	   /* vertical line */
	   /* x coord must be in range, and line must span both ymin and ymax */
	   /* note that spanning ymin implies spanning ymax */
	   if (inrange(bx, xmin, xmax) && inrange(ymin, ay, by)) {
		  *lx++ = bx;
		  *ly++ = ymin;
		  *lx++ = bx;
		  *ly++ = ymax;
		  return(TRUE);
	   } else
		return(FALSE);
    }

    /* slanted line of some kind */
    /* there can be only zero or two intersections below */

    /* does it intersect ymin edge */
    if (inrange(ymin, ay, by)) {
	   x = ax + (ymin-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, xmin, xmax)) {
		  *lx++ = x;
		  *ly++ = ymin;
		  intersect = TRUE;
	   }
    }
    
    /* does it intersect ymax edge */
    if (inrange(ymax, ay, by)) {
	   x = ax + (ymax-ay) * ((bx-ax) / (by-ay));
	   if (inrange(x, xmin, xmax)) {
		  *lx++ = x;
		  *ly++ = ymax;
		  intersect = TRUE;
	   }
    }

    /* does it intersect xmin edge */
    if (inrange(xmin, ax, bx)) {
	   y = ay + (xmin-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, ymin, ymax)) {
		  *lx++ = xmin;
		  *ly++ = y;
		  intersect = TRUE;
	   }
    }

    /* does it intersect xmax edge */
    if (inrange(xmax, ax, bx)) {
	   y = ay + (xmax-ax) * ((by-ay) / (bx-ax));
	   if (inrange(y, ymin, ymax)) {
		  *lx++ = xmax;
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
		  && inrange(by, ymin, ymax) && inrange(xmax, ax, bx)) {
		  *lx++ = xmin;
		  *ly = by;
		  *lx++ = xmax;
		  *ly = by;
		  intersect = TRUE;
	   }
    } else if (bx == -VERYLARGE) {
	   if (by != -VERYLARGE
		  && inrange(ay, ymin, ymax) && inrange(xmax, ax, bx)) {
		  *lx++ = xmin;
		  *ly = ay;
		  *lx++ = xmax;
		  *ly = ay;
		  intersect = TRUE;
	   }
    } else if (ay == -VERYLARGE) {
	   /* note we know ax != -VERYLARGE */
	   if (inrange(bx, xmin, xmax) && inrange(ymax, ay, by)) {
		  *lx++ = bx;
		  *ly = ymin;
		  *lx++ = bx;
		  *ly = ymax;
		  intersect = TRUE;
	   }
    } else if (by == -VERYLARGE) {
	   /* note we know bx != -VERYLARGE */
	   if (inrange(ax, xmin, xmax) && inrange(ymax, ay, by)) {
		  *lx++ = ax;
		  *ly = ymin;
		  *lx++ = ax;
		  *ly = ymax;
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
     struct coordinate *pnts;	/* abbrev. for points array */
	double x, y;			/* new cartesian value */
	BOOLEAN anydefined = FALSE;

/*
	Cycle through all the plots converting polar to rectangular.
     If autoscaling, adjust max and mins. Ignore previous values.
	If not autoscaling, use the yrange for both x and y ranges.
*/
	if (autoscale_ly) {
	    xmin = VERYLARGE;
	    ymin = VERYLARGE;
	    xmax = -VERYLARGE;
	    ymax = -VERYLARGE;
	    autoscale_lx = TRUE;
	} else {
	    xmin = ymin;
	    xmax = ymax;
	}
    
	this_plot = plots;
	for (curve = 0; curve < pcount; this_plot = this_plot->next_cp, curve++) {
		p_cnt = this_plot->p_count;
        pnts = &(this_plot->points[0]);

	/*	Convert to cartesian all points in this curve. */
		for (i = 0; i < p_cnt; i++) {
			if (pnts[i].type != UNDEFINED) {
			     anydefined = TRUE;
				x = pnts[i].y*cos(pnts[i].x);
				y = pnts[i].y*sin(pnts[i].x);
				pnts[i].x = x;
				pnts[i].y = y;
				if (autoscale_ly) {
				    if (xmin > x) xmin = x;
				    if (xmax < x) xmax = x;
				    if (ymin > y) ymin = y;
				    if (ymax < y) ymax = y;
				    pnts[i].type = INRANGE;
				} else if(inrange(x, xmin, xmax) && inrange(y, ymin, ymax))
				  pnts[i].type = INRANGE;
				else
				  pnts[i].type = OUTRANGE;
			}
		}	
	}

	if (autoscale_lx && anydefined && fabs(xmax - xmin) < zero) {
	    /* This happens at least for the plot of 1/cos(x) (vertical line). */
	    fprintf(stderr, "Warning: empty x range [%g:%g], ", xmin,xmax);
	    if (xmin == 0.0) {
		   xmin = -1; 
		   xmax = 1;
	    } else {
		   xmin *= 0.9;
		   xmax *= 1.1;
	    }
	    fprintf(stderr, "adjusting to [%g:%g]\n", xmin,xmax);
	}
	if (autoscale_ly && anydefined && fabs(ymax - ymin) < zero) {
	    /* This happens at least for the plot of 1/sin(x) (horiz. line). */
	    fprintf(stderr, "Warning: empty y range [%g:%g], ", ymin, ymax);
	    if (ymin == 0.0) {
		   ymin = -1;
		   ymax = 1;
	    } else {
		   ymin *= 0.9;
		   ymax *= 1.1;
	    }
	    fprintf(stderr, "adjusting to [%g:%g]\n", ymin, ymax);
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
		end = max(ymin,ymax);

	/* limit to right side of plot */
	end = min(end, max(ymin,ymax));

	/* to allow for rounding errors */
	ticmin = min(ymin,ymax) - SIGNIF*incr;
	ticmax = max(ymin,ymax) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if ( inrange(ticplace,ticmin,ticmax) )
			ytick(ticplace, yformat, incr, 1.0);
		if (log_y && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic <= 9; ltic++) {
				lticplace = ticplace+log10((double)ltic);
				if ( inrange(lticplace,ticmin,ticmax) )
					ytick(lticplace, (char *)NULL, incr, 0.5);
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
		end = max(xmin,xmax);

	/* limit to right side of plot */
	end = min(end, max(xmin,xmax));

	/* to allow for rounding errors */
	ticmin = min(xmin,xmax) - SIGNIF*incr;
	ticmax = max(xmin,xmax) + SIGNIF*incr;
	end = end + SIGNIF*incr; 

	for (ticplace = start; ticplace <= end; ticplace +=incr) {
		if ( inrange(ticplace,ticmin,ticmax) )
			xtick(ticplace, xformat, incr, 1.0);
		if (log_x && incr == 1.0) {
			/* add mini-ticks to log scale ticmarks */
			for (ltic = 2; ltic <= 9; ltic++) {
				lticplace = ticplace+log10((double)ltic);
				if ( inrange(lticplace,ticmin,ticmax) )
					xtick(lticplace, (char *)NULL, incr, 0.5);
			}
		}
	}
}

/* DRAW_USER_YTICS: draw a user tic series, y axis */
draw_user_ytics(list)
	struct ticmark *list;	/* list of tic marks */
{
    double ticplace;
    double incr = (ymax - ymin) / 10;
    /* global xmin, xmax, xscale, ymin, ymax, yscale */

    while (list != NULL) {
	   ticplace = list->position;
	   if ( inrange(ticplace, ymin, ymax) 		/* in range */
		  || NearlyEqual(ticplace, ymin, incr)	/* == ymin */
		  || NearlyEqual(ticplace, ymax, incr))	/* == ymax */
		ytick(ticplace, list->label, incr, 1.0);

	   list = list->next;
    }
}

/* DRAW_USER_XTICS: draw a user tic series, x axis */
draw_user_xtics(list)
	struct ticmark *list;	/* list of tic marks */
{
    double ticplace;
    double incr = (xmax - xmin) / 10;
    /* global xmin, xmax, xscale, ymin, ymax, yscale */

    while (list != NULL) {
	   ticplace = list->position;
	   if ( inrange(ticplace, xmin, xmax) 		/* in range */
		  || NearlyEqual(ticplace, xmin, incr)	/* == xmin */
		  || NearlyEqual(ticplace, xmax, incr))	/* == xmax */
		xtick(ticplace, list->label, incr, 1.0);

	   list = list->next;
    }
}

/* draw and label a y-axis ticmark */
ytick(place, text, spacing, ticscale)
        double place;                   /* where on axis to put it */
        char *text;                     /* optional text label */
        double spacing;         /* something to use with checkzero */
        float ticscale;         /* scale factor for tic mark (0..1] */
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int ticsize = (int)((t->h_tic) * ticscale);

	place = CheckZero(place,spacing); /* to fix rounding error near zero */
    if (grid) {
           (*t->linetype)(-1);  /* axis line type */
           (*t->move)(xleft, map_y(place));
           (*t->vector)(xright, map_y(place));
           (*t->linetype)(-2); /* border linetype */
    }
    if (tic_in) {
           (*t->move)(xleft, map_y(place));
           (*t->vector)(xleft + ticsize, map_y(place));
           (*t->move)(xright, map_y(place));
           (*t->vector)(xright - ticsize, map_y(place));
    } else {
           (*t->move)(xleft, map_y(place));
           (*t->vector)(xleft - ticsize, map_y(place));
    }

    /* label the ticmark */
	if (text) {
	    (void) sprintf(ticlabel, text, CheckLog(log_y, place));
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
        float ticscale;         /* scale factor for tic mark (0..1] */
{
    register struct termentry *t = &term_tbl[term];
    char ticlabel[101];
    int ticsize = (int)((t->v_tic) * ticscale);

	place = CheckZero(place,spacing); /* to fix rounding error near zero */
    if (grid) {
           (*t->linetype)(-1);  /* axis line type */
           (*t->move)(map_x(place), ybot);
           (*t->vector)(map_x(place), ytop);
           (*t->linetype)(-2); /* border linetype */
    }
    if (tic_in) {
           (*t->move)(map_x(place), ybot);
           (*t->vector)(map_x(place), ybot + ticsize);
           (*t->move)(map_x(place), ytop);
           (*t->vector)(map_x(place), ytop - ticsize);
    } else {
           (*t->move)(map_x(place), ybot);
           (*t->vector)(map_x(place), ybot - ticsize);
    }

    /* label the ticmark */
    if (text) {
	   (void) sprintf(ticlabel, text, CheckLog(log_x, place));
	   if ((*t->justify_text)(CENTRE)) {
		  (*t->put_text)(map_x(place),
					  ybot-(t->v_char), ticlabel);
	   } else {
		  (*t->put_text)(map_x(place)-(t->h_char)*strlen(ticlabel)/2,
					  ybot-(t->v_char), ticlabel);
	   }
    }
}
