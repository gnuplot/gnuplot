#ifndef lint
static char *RCSid = "$Id: internal.c,v 1.22 1998/03/22 22:31:50 drd Exp $";
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


#include "plot.h"
#include "fnproto.h"

/* some machines have trouble with exp(-x) for large x
 * if MINEXP is defined at compile time, use gp_exp(x) instead,
 * which returns 0 for exp(x) with x < MINEXP
 * exp(x) will already have been defined as gp_exp(x) in plot.h
 */
 
#ifdef MINEXP
double gp_exp(x)
double x;
{
	return (x < (MINEXP) ) ? 0.0 : exp(x);
}
#endif

TBOOLEAN undefined;

static void int_check __PROTO((struct value *v));

struct value stack[STACK_DEPTH];

int s_p = -1;   /* stack pointer */


/*
 * System V and MSC 4.0 call this when they wants to print an error message.
 * Don't!
 */
#ifndef _CRAY
# ifdef AMIGA_SC_6_1
#  define matherr __matherr
#  define exception __exception
# endif /* AMIGA_SC_6_1 */
# if defined(__BORLANDC__) && __BORLANDC__ >= 0x450
#  define matherr _matherr
# endif /* __BORLANDC__ >= 0x450 */
# if (defined(MSDOS) || defined(DOS386)) && defined(__TURBOC__) || defined(VMS)
int matherr ()
#else
int matherr(x)
struct exception *x;
# endif /* (MSDOS || DOS386) && __TURBOC__ */
{
	return (undefined = TRUE);		/* don't print error message */
}
#endif /* not _CRAY */


void reset_stack()
{
	s_p = -1;
}


void check_stack()	/* make sure stack's empty */
{
	if (s_p != -1)
		fprintf(stderr,"\nwarning:  internal error--stack not empty!\n");
}

#define BAD_DEFAULT default: int_error("interal error : type neither INT or CMPLX", NO_CARET); return;

struct value *pop(x)
struct value *x;
{
	if (s_p  < 0 )
		int_error("stack underflow",NO_CARET);
	*x = stack[s_p--];
	return(x);
}


void push(x)
struct value *x;
{
	if (s_p == STACK_DEPTH - 1)
		int_error("stack overflow",NO_CARET);
	stack[++s_p] = *x;
}


#define ERR_VAR "undefined variable: "

void f_push(x)
union argument *x;		/* contains pointer to value to push; */
{
static char err_str[sizeof(ERR_VAR) + MAX_ID_LEN] = ERR_VAR;
struct udvt_entry *udv;

	udv = x->udv_arg;
	if (udv->udv_undef) {	 /* undefined */
		(void) strcpy(&err_str[sizeof(ERR_VAR) - 1], udv->udv_name);
		int_error(err_str,NO_CARET);
	}
	push(&(udv->udv_value));
}


void f_pushc(x)
union argument *x;
{
	push(&(x->v_arg));
}


void f_pushd1(x)
union argument *x;
{
	push(&(x->udf_arg->dummy_values[0]));
}


void f_pushd2(x)
union argument *x;
{
	push(&(x->udf_arg->dummy_values[1]));
}


void f_pushd(x)
union argument *x;
{
struct value param;
	(void) pop(&param);
	push(&(x->udf_arg->dummy_values[param.v.int_val]));
}


#define ERR_FUN "undefined function: "

void f_call(x)  /* execute a udf */
union argument *x;
{
static char err_str[sizeof(ERR_FUN) + MAX_ID_LEN] = ERR_FUN;
register struct udft_entry *udf;
struct value save_dummy;

	udf = x->udf_arg;
	if (!udf->at) { /* undefined */
		(void) strcpy(&err_str[sizeof(ERR_FUN) - 1],
				udf->udf_name);
		int_error(err_str,NO_CARET);
	}
	save_dummy = udf->dummy_values[0];
	(void) pop(&(udf->dummy_values[0]));

	execute_at(udf->at);
	udf->dummy_values[0] = save_dummy;
}


void f_calln(x)  /* execute a udf of n variables */
union argument *x;
{
static char err_str[sizeof(ERR_FUN) + MAX_ID_LEN] = ERR_FUN;
register struct udft_entry *udf;
struct value save_dummy[MAX_NUM_VAR];

	int i;
	int num_pop;
	struct value num_params;

	udf = x->udf_arg;
	if (!udf->at) { /* undefined */
		(void) strcpy(&err_str[sizeof(ERR_FUN) - 1],
				udf->udf_name);
		int_error(err_str,NO_CARET);
	}
	for(i=0; i<MAX_NUM_VAR; i++) 
		save_dummy[i] = udf->dummy_values[i];

	/* if there are more parameters than the function is expecting */
	/* simply ignore the excess */
	(void) pop(&num_params);

	if(num_params.v.int_val > MAX_NUM_VAR) {
		/* pop the dummies that there is no room for */
		num_pop = num_params.v.int_val - MAX_NUM_VAR;
		for(i=0; i< num_pop; i++)
			(void) pop(&(udf->dummy_values[i]));

		num_pop = MAX_NUM_VAR;
	} else {
		num_pop = num_params.v.int_val;
	}

	/* pop parameters we can use */
	for(i=num_pop-1; i>=0; i--)
		(void) pop(&(udf->dummy_values[i]));

	execute_at(udf->at);
	for(i=0; i<MAX_NUM_VAR; i++) 
		udf->dummy_values[i] = save_dummy[i];
}


static void int_check(v)
struct value *v;
{
	if (v->type != INTGR)
		int_error("non-integer passed to boolean operator",NO_CARET);
}


void f_lnot()
{
struct value a;
	int_check(pop(&a));
	push(Ginteger(&a,!a.v.int_val) );
}


void f_bnot()
{
struct value a;
	int_check(pop(&a));
	push( Ginteger(&a,~a.v.int_val) );
}


void f_bool()
{			/* converts top-of-stack to boolean */
	int_check(&top_of_stack);
	top_of_stack.v.int_val = !!top_of_stack.v.int_val;
}


void f_lor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val || b.v.int_val) );
}

void f_land()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val && b.v.int_val) );
}


void f_bor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val | b.v.int_val) );
}


void f_xor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val ^ b.v.int_val) );
}


void f_band()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val & b.v.int_val) );
}


void f_uminus()
{
struct value a;
	(void) pop(&a);
	switch(a.type) {
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


void f_eq() /* note: floating point equality is rare because of roundoff error! */
{
struct value a, b;
	register int result=0;
/* HBB: Here and at several other places in this module, I introduced */
/* initial values, to shut up 'gcc -W' */
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
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
					result = (a.v.cmplx_val.real==
						b.v.cmplx_val.real &&
						a.v.cmplx_val.imag==
						b.v.cmplx_val.imag);
					break;
				BAD_DEFAULT
			}
			break;
		BAD_DEFAULT
	}
	push(Ginteger(&a,result));
}


void f_ne()
{
struct value a, b;
	register int result=0;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
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
	push(Ginteger(&a,result));
}


void f_gt()
{
struct value a, b;
	register int result=0;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
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
	push(Ginteger(&a,result));
}


void f_lt()
{
struct value a, b;
	register int result=0;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
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
	push(Ginteger(&a,result));
}


void f_ge()
{
struct value a, b;
	register int result=0;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
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
	push(Ginteger(&a,result));
}


void f_le()
{
struct value a, b;
	register int result=0;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
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
	push(Ginteger(&a,result));
}


void f_plus()
{
struct value a, b, result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INTGR:
			switch (b.type) {
				case INTGR:
					(void) Ginteger(&result,a.v.int_val +
						b.v.int_val);
					break;
				case CMPLX:
					(void) Gcomplex(&result,a.v.int_val +
						b.v.cmplx_val.real,
					   b.v.cmplx_val.imag);
					break;
				BAD_DEFAULT
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INTGR:
					(void) Gcomplex(&result,b.v.int_val +
						a.v.cmplx_val.real,
					   a.v.cmplx_val.imag);
					break;
				case CMPLX:
					(void) Gcomplex(&result,a.v.cmplx_val.real+
						b.v.cmplx_val.real,
						a.v.cmplx_val.imag+
						b.v.cmplx_val.imag);
					break;
				BAD_DEFAULT
			}
			break;
		BAD_DEFAULT
	}
	push(&result);
}


void f_minus()
{
struct value a, b, result;
	(void) pop(&b);
	(void) pop(&a);		/* now do a - b */
	switch(a.type) {
		case INTGR:
			switch (b.type) {
				case INTGR:
					(void) Ginteger(&result,a.v.int_val -
						b.v.int_val);
					break;
				case CMPLX:
					(void) Gcomplex(&result,a.v.int_val -
						b.v.cmplx_val.real,
					   -b.v.cmplx_val.imag);
					break;
				BAD_DEFAULT
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INTGR:
					(void) Gcomplex(&result,a.v.cmplx_val.real -
						b.v.int_val,
					    a.v.cmplx_val.imag);
					break;
				case CMPLX:
					(void) Gcomplex(&result,a.v.cmplx_val.real-
						b.v.cmplx_val.real,
						a.v.cmplx_val.imag-
						b.v.cmplx_val.imag);
					break;
				BAD_DEFAULT
			}
			break;
		BAD_DEFAULT
	}
	push(&result);
}


void f_mult()
{
struct value a, b, result;
	(void) pop(&b);
	(void) pop(&a);	/* now do a*b */

	switch(a.type) {
		case INTGR:
			switch (b.type) {
				case INTGR:
					(void) Ginteger(&result,a.v.int_val *
						b.v.int_val);
					break;
				case CMPLX:
					(void) Gcomplex(&result,a.v.int_val *
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
					(void) Gcomplex(&result,b.v.int_val *
						a.v.cmplx_val.real,
						b.v.int_val *
						a.v.cmplx_val.imag);
					break;
				case CMPLX:
					(void) Gcomplex(&result,a.v.cmplx_val.real*
						b.v.cmplx_val.real-
						a.v.cmplx_val.imag*
						b.v.cmplx_val.imag,
						a.v.cmplx_val.real*
						b.v.cmplx_val.imag+
						a.v.cmplx_val.imag*
						b.v.cmplx_val.real);
					break;
				BAD_DEFAULT
			}
			break;
		BAD_DEFAULT
	}
	push(&result);
}


void f_div()
{
struct value a, b, result;
register double square;
	(void) pop(&b);
	(void) pop(&a);	/* now do a/b */

	switch(a.type) {
		case INTGR:
			switch (b.type) {
				case INTGR:
					if (b.v.int_val)
					  (void) Ginteger(&result,a.v.int_val /
						b.v.int_val);
					else {
					  (void) Ginteger(&result,0);
					  undefined = TRUE;
					}
					break;
				case CMPLX:
					square = b.v.cmplx_val.real*
						b.v.cmplx_val.real +
						b.v.cmplx_val.imag*
						b.v.cmplx_val.imag;
					if (square)
						(void) Gcomplex(&result,a.v.int_val*
						b.v.cmplx_val.real/square,
						-a.v.int_val*
						b.v.cmplx_val.imag/square);
					else {
						(void) Gcomplex(&result,0.0,0.0);
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
					  
					  (void) Gcomplex(&result,a.v.cmplx_val.real/
						b.v.int_val,
						a.v.cmplx_val.imag/
						b.v.int_val);
					else {
						(void) Gcomplex(&result,0.0,0.0);
						undefined = TRUE;
					}
					break;
				case CMPLX:
					square = b.v.cmplx_val.real*
						b.v.cmplx_val.real +
						b.v.cmplx_val.imag*
						b.v.cmplx_val.imag;
					if (square)
					(void) Gcomplex(&result,(a.v.cmplx_val.real*
						b.v.cmplx_val.real+
						a.v.cmplx_val.imag*
						b.v.cmplx_val.imag)/square,
						(a.v.cmplx_val.imag*
						b.v.cmplx_val.real-
						a.v.cmplx_val.real*
						b.v.cmplx_val.imag)/
							square);
					else {
						(void) Gcomplex(&result,0.0,0.0);
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


void f_mod()
{
struct value a, b;
	(void) pop(&b);
	(void) pop(&a);	/* now do a%b */

	if (a.type != INTGR || b.type != INTGR)
		int_error("can only mod ints",NO_CARET);
	if (b.v.int_val)
		push(Ginteger(&a,a.v.int_val % b.v.int_val));
	else {
		push(Ginteger(&a,0));
		undefined = TRUE;
	}
}


void f_power()
{
struct value a, b, result;
register int i, t, count;
register double mag, ang;
	(void) pop(&b);
	(void) pop(&a);	/* now find a**b */

	switch(a.type) {
		case INTGR:
			switch (b.type) {
				case INTGR:
					count = abs(b.v.int_val);
					t = 1;
					/* this ought to use bit-masks and squares, etc */
					for(i = 0; i < count; i++)
						t *= a.v.int_val;
					if (b.v.int_val >= 0)
						(void) Ginteger(&result,t);
					else
					  if (t != 0)
					    (void) Gcomplex(&result,1.0/t,0.0);
					  else {
						 undefined = TRUE;
						 (void) Gcomplex(&result, 0.0, 0.0);
					  }
					break;
				case CMPLX:
					if (a.v.int_val==0) {
					  if(b.v.cmplx_val.imag!=0 || b.v.cmplx_val.real<0) {
					    undefined = TRUE;
					  }
					  /* return 1.0 for 0**0 */
					  Gcomplex(&result, b.v.cmplx_val.real==0 ? 1.0 : 0.0, 0.0);
					} else {
					  mag =
					    pow(magnitude(&a),fabs(b.v.cmplx_val.real));
					  if (b.v.cmplx_val.real < 0.0)
					    if (mag != 0.0)
					      mag = 1.0/mag;
					   else 
					      undefined = TRUE;
					  mag *= gp_exp(-b.v.cmplx_val.imag*angle(&a));
					  ang = b.v.cmplx_val.real*angle(&a) +
					        b.v.cmplx_val.imag*log(magnitude(&a));
					  (void) Gcomplex(&result,mag*cos(ang),
						  mag*sin(ang));
					}
					break;
				BAD_DEFAULT
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INTGR:
					if (a.v.cmplx_val.imag == 0.0) {
						mag = pow(a.v.cmplx_val.real,(double)abs(b.v.int_val));
						if (b.v.int_val < 0)
						  if (mag != 0.0)
						    mag = 1.0/mag;
						  else 
						    undefined = TRUE;
						(void) Gcomplex(&result,mag,0.0);
					}
					else {
						/* not so good, but...! */
						mag = pow(magnitude(&a),(double)abs(b.v.int_val));
						if (b.v.int_val < 0)
						  if (mag != 0.0)
						    mag = 1.0/mag;
						  else 
						    undefined = TRUE;
						ang = angle(&a)*b.v.int_val;
						(void) Gcomplex(&result,mag*cos(ang),
							mag*sin(ang));
					}
					break;
				case CMPLX:
					if (a.v.cmplx_val.real==0 && a.v.cmplx_val.imag==0) {
					  if(b.v.cmplx_val.imag!=0 || b.v.cmplx_val.real<0) {
					    undefined = TRUE;
					  }
					  /* return 1.0 for 0**0 */
					  Gcomplex(&result, b.v.cmplx_val.real==0 ? 1.0 : 0.0, 0.0);
					} else {
					  mag = pow(magnitude(&a),fabs(b.v.cmplx_val.real));
					  if (b.v.cmplx_val.real < 0.0)
					    if (mag != 0.0)
					      mag = 1.0/mag;
					    else 
					      undefined = TRUE;
					  mag *= gp_exp(-b.v.cmplx_val.imag*angle(&a));
					  ang = b.v.cmplx_val.real*angle(&a) +
					        b.v.cmplx_val.imag*log(magnitude(&a));
					  (void) Gcomplex(&result,mag*cos(ang),
						  mag*sin(ang));
					}
					break;
				BAD_DEFAULT
			}
			break;
		BAD_DEFAULT
	}
	push(&result);
}


void f_factorial()
{
struct value a;
register int i;
register double val=0.0;

	(void) pop(&a);	/* find a! (factorial) */

	switch (a.type) {
		case INTGR:
			val = 1.0;
			for (i = a.v.int_val; i > 1; i--)  /*fpe's should catch overflows*/
				val *= i;
			break;
		default:
			int_error("factorial (!) argument must be an integer",
			NO_CARET);
			return; /* avoid gcc -Wall warning about val */
		}

	push(Gcomplex(&a,val,0.0));
			
}


int
f_jump(x)
union argument *x;
{
	return(x->j_arg);
}


int
f_jumpz(x)
union argument *x;
{
struct value a;
	int_check(&top_of_stack);
	if (top_of_stack.v.int_val) {	/* non-zero */
		(void) pop(&a);
		return 1;				/* no jump */
	}
	else
		return(x->j_arg);		/* leave the argument on TOS */
}


int
f_jumpnz(x)
union argument *x;
{
struct value a;
	int_check(&top_of_stack);
	if (top_of_stack.v.int_val)	/* non-zero */
		return(x->j_arg);		/* leave the argument on TOS */
	else {
		(void) pop(&a);
		return 1;				/* no jump */
	}
}


int
f_jtern(x)
union argument *x;
{
struct value a;

	int_check(pop(&a));
	if (a.v.int_val)
		return(1);				/* no jump; fall through to TRUE code */
	else
		return(x->j_arg);		/* go jump to FALSE code */
}
