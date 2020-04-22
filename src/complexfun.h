/*
 * Prototypes for complex functions
 */
#ifdef HAVE_COMPLEX_FUNCS

void f_Igamma(union argument *arg);
void f_LambertW(union argument *arg);
void f_lnGamma(union argument *arg);
void f_Sign(union argument *arg);

double chisq_cdf(int dof, double chisqr);
#endif
