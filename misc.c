#ifndef lint
static char *RCSid = "$Id: misc.c%v 3.50 1993/07/09 05:35:24 woo Exp $";
#endif


/* GNUPLOT - misc.c */
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
 */

#include <stdio.h>
#include <math.h>
#ifndef __PUREC__
#ifndef VMS
#ifdef AMIGA_AC_5
#include <exec/types.h>
#else
#include <sys/types.h>
#endif /* AMIGA_AC_5 */
#else
#include <types.h>
#endif
#endif /* __PUREC__ */
#include "plot.h"
#include "setshow.h"
#ifndef _Windows
#include "help.h"
#endif

#if !defined(__TURBOC__) && !defined (_IBMR2) && !defined (ATARI) && !defined (AMIGA_SC_6_1) && !defined(OSF1)
extern char *malloc();
extern char *realloc();
#endif

#if defined(__TURBOC__) && (defined(MSDOS) || defined(_Windows))
#include <alloc.h>		/* for malloc, farmalloc, farrealloc */
#endif
#if defined(sequent)
#include <sys/types.h>          /* typedef long size_t; */
#endif

extern int c_token;
extern char replot_line[];
extern struct at_type at;
extern struct ft_entry ft[];
extern struct udft_entry *first_udf;
extern struct udvt_entry *first_udv;

extern struct at_type *temp_at();

extern TBOOLEAN interactive;
extern char *infile_name;
extern int inline_num;

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
typedef struct lf_state_struct LFS;
struct lf_state_struct {
    FILE *fp;				/* file pointer for load file */
    char *name;			/* name of file */
    TBOOLEAN interactive;		/* value of interactive flag on entry */
    int inline_num;			/* inline_num on entry */
    LFS *prev;				/* defines a stack */
} *lf_head = NULL;			/* NULL if not in load_file */

static TBOOLEAN lf_pop();
static void lf_push();
FILE *lf_top();


/*
 * instead of <strings.h>
 */
#ifdef _Windows
#include <string.h>
#else
#ifndef AMIGA_SC_6_1
extern int strcmp();
#endif /* !AMIGA_SC_6_1 */
#endif

/* alloc:
 * allocate memory 
 * This is a protected version of malloc. It causes an int_error 
 * if there is not enough memory, but first it tries FreeHelp() 
 * to make some room, and tries again. If message is NULL, we 
 * allow NULL return. Otherwise, we handle the error, using the
 * message to create the int_error string. Note cp/sp_extend uses realloc,
 * so it depends on this using malloc().
 */

char *
alloc(size, message)
	unsigned long size;		/* # of bytes */
	char *message;			/* description of what is being allocated */
{
    char *p;				/* the new allocation */
    char errbuf[100];		/* error message string */

    p = malloc((size_t)size);
    if (p == (char *)NULL) {
#ifndef vms
#ifndef _Windows
	   FreeHelp();			/* out of memory, try to make some room */
#endif
#endif
	   p = malloc((size_t)size);		/* try again */
	   if (p == (char *)NULL) {
		  /* really out of memory */
		  if (message != NULL) {
			 (void) sprintf(errbuf, "out of memory for %s", message);
			 int_error(errbuf, NO_CARET);
			 /* NOTREACHED */
		  }
		  /* else we return NULL */
	   }
    }
    return(p);
}

/*
 * cp_alloc() allocates a curve_points structure that can hold 'num'
 * points.
 */
struct curve_points *
cp_alloc(num)
	int num;
{
    struct curve_points *cp;
    cp = (struct curve_points *) alloc((unsigned long)sizeof(struct curve_points), "curve");
    cp->p_max = (num >= 0 ? num : 0);
    if (num > 0) {
	   cp->points = (struct coordinate GPHUGE *)
		gpfaralloc((unsigned long)num * sizeof(struct coordinate), "curve points");
    } else
	   cp->points = (struct coordinate GPHUGE *) NULL;
    cp->next_cp = NULL;
    cp->title = NULL;
    return(cp);
}


/*
 * cp_extend() reallocates a curve_points structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
cp_extend(cp, num)
	struct curve_points *cp;
	int num;
{
    struct coordinate GPHUGE *new;

#if (defined(MSDOS) || defined(_Windows))  &&  !defined(WIN32)
    /* Make sure we do not allocate more than 64k points in msdos since 
	 * indexing is done with 16-bit int
	 * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
		int_error("Array index must be less than 32k in msdos", NO_CARET);
#endif /* MSDOS */

    if (num == cp->p_max) return(0);

    if (num > 0) {
	   if (cp->points == NULL) {
		  cp->points = (struct coordinate GPHUGE *)
		    gpfaralloc((unsigned long)num * sizeof(struct coordinate), "curve points");
	   } else {
		  new = (struct coordinate GPHUGE *)
		    gpfarrealloc(cp->points, (unsigned long)num * sizeof(struct coordinate));
		  if (new == (struct coordinate GPHUGE *) NULL) {
			 int_error("No memory available for expanding curve points",
					 NO_CARET);
			 /* NOTREACHED */
		  }
		  cp->points = new;
	   }
	   cp->p_max = num;
    } else {
	   if (cp->points != (struct coordinate GPHUGE *) NULL)
		gpfarfree(cp->points);
	   cp->points = (struct coordinate GPHUGE *) NULL;
	   cp->p_max = 0;
    }
}

/*
 * cp_free() releases any memory which was previously malloc()'d to hold
 *   curve points (and recursively down the linked list).
 */
cp_free(cp)
struct curve_points *cp;
{
	if (cp) {
		cp_free(cp->next_cp);
		if (cp->title)
			free((char *)cp->title);
		if (cp->points)
			gpfarfree((char *)cp->points);
		free((char *)cp);
	}
}

/*
 * iso_alloc() allocates a iso_curve structure that can hold 'num'
 * points.
 */
struct iso_curve *
iso_alloc(num)
	int num;
{
    struct iso_curve *ip;
    ip = (struct iso_curve *) alloc((unsigned long)sizeof(struct iso_curve), "iso curve");
    ip->p_max = (num >= 0 ? num : 0);
    if (num > 0) {
	   ip->points = (struct coordinate GPHUGE *)
		gpfaralloc((unsigned long)num * sizeof(struct coordinate), "iso curve points");
    } else
	   ip->points = (struct coordinate GPHUGE *) NULL;
    ip->next = NULL;
    return(ip);
}

/*
 * iso_extend() reallocates a iso_curve structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
iso_extend(ip, num)
	struct iso_curve *ip;
	int num;
{
    struct coordinate GPHUGE *new;

    if (num == ip->p_max) return(0);

#if (defined(MSDOS) || defined(_Windows))  &&  !defined(WIN32)
    /* Make sure we do not allocate more than 64k points in msdos since 
	 * indexing is done with 16-bit int
	 * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
		int_error("Array index must be less than 32k in msdos", NO_CARET);
#endif /* MSDOS */

    if (num > 0) {
	   if (ip->points == NULL) {
		  ip->points = (struct coordinate GPHUGE *)
		    gpfaralloc((unsigned long)num * sizeof(struct coordinate), "iso curve points");
	   } else {
		  new = (struct coordinate GPHUGE *)
		    gpfarrealloc(ip->points, (unsigned long)num * sizeof(struct coordinate));
		  if (new == (struct coordinate GPHUGE *) NULL) {
			 int_error("No memory available for expanding curve points",
					 NO_CARET);
			 /* NOTREACHED */
		  }
		  ip->points = new;
	   }
	   ip->p_max = num;
    } else {
	   if (ip->points != (struct coordinate GPHUGE *) NULL)
		gpfarfree(ip->points);
	   ip->points = (struct coordinate GPHUGE *) NULL;
	   ip->p_max = 0;
    }
}

/*
 * iso_free() releases any memory which was previously malloc()'d to hold
 *   iso curve points.
 */
iso_free(ip)
struct iso_curve *ip;
{
	if (ip) {
		if (ip->points)
			gpfarfree((char *)ip->points);
		free((char *)ip);
	}
}

/*
 * sp_alloc() allocates a surface_points structure that can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
struct surface_points *
sp_alloc(num_samp_1,num_iso_1,num_samp_2,num_iso_2)
	int num_samp_1,num_iso_1,num_samp_2,num_iso_2;
{
    struct surface_points *sp;

    sp = (struct surface_points *) alloc((unsigned long)sizeof(struct surface_points), "surface");
    sp->next_sp = NULL;
    sp->title = NULL;
    sp->contours = NULL;
    sp->iso_crvs = NULL;
    sp->num_iso_read = 0;

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	int i;
	struct iso_curve *icrv;

	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = (struct iso_curve *) NULL;

    return(sp);
}

/*
 * sp_replace() updates a surface_points structure so it can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
sp_replace(sp,num_samp_1,num_iso_1,num_samp_2,num_iso_2)
	   struct surface_points *sp;
	   int num_samp_1,num_iso_1,num_samp_2,num_iso_2;
{
    int i;
    struct iso_curve *icrv, *icrvs = sp->iso_crvs;

    while ( icrvs ) {
	icrv = icrvs;
	icrvs = icrvs->next;
	iso_free( icrv );
    }
    sp->iso_crvs = NULL;
    
    if (num_iso_2 > 0 && num_samp_1 > 0) {
	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = (struct iso_curve *) NULL;
}

/*
 * sp_free() releases any memory which was previously malloc()'d to hold
 *   surface points.
 */
sp_free(sp)
struct surface_points *sp;
{
	if (sp) {
		sp_free(sp->next_sp);
		if (sp->title)
			free((char *)sp->title);
		if (sp->contours) {
			struct gnuplot_contours *cntr, *cntrs = sp->contours;

			while (cntrs) {
				cntr = cntrs;
				cntrs = cntrs->next;
				gpfarfree(cntr->coords);
				free(cntr);
			}
		}
		if (sp->iso_crvs) {
			struct iso_curve *icrv, *icrvs = sp->iso_crvs;

			while (icrvs) {
				icrv = icrvs;
				icrvs = icrvs->next;
				iso_free(icrv);
			}
		}
		free((char *)sp);
	}
}



save_functions(fp)
FILE *fp;
{
register struct udft_entry *udf = first_udf;

	if (fp) {
		while (udf) {
			if (udf->definition)
				fprintf(fp,"%s\n",udf->definition);
			udf = udf->next_udf;
		}
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


save_variables(fp)
FILE *fp;
{
register struct udvt_entry *udv = first_udv->next_udv;	/* skip pi */

	if (fp) {
		while (udv) {
			if (!udv->udv_undef) {
				fprintf(fp,"%s = ",udv->udv_name);
				disp_value(fp,&(udv->udv_value));
				(void) putc('\n',fp);
			}
			udv = udv->next_udv;
		}
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


save_all(fp)
FILE *fp;
{
register struct udft_entry *udf = first_udf;
register struct udvt_entry *udv = first_udv->next_udv;	/* skip pi */

	if (fp) {
		save_set_all(fp);
		while (udf) {
			if (udf->definition)
				fprintf(fp,"%s\n",udf->definition);
			udf = udf->next_udf;
		}
		while (udv) {
			if (!udv->udv_undef) {
				fprintf(fp,"%s = ",udv->udv_name);
				disp_value(fp,&(udv->udv_value));
				(void) putc('\n',fp);
			}
			udv = udv->next_udv;
		}
		fprintf(fp,"%s\n",replot_line);
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


save_set(fp)
FILE *fp;
{
	if (fp) {
		save_set_all(fp);
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


save_set_all(fp)
FILE *fp;
{
struct text_label *this_label;
struct arrow_def *this_arrow;
char *quote;
	fprintf(fp,"set terminal %s %s\n", term_tbl[term].name, term_options);
	fprintf(fp,"set output %s\n",strcmp(outstr,"STDOUT")? outstr : "" );
	fprintf(fp,"set %sclip points\n", (clip_points)? "" : "no");
	fprintf(fp,"set %sclip one\n", (clip_lines1)? "" : "no");
	fprintf(fp,"set %sclip two\n", (clip_lines2)? "" : "no");
	fprintf(fp,"set %sborder\n",draw_border ? "" : "no");
	if (boxwidth<0.0)
		fprintf(fp,"set boxwidth\n");
	else
		fprintf(fp,"set boxwidth %g\n",boxwidth);
	if (dgrid3d)
		fprintf(fp,"set dgrid3d %d,%d, %d\n",
			dgrid3d_row_fineness,
			dgrid3d_col_fineness,
			dgrid3d_norm_value);

	fprintf(fp,"set dummy %s,%s\n",dummy_var[0], dummy_var[1]);
	fprintf(fp,"set format x \"%s\"\n", xformat);
	fprintf(fp,"set format y \"%s\"\n", yformat);
	fprintf(fp,"set format z \"%s\"\n", zformat);
	fprintf(fp,"set %sgrid\n", (grid)? "" : "no");
	switch (key) {
		case -1 :
			fprintf(fp,"set key\n");
			break;
		case 0 :
			fprintf(fp,"set nokey\n");
			break;
		case 1 :
			fprintf(fp,"set key %g,%g,%g\n",key_x,key_y,key_z);
			break;
	}
	fprintf(fp,"set nolabel\n");
	for (this_label = first_label; this_label != NULL;
			this_label = this_label->next) {
		fprintf(fp,"set label %d \"%s\" at %g,%g,%g ",
			   this_label->tag,
			   this_label->text, this_label->x,
						 this_label->y,
						 this_label->z);
		switch(this_label->pos) {
			case LEFT :
				fprintf(fp,"left");
				break;
			case CENTRE :
				fprintf(fp,"centre");
				break;
			case RIGHT :
				fprintf(fp,"right");
				break;
		}
		fputc('\n',fp);
	}
	fprintf(fp,"set noarrow\n");
	for (this_arrow = first_arrow; this_arrow != NULL;
			this_arrow = this_arrow->next) {
		fprintf(fp,"set arrow %d from %g,%g,%g to %g,%g,%g%s\n",
			   this_arrow->tag,
			   this_arrow->sx, this_arrow->sy, this_arrow->sz,
			   this_arrow->ex, this_arrow->ey, this_arrow->ez,
			   this_arrow->head ? "" : " nohead");
	}
	fprintf(fp,"set nologscale\n");
	if (is_log_x) fprintf(fp,"set logscale x %g\n", base_log_x);
	if (is_log_y) fprintf(fp,"set logscale y %g\n", base_log_y);
	if (is_log_z) fprintf(fp,"set logscale z %g\n", base_log_z);
	fprintf(fp,"set offsets %g, %g, %g, %g\n",loff,roff,toff,boff);
	fprintf(fp,"set %spolar\n", (polar)? "" : "no");
	fprintf(fp,"set angles %s\n", (angles_format == ANGLES_RADIANS)?
						"radians" : "degrees");
	fprintf(fp,"set %sparametric\n", (parametric)? "" : "no");
	fprintf(fp,"set view %g, %g, %g, %g\n",
		surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
	fprintf(fp,"set samples %d, %d\n",samples_1,samples_2);
	fprintf(fp,"set isosamples %d, %d\n",iso_samples_1,iso_samples_2);
	fprintf(fp,"set %ssurface\n",(draw_surface) ? "" : "no");
	fprintf(fp,"set %scontour",(draw_contour) ? "" : "no");
	switch (draw_contour) {
		case CONTOUR_NONE: fprintf(fp, "\n"); break;
		case CONTOUR_BASE: fprintf(fp, " base\n"); break;
		case CONTOUR_SRF:  fprintf(fp, " surface\n"); break;
		case CONTOUR_BOTH: fprintf(fp, " both\n"); break;
	}
 	fprintf(fp,"set %sclabel\n",(label_contours) ? "" : "no");
	fprintf(fp,"set %shidden3d\n",(hidden3d) ? "" : "no");
	fprintf(fp,"set cntrparam order %d\n", contour_order);
	fprintf(fp,"set cntrparam ");
	switch (contour_kind) {
		case CONTOUR_KIND_LINEAR:    fprintf(fp, "linear\n"); break;
		case CONTOUR_KIND_CUBIC_SPL: fprintf(fp, "cubicspline\n"); break;
		case CONTOUR_KIND_BSPLINE:   fprintf(fp, "bspline\n"); break;
	}
	fprintf(fp,"set cntrparam levels ");
	switch (levels_kind) {
		int i;
		case LEVELS_AUTO:
			fprintf(fp, "auto %d\n", contour_levels);
			break;
		case LEVELS_INCREMENTAL:
			fprintf(fp, "incremental %g,%g,%g\n",
				levels_list[0], levels_list[1], 
				levels_list[0]+levels_list[1]*contour_levels);
			break;
		case LEVELS_DISCRETE:
			fprintf(fp, "discrete ");
		        fprintf(fp, "%g", levels_list[0]);
			for(i = 1; i < contour_levels; i++)
				fprintf(fp, ",%g ", levels_list[i]);
			fprintf(fp, "\n");
	}
	fprintf(fp,"set cntrparam points %d\n", contour_pts);
	fprintf(fp,"set size %g,%g\n",xsize,ysize);
	fprintf(fp,"set data style ");
	switch (data_style) {
		case LINES: fprintf(fp,"lines\n"); break;
		case POINTSTYLE: fprintf(fp,"points\n"); break;
		case IMPULSES: fprintf(fp,"impulses\n"); break;
		case LINESPOINTS: fprintf(fp,"linespoints\n"); break;
		case DOTS: fprintf(fp,"dots\n"); break;
		case ERRORBARS: fprintf(fp,"errorbars\n"); break;
		case BOXES: fprintf(fp,"boxes\n"); break;
		case BOXERROR: fprintf(fp,"boxerrorbars\n"); break;
		case STEPS: fprintf(fp,"steps\n"); break;       /* JG */
	}
	fprintf(fp,"set function style ");
	switch (func_style) {
		case LINES: fprintf(fp,"lines\n"); break;
		case POINTSTYLE: fprintf(fp,"points\n"); break;
		case IMPULSES: fprintf(fp,"impulses\n"); break;
		case LINESPOINTS: fprintf(fp,"linespoints\n"); break;
		case DOTS: fprintf(fp,"dots\n"); break;
		case ERRORBARS: fprintf(fp,"errorbars\n"); break;
		case BOXES: fprintf(fp,"boxes\n"); break;
		case BOXERROR: fprintf(fp,"boxerrorbars\n"); break;
		case STEPS: fprintf(fp,"steps\n"); break;       /* JG */
	}
        fprintf(fp,"set %sxzeroaxis\n", (xzeroaxis)? "" : "no");
        fprintf(fp,"set %syzeroaxis\n", (yzeroaxis)? "" : "no");
        fprintf(fp,"set tics %s\n", (tic_in)? "in" : "out");
        fprintf(fp,"set ticslevel %g\n", ticslevel);
        save_tics(fp, xtics, 'x', &xticdef);
        save_tics(fp, ytics, 'y', &yticdef);
        save_tics(fp, ztics, 'z', &zticdef);
        for (quote = &(title[0]); quote && *quote && (*quote != '"'); quote++)
            ;
        fprintf(fp,"set title %c%s%c %d,%d\n",*quote ? '\'' : '"',title,*quote ? '\'' : '"',title_xoffset,title_yoffset);
	
        if (timedate)
	    fprintf(fp,"set time %d,%d\n",time_xoffset,time_yoffset);
	else
	    fprintf(fp,"set notime\n");
 	fprintf(fp,"set rrange [%g : %g]\n",rmin,rmax);
	fprintf(fp,"set trange [%g : %g]\n",tmin,tmax);
	fprintf(fp,"set urange [%g : %g]\n",umin,umax);
	fprintf(fp,"set vrange [%g : %g]\n",vmin,vmax);
        for (quote = &(xlabel[0]); quote && *quote && (*quote != '"'); quote++)
            ;
        fprintf(fp,"set xlabel %c%s%c %d,%d\n",*quote ? '\'' : '"',xlabel,*quote ? '\'' : '"',xlabel_xoffset,xlabel_yoffset);
        fprintf(fp,"set xrange [%g : %g]\n",xmin,xmax);
        for (quote = &(ylabel[0]); quote && *quote && (*quote != '"'); quote++)
            ;
        fprintf(fp,"set ylabel %c%s%c %d,%d\n",*quote ? '\'' : '"',ylabel,*quote ? '\'' : '"',ylabel_xoffset,ylabel_yoffset);
        fprintf(fp,"set yrange [%g : %g]\n",ymin,ymax);
        for (quote = &(zlabel[0]); quote && *quote && (*quote != '"'); quote++)
            ;
        fprintf(fp,"set zlabel %c%s%c %d,%d\n",*quote ? '\'' : '"',zlabel,*quote ? '\'' : '"',zlabel_xoffset,zlabel_yoffset);
	fprintf(fp,"set zrange [%g : %g]\n",zmin,zmax);
	fprintf(fp,"set %s %c\n", 
		autoscale_r ? "autoscale" : "noautoscale", 'r');
	fprintf(fp,"set %s %c\n", 
		autoscale_t ? "autoscale" : "noautoscale", 't');
	fprintf(fp,"set %s %c%c\n", 
		(autoscale_y||autoscale_x) ? "autoscale" : "noautoscale", 
		autoscale_x ? 'x' : ' ', autoscale_y ? 'y' : ' ');
	fprintf(fp,"set %s %c\n", 
		autoscale_z ? "autoscale" : "noautoscale", 'z');
	fprintf(fp,"set zero %g\n",zero);
}

save_tics(fp, onoff, axis, tdef)
	FILE *fp;
	TBOOLEAN onoff;
	char axis;
	struct ticdef *tdef;
{
    if (onoff) {
	   fprintf(fp,"set %ctics", axis);
	   switch(tdef->type) {
		  case TIC_COMPUTED: {
			 break;
		  }
		  case TIC_MONTH:{
		      fprintf(fp,"\nset %cmtics",axis);
			break;
		  }
		    case TIC_DAY:{
			fprintf(fp,"\nset %cdtics",axis);
			break;
		    }
		  case TIC_SERIES: {
		         if (tdef->def.series.end >= VERYLARGE)
			     fprintf(fp, " %g,%g", tdef->def.series.start,
                                        tdef->def.series.incr);
                         else
			     fprintf(fp, " %g,%g,%g", tdef->def.series.start,
                                        tdef->def.series.incr, tdef->def.series.end);
			 break;
		  }
		  case TIC_USER: {
			 register struct ticmark *t;
			 fprintf(fp, " (");
			 for (t = tdef->def.user; t != NULL; t=t->next) {
				if (t->label)
				  fprintf(fp, "\"%s\" ", t->label);
				if (t->next)
				  fprintf(fp, "%g, ", t->position);
				else
				  fprintf(fp, "%g", t->position);
			 }
			 fprintf(fp, ")");
			 break;
		  } 
	   }
	   fprintf(fp, "\n");
    } else {
	   fprintf(fp,"set no%ctics\n", axis);
    }
}

load_file(fp, name)
	FILE *fp;
	char *name;
{
    register int len;
    extern char input_line[];

    int start, left;
    int more;
    int stop = FALSE;

    lf_push(fp);			/* save state for errors and recursion */

    if (fp == (FILE *)NULL) {
	   char errbuf[BUFSIZ];
	   (void) sprintf(errbuf, "Cannot open load file '%s'", name);
	   os_error(errbuf, c_token);
    } else {
	   /* go into non-interactive mode during load */
	   /* will be undone below, or in load_file_error */
	   interactive = FALSE;
	   inline_num = 0;
	   infile_name = name;

	   while (!stop) {		/* read all commands in file */
		  /* read one command */
		  left = MAX_LINE_LEN;
		  start = 0;
		  more = TRUE;

		  while (more) {
			 if (fgets(&(input_line[start]), left, fp) == (char *)NULL) {
				stop = TRUE; /* EOF in file */
				input_line[start] = '\0';
				more = FALSE;	
			 } else {
				inline_num++;
				len = strlen(input_line) - 1;
				if (input_line[len] == '\n') { /* remove any newline */
				    input_line[len] = '\0';
				    /* Look, len was 1-1 = 0 before, take care here! */
				    if (len > 0) --len;
				} else if (len+1 >= left)
				  int_error("Input line too long",NO_CARET);
				 
				if (input_line[len] == '\\') { /* line continuation */
				    start = len;
				    left  = MAX_LINE_LEN - start; /* left -=len;*/
				} else
				  more = FALSE;
			 }
		  }

		  if (strlen(input_line) > 0) {
			 screen_ok = FALSE;	/* make sure command line is
							   echoed on error */
			 do_line();
		  }
	   }
    }

    /* pop state */
    (void) lf_pop();		/* also closes file fp */
}

/* pop from load_file state stack */
static TBOOLEAN				/* FALSE if stack was empty */
lf_pop()					/* called by load_file and load_file_error */
{
    LFS *lf;

    if (lf_head == NULL)
	 return(FALSE);
    else {
	   lf = lf_head;
	   if (lf->fp != (FILE *)NULL)
		(void) fclose(lf->fp);
	   interactive = lf->interactive;
	   inline_num = lf->inline_num;
	   infile_name = lf->name;
	   lf_head = lf->prev;
	   free((char *)lf);
	   return(TRUE);
    }
}

/* push onto load_file state stack */
/* essentially, we save information needed to undo the load_file changes */
static void
lf_push(fp)			/* called by load_file */
	FILE *fp;
{
    LFS *lf;
    
    lf = (LFS *)alloc((unsigned long)sizeof(LFS), (char *)NULL);
    if (lf == (LFS *)NULL) {
	   if (fp != (FILE *)NULL)
		(void) fclose(fp);		/* it won't be otherwise */
	   int_error("not enough memory to load file", c_token);
    }
	 
    lf->fp = fp;			/* save this file pointer */
    lf->name = infile_name;	/* save current name */
    lf->interactive = interactive;	/* save current state */
    lf->inline_num = inline_num; /* save current line number */
    lf->prev = lf_head;		/* link to stack */
    lf_head = lf;
}

FILE *lf_top()		/* used for reread  vsnyder@math.jpl.nasa.gov */
{   if (lf_head == (LFS *) NULL) return((FILE *)NULL);
    return(lf_head->fp);
}


load_file_error()			/* called from main */
{
    /* clean up from error in load_file */
    /* pop off everything on stack */
    while(lf_pop())
	 ;
}

/* find char c in string str; return p such that str[p]==c;
 * if c not in str then p=strlen(str)
 */
int
instring(str, c)
	char *str;
	char c;
{
    int pos = 0;

    while (str != NULL && *str != '\0' && c != *str) {
	   str++; 
	   pos++;
    }
    return (pos);
}

show_functions()
{
register struct udft_entry *udf = first_udf;

	fprintf(stderr,"\n\tUser-Defined Functions:\n");

	while (udf) {
		if (udf->definition)
			fprintf(stderr,"\t%s\n",udf->definition);
		else
			fprintf(stderr,"\t%s is undefined\n",udf->udf_name);
		udf = udf->next_udf;
	}
}


show_at()
{
	(void) putc('\n',stderr);
	disp_at(temp_at(),0);
}


disp_at(curr_at, level)
struct at_type *curr_at;
int level;
{
register int i, j;
register union argument *arg;

	for (i = 0; i < curr_at->a_count; i++) {
		(void) putc('\t',stderr);
		for (j = 0; j < level; j++)
			(void) putc(' ',stderr);	/* indent */

			/* print name of instruction */

		fputs(ft[(int)(curr_at->actions[i].index)].f_name,stderr);
		arg = &(curr_at->actions[i].arg);

			/* now print optional argument */

		switch(curr_at->actions[i].index) {
		  case PUSH:	fprintf(stderr," %s\n", arg->udv_arg->udv_name);
					break;
		  case PUSHC:	(void) putc(' ',stderr);
					disp_value(stderr,&(arg->v_arg));
					(void) putc('\n',stderr);
					break;
		  case PUSHD1:	fprintf(stderr," %c dummy\n",
					  arg->udf_arg->udf_name[0]);
					break;
		  case PUSHD2:	fprintf(stderr," %c dummy\n",
					  arg->udf_arg->udf_name[1]);
					break;
		  case CALL:	fprintf(stderr," %s", arg->udf_arg->udf_name);
					if(level < 6) {
					if (arg->udf_arg->at) {
						(void) putc('\n',stderr);
						disp_at(arg->udf_arg->at,level+2); /* recurse! */
					} else
						fputs(" (undefined)\n",stderr);
					} else
						(void) putc('\n',stderr);
					break;
		  case CALLN:	fprintf(stderr," %s", arg->udf_arg->udf_name);
					if(level < 6) {
					if (arg->udf_arg->at) {
						(void) putc('\n',stderr);
						disp_at(arg->udf_arg->at,level+2); /* recurse! */
					} else
						fputs(" (undefined)\n",stderr);
					} else
						(void) putc('\n',stderr);
					break;
		  case JUMP:
		  case JUMPZ:
		  case JUMPNZ:
		  case JTERN:
					fprintf(stderr," +%d\n",arg->j_arg);
					break;
		  default:
					(void) putc('\n',stderr);
		}
	}
}
