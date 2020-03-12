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
 * The cairy() routine (note C naming but Fortran parameter passing)
 * is from my own build of the AMOS routines.
 *
 * Ethan A Merritt - March 2020
 */

#include "eval.h"
#include "stdfn.h"	/* for not_a_number() */

#ifdef HAVE_AMOS

extern void zairy_( double *zr, double *zi, int32_t *id, int32_t *kode,
		   double *air, double *aii, int32_t *nz, int32_t *ierr );
extern void zbiry_( double *zr, double *zi, int32_t *id, int32_t *kode,
		   double *air, double *aii, int32_t *ierr );

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
	Gcomplex(&a, not_a_number(), 0.0);
    } else {
	Gcomplex(&a, bi.real, bi.imag);
    }

    push(&a);
}
#endif	/* HAVE_AMOS */
