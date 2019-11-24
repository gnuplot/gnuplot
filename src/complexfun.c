/* GNUPLOT - complexfun.c */

/*[
 * Copyright Ethan A Merritt 2019
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
]*/

#include "syscfg.h"
#include "gp_types.h"
#include "eval.h"
#include "stdfn.h"
#include "util.h"	/* for int_error() */
#include <complex.h>	/* C99 _Complex */
#include "complexfun.h"

#ifdef HAVE_COMPLEX_FUNCS
/*
 * Complex Sign function
 * Sign(z) = z/|z| for z non-zero
 */
void
f_Sign(union argument *arg)
{
    struct value result;
    struct value a;
    complex double z;

    pop(&a);	/* Complex argument z */
    if (a.type == INTGR) {
	push(Gcomplex(&result, sgn(a.v.int_val), 0.0));
    } else if (a.type == CMPLX) {
	z = a.v.cmplx_val.real + I*a.v.cmplx_val.imag;
	if (z != 0.0)
	    z = z/cabs(z);
	push(Gcomplex(&result, creal(z), cimag(z)));
    } else
	int_error(NO_CARET, "z must be numeric");
}

#endif /* HAVE_COMPLEX_FUNCS */
