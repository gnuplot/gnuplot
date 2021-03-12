/* GNUPLOT - parse.h */

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

#ifndef PARSE_H
# define PARSE_H

# include "syscfg.h"
# include "gp_types.h"

#include "eval.h"

/* externally usable types defined by parse.h */

/* exported variables of parse.c */
extern TBOOLEAN scanning_range_in_progress;

/* The choice of dummy variables, as set by 'set dummy', 'set polar'
 * and 'set parametric' */
extern char set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
/* Dummy variables referenced by name in a fit command */
/* Sep 2014 (DEBUG) used to deduce how many independent variables */
extern int  fit_dummy_var[MAX_NUM_VAR];
/* the currently used 'dummy' variables. Usually a copy of
 * set_dummy_var, but may be changed by the '(s)plot' command
 * containing an explicit range (--> 'plot [phi=0..pi]') */
extern char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];

/* This is used by plot_option_using() */
extern int at_highest_column_used;

/* This is checked by df_readascii() */
extern TBOOLEAN parse_1st_row_as_headers;

/* This is used by df_open() and df_readascii() */
extern udvt_entry *df_array;

/* This is used to block re-definition of built-in functions */
extern int is_builtin_function(int t_num);

/* Protection mechanism for trying to parse a string followed by a + or - sign.
 * Also suppresses an undefined variable message if an unrecognized token
 * is encountered during try_to_get_string().
 */
extern TBOOLEAN string_result_only;

/* Prototypes of exported functions in parse.c */

intgr_t int_expression(void);
double real_expression(void);
void parse_reset_after_error(void);
struct value * const_string_express(struct value *valptr);
struct value * const_express(struct value *valptr);
char* string_or_express(struct at_type **atptr);
struct at_type * temp_at(void);
struct at_type * perm_at(void);
struct at_type * create_call_column_at(char *);
struct at_type * create_call_columnhead(void);
struct udvt_entry * add_udv(int t_num);
struct udft_entry * add_udf(int t_num);
void cleanup_udvlist(void);
int is_function(int t_num);

/* Code that uses the iteration routines here must provide */
/* a blank iteration structure to use for bookkeeping.     */
typedef struct iterator {
	struct iterator *next;		/* linked list */
	struct udvt_entry *iteration_udv;
	t_value original_udv_value;	/* prior value of iteration variable */
	char *iteration_string;
	int iteration_start;
	int iteration_end;
	int iteration_increment;
	int iteration_current;		/* start + increment * iteration */
	int iteration;			/* runs from 0 to (end-start)/increment */
	struct at_type *start_at;	/* expression that evaluates to iteration_start */
	struct at_type *end_at;		/* expression that evaluates to iteration_end */
} t_iterator;

extern t_iterator * plot_iterator;	/* Used for plot and splot */
extern t_iterator * set_iterator;	/* Used by set/unset commands */

/* These are used by the iteration code */
t_iterator * check_for_iteration(void);
TBOOLEAN next_iteration (t_iterator *);
TBOOLEAN empty_iteration (t_iterator *);
TBOOLEAN forever_iteration (t_iterator *);
t_iterator * cleanup_iteration(t_iterator *);

void parse_link_via(struct udft_entry *);

/* Magic number used instead of actual column number in $# */
#define DOLLAR_NCOLUMNS -123

#endif /* PARSE_H */
