/* GNUPLOT - complexfun.c */

/*[
 * Copyright Ethan A Merritt 2019
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
]*/

#include "syscfg.h"
#include "gp_types.h"
#include "eval.h"
#include "stdfn.h"
#include "util.h"	/* for int_error() */
#include <complex.h>	/* C99 _Complex */
#include "complexfun.h"

#ifdef HAVE_COMPLEX_FUNCS

/*
 * Complex Sign function
 * Sign(z) = z/|z| for z non-zero
 */
void
f_Sign(union argument *arg)
{
    struct value result;
    struct value a;
    complex double z;

    pop(&a);	/* Complex argument z */
    if (a.type == INTGR) {
	push(Gcomplex(&result, sgn(a.v.int_val), 0.0));
    } else if (a.type == CMPLX) {
	z = a.v.cmplx_val.real + I*a.v.cmplx_val.imag;
	if (z != 0.0)
	    z = z/cabs(z);
	push(Gcomplex(&result, creal(z), cimag(z)));
    } else
	int_error(NO_CARET, "z must be numeric");
}


/*
 * Lambert W function for complex numbers
 *
 * W(z) is a multi-valued function with the defining property
 *               
 *     z = W(z) exp(W(z))   for complex z
 *
 * LambertW( z, k ) is the kth branch of W 
 *
 * This implementation guided by C++ code by István Mező <istvanmezo81@gmail.com>
 * See also
 *   R. M. Corless, G. H. Gonnet, D. E. G. Hare, D. J. Jeffrey, and D. E. Knuth, 
 *   On the Lambert W function, Adv. Comput. Math. 5 (1996), no. 4, 329–359.
 *   DOI 10.1007/BF02124750.
 */

/* Internal Prototypes */
complex double lambert_initial(complex double z, int k);
complex double LambertW(complex double z, int k);

void
f_LambertW(union argument *arg)
{
    struct value result;
    struct value a;

    struct cmplx z;	/* gnuplot complex parameter z */
    int k;		/* gnuplot integer parameter k */

    complex double w;	/* C99 _Complex representation */

    pop(&a);		/* Integer argument k */
    if (a.type != INTGR)
	int_error(NO_CARET, "k must be integer");
    k = a.v.int_val;
    pop(&a);		/* Complex argument z */
    if (a.type != CMPLX)
	int_error(NO_CARET, "z must be real or complex");
    z = a.v.cmplx_val;

    w = z.real + I*z.imag;
    w = LambertW( w, k );

    push(Gcomplex(&result, creal(w), cimag(w)));
}

/*
 * First and second derivatives for z * e^z
 * dzexpz( z )  = first derivative of ze^z = e^z + ze^z
 * ddzexpz( z ) = second derivative of ze^z = e^z + e^z + ze^z
 */
#define dzexpz(z)  (cexp(z) + z * cexp(z))
#define ddzexpz(z) (2. * cexp(z) + z * cexp(z))

/*
 * The hard part is choosing a starting point
 * since Halley's method does not have a large radius of convergence
 * EAM: The domain windows in which special case starting points are used
 *      as found in the Mező code produced glitches in my tests.
 *      I adjusted them empirically but I have no justification for
 *      the specific window sizes or thresholds.
 */
complex double
lambert_initial( complex double z, int k )
{
    complex double e = 2.71828182845904523536;
    complex double branch = 2 * M_PI * I * k;
    complex double ip;
    double close;

    double case1_window = 1.2;	/* see note above, was 1.0 */
    double case2_window = 0.9;	/* see note above, was 1.0 */
    double case3_window = 0.5;	/* see note above, was 0.5 */

    /* Initial term of Eq (4.20) from Corless et al */
    ip = clog(z) + branch - clog(clog(z) + branch);

    /* Close to a branch point use (4.22) from Corless et al */
    close = cabs(z - (-1/e));
    if (close <= case1_window) {
	complex double p = csqrt( 2. * (e * z + 1.) );

	if (k == 0) {
	    if (creal(z) > 0 || close < case2_window)
		ip = -1. + p - (1./3.) * p*p + (11./72.) * p*p*p;
	}
#if (0)
	/* This treatment empirically causes more glitches than it removes */
	if (k == 1 && cimag(z) < 0.0) {
	    if (creal(z) > 0 && close < case2_window) {
		ip = -1. - p - (1./3.) * p*p - (11./72.) * p*p*p;
		if (cimag(z) > -0.1)
		    ip += (-43./540.) * p*p*p*p;
	    }
	}
#endif
	if (k == -1 && cimag(z) > 0.) {
	    if (close < case2_window)
		ip = -1. - p - (1./3.) * p*p - (11./72.) * p*p*p;
	}
    }

    /* Padé approximant for W(0,a) */
    if (k == 0 && cabs(z - 0.5) <= case3_window) {
	ip = (0.35173371 * (0.1237166 + 7.061302897 * z)) / (2. + 0.827184 * (1. + 2. * z));
    }

    /* Padé approximant for W(-1,a) */
    if (k == -1 && cabs(z - 0.5) <= case3_window) {
	ip = -(((2.2591588985 + 4.22096*I) * ((-14.073271 - 33.767687754*I) * z
			- (12.7127 - 19.071643*I) * (1. + 2.*z)))
	     / (2. - (17.23103 - 10.629721*I) * (1. + 2.*z)));
    }

    return ip;
}

complex double
LambertW(complex double z, int k)
{
#   define LAMBERT_MAXITER 300
#   define LAMBERT_CONVERGENCE 1.E-13

    int i;		/* iteration variable */
    double residual;	/* target for convergence */

    complex double w;

    /* Special cases */
    if (z == 0) {
	return (k == 0) ? 0.0 : not_a_number();
    }
    if ((k == 0 || k == -1)
    &&  (fabs(creal(z) + exp(-1.0)) < LAMBERT_CONVERGENCE) && cimag(z) == 0) {
	return -1.0;
    }
    if ((k == 0) && (fabs(creal(z) - exp(1.0)) < LAMBERT_CONVERGENCE) && cimag(z) == 0) {
	return 1.0;
    }

    /* Halley's method requires a good starting point */
    w = lambert_initial(z, k);

    for (i = 0; i < LAMBERT_MAXITER; i++) {
	complex double wprev = w;
	complex double delta = w * cexp(w) - z;

	w -=    2. * (delta * dzexpz(w))
	     / (2. * dzexpz(w) * dzexpz(w) - delta * ddzexpz(w));

	residual = cabs(w - wprev);
	if (residual < LAMBERT_CONVERGENCE)
	    break;
    }
    if (i >= LAMBERT_MAXITER) {
	char message[1024];
	snprintf(message, 1023, "LambertW( {%g, %g}, %d) converged only to %g",
		creal(z), cimag(z), k, residual);
	int_warn(NO_CARET, message);
    }

    return w;
}

#undef dzexpz
#undef ddzexpz


/*
 * lnGamma(z) computes the natural logarithm of the Gamma function
 * valid over the entire complex plane using a Lanczos approximation.
 * The imaginary component of the result is adjusted to describe a
 * continuous surface everywhere except the negative real axis.
 *
 * This implementation copyright 2019 Ethan A Merritt
 *
 * references
 * C. Lanczos, SIAM JNA  1, 1964. pp. 86-96.
 * J. Spouge,  SIAM JNA 31, 1994. pp. 931.
 * W. Press et al, "Numerical Recipes" Section 6.1.
 */

/* Internal Prototypes */
static complex double lnGamma( complex double z );

void
f_lnGamma(union argument *arg)
{
    struct value result;
    struct value a;

    struct cmplx z;	/* gnuplot complex parameter z */
    complex double w;	/* C99 _Complex representation of z */

    pop(&a);
    if (a.type != CMPLX)
	int_error(NO_CARET, "z must be real or complex");
    z = a.v.cmplx_val;

    /* Negative integers are pole points */
    if ( z.real < 0 && fabs(z.imag) < FLT_EPSILON
    &&   fabs(z.real - round(z.real)) < FLT_EPSILON ) {
	push(Gcomplex(&result, VERYLARGE, 0.0));
	return;
    }

    /* The Lancosz approximation is valid on the half-plane with Real(z) > 0.
     * To deal with z for which Real(z) < 0 we use the equivalence
     *     Gamma(1-z) = (pi*z) / ( Gamma(1+z) * sin(pi*z) )
     * to reframe the request in terms of a "reflected" z.
     */
    if (z.real < 0.5) {
	complex double lnpi  = 1.14472988584940017414342735;
	complex double t1;
	double treal, timag;

	w = (1.0 - z.real) + I*(-z.imag);
	w = lnGamma(w);
	t1 = clog(csin( M_PI*z.real + I*M_PI*z.imag ));
	treal = lnpi - creal(w) - creal(t1);
	timag = -cimag(w) - cimag(t1);
	/* Shift result by 2pi to maintain a continuous surface
	 * other than the discontinuity at the negative real axis
	 */
	timag += sgn(z.imag) * 2 * M_PI * floor((z.real+0.5)/2.);
	push(Gcomplex(&result, treal, timag));

    } else {
	w = z.real + I*z.imag;
	w = lnGamma(w);
	push(Gcomplex(&result, creal(w), cimag(w)));
    }
}

static complex double
lnGamma( complex double z )
{
    static double coef[15] = {
        0.99999999999999709182,
       57.156235665862923517,    -59.597960355475491248,
       14.136097974741747174,     -0.49191381609762019978,
        0.33994649984811888699e-4, 0.46523628927048575665e-4,
       -0.98374475304879564677e-4, 0.15808870322491248884e-3,
       -0.21026444172410488319e-3, 0.21743961811521264320e-3,
       -0.16431810653676389022e-3, 0.84418223983852743293e-4,
       -0.26190838401581408670e-4, 0.36899182659531622704e-5
    };
    complex double sqrt2pi = 2.5066282746310005;
    complex double g = 671./128.;
    complex double sum, temp;

    complex double f;
    int k;

    /* 15 term Lanczos approximation */
    sum = coef[0];
    for (k=1; k<15; k++)
	sum = sum + coef[k] / (z + k);

    if (z == 1.0 || z == 2.0)
	return 0.0;

    /* Numerical Recipes formulation */
    temp = z + g;
    temp = (z + 0.5) * clog(temp) - temp;
    f = temp + clog( sqrt2pi * sum/z );

    return f;
}

#endif /* HAVE_COMPLEX_FUNCS */
