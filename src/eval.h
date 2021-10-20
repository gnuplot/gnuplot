/* GNUPLOT - eval.h */

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

#ifndef GNUPLOT_EVAL_H
# define GNUPLOT_EVAL_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "gp_types.h"

#include <stdio.h>		/* for FILE* */

#define STACK_DEPTH 250		/* maximum size of the execution stack */
#define MAX_AT_LEN 150		/* max number of entries in action table */

/* These are used by add_action() to index the subroutine list ft[] in eval.c */
enum operators {
    /* keep this in line with table in eval.c */
    PUSH, PUSHC, PUSHD1, PUSHD2, PUSHD, POP,
    CALL, CALLN, SUM, LNOT, BNOT, UMINUS, NOP,
    LOR, LAND, BOR, XOR, BAND, EQ, NE, GT, LT, GE, LE, 
    LEFTSHIFT, RIGHTSHIFT, PLUS, MINUS,
    MULT, DIV, MOD, POWER, FACTORIAL, BOOLE,
    DOLLARS,
    CONCATENATE, EQS, NES, RANGE, INDEX, CARDINALITY,
    ASSIGN,
    /* only jump operators go between jump and sf_start, for is_jump() */
    JUMP, JUMPZ, JUMPNZ, JTERN, SF_START,

    /* External function call */
#ifdef HAVE_EXTERNAL_FUNCTIONS
    CALLE,
#endif

    /* functions specific to using spec */
    COLUMN, STRINGCOLUMN, STRCOL, COLUMNHEAD
};
#define is_jump(operator) \
    ((operator) >=(int)JUMP && (operator) <(int)SF_START)

enum int64_overflow {
    INT64_OVERFLOW_IGNORE=0, INT64_OVERFLOW_TO_FLOAT, \
    INT64_OVERFLOW_UNDEFINED, INT64_OVERFLOW_NAN
};

/* user-defined function table entry */
typedef struct udft_entry {
    struct udft_entry *next_udf; /* pointer to next udf in linked list */
    char *udf_name;		 /* name of this function entry */
    struct at_type *at;		 /* pointer to action table to execute */
    char *definition;		 /* definition of function as typed */
    int dummy_num;		 /* required number of input variables */
    t_value dummy_values[MAX_NUM_VAR]; /* current value of dummy variables */
} udft_entry;


/* user-defined variable table entry */
typedef struct udvt_entry {
    struct udvt_entry *next_udv; /* pointer to next value in linked list */
    char *udv_name;		/* name of this value entry */
    t_value udv_value;		/* value it has */
} udvt_entry;

/* p-code argument */
typedef union argument {
	int j_arg;		/* offset for jump */
	struct value v_arg;	/* constant value */
	struct udvt_entry *udv_arg; /* pointer to dummy variable */
	struct udft_entry *udf_arg; /* pointer to udf to execute */
#ifdef HAVE_EXTERNAL_FUNCTIONS
	struct exft_entry *exf_arg; /* pointer to external function */
#endif
} argument;


/* This type definition has to come after union argument has been declared. */
typedef void (*FUNC_PTR)(union argument *arg);

/* standard/internal function table entry */
typedef struct ft_entry {
    const char *f_name;		/* pointer to name of this function */
    FUNC_PTR func;		/* address of function to call */
} ft_entry;

/* action table entry */
struct at_entry {
    enum operators index;	/* index of p-code function */
    union argument arg;
};

struct at_type {
    /* count of entries in .actions[] */
    int a_count;
    /* will usually be less than MAX_AT_LEN is malloc()'d copy */
    struct at_entry actions[MAX_AT_LEN];
};


/* Variables of eval.c needed by other modules: */

extern const struct ft_entry ft[]; /* The table of builtin functions */
extern struct udft_entry *first_udf; /* user-def'd functions */
extern struct udvt_entry *first_udv; /* user-def'd variables */
extern struct udvt_entry udv_pi; /* 'pi' variable */
extern struct udvt_entry *udv_I; /* 'I' (sqrt(-1)) */
extern struct udvt_entry *udv_NaN; /* 'NaN' variable */
extern struct udvt_entry **udv_user_head; /* first udv that can be deleted */
extern TBOOLEAN undefined;

extern enum int64_overflow overflow_handling;

/* Prototypes of functions exported by eval.c */

double gp_exp(double x);

/* HBB 20010726: Moved these here, from util.h. */
double real(struct value *);
double imag(struct value *);
double magnitude(struct value *);
double angle(struct value *);
struct value * Gcomplex(struct value *, double, double);
struct value * Ginteger(struct value *, intgr_t);
struct value * Gstring(struct value *, char *);
struct value * pop_or_convert_from_string(struct value *);
void free_value(struct value *a);
void gpfree_string(struct value *a);
void gpfree_array(struct value *a);

void reset_stack(void);
void check_stack(void);
TBOOLEAN more_on_stack(void);
struct value *pop(struct value *x);
void push(struct value *x);
void int_check(struct value * v);

void f_bool(union argument *x);
void f_jump(union argument *x);
void f_jumpz(union argument *x);
void f_jumpnz(union argument *x);
void f_jtern(union argument *x);

void execute_at(struct at_type *at_ptr);
void evaluate_at(struct at_type *at_ptr, struct value *val_ptr);
void real_free_at(struct at_type *at_ptr);
struct udvt_entry * add_udv_by_name(char *key);
struct udvt_entry * get_udv_by_name(char *key);
void del_udv_by_name( char *key, TBOOLEAN isWildcard );
void clear_udf_list(void);

/* update GPVAL_ variables available to user */
void update_gpval_variables(int from_plot_command);
/* note: the routines below work for any variable name, not just those beginning GPVAL_ */
void fill_gpval_string(char *var, const char *value);
void fill_gpval_integer(char *var, intgr_t value);
void fill_gpval_float(char *var, double value);
void fill_gpval_complex(char *var, double areal, double aimag);

/* C-callable versions of internal gnuplot functions word() and words() */
char * gp_word(char *string, int i);
int gp_words(char *string);

/* Wrap real_free_at in a macro */
#define free_at(at_ptr) \
 do { real_free_at(at_ptr); at_ptr = NULL; } while (0)

#endif /* GNUPLOT_EVAL_H */
