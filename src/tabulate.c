#ifndef lint
static char *RCSid() { return RCSid("$Id: tabulate.c,v 1.17 2012/11/15 03:53:24 sfeam Exp $"); }
#endif

/* GNUPLOT - tabulate.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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
 * EAM - April 2007
 * Collect the routines for tablular output into a separate file, and
 * extend support to newer plot styles (labels, vectors, ...).
 * These routines used to masquerade as a terminal type "set term table",
 * but since version 4.2 they are controlled by "set table <foo>" independent
 * of terminal settings.
 */

#include "alloc.h"
#include "axis.h"
#include "datafile.h"
#include "gp_time.h"
#include "graphics.h"
#include "graph3d.h"
#include "plot.h"
#include "plot3d.h"
#include "tabulate.h"

static char *expand_newline __PROTO((const char *in));

static FILE *outfile;

/* This routine got longer than is reasonable for a macro */
#define OUTPUT_NUMBER(x,y) output_number(x,y,buffer)
#define BUFFERSIZE 150

static void
output_number(double coord, int axis, char *buffer) {
    /* treat timedata and "%s" output format as a special case:
     * return a number.
     * "%s" in combination with any other character is treated
     * like a normal time format specifier and handled in time.c
     */
    if (axis_array[axis].datatype == DT_TIMEDATE &&
	strcmp(axis_array[axis].formatstring, "%s") == 0) {
	gprintf(buffer, BUFFERSIZE, "%.0f", 1.0, coord);
    } else
    if (axis_array[axis].datatype == DT_TIMEDATE) {
	buffer[0] = '"';
	gstrftime(buffer+1, BUFFERSIZE-1, axis_array[axis].formatstring, coord);
	while (strchr(buffer,'\n')) {*(strchr(buffer,'\n')) = ' ';}
	strcat(buffer,"\"");
    } else if (axis_array[axis].log) {
	double x = pow(axis_array[axis].base, coord);
	gprintf(buffer, BUFFERSIZE, axis_array[axis].formatstring, 1.0, x);
    } else
	gprintf(buffer, BUFFERSIZE, axis_array[axis].formatstring, 1.0, coord);
    fputs(buffer, outfile);
    fputc(' ', outfile);
}


void
print_table(struct curve_points *current_plot, int plot_num)
{
    int i, curve;
    char *buffer = gp_alloc(BUFFERSIZE, "print_table: output buffer");
    outfile = (table_outfile) ? table_outfile : gpoutfile;

    for (curve = 0; curve < plot_num;
	 curve++, current_plot = current_plot->next) {
	struct coordinate GPHUGE *point = NULL;

	/* "with table" already wrote the output */
	if (current_plot->plot_style == TABLESTYLE)
	    continue;

	/* two blank lines between tabulated plots by prepending \n here */
	fprintf(outfile, "\n# Curve %d of %d, %d points",
		curve, plot_num, current_plot->p_count);

	if ((current_plot->title) && (*current_plot->title)) {
	    char *title = expand_newline(current_plot->title);
    	    fprintf(outfile, "\n# Curve title: \"%s\"", title);
	    free(title);
	}

	fprintf(outfile, "\n# x y");
	switch (current_plot->plot_style) {
	case BOXES:
	case XERRORBARS:
	    fputs(" xlow xhigh", outfile);
	    break;
	case BOXERROR:
	case YERRORBARS:
	    fputs(" ylow yhigh", outfile);
	    break;
	case BOXXYERROR:
	case XYERRORBARS:
	    fputs(" xlow xhigh ylow yhigh", outfile);
	    break;
	case FILLEDCURVES:
	    fputs("1 y2", outfile);
	    break;
	case FINANCEBARS:
	    fputs(" open ylow yhigh yclose", outfile);
	    break;
	case CANDLESTICKS:
	    fputs(" open ylow yhigh yclose width", outfile);
	    break;
	case LABELPOINTS:
	    fputs(" label",outfile);
	    break;
	case VECTOR:
	    fputs(" delta_x delta_y",outfile);
	    break;
	case LINES:
	case POINTSTYLE:
	case LINESPOINTS:
	case DOTS:
	case IMPULSES:
	case STEPS:
	case FSTEPS:
	case HISTEPS:
	    break;
	case IMAGE:
	    fputs("  pixel", outfile);
	    break;
	case RGBIMAGE:
	case RGBA_IMAGE:
	    fputs("  red green blue alpha", outfile);
	    break;

	default:
	    if (interactive)
		fprintf(stderr, "Tabular output of %s plot style not fully implemented\n",
		    current_plot->plot_style == HISTOGRAMS ? "histograms" :
		    "this");
	    break;
	}

	if (current_plot->varcolor)
	    fputs("  color", outfile);

	fputs(" type\n", outfile);

	if (current_plot->plot_style == LABELPOINTS) {
	    struct text_label *this_label;
	    for (this_label = current_plot->labels->next; this_label != NULL;
		 this_label = this_label->next) {
		 char *label = expand_newline(this_label->text);
		 OUTPUT_NUMBER(this_label->place.x, current_plot->x_axis);
		 OUTPUT_NUMBER(this_label->place.y, current_plot->y_axis);
		fprintf(outfile, " \"%s\"\n", label);
		free(label);
	    }

	} else {
	    int plotstyle = current_plot->plot_style;
	    if (plotstyle == HISTOGRAMS && current_plot->histogram->type == HT_ERRORBARS)
		plotstyle = YERRORBARS;

	    for (i = 0, point = current_plot->points; i < current_plot->p_count;
		i++, point++) {

		/* Reproduce blank lines read from original input file, if any */
		if (!memcmp(point, &blank_data_line, sizeof(struct coordinate))) {
		    fprintf(outfile, "\n");
		    continue;
		}

		/* FIXME HBB 20020405: had better use the real x/x2 axes of this plot */
		OUTPUT_NUMBER(point->x, current_plot->x_axis);
		OUTPUT_NUMBER(point->y, current_plot->y_axis);

		switch (plotstyle) {
		    case BOXES:
		    case XERRORBARS:
			OUTPUT_NUMBER(point->xlow, current_plot->x_axis);
			OUTPUT_NUMBER(point->xhigh, current_plot->x_axis);
			/* Hmmm... shouldn't this write out width field of box
			 * plots, too, if stored? */
			break;
		    case BOXXYERROR:
		    case XYERRORBARS:
			OUTPUT_NUMBER(point->xlow, current_plot->x_axis);
			OUTPUT_NUMBER(point->xhigh, current_plot->x_axis);
			/* FALLTHROUGH */
		    case BOXERROR:
		    case YERRORBARS:
			OUTPUT_NUMBER(point->ylow, current_plot->y_axis);
			OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
			break;
		    case IMAGE:
			fprintf(outfile,"%g ",point->z);
			break;
		    case RGBIMAGE:
		    case RGBA_IMAGE:
			fprintf(outfile,"%4d ",(int)point->CRD_R);
			fprintf(outfile,"%4d ",(int)point->CRD_G);
			fprintf(outfile,"%4d ",(int)point->CRD_B);
			fprintf(outfile,"%4d ",(int)point->CRD_A);
			break;
		    case FILLEDCURVES:
			OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
			break;
		    case FINANCEBARS:
			OUTPUT_NUMBER(point->ylow, current_plot->y_axis);
			OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
			OUTPUT_NUMBER(point->z, current_plot->y_axis);
			break;
		    case CANDLESTICKS:
			OUTPUT_NUMBER(point->ylow, current_plot->y_axis);
			OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
			OUTPUT_NUMBER(point->z, current_plot->y_axis);
			OUTPUT_NUMBER(2. * (point->x - point->xlow), current_plot->x_axis);
			break;
		    case VECTOR:
			OUTPUT_NUMBER((point->xhigh - point->x), current_plot->x_axis);
			OUTPUT_NUMBER((point->yhigh - point->y), current_plot->y_axis);
			break;
		    case LINES:
		    case POINTSTYLE:
		    case LINESPOINTS:
		    case DOTS:
		    case IMPULSES:
		    case STEPS:
		    case FSTEPS:
		    case HISTEPS:
			break;
		    default:
			/* ? */
			break;
		} /* switch(plot type) */

		if (current_plot->varcolor) {
		    double colorval = current_plot->varcolor[i];
		    if ((current_plot->lp_properties.pm3d_color.value < 0.0)
		    &&  (current_plot->lp_properties.pm3d_color.type == TC_RGB)) {
			fprintf(outfile, "0x%06x", (unsigned int)(colorval));
		    } else if (current_plot->lp_properties.pm3d_color.type == TC_Z) {
			OUTPUT_NUMBER(colorval, COLOR_AXIS);
		    } else if (current_plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
			OUTPUT_NUMBER(colorval, COLOR_AXIS);
		    }
		}

		fprintf(outfile, " %c\n",
		    current_plot->points[i].type == INRANGE
		    ? 'i' : current_plot->points[i].type == OUTRANGE
		    ? 'o' : 'u');
	    } /* for(point i) */
	}

	putc('\n', outfile);
    } /* for(curve) */

    fflush(outfile);
    free(buffer);
}


void
print_3dtable(int pcount)
{
    struct surface_points *this_plot;
    int i, surface;
    struct coordinate GPHUGE *point;
    struct coordinate GPHUGE *tail;
    char *buffer = gp_alloc(BUFFERSIZE, "print_3dtable output buffer");
    outfile = (table_outfile) ? table_outfile : gpoutfile;

    for (surface = 0, this_plot = first_3dplot;
	 surface < pcount;
	 this_plot = this_plot->next_sp, surface++) {
	fprintf(outfile, "\n# Surface %d of %d surfaces\n", surface, pcount);

	if ((this_plot->title) && (*this_plot->title)) {
	    char *title = expand_newline(this_plot->title);
    	    fprintf(outfile, "\n# Curve title: \"%s\"", title);
	    free(title);
	}

	switch (this_plot->plot_style) {
	case LABELPOINTS:
	    {
	    struct text_label *this_label;
	    for (this_label = this_plot->labels->next; this_label != NULL;
		 this_label = this_label->next) {
		 char *label = expand_newline(this_label->text);
		 OUTPUT_NUMBER(this_label->place.x, FIRST_X_AXIS);
		 OUTPUT_NUMBER(this_label->place.y, FIRST_Y_AXIS);
		 OUTPUT_NUMBER(this_label->place.z, FIRST_Z_AXIS);
		fprintf(outfile, " \"%s\"\n", label);
		free(label);
	    }
	    }
	    continue;
	case LINES:
	case POINTSTYLE:
	case IMPULSES:
	case DOTS:
	case VECTOR:
	case IMAGE:
	    break;
	default:
	    fprintf(stderr, "Tabular output of this 3D plot style not implemented\n");
	    continue;
	}

	if (draw_surface) {
	    struct iso_curve *icrvs;
	    int curve;

	    /* only the curves in one direction */
	    for (curve = 0, icrvs = this_plot->iso_crvs;
		 icrvs && curve < this_plot->num_iso_read;
		 icrvs = icrvs->next, curve++) {

		fprintf(outfile, "\n# IsoCurve %d, %d points\n# x y z",
			curve, icrvs->p_count);
		if (this_plot->plot_style == VECTOR) {
		    tail = icrvs->next->points;
		    fprintf(outfile, " delta_x delta_y delta_z");
		} else tail = NULL;  /* Just to shut up a compiler warning */

		fprintf(outfile, " type\n");

		for (i = 0, point = icrvs->points;
		     i < icrvs->p_count;
		     i++, point++) {
		    OUTPUT_NUMBER(point->x, FIRST_X_AXIS);
		    OUTPUT_NUMBER(point->y, FIRST_Y_AXIS);
		    OUTPUT_NUMBER(point->z, FIRST_Z_AXIS);
		    if (this_plot->plot_style == VECTOR) {
			OUTPUT_NUMBER((tail->x - point->x), FIRST_X_AXIS);
			OUTPUT_NUMBER((tail->y - point->y), FIRST_Y_AXIS);
			OUTPUT_NUMBER((tail->z - point->z), FIRST_Z_AXIS);
			tail++;
		    } else if (this_plot->plot_style == IMAGE) {
			fprintf(outfile,"%g ",point->CRD_COLOR);
		    }
		    fprintf(outfile, "%c\n",
			    point->type == INRANGE
			    ? 'i' : point->type == OUTRANGE
			    ? 'o' : 'u');
		} /* for(point) */
	    } /* for(icrvs) */
	    putc('\n', outfile);
	} /* if(draw_surface) */

	if (draw_contour) {
	    int number = 0;
	    struct gnuplot_contours *c = this_plot->contours;

	    while (c) {
		int count = c->num_pts;
		struct coordinate GPHUGE *point = c->coords;

		if (c->isNewLevel)
		    /* don't display count - contour split across chunks */
		    /* put # in case user wants to use it for a plot */
		    /* double blank line to allow plot ... index ... */
		    fprintf(outfile, "\n# Contour %d, label: %s\n",
			    number++, c->label);

		for (; --count >= 0; ++point) {
		    OUTPUT_NUMBER(point->x, FIRST_X_AXIS);
		    OUTPUT_NUMBER(point->y, FIRST_Y_AXIS);
		    OUTPUT_NUMBER(point->z, FIRST_Z_AXIS);
		    putc('\n', outfile);
		}

		/* blank line between segments of same contour */
		putc('\n', outfile);
		c = c->next;

	    } /* while (contour) */
	} /* if (draw_contour) */
    } /* for(surface) */
    fflush(outfile);

    free(buffer);
}

static char *
expand_newline(const char *in)
{
    char *tmpstr = gp_alloc(2*strlen(in),"enl");
    const char *s = in;
    char *t = tmpstr;
    do {
	if (*s == '\n') {
	    *t++ = '\\';
	    *t++ = 'n';
	} else
	    *t++ = *s;
    } while (*s++);
    return tmpstr;
}
