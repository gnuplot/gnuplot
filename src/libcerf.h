/*
 * $Id: libcerf.h,v 1.0 2013/07/12 18:11:05 sfeam Exp $
 */
#ifdef HAVE_LIBCERF
void f_cerf __PROTO((union argument *z));
void f_cdawson __PROTO((union argument *z));
void f_faddeeva __PROTO((union argument *z));
void f_voigtp __PROTO((union argument *z));
void f_voigt __PROTO((union argument *z));
void f_erfi __PROTO((union argument *z));
#endif
