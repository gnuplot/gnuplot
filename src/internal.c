#ifndef lint
static char *RCSid() { return RCSid("$Id: internal.c,v 1.12 2002/02/27 17:05:53 broeker Exp $"); }
#endif

/* GNUPLOT - internal.c */

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


#include "internal.h"

#include "stdfn.h"

#include "util.h"		/* for int_error() */

#include <math.h>

/*
 * Excerpt from the Solaris man page for matherr():
 *
 *   The The System V Interface Definition, Third Edition (SVID3)
 *   specifies  that  certain  libm functions call matherr() when
 *   exceptions are detected. Users may define their own  mechan-
 *   isms  for handling exceptions, by including a function named
 *   matherr() in their programs.
 */

#define matherr GP_MATHERR
#define exception GP_EXCEPTION_NAME

#ifdef HAVE_STRUCT_EXCEPTION_IN_MATH_H
# define STRUCT_EXCEPTION_P_X struct exception *x
#endif

int
matherr(STRUCT_EXCEPTION_P_X)
{
    return (undefined = TRUE);	/* don't print error message */
}
#undef matherr

#define BAD_DEFAULT default: int_error(NO_CARET, "interal error : type neither INT or CMPLX"); return;

void
f_push(x)
    union argument *x;		/* contains pointer to value to push; */
{
    struct udvt_entry *udv;

    udv = x->udv_arg;
    if (udv->udv_undef) {	/* undefined */
	int_error(NO_CARET, "undefined variable: %s", udv->udv_name);
    }
    push(&(udv->udv_value));
}


void
f_pushc(x)
    union argument *x;
{
    push(&(x->v_arg));
}


void
f_pushd1(x)
    union argument *x;
{
    push(&(x->udf_arg->dummy_values[0]));
}


void
f_pushd2(x)
    union argument *x;
{
    push(&(x->udf_arg->dummy_values[1]));
}


void
f_pushd(x)
    union argument *x;
{
    struct value param;
    (void) pop(&param);
    push(&(x->udf_arg->dummy_values[param.v.int_val]));
}


void
f_call(x)			/* execute a udf */
    union argument *x;
{
    register struct udft_entry *udf;
    struct value save_dummy;

    udf = x->udf_arg;
    if (!udf->at) {		/* undefined */
	int_error(NO_CARET, "undefined function: %s", udf->udf_name);
    }
    save_dummy = udf->dummy_values[0];
    (void) pop(&(udf->dummy_values[0]));

    execute_at(udf->at);
    udf->dummy_values[0] = save_dummy;
}


void
f_calln(x)			/* execute a udf of n variables */
    union argument *x;
{
    register struct udft_entry *udf;
    struct value save_dummy[MAX_NUM_VAR];

    int i;
    int num_pop;
    struct value num_params;

    udf = x->udf_arg;
    if (!udf->at) {		/* undefined */
	int_error(NO_CARET, "undefined function: %s", udf->udf_name);
    }
    for (i = 0; i < MAX_NUM_VAR; i++)
	save_dummy[i] = udf->dummy_values[i];

    /* if there are more parameters than the function is expecting */
    /* simply ignore the excess */
    (void) pop(&num_params);

    if (num_params.v.int_val > MAX_NUM_VAR) {
	/* pop the dummies that there is no room for */
	num_pop = num_params.v.int_val - MAX_NUM_VAR;
	for (i = 0; i < num_pop; i++)
	    (void) pop(&(udf->dummy_values[i]));

	num_pop = MAX_NUM_VAR;
    } else {
	num_pop = num_params.v.int_val;
    }

    /* pop parameters we can use */
    for (i = num_pop - 1; i >= 0; i--)
	(void) pop(&(udf->dummy_values[i]));

    execute_at(udf->at);
    for (i = 0; i < MAX_NUM_VAR; i++)
	udf->dummy_values[i] = save_dummy[i];
}


void
f_lnot(arg)
    union argument *arg;
{
    struct value a;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&a));
    push(Ginteger(&a, !a.v.int_val));
}


void
f_bnot(arg)
    union argument *arg;
{
    struct value a;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&a));
    push(Ginteger(&a, ~a.v.int_val));
}


void
f_lor(arg)
    union argument *arg;
{
    struct value a, b;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&b));
    int_check(pop(&a));
    push(Ginteger(&a, a.v.int_val || b.v.int_val));
}

void
f_land(arg)
    union argument *arg;
{
    struct value a, b;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&b));
    int_check(pop(&a));
    push(Ginteger(&a, a.v.int_val && b.v.int_val));
}


void
f_bor(arg)
    union argument *arg;
{
    struct value a, b;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&b));
    int_check(pop(&a));
    push(Ginteger(&a, a.v.int_val | b.v.int_val));
}


void
f_xor(arg)
    union argument *arg;
{
    struct value a, b;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&b));
    int_check(pop(&a));
    push(Ginteger(&a, a.v.int_val ^ b.v.int_val));
}


void
f_band(arg)
    union argument *arg;
{
    struct value a, b;

    (void) arg;			/* avoid -Wunused warning */
    int_check(pop(&b));
    int_check(pop(&a));
    push(Ginteger(&a, a.v.int_val & b.v.int_val));
}


void
f_uminus(arg)
    union argument *arg;
{
    struct value a;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	a.v.int_val = -a.v.int_val;
	break;
    case CMPLX:
	a.v.cmplx_val.real =
	    -a.v.cmplx_val.real;
	a.v.cmplx_val.imag =
	    -a.v.cmplx_val.imag;
	break;
	BAD_DEFAULT
    }
    push(&a);
}


void
f_eq(arg)
    union argument *arg;
{
    /* note: floating point equality is rare because of roundoff error! */
    struct value a, b;
    register int result = 0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    result = (a.v.int_val ==
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.int_val ==
		      b.v.cmplx_val.real &&
		      b.v.cmplx_val.imag == 0.0);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    result = (b.v.int_val == a.v.cmplx_val.real &&
		      a.v.cmplx_val.imag == 0.0);
	    break;
	case CMPLX:
	    result = (a.v.cmplx_val.real ==
		      b.v.cmplx_val.real &&
		      a.v.cmplx_val.imag ==
		      b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(Ginteger(&a, result));
}


void
f_ne(arg)
    union argument *arg;
{
    struct value a, b;
    register int result = 0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    result = (a.v.int_val !=
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.int_val !=
		      b.v.cmplx_val.real ||
		      b.v.cmplx_val.imag != 0.0);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    result = (b.v.int_val !=
		      a.v.cmplx_val.real ||
		      a.v.cmplx_val.imag != 0.0);
	    break;
	case CMPLX:
	    result = (a.v.cmplx_val.real !=
		      b.v.cmplx_val.real ||
		      a.v.cmplx_val.imag !=
		      b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(Ginteger(&a, result));
}


void
f_gt(arg)
    union argument *arg;
{
    struct value a, b;
    register int result = 0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    result = (a.v.int_val >
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.int_val >
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    result = (a.v.cmplx_val.real >
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.cmplx_val.real >
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(Ginteger(&a, result));
}


void
f_lt(arg)
    union argument *arg;
{
    struct value a, b;
    register int result = 0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    result = (a.v.int_val <
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.int_val <
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    result = (a.v.cmplx_val.real <
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.cmplx_val.real <
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(Ginteger(&a, result));
}


void
f_ge(arg)
    union argument *arg;
{
    struct value a, b;
    register int result = 0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    result = (a.v.int_val >=
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.int_val >=
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    result = (a.v.cmplx_val.real >=
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.cmplx_val.real >=
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(Ginteger(&a, result));
}


void
f_le(arg)
    union argument *arg;
{
    struct value a, b;
    register int result = 0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    result = (a.v.int_val <=
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.int_val <=
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    result = (a.v.cmplx_val.real <=
		      b.v.int_val);
	    break;
	case CMPLX:
	    result = (a.v.cmplx_val.real <=
		      b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(Ginteger(&a, result));
}


void
f_plus(arg)
    union argument *arg;
{
    struct value a, b, result;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    (void) Ginteger(&result, a.v.int_val +
			    b.v.int_val);
	    break;
	case CMPLX:
	    (void) Gcomplex(&result, a.v.int_val +
			    b.v.cmplx_val.real,
			    b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    (void) Gcomplex(&result, b.v.int_val +
			    a.v.cmplx_val.real,
			    a.v.cmplx_val.imag);
	    break;
	case CMPLX:
	    (void) Gcomplex(&result, a.v.cmplx_val.real +
			    b.v.cmplx_val.real,
			    a.v.cmplx_val.imag +
			    b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(&result);
}


void
f_minus(arg)
    union argument *arg;
{
    struct value a, b, result;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);		/* now do a - b */
    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    (void) Ginteger(&result, a.v.int_val -
			    b.v.int_val);
	    break;
	case CMPLX:
	    (void) Gcomplex(&result, a.v.int_val -
			    b.v.cmplx_val.real,
			    -b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    (void) Gcomplex(&result, a.v.cmplx_val.real -
			    b.v.int_val,
			    a.v.cmplx_val.imag);
	    break;
	case CMPLX:
	    (void) Gcomplex(&result, a.v.cmplx_val.real -
			    b.v.cmplx_val.real,
			    a.v.cmplx_val.imag -
			    b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(&result);
}


void
f_mult(arg)
    union argument *arg;
{
    struct value a, b, result;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);		/* now do a*b */

    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    (void) Ginteger(&result, a.v.int_val *
			    b.v.int_val);
	    break;
	case CMPLX:
	    (void) Gcomplex(&result, a.v.int_val *
			    b.v.cmplx_val.real,
			    a.v.int_val *
			    b.v.cmplx_val.imag);
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    (void) Gcomplex(&result, b.v.int_val *
			    a.v.cmplx_val.real,
			    b.v.int_val *
			    a.v.cmplx_val.imag);
	    break;
	case CMPLX:
	    (void) Gcomplex(&result, a.v.cmplx_val.real *
			    b.v.cmplx_val.real -
			    a.v.cmplx_val.imag *
			    b.v.cmplx_val.imag,
			    a.v.cmplx_val.real *
			    b.v.cmplx_val.imag +
			    a.v.cmplx_val.imag *
			    b.v.cmplx_val.real);
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(&result);
}


void
f_div(arg)
    union argument *arg;
{
    struct value a, b, result;
    register double square;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);		/* now do a/b */

    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    if (b.v.int_val)
		(void) Ginteger(&result, a.v.int_val /
				b.v.int_val);
	    else {
		(void) Ginteger(&result, 0);
		undefined = TRUE;
	    }
	    break;
	case CMPLX:
	    square = b.v.cmplx_val.real *
		b.v.cmplx_val.real +
		b.v.cmplx_val.imag *
		b.v.cmplx_val.imag;
	    if (square)
		(void) Gcomplex(&result, a.v.int_val *
				b.v.cmplx_val.real / square,
				-a.v.int_val *
				b.v.cmplx_val.imag / square);
	    else {
		(void) Gcomplex(&result, 0.0, 0.0);
		undefined = TRUE;
	    }
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    if (b.v.int_val)
		(void) Gcomplex(&result, a.v.cmplx_val.real /
				b.v.int_val,
				a.v.cmplx_val.imag /
				b.v.int_val);
	    else {
		(void) Gcomplex(&result, 0.0, 0.0);
		undefined = TRUE;
	    }
	    break;
	case CMPLX:
	    square = b.v.cmplx_val.real *
		b.v.cmplx_val.real +
		b.v.cmplx_val.imag *
		b.v.cmplx_val.imag;
	    if (square)
		(void) Gcomplex(&result, (a.v.cmplx_val.real *
					  b.v.cmplx_val.real +
					  a.v.cmplx_val.imag *
					  b.v.cmplx_val.imag) / square,
				(a.v.cmplx_val.imag *
				 b.v.cmplx_val.real -
				 a.v.cmplx_val.real *
				 b.v.cmplx_val.imag) /
				square);
	    else {
		(void) Gcomplex(&result, 0.0, 0.0);
		undefined = TRUE;
	    }
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(&result);
}


void
f_mod(arg)
    union argument *arg;
{
    struct value a, b;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);		/* now do a%b */

    if (a.type != INTGR || b.type != INTGR)
	int_error(NO_CARET, "can only mod ints");
    if (b.v.int_val)
	push(Ginteger(&a, a.v.int_val % b.v.int_val));
    else {
	push(Ginteger(&a, 0));
	undefined = TRUE;
    }
}


void
f_power(arg)
    union argument *arg;
{
    struct value a, b, result;
    register int i, t, count;
    register double mag, ang;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&b);
    (void) pop(&a);		/* now find a**b */

    switch (a.type) {
    case INTGR:
	switch (b.type) {
	case INTGR:
	    count = abs(b.v.int_val);
	    t = 1;
	    /* this ought to use bit-masks and squares, etc */
	    for (i = 0; i < count; i++)
		t *= a.v.int_val;
	    if (b.v.int_val >= 0)
		(void) Ginteger(&result, t);
	    else if (t != 0)
		(void) Gcomplex(&result, 1.0 / t, 0.0);
	    else {
		undefined = TRUE;
		(void) Gcomplex(&result, 0.0, 0.0);
	    }
	    break;
	case CMPLX:
	    if (a.v.int_val == 0) {
		if (b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
		    undefined = TRUE;
		}
		/* return 1.0 for 0**0 */
		Gcomplex(&result, b.v.cmplx_val.real == 0 ? 1.0 : 0.0, 0.0);
	    } else {
		mag =
		    pow(magnitude(&a), fabs(b.v.cmplx_val.real));
		if (b.v.cmplx_val.real < 0.0) {
		    if (mag != 0.0)
			mag = 1.0 / mag;
		    else
			undefined = TRUE;
		}
		mag *= gp_exp(-b.v.cmplx_val.imag * angle(&a));
		ang = b.v.cmplx_val.real * angle(&a) +
		    b.v.cmplx_val.imag * log(magnitude(&a));
		(void) Gcomplex(&result, mag * cos(ang),
				mag * sin(ang));
	    }
	    break;
	    BAD_DEFAULT
	}
	break;
    case CMPLX:
	switch (b.type) {
	case INTGR:
	    if (a.v.cmplx_val.imag == 0.0) {
		mag = pow(a.v.cmplx_val.real, (double) abs(b.v.int_val));
		if (b.v.int_val < 0) {
		    if (mag != 0.0)
			mag = 1.0 / mag;
		    else
			undefined = TRUE;
		}
		(void) Gcomplex(&result, mag, 0.0);
	    } else {
		/* not so good, but...! */
		mag = pow(magnitude(&a), (double) abs(b.v.int_val));
		if (b.v.int_val < 0) {
		    if (mag != 0.0)
			mag = 1.0 / mag;
		    else
			undefined = TRUE;
		}
		ang = angle(&a) * b.v.int_val;
		(void) Gcomplex(&result, mag * cos(ang),
				mag * sin(ang));
	    }
	    break;
	case CMPLX:
	    if (a.v.cmplx_val.real == 0 && a.v.cmplx_val.imag == 0) {
		if (b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
		    undefined = TRUE;
		}
		/* return 1.0 for 0**0 */
		Gcomplex(&result, b.v.cmplx_val.real == 0 ? 1.0 : 0.0, 0.0);
	    } else {
		mag = pow(magnitude(&a), fabs(b.v.cmplx_val.real));
		if (b.v.cmplx_val.real < 0.0) {
		    if (mag != 0.0)
			mag = 1.0 / mag;
		    else
			undefined = TRUE;
		}
		mag *= gp_exp(-b.v.cmplx_val.imag * angle(&a));
		ang = b.v.cmplx_val.real * angle(&a) +
		    b.v.cmplx_val.imag * log(magnitude(&a));
		(void) Gcomplex(&result, mag * cos(ang),
				mag * sin(ang));
	    }
	    break;
	    BAD_DEFAULT
	}
	break;
	BAD_DEFAULT
    }
    push(&result);
}


void
f_factorial(arg)
    union argument *arg;
{
    struct value a;
    register int i;
    register double val = 0.0;

    (void) arg;			/* avoid -Wunused warning */
    (void) pop(&a);		/* find a! (factorial) */

    switch (a.type) {
    case INTGR:
	val = 1.0;
	for (i = a.v.int_val; i > 1; i--)	/*fpe's should catch overflows */
	    val *= i;
	break;
    default:
	int_error(NO_CARET, "factorial (!) argument must be an integer");
	return;			/* avoid gcc -Wall warning about val */
    }

    push(Gcomplex(&a, val, 0.0));

}


