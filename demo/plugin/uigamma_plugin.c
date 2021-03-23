#include "gnuplot_plugin.h"
#include <math.h>

#define MACHEP 1.E-14
#define MAXLOG 708.396418532264106224   /* log(2**1022) */

/*
 * ==================== IGAMC =====================
 * Start of code supporting igamc(a,x)
 * the regularized upper incomplete gamma integral
 *
 * EAM March 2021
 * modified cephes igam.c to be called from inside gnuplot
 * This code is already included in version 5.5 as uigamma(a,x)
 * but may be useful as a plugin for earlier gnuplot versions.
 * The function is exported as Q(a,x)
 * You can load it as a plugin from inside gnuplot
 *
 * gnuplot> import Q(a,x) from "uigamma_plugin"
 * gnuplot> uigamma(a,x) = ((x<1 || x<a) ? 1.0-igamma(a,x) : Q(a,x))
 */
/*							igamc()
 *	Complemented incomplete gamma integral
 *
 * SYNOPSIS:
 *
 * double a, x, y, igamc();
 *
 * y = igamc( a, x );
 *
 * DESCRIPTION:
 *
 * The function is defined by
 *
 *
 *  igamc(a,x)   =   1 - igam(a,x)
 *
 *                            inf.
 *                              -
 *                     1       | |  -t  a-1
 *               =   -----     |   e   t   dt.
 *                    -      | |
 *                   | (a)    -
 *                             x
 *
 *
 * In this implementation both arguments must be positive.
 * The integral is evaluated by either a power series or
 * continued fraction expansion, depending on the relative
 * values of a and x.
 *
 * ACCURACY:
 *
 * Tested at random a, x.
 *                a         x                      Relative error:
 * arithmetic   domain   domain     # trials      peak         rms
 *    IEEE     0.5,100   0,100      200000       1.9e-14     1.7e-15
 *    IEEE     0.01,0.5  0,100      200000       1.4e-13     1.6e-15
 */
/*
Cephes Math Library Release 2.8:  June, 2000
Copyright 1985, 1987, 2000 by Stephen L. Moshier
*/


static double
igamc( double a, double x )
{
    double ans, ax, c, yc, r, t, y, z;
    double pk, pkm1, pkm2, qk, qkm1, qkm2;
    static double big = 4.503599627370496e15;

    if( (x < 0) || ( a <= 0) )
	return( -1.0 );

    /* Ideally we would return 1-P, but this stand-alone
     * bit of code does not provide an implementation of P
     *     return( 1.0 - igamma(a, x) );
     */
    if( (x < 1.0) || (x < a) )
	return NAN;

    ax = a * log(x) - x - lgamma(a);
    if( ax < -MAXLOG )
	{
	/* mtherr( "igamc", MTHERR_UNDERFLOW ); */
	return( 0.0 );
	}
    ax = exp(ax);

    /* continued fraction */
    y = 1.0 - a;
    z = x + y + 1.0;
    c = 0.0;
    pkm2 = 1.0;
    qkm2 = x;
    pkm1 = x + 1.0;
    qkm1 = z * x;
    ans = pkm1/qkm1;

    do {
	c += 1.0;
	y += 1.0;
	z += 2.0;
	yc = y * c;
	pk = pkm1 * z  -  pkm2 * yc;
	qk = qkm1 * z  -  qkm2 * yc;
	if( qk != 0 )
		{
		r = pk/qk;
		t = fabs( (ans - r)/r );
		ans = r;
		}
	else
		t = 1.0;
	pkm2 = pkm1;
	pkm1 = pk;
	qkm2 = qkm1;
	qkm1 = qk;
	if( fabs(pk) > big )
		{
		pkm2 /= big;
		pkm1 /= big;
		qkm2 /= big;
		qkm1 /= big;
		}

    } while( t > MACHEP );

    return( ans * ax );
}
/*
 * End of code supporting igamc(a,x)
 * ==================== IGAMC =====================
 */


DLLEXPORT struct value Q(int nargs, struct value *arg, void *p)
{
  struct value r;
  double a;     /* 1st parameter to uigamma(a,x) */
  double x;     /* 2nd parameter to uigamma(a,x) */

  /* Enforce a match between the number of parameters declared
   * by the gnuplot import command and the number implemented here.
   */
  RETURN_ERROR_IF_WRONG_NARGS(r, nargs, 2);

  /* Sanity check on argument type */
  RETURN_ERROR_IF_NONNUMERIC(r, arg[0]);
  RETURN_ERROR_IF_NONNUMERIC(r, arg[1]);

  a = RVAL(arg[0]);	/* This is the a in uigamma(a,z) */
  x = RVAL(arg[1]);

  r.type = CMPLX;
  r.v.cmplx_val.real = igamc(a,x);
  r.v.cmplx_val.imag = 0.0;

  return r;
}

