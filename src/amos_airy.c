/*
 * Bessel functions and related special functions.
 * The routines in this file are wrappers for implementations in an
 * external library detected at configuration/build time.
 *	AMOS - A Portable Package for Bessel Functions of a Complex Argument
 *	       and Nonnegative Order
 *             D. E. Amos, Sandia National Laboratories, SAND85-1018 (1985)
 * The amos routines have been packaged by various projects in slightly 
 * different ways.
 * The zairy_() routine (note Fortran naming and parameter passing conventions)
 * is found, for example, in libopenspecfun.  We use it to calculate Ai(z).
 * Similarly zbiry_() is used to calcualte Bi(z).
 *
 * The cexint_() routine is also from the AMOS collection, but is not part
 * of the Bessel function subset and not included in libopenspecfun.
 * I could not find source for a version of the source that splits the
 * real and imaginary parts of the arguments as with the zxxxx.f Bessel
 * routines; this one accepts and returns a CMPLX argument rather than separate
 * real and imaginary parts.
 *             D. E. Amos, ALGORITHM 683  A Portable FORTRAN Subroutine
 *             for Exponential Integrals of a Complex Argument
 *             ACM Trans. Math. Software 16:178-182 (1990)
 *
 * IERR codes from libamos Bessel functions
 *	0	no error
 *	1	input error, no computation
 *	2	overflow predicted from input value
 *	3	overflow from computation
 *	4	input value out of range, no computation
 *	5	other error
 * IERR codes from cexint
 *	2	underflow
 *	4-5	argument reduction caused loss of significance
 *	6	failed to converge
 *
 * Ethan A Merritt - March 2020
 */

#include "eval.h"
#include "stdfn.h"	/* for not_a_number() */

#ifdef HAVE_AMOS

extern void zairy_( double *zr, double *zi, int32_t *id, int32_t *kode,
		    double *air, double *aii, int32_t *underflow, int32_t *ierr );
extern void zbiry_( double *zr, double *zi, int32_t *id, int32_t *kode,
		    double *air, double *aii, int32_t *ierr );
extern void zbesh_( double *zr, double *zi, double *nu, int32_t *kode,
		    int32_t *kind, int32_t *length,
		    double *zbr, double *zbi, int32_t *underflow, int32_t *ierr);
extern void zbesi_( double *zr, double *zi, double *nu, int32_t *kode, int32_t *length,
		    double *zbr, double *zbi, int32_t *underflow, int32_t *ierr);
extern void zbesj_( double *zr, double *zi, double *nu, int32_t *kode, int32_t *length,
		    double *zbr, double *zbi, int32_t *underflow, int32_t *ierr);
extern void zbesk_( double *zr, double *zi, double *nu, int32_t *kode, int32_t *length,
		    double *zbr, double *zbi, int32_t *underflow, int32_t *ierr);
extern void zbesy_( double *zr, double *zi, double *nu, int32_t *kode, int32_t *length,
		    double *zbr, double *zbi, int32_t *underflow,
		    double *wr, double *wi, int32_t *ierr);

#if defined(HAVE_CEXINT) && defined(HAVE_COMPLEX_H)
#include <complex.h>
#include <util.h>	/* for int_error() */

extern void cexint_( double complex *z, int32_t *norder, int32_t *kode,
		     double *tol, int32_t *length,
		     double complex *cy, int32_t *ierr);
#endif

void
f_amos_Ai(union argument *arg)
{
    struct value a;
    struct cmplx z, ai;
    int32_t id, kode, underflow, ierr;

    (void) arg;                        /* avoid -Wunused warning */
    pop(&a);

    id = 0;	/* 0 = Ai  1 = delAi/delZ */
    kode = 1;	/* 1 = unscaled   2 = scaled */

    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }

    /* Fortran calling conventions! */
    zairy_( &z.real, &z.imag, &id, &kode, &ai.real, &ai.imag, &underflow, &ierr );

    if (underflow != 0 || ierr != 0) {
	FPRINTF((stderr,"zairy( {%.3f, %.3f} ): underflow = %d   ierr = %d\n",
		z.real, z.imag, underflow, ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, ai.real, ai.imag);
    }

    push(&a);
}

void
f_amos_Bi(union argument *arg)
{
    struct value a;
    struct cmplx z, bi;
    int32_t id, kode, ierr;

    (void) arg;                        /* avoid -Wunused warning */
    pop(&a);

    id = 0;	/* 0 = Bi  1 = delBi/delZ */
    kode = 1;	/* 1 = unscaled   2 = scaled */

    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }

    /* Fortran calling conventions! */
    zbiry_( &z.real, &z.imag, &id, &kode, &bi.real, &bi.imag, &ierr );

    if (ierr != 0) {
	FPRINTF((stderr,"zbiry( {%.3f, %.3f} ): ierr = %d\n",
		z.real, z.imag, ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, bi.real, bi.imag);
    }

    push(&a);
}

/*
 * Modified Bessel function of the second kind K_nu(z)
 *	val = BesselK( nu, z )
 * The underlying libamos routine besk fills in an array of values
 * val[j] corresponding to a sequence functions BesselK( nu+j, z )
 * but we ask for only the j=0 case.
 */
#define NJ 1
void
f_amos_BesselK(union argument *arg)
{
    struct value a;
    struct cmplx z;
    struct cmplx Bk[NJ];
    double nu;
    int32_t kode, length, underflow, ierr;

    (void) arg;                        /* avoid -Wunused warning */

    /* ... unpack arguments ... */
    pop(&a);
    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }
    nu = real(pop(&a));


    kode = 1;		/* 1 = unscaled   2 = scaled */
    length = NJ;	/* number of members in the returned sequence of functions */

    /* Fortran calling conventions! */
    zbesk_( &z.real, &z.imag, &nu, &kode, &length,
	    &Bk[0].real, &Bk[0].imag, &underflow, &ierr );

    if (ierr != 0) {
	FPRINTF((stderr,"zbesk( {%.3f, %.3f} ): ierr = %d\n",
		z.real, z.imag, ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, Bk[0].real, Bk[0].imag);
    }

    push(&a);
}


/*
 * Hankel functions of the first and second kinds
 *	val = Hankel1( nu, z ) = Jnu(z) + iYnu(z)
 *	val = Hankel2( nu, z ) = Jnu(z) - iYnu(z)
 * The underlying libamos routine besi fills in an array of values
 * val[j] corresponding to a sequence functions Hankel( kind, nu+j, z )
 * but we ask for only the j=0 case.
 */
void
f_amos_Hankel(int k, union argument *arg)
{
    struct value a;
    struct cmplx z;
    struct cmplx H[NJ];
    double nu;
    int32_t kode, kind, length, underflow, ierr;

    (void) arg;                        /* avoid -Wunused warning */

    /* ... unpack arguments ... */
    pop(&a);
    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }
    nu = real(pop(&a));


    kode = 1;		/* 1 = unscaled   2 = scaled */
    kind = k;		/* 1 = first kind, 2 = second kind */
    length = NJ;	/* number of members in the returned sequence of functions */

    /* Fortran calling conventions! */
    zbesh_( &z.real, &z.imag, &nu, &kode, &kind, &length,
	    &H[0].real, &H[0].imag, &underflow, &ierr );

    if (ierr != 0) {
	FPRINTF((stderr,"zbesh( {%.3f, %.3f} ): ierr = %d\n",
		z.real, z.imag, ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, H[0].real, H[0].imag);
    }

    push(&a);
}

void
f_Hankel1(union argument *arg) { f_amos_Hankel( 1, arg ); }

void
f_Hankel2(union argument *arg) { f_amos_Hankel( 2, arg ); }



/*
 * Modified Bessel function of the first kind I_nu(z)
 * with complex argument z.
 *	val = BesselI( nu, z )
 * The underlying libamos routine besi fills in an array of values
 * val[j] corresponding to a sequence functions BesselI( nu+j, z )
 * but we ask for only the j=0 case.
 */
void
f_amos_BesselI(union argument *arg)
{
    struct value a;
    struct cmplx z;
    struct cmplx Bi[NJ];
    double nu;
    int32_t kode, length, underflow, ierr;

    (void) arg;                        /* avoid -Wunused warning */

    /* ... unpack arguments ... */
    pop(&a);
    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }
    nu = real(pop(&a));


    kode = 1;		/* 1 = unscaled   2 = scaled */
    length = NJ;	/* number of members in the returned sequence of functions */

    /* Fortran calling conventions! */
    zbesi_( &z.real, &z.imag, &nu, &kode, &length,
	    &Bi[0].real, &Bi[0].imag, &underflow, &ierr );

    if (ierr != 0) {
	FPRINTF((stderr,"zbesi( {%.3f, %.3f} ): ierr = %d\n",
		z.real, z.imag, ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, Bi[0].real, Bi[0].imag);
    }

    push(&a);
}


/*
 * Modified Bessel function of the first kind J_nu(z)
 * with complex argument z.
 *	val = BesselJ( nu, z )
 * The underlying libamos routine besi fills in an array of values
 * val[j] corresponding to a sequence functions BesselJ( nu+j, z )
 * but we ask for only the j=0 case.
 */
void
f_amos_BesselJ(union argument *arg)
{
    struct value a;
    struct cmplx z;
    struct cmplx Bj[NJ];
    double nu;
    int32_t kode, length, underflow, ierr;

    (void) arg;                        /* avoid -Wunused warning */

    /* ... unpack arguments ... */
    pop(&a);
    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }
    nu = real(pop(&a));


    kode = 1;		/* 1 = unscaled   2 = scaled */
    length = NJ;	/* number of members in the returned sequence of functions */

    /* Fortran calling conventions! */
    zbesj_( &z.real, &z.imag, &nu, &kode, &length,
	    &Bj[0].real, &Bj[0].imag, &underflow, &ierr );

    if (ierr != 0) {
	FPRINTF((stderr,"zbesj( {%.3f, %.3f} ): ierr = %d\n",
		z.real, z.imag, ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, Bj[0].real, Bj[0].imag);
    }

    push(&a);
}


/*
 * Modified Bessel function of the second kind Y_nu(z)
 * with complex argument z.
 *	val = BesselY( nu, z )
 * The underlying libamos routine besi fills in an array of values
 * val[j] corresponding to a sequence functions BesselJ( nu+j, z )
 * but we ask for only the j=0 case.
 */
void
f_amos_BesselY(union argument *arg)
{
    struct value a;
    struct cmplx z;
    struct cmplx By[NJ];
    double WorkR[NJ], WorkI[NJ];	/* Scratch space for zbesy_ */
    double nu;
    int32_t kode, length, underflow, ierr;

    (void) arg;                        /* avoid -Wunused warning */

    /* ... unpack arguments ... */
    pop(&a);
    if (a.type == INTGR) {
	z.real = a.v.int_val;
	z.imag = 0;
    } else {
	z.real = a.v.cmplx_val.real;
	z.imag = a.v.cmplx_val.imag;
    }
    nu = real(pop(&a));


    kode = 1;		/* 1 = unscaled   2 = scaled */
    length = NJ;	/* number of members in the returned sequence of functions */

    /* Fortran calling conventions! */
    zbesy_( &z.real, &z.imag, &nu, &kode, &length,
	    &By[0].real, &By[0].imag, &underflow,
	    &WorkR[0],  &WorkI[0], &ierr );

    if (ierr != 0) {
	fprintf(stderr,"zbesy( {%.3f, %.3f} ): ierr = %d\n",
		z.real, z.imag, ierr);
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, By[0].real, By[0].imag);
    }

    push(&a);
}

#if defined(HAVE_CEXINT) && defined(HAVE_COMPLEX_H)

void
f_amos_cexint(union argument *arg)
{
    struct value a;
    double complex z, cy;
    double tolerance;
    int32_t norder, kode, length, ierr;

    (void) arg;		/* avoid -Wunused warning */
    pop(&a);		/* complex z */

    kode = 1;		/* 1 = unscaled   2 = scaled */
    length = NJ;	/* number of members in the returned sequence of functions */
    tolerance = 1.e-8;	/* FIXME: not sure what is reasonable to ask for */

    if (a.type == INTGR) {
	z = (double)a.v.int_val;
    } else {
	z = a.v.cmplx_val.real + I * a.v.cmplx_val.imag;
    }
    if (carg(z) < -M_PI || carg(z) > M_PI)
	int_error(NO_CARET, "cexint requires arg(z) in [-pi:pi]");

    pop(&a);		/* integer norder */
    if (a.type == INTGR)
	norder = a.v.int_val;
    if (a.type != INTGR || norder < 0)
	int_error(NO_CARET, "cexint requires integer n >= 0");

    /* Special case for n = 0.
     * E0(z) = exp(-z)/z by definition, so just return that
     */
    if (norder == 0) {
	cy = cexp(-z)/z;
	push(Gcomplex(&a, creal(cy), cimag(cy)));
	return;
    }

    /* Fortran calling conventions! */
    cexint_( &z, &norder, &kode, &tolerance, &length, &cy, &ierr );

    /* ierr == 2 means underflow, in which case cy has already been set to 0 */
    if (ierr == 1 || ierr > 2) {
	FPRINTF((stderr, "f_amos_cexint( %d, {%g, %g} ): ierr = %d\n",
		norder, creal(z), cimag(z), ierr));
	undefined = TRUE;
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, creal(cy), cimag(cy));
    }

    push(&a);
}

#endif


#undef NJ

#endif	/* HAVE_AMOS */
