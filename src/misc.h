/*
 * $Id: misc.h,v 1.25 2008/03/31 01:47:58 sfeam Exp $
 */

/* GNUPLOT - misc.h */

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

#ifndef GNUPLOT_MISC_H
# define GNUPLOT_MISC_H

#include "syscfg.h"
#include "gp_types.h"
#include "stdfn.h"

#include "graphics.h"
#include "graph3d.h"


/* Variables of misc.c needed by other modules: */

extern char *infile_name;

/* Prototypes from file "misc.c" */

struct iso_curve * iso_alloc __PROTO((int num));
void iso_extend __PROTO((struct iso_curve *ip, int num));
void iso_free __PROTO((struct iso_curve *ip));
void load_file __PROTO((FILE *fp, char *name, TBOOLEAN subst_args));
FILE *lf_top __PROTO((void));
void load_file_error __PROTO((void));
FILE *loadpath_fopen __PROTO((const char *, const char *));
char *fontpath_fullname __PROTO((const char *));
void push_terminal __PROTO((int is_interactive));
void pop_terminal __PROTO((void));

/* moved here, from setshow */
enum PLOT_STYLE get_style __PROTO((void));
void get_filledcurves_style_options __PROTO((filledcurves_opts *));
void filledcurves_options_tofile __PROTO((filledcurves_opts *, FILE *));
void lp_parse __PROTO((struct lp_style_type *, TBOOLEAN, TBOOLEAN));
void lp_use_properties __PROTO((struct lp_style_type *lp, int tag));

void arrow_parse __PROTO((struct arrow_style_type *, TBOOLEAN));

void parse_fillstyle __PROTO((struct fill_style_type *fs, int def_style,
                              int def_density, int def_pattern, int def_border ));
void parse_colorspec __PROTO((struct t_colorspec *tc, int option));
TBOOLEAN need_fill_border __PROTO((struct fill_style_type *fillstyle));

#ifdef WITH_IMAGE
void get_image_options __PROTO((t_image *image));
#endif

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
typedef struct lf_state_struct {
    FILE *fp;			/* file pointer for load file */
    char *name;			/* name of file */
    TBOOLEAN interactive;	/* value of interactive flag on entry */
    TBOOLEAN do_load_arg_substitution;	/* likewise ... */
    int inline_num;		/* inline_num on entry */
    int depth;			/* recursion depth */
    struct lf_state_struct *prev;			/* defines a stack */
    char *call_args[10];	/* args when file is 'call'ed instead of 'load'ed */
}  LFS;
extern LFS *lf_head;

#endif /* GNUPLOT_MISC_H */
