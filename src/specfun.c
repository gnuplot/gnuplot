#ifndef lint
static char *RCSid() { return RCSid("$Id: specfun.c,v 1.7.4.1 2000/06/22 12:57:39 broeker Exp $"); }
#endif

/* GNUPLOT - specfun.c */

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

/*
 * AUTHORS
 *
 *   Original Software:
 *   Jos van der Woude, jvdwoude@hut.nl
 *
 */

#include "specfun.h"

#include "internal.h"
/*  #include "setshow.h" */
#include "util.h"

#define ITMAX   200

#ifdef FLT_EPSILON
# define MACHEPS FLT_EPSILON	/* 1.0E-08 */
#else
# define MACHEPS 1.0E-08
#endif

/* AS239 value, e^-88 = 2^-127 */
#define MINEXP  -88.0

#ifdef FLT_MAX
# define OFLOW   FLT_MAX	/* 1.0E+37 */
#else
# define OFLOW   1.0E+37
#endif

/* AS239 value for igamma(a,x>=XBIG) = 1.0 */
#define XBIG    1.0E+08

/*
 * Mathematical constants
 */
#define LNPI 1.14472988584940016
#define LNSQRT2PI 0.9189385332046727
#define PNT68 0.6796875
#define SQRT_TWO 1.41421356237309504880168872420969809	/* JG */

/* Prefer lgamma */
#ifndef GAMMA
# ifdef HAVE_LGAMMA
#  define GAMMA(x) lgamma (x)
# elif defined(HAVE_GAMMA)
#  define GAMMA(x) gamma (x)
# else
#  undef GAMMA
# endif
#endif

#ifndef GAMMA
int signgam = 0;
#else
extern int signgam;		/* this is not always declared in math.h */
#endif

/* Global variables, not visible outside this file */
static long Xm1 = 2147483563L;
static long Xm2 = 2147483399L;
static long Xa1 = 40014L;
static long Xa2 = 40692L;

/* Local function declarations, not visible outside this file */
static double confrac __PROTO((double a, double b, double x));
static double ibeta __PROTO((double a, double b, double x));
static double igamma __PROTO((double a, double x));
static double ranf __PROTO((double init));
static double inverse_normal_func __PROTO((double p));
static double inverse_error_func __PROTO((double p));

#ifndef GAMMA
/* Provide GAMMA function for those who do not already have one */
static double lngamma __PROTO((double z));
static double lgamneg __PROTO((double z));
static double lgampos __PROTO((double z));

/**
 * from statlib, Thu Jan 23 15:02:27 EST 1992 ***
 *
 * This file contains two algorithms for the logarithm of the gamma function.
 * Algorithm AS 245 is the faster (but longer) and gives an accuracy of about
 * 10-12 significant decimal digits except for small regions around X = 1 and
 * X = 2, where the function goes to zero.
 * The second algorithm is not part of the AS algorithms.   It is slower but
 * gives 14 or more significant decimal digits accuracy, except around X = 1
 * and X = 2.   The Lanczos series from which this algorithm is derived is
 * interesting in that it is a convergent series approximation for the gamma
 * function, whereas the familiar series due to De Moivre (and usually wrongly
 * called Stirling's approximation) is only an asymptotic approximation, as
 * is the true and preferable approximation due to Stirling.
 * 
 * Uses Lanczos-type approximation to ln(gamma) for z > 0. Reference: Lanczos,
 * C. 'A precision approximation of the gamma function', J. SIAM Numer.
 * Anal., B, 1, 86-96, 1964. Accuracy: About 14 significant digits except for
 * small regions in the vicinity of 1 and 2.
 * 
 * Programmer: Alan Miller CSIRO Division of Mathematics & Statistics
 * 
 * Latest revision - 17 April 1988
 * 
 * Additions: Translated from fortran to C, code added to handle values z < 0.
 * The global variable signgam contains the sign of the gamma function.
 * 
 * IMPORTANT: The signgam variable contains garbage until AFTER the call to
 * lngamma().
 * 
 * Permission granted to distribute freely for non-commercial purposes only
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 */

/* Local data, not visible outside this file 
static double   a[] =
   {
   0.9999999999995183E+00,
   0.6765203681218835E+03,
   -.1259139216722289E+04,
   0.7713234287757674E+03,
   -.1766150291498386E+03,
   0.1250734324009056E+02,
   -.1385710331296526E+00,
   0.9934937113930748E-05,
   0.1659470187408462E-06,
   };   */

/* from Ray Toy */
static double GPFAR a[] =
{
    .99999999999980993227684700473478296744476168282198,
    676.52036812188509856700919044401903816411251975244084,
    -1259.13921672240287047156078755282840836424300664868028,
    771.32342877765307884865282588943070775227268469602500,
    -176.61502916214059906584551353999392943274507608117860,
    12.50734327868690481445893685327104972970563021816420,
    -.13857109526572011689554706984971501358032683492780,
    .00000998436957801957085956266828104544089848531228,
    .00000015056327351493115583383579667028994545044040,
};

static double
lgamneg(z)
double z;
{
    double tmp;

    /* Use reflection formula, then call lgampos() */
    tmp = sin(z * M_PI);

    if (fabs(tmp) < MACHEPS) {
	tmp = 0.0;
    } else if (tmp < 0.0) {
	tmp = -tmp;
	signgam = -1;
    }
    return LNPI - lgampos(1.0 - z) - log(tmp);

}

static double
lgampos(z)
double z;
{
    double sum;
    double tmp;
    int i;

    sum = a[0];
    for (i = 1, tmp = z; i < 9; i++) {
	sum += a[i] / tmp;
	tmp++;
    }

    return log(sum) + LNSQRT2PI - z - 6.5 + (z - 0.5) * log(z + 6.5);
}

static double
lngamma(z)
double z;
{
    signgam = 1;

    if (z <= 0.0)
	return lgamneg(z);
    else
	return lgampos(z);
}

# define GAMMA(x) lngamma ((x))
#endif /* !GAMMA */

void
f_erf()
{
    struct value a;
    double x;

    x = real(pop(&a));

#ifdef HAVE_ERF
    x = erf(x);
#else
    {
	int fsign;
	fsign = x >= 0 ? 1 : 0;
	x = igamma(0.5, (x) * (x));
	if (x == -1.0) {
	    undefined = TRUE;
	    x = 0.0;
	} else {
	    if (fsign == 0)
		x = -x;
	}
    }
#endif
    push(Gcomplex(&a, x, 0.0));
}

void
f_erfc()
{
    struct value a;
    double x;

    x = real(pop(&a));
#ifdef HAVE_ERFC
    x = erfc(x);
#else
    {
	int fsign;
	fsign = x >= 0 ? 1 : 0;
	x = igamma(0.5, (x) * (x));
	if (x == 1.0) {
	    undefined = TRUE;
	    x = 0.0;
	} else {
	    x = fsign > 0 ? 1.0 - x : 1.0 + x;
	}
    }
#endif
    push(Gcomplex(&a, x, 0.0));
}

void
f_ibeta()
{
    struct value a;
    double x;
    double arg1;
    double arg2;

    x = real(pop(&a));
    arg2 = real(pop(&a));
    arg1 = real(pop(&a));

    x = ibeta(arg1, arg2, x);
    if (x == -1.0) {
	undefined = TRUE;
	push(Ginteger(&a, 0));
    } else
	push(Gcomplex(&a, x, 0.0));
}

void
f_igamma()
{
    struct value a;
    double x;
    double arg1;

    x = real(pop(&a));
    arg1 = real(pop(&a));

    x = igamma(arg1, x);
    if (x == -1.0) {
	undefined = TRUE;
	push(Ginteger(&a, 0));
    } else
	push(Gcomplex(&a, x, 0.0));
}

void
f_gamma()
{
    register double y;
    struct value a;

    y = GAMMA(real(pop(&a)));
    if (y > 88.0) {
	undefined = TRUE;
	push(Ginteger(&a, 0));
    } else
	push(Gcomplex(&a, signgam * gp_exp(y), 0.0));
}

void
f_lgamma()
{
    struct value a;

    push(Gcomplex(&a, GAMMA(real(pop(&a))), 0.0));
}

#ifndef BADRAND

void
f_rand()
{
    struct value a;

    push(Gcomplex(&a, ranf(real(pop(&a))), 0.0));
}

#else /* BADRAND */

/* Use only to observe the effect of a "bad" random number generator. */
void
f_rand()
{
    struct value a;

    static unsigned int y = 0;
    unsigned int maxran = 1000;

    (void) real(pop(&a));
    y = (781 * y + 387) % maxran;

    push(Gcomplex(&a, (double) y / maxran, 0.0));
}

#endif /* BADRAND */

/* ** ibeta.c
 *
 *   DESCRIB   Approximate the incomplete beta function Ix(a, b).
 *
 *                           _
 *                          |(a + b)     /x  (a-1)         (b-1)
 *             Ix(a, b) = -_-------_--- * |  t     * (1 - t)     dt (a,b > 0)
 *                        |(a) * |(b)   /0
 *
 *
 *
 *   CALL      p = ibeta(a, b, x)
 *
 *             double    a    > 0
 *             double    b    > 0
 *             double    x    [0, 1]
 *
 *   WARNING   none
 *
 *   RETURN    double    p    [0, 1]
 *                            -1.0 on error condition
 *
 *   XREF      lngamma()
 *
 *   BUGS      none
 *
 *   REFERENCE The continued fraction expansion as given by
 *             Abramowitz and Stegun (1964) is used.
 *
 * Permission granted to distribute freely for non-commercial purposes only
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 */

static double
ibeta(a, b, x)
double a, b, x;
{
    /* Test for admissibility of arguments */
    if (a <= 0.0 || b <= 0.0)
	return -1.0;
    if (x < 0.0 || x > 1.0)
	return -1.0;;

    /* If x equals 0 or 1, return x as prob */
    if (x == 0.0 || x == 1.0)
	return x;

    /* Swap a, b if necessarry for more efficient evaluation */
    return a < x * (a + b) ? 1.0 - confrac(b, a, 1.0 - x) : confrac(a, b, x);
}

static double
confrac(a, b, x)
double a, b, x;
{
    double Alo = 0.0;
    double Ahi;
    double Aev;
    double Aod;
    double Blo = 1.0;
    double Bhi = 1.0;
    double Bod = 1.0;
    double Bev = 1.0;
    double f;
    double fold;
    double Apb = a + b;
    double d;
    int i;
    int j;

    /* Set up continued fraction expansion evaluation. */
    Ahi = gp_exp(GAMMA(Apb) + a * log(x) + b * log(1.0 - x) -
		 GAMMA(a + 1.0) - GAMMA(b));

    /*
     * Continued fraction loop begins here. Evaluation continues until
     * maximum iterations are exceeded, or convergence achieved.
     */
    for (i = 0, j = 1, f = Ahi; i <= ITMAX; i++, j++) {
	d = a + j + i;
	Aev = -(a + i) * (Apb + i) * x / d / (d - 1.0);
	Aod = j * (b - j) * x / d / (d + 1.0);
	Alo = Bev * Ahi + Aev * Alo;
	Blo = Bev * Bhi + Aev * Blo;
	Ahi = Bod * Alo + Aod * Ahi;
	Bhi = Bod * Blo + Aod * Bhi;

	if (fabs(Bhi) < MACHEPS)
	    Bhi = 0.0;

	if (Bhi != 0.0) {
	    fold = f;
	    f = Ahi / Bhi;
	    if (fabs(f - fold) < fabs(f) * MACHEPS)
		return f;
	}
    }

    return -1.0;
}

/* ** igamma.c
 *
 *   DESCRIB   Approximate the incomplete gamma function P(a, x).
 *
 *                         1     /x  -t   (a-1)
 *             P(a, x) = -_--- * |  e  * t     dt      (a > 0)
 *                       |(a)   /0
 *
 *   CALL      p = igamma(a, x)
 *
 *             double    a    >  0
 *             double    x    >= 0
 *
 *   WARNING   none
 *
 *   RETURN    double    p    [0, 1]
 *                            -1.0 on error condition
 *
 *   XREF      lngamma()
 *
 *   BUGS      Values 0 <= x <= 1 may lead to inaccurate results.
 *
 *   REFERENCE ALGORITHM AS239  APPL. STATIST. (1988) VOL. 37, NO. 3
 *
 * Permission granted to distribute freely for non-commercial purposes only
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 */

/* Global variables, not visible outside this file */
static double pn1, pn2, pn3, pn4, pn5, pn6;

static double
igamma(a, x)
double a, x;
{
    double arg;
    double aa;
    double an;
    double b;
    int i;

    /* Check that we have valid values for a and x */
    if (x < 0.0 || a <= 0.0)
	return -1.0;

    /* Deal with special cases */
    if (x == 0.0)
	return 0.0;
    if (x > XBIG)
	return 1.0;

    /* Check value of factor arg */
    arg = a * log(x) - x - GAMMA(a + 1.0);
    if (arg < MINEXP)
	return -1.0;
    arg = gp_exp(arg);

    /* Choose infinite series or continued fraction. */

    if ((x > 1.0) && (x >= a + 2.0)) {
	/* Use a continued fraction expansion */

	double rn;
	double rnold;

	aa = 1.0 - a;
	b = aa + x + 1.0;
	pn1 = 1.0;
	pn2 = x;
	pn3 = x + 1.0;
	pn4 = x * b;
	rnold = pn3 / pn4;

	for (i = 1; i <= ITMAX; i++) {

	    aa++;
	    b += 2.0;
	    an = aa * (double) i;

	    pn5 = b * pn3 - an * pn1;
	    pn6 = b * pn4 - an * pn2;

	    if (pn6 != 0.0) {

		rn = pn5 / pn6;
		if (fabs(rnold - rn) <= GPMIN(MACHEPS, MACHEPS * rn))
		    return 1.0 - arg * rn * a;

		rnold = rn;
	    }
	    pn1 = pn3;
	    pn2 = pn4;
	    pn3 = pn5;
	    pn4 = pn6;

	    /* Re-scale terms in continued fraction if terms are large */
	    if (fabs(pn5) >= OFLOW) {

		pn1 /= OFLOW;
		pn2 /= OFLOW;
		pn3 /= OFLOW;
		pn4 /= OFLOW;
	    }
	}
    } else {
	/* Use Pearson's series expansion. */

	for (i = 0, aa = a, an = b = 1.0; i <= ITMAX; i++) {

	    aa++;
	    an *= x / aa;
	    b += an;
	    if (an < b * MACHEPS)
		return arg * b;
	}
    }
    return -1.0;
}

/***********************************************************************
     double ranf(double init)
                RANDom number generator as a Function
     Returns a random floating point number from a uniform distribution
     over 0 - 1 (endpoints of this interval are not returned) using a
     large integer generator.
     This is a transcription from Pascal to Fortran of routine
     Uniform_01 from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)

               GeNerate LarGe Integer
     Returns a random integer following a uniform distribution over
     (1, 2147483562) using the generator.
     This is a transcription from Pascal to Fortran of routine
     Random from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
***********************************************************************/
static double
ranf(init)
double init;
{

    long k, z;
    static int firsttime = 1;
    static long s1, s2;

    /* (Re)-Initialize seeds if necessary */
    if (init < 0.0 || firsttime == 1) {
	firsttime = 0;
	s1 = 1234567890L;
	s2 = 1234567890L;
    }
    /* Generate pseudo random integers */
    k = s1 / 53668L;
    s1 = Xa1 * (s1 - k * 53668L) - k * 12211;
    if (s1 < 0)
	s1 += Xm1;
    k = s2 / 52774L;
    s2 = Xa2 * (s2 - k * 52774L) - k * 3791;
    if (s2 < 0)
	s2 += Xm2;
    z = s1 - s2;
    if (z < 1)
	z += (Xm1 - 1);

    /*
     * 4.656613057E-10 is 1/Xm1.  Xm1 is set at the top of this file and is
     * currently 2147483563. If Xm1 changes, change this also.
     */
    return (double) 4.656613057E-10 *z;
}

/* ----------------------------------------------------------------
   Following to specfun.c made by John Grosh (jgrosh@arl.mil)
   on 28 OCT 1992.
   ---------------------------------------------------------------- */

void
f_normal()
{				/* Normal or Gaussian Probability Function */
    struct value a;
    double x;

    /* ref. Abramowitz and Stegun 1964, "Handbook of Mathematical 
       Functions", Applied Mathematics Series, vol 55,
       Chapter 26, page 934, Eqn. 26.2.29 and Jos van der Woude 
       code found above */

    x = real(pop(&a));

    x = 0.5 * SQRT_TWO * x;
#ifdef HAVE_ERF
    x = 0.5 * (1.0 + erf(x));
#else
    {
	int fsign;
	fsign = x >= 0 ? 1 : 0;
	x = igamma(0.5, (x) * (x));
	if (x == 1.0) {
	    undefined = TRUE;
	    x = 0.0;
	} else {
	    if (fsign == 0)
		x = -(x);
	    x = 0.5 * (1.0 + x);
	}
    }
#endif
    push(Gcomplex(&a, x, 0.0));
}

void
f_inverse_normal()
{				/* Inverse normal distribution function */
    struct value a;
    double x;

    x = real(pop(&a));

    if (x <= 0.0 || x >= 1.0) {
	undefined = TRUE;
	push(Gcomplex(&a, 0.0, 0.0));
    } else {
	push(Gcomplex(&a, inverse_normal_func(x), 0.0));
    }
}


void
f_inverse_erf()
{				/* Inverse error function */
    struct value a;
    double x;

    x = real(pop(&a));

    if (fabs(x) >= 1.0) {
	undefined = TRUE;
	push(Gcomplex(&a, 0.0, 0.0));
    } else {
	push(Gcomplex(&a, inverse_error_func(x), 0.0));
    }
}

static double
inverse_normal_func(p)
double p;
{
    /* 
       Source: This routine was derived (using f2c) from the 
       FORTRAN subroutine MDNRIS found in 
       ACM Algorithm 602 obtained from netlib.

       MDNRIS code contains the 1978 Copyright 
       by IMSL, INC. .  Since MDNRIS has been 
       submitted to netlib it may be used with 
       the restriction that it may only be 
       used for noncommercial purposes and that
       IMSL be acknowledged as the copyright-holder
       of the code.
     */

    /* Initialized data */
    static double eps = 1e-10;
    static double g0 = 1.851159e-4;
    static double g1 = -.002028152;
    static double g2 = -.1498384;
    static double g3 = .01078639;
    static double h0 = .09952975;
    static double h1 = .5211733;
    static double h2 = -.06888301;
    static double sqrt2 = 1.414213562373095;

    /* Local variables */
    static double a, w, x;
    static double sd, wi, sn, y;

    /* Note: 0.0 < p < 1.0 */

    /* p too small, compute y directly */
    if (p <= eps) {
	a = p + p;
	w = sqrt(-(double) log(a + (a - a * a)));

	/* use a rational function in 1.0 / w */
	wi = 1.0 / w;
	sn = ((g3 * wi + g2) * wi + g1) * wi;
	sd = ((wi + h2) * wi + h1) * wi + h0;
	y = w + w * (g0 + sn / sd);
	y = -y * sqrt2;
    } else {
	x = 1.0 - (p + p);
	y = inverse_error_func(x);
	y = -sqrt2 * y;
    }
    return (y);
}


static double
inverse_error_func(p)
double p;
{
    /* 
       Source: This routine was derived (using f2c) from the 
       FORTRAN subroutine MERFI found in 
       ACM Algorithm 602 obtained from netlib.

       MDNRIS code contains the 1978 Copyright 
       by IMSL, INC. .  Since MERFI has been 
       submitted to netlib, it may be used with 
       the restriction that it may only be 
       used for noncommercial purposes and that
       IMSL be acknowledged as the copyright-holder
       of the code.
     */



    /* Initialized data */
    static double a1 = -.5751703;
    static double a2 = -1.896513;
    static double a3 = -.05496261;
    static double b0 = -.113773;
    static double b1 = -3.293474;
    static double b2 = -2.374996;
    static double b3 = -1.187515;
    static double c0 = -.1146666;
    static double c1 = -.1314774;
    static double c2 = -.2368201;
    static double c3 = .05073975;
    static double d0 = -44.27977;
    static double d1 = 21.98546;
    static double d2 = -7.586103;
    static double e0 = -.05668422;
    static double e1 = .3937021;
    static double e2 = -.3166501;
    static double e3 = .06208963;
    static double f0 = -6.266786;
    static double f1 = 4.666263;
    static double f2 = -2.962883;
    static double g0 = 1.851159e-4;
    static double g1 = -.002028152;
    static double g2 = -.1498384;
    static double g3 = .01078639;
    static double h0 = .09952975;
    static double h1 = .5211733;
    static double h2 = -.06888301;

    /* Local variables */
    static double a, b, f, w, x, y, z, sigma, z2, sd, wi, sn;

    x = p;

    /* determine sign of x */
    if (x > 0)
	sigma = 1.0;
    else
	sigma = -1.0;

    /* Note: -1.0 < x < 1.0 */

    z = fabs(x);

    /* z between 0.0 and 0.85, approx. f by a 
       rational function in z  */

    if (z <= 0.85) {
	z2 = z * z;
	f = z + z * (b0 + a1 * z2 / (b1 + z2 + a2
				     / (b2 + z2 + a3 / (b3 + z2))));

	/* z greater than 0.85 */
    } else {
	a = 1.0 - z;
	b = z;

	/* reduced argument is in (0.85,1.0), 
	   obtain the transformed variable */

	w = sqrt(-(double) log(a + a * b));

	/* w greater than 4.0, approx. f by a 
	   rational function in 1.0 / w */

	if (w >= 4.0) {
	    wi = 1.0 / w;
	    sn = ((g3 * wi + g2) * wi + g1) * wi;
	    sd = ((wi + h2) * wi + h1) * wi + h0;
	    f = w + w * (g0 + sn / sd);

	    /* w between 2.5 and 4.0, approx. 
	       f by a rational function in w */

	} else if (w < 4.0 && w > 2.5) {
	    sn = ((e3 * w + e2) * w + e1) * w;
	    sd = ((w + f2) * w + f1) * w + f0;
	    f = w + w * (e0 + sn / sd);

	    /* w between 1.13222 and 2.5, approx. f by 
	       a rational function in w */
	} else if (w <= 2.5 && w > 1.13222) {
	    sn = ((c3 * w + c2) * w + c1) * w;
	    sd = ((w + d2) * w + d1) * w + d0;
	    f = w + w * (c0 + sn / sd);
	}
    }
    y = sigma * f;
    return (y);
}
