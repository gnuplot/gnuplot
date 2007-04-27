/*
 * $Id: tabulate.h,v 1.1 2007/04/26 20:35:35 sfeam Exp $
 */

/* GNUPLOT - tabulate.h */

#ifndef GNUPLOT_TABULATE_H
# define GNUPLOT_TABULATE_H

#include "syscfg.h"

/* Routines in tabulate.c needed by other modules: */

void print_table __PROTO((struct curve_points * first_plot, int plot_num));
void print_3dtable __PROTO((int pcount));


#endif /* GNUPLOT_TABULATE_H */
