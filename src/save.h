/* GNUPLOT - save.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_SAVE_H
# define GNUPLOT_SAVE_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "stdfn.h"

#include "axis.h"

/* Type definitions */

/* Variables of save.c needed by other modules: */
extern const char *coord_msg[];

/* Prototypes of functions exported by save.c */
void save_functions(FILE *fp);
void save_variables(FILE *fp);
void save_set(FILE *fp);
void save_term(FILE *fp);
void save_all(FILE *fp);
void save_axis_label_or_title(FILE *fp, char *name, char *suffix,
			struct text_label *label, TBOOLEAN savejust);
void save_position(FILE *, struct position *, int, TBOOLEAN);
void save_prange(FILE *, struct axis *);
void save_link(FILE *, struct axis *);
void save_nonlinear(FILE *, struct axis *);
void save_textcolor(FILE *, const struct t_colorspec *);
void save_pm3dcolor(FILE *, const struct t_colorspec *);
void save_fillstyle(FILE *, const struct fill_style_type *);
void save_offsets(FILE *, char *);
void save_histogram_opts(FILE *fp);
void save_pixmaps(FILE *fp);
void save_object(FILE *, int);
void save_walls(FILE *);
void save_style_textbox(FILE *);
void save_style_parallel(FILE *);
void save_style_spider(FILE *);
void save_data_func_style(FILE *, const char *, enum PLOT_STYLE);
void save_linetype(FILE *, lp_style_type *, TBOOLEAN);
void save_dashtype(FILE *, int, const t_dashtype *);
void save_num_or_time_input(FILE *, double x, struct axis *);
void save_axis_format(FILE *fp, AXIS_INDEX axis);
void save_bars(FILE *);
void save_array_content(FILE *, struct value *);

#endif /* GNUPLOT_SAVE_H */
