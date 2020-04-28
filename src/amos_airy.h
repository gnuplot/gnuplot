/*
 * prototypes for user-callable functions wrapping routines
 * from the Amos collection of complex special functions
 * Ai(z) Airy function (complex argument)
 * Bi(z) Airy function (complex argument)
 * BesselJ(z,nu) Bessel function of the first kind J_nu(z)
 * BesselY(z,nu) Bessel function of the second kind Y_nu(z)
 * BesselI(z,nu) modified Bessel function of the first kind I_nu(z)
 * BesselK(z,nu) modified Bessel function of the second kind K_nu(z)
 */
#ifdef HAVE_AMOS
void f_amos_Ai (union argument *x);
void f_amos_Bi (union argument *x);
void f_amos_BesselI (union argument *x);
void f_amos_BesselJ (union argument *x);
void f_amos_BesselK (union argument *x);
void f_amos_BesselY (union argument *x);
void f_Hankel1 (union argument *x);
void f_Hankel2 (union argument *x);
#endif

/* CEXINT is in libamos but not in libopenspecfun */
void f_amos_cexint (union argument *x);
