/*
 *
 *    G N U P L O T  --  standard.c
 *
 *  Copyright (C) 1986, 1987  Thomas Williams, Colin Kelley
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

#ifdef vms
#include <errno.h>
#else
extern int errno;
#endif /* vms */


extern struct value stack[STACK_DEPTH];
extern int s_p;

struct value *pop(), *complex(), *integer();

double magnitude(), angle(), real(), imag();


f_real()
{
struct value a;
	push( complex(&a,real(pop(&a)), 0.0) );
}

f_imag()
{
struct value a;
	push( complex(&a,imag(pop(&a)), 0.0) );
}

f_arg()
{
struct value a;
	push( complex(&a,angle(pop(&a)), 0.0) );
}

f_conjg()
{
struct value a;
	(void) pop(&a);
	push( complex(&a,real(&a),-imag(&a) ));
}

f_sin()
{
struct value a;
	(void) pop(&a);
	push( complex(&a,sin(real(&a))*cosh(imag(&a)), cos(real(&a))*sinh(imag(&a))) );
}

f_cos()
{
struct value a;
	(void) pop(&a);
	push( complex(&a,cos(real(&a))*cosh(imag(&a)), -sin(real(&a))*sinh(imag(&a))));
}

f_tan()
{
struct value a;
register double den;
	(void) pop(&a);
	if (imag(&a) == 0.0)
		push( complex(&a,tan(real(&a)),0.0) );
	else {
		den = cos(2*real(&a))+cosh(2*imag(&a));
		if (den == 0.0) {
			undefined = TRUE;
			push( &a );
		}
		else
			push( complex(&a,sin(2*real(&a))/den, sinh(2*imag(&a))/den) );
	}
}

f_asin()
{
struct value a;
register double alpha, beta, x, y;
	(void) pop(&a);
	x = real(&a); y = imag(&a);
	if (y == 0.0) {
		if (fabs(x) > 1.0) {
			undefined = TRUE;
			push(complex(&a,0.0, 0.0));
		} else
			push( complex(&a,asin(x),0.0) );
	} else {
		beta  = sqrt((x + 1)*(x + 1) + y*y)/2 - sqrt((x - 1)*(x - 1) + y*y)/2;
		alpha = sqrt((x + 1)*(x + 1) + y*y)/2 + sqrt((x - 1)*(x - 1) + y*y)/2;
		push( complex(&a,asin(beta), log(alpha + sqrt(alpha*alpha-1))) );
	}
}

f_acos()
{
struct value a;
register double alpha, beta, x, y;
	(void) pop(&a);
	x = real(&a); y = imag(&a);
	if (y == 0.0) {
		if (fabs(x) > 1.0) {
			undefined = TRUE;
			push(complex(&a,0.0, 0.0));
		} else
			push( complex(&a,acos(x),0.0) );
	} else {
		alpha = sqrt((x + 1)*(x + 1) + y*y)/2 + sqrt((x - 1)*(x - 1) + y*y)/2;
		beta  = sqrt((x + 1)*(x + 1) + y*y)/2 - sqrt((x - 1)*(x - 1) + y*y)/2;
		push( complex(&a,acos(beta), log(alpha + sqrt(alpha*alpha-1))) );
	}
}

f_atan()
{
struct value a;
register double x, y;
	(void) pop(&a);
	x = real(&a); y = imag(&a);
	if (y == 0.0)
		push( complex(&a,atan(x), 0.0) );
	else if (x == 0.0 && fabs(y) == 1.0) {
		undefined = TRUE;
		push(complex(&a,0.0, 0.0));
	} else
		push( complex(&a,atan(2*x/(1-x*x-y*y)),
	    		log((x*x+(y+1)*(y+1))/(x*x+(y-1)*(y-1)))/4) );
}

f_sinh()
{
struct value a;
	(void) pop(&a);
	push( complex(&a,sinh(real(&a))*cos(imag(&a)), cosh(real(&a))*sin(imag(&a))) );
}

f_cosh()
{
struct value a;
	(void) pop(&a);
	push( complex(&a,cosh(real(&a))*cos(imag(&a)), sinh(real(&a))*sin(imag(&a))) );
}

f_tanh()
{
struct value a;
register double den;
	(void) pop(&a);
	den = cosh(2*real(&a)) + cos(2*imag(&a));
	push( complex(&a,sinh(2*real(&a))/den, sin(2*imag(&a))/den) );
}

f_int()
{
struct value a;
	push( integer(&a,(int)real(pop(&a))) );
}


f_abs()
{
struct value a;
	(void) pop(&a);
	switch (a.type) {
		case INT:
			push( integer(&a,abs(a.v.int_val)) );			
			break;
		case CMPLX:
			push( complex(&a,magnitude(&a), 0.0) );
	}
}

f_sgn()
{
struct value a;
	(void) pop(&a);
	switch(a.type) {
		case INT:
			push( integer(&a,(a.v.int_val > 0) ? 1 : 
					(a.v.int_val < 0) ? -1 : 0) );
			break;
		case CMPLX:
			push( integer(&a,(a.v.cmplx_val.real > 0.0) ? 1 : 
					(a.v.cmplx_val.real < 0.0) ? -1 : 0) );
			break;
	}
}


f_sqrt()
{
struct value a;
register double mag, ang;
	(void) pop(&a);
	mag = sqrt(magnitude(&a));
	if (imag(&a) == 0.0 && real(&a) < 0.0)
		push( complex(&a,0.0,mag) );
	else
	{
		if ( (ang = angle(&a)) < 0.0)
			ang += 2*Pi;
		ang /= 2;
		push( complex(&a,mag*cos(ang), mag*sin(ang)) );
	}
}


f_exp()
{
struct value a;
register double mag, ang;
	(void) pop(&a);
	mag = exp(real(&a));
	ang = imag(&a);
	push( complex(&a,mag*cos(ang), mag*sin(ang)) );
}


f_log10()
{
struct value a;
register double l10;;
	(void) pop(&a);
	l10 = log(10.0);	/***** replace with a constant! ******/
	push( complex(&a,log(magnitude(&a))/l10, angle(&a)/l10) );
}


f_log()
{
struct value a;
	(void) pop(&a);
	push( complex(&a,log(magnitude(&a)), angle(&a)) );
}


f_besj0()	/* j0(a) = sin(a)/a */
{
struct value a;
	a = top_of_stack;
	f_sin();
	push(&a);
	f_div();
}


f_besj1()	/* j1(a) = sin(a)/(a**2) - cos(a)/a */
{
struct value a;
	a = top_of_stack;
	f_sin();
	push(&a);
	push(&a);
	f_mult();
	f_div();
	push(&a);
	f_cos();
	push(&a);
	f_div();
	f_minus();
}


f_besy0()	/* y0(a) = -cos(a)/a */
{
struct value a;
	a = top_of_stack;
	f_cos();
	push(&a);
	f_div();
	f_uminus();
}


f_besy1()	/* y1(a) = -cos(a)/(a**2) - sin(a)/a */
{
struct value a;

	a = top_of_stack;
	f_cos();
	push(&a);
	push(&a);
	f_mult();
	f_div();
	push(&a);
	f_sin();
	push(&a);
	f_div();
	f_plus();
	f_uminus();
}


f_floor()
{
struct value a;

	(void) pop(&a);
	switch (a.type) {
		case INT:
			push( integer(&a,(int)floor((double)a.v.int_val)));			
			break;
		case CMPLX:
			push( complex(&a,floor(a.v.cmplx_val.real),
				floor(a.v.cmplx_val.imag)) );
	}
}


f_ceil()
{
struct value a;

	(void) pop(&a);
	switch (a.type) {
		case INT:
			push( integer(&a,(int)ceil((double)a.v.int_val)));			
			break;
		case CMPLX:
			push( complex(&a,ceil(a.v.cmplx_val.real), ceil(a.v.cmplx_val.imag)) );
	}
}

#ifdef GAMMA

f_gamma()
{
extern int signgam;
register double y;
struct value a;

	y = gamma(real(pop(&a)));
	if (y > 88.0) {
		undefined = TRUE;
		push( integer(&a,0) );
	}
	else
		push( complex(&a,signgam * exp(y),0.0) );
}

#endif /* GAMMA */
