/*
 * Complex error function (cerf) and related special functions from libcerf.
 * libcerf itself uses the C99 _Complex mechanism for describing complex
 * numbers.  This set of wrapper routines converts back and forth from
 * gnuplot's own representation of complex numbers.
 * Ethan A Merritt - July 2013
 * Private implementations of special functions related to functions in libcerf
 *	VP_fwhm - full width at half max for the Voigt profile
 *	FresnelC - Fresnel integral cosine (real) term
 *	FresnelS - Fresnel integral sine (imaginary) term
 * Ethan A Merritt - April 2020
 */

#include "gp_types.h"

#ifdef HAVE_LIBCERF
#include <complex.h>	/* C99 _Complex */
#include <cerf.h>	/* libcerf library header */

#include "eval.h"
#include "stdfn.h"	/* for not_a_number */
#include "util.h"	/* for int_error() */
#include "libcerf.h"	/* our own prototypes */

/* The libcerf complex error function
 *     cerf(z) = 2/sqrt(pi) * int[0,z] exp(-t^2) dt
 */
void
f_cerf(union argument *arg)
{
    struct value a;
    complex double z;

    pop(&a);
    z = real(&a) + I * imag(&a);	/* Convert gnuplot complex to C99 complex */
    z = cerf(z);			/* libcerf complex -> complex function */
    push(Gcomplex(&a, creal(z), cimag(z)));
}

/* The libcerf cdawson function returns Dawson's integral
 *      cdawson(z) = exp(-z^2) int[0,z] exp(t^2) dt
 *                 = sqrt(pi)/2 * exp(-z^2) * erfi(z)
 * for complex z.
 */
void
f_cdawson(union argument *arg)
{
    struct value a;
    complex double z;

    pop(&a);
    z = real(&a) + I * imag(&a);	/* Convert gnuplot complex to C99 complex */
    z = cdawson(z);			/* libcerf complex -> complex function */
    push(Gcomplex(&a, creal(z), cimag(z)));
}

/* The libcerf routine w_of_z returns the Faddeeva rescaled complex error function
 *     w(z) = exp(-z^2) * erfc(-i*z)
 * This corresponds to Abramowitz & Stegun Eqs. 7.1.3 and 7.1.4
 * This is also known as the plasma dispersion function.
 */
void
f_faddeeva(union argument *arg)
{
    struct value a;
    complex double z;

    pop(&a);
    z = real(&a) + I * imag(&a);	/* Convert gnuplot complex to C99 complex */
    z = w_of_z(z);			/* libcerf complex -> complex function */
    push(Gcomplex(&a, creal(z), cimag(z)));
}

/* The libcerf voigt(z, sigma, gamma) function returns the Voigt profile
 * corresponding to the convolution
 *     voigt(x,sigma,gamma) = integral G(t,sigma) L(x-t,gamma) dt
 * of Gaussian
 *     G(x,sigma) = 1/sqrt(2*pi)/|sigma| * exp(-x^2/2/sigma^2)
 * with Lorentzian
 *     L(x,gamma) = |gamma| / pi / ( x^2 + gamma^2 )
 * over the integral from -infinity to +infinity.
 */
void
f_voigtp(union argument *arg)
{
    struct value a;
    double z, sigma, gamma;

    gamma = real(pop(&a));
    sigma = real(pop(&a));
    z = real(pop(&a));
    z = voigt(z, sigma, gamma);		/* libcerf double -> double function */
    push(Gcomplex(&a, z, 0.0));
}
/* The libcerf routine re_w_of_z( double x, double y )
 * is equivalent to the previously implemented gnuplot routine voigt(x,y).
 * Use it in preference to the previous one.
 */
void
f_voigt(union argument *arg)
{
    struct value a;
    double x, y, w;

    y = real(pop(&a));
    x = real(pop(&a));
    w = re_w_of_z(x, y);
    push(Gcomplex(&a, w, 0.0));
}

/* erfi(z) = -i * erf(iz)
 */
void
f_erfi(union argument *arg)
{
    struct value a;
    double z;

    z = real(pop(&a));
    push(Gcomplex(&a, erfi(z), 0.0));
}

/* Full width at half maximum of the Voigt profile
 * VP_fwhm( sigma, gamma )
 */
void
f_VP_fwhm(union argument *arg)
{
    struct value par;
    double sigma, gamma;
    double HM, fwhm;
    double fG, fL;
    double a, b, c;		/* 3 points used by regula falsi */
    double del_a, del_b, del_c;
    int k;
    int side = 0;

    gamma = fabs(real(pop(&par)));
    sigma = fabs(real(pop(&par)));
    HM = voigt(0.0, sigma, gamma) / 2.0;

    /* This approximation claims accuracy of 0.02%
     * Olivero & Longbothum [1977]
     * Journal of Quantitative Spectroscopy and Radiative Transfer. 17:233
     */
    fG = 2. * sigma * sqrt(2.*log(2.));
    fL = 2. * gamma;
    fwhm = 0.5346 * fL + sqrt( 0.2166*fL*fL + fG*fG);

    /* Choose initial points a,b that bracket the expected root */
    a = fwhm/2. * 0.995;
    b = fwhm/2. * 1.005;
    del_a = voigt(a, sigma, gamma) - HM;
    del_b = voigt(b, sigma, gamma) - HM;

    /* Iteratation using regula falsi (Illinois variant).
     * Empirically, this takes <5 iterations to converge to FLT_EPSILON
     * and <10 iterations to converge to DBL_EPSILON.
     */
    for (k=0; k<100; k++) {
	c = (b*del_a - a*del_b) / (del_a - del_b);
	if (fabs(b-a) < 2. * DBL_EPSILON * fabs(b+a))
	    break;
	del_c = voigt(c, sigma, gamma) - HM;

	if (del_b * del_c > 0) {
	    b = c; del_b = del_c;
	    if (side < 0)
		del_a /= 2.;
	    side = -1;
	} else if (del_a * del_c > 0) {
	    a = c; del_a = del_c;
	    if (side > 0)
		del_b /= 2.;
	    side = 1;
	} else {
	    break;
	}
    }
    fwhm = 2.*c;

    /* I have never seen convergence worse than k = 15 */
    if (k > 50)
	fwhm = not_a_number();

    push(Gcomplex(&par, fwhm, 0.0));
}

/* Fresnel integrals
 *              x
 *     C(x) =  ∫ cos(pi/2 * t^2) dt
 *              x
 *     S(x) =  ∫ sin(pi/2 * t^2) dt
 *
 * calculated from the relationship
 *
 *     C(x) + iS(x) = (1+i)/2 erf(z) where z = √π/2 (1-i) x
 */
void
f_FresnelC(union argument *arg)
{
    struct value a;
    complex double z;
    double x = real(pop(&a));
    static double sqrt_pi_2 = 0.886226925452758;

    z = sqrt_pi_2 * (x - I*x);
    z = (1. + I)/2. * cerf(z);
    push(Gcomplex(&a, creal(z), 0.0));
}

void
f_FresnelS(union argument *arg)
{
    struct value a;
    complex double z;
    double x = real(pop(&a));
    static double sqrt_pi_2 = 0.886226925452758;

    z = sqrt_pi_2 * (x - I*x);
    z = (1. + I)/2. * cerf(z);
    push(Gcomplex(&a, cimag(z), 0.0));
}

#endif
