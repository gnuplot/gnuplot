/*
 * Complex error function (cerf) and related special functions from libcerf.
 * libcerf itself uses the C99 _Complex mechanism for describing complex
 * numbers.  This set of wrapper routines converts back and forth from 
 * gnuplot's own representation of complex numbers.
 * Ethan A Merritt - July 2013
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
 * EXPERIMENTAL
 *	- the libcerf function is not reliable (not available prior to version 1.11
 *	and handles out-of-range input by printing to stderr and returning a bad
 *      value or exiting
 *	- the fall-back approximation is accurate onlyt to 0.02%
 */
void
f_VP_fwhm(union argument *arg)
{
    struct value a;
    double sigma, gamma;
    double fwhm;

    gamma = real(pop(&a));
    sigma = real(pop(&a));
#ifdef HAVE_VPHWHM
    fwhm = 2. * voigt_hwhm(sigma, gamma);
#else
    /* This approximation claims accuracy of only 0.02% */
    {
    double fG = 2. * sigma * sqrt(2.*log(2.));
    double fL = 2. * gamma;
    fwhm = 0.5346 * fL + sqrt( 0.2166*fL*fL + fG*fG);
    }
#endif
    push(Gcomplex(&a, fwhm, 0.0));
}

#endif
