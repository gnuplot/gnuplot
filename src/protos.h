/*
 * $Id: protos.h,v 1.9 1999/06/14 19:23:54 lhecking Exp $
 *
 */

/* GNUPLOT - protos.h */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
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


#ifndef GNUPLOT_PROTOS_H
# define GNUPLOT_PROTOS_H

#include "ansichek.h"

/* note that this before this file, some headers that define stuff like FILE,
   time_t and GPFAR must be already be included */

/* Prototypes from file "command.c" */

void extend_input_line __PROTO((void));
void extend_token_table __PROTO((void));
void init_memory __PROTO((void));
int com_line __PROTO((void));
int do_line __PROTO((void));
void done __PROTO((int status));
void define __PROTO((void));
void bail_to_command_line __PROTO((void));
void replotrequest __PROTO((void));


/* Prototypes from file "contour.c" */
typedef double tri_diag[3];         /* Used to allocate the tri-diag matrix. */

struct gnuplot_contours *contour __PROTO((int num_isolines, struct iso_curve *iso_lines, int ZLevels, int approx_pts, int int_kind, int order1, int levels_kind, double *levels_list));
int solve_tri_diag __PROTO((tri_diag m[], double r[], double x[], int n));

/* Prototypes from file "eval.c" */

struct udvt_entry * add_udv __PROTO((int t_num));
struct udft_entry * add_udf __PROTO((int t_num));
int standard __PROTO((int t_num));
void execute_at __PROTO((struct at_type *at_ptr));


/* Prototypes from file "fit.c" */

char *get_next_word __PROTO((char **s, char *subst)); 
void init_fit __PROTO((void));
void setvar __PROTO((char *varname, struct value data));
int getivar __PROTO((const char *varname));
void update __PROTO((char *pfile, char *npfile));
void do_fit __PROTO((void));
size_t wri_to_fil_last_fit_cmd __PROTO((FILE *fp));


/* Prototypes from file "graph3d.c" */

void map3d_xy __PROTO((double x, double y, double z, unsigned int *xt, unsigned int *yt));
int map3d_z __PROTO((double x, double y, double z));
void do_3dplot __PROTO((struct surface_points *plots, int pcount));


/* Prototypes from file "help.c" */

int  help __PROTO((char *keyword, char *path, TBOOLEAN *subtopics));
void FreeHelp __PROTO((void));
void StartOutput __PROTO((void));
void OutLine __PROTO((char *line));
void EndOutput __PROTO((void));


/* Prototypes from file "hidden3d.c" */

void clip_move __PROTO((unsigned int x, unsigned int y));
void clip_vector __PROTO((unsigned int x, unsigned int y));
/* HBB 970618: new function: */
void set_hidden3doptions __PROTO((void));
void show_hidden3doptions __PROTO((void));
/* HBB 971117: another new  function: */
void save_hidden3doptions __PROTO((FILE *fp));
#ifndef LITE
void init_hidden_line_removal __PROTO((void));
void reset_hidden_line_removal __PROTO((void));
void term_hidden_line_removal __PROTO((void));
void plot3d_hidden __PROTO((struct surface_points *plots, int pcount));
void draw_line_hidden __PROTO((unsigned int, unsigned int, unsigned int, unsigned int));
#endif


/* Prototypes from file "internal.c" */

#ifdef MINEXP
double gp_exp __PROTO((double x));
#else
#define gp_exp(x) exp(x)
#endif

/* int matherr __PROTO((void)); */
void reset_stack __PROTO((void));
void check_stack __PROTO((void));
struct value *pop __PROTO((struct value *x));
void push __PROTO((struct value *x));


/* Prototypes from file "interpol.c" */

void gen_interp __PROTO((struct curve_points *plot));
void sort_points __PROTO((struct curve_points *plot));
void cp_implode __PROTO((struct curve_points *cp));


/* Prototypes from file "misc.c" */

struct curve_points * cp_alloc __PROTO((int num));
void cp_extend __PROTO((struct curve_points *cp, int num));
void cp_free __PROTO((struct curve_points *cp));
struct iso_curve * iso_alloc __PROTO((int num));
void iso_extend __PROTO((struct iso_curve *ip, int num));
void iso_free __PROTO((struct iso_curve *ip));
struct surface_points * sp_alloc __PROTO((int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2));
void sp_replace __PROTO((struct surface_points *sp, int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2));
void sp_free __PROTO((struct surface_points *sp));
void save_functions __PROTO((FILE *fp));
void save_variables __PROTO((FILE *fp));
void save_all __PROTO((FILE *fp));
void save_set __PROTO((FILE *fp));
void save_set_all __PROTO((FILE *fp));
void load_file __PROTO((FILE *fp, char *name, TBOOLEAN subst_args));
FILE *lf_top __PROTO((void));
void load_file_error __PROTO((void));
int instring __PROTO((char *str, int c));
void show_functions __PROTO((void));
void show_at __PROTO((void));
void disp_at __PROTO((struct at_type *curr_at, int level));
int find_maxl_keys __PROTO((struct curve_points *plots, int count, int *kcnt));
int find_maxl_keys3d __PROTO((struct surface_points *plots, int count, int *kcnt));
TBOOLEAN valid_format __PROTO((const char *format));
FILE *loadpath_fopen __PROTO((const char *, const char *));


/* Prototypes from file "parse.c" */

/* void fpe __PROTO((void)); */
void evaluate_at __PROTO((struct at_type *at_ptr, struct value *val_ptr));
struct value * const_express __PROTO((struct value *valptr));
struct at_type * temp_at __PROTO((void));
struct at_type * perm_at __PROTO((void));


/* Prototypes from file "plot.c" */

void interrupt_setup __PROTO((void));
void gp_expand_tilde __PROTO((char **));

/* prototypes from plot2d.c */

void plotrequest __PROTO((void));


/* prototypes from plot3d.c */

void plot3drequest __PROTO((void));


/* Prototypes from file "readline.c" */

#ifndef HAVE_LIBREADLINE
char *readline __PROTO((char *prompt));
void add_history __PROTO((char *line));
#endif /* HAVE_LIBREADLINE */

#if defined(ATARI) || defined(MTOS)
char tos_getch();
#endif


/* Prototypes from file "scanner.c" */

int scanner __PROTO((char **expression, int *line_lengthp));


/* Prototypes from "stdfn.c" */

char *safe_strncpy __PROTO((char *, const char *, size_t));
#ifndef HAVE_SLEEP
unsigned int sleep __PROTO((unsigned int));
#endif


/* Prototypes from file "term.c" */

void term_set_output __PROTO((char *));
void term_init __PROTO((void));
void term_start_plot __PROTO((void));
void term_end_plot __PROTO((void));
void term_start_multiplot __PROTO((void));
void term_end_multiplot __PROTO((void));
/* void term_suspend __PROTO((void)); */
void term_reset __PROTO((void));
void term_apply_lp_properties __PROTO((struct lp_style_type *lp));
void term_check_multiplot_okay __PROTO((TBOOLEAN));

void list_terms __PROTO((void));
struct termentry *set_term __PROTO((int));
struct termentry *change_term __PROTO((char *name, int length));
void init_terminal __PROTO((void));
void test_term __PROTO((void));
void UP_redirect __PROTO((int called));
#ifdef LINUXVGA
void LINUX_setup __PROTO((void));
#endif
#ifdef VMS
void vms_reset();
#endif

/* used by the drivers (?) */

int null_text_angle __PROTO((int ang));
int null_justify_text __PROTO((enum JUSTIFY just));
int null_scale __PROTO((double x, double y));
int do_scale __PROTO((double x, double y));
void options_null __PROTO((void));
void UNKNOWN_null __PROTO((void));
void MOVE_null __PROTO((unsigned int, unsigned int));
void LINETYPE_null __PROTO((int));
void PUTTEXT_null __PROTO((unsigned int, unsigned int, char *));


/* prototypes for functions from time.c */

char * gstrptime __PROTO((char *, char *, struct tm *)); /* string to *tm */
int gstrftime __PROTO((char *, int, char *, double)); /* *tm to string */
double gtimegm __PROTO((struct tm *)); /* *tm to seconds */
int ggmtime __PROTO((struct tm *, double)); /* seconds to *tm */


/* contents of the old fnproto.h file */

/* prototypes for gnuplot function primitives. These functions are not
 * called directly, only via a function table.
 */

/* Prototypes from file "internal.c" */

void f_push __PROTO((union argument *x));
void f_pushc __PROTO((union argument *x));
void f_pushd1 __PROTO((union argument *x));
void f_pushd2 __PROTO((union argument *x));
void f_pushd __PROTO((union argument *x));
void f_call __PROTO((union argument *x));
void f_calln __PROTO((union argument *x));
void f_lnot __PROTO((void));
void f_bnot __PROTO((void));
void f_bool __PROTO((void));
void f_lor __PROTO((void));
void f_land __PROTO((void));
void f_bor __PROTO((void));
void f_xor __PROTO((void));
void f_band __PROTO((void));
void f_uminus __PROTO((void));
void f_eq __PROTO((void));
void f_ne __PROTO((void));
void f_gt __PROTO((void));
void f_lt __PROTO((void));
void f_ge __PROTO((void));
void f_le __PROTO((void));
void f_plus __PROTO((void));
void f_minus __PROTO((void));
void f_mult __PROTO((void));
void f_div __PROTO((void));
void f_mod __PROTO((void));
void f_power __PROTO((void));
void f_factorial __PROTO((void));
int f_jump __PROTO((union argument *x));
int f_jumpz __PROTO((union argument *x));
int f_jumpnz __PROTO((union argument *x));
int f_jtern __PROTO((union argument *x));

/* Prototypes from file "standard.c" */

void f_real __PROTO((void));
void f_imag __PROTO((void));
void f_int __PROTO((void));
void f_arg __PROTO((void));
void f_conjg __PROTO((void));
void f_sin __PROTO((void));
void f_cos __PROTO((void));
void f_tan __PROTO((void));
void f_asin __PROTO((void));
void f_acos __PROTO((void));
void f_atan __PROTO((void));
void f_atan2 __PROTO((void));
void f_sinh __PROTO((void));
void f_cosh __PROTO((void));
void f_tanh __PROTO((void));
void f_asinh __PROTO((void));
void f_acosh __PROTO((void));
void f_atanh __PROTO((void));
void f_void __PROTO((void));
void f_abs __PROTO((void));
void f_sgn __PROTO((void));
void f_sqrt __PROTO((void));
void f_exp __PROTO((void));
void f_log10 __PROTO((void));
void f_log __PROTO((void));
void f_floor __PROTO((void));
void f_ceil __PROTO((void));
void f_besj0 __PROTO((void));
void f_besj1 __PROTO((void));
void f_besy0 __PROTO((void));
void f_besy1 __PROTO((void));

void f_tmsec __PROTO((void));
void f_tmmin __PROTO((void));
void f_tmhour __PROTO((void));
void f_tmmday __PROTO((void));
void f_tmmon __PROTO((void));
void f_tmyear __PROTO((void));
void f_tmwday __PROTO((void));
void f_tmyday __PROTO((void));

/* Prototypes from file "specfun.c" */

void f_erf __PROTO((void));
void f_erfc __PROTO((void));
void f_ibeta __PROTO((void));
void f_igamma __PROTO((void));
void f_gamma __PROTO((void));
void f_lgamma __PROTO((void));
void f_rand __PROTO((void));
void f_normal __PROTO((void));
void f_inverse_normal __PROTO((void));
void f_inverse_erf __PROTO((void));

/* prototypes from file "datafile.c" */

void f_dollars __PROTO((union argument *x));
void f_column  __PROTO((void));
void f_valid   __PROTO((void));
void f_timecolumn   __PROTO((void));

#include "alloc.h"

#endif /* GNUPLOT_PROTOS_H */
