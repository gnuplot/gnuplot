#ifndef lint
static char *RCSid = "$Id: specfun.c,v 1.8.2.4 2002/01/31 19:23:21 lhecking Exp $";
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

#include "plot.h"
#include "fnproto.h"


extern struct value stack[STACK_DEPTH];
extern int s_p;
extern double zero;

#define ITMAX   200

#ifdef FLT_EPSILON
# define MACHEPS FLT_EPSILON	/* 1.0E-08 */
#else
# define MACHEPS 1.0E-08
#endif

/* AS239 value, e^-88 = 2^-127 */
#define MINEXP  -88.0

#ifdef FLT_MAX
# define OFLOW   FLT_MAX		/* 1.0E+37 */
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
#ifdef PI
# undef PI
#endif
#define PI 3.14159265358979323846
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

static double lngamma(z)
double z;
{
    fprintf(stderr, "lngamma() removed due to license issues\n");
    return 0;
}

# define GAMMA(x) lngamma ((x))
#endif /* !GAMMA */

void f_erf()
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
	x = igamma(0.5, (x)*(x));
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

void f_erfc()
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
	x = igamma(0.5, (x)*(x));
	if (x == 1.0) {
	    undefined = TRUE;
	    x = 0.0;
	} else { 
	    x = fsign > 0 ? 1.0 - x : 1.0 + x ;
	}
    }
#endif
    push(Gcomplex(&a, x, 0.0));
}

void f_ibeta()
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

void f_igamma()
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

void f_gamma()
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

void f_lgamma()
{
    struct value a;

    push(Gcomplex(&a, GAMMA(real(pop(&a))), 0.0));
}

#ifndef BADRAND

void f_rand()
{
    struct value a;

    push(Gcomplex(&a, ranf(real(pop(&a))), 0.0));
}

#else /* BADRAND */

/* Use only to observe the effect of a "bad" random number generator. */
void f_rand()
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
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 *
 * Note: this function was translated from the Public Domain Fortran
 *       version available from http://lib.stat.cmu.edu/apstat/xxx
 *
 */

static double ibeta(a, b, x)
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

static double confrac(a, b, x)
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
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 *
 * Note: this function was translated from the Public Domain Fortran
 *       version available from http://lib.stat.cmu.edu/apstat/239
 *
 */

/* Global variables, not visible outside this file */
static double pn1, pn2, pn3, pn4, pn5, pn6;

static double igamma(a, x)
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
static double ranf(init)
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

void f_normal()
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
	x = igamma(0.5, (x)*(x));
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

void f_inverse_normal()
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


void f_inverse_erf()
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

static double inverse_normal_func(p)
double p;
{
    fprintf(stderr, "inverse_normal_func() removed due to license issues\n");
    return 0;
}

static double inverse_error_func(p)
double p;
{
    fprintf(stderr, "inverse_error_func() removed due to license issues\n");
    return 0;
}
