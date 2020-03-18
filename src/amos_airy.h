/*
 * prototypes for user-callable functions wrapping routines
 * from the Amos collection of complex special functions
 * Ai(z) Airy function (complex argument)
 * Bi(z) Airy function (complex argument)
 * BesselI(z,nu) modified Bessel function of the first kind I_nu(z)
 * BesselK(z,nu) modified Bessel function of the second kind K_nu(z)
 */
#ifdef HAVE_AMOS
void f_amos_Ai (union argument *x);
void f_amos_Bi (union argument *x);
void f_amos_BesselI (union argument *x);
void f_amos_BesselK (union argument *x);
#endif
