/*
 * $Id: eval.h,v 1.4 2000/10/31 19:59:30 joze Exp $
 */

/* GNUPLOT - eval.h */

/*[
 * Copyright 1999   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_EVAL_H
# define GNUPLOT_EVAL_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "gp_types.h"

/*  #include "parse.h" */

/* Type definitions */

/* user-defined function table entry */
typedef struct udft_entry {
    struct udft_entry *next_udf; /* pointer to next udf in linked list */
    char *udf_name;		/* name of this function entry */
    struct at_type *at;		/* pointer to action table to execute */
    char *definition;		/* definition of function as typed */
    t_value dummy_values[MAX_NUM_VAR]; /* current value of dummy variables */
} udft_entry;


/* user-defined variable table entry */
typedef struct udvt_entry {
    struct udvt_entry *next_udv; /* pointer to next value in linked list */
    char *udv_name;		/* name of this value entry */
    TBOOLEAN udv_undef;		/* true if not defined yet */
    t_value udv_value;		/* value it has */
} udvt_entry;

/* p-code argument */
typedef union argument {
	int j_arg;		/* offset for jump */
	struct value v_arg;	/* constant value */
	struct udvt_entry *udv_arg; /* pointer to dummy variable */
	struct udft_entry *udf_arg; /* pointer to udf to execute */
} argument;


/* This type definition has to come after union argument has been declared. */
#ifdef __ZTC__
typedef int (*FUNC_PTR)(...);
#else
typedef int (*FUNC_PTR) __PROTO((union argument *arg));
#endif

/* standard/internal function table entry */
typedef struct ft_entry {
    const char *f_name;		/* pointer to name of this function */
    FUNC_PTR func;		/* address of function to call */
} ft_entry;

/* Variables of eval.c needed by other modules: */

extern struct ft_entry GPFAR ft[]; /* The table of builtin functions */
extern struct udft_entry *first_udf; /* user-def'd functions */
extern struct udvt_entry *first_udv; /* user-def'd variables */
extern struct udvt_entry udv_pi; /* 'pi' variable */


/* Prototypes of functions exported by eval.c */

struct udvt_entry * add_udv __PROTO((int t_num));
struct udft_entry * add_udf __PROTO((int t_num));
int standard __PROTO((int t_num));
void execute_at __PROTO((struct at_type *at_ptr));

#endif /* GNUPLOT_EVAL_H */
