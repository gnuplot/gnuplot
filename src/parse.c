#ifndef lint
static char *RCSid() { return RCSid("$Id: parse.c,v 1.10.2.1 2000/05/16 11:26:40 joze Exp $"); }
#endif

/* GNUPLOT - parse.c */

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <signal.h>
#include <setjmp.h>

#ifdef HAVE_SIGSETJMP
# define SETJMP(env, save_signals) sigsetjmp(env, save_signals)
# define LONGJMP(env, retval) siglongjmp(env, retval)
#else
# define SETJMP(env, save_signals) setjmp(env)
# define LONGJMP(env, retval) longjmp(env, retval)
#endif

#include "parse.h"

#include "alloc.h"
#include "command.h"
#include "eval.h"
#include "help.h"
#include "internal.h"
#include "util.h"

static RETSIGTYPE fpe __PROTO((int an_int));
static void extend_at __PROTO((void));
static union argument *add_action __PROTO((enum operators sf_index));
static void express __PROTO((void));
static void xterm __PROTO((void));
static void aterm __PROTO((void));
static void bterm __PROTO((void));
static void cterm __PROTO((void));
static void dterm __PROTO((void));
static void eterm __PROTO((void));
static void fterm __PROTO((void));
static void gterm __PROTO((void));
static void hterm __PROTO((void));
static void factor __PROTO((void));
static void xterms __PROTO((void));
static void aterms __PROTO((void));
static void bterms __PROTO((void));
static void cterms __PROTO((void));
static void dterms __PROTO((void));
static void eterms __PROTO((void));
static void fterms __PROTO((void));
static void gterms __PROTO((void));
static void hterms __PROTO((void));
static void iterms __PROTO((void));
static void unary __PROTO((void));

static struct at_type *at = NULL;
static int at_size = 0;
#if defined(_Windows) && !defined(WIN32)
static jmp_buf far fpe_env;
#else
static jmp_buf fpe_env;
#endif

#define dummy (struct value *) 0

static RETSIGTYPE
fpe(an_int)
int an_int;
{
#if defined(MSDOS) && !defined(__EMX__) && !defined(DJGPP) && !defined(_Windows) || defined(DOS386)
    /* thanks to lotto@wjh12.UUCP for telling us about this  */
    _fpreset();
#endif

#ifdef OS2
    (void) signal(an_int, SIG_ACK);
#else
    (void) signal(SIGFPE, (sigfunc) fpe);
#endif
    undefined = TRUE;
    LONGJMP(fpe_env, TRUE);
}


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

pfm_$fh_func_val_t
apollo_sigfpe(pfm_$fault_rec_t & fault_rec)
{
    kill(getpid(), SIGFPE);
    return pfm_$continue_fault_handling;
}

apollo_pfm_catch()
{
    status_$t status;
    pfm_$establish_fault_handler(fault_$fp_bsun, pfm_$fh_backstop,
				 apollo_sigfpe, &status);
    pfm_$establish_fault_handler(fault_$fp_sig_nan, pfm_$fh_backstop,
				 apollo_sigfpe, &status);
}
#endif /* APOLLO */


void
evaluate_at(at_ptr, val_ptr)
struct at_type *at_ptr;
struct value *val_ptr;
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


struct value *
const_express(valptr)
struct value *valptr;
{
    register int tkn = c_token;

    if (END_OF_COMMAND)
	int_error(c_token, "constant expression required");

    /* div - no dummy variables in a constant expression */
    dummy_func = NULL;

    evaluate_at(temp_at(), valptr);	/* run it and send answer back */
    if (undefined) {
	int_error(tkn, "undefined value");
    }
    return (valptr);
}


/* if dummy_dunc == NULL on entry, do not attempt to compile dummy variables
 * - div
 */
struct at_type *
temp_at()
{
    /* build a static action table and return its
     * pointer */

    if (at != NULL) {
	free(at);
	at = NULL;
    }
    at = (struct at_type *) gp_alloc(sizeof(struct at_type), "action table");

    at->a_count = 0;		/* reset action table !!! */
    at_size = MAX_AT_LEN;
    express();
    return (at);
}


/* build an action table, put it in dynamic memory, and return its pointer */

struct at_type *
perm_at()
{
    register struct at_type *at_ptr;
    size_t len;

    (void) temp_at();
    len = sizeof(struct at_type) +
     (at->a_count - MAX_AT_LEN) * sizeof(struct at_entry);
    at_ptr = (struct at_type *) gp_realloc(at, len, "perm_at");
    at = NULL;			/* invalidate at pointer */
    return (at_ptr);
}

static void
extend_at()
{
    size_t newsize = sizeof(struct at_type) + at_size * sizeof(struct at_entry);

    at = gp_realloc(at, newsize, "extend_at");
    at_size += MAX_AT_LEN;
    FPRINTF((stderr, "Extending at size to %d\n", at_size));
}

/* moved from eval.c, the function is only called from this module */
static union argument *
add_action(sf_index)
enum operators sf_index;	/* index of p-code function */
{
    if (at->a_count >= at_size) {
	extend_at();
    }
    at->actions[at->a_count].index = sf_index;
    return (&(at->actions[at->a_count++].arg));
}


static void
express()
{				/* full expressions */
    xterm();
    xterms();
}

static void
xterm()
{				/* ? : expressions */
    aterm();
    aterms();
}


static void
aterm()
{
    bterm();
    bterms();
}


static void
bterm()
{
    cterm();
    cterms();
}


static void
cterm()
{
    dterm();
    dterms();
}


static void
dterm()
{
    eterm();
    eterms();
}


static void
eterm()
{
    fterm();
    fterms();
}


static void
fterm()
{
    gterm();
    gterms();
}


static void
gterm()
{
    hterm();
    hterms();
}


static void
hterm()
{
    unary();			/* - things */
    iterms();			/* * / % */
}


static void
factor()
{
    if (equals(c_token, "(")) {
	c_token++;
	express();
	if (!equals(c_token, ")"))
	    int_error(c_token, "')' expected");
	c_token++;
    } else if (equals(c_token, "$")) {
	struct value a;
	if (!isanumber(++c_token))
	    int_error(c_token, "Column number expected");
	convert(&a, c_token++);
	if (a.type != INTGR || a.v.int_val < 0)
	    int_error(c_token, "Positive integer expected");
	add_action(DOLLARS)->v_arg = a;
    } else if (isanumber(c_token)) {
	/* work around HP 9000S/300 HP-UX 9.10 cc limitation ... */
#if defined(__hpux) && defined(__hp9000s300) && !defined(__GNUC__)
	union argument *foo = add_action(PUSHC);
	convert(&(foo->v_arg), c_token);
#else
	convert(&(add_action(PUSHC)->v_arg), c_token);
#endif
	c_token++;
    } else if (isletter(c_token)) {
	if ((c_token + 1 < num_tokens) && equals(c_token + 1, "(")) {
	    enum operators value = standard(c_token);
	    if (value) {	/* it's a standard function */
		c_token += 2;
		express();
		if (equals(c_token, ",")) {
		    while (equals(c_token, ",")) {
			c_token += 1;
			express();
		    }
		}
		if (!equals(c_token, ")"))
		    int_error(c_token, "')' expected");
		c_token++;
		(void) add_action(value);
	    } else {
		enum operators call_type = (int) CALL;
		int tok = c_token;
		c_token += 2;
		express();
		if (equals(c_token, ",")) {
		    struct value num_params;
		    num_params.type = INTGR;
		    num_params.v.int_val = 1;
		    while (equals(c_token, ",")) {
			num_params.v.int_val += 1;
			c_token += 1;
			express();
		    }
		    add_action(PUSHC)->v_arg = num_params;
		    call_type = (int) CALLN;
		}
		if (!equals(c_token, ")"))
		    int_error(c_token, "')' expected");
		c_token++;
		add_action(call_type)->udf_arg = add_udf(tok);
	    }
	    /* dummy_func==NULL is a flag to say no dummy variables active */
	} else if (dummy_func) {
	    if (equals(c_token, c_dummy_var[0])) {
		c_token++;
		add_action(PUSHD1)->udf_arg = dummy_func;
	    } else if (equals(c_token, c_dummy_var[1])) {
		c_token++;
		add_action(PUSHD2)->udf_arg = dummy_func;
	    } else {
		int i, param = 0;
		for (i = 2; i < MAX_NUM_VAR; i++) {
		    if (equals(c_token, c_dummy_var[i])) {
			struct value num_params;
			num_params.type = INTGR;
			num_params.v.int_val = i;
			param = 1;
			c_token++;
			add_action(PUSHC)->v_arg = num_params;
			add_action(PUSHD)->udf_arg = dummy_func;
			break;
		    }
		}
		if (!param) {	/* defined variable */
		    add_action(PUSH)->udv_arg = add_udv(c_token);
		    c_token++;
		}
	    }
	    /* its a variable, with no dummies active - div */
	} else {
	    add_action(PUSH)->udv_arg = add_udv(c_token);
	    c_token++;
	}
    }
    /* end if letter */
    else
	int_error(c_token, "invalid expression ");

    /* add action code for ! (factorial) operator */
    while (equals(c_token, "!")) {
	c_token++;
	(void) add_action(FACTORIAL);
    }
    /* add action code for ** operator */
    if (equals(c_token, "**")) {
	c_token++;
	unary();
	(void) add_action(POWER);
    }
}



static void
xterms()
{
    /* create action code for ? : expressions */

    if (equals(c_token, "?")) {
	register int savepc1, savepc2;
	register union argument *argptr1, *argptr2;
	c_token++;
	savepc1 = at->a_count;
	argptr1 = add_action(JTERN);
	express();
	if (!equals(c_token, ":"))
	    int_error(c_token, "expecting ':'");
	c_token++;
	savepc2 = at->a_count;
	argptr2 = add_action(JUMP);
	argptr1->j_arg = at->a_count - savepc1;
	express();
	argptr2->j_arg = at->a_count - savepc2;
    }
}


static void
aterms()
{
    /* create action codes for || operator */

    while (equals(c_token, "||")) {
	register int savepc;
	register union argument *argptr;
	c_token++;
	savepc = at->a_count;
	argptr = add_action(JUMPNZ);	/* short-circuit if already
					 * TRUE */
	aterm();
	argptr->j_arg = at->a_count - savepc;	/* offset for jump */
	(void) add_action(BOOLE);
    }
}


static void
bterms()
{
    /* create action code for && operator */

    while (equals(c_token, "&&")) {
	register int savepc;
	register union argument *argptr;
	c_token++;
	savepc = at->a_count;
	argptr = add_action(JUMPZ);	/* short-circuit if already
					 * FALSE */
	bterm();
	argptr->j_arg = at->a_count - savepc;	/* offset for jump */
	(void) add_action(BOOLE);
    }
}


static void
cterms()
{
    /* create action code for | operator */

    while (equals(c_token, "|")) {
	c_token++;
	cterm();
	(void) add_action(BOR);
    }
}


static void
dterms()
{
    /* create action code for ^ operator */

    while (equals(c_token, "^")) {
	c_token++;
	dterm();
	(void) add_action(XOR);
    }
}


static void
eterms()
{
    /* create action code for & operator */

    while (equals(c_token, "&")) {
	c_token++;
	eterm();
	(void) add_action(BAND);
    }
}


static void
fterms()
{
    /* create action codes for == and !=
     * operators */

    while (TRUE) {
	if (equals(c_token, "==")) {
	    c_token++;
	    fterm();
	    (void) add_action(EQ);
	} else if (equals(c_token, "!=")) {
	    c_token++;
	    fterm();
	    (void) add_action(NE);
	} else
	    break;
    }
}


static void
gterms()
{
    /* create action code for < > >= or <=
     * operators */

    while (TRUE) {
	/* I hate "else if" statements */
	if (equals(c_token, ">")) {
	    c_token++;
	    gterm();
	    (void) add_action(GT);
	} else if (equals(c_token, "<")) {
	    c_token++;
	    gterm();
	    (void) add_action(LT);
	} else if (equals(c_token, ">=")) {
	    c_token++;
	    gterm();
	    (void) add_action(GE);
	} else if (equals(c_token, "<=")) {
	    c_token++;
	    gterm();
	    (void) add_action(LE);
	} else
	    break;
    }

}



static void
hterms()
{
    /* create action codes for + and - operators */

    while (TRUE) {
	if (equals(c_token, "+")) {
	    c_token++;
	    hterm();
	    (void) add_action(PLUS);
	} else if (equals(c_token, "-")) {
	    c_token++;
	    hterm();
	    (void) add_action(MINUS);
	} else
	    break;
    }
}


static void
iterms()
{
    /* add action code for * / and % operators */

    while (TRUE) {
	if (equals(c_token, "*")) {
	    c_token++;
	    unary();
	    (void) add_action(MULT);
	} else if (equals(c_token, "/")) {
	    c_token++;
	    unary();
	    (void) add_action(DIV);
	} else if (equals(c_token, "%")) {
	    c_token++;
	    unary();
	    (void) add_action(MOD);
	} else
	    break;
    }
}


static void
unary()
{
    /* add code for unary operators */

    if (equals(c_token, "!")) {
	c_token++;
	unary();
	(void) add_action(LNOT);
    } else if (equals(c_token, "~")) {
	c_token++;
	unary();
	(void) add_action(BNOT);
    } else if (equals(c_token, "-")) {
	c_token++;
	unary();
	(void) add_action(UMINUS);
    } else if (equals(c_token, "+")) {	/* unary + is no-op */
	c_token++;
	unary();
    } else
	factor();
}
