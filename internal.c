#ifndef lint
static char *RCSid = "$Id: internal.c%v 3.50.1.8 1993/07/27 05:37:15 woo Exp $";
#endif


/* GNUPLOT - internal.c */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 */

#include <math.h>
#include <stdio.h>
#include "plot.h"

TBOOLEAN undefined;

#ifndef AMIGA_SC_6_1
char *strcpy();
#endif /* !AMIGA_SC_6_1 */

struct value *pop(), *Gcomplex(), *Ginteger();
double magnitude(), angle(), real();

struct value stack[STACK_DEPTH];

int s_p = -1;   /* stack pointer */


/*
 * System V and MSC 4.0 call this when they wants to print an error message.
 * Don't!
 */
#ifndef _CRAY
#if defined(MSDOS) || defined(DOS386)
#ifdef __TURBOC__
int matherr()	/* Turbo C */
#else
int matherr(x)	/* MSC 5.1 */
struct exception *x;
#endif /* TURBOC */
#else /* not MSDOS */
#ifdef apollo
int matherr(struct exception *x)	/* apollo */
#else /* apollo */
#if defined(AMIGA_SC_6_1)||defined(ATARI)&&defined(__GNUC__)||defined(__hpux__)||defined(PLOSS) ||defined(SOLARIS)
int matherr(x)
struct exception *x;
#else    /* Most everyone else (not apollo). */
int matherr()
#endif /* AMIGA_SC_6_1 || GCC_ST */
#endif /* apollo */
#endif /* MSDOS */
{
	return (undefined = TRUE);		/* don't print error message */
}
#endif /* not _CRAY */


reset_stack()
{
	s_p = -1;
}


check_stack()	/* make sure stack's empty */
{
	if (s_p != -1)
		fprintf(stderr,"\nwarning:  internal error--stack not empty!\n");
}


struct value *pop(x)
struct value *x;
{
	if (s_p  < 0 )
		int_error("stack underflow",NO_CARET);
	*x = stack[s_p--];
	return(x);
}


push(x)
struct value *x;
{
	if (s_p == STACK_DEPTH - 1)
		int_error("stack overflow",NO_CARET);
	stack[++s_p] = *x;
}


#define ERR_VAR "undefined variable: "

f_push(x)
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


f_pushc(x)
union argument *x;
{
	push(&(x->v_arg));
}


f_pushd1(x)
union argument *x;
{
	push(&(x->udf_arg->dummy_values[0]));
}


f_pushd2(x)
union argument *x;
{
	push(&(x->udf_arg->dummy_values[1]));
}


f_pushd(x)
union argument *x;
{
struct value param;
	(void) pop(&param);
	push(&(x->udf_arg->dummy_values[param.v.int_val]));
}


#define ERR_FUN "undefined function: "

f_call(x)  /* execute a udf */
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


f_calln(x)  /* execute a udf of n variables */
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


static int_check(v)
struct value *v;
{
	if (v->type != INTGR)
		int_error("non-integer passed to boolean operator",NO_CARET);
}


f_lnot()
{
struct value a;
	int_check(pop(&a));
	push(Ginteger(&a,!a.v.int_val) );
}


f_bnot()
{
struct value a;
	int_check(pop(&a));
	push( Ginteger(&a,~a.v.int_val) );
}


f_bool()
{			/* converts top-of-stack to boolean */
	int_check(&top_of_stack);
	top_of_stack.v.int_val = !!top_of_stack.v.int_val;
}


f_lor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val || b.v.int_val) );
}

f_land()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val && b.v.int_val) );
}


f_bor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val | b.v.int_val) );
}


f_xor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val ^ b.v.int_val) );
}


f_band()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( Ginteger(&a,a.v.int_val & b.v.int_val) );
}


f_uminus()
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
	}
	push(&a);
}


f_eq() /* note: floating point equality is rare because of roundoff error! */
{
struct value a, b;
	register int result;
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
			}
	}
	push(Ginteger(&a,result));
}


f_ne()
{
struct value a, b;
	register int result;
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
			}
	}
	push(Ginteger(&a,result));
}


f_gt()
{
struct value a, b;
	register int result;
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
			}
	}
	push(Ginteger(&a,result));
}


f_lt()
{
struct value a, b;
	register int result;
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
			}
	}
	push(Ginteger(&a,result));
}


f_ge()
{
struct value a, b;
	register int result;
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
			}
	}
	push(Ginteger(&a,result));
}


f_le()
{
struct value a, b;
	register int result;
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
			}
	}
	push(Ginteger(&a,result));
}


f_plus()
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
			}
	}
	push(&result);
}


f_minus()
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
			}
	}
	push(&result);
}


f_mult()
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
			}
	}
	push(&result);
}


f_div()
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
			}
	}
	push(&result);
}


f_mod()
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


f_power()
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
					mag =
					  pow(magnitude(&a),fabs(b.v.cmplx_val.real));
					if (b.v.cmplx_val.real < 0.0)
					  if (mag != 0.0)
					    mag = 1.0/mag;
					  else 
					    undefined = TRUE;
					mag *= exp(-b.v.cmplx_val.imag*angle(&a));
					ang = b.v.cmplx_val.real*angle(&a) +
					      b.v.cmplx_val.imag*log(magnitude(&a));
					(void) Gcomplex(&result,mag*cos(ang),
						mag*sin(ang));
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
					mag = pow(magnitude(&a),fabs(b.v.cmplx_val.real));
					if (b.v.cmplx_val.real < 0.0)
					  if (mag != 0.0)
					    mag = 1.0/mag;
					  else 
					    undefined = TRUE;
					mag *= exp(-b.v.cmplx_val.imag*angle(&a));
					ang = b.v.cmplx_val.real*angle(&a) +
					      b.v.cmplx_val.imag*log(magnitude(&a));
					(void) Gcomplex(&result,mag*cos(ang),
						mag*sin(ang));
			}
	}
	push(&result);
}


f_factorial()
{
struct value a;
register int i;
register double val;

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
