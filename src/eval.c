/* GNUPLOT - eval.c */

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

#include "eval.h"

#include "syscfg.h"
#include "alloc.h"
#include "datafile.h"
#include "datablock.h"
#include "external.h"	/* for f_calle */
#include "internal.h"
#include "libcerf.h"
#include "specfun.h"
#include "standard.h"
#include "util.h"
#include "version.h"
#include "term_api.h"
#include "voxelgrid.h"

#include <signal.h>
#include <setjmp.h>

/* Internal prototypes */
static RETSIGTYPE fpe(int an_int);

/* Global variables exported by this module */
struct udvt_entry udv_pi = { NULL, "pi", {INTGR, {0} } };
struct udvt_entry *udv_I;
struct udvt_entry *udv_NaN;
/* first in linked list */
struct udvt_entry *first_udv = &udv_pi;
struct udft_entry *first_udf = NULL;
/* pointer to first udv users can delete */
struct udvt_entry **udv_user_head;

/* Various abnormal conditions during evaluation of an action table
 * (the stored form of an expression) are signalled by setting
 * undefined = TRUE.
 * NB:  A test for  "if (undefined)"  is only valid immediately
 * following a call to evaluate_at() or eval_link_function().
 */
TBOOLEAN undefined;

enum int64_overflow overflow_handling = INT64_OVERFLOW_TO_FLOAT;

/* The stack this operates on */
static struct value stack[STACK_DEPTH];
static int s_p = -1;		/* stack pointer */
#define top_of_stack stack[s_p]

static int jump_offset;		/* to be modified by 'jump' operators */

/* The table of built-in functions */
/* These must strictly parallel enum operators in eval.h */
const struct ft_entry ft[] =
{
    /* internal functions: */
    {"push",  f_push},
    {"pushc",  f_pushc},
    {"pushd1",  f_pushd1},
    {"pushd2",  f_pushd2},
    {"pushd",  f_pushd},
    {"pop",  f_pop},
    {"call",  f_call},
    {"calln",  f_calln},
    {"sum", f_sum},
    {"lnot",  f_lnot},
    {"bnot",  f_bnot},
    {"uminus",  f_uminus},
    {"nop",  f_nop},
    {"lor",  f_lor},
    {"land",  f_land},
    {"bor",  f_bor},
    {"xor",  f_xor},
    {"band",  f_band},
    {"eq",  f_eq},
    {"ne",  f_ne},
    {"gt",  f_gt},
    {"lt",  f_lt},
    {"ge",  f_ge},
    {"le",  f_le},
    {"leftshift", f_leftshift},
    {"rightshift", f_rightshift},
    {"plus",  f_plus},
    {"minus",  f_minus},
    {"mult",  f_mult},
    {"div",  f_div},
    {"mod",  f_mod},
    {"power",  f_power},
    {"factorial",  f_factorial},
    {"bool",  f_bool},
    {"dollars",  f_dollars},	/* for usespec */
    {"concatenate",  f_concatenate},	/* for string variables only */
    {"eqs",  f_eqs},			/* for string variables only */
    {"nes",  f_nes},			/* for string variables only */
    {"[]",  f_range},			/* for string variables only */
    {"[]",  f_index},			/* for array variables only */
    {"||",  f_cardinality},		/* for array variables only */
    {"assign", f_assign},		/* assignment operator '=' */
    {"jump",  f_jump},
    {"jumpz",  f_jumpz},
    {"jumpnz",  f_jumpnz},
    {"jtern",  f_jtern},

/* Placeholder for SF_START */
    {"", NULL},

#ifdef HAVE_EXTERNAL_FUNCTIONS
    {"", f_calle},
#endif

/* legal in using spec only */
    {"column",  f_column},
    {"stringcolumn",  f_stringcolumn},	/* for using specs */
    {"strcol",  f_stringcolumn},	/* shorthand form */
    {"columnhead",  f_columnhead},
    {"columnheader",  f_columnhead},
    {"valid",  f_valid},
    {"timecolumn",  f_timecolumn},

/* standard functions: */
    {"real",  f_real},
    {"imag",  f_imag},
    {"arg",  f_arg},
    {"conjg",  f_conjg},
    {"sin",  f_sin},
    {"cos",  f_cos},
    {"tan",  f_tan},
    {"asin",  f_asin},
    {"acos",  f_acos},
    {"atan",  f_atan},
    {"atan2",  f_atan2},
    {"sinh",  f_sinh},
    {"cosh",  f_cosh},
    {"tanh",  f_tanh},
    {"EllipticK",  f_ellip_first},
    {"EllipticE",  f_ellip_second},
    {"EllipticPi", f_ellip_third},
    {"int",  f_int},
    {"abs",  f_abs},
    {"sgn",  f_sgn},
    {"sqrt",  f_sqrt},
    {"exp",  f_exp},
    {"log10",  f_log10},
    {"log",  f_log},
    {"besi0",  f_besi0},
    {"besi1",  f_besi1},
    {"besj0",  f_besj0},
    {"besj1",  f_besj1},
    {"besjn",  f_besjn},
    {"besy0",  f_besy0},
    {"besy1",  f_besy1},
    {"besyn",  f_besyn},
    {"erf",  f_erf},
    {"erfc",  f_erfc},
    {"gamma",  f_gamma},
    {"lgamma",  f_lgamma},
    {"ibeta",  f_ibeta},
    {"voigt",  f_voigt},
    {"igamma",  f_igamma},
    {"rand",  f_rand},
    {"floor",  f_floor},
    {"ceil",  f_ceil},

    {"norm",  f_normal},	/* XXX-JG */
    {"inverf",  f_inverse_erf},	/* XXX-JG */
    {"invnorm",  f_inverse_normal},	/* XXX-JG */
    {"asinh",  f_asinh},
    {"acosh",  f_acosh},
    {"atanh",  f_atanh},
    {"lambertw",  f_lambertw}, /* HBB, from G.Kuhnle 20001107 */
    {"airy",  f_airy},         /* janert, 20090905 */
    {"expint",  f_expint},     /* Jim Van Zandt, 20101010 */
    {"besin",  f_besin},

#ifdef HAVE_LIBCERF
    {"cerf", f_cerf},		/* complex error function */
    {"cdawson", f_cdawson},	/* complex Dawson's integral */
    {"erfi", f_erfi},		/* imaginary error function */
    {"VP", f_voigtp},		/* Voigt profile */
    {"faddeeva", f_faddeeva},	/* Faddeeva rescaled complex error function "w_of_z" */
#endif

    {"tm_sec",  f_tmsec},	/* time function */
    {"tm_min",  f_tmmin},	/* time function */
    {"tm_hour", f_tmhour},	/* time function */
    {"tm_mday", f_tmmday},	/* time function */
    {"tm_mon",  f_tmmon},	/* time function */
    {"tm_year", f_tmyear},	/* time function */
    {"tm_wday", f_tmwday},	/* time function */
    {"tm_yday", f_tmyday},	/* time function */
    {"tm_week", f_tmweek},	/* time function */
    {"weekdate_iso", f_weekdate_iso},
    {"weekdate_cdc", f_weekdate_cdc},

    {"sprintf",  f_sprintf},	/* for string variables only */
    {"gprintf",  f_gprintf},	/* for string variables only */
    {"strlen",  f_strlen},	/* for string variables only */
    {"strstrt",  f_strstrt},	/* for string variables only */
    {"substr",  f_range},	/* for string variables only */
    {"trim",  f_trim},		/* for string variables only */
    {"word",  f_word},		/* for string variables only */
    {"words", f_words},		/* implemented as word(s,-1) */
    {"strftime",  f_strftime},  /* time to string */
    {"strptime",  f_strptime},  /* string to time */
    {"time", f_time},		/* get current time */
    {"system", f_system},       /* "dynamic backtics" */
    {"exist", f_exists},	/* exists("foo") replaces defined(foo) */
    {"exists", f_exists},	/* exists("foo") replaces defined(foo) */
    {"value", f_value},		/* retrieve value of variable known by name */

    {"hsv2rgb", f_hsv2rgb},	/* color conversion */
    {"palette", f_palette},	/* palette color lookup */

#ifdef VOXEL_GRID_SUPPORT
    {"voxel", f_voxel},		/* extract value of single voxel */
#endif

    {NULL, NULL}
};

/* Module-local variables: */

static JMP_BUF fpe_env;

/* Internal helper functions: */

static RETSIGTYPE
fpe(int an_int)
{
#if defined(MSDOS) && !defined(__EMX__) && !defined(DJGPP)
    /* thanks to lotto@wjh12.UUCP for telling us about this  */
    _fpreset();
#endif

    (void) an_int;		/* avoid -Wunused warning */
    (void) signal(SIGFPE, (sigfunc) fpe);
    undefined = TRUE;
    LONGJMP(fpe_env, TRUE);
}

/* Exported functions */

/* First, some functions that help other modules use 'struct value' ---
 * these might justify a separate module, but I'll stick with this,
 * for now */

/* returns the real part of val */
double
real(struct value *val)
{
    switch (val->type) {
    case INTGR:
	return ((double) val->v.int_val);
    case CMPLX:
	return (val->v.cmplx_val.real);
    case STRING:    /* FIXME is this ever used? */
	return (atof(val->v.string_val));
    case NOTDEFINED:
	return not_a_number();
    default:
	int_error(NO_CARET, "unknown type in real()");
    }
    /* NOTREACHED */
    return ((double) 0.0);
}


/* returns the imag part of val */
double
imag(struct value *val)
{
    switch (val->type) {
    case INTGR:
	return (0.0);
    case CMPLX:
	return (val->v.cmplx_val.imag);
    case STRING:
	/* This is where we end up if the user tries: */
	/*     x = 2;  plot sprintf(format,x)         */
	int_warn(NO_CARET, "encountered a string when expecting a number");
	int_error(NO_CARET, "Did you try to generate a file name using dummy variable x or y?");
    case NOTDEFINED:
	return not_a_number();
    default:
	int_error(NO_CARET, "unknown type in imag()");
    }
    /* NOTREACHED */
    return ((double) 0.0);
}



/* returns the magnitude of val */
double
magnitude(struct value *val)
{
    switch (val->type) {
    case INTGR:
	return (fabs((double)val->v.int_val));
    case CMPLX:
	{
	    /* The straightforward implementation sqrt(r*r+i*i)
	     * over-/underflows if either r or i is very large or very
	     * small. This implementation avoids over-/underflows from
	     * squaring large/small numbers whenever possible.  It
	     * only over-/underflows if the correct result would, too.
	     * CAVEAT: sqrt(1+x*x) can still have accuracy
	     * problems. */
	    double abs_r = fabs(val->v.cmplx_val.real);
	    double abs_i = fabs(val->v.cmplx_val.imag);
	    double quotient;

	    if (abs_i == 0)
	    	return abs_r;
	    if (abs_r > abs_i) {
		quotient = abs_i / abs_r;
		return abs_r * sqrt(1 + quotient*quotient);
	    } else {
		quotient = abs_r / abs_i;
		return abs_i * sqrt(1 + quotient*quotient);
	    }
	}
    default:
	int_error(NO_CARET, "unknown type in magnitude()");
    }
    /* NOTREACHED */
    return ((double) 0.0);
}



/* returns the angle of val */
double
angle(struct value *val)
{
    switch (val->type) {
    case INTGR:
	return ((val->v.int_val >= 0) ? 0.0 : M_PI);
    case CMPLX:
	if (val->v.cmplx_val.imag == 0.0) {
	    if (val->v.cmplx_val.real >= 0.0)
		return (0.0);
	    else
		return (M_PI);
	}
	return (atan2(val->v.cmplx_val.imag,
		      val->v.cmplx_val.real));
    default:
	int_error(NO_CARET, "unknown type in angle()");
    }
    /* NOTREACHED */
    return ((double) 0.0);
}


struct value *
Gcomplex(struct value *a, double realpart, double imagpart)
{
    a->type = CMPLX;
    a->v.cmplx_val.real = realpart;
    a->v.cmplx_val.imag = imagpart;
    return (a);
}


struct value *
Ginteger(struct value *a, intgr_t i)
{
    a->type = INTGR;
    a->v.int_val = i;
    return (a);
}

struct value *
Gstring(struct value *a, char *s)
{
    a->type = STRING;
    a->v.string_val = s ? s : strdup("");
    return (a);
}

/* Common interface for freeing data structures attached to a struct value.
 * Each of the type-specific routines will ignore values of other types.
 */
void
free_value(struct value *a)
{
    gpfree_string(a);
    /* gpfree_datablock(a); */
    gpfree_array(a);
}

/* It is always safe to call gpfree_string with a->type is INTGR or CMPLX.
 * However it would be fatal to call it with a->type = STRING if a->string_val
 * was not obtained by a previous call to gp_alloc(), or has already been freed.
 * Thus 'a->type' is set to NOTDEFINED afterwards to make subsequent calls safe.
 */
void
gpfree_string(struct value *a)
{
    if (a->type == STRING) {
	free(a->v.string_val);
	a->type = NOTDEFINED;
    }
}

void
gpfree_array(struct value *a)
{
    int i;
    int size;

    if (a->type == ARRAY) {
	size = a->v.value_array[0].v.int_val;
	for (i=1; i<=size; i++)
	    gpfree_string(&(a->v.value_array[i]));
	free(a->v.value_array);
	a->type = NOTDEFINED;
    }
}

/* some machines have trouble with exp(-x) for large x
 * if E_MINEXP is defined at compile time, use gp_exp(x) instead,
 * which returns 0 for exp(x) with x < E_MINEXP
 * exp(x) will already have been defined as gp_exp(x) in plot.h
 */

double
gp_exp(double x)
{
#ifdef E_MINEXP
    return (x < (E_MINEXP)) ? 0.0 : exp(x);
#else  /* E_MINEXP */
    int old_errno = errno;
    double result = exp(x);

    /* exp(-large) quite uselessly raises ERANGE --- stop that */
    if (result == 0.0)
	errno = old_errno;
    return result;
#endif /* E_MINEXP */
}

void
reset_stack()
{
    s_p = -1;
}


void
check_stack()
{				/* make sure stack's empty */
    if (s_p != -1)
	fprintf(stderr, "\n\
warning:  internal error--stack not empty!\n\
          (function called with too many parameters?)\n");
}

TBOOLEAN
more_on_stack()
{
    return (s_p >= 0);
}

struct value *
pop(struct value *x)
{
    if (s_p < 0)
	int_error(NO_CARET, "stack underflow (function call with missing parameters?)");
    *x = stack[s_p--];
    return (x);
}

/*
 * Allow autoconversion of string variables to floats if they
 * are dereferenced in a numeric context.
 * Jun 2022: Stricter error checking for non-numeric string.
 */
struct value *
pop_or_convert_from_string(struct value *v)
{
    pop(v);

    /* FIXME: Test for INVALID_VALUE? Other corner cases? */
    if (v->type == INVALID_NAME)
	int_error(NO_CARET, "invalid dummy variable name");

    if (v->type == STRING) {
	char *string = v->v.string_val;
	char *eov = string;
	char trailing = *eov;

	/* If the string contains no decimal point, try to interpret it as an integer.
	 * We treat a string starting with "0x" as a hexadecimal; everything else
	 * as decimal.  So int("010") promotes to 10, not 8.
	 */
	if (strcspn(string, ".") == strlen(string)) {
	    long long li;
	    if (string[0] == '0' && string[1] == 'x')
		li = strtoll( string, &eov, 16 );
	    else
		li = strtoll( string, &eov, 10 );
	    trailing = *eov;
	    Ginteger(v, li);
	}
	/* Successful interpretation as an integer leaves (eov != string).
	 * Otherwise try again as a floating point, including oddball cases like
	 * "NaN" or "-Inf" that contain no decimal point.
	 */
	if (eov == string) {
	    double d = strtod(string, &eov);
	    trailing = *eov;
	    Gcomplex(v, d, 0.);
	}
	free(string);	/* NB: invalidates dereference of eov */
	if (eov == string)
	    int_error(NO_CARET,"Non-numeric string found where a numeric expression was expected");
	if (trailing && !isspace(trailing))
	    int_warn(NO_CARET,"Trailing characters after numeric expression");
    }
    return(v);
}

void
push(struct value *x)
{
    if (s_p == STACK_DEPTH - 1)
	int_error(NO_CARET, "stack overflow");
    stack[++s_p] = *x;

    /* WARNING - This is a memory leak if the string is not later freed */
    if (x->type == STRING && x->v.string_val)
	stack[s_p].v.string_val = gp_strdup(x->v.string_val);
}


void
int_check(struct value *v)
{
    if (v->type != INTGR)
	int_error(NO_CARET, "non-integer passed to boolean operator");
}



/* Internal operators of the stack-machine, not directly represented
 * by any user-visible operator, or using private status variables
 * directly */

/* converts top-of-stack to boolean */
void
f_bool(union argument *x)
{
    (void) x;			/* avoid -Wunused warning */

    int_check(&top_of_stack);
    top_of_stack.v.int_val = !!top_of_stack.v.int_val;
}


void
f_jump(union argument *x)
{
    (void) x;			/* avoid -Wunused warning */
    jump_offset = x->j_arg;
}


void
f_jumpz(union argument *x)
{
    struct value a;

    (void) x;			/* avoid -Wunused warning */
    int_check(&top_of_stack);
    if (top_of_stack.v.int_val) {	/* non-zero --> no jump*/
	(void) pop(&a);
    } else
	jump_offset = x->j_arg;	/* leave the argument on TOS */
}


void
f_jumpnz(union argument *x)
{
    struct value a;

    (void) x;			/* avoid -Wunused warning */
    int_check(&top_of_stack);
    if (top_of_stack.v.int_val)	/* non-zero */
	jump_offset = x->j_arg;	/* leave the argument on TOS */
    else {
	(void) pop(&a);
    }
}

void
f_jtern(union argument *x)
{
    struct value a;

    (void) x;			/* avoid -Wunused warning */
    int_check(pop(&a));
    if (! a.v.int_val)
	jump_offset = x->j_arg;	/* go jump to FALSE code */
}

/* This is the heart of the expression evaluation module: the stack
   program execution loop.

  'ft' is a table containing C functions within this program.

   An 'action_table' contains pointers to these functions and
   arguments to be passed to them.

   at_ptr is a pointer to the action table which must be executed
   (evaluated).

   so the iterated line executes the function indexed by the at_ptr
   and passes the address of the argument which is pointed to by the
   arg_ptr

*/

void
execute_at(struct at_type *at_ptr)
{
    int instruction_index, operator, count;
    int saved_jump_offset = jump_offset;

    count = at_ptr->a_count;
    for (instruction_index = 0; instruction_index < count;) {
	operator = (int) at_ptr->actions[instruction_index].index;
	jump_offset = 1;	/* jump operators can modify this */
	(*ft[operator].func) (&(at_ptr->actions[instruction_index].arg));
	assert(is_jump(operator) || (jump_offset == 1));
	instruction_index += jump_offset;
    }

    jump_offset = saved_jump_offset;
}

/* As of May 2013 input of Inf/NaN values through evaluation is treated */
/* equivalently to direct input of a formatted value.  See imageNaN.dem. */
void
evaluate_at(struct at_type *at_ptr, struct value *val_ptr)
{
    /* A test for if (undefined) is allowed only immediately following
     * evalute_at() or eval_link_function().  Both must clear it on entry
     * so that the value on return reflects what really happened.
     */
    undefined = FALSE;
    val_ptr->type = NOTDEFINED;

    errno = 0;
    reset_stack();

    if (!evaluate_inside_using || !df_nofpe_trap) {
	if (SETJMP(fpe_env, 1))
	    return;
	(void) signal(SIGFPE, (sigfunc) fpe);
    }

    execute_at(at_ptr);

    if (!evaluate_inside_using || !df_nofpe_trap)
	(void) signal(SIGFPE, SIG_DFL);

    if (errno == EDOM || errno == ERANGE)
	undefined = TRUE;
    else if (!undefined) {
	(void) pop(val_ptr);
	check_stack();
    }

    if (!undefined && val_ptr->type == ARRAY) {
	/* Aug 2016: error rather than warning because too many places
	 * cannot deal with UNDEFINED or NaN where they were expecting a number
	 * E.g. load_one_range()
	 */
	val_ptr->type = NOTDEFINED;
	if (!string_result_only)
	    int_error(NO_CARET, "evaluate_at: unsupported array operation");
    }
}

void
real_free_at(struct at_type *at_ptr)
{
    int i;
    /* All string constants belonging to this action table have to be
     * freed before destruction. */
    if (!at_ptr)
        return;
    for (i=0; i<at_ptr->a_count; i++) {
	struct at_entry *a = &(at_ptr->actions[i]);
	/* if union a->arg is used as a->arg.v_arg free potential string */
	if ( a->index == PUSHC || a->index == DOLLARS )
	    gpfree_string(&(a->arg.v_arg));
	/* a summation contains its own action table wrapped in a private udf */
	if (a->index == SUM) {
	    real_free_at(a->arg.udf_arg->at);
	    free(a->arg.udf_arg);
	}
#ifdef HAVE_EXTERNAL_FUNCTIONS
	/* external function calls contain a parameter list */
	if (a->index == CALLE)
	    free(a->arg.exf_arg);
#endif
    }
    free(at_ptr);
}

/* EAM July 2003 - Return pointer to udv with this name; if the key does not
 * match any existing udv names, create a new one and return a pointer to it.
 */
struct udvt_entry *
add_udv_by_name(char *key)
{
    struct udvt_entry **udv_ptr = &first_udv;

    /* check if it's already in the table... */

    while (*udv_ptr) {
	if (!strcmp(key, (*udv_ptr)->udv_name))
	    return (*udv_ptr);
	udv_ptr = &((*udv_ptr)->next_udv);
    }

    *udv_ptr = (struct udvt_entry *)
	gp_alloc(sizeof(struct udvt_entry), "value");
    (*udv_ptr)->next_udv = NULL;
    (*udv_ptr)->udv_name = gp_strdup(key);
    (*udv_ptr)->udv_value.type = NOTDEFINED;
    return (*udv_ptr);
}

struct udvt_entry *
get_udv_by_name(char *key)
{
    struct udvt_entry *udv = first_udv;

    while (udv) {
        if (!strcmp(key, udv->udv_name))
            return udv;

        udv = udv->next_udv;
    }

    return NULL;
}

/* This doesn't really delete, it just marks the udv as undefined */
void
del_udv_by_name(char *key, TBOOLEAN wildcard)
{
    struct udvt_entry *udv_ptr = *udv_user_head;

    while (udv_ptr) {
	/* Forbidden to delete GPVAL_* */
	if (!strncmp(udv_ptr->udv_name,"GPVAL",5))
	    ;
	else if (!strncmp(udv_ptr->udv_name,"GNUTERM",7))
	    ;

 	/* exact match */
	else if (!wildcard && !strcmp(key, udv_ptr->udv_name)) {
	    gpfree_vgrid(udv_ptr);
	    free_value(&(udv_ptr->udv_value));
	    udv_ptr->udv_value.type = NOTDEFINED;
	    break;
	}

	/* wildcard match: prefix matches */
	else if ( wildcard && !strncmp(key, udv_ptr->udv_name, strlen(key)) ) {
	    gpfree_vgrid(udv_ptr);
	    free_value(&(udv_ptr->udv_value));
	    udv_ptr->udv_value.type = NOTDEFINED;
	    /* no break - keep looking! */
	}

	udv_ptr = udv_ptr->next_udv;
    }
}

/* Clear (delete) all user defined functions */
void
clear_udf_list()
{
    struct udft_entry *udf_ptr = first_udf;
    struct udft_entry *udf_next;

    while (udf_ptr) {
	free(udf_ptr->udf_name);
	free(udf_ptr->definition);
	free_at(udf_ptr->at);
	udf_next = udf_ptr->next_udf;
	free(udf_ptr);
	udf_ptr = udf_next;
    }
    first_udf = NULL;
}

static void update_plot_bounds(void);
static void fill_gpval_axis(AXIS_INDEX axis);
static void fill_gpval_sysinfo(void);
static void set_gpval_axis_sth_double(const char *prefix, AXIS_INDEX axis, const char *suffix, double value);

static void
set_gpval_axis_sth_double(const char *prefix, AXIS_INDEX axis, const char *suffix, double value)
{
    struct udvt_entry *v;
    char *cc, s[24];
    sprintf(s, "%s_%s_%s", prefix, axis_name(axis), suffix);
    for (cc=s; *cc; cc++)
	*cc = toupper((unsigned char)*cc); /* make the name uppercase */
    v = add_udv_by_name(s);
    if (!v)
	return; /* should not happen */
    Gcomplex(&v->udv_value, value, 0);
}

static void
fill_gpval_axis(AXIS_INDEX axis)
{
    const char *prefix = "GPVAL";
    AXIS *ap = &axis_array[axis];
    set_gpval_axis_sth_double(prefix, axis, "MIN", ap->min);
    set_gpval_axis_sth_double(prefix, axis, "MAX", ap->max);
    set_gpval_axis_sth_double(prefix, axis, "LOG", ap->base);

    if (axis < POLAR_AXIS) {
	set_gpval_axis_sth_double("GPVAL_DATA", axis, "MIN", ap->data_min);
	set_gpval_axis_sth_double("GPVAL_DATA", axis, "MAX", ap->data_max);
    }
}

/* Fill variable "var" visible by "show var" or "show var all" ("GPVAL_*")
 * by the given value (string, integer, float, complex).
 */
void
fill_gpval_string(char *var, const char *stringvalue)
{
    struct udvt_entry *v = add_udv_by_name(var);
    if (!v)
	return;
    if (v->udv_value.type == STRING && !strcmp(v->udv_value.v.string_val, stringvalue))
	return;
    else
	gpfree_string(&v->udv_value);
    Gstring(&v->udv_value, gp_strdup(stringvalue));
}

void
fill_gpval_integer(char *var, intgr_t value)
{
    struct udvt_entry *v = add_udv_by_name(var);
    if (!v)
	return;
    Ginteger(&v->udv_value, value);
}

void
fill_gpval_float(char *var, double value)
{
    struct udvt_entry *v = add_udv_by_name(var);
    if (!v)
	return;
    Gcomplex(&v->udv_value, value, 0);
}

void
fill_gpval_complex(char *var, double areal, double aimag)
{
    struct udvt_entry *v = add_udv_by_name(var);
    if (!v)
	return;
    Gcomplex(&v->udv_value, areal, aimag);
}

/*
 * Export axis bounds in terminal coordinates from previous plot.
 * This allows offline mapping of pixel coordinates onto plot coordinates.
 */
static void
update_plot_bounds(void)
{
    fill_gpval_float("GPVAL_TERM_XMIN", (double)axis_array[FIRST_X_AXIS].term_lower / term->tscale);
    fill_gpval_float("GPVAL_TERM_XMAX", (double)axis_array[FIRST_X_AXIS].term_upper / term->tscale);
    fill_gpval_float("GPVAL_TERM_YMIN", (double)axis_array[FIRST_Y_AXIS].term_lower / term->tscale);
    fill_gpval_float("GPVAL_TERM_YMAX", (double)axis_array[FIRST_Y_AXIS].term_upper / term->tscale);
    fill_gpval_integer("GPVAL_TERM_XSIZE", canvas.xright+1);
    fill_gpval_integer("GPVAL_TERM_YSIZE", canvas.ytop+1);
    fill_gpval_integer("GPVAL_TERM_SCALE", term->tscale);
    /* May be useful for debugging font problems */
    fill_gpval_integer("GPVAL_TERM_HCHAR", term->h_char);
    fill_gpval_integer("GPVAL_TERM_VCHAR", term->v_char);
}

/*
 * Put all the handling for GPVAL_* variables in this one routine.
 * We call it from one of several contexts:
 * 0: following a successful set/unset command
 * 1: following a successful plot/splot
 * 2: following an unsuccessful command (int_error)
 * 3: program entry
 * 4: explicit reset of error status
 * 5: directory changed
 * 6: X11 Window ID changed
 */
void
update_gpval_variables(int context)
{
    /* These values may change during a plot command due to auto range */
    if (context == 1) {
	fill_gpval_axis(FIRST_X_AXIS);
	fill_gpval_axis(FIRST_Y_AXIS);
	fill_gpval_axis(SECOND_X_AXIS);
	fill_gpval_axis(SECOND_Y_AXIS);
	fill_gpval_axis(FIRST_Z_AXIS);
	fill_gpval_axis(COLOR_AXIS);
	fill_gpval_axis(T_AXIS);
	fill_gpval_axis(U_AXIS);
	fill_gpval_axis(V_AXIS);
	fill_gpval_float("GPVAL_R_MIN", R_AXIS.min);
	fill_gpval_float("GPVAL_R_MAX", R_AXIS.max);
	fill_gpval_float("GPVAL_R_LOG", R_AXIS.base);
	update_plot_bounds();
	fill_gpval_integer("GPVAL_PLOT", is_3d_plot ? 0:1);
	fill_gpval_integer("GPVAL_SPLOT", is_3d_plot ? 1:0);
	fill_gpval_integer("GPVAL_VIEW_MAP", splot_map ? 1:0);
	fill_gpval_float("GPVAL_VIEW_ROT_X", surface_rot_x);
	fill_gpval_float("GPVAL_VIEW_ROT_Z", surface_rot_z);
	fill_gpval_float("GPVAL_VIEW_SCALE", surface_scale);
	fill_gpval_float("GPVAL_VIEW_ZSCALE", surface_zscale);
	fill_gpval_float("GPVAL_VIEW_AZIMUTH", azimuth);

	/* Screen coordinates of 3D rotational center and radius of the sphere */
	/* in which x/y axes are drawn after 'set view equal xy[z]' */
	fill_gpval_float("GPVAL_VIEW_XCENT",
		(double)(canvas.xright+1 - xmiddle)/(double)(canvas.xright+1));
	fill_gpval_float("GPVAL_VIEW_YCENT",
		1.0 - (double)(canvas.ytop+1 - ymiddle)/(double)(canvas.ytop+1));
	fill_gpval_float("GPVAL_VIEW_RADIUS",
		0.5 * surface_scale * xscaler/(double)(canvas.xright+1));

	return;
    }

    /* These are set after every "set" command, which is kind of silly */
    /* because they only change after 'set term' 'set output' ...      */
    if (context == 0 || context == 2 || context == 3) {
	/* This prevents a segfault if term==NULL, which can */
	/* happen if set_terminal() exits via int_error().   */
	if (!term)
	    fill_gpval_string("GPVAL_TERM", "unknown");
	else
	    fill_gpval_string("GPVAL_TERM", (char *)(term->name));

	fill_gpval_string("GPVAL_TERMOPTIONS", term_options);
	fill_gpval_string("GPVAL_OUTPUT", (outstr) ? outstr : "");
	fill_gpval_string("GPVAL_ENCODING", encoding_names[encoding]);
	fill_gpval_string("GPVAL_MINUS_SIGN", minus_sign ? minus_sign : "-");
	fill_gpval_string("GPVAL_MICRO", micro ? micro : "u");
	fill_gpval_string("GPVAL_DEGREE_SIGN", degree_sign);
    }

    /* If we are called from int_error() then set the error state */
    if (context == 2)
	fill_gpval_integer("GPVAL_ERRNO", 1);

    /* These initializations need only be done once, on program entry */
    if (context == 3) {
	struct udvt_entry *v = add_udv_by_name("GPVAL_VERSION");
	char *tmp;
	if (v && v->udv_value.type == NOTDEFINED)
	    Gcomplex(&v->udv_value, atof(gnuplot_version), 0);
	v = add_udv_by_name("GPVAL_PATCHLEVEL");
	if (v && v->udv_value.type == NOTDEFINED)
	    fill_gpval_string("GPVAL_PATCHLEVEL", gnuplot_patchlevel);
	v = add_udv_by_name("GPVAL_COMPILE_OPTIONS");
	if (v && v->udv_value.type == NOTDEFINED)
	    fill_gpval_string("GPVAL_COMPILE_OPTIONS", compile_options);

	/* Start-up values */
	fill_gpval_integer("GPVAL_MULTIPLOT", 0);
	fill_gpval_integer("GPVAL_PLOT", 0);
	fill_gpval_integer("GPVAL_SPLOT", 0);

	tmp = get_terminals_names();
	fill_gpval_string("GPVAL_TERMINALS", tmp);
	free(tmp);

	fill_gpval_string("GPVAL_ENCODING", encoding_names[encoding]);

	/* Permanent copy of user-clobberable variables pi and NaN */
	fill_gpval_float("GPVAL_pi", M_PI);
	fill_gpval_float("GPVAL_NaN", not_a_number());

	/* System information */
	fill_gpval_sysinfo();
    }

    if (context == 3 || context == 4) {
	fill_gpval_integer("GPVAL_ERRNO", 0);
	fill_gpval_string("GPVAL_ERRMSG","");
	fill_gpval_integer("GPVAL_SYSTEM_ERRNO", 0);
	fill_gpval_string("GPVAL_SYSTEM_ERRMSG","");
    }

    /* GPVAL_PWD is unreliable.  If the current directory becomes invalid,
     * GPVAL_PWD does not reflect this.  If this matters, the user can
     * instead do something like    MY_PWD = "`pwd`"
     */
    if (context == 3 || context == 5) {
	char *save_file = gp_alloc(PATH_MAX, "GPVAL_PWD");
	int ierror = (GP_GETCWD(save_file, PATH_MAX) == NULL);
	fill_gpval_string("GPVAL_PWD", ierror ? "" : save_file);
	free(save_file);
    }

    if (context == 6) {
	fill_gpval_integer("GPVAL_TERM_WINDOWID", current_x11_windowid);
    }
}

/* System information is stored in GPVAL_BITS GPVAL_MACHINE GPVAL_SYSNAME */
#ifdef HAVE_UNAME
# include <sys/utsname.h>
#elif defined(_WIN32)
# include <windows.h>
#endif

void
fill_gpval_sysinfo()
{
/* For linux/posix systems with uname */
#ifdef HAVE_UNAME
    struct utsname uts;

    if (uname(&uts) < 0)
	return;
    fill_gpval_string("GPVAL_SYSNAME", uts.sysname);
    fill_gpval_string("GPVAL_MACHINE", uts.machine);

/* For Windows systems */
#elif defined(_WIN32)
    SYSTEM_INFO stInfo;
    OSVERSIONINFO osvi;
    char s[30];

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    snprintf(s, 30, "Windows_NT-%ld.%ld", osvi.dwMajorVersion, osvi.dwMinorVersion);
    fill_gpval_string("GPVAL_SYSNAME", s);

    GetSystemInfo(&stInfo);
    switch (stInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        fill_gpval_string("GPVAL_MACHINE", "x86");
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        fill_gpval_string("GPVAL_MACHINE", "ia64");
       break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        fill_gpval_string("GPVAL_MACHINE", "x86_64");
        break;
    default:
        fill_gpval_string("GPVAL_MACHINE", "unknown");
    }
#endif

/* For all systems */
    fill_gpval_integer("GPVAL_BITS", 8 * sizeof(void *));
}

/* Callable wrapper for the words() internal function */
int
gp_words(char *string)
{
    struct value a;

    push(Gstring(&a, string));
    f_words((union argument *)NULL);
    pop(&a);

    return a.v.int_val;
}

/* Callable wrapper for the word() internal function */
char *
gp_word(char *string, int i)
{
    struct value a;

    push(Gstring(&a, string));
    push(Ginteger(&a, (intgr_t)i));
    f_word((union argument *)NULL);
    pop(&a);

    return a.v.string_val;
}

