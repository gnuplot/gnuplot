/*
 * $Id: protos.h,v 1.56 1998/04/14 00:16:11 drd Exp $
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

/* Prototypes from file "contour.c" */
typedef double tri_diag[3];         /* Used to allocate the tri-diag matrix. */

struct gnuplot_contours *contour __PROTO((int num_isolines, struct iso_curve *iso_lines, int ZLevels, int approx_pts, int int_kind, int order1, int levels_kind, double *levels_list));
int solve_tri_diag __PROTO((tri_diag m[], double r[], double x[], int n));

/* Prototypes from file "datafile.c" */
int df_open __PROTO((int max_using));
int df_readline __PROTO((double v[], int max));
void df_close __PROTO((void));
int df_2dbinary __PROTO((struct curve_points *this_plot));
int df_3dmatrix __PROTO((struct surface_points *this_plot));

/* Prototypes from file "eval.c" */

struct udvt_entry * add_udv __PROTO((int t_num));
struct udft_entry * add_udf __PROTO((int t_num));
int standard __PROTO((int t_num));
void execute_at __PROTO((struct at_type *at_ptr));


/* Prototypes from file "fit.c" */

char *get_next_word __PROTO((char **s, char *subst)); 
void init_fit __PROTO((void));
void setvar __PROTO((char *varname, struct value data));
int getivar __PROTO((char *varname));
void update __PROTO((char *pfile, char *npfile));
void do_fit __PROTO((void));
int wri_to_fil_last_fit_cmd __PROTO((FILE *fp));


/* Prototypes from file "graphics.c" */

void graph_error __PROTO((char *text));
void fixup_range __PROTO((int axis, char *axis_name));
void timetic_format __PROTO((int axis, double amin, double amax));
void do_plot __PROTO((struct curve_points *plots, int pcount));
double time_tic_just __PROTO((int level, double ticplace));
double make_ltic __PROTO((int tlevel, double incr));
int label_width __PROTO((char *str, int *lines));
double set_tic __PROTO((double l10, int guide));
void setup_tics __PROTO((int axis, struct ticdef *def, char *format, int max));
/* is this valid use of __P ? */
typedef void (*tic_callback) __PROTO((int axis, double place, char *text, struct lp_style_type grid));
void gen_tics __PROTO((int axis, struct ticdef *def, int grid, int minitic, double minifreq, tic_callback callback));
void write_multiline __PROTO((unsigned int x, unsigned int y, char *text_will_be_mangled, enum JUSTIFY hor, int vert, int angle, char *font));

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


/* Prototypes from file "parse.c" */

/* void fpe __PROTO((void)); */
void evaluate_at __PROTO((struct at_type *at_ptr, struct value *val_ptr));
struct value * const_express __PROTO((struct value *valptr));
struct at_type * temp_at __PROTO((void));
struct at_type * perm_at __PROTO((void));


/* Prototypes from file "plot.c" */

void interrupt_setup __PROTO((void));


/* prototypes from plot2d.c */

void plotrequest __PROTO((void));


/* prototypes from plot3d.c */

void plot3drequest __PROTO((void));


/* Prototypes from file "readline.c" */

#if defined(READLINE) && !defined(GNU_READLINE)
char *readline __PROTO((char *prompt));
void add_history __PROTO((char *line));
#endif

#if defined(ATARI) || defined(MTOS)
char tos_getch();
#endif


/* Prototypes from file "scanner.c" */

int scanner __PROTO((char expression[]));


/* Prototypes from "stdfn.c" */

char *safe_strncpy __PROTO((char *, char *, size_t));
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


/* Prototypes from file "util.c" */

int chr_in_str __PROTO((int t_num, int c));
int equals __PROTO((int t_num, char *str));
int almost_equals __PROTO((int t_num, char *str));
int isstring __PROTO((int t_num));
int isanumber __PROTO((int t_num));
int isletter __PROTO((int t_num));
int is_definition __PROTO((int t_num));
void copy_str __PROTO((char str[], int t_num, int max));
int token_len __PROTO((int t_num));
void quote_str __PROTO((char str[], int t_num, int max));
void capture __PROTO((char str[], int start, int end, int max));
void m_capture __PROTO((char **str, int start, int end));
void m_quote_capture __PROTO((char **str, int start, int end));
void convert __PROTO((struct value *val_ptr, int t_num));
void disp_value __PROTO((FILE *fp, struct value *val));
double real __PROTO((struct value *val));
double imag __PROTO((struct value *val));
double magnitude __PROTO((struct value *val));
double angle __PROTO((struct value *val));
struct value * Gcomplex __PROTO((struct value *a, double realpart, double imagpart));
struct value * Ginteger __PROTO((struct value *a, int i));
void os_error __PROTO((char str[], int t_num));
void int_error __PROTO((char str[], int t_num));
void int_warn __PROTO((char str[], int t_num));
void lower_case __PROTO((char *s));
void squash_spaces __PROTO((char *s));


/* Prototypes from file "util3d.c" */

void draw_clip_line __PROTO((unsigned int, unsigned int, unsigned int, unsigned int));
/* HBB: these two are now in hidden3d.c : */
/*void clip_move __PROTO((unsigned int x, unsigned int y));*/
/*void clip_vector __PROTO((unsigned int x, unsigned int y));*/
/* HBB: this one didn't have any prototype yet: */
int clip_line __PROTO((int *, int *, int *, int *));
void edge3d_intersect __PROTO((struct coordinate GPHUGE *points, int i, double *ex, double *ey, double *ez));
TBOOLEAN two_edge3d_intersect __PROTO((struct coordinate GPHUGE *points, int i, double *lx, double *ly, double *lz));
void mat_unit __PROTO((double mat[4][4]));
void mat_trans __PROTO((double tx, double ty, double tz, double mat[4][4]));
void mat_scale __PROTO((double sx, double sy, double sz, double mat[4][4]));
void mat_rot_x __PROTO((double teta, double mat[4][4]));
void mat_rot_y __PROTO((double teta, double mat[4][4]));
void mat_rot_z __PROTO((double teta, double mat[4][4]));
void mat_mult __PROTO((double mat_res[4][4], double mat1[4][4], double mat2[4][4]));
int clip_point __PROTO((unsigned int x, unsigned int y));
void clip_put_text __PROTO((unsigned int x, unsigned int y, char *str));
void clip_put_text_just __PROTO((unsigned int x, unsigned int y, char *str, enum JUSTIFY just));

#include "alloc.h"

