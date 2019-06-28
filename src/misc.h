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
#include "term_api.h"


/* Variables of misc.c needed by other modules: */

/* Used by postscript terminal if a font file is found by loadpath_fopen() */
extern char *loadpath_fontname;

/* these two are global so that plot.c can load them on program entry */
extern char *call_args[10];
extern int call_argc;

/* Prototypes from file "misc.c" */

const char *expand_call_arg(int c);
void load_file(FILE *fp, char *name, int calltype);
FILE *lf_top(void);
TBOOLEAN lf_pop(void);
void lf_push(FILE *fp, char *name, char *cmdline);
void load_file_error(void);
FILE *loadpath_fopen(const char *, const char *);
void push_terminal(int is_interactive);
void pop_terminal(void);

/* moved here, from setshow */
enum PLOT_STYLE get_style(void);
void get_filledcurves_style_options(filledcurves_opts *);
void filledcurves_options_tofile(filledcurves_opts *, FILE *);
int lp_parse(struct lp_style_type *lp, lp_class destination_class, TBOOLEAN allow_point);

void arrow_parse(struct arrow_style_type *, TBOOLEAN);
void arrow_use_properties(struct arrow_style_type *arrow, int tag);

void parse_fillstyle(struct fill_style_type *fs);
void parse_colorspec(struct t_colorspec *tc, int option);
long parse_color_name(void);
TBOOLEAN need_fill_border(struct fill_style_type *fillstyle);

void get_image_options(t_image *image);

int parse_dashtype(struct t_dashtype *dt);

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
typedef struct lf_state_struct {
    /* new recursion level: */
    FILE *fp;			/* file pointer for load file */
    char *name;			/* name of file */
    char *cmdline;              /* content of command string for do_string() */
    /* last recursion level: */
    TBOOLEAN interactive;	/* value of interactive flag on entry */
    int inline_num;		/* inline_num on entry */
    int depth;			/* recursion depth */
    int if_depth;		/* used by _old_ if/else syntax */
    TBOOLEAN if_open_for_else;	/* used by _new_ if/else syntax */
    TBOOLEAN if_condition;	/* used by both old and new if/else syntax */
    char *input_line;		/* Input line text to restore */
    struct lexical_unit *tokens;/* Input line tokens to restore */
    int num_tokens;		/* How big is the above ? */
    int c_token;		/* Which one were we on ? */
    struct lf_state_struct *prev;			/* defines a stack */
    int call_argc;		/* This saves the _caller's_ argc */
    char *call_args[10];	/* ARG0 through ARG9 from "call" command */
    struct value argv[10];	/* content of global ARGV[] array */
}  LFS;
extern LFS *lf_head;

#endif /* GNUPLOT_MISC_H */
