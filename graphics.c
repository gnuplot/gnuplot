/*
 *
 *    G N U P L O T  --  graphics.c
 *
 *  Copyright (C) 1986, 1987  Thomas Williams, Colin Kelley
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#include <stdio.h>
#include <math.h>
#include "plot.h"

char *strcpy(),*strncpy(),*strcat();

extern BOOLEAN autoscale;
extern FILE *outfile;
extern BOOLEAN log_x, log_y;
extern int term;

extern BOOLEAN screen_ok;
extern BOOLEAN term_init;

extern struct termentry term_tbl[];


#ifndef max		/* Lattice C has max() in math.h, but shouldn't! */
#define max(a,b) ((a > b) ? a : b)
#endif

#define map_x(x) (int)((x-xmin)*xscale) /* maps floating point x to screen */ 
#define map_y(y) (int)((y-ymin)*yscale)	/* same for y */


double raise(x,y)
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
		tic = raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
		if (tic < 1.0)
			tic = 1.0;
	} else {
		xnorm = pow(10.0,l10-(double)((l10 >= 0.0 ) ? (int)l10 : ((int)l10-1)));
		if (xnorm <= 2)
			tics = 0.2;
		else if (xnorm <= 5)
			tics = 0.5;
		else tics = 1.0;	
		tic = tics * raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
	}
	return(tic);
}

char *idx(a,b)
char *a,b;
{
	do {
		if (*a == b)
			return(a);
	} while (*a++);
	return(0);
}

 
num2str(num,str)
double num;
char str[];
{
static char temp[80];
register double d;
register char *a,*b;

 	if ((d = fabs(num)) > 9999.0 || d < 0.001 && d != 0.0) 
		(void) sprintf(temp,"%-.3e",num);	
	else
		(void) sprintf(temp,"%-.3g",num);
	if (b = idx(temp,'e')) {
		a = b;
		while ( *(--a) == '0') /* trailing zeros */
			;	
		if ( *a == '.') 
			a--;
		(void) strncpy(str,temp,(int)(a-temp)+1);
		str[(int)(a-temp)+1] = '\0';
		a = b+1;	/* point to 1 after 'e' */
		(void) strcat(str,"e");
		if ( *a == '-') 
			(void) strcat(str,"-");
		a++;						 /* advance a past '+' or '-' */
		while ( *a == '0' && *(a+1) != '\0') /* leading zeroes */
			a++;
		(void) strcat(str,a); /* copy rest of string */
	}
	else
		(void) strcpy(str,temp);	
}


do_plot(plots, pcount, xmin, xmax, ymin, ymax)
struct curve_points *plots;
int pcount;			/* count of plots in linked list */
double xmin, xmax;
double ymin, ymax;
{
register int i, x;
register struct termentry *t = &term_tbl[term];
register BOOLEAN prev_undef;
register int curve, xaxis_y, yaxis_x, dpcount;
register struct curve_points *this_plot;
register enum PLOT_TYPE p_type;
register double xscale, yscale;
register double ytic, xtic, least, most, ticplace;
register int mms,mts;
			/* only a Pyramid would have this many registers! */
static char xns[20],xms[20],yns[20],yms[20],xts[20],yts[20];
static char label[80];

	if (ymin == HUGE || ymax == -HUGE)
		int_error("all points undefined!", NO_CARET);

	ytic = make_tics(ymin,ymax,log_y);
	xtic = make_tics(xmin,xmax,log_x);
	dpcount = 0;
	
	if (ymin < ymax ) {
		ymin = ytic * floor(ymin/ytic);	
		ymax = ytic * ceil(ymax/ytic);
	}
	else {
		ymin = ytic * ceil(ymin/ytic);
		ymax = ytic * floor(ymax/ytic);
	}

	if (xmin == xmax)
		int_error("xmin should not equal xmax!",NO_CARET);
	if (ymin == ymax)
		int_error("ymin should not equal ymax!",NO_CARET);
	
	yscale = (t->ymax - 2)/(ymax - ymin);
	xscale = (t->xmax - 2)/(xmax - xmin);
	
	if (!term_init) {
		(*t->init)();
		term_init = TRUE;
	}
	screen_ok = FALSE;
	(*t->graphics)();
	(*t->linetype)(-2); /* border linetype */

	/* draw plot border */
	(*t->move)(0,0);	
	(*t->vector)(t->xmax-1,0);	
	(*t->vector)(t->xmax-1,t->ymax-1);	
	(*t->vector)(0,t->ymax-1);	
	(*t->vector)(0,0);

	least = (ymin < ymax) ? ymin : ymax;
	most = (ymin < ymax) ? ymax : ymin;

	for (ticplace = ytic + least; ticplace < most ; ticplace += ytic) { 
		(*t->move)(0,map_y(ticplace));
		(*t->vector)(t->h_tic,map_y(ticplace));
		(*t->move)(t->xmax-1,map_y(ticplace));
       	        (*t->vector)(t->xmax-1-t->h_tic,map_y(ticplace));
	}

	if (xmin < xmax ) {
		least = xtic * floor(xmin/xtic);	
		most = xtic * ceil(xmax/xtic);
	}
	else {
		least = xtic * ceil(xmin/xtic);
		most = xtic * floor(xmax/xtic);
	}

	for (ticplace = xtic + least; ticplace < most ; ticplace += xtic) { 
		(*t->move)(map_x(ticplace),0);
		(*t->vector)(map_x(ticplace),t->v_tic);
		(*t->move)(map_x(ticplace),t->ymax-1);
       	        (*t->vector)(map_x(ticplace),t->ymax-1-t->v_tic);
	}

	if (log_x) {
		num2str(pow(10.0,xmin),xns);
		num2str(pow(10.0,xmax),xms);
		num2str(pow(10.0,xtic),xts);
	}
	else {
		num2str(xmin,xns);
		num2str(xmax,xms);
		num2str(xtic,xts);
	}
	if (log_y) {
		num2str(pow(10.0,ymin),yns);
		num2str(pow(10.0,ymax),yms);
		num2str(pow(10.0,ytic),yts);
	} else {
		num2str(ymin,yns);
		num2str(ymax,yms);
		num2str(ytic,yts);
	}
	mms = max(strlen(xms),strlen(yms));
	mts = max(strlen(xts),strlen(yts));

	(void) sprintf(label,"%s < y < %-*s  inc = %-*s",yns,mms,yms,mts,yts);
	(*t->lrput_text)(0, label);
	(void) sprintf(label,"%s < x < %-*s  inc = %-*s",xns,mms,xms,mts,xts);
	(*t->lrput_text)(1, label);


/* DRAW AXES */
	(*t->linetype)(-1);	/* axis line type */
	xaxis_y = map_y(0.0);
	yaxis_x = map_x(0.0); 

	if (xaxis_y < 0)
		xaxis_y = 0;				/* save for impulse plotting */
	else if (xaxis_y >= t->ymax)
		xaxis_y = t->ymax - 1;
	else if (!log_y) {
		(*t->move)(0,xaxis_y);
		(*t->vector)((t->xmax-1),xaxis_y);
	}

	if (!log_x && yaxis_x >= 0 && yaxis_x < t->xmax) {
		(*t->move)(yaxis_x,0);
		(*t->vector)(yaxis_x,(t->ymax-1));
	}

/* DRAW CURVES */
	this_plot = plots;
	for (curve = 0; curve < pcount; this_plot = this_plot->next_cp, curve++) {
		(*t->linetype)(curve);
		(*t->ulput_text)(curve, this_plot->title);
		(*t->linetype)(curve);

		p_type = this_plot->plot_type;
		switch(this_plot->plot_style) {
			case IMPULSES:
				for (i = 0; i < this_plot->p_count; i++) {
					if (!this_plot->points[i].undefined) {
						if (p_type == DATA)
							x = map_x(this_plot->points[i].x);
						else
							x = (long)(t->xmax-1)*i/(this_plot->p_count-1);
						(*t->move)(x,xaxis_y);
						(*t->vector)(x,map_y(this_plot->points[i].y));
					}
				}
				break;
			case LINES:
				prev_undef = TRUE;
				for (i = 0; i < this_plot->p_count; i++) {
					if (!this_plot->points[i].undefined) {
						if (p_type == DATA)
							x = map_x(this_plot->points[i].x);
						else
							x = (long)(t->xmax-1)*i/(this_plot->p_count-1);
						if (prev_undef)
							(*t->move)(x,
							map_y(this_plot->points[i].y));
						(*t->vector)(x,
							map_y(this_plot->points[i].y));
					}
					prev_undef = this_plot->points[i].undefined;
				}
				break;
			case POINTS:
				for (i = 0; i < this_plot->p_count; i++) {
					if (!this_plot->points[i].undefined) {
						if (p_type == DATA)
							x = map_x(this_plot->points[i].x);
						else
							x = (long)(t->xmax-1)*i/(this_plot->p_count-1);
						(*t->point)(x,map_y(this_plot->points[i].y),dpcount);
					}
				}
				dpcount++;
				break;

		}
	}
	(*t->text)();
	(void) fflush(outfile);
}
