#ifndef lint
static char *RCSid() { return RCSid("$Id: eval.c,v 1.19 2004/08/17 21:21:25 sfeam Exp $"); }
#endif

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

/* HBB 20010724: I moved several variables and functions from parse.c
 * to here, because they're involved with *evaluating* functions, not
 * with parsing them: evaluate_at(), fpe(), the APOLLO signal handling
 * stuff, and fpe_env */

#include "eval.h"

#include "syscfg.h"
#include "alloc.h"
#include "datafile.h"
#include "internal.h"
#include "specfun.h"
#include "standard.h"
#include "util.h"

#include <signal.h>
#include <setjmp.h>

/* Internal prototypes */
static char *num_to_str __PROTO((double r));
static RETSIGTYPE fpe __PROTO((int an_int));
#ifdef APOLLO
static pfm_$fh_func_val_t apollo_sigfpe(pfm_$fault_rec_t & fault_rec)
#endif

/* Global variables exported by this module */
struct udvt_entry udv_pi = { NULL, "pi", FALSE, {INTGR, {0} } };
/* first in linked list */
struct udvt_entry *first_udv = &udv_pi;
struct udft_entry *first_udf = NULL;

TBOOLEAN undefined;

/* The stack this operates on */
static struct value stack[STACK_DEPTH];
static int s_p = -1;		/* stack pointer */
#define top_of_stack stack[s_p]

static int jump_offset;		/* to be modified by 'jump' operators */

/* The table of built-in functions */
/* HBB 20010725: I've removed all the casts to type (FUNC_PTR) ---
 * According to ANSI/ISO C Standards it causes undefined behaviouf if
 * you cast a function pointer to any other type, including a function
 * pointer with a different set of arguments, and then call the
 * function.  Instead, I made all these functions adhere to the common
 * type, directly */
const struct ft_entry GPFAR ft[] =
{
    /* internal functions: */
    {"push",  f_push},
    {"pushc",  f_pushc},
    {"pushd1",  f_pushd1},
    {"pushd2",  f_pushd2},
    {"pushd",  f_pushd},
#ifdef GP_ISVAR
    {"pushv", f_pushv},
#endif  /*GP_ISVAR*/
    {"call",  f_call},
    {"calln",  f_calln},
    {"lnot",  f_lnot},
    {"bnot",  f_bnot},
    {"uminus",  f_uminus},
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
    {"plus",  f_plus},
    {"minus",  f_minus},
    {"mult",  f_mult},
    {"div",  f_div},
    {"mod",  f_mod},
    {"power",  f_power},
    {"factorial",  f_factorial},
    {"bool",  f_bool},
    {"dollars",  f_dollars},	/* for using extension */
#ifdef GP_STRING_VARS
    {"concatenate",  f_concatenate},	/* for string variables only */
    {"eqs",  f_eqs},			/* for string variables only */
    {"nes",  f_nes},			/* for string variables only */
#endif
    {"jump",  f_jump},
    {"jumpz",  f_jumpz},
    {"jumpnz",  f_jumpnz},
    {"jtern",  f_jtern},

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
    {"int",  f_int},
    {"abs",  f_abs},
    {"sgn",  f_sgn},
    {"sqrt",  f_sqrt},
    {"exp",  f_exp},
    {"log10",  f_log10},
    {"log",  f_log},
    {"besj0",  f_besj0},
    {"besj1",  f_besj1},
    {"besy0",  f_besy0},
    {"besy1",  f_besy1},
    {"erf",  f_erf},
    {"erfc",  f_erfc},
    {"gamma",  f_gamma},
    {"lgamma",  f_lgamma},
    {"ibeta",  f_ibeta},
    {"igamma",  f_igamma},
    {"rand",  f_rand},
    {"floor",  f_floor},
    {"ceil",  f_ceil},
#ifdef GP_ISVAR
    {"defined",  f_isvar},       /* isvar function */
#endif  /*GP_ISVAR*/

    {"norm",  f_normal},	/* XXX-JG */
    {"inverf",  f_inverse_erf},	/* XXX-JG */
    {"invnorm",  f_inverse_normal},	/* XXX-JG */
    {"asinh",  f_asinh},
    {"acosh",  f_acosh},
    {"atanh",  f_atanh},
    {"lambertw",  f_lambertw}, /* HBB, from G.Kuhnle 20001107 */

    {"column",  f_column},	/* for using */
    {"valid",  f_valid},	/* for using */
    {"timecolumn",  f_timecolumn},	/* for using */

    {"tm_sec",  f_tmsec},	/* for timeseries */
    {"tm_min",  f_tmmin},	/* for timeseries */
    {"tm_hour",  f_tmhour},	/* for timeseries */
    {"tm_mday",  f_tmmday},	/* for timeseries */
    {"tm_mon",  f_tmmon},	/* for timeseries */
    {"tm_year",  f_tmyear},	/* for timeseries */
    {"tm_wday",  f_tmwday},	/* for timeseries */
    {"tm_yday",  f_tmyday},	/* for timeseries */

#ifdef GP_STRING_VARS
    {"sprintf",  f_sprintf},	/* for string variables only */
    {"gprintf",  f_gprintf},	/* for string variables only */
#endif

    {NULL, NULL}
};

/* Module-local variables: */

#if defined(_Windows) && !defined(WIN32)
static JMP_BUF far fpe_env;
#else
static JMP_BUF fpe_env;
#endif

/* Internal helper functions: */

static RETSIGTYPE
fpe(int an_int)
{
#if defined(MSDOS) && !defined(__EMX__) && !defined(DJGPP) && !defined(_Windows) || defined(DOS386)
    /* thanks to lotto@wjh12.UUCP for telling us about this  */
    _fpreset();
#endif

    (void) an_int;		/* avoid -Wunused warning */
    (void) signal(SIGFPE, (sigfunc) fpe);
    undefined = TRUE;
    LONGJMP(fpe_env, TRUE);
}

/* FIXME HBB 20010724: do we really want this in *here*? Maybe it
 * should be in syscfg.c or somewhere similar. */
#ifdef APOLLO
# include <apollo/base.h>
# include <apollo/pfm.h>
# include <apollo/fault.h>

/*
 * On an Apollo, the OS can signal a couple errors that are not mapped into
 * SIGFPE, namely signalling NaN and branch on an unordered comparison.  I
 * suppose there are others, but none of these are documented, so I handle
 * them as they arise.
 *
 * Anyway, we need to catch these faults and signal SIGFPE.
 */

static pfm_$fh_func_val_t
apollo_sigfpe(pfm_$fault_rec_t & fault_rec)
{
    kill(getpid(), SIGFPE);
    return pfm_$continue_fault_handling;
}

/* This is called from main(), if the platform is an APOLLO */
void
apollo_pfm_catch()
{
    status_$t status;
    pfm_$establish_fault_handler(fault_$fp_bsun, pfm_$fh_backstop,
				 apollo_sigfpe, &status);
    pfm_$establish_fault_handler(fault_$fp_sig_nan, pfm_$fh_backstop,
				 apollo_sigfpe, &status);
}
#endif /* APOLLO */

/* Helper for disp_value(): display a single number in decimal
 * format. Rotates through 4 buffers 's[j]', and returns pointers to
 * them, to avoid execution ordering problems if this function is
 * called more than once between sequence points. */
static char *
num_to_str(double r)
{
    static int i = 0;
    static char s[4][25];
    int j = i++;

    if (i > 3)
	i = 0;

    sprintf(s[j], "%.15g", r);
    if (strchr(s[j], '.') == NULL &&
	strchr(s[j], 'e') == NULL &&
	strchr(s[j], 'E') == NULL)
	strcat(s[j], ".0");

    return s[j];
}

/* Exported functions */

/* First, some functions tha help other modules use 'struct value' ---
 * these might justify a separate module, but I'll stick with this,
 * for now */

/* Display a value in human-readable form. */
void
disp_value(FILE *fp, struct value *val)
{
    switch (val->type) {
    case INTGR:
	fprintf(fp, "%d", val->v.int_val);
	break;
    case CMPLX:
	if (val->v.cmplx_val.imag != 0.0)
	    fprintf(fp, "{%s, %s}",
		    num_to_str(val->v.cmplx_val.real),
		    num_to_str(val->v.cmplx_val.imag));
	else
	    fprintf(fp, "%s",
		    num_to_str(val->v.cmplx_val.real));
	break;
#ifdef GP_STRING_VARS
    case STRING:
    	if (val->v.string_val)
	    fprintf(fp, "%s", val->v.string_val);
	break;
#endif
    default:
	int_error(NO_CARET, "unknown type in disp_value()");
    }
}

/* returns the real part of val */
double
real(struct value *val)
{
    switch (val->type) {
    case INTGR:
	return ((double) val->v.int_val);
    case CMPLX:
	return (val->v.cmplx_val.real);
#ifdef GP_STRING_VARS
    case STRING:              /* is this ever used? */
	return (atof(val->v.string_val));
#endif
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
#ifdef GP_STRING_VARS
    case STRING:
	/* FIXME: It would be better to catch this earlier and treat it */
	/*        as a file name:   plot foo(x) using 1:2               */
	int_warn(NO_CARET, "encountered a string when expecting a number");
	int_error(NO_CARET, "NB: you cannot plot a string-valued function");
#endif
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
	return ((double) abs(val->v.int_val));
    case CMPLX:
	return (sqrt(val->v.cmplx_val.real *
		     val->v.cmplx_val.real +
		     val->v.cmplx_val.imag *
		     val->v.cmplx_val.imag));
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
Ginteger(struct value *a, int i)
{
    a->type = INTGR;
    a->v.int_val = i;
    return (a);
}

#ifdef GP_STRING_VARS
struct value *
Gstring(struct value *a, char *s)
{
    if (a->type == STRING && a->v.string_val)
	free(a->v.string_val);
    a->type = STRING;
    a->v.string_val = s;
    return (a);
}
#endif

/* some machines have trouble with exp(-x) for large x
 * if MINEXP is defined at compile time, use gp_exp(x) instead,
 * which returns 0 for exp(x) with x < MINEXP
 * exp(x) will already have been defined as gp_exp(x) in plot.h
 */

double
gp_exp(double x)
{
#ifdef MINEXP
    return (x < (MINEXP)) ? 0.0 : exp(x);
#else  /* MINEXP */
    int old_errno = errno;
    double result = exp(x);

    /* exp(-large) quite uselessly raises ERANGE --- stop that */
    if (result == 0.0)
	errno = old_errno;
    return result;
#endif /* MINEXP */
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


void
push(struct value *x)
{
    if (s_p == STACK_DEPTH - 1)
	int_error(NO_CARET, "stack overflow");
    stack[++s_p] = *x;
#ifdef GP_STRING_VARS
    /* WARNING - This is a memory leak if the string is not later freed */
    if (x->type == STRING && x->v.string_val)
	stack[s_p].v.string_val = gp_strdup(x->v.string_val);
#endif
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

   so the iterated line exectues the function indexed by the at_ptr
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

/* 20010724: moved here from parse.c, where it didn't belong */
void
evaluate_at(struct at_type *at_ptr, struct value *val_ptr)
{
    double temp;

    undefined = FALSE;
    errno = 0;
    reset_stack();

#ifndef DOSX286
    if (SETJMP(fpe_env, 1))
	return;			/* just bail out */
    (void) signal(SIGFPE, (sigfunc) fpe);
#endif

    execute_at(at_ptr);

#ifndef DOSX286
    (void) signal(SIGFPE, SIG_DFL);
#endif

    if (errno == EDOM || errno == ERANGE) {
	undefined = TRUE;
    } else if (!undefined) {	/* undefined (but not errno) may have been set by matherr */
	(void) pop(val_ptr);
	check_stack();
	/* At least one machine (ATT 3b1) computes Inf without a SIGFPE */
	temp = real(val_ptr);
	if (temp > VERYLARGE || temp < -VERYLARGE) {
	    undefined = TRUE;
	}
    }
#if defined(NeXT) || defined(ultrix)
    /*
     * linux was able to fit curves which NeXT gave up on -- traced it to
     * silently returning NaN for the undefined cases and plowing ahead
     * I can force that behavior this way.  (0.0/0.0 generates NaN)
     */
    if (undefined && (errno == EDOM || errno == ERANGE)) {	/* corey@cac */
	undefined = FALSE;
	errno = 0;
	Gcomplex(val_ptr, 0.0 / 0.0, 0.0 / 0.0);
    }
#endif /* NeXT || ultrix */

}


