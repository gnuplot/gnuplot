/*
 *
 *    G N U P L O T  --  internal.c
 *
 *  Copyright (C) 1986 Colin Kelley, Thomas Williams
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#include <math.h>
#include <stdio.h>
#include "plot.h"

extern BOOLEAN undefined;
extern struct vt_entry vt[MAX_VALUES];
extern struct udft_entry udft[MAX_UDFS];

char *strcpy();

struct value *pop(), *complex(), *integer();
double magnitude(), angle(), real();

struct value stack[STACK_DEPTH];

int s_p = -1;   /* stack pointer */

#ifndef MSDOS /* suggested by "J.D. McDonald " <mcdonald@uxe.cso.uiuc.edu> */
/*
 * System V and MSC 4.0 call this when they wants to print an error message.
 * Don't!
 */
matherr()
{
	return (undefined = TRUE);		/* don't print error message */
}
#endif MSDOS

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

#define ERR_VAR "undefined variable: "

f_push(x)
struct value *x;		/* contains index of value to push; must be integer! */
{
static char err_str[sizeof(ERR_VAR) + MAX_ID_LEN] = ERR_VAR;
register int index;

	if (x->type != INT)
		int_error("internal error--non-int passed to f_push!",NO_CARET);
	index = x->v.int_val;

	if (vt[index].vt_undef) {	 /* undefined */
		(void) strcpy(&err_str[sizeof(ERR_VAR) - 1], vt[index].vt_name);
		int_error(err_str,NO_CARET);
	}
	push(&vt[index].vt_value);
}


f_pushc(x)
struct value *x;
{
	if (s_p == STACK_DEPTH - 1)
		int_error("stack overflow",NO_CARET);
	stack[++s_p] = *x;
}


f_pushd(x)
struct value *x;
{
	f_pushc(&udft[x->v.int_val].dummy_value);
}


#define ERR_FUN "undefined function: "

f_call(f_index)  /* execute a udf */
struct value *f_index;
{
static char err_str[sizeof(ERR_FUN) + MAX_ID_LEN] = ERR_FUN;

	if (udft[f_index->v.int_val].at.count == 0) { /* undefined */
		(void) strcpy(&err_str[sizeof(ERR_FUN) - 1],
				udft[f_index->v.int_val].udft_name);
		int_error(err_str,NO_CARET);
	}
	(void) pop(&udft[f_index->v.int_val].dummy_value);

	execute_at(&udft[f_index->v.int_val].at);
}


static int_check(v)
struct value *v;
{
	if (v->type != INT)
		int_error("non-integer passed to boolean operator",NO_CARET);
}


f_terniary()		/* code for (a) ? b : c */
{
struct value a, b, c;
	(void) pop(&c);	(void) pop(&b);	int_check(pop(&a));
	push((a.v.int_val) ? &b : &c);
			/* I just had to use ? : here! */
}


f_lnot()
{
struct value a;
	int_check(pop(&a));
	push(integer(&a,!a.v.int_val) );
}


f_bnot()
{
struct value a;
	int_check(pop(&a));
	push( integer(&a,~a.v.int_val) );
}


f_lor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( integer(&a,a.v.int_val || b.v.int_val) );
}


f_land()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( integer(&a,a.v.int_val && b.v.int_val) );
}


f_bor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( integer(&a,a.v.int_val | b.v.int_val) );
}


f_xor()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( integer(&a,a.v.int_val ^ b.v.int_val) );
}


f_band()
{
struct value a,b;
	int_check(pop(&b));
	int_check(pop(&a));
	push( integer(&a,a.v.int_val & b.v.int_val) );
}


f_uminus()
{
struct value a;
	(void) pop(&a);
	switch(a.type) {
		case INT:
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
		case INT:
			switch (b.type) {
				case INT:
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
				case INT:
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
	push(integer(&a,result));
}


f_ne()
{
struct value a, b;
	register int result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INT:
			switch (b.type) {
				case INT:
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
				case INT:
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
	push(integer(&a,result));
}


f_gt()
{
struct value a, b;
	register int result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INT:
			switch (b.type) {
				case INT:
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
				case INT:
					result = (a.v.cmplx_val.real >
						b.v.int_val);
					break;
				case CMPLX:
					result = (a.v.cmplx_val.real >
						b.v.cmplx_val.real);
			}
	}
	push(integer(&a,result));
}


f_lt()
{
struct value a, b;
	register int result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INT:
			switch (b.type) {
				case INT:
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
				case INT:
					result = (a.v.cmplx_val.real <
						b.v.int_val);
					break;
				case CMPLX:
					result = (a.v.cmplx_val.real <
						b.v.cmplx_val.real);
			}
	}
	push(integer(&a,result));
}


f_ge()
{
struct value a, b;
	register int result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INT:
			switch (b.type) {
				case INT:
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
				case INT:
					result = (a.v.cmplx_val.real >=
						b.v.int_val);
					break;
				case CMPLX:
					result = (a.v.cmplx_val.real >=
						b.v.cmplx_val.real);
			}
	}
	push(integer(&a,result));
}


f_le()
{
struct value a, b;
	register int result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INT:
			switch (b.type) {
				case INT:
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
				case INT:
					result = (a.v.cmplx_val.real <=
						b.v.int_val);
					break;
				case CMPLX:
					result = (a.v.cmplx_val.real <=
						b.v.cmplx_val.real);
			}
	}
	push(integer(&a,result));
}


f_plus()
{
struct value a, b, result;
	(void) pop(&b);
	(void) pop(&a);
	switch(a.type) {
		case INT:
			switch (b.type) {
				case INT:
					(void) integer(&result,a.v.int_val +
						b.v.int_val);
					break;
				case CMPLX:
					(void) complex(&result,a.v.int_val +
						b.v.cmplx_val.real,
					   b.v.cmplx_val.imag);
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INT:
					(void) complex(&result,b.v.int_val +
						a.v.cmplx_val.real,
					   a.v.cmplx_val.imag);
					break;
				case CMPLX:
					(void) complex(&result,a.v.cmplx_val.real+
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
		case INT:
			switch (b.type) {
				case INT:
					(void) integer(&result,a.v.int_val -
						b.v.int_val);
					break;
				case CMPLX:
					(void) complex(&result,a.v.int_val -
						b.v.cmplx_val.real,
					   -b.v.cmplx_val.imag);
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INT:
					(void) complex(&result,a.v.cmplx_val.real -
						b.v.int_val,
					    a.v.cmplx_val.imag);
					break;
				case CMPLX:
					(void) complex(&result,a.v.cmplx_val.real-
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
		case INT:
			switch (b.type) {
				case INT:
					(void) integer(&result,a.v.int_val *
						b.v.int_val);
					break;
				case CMPLX:
					(void) complex(&result,a.v.int_val *
						b.v.cmplx_val.real,
						a.v.int_val *
						b.v.cmplx_val.imag);
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INT:
					(void) complex(&result,b.v.int_val *
						a.v.cmplx_val.real,
						b.v.int_val *
						a.v.cmplx_val.imag);
					break;
				case CMPLX:
					(void) complex(&result,a.v.cmplx_val.real*
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
		case INT:
			switch (b.type) {
				case INT:
					if (b.v.int_val)
					  (void) integer(&result,a.v.int_val /
						b.v.int_val);
					else {
					  (void) integer(&result,0);
					  undefined = TRUE;
					}
					break;
				case CMPLX:
					square = b.v.cmplx_val.real*
						b.v.cmplx_val.real +
						b.v.cmplx_val.imag*
						b.v.cmplx_val.imag;
					if (square)
						(void) complex(&result,a.v.int_val*
						b.v.cmplx_val.real/square,
						-a.v.int_val*
						b.v.cmplx_val.imag/square);
					else {
						(void) complex(&result,0.0,0.0);
						undefined = TRUE;
					}
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INT:
					if (b.v.int_val)
					  
					  (void) complex(&result,a.v.cmplx_val.real/
						b.v.int_val,
						a.v.cmplx_val.imag/
						b.v.int_val);
					else {
						(void) complex(&result,0.0,0.0);
						undefined = TRUE;
					}
					break;
				case CMPLX:
					square = b.v.cmplx_val.real*
						b.v.cmplx_val.real +
						b.v.cmplx_val.imag*
						b.v.cmplx_val.imag;
					if (square)
					(void) complex(&result,(a.v.cmplx_val.real*
						b.v.cmplx_val.real+
						a.v.cmplx_val.imag*
						b.v.cmplx_val.imag)/square,
						(a.v.cmplx_val.imag*
						b.v.cmplx_val.real-
						a.v.cmplx_val.real*
						b.v.cmplx_val.imag)/
							square);
					else {
						(void) complex(&result,0.0,0.0);
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

	if (a.type != INT || b.type != INT)
		int_error("can only mod ints",NO_CARET);
	if (b.v.int_val)
		push(integer(&a,a.v.int_val % b.v.int_val));
	else {
		push(integer(&a,0));
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
		case INT:
			switch (b.type) {
				case INT:
					count = abs(b.v.int_val);
					t = 1;
					for(i=0; i < count; i++)
						t *= a.v.int_val;
					if (b.v.int_val >= 0)
						(void) integer(&result,t);
					else
						(void) complex(&result,1.0/t,0.0);
					break;
				case CMPLX:
					mag =
					  pow(magnitude(&a),fabs(b.v.cmplx_val.real));
					if (b.v.cmplx_val.real < 0.0)
						mag = 1.0/mag;
					ang = angle(&a)*b.v.cmplx_val.real+
					  b.v.cmplx_val.imag;
					(void) complex(&result,mag*cos(ang),
						mag*sin(ang));
			}
			break;
		case CMPLX:
			switch (b.type) {
				case INT:
					/* not so good, but...! */
					mag =
					  pow(magnitude(&a),(double)abs(b.v.int_val));
					if (b.v.int_val < 0)
						mag = 1.0/mag;
					ang = angle(&a)*b.v.int_val;
					(void) complex(&result,mag*cos(ang),
						mag*sin(ang));
					break;
				case CMPLX:
					mag =
					  pow(magnitude(&a),fabs(b.v.cmplx_val.real));
					if (b.v.cmplx_val.real < 0.0)
					  mag = 1.0/mag;
					ang = angle(&a)*b.v.cmplx_val.real+
					  b.v.cmplx_val.imag;
					(void) complex(&result,mag*cos(ang),
						mag*sin(ang));
			}
	}
	push(&result);
}
