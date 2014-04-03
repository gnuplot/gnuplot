/*
 * $Id: tabulate.h,v 1.1 2007/04/27 20:35:45 sfeam Exp $
 */

/* GNUPLOT - tabulate.h */

#ifndef GNUPLOT_TABULATE_H
# define GNUPLOT_TABULATE_H

#include "syscfg.h"

/* Routines in tabulate.c needed by other modules: */

void print_table __PROTO((struct curve_points * first_plot, int plot_num));
void print_3dtable __PROTO((int pcount));

extern FILE *table_outfile;
extern udvt_entry *table_var;
extern TBOOLEAN table_mode;

#endif /* GNUPLOT_TABULATE_H */
